#!/bin/bash

# Exit on any error
set -e

# Define required variables with no defaults
BUILD_TYPE=""
NUM_KEEPERS=""
NUM_GRAPHERS=""

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
        *) echo "Unknown parameter passed: $1"; usage ;;
    esac
    shift
done

# Check if mandatory parameters are set, otherwise show usage
if [[ -z "$BUILD_TYPE" || -z "$NUM_KEEPERS" || -z "$NUM_GRAPHERS" ]]; then
    echo "Error: Missing mandatory parameter(s)"
    usage
fi

# Standardized installation directory
INSTALL_DIR="/home/$USER/chronolog/$BUILD_TYPE"
BIN_DIR="${INSTALL_DIR}/bin"
CONF_DIR="${INSTALL_DIR}/conf"

# Print a message with a green background indicating that the build process is starting
echo -e "\033[42;30mBuilding ChronoLog in ${BUILD_TYPE} mode...\033[0m"
"${SCRIPT_DIR}/build.sh" -type "$BUILD_TYPE"

# Print a message with a green background indicating that the install process is starting
echo -e "\033[42;30mInstalling ChronoLog...\033[0m"
"${SCRIPT_DIR}/install.sh"

# Print a message with a green background indicating that the deployment process is starting
echo -e "\033[42;30mDeploying ChronoLog...\033[0m"

# Run the deployment script with the specified parameters and standard directories
"${SCRIPT_DIR}/local_single_user_deploy.sh" \
    -n "$NUM_KEEPERS" \
    -j "$NUM_GRAPHERS" \
    -w "$INSTALL_DIR" \
    ${DEPLOY:+-d} ${RESET:+-r} ${STOP:+-s} ${KILL:+-k}

# Print a message with a green background indicating that the deployment process has completed
echo -e "\033[42;30mChronoLog Deployed Successfully\033[0m"
