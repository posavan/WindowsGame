@echo off
::subst w: "C:\Users\posav\source\repos"
mkdir ..\..\build
pushd ..\..\build
cl -FAsc -Zi w:/WindowsGame/BareBones/code/win32_barebones.cpp user32.lib gdi32.lib 
popd