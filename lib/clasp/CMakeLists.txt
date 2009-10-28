file(GLOB LIBCLASP_SRC src/*.cpp)
include_directories(..)
if (MSVC)
file(GLOB LIBCLASP_HEAD include/*.h)
add_library(clasp-lib STATIC ${LIBCLASP_SRC} ${LIBCLASP_HEAD})
else (MSVC)
add_library(clasp-lib STATIC ${LIBCLASP_SRC})
endif (MSVC)

set_target_properties(clasp-lib PROPERTIES OUTPUT_NAME clasp)

