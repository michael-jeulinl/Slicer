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

#ifndef __qSlicerAnnotationsIO_h
#define __qSlicerAnnotationsIO_h

// SlicerQt includes
#include "qSlicerFileReader.h"

class qSlicerAnnotationsIOPrivate;
class vtkSlicerAnnotationModuleLogic;

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Annotations
class qSlicerAnnotationsIO
  : public qSlicerFileReader
{
  Q_OBJECT
public:
  typedef qSlicerFileReader Superclass;
  qSlicerAnnotationsIO(QObject* parent = 0);
  qSlicerAnnotationsIO(vtkSlicerAnnotationModuleLogic* logic, QObject* parent = 0);
  virtual ~qSlicerAnnotationsIO();
  
  vtkSlicerAnnotationModuleLogic* annotationLogic()const;
  void setAnnotationLogic(vtkSlicerAnnotationModuleLogic* logic);

  virtual QString description()const;
  virtual QString fileType()const;
  virtual QStringList extensions()const;
  virtual qSlicerIOOptions* options()const;

  virtual bool load(const IOProperties& properties);
protected:
  QScopedPointer<qSlicerAnnotationsIOPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerAnnotationsIO);
  Q_DISABLE_COPY(qSlicerAnnotationsIO);

};

#endif
