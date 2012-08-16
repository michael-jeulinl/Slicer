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

#ifndef __qSlicerVolumesIO
#define __qSlicerVolumesIO

// SlicerQt includes
#include "qSlicerFileReader.h"
class qSlicerVolumesIOPrivate;
class vtkSlicerVolumesLogic;

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Volumes
class qSlicerVolumesIO
  : public qSlicerFileReader
{
  Q_OBJECT
public:
  typedef qSlicerFileReader Superclass;
  qSlicerVolumesIO(QObject* parent = 0);
  qSlicerVolumesIO(vtkSlicerVolumesLogic* logic, QObject* parent = 0);
  virtual ~qSlicerVolumesIO();

  vtkSlicerVolumesLogic* logic()const;
  void setLogic(vtkSlicerVolumesLogic* logic);

  virtual QString description()const;
  virtual QString fileType()const;
  virtual QStringList extensions()const;
  virtual qSlicerIOOptions* options()const;

  virtual bool load(const IOProperties& properties);
protected:
  QScopedPointer<qSlicerVolumesIOPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerVolumesIO);
  Q_DISABLE_COPY(qSlicerVolumesIO);
};

#endif
