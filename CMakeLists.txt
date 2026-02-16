# Syphax-Engine - Ougi Washi

## General
cmake_minimum_required(VERSION 3.22.1)

project(SYPHAX_ENGINE LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -O0)

set(SE_BACKEND_RENDER "gl" CACHE STRING "Render backend: gl|gles|webgl|metal|vulkan|software")
set_property(CACHE SE_BACKEND_RENDER PROPERTY STRINGS gl gles webgl metal vulkan software)
set(SE_BACKEND_PLATFORM "desktop_glfw" CACHE STRING "Platform backend: desktop_glfw|android|ios|web|terminal")
set_property(CACHE SE_BACKEND_PLATFORM PROPERTY STRINGS desktop_glfw android ios web terminal)

set(SE_SUPPORTED_RENDER_BACKENDS gl gles webgl metal vulkan software)
list(FIND SE_SUPPORTED_RENDER_BACKENDS "${SE_BACKEND_RENDER}" SE_RENDER_BACKEND_INDEX)
if(SE_RENDER_BACKEND_INDEX EQUAL -1)
	message(FATAL_ERROR "Invalid SE_BACKEND_RENDER='${SE_BACKEND_RENDER}'. Valid values: ${SE_SUPPORTED_RENDER_BACKENDS}")
endif()

set(SE_SUPPORTED_PLATFORM_BACKENDS desktop_glfw android ios web terminal)
list(FIND SE_SUPPORTED_PLATFORM_BACKENDS "${SE_BACKEND_PLATFORM}" SE_PLATFORM_BACKEND_INDEX)
if(SE_PLATFORM_BACKEND_INDEX EQUAL -1)
	message(FATAL_ERROR "Invalid SE_BACKEND_PLATFORM='${SE_BACKEND_PLATFORM}'. Valid values: ${SE_SUPPORTED_PLATFORM_BACKENDS}")
endif()

set(SE_RENDER_BACKEND_DEFINE "")
set(SE_WINDOW_BACKEND_DEFINE "")
set(SE_RENDER_LINK_LIBS "")

if(SE_BACKEND_RENDER STREQUAL "gl")
	find_package(OpenGL REQUIRED)
	list(APPEND SE_RENDER_LINK_LIBS OpenGL::GL)
	set(SE_RENDER_BACKEND_DEFINE SE_RENDER_BACKEND_OPENGL)
elseif(SE_BACKEND_RENDER STREQUAL "gles")
	find_library(SE_GLES_LIBRARY NAMES GLESv2)
	find_library(SE_EGL_LIBRARY NAMES EGL)
	if(NOT SE_GLES_LIBRARY OR NOT SE_EGL_LIBRARY)
		message(FATAL_ERROR "SE_BACKEND_RENDER=gles requires GLESv2 and EGL libraries.")
	endif()
	list(APPEND SE_RENDER_LINK_LIBS "${SE_GLES_LIBRARY}" "${SE_EGL_LIBRARY}")
	set(SE_RENDER_BACKEND_DEFINE SE_RENDER_BACKEND_GLES)
else()
	message(FATAL_ERROR "SE_BACKEND_RENDER='${SE_BACKEND_RENDER}' is not implemented yet. Supported today: gl, gles.")
endif()

if(SE_BACKEND_PLATFORM STREQUAL "desktop_glfw")
	set(SE_WINDOW_BACKEND_DEFINE SE_WINDOW_BACKEND_GLFW)
elseif(SE_BACKEND_PLATFORM STREQUAL "terminal")
	set(SE_WINDOW_BACKEND_DEFINE SE_WINDOW_BACKEND_TERMINAL)
	message(STATUS "SE_BACKEND_PLATFORM=terminal is experimental.")
else()
	message(FATAL_ERROR "SE_BACKEND_PLATFORM='${SE_BACKEND_PLATFORM}' is not implemented yet. Supported today: desktop_glfw, terminal.")
endif()

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
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")
set(EXAMPLES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/examples")

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
	file(GLOB CURRENT_SRC_FILES CONFIGURE_DEPENDS	"${arg_source_path}/*.c" 
					"${arg_source_path}/*.cpp"
					"${arg_source_path}/**/*.c" 
					"${arg_source_path}/**/*.cpp")
	add_library(${arg_lib_name} STATIC	${CURRENT_SRC_FILES})
	target_include_directories(${arg_lib_name} PUBLIC ${arg_include_path})
    target_compile_definitions(${arg_lib_name} PUBLIC RESOURCES_DIR="${RESOURCES_DIR}/")
endmacro()

# Main module
set(MAIN_MODULE_INCLUDES ${INCLUDE_DIR} ${LIB_DIR})
setup_library(MAIN ${SRC_DIR} "${MAIN_MODULE_INCLUDES}")
target_include_directories(MAIN PRIVATE ${SRC_DIR})
target_link_libraries(MAIN PUBLIC glfw miniaudio ${SE_RENDER_LINK_LIBS})
target_compile_definitions(
	MAIN
	PUBLIC
	${SE_RENDER_BACKEND_DEFINE}
	${SE_WINDOW_BACKEND_DEFINE}
	SE_BACKEND_RENDER_NAME="${SE_BACKEND_RENDER}"
	SE_BACKEND_PLATFORM_NAME="${SE_BACKEND_PLATFORM}")

message(STATUS "Configured render backend: ${SE_BACKEND_RENDER}")
message(STATUS "Configured platform backend: ${SE_BACKEND_PLATFORM}")

macro(setup_executable arg_exec_file arg_exec_name modules)
    message(STATUS "Generating executable ${arg_exec_name}")
    add_executable(${arg_exec_name} "${arg_exec_file}")
    target_link_libraries(${arg_exec_name} PUBLIC MAIN)
    target_compile_definitions(${arg_exec_name} PUBLIC RESOURCES_DIR="${RESOURCES_DIR}/")
    set_target_properties(${arg_exec_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})
endmacro()

set(EXAMPLES_ADVANCED_DIR "${EXAMPLES_DIR}/advanced")
file(GLOB EXAMPLES_BEGINNER CONFIGURE_DEPENDS
	"${EXAMPLES_DIR}/*.c"
	"${EXAMPLES_DIR}/*.cpp")
file(GLOB EXAMPLES_ADVANCED CONFIGURE_DEPENDS
	"${EXAMPLES_ADVANCED_DIR}/*.c"
	"${EXAMPLES_ADVANCED_DIR}/*.cpp")

list(SORT EXAMPLES_BEGINNER)
list(SORT EXAMPLES_ADVANCED)
set(EXAMPLES ${EXAMPLES_BEGINNER} ${EXAMPLES_ADVANCED})

foreach(EXAMPLE ${EXAMPLES})
	get_filename_component(EXAMPLE_NAME ${EXAMPLE} NAME_WE)
	if(TARGET ${EXAMPLE_NAME})
		message(FATAL_ERROR "Duplicate example target '${EXAMPLE_NAME}' from ${EXAMPLE}")
	endif()
	message(STATUS "Adding example ${EXAMPLE_NAME} (${EXAMPLE})")
	setup_executable(${EXAMPLE} ${EXAMPLE_NAME} MAIN)
endforeach()
