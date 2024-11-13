#!/bin/bash

# Exit on any error
set -e

# Default values and color codes
BUILD_TYPE=""
BUILD_DIR="build"
GREEN="\033[42;30m"
RESET="\033[0m"

# Parse arguments
while [[ "$#" -gt 0 ]]; do
    case $1 in
        -type) BUILD_TYPE="$2"; shift ;;
        *) echo "Unknown parameter passed: $1"; exit 1 ;;
    esac
    shift
done

# Validate BUILD_TYPE
if [[ -z "$BUILD_TYPE" || ! "$BUILD_TYPE" =~ ^(Debug|Release)$ ]]; then
    echo "Usage: $0 -type <BuildType>"
    echo "Example: $0 -type Debug or $0 -type Release"
    exit 1
fi

# Move to the root directory of the project
cd "$(dirname "$0")/.."

# Check if Spack is installed
if ! command -v spack &> /dev/null; then
    echo "Spack is not installed or not in the PATH."
    echo "Please install Spack and make it accessible in your PATH. Refer to the prerequisites section of the Wiki at https://github.com/grc-iit/ChronoLog/wiki"
    exit 1
fi

# Activate Spack environment and install dependencies if not already installed
spack env activate -p .
if ! spack find --loaded | grep -q "ChronoLog"; then
    spack install -v
fi

# Remove and recreate the build directory
if [ -d "$BUILD_DIR" ]; then
  echo "Build directory exists. Removing it..."
  rm -rf "$BUILD_DIR"
fi

mkdir "$BUILD_DIR"
cd "$BUILD_DIR"

# Print a message indicating that the build process is starting
echo -e "${GREEN}Building ChronoLog in ${BUILD_TYPE} mode...${RESET}"

# Run CMake configuration and build all targets
cmake -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" ..
make all

# Print a message indicating that the build process has completed
echo -e "${GREEN}ChronoLog Built in ${BUILD_TYPE} mode${RESET}"
