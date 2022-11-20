@echo off

set files=../src/betsy.c

if NOT exist build mkdir build 
pushd build

echo %files%

cl /nologo /std:c11 /Zi /W4 %files% ^
    /link

move betsy.exe ../betsy.exe > NUL

popd