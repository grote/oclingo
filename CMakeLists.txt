project (GrinGo)

cmake_minimum_required(VERSION 2.4)
cmake_policy(SET CMP0005 OLD)

IF(NOT MSVC)
	include (pch.cmake)
ENDIF(NOT MSVC)
include (macros.cmake)
ENSURE_OUT_OF_SOURCE_BUILD("you have to create an out-of-source build")

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")
#FIND_PACKAGE(RE2C REQUIRED)

SET(CPACK_PACKAGE_VERSION "2.0.4")
SET(CPACK_PACKAGE_VERSION_MAJOR "2")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "4")
# that sucks this is nowhere documented!!!
SET(CPACK_SOURCE_IGNORE_FILES "/build/" "/examples/" "/bin/" "/\\\\.svn/" "release.sh" "build.sh" "TODO")
SET(CPACK_SOURCE_GENERATOR "TGZ")
include (CPack)

set(EXECUTABLE_OUTPUT_PATH ${GrinGo_BINARY_DIR}/bin CACHE PATH "Single output directory for building all executables." FORCE)
set(LIBRARY_OUTPUT_PATH ${GrinGo_BINARY_DIR}/lib CACHE PATH "Single output directory for building all libraries." FORCE)


if(LINK_STATIC)
	set(CMAKE_CXX_FLAGS_DEBUG "-g -D_GLIBCXX_DEBUG -D_DEBUG -O0 -Wall -Wno-deprecated -static" CACHE STRING "Flags used by the compiler during debug builds." FORCE)
	set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -Wno-deprecated -static" CACHE STRING "Flags used by the compiler during release builds" FORCE)
else(LINK_STATIC)
	set(CMAKE_CXX_FLAGS_DEBUG "-g -D_GLIBCXX_DEBUG -D_DEBUG -O0 -Wall -Wno-deprecated" CACHE STRING "Flags used by the compiler during debug builds." FORCE)
	set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -Wno-deprecated" CACHE STRING "Flags used by the compiler during release builds" FORCE)
endif(LINK_STATIC)

IF(MSVC)
	set(CMAKE_CXX_FLAGS_DEBUG "/Od /D \"_DEBUG\" /D \"_SCL_SECURE_NO_WARNINGS\" /D \"_UNICODE\" /D \"UNICODE\" /Gm /EHsc /RTC1 /MDd /W3 /nologo /c /ZI /TP" CACHE STRING "Flags used by the compiler during debug builds." FORCE)
	set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Oi /GL /D \"NDEBUG\" /D \"_UNICODE\" /D \"UNICODE\" /FD /EHsc /MD /Gy /W3 /nologo /c /Zi /TP" CACHE STRING "Flags used by the compiler during release builds" FORCE)
	ADD_DEFINITIONS(/D \"HAS_TR1_UNORDERED=1\" /D \"NS_TR1=std::tr1\")
ENDIF(MSVC)



if(NOT CMAKE_VERBOSE_MAKEFILE)
	set(CMAKE_VERBOSE_MAKEFILE OFF CACHE BOOL "If this value is on, makefiles will be generated without the .SILENT directive, and all commands will be echoed to the console during the make.  This is useful for debugging only. With Visual Studio IDE projects all commands are done without /nologo" FORCE)
endif(NOT CMAKE_VERBOSE_MAKEFILE)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

set(WITH_CLASP OFF CACHE BOOL "Enable this flag to build GrinGo with an internal clasp version.")

set(WITH_ICLASP OFF CACHE BOOL "Enable this flag to build GrinGo with an incremental clasp interface.")
if(WITH_ICLASP)
	set(WITH_CLASP ON CACHE BOOL "Enable this flag to build GrinGo with an internal clasp version.")
endif(WITH_ICLASP)

# add macros for version information
ADD_DEFINITIONS(-DPACK_VERSION="\\"${CPACK_PACKAGE_VERSION}\\"")
ADD_DEFINITIONS(-DGRINGO_VERSION="\\"${CPACK_PACKAGE_VERSION}\\"")
ADD_DEFINITIONS(-DCLASP_VERSION="\\"1.3.1\\"")
if(WITH_CLASP)
	ADD_DEFINITIONS(-DWITH_CLASP)
endif(WITH_CLASP)

if(WITH_ICLASP)
	ADD_DEFINITIONS(-DWITH_ICLASP)
endif(WITH_ICLASP)

if(WITH_ICLASP)
	ADD_DEFINITIONS(-DEXECUTABLE="\\"iclingo\\"")
else(WITH_ICLASP)
	if(WITH_CLASP)
		ADD_DEFINITIONS(-DEXECUTABLE="\\"clingo\\"")
	else(WITH_CLASP)
		ADD_DEFINITIONS(-DEXECUTABLE="\\"gringo\\"")
	endif(WITH_CLASP)
endif(WITH_ICLASP)

add_subdirectory (app)
add_subdirectory (lib)

