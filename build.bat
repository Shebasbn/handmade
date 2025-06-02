@echo off

mkdir .\build
pushd .\build
del win32_handmade.exe
cl -Zi ..\src\win32_handmade.cpp /link User32.lib Gdi32.lib
popd
