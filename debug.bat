@echo off

pushd .\data\
raddbg.exe --auto_step ..\build\win32_handmade.exe
popd