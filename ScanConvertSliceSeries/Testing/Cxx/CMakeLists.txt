
#-----------------------------------------------------------------------------
set(BASELINE ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/Baseline)
set(INPUT ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/Input)
set(TEMP "${CMAKE_BINARY_DIR}/Testing/Temporary")

set(CLP ${MODULE_NAME})

#-----------------------------------------------------------------------------
add_executable(${CLP}Test ${CLP}Test.cxx)
target_link_libraries(${CLP}Test ${CLP}Lib ${SlicerExecutionModel_EXTRA_EXECUTABLE_TARGET_LIBRARIES})
set_target_properties(${CLP}Test PROPERTIES LABELS ${CLP})

#-----------------------------------------------------------------------------
set(testname ${CLP}Test)
ExternalData_add_test(${CLP}Data NAME ${testname} COMMAND
  ${SEM_LAUNCH_COMMAND} $<TARGET_FILE:${CLP}Test>
  --compare DATA{${BASELINE}/${CLP}TestOutput.mha}
    ${TEMP}/${CLP}TestOutput.mha
  ModuleEntryPoint
    --outputSpacing 1.0,1.0,1.0
    DATA{${INPUT}/bmode_p59.hdf5}
    ${TEMP}/${CLP}TestOutput.mha
  )
set_property(TEST ${testname} PROPERTY LABELS ${CLP})

#-----------------------------------------------------------------------------
ExternalData_add_target(${CLP}Data)
