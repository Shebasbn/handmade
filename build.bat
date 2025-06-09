@echo off

IF NOT EXIST .\build mkdir .\build
pushd .\build
del win32_handmade.exe
cl -DUNICODE -D_UNICODE -Zi -FC -nologo ..\src\win32_handmade.cpp /link User32.lib Gdi32.lib
popd
