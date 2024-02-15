@echo off
::subst w: "C:\Users\posav\source\repos"
IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build
cl -nologo -Gm- -GR- -EHa- -Oi -DBAREBONES_INTERNAL=0 -FC -Z7 w:/WindowsGame/BareBones/code/win32_barebones.cpp user32.lib gdi32.lib winmm.lib
popd