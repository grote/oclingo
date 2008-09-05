file(GLOB LIBCLASP_SRC src/*.cpp)

include_directories(..)

add_library(clasp-lib STATIC ${LIBCLASP_SRC})
set_target_properties(clasp-lib PROPERTIES OUTPUT_NAME clasp)

