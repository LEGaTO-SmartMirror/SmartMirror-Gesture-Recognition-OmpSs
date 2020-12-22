#!/bin/bash

export NANOS6=optimized
export NANOS6_SCHEDULER=locality
export NANOS6_CLUSTER_SCHEDULING_POLICY=locality
export NANOS6_COMMUNICATION=mpi-2sided
export NANOS6_LOCAL_MEMORY=8GB
#export NANOS6_DISTRIBUTED_MEMORY=1GB
export NANOS6_LOADER_VERBOSE=1

cd /opt/dev/MagicMirror/modules/SmartMirror-Gesture-Recognition-OmpSs/object_detection_ompss/bin

#source	env.sh

mpirun.mpich -np 2 -f hostfile ./YoloTRT.bin config.ini
