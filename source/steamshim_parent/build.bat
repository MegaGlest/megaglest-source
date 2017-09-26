@echo off

rem change to the directory of this batch file
rem SET VCVARS_PLATFORM=x86_amd64
SET VCVARS_PLATFORM=
SET MSBUILD_CONFIG=Release

ECHO --------------------------------
ECHO Changing to build folder [%~dp0] p1 ["%1"] p2 ["%2"]
rem pause
cd /d "%~dp0"

ECHO using msbuild config [%MSBUILD_CONFIG%]
ECHO Checking for windows binary runtime tools...

rem setup the Visual Studio 2015 environment
ECHO --------------------------------
ECHO Setting up Visual Studio 2015 environment vars...
REM Ensure ultifds HP doesn't mess the build up
SET Platform=
if "%DevEnvDir%." == "." goto SETVCVARS
GOTO GITSECTION

:SETVCVARS

IF EXIST "%VS140COMNTOOLS%..\..\"                             GOTO VC_Common_15
IF EXIST "\Program Files\Microsoft Visual Studio 14.0\"       GOTO VC_32_15
IF EXIST "\Program Files (x86)\Microsoft Visual Studio 14.0\" GOTO VC_64_15
goto GITSECTION

:VC_Common_15
call "%VS140COMNTOOLS%..\..\vc\vcvarsall.bat" %VCVARS_PLATFORM%
goto GITSECTION

:VC_32_15
ECHO 32 bit Windows detected...
call "\Program Files\Microsoft Visual Studio 14.0\vc\vcvarsall.bat" %VCVARS_PLATFORM%
goto GITSECTION

:VC_64_15
ECHO 64 bit Windows detected...
call "\Program Files (x86)\Microsoft Visual Studio 14.0\vc\vcvarsall.bat" %VCVARS_PLATFORM%
goto GITSECTION

:GITSECTION
rem Update from GIT to latest rev
ECHO --------------------------------
ECHO Will build using GIT Revision: [%GITVERSION_REV%.%GITVERSION_SHA1%]

rem Build Mega-Glest in release mode
ECHO --------------------------------
Echo Building MegaGlest using Visual Studio 2015...

set CL=/MP
IF EXIST "steamshim_parent.obj" del steamshim_parent.obj
IF EXIST "megaglest_shim.exe" del megaglest_shim.exe

rem set VisualStudioVersion=11.0
set msBuildMaxCPU=
SET BuildInParallel=false
if %NUMBER_OF_PROCESSORS% GTR 2 (
                SET NumberOfProcessesToUseForBuild=2
                SET BuildInParallel=true
				SET msBuildMaxCPU=/maxcpucount)

ECHO Found CPU Count [%NUMBER_OF_PROCESSORS%]
SET MSBUILD_PATH_MG_x64="%ProgramFiles(x86)%\MSBuild\Microsoft.Cpp\v4.0\V140\\"

echo Doing a FULL REBUILD...
nmake /f Makefile.win STEAMWORKS=..\..\..\steamworks_sdk\sdk

ECHO ... End.
rem pause execution so we can see the output before the batch file exits
if not "%1" == "nopause" pause
