cmake_minimum_required(VERSION 3.0.0)

file(GLOB c_FILES src/*.c)

add_library(${PROJECT_NAME}_common ${c_FILES})
#add_library(${PROJECT_NAME}_sw_tst_common ${c_FILES})
#set_target_properties (${PROJECT_NAME}_sw_tst_common PROPERTIES COMPILE_DEFINITIONS "UIO_UNIT_TEST_SW_MODEL_MODE")

target_include_directories(${PROJECT_NAME}_common PUBLIC inc)
target_include_directories(${PROJECT_NAME}_common PUBLIC "$<TARGET_PROPERTY:${PROJECT_NAME},INTERFACE_INCLUDE_DIRECTORIES>")
#target_include_directories(${PROJECT_NAME}_sw_tst_common PUBLIC inc)

