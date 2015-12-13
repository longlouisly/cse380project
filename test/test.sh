#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo Entering $DIR
cd $DIR

../chanvese circleInput.txt && ../compare circle.png circleOut.png


