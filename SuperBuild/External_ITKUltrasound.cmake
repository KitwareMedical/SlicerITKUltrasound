#-----------------------------------------------------------------------------
# Build the ITK Ultrasound module, pointing it to Slicer's ITK

set(proj ITKUltrasound)

# Dependencies
set(${proj}_DEPENDENCIES )
ExternalProject_Include_Dependencies(${proj} PROJECT_VAR proj DEPENDS_VAR ${proj}_DEPENDENCIES)

set(${proj}_BINARY_DIR ${CMAKE_BINARY_DIR}/${proj}-build)

set(${proj}_GIT_TAG 2443abe29ae44799a5e189fb66c7b8b7a044dcdf)
ExternalProject_Add(${proj}
  ${${proj}_EP_ARGS}
  GIT_REPOSITORY ${EP_GIT_PROTOCOL}://github.com/KitwareMedical/ITKUltrasound.git
  GIT_TAG ${${proj}_GIT_TAG}
  SOURCE_DIR ${proj}
  BINARY_DIR ${${proj}_BINARY_DIR}
  CMAKE_CACHE_ARGS
    # Compiler settings
    -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
    -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
    -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
    -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
    -DCMAKE_CXX_STANDARD:STRING=${CMAKE_CXX_STANDARD}
    # Dependencies
    -DITK_DIR:PATH=${ITK_DIR}
    -DITKUltrasound_USE_VTK:BOOL=ON
    -DVTK_DIR:PATH=${VTK_DIR}
    -DPYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE}
    -DPYTHON_INCLUDE_DIR:PATH=${PYTHON_INCLUDE_DIR}
    -DPYTHON_LIBRARY:FILEPATH=${PYTHON_LIBRARY}
    # Options
    -DBUILD_TESTING:BOOL=OFF
    # Install directories
    -DITK_INSTALL_RUNTIME_DIR:STRING=${Slicer_INSTALL_LIB_DIR}
    -DITK_INSTALL_LIBRARY_DIR:STRING=${Slicer_INSTALL_LIB_DIR}
  INSTALL_COMMAND ""
  DEPENDS ${${proj}_DEPENDENCIES}
)

set(${proj}_DIR ${${proj}_BINARY_DIR})
mark_as_superbuild(VARS ${proj}_DIR:PATH)

ExternalProject_Message(${proj} "${proj}_DIR:${${proj}_DIR}")
