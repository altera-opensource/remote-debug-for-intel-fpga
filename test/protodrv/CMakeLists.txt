file(GLOB cpp_FILES *.cpp)

add_executable(protoDriverTest ${cpp_FILES})

target_link_libraries(protoDriverTest LINK_PUBLIC protodrv_api_sw gtest)

