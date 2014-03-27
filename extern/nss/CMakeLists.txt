cmake_minimum_required (VERSION 2.6)
project(nss)

# Download information for NSS
set(NSS_VERSION     3.16)
set(NSPR_VERSION    4.10.4)
set(NSS_SHA1        8ae6ddec43556b4deb949dc889123ff1d09ab737)

#############################################################
# DOWNLOAD BOOTSTRAP 
#############################################################

# Convert NSS_VERSION to dashes
string(REPLACE "." "_" NSS_VER_DASHED ${NSS_VERSION})

# Setup archive
set( ARCHIVE_URL "ftp://ftp.mozilla.org/pub/mozilla.org/security/nss/releases/NSS_${NSS_VER_DASHED}_RTM/src/nss-${NSS_VERSION}-with-nspr-${NSPR_VERSION}.tar.gz" )
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

# Log status
message( STATUS "Using ${PROJECT_NAME} sources in: ${ARCHIVE_DIR}" )

#############################################################
# BUILD SCRIPT 
#############################################################

