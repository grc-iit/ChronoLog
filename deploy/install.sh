#!/bin/bash

# Exit on any error
set -e

# Color codes for messages
ERR='\033[7;37m\033[41m'
DEBUG='\033[0;33m'
NC='\033[0m'

# Function to check for Spack and activate the environment
check_spack() {
    echo -e "${DEBUG}Checking if Spack is installed and accessible...${NC}"
    if ! command -v spack &> /dev/null; then
        echo -e "${ERR}Spack is not installed or not in the PATH.${NC}"
        echo "Please install Spack and make it accessible in your PATH. Refer to the prerequisites section of the Wiki at https://github.com/grc-iit/ChronoLog/wiki"
        exit 1
    fi

    echo -e "${DEBUG}Activating the Spack environment...${NC}"
    spack env activate -p .

    echo -e "${DEBUG}Checking if ChronoLog is already installed in Spack...${NC}"
    if ! spack find --loaded | grep -q "ChronoLog"; then
        echo -e "${DEBUG}ChronoLog not found in Spack. Installing...${NC}"
        spack install -v
    else
        echo -e "${DEBUG}ChronoLog is already installed.${NC}"
    fi
}

# Function to check and navigate to the build directory
navigate_to_build_directory() {
    echo -e "${DEBUG}Checking for the presence of the 'build' directory...${NC}"
    if [ -d "build" ]; then
        echo -e "${DEBUG}'build' directory found. Navigating to it...${NC}"
        cd build
    else
        echo -e "${ERR}Build directory not found. Please run the build script first.${NC}"
        exit 1
    fi
}

# Main function
main() {
    echo -e "${DEBUG}Script started. Setting working directory to the script's parent directory...${NC}"
    cd "$(dirname "$0")/.."

    echo -e "${DEBUG}Running Spack environment checks and installations...${NC}"
    check_spack

    echo -e "${DEBUG}Navigating to the build directory...${NC}"
    navigate_to_build_directory

    echo -e "${DEBUG}Running 'make install' to install ChronoLog...${NC}"
    make install

    echo -e "${DEBUG}Installation complete.${NC}"
}

# Run the main function with all script arguments
main "$@"
