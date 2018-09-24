set(ROOT_DAQDB_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
set(cmake_generated bin
                    CMakeCache.txt
                    cmake_install.cmake
                    Makefile
                    CMakeFiles
                    Testing
                    CTestTestfile.cmake
                    DartConfiguration.tcl
)
set(folders_to_clean . apps apps/minidaq apps/mist
	examples examples/basic examples/cli_node examples/fabric_node
	tests
	third-party)

file(REMOVE_RECURSE ${ROOT_DAQDB_DIR}/bin)
foreach(file ${cmake_generated})
	foreach(folder ${folders_to_clean})
		if (EXISTS ${ROOT_DAQDB_DIR}/${folder}/${file})
			file(REMOVE_RECURSE ${ROOT_DAQDB_DIR}/${folder}/${file})
		endif()
	endforeach(folder)
endforeach(file)
