include_directories(${CMAKE_SOURCE_DIR}/libgringo ${CMAKE_SOURCE_DIR}/libclingcon ${CMAKE_SOURCE_DIR}/libprogram_opts ${CMAKE_SOURCE_DIR}/libclasp .)

add_subdirectory(gringo)
add_subdirectory(clingo)
if(WITH_CLINGCON)
	add_subdirectory(clingcon)
endif()
add_subdirectory(oclingo)

set(gringo-libs gringo-lib program_opts-lib ${Boost_LIBRARIES})
if(WITH_LUA STREQUAL "system")
	set(gringo-libs ${gringo-libs} ${LUA_LIBRARIES})
elseif(WITH_LUA STREQUAL "shipped")
	if(WITH_LUASQL)
		set(gringo-libs ${gringo-libs} luasql-lib)
	endif()
	set(gringo-libs ${gringo-libs} lua-lib)
endif()

add_executable(gringo-app gringo.cpp)
target_link_libraries(gringo-app appgringo-lib ${gringo-libs})
set_target_properties(gringo-app PROPERTIES OUTPUT_NAME gringo)

add_executable(clingo-app clingo.cpp)
target_link_libraries(clingo-app appclingo-lib appgringo-lib clasp-lib ${gringo-libs})
set_target_properties(clingo-app PROPERTIES OUTPUT_NAME clingo)

add_executable(iclingo-app iclingo.cpp)
target_link_libraries(iclingo-app appclingo-lib appgringo-lib clasp-lib ${gringo-libs})
set_target_properties(iclingo-app PROPERTIES OUTPUT_NAME iclingo)

if(WITH_CLINGCON)
	add_executable(clingcon-app clingcon.cpp)
	target_link_libraries(clingcon-app appclingcon-lib appgringo-lib clasp-lib clingcon-lib ${gringo-libs} clingcon-lib)
	set_target_properties(clingcon-app PROPERTIES OUTPUT_NAME clingcon)
endif()

add_executable(oclingo-app oclingo.cpp)
target_link_libraries(oclingo-app appoclingo-lib appclingo-lib appgringo-lib clasp-lib ${gringo-libs})
set_target_properties(oclingo-app PROPERTIES OUTPUT_NAME oclingo)

add_subdirectory(test)
