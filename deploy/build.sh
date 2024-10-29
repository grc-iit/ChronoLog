#!/bin/bash

# Exit on any error
set -e

# Initialize build type variable as empty to ensure it's set by the user
BUILD_TYPE=""

# Parse arguments
while [[ "$#" -gt 0 ]]; do
    case $1 in
        -type) BUILD_TYPE="$2"; shift ;;
        *) echo "Unknown parameter passed: $1"; exit 1 ;;
    esac
    shift
done

# Check if BUILD_TYPE is set, if not, print usage and exit
if [[ -z "$BUILD_TYPE" ]]; then
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
else
    echo "Spack installed and detected"
fi

# Activate Spack environment
spack env activate -p .
spack install -v

# Check if build directory exists and delete it if it does
if [ -d "build" ]; then
  echo "Build directory exists. Removing it..."
  rm -rf build
fi

# Create a fresh build directory
mkdir build
cd build

# Print a message with a green background indicating that the build process is starting
echo -e "\033[42;30mBuilding ChronoLog in ${BUILD_TYPE} mode...\033[0m"

# Run CMake configuration with specified build type
cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} ..

# Build all targets
make all

# Print a message with a green background indicating that the build process has completed
echo -e "\033[42;30mChronoLog Built in ${BUILD_TYPE} mode\033[0m"
