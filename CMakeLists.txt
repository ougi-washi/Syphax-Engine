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
option(SE_ANDROID_BUILD_ALL_EXAMPLES "Build compile-only Android shared libraries for every example source" OFF)

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
set(SE_ANDROID_ENTRY_SOURCE "${EXAMPLES_DIR}/hello_text.c" CACHE FILEPATH "Android NativeActivity entry source file")
set(SE_ANDROID_APP_ID "com.syphax.engine" CACHE STRING "Android application id")
set(SE_ANDROID_APP_NAME "Syphax Engine" CACHE STRING "Android application display name")
set(SE_ANDROID_LIB_NAME "syphax_android_app" CACHE STRING "Android NativeActivity library name")
set(SE_ANDROID_MIN_SDK "24" CACHE STRING "Android minSdkVersion for the NativeActivity package")
set(SE_ANDROID_TARGET_SDK "34" CACHE STRING "Android targetSdkVersion for the NativeActivity package")
set(SE_ANDROID_VERSION_CODE "1" CACHE STRING "Android versionCode for the NativeActivity package")
set(SE_ANDROID_VERSION_NAME "0.1.0" CACHE STRING "Android versionName for the NativeActivity package")

set(SE_RENDER_BACKEND_DEFINE "")
set(SE_WINDOW_BACKEND_DEFINE "")
set(SE_CONFIG_NEEDS_OPENGL OFF)

if(SE_BACKEND_RENDER STREQUAL "gl")
	find_package(OpenGL REQUIRED)
	set(SE_RENDER_BACKEND_DEFINE SE_RENDER_BACKEND_OPENGL)
	set(SE_CONFIG_NEEDS_OPENGL ON)
elseif(SE_BACKEND_RENDER STREQUAL "gles")
	find_library(SE_GLES_LIBRARY NAMES GLESv3 GLESv2)
	find_library(SE_EGL_LIBRARY NAMES EGL)
	if(NOT SE_GLES_LIBRARY OR NOT SE_EGL_LIBRARY)
		message(FATAL_ERROR "SE_BACKEND_RENDER=gles requires GLESv2 and EGL libraries.")
	endif()
	set(SE_RENDER_BACKEND_DEFINE SE_RENDER_BACKEND_GLES)
else()
	message(FATAL_ERROR "SE_BACKEND_RENDER='${SE_BACKEND_RENDER}' is not implemented yet. Supported today: gl, gles.")
endif()

if(SE_BACKEND_PLATFORM STREQUAL "android")
	if(NOT ANDROID)
		message(FATAL_ERROR "SE_BACKEND_PLATFORM=android requires the Android NDK CMake toolchain.")
	endif()
	if(NOT SE_BACKEND_RENDER STREQUAL "gles")
		message(FATAL_ERROR "SE_BACKEND_PLATFORM=android currently requires SE_BACKEND_RENDER=gles.")
	endif()
endif()

if(SE_BACKEND_PLATFORM STREQUAL "desktop_glfw")
	set(SE_WINDOW_BACKEND_DEFINE SE_WINDOW_BACKEND_GLFW)
elseif(SE_BACKEND_PLATFORM STREQUAL "android")
	set(SE_WINDOW_BACKEND_DEFINE SE_WINDOW_BACKEND_ANDROID)
elseif(SE_BACKEND_PLATFORM STREQUAL "terminal")
	set(SE_WINDOW_BACKEND_DEFINE SE_WINDOW_BACKEND_TERMINAL)
	message(STATUS "SE_BACKEND_PLATFORM=terminal is experimental.")
else()
	message(FATAL_ERROR "SE_BACKEND_PLATFORM='${SE_BACKEND_PLATFORM}' is not implemented yet. Supported today: desktop_glfw, android, terminal.")
endif()

set(SE_GLFW_TARGET "")
set(SE_GLFW_INCLUDE_DIR "")
set(SE_GLFW_BUILD_LINK_ITEM "")
set(SE_GLFW_INSTALL_LINK_ITEM "")
if(SE_BACKEND_PLATFORM STREQUAL "desktop_glfw" AND SE_USE_SYSTEM_GLFW)
	message(STATUS "Resolving system GLFW")
	find_path(SE_GLFW_SYSTEM_INCLUDE_DIR NAMES GLFW/glfw3.h)
	find_library(SE_GLFW_SYSTEM_LIBRARY NAMES glfw3 glfw)
	if(SE_GLFW_SYSTEM_INCLUDE_DIR AND SE_GLFW_SYSTEM_LIBRARY)
		set(SE_GLFW_INCLUDE_DIR "${SE_GLFW_SYSTEM_INCLUDE_DIR}")
		set(SE_GLFW_BUILD_LINK_ITEM "${SE_GLFW_SYSTEM_LIBRARY}")
	endif()
elseif(SE_BACKEND_PLATFORM STREQUAL "desktop_glfw")
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

if(SE_BACKEND_PLATFORM STREQUAL "desktop_glfw" AND SE_GLFW_BUILD_LINK_ITEM STREQUAL "")
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

if(NOT SE_GLFW_BUILD_LINK_ITEM STREQUAL "")
	target_link_libraries(
		se_engine
		PRIVATE
			$<BUILD_INTERFACE:${SE_GLFW_BUILD_LINK_ITEM}>)
endif()

target_link_libraries(
	se_engine
	PRIVATE
		$<BUILD_INTERFACE:${SE_MINIAUDIO_BUILD_LINK_ITEM}>)

if(NOT SE_GLFW_INSTALL_LINK_ITEM STREQUAL "")
	target_link_libraries(
		se_engine
		INTERFACE
			$<INSTALL_INTERFACE:${SE_GLFW_INSTALL_LINK_ITEM}>)
endif()

target_link_libraries(
	se_engine
	INTERFACE
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

if(SE_BACKEND_PLATFORM STREQUAL "android" AND SE_BUILD_EXAMPLES)
	message(STATUS "Disabling standalone example executables for Android builds.")
	set(SE_BUILD_EXAMPLES OFF)
endif()

message(STATUS "Configured render backend: ${SE_BACKEND_RENDER}")
message(STATUS "Configured platform backend: ${SE_BACKEND_PLATFORM}")
message(STATUS "Syphax examples enabled: ${SE_BUILD_EXAMPLES}")
message(STATUS "Syphax install/export enabled: ${SE_INSTALL}")
message(STATUS "Syphax vendored dependency install enabled: ${SE_INSTALL_VENDOR_DEPS}")
message(STATUS "Resolved GLFW target: ${SE_GLFW_TARGET}")
message(STATUS "Resolved GLFW build link item: ${SE_GLFW_BUILD_LINK_ITEM}")
message(STATUS "Resolved miniaudio target: ${SE_MINIAUDIO_TARGET}")
message(STATUS "Resolved miniaudio build link item: ${SE_MINIAUDIO_BUILD_LINK_ITEM}")

if(SE_BACKEND_PLATFORM STREQUAL "android")
	set(SE_ANDROID_NDK_ROOT "")
	if(DEFINED CMAKE_ANDROID_NDK AND NOT "${CMAKE_ANDROID_NDK}" STREQUAL "")
		set(SE_ANDROID_NDK_ROOT "${CMAKE_ANDROID_NDK}")
	elseif(DEFINED ANDROID_NDK AND NOT "${ANDROID_NDK}" STREQUAL "")
		set(SE_ANDROID_NDK_ROOT "${ANDROID_NDK}")
	endif()
	if(SE_ANDROID_NDK_ROOT STREQUAL "")
		message(FATAL_ERROR "Unable to resolve the Android NDK root for native_app_glue.")
	endif()
	if(NOT EXISTS "${SE_ANDROID_ENTRY_SOURCE}")
		message(FATAL_ERROR "SE_ANDROID_ENTRY_SOURCE='${SE_ANDROID_ENTRY_SOURCE}' does not exist.")
	endif()
	get_filename_component(SE_ANDROID_ENTRY_SOURCE_REALPATH "${SE_ANDROID_ENTRY_SOURCE}" REALPATH)

	set(SE_ANDROID_NATIVE_APP_GLUE_DIR "${SE_ANDROID_NDK_ROOT}/sources/android/native_app_glue")
	set(SE_ANDROID_NATIVE_APP_GLUE_SOURCE "${SE_ANDROID_NATIVE_APP_GLUE_DIR}/android_native_app_glue.c")
	if(NOT EXISTS "${SE_ANDROID_NATIVE_APP_GLUE_SOURCE}")
		message(FATAL_ERROR "native_app_glue source not found at ${SE_ANDROID_NATIVE_APP_GLUE_SOURCE}")
	endif()

	set(SE_ANDROID_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/android")
	set(SE_ANDROID_GENERATED_RES_DIR "${SE_ANDROID_GENERATED_DIR}/res/values")
	file(MAKE_DIRECTORY "${SE_ANDROID_GENERATED_RES_DIR}")

	configure_file(
		${CMAKE_CURRENT_SOURCE_DIR}/android/AndroidManifest.xml.in
		${SE_ANDROID_GENERATED_DIR}/AndroidManifest.xml
		@ONLY)
	configure_file(
		${CMAKE_CURRENT_SOURCE_DIR}/android/res/values/strings.xml.in
		${SE_ANDROID_GENERATED_RES_DIR}/strings.xml
		@ONLY)

	add_library(se_android_native_app_glue STATIC ${SE_ANDROID_NATIVE_APP_GLUE_SOURCE})
	target_include_directories(se_android_native_app_glue PUBLIC ${SE_ANDROID_NATIVE_APP_GLUE_DIR})
	target_include_directories(se_engine PRIVATE ${SE_ANDROID_NATIVE_APP_GLUE_DIR})

	add_library(se_android_entry OBJECT ${SE_ANDROID_ENTRY_SOURCE})
	target_compile_definitions(se_android_entry PRIVATE main=se_android_example_main)
	target_link_libraries(se_android_entry PRIVATE se_engine)

	add_library(
		${SE_ANDROID_LIB_NAME}
		SHARED
			android/launcher/se_android_main.c
			$<TARGET_OBJECTS:se_android_entry>)
	target_compile_options(${SE_ANDROID_LIB_NAME} PRIVATE -Wall $<$<CONFIG:Debug>:-O0>)
	target_include_directories(
		${SE_ANDROID_LIB_NAME}
		PRIVATE
			${SRC_DIR}
			${SE_ANDROID_NATIVE_APP_GLUE_DIR})
	target_link_libraries(
		${SE_ANDROID_LIB_NAME}
		PRIVATE
			se_engine
			se_android_native_app_glue
			android
			log)
	target_link_options(${SE_ANDROID_LIB_NAME} PRIVATE "LINKER:-u,ANativeActivity_onCreate")
	set_target_properties(
		${SE_ANDROID_LIB_NAME}
		PROPERTIES
			OUTPUT_NAME "${SE_ANDROID_LIB_NAME}"
			LIBRARY_OUTPUT_DIRECTORY "${BIN_DIR}/android/${ANDROID_ABI}"
			RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}/android/${ANDROID_ABI}")

	if(SE_ANDROID_BUILD_ALL_EXAMPLES)
		function(se_add_android_example_library example_file)
			get_filename_component(example_name "${example_file}" NAME_WE)
			set(entry_target "se_android_entry_${example_name}")
			set(library_target "se_android_example_${example_name}")

			if(TARGET "${entry_target}" OR TARGET "${library_target}")
				message(FATAL_ERROR "Duplicate Android example target '${example_name}' from ${example_file}")
			endif()

			add_library("${entry_target}" OBJECT "${example_file}")
			target_compile_definitions("${entry_target}" PRIVATE main=se_android_example_main)
			target_link_libraries("${entry_target}" PRIVATE se_engine)

			add_library(
				"${library_target}"
				SHARED
					android/launcher/se_android_main.c
					$<TARGET_OBJECTS:${entry_target}>)
			target_compile_options("${library_target}" PRIVATE -Wall $<$<CONFIG:Debug>:-O0>)
			target_include_directories(
				"${library_target}"
				PRIVATE
					${SRC_DIR}
					${SE_ANDROID_NATIVE_APP_GLUE_DIR})
			target_link_libraries(
				"${library_target}"
				PRIVATE
					se_engine
					se_android_native_app_glue
					android
					log)
			target_link_options("${library_target}" PRIVATE "LINKER:-u,ANativeActivity_onCreate")
			set_target_properties(
				"${library_target}"
				PROPERTIES
					OUTPUT_NAME "syphax_android_example_${example_name}"
					LIBRARY_OUTPUT_DIRECTORY "${BIN_DIR}/android/${ANDROID_ABI}/examples"
					RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}/android/${ANDROID_ABI}/examples")
			list(APPEND SE_ANDROID_ALL_EXAMPLE_TARGETS "${library_target}")
			set(SE_ANDROID_ALL_EXAMPLE_TARGETS "${SE_ANDROID_ALL_EXAMPLE_TARGETS}" PARENT_SCOPE)
		endfunction()

		set(EXAMPLES_ADVANCED_DIR "${EXAMPLES_DIR}/advanced")
		file(GLOB SE_ANDROID_EXAMPLES_BEGINNER CONFIGURE_DEPENDS
			"${EXAMPLES_DIR}/*.c"
			"${EXAMPLES_DIR}/*.cpp")
		file(GLOB SE_ANDROID_EXAMPLES_ADVANCED CONFIGURE_DEPENDS
			"${EXAMPLES_ADVANCED_DIR}/*.c"
			"${EXAMPLES_ADVANCED_DIR}/*.cpp")
		list(SORT SE_ANDROID_EXAMPLES_BEGINNER)
		list(SORT SE_ANDROID_EXAMPLES_ADVANCED)
		set(SE_ANDROID_ALL_EXAMPLE_SOURCES ${SE_ANDROID_EXAMPLES_BEGINNER} ${SE_ANDROID_EXAMPLES_ADVANCED})
		set(SE_ANDROID_ALL_EXAMPLE_TARGETS "")
		foreach(example_file ${SE_ANDROID_ALL_EXAMPLE_SOURCES})
			get_filename_component(example_realpath "${example_file}" REALPATH)
			if(example_realpath STREQUAL SE_ANDROID_ENTRY_SOURCE_REALPATH)
				continue()
			endif()
			se_add_android_example_library("${example_file}")
		endforeach()

		if(SE_ANDROID_ALL_EXAMPLE_TARGETS)
			add_custom_target(se_android_all_examples DEPENDS ${SE_ANDROID_ALL_EXAMPLE_TARGETS})
			message(STATUS "Configured Android all-example compile target: se_android_all_examples")
		else()
			message(STATUS "No additional Android examples found for compile-only coverage.")
		endif()
	endif()

	message(STATUS "Configured Android entry source: ${SE_ANDROID_ENTRY_SOURCE}")
	message(STATUS "Configured Android library target: ${SE_ANDROID_LIB_NAME}")
	message(STATUS "Configured Android manifest: ${SE_ANDROID_GENERATED_DIR}/AndroidManifest.xml")
endif()

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
