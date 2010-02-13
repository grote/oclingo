file(GLOB LIBGRINGO_INC gringo/*.h)
file(GLOB LIBGRINGO_SRC src/*.cpp)
 
if(${GRINGO_TYPE} STREQUAL "gringo")
	include_directories(${CMAKE_CURRENT_BINARY_DIR}	${CMAKE_CURRENT_BINARY_DIR}/config gringo generated)
else()
	include_directories(${CMAKE_CURRENT_BINARY_DIR}	${CMAKE_CURRENT_BINARY_DIR}/config gringo generated  ../libclasp)
endif()

RE2C(src/lparselexer lparselexer LIBGRINGO_SRC)
LEMON(src/lparseparser_impl lparseparser_impl LIBGRINGO_SRC)

RE2C(src/plainlparselexer plainlparselexer LIBGRINGO_SRC)
LEMON(src/lparseconverter_impl lparseconverter_impl LIBGRINGO_SRC)

add_library(gringo-lib STATIC ${LIBGRINGO_SRC} ${LIBGRINGO_INC})

target_link_libraries (gringo-lib)
set_target_properties(gringo-lib PROPERTIES OUTPUT_NAME gringo)

if(PCHSupport_FOUND)
	add_precompiled_header(gringo-lib ${CMAKE_CURRENT_SOURCE_DIR}/gringo/gringo.h)
endif()

