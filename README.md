libCernVM
=========

A cross-platform library for starting, controlling and interfacing with Virtual Machines, mainly targeting desktop computers. It started as a browser plugin, using the NPAPI interface, but since it's getting deprecated, the logic was forked into an individual project, so it can be interfaced with newer implementations.

__libCernVM__ takes care of three major interactions:

 * *Ensuring* that a hypervisor is installed in the host's computer and is in functional state.
 * *Abstracting* the interface with the hypervisor, ensuring integrity and security on the operations.
 * *Interfacing* with the running VM instance.

Building
========

The library can be built with CMake. There is an extensive CMake configuration that allows all the dependant libraries to be built along. To include libcernvm in your project you will have to do the following:

```CMake

# CernVM Library requries CMake 2.8
cmake_minimum_required (VERSION 2.8)

...

# Include CernVM Library project
set( CERNVM_LIBSRC "/path/to/libcernvm" )

# Include the CernVM Sub-Project
add_subdirectory( ${CERNVM_LIBSRC} libcernvm )
include_directories( ${CERNVM_INCLUDE_DIRS} )

...

# Link CernVM libraries to your project
target_link_libraries ( ${PROJECT_NAME} ${CERNVM_LIBRARIES} )

```

Build Parameters
----------------

The CernVM library supports the following CMake options:

Crash reporting and debug logging:

 * **-DLOGGING=ON** : Enable verbose logging of each action taken in the project.
 * **-DCRASH_REPORTING** : Enable sending crash reports for debugging when an unhandled exception occurs.

Linking and building options:

 * **-DSYSTEM_ZLIB=ON** : Use shared, system-provided z-lib library instead of linking it statically.
 * **-DSYSTEM_JSONCPP=ON** : Use shared, system-provided JsonCPP library instead of linking it statically.
 * **-DSYSTEM_OPENSSL=ON** : Use shared, system-provided OpenSSL library instead of linking it statically.
 * **-DSYSTEM_CURL=ON** : Use shared, system-provided CURL library instead of linking it statically.
 * **-DSYSTEM_BOOST=ON** : Use shared, system-provided BOOST library instead of linking it statically.
 * **-DUSE_SYSTEM_LIBS=ON** : Use all libraries provided from system

