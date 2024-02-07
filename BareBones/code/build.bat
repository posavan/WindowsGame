@echo off
::subst w: "C:\Users\posav\source\repos"
IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build
cl -FC -Zi w:/WindowsGame/BareBones/code/win32_barebones.cpp user32.lib gdi32.lib 
popd