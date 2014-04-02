cmake_minimum_required (VERSION 2.8.0)
project(openssl)

# Download information for NSS
set(OPENSSL_VERSION 1.0.1f)
set(OPENSSL_SHA1    9ef09e97dfc9f14ac2c042f3b7e301098794fc0f)

# Setup defaults
if (NOT DEFINED(TARGET_ARCH))
    set(TARGET_ARCH "x86")
endif()

#############################################################
# DOWNLOAD BOOTSTRAP 
#############################################################

# Convert NSS_VERSION to dashes
string(REPLACE "." "_" OPENSSL_VER_DASHED ${OPENSSL_VERSION})

# Setup archive
set( ARCHIVE_URL "https://github.com/openssl/openssl/archive/OpenSSL_${OPENSSL_VER_DASHED}.zip" )
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
file(GLOB _ARCHIVE_SUBDIRS ${ARCHIVE_DIR}/*)
foreach( _ARCHIVE_SUBDIR ${_ARCHIVE_SUBDIRS} )
    if(IS_DIRECTORY ${_ARCHIVE_SUBDIR})
        set(ARCHIVE_DIR ${_ARCHIVE_SUBDIR})
    endif()
endforeach()

#############################################################
# BUILD SCRIPT 
#############################################################

# We need the ExternalProject_* macros
include(ExternalProject)

# Make sure we have perl installed
find_package(Perl)
if(NOT PERL_FOUND)
    message( FATAL_ERROR "OpenSSL requires perl for building. On windows try ActivePerl (http://www.activestate.com/activeperl)" )
endif()

# Setup directories
set( OPENSSL_SRC_DIR ${ARCHIVE_DIR})
set( OPENSSL_BIN_DIR ${ARCHIVE_DIR}/Build )

# Pick the appropriate OpenSSL target
set( OPENSSL_TARGET "" )

# Check if we are on Windows
if(WIN32)

    # Check if we are using the Visual Studio compiler
    if(MSVC)
        if (${TARGET_ARCH} STREQUAL "x86_64")
            # 64-bit Windows, Visual Stuio
            set( OPENSSL_TARGET "VC-WIN64A" )
        elseif (${TARGET_ARCH} STREQUAL "x86")
            # 32-bit Windows, Visual Stuio
            set( OPENSSL_TARGET "VC-WIN32" )
        else()

    # Check if we are using GCC on windows (mingw)
    elseif(CMAKE_COMPILER_IS_GNUCXX)
        if (${TARGET_ARCH} STREQUAL "x86_64")
            # 64-bit Windows, GCC
            set( OPENSSL_TARGET "mingw64" )
        elseif (${TARGET_ARCH} STREQUAL "x86")
            # 32-bit Windows, GCC
            set( OPENSSL_TARGET "mingw" )
        else()

    else()
        message(SEND_ERROR "You are using an unsupported Windows compiler! (Not MSVC or GCC)")
    endif()

elseif(APPLE)
    if (${TARGET_ARCH} STREQUAL "x86_64")
        # 64-bit MacOSX, Any Compiler
        set( OPENSSL_TARGET "darwin64-x86_64-cc" )
    elseif (${TARGET_ARCH} STREQUAL "x86")
        # 32-bit MacOSX, Any Compiler
        set( OPENSSL_TARGET "darwin-i386-cc" )
    else()

elseif(UNIX)
    if (${TARGET_ARCH} STREQUAL "x86_64")
        # 64-bit MacOSX, Any Compiler
        set( OPENSSL_TARGET "linux-generic64" )
    elseif (${TARGET_ARCH} STREQUAL "x86")
        # 32-bit Intel processor, Any Compiler
        set( OPENSSL_TARGET "linux-generic32" )
    else()

endif()

if (WIN32 AND MSVC)
    # Windows/MSVC we should use nmake for building
    ExternalProject_Add(
        openssl
        PREFIX ${OPENSSL_BIN_DIR}
        SOURCE_DIR ${OPENSSL_SRC_DIR}
        BINARY_DIR ${OPENSSL_BIN_DIR}
        CONFIGURE_COMMAND cmd.exe /c "ms\\do_ms.bat"
        BUILD_COMMAND     nmake.exe -f "ms\\nt.mak"
        INSTALL_COMMAND   nmake.exe -f "ms\\nt.mak" "install"
        LOG_BUILD ON
    )
else()
    # Any other POSIX system uses make
    ExternalProject_Add(
        openssl
        PREFIX ${OPENSSL_BIN_DIR}
        SOURCE_DIR ${OPENSSL_SRC_DIR}
        BINARY_DIR ${OPENSSL_BIN_DIR}
        CONFIGURE_COMMAND ""
        BUILD_COMMAND     make
        INSTALL_COMMAND   make install
        LOG_BUILD ON
    )
endif()

# Configure is an intermediate step
ExternalProject_Add_Step(
    openssl
    build_configure
    COMMENT "Configuring OpenSSL environment"
    COMMAND ${PERL_EXECUTABLE} "Configure" ${OPENSSL_TARGET} --prefix=Build
    WORKING_DIRECTORY ${OPENSSL_SRC_DIR}
    DEPENDEES download
    DEPENDERS configure
)

# Collect non-directory files from lib (they are the libeay & ssleay libs)
set (OPENSSL_LIB_FILES "")
file(GLOB _LIB_FILES ${OPENSSL_BIN_DIR}/lib/*)
foreach( LIB ${_LIB_FILES} )
    if(NOT IS_DIRECTORY ${LIB})
        set (OPENSSL_LIB_FILES ${OPENSSL_LIB_FILES} ${LIB})
    endif()
endforeach()

# Get the first sub-directory that matches the inc* prefix
set (OPENSSL_INC_DIR "")
file(GLOB _INC_SUBDIRS ${ARCHIVE_DIR}/inc*)
foreach( _INC_SUBDIR ${_INC_SUBDIRS} )
    if(IS_DIRECTORY ${_INC_SUBDIR})
        set(OPENSSL_INC_DIR ${_INC_SUBDIR})
    endif()
endforeach()

# Expose to parent scope
set (OPENSSL_LIBRARIES  ${OPENSSL_LIB_FILES} PARENT_SCOPE)
set (OPENSSL_INCLUDE_DIRS ${OPENSSL_INC_DIR} PARENT_SCOPE)
