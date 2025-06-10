@echo off

IF NOT EXIST .\build mkdir .\build
pushd .\build
rem del win32_handmade.exe
cl -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 -DUNICODE -D_UNICODE -Zi -FC -nologo ..\src\win32_handmade.cpp /link User32.lib Gdi32.lib
popd
