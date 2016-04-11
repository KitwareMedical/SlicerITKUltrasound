#!/usr/bin/env bash

# This script builds the SlicerITKUltrasound plugin against Slicer.
# The 'run.sh' script uses this script when running inside the
# slicer/slicer-build Docker
# image.

set -x

mkdir -p /usr/src/SlicerITKUltrasound-build

set -e

cd /usr/src/SlicerITKUltrasound-build
cmake \
  -DSlicer_DIR:PATH=/usr/src/Slicer-build/Slicer-build \
  -DBUILDNAME:STRING=Extension-SlicerITKUltrasound \
    /usr/src/SlicerITKUltrasound/
ctest -VV -D Experimental
