file(GLOB c_FILES *.c)

add_executable(protodrv_api_uio_int_tst ${c_FILES})

target_link_libraries(protodrv_api_uio_int_tst LINK_PUBLIC protodrv_lib protodrv_lib_common)
target_include_directories(protodrv_api_uio_int_tst PUBLIC "$<TARGET_PROPERTY:protodrv_lib,INTERFACE_INCLUDE_DIRECTORIES>")
target_include_directories(protodrv_api_uio_int_tst PUBLIC "$<TARGET_PROPERTY:protodrv_lib_common,INTERFACE_INCLUDE_DIRECTORIES>")
