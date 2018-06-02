# fray
"The Fifth (Render) Element" (educational raytracer)

This is the code for the v5 FMI raytracing course.
The course site is [http://raytracing-bg.net/](http://raytracing-bg.net/)

How to set it up on your machine
--------------------------------

On Windows:
-----------
   Open Git Bash in "scripts/" and type `./SETUP_dev_win.sh`, then follow the instructions.
   This will install SDL and OpenEXR in a SDK/ subdirectory of the project, copy SDL.dll to this directory, and copy the project files from the versioned templates to a local, untracked copy.

On Linux and Mac OS X:
----------------------
   Use your package manager (apt, yum/dnf, or brew) to install SDL-1.2 (package names `libsdl-dev` or `SDL-devel` or similar) and OpenEXR (`libopenexr-dev`, `OpenEXR-devel`, etc).
   If you're using Code::Blocks, copy the .cbp file from projfiles/ to this directory and rename it to `fray.cbp`
