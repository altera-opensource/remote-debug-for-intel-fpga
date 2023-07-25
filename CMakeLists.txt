cmake_minimum_required(VERSION 3.2)

option(SW_MODEL "SW Model Unit Test")

set(CMAKE_C_COMPILER        "gcc")
set(CMAKE_CXX_COMPILER      "g++")

if(NOT ${CROSS_COMPILE} STREQUAL "")
    message("App info -- Cross Compilation = ${CROSS_COMPILE}")
    set(CMAKE_C_COMPILER "${CROSS_COMPILE}gcc")
    set(CMAKE_CXX_COMPILER "${CROSS_COMPILE}g++")
endif()

option(FPGA_PLATFORM_FORCE_64BIT_MMIO_EMULATION_WITH_32BIT "Option to use 32-bit MMIO for 64-bit MMIO access on certainly PCIe endpoint lacking 64-bit MMIO support" OFF) # Disabled by default
if(FPGA_PLATFORM_FORCE_64BIT_MMIO_EMULATION_WITH_32BIT)
    add_definitions(-DFPGA_PLATFORM_FORCE_64BIT_MMIO_EMULATION_WITH_32BIT)
endif()

message("App info -- C compiler   : ${CMAKE_C_COMPILER}")
message("App info -- C++ compiler : ${CMAKE_CXX_COMPILER}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SW_MODEL_FLAG} -Wall -Wno-unused-function")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L /usr/local/lib -pthread" )

project(remote-debug-for-intel-fpga)

add_subdirectory(${CMAKE_SOURCE_DIR}/protodrv_api)
add_subdirectory(${CMAKE_SOURCE_DIR}/streaming)

#include(version.cmake)

add_custom_target(version
  ${CMAKE_COMMAND} -D SRC=${CMAKE_SOURCE_DIR}/app_version.h.in
                   -D DST=${CMAKE_BINARY_DIR}/app_version.h
                   -P ${CMAKE_SOURCE_DIR}/version.cmake
  )

file(GLOB main_FILE main.cpp)
add_executable(etherlink ${main_FILE})
target_include_directories(etherlink PRIVATE "$<TARGET_PROPERTY:protodrv_lib,INTERFACE_INCLUDE_DIRECTORIES>" ${CMAKE_BINARY_DIR})
target_link_libraries(etherlink LINK_PUBLIC protodrv_lib protodrv_lib_common streaming )
add_dependencies(etherlink version)
install(TARGETS etherlink DESTINATION bin)
