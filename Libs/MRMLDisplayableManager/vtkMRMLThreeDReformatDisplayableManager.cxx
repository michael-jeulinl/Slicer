/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Michael Jeulin-L, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// MRMLDisplayableManager includes
#include "vtkMRMLThreeDReformatDisplayableManager.h"

// MRML includes
#include <vtkThreeDViewInteractorStyle.h>
#include <vtkMRMLSliceCompositeNode.h>
#include <vtkMRMLSliceNode.h>
#include <vtkMRMLSliceLogic.h>
#include <vtkMRMLVolumeNode.h>

// VTK includes
#include <vtkActor.h>
#include <vtkCallbackCommand.h>
#include <vtkImageData.h>
#include <vtkImplicitPlaneWidget2.h>
#include <vtkImplicitPlaneRepresentation.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSphereSource.h>
#include <vtkTransform.h>

// STD includes
#include <algorithm>
#include <cmath>

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkMRMLThreeDReformatDisplayableManager);
vtkCxxRevisionMacro(vtkMRMLThreeDReformatDisplayableManager, "$Revision: 13525 $");

//---------------------------------------------------------------------------
class vtkMRMLThreeDReformatDisplayableManager::vtkInternal
{
public:
  typedef std::map<vtkMRMLSliceNode*, vtkImplicitPlaneWidget2*> SliceNodesLink;

  vtkInternal(vtkMRMLThreeDReformatDisplayableManager* external);
  ~vtkInternal();

  // View
  vtkMRMLViewNode* GetViewNode();

  // SliceNodes
  void AddSliceNode(vtkMRMLSliceNode*);
  void RemoveSliceNode(vtkMRMLSliceNode*);
  void RemoveSliceNode(SliceNodesLink::iterator);
  void UpdateSliceNodes();
  void UpdateSliceNode(vtkMRMLSliceNode*, vtkImplicitPlaneWidget2*);
  vtkMRMLSliceNode* GetSliceNode(vtkImplicitPlaneWidget2*);

  // Widget
  vtkImplicitPlaneWidget2* NewImplicitPlaneWidget();
  vtkImplicitPlaneWidget2* GetWidget(vtkMRMLSliceNode*);
  void UpdateWidget(vtkMRMLSliceNode*, vtkImplicitPlaneWidget2*);

  SliceNodesLink                                SliceNodes;
  vtkMRMLThreeDReformatDisplayableManager*      External;

  // Callback
  vtkCallbackCommand* ImplicitPlaneWidgetCallbackCommand;
};

//---------------------------------------------------------------------------
// vtkInternal methods

//---------------------------------------------------------------------------
vtkMRMLThreeDReformatDisplayableManager::vtkInternal::vtkInternal(
    vtkMRMLThreeDReformatDisplayableManager* _external)
{
  this->External = _external;

  // Setup Widget callback
  this->ImplicitPlaneWidgetCallbackCommand = vtkCallbackCommand::New();
  this->ImplicitPlaneWidgetCallbackCommand->SetClientData(reinterpret_cast<void *> (_external));
  this->ImplicitPlaneWidgetCallbackCommand->SetCallback(
        vtkMRMLThreeDReformatDisplayableManager::WidgetCallback);
}

//---------------------------------------------------------------------------
vtkMRMLThreeDReformatDisplayableManager::vtkInternal::~vtkInternal()
{
  // The manager has the responsabilty to delete the widgets.
  while(this->SliceNodes.size() > 0)
    {
    RemoveSliceNode(this->SliceNodes.begin());
    }
  SliceNodes.clear();

  this->ImplicitPlaneWidgetCallbackCommand->Delete();
}

//---------------------------------------------------------------------------
vtkMRMLViewNode* vtkMRMLThreeDReformatDisplayableManager::vtkInternal
::GetViewNode()
{
  return this->External->GetMRMLViewNode();
}

//---------------------------------------------------------------------------
void vtkMRMLThreeDReformatDisplayableManager::vtkInternal
::AddSliceNode(vtkMRMLSliceNode* sliceNode)
{
  if(!sliceNode ||
     this->SliceNodes.find(vtkMRMLSliceNode::SafeDownCast(sliceNode)) != this->SliceNodes.end())
    {
    return;
    }

  // We associate the node with the widget if an instantiation is called.
  // We add the sliceNode without instantiating the widget first.
  sliceNode->AddObserver(vtkCommand::ModifiedEvent,
                           this->External->GetMRMLCallbackCommand());
  this->SliceNodes.insert(
        std::pair<vtkMRMLSliceNode*, vtkImplicitPlaneWidget2*>(sliceNode,0));

}

//---------------------------------------------------------------------------
void vtkMRMLThreeDReformatDisplayableManager::vtkInternal
::RemoveSliceNode(vtkMRMLSliceNode* sliceNode)
{
  if(!sliceNode)
    {
    return;
    }

   this->RemoveSliceNode(
        this->SliceNodes.find(vtkMRMLSliceNode::SafeDownCast(sliceNode)));
}

//---------------------------------------------------------------------------
void vtkMRMLThreeDReformatDisplayableManager::vtkInternal
::RemoveSliceNode(SliceNodesLink::iterator it)
{
  if(it == this->SliceNodes.end())
    {
    return;
    }

  // The manager has the responsabilty to delete the widget.
  if(it->second)
    {
    it->second->Delete();
    }

  it->first->RemoveObserver(this->External->GetMRMLCallbackCommand());
  this->SliceNodes.erase(it);
}

//---------------------------------------------------------------------------
void vtkMRMLThreeDReformatDisplayableManager::vtkInternal
::UpdateSliceNodes()
{
  if (this->External->GetMRMLScene() == 0)
    {
    return;
    }

  vtkMRMLNode* node;
  vtkCollectionSimpleIterator it;
  vtkCollection* scene = this->External->GetMRMLScene()->GetCurrentScene();
  for (scene->InitTraversal(it);
       (node = vtkMRMLNode::SafeDownCast(scene->GetNextItemAsObject(it))) ;)
    {
    vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(node);
    if (sliceNode)
      {
      this->AddSliceNode(sliceNode);
      }
    }
}

//---------------------------------------------------------------------------
void vtkMRMLThreeDReformatDisplayableManager::vtkInternal
::UpdateSliceNode(vtkMRMLSliceNode* sliceNode, vtkImplicitPlaneWidget2* planeWidget)
{
  if(!sliceNode->GetWidgetVisible() || planeWidget)
    {
    return;
    }

  // Instantiate widget and link it if
  // there is no one associated to the sliceNode yet
  planeWidget = this->NewImplicitPlaneWidget();

  // Color the Edge of the plane representation depending on the Slice
  if (std::string(sliceNode->GetLayoutName()) == "Red")
    {
    planeWidget->GetImplicitPlaneRepresentation()->SetEdgeColor(1.,0.,0.);
    }
  else if (std::string(sliceNode->GetLayoutName()) == "Yellow")
    {
    planeWidget->GetImplicitPlaneRepresentation()->SetEdgeColor(1.,1.,0.);
    }
  else if (std::string(sliceNode->GetLayoutName()) == "Green")
    {
    planeWidget->GetImplicitPlaneRepresentation()->SetEdgeColor(0.,1.,0.);
    }

  this->SliceNodes.find(sliceNode)->second  = planeWidget;
}

//---------------------------------------------------------------------------
vtkMRMLSliceNode* vtkMRMLThreeDReformatDisplayableManager::vtkInternal
::GetSliceNode(vtkImplicitPlaneWidget2* planeWidget)
{
  if(!planeWidget)
    {
    return 0;
    }

  // Get the slice node
  vtkMRMLSliceNode* sliceNode = 0;
  for (SliceNodesLink::iterator it=this->SliceNodes.begin() ; it!=this->SliceNodes.end(); ++it)
    {
      if (it->second == planeWidget)
      {
      sliceNode = it->first;
      break;
      }
    }

  return sliceNode;
}

//---------------------------------------------------------------------------
vtkImplicitPlaneWidget2* vtkMRMLThreeDReformatDisplayableManager::vtkInternal
::NewImplicitPlaneWidget()
{
  // Instantiate implcite plane widget and his representation
  vtkImplicitPlaneRepresentation* rep = vtkImplicitPlaneRepresentation::New();
  rep->SetOutlineTranslation(0);
  rep->SetScaleEnabled(0);
  rep->SetDrawPlane(0);

  // The Manager has to manage the destruction of the widgets
  vtkImplicitPlaneWidget2* planeWidget = vtkImplicitPlaneWidget2::New();
  planeWidget->SetInteractor(this->External->GetInteractor());
  planeWidget->SetRepresentation(rep);
  planeWidget->SetEnabled(0);

  // Link widget evenement to the LogicCallbackCommand
  planeWidget->AddObserver(vtkCommand::StartInteractionEvent,
                           this->ImplicitPlaneWidgetCallbackCommand);
  planeWidget->AddObserver(vtkCommand::InteractionEvent,
                           this->ImplicitPlaneWidgetCallbackCommand);
  planeWidget->AddObserver(vtkCommand::EndInteractionEvent,
                           this->ImplicitPlaneWidgetCallbackCommand);
  planeWidget->AddObserver(vtkCommand::UpdateEvent,
                           this->ImplicitPlaneWidgetCallbackCommand);

  return planeWidget;
}

//---------------------------------------------------------------------------
vtkImplicitPlaneWidget2* vtkMRMLThreeDReformatDisplayableManager::vtkInternal
::GetWidget(vtkMRMLSliceNode* sliceNode)
{
  if(!sliceNode)
    {
    return 0;
    }

  SliceNodesLink::iterator it = this->SliceNodes.find(sliceNode);
  return (it != this->SliceNodes.end()) ? it->second : 0;
}

//---------------------------------------------------------------------------
void vtkMRMLThreeDReformatDisplayableManager::vtkInternal
::UpdateWidget(vtkMRMLSliceNode* sliceNode, vtkImplicitPlaneWidget2* planeWidget)
{
  if(!sliceNode || !planeWidget)
    {
    return;
    }

  // Update the representation
  vtkImplicitPlaneRepresentation* rep =  planeWidget->GetImplicitPlaneRepresentation();
  vtkMatrix4x4* sliceToRAS = sliceNode->GetSliceToRAS();

    // Update Bound size
  vtkMRMLSliceCompositeNode* sliceCompositeNode =
    vtkMRMLSliceLogic::GetSliceCompositeNode(sliceNode);
  vtkMRMLVolumeNode* volumeNode = vtkMRMLVolumeNode::SafeDownCast(
    this->External->GetMRMLScene()->GetNodeByID(
      sliceCompositeNode->GetBackgroundVolumeID()));
  double dimensions[3], center[3];
  vtkMRMLSliceLogic::GetVolumeRASBox(volumeNode, dimensions, center);
  double bounds[6] = {bounds[0] = center[0] - dimensions[0] / 2,
                      bounds[1] = center[0] + dimensions[0] / 2,
                      bounds[2] = center[1] - dimensions[1] / 2,
                      bounds[3] = center[1] + dimensions[1] / 2,
                      bounds[4] = center[2] - dimensions[2] / 2,
                      bounds[5] = center[2] + dimensions[2] / 2};
  rep->SetPlaceFactor(1.);
  rep->PlaceWidget(bounds);

    // Update normal
  rep->SetNormal(sliceToRAS->GetElement(0,2),
                 sliceToRAS->GetElement(1,2),
                 sliceToRAS->GetElement(2,2));
    // Update origin position
  rep->SetOrigin(sliceToRAS->GetElement(0,3),
                 sliceToRAS->GetElement(1,3),
                 sliceToRAS->GetElement(2,3));

  // Update the widget itself if necessary
  if((!planeWidget->GetEnabled() && sliceNode->GetWidgetVisible()) ||
     (planeWidget->GetEnabled() && !sliceNode->GetWidgetVisible()) ||
     (!rep->GetLockNormalToCamera() && sliceNode->GetPlaneLockedToCamera()) ||
     (rep->GetLockNormalToCamera() && !sliceNode->GetPlaneLockedToCamera()))
    {
    planeWidget->SetEnabled(sliceNode->GetWidgetVisible());

    // We update the normal of the widget and the sliceNode if changed
    rep->SetLockNormalToCamera(sliceNode->GetPlaneLockedToCamera());
    if(rep->GetLockNormalToCamera())
      {
      rep->SetNormalToCamera();
      planeWidget->InvokeEvent(vtkCommand::UpdateEvent, NULL);
      }
    }
}

//---------------------------------------------------------------------------
// vtkMRMLSliceModelDisplayableManager methods

//---------------------------------------------------------------------------
vtkMRMLThreeDReformatDisplayableManager::vtkMRMLThreeDReformatDisplayableManager()
{
  this->Internal = new vtkInternal(this);
}

//---------------------------------------------------------------------------
vtkMRMLThreeDReformatDisplayableManager::~vtkMRMLThreeDReformatDisplayableManager()
{
  delete this->Internal;
}

//---------------------------------------------------------------------------
void vtkMRMLThreeDReformatDisplayableManager::OnMRMLSceneNodeAddedEvent(vtkMRMLNode* nodeAdded)
{
  if (this->GetMRMLScene()->GetIsUpdating() ||
      !nodeAdded->IsA("vtkMRMLSliceNode"))
    {
    return;
    }

  this->Internal->AddSliceNode(vtkMRMLSliceNode::SafeDownCast(nodeAdded));
}

//---------------------------------------------------------------------------
void vtkMRMLThreeDReformatDisplayableManager::OnMRMLSceneNodeRemovedEvent(vtkMRMLNode* nodeRemoved)
{
  if (this->GetMRMLScene()->GetIsUpdating() ||
      !nodeRemoved->IsA("vtkMRMLSliceNode"))
    {
    return;
    }

  this->Internal->RemoveSliceNode(vtkMRMLSliceNode::SafeDownCast(nodeRemoved));
}

//---------------------------------------------------------------------------
void vtkMRMLThreeDReformatDisplayableManager::ProcessMRMLEvents(vtkObject *caller,
                                                       unsigned long event,
                                                       void *callData)
{
  // SceneEvent
  vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(caller);
  if (sliceNode)
    {
    if (event == vtkCommand::ModifiedEvent)
      {
      vtkImplicitPlaneWidget2* planeWidget = this->Internal->GetWidget(sliceNode);
      this->Internal->UpdateSliceNode(sliceNode, planeWidget);

      // Refresh pointer if a new instantiation happend in UpdateSliceNode
      planeWidget = (planeWidget) ? planeWidget :
                                    this->Internal->SliceNodes.find(sliceNode)->second;
      this->Internal->UpdateWidget(sliceNode, planeWidget);
      this->RequestRender();
      }
    }
  else
    {
    this->Superclass::ProcessMRMLEvents(caller, event, callData);
    }

  return;
}

//----------------------------------------------------------------------------
void vtkMRMLThreeDReformatDisplayableManager::ProcessWidgetEvents(
    vtkObject *caller, unsigned long vtkNotUsed(event),
    void *vtkNotUsed(callData))
{
  vtkImplicitPlaneWidget2* planeWidget = vtkImplicitPlaneWidget2::SafeDownCast(caller);
  vtkMRMLSliceNode* sliceNode = Internal->GetSliceNode(planeWidget);
  vtkImplicitPlaneRepresentation* rep = (!planeWidget) ? 0 :
      vtkImplicitPlaneRepresentation::SafeDownCast(planeWidget->GetRepresentation());

  if (!planeWidget || !sliceNode || !rep)
  {
  return;
  }

  double cross[3], dot, rotation;
  vtkTransform* transform = vtkTransform::New();

  vtkMatrix4x4* sliceToRAS = sliceNode->GetSliceToRAS();
  double sliceNormal[3] = {sliceToRAS->GetElement(0,2),
                           sliceToRAS->GetElement(1,2),
                           sliceToRAS->GetElement(2,2)};

  // Reset current translation
  sliceToRAS->SetElement(0,3,0);
  sliceToRAS->SetElement(1,3,0);
  sliceToRAS->SetElement(2,3,0);

  // Rotate the sliceNode to match the planeWidget normal
  vtkMath::Cross(sliceNormal, rep->GetNormal(), cross);
  dot = vtkMath::Dot(sliceNormal, rep->GetNormal());
   // Clamp the dot product
  dot = (dot < -1.0) ? -1.0 : (dot > 1.0 ? 1.0 : dot);
  rotation = vtkMath::DegreesFromRadians(acos(dot));

    // Apply the rotation
  transform->PostMultiply();
  transform->SetMatrix(sliceToRAS);
  transform->RotateWXYZ(rotation,cross);
  transform->GetMatrix(sliceToRAS); // Update the changes
  transform->Delete();

  // Insert the widget translation
  double* planeWidgetOrigin = rep->GetOrigin();
  sliceToRAS->SetElement(0, 3, planeWidgetOrigin[0]);
  sliceToRAS->SetElement(1, 3, planeWidgetOrigin[1]);
  sliceToRAS->SetElement(2, 3, planeWidgetOrigin[2]);
  sliceNode->UpdateMatrices();

  this->RequestRender();
}

//----------------------------------------------------------------------------
// Description:
// the WidgetCallback is a static function to relay modified events from the
// observed mrml node back into the gui layer for further processing
//
void vtkMRMLThreeDReformatDisplayableManager::WidgetCallback(vtkObject *caller, unsigned long eid,
                                         void *clientData, void *callData)
{
  vtkMRMLThreeDReformatDisplayableManager* self =
      reinterpret_cast<vtkMRMLThreeDReformatDisplayableManager *>(clientData);

  self->ProcessWidgetEvents(caller, eid, callData);
}

//---------------------------------------------------------------------------
void vtkMRMLThreeDReformatDisplayableManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkMRMLThreeDReformatDisplayableManager::Create()
{
  this->Internal->UpdateSliceNodes();
}
