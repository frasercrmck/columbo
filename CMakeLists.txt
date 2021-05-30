cmake_minimum_required( VERSION 3.0 )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project( columbo )

#set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z \
#  -Weverything \
#  -Wno-c++98-compat \
#  -Wno-c++98-c++11-compat-pedantic \
#  -Wno-missing-prototypes \
#  -Wno-padded"
#)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )

set( src_files
  columbo.cpp
  cage.cpp
  utils.cpp
  defs.cpp
  all_steps.cpp
  intersections.cpp
  cage_unit_overlap.cpp
  hiddens.cpp
  fixed_cell_cleanup.cpp
  innies_outies.cpp
  killer_combos.cpp
  strategy.cpp
  printers/terminal_printer.cpp
)

add_executable( columbo ${src_files} )

set_target_properties( columbo PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
)

set( CURSES_NEED_WIDE TRUE )
find_package( Curses )

option( COLUMBO_BUILD_TOOL_COLUMBO_CLI
        "Build the ncurses cli tool for describing sudokus"
        ${CURSES_FOUND}
)

if( ${CURSES_FOUND} AND ${COLUMBO_BUILD_TOOL_COLUMBO_CLI} )
  add_subdirectory( tools )
endif()
