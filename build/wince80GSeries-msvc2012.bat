SETLOCAL

CALL "%ProgramFiles(x86)%\Microsoft Visual Studio 11.0\VC\vcvarsall.bat"

SET QMAKESPEC=%~n0

SET QT_HOST_PATH=C:\Qt\Qt5.5.1\msvc2013_32

SET ICU_DIST=%~dp0icu\dist-32
SET OPENSSL_INC=%~dp0openssl\openssl-1.1\x86\include

CD %~dp0..
SET QT_SOURCE=%CD%
CD %~dp0..\..
SET QT_BUILD=%CD%\%QMAKESPEC%-build

RD /s /q %QT_BUILD%
MKDIR %QT_BUILD%
CD /D %QT_BUILD%

SET PATH=%QT_SOURCE%\gnuwin32\bin;%ICU_DIST%\bin;C:\perl\bin;%PATH%
CALL "C:\Ruby31-x64\bin\setrbvars.cmd"

FOR /F "tokens=1,2,* delims== " %%A IN (%QT_SOURCE%\qtBase\mkspecs\%QMAKESPEC%\qmake.conf) DO IF "%%A" geq "CE_ARCH" IF "%%A" leq "CE_SDK" SET "%%A=%%B" 
SET CE_SDKDIR=%ProgramFiles(x86)%\Windows CE Tools\SDKs\%CE_SDK%\Sdk
SET CE_SDKDIR_TORADEX=%ProgramFiles(x86)%\Windows CE Tools\SDKs\Toradex_CE800\Sdk
SET CE_
xcopy /S /R /Y "%CE_SDKDIR_TORADEX%\Inc\egl\" "egl\Inc\egl\"
xcopy /S /R /Y "%CE_SDKDIR_TORADEX%\Inc\GLES\" "egl\Inc\GLES\"
xcopy /S /R /Y "%CE_SDKDIR_TORADEX%\Inc\GLES2\" "egl\Inc\GLES2\"
xcopy /T "%CE_SDKDIR%\Lib\" "egl\Lib\"
FOR %%X IN (libegl libglesv2) DO lib /def:%~dp0egl\%%X.def /machine:%CE_ARCH% /out:egl\Lib\%CE_ARCH%\retail\%%X.lib
SET OPENGL_ES2=%QT_BUILD%\egl

SETLOCAL
	SET "INCLUDE=%INCLUDE%;%OPENSSL_INC%;"
	CALL %QT_SOURCE%\configure.bat -opensource -confirm-license -rtti -no-harfbuzz -ssl -openssl -icu -opengl es2 -nomake tests -nomake examples -nomake tools -skip webengine -skip translations -platform win32-msvc2012 -xplatform %QMAKESPEC% -release 2>&1 | perl.exe %QT_SOURCE%\build\tools\tee.pl -t "%QT_BUILD%\logfile.log"
	REM Change QT_QPA_DEFAULT_PLATFORM_NAME from "windows" to "windows:fontengine=freetype"
	gsar -s:x20QT_QPA_DEFAULT_PLATFORM_NAME:x20:x22windows:x22 -r:x20QT_QPA_DEFAULT_PLATFORM_NAME:x20:x22windows::fontengine=freetype:x22 -o "%QT_BUILD%\qtBase\src\corelib\global\qconfig.h"
ENDLOCAL

SET "PATH=%CE_SDKDIR%\bin\i386;%CE_SDKDIR%\bin\i386\%CE_ARCH:~0,3%;%PATH%"
SET "INCLUDE=%CE_SDKDIR%\crt\Include;%CE_SDKDIR%\crt\Include\sys;%CE_SDKDIR%\crt\Include\stl;%CE_SDKDIR%\atlmfc\Include;%CE_SDKDIR%\Inc;%OPENSSL_INC%;%QT_BUILD%\egl\Inc;"
SET "LIB=%CE_SDKDIR%\crt\Lib\%CE_ARCH:~0,3%;%CE_SDKDIR%\atlmfc\lib\%CE_ARCH:~0,3%;%CE_SDKDIR%\Lib\%CE_ARCH%\retail;%QT_BUILD%\egl\Lib\%CE_ARCH%\retail;"

nmake.exe 2>&1 | perl.exe %QT_SOURCE%\build\tools\tee.pl -t -a "%QT_BUILD%\logfile.log"
