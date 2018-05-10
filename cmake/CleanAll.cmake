set(cmake_generated ${CMAKE_BINARY_DIR}/bin
					${CMAKE_BINARY_DIR}/CMakeCache.txt
                    ${CMAKE_BINARY_DIR}/cmake_install.cmake  
                    ${CMAKE_BINARY_DIR}/Makefile
                    ${CMAKE_BINARY_DIR}/CMakeFiles
                    ${CMAKE_BINARY_DIR}/Testing
                    ${CMAKE_BINARY_DIR}/CTestTestfile.cmake
                    ${CMAKE_BINARY_DIR}/DartConfiguration.tcl
                    ${CMAKE_BINARY_DIR}/third-party/CMakeFiles
                    ${CMAKE_BINARY_DIR}/third-party/cmake_install.cmake
                    ${CMAKE_BINARY_DIR}/third-party/MakeFile
                    ${CMAKE_BINARY_DIR}/third-party/CTestTestfile.cmake
                    ${CMAKE_BINARY_DIR}/tests/CMakeFiles
                    ${CMAKE_BINARY_DIR}/tests/cmake_install.cmake
                    ${CMAKE_BINARY_DIR}/tests/MakeFile
                    ${CMAKE_BINARY_DIR}/tests/CTestTestfile.cmake
                    ${CMAKE_BINARY_DIR}/examples/cli_node/CMakeFiles
                    ${CMAKE_BINARY_DIR}/examples/cli_node/cmake_install.cmake
                    ${CMAKE_BINARY_DIR}/examples/cli_node/Makefile
                    ${CMAKE_BINARY_DIR}/examples/cli_node/CTestTestfile.cmake                   
)

foreach(file ${cmake_generated})
  if (EXISTS ${file})
     file(REMOVE_RECURSE ${file})
  endif()
endforeach(file)