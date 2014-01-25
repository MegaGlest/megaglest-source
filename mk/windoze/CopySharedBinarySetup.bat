@echo off

copy ..\shared\*.ini .\
rem copy glest.ini ..\..\data\glest_game\
if not "%1" == "nopause" pause
