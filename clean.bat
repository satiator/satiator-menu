@ECHO OFF
SET COMPILER_DIR=Compiler
SET PATH=%COMPILER_DIR%\WINDOWS\Other Utilities;%COMPILER_DIR%\WINDOWS\bin;%PATH%
rm makefile
copy makefile_windows makefile
make clean
rm makefile
copy makefile_linux makefile
pause