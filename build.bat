@echo off

set CommonCompilerFlags= -MTd -Gm- -GR- -EHa- -Od -Oi -fp:fast -FC -Z7 -nologo -W4 -WX -wd4201 -wd4100 -wd4189 -wd4505 -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 -DUNICODE -D_UNICODE
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

IF NOT EXIST .\build mkdir .\build
pushd .\build
rem del win32_handmade.exe
rem 32-bit build
rem cl %CommonCompilerFlags% ..\src\win32_handmade.cpp -subsystem:windows,5.1 /link %link_opts% 

REM 64-bit build
REM Optimization switches /O2 /Oi /fp:fast
cl %CommonCompilerFlags% -Fmhandmade.map ..\src\handmade.cpp -LD /link -incremental:no -opt:ref -EXPORT:GameGetSoundSamples -EXPORT:GameUpdateAndRender
cl %CommonCompilerFlags% -Fmwin32_handmade.map ..\src\win32_handmade.cpp /link %CommonLinkerFlags% 
popd
