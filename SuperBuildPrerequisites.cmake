if(DEFINED slicersources_SOURCE_DIR AND NOT DEFINED Slicer_SOURCE_DIR)
  # Explicitly setting "Slicer_SOURCE_DIR" when only "slicersources_SOURCE_DIR"
  # is defined is required to successfully complete configuration in an empty
  # build directory
  #
  # Indeed, in that case, Slicer sources have been downloaded by they have not been
  # added using "add_subdirectory()" and the variable "Slicer_SOURCE_DIR" is not yet in
  # in the CACHE.
  set(Slicer_SOURCE_DIR ${slicersources_SOURCE_DIR})
endif()

if(NOT DEFINED Slicer_SOURCE_DIR)
  # Extension is built standalone

  # NA

else()
  # Extension is bundled in a custom application

  # Additional external project dependencies
  foreach(itk_module IN ITEMS
    ITKBSplineGradient
    ITKHigherOrderAccurateGradient
    ITKMeshToPolyData
    ITKSplitComponents
    ITKStrain
    ITKUltrasound
    )
    ExternalProject_Add_Dependencies(${itk_module}
      DEPENDS
        ITK
      )
  endforeach()

  ExternalProject_Add_Dependencies(ITKUltrasound
    DEPENDS
      VTK
    )

endif()
