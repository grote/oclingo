IF(NOT RE2C_FOUND)
	FIND_PROGRAM (RE2C_EXE NAMES re2c)

	IF (RE2C_EXE)
		SET(RE2C_FOUND TRUE CACHE BOOL "Whether re2c has been found")
	ENDIF ()

	IF (RE2C_FOUND)
		IF (NOT RE2C_FIND_QUIETLY)
			MESSAGE(STATUS "Looking for re2c... - found ${RE2C_EXECUTABLE}")
		ENDIF ()
	ELSE ()
		IF (RE2C_FIND_REQUIRED)
			MESSAGE(FATAL_ERROR "Looking for re2c... - NOT found")
		ENDIF ()
		MESSAGE(STATUS "Looking for re2c... - NOT found")
	ENDIF ()
ENDIF()

IF(RE2C_FOUND)
	SET(RE2C_EXECUTABLE ${RE2C_EXE})
ENDIF()
