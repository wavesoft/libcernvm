cmake_minimum_required (VERSION 2.6)

# CernVM Library
project ( libcernvm )
set(VERSION "1.0.0")

# Which hypervisors to enable
set( HV_NAMES Virtualbox )

# Guess system architecture
set(GUESS_ARCH ${CMAKE_SYSTEM_PROCESSOR})
if(CMAKE_OSX_ARCHITECTURES)
	set(GUESS_ARCH ${CMAKE_OSX_ARCHITECTURES})
endif()

# Static build by default
option(BUILD_SHARED_LIBS "Set to ON to build shared libraries instead of static" OFF)
option(USE_SYSTEM_LIBS "Set to ON to use system libraries instead the ones shipped with libcernvm" OFF)
option(TARGET_ARCH "Define the target architecture" "${GUESS_ARCH}")

# Validation banner
if (NOT ${TARGET_ARCH})
	set(TARGET_ARCH ${GUESS_ARCH})
endif()
message(STATUS "Building libcernvm for arch: ${TARGET_ARCH}")
set(CMAKE_OSX_ARCHITECTURES ${TARGET_ARCH})

#############################################################
# HELPER FUNCTIONS 
#############################################################

# Include additional libraries
include(cmake/AddCompileLinkFlags.cmake)

# Helper function to convert a string to camel-case
function(to_camel_case VarIn VarOut)
	string(SUBSTRING ${VarIn} 0 1 Initial)
	string(SUBSTRING ${VarIn} 1 -1 VarIn)
	string(TOUPPER ${Initial} Initial)
	set(${VarOut} ${Initial}${VarIn} PARENT_SCOPE)
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
	if (${USE_SYSTEM_LIBS})
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
	if (${USE_SYSTEM_LIBS})
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
	if (${USE_SYSTEM_LIBS})
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

		# Log
		set( CURL_MODE "LOCAL" )
		message( STATUS "Using CURL shipped with libcernvm")

	endif()

else()
	message( STATUS "Using CURL from: ${CURL_INCLUDE_DIRS}")

endif()

#
# [BOOST] We use various accelerators from boost
#
set( BOOST_MODE "EXTERN" )
set( BOOST_REQUIRE_LIBRARIES thread system filesystem regex )
if ( NOT DEFINED(BOOST_LIBRARIES) OR NOT DEFINED(BOOST_INCLUDE_DIRS) )

	# Check if we should check for local
	set( USE_LOCAL 1 )
	if (${USE_SYSTEM_LIBS})
 		# Check if we can find it on the system
		set(Boost_USE_MULTITHREADED ON)  
		set(Boost_USE_STATIC_RUNTIME NOT BUILD_SHARED_LIBS) 
		set(Boost_USE_STATIC_LIBS NOT BUILD_SHARED_LIBS) 
		find_package(Boost COMPONENTS ${BOOST_REQUIRE_LIBRARIES}) 
		if(Boost_FOUND)
			set( USE_LOCAL 0 )
			set( BOOST_MODE "SYSTEM" )
			set( BOOST_LIBRARIES ${Boost_LIBRARIES} )
			set( BOOST_INCLUDE_DIRS ${Boost_INCLUDE_DIRS} )
			message( STATUS "Using BOOST from system")
		endif()
	endif()

	# Use shipped BOOST if not found locally
	if(${USE_LOCAL})

		# We are shipping BOOST with the project
		set( EXTERN_BOOST "extern/boost" )
		add_subdirectory( ${EXTERN_BOOST} ${CMAKE_BINARY_DIR}/${EXTERN_BOOST} )

		# Link targets and populate libraries/include dirs
		set( BOOST_LIBRARIES "" )
		set( BOOST_DEPEND_TARGETS "")
		set( BOOST_INCLUDE_DIRS ${BoostSourceDir} )
		foreach(LIB ${BOOST_REQUIRE_LIBRARIES}) 
			# Camel-case lib
			to_camel_case( ${LIB} LIB )
			# Depend target
			set( BOOST_DEPEND_TARGETS ${BOOST_DEPEND_TARGETS} boost_${LIB} )
			# Use given library
			set( BOOST_LIBRARIES ${BOOST_LIBRARIES} ${Boost${LIB}Libs} )
		endforeach()

		# Log
		set( BOOST_MODE "LOCAL" )
		message( STATUS "Using BOOST shipped with libcernvm")

	endif()

else()
	message( STATUS "Using BOOST from: ${BOOST_INCLUDE_DIRS}")

endif()

# Include libraries
set( PROJECT_INCLUDES
	${ZLIB_INCLUDE_DIRS}
	${JSONCPP_INCLUDE_DIRS}
	${OPENSSL_INCLUDE_DIRS}
	${CURL_INCLUDE_DIRS}
	${BOOST_INCLUDE_DIRS}
)

# Collect library names
set( PROJECT_LIBRARIES 
	${ZLIB_LIBRARIES}
	${JSONCPP_LIBRARIES}
	${OPENSSL_LIBRARIES}
	${CURL_LIBRARIES}
	${BOOST_LIBRARIES}
)

#############################################################
# PROJECT SOURCES 
#############################################################

# Locate the common sources
file ( GLOB LIB_SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp  )

# Collect the hypervisor sources
set( HYPERVISOR_SOURCES "" )
FOREACH(HV ${HV_NAMES}) 
	message( STATUS "Using hypervisor driver: ${HV}" )
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

#############################################################
# ADDITIONAL BUILD INFORMATION
#############################################################

# Add custom definitions
ADD_DEFINITIONS(-DLOGGING)

# Windows additional definitions
if (WIN32)
	# Fix: Disable Min/Max macros
	ADD_DEFINITIONS(-DNOMINMAX)
	# Fix: For some reason CURL_STATICLIB is not defined
	ADD_DEFINITIONS(-DCURL_STATICLIB)
	# Fix: OpenSSL builds are not built with safe exception handlers
	ADD_DEFINITIONS(-DSAFESEH:NO)
endif(WIN32)

# Include project and headers
include_directories( ${PROJECT_SOURCE_DIR}/include )
include_directories( ${PROJECT_SOURCE_DIR}/src )
include_directories( ${PROJECT_INCLUDES} )

# Add additional definitions used in the library
if (WIN32)
	IF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	  ADD_DEFINITIONS(/D "DEBUG")   
	ELSE (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	  ADD_DEFINITIONS(/D "NDEBUG")
	ENDIF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
else(WIN32)
	IF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	  ADD_DEFINITIONS(-DDEBUG)   
	ELSE (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	  ADD_DEFINITIONS(-DNDEBUG)
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

# Depend boost targets if we are building them
# ourselves - That's the only way I know for starting
# the build process for these libraries, since BOOST
# was not added as a library, but as an external project
if (${BOOST_MODE} STREQUAL "LOCAL")
	foreach(TARGET ${BOOST_DEPEND_TARGETS})
		add_dependencies(${PROJECT_NAME} ${TARGET})
	endforeach()
endif()

# in the same way we have to depend openssl (if we are the ones)
# building it.
if (${OPENSSL_MODE} STREQUAL "LOCAL")
	add_dependencies(${PROJECT_NAME} openssl)
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
