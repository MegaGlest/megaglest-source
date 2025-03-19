@echo off

rem if not exist ..\..\data\glest_game\dsound.dll copy dsound.dll ..\..\data\glest_game\
rem if not exist ..\..\data\glest_game\xerces-c_3_0.dll copy ..\..\source\windows_deps\bin\xerces-c_3_0.dll ..\..\data\glest_game\
copy ..\..\source\windows_deps_2012\lib\openal64.dll .\
rem copy ..\..\source\windows_deps\lib\libeay32.dll ..\..\data\glest_game\
rem copy ..\..\source\windows_deps\lib\ssleay32.dll ..\..\data\glest_game\

if not "%1" == "nopause" pause
