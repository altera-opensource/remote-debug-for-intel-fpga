file(GLOB cpp_FILES *.cpp)

add_executable(protodrv_api_uio_utst ${cpp_FILES})

target_link_libraries(protodrv_api_uio_utst LINK_PUBLIC protodrv_lib_sw_tst protodrv_lib_common gtest)
target_include_directories(protodrv_api_uio_utst PUBLIC "$<TARGET_PROPERTY:protodrv_lib,INTERFACE_INCLUDE_DIRECTORIES>")

