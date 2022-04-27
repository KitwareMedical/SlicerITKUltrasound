cmake_minimum_required(VERSION 3.16.3)

project(SlicerITKUltrasound)

#-----------------------------------------------------------------------------
# Extension meta-information
set(EXTENSION_HOMEPAGE "https://kitwaremedical.github.io/SlicerITKUltrasoundDoc/")
set(EXTENSION_CATEGORY "Filtering")
set(EXTENSION_CONTRIBUTORS "Matt McCormick (Kitware, Inc.), Mark Palmeri (Duke University), Stephen Aylward (Kitware, Inc.), Jean-Christophe Fillion-Robin (Kitware, Inc.)")
set(EXTENSION_DESCRIPTION "Ultrasound image formation, processing, and analysis. Interfaces built off the ITKUltrasound library.")
set(EXTENSION_ICONURL "https://github.com/KitwareMedical/SlicerITKUltrasound/raw/master/SlicerITKUltrasound.png")
set(EXTENSION_SCREENSHOTURLS
  "https://github.com/KitwareMedical/SlicerITKUltrasound/raw/f1f347fc6b9a1a56200ed1bbc2ff7f4e3d0836ef/Documentation/source/Modules/ScanConversion/ScanConvertPhasedArray3D.png"
  "https://github.com/KitwareMedical/SlicerITKUltrasound/raw/f1f347fc6b9a1a56200ed1bbc2ff7f4e3d0836ef/Documentation/source/Modules/ScanConversion/ScanConvertCurvilinearArray.png"
  "https://github.com/KitwareMedical/SlicerITKUltrasound/raw/f1f347fc6b9a1a56200ed1bbc2ff7f4e3d0836ef/Documentation/source/Modules/ScanConversion/ScanConvertSliceSeries.png"
  )
set(EXTENSION_BUILD_SUBDIRECTORY SlicerITKUltrasound-build)
set(EXTENSION_STATUS Beta)

set(SUPERBUILD_TOPLEVEL_PROJECT SlicerITKUltrasound)

find_package(Git REQUIRED)

#-----------------------------------------------------------------------------
# Extension dependencies
find_package(Slicer REQUIRED)
include(${Slicer_USE_FILE})
mark_as_superbuild(Slicer_DIR)
mark_as_superbuild(PYTHON_EXECUTABLE)
mark_as_superbuild(PYTHON_INCLUDE_DIR)
mark_as_superbuild(PYTHON_LIBRARY)

#-----------------------------------------------------------------------------
option(${EXTENSION_NAME}_SUPERBUILD "Build ${EXTENSION_NAME} and the projects it depends on." ON)
mark_as_advanced(${EXTENSION_NAME}_SUPERBUILD)
if(${EXTENSION_NAME}_SUPERBUILD)
  include("${CMAKE_CURRENT_SOURCE_DIR}/SuperBuild.cmake")
  return()
endif()

#-----------------------------------------------------------------------------
# Extension modules
add_subdirectory(ApplyButterworthHighpass1D)
add_subdirectory(GenerateBModeFromRF)
add_subdirectory(GenerateDisplacementFromTimeSeries)
add_subdirectory(GenerateDisplacementFromFrames)
add_subdirectory(ScanConvertPhasedArray3D)
add_subdirectory(ScanConvertCurvilinearArray)
add_subdirectory(ScanConvertSliceSeries)

#-----------------------------------------------------------------------------
set(EXTENSION_CPACK_INSTALL_CMAKE_PROJECTS)
if(NOT APPLE)
  # Skip packaging of ITK modules without ITK libraries.
  #list(APPEND EXTENSION_CPACK_INSTALL_CMAKE_PROJECTS "${ITKBSplineGradient_DIR};BSplineGradient;RuntimeLibraries;/")
  #list(APPEND EXTENSION_CPACK_INSTALL_CMAKE_PROJECTS "${ITKHigherOrderAccurateGradient_DIR};HigherOrderAccurateGradient;RuntimeLibraries;/")
  #list(APPEND EXTENSION_CPACK_INSTALL_CMAKE_PROJECTS "${ITKSplitComponents_DIR};SplitComponents;RuntimeLibraries;/")
  #list(APPEND EXTENSION_CPACK_INSTALL_CMAKE_PROJECTS "${ITKStrain_DIR};Strain;RuntimeLibraries;/")
  list(APPEND EXTENSION_CPACK_INSTALL_CMAKE_PROJECTS "${ITKUltrasound_DIR};Ultrasound;RuntimeLibraries;/")
endif()
set(${EXTENSION_NAME}_CPACK_INSTALL_CMAKE_PROJECTS "${EXTENSION_CPACK_INSTALL_CMAKE_PROJECTS}" CACHE STRING "List of external projects to install" FORCE)

#-----------------------------------------------------------------------------
list(APPEND CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};${EXTENSION_NAME};ALL;/")
list(APPEND CPACK_INSTALL_CMAKE_PROJECTS "${${EXTENSION_NAME}_CPACK_INSTALL_CMAKE_PROJECTS}")
include(${Slicer_EXTENSION_GENERATE_CONFIG})
include(${Slicer_EXTENSION_CPACK})
