file(GLOB c_FILES *.c)

add_library(protodrv_api ${c_FILES})
add_library(protodrv_api_sw ${c_FILES})
set_target_properties (protodrv_api_sw PROPERTIES COMPILE_DEFINITIONS "UIO_UNIT_TEST_SW_MODEL_MODE")

target_include_directories(protodrv_api PUBLIC ${CMAKE_CURRENT_DIR})
target_include_directories(protodrv_api_sw PUBLIC ${CMAKE_CURRENT_DIR})
