// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "platform_util.h"

#include <io.h>
#include <dbghelp.h>

#include <cassert>
#include <algorithm>
#include <string.h>
#include "SDL_syswm.h"
#include <iostream>

#include "leak_dumper.h"

using namespace Shared::Util;
using namespace std;

namespace Shared { namespace Platform {

// =====================================================
//	class PlatformExceptionHandler
// =====================================================

PlatformExceptionHandler *PlatformExceptionHandler::thisPointer= NULL;

// Constructs object and convert lpaszString to Unicode
LPWSTR Ansi2WideString(LPCSTR lpaszString) {
	LPWSTR lpwszString(NULL);
	int nLen = ::lstrlenA(lpaszString) + 1;
	lpwszString = new WCHAR[nLen];
	if (lpwszString == NULL) {
		return lpwszString;
	}

	memset(lpwszString, 0, nLen * sizeof(WCHAR));

	if (::MultiByteToWideChar(CP_ACP, 0, lpaszString, nLen, lpwszString, nLen) == 0) {
		// Conversation failed
		return lpwszString;
	}

	return lpwszString;
}

// Convert a wide Unicode string to an UTF8 string
std::string utf8_encode(const std::wstring wstr) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo( size_needed, 0 );
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	replaceAll(strTo, "/", "\\");
	replaceAll(strTo, "\\\\", "\\");
	updatePathClimbingParts(strTo);
    return strTo;
}

// Convert an UTF8 string to a wide Unicode String
std::wstring utf8_decode(const std::string str) {
	string friendly_path = str;
	replaceAll(friendly_path, "/", "\\");
	replaceAll(friendly_path, "\\\\", "\\");
	updatePathClimbingParts(friendly_path);
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &friendly_path[0], (int)friendly_path.size(), NULL, 0);
    std::wstring wstrTo( size_needed, 0 );
    MultiByteToWideChar(CP_UTF8, 0, &friendly_path[0], (int)friendly_path.size(), &wstrTo[0], size_needed);
    return wstrTo;
}


LONG WINAPI PlatformExceptionHandler::handler(LPEXCEPTION_POINTERS pointers){

	//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	LPWSTR wstr = Ansi2WideString(thisPointer->dumpFileName.c_str());	
	HANDLE hFile = CreateFile(
		wstr,
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		0);
	delete [] wstr;

	//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	MINIDUMP_EXCEPTION_INFORMATION lExceptionInformation;

	lExceptionInformation.ThreadId= GetCurrentThreadId();
	lExceptionInformation.ExceptionPointers= pointers;
	lExceptionInformation.ClientPointers= false;

#if defined(__WIN32__) && !defined(__GNUC__)
	MiniDumpWriteDump(
		GetCurrentProcess(),
		GetCurrentProcessId(),
		hFile,
		MiniDumpNormal,
		&lExceptionInformation,
		NULL,
		NULL );

	//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	thisPointer->handle();
#endif
	//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	return EXCEPTION_EXECUTE_HANDLER;
}

void PlatformExceptionHandler::install(string dumpFileName){
	thisPointer= this;
	this->dumpFileName= dumpFileName;
	SetUnhandledExceptionFilter(handler);
}

// =====================================================
//	class Misc
// =====================================================

//void getFullscreenVideoInfo(int &colorBits,int &screenWidth,int &screenHeight) {
//    // Get the current video hardware information
//    //const SDL_VideoInfo* vidInfo = SDL_GetVideoInfo();
//    //colorBits      = vidInfo->vfmt->BitsPerPixel;
//    //screenWidth    = vidInfo->current_w;
//
///*
//	//screenHeight   = vidInfo->current_h;
//    int cx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
//    // height
//    int cy = GetSystemMetrics(SM_CYVIRTUALSCREEN);
//
//	printf("cx = %d, cy = %d\n",cx,cy);
//
//	if(cx > screenWidth) {
//		screenWidth  = cx;
//		screenHeight = cy;
//	}
//*/
//	int iMaxWidth	= -1;
//	int iMaxHeight	= -1;
//	int iMaxBits	= -1;
//
//	DEVMODE devMode;
//	for (int i=0; EnumDisplaySettings(NULL, i, &devMode) ;i++){
//
//		//printf("devMode.dmPelsWidth = %d, devMode.dmPelsHeight = %d, devMode.dmBitsPerPel = %d\n",devMode.dmPelsWidth,devMode.dmPelsHeight,devMode.dmBitsPerPel);
//
//		if(devMode.dmPelsWidth > iMaxWidth) {
//			iMaxWidth  = devMode.dmPelsWidth;
//			iMaxHeight = devMode.dmPelsHeight;
//			iMaxBits   = devMode.dmBitsPerPel;
//			//devMode.dmDisplayFrequency=refreshFrequency;
//		}
//	}
//	if(iMaxWidth > 0) {
//		colorBits      = iMaxBits;
//		screenWidth    = iMaxWidth;
//		screenHeight   = iMaxHeight;
//	}
//}
//
//bool changeVideoMode(int resW, int resH, int colorBits, int refreshFrequency){
//	DEVMODE devMode;
//
//	for (int i=0; EnumDisplaySettings(NULL, i, &devMode) ;i++){
//		if (devMode.dmPelsWidth== resW &&
//			devMode.dmPelsHeight== resH &&
//			devMode.dmBitsPerPel== colorBits){
//
//			devMode.dmDisplayFrequency=refreshFrequency;
//
//			LONG result= ChangeDisplaySettings(&devMode, 0);
//			if(result == DISP_CHANGE_SUCCESSFUL){
//				return true;
//			}
//			else{
//				return false;
//			}
//		}
//	}
//
//	return false;
//}
//
//void restoreVideoMode(bool exitingApp) {
//	int dispChangeErr= ChangeDisplaySettings(NULL, 0);
//	assert(dispChangeErr==DISP_CHANGE_SUCCESSFUL);
//}

void message(string message){
	std::cerr << "******************************************************\n";
	std::cerr << "    " << message << "\n";
	std::cerr << "******************************************************\n";

	LPWSTR wstr = Ansi2WideString(message.c_str());
	MessageBox(NULL, wstr, L"Message", MB_OK);
	delete [] wstr;
}

bool ask(string message){
	LPWSTR wstr = Ansi2WideString(message.c_str());	
	bool result = MessageBox(NULL, wstr, L"Confirmation", MB_YESNO)==IDYES;
	delete [] wstr;
	return result;
}

void exceptionMessage(const exception &excp){
	string message, title;
	showCursor(true);

	message+= "ERROR(S):\n\n";
	message+= excp.what();

	title= "Error: Unhandled Exception";
	printf("Error detected with text: %s\n",message.c_str());

	LPWSTR wstr = Ansi2WideString(message.c_str());	
	LPWSTR wstr1 = Ansi2WideString(title.c_str());	
	MessageBox(NULL, wstr, wstr1, MB_ICONSTOP | MB_OK | MB_TASKMODAL);
	delete [] wstr;
	delete [] wstr1;
}

//int getScreenW(){
//	return GetSystemMetrics(SM_CXSCREEN);
//}

//int getScreenH(){
//	return GetSystemMetrics(SM_CYSCREEN);
//}

// This lets us set the SDL Window Icon in Windows
// since its a console application
HICON icon;

void init_win32() {
	HINSTANCE handle = ::GetModuleHandle(NULL);
	icon = ::LoadIcon(handle, L"IDI_ICON1");

	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version)
	if (SDL_GetWMInfo(&wminfo) != 1)
	{
		// error: wrong SDL version
	}

	HWND hwnd = wminfo.window;

	::SetClassLong(hwnd, GCL_HICON, (LONG) icon);
}
void done_win32() {
	::DestroyIcon(icon);
}

}}//end namespace
