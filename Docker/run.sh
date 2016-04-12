#!/usr/bin/env bash

script_dir="`cd $(dirname $0); pwd`"

docker run \
  --rm \
  -v $script_dir/..:/usr/src/SlicerITKUltrasound \
  slicer/slicer-build \
    /usr/src/SlicerITKUltrasound/Docker/test.sh
