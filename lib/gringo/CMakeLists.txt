file(GLOB LIBGRINGO_SRC src/*.cpp)
SET(LIBGRINGO_SRC 
	${LIBGRINGO_SRC} 
	${CMAKE_CURRENT_BINARY_DIR}/lparselexer.cpp 
	${CMAKE_CURRENT_BINARY_DIR}/lparseparser_impl.cpp
	${CMAKE_CURRENT_BINARY_DIR}/plainlparselexer.cpp 
	${CMAKE_CURRENT_BINARY_DIR}/lparseconverter_impl.cpp)
#SET_SOURCE_FILES_PROPERTIES(lparselexer.cpp GENERATED)
#SET_SOURCE_FILES_PROPERTIES(lparseparser_impl.cpp GENERATED)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/config include ..)

ADD_EXECUTABLE(lemon src/lemon.c)

MACRO(RE2C SRC DST)
	ADD_CUSTOM_COMMAND(
		OUTPUT ${DST}
		COMMAND ${RE2C_EXECUTABLE} -o ${DST} ${SRC}
		MAIN_DEPENDENCY ${SRC}
	)
ENDMACRO(RE2C SRC DEST)

MACRO(LEMON NAME SRC DST)
	ADD_CUSTOM_COMMAND(
		OUTPUT ${DST}/${NAME}.cpp ${DST}/${NAME}.h
		COMMAND cp ${SRC}/lempar.c ${DST}/lempar.c
		COMMAND ${EXECUTABLE_OUTPUT_PATH}/lemon -q ${SRC}/${NAME}.y
		COMMAND rm ${DST}/lempar.c
		COMMAND mv ${SRC}/${NAME}.c ${DST}/${NAME}.cpp
		COMMAND mv ${SRC}/${NAME}.h ${DST}/${NAME}.h
		COMMAND touch ${DST}/${NAME}.h ${DST}/${NAME}.cpp
		MAIN_DEPENDENCY ${SRC}/${NAME}.y
		DEPENDS lemon ${SRC}/lempar.c
	)
ENDMACRO(LEMON NAME SRC DST)

RE2C(${CMAKE_CURRENT_SOURCE_DIR}/src/lparselexer.re ${CMAKE_CURRENT_BINARY_DIR}/lparselexer.cpp)
LEMON(lparseparser_impl ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})

RE2C(${CMAKE_CURRENT_SOURCE_DIR}/src/plainlparselexer.re ${CMAKE_CURRENT_BINARY_DIR}/plainlparselexer.cpp)
LEMON(lparseconverter_impl ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})

add_library(gringo-lib STATIC ${LIBGRINGO_SRC})
target_link_libraries (gringo-lib)
set_target_properties(gringo-lib PROPERTIES OUTPUT_NAME gringo)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/include/gringo.h.in ${CMAKE_CURRENT_BINARY_DIR}/config/gringo.h)

IF(PCHSupport_FOUND)
	add_precompiled_header(gringo-lib ${CMAKE_CURRENT_BINARY_DIR}/config/gringo.h)
ENDIF(PCHSupport_FOUND)



