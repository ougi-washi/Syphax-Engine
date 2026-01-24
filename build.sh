#!/bin/bash

# Syphax-Engine - Ougi Washi

BUILD_DIR="build"
CLEAN=false
TARGET=""

show_help() {
    echo "Usage: ./build.sh [--clean] [target]"
    echo ""
    echo "Options:"
    echo "  --clean          Regenerate CMake files before building"
    echo "  target           Specific target to build (e.g., example_a)"
    echo ""
    echo "Examples:"
    echo "  ./build.sh                    Build everything"
    echo "  ./build.sh example_a          Build only example_a"
    echo "  ./build.sh --clean            Regenerate CMake files and build everything"
    echo "  ./build.sh --clean example_a  Regenerate CMake files and build only example_a"
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

# Run cmake only if it's a clean build or if CMakeCache doesn't exist
if [ "$CLEAN" = true ] || [ ! -f "CMakeCache.txt" ]; then
    echo "Generating CMake files..."
    cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
fi

if [ -z "$TARGET" ]; then
    echo "Building all targets..."
    make -j $(nproc)
else
    echo "Building target: $TARGET"
    make -j $(nproc) "$TARGET"
fi

cd ..
