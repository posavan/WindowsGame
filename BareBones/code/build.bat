@echo off
::subst w: "C:\Users\posav\source\repos"
set CompilerFlags=-nologo -Gm- -GR- -EHa- -Oi -DBAREBONES_INTERNAL=0 -FC -Z7
set LinkerFlags= -opt:ref user32.lib gdi32.lib winmm.lib

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build
cl %CompilerFlags% ..\BareBones\code\win32_barebones.cpp /link %LinkerFlags%
cl %CompilerFlags% ..\BareBones\code\barebones.cpp /link /DLL
popd