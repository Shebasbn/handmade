@echo off

mkdir .\build
pushd .\build
del win32_handmade.exe
cl -Zi -FC -nologo ..\src\win32_handmade.cpp /link User32.lib Gdi32.lib
popd
