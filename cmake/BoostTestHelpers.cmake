function(add_boost_test SOURCE_FILE_NAME)
	get_filename_component(TEST_EXECUTABLE_NAME ${SOURCE_FILE_NAME} NAME_WE)

	add_executable(${TEST_EXECUTABLE_NAME} ${SOURCE_FILE_NAME})
	set(Dpdk_LIBRARIES -Wl,--whole-archive dpdk -Wl,--no-whole-archive)
	target_link_libraries(${TEST_EXECUTABLE_NAME} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} 
		fabric fogkv pmemobj ${Dpdk_LIBRARIES} dl numa
		${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

	file(READ "${SOURCE_FILE_NAME}" SOURCE_FILE_CONTENTS)
	string(REGEX MATCHALL "BOOST_AUTO_TEST_CASE\\( *([A-Za-z_0-9]+) *\\)" 
		FOUND_TESTS ${SOURCE_FILE_CONTENTS})
	
	foreach(HIT ${FOUND_TESTS})
		string(REGEX REPLACE ".*\\( *([A-Za-z_0-9]+) *\\).*" "\\1" TEST_NAME ${HIT})

		add_test(NAME "${TEST_EXECUTABLE_NAME}.${TEST_NAME}" 
			COMMAND ${TEST_EXECUTABLE_NAME} --run_test=${TEST_NAME} --log_level=all --catch_system_error=yes)
	endforeach()

	string(REGEX MATCHALL "BOOST_FIXTURE_TEST_CASE\\(([A-Za-z_0-9]+), [A-Za-z_0-9:]+\\)" 
		FOUND_TESTS_F ${SOURCE_FILE_CONTENTS})

	foreach(HIT ${FOUND_TESTS_F})
		string(REGEX REPLACE "BOOST_FIXTURE_TEST_CASE\\(([A-Za-z_0-9]+), [A-Za-z_0-9:]+\\).*" "\\1" TEST_NAME ${HIT})

		add_test(NAME "${TEST_EXECUTABLE_NAME}.${TEST_NAME}" 
			COMMAND ${TEST_EXECUTABLE_NAME} --run_test=${TEST_NAME} --log_level=all --catch_system_error=yes)
	endforeach()

endfunction()