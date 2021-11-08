@ECHO OFF
SET COMPILER_DIR=Compiler
SET PATH=%COMPILER_DIR%\WINDOWS\Other Utilities;%COMPILER_DIR%\WINDOWS\bin;%PATH%
rm makefile
copy makefile_windows makefile
make clean
mkdir out
make
type ip.bin menu_code.bin >> out/menu.bin
echo "created menu.bin"
move menu_code.bin out/menu_code.bin
move menu.elf out/menu.elf
rm makefile
copy makefile_linux makefile
pause