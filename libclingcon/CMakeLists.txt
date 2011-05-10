#file(GLOB_RECURSE LIBGRINGO_SRC    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.cpp *.h)
#file(GLOB_RECURSE LIBGRINGO_LEXER  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.r2c)
#file(GLOB_RECURSE LIBGRINGO_PARSER RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.y)
file(GLOB_RECURSE LIBCLINGCON_SRC    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.cpp *.h)
file(GLOB_RECURSE LIBCLINGCON_LEXER  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.r2c)
file(GLOB_RECURSE LIBCLINGCON_PARSER RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.y)

include_directories(src .)


file(GLOB LIBCLINGCON_SRC src/*.cpp)
file(GLOB LIBCLINGCON_INC clingcon/*.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR} clingcon ../libclasp ../libgringo .)

RE2C(LIBCLINGCON_SRC ${LIBCLINGCON_LEXER})
LEMON(LIBCLINGCON_SRC ${LIBCLINGCON_PARSER})
#RE2C(src/lparsecsplexer lparsecsplexer LIBCLINGCON_SRC)
#LEMON(src/lparsecspparser_impl lparsecspparser_impl LIBCLINGCON_SRC)

add_library(clingcon-lib STATIC ${LIBCLINGCON_SRC} ${LIBCLINGCON_INC})
target_link_libraries (clingcon-lib gringo-lib clasp-lib ${GECODE_LIBRARIES})
set_target_properties(clingcon-lib PROPERTIES OUTPUT_NAME clingcon)

#IF(PCHSupport_FOUND)
#	add_precompiled_header(gringo-lib ${CMAKE_CURRENT_SOURCE_DIR}/gringo/gringo.h)
#ENDIF(PCHSupport_FOUND)

#install(FILES ${LIBCLINGCON_INC} DESTINATION include/clingcon)
#install(TARGETS clingcon-lib ARCHIVE DESTINATION lib)
