@echo off

rem change to the directory of this batch file
ECHO --------------------------------
ECHO Changing to build folder [%~dp0] p1 [%1] p2 [%2]
rem pause
cd /d "%~dp0"

ECHO Checking for windows binaries...

SET MG_BINARY_NAME=megaglest.exe
SET MG_BINARY_PATH=..\..\data\glest_game\
rem if exist "megaglestx64.exe" SET MG_BINARY_NAME=megaglestx64.exe
ECHO Calling .\%MG_BINARY_PATH%%MG_BINARY_NAME% --version

set mg_version=
set mg_WIN32_ARCH=win32-i386
set mg_WIN64_ARCH=win64-x86_64
set mg_arch=%mg_WIN32_ARCH%
for /f "tokens=* delims= " %%i in ('.\%MG_BINARY_PATH%%MG_BINARY_NAME% --version') do call :mgver %%i
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

rem cd /d release-data\

echo creating [%PACKAGE%] ...
if exist "%PACKAGE%" del "%PACKAGE%"
set custom_sevenZ_params=
if not "%SEVENZ_MG_COMPRESS_PARAMS%." == "." set custom_sevenZ_params=%SEVENZ_MG_COMPRESS_PARAMS%
echo custom_sevenZ_params [%custom_sevenZ_params%] ...

if "%mg_arch%" == "%mg_WIN32_ARCH%" 7z a -mmt -mx=9 %custom_sevenZ_params% -ms=on -mhc=on "release-data\%PACKAGE%" %MG_BINARY_PATH%megaglest.exe %MG_BINARY_PATH%megaglest_g3dviewer.exe %MG_BINARY_PATH%megaglest_editor.exe 7z.exe 7z.dll %MG_BINARY_PATH%xml2g.exe %MG_BINARY_PATH%openal32.dll %MG_BINARY_PATH%g2xml.exe glest.ini ..\shared\glestkeys.ini ..\shared\servers.ini
if "%mg_arch%" == "%mg_WIN64_ARCH%" 7z a -mmt -mx=9 %custom_sevenZ_params% -ms=on -mhc=on "release-data\%PACKAGE%" megaglestx64.exe megaglest_g3dviewerx64.exe megaglest_editorx64.exe 7z.exe 7z.dll xml2gx64.exe openal64.dll g2xmlx64.exe glest.ini ..\shared\glestkeys.ini ..\shared\servers.ini

dir "release-data\%PACKAGE%"
cd /d "%~dp0"

rem pause execution so we can see the output before the batch file exits
if not "%1" == "nopause" pause

