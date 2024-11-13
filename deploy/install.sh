#!/bin/bash

# Exit on any error
set -e

# Color codes for messages
GREEN="\033[42;30m"
RED="\033[41;30m"
RESET="\033[0m"
BUILD_DIR="build"

# Parse arguments to get BUILD_TYPE
while [[ "$#" -gt 0 ]]; do
    case $1 in
        -type) BUILD_TYPE="$2"; shift ;;
        *) echo -e "${RED}Unknown parameter passed: $1${RESET}"; exit 1 ;;
    esac
    shift
done

# Check if BUILD_TYPE is set; if not, prompt the user to provide it
if [ -z "$BUILD_TYPE" ]; then
    echo -e "${RED}Error: BUILD_TYPE is not set. Use -type to specify Debug or Release.${RESET}"
    exit 1
fi

# Define installation directory using BUILD_TYPE
INSTALL_DIR="/home/$USER/chronolog/$BUILD_TYPE"

# Move to the root directory of the project
cd "$(dirname "$0")/.."

# Check if Spack is installed
if ! command -v spack &> /dev/null; then
    echo -e "${RED}Spack is not installed or not in the PATH.${RESET}"
    echo "Please install Spack and make it accessible in your PATH. Refer to the prerequisites section of the Wiki at https://github.com/grc-iit/ChronoLog/wiki"
    exit 1
fi

# Activate Spack environment and install dependencies if not already installed
spack env activate -p .
if ! spack find --loaded | grep -q "ChronoLog"; then
    spack install -v
fi

# Navigate to the build directory
if [ -d "$BUILD_DIR" ]; then
  cd "$BUILD_DIR"
else
  echo -e "${RED}Build directory not found. Please run the build script first.${RESET}"
  exit 1
fi

# Print a message indicating that the installation process is starting
echo -e "${GREEN}Installing ChronoLog...${RESET}"

# Run make install to install the built binaries
make install

# Verify the installation by checking the existence of key files
if [ ! -f "${INSTALL_DIR}/bin/chrono_grapher" ] || [ ! -f "${INSTALL_DIR}/conf/default_conf.json" ]; then
    echo -e "${RED}Installation verification failed: key files not found in ${INSTALL_DIR}.${RESET}"
    exit 1
fi

# Print a message indicating that the installation process has completed
echo -e "${GREEN}ChronoLog Installed Successfully at ${INSTALL_DIR}${RESET}"
