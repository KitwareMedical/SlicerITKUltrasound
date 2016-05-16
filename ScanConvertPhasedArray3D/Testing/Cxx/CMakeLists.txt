
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
ExternalData_add_test(${CLP}Data NAME ${testname} COMMAND ${SEM_LAUNCH_COMMAND} $<TARGET_FILE:${CLP}Test>
  --compare DATA{${BASELINE}/${CLP}TestOutput.mha}
    ${TEMP}/${CLP}TestOutput.mha
  ModuleEntryPoint
    --azimuthAngularSeparation 0.0872665
    --elevationAngularSeparation 0.0174533
    --radiusSampleSize 0.2
    --firstSampleDistance 8.0
    --outputSize 128,128,128
    --outputSpacing 0.2,0.2,0.2
    DATA{${INPUT}/${CLP}TestInput.mha}
    ${TEMP}/${CLP}TestOutput.mha
  )
set_property(TEST ${testname} PROPERTY LABELS ${CLP})

set(testname ${CLP}ITKNearestNeighborTest)
ExternalData_add_test(${CLP}Data NAME ${testname} COMMAND ${SEM_LAUNCH_COMMAND} $<TARGET_FILE:${CLP}Test>
  --compare DATA{${BASELINE}/${testname}Output.mha}
    ${TEMP}/${testname}Output.mha
  ModuleEntryPoint
    --azimuthAngularSeparation 0.0872665
    --elevationAngularSeparation 0.0174533
    --radiusSampleSize 0.2
    --firstSampleDistance 8.0
    --outputSize 128,128,128
    --outputSpacing 0.2,0.2,0.2
    --method ITKNearestNeighbor
    DATA{${INPUT}/${CLP}TestInput.mha}
    ${TEMP}/${testname}Output.mha
  )
set_property(TEST ${testname} PROPERTY LABELS ${CLP})

#-----------------------------------------------------------------------------
ExternalData_add_target(${CLP}Data)
