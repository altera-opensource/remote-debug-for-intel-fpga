cmake_minimum_required(VERSION 3.2)
project(remote-debug-for-intel-fpga)

option(SW_MODEL "SW Model Unit Test")

set(CMAKE_C_COMPILER        "gcc")
set(CMAKE_CXX_COMPILER      "g++")

if(NOT ${CROSS_COMPILE} STREQUAL "")
    message("App info -- Cross Compilation = ${CROSS_COMPILE}")
    set(CMAKE_C_COMPILER "${CROSS_COMPILE}gcc")
    set(CMAKE_CXX_COMPILER "${CROSS_COMPILE}g++")
endif()

message("App info -- C compiler   : ${CMAKE_C_COMPILER}")
message("App info -- C++ compiler : ${CMAKE_CXX_COMPILER}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SW_MODEL_FLAG} -Wall -Wno-unused-function")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L /usr/local/lib -pthread" )

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_BINARY_DIR}/include)
add_subdirectory(${CMAKE_SOURCE_DIR}/protodrv_api)
add_subdirectory(${CMAKE_SOURCE_DIR}/streaming)

#include(version.cmake)

add_custom_target(version
  ${CMAKE_COMMAND} -D SRC=${CMAKE_SOURCE_DIR}/include/app_version.h.in
                   -D DST=${CMAKE_BINARY_DIR}/include/app_version.h
                   -P ${CMAKE_SOURCE_DIR}/version.cmake
  )

if(${TEST})
    add_subdirectory(test)
endif()

file(GLOB main_FILE main.cpp)
add_executable(etherlink ${main_FILE})
target_link_libraries(etherlink LINK_PUBLIC protodrv_api streaming)
add_dependencies(etherlink version)
install(TARGETS etherlink DESTINATION bin)
