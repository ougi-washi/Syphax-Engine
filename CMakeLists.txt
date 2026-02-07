# Syphax-Engine - Ougi Washi

## General
cmake_minimum_required(VERSION 3.22.1)

project(SYPHAX_ENGINE LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -O0)

if(NOT CMAKE_GENERATOR)
    find_program(NINJA_EXECUTABLE ninja)
    if(NINJA_EXECUTABLE)
        set(CMAKE_GENERATOR "Ninja" CACHE INTERNAL "Ninja")
        message(STATUS "Ninja build system found. Using Ninja generator.")
    else()
        message(STATUS "Ninja build system not found. Using default generator.")
    endif()
endif()

# Directory setup
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")
set(EXAMPLES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/examples")

# GLSL
find_package(OpenGL REQUIRED)

# GLFW
message(STATUS "Adding GLFW library")
add_subdirectory(${LIB_DIR}/glfw)

# STB
message(STATUS "Adding STB library")
include_directories(${LIB_DIR}/stb)

# MINIAUDIO
message(STATUS "Adding MINIAUDIO library")
add_subdirectory(${LIB_DIR}/miniaudio)

# setup library macro
macro(setup_library arg_lib_name arg_source_path arg_include_path)
	message(STATUS "Adding ${arg_lib_name}")
	file(GLOB CURRENT_SRC_FILES	"${arg_source_path}/*.c" 
					"${arg_source_path}/*.cpp"
					"${arg_source_path}/**/*.c" 
					"${arg_source_path}/**/*.cpp")
	add_library(${arg_lib_name} STATIC	${CURRENT_SRC_FILES})
	target_include_directories(${arg_lib_name} PUBLIC ${arg_include_path})
    target_compile_definitions(${arg_lib_name} PUBLIC RESOURCES_DIR="${RESOURCES_DIR}/")
endmacro()

# Main module
set(MAIN_MODULE_INCLUDES ${SRC_DIR} ${LIB_DIR})
setup_library(MAIN ${SRC_DIR} "${MAIN_MODULE_INCLUDES}")
target_link_libraries(MAIN PUBLIC glfw OpenGL::GL miniaudio)

macro(setup_executable arg_exec_dir arg_exec_name modules)
    message(STATUS "Generating executable ${arg_exec_name}")
    add_executable(${arg_exec_name} "${arg_exec_dir}/${arg_exec_name}.c")
    target_link_libraries(${arg_exec_name} PUBLIC MAIN)
    target_compile_definitions(${arg_exec_name} PUBLIC RESOURCES_DIR="${RESOURCES_DIR}/")
    set_target_properties(${arg_exec_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})
endmacro()

file(GLOB EXAMPLES 	"${EXAMPLES_DIR}/*.c" 
			"${EXAMPLES_DIR}/*.cpp"
			"${EXAMPLES_DIR}/**/*.c" 
			"${EXAMPLES_DIR}/**/*.cpp")

foreach(EXAMPLE ${EXAMPLES})
   	get_filename_component(EXAMPLE_NAME ${EXAMPLE} NAME_WE)
   	message(STATUS "Adding example ${EXAMPLE_NAME}")
    setup_executable(${EXAMPLES_DIR} ${EXAMPLE_NAME} MAIN)
endforeach()

