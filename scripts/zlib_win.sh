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

echo ">> Downloading ZLib 1.2.11 into $SDK_DIR"
curl -L "https://drive.google.com/uc?export=download&id=1UQgPZHIDUqmywBBycLI0AgRLhGK5-fja" --output $SDK_DIR/zlib.zip

if [ $? -ne 0 ]; then
	echo ">> ERROR: cURL failed to download ZLib archive"
else

	echo ">> Extracting ZLib sdk in $SDK_DIR/zlib"
	# unzip doesn't handle relative paths well
	unzip -q -u $SDK_DIR/zlib.zip -d $SDK_DIR

	rm $SDK_DIR/zlib.zip
fi
