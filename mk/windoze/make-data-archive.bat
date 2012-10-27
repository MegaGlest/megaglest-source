@echo off

rem change to the directory of this batch file
ECHO --------------------------------
ECHO Changing to build folder [%~dp0] p1 [%1] p2 [%2]
rem pause
cd /d "%~dp0"

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

set RELEASENAME=megaglest-standalone-data
set PACKAGE=%RELEASENAME%-%mg_version%.7z
set RELEASEDIR=release-data\%RELEASENAME%-%mg_version%
set PROJDIR=..\..\

rem to debug creating the archive only
rem goto make_archive

echo Creating data package in [%RELEASEDIR%]

if exist %RELEASEDIR% rd /s /q %RELEASEDIR%
mkdir %RELEASEDIR%

rem copy data
echo copying data ...
mkdir %RELEASEDIR%\data\
svn export --force ..\..\data\glest_game\data %RELEASEDIR%\data\
mkdir %RELEASEDIR%\docs\
svn export --force ..\..\data\glest_game\docs %RELEASEDIR%\docs\
svn export --force ..\..\data\glest_game\docs\CHANGELOG.txt %RELEASEDIR%\docs\CHANGELOG.txt
svn export --force ..\..\data\glest_game\README.txt %RELEASEDIR%\docs\README.txt
mkdir %RELEASEDIR%\maps\
svn export --force ..\..\data\glest_game\maps %RELEASEDIR%\maps\
mkdir %RELEASEDIR%\scenarios\
svn export --force ..\..\data\glest_game\scenarios %RELEASEDIR%\scenarios\
mkdir %RELEASEDIR%\techs\
svn export --force ..\..\data\glest_game\techs %RELEASEDIR%\techs\
mkdir %RELEASEDIR%\tilesets\
svn export --force ..\..\data\glest_game\tilesets %RELEASEDIR%\tilesets\
mkdir %RELEASEDIR%\tutorials\
svn export --force ..\..\data\glest_game\tutorials %RELEASEDIR%\tutorials\

rem special export for flag images
mkdir %RELEASEDIR%\data\core\misc_textures\flags\
svn export --force ..\..\source\masterserver\flags %RELEASEDIR%\data\core\misc_textures\flags\

rem START
rem remove embedded data
rem rm -rf "%RELEASEDIR%\data\core\fonts"
rem END
:make_archive
rem echo Current directory[%CD%]
echo creating data archive: %PACKAGE%
if exist release-data%PACKAGE% del release-data%PACKAGE%
cd /d %RELEASEDIR%
rem echo Current directory[%CD%]

..\..\..\..\data\glest_game\7z.exe a -mmt -mx=9 -ms=on -mhc=on ..\%PACKAGE% *

dir "..\%PACKAGE%"
cd /d "%~dp0"

rem pause execution so we can see the output before the batch file exits
if not "%1" == "nopause" pause
