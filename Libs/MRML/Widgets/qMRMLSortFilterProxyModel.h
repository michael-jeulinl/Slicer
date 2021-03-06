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

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

#ifndef __qMRMLSortFilterProxyModel_h
#define __qMRMLSortFilterProxyModel_h

// Qt includes
#include <QSortFilterProxyModel>
#include <QStringList>

// CTK includes
#include <ctkVTKObject.h>

// qMRML includes
#include "qMRMLWidgetsExport.h"

class vtkMRMLNode;
class vtkMRMLScene;
class qMRMLAbstractItemHelper;
class qMRMLSortFilterProxyModelPrivate;
class QStandardItem;

///
/// Filter nodes based on their types and attributes
/// Support filtering QSortFilterProxyModel::filterRegExp
class QMRML_WIDGETS_EXPORT qMRMLSortFilterProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT
  QVTK_OBJECT

  /// This property controls which node is visible. The node class name must
  /// be provided.
  /// An empty list means all the nodes are visible (default).
  Q_PROPERTY(QStringList nodeTypes READ nodeTypes WRITE setNodeTypes)

  /// This property controls whether the nodes with the HideFromEditor flag
  /// on are filtered by the proxy model or not.
  /// False by default.
  Q_PROPERTY(bool showHidden READ showHidden WRITE setShowHidden)

  /// This property overrides the behavior of \a showHidden for specific types.
  /// Empty by default.
  Q_PROPERTY(QStringList showHiddenForTypes READ showHiddenForTypes WRITE setShowHiddenForTypes)

  /// This property controls whether \a nodeType subclasses are visible
  /// If \a showChildNodeTypes is false and \a nodeTypes is vtkMRMLVolumeNode
  /// then vtkMRMLDScalarVolumeNode are not visible.
  /// True by default.
  Q_PROPERTY(bool showChildNodeTypes READ showChildNodeTypes WRITE setShowChildNodeTypes)

  /// This property controls the nodes to hide by node type
  /// Any node of type \a nodeType are visible except the ones
  /// also of type \a hideChildNodeTypes.
  /// e.g.: nodeTypes = vtkMRMLVolumeNode, showChildNodeTypes = true,
  /// hideChildNodeTypes = vtkMRMLDiffusionWeightedVolumeNode
  /// -> all the nodes of type vtkMRMLScalarVolumeNode, vtkMRMLTensorVolumeNode,
  /// vtkMRMLDiffusionImageVolumeNode... (but not vtkMRMLDiffusionWeightedVolumeNode)
  /// will be visible.
  Q_PROPERTY(QStringList hideChildNodeTypes READ hideChildNodeTypes WRITE setHideChildNodeTypes)

  /// This property controls the nodes to hide by node IDs.
  Q_PROPERTY(QStringList hiddenNodeIDs READ hiddenNodeIDs WRITE setHiddenNodeIDs)
public:
  typedef QSortFilterProxyModel Superclass;
  qMRMLSortFilterProxyModel(QObject *parent=0);
  virtual ~qMRMLSortFilterProxyModel();

  /// Retrive the associated vtkMRMLNode
  vtkMRMLScene* mrmlScene()const;

  /// Retrieve the mrml scene index
  QModelIndex mrmlSceneIndex()const;

  /// Retrieve the associated vtkMRMLNode
  vtkMRMLNode* mrmlNodeFromIndex(const QModelIndex& index)const;

  /// Retrieve an index for a given vtkMRMLNode
  QModelIndex indexFromMRMLNode(vtkMRMLNode* node, int column = 0)const;

  /// Set/Get node types to display in the list
  /// NodeTypes are the class names, i.e. vtkMRMLViewNode,
  /// vtkMRMLTransformNode
  QStringList nodeTypes()const;
  void setNodeTypes(const QStringList& nodeTypes);

  /// If a vtkMRMLNode has the property HideFromEditors set to true,
  /// bypass the property and show the node anyway.
  /// \sa setShowHiddenForTypes, showHiddenForTypes
  bool showHidden()const;

  /// Give more control over the types of mrml node you want to force
  /// the display even if their HideFromEditors property is true.
  /// Don't do anything if the list is empty.
  /// \sa setShowHiddenForTypes, showHiddenForTypes
  QStringList showHiddenForTypes()const;
  void setShowHiddenForTypes(const QStringList& nodeTypes);

  /// Add node type attribute that filter the nodes to
  /// display
  void addAttribute(const QString& nodeType,
                    const QString& attributeName,
                    const QVariant& attributeValue);

  /// Display or not the nodes that are excluded by
  /// the ExcludedChildNodeTypes list.
  /// true by default.
  void setShowChildNodeTypes(bool show);
  bool showChildNodeTypes()const;

  /// If a node is a nodeType, hide the node if it is also
  /// a ExcludedChildNodeType. (this can happen if nodeType is a
  /// mother class of ExcludedChildNodeType)
  void setHideChildNodeTypes(const QStringList& nodeTypes);
  QStringList hideChildNodeTypes()const;

  void setHiddenNodeIDs(const QStringList& nodeIDsToHide);
  QStringList hiddenNodeIDs()const;

public slots:
   void setShowHidden(bool);

  // TODO Add setMRMLScene() to propagate to the scene model
protected:
  //virtual bool filterAcceptsColumn(int source_column, const QModelIndex & source_parent)const;
  virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent)const;
  //virtual bool lessThan(const QModelIndex &left, const QModelIndex &right)const;

  QStandardItem* sourceItem(const QModelIndex& index)const;
protected:
  QScopedPointer<qMRMLSortFilterProxyModelPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qMRMLSortFilterProxyModel);
  Q_DISABLE_COPY(qMRMLSortFilterProxyModel);
};

#endif
