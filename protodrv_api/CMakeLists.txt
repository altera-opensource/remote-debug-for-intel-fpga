cmake_minimum_required(VERSION 3.0.0)
project(protodrv_lib VERSION 0.1.0)

# Create an option to specify which ProtoApi to use, defult to "UIO"
set(BUILD_PROTO_API_MODE UIO CACHE STRING "Which ProtoApi to use")
set(PROTO_DRIVER_OPTIONS "")

# Get a list of all files and sub directories
FILE(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
# Iterate all directories and add any which have a CMakeLists.txt
FOREACH( child ${children} )
    IF( IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child} )
        IF( EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${child}/CMakeLists.txt )
            
            # It is assumed that any CMakeLists.txt follows the rules for a ProtoDriver Library
            # The subdirectory needs to do the following:
            #   Added the OPTION name to PROTO_DRIVER_OPTIONS
            #   Set up a library using the ${PROJECT_NAME} as this is inherited from this CMakeLists.txt
            add_subdirectory(${child})
        ENDIF()
    ENDIF()
ENDFOREACH()

# each subdirectory should setup the library and add it's option name to PROTO_DRIVER_OPTIONS
# Use "cmake -DBUILD_PROTO_API_MODE=XXX" to change which library you want to build with
set_property(CACHE BUILD_PROTO_API_MODE PROPERTY STRINGS ${PROTO_DRIVER_OPTIONS})
