@echo off

set IncludeDirs=-I"..\vendor\glm" -I"..\vendor\GLFW\include" -I"..\vendor\glad\include" -I"..\vendor\stb_image" -I"..\vendor\freetype\include" -I"..\vendor\msdf-atlas-gen\include" -I"..\vendor\msdfgen\include" -I"..\vendor\msdfgen\include\msdfgen" -I"..\vendor\miniaudio"

set CommonCompilerFlags=-MT -nologo -Gm- -GR- -EHa- -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -DBUOYANT_DEV=1 -FC -Z7 -Fmwin32_buoyant.map

set LibPaths=-LIBPATH:"..\vendor\GLFW\lib-vc2022" -LIBPATH:"..\vendor\freetype" -LIBPATH:"..\vendor\msdf-atlas-gen" -LIBPATH:"..\vendor\msdfgen"

set CommonLinkerFlags=-opt:ref glfw3_mt.lib freetype.lib msdf-atlas-gen.lib msdfgen-core.lib msdfgen-ext.lib user32.lib Gdi32.lib Shell32.lib

if not exist "build" mkdir "build"

pushd build

echo 64-bit (x64) build
cl %IncludeDirs% %CommonCompilerFlags% "..\src\win32_buoyant.cpp" "..\vendor\glad\src\gl.c" /link %LibPaths% %CommonLinkerFlags%

popd
