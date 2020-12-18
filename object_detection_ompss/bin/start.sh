#!/bin/bash


cd /opt/dev/MagicMirror/modules/SmartMirror-Gesture-Recognition-OmpSs/object_detection_ompss/bin

source	env.sh

mpirun.mpich -np 2 -f hostfile ./YoloTRT.bin
