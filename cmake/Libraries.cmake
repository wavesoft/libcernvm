
# Include all the necessary files for macros
include(CMakeParseArguments)

# include_library(

# 		# The visual name for the messages
# 		LIB_NAME 		"Zlib"

# 		# Prefixes for importing (checking if they are already defined) 
# 		# and exporting (defining in the environment) the _INCLUDE_DIR 
# 		# and _LIBRARIES variables
# 		EXPORT_PREFIX	"ZLIB"
# 		IMPORT_PREFIX	"ZLIB"

# 		# parameters for find_package function
# 		FIND_NAME		"ZLIB"
# 		FIND_ARGS		""

# 		# Project by source
# 		EXTERN_SRC		"extern/zlib"

# 		# Build flags
# 		CHECK_IMPORT	TRUE
# 		CHECK_PACKAGE	${USE_SYSTEM_LIBS}

# 		# And be verbose
# 		VERBOSE

# 	)

######################################
## Include a library in the project ##
## -------------------------------- ##
######################################
function(include_library)

	# Parse arguments
	set(options VERBOSE)
	set(oneValueArgs LIB_NAME EXPORT_PREFIX IMPORT_PREFIX FIND_NAME EXTERN_SRC CHECK_IMPORT CHECK_PACKAGE)
	set(multiValueArgs FIND_ARGS)
	cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )


	# Check if we should check environment


	# Check if it is defined 
	if ( NOT DEFINED(${LIB_NAME}_LIBRARIES) OR NOT DEFINED(${LIB_NAME}_INCLUDE_DIR) )

		# Check if we can find the library defined in the system
		find_package(${LIB_NAME} ${LIB_FIND_ARGS})
		if(ZLIB_FOUND AND ${USE_SYSTEM_LIBS})

			# Log
			message( STATUS "Using ZLib from system")

		else()

			# We are shipping zlib with the project
			set( EXTERN_ZLIB "extern/zlib" )
			add_subdirectory( ${EXTERN_ZLIB} ${CMAKE_BINARY_DIR}/${EXTERN_ZLIB} )
			include_directories( ${CMAKE_BINARY_DIR}/${EXTERN_ZLIB} )

			# Log
			message( STATUS "Using ZLib shipped with libcernvm")

		endif()

	else()
		message( STATUS "Using ZLib from: ${ZLIB_INCLUDE_DIR}")

	endif()

endfunction()