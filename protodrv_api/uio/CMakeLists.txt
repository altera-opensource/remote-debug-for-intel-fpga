cmake_minimum_required(VERSION 3.0.0)

# Set the PROTO_DRIVER_OPTIONS name on the parent variable
# use "cmake -DBUILD_PROTO_API_MODE=UIO" to set the option (Note: This is assumed to be the default mode). Can also be set with ccmake and vscode cmake options.
set(PROTO_DRIVER_OPTIONS ${PROTO_DRIVER_OPTIONS} UIO PARENT_SCOPE)

# Only include if the BUILD_PROTO_API_MODE is set to UIO (case sensitive)
if( BUILD_PROTO_API_MODE STREQUAL UIO)
    message("INFO: Target Platform: Linux UIO driver")
    file(GLOB c_FILES src/*.c)

    add_library(${PROJECT_NAME} ${c_FILES})
    add_library(${PROJECT_NAME}_sw_tst ${c_FILES})
    set_target_properties (${PROJECT_NAME}_sw_tst PROPERTIES COMPILE_DEFINITIONS "UIO_UNIT_TEST_SW_MODEL_MODE")

    target_include_directories(${PROJECT_NAME} PUBLIC inc)
    target_include_directories(${PROJECT_NAME} PUBLIC "$<TARGET_PROPERTY:${PROJECT_NAME}_common,INTERFACE_INCLUDE_DIRECTORIES>")
    target_include_directories(${PROJECT_NAME}_sw_tst PUBLIC inc)
    target_include_directories(${PROJECT_NAME}_sw_tst PUBLIC "$<TARGET_PROPERTY:${PROJECT_NAME}_common,INTERFACE_INCLUDE_DIRECTORIES>")

    if(${TEST})
        add_subdirectory(test)
    endif()
endif()

