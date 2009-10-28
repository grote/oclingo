file(GLOB LIBGRINGO_SRC src/*.cpp)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/config include generated ..)
if (MSVC)
ADD_DEFINITIONS(/D \"HAS_TR1_UNORDERED=1\" /D \"NS_TR1=std::tr1\")
endif (MSVC)

include(../macros.cmake)


ADD_EXECUTABLE(lemon src/lemon.c)
if (MSVC)
SET_TARGET_PROPERTIES(lemon PROPERTIES PREFIX "../") 
endif (MSVC)


RE2C(lparselexer ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR} LIBGRINGO_SRC)
LEMON(lparseparser_impl ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR} LIBGRINGO_SRC)

RE2C(plainlparselexer ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR} LIBGRINGO_SRC)
LEMON(lparseconverter_impl ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR} LIBGRINGO_SRC)

add_library(gringo-lib STATIC ${LIBGRINGO_SRC})
target_link_libraries (gringo-lib)
set_target_properties(gringo-lib PROPERTIES OUTPUT_NAME gringo)

#CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/include/gringo.h.in ${CMAKE_CURRENT_BINARY_DIR}/config/gringo.h)

IF(PCHSupport_FOUND)
	add_precompiled_header(gringo-lib ${CMAKE_CURRENT_SOURCE_DIR}/include/gringo.h)
ENDIF(PCHSupport_FOUND)



