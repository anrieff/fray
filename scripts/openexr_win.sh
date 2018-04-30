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

echo ">> Downloading OpenEXR 2.2.1 into $SDK_DIR"
curl -L "https://drive.google.com/uc?export=download&id=1rJDmlyAEjAx94JQ7BXENrRLNDwfahbHf" --output $SDK_DIR/openexr.zip

if [ $? -ne 0 ]; then
	echo ">> ERROR: cURL failed to download OpenEXR archive"
else

	echo ">> Extracting OpenEXR sdk in $SDK_DIR/OpenEXR"
	# unzip doesn't handle relative paths well
	unzip -q -u $SDK_DIR/openexr.zip -d $SDK_DIR

	rm $SDK_DIR/openexr.zip
fi
