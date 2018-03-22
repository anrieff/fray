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
curl https://www.libsdl.org/release/SDL-devel-1.2.15-VC.zip --output $SDK_DIR/sdl.zip

if [ $? -ne 0 ]; then
	echo ">> ERROR: cURL failed to download SDL archive"
else

	echo ">> Extracting SDL sdk in $SDK_DIR/SDL"
	# unzip doesn't handle relative paths well
	unzip -q -u $SDK_DIR/sdl.zip -d $SDK_DIR

	mv -T $SDK_DIR/SDL-1.2.15 $SDK_DIR/SDL

	rm $SDK_DIR/sdl.zip
fi
