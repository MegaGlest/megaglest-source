@echo off

if "%1." == "." goto NOTSET
if NOT EXIST "%1" goto NOTFOUND

..\..\source\windows_deps\google-breakpad\trunk\src\minidump_stackwalk.exe "%1" .\windows_symbols
goto END

:NOTSET
echo You need to pass the full path to the DMP file to process (usually in %AppData%\megaglest) as first argument on the command line.
goto END

:NOTFOUND
echo File not found [%1]
goto END

:END
