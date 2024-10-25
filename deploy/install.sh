#!/bin/bash

# Exit on any error
set -e

# Move to the root directory of the project
cd "$(dirname "$0")/.."

# Load Spack environment
source ~/Spack/spack/share/spack/setup-env.sh

# Activate Spack environment
spack env activate -p .
spack install -v

# Navigate to the build directory
if [ -d "build" ]; then
  cd build
else
  echo -e "\033[41;30mBuild directory not found. Please run the build script first.\033[0m"
  exit 1
fi

# Print a message with a green background indicating that the installation process is starting
echo -e "\033[42;30mInstalling ChronoLog...\033[0m"

# Run make install to install the built binaries
make install

# Print a message with a green background indicating that the installation process has completed
echo -e "\033[42;30mChronoLog Installed Successfully\033[0m"
