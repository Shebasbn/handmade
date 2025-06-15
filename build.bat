@echo off

set opts=-DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 -DUNICODE -D_UNICODE
set cl_opts=-MT -Gm- -GR- -EHa- -Od -Oi -FC -Z7 -nologo -W4 -WX -wd4201 -wd4100 -wd4189 %opts%
set link_opts=-incremental:no -opt:ref User32.lib Gdi32.lib Winmm.lib
IF NOT EXIST .\build mkdir .\build
pushd .\build
rem del win32_handmade.exe
rem 32-bit build
rem cl %cl_opts% ..\src\win32_handmade.cpp -subsystem:windows,5.1 /link %link_opts% 
cl %cl_opts% -Fmhandmade.map ..\src\handmade.cpp -LD /link -incremental:no -opt:ref -EXPORT:GameGetSoundSamples -EXPORT:GameUpdateAndRender
cl %cl_opts% -Fmwin32_handmade.map ..\src\win32_handmade.cpp /link %link_opts% 
popd
