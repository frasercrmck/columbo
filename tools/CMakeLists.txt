set( src_files
  ../defs.cpp
  ../cage.cpp
  ../utils.cpp
  ../tools/columbo-cli.cpp
  ../printers/ncurses_printer.cpp
)

include_directories( ${CURSES_INCLUDE_DIRS} )

add_executable( columbo-cli ${src_files} )
target_link_libraries( columbo-cli ${CURSES_LIBRARIES} )

set_target_properties( columbo-cli PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
)
