#!/bin/bash

# Syphax-Engine - Ougi Washi

set -euo pipefail

BUILD_DIR="build"
CLEAN=false
TARGET=""
RENDER="gl"
PLATFORM="desktop_glfw"

show_help() {
    echo "Usage: ./build.sh [--clean] [-target=name] [-render=gl|gles] [-platform=desktop_glfw|terminal] [target]"
    echo ""
    echo "Options:"
    echo "  --clean          Regenerate CMake files before building"
    echo "  -target=name     Specific target to build (e.g., hello_text)"
    echo "  -render=backend  Render backend: gl (default), gles"
    echo "                   Planned (not implemented yet): webgl, metal, vulkan, software"
    echo "  -platform=kind   Platform backend: desktop_glfw (default), terminal (experimental)"
    echo "                   Planned (not implemented yet): android, ios, web"
    echo "  target           Specific target to build (e.g., hello_text)"
    echo ""
    echo "Examples:"
    echo "  ./build.sh                    Build everything"
    echo "  ./build.sh hello_text         Build beginner example hello_text"
    echo "  ./build.sh rts_integration    Build advanced integration example"
    echo "  ./build.sh --clean            Regenerate CMake files and build everything"
    echo "  ./build.sh --clean hello_text Regenerate CMake files and build only hello_text"
    echo "  ./build.sh -target=model_viewer -render=gles"
    echo "  ./build.sh -platform=terminal rts_integration"
    exit 1
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --clean)
            CLEAN=true
            shift
            ;;
        --help|-h)
            show_help
            ;;
        -target=*)
            if [ -n "$TARGET" ]; then
                echo "Error: Multiple targets specified"
                echo ""
                show_help
            fi
            TARGET="${1#-target=}"
            shift
            ;;
        -render=*)
            RENDER="${1#-render=}"
            shift
            ;;
        -platform=*)
            PLATFORM="${1#-platform=}"
            shift
            ;;
        -*)
            echo "Error: Unknown option: $1"
            echo ""
            show_help
            ;;
        *)
            if [ -n "$TARGET" ]; then
                echo "Error: Multiple targets specified"
                echo ""
                show_help
            fi
            TARGET="$1"
            shift
            ;;
    esac
done

if [ "$CLEAN" = true ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

case "$RENDER" in
    opengl|gl)
        RENDER="gl"
		;;
    gles)
        RENDER="gles"
        ;;
    webgl|metal|vulkan|software)
        ;;
    *)
        echo "Error: Unknown render backend: $RENDER"
        echo ""
        show_help
        ;;
esac

case "$PLATFORM" in
    desktop|desktop_glfw|glfw)
        PLATFORM="desktop_glfw"
        ;;
    terminal|android|ios|web)
        ;;
    *)
        echo "Error: Unknown platform backend: $PLATFORM"
        echo ""
        show_help
        ;;
esac

# Run cmake only if it's a clean build or if CMakeCache doesn't exist
RECONFIGURE=false
if [ "$CLEAN" = true ] || [ ! -f "CMakeCache.txt" ]; then
    RECONFIGURE=true
elif ! grep -q "^SE_BACKEND_RENDER:STRING=$RENDER$" CMakeCache.txt; then
    RECONFIGURE=true
elif ! grep -q "^SE_BACKEND_PLATFORM:STRING=$PLATFORM$" CMakeCache.txt; then
    RECONFIGURE=true
fi

if [ "$RECONFIGURE" = true ]; then
    echo "Generating CMake files (render=$RENDER, platform=$PLATFORM)..."
    cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DSE_BACKEND_RENDER="$RENDER" -DSE_BACKEND_PLATFORM="$PLATFORM" ..
fi

if [ -z "$TARGET" ]; then
    echo "Building all targets..."
    cmake --build . --parallel "$(nproc)"
else
    echo "Building target: $TARGET"
    cmake --build . --parallel "$(nproc)" --target "$TARGET"
fi

cd ..
