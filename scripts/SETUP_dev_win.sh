#!/bin/bash

# Usage: SETUP_dev.sh <vc|cb>"

compiler="$1"

if [ ! -f "sdl_win.sh" ]; then
	echo "Invalid directory, you should run this from within scripts/"
	exit 2
fi

if [ ! "x$compiler" == "xvc" -a ! "x$compiler" == "xcb" ]; then
	echo "Choose your compiler/IDE:"
	echo ""
	echo "   1) Microsoft Visual Studio 2017"
	echo "   2) Code::Blocks 17.12"
	echo ""
	echo -n "? "
	read choice
	if [ "x$choice" == "x1" ]; then
		compiler="vc"
	elif [ "x$choice" == "x2" ]; then
		compiler="cb"
	else
		echo "Invalid choice"
		exit 1
	fi
fi
echo "Compiler is $compiler"

PROJ="../projfiles"

if [ $compiler == "vc" ]; then
	./sdl_win.sh
	./openexr_win.sh
	./zlib_win.sh
	
	echo ">> Copying project files..."
	cp $PROJ/fray.vcxproj* ..	
	
	echo ">> All set up! Open fray.vcxproj (in the root project dir) and happy hacking!"
fi

if [ $compiler == "cb" ]; then
	./sdl_win_cb.sh
	./openexr_win_cb.sh
	
	echo ">> Copying project files..."
	cp $PROJ/CodeBlocks-win.cbp ../fray.cbp

	echo ">> All set up! Open fray.cbp (in the root project dir) and happy hacking!"
fi
