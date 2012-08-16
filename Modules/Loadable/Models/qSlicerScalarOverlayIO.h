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

#ifndef __qSlicerScalarOverlayIO_h
#define __qSlicerScalarOverlayIO_h

// SlicerQt includes
#include "qSlicerFileReader.h"
class qSlicerScalarOverlayIOPrivate;

// Slicer includes
class vtkSlicerModelsLogic;

//-----------------------------------------------------------------------------
class qSlicerScalarOverlayIO
  : public qSlicerFileReader
{
  Q_OBJECT
public:
  typedef qSlicerFileReader Superclass;
  qSlicerScalarOverlayIO(vtkSlicerModelsLogic* modelsLogic, QObject* parent = 0);
  virtual ~qSlicerScalarOverlayIO();

  void setModelsLogic(vtkSlicerModelsLogic* modelsLogic);
  vtkSlicerModelsLogic* modelsLogic()const;

  virtual QString description()const;
  virtual QString fileType()const;
  virtual QStringList extensions()const;
  virtual qSlicerIOOptions* options()const;

  virtual bool load(const IOProperties& properties);

protected:
  QScopedPointer<qSlicerScalarOverlayIOPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerScalarOverlayIO);
  Q_DISABLE_COPY(qSlicerScalarOverlayIO);
};

#endif
