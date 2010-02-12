project (GrinGo)

cmake_minimum_required(VERSION 2.4)
cmake_policy(SET CMP0005 OLD)

IF(NOT MSVC)
	include (cmake/pch.cmake)
ENDIF(NOT MSVC)
include (cmake/macros.cmake)
ENSURE_OUT_OF_SOURCE_BUILD("you have to create an out-of-source build")

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")

SET(CPACK_PACKAGE_VERSION "2.0.4")
SET(CPACK_PACKAGE_VERSION_MAJOR "2")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "4")
# that sucks this is nowhere documented!!!
SET(CPACK_SOURCE_IGNORE_FILES "/build/" "/examples/" "/bin/" "/\\\\.svn/" "release.sh" "build.sh" "TODO")
SET(CPACK_SOURCE_GENERATOR "TGZ")
include (CPack)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
set(EXECUTABLE_OUTPUT_PATH ${GrinGo_BINARY_DIR}/bin CACHE PATH "Single output directory for building all executables." FORCE)
set(LIBRARY_OUTPUT_PATH ${GrinGo_BINARY_DIR}/lib CACHE PATH "Single output directory for building all libraries." FORCE)

IF(MSVC)
	set(CMAKE_CXX_FLAGS_DEBUG "/Od /D \"_DEBUG\" /D \"_SCL_SECURE_NO_WARNINGS\" /D \"_UNICODE\" /D \"UNICODE\" /Gm /EHsc /RTC1 /MDd /W3 /nologo /c /ZI /TP" CACHE STRING "Flags used by the compiler during debug builds." FORCE)
	set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Oi /GL /D \"NDEBUG\" /D \"_UNICODE\" /D \"UNICODE\" /FD /EHsc /MD /Gy /W3 /nologo /c /Zi /TP" CACHE STRING "Flags used by the compiler during release builds" FORCE)
	ADD_DEFINITIONS(/D \"HAS_TR1_UNORDERED=1\" /D \"NS_TR1=std::tr1\")
ENDIF()

set(WITH_CLASP OFF CACHE BOOL "Enable this flag to build GrinGo with an internal clasp version.")
set(WITH_ICLASP OFF CACHE BOOL "Enable this flag to build GrinGo with an incremental clasp interface.")
if(WITH_ICLASP)
	set(WITH_CLASP ON CACHE BOOL "Enable this flag to build GrinGo with an internal clasp version.")
endif()

# add macros for version information
ADD_DEFINITIONS(-DPACK_VERSION="\\"${CPACK_PACKAGE_VERSION}\\"")
ADD_DEFINITIONS(-DGRINGO_VERSION="\\"${CPACK_PACKAGE_VERSION}\\"")
ADD_DEFINITIONS(-DCLASP_VERSION="\\"1.3.1\\"")
if(WITH_CLASP)
	ADD_DEFINITIONS(-DWITH_CLASP)
endif()

if(WITH_ICLASP)
	ADD_DEFINITIONS(-DWITH_ICLASP)
endif()

if(WITH_ICLASP)
	ADD_DEFINITIONS(-DEXECUTABLE="\\"iclingo\\"")
else()
	if(WITH_CLASP)
		ADD_DEFINITIONS(-DEXECUTABLE="\\"clingo\\"")
	else()
		ADD_DEFINITIONS(-DEXECUTABLE="\\"gringo\\"")
	endif()
endif()

add_subdirectory (app)
add_subdirectory (libgringo)
add_subdirectory (libprogram_opts)
if(WITH_CLASP)
	add_subdirectory (libclasp)
endif(WITH_CLASP)

