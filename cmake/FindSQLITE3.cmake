IF(NOT SQLITE3_FOUND)
	IF(NOT "$ENV{SQLITE3_ROOT}" STREQUAL "")
		SET(SQLITE3_ROOT "$ENV{SQLITE3_ROOT}")
	ENDIF()

	SET(LIBS sqlite3)
	IF(SQLITE3_USE_STATIC_LIBS OR USE_STATIC_LIBS)
		SET(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a)
		SET(LIBS ${LIBS} pthread dl)
	ENDIF()

	FIND_PATH(SQLITE3_INCLUDE sqlite3.h HINTS ${SQLITE3_ROOT}/include)
	FOREACH(LIB ${LIBS})
		FIND_LIBRARY(SQLITE3_${LIB}_LIB ${LIB} HINTS ${SQLITE3_ROOT}/lib)
		IF(SQLITE3_${LIB}_LIB)
			SET(SQLITE3_LIBS ${SQLITE3_LIBS} ${SQLITE3_${LIB}_LIB})
		ELSE()
			SET(ERROR TRUE)
		ENDIF()
	ENDFOREACH()

	IF(SQLITE3_INCLUDE AND NOT ERROR)
		SET(SQLITE3_LIBS ${SQLITE3_LIBS} CACHE INTERNAL "")
		SET(SQLITE3_FOUND TRUE CACHE BOOL "Whether sqlite has been found")
	ENDIF()

	IF(SQLITE3_FOUND)
		IF (NOT SQLITE3_FIND_QUIETLY)
			MESSAGE(STATUS "Looking for sqlite3... - found ${SQLITE3_LIB}")
		ENDIF (NOT SQLITE3_FIND_QUIETLY)
	ELSE()
		IF (SQLITE3_FIND_REQUIRED)
			MESSAGE(FATAL_ERROR "Looking for sqlite3... - NOT found")
		ENDIF (SQLITE3_FIND_REQUIRED)
		MESSAGE(STATUS "Looking for sqlite3... - NOT found")
	ENDIF()
ENDIF()

IF(SQLITE3_FOUND)
	SET(SQLITE3_INCLUDE_DIR ${SQLITE3_INCLUDE})
	SET(SQLITE3_LIBRARIES ${SQLITE3_LIBS})
ENDIF()
