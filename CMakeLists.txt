# Syphax-Engine - Ougi Washi

cmake_minimum_required(VERSION 3.22.1)

project(SYPHAX_ENGINE LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(SE_IS_TOP_LEVEL OFF)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	set(SE_IS_TOP_LEVEL ON)
endif()

option(SE_BUILD_EXAMPLES "Build Syphax examples" ${SE_IS_TOP_LEVEL})
option(SE_INSTALL "Enable install/export targets for Syphax" OFF)
option(SE_USE_SYSTEM_GLFW "Use system GLFW instead of vendored GLFW" OFF)
option(SE_USE_SYSTEM_MINIAUDIO "Use system miniaudio instead of vendored miniaudio" OFF)
option(SE_INSTALL_VENDOR_DEPS "Install vendored dependency libraries when vendored mode is active" ${SE_IS_TOP_LEVEL})

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

set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")
set(EXAMPLES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/examples")

set(SE_RENDER_BACKEND_DEFINE "")
set(SE_WINDOW_BACKEND_DEFINE "")
set(SE_CONFIG_NEEDS_OPENGL OFF)

if(SE_BACKEND_RENDER STREQUAL "gl")
	find_package(OpenGL REQUIRED)
	set(SE_RENDER_BACKEND_DEFINE SE_RENDER_BACKEND_OPENGL)
	set(SE_CONFIG_NEEDS_OPENGL ON)
elseif(SE_BACKEND_RENDER STREQUAL "gles")
	find_library(SE_GLES_LIBRARY NAMES GLESv2)
	find_library(SE_EGL_LIBRARY NAMES EGL)
	if(NOT SE_GLES_LIBRARY OR NOT SE_EGL_LIBRARY)
		message(FATAL_ERROR "SE_BACKEND_RENDER=gles requires GLESv2 and EGL libraries.")
	endif()
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

set(SE_GLFW_TARGET "")
set(SE_GLFW_INCLUDE_DIR "")
set(SE_GLFW_BUILD_LINK_ITEM "")
set(SE_GLFW_INSTALL_LINK_ITEM "-lglfw")
if(SE_USE_SYSTEM_GLFW)
	message(STATUS "Resolving system GLFW")
	find_path(SE_GLFW_SYSTEM_INCLUDE_DIR NAMES GLFW/glfw3.h)
	find_library(SE_GLFW_SYSTEM_LIBRARY NAMES glfw3 glfw)
	if(SE_GLFW_SYSTEM_INCLUDE_DIR AND SE_GLFW_SYSTEM_LIBRARY)
		set(SE_GLFW_INCLUDE_DIR "${SE_GLFW_SYSTEM_INCLUDE_DIR}")
		set(SE_GLFW_BUILD_LINK_ITEM "${SE_GLFW_SYSTEM_LIBRARY}")
	endif()
else()
	message(STATUS "Using vendored GLFW")
	set(GLFW_INSTALL OFF CACHE BOOL "Generate installation targets" FORCE)
	set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build GLFW docs" FORCE)
	set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build GLFW examples" FORCE)
	set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build GLFW tests" FORCE)
	if(TARGET glfw)
		set(SE_GLFW_TARGET glfw)
	elseif(TARGET glfw3::glfw)
		set(SE_GLFW_TARGET glfw3::glfw)
	else()
		add_subdirectory(${LIB_DIR}/glfw EXCLUDE_FROM_ALL)
		if(TARGET glfw)
			set(SE_GLFW_TARGET glfw)
		elseif(TARGET glfw3::glfw)
			set(SE_GLFW_TARGET glfw3::glfw)
		endif()
	endif()
	if(NOT SE_GLFW_TARGET STREQUAL "")
		set(SE_GLFW_INCLUDE_DIR "${LIB_DIR}/glfw/include")
		if(SE_INSTALL)
			set(SE_GLFW_BUILD_LINK_ITEM "$<TARGET_LINKER_FILE:${SE_GLFW_TARGET}>")
		else()
			set(SE_GLFW_BUILD_LINK_ITEM "${SE_GLFW_TARGET}")
		endif()
		get_target_property(SE_GLFW_OUTPUT_NAME ${SE_GLFW_TARGET} OUTPUT_NAME)
		if(NOT SE_GLFW_OUTPUT_NAME OR SE_GLFW_OUTPUT_NAME STREQUAL "SE_GLFW_OUTPUT_NAME-NOTFOUND")
			set(SE_GLFW_OUTPUT_NAME "${SE_GLFW_TARGET}")
		endif()
		set(SE_GLFW_INSTALL_LINK_ITEM "-l${SE_GLFW_OUTPUT_NAME}")
	endif()
endif()

if(SE_GLFW_BUILD_LINK_ITEM STREQUAL "")
	message(FATAL_ERROR "Unable to resolve GLFW. Use vendored mode or provide system GLFW include/library paths.")
endif()

set(SE_MINIAUDIO_TARGET "")
set(SE_MINIAUDIO_INCLUDE_DIR "")
set(SE_MINIAUDIO_BUILD_LINK_ITEM "")
set(SE_MINIAUDIO_INSTALL_LINK_ITEM "-lminiaudio")
if(SE_USE_SYSTEM_MINIAUDIO)
	message(STATUS "Resolving system miniaudio")
	find_path(SE_MINIAUDIO_SYSTEM_INCLUDE_DIR NAMES miniaudio/miniaudio.h miniaudio.h PATH_SUFFIXES miniaudio)
	find_library(SE_MINIAUDIO_SYSTEM_LIBRARY NAMES miniaudio)
	if(SE_MINIAUDIO_SYSTEM_INCLUDE_DIR AND SE_MINIAUDIO_SYSTEM_LIBRARY)
		set(SE_MINIAUDIO_INCLUDE_DIR "${SE_MINIAUDIO_SYSTEM_INCLUDE_DIR}")
		set(SE_MINIAUDIO_BUILD_LINK_ITEM "${SE_MINIAUDIO_SYSTEM_LIBRARY}")
	endif()
else()
	message(STATUS "Using vendored miniaudio")
	set(MINIAUDIO_INSTALL OFF CACHE BOOL "Enable installation targets" FORCE)
	set(MINIAUDIO_BUILD_EXAMPLES OFF CACHE BOOL "Build miniaudio examples" FORCE)
	set(MINIAUDIO_BUILD_TESTS OFF CACHE BOOL "Build miniaudio tests" FORCE)
	set(MINIAUDIO_BUILD_TOOLS OFF CACHE BOOL "Build miniaudio tools" FORCE)
	if(TARGET miniaudio)
		set(SE_MINIAUDIO_TARGET miniaudio)
	elseif(TARGET miniaudio::miniaudio)
		set(SE_MINIAUDIO_TARGET miniaudio::miniaudio)
	else()
		add_subdirectory(${LIB_DIR}/miniaudio EXCLUDE_FROM_ALL)
		if(TARGET miniaudio)
			set(SE_MINIAUDIO_TARGET miniaudio)
		elseif(TARGET miniaudio::miniaudio)
			set(SE_MINIAUDIO_TARGET miniaudio::miniaudio)
		endif()
	endif()
	if(NOT SE_MINIAUDIO_TARGET STREQUAL "")
		set(SE_MINIAUDIO_INCLUDE_DIR "${LIB_DIR}")
		if(SE_INSTALL)
			set(SE_MINIAUDIO_BUILD_LINK_ITEM "$<TARGET_LINKER_FILE:${SE_MINIAUDIO_TARGET}>")
		else()
			set(SE_MINIAUDIO_BUILD_LINK_ITEM "${SE_MINIAUDIO_TARGET}")
		endif()
		get_target_property(SE_MINIAUDIO_OUTPUT_NAME ${SE_MINIAUDIO_TARGET} OUTPUT_NAME)
		if(NOT SE_MINIAUDIO_OUTPUT_NAME OR SE_MINIAUDIO_OUTPUT_NAME STREQUAL "SE_MINIAUDIO_OUTPUT_NAME-NOTFOUND")
			set(SE_MINIAUDIO_OUTPUT_NAME "${SE_MINIAUDIO_TARGET}")
		endif()
		set(SE_MINIAUDIO_INSTALL_LINK_ITEM "-l${SE_MINIAUDIO_OUTPUT_NAME}")
	endif()
endif()

if(SE_MINIAUDIO_BUILD_LINK_ITEM STREQUAL "")
	message(FATAL_ERROR "Unable to resolve miniaudio. Use vendored mode or provide system miniaudio include/library paths.")
endif()

file(GLOB SE_ENGINE_SOURCES CONFIGURE_DEPENDS
	"${SRC_DIR}/*.c"
	"${SRC_DIR}/*.cpp"
	"${SRC_DIR}/**/*.c"
	"${SRC_DIR}/**/*.cpp")

add_library(se_engine ${SE_ENGINE_SOURCES})
add_library(syphax::engine ALIAS se_engine)
add_library(MAIN ALIAS se_engine)

set_target_properties(se_engine PROPERTIES EXPORT_NAME engine)

if(NOT SE_GLFW_TARGET STREQUAL "")
	add_dependencies(se_engine ${SE_GLFW_TARGET})
endif()
if(NOT SE_MINIAUDIO_TARGET STREQUAL "")
	add_dependencies(se_engine ${SE_MINIAUDIO_TARGET})
endif()

target_include_directories(
	se_engine
	PUBLIC
		$<BUILD_INTERFACE:${INCLUDE_DIR}>
		$<BUILD_INTERFACE:${LIB_DIR}>
		$<BUILD_INTERFACE:${LIB_DIR}/stb>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/stb>
	PRIVATE
		${SRC_DIR})

target_include_directories(
	se_engine
	BEFORE
	PRIVATE
		${SE_GLFW_INCLUDE_DIR}
		${SE_MINIAUDIO_INCLUDE_DIR})

target_link_libraries(
	se_engine
	PRIVATE
		$<BUILD_INTERFACE:${SE_GLFW_BUILD_LINK_ITEM}>
		$<BUILD_INTERFACE:${SE_MINIAUDIO_BUILD_LINK_ITEM}>
)

target_link_libraries(
	se_engine
	INTERFACE
		$<INSTALL_INTERFACE:${SE_GLFW_INSTALL_LINK_ITEM}>
		$<INSTALL_INTERFACE:${SE_MINIAUDIO_INSTALL_LINK_ITEM}>)

if(SE_BACKEND_RENDER STREQUAL "gl")
	target_link_libraries(
		se_engine
		PRIVATE
			$<BUILD_INTERFACE:OpenGL::GL>)
	target_link_libraries(
		se_engine
		INTERFACE
			$<INSTALL_INTERFACE:OpenGL::GL>)
elseif(SE_BACKEND_RENDER STREQUAL "gles")
	target_link_libraries(
		se_engine
		PRIVATE
			$<BUILD_INTERFACE:${SE_GLES_LIBRARY}>
			$<BUILD_INTERFACE:${SE_EGL_LIBRARY}>)
	target_link_libraries(
		se_engine
		INTERFACE
			$<INSTALL_INTERFACE:GLESv2>
			$<INSTALL_INTERFACE:EGL>)
endif()

if(UNIX)
	target_link_libraries(
		se_engine
		PRIVATE
			$<BUILD_INTERFACE:m>)
	target_link_libraries(
		se_engine
		INTERFACE
			$<INSTALL_INTERFACE:-lm>)
endif()

target_link_directories(
	se_engine
	INTERFACE
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_LIBDIR}>)

target_compile_definitions(
	se_engine
	PRIVATE
		${SE_RENDER_BACKEND_DEFINE}
		${SE_WINDOW_BACKEND_DEFINE}
		SE_BACKEND_RENDER_NAME="${SE_BACKEND_RENDER}"
		SE_BACKEND_PLATFORM_NAME="${SE_BACKEND_PLATFORM}")

target_compile_options(se_engine PRIVATE -Wall $<$<CONFIG:Debug>:-O0>)

message(STATUS "Configured render backend: ${SE_BACKEND_RENDER}")
message(STATUS "Configured platform backend: ${SE_BACKEND_PLATFORM}")
message(STATUS "Syphax examples enabled: ${SE_BUILD_EXAMPLES}")
message(STATUS "Syphax install/export enabled: ${SE_INSTALL}")
message(STATUS "Syphax vendored dependency install enabled: ${SE_INSTALL_VENDOR_DEPS}")
message(STATUS "Resolved GLFW target: ${SE_GLFW_TARGET}")
message(STATUS "Resolved GLFW build link item: ${SE_GLFW_BUILD_LINK_ITEM}")
message(STATUS "Resolved miniaudio target: ${SE_MINIAUDIO_TARGET}")
message(STATUS "Resolved miniaudio build link item: ${SE_MINIAUDIO_BUILD_LINK_ITEM}")

if(SE_BUILD_EXAMPLES)
	function(se_add_example example_file)
		get_filename_component(example_name "${example_file}" NAME_WE)
		if(TARGET "${example_name}")
			message(FATAL_ERROR "Duplicate example target '${example_name}' from ${example_file}")
		endif()
		message(STATUS "Adding example ${example_name} (${example_file})")
		add_executable("${example_name}" "${example_file}")
		target_link_libraries("${example_name}" PRIVATE se_engine)
		target_compile_definitions("${example_name}" PRIVATE RESOURCES_DIR="${RESOURCES_DIR}/")
		target_compile_options("${example_name}" PRIVATE -Wall $<$<CONFIG:Debug>:-O0>)
		set_target_properties("${example_name}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}")
	endfunction()

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
		se_add_example("${EXAMPLE}")
	endforeach()
endif()

if(SE_INSTALL)
	if(SE_INSTALL_VENDOR_DEPS AND NOT SE_USE_SYSTEM_GLFW AND NOT SE_GLFW_TARGET STREQUAL "")
		install(
			TARGETS ${SE_GLFW_TARGET}
			ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
			LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
			RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
		install(
			FILES
				${LIB_DIR}/glfw/include/GLFW/glfw3.h
				${LIB_DIR}/glfw/include/GLFW/glfw3native.h
			DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/GLFW)
	endif()

	if(SE_INSTALL_VENDOR_DEPS AND NOT SE_USE_SYSTEM_MINIAUDIO AND NOT SE_MINIAUDIO_TARGET STREQUAL "")
		install(
			TARGETS ${SE_MINIAUDIO_TARGET}
			ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
			LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
			RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
		install(
			FILES ${LIB_DIR}/miniaudio/miniaudio.h
			DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/miniaudio)
	endif()

	install(
		TARGETS se_engine
		EXPORT SyphaxEngineTargets
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

	install(
		DIRECTORY ${INCLUDE_DIR}/
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		FILES_MATCHING PATTERN "*.h")

	install(
		FILES
			${LIB_DIR}/syphax/s_types.h
			${LIB_DIR}/syphax/s_array.h
			${LIB_DIR}/syphax/s_memory.h
			${LIB_DIR}/syphax/s_thread.h
			${LIB_DIR}/syphax/s_json.h
			${LIB_DIR}/syphax/s_files.h
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/syphax)

	install(
		FILES
			${LIB_DIR}/stb/stb_image.h
			${LIB_DIR}/stb/stb_truetype.h
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/stb)

	install(
		DIRECTORY ${RESOURCES_DIR}/public/
		DESTINATION ${CMAKE_INSTALL_DATADIR}/syphax-engine/public)

	set(SE_ENGINE_PACKAGE_VERSION "0.0.0")
	if(DEFINED PROJECT_VERSION AND NOT "${PROJECT_VERSION}" STREQUAL "")
		set(SE_ENGINE_PACKAGE_VERSION "${PROJECT_VERSION}")
	endif()

	configure_package_config_file(
		${CMAKE_CURRENT_SOURCE_DIR}/cmake/SyphaxEngineConfig.cmake.in
		${CMAKE_CURRENT_BINARY_DIR}/SyphaxEngineConfig.cmake
		INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SyphaxEngine
		NO_SET_AND_CHECK_MACRO
		NO_CHECK_REQUIRED_COMPONENTS_MACRO)

	write_basic_package_version_file(
		${CMAKE_CURRENT_BINARY_DIR}/SyphaxEngineConfigVersion.cmake
		VERSION ${SE_ENGINE_PACKAGE_VERSION}
		COMPATIBILITY SameMajorVersion)

	install(
		EXPORT SyphaxEngineTargets
		FILE SyphaxEngineTargets.cmake
		NAMESPACE syphax::
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SyphaxEngine)

	install(
		FILES
			${CMAKE_CURRENT_BINARY_DIR}/SyphaxEngineConfig.cmake
			${CMAKE_CURRENT_BINARY_DIR}/SyphaxEngineConfigVersion.cmake
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SyphaxEngine)
endif()
