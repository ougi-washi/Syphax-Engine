#!/bin/bash

# Syphax-Engine - Ougi Washi

BUILD_DIR="build"
CLEAN=false
TARGET=""
RENDER="opengl"

show_help() {
    echo "Usage: ./build.sh [--clean] [-target=name] [-render=opengl|gles|vulkan] [target]"
    echo ""
    echo "Options:"
    echo "  --clean          Regenerate CMake files before building"
    echo "  -target=name     Specific target to build (e.g., hello)"
    echo "  -render=backend  Render backend: opengl (default), gles, vulkan"
    echo "  target           Specific target to build (e.g., example_a)"
    echo ""
    echo "Examples:"
    echo "  ./build.sh                    Build everything"
    echo "  ./build.sh example_a          Build only example_a"
    echo "  ./build.sh --clean            Regenerate CMake files and build everything"
    echo "  ./build.sh --clean example_a  Regenerate CMake files and build only example_a"
    echo "  ./build.sh -target=hello -render=gles"
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

# Resolve render backend flags
RENDER_FLAG=""
case "$RENDER" in
    opengl)
        RENDER_FLAG="-DSE_RENDER_BACKEND_OPENGL"
        ;;
    gles)
        RENDER_FLAG="-DSE_RENDER_BACKEND_GLES"
        ;;
    vulkan)
        RENDER_FLAG="-DSE_RENDER_BACKEND_VULKAN"
        ;;
    *)
        echo "Error: Unknown render backend: $RENDER"
        echo ""
        show_help
        ;;
esac

WINDOW_FLAG="-DSE_WINDOW_BACKEND_GLFW"
CMAKE_C_FLAGS_VALUE="$RENDER_FLAG $WINDOW_FLAG"

# Run cmake only if it's a clean build or if CMakeCache doesn't exist
RECONFIGURE=false
if [ "$CLEAN" = true ] || [ ! -f "CMakeCache.txt" ]; then
    RECONFIGURE=true
elif ! grep -q "CMAKE_C_FLAGS:STRING=$CMAKE_C_FLAGS_VALUE" CMakeCache.txt; then
    RECONFIGURE=true
fi

if [ "$RECONFIGURE" = true ]; then
    echo "Generating CMake files..."
    cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_C_FLAGS="$CMAKE_C_FLAGS_VALUE" ..
fi

if [ -z "$TARGET" ]; then
    echo "Building all targets..."
    make -j $(nproc)
else
    echo "Building target: $TARGET"
    make -j $(nproc) "$TARGET"
fi

cd ..
