@echo off

if NOT exist build mkdir build 
pushd build

cl /nologo /std:c11 /W4 ../src/betsy.c ^
    /link

move betsy.exe ../betsy.exe > NUL

popd