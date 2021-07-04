set( SOURCES
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

add_executable( columbo ${SOURCES} )

add_library( columbo_lib STATIC ${SOURCES} )

set( CURSES_NEED_WIDE TRUE )
find_package( Curses )

option( COLUMBO_BUILD_TOOL_COLUMBO_CLI
        "Build the ncurses cli tool for describing sudokus"
        ${CURSES_FOUND}
)

if( ${CURSES_FOUND} AND ${COLUMBO_BUILD_TOOL_COLUMBO_CLI} )
  add_subdirectory( tools )
endif()
