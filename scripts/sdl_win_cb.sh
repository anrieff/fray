#!/bin/sh

if [ -z $BASH_SOURCE ]; then
	echo "ERROR \$BASH_SOURCE is required"
	exit 1
fi

BASH_DIR=$(dirname $BASH_SOURCE)
SDK_DIR=$BASH_DIR/../SDK

if [ ! -d $SDK_DIR ]; then
	mkdir $SDK_DIR
fi

echo ">> Downloading SDL 1.2 into $SDK_DIR"
curl http://libsdl.org/release/SDL-devel-1.2.15-mingw32.tar.gz --output $SDK_DIR/sdl.tar.gz

if [ $? -ne 0 ]; then
	echo ">> ERROR: cURL failed to download SDL archive"
else

	echo ">> Extracting SDL SDK in $SDK_DIR/SDL"
	# unzip doesn't handle relative paths well
	pushd .
	cd $SDK_DIR
	tar xzf sdl.tar.gz
	popd

	rm $SDK_DIR/sdl.tar.gz

	echo ">> Copying SDL.dll"
	cp $SDK_DIR/SDL-1.2.15/bin/SDL.dll ..
fi
