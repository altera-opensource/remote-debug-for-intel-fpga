file(GLOB c_FILES *.c)

add_library(protodrv_api ${c_FILES})

target_include_directories(protodrv_api PUBLIC ${CMAKE_CURRENT_DIR})
