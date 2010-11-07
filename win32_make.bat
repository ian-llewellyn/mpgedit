@echo off
cd mpglib
echo making in mpglib
nmake /nologo -f makefile.win32 %1

cd ..
cd md5lib
echo making in md5lib
nmake /nologo -f makefile.win32 %1

cd ..
cd mad\win32
echo making in mad
nmake /nologo -f ..\src\makefile.win32

cd ..\..
echo making in mpgedit
nmake /nologo -f makefile.win32 %1

cd gui
echo making in mpgedit\gui
nmake /nologo -f makefile.win32 %1
rem nmake /nologo -f makefile.win32 installer

cd ..
rem copy ..\pdcurses\curses.dll

cd contrib\python\py_mpgedit
nmake /nologo -f makefile.win32 %1
cd ..\..\..

nmake /nologo -f makefile.win32 mpgedit_sdk %1

echo make done
