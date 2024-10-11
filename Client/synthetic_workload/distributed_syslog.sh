#!/bin/bash

NUM_NODES=4
NUM_PROCS=96
HOST_FILE=client_hosts
BUILD_TYPE=Release
CHRONOLOG_INSTALL_DIR=/home/${USER}/chronolog/${BUILD_TYPE}
CHRONOLOG_BIN_DIR=${CHRONOLOG_INSTALL_DIR}/bin
CHRONOLOG_LIB_DIR=${CHRONOLOG_INSTALL_DIR}/lib
CLIENT_ADMIN_BIN=${CHRONOLOG_BIN_DIR}/client_admin
MPIEXEC_BIN=/home/${USER}/spack/opt/spack/linux-ubuntu22.04-zen3/gcc-11.4.0/mpich-4.0.2-nb7jqr5onqb7g7ynodbtsnqrax5sofyo/bin/mpiexec
CONF_FILE=/home/${USER}/chronolog/${BUILD_TYPE}/conf/default_conf.json
INPUT_FILE=/var/log/syslog

head -${NUM_NODES} ${HOST_FILE} > "${HOST_FILE}.${NUM_NODES}"
LD_LIBRARY_PATH=${CHRONOLOG_LIB_DIR} ${MPIEXEC_BIN} -n ${NUM_PROCS} -f "${HOST_FILE}.${NUM_NODES}" "${CLIENT_ADMIN_BIN}" -c "${CONF_FILE}" -f "${INPUT_FILE}"