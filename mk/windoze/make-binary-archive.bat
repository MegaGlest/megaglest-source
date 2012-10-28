@echo off

rem change to the directory of this batch file
ECHO --------------------------------
ECHO Changing to build folder [%~dp0] p1 [%1] p2 [%2]
rem pause
cd /d "%~dp0"

ECHO Checking for windows binaries...

set mg_version=
for /f "tokens=2 delims= " %%i in ('..\..\data\glest_game\megaglest.exe --version') do call :mgver %%i
goto got_ver

:mgver
rem echo *[%1%]*
if "%mg_version%." == "." goto set_mg_ver
goto exit_mg_ver

:set_mg_ver
set mg_version=%1%
rem echo *1[%mg_version%]
set mg_version=%mg_version:~1%
rem echo *2[%mg_version%]

:exit_mg_ver
exit /B 0

:got_ver
echo [%mg_version%]

set RELEASENAME=megaglest-binary-win32-i386
set PACKAGE=%RELEASENAME%-%mg_version%.7z

cd /d ..\..\data\glest_game\

echo creating [%PACKAGE%] ...
if exist "%PACKAGE%" del "%PACKAGE%"
set custom_sevenZ_params=
if not "%SEVENZ_MG_COMPRESS_PARAMS%." == "." set custom_sevenZ_params=%SEVENZ_MG_COMPRESS_PARAMS%
echo custom_sevenZ_params [%custom_sevenZ_params%] ...

7z a -mmt -mx=9 %custom_sevenZ_params% -ms=on -mhc=on "%PACKAGE%" megaglest.exe megaglest_g3dviewer.exe megaglest_editor.exe xerces-c_3_0.dll libvlc.dll libvlccore.dll lua plugins 7z.exe 7z.dll xml2g.exe openal32.dll g2xml.exe glest.ini glestkeys.ini servers.ini

dir "%PACKAGE%"
cd /d "%~dp0"

rem pause execution so we can see the output before the batch file exits
if not "%1" == "nopause" pause
