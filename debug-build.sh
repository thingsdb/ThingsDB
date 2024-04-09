#!/bin/bash
cd "$(dirname "$0")"
cmake -DCMAKE_BUILD_TYPE=Debug .
make
