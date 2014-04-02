cmake_minimum_required (VERSION 2.8.0)
project(curl)

# Download information for NSS
set(CURL_VERSION 	7.34.0)
set(CURL_SHA1    	9ef09e97dfc9f14ac2c042f3b7e301098794fc0f)

# Setup defaults
if (NOT DEFINED(TARGET_ARCH))
    set(TARGET_ARCH "x86")
endif()

#############################################################
# DOWNLOAD BOOTSTRAP 
#############################################################

# Setup archive
set( ARCHIVE_URL "http://curl.haxx.se/download/curl-${CURL_VERSION}.zip" )
set( ARCHIVE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/archive" )
set( ARCHIVE_EXT ".zip" )

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

#############################################################
# BUILD SCRIPT 
#############################################################

