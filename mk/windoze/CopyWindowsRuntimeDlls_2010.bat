@echo off

rem if not exist ..\..\data\glest_game\dsound.dll copy dsound.dll ..\..\data\glest_game\
rem if not exist ..\..\data\glest_game\xerces-c_3_0.dll copy ..\..\source\windows_deps\bin\xerces-c_3_0.dll ..\..\data\glest_game\
copy ..\..\source\windows_deps\lib\openal32.dll .\
rem copy ..\..\source\windows_deps\lib\libeay32.dll ..\..\data\glest_game\
rem copy ..\..\source\windows_deps\lib\ssleay32.dll ..\..\data\glest_game\

if not exist libvlc.dll copy ..\..\source\windows_deps\lib\libvlc.dll .\
if not exist libvlccore.dll copy ..\..\source\windows_deps\lib\libvlccore.dll .\
if not exist plugins\nul xcopy ..\..\source\windows_deps\vlc-2.1.2\plugins .\plugins\ /s
if not exist lua\nul xcopy ..\..\source\windows_deps\vlc-2.1.2\lua .\lua\ /s

if not "%1" == "nopause" pause
