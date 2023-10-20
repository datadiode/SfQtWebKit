C:/Python35-x64/Scripts/pip.exe install selenium
C:/Python35-x64/python.exe download_toradex_sdk.py
C:/msys64/usr/bin/wget.exe -nv https://download.microsoft.com/download/B/C/4/BC4FA89D-4F7B-4022-A4C1-2B3B6E08D8BE/AppBuilderSetup_VS2012_v50806.zip
C:/msys64/usr/bin/wget.exe -nv https://github.com/datadiode/supplements/raw/main/Compact2013_SDK_GSeries.msi
7z.exe x -oAppBuilderSetup AppBuilderSetup_VS2012_v50806.zip
7z.exe x toradex_ce8_sdk_2.3_bis.zip
AppBuilderSetup\VSEmbedded_AppBuilder.exe /Quiet /NoRestart
msiexec /i Compact2013_SDK_GSeries.msi /quiet /norestart
msiexec /i Toradex_CE8_SDK_2_3_bis.msi /quiet /norestart
