@echo off

set name="test"
set source=%cd%\source\main.c
set includes=-I %cd%/molib/source
set options=%source% /nologo /Zi /Od /DEBUG %includes% 

if not exist build mkdir build

pushd build

rem test C compilation
call emcc %source% -v -o %name%.html -g -O0 -gsource-map -sEXIT_RUNTIME %includes% 

rem test C++ compilation
rem cl /Fe%name% /TP %options%

popd

rem copy build\%name%.* %name%.* >NUL