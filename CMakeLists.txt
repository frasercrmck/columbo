cmake_minimum_required( VERSION 3.3 )

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

get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if(isMultiConfig)
    if(NOT "Asan" IN_LIST CMAKE_CONFIGURATION_TYPES)
        list(APPEND CMAKE_CONFIGURATION_TYPES Asan)
    endif()
else()
    set(allowedBuildTypes Asan Debug Release RelWithDebInfo MinSizeRel)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "${allowedBuildTypes}")

    if(CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE IN_LIST allowedBuildTypes)
        message(FATAL_ERROR "Invalid build type: ${CMAKE_BUILD_TYPE}")
    endif()
endif()

set(CMAKE_C_FLAGS_ASAN
    "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer" CACHE STRING
    "Flags used by the C compiler for Asan build type or configuration." FORCE)

set(CMAKE_CXX_FLAGS_ASAN
    "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer" CACHE STRING
    "Flags used by the C++ compiler for Asan build type or configuration." FORCE)

set(CMAKE_EXE_LINKER_FLAGS_ASAN
    "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -fsanitize=address" CACHE STRING
    "Linker flags to be used to create executables for Asan build type." FORCE)

set(CMAKE_SHARED_LINKER_FLAGS_ASAN
    "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -fsanitize=address" CACHE STRING
    "Linker lags to be used to create shared libraries for Asan build type." FORCE)

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
  combinations.cpp
  printers/terminal_printer.cpp
)

add_executable( columbo ${src_files} )

set( CURSES_NEED_WIDE TRUE )
find_package( Curses )

option( COLUMBO_BUILD_TOOL_COLUMBO_CLI
        "Build the ncurses cli tool for describing sudokus"
        ${CURSES_FOUND}
)

if( ${CURSES_FOUND} AND ${COLUMBO_BUILD_TOOL_COLUMBO_CLI} )
  add_subdirectory( tools )
endif()
