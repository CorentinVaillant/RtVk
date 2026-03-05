cmake_minimum_required(VERSION 3.12)

project("RtVk")

find_package(Vulkan REQUIRED)

if(LINUX)
  option(CMAKE_CXX_COMPILER clang++)
endif()

option(SDL_X11_XTEST OFF)

add_subdirectory(libs)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

add_subdirectory(src)

message("Compiler : ${CMAKE_CXX_COMPILER}")

# SLANG shader compilation
set(SHADER_HEADER_DIR ${PROJECT_SOURCE_DIR}/build/src/shaders)
file(MAKE_DIRECTORY ${SHADER_HEADER_DIR})   # ← fix ici

find_program(SLANG_VALIDATOR slangc
    HINTS /usr/bin /usr/local/bin
          $ENV{VULKAN_SDK}/Bin/
          $ENV{VULKAN_SDK}/Bin32/
)

file(GLOB_RECURSE SLANG_SOURCE_FILE
    "${PROJECT_SOURCE_DIR}/shaders/src/*.slang"
)

set(SHADER_HEADER_FILES "")

foreach(SLANG_FILE ${SLANG_SOURCE_FILE})
    get_filename_component(FILE_NAME ${SLANG_FILE} NAME)
    get_filename_component(FILE_STEM ${SLANG_FILE} NAME_WE)

    set(SPIRV_FILE  "${CMAKE_BINARY_DIR}/shaders/${FILE_NAME}.spv")
    set(HEADER_FILE "${SHADER_HEADER_DIR}/${FILE_NAME}.h")

    string(TOUPPER "${FILE_STEM}" STEM_UPPER)
    string(REPLACE "-" "_" STEM_UPPER "${STEM_UPPER}")
    set(VAR_NAME "${STEM_UPPER}_SPIRV")

    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/shaders")

    add_custom_command(
        OUTPUT  ${SPIRV_FILE}
        COMMAND ${SLANG_VALIDATOR} -target spirv -o ${SPIRV_FILE} -- ${SLANG_FILE}
        DEPENDS ${SLANG_FILE}
        COMMENT "Compiling shader: ${FILE_NAME}"
    )

    add_custom_command(
        OUTPUT  ${HEADER_FILE}
        COMMAND ${CMAKE_COMMAND}
                -DINPUT_FILE=${SPIRV_FILE}
                -DOUTPUT_FILE=${HEADER_FILE}
                -DVARIABLE_NAME=${VAR_NAME}
                -P ${PROJECT_SOURCE_DIR}/cmake/spirv_to_header.cmake
        DEPENDS ${SPIRV_FILE}
        COMMENT "Generating header: ${FILE_NAME}.h"
    )

    list(APPEND SHADER_HEADER_FILES ${HEADER_FILE})
    message(STATUS "Registering shader: ${FILE_NAME} -> ${VAR_NAME}")
endforeach()

add_custom_target(Shaders DEPENDS ${SHADER_HEADER_FILES})
add_dependencies(RtVk Shaders)