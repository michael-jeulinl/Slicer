cmake_minimum_required(VERSION 2.8.7)

#-----------------------------------------------------------------------------
set(MODULE_NAME LoadableExtensionTemplate) # Do not use 'project()'

#-----------------------------------------------------------------------------
if(NOT Slicer_SOURCE_DIR)
  set(MODULE_HOMEPAGE "http://www.slicer.org/slicerWiki/index.php/Documentation/Snapshot/Developers/Tutorials/ExtensionWriting")
  set(MODULE_CATEGORY "Examples")
  set(MODULE_STATUS "Beta")
  set(MODULE_CONTRIBUTORS "Jean-Christophe Fillion-Robin (Kitware)")
  set(MODULE_DESCRIPTION "This is an example of Qt loadable module built as an extension")
  set(MODULE_ICONURL "http://viewvc.slicer.org/viewvc.cgi/Slicer4/trunk/Extensions/Testing/LoadableExtensionTemplate/Resources/Icons/LoadableExtensionTemplate.png?revision=19437&view=co")
  set(MODULE_SCREENSHOTURLS "http://wiki.slicer.org/slicerWiki/images/4/42/Slicer-r19441-LoadableExtensionTemplate-screenshot.png")
  set(MODULE_DEPENDS CLIExtensionTemplate) # Specified as a space separated list or 'NA' if any
endif()

#-----------------------------------------------------------------------------
if(NOT Slicer_SOURCE_DIR)
  find_package(Slicer REQUIRED)
  include(${Slicer_USE_FILE})
endif()

#-----------------------------------------------------------------------------
add_subdirectory(Logic)

#-----------------------------------------------------------------------------
set(qt_module_export_directive "Q_SLICER_QTMODULES_LOADABLEEXTENSIONTEMPLATE_EXPORT")

# Current_{source,binary} and Slicer_{Libs,Base} already included
set(qt_module_include_directories
  ${CMAKE_CURRENT_SOURCE_DIR}/Logic
  ${CMAKE_CURRENT_BINARY_DIR}/Logic
  )

set(qt_module_SRCS
  qSlicerLoadableExtensionTemplateModule.cxx
  qSlicerLoadableExtensionTemplateModule.h
  qSlicerLoadableExtensionTemplateModuleWidget.cxx
  qSlicerLoadableExtensionTemplateModuleWidget.h
  )

set(qt_module_MOC_SRCS
  qSlicerLoadableExtensionTemplateModule.h
  qSlicerLoadableExtensionTemplateModuleWidget.h
  )

set(qt_module_UI_SRCS
  Resources/UI/qSlicerLoadableExtensionTemplateModule.ui
  )

set(qt_module_target_libraries
  vtkSlicerLoadableExtensionTemplateModuleLogic
  )

set(qt_module_resources
  Resources/qSlicerLoadableExtensionTemplateModule.qrc
  )

#-----------------------------------------------------------------------------
slicerMacroBuildQtModule(
  NAME ${MODULE_NAME}
  EXPORT_DIRECTIVE ${qt_module_export_directive}
  INCLUDE_DIRECTORIES ${qt_module_include_directories}
  SRCS ${qt_module_SRCS}
  MOC_SRCS ${qt_module_MOC_SRCS}
  UI_SRCS ${qt_module_UI_SRCS}
  TARGET_LIBRARIES ${qt_module_target_libraries}
  RESOURCES ${qt_module_resources}
  )

#-----------------------------------------------------------------------------
if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()

#-----------------------------------------------------------------------------
if(NOT Slicer_SOURCE_DIR)
  include(${Slicer_EXTENSION_CPACK})
endif()
