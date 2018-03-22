#!/bin/sh

if [ -z $BASH_SOURCE ]; then
	echo "ERROR \$BASH_SOURCE is required"
	exit 1
fi

FRAY_DIR=$(cd $(dirname $BASH_SOURCE)/.. && pwd)

echo $FRAY_DIR

if [ ! -d $FRAY_DIR/develop ]; then
	mkdir $FRAY_DIR/develop
fi

# change workdir, because cmake generates project files in its current directory
pushd $FRAY_DIR/develop

# call cmake (the generator can be changed - probably with user interaction?)
$FRAY_DIR/SDK/cmake/bin/cmake.exe -G "Visual Studio 15 2017 Win64" $FRAY_DIR/cmake

popd
