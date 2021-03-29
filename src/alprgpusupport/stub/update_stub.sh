#!/bin/bash

SCRIPTPATH="$( cd "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
cat ${SCRIPTPATH}/../cuda/alprgpusupport_cudaimpl.h | sed 's/);/) { }/g' > ${SCRIPTPATH}/stub.cpp
