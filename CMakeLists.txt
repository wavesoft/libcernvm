cmake_minimum_required (VERSION 2.6)

# CernVM Library
project ( libcernvm )
set(VERSION "1.0.0")

# Which hypervisors to enable
set( HV_NAMES Virtualbox )

# Static build
SET(BUILD_SHARED_LIBS 	OFF)

# Should we try to find system libs?
SET(USE_SYSTEM_LIBS 	1)

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

# Common environment cleanups
if (NOT DEFINED(USE_SYSTEM_LIBS))
	set(USE_SYSTEM_LIBS 0)
endif()

#
# [ZLib] For GZIP compression/decompression
#
if ( NOT DEFINED(ZLIB_LIBRARIES) OR NOT DEFINED(ZLIB_INCLUDE_DIR) )

	# Check if we can find ZLib in the system
	find_package(ZLIB)
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

#
# [JSONCPP] For parsing JSON data
#
if ( NOT DEFINED(JSONCPP_LIBRARIES) OR NOT DEFINED(JSONCPP_INCLUDE_DIRS) )

	# We are shipping jsoncpp with the project
	set( EXTERN_JSONCPP "extern/jsoncpp" )
	add_subdirectory( ${EXTERN_JSONCPP} ${CMAKE_BINARY_DIR}/${EXTERN_JSONCPP} )

	# Log
	message( STATUS "Using JsonCPP shipped with libcernvm")

else()
	message( STATUS "Using JsonCPP from: ${JSONCPP_INCLUDE_DIRS}")

endif()

#
# [OpenSSL] For PKI functions
#
if ( NOT DEFINED(OPENSSL_LIBRARIES) OR NOT DEFINED(OPENSSL_INCLUDE_DIRS) )

	# Check if we can find ZLib in the system
	find_package(OpenSSL)
	if(OPENSSL_FOUND AND ${USE_SYSTEM_LIBS})

		# Log
		message( STATUS "Using OpenSSL from system")

	else()

		# We are shipping jsoncpp with the project
		set( EXTERN_OPENSSL "extern/openssl" )
		add_subdirectory( ${EXTERN_OPENSSL} ${CMAKE_BINARY_DIR}/${EXTERN_OPENSSL} )

		# Log
		message( STATUS "Using OpenSSL shipped with libcernvm")

	endif()

else()
	message( STATUS "Using OpenSSL from: ${OPENSSL_INCLUDE_DIRS}")

endif()

#
# [BOOST] We use various accelerators from boost
#
set( BOOST_REQUIRE_LIBRARIES thread system filesystem regex )
if ( NOT DEFINED(BOOST_LIBRARIES) OR NOT DEFINED(BOOST_INCLUDE_DIRS) )

	# Try to find system boost
	set(Boost_USE_MULTITHREADED ON)  
	set(Boost_USE_STATIC_RUNTIME NOT BUILD_SHARED_LIBS) 
	set(Boost_USE_STATIC_LIBS NOT BUILD_SHARED_LIBS) 
	find_package(Boost COMPONENTS ${BOOST_REQUIRE_LIBRARIES}) 
	if(Boost_FOUND AND ${USE_SYSTEM_LIBS})

		# Log
		message( STATUS "Using BOOST from system")

	else()

		# We are shipping BOOST with the project
		set( EXTERN_BOOST "extern/boost" )
		add_subdirectory( ${EXTERN_BOOST} ${CMAKE_BINARY_DIR}/${EXTERN_BOOST} )

		# Link targets and populate libraries/include dirs
		set( BOOST_LIBRARIES "" )
		set( BOOST_DEPEND_TARGETS "")
		set( BOOST_INCLUDE_DIRS ${BoostSourceDir} )
		foreach(LIB ${BOOST_REQUIRE_LIBRARIES}) 
			# Depend target
			set( BOOST_DEPEND_TARGETS ${BOOST_DEPEND_TARGETS} boost_${LIB} )
			# Use given library
			set( BOOST_LIBRARIES ${BOOST_LIBRARIES} ${Boost${LIB}Libs} )
		endforeach()

		# Log
		message( STATUS "Using BOOST shipped with libcernvm")

	endif()

else()
	message( STATUS "Using BOOST from: ${BOOST_INCLUDE_DIRS}")

endif()

# Include libraries
set( PROJECT_INCLUDES
	${ZLIB_INCLUDE_DIR}
	${JSONCPP_INCLUDE_DIRS}
	${OPENSSL_INCLUDE_DIRS}
	${BOOST_INCLUDE_DIRS}
)

# Collect library names
set( PROJECT_LIBRARIES 
	${ZLIB_LIBRARIES}
	${JSONCPP_LIBRARIES}
	${OPENSSL_LIBRARIES}
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
	ADD_DEFINITIONS(-DNOMINMAX)   
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
# ourselves
if (DEFINED(BOOST_DEPEND_TARGETS))
	foreach(TARGET ${BOOST_DEPEND_TARGETS})
		add_dependencies(${PROJECT_NAME} ${TARGET})
	endforeach()
endif()

# Libraries
target_link_libraries ( ${PROJECT_NAME} ${PROJECT_LIBRARIES} )
