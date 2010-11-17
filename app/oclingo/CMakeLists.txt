file(GLOB_RECURSE OCLINGO_SRC *.cpp *.h)

add_library(appoclingo-lib STATIC ${OCLINGO_SRC})
set_target_properties(appoclingo-lib PROPERTIES OUTPUT_NAME appoclingo)
