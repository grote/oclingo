file(GLOB_RECURSE OCLINGO_SRC    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.h)
file(GLOB_RECURSE OCLINGO_LEXER  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ./*.r2c)
file(GLOB_RECURSE OCLINGO_PARSER RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ./*.y)

RE2C(OCLINGO_SRC ${OCLINGO_LEXER})
LEMON(OCLINGO_SRC ${OCLINGO_PARSER})

add_library(appoclingo-lib STATIC ${OCLINGO_SRC})
set_target_properties(appoclingo-lib PROPERTIES OUTPUT_NAME appoclingo)
if(WIN32)
	target_link_libraries(appoclingo-lib ws2_32)
else()
	target_link_libraries(appoclingo-lib pthread)
endif()

