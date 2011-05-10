file(GLOB_RECURSE CLINGCON_SRC *.cpp *.h)

add_library(appclingcon-lib STATIC ${CLINGCON_SRC})
set_target_properties(appclingcon-lib PROPERTIES OUTPUT_NAME appclingcon)
