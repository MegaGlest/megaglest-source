@echo off

rem change to the directory of this batch file
ECHO --------------------------------
ECHO Changing to build folder [%~dp0] p1 [%1] p2 [%2]
rem pause
cd /d "%~dp0"

ECHO Checking for windows binaries...
ECHO Calling ..\..\data\glest_game\megaglest.exe --version
rem call ..\..\data\glest_game\megaglest.exe --version

set mg_version=
set mg_WIN32_ARCH=win32-i386
set mg_WIN64_ARCH=win64-x86_64
set mg_arch=%mg_WIN32_ARCH%
for /f "tokens=* delims= " %%i in ('..\..\data\glest_game\megaglest.exe --version') do call :mgver %%i
echo after #1 for loop
goto got_ver

:mgver
rem echo mgver [%1%] mg_version is [%mg_version%]
if "%mg_version%." == "." goto set_mg_ver

rem echo in mgver ARCH [%mg_arch%] 1[%1] 2[%2] 3[%3] 4[%4] 5[%5] 6[%6]
if "%1." == "[64bit]." set mg_arch=%mg_WIN64_ARCH%
if "%2." == "[64bit]." set mg_arch=%mg_WIN64_ARCH%
if "%3." == "[64bit]." set mg_arch=%mg_WIN64_ARCH%
if "%4." == "[64bit]." set mg_arch=%mg_WIN64_ARCH%
if "%5." == "[64bit]." set mg_arch=%mg_WIN64_ARCH%
if "%6." == "[64bit]." set mg_arch=%mg_WIN64_ARCH%
if "%7." == "[64bit]." set mg_arch=%mg_WIN64_ARCH%
if "%8." == "[64bit]." set mg_arch=%mg_WIN64_ARCH%
if "%9." == "[64bit]." set mg_arch=%mg_WIN64_ARCH%

rem echo after #2 mg_arch [%mg_arch%]
goto :eof

:set_mg_ver
set mg_version=%2%
rem echo set_mg_ver 1[%mg_version%]

set mg_version=%mg_version:~1%
rem echo set_mg_ver 2[%mg_version%]

:exit_mg_ver
rem exit /B 0
goto :eof

:got_ver
rem echo got_ver [%mg_version%]
rem pause

set RELEASENAME=megaglest-binary-%mg_arch%
set PACKAGE=%RELEASENAME%-%mg_version%.7z

cd /d ..\..\data\glest_game\

echo creating [%PACKAGE%] ...
if exist "%PACKAGE%" del "%PACKAGE%"
set custom_sevenZ_params=
if not "%SEVENZ_MG_COMPRESS_PARAMS%." == "." set custom_sevenZ_params=%SEVENZ_MG_COMPRESS_PARAMS%
echo custom_sevenZ_params [%custom_sevenZ_params%] ...

if "%mg_arch%" == "%mg_WIN32_ARCH%" 7z a -mmt -mx=9 %custom_sevenZ_params% -ms=on -mhc=on "%PACKAGE%" megaglest.exe megaglest_g3dviewer.exe megaglest_editor.exe  libvlc.dll libvlccore.dll lua plugins 7z.exe 7z.dll xml2g.exe openal32.dll g2xml.exe glest.ini glestkeys.ini servers.ini
if "%mg_arch%" == "%mg_WIN64_ARCH%" 7z a -mmt -mx=9 %custom_sevenZ_params% -ms=on -mhc=on "%PACKAGE%" megaglest.exe megaglest_g3dviewer.exe megaglest_editor.exe  7z.exe 7z.dll xml2g.exe openal64.dll g2xml.exe glest.ini glestkeys.ini servers.ini

dir "%PACKAGE%"
cd /d "%~dp0"

rem pause execution so we can see the output before the batch file exits
if not "%1" == "nopause" pause

