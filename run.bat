@echo off

set args=
pushd .\data\
call ..\build\win32_handmade.exe %args%
popd
