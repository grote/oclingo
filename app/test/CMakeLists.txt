find_package(Boost COMPONENTS unit_test_framework)

if(Boost_UNIT_TEST_FRAMEWORK_FOUND)
    if("${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}" MATCHES ".*\\.((so)|(dll)|(dynlib))$")
            add_definitions("-DBOOST_TEST_DYN_LINK")
    endif()

    file(GLOB_RECURSE TEST_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.h)

    add_executable(test-app ../test.cpp ${TEST_SRC})
    target_link_libraries(test-app appclingo-lib appgringo-lib clasp-lib gringo-lib ${Boost_LIBRARIES})
    set_target_properties(test-app PROPERTIES OUTPUT_NAME test)
endif()
