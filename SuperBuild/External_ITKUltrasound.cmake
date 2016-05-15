#-----------------------------------------------------------------------------
# Build the ITK Ultrasound module, pointing it to Slicer's ITK

set(proj ITKUltrasound)

# Dependencies
set(${proj}_DEPENDENCIES )
ExternalProject_Include_Dependencies(${proj} PROJECT_VAR proj DEPENDS_VAR ${proj}_DEPENDENCIES)

# master 2016-05-25
set(${proj}_GIT_TAG 5d8bb437908cda9769f05583edec43d2b0d1831d)
ExternalProject_Add(${proj}
  ${${proj}_EP_ARGS}
  GIT_REPOSITORY ${git_protocol}://github.com/KitwareMedical/ITKUltrasound.git
  GIT_TAG ${${proj}_GIT_TAG}
  SOURCE_DIR ${proj}
  BINARY_DIR ${proj}-build
  INSTALL_COMMAND ""
  CMAKE_CACHE_ARGS
    -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
    -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
    -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
    -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
    # to find ITKConfig.cmake
    -DITK_DIR:PATH=${ITK_DIR}
    -DPYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE}
    -DPYTHON_INCLUDE_DIR:PATH=${PYTHON_INCLUDE_DIR}
    -DPYTHON_LIBRARY:FILEPATH=${PYTHON_LIBRARY}
    -DBUILD_TESTING:BOOL=OFF
    -DITK_INSTALL_RUNTIME_DIR:STRING=${Slicer_INSTALL_LIB_DIR}
    -DITK_INSTALL_LIBRARY_DIR:STRING=${Slicer_INSTALL_LIB_DIR}
  DEPENDS ${${proj}_DEPENDENCIES}
)
