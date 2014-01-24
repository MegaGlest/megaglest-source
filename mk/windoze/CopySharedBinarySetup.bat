@echo off

copy ..\shared\*.* ..\..\data\glest_game\
copy glest.ini ..\..\data\glest_game\
if not "%1" == "nopause" pause
