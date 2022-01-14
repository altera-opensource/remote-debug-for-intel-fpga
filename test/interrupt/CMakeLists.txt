file(GLOB c_FILES *.c)

add_executable(interruptTest ${c_FILES})

target_link_libraries(interruptTest LINK_PUBLIC protodrv_api)
