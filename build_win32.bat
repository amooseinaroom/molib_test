@echo off

set name="test"
set source=%cd%\source\main.c
set includes=/I %cd%/molib/source
set options=%source% /nologo /Zi /Od /DEBUG %includes%

if not exist build mkdir build

pushd build

rem test C compilation
cl /Fe%name% /TC %options%

rem test C++ compilation
cl /Fe%name% /TP %options%

popd

copy build\%name%.* %name%.* >NUL