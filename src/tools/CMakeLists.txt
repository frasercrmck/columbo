set( src_files
  ../defs.cpp
  ../cage.cpp
  ../combinations.cpp
  ../utils.cpp
  ../tools/columbo-cli.cpp
  ../printers/ncurses_printer.cpp
)

include_directories( ${CURSES_INCLUDE_DIRS} )

add_executable( columbo-cli ${src_files} )
target_link_libraries( columbo-cli ${CURSES_LIBRARIES} )
