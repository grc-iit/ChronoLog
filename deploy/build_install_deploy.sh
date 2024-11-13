#!/bin/bash

# Exit on any error
set -e

# Color codes
GREEN="\033[42;30m"
RED="\033[41;30m"
RESET="\033[0m"

# Define required variables with no defaults
BUILD_TYPE=""
NUM_KEEPERS=""
NUM_GRAPHERS=""

# Set default values for optional flags
DEPLOY=false
STOP=false
RESET=false
KILL=false

# Directory of the script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Usage function to show required and optional parameters
usage() {
    echo "Usage: $0 -type <BuildType> -n <NUM_KEEPERS> -j <NUM_GRAPHERS>"
    echo ""
    echo "Options:"
    echo "  -type           Specify the build type (Debug or Release)"
    echo "  -n|--keepers    Number of keeper processes (mandatory)"
    echo "  -j|--record-groups  Number of recording groups or grapher processes (mandatory)"
    echo ""
    echo "Additional Options for local_single_user_deploy.sh:"
    echo "  -d|--deploy     Start ChronoLog Deployment (default: false)"
    echo "  -s|--stop       Stop ChronoLog Deployment (default: false)"
    echo "  -r|--reset      Reset/CleanUp ChronoLog Deployment (default: false)"
    echo "  -k|--kill       Terminate ChronoLog Deployment (default: false)"
    exit 1
}

# Parse arguments
while [[ "$#" -gt 0 ]]; do
    case $1 in
        -type) BUILD_TYPE="$2"; shift ;;
        -n|--keepers) NUM_KEEPERS="$2"; shift ;;
        -j|--record-groups) NUM_GRAPHERS="$2"; shift ;;
        -d|--deploy) DEPLOY=true ;;
        -s|--stop) STOP=true ;;
        -r|--reset) RESET=true ;;
        -k|--kill) KILL=true ;;
        *) echo -e "${RED}Unknown parameter passed: $1${RESET}"; usage ;;
    esac
    shift
done

# Check if mandatory parameters are set and validate BUILD_TYPE
if [[ -z "$BUILD_TYPE" || -z "$NUM_KEEPERS" || -z "$NUM_GRAPHERS" ]]; then
    echo -e "${RED}Error: Missing mandatory parameter(s)${RESET}"
    usage
fi

if [[ ! "$BUILD_TYPE" =~ ^(Debug|Release)$ ]]; then
    echo -e "${RED}Error: Invalid build type. Use 'Debug' or 'Release'.${RESET}"
    usage
fi

# Standardized installation directory
INSTALL_DIR="/home/$USER/chronolog/$BUILD_TYPE"
BIN_DIR="${INSTALL_DIR}/bin"
CONF_DIR="${INSTALL_DIR}/conf"

# Check if the required scripts exist and are executable
for script in build.sh install.sh local_single_user_deploy.sh; do
    if [[ ! -x "${SCRIPT_DIR}/${script}" ]]; then
        echo -e "${RED}Error: ${script} not found or not executable in ${SCRIPT_DIR}${RESET}"
        exit 1
    fi
done

# Build ChronoLog
echo -e "${GREEN}Building ChronoLog in ${BUILD_TYPE} mode...${RESET}"
"${SCRIPT_DIR}/build.sh" -type "$BUILD_TYPE"

# Install ChronoLog
echo -e "${GREEN}Installing ChronoLog...${RESET}"
"${SCRIPT_DIR}/install.sh" -type "$BUILD_TYPE"

# Deploy ChronoLog
echo -e "${GREEN}Deploying ChronoLog...${RESET}"
"${SCRIPT_DIR}/local_single_user_deploy.sh" \
    -n "$NUM_KEEPERS" \
    -j "$NUM_GRAPHERS" \
    -w "$INSTALL_DIR" \
    ${DEPLOY:+-d} ${RESET:+-r} ${STOP:+-s} ${KILL:+-k}

# Completion message
echo -e "${GREEN}ChronoLog Deployed Successfully${RESET}"
