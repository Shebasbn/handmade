@echo off

set opts=-DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 -DUNICODE -D_UNICODE
set cl_opts=-MT -Gm- -GR- -EHa- -Oi -FC -Fmwin32_handmade.map -Z7 -nologo -W4 -WX -wd4201 -wd4100 -wd4189 %opts%
set link_opts=-incremental:no -opt:ref 
IF NOT EXIST .\build mkdir .\build
pushd .\build
rem del win32_handmade.exe
cl %cl_opts% ..\src\win32_handmade.cpp /link %link_opts% User32.lib Gdi32.lib
popd
