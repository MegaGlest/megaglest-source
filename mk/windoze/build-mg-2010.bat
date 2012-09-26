@echo off

rem change to the directory of this batch file
ECHO --------------------------------
ECHO Changing to build folder [%~dp0] p1 ["%1"] p2 ["%2"]
rem pause
cd /d "%~dp0"

ECHO Checking for windows binary runtime tools...
if NOT EXIST ..\..\data\glest_game\7z.exe call cscript getTools.vbs
if NOT EXIST ..\..\data\glest_game\7z.dll call cscript getTools.vbs
if NOT EXIST ..\..\data\glest_game\wget.exe call cscript getTools.vbs

set depfolder=windows_deps
set depfile=%depfolder%.7z 

dir ..\..\source\
if NOT EXIST ..\..\source\%depfolder%\NUL echo folder not found [%depfolder%]
if NOT EXIST ..\..\source\%depfolder%\NUL goto checkDepIntegrity
goto processBuildStageA

:getDepFile
ECHO Retrieving windows dependency archive...
call ..\..\data\glest_game\wget.exe -c -O ..\..\source\%depfile%  http://master.dl.sourceforge.net/project/megaglest/%depfile%
call ..\..\data\glest_game\7z.exe x -r -o..\..\source\ ..\..\source\%depfile%
goto processBuildStageA

:checkDepIntegrity
ECHO Looking for windows dependency archive...
call ..\..\data\glest_game\7z.exe t ..\..\source\%depfile% >nul
set 7ztestdep=%ERRORLEVEL%
ECHO Result of windows dependency archive [%7ztestdep%]
if NOT "%7ztestdep%" == "0" goto getDepFile
goto processBuildStageA

:processBuildStageA
call CopyWindowsRuntimeDlls.bat nopause

rem setup the Visual Studio 2010 environment
ECHO --------------------------------
ECHO Setting up Visual Studio 2010 environment vars...
REM Ensure ultifds HP doesn't mess the build up
SET Platform=
if "%DevEnvDir%." == "." goto SETVCVARS
GOTO SVNSECTION

:SETVCVARS

IF EXIST "%VS100COMNTOOLS%..\..\"                             GOTO VC_Common
IF EXIST "\Program Files\Microsoft Visual Studio 10.0\"       GOTO VC_32
IF EXIST "\Program Files (x86)\Microsoft Visual Studio 10.0\" GOTO VC_64
goto SVNSECTION

:VC_Common
call "%VS100COMNTOOLS%..\..\vc\vcvarsall.bat"
goto SVNSECTION

:VC_32
ECHO 32 bit Windows detected...
call "\Program Files\Microsoft Visual Studio 10.0\vc\vcvarsall.bat"
goto SVNSECTION

:VC_64
ECHO 64 bit Windows detected...
call "\Program Files (x86)\Microsoft Visual Studio 10.0\vc\vcvarsall.bat"
goto SVNSECTION


:SVNSECTION
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
rem touch ..\..\source\glest_game\facilities\game_util.cpp
copy /b ..\..\source\glest_game\facilities\game_util.cpp +,,

rem Build Mega-Glest in release mode
ECHO --------------------------------
Echo Building Mega-Glest using Visual Studio 2010...

set CL=
del ..\..\source\glest_game\facilities\svnversion.h

if not "%SVNVERSION%" == "." set CL=/DSVNVERSIONHEADER
if not "%SVNVERSION%" == "."  echo building with CL [%CL%]
if not "%SVNVERSION%" == "." echo #define SVNVERSION "%SVNVERSION%" > ..\..\source\glest_game\facilities\svnversion.h

set msBuildMaxCPU=
SET BuildInParallel=false
if %NUMBER_OF_PROCESSORS% GTR 2 (
                SET NumberOfProcessesToUseForBuild=2
                SET BuildInParallel=true
				SET msBuildMaxCPU=/maxcpucount)

ECHO Found CPU Count [%NUMBER_OF_PROCESSORS%]
if "%2" == "rebuild" echo Doing a FULL REBUILD...
rem if "%2" == "rebuild" msbuild /detailedsummary %msBuildMaxCPU% /p:BuildInParallel=%BuildInParallel% /p:Configuration=Release /t:Rebuild Glest_vc2010.sln
if "%2" == "rebuild" msbuild %msBuildMaxCPU% /p:Configuration=Release /t:Rebuild Glest_vc2010.sln
rem if not "%2" == "rebuild" msbuild /detailedsummary %msBuildMaxCPU% /p:BuildInParallel=%BuildInParallel% /p:Configuration=Release Glest_vc2010.sln
if not "%2" == "rebuild" msbuild %msBuildMaxCPU% /p:Configuration=Release Glest_vc2010.sln

rem pause execution so we can see the output before the batch file exits
if not "%1" == "nopause" pause
