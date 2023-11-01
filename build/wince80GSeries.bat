SETLOCAL

CALL "%ProgramFiles(x86)%\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86

SET QMAKESPEC=%~n0-msvc2012

SET QT_HOST_PATH=C:\qt\5.5\msvc2013
SET QT_TOOLS_PATH=C:\qt\Tools\QtCreator\bin

CD %~dp0..
SET QT_SOURCE=%CD%
SET QT_BUILD=%CD%\%~n0-build

SET ICU_DIST=%QT_SOURCE%\icu4c
SET OPENSSL_INC=%~dp0wolfssl;%~dp0wolfssl\wolfssl
SET OPENSSL_LIB=%~dp0wolfssl\DLL Release\Compact2013_SDK_GSeries
SET OPENSSL_LIBS=-lwolfssl

MKDIR %QT_BUILD%
CD /D %QT_BUILD%

SET PATH=%QT_SOURCE%\gnuwin32\bin;%ICU_DIST%\bin;%PATH%
ECHO CONFIGURATION=%CONFIGURATION%

FOR /F "tokens=1,2,* delims== " %%A IN (%QT_SOURCE%\qtBase\mkspecs\%QMAKESPEC%\qmake.conf) DO IF "%%A" geq "CE_ARCH" IF "%%A" leq "CE_SDK" SET "%%A=%%B" 
SET CE_SDKDIR=%ProgramFiles(x86)%\Windows CE Tools\SDKs\%CE_SDK%\Sdk
SET CE_SDKDIR_TORADEX=%ProgramFiles(x86)%\Windows CE Tools\SDKs\Toradex_CE800\Sdk
SET CE_
SET OPENGL_ES2=%QT_BUILD%\egl

GOTO %CONFIGURATION%

:module-qtbase-make_first
:module-qtmultimedia-make_first
:module-qtlocation-make_first
:module-qtsensors-make_first
:module-qtimageformats-make_first

xcopy /I /S /R /Y "%CE_SDKDIR_TORADEX%\Inc\egl" "egl\Inc\egl"
xcopy /I /S /R /Y "%CE_SDKDIR_TORADEX%\Inc\GLES" "egl\Inc\GLES"
xcopy /I /S /R /Y "%CE_SDKDIR_TORADEX%\Inc\GLES2" "egl\Inc\GLES2"
xcopy /I /T "%CE_SDKDIR%\Lib" "egl\Lib"
FOR %%X IN (libegl libglesv2) DO lib /def:%~dp0egl\%%X.def /machine:%CE_ARCH% /out:egl\Lib\%CE_ARCH%\retail\%%X.lib

"%QT_SOURCE%\sfk198.exe" sel "%QT_SOURCE%\icu4c\source" *.vcxproj *.sln +replace -text "/Compact2013_SDK_86Duino_80B/Compact2013_SDK_GSeries/" -yes > nul
msbuild /v:minimal /t:Rebuild "%QT_SOURCE%\icu4c\source\allinone\allinone.sln" /p:Platform="Win32" /p:Configuration="Release" /p:PlatformToolset="v120"
msbuild /v:minimal /t:Build "%QT_SOURCE%\icu4c\source\allinone\allinone.sln" /p:Platform="Compact2013_SDK_GSeries" /p:Configuration="Release"

msbuild /v:minimal /t:Rebuild "%~dp0wolfssl\wolfssl64.sln" /p:Platform="Compact2013_SDK_GSeries" /p:Configuration="DLL Release"

SETLOCAL
	SET "INCLUDE=%INCLUDE%;%OPENSSL_INC%;"
	CALL %QT_SOURCE%\configure.bat -opensource -confirm-license -rtti -no-harfbuzz -ssl -openssl-linked OPENSSL_LIBS="%OPENSSL_LIBS%" -icu -opengl es2 -nomake tests -nomake examples -nomake tools -skip translations -skip qtdoc -skip qt3d -skip qtsvg -skip qtxmlpatterns -skip qtenginio -skip qtconnectivity -skip qtserialport -skip qttools -skip qtwayland -skip qtwebchannel -skip qtwebsockets -skip qtdeclarative -skip qtquick1 -skip qtscript -skip qtwebkit-examples -platform win32-msvc2012 -xplatform %QMAKESPEC% -release
	REM Change QT_QPA_DEFAULT_PLATFORM_NAME from "windows" to "windows:fontengine=freetype"
	gsar -s:x20QT_QPA_DEFAULT_PLATFORM_NAME:x20:x22windows:x22 -r:x20QT_QPA_DEFAULT_PLATFORM_NAME:x20:x22windows::fontengine=freetype:x22 -o "%QT_BUILD%\qtBase\src\corelib\global\qconfig.h"
ENDLOCAL

SET "PATH=%CE_SDKDIR%\bin\i386;%CE_SDKDIR%\bin\i386\%CE_ARCH:~0,3%;%PATH%"
SET "INCLUDE=%CE_SDKDIR%\crt\Include;%CE_SDKDIR%\crt\Include\sys;%CE_SDKDIR%\crt\Include\stl;%CE_SDKDIR%\Inc;%OPENSSL_INC%;%QT_BUILD%\egl\Inc;"
SET "LIB=%CE_SDKDIR%\crt\Lib\%CE_ARCH:~0,3%;%CE_SDKDIR%\Lib\%CE_ARCH%\retail;%QT_BUILD%\egl\Lib\%CE_ARCH%\retail;"

"%QT_TOOLS_PATH%\jom.exe" /C %CONFIGURATION%
"%QT_TOOLS_PATH%\jom.exe" /C module-qtwebkit-qmake_all
CD %QT_SOURCE%
7z.exe a -mx9 -xr!wolfssl_obj -xr!examples -xr!tests %~n0.7z build\wolfssl\DLL* build\wolfssl\wolfssl build\egl %~n0-build qtbase\include\QtCore\qconfig.h qtbase\include\QtCore\qfeatures.h qtbase\include\QtCore\QtConfig qtwebkit\Source\JavaScriptCore\disassembler\udis86\*.pyc qtwebkit\Source\WebCore\inspector\*.pyc qtwebkit\Source\WebKit2\Scripts\webkit2\*.pyc icu4c\bin icu4c\include icu4c\lib

GOTO :eof

:sub-Source-WTF-WTF-pro-make_first-ordered
:sub-Source-JavaScriptCore-JavaScriptCore-pro-make_first-ordered
:sub-Source-ThirdParty-leveldb-leveldb-pro-make_first-ordered
:sub-Source-WebCore-WebCore-pro-make_first-ordered

REM SET "PATH=%CE_SDKDIR%\bin\i386;%CE_SDKDIR%\bin\i386\%CE_ARCH:~0,3%;%PATH%"
REM SET "INCLUDE=%CE_SDKDIR%\crt\Include;%CE_SDKDIR%\crt\Include\sys;%CE_SDKDIR%\crt\Include\stl;%CE_SDKDIR%\Inc;%OPENSSL_INC%;%QT_BUILD%\egl\Inc;"
REM SET "LIB=%CE_SDKDIR%\crt\Lib\%CE_ARCH:~0,3%;%CE_SDKDIR%\Lib\%CE_ARCH%\retail;%QT_BUILD%\egl\Lib\%CE_ARCH%\retail;"

REM CD qtwebkit
REM "%QT_TOOLS_PATH%\jom.exe" /C %CONFIGURATION%
REM CD %QT_SOURCE%
REM 7z.exe a -mx9 snapshot-qtwebkit.7z %~n0-build\qtwebkit qtwebkit\Source\JavaScriptCore\disassembler\udis86\*.pyc qtwebkit\Source\WebCore\inspector\*.pyc qtwebkit\Source\WebKit2\Scripts\webkit2\*.pyc

REM GOTO :eof

:sub-Source-WebKit-WebKit1-pro-make_first-ordered
:sub-Source-QtWebKit-pro-make_first-ordered
:sub-Tools-Tools-pro-make_first-ordered

SET "PATH=%CE_SDKDIR%\bin\i386;%CE_SDKDIR%\bin\i386\%CE_ARCH:~0,3%;%PATH%"
SET "INCLUDE=%CE_SDKDIR%\crt\Include;%CE_SDKDIR%\crt\Include\sys;%CE_SDKDIR%\crt\Include\stl;%CE_SDKDIR%\Inc;%OPENSSL_INC%;%QT_BUILD%\egl\Inc;"
SET "LIB=%CE_SDKDIR%\crt\Lib\%CE_ARCH:~0,3%;%CE_SDKDIR%\Lib\%CE_ARCH%\retail;%QT_BUILD%\egl\Lib\%CE_ARCH%\retail;"

CD qtwebkit
"%QT_TOOLS_PATH%\jom.exe" /C %CONFIGURATION%

MKDIR %QT_BUILD%\browser
CD %QT_BUILD%\browser
cmake.exe -G "Visual Studio 11 2012" ^
	-A"Compact2013_SDK_GSeries" ^
	-T"CE800" ^
	-DCMAKE_SYSTEM_NAME="WindowsCE" ^
	-DCMAKE_SYSTEM_PROCESSOR="x86" ^
	-DCMAKE_SYSTEM_VERSION="8.0" ^
	-DCMAKE_PREFIX_PATH=%QT_BUILD%\qtbase\lib\cmake ^
	-DQT_MOC_EXECUTABLE=%QT_HOST_PATH%\bin\moc.exe ^
	-DQT_RCC_EXECUTABLE=%QT_HOST_PATH%\bin\rcc.exe ^
	-DQT_UIC_EXECUTABLE=%QT_HOST_PATH%\bin\uic.exe ^
	%QT_SOURCE%\browser
gsar -s"DLL</RuntimeLibrary>" -r"</RuntimeLibrary>" -o endorphin.vcxproj
msbuild /v:minimal /t:Rebuild "Endorphin.sln" /p:Platform="Compact2013_SDK_GSeries" /p:Configuration="MinSizeRel"
CD %QT_SOURCE%\browser
CALL rcupdate.bat "%QT_BUILD%\browser\MinSizeRel\endorphin.exe"
MKDIR %CE_ARCH:~0,3%_800
MKDIR %CE_ARCH:~0,3%_800\imageformats
MKDIR %CE_ARCH:~0,3%_800\platforms
COPY "%QT_BUILD%\browser\MinSizeRel\endorphin.exe" %CE_ARCH:~0,3%_800
COPY "%QT_BUILD%\qtbase\lib\Qt5Core.dll" %CE_ARCH:~0,3%_800
COPY "%QT_BUILD%\qtbase\lib\Qt5Gui.dll" %CE_ARCH:~0,3%_800
COPY "%QT_BUILD%\qtbase\lib\Qt5Multimedia.dll" %CE_ARCH:~0,3%_800
COPY "%QT_BUILD%\qtbase\lib\Qt5MultimediaWidgets.dll" %CE_ARCH:~0,3%_800
COPY "%QT_BUILD%\qtbase\lib\Qt5Network.dll" %CE_ARCH:~0,3%_800
COPY "%QT_BUILD%\qtbase\lib\Qt5OpenGL.dll" %CE_ARCH:~0,3%_800
COPY "%QT_BUILD%\qtbase\lib\Qt5Positioning.dll" %CE_ARCH:~0,3%_800
COPY "%QT_BUILD%\qtbase\lib\Qt5Sensors.dll" %CE_ARCH:~0,3%_800
COPY "%QT_BUILD%\qtbase\lib\Qt5Sql.dll" %CE_ARCH:~0,3%_800
COPY "%QT_BUILD%\qtbase\lib\Qt5WebKit.dll" %CE_ARCH:~0,3%_800
COPY "%QT_BUILD%\qtbase\lib\Qt5WebKitWidgets.dll" %CE_ARCH:~0,3%_800
COPY "%QT_BUILD%\qtbase\lib\Qt5Widgets.dll" %CE_ARCH:~0,3%_800
COPY "%QT_BUILD%\qtbase\plugins\imageformats\*.dll" %CE_ARCH:~0,3%_800\imageformats
COPY "%QT_BUILD%\qtbase\plugins\platforms\qwindows.dll" %CE_ARCH:~0,3%_800\platforms
COPY "%ICU_DIST%\bin\Compact2013_SDK_GSeries\icudt56.dll" %CE_ARCH:~0,3%_800
COPY "%ICU_DIST%\bin\Compact2013_SDK_GSeries\icuin56.dll" %CE_ARCH:~0,3%_800
COPY "%ICU_DIST%\bin\Compact2013_SDK_GSeries\icuuc56.dll" %CE_ARCH:~0,3%_800
COPY "%OPENSSL_LIB%\wolfssl.dll" %CE_ARCH:~0,3%_800
FOR /F %%X IN ('git describe') DO 7z.exe a -mx9 browser_%%X.7z ARM_800 X86_800 LICENSE.GPL2 piimake.bat

CD %QT_SOURCE%
7z.exe a -mx9 -xr!.obj %~n0.7z %~n0-build\qtbase\bin\Qt5WebKit*.* %~n0-build\qtbase\lib\Qt5WebKit*.* %~n0-build\qtwebkit %~n0-build\browser
