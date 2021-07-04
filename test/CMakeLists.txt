find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(TEST_BINARIES)

macro(add_test_binary TEST_BINARY)
  set(TEST_BINARIES ${TEST_BINARIES} ${TEST_BINARY} PARENT_SCOPE)
endmacro(add_test_binary)

add_subdirectory( unit )

add_custom_target(
  check
  USES_TERMINAL
  DEPENDS ${TEST_BINARIES}
  COMMAND ${CMAKE_CTEST_COMMAND}
  COMMAND ${PYTHON3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run_tests.py --columbo-binary ${CMAKE_CURRENT_BINARY_DIR}/../src/columbo
)
