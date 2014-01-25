@echo off

rem change to the directory of this batch file
ECHO --------------------------------
ECHO Changing to build folder [%~dp0] p1 [%1] p2 [%2]
rem pause
cd /d "%~dp0"

set mg_version=
for /f "tokens=2 delims= " %%i in ('.\megaglest.exe --version') do call :mgver %%i
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
set REPODIR=%~dp0\..\..\
set PATH=%path%;%~dp0.\
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
git archive --remote %REPODIR%\data\glest_game\ HEAD:data | tar -x
cd /d "%~dp0"
rem pause

mkdir %RELEASEDIR%\docs\
cd /d %RELEASEDIR%\docs\
echo GIT ARCHIVE docs ...
git archive --remote %REPODIR%/data/glest_game/ HEAD:docs | tar -x
cd /d "%~dp0"

cd /d %RELEASEDIR%\docs\
echo GIT ARCHIVE CHANGELOG.txt ...
git archive --remote %REPODIR% HEAD:docs/ CHANGELOG.txt | tar -x
cd /d "%~dp0"
rem pause

cd /d %RELEASEDIR%\docs\
echo GIT ARCHIVE README.txt ...
git archive --remote %REPODIR% HEAD:docs/ README.txt | tar -x
cd /d "%~dp0"
rem pause

mkdir %RELEASEDIR%\maps\
cd /d %RELEASEDIR%\maps\
echo GIT ARCHIVE maps ...
git archive --remote %REPODIR%/data/glest_game/ HEAD:maps | tar -x
cd /d "%~dp0"

mkdir %RELEASEDIR%\scenarios\
cd /d %RELEASEDIR%\scenarios\
echo GIT ARCHIVE scenarios ...
git archive --remote %REPODIR%/data/glest_game/ HEAD:scenarios | tar -x
cd /d "%~dp0"

mkdir %RELEASEDIR%\techs\
cd /d %RELEASEDIR%\techs\
echo GIT ARCHIVE techs ...
git archive --remote %REPODIR%/data/glest_game/ HEAD:techs | tar -x
cd /d "%~dp0"

mkdir %RELEASEDIR%\tilesets\
cd /d %RELEASEDIR%\tilesets\
echo GIT ARCHIVE tilesets ...
git archive --remote %REPODIR%/data/glest_game/ HEAD:tilesets | tar -x
cd /d "%~dp0"

mkdir %RELEASEDIR%\tutorials\
cd /d %RELEASEDIR%\tutorials\
echo GIT ARCHIVE tutorials ...
git archive --remote %REPODIR%/data/glest_game/ HEAD:tutorials | tar -x
cd /d "%~dp0"

rem special export for flag images
mkdir %RELEASEDIR%\data\core\misc_textures\flags\
cd /d %RELEASEDIR%\data\core\misc_textures\flags\
echo GIT ARCHIVE flags ...
git archive --remote %REPODIR% HEAD:source/masterserver/flags | tar -x
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

