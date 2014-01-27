@echo off
rem
rem Upload Coverity s
rem Requires:
rem - curl.exe, built with SSL support: http://curl.haxx.se/download.html
rem - wget.exe (should get installed automatically during a build)
rem - 7z.exe (should get installed automatically during a build)
rem - Coverity Scan Build Tool installed and its install path configured in .coverity-scan
rem

rem Change into this directory
cd /d "%~dp0"

rem Project name (case sensitive)
set PROJECT=MegaGlest

rem read in config settings
if not exist ".coverity-scan" (
        echo -----------------------------------------
        echo **Missing Config** To use this script please create a config file named [%CD%\.coverity-scan]
        echo Containing: TOKEN=x , EMAIL=x , COVERITY_ANALYSIS_ROOT=x , NUMCORES=x
        goto END
)

setlocal disabledelayedexpansion
FOR /F "tokens=1* delims==" %%i IN (.coverity-scan) DO set "prop_%%i=%%j"

rem Coverity Scan project token as listed on the Coverity Scan project page
set TOKEN=%prop_TOKEN%

rem E-Mail address of registered Coverity Scan user with project access
set EMAIL=%prop_EMAIL%

set COVERITY_ANALYSIS_ROOT=%prop_COVERITY_ANALYSIS_ROOT%

echo TOKEN [%TOKEN%] EMAIL [%EMAIL%] COVERITY_ANALYSIS_ROOT [%COVERITY_ANALYSIS_ROOT%]
rem pause

rem Description of this build (can be any string)
set DESCRIPTION=Windows-32_%COMPUTERNAME%

rem Where to store the data gathered by the Coverity Scan Build Tool
set BUILDTOOL=cov-int

rem ------------------------------------------------------------------------------

set GITVERSION_SHA1=.
set GITVERSION_REV=.
set GET_GIT_SHA1="git log -1 --format=%%h"
for /f "delims=" %%a in ('%GET_GIT_SHA1%') do @set GITVERSION_SHA1=%%a
for /f "delims=" %%a in ('git rev-list HEAD --count') do @set GITVERSION_REV=%%a

set VERSION=%GITVERSION_REV%.%GITVERSION_SHA1%

set FILENAME=%PROJECT%_%DESCRIPTION%_%VERSION%

rem Untested! Requires modification.
rem wget.exe --no-check-certificate https://scan.coverity.com/download/win-32 --post-data "token=%TOKEN%&project=%PROJECT%" -O %TEMP%\coverity_tool.zip
rem 7z.exe x %TEMP%\coverity_tool.zip
rem set PATH=%PATH%;C:\build\megaglest-source\mk\windoze\cov-analysis-win32-6.6.1\bin\

if "%MG_COV_PATH_SET%." == "." set PATH=%PATH%;%COVERITY_ANALYSIS_ROOT%\bin\
set MG_COV_PATH_SET=TRUE

cov-build --dir %BUILDTOOL% build-mg-2010.bat nopause rebuild
if ERRORLEVEL 1 GOTO ERROR

7z.exe a %FILENAME%.tar %BUILDTOOL%\
7z.exe a %FILENAME%.tar.gz %FILENAME%.tar
del /Q /F %FILENAME%.tar
dir %FILENAME%.tar.gz

echo **About to run: curl.exe --progress-bar --insecure --form "project=%PROJECT%" --form "token=%TOKEN%" --form "email=%EMAIL%" --form "version=%VERSION%" --form "description=%DESCRIPTION%" --form "file=@%FILENAME%.tar.gz" https://scan5.coverity.com/cgi-bin/upload.py
rem pause
rem echo Running curl
curl.exe --progress-bar --insecure --form "project=%PROJECT%" --form "token=%TOKEN%" --form "email=%EMAIL%" --form "version=%VERSION%" --form "description=%DESCRIPTION%" --form "file=@%FILENAME%.tar.gz" https://scan5.coverity.com/cgi-bin/upload.py
if ERRORLEVEL 1 GOTO ERROR
GOTO CLEANUP

:CLEANUP
del /Q /F %FILENAME%.tar.gz
rd /Q /S %BUILDTOOL%\
GOTO END

:ERROR
echo An error occurred.

:END
