@echo

if "%1." == "." goto NOTSET
if NOT EXIST "%AppData%\megaglest\%1" goto NOTFOUND

..\..\source\windows_deps\google-breakpad\trunk\src\minidump_stackwalk.exe "%AppData%\megaglest\%1" .\windows_symbols
goto END

:NOTSET
echo "You need to pass the name of the DMP file (in %AppData%) to process as the first argument."
goto END

:NOTFOUND
echo "File not found [%AppData%\megaglest\%1]"
goto END

:END
