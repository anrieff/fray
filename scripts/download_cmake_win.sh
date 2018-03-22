#!/bin/sh

if [ -z $BASH_SOURCE ]; then
	echo "ERROR \$BASH_SOURCE is required"
	exit 1
fi

BASH_DIR=$(dirname $BASH_SOURCE)
SDK_DIR=$BASH_DIR/../SDK

if [ ! -d $SDK_DIR ]; then
	mkdir -p $SDK_DIR
fi

echo ">> Downloading cmake 3.10 into $SDK_DIR"
curl https://cmake.org/files/v3.10/cmake-3.10.3-win64-x64.zip --output $SDK_DIR/cmake.zip

if [ $? -ne 0 ]; then
	echo ">> ERROR: cURL failed to download CMake archive"
else

	echo ">> Extracting CMake in $SDK_DIR/cmake"
	# unzip doesn't handle relative paths well
	unzip -q -u $SDK_DIR/cmake.zip -d $SDK_DIR

	mv -T $SDK_DIR/cmake-3.10.3-win64-x64 $SDK_DIR/cmake

	rm $SDK_DIR/cmake.zip
fi
