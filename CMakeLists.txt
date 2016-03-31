cmake_minimum_required(VERSION 2.8.9)

project(SlicerITKUltrasound)

#-----------------------------------------------------------------------------
# Extension meta-information
set(EXTENSION_HOMEPAGE "http://slicer.org/slicerWiki/index.php/Documentation/Nightly/Extensions/SlicerITKUltrasound")
set(EXTENSION_CATEGORY "Filtering")
set(EXTENSION_CONTRIBUTORS "Matt McCormick (Kitware, Inc.), Stephen Aylward (Kitware, Inc.), Hyun Jae Kang (Kitware, Inc.)")
set(EXTENSION_DESCRIPTION "Ultrasound image formation, processing, and analysis. Interfaces built off the ITKUltrasound library.")
set(EXTENSION_ICONURL "http://www.example.com/Slicer/Extensions/SlicerITKUltrasound.png")
set(EXTENSION_SCREENSHOTURLS "http://www.example.com/Slicer/Extensions/SlicerITKUltrasound/Screenshots/1.png")

#-----------------------------------------------------------------------------
# Extension dependencies
find_package(Slicer REQUIRED)
include(${Slicer_USE_FILE})

#-----------------------------------------------------------------------------
# Extension modules
add_subdirectory(ScanConvertPhasedArray3D)
## NEXT_MODULE

#-----------------------------------------------------------------------------
include(${Slicer_EXTENSION_CPACK})
