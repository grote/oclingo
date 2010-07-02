file(GLOB_RECURSE LIBGRINGO_SRC    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.cpp *.h)
file(GLOB_RECURSE LIBGRINGO_LEXER  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.r2c)
file(GLOB_RECURSE LIBGRINGO_PARSER RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.y)

include_directories(src .)

if(WITH_LUA STREQUAL "system")
	add_definitions("-DWITH_LUA=1")
	include_directories(${LUA_INCLUDE_DIR})
elseif(WITH_LUA STREQUAL "shipped")
	add_definitions("-DWITH_LUA=2")
	include_directories(${CMAKE_SOURCE_DIR}/liblua)
	if(WITH_LUASQL)
		add_definitions("-DWITH_LUASQL")
		include_directories(${CMAKE_SOURCE_DIR}/libluasql ${CMAKE_BINARY_DIR}/libluasql)
	endif()
endif()

RE2C(LIBGRINGO_SRC ${LIBGRINGO_LEXER})
LEMON(LIBGRINGO_SRC ${LIBGRINGO_PARSER})

add_library(gringo-lib STATIC ${LIBGRINGO_SRC})

target_link_libraries(gringo-lib)
set_target_properties(gringo-lib PROPERTIES OUTPUT_NAME gringo)

ADD_PRECOMPILED_HEADER(gringo-lib ${CMAKE_SOURCE_DIR}/libgringo/gringo/gringo.h)
