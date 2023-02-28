SETLOCAL

CALL "%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86 8.1

SET QMAKESPEC=%~n0

SET ICU_DIST=%~dp0icu\dist-32
SET OPENSSL_INC=%~dp0openssl\openssl-1.1\x86\include
SET DXSDK_DIR=C:\msdxjune2010\

CD %~dp0..
SET QT_SOURCE=%CD%
CD %~dp0..\..
SET QT_BUILD=%CD%\%QMAKESPEC%-build

RD /s /q %QT_BUILD%
MKDIR %QT_BUILD%
CD /D %QT_BUILD%

SET PATH=%QT_BUILD%\qtbase\lib;%QT_SOURCE%\gnuwin32\bin;%ICU_DIST%\bin;C:\perl\bin;%PATH%
CALL "C:\Ruby31-x64\bin\setrbvars.cmd"

SET "INCLUDE=%INCLUDE%;%OPENSSL_INC%;%ICU_DIST%\include;"
SET "LIB=%LIB%;%ICU_DIST%\lib;"

CALL %QT_SOURCE%\configure.bat -rtti -no-harfbuzz -ssl -openssl -icu -opengl desktop -opensource -nomake tests -nomake examples -nomake tools -skip translations -confirm-license -platform win32-msvc2012 -release 2>&1 | perl.exe %QT_SOURCE%\build\tools\tee.pl -t "%QT_BUILD%\logfile.log"

nmake.exe 2>&1 | perl.exe %QT_SOURCE%\build\tools\tee.pl -t -a "%QT_BUILD%\logfile.log"
