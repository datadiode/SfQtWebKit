SETLOCAL

CALL "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 8.1

SET QMAKESPEC=%~n0-msvc2015

SET QT_TOOLS_PATH=C:\qt\Tools\QtCreator\bin

CD %~dp0..
SET QT_SOURCE=%CD%
SET QT_BUILD=%CD%\%~n0-build

SET FFMPEG_SRC=%QT_SOURCE%\FFmpeg
SET FFMPEG_LIB=%QT_SOURCE%\FFmpeg\SMP\out\Release\Win32
SET ICU_DIST=%QT_SOURCE%\icu4c
SET OPENSSL_INC=%~dp0wolfssl;%~dp0wolfssl\wolfssl
SET OPENSSL_LIB=%~dp0wolfssl\DLL Release\Win32
SET OPENSSL_LIBS=-lwolfssl
ECHO DXSDK_DIR=%DXSDK_DIR%

MKDIR %QT_BUILD%
CD /D %QT_BUILD%

SET PATH=%QT_BUILD%\qtbase\lib;%QT_SOURCE%\gnuwin32\bin;%ICU_DIST%\bin;%PATH%
ECHO CONFIGURATION=%CONFIGURATION%

SET "INCLUDE=%INCLUDE%;%OPENSSL_INC%;%ICU_DIST%\include;"
SET "LIB=%LIB%;%ICU_DIST%\lib;"

GOTO %CONFIGURATION%

:module-qtbase-make_first
:module-qtmultimedia-make_first
:module-qtlocation-make_first
:module-qtsensors-make_first
:module-qtimageformats-make_first

CALL %QT_SOURCE%\VSYASM\install_script.bat %VisualStudioVersion:~0,2% "%VSINSTALLDIR:~0,-1%"
CALL %QT_SOURCE%\VSC99WRAP\install_script.bat %VisualStudioVersion:~0,2% "%VSINSTALLDIR:~0,-1%"

xcopy /Y /S /I %QT_SOURCE%\build\wolfssl\wolfssl %QT_SOURCE%\prebuilt\include\wolfssl

msbuild /v:minimal /t:Rebuild "%QT_SOURCE%\FFmpeg\SMP\ffmpeg.sln" /p:Platform="x86" /p:Configuration="Release"

msbuild /v:minimal /t:Rebuild "%QT_SOURCE%\icu4c\source\allinone\allinone.sln" /p:Platform="Win32" /p:Configuration="Release"

msbuild /v:minimal /t:Rebuild "%~dp0wolfssl\wolfssl64.sln" /p:Platform="Win32" /p:Configuration="DLL Release"

CALL %QT_SOURCE%\configure.bat -rtti -no-harfbuzz -no-wmf-backend -ssl -openssl-linked OPENSSL_LIBS="%OPENSSL_LIBS%" -icu -opengl desktop -opensource -nomake tests -nomake examples -nomake tools -skip translations -skip qtdoc -skip qt3d -skip qtsvg -skip qtxmlpatterns -skip qtenginio -skip qtconnectivity -skip qtserialport -skip qttools -skip qtwebchannel -skip qtwebsockets -skip qtdeclarative -skip qtquick1 -skip qtscript -skip qtwebkit-examples -confirm-license -platform %QMAKESPEC% -release

"%QT_TOOLS_PATH%\jom.exe" /C %CONFIGURATION%
"%QT_TOOLS_PATH%\jom.exe" /C module-qtwebkit-qmake_all
CD %QT_SOURCE%
7z.exe a -mx9 -xr!wolfssl_obj -xr!examples -xr!tests %~n0.7z build\wolfssl\DLL* build\wolfssl\wolfssl build\egl %~n0-build qtbase\include\QtCore\qconfig.h qtbase\include\QtCore\qfeatures.h qtbase\include\QtCore\QtConfig qtwebkit\Source\JavaScriptCore\disassembler\udis86\*.pyc qtwebkit\Source\WebCore\inspector\*.pyc qtwebkit\Source\WebKit2\Scripts\webkit2\*.pyc icu4c\bin icu4c\include icu4c\lib

GOTO :eof

:sub-Source-WTF-WTF-pro-make_first-ordered
:sub-Source-JavaScriptCore-JavaScriptCore-pro-make_first-ordered
:sub-Source-ThirdParty-leveldb-leveldb-pro-make_first-ordered
:sub-Source-WebCore-WebCore-pro-make_first-ordered

REM CD qtwebkit
REM "%QT_TOOLS_PATH%\jom.exe" /C %CONFIGURATION%
REM CD %QT_SOURCE%
REM 7z.exe a -mx9 snapshot-qtwebkit.7z %~n0-build\qtwebkit qtwebkit\Source\JavaScriptCore\disassembler\udis86\*.pyc qtwebkit\Source\WebCore\inspector\*.pyc qtwebkit\Source\WebKit2\Scripts\webkit2\*.pyc

REM GOTO :eof

:sub-Source-WebKit-WebKit1-pro-make_first-ordered
:sub-Source-QtWebKit-pro-make_first-ordered
:sub-Tools-Tools-pro-make_first-ordered

CD qtwebkit
"%QT_TOOLS_PATH%\jom.exe" /C %CONFIGURATION%

MKDIR %QT_BUILD%\browser
CD %QT_BUILD%\browser
cmake.exe -G "Visual Studio 15 2017" ^
	-DCMAKE_PREFIX_PATH=%QT_BUILD%\qtbase\lib\cmake ^
	-DQT_MOC_EXECUTABLE=%QT_BUILD%\qtbase\bin\moc.exe ^
	-DQT_RCC_EXECUTABLE=%QT_BUILD%\qtbase\bin\rcc.exe ^
	-DQT_UIC_EXECUTABLE=%QT_BUILD%\qtbase\bin\uic.exe ^
	%QT_SOURCE%\browser
gsar -s"DLL</RuntimeLibrary>" -r"</RuntimeLibrary>" -o endorphin.vcxproj
msbuild /v:minimal /t:Rebuild "Endorphin.sln" /p:Platform="Win32" /p:Configuration="Release"
CD %QT_SOURCE%\browser
CALL rcupdate.bat "%QT_BUILD%\browser\Release\endorphin.exe"
MKDIR WIN32
MKDIR WIN32\imageformats
MKDIR WIN32\platforms
MKDIR WIN32\mediaservice
COPY "%QT_BUILD%\browser\Release\endorphin.exe" WIN32
COPY "%QT_BUILD%\qtbase\lib\Qt5Core.dll" WIN32
COPY "%QT_BUILD%\qtbase\lib\Qt5Gui.dll" WIN32
COPY "%QT_BUILD%\qtbase\lib\Qt5Multimedia.dll" WIN32
COPY "%QT_BUILD%\qtbase\lib\Qt5MultimediaWidgets.dll" WIN32
COPY "%QT_BUILD%\qtbase\lib\Qt5Network.dll" WIN32
COPY "%QT_BUILD%\qtbase\lib\Qt5OpenGL.dll" WIN32
COPY "%QT_BUILD%\qtbase\lib\Qt5Positioning.dll" WIN32
COPY "%QT_BUILD%\qtbase\lib\Qt5PrintSupport.dll" WIN32
COPY "%QT_BUILD%\qtbase\lib\Qt5Sensors.dll" WIN32
COPY "%QT_BUILD%\qtbase\lib\Qt5Sql.dll" WIN32
COPY "%QT_BUILD%\qtbase\lib\Qt5WebKit.dll" WIN32
COPY "%QT_BUILD%\qtbase\lib\Qt5WebKitWidgets.dll" WIN32
COPY "%QT_BUILD%\qtbase\lib\Qt5Widgets.dll" WIN32
COPY "%QT_BUILD%\qtbase\plugins\imageformats\*.dll" WIN32\imageformats
COPY "%QT_BUILD%\qtbase\plugins\platforms\qwindows.dll" WIN32\platforms
COPY "%QT_BUILD%\qtbase\plugins\mediaservice\dsengine.dll" WIN32\mediaservice
COPY "%QT_BUILD%\qtbase\plugins\mediaservice\ffmpeg-*.dll" WIN32\mediaservice
COPY "%ICU_DIST%\bin\icudt56.dll" WIN32
COPY "%ICU_DIST%\bin\icuin56.dll" WIN32
COPY "%ICU_DIST%\bin\icuuc56.dll" WIN32
COPY "%OPENSSL_LIB%\wolfssl.dll" WIN32
FOR /F %%X IN ('git describe') DO 7z.exe a -mx9 browser_%%X.7z WIN32 ARM_800 X86_800 LICENSE.GPL2 piimake.bat

CD %QT_SOURCE%
7z.exe a -mx9 -xr!.obj %~n0.7z %~n0-build\qtbase\bin\Qt5WebKit*.* %~n0-build\qtbase\lib\Qt5WebKit*.* %~n0-build\qtwebkit %~n0-build\browser
