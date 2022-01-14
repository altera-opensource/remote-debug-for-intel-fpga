file(GLOB all_FILES *.c *cpp)

add_library(streaming ${all_FILES})

target_include_directories(streaming PUBLIC ${CMAKE_CURRENT_DIR})
