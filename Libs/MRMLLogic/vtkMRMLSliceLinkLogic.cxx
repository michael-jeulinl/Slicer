/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLSliceLinkLogic.cxx,v $
  Date:      $Date$
  Version:   $Revision$

=========================================================================auto=*/

// MRMLLogic includes
#include "vtkMRMLSliceLinkLogic.h"
#include "vtkMRMLSliceLogic.h"
#include "vtkMRMLApplicationLogic.h"

// MRML includes
#include <vtkEventBroker.h>
#include <vtkMRMLSliceCompositeNode.h>
#include <vtkMRMLSliceNode.h>

// VTK includes
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>

// STD includes
#include <cassert>


//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkMRMLSliceLinkLogic, "$Revision$");
vtkStandardNewMacro(vtkMRMLSliceLinkLogic);

//----------------------------------------------------------------------------
vtkMRMLSliceLinkLogic::vtkMRMLSliceLinkLogic()
{
  this->BroadcastingEvents = 0;
}

//----------------------------------------------------------------------------
vtkMRMLSliceLinkLogic::~vtkMRMLSliceLinkLogic()
{
}

//----------------------------------------------------------------------------
void vtkMRMLSliceLinkLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  // List of events the slice logics should listen
  vtkSmartPointer<vtkIntArray> events = vtkSmartPointer<vtkIntArray>::New();
  events->InsertNextValue(vtkMRMLScene::NewSceneEvent);
  events->InsertNextValue(vtkMRMLScene::SceneClosedEvent);
  events->InsertNextValue(vtkMRMLScene::SceneAboutToBeClosedEvent);
  events->InsertNextValue(vtkMRMLScene::SceneRestoredEvent);
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);

  this->SetAndObserveMRMLSceneEventsInternal(newScene, events);

  this->ProcessLogicEvents();
  this->ProcessMRMLEvents(newScene, vtkCommand::ModifiedEvent, 0);
}

//----------------------------------------------------------------------------
void vtkMRMLSliceLinkLogic::ProcessMRMLEvents(vtkObject * caller, 
                                            unsigned long event, 
                                            void * callData )
{
  // SliceLinkLogic needs to observe EVERY SliceNode and
  // SliceCompositeNode in the scene.
  if (vtkMRMLScene::SafeDownCast(caller) == this->GetMRMLScene())
    {
    if (event == vtkMRMLScene::NodeAddedEvent 
        || event == vtkMRMLScene::NodeRemovedEvent)
      {
      vtkMRMLNode *node =  reinterpret_cast<vtkMRMLNode*> (callData);
      if (!node)
        {
        return;
        }
      // Return if different from SliceCompositeNode or SliceNode 
      if (node->IsA("vtkMRMLSliceCompositeNode") 
          || node->IsA("vtkMRMLSliceNode"))
        {
        vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(node);
        if (event == vtkMRMLScene::NodeAddedEvent)
          {
          vtkEventBroker::GetInstance()->AddObservation(node, vtkCommand::ModifiedEvent, this, this->GetMRMLCallbackCommand());

          // If sliceNode we insert in our map the current status of the node
          if (sliceNode && this->SliceNodeStatus.count(sliceNode->GetID()) < 1)
            {
            SliceNodeInfos sliceNodeInfos = {{0,0,0},0};
                                             //sliceNode->GetInteracting()};
            this->SliceNodeStatus.insert(std::pair<std::string, SliceNodeInfos>
              (sliceNode->GetID(), sliceNodeInfos));
            }
          }
        else if (event == vtkMRMLScene::NodeRemovedEvent)
          {
          vtkEventBroker::GetInstance()->RemoveObservations(node, vtkCommand::ModifiedEvent, this, this->GetMRMLCallbackCommand());

          // Update the map
          if (sliceNode)
            {
            SliceNodeStatusMap::iterator it = this->SliceNodeStatus.find(node->GetID());
            if(it != this->SliceNodeStatus.end())
              {
              this->SliceNodeStatus.erase(it);
              }
            }
          }
        return;
        }
      }

    if (event == vtkMRMLScene::SceneAboutToBeClosedEvent ||
        caller == 0)
      {
      return;
      }
    }
  if (this->GetMRMLScene()->GetIsClosing())
    {
    // Do we need to remove the observers?
    return;
    }

  // Update from SliceNode
  vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(caller);
  if (sliceNode && !this->GetMRMLScene()->GetIsUpdating())
    {
    assert (event == vtkCommand::ModifiedEvent);

    SliceNodeStatusMap::iterator it = this->SliceNodeStatus.find(sliceNode->GetID());
    // if this is not the node that we are interacting with, short circuit

    if (!sliceNode->GetInteracting() || !sliceNode->GetInteractionFlags())
      {
      // We end up an interaction on the sliceNode
      if (it != this->SliceNodeStatus.end() && it->second.Interacting)
        {
        vtkMRMLSliceCompositeNode* compositeNode = this->GetCompositeNode(sliceNode);
        if (!compositeNode->GetHotLinkedControl() &&
            sliceNode->GetInteractionFlags() == vtkMRMLSliceNode::MultiplanarReformatFlag)
          {
          this->BroadcastSliceNodeEvent(sliceNode);
          }
        this->SliceNodeStatus.find(sliceNode->GetID())->second.Interacting =
          sliceNode->GetInteracting();
        }
      return;
      }

    // SliceNode was modified. Need to find the corresponding
    // SliceCompositeNode to check whether operations are linked
    vtkMRMLSliceCompositeNode* compositeNode = this->GetCompositeNode(sliceNode);

    if (compositeNode && compositeNode->GetLinkedControl())
      {
      // Slice node changed and slices are linked. Broadcast.
      //std::cout << "Slice node changed and slices are linked!" << std::endl;

      if (it != this->SliceNodeStatus.end() && !it->second.Interacting )
        {
        it->second.Interacting = sliceNode->GetInteracting();
        // Start Interaction event : we update the current sliceNodeNormal
        this->UpdateSliceNodeStatus(sliceNode);
        }

      if (compositeNode->GetHotLinkedControl() ||
          sliceNode->GetInteractionFlags() != vtkMRMLSliceNode::MultiplanarReformatFlag)
        {
        this->BroadcastSliceNodeEvent(sliceNode);
        }
      }
    else
      {
      // Slice node changed and slices are not linked. Do not broadcast.
      //std::cout << "Slice node changed and slices are NOT linked!" << std::endl;
      return;
      }
    }

  // Update from SliceCompositeNode
  vtkMRMLSliceCompositeNode* compositeNode 
    = vtkMRMLSliceCompositeNode::SafeDownCast(caller);
  if (compositeNode && !this->GetMRMLScene()->GetIsUpdating())
    {
    assert (event == vtkCommand::ModifiedEvent);

    // if this is not the node that we are interacting with, short circuit
    if (!compositeNode->GetInteracting() 
        || !compositeNode->GetInteractionFlags())
      {
      return;
      }

    if (compositeNode && compositeNode->GetLinkedControl())
      {
      // Slice composite node changed and slices are linked. Broadcast.
      //std::cout << "SliceCompositeNode changed and slices are linked!" << std::endl;
      this->BroadcastSliceCompositeNodeEvent(compositeNode);
      }
    else
      {
      // Slice composite node changed and slices are not linked. Do
      // not broadcast.
      //std::cout << "SliceCompositeNode changed and slices are NOT linked!" << std::endl;
      }
    }
}

//----------------------------------------------------------------------------
void vtkMRMLSliceLinkLogic::ProcessLogicEvents()
{
}



//----------------------------------------------------------------------------
void vtkMRMLSliceLinkLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  vtkIndent nextIndent;
  nextIndent = indent.GetNextIndent();

}


//----------------------------------------------------------------------------
void vtkMRMLSliceLinkLogic::BroadcastSliceNodeEvent(vtkMRMLSliceNode *sliceNode)
{
  // only broadcast a slice node event if we are not already actively
  // broadcasting events and we are actively interacting with the node
  // std::cout << "BroadcastingEvents: " << this->GetBroadcastingEvents()
  //           << ", Interacting: " << sliceNode->GetInteracting()
  //           << std::endl;
  if (!this->GetBroadcastingEvents())// && sliceNode->GetInteracting())
    {
    this->BroadcastingEventsOn();

    vtkMRMLSliceNode *sNode = 0;
    int nnodes = this->GetMRMLScene()->GetNumberOfNodesByClass("vtkMRMLSliceNode");
    for (int n=0; n<nnodes; n++)
      {
      sNode = vtkMRMLSliceNode::SafeDownCast (
        this->GetMRMLScene()->GetNthNodeByClass(n,"vtkMRMLSliceNode"));
      
      if (sNode != sliceNode)
        {
        // Link slice parameters whenever the reformation is consistent
        if (!strcmp(sNode->GetOrientationString(), 
                    sliceNode->GetOrientationString()))
        // if (sliceNode->Matrix4x4AreEqual(sliceNode->GetSliceToRAS(), 
        //                                  sNode->GetSliceToRAS()))
          {
          //std::cout << "Orientation match, flags = " << sliceNode->GetInteractionFlags() << std::endl;
          // std::cout << "Broadcasting SliceToRAS and FieldOfView to "
          //           << sNode->GetName() << std::endl;
          // 

          // Copy the slice to RAS information
          if (sliceNode->GetInteractionFlags() & vtkMRMLSliceNode::SliceToRASFlag)
            {
            // Need to copy the SliceToRAS. SliceNode::SetSliceToRAS()
            // does a shallow copy. So we have to explictly call DeepCopy()
            sNode->GetSliceToRAS()->DeepCopy( sliceNode->GetSliceToRAS() );
            std::cout << "Broadcast other Flag ?!."
                      << sNode << std::endl;
            }

          // Copy the field of view information. Use the new
          // prescribed x fov, aspect corrected y fov, and keep z fov
          // constant
          if (sliceNode->GetInteractionFlags() & vtkMRMLSliceNode::FieldOfViewFlag)
            {
            sNode->SetFieldOfView( sliceNode->GetFieldOfView()[0], 
                                   sliceNode->GetFieldOfView()[0] 
                                   * sNode->GetFieldOfView()[1] 
                                   / sNode->GetFieldOfView()[0], 
                                   sNode->GetFieldOfView()[2] );
            }

          // need to manage prescribed spacing here as well?

          // Forces the internal matrices to be updated which results
          // in this being modified so a Render can occur
          sNode->UpdateMatrices();
          }

        //
        // Some parameters and commands do not require the
        // orientations to match. These are handled here.
        //

        // Setting the orientation of the slice plane does not
        // require that the orientations initially match.
        if (sliceNode->GetInteractionFlags() & vtkMRMLSliceNode::OrientationFlag)
          {
          // We could copy the orientation strings, but we really
          // want the slice to ras to match, so copy that
          
          // Need to copy the SliceToRAS. SliceNode::SetSliceToRAS()
          // does a shallow copy. So we have to explictly call DeepCopy()
          sNode->GetSliceToRAS()->DeepCopy( sliceNode->GetSliceToRAS() );
          
          // Forces the internal matrices to be updated which results
          // in this being modified so a Render can occur
          sNode->UpdateMatrices();
          }
        
        // Reseting the field of view does not require the
        // orientations to match
        if (sliceNode->GetInteractionFlags() & vtkMRMLSliceNode::ResetFieldOfViewFlag)
          {
          // need the logic for this slice (sNode)
          vtkMRMLSliceLogic* logic;
          vtkCollectionSimpleIterator it;
          vtkCollection* logics = this->GetMRMLApplicationLogic()->GetSliceLogics();
          for (logics->InitTraversal(it); 
               (logic=vtkMRMLSliceLogic::SafeDownCast(logics->GetNextItemAsObject(it)));)
            {
            if (logic->GetSliceNode() == sNode)
              {
              logic->FitSliceToAll();
              sNode->UpdateMatrices();
              break;
              }
            }
          }

        // Broadcasting the rotation from a ReformatWidget
        if (sliceNode->GetInteractionFlags() & vtkMRMLSliceNode::MultiplanarReformatFlag)
          {
          SliceNodeStatusMap::iterator it = this->SliceNodeStatus.find(sliceNode->GetID());
          if (it != this->SliceNodeStatus.end())
            {
            // Calculate the rotation applied to the sliceNode
            double cross[3], dot, rotation;
            vtkTransform* transform = vtkTransform::New();
            vtkMatrix4x4* sNodeToRAS = sNode->GetSliceToRAS();
            double sliceNormal[3] = {sliceNode->GetSliceToRAS()->GetElement(0,2),
                                     sliceNode->GetSliceToRAS()->GetElement(1,2),
                                     sliceNode->GetSliceToRAS()->GetElement(2,2)};

            // Rotate the sliceNode to match the planeWidget normal
            vtkMath::Cross(it->second.LastNormal,sliceNormal, cross);
            dot = vtkMath::Dot(it->second.LastNormal, sliceNormal);
            // Clamp the dot product
            dot = (dot < -1.0) ? -1.0 : (dot > 1.0 ? 1.0 : dot);
            rotation = vtkMath::DegreesFromRadians(acos(dot));

            // Apply the rotation
            transform->PostMultiply();
            transform->SetMatrix(sNodeToRAS);
            transform->RotateWXYZ(rotation,cross);
            transform->GetMatrix(sNodeToRAS); // Update the changes
            transform->Delete();

            sNode->UpdateMatrices();
            }
          }

        //
        // End of the block for broadcasting parametes and command
        // that do not require the orientation to match
        //

        }
      }

    // Update sliceNodeStatus after MultiplanarReformat interaction
    if ((sliceNode->GetInteractionFlags() & vtkMRMLSliceNode::MultiplanarReformatFlag))
      {
      this->UpdateSliceNodeStatus(sliceNode);
      }

    this->BroadcastingEventsOff();
    }
  //std::cout << "End Broadcast" << std::endl;
}

//----------------------------------------------------------------------------
void vtkMRMLSliceLinkLogic::BroadcastSliceCompositeNodeEvent(vtkMRMLSliceCompositeNode *sliceCompositeNode)
{
  // only broadcast a slice composite node event if we are not already actively
  // broadcasting events and we actively interacting with the node
  // std::cout << "BroadcastingEvents: " << this->GetBroadcastingEvents()
  //           << ", Interacting: " << sliceCompositeNode->GetInteracting()
  //           << std::endl;
  if (!this->GetBroadcastingEvents() && sliceCompositeNode->GetInteracting())
    {
    this->BroadcastingEventsOn();

    vtkMRMLSliceCompositeNode *cNode = 0;
    int nnodes = this->GetMRMLScene()->GetNumberOfNodesByClass("vtkMRMLSliceCompositeNode");
    for (int n=0; n<nnodes; n++)
      {
      cNode = vtkMRMLSliceCompositeNode::SafeDownCast (
        this->GetMRMLScene()->GetNthNodeByClass(n,"vtkMRMLSliceCompositeNode"));
      
      if (cNode != sliceCompositeNode)
        {
        if (sliceCompositeNode->GetInteractionFlags() 
            & vtkMRMLSliceCompositeNode::ForegroundVolumeFlag)
          {
          cNode->SetForegroundVolumeID(sliceCompositeNode->GetForegroundVolumeID());
          }

        if (sliceCompositeNode->GetInteractionFlags() 
            & vtkMRMLSliceCompositeNode::BackgroundVolumeFlag)
          {
          cNode->SetBackgroundVolumeID(sliceCompositeNode->GetBackgroundVolumeID());
          }

        if (sliceCompositeNode->GetInteractionFlags() 
            & vtkMRMLSliceCompositeNode::LabelVolumeFlag)
          {
          cNode->SetLabelVolumeID(sliceCompositeNode->GetLabelVolumeID());
          }

        }
      }

    this->BroadcastingEventsOff();
    }
}

//----------------------------------------------------------------------------
vtkMRMLSliceCompositeNode* vtkMRMLSliceLinkLogic::GetCompositeNode(vtkMRMLSliceNode* sliceNode)
{
  vtkMRMLSliceCompositeNode* compositeNode = 0;
  int nnodes = this->GetMRMLScene()->GetNumberOfNodesByClass("vtkMRMLSliceCompositeNode");
  for (int n=0; n<nnodes; n++)
    {
    // should we cache this list of composite nodes?
    // or should we have special query methods in MRMLScene that
    // will be fast?????

    compositeNode = vtkMRMLSliceCompositeNode::SafeDownCast (
      this->GetMRMLScene()->GetNthNodeByClass(n,"vtkMRMLSliceCompositeNode"));

    // Is the the composite node that goes with this slice node?
    if (compositeNode->GetLayoutName()
        && !strcmp(compositeNode->GetLayoutName(), sliceNode->GetName()))
      {
      // Matching layout
      break;
      }

    compositeNode = 0;
    }

  return compositeNode;
}

//----------------------------------------------------------------------------
void vtkMRMLSliceLinkLogic::UpdateSliceNodeStatus(vtkMRMLSliceNode* sliceNode)
{
  SliceNodeStatusMap::iterator it = this->SliceNodeStatus.find(sliceNode->GetID());

  if (it != SliceNodeStatus.end())
    {
    it->second.LastNormal[0] = sliceNode->GetSliceToRAS()->GetElement(0,2);
    it->second.LastNormal[1] = sliceNode->GetSliceToRAS()->GetElement(1,2);
    it->second.LastNormal[2] = sliceNode->GetSliceToRAS()->GetElement(2,2);
    }
}
