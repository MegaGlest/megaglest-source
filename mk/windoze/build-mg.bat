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
if "%DevEnvDir%." == "." call "c:\Program Files\Microsoft Visual Studio 9.0\vc\vcvarsall.bat"

rem Update from SVN to latest rev
ECHO --------------------------------
Echo Updating Code from SVN to latest Revision...
svn update ..\..\

set SVNVERSION=.
for /f "delims=" %%a in ('svnversion ..\..\ -n') do @set SVNVERSION=%%a
ECHO Will build using SVN Revision: [%SVNVERSION%]
rem pause

ECHO --------------------------------
Echo Touching the build date/time file so we get proper build stamp
touch ..\..\source\glest_game\facilities\game_util.cpp

rem Build Mega-Glest in release mode
ECHO --------------------------------
Echo Building Mega-Glest...

set CL=
del ..\..\source\glest_game\facilities\svnversion.h

if not "%SVNVERSION%" == "." set CL=/DSVNVERSIONHEADER
if not "%SVNVERSION%" == "."  echo building with CL [%CL%]
if not "%SVNVERSION%" == "." echo #define SVNVERSION "%SVNVERSION%" > ..\..\source\glest_game\facilities\svnversion.h

msbuild /p:Configuration=Release Glest.sln

rem pause execution so we can see the output before the batch file exits
pause