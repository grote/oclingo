# Try to find precompiled headers support for GCC 4.x
#
# Variable:
#   PCHSupport_FOUND
#
# Macro:
#   ADD_PRECOMPILED_HEADER  _targetName _input  _dowarn
#   ADD_PRECOMPILED_HEADER_TO_TARGET _targetName _input _pch_output_to_use _dowarn

IF(NOT PCHSupport_CHECKED)
	MESSAGE(STATUS "Checking pch support (optional)")
	IF(CMAKE_COMPILER_IS_GNUCXX)
		STRING(TOUPPER ${CMAKE_BUILD_TYPE} _build_type)
		IF(${_build_type} STREQUAL "DEBUG")
			EXEC_PROGRAM(
				${CMAKE_CXX_COMPILER}  
				ARGS ${CMAKE_CXX_COMPILER_ARG1} -dumpversion 
				OUTPUT_VARIABLE gcc_compiler_version)
			IF(gcc_compiler_version MATCHES "4\\.[4-9]\\.[0-9]")
				SET(PCHSupport_FOUND TRUE)
			ENDIF()
			SET(_PCH_include_prefix "-I")
		ENDIF()
	ELSE(CMAKE_COMPILER_IS_GNUCXX)
		SET(PCHSupport_FOUND FALSE)
	ENDIF(CMAKE_COMPILER_IS_GNUCXX)
	IF(PCHSupport_FOUND)
		MESSAGE(STATUS "Found pch support")
	ELSE()
		MESSAGE(STATUS "No pch support found")
	ENDIF()
	SET(PCHSupport_FOUND ${PCHSupport_FOUND} CACHE BOOL "Whether pch support has neen found")
	SET(PCHSupport_CHECKED TRUE CACHE BOOL "Whether pch support has been checked")
ENDIF()

IF(PCHSupport_FOUND)
	MACRO(_PCH_GET_COMPILE_FLAGS _out_compile_flags)
		STRING(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" _flags_var_name)
		SET(${_out_compile_flags} ${${_flags_var_name}} )

		GET_TARGET_PROPERTY(_targetType ${_PCH_current_target} TYPE)
		IF(${_targetType} STREQUAL SHARED_LIBRARY)
			LIST(APPEND ${_out_compile_flags} "${${_out_compile_flags}} -fPIC")
		ENDIF(${_targetType} STREQUAL SHARED_LIBRARY)

		GET_DIRECTORY_PROPERTY(DIRINC INCLUDE_DIRECTORIES )
		FOREACH(item ${DIRINC})
			LIST(APPEND ${_out_compile_flags} "${_PCH_include_prefix}${item}")
		ENDFOREACH(item)

		GET_DIRECTORY_PROPERTY(_directory_flags DEFINITIONS)
		LIST(APPEND ${_out_compile_flags} ${_directory_flags})
		LIST(APPEND ${_out_compile_flags} ${CMAKE_CXX_FLAGS} )

		SEPARATE_ARGUMENTS(${_out_compile_flags})
	ENDMACRO(_PCH_GET_COMPILE_FLAGS)

	MACRO(_PCH_WRITE_PCHDEP_CXX _targetName _include_file _dephelp)
		SET(${_dephelp} ${CMAKE_CURRENT_BINARY_DIR}/${_targetName}_pch_dephelp.cxx)
		FILE(WRITE ${${_dephelp}}.in
			"#include \"${_include_file}\" 
			int testfunction()
			{
			return 0;
			}
			"
			) 
		configure_file(${${_dephelp}}.in ${${_dephelp}} COPYONLY)
	ENDMACRO(_PCH_WRITE_PCHDEP_CXX )

	MACRO(_PCH_GET_COMPILE_COMMAND out_command _input _output)
		FILE(TO_NATIVE_PATH ${_input} _native_input)
		FILE(TO_NATIVE_PATH ${_output} _native_output)

		IF(CMAKE_CXX_COMPILER_ARG1)
			STRING(REGEX REPLACE "^ +" "" pchsupport_compiler_cxx_arg1 ${CMAKE_CXX_COMPILER_ARG1})
			SET(${out_command} ${CMAKE_CXX_COMPILER} ${pchsupport_compiler_cxx_arg1} ${_compile_FLAGS} -x c++-header -o ${_output} ${_input})
		ELSE(CMAKE_CXX_COMPILER_ARG1)
			SET(${out_command} ${CMAKE_CXX_COMPILER}  ${_compile_FLAGS} -x c++-header -o ${_output} ${_input})
		ENDIF(CMAKE_CXX_COMPILER_ARG1)
	ENDMACRO(_PCH_GET_COMPILE_COMMAND )

	MACRO(_PCH_GET_TARGET_COMPILE_FLAGS _cflags  _header_name _pch_path _dowarn )
		IF (_dowarn)
			SET(${_cflags} "${PCH_ADDITIONAL_COMPILER_FLAGS} -include ${CMAKE_CURRENT_BINARY_DIR}/${_header_name} -Winvalid-pch " )
		ELSE (_dowarn)
			SET(${_cflags} "${PCH_ADDITIONAL_COMPILER_FLAGS} -include ${CMAKE_CURRENT_BINARY_DIR}/${_header_name} " )
		ENDIF (_dowarn)
	ENDMACRO(_PCH_GET_TARGET_COMPILE_FLAGS )

	MACRO(GET_PRECOMPILED_HEADER_OUTPUT _targetName _input _output)
		GET_FILENAME_COMPONENT(_name ${_input} NAME)
		GET_FILENAME_COMPONENT(_path ${_input} PATH)
		SET(_output "${CMAKE_CURRENT_BINARY_DIR}/${_name}.gch/${_targetName}_${CMAKE_BUILD_TYPE}.h++")
	ENDMACRO(GET_PRECOMPILED_HEADER_OUTPUT _targetName _input)

	MACRO(ADD_PRECOMPILED_HEADER_TO_TARGET _targetName _input _pch_output_to_use )
		GET_FILENAME_COMPONENT(_name ${_input} NAME)

		IF( "${ARGN}" STREQUAL "0")
			SET(_dowarn 0)
		ELSE( "${ARGN}" STREQUAL "0")
			SET(_dowarn 1)
		ENDIF("${ARGN}" STREQUAL "0")

		_PCH_GET_TARGET_COMPILE_FLAGS(_target_cflags ${_name} ${_pch_output_to_use} ${_dowarn})
		SET_TARGET_PROPERTIES(${_targetName} PROPERTIES COMPILE_FLAGS ${_target_cflags})

		ADD_CUSTOM_TARGET(pch_Generate_${_targetName} DEPENDS ${_pch_output_to_use})

		ADD_DEPENDENCIES(${_targetName} pch_Generate_${_targetName} )

	ENDMACRO(ADD_PRECOMPILED_HEADER_TO_TARGET)

	MACRO(ADD_PRECOMPILED_HEADER _targetName _input)

		SET(_PCH_current_target ${_targetName})

		IF(NOT CMAKE_BUILD_TYPE)
			MESSAGE(FATAL_ERROR 
				"This is the ADD_PRECOMPILED_HEADER macro. " 
				"You must set CMAKE_BUILD_TYPE!"
				)
		ENDIF(NOT CMAKE_BUILD_TYPE)

		IF( "${ARGN}" STREQUAL "0")
			SET(_dowarn 0)
		ELSE( "${ARGN}" STREQUAL "0")
			SET(_dowarn 1)
		ENDIF("${ARGN}" STREQUAL "0")


		GET_FILENAME_COMPONENT(_name ${_input} NAME)
		GET_FILENAME_COMPONENT(_path ${_input} PATH)
		GET_PRECOMPILED_HEADER_OUTPUT( ${_targetName} ${_input} _output)

		GET_FILENAME_COMPONENT(_outdir ${_output} PATH )

		GET_TARGET_PROPERTY(_targetType ${_PCH_current_target} TYPE)
		_PCH_WRITE_PCHDEP_CXX(${_targetName} ${_input} _pch_dephelp_cxx)

		IF(${_targetType} STREQUAL SHARED_LIBRARY)
			ADD_LIBRARY(${_targetName}_pch_dephelp SHARED ${_pch_dephelp_cxx})
		ELSE(${_targetType} STREQUAL SHARED_LIBRARY)
			ADD_LIBRARY(${_targetName}_pch_dephelp STATIC ${_pch_dephelp_cxx})
		ENDIF(${_targetType} STREQUAL SHARED_LIBRARY)

		FILE(MAKE_DIRECTORY ${_outdir})

		_PCH_GET_COMPILE_FLAGS(_compile_FLAGS)

		SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/${_name} PROPERTIES GENERATED 1)
		ADD_CUSTOM_COMMAND(
			OUTPUT	${CMAKE_CURRENT_BINARY_DIR}/${_name} 
			COMMAND ${CMAKE_COMMAND} -E copy  ${_input} ${CMAKE_CURRENT_BINARY_DIR}/${_name} # ensure same directory! Required by gcc
			DEPENDS ${_input}
			)

		_PCH_GET_COMPILE_COMMAND(_command  ${CMAKE_CURRENT_BINARY_DIR}/${_name} ${_output} )

		ADD_CUSTOM_COMMAND(
			OUTPUT ${_output} 	
			COMMAND ${_command}
			DEPENDS ${_input}   ${CMAKE_CURRENT_BINARY_DIR}/${_name} ${_targetName}_pch_dephelp
			)

		ADD_PRECOMPILED_HEADER_TO_TARGET(${_targetName} ${_input} ${_output} ${_dowarn})
	ENDMACRO(ADD_PRECOMPILED_HEADER)
ENDIF()
