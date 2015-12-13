#! /bin/bash
# Chan-Vese segmentation example BASH shell script
# Echo shell commands
set -v

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo Entering $DIR
cd $DIR

../chanvese exampleInput.txt
