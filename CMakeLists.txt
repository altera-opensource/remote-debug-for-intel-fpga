cmake_minimum_required(VERSION 3.2)
project(Fpga_remote_debug)

option(SW_MODEL "SW Model Unit Test")

set(CMAKE_C_COMPILER        "gcc")
set(CMAKE_CXX_COMPILER      "g++")

if(${SW_MODEL})
    set(SW_MODEL_FLAG "-DUIO_UNIT_TEST_SW_MODEL_MODE")
endif()

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

include_directories(include)
add_subdirectory(protodrv_api)
add_subdirectory(streaming)

file(GLOB main_FILE main.cpp)
add_executable(etherlink ${main_FILE})
target_link_libraries(etherlink LINK_PUBLIC protodrv_api streaming)
install(TARGETS etherlink DESTINATION bin)
