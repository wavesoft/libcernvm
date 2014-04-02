
# Written to work with cmake 2.6
cmake_minimum_required (VERSION 2.6)
set (CMAKE_BACKWARDS_COMPATIBILITY 2.6)

project(jsoncpp)

#############################################################
# DOWNLOAD BOOTSTRAP 
#############################################################

# Setup archive
set( ARCHIVE_URL "http://downloads.sourceforge.net/project/jsoncpp/jsoncpp/0.5.0/jsoncpp-src-0.5.0.tar.gz" )
set( ARCHIVE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/archive" )
set( ARCHIVE_EXT ".tar.gz" )

# Check if we have the archive
if ( NOT EXISTS ${ARCHIVE_DIR} )
	set( ARCHIVE_TEMP "${CMAKE_CURRENT_SOURCE_DIR}/~download-archive${ARCHIVE_EXT}" )

	# Make the directory
	FILE( MAKE_DIRECTORY ${ARCHIVE_DIR} )

	# Download the archive
   	message( STATUS "Downloading ${PROJECT_NAME} archive" )
	file(DOWNLOAD ${ARCHIVE_URL} ${ARCHIVE_TEMP}
		SHOW_PROGRESS STATUS ARCHIVE_DOWNLOAD_STATUS)

	# Check status
    list( GET ARCHIVE_DOWNLOAD_STATUS 0 ARCHIVE_DOWLOAD_ERROR )
    if( ARCHIVE_DOWLOAD_ERROR )

    	# Remove file and dir
        file( REMOVE ${ARCHIVE_TEMP} )
        file( REMOVE ${ARCHIVE_DIR} )

        # Fatal error
        message( FATAL_ERROR "Could not download ${PROJECT_NAME} archive!" )
   	endif()

   	# Extract archive
	execute_process(
		COMMAND ${CMAKE_COMMAND} -E tar xzf ${ARCHIVE_TEMP}
		WORKING_DIRECTORY ${ARCHIVE_DIR}
	)

	# Remove the archive
   	file( REMOVE ${ARCHIVE_TEMP} )

endif()

# Get the first sub-directory in the archive
FILE(GLOB _ARCHIVE_SUBDIRS ${ARCHIVE_DIR}/*)
FOREACH( _ARCHIVE_SUBDIR ${_ARCHIVE_SUBDIRS} )
	IF(IS_DIRECTORY ${_ARCHIVE_SUBDIR})
		SET(ARCHIVE_DIR ${_ARCHIVE_SUBDIR})
	ENDIF()
ENDFOREACH()

# Rebase
set( CMAKE_CURRENT_SOURCE_DIR ${ARCHIVE_DIR} )

# Expose project variables to parent
set (JSONCPP_LIBRARIES 
	${PROJECT_NAME} 
	PARENT_SCOPE)
set (JSONCPP_INCLUDE_DIRS 
	${ARCHIVE_DIR}/src/lib_json
	${ARCHIVE_DIR}/include 
	PARENT_SCOPE)

#############################################################
# BUILD SCRIPT 
#############################################################

# Include directories
include_directories (
	${ARCHIVE_DIR}/src/lib_json
	${ARCHIVE_DIR}/include
)

# Set sources
file (GLOB JSONCPP_SOURCES
    ${ARCHIVE_DIR}/src/lib_json/*.inl
    ${ARCHIVE_DIR}/src/lib_json/*.cpp
    ${ARCHIVE_DIR}/src/*.inl
)

# Link library
add_library (jsoncpp STATIC ${JSONCPP_SOURCES})
