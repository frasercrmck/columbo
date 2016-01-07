cmake_minimum_required( VERSION 3.0 )

project( solver )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 \
  -Weverything \
  -Wno-c++98-compat \
  -Wno-c++98-c++11-compat-pedantic \
  -Wno-missing-prototypes \
  -Wno-padded"
)

set( src_files
  solver.cpp
  cage.cpp
  utils.cpp
  defs.cpp
)

add_executable( solver ${src_files} )

set_target_properties( solver PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
)
