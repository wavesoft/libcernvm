
# Lookup cocoa framework
find_library(COCOA_LIBRARY Cocoa)
if (NOT COCOA_LIBRARY)
    message(FATAL_ERROR "Cocoa framework not found")
endif()

# Lookup IOKit framework
find_library(IOKIT_LIBRARY IOKit)
if (NOT IOKIT_LIBRARY)
    message(FATAL_ERROR "IOKit framework not found")
endif()

# Append framework libraries to the project
set( PROJECT_LIBRARIES 
	 ${PROJECT_LIBRARIES}
	 ${COCOA_LIBRARY}
	 ${IOKIT_LIBRARY}
	)
