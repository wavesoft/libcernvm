cmake_minimum_required (VERSION 2.8)

# Which hypervisors to enable
set( HV_NAMES Virtualbox )

# Guess system architecture
set(GUESS_ARCH "i386") 							# 1) Fallback to x86
if(CMAKE_SYSTEM_PROCESSOR) 						# 2) Build on the same processor
	set(GUESS_ARCH ${CMAKE_SYSTEM_PROCESSOR})
	if(${GUESS_ARCH} STREQUAL "AMD64")
		# We don't do any differentiation for AMD64 instruction set
		set(GUESS_ARCH "x86_64")
	endif()
endif()
if(CMAKE_OSX_ARCHITECTURES)						# 3) Lookup on OSX Architectures
	set(GUESS_ARCH ${CMAKE_OSX_ARCHITECTURES})
elseif ("${CMAKE_GENERATOR}" MATCHES "Win64") 	# 4) Lookup on Windows Generator
	set(GUESS_ARCH "x86_64")
endif()

# Static build by default
option(LOGGING "Set to ON to enable verbose logging on screen" OFF)
option(CRASH_REPORTING "Set to ON to enable crash reporting" OFF)
option(BUILD_SHARED_LIBS "Set to ON to build shared libraries instead of static" OFF)
option(USE_SYSTEM_LIBS "Set to ON to use system libraries instead the ones shipped with libcernvm" OFF)
option(SYSTEM_ZLIB "Set to ON to use zlib from the system" OFF)
option(SYSTEM_JSONCPP "Set to ON to use jsoncpp from the system" OFF)
option(SYSTEM_OPENSSL "Set to ON to use openssl from the system" OFF)
option(SYSTEM_CURL "Set to ON to use CURL from the system" OFF)
option(COVERITY_RUN "Set to ON when running this application with coverity" OFF)

set(TARGET_ARCH "${GUESS_ARCH}" CACHE STRING "Override the identified target architecture (x86_64 or i386)" )
set(BUILD_ID "" CACHE STRING "A build ID which is used to address different library configuration")

# Validation banner
message(STATUS "Building libcernvm for arch: ${TARGET_ARCH}")

# Change OSX architectures
if (APPLE)
	set(CMAKE_OSX_ARCHITECTURES ${TARGET_ARCH})
	set(OSX_ARCHITECTURES ${TARGET_ARCH})
endif()

# CernVM Library
project ( libcernvm )
set(VERSION "2.0.12")

#############################################################
# HELPER FUNCTIONS 
#############################################################

# Include additional libraries
include(cmake/AddCompileLinkFlags.cmake)

# Helper function to convert underscores to CamelCase
function(ms_underscores_to_camel_case VarIn VarOut)
  string(REPLACE "_" ";" Pieces ${VarIn})
  foreach(Part ${Pieces})
    string(SUBSTRING ${Part} 0 1 Initial)
    string(SUBSTRING ${Part} 1 -1 Part)
    string(TOUPPER ${Initial} Initial)
    set(CamelCase ${CamelCase}${Initial}${Part})
  endforeach()
  set(${VarOut} ${CamelCase} PARENT_SCOPE)
endfunction()

#############################################################
# LIBRARIES 
#############################################################

# For every library, we give the option to the project that
# includes us to provide them by itself.
#
# If not, we will try our best to build them as our child
# libraries (statically).
#
# To define a library, you must define the following two directives:
#   XXXXXX_LIBRARIES : Which contain the name(s) of the libraries
#	XXXXXX_INCLUDE 	 : Which contain the additiona include directories
#

#
# [ZLib] For GZIP compression/decompression
#
set( ZLIB_MODE "EXTERN" )
if ( NOT DEFINED(ZLIB_LIBRARIES) OR NOT DEFINED(ZLIB_INCLUDE_DIRS) )

	# Check if we should check for local
	set( USE_LOCAL 1 )
	if (${USE_SYSTEM_LIBS} OR ${SYSTEM_ZLIB})
		# Check if we can find it on the system
		find_package(ZLIB)
		if(ZLIB_FOUND)
			set( USE_LOCAL 0 )
			set( ZLIB_MODE "SYSTEM" )
			message( STATUS "Using ZLib from system")
		endif()
	endif()

	# Use shipped ZLib if not found locally
	if(${USE_LOCAL})

		# We are shipping zlib with the project
		set( EXTERN_ZLIB "extern/zlib" )
		add_subdirectory( ${EXTERN_ZLIB} ${CMAKE_BINARY_DIR}/${EXTERN_ZLIB} )
		include_directories( ${CMAKE_BINARY_DIR}/${EXTERN_ZLIB} )

		# Log
		set( ZLIB_MODE "LOCAL" )
		message( STATUS "Using ZLib shipped with libcernvm")

	endif()

else()
	message( STATUS "Using ZLib from: ${ZLIB_INCLUDE_DIRS}")

endif()

#
# [JSONCPP] For parsing JSON data
#
set( JSONCPP_MODE "EXTERN" )
if ( NOT DEFINED(JSONCPP_LIBRARIES) OR NOT DEFINED(JSONCPP_INCLUDE_DIRS) )

	# We are shipping jsoncpp with the project
	set( EXTERN_JSONCPP "extern/jsoncpp" )
	add_subdirectory( ${EXTERN_JSONCPP} ${CMAKE_BINARY_DIR}/${EXTERN_JSONCPP} )

	# Log
	set( JSONCPP_MODE "LOCAL" )
	message( STATUS "Using JsonCPP shipped with libcernvm")

else()
	message( STATUS "Using JsonCPP from: ${JSONCPP_INCLUDE_DIRS}")

endif()

#
# [OpenSSL] For PKI functions and for libCurl
#
set( OPENSSL_MODE "EXTERN" )
if ( NOT DEFINED(OPENSSL_LIBRARIES) OR NOT DEFINED(OPENSSL_INCLUDE_DIRS) )

	# Check if we should check for local
	set( USE_LOCAL 1 )
	if (${USE_SYSTEM_LIBS} OR ${SYSTEM_OPENSSL})
		# Check if we can find it on the system
		find_package(OpenSSL)
		if(OPENSSL_FOUND)
			set( USE_LOCAL 0 )
			set( OPENSSL_MODE "SYSTEM" )
			message( STATUS "Using OpenSSL from system")
		endif()
	endif()

	# Use shipped OpenSSL if not found locally
	if(${USE_LOCAL})

		# We are shipping jsoncpp with the project
		set( EXTERN_OPENSSL "extern/openssl" )
		add_subdirectory( ${EXTERN_OPENSSL} ${CMAKE_BINARY_DIR}/${EXTERN_OPENSSL} )

		# Log
		set( OPENSSL_MODE "LOCAL" )
		message( STATUS "Using OpenSSL shipped with libcernvm")

	endif()

else()
	message( STATUS "Using OpenSSL from: ${OPENSSL_INCLUDE_DIRS}")

endif()

#
# [CURL] For interacting with web endpoints
#
set( CURL_MODE "EXTERN" )
if ( NOT DEFINED(CURL_LIBRARIES) OR NOT DEFINED(CURL_INCLUDE_DIRS) )

	# Check if we should check for local
	set( USE_LOCAL 1 )
	if (${USE_SYSTEM_LIBS} OR ${SYSTEM_CURL})
		# Check if we can find it on the system
		find_package(CURL)
		if(CURL_FOUND)
			set( USE_LOCAL 0 )
			set( CURL_MODE "SYSTEM" )
			message( STATUS "Using CURL from system")
		endif()
	endif()

	# Use shipped CURL if not found locally
	if(${USE_LOCAL})

		# We are shipping curl with the project
		set( EXTERN_CURL "extern/curl" )
		add_subdirectory( ${EXTERN_CURL} ${CMAKE_BINARY_DIR}/${EXTERN_CURL} )

		# Add a definition required by the curl header to switch
		# to static build
		add_definitions(-DCURL_STATICLIB)

		# Log
		set( CURL_MODE "LOCAL" )
		message( STATUS "Using CURL shipped with libcernvm")

	endif()

else()
	message( STATUS "Using CURL from: ${CURL_INCLUDE_DIRS}")

endif()

#
# [PThread] required by BOOST only on Linux
#
set( EXTRA_LIBRARIES "" )
if (UNIX AND NOT APPLE)
	set( EXTRA_LIBRARIES pthread )
endif()

#
# [Advapi32.lib] required only on Windows
# 
if (WIN32)
	set( EXTRA_LIBRARIES advapi32 )
endif()

# Include libraries
set( PROJECT_INCLUDES
	${ZLIB_INCLUDE_DIRS}
	${JSONCPP_INCLUDE_DIRS}
	${OPENSSL_INCLUDE_DIRS}
	${CURL_INCLUDE_DIRS}
)

# Collect library names
set( PROJECT_LIBRARIES 
	${ZLIB_LIBRARIES}
	${JSONCPP_LIBRARIES}
	${OPENSSL_LIBRARIES}
	${CURL_LIBRARIES}
	${EXTRA_LIBRARIES}
)

#############################################################
# PROJECT SOURCES 
#############################################################

# Locate the common sources
file ( GLOB LIB_SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp  )

# Collect the hypervisor sources
message( STATUS "Compiling drivers for the following hypervisors:" )
set( HYPERVISOR_SOURCES "" )
FOREACH(HV ${HV_NAMES}) 
	message( STATUS " - ${HV}" )
	file ( GLOB HV_CPP ${PROJECT_SOURCE_DIR}/src/Hypervisor/${HV}/*.cpp  )
	set( HYPERVISOR_SOURCES ${HYPERVISOR_SOURCES} ${HV_CPP} )
ENDFOREACH(HV)

# Locate platform-dependant sources
if (WIN32)
	set ( PLATFORM_DIR ${PROJECT_SOURCE_DIR}/src/Platform/win )
	file ( GLOB PLATFORM_SOURCES ${PLATFORM_DIR}/*.cpp  )
elseif (APPLE)
	set ( PLATFORM_DIR ${PROJECT_SOURCE_DIR}/src/Platform/osx )
	file ( GLOB PLATFORM_SOURCES ${PLATFORM_DIR}/*.mm  )
elseif(UNIX)
	set ( PLATFORM_DIR ${PROJECT_SOURCE_DIR}/src/Platform/unix )
	file ( GLOB PLATFORM_SOURCES ${PLATFORM_DIR}/*.cpp  )
endif()

# Include CMAKE pre
include ( ${PLATFORM_DIR}/platform-pre.cmake )

#############################################################
# ADDITIONAL BUILD INFORMATION
#############################################################

# Add custom definitions
if (LOGGING)
	add_definitions(-DLOGGING)
endif()
if (CRASH_REPORTING)
	add_definitions(-DCRASH_REPORTING)
endif()

# Windows additional definitions
if (WIN32)
	# Fix: Disable Min/Max macros
	add_definitions(-DNOMINMAX)
endif(WIN32)

# Include project and headers
include_directories( ${PROJECT_SOURCE_DIR}/include )
include_directories( ${PROJECT_SOURCE_DIR}/src )
include_directories( ${PROJECT_INCLUDES} )

# Add additional definitions used in the library
if (WIN32)
	IF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	  add_definitions(/D "DEBUG")   
	ELSE (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	  add_definitions(/D "NDEBUG")
	ENDIF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
else(WIN32)
	IF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	  add_definitions(-DDEBUG)   
	ELSE (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	  add_definitions(-DNDEBUG)
	ENDIF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
endif(WIN32)

#############################################################
# TARGET BUILD
#############################################################

# Sources
add_library( ${PROJECT_NAME}
	${LIB_SOURCES}
	${HYPERVISOR_SOURCES}
	${PLATFORM_SOURCES}
)

# in the same way we have to depend openssl (if we are the ones)
# building it.
if (${OPENSSL_MODE} STREQUAL "LOCAL")
	add_dependencies(${PROJECT_NAME} openssl)
endif()

# Enable C++11 extensions
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
	add_compile_flags( ${PROJECT_NAME} -std=c++11 )
elseif(COMPILER_SUPPORTS_CXX0X)
	add_compile_flags( ${PROJECT_NAME} -std=c++0x )
else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

# On apple, add -stdlib=libc++
if (APPLE)
	add_compile_flags( ${PROJECT_NAME} -stdlib=libc++ )
endif()

# Libraries
target_link_libraries ( ${PROJECT_NAME} ${PROJECT_LIBRARIES} )

# Expose everything to the parent context
set( CERNVM_LIBRARIES 
	${PROJECT_NAME} 
	${PROJECT_LIBRARIES}
	PARENT_SCOPE )
set( CERNVM_INCLUDE_DIRS
	${PROJECT_SOURCE_DIR}/include 	# Library includes
	${PROJECT_INCLUDES}				# All includes are for the project too
	PARENT_SCOPE )
