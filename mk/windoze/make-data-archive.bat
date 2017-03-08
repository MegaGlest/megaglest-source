@echo off

rem change to the directory of this batch file
ECHO --------------------------------
ECHO Changing to build folder [%~dp0] p1 [%1] p2 [%2]
rem pause
cd /d "%~dp0"

set CUR_DIR=%~dp0
set RELEASENAME=megaglest-standalone-data
set PROJDIR=..\..\
set REPODIR=%~dp0\..\..\
set PATH=%path%;%~dp0.\

set mg_version=
if exist ".\megaglest.exe" for /f "tokens=2 delims= " %%i in ('.\megaglest.exe --version') do call :mgver %%i
if exist "%REPODIR%data\glest_game\megaglest.exe" for /f "tokens=2 delims= " %%i in ('%REPODIR%data\glest_game\megaglest.exe --version') do call :mgver %%i
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

set PACKAGE=%RELEASENAME%-%mg_version%.7z
set RELEASEDIR=release-data\%RELEASENAME%-%mg_version%
rem to debug creating the archive only
rem goto make_archive

echo Creating data package in [%RELEASEDIR%]

if exist %RELEASEDIR% echo Cleaning previous release folder [%RELEASEDIR%]
if exist %RELEASEDIR% rd /s /q %RELEASEDIR%
mkdir %RELEASEDIR%

rem copy data
echo Copying data ...
mkdir %RELEASEDIR%\data\
cd /d %RELEASEDIR%\data\
echo GIT ARCHIVE data ...
git archive --format=tar --remote %REPODIR%\data\glest_game\ HEAD:data | %CUR_DIR%\tar.exe -xf -
cd /d "%~dp0"
rem pause

mkdir %RELEASEDIR%\docs\
cd /d %RELEASEDIR%\docs\
echo GIT ARCHIVE docs ...
git archive --format=tar --remote %REPODIR%/data/glest_game/ HEAD:docs | %CUR_DIR%\tar.exe -xf -
cd /d "%~dp0"

cd /d %RELEASEDIR%\docs\
echo GIT ARCHIVE CHANGELOG.txt ...
git archive --format=tar --remote %REPODIR% HEAD:docs/ CHANGELOG.txt | %CUR_DIR%\tar.exe -xf -
cd /d "%~dp0"
rem pause

cd /d %RELEASEDIR%\docs\
echo GIT ARCHIVE README.txt ...
git archive --format=tar --remote %REPODIR% HEAD:docs/ README.txt | %CUR_DIR%\tar.exe -xf -
cd /d "%~dp0"
rem pause

mkdir %RELEASEDIR%\maps\
cd /d %RELEASEDIR%\maps\
echo GIT ARCHIVE maps ...
git archive --format=tar --remote %REPODIR%/data/glest_game/ HEAD:maps | %CUR_DIR%\tar.exe -xf -
cd /d "%~dp0"

mkdir %RELEASEDIR%\scenarios\
cd /d %RELEASEDIR%\scenarios\
echo GIT ARCHIVE scenarios ...
git archive --format=tar --remote %REPODIR%/data/glest_game/ HEAD:scenarios | %CUR_DIR%\tar.exe -xf -
cd /d "%~dp0"

mkdir %RELEASEDIR%\techs\
cd /d %RELEASEDIR%\techs\
echo GIT ARCHIVE techs ...
git archive --format=tar --remote %REPODIR%/data/glest_game/ HEAD:techs | %CUR_DIR%\tar.exe -xf -
cd /d "%~dp0"

mkdir %RELEASEDIR%\tilesets\
cd /d %RELEASEDIR%\tilesets\
echo GIT ARCHIVE tilesets ...
git archive --format=tar --remote %REPODIR%/data/glest_game/ HEAD:tilesets | %CUR_DIR%\tar.exe -xf -
cd /d "%~dp0"

mkdir %RELEASEDIR%\tutorials\
cd /d %RELEASEDIR%\tutorials\
echo GIT ARCHIVE tutorials ...
git archive --format=tar --remote %REPODIR%/data/glest_game/ HEAD:tutorials | %CUR_DIR%\tar.exe -xf -
cd /d "%~dp0"

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

set custom_sevenZ_params=
if not "%SEVENZ_MG_COMPRESS_PARAMS%." == "." set custom_sevenZ_params=%SEVENZ_MG_COMPRESS_PARAMS%
echo custom_sevenZ_params [%custom_sevenZ_params%] ...

..\..\7z.exe a -mmt -mx=9 %custom_sevenZ_params% -ms=on -mhc=on ..\%PACKAGE% *

dir "..\%PACKAGE%"
cd /d "%~dp0"

rem pause execution so we can see the output before the batch file exits
if not "%1" == "nopause" pause

