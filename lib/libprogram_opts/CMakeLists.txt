file(GLOB LIBPROGRAM_OPTS_SRC src/*.cpp)

include_directories(.)

add_library(program_opts-lib STATIC ${LIBPROGRAM_OPTS_SRC})
set_target_properties(program_opts-lib PROPERTIES OUTPUT_NAME libprogram_opts)


