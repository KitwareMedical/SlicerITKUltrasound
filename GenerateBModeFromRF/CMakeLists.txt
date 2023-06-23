cmake_minimum_required(VERSION 3.16.3)

#-----------------------------------------------------------------------------
set(MODULE_NAME GenerateBModeFromRF)

#-----------------------------------------------------------------------------

#
# SlicerExecutionModel
#
find_package(SlicerExecutionModel REQUIRED)
include(${SlicerExecutionModel_USE_FILE})

#
# ITK
#
set(${PROJECT_NAME}_ITK_COMPONENTS
  ITKIOImageBase
  Ultrasound
  )
if(NOT DEFINED Slicer_DIR)
  list(APPEND ${PROJECT_NAME}_ITK_COMPONENTS ITKImageIO)
endif()
find_package(ITK 4.9 COMPONENTS ${${PROJECT_NAME}_ITK_COMPONENTS} REQUIRED)
if(DEFINED Slicer_DIR)
  set(ITK_NO_IO_FACTORY_REGISTER_MANAGER 1) # See Libs/ITKFactoryRegistration/CMakeLists.txt
endif()
include(${ITK_USE_FILE})

#-----------------------------------------------------------------------------
set(MODULE_INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  )

set(MODULE_SRCS
  )

set(MODULE_TARGET_LIBRARIES
  ${ITK_LIBRARIES}
  )

#-----------------------------------------------------------------------------
SEMMacroBuildCLI(
  NAME ${MODULE_NAME}
  TARGET_LIBRARIES ${MODULE_TARGET_LIBRARIES}
  INCLUDE_DIRECTORIES ${MODULE_INCLUDE_DIRECTORIES}
  ADDITIONAL_SRCS ${MODULE_SRCS}
  )

#-----------------------------------------------------------------------------
if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()
