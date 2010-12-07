@echo off

rem change to the directory of this batch file
ECHO --------------------------------
ECHO Changing to build folder [%~dp0]
cd /d "%~dp0"

rem setup the Visual Studio 2008 environment
ECHO --------------------------------
ECHO Setting up Visual Studio 2008 environment vars...
REM Ensure ultifds HP doesn't mess the build up
SET Platform=
call "c:\Program Files\Microsoft Visual Studio 9.0\vc\vcvarsall.bat"

rem Update from SVN to latest rev
ECHO --------------------------------
Echo Updating Code from SVN to latest Revision...
svn update ..\..\

ECHO --------------------------------
Echo Touching the build date/time file so we get proper build stamp
touch ..\..\source\glest_game\facilities\game_util.cpp

rem Build Mega-Glest in release mode
ECHO --------------------------------
Echo Building Mega-Glest...
msbuild Glest.sln /p:Configuration=Release

rem pause execution so we can see the output before the batch file exits
pause