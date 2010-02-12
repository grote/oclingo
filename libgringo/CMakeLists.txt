file(GLOB LIBGRINGO_INC include/*.h)
file(GLOB LIBGRINGO_SRC src/*.cpp)
 
if (WITH_CLASP)
	include_directories(${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_BINARY_DIR}/config gringo generated  ../libclasp)
else (WITH_CLASP)
	include_directories(${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_BINARY_DIR}/config gringo generated)
endif (WITH_CLASP)

include(macros.cmake)

ADD_EXECUTABLE(lemon src/lemon.c)
if (MSVC)
	SET_TARGET_PROPERTIES(lemon PROPERTIES PREFIX "../") 
endif ()


RE2C(lparselexer ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR} LIBGRINGO_SRC)
LEMON(lparseparser_impl ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR} LIBGRINGO_SRC)

RE2C(plainlparselexer ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR} LIBGRINGO_SRC)
LEMON(lparseconverter_impl ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR} LIBGRINGO_SRC)

add_library(gringo-lib STATIC ${LIBGRINGO_SRC} ${LIBGRINGO_INC})

target_link_libraries (gringo-lib)
set_target_properties(gringo-lib PROPERTIES OUTPUT_NAME gringo)

IF(PCHSupport_FOUND)
	add_precompiled_header(gringo-lib ${CMAKE_CURRENT_SOURCE_DIR}/gringo/gringo.h)
ENDIF(PCHSupport_FOUND)

