//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "math_wrapper.h"
#include "main.h"

#include <string>
#include <cstdlib>

#include "game.h"
#include "main_menu.h"
#include "program.h"
#include "config.h"
#include "metrics.h"
#include "game_util.h"
#include "platform_util.h"
#include "platform_main.h"
#include "network_interface.h"
#include "ImageReaders.h"
#include "renderer.h"
#include "simple_threads.h"
#include <memory>
#include "font.h"
#include <curl/curl.h>
#include "menu_state_masterserver.h"
#include "checksum.h"
#include <algorithm>
#include "sound_renderer.h"
#include "font_gl.h"
#include "FileReader.h"
#include "cache_manager.h"
#include <iterator>
#include "core_data.h"
#include "font_text.h"
//#include "FileReader.h"
//#include "JPGReader.h"
//#include "sound.h"
//#include "unicode/uclean.h"
#include <locale.h>

// For gcc backtrace on crash!
#if defined(__GNUC__) && !defined(__MINGW32__) && !defined(__FreeBSD__) && !defined(BSD)

//#include <mcheck.h>

#include <execinfo.h>
#include <cxxabi.h>
#include <signal.h>
#endif

#ifdef WIN32
#if defined(__WIN32__) && !defined(__GNUC__)
#include <eh.h>
#endif
#include <dbghelp.h>
#endif

#include <stdlib.h>

#include "leak_dumper.h"

#ifndef WIN32
  #include <poll.h>

  #define stricmp strcasecmp
  #define strnicmp strncasecmp
  #define _strnicmp strncasecmp
#endif

#ifdef WIN32
#ifndef _DEBUG
#ifndef __GNUC__

#define WIN32_STACK_TRACE

#endif
#endif
#endif

using namespace std;
using namespace Shared::Platform;
using namespace Shared::Util;
using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;
using namespace	Shared::Xml;
using namespace Shared;

namespace Glest{ namespace Game{

bool disableheadless_console = false;
bool disableBacktrace = false;
bool gameInitialized = false;
//static string application_binary="";
static string application_binary="";
static string mg_app_name = "";
static string mailStringSupport = "";
static bool sdl_quitCalled = false;
static bool isMasterServerModeEnabled = false;

FileCRCPreCacheThread *preCacheThread=NULL;

string runtimeErrorMsg = "";

void cleanupCRCThread() {
	if(preCacheThread != NULL) {
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		time_t elapsed = time(NULL);
		preCacheThread->signalQuit();
		for(;preCacheThread->canShutdown(false) == false &&
			difftime(time(NULL),elapsed) <= 45;) {
			//sleep(150);
		}
		if(preCacheThread->canShutdown(false)) {
			if(preCacheThread->shutdownAndWait() == true) {
				delete preCacheThread;
			}
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		}
		else {
			if(preCacheThread->shutdownAndWait() == true) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				delete preCacheThread;

				//printf("Stopping broadcast thread [%p] - C\n",broadCastThread);
			}
		}
		preCacheThread = NULL;
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

static void cleanupProcessObjects() {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(isMasterServerModeEnabled == false) {
		showCursor(true);
		restoreVideoMode(true);
	}

    cleanupCRCThread();
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    if(Renderer::isEnded() == false) Renderer::getInstance().end();

    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	SystemFlags::Close();
	SystemFlags::SHUTDOWN_PROGRAM_MODE=true;

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("start running threads = %lu\n",Thread::getThreadList().size());
	time_t elapsed = time(NULL);
    for(;Thread::getThreadList().size() > 0 &&
    	 difftime(time(NULL),elapsed) <= 10;) {
    	//sleep(0);
    }
    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("end running threads = %lu\n",Thread::getThreadList().size());

	std::map<int,Texture2D *> &crcPlayerTextureCache = CacheManager::getCachedItem< std::map<int,Texture2D *> >(GameConstants::playerTextureCacheLookupKey);
	//deleteMapValues(crcPlayerTextureCache.begin(),crcPlayerTextureCache.end());
	crcPlayerTextureCache.clear();

	std::map<string,Texture2D *> &crcFactionPreviewTextureCache = CacheManager::getCachedItem< std::map<string,Texture2D *> >(GameConstants::factionPreviewTextureCacheLookupKey);
	//deleteMapValues(crcFactionPreviewTextureCache.begin(),crcFactionPreviewTextureCache.end());
	crcFactionPreviewTextureCache.clear();

	std::map<string, vector<FileReader<Pixmap2D> const * >* > &list2d = FileReader<Pixmap2D>::getFileReadersMap();
	//printf("list2d = %lu\n",list2d.size());
	deleteMapValues(list2d.begin(),list2d.end());
	std::map<string, vector<FileReader<Pixmap3D> const * >* > &list3d = FileReader<Pixmap3D>::getFileReadersMap();
	//printf("list3d = %lu\n",list3d.size());
	deleteMapValues(list3d.begin(),list3d.end());

	XmlIo::getInstance().cleanup();

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);


	SystemFlags::globalCleanupHTTP();
	CacheManager::cleanupMutexes();
}

#if defined(WIN32) && !defined(_DEBUG) && !defined(__GNUC__)
void fatal(const char *s, ...)    // failure exit
{
    static int errors = 0;
    errors++;

	if(errors <= 5) { // print up to two extra recursive errors
        defvformatstring(msg,s,s);
		string errText = string(msg) + " [" + runtimeErrorMsg + "]";
        //puts(msg);
	    string sErr = string(mg_app_name) + " fatal error";
		SystemFlags::OutputDebug(SystemFlags::debugError,"%s\n",errText.c_str());
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s\n",errText.c_str());

		if(errors <= 1) { // avoid recursion
            if(SDL_WasInit(SDL_INIT_VIDEO)) {
                SDL_ShowCursor(1);
                SDL_WM_GrabInput(SDL_GRAB_OFF);
                SDL_SetGamma(1, 1, 1);
            }
            #ifdef WIN32
				LPWSTR wstr = Ansi2WideString(errText.c_str());
				LPWSTR wstr1 = Ansi2WideString(sErr.c_str());

                MessageBox(NULL, wstr, wstr1, MB_OK|MB_SYSTEMMODAL);

				delete [] wstr;
				delete [] wstr1;
            #endif
            //SDL_Quit();
        }
    }

    // Now try to shutdown threads if possible
	Program *program = Program::getInstance();
    delete program;
    program = NULL;
    // END

    if(sdl_quitCalled == false) {
    	sdl_quitCalled = true;
    	SDL_Quit();
    }
    exit(EXIT_FAILURE);
}

void stackdumper(unsigned int type, EXCEPTION_POINTERS *ep) {
    if(!ep) fatal("unknown type");
    EXCEPTION_RECORD *er = ep->ExceptionRecord;
    CONTEXT *context = ep->ContextRecord;
    stringType out, t;
    formatstring(out)("%s Exception: 0x%x [0x%x]\n\n", mg_app_name, er->ExceptionCode, er->ExceptionCode==EXCEPTION_ACCESS_VIOLATION ? er->ExceptionInformation[1] : -1);
    STACKFRAME sf = {{context->Eip, 0, AddrModeFlat}, {}, {context->Ebp, 0, AddrModeFlat}, {context->Esp, 0, AddrModeFlat}, 0};
    SymInitialize(GetCurrentProcess(), NULL, TRUE);

    while(::StackWalk(IMAGE_FILE_MACHINE_I386, GetCurrentProcess(), GetCurrentThread(), &sf, context, NULL, ::SymFunctionTableAccess, ::SymGetModuleBase, NULL)) {
        struct { IMAGEHLP_SYMBOL sym; stringType n; }
		si = { { sizeof( IMAGEHLP_SYMBOL ), 0, 0, 0, sizeof(stringType) } };
        IMAGEHLP_LINE li = { sizeof( IMAGEHLP_LINE ) };
        DWORD off=0;
		DWORD dwDisp=0;
        if( SymGetSymFromAddr(GetCurrentProcess(), (DWORD)sf.AddrPC.Offset, &off, &si.sym) &&
			SymGetLineFromAddr(GetCurrentProcess(), (DWORD)sf.AddrPC.Offset, &dwDisp, &li)) {
            char *del = strrchr(li.FileName, '\\');
            formatstring(t)("%s - %s [%d]\n", si.sym.Name, del ? del + 1 : li.FileName, li.LineNumber+dwDisp);
            concatstring(out, t);
        }
    }
    fatal(out);
}
#endif

// =====================================================
// 	class ExceptionHandler
// =====================================================

class ExceptionHandler: public PlatformExceptionHandler{
public:
	virtual void handle() {
		string msg = "#1 An error occurred and " + string(mg_app_name) + " will close.\nPlease report this bug to: " + string(mailString);
#ifdef WIN32
		msg += ", attaching the generated " + getCrashDumpFileName()+ " file.";
#endif
		SystemFlags::OutputDebug(SystemFlags::debugError,"%s\n",msg.c_str());
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s\n",msg.c_str());

        Program *program = Program::getInstance();
        if(program && gameInitialized == true) {
            program->showMessage(msg.c_str());
        }

        message(msg.c_str());
	}

#if defined(__GNUC__) && !defined(__FreeBSD__) && !defined(BSD)
    static int getFileAndLine(void *address, char *file, size_t flen) {
        int line=-1;
        const int maxbufSize = 1024;
        static char buf[maxbufSize+1]="";
        //char *p=NULL;

        // prepare command to be executed
        // our program need to be passed after the -e parameter
        //sprintf (buf, "/usr/bin/addr2line -C -e ./a.out -f -i %lx", addr);
        sprintf(buf, "addr2line -C -e %s -f -i %p",application_binary.c_str(),address);

        FILE* f = popen (buf, "r");
        if (f == NULL) {
            perror (buf);
            return 0;
        }

        // get function name
        char *ret = fgets (buf, maxbufSize, f);
        if(ret == NULL) {
        	pclose(f);
        	return 0;
        }

        // get file and line
        ret = fgets (buf, maxbufSize, f);
        if(ret == NULL) {
        	pclose(f);
        	return 0;
        }

        if(strlen(buf) > 0 && buf[0] != '?') {
            //int l;
            char *p = buf;

            // file name is until ':'
            while(*p != 0 && *p != ':') {
                p++;
            }

            *p++ = 0;
            // after file name follows line number
            strcpy (file , buf);
            sscanf (p,"%d", &line);
        }
        else {
            strcpy (file,"unknown");
            line = 0;
        }
        pclose(f);

        return line;
    }
#endif

    static void logError(const char *msg, bool confirmToConsole) {
		string errorLogFile = "error.log";
		if(getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) != "") {
			errorLogFile = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) + errorLogFile;
		}
		else {
	        string userData = Config::getInstance().getString("UserData_Root","");
	        if(userData != "") {
	        	endPathWithSlash(userData);
	        }
	        errorLogFile = userData + errorLogFile;
		}

		//printf("Attempting to write error to file [%s]\n",errorLogFile.c_str());

#if defined(WIN32) && !defined(__MINGW32__)
		FILE *fp = _wfopen(utf8_decode(errorLogFile).c_str(), L"w");
		std::ofstream logFile(fp);
#else
		std::ofstream logFile;
		logFile.open(errorLogFile.c_str(), ios_base::out | ios_base::trunc);
#endif

		if(logFile.is_open() == true) {
			time_t curtime = time (NULL);
			struct tm *loctime = localtime (&curtime);
			char szBuf2[100]="";
			strftime(szBuf2,100,"%Y-%m-%d %H:%M:%S",loctime);

			logFile << "[" << szBuf2 << "] Runtime Error information:"  << std::endl;
			logFile << "======================================================"  << std::endl;
			logFile << (msg != NULL ? msg : "null") << std::endl;
			logFile.close();

#if defined(WIN32) && !defined(__MINGW32__)
			if(fp) {
				fclose(fp);
			}
#endif
			if(confirmToConsole == true) {
				printf("Error saved to logfile [%s]\n",errorLogFile.c_str());
				fflush(stdout);
			}
		}
		else {
			if(confirmToConsole == true) {
				printf("COULD NOT SAVE TO ERROR logfile [%s]\n",errorLogFile.c_str());
				fflush(stdout);
			}
		}
    }

	static void handleRuntimeError(const char *msg) {
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		static bool inErrorNow = false;
		if(inErrorNow == true) {
			printf("\n** Already in error handler, msg [%s]\n",msg);
			fflush(stdout);
			abort();
			return;
		}
		inErrorNow = true;

		logError(msg,true);

		Program *program = Program::getInstance();

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] program = %p gameInitialized = %d msg [%s]\n",__FILE__,__FUNCTION__,__LINE__,program,gameInitialized,msg);
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] [%s] gameInitialized = %d, program = %p\n",__FILE__,__FUNCTION__,__LINE__,msg,gameInitialized,program);
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] [%s] gameInitialized = %d, program = %p\n",__FILE__,__FUNCTION__,__LINE__,msg,gameInitialized,program);

        string errMsg = (msg != NULL ? msg : "null");

        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

        #if defined(__GNUC__) && !defined(__MINGW32__) && !defined(__FreeBSD__) && !defined(BSD)
        if(disableBacktrace == false && sdl_quitCalled == false) {
        errMsg += "\nStack Trace:\n";
        //errMsg += "To find line #'s use:\n";
        //errMsg += "readelf --debug-dump=decodedline %s | egrep 0xaddress-of-stack\n";

        const size_t max_depth = 15;
        void *stack_addrs[max_depth];
        size_t stack_depth = backtrace(stack_addrs, max_depth);
        char **stack_strings = backtrace_symbols(stack_addrs, stack_depth);
        //for (size_t i = 1; i < stack_depth; i++) {
        //    errMsg += string(stack_strings[i]) + "\n";
        //}

        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

        char szBuf[4096]="";
        for(size_t i = 1; i < stack_depth; i++) {
            //const unsigned int stackIndex = i-1;

            //printf("In [%s::%s Line: %d] [%s] gameInitialized = %d, i = %d, stack_depth = %d\n",__FILE__,__FUNCTION__,__LINE__,msg,gameInitialized,i,stack_depth);

            void *lineAddress = stack_addrs[i]; //getStackAddress(stackIndex);

            //printf("In [%s::%s Line: %d] [%s] gameInitialized = %d, i = %d, stack_depth = %d\n",__FILE__,__FUNCTION__,__LINE__,msg,gameInitialized,i,stack_depth);

            size_t sz = 1024; // just a guess, template names will go much wider
            char *function = static_cast<char *>(malloc(sz));
            char *begin = 0;
            char *end = 0;

            // find the parentheses and address offset surrounding the mangled name
            for (char *j = stack_strings[i]; *j; ++j) {
                if (*j == '(') {
                    begin = j;
                }
                else if (*j == '+') {
                    end = j;
                }
            }
            if (begin && end) {
                *begin++ = '\0';
                *end = '\0';
                // found our mangled name, now in [begin, end)

                int status;
                char *ret = abi::__cxa_demangle(begin, function, &sz, &status);
                if (ret) {
                    // return value may be a realloc() of the input
                    function = ret;
                }
                else {
                    // demangling failed, just pretend it's a C function with no args
                    strncpy(function, begin, sz);
                    strncat(function, "()", sz);
                    function[sz-1] = '\0';
                }
                //fprintf(out, "    %s:%s\n", stack.strings[i], function);

                sprintf(szBuf,"%s:%s address [%p]",stack_strings[i],function,lineAddress);
            }
            else {
                // didn't find the mangled name, just print the whole line
                //fprintf(out, "    %s\n", stack.strings[i]);
                sprintf(szBuf,"%s address [%p]",stack_strings[i],lineAddress);
            }

            errMsg += string(szBuf);
            char file[4096]="";
            int line = getFileAndLine(lineAddress, file, 4096);
            if(line >= 0) {
                errMsg += " line: " + intToStr(line);
            }
            errMsg += "\n";

            free(function);
        }

        free(stack_strings); // malloc()ed by backtrace_symbols
        }
        #endif

        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

        logError(errMsg.c_str(),false);

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] [%s]\n",__FILE__,__FUNCTION__,__LINE__,errMsg.c_str());
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] [%s]\n",__FILE__,__FUNCTION__,__LINE__,errMsg.c_str());

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//abort();

        if(program && gameInitialized == true) {
			//printf("\nprogram->getState() [%p]\n",program->getState());
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			if(program->getState() != NULL) {
				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
                program->showMessage(errMsg.c_str());
                if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
                for(;isMasterServerModeEnabled == false && program->isMessageShowing();) {
                    //program->getState()->render();
                    Window::handleEvent();
                    program->loop();

                    //printf("\nhandle error #1\n");
                }
			}
			else {
				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
                program->showMessage(errMsg.c_str());
                if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
                for(;isMasterServerModeEnabled == false && program->isMessageShowing();) {
                    //program->renderProgramMsgBox();
                    Window::handleEvent();
                    program->loop();

                    //printf("\nhandle error #2\n");
                }
			}
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
        }
        else {
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

            string err = "#2 An error occurred and ";
            if(sdl_quitCalled == false) {
            	err += mg_app_name;
            }
            err += " will close.\nError msg = [" + errMsg + "]\n\nPlease report this bug to ";
            if(sdl_quitCalled == false) {
            	err += mailStringSupport;
            }
#ifdef WIN32
            err += string(", attaching the generated ") + getCrashDumpFileName() + string(" file.");
#endif
            if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			message(err);
        }

        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

        // Now try to shutdown threads if possible
        delete program;
        program = NULL;
        // END

        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

#ifdef WIN32

        if(isMasterServerModeEnabled == false) {
        	showCursor(true);
        	restoreVideoMode(true);
        }

		runtimeErrorMsg = errMsg;
		inErrorNow = false;
		throw runtimeErrorMsg;
#endif
		//printf("In [%s::%s Line: %d] [%s] gameInitialized = %d\n",__FILE__,__FUNCTION__,__LINE__,msg,gameInitialized);

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		cleanupProcessObjects();

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	    if(sdl_quitCalled == false) {
	    	sdl_quitCalled = true;
	    	SDL_Quit();
	    }

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		inErrorNow = false;

        abort();
	}

	static int DisplayMessage(const char *msg, bool exitApp) {
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] msg [%s] exitApp = %d\n",__FILE__,__FUNCTION__,__LINE__,msg,exitApp);
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] msg [%s] exitApp = %d\n",__FILE__,__FUNCTION__,__LINE__,msg,exitApp);

        Program *program = Program::getInstance();

        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] msg [%s] exitApp = %d\n",__FILE__,__FUNCTION__,__LINE__,msg,exitApp);

        if(program && gameInitialized == true) {
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] msg [%s] exitApp = %d\n",__FILE__,__FUNCTION__,__LINE__,msg,exitApp);
            program->showMessage(msg);
        }
        else {
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] msg [%s] exitApp = %d\n",__FILE__,__FUNCTION__,__LINE__,msg,exitApp);
            message(msg);
        }

        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] msg [%s] exitApp = %d\n",__FILE__,__FUNCTION__,__LINE__,msg,exitApp);

        if(exitApp == true) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s\n",msg);

		    // Now try to shutdown threads if possible
			Program *program = Program::getInstance();
		    delete program;
		    program = NULL;
		    // END

		    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] msg [%s] exitApp = %d\n",__FILE__,__FUNCTION__,__LINE__,msg,exitApp);
		    cleanupProcessObjects();

		    if(sdl_quitCalled == false) {
		    	sdl_quitCalled = true;
		    	SDL_Quit();
		    }
		    exit(-1);
        }

        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] msg [%s] exitApp = %d\n",__FILE__,__FUNCTION__,__LINE__,msg,exitApp);
	    return 0;
	}
};

#if defined(__GNUC__)  && !defined(__FreeBSD__) && !defined(BSD)
void handleSIGSEGV(int sig) {
    char szBuf[4096]="";
    sprintf(szBuf, "In [%s::%s Line: %d] Error detected: signal %d:\n",__FILE__,__FUNCTION__,__LINE__, sig);
    printf("%s",szBuf);
    //abort();

    ExceptionHandler::handleRuntimeError(szBuf);
}
#endif


// =====================================================
// 	class MainWindow
// =====================================================

MainWindow::MainWindow(Program *program) : WindowGl() {
	this->program= program;
	this->popupMenu.setEnabled(false);
	this->popupMenu.setVisible(false);
    this->triggerLanguageToggle = false;
    this->triggerLanguage = "";
}

MainWindow::~MainWindow(){
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	delete program;
	program = NULL;
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MainWindow::eventMouseDown(int x, int y, MouseButton mouseButton){
    const Metrics &metrics = Metrics::getInstance();
    int vx = metrics.toVirtualX(x);
    int vy = metrics.toVirtualY(getH() - y);

    if(program == NULL) {
    	throw runtime_error("In [MainWindow::eventMouseDown] ERROR, program == NULL!");
    }

    //printf("eventMouseDown popupMenu.getVisible() = %d\n",popupMenu.getVisible());
	if(popupMenu.getVisible() == true && popupMenu.mouseClick(vx, vy)) {
		std::pair<int,string> result = popupMenu.mouseClickedMenuItem(vx, vy);
		//printf("In popup callback menuItemSelected [%s] menuIndexSelected = %d\n",result.second.c_str(),result.first);

		popupMenu.setEnabled(false);
		popupMenu.setVisible(false);

		//printf("result.first = %d [%s] cancelLanguageSelection = %d\n",result.first,result.second.c_str(),cancelLanguageSelection);

		// Exit game
		if(result.first != cancelLanguageSelection) {
			//toggleLanguage(result.second);
		    this->triggerLanguageToggle = true;
		    this->triggerLanguage = result.second;
		}

		return;
	}
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	switch(mouseButton) {
		case mbLeft:
			program->mouseDownLeft(vx, vy);
			break;
		case mbRight:
			//program->mouseDownRight(vx, vy);
			break;
		case mbCenter:
			//program->mouseDownCenter(vx, vy);
			break;
		default:
			break;
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    ProgramState *programState = program->getState();
    if(programState != NULL) {
		switch(mouseButton) {
			case mbLeft:
				programState->mouseDownLeft(vx, vy);
				break;
			case mbRight:
				programState->mouseDownRight(vx, vy);
				break;
			case mbCenter:
				programState->mouseDownCenter(vx, vy);
				break;
			default:
				break;
		}
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    }
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MainWindow::eventMouseUp(int x, int y, MouseButton mouseButton){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    const Metrics &metrics = Metrics::getInstance();
    int vx = metrics.toVirtualX(x);
    int vy = metrics.toVirtualY(getH() - y);

    if(program == NULL) {
    	throw runtime_error("In [MainWindow::eventMouseUp] ERROR, program == NULL!");
    }

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    ProgramState *programState = program->getState();

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    if(programState != NULL) {
    	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		switch(mouseButton) {
			case mbLeft:
				programState->mouseUpLeft(vx, vy);
				break;
			case mbRight:
				programState->mouseUpRight(vx, vy);
				break;
			case mbCenter:
				programState->mouseUpCenter(vx, vy);
				break;
			default:
				break;
		}
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    }
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MainWindow::eventMouseDoubleClick(int x, int y, MouseButton mouseButton) {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    const Metrics &metrics= Metrics::getInstance();
    int vx = metrics.toVirtualX(x);
    int vy = metrics.toVirtualY(getH() - y);

    if(program == NULL) {
    	throw runtime_error("In [MainWindow::eventMouseDoubleClick] ERROR, program == NULL!");
    }

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    ProgramState *programState = program->getState();

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    if(programState != NULL) {
    	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		switch(mouseButton){
			case mbLeft:
				programState->mouseDoubleClickLeft(vx, vy);
				break;
			case mbRight:
				programState->mouseDoubleClickRight(vx, vy);
				break;
			case mbCenter:
				programState->mouseDoubleClickCenter(vx, vy);
				break;
			default:
				break;
		}

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    }
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MainWindow::eventMouseMove(int x, int y, const MouseState *ms){

    const Metrics &metrics= Metrics::getInstance();
    int vx = metrics.toVirtualX(x);
    int vy = metrics.toVirtualY(getH() - y);

    if(program == NULL) {
    	throw runtime_error("In [MainWindow::eventMouseMove] ERROR, program == NULL!");
    }

    program->eventMouseMove(vx, vy, ms);

    ProgramState *programState = program->getState();
    if(programState != NULL) {
    	programState->mouseMove(vx, vy, ms);
    }
}

void MainWindow::eventMouseWheel(int x, int y, int zDelta) {

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	const Metrics &metrics= Metrics::getInstance();
	int vx = metrics.toVirtualX(x);
	int vy = metrics.toVirtualY(getH() - y);

    if(program == NULL) {
    	throw runtime_error("In [MainWindow::eventMouseMove] ERROR, program == NULL!");
    }

    ProgramState *programState = program->getState();

    if(programState != NULL) {
    	programState->eventMouseWheel(vx, vy, zDelta);
    }

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MainWindow::render() {
	if(popupMenu.getVisible() == true) {
		Renderer &renderer= Renderer::getInstance();
		renderer.renderPopupMenu(&popupMenu);

		//printf("Render lang popup\n");
	}
}

void MainWindow::showLanguages() {
	Lang &lang= Lang::getInstance();
	//PopupMenu popupMenu;
	std::vector<string> menuItems;

	vector<string> langResults;
    string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);

    string userDataPath = getGameCustomCoreDataPath(data_path, "");
	findAll(userDataPath + "data/lang/*.lng", langResults, true, false);
	for(unsigned int i = 0; i < langResults.size(); ++i) {
		string testLanguage = langResults[i];
		menuItems.push_back(testLanguage);
	}

	vector<string> langResults2;
	findAll(data_path + "data/lang/*.lng", langResults2, true);
	if(langResults2.empty() && langResults.empty()) {
        throw runtime_error("There are no lang files");
	}
	for(unsigned int i = 0; i < langResults2.size(); ++i) {
		string testLanguage = langResults2[i];
		if(std::find(menuItems.begin(),menuItems.end(),testLanguage) == menuItems.end()) {
			menuItems.push_back(testLanguage);
		}
	}
	menuItems.push_back(lang.get("Exit"));
	cancelLanguageSelection = menuItems.size()-1;

	popupMenu.setW(100);
	popupMenu.setH(100);
	popupMenu.init(lang.get("GameMenuTitle"),menuItems);
	popupMenu.setEnabled(true);
	popupMenu.setVisible(true);
}

void MainWindow::toggleLanguage(string language) {
	popupMenu.setEnabled(false);
	popupMenu.setVisible(false);
    this->triggerLanguageToggle = false;
    this->triggerLanguage = "";

	Lang &lang= Lang::getInstance();
	string currentLanguage = lang.getLanguage();

	string newLanguageSelected = language;
	if(language == "") {
		newLanguageSelected = currentLanguage;

		vector<string> langResults;
	    string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);

	    string userDataPath = getGameCustomCoreDataPath(data_path, "");
		findAll(userDataPath + "data/lang/*.lng", langResults, true, false);

		vector<string> langResults2;
		findAll(data_path + "data/lang/*.lng", langResults2, true);
		if(langResults2.empty() && langResults.empty()) {
	        throw runtime_error("There are no lang files");
		}
		for(unsigned int i = 0; i < langResults2.size(); ++i) {
			string testLanguage = langResults2[i];
			if(std::find(langResults.begin(),langResults.end(),testLanguage) == langResults.end()) {
				langResults.push_back(testLanguage);
			}
		}

		for(unsigned int i = 0; i < langResults.size(); ++i) {
			string testLanguage = langResults[i];
			if(testLanguage == currentLanguage) {
				if( i+1 < langResults.size()) {
					newLanguageSelected = langResults[i+1];
				}
				else {
					newLanguageSelected = langResults[0];
				}
				break;
			}
		}
	}
	if(newLanguageSelected != currentLanguage) {
		lang.loadStrings(newLanguageSelected);
		program->reloadUI();
		program->consoleAddLine(lang.get("Language") + " " + newLanguageSelected);
	}
}

void MainWindow::eventKeyDown(SDL_KeyboardEvent key) {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key.keysym.sym);

	//SDL_keysym keystate = Window::getKeystate();
	SDL_keysym keystate = key.keysym;

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c][%d]\n",__FILE__,__FUNCTION__,__LINE__,key,key);

    if(program == NULL) {
    	throw runtime_error("In [MainWindow::eventKeyDown] ERROR, program == NULL!");
    }

    if(popupMenu.getVisible() == true && isKeyPressed(SDLK_ESCAPE,key) == true) {
    	this->popupMenu.setEnabled(false);
    	this->popupMenu.setVisible(false);
    	return;
    }

    //{
    //Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
    //printf("----------------------- key [%d] CameraModeLeft [%d]\n",key,configKeys.getCharKey("CameraModeLeft"));
    //}

	program->keyDown(key);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(keystate.mod & (KMOD_LALT | KMOD_RALT)) {
		//if(key == vkReturn) {
		if(isKeyPressed(SDLK_RETURN,key) == true) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] ALT-ENTER pressed\n",__FILE__,__FUNCTION__,__LINE__);

			// This stupidity only required in win32.
			// We reload the textures so that the canvas paints textures properly
#ifdef WIN32
			if(Window::getAllowAltEnterFullscreenToggle() == true) {
				Renderer &renderer= Renderer::getInstance();
				renderer.reinitAll();
			}
#endif

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		}
	}

	if(program != NULL && program->isInSpecialKeyCaptureEvent() == false) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
		//if(key == configKeys.getCharKey("HotKeyShowDebug")) {
		if(isKeyPressed(configKeys.getSDLKey("HotKeyShowDebug"),key) == true) {
			//printf("debug key pressed keystate.mod = %d [%d]\n",keystate.mod,keystate.mod & (KMOD_LALT | KMOD_RALT));

			Renderer &renderer= Renderer::getInstance();
			//if(keystate.mod & (KMOD_LCTRL | KMOD_RCTRL)) {
			if(keystate.mod & (KMOD_LALT | KMOD_RALT)) {
				renderer.cycleShowDebugUILevel();
			}
			else {
				bool showDebugUI = renderer.getShowDebugUI();
				renderer.setShowDebugUI(!showDebugUI);
			}
		}
		else if(keystate.mod & (KMOD_LCTRL | KMOD_RCTRL) &&
				isKeyPressed(configKeys.getSDLKey("SwitchLanguage"),key) == true) {
			if(keystate.mod & (KMOD_LSHIFT | KMOD_RSHIFT)) {
				//toggleLanguage("");
			    this->triggerLanguageToggle = true;
			    this->triggerLanguage = "";
			}
			else {
				showLanguages();
			}
		}
		//else if(key == configKeys.getCharKey("ReloadINI")) {
		else if(isKeyPressed(configKeys.getSDLKey("ReloadINI"),key) == true) {
			Config &config = Config::getInstance();
			config.reload();
		}
		//else if(key == configKeys.getCharKey("Screenshot")) {
		else if(isKeyPressed(configKeys.getSDLKey("Screenshot"),key) == true) {
	        string userData = Config::getInstance().getString("UserData_Root","");
	        if(userData != "") {
        		endPathWithSlash(userData);
	        }

			string path = userData + GameConstants::folder_path_screenshots;
			if(isdir(path.c_str()) == true) {
				Config &config= Config::getInstance();
				string fileFormat = config.getString("ScreenShotFileType","jpg");

				unsigned int queueSize = Renderer::getInstance().getSaveScreenQueueSize();

				for(int i=0; i < 5000; ++i) {
					path = userData + GameConstants::folder_path_screenshots;
					path += string("screen") + intToStr(i + queueSize) + string(".") + fileFormat;
#ifdef WIN32
					FILE *f= _wfopen(utf8_decode(path).c_str(), L"rb");
#else
					FILE *f= fopen(path.c_str(), "rb");
#endif
					if(f == NULL) {
						Lang &lang= Lang::getInstance();
						char szBuf[1024]="";
						if(lang.get("ScreenshotSavedTo").length() > 0 && lang.get("ScreenshotSavedTo")[0] != '?') {
							sprintf(szBuf,lang.get("ScreenshotSavedTo").c_str(),path.c_str());
						}
						else {
							sprintf(szBuf,"Screenshot will be saved to: %s",path.c_str());
						}

						if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] %s\n",__FILE__,__FUNCTION__,__LINE__,szBuf);

						if(Config::getInstance().getBool("DisableScreenshotConsoleText","false") == false) {
							program->consoleAddLine(szBuf);
						}

						Renderer::getInstance().saveScreen(path);
						break;
					}
					else {
						fclose(f);
					}
				}
			}
		}
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MainWindow::eventKeyUp(SDL_KeyboardEvent key) {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key);
    if(program == NULL) {
    	throw runtime_error("In [MainWindow::eventKeyUp] ERROR, program == NULL!");
    }

	program->keyUp(key);
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key);
}

void MainWindow::eventKeyPress(SDL_KeyboardEvent c) {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] [%d]\n",__FILE__,__FUNCTION__,__LINE__,c);
    if(program == NULL) {
    	throw runtime_error("In [MainWindow::eventKeyPress] ERROR, program == NULL!");
    }

	program->keyPress(c);

	if(program != NULL && program->isInSpecialKeyCaptureEvent() == false) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
		//if(c == configKeys.getCharKey("HotKeyToggleOSMouseEnabled")) {
		if(isKeyPressed(configKeys.getSDLKey("HotKeyToggleOSMouseEnabled"),c) == true) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			bool showCursorState = false;
			int state = SDL_ShowCursor(SDL_QUERY);
			if(state == SDL_DISABLE) {
				showCursorState = true;
			}

			showCursor(showCursorState);
			Renderer &renderer= Renderer::getInstance();
			renderer.setNo2DMouseRendering(showCursorState);

			Window::lastShowMouseState = SDL_ShowCursor(SDL_QUERY);
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Window::lastShowMouseState = %d\n",__FILE__,__FUNCTION__,__LINE__,Window::lastShowMouseState);
		}
	}
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] [%d]\n",__FILE__,__FUNCTION__,__LINE__,c);
}

void MainWindow::eventActivate(bool active) {
	if(!active){
		//minimize();
	}
}

void MainWindow::eventResize(SizeState sizeState) {
    if(program == NULL) {
    	throw runtime_error("In [MainWindow::eventResize] ERROR, program == NULL!");
    }

	program->resize(sizeState);
}

void MainWindow::eventClose(){
	delete program;
	program= NULL;
}

void MainWindow::setProgram(Program *program) {
	this->program= program;
}

// =====================================================
// Main
// =====================================================
SystemFlags debugger;

void print_SDL_version(const char *preamble, SDL_version *v) {
	printf("%s %u.%u.%u\n", preamble, v->major, v->minor, v->patch);
}

int setupGameItemPaths(int argc, char** argv, Config *config) {
    // Setup path cache for files and folders used in the game
    std::map<string,string> &pathCache = CacheManager::getCachedItem< std::map<string,string> >(GameConstants::pathCacheLookupKey);

    //GAME_ARG_DATA_PATH
    if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_DATA_PATH]) == true) {
        int foundParamIndIndex = -1;
        hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_DATA_PATH]) + string("="),&foundParamIndIndex);
        if(foundParamIndIndex < 0) {
            hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_DATA_PATH]),&foundParamIndIndex);
        }
        string customPath = argv[foundParamIndIndex];
        vector<string> paramPartTokens;
        Tokenize(customPath,paramPartTokens,"=");
        if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
            string customPathValue = paramPartTokens[1];
            Properties::applyTagsToValue(customPathValue);
            if(customPathValue != "") {
            	endPathWithSlash(customPathValue);
            }
            pathCache[GameConstants::path_data_CacheLookupKey]=customPathValue;
            if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Using custom data path [%s]\n",customPathValue.c_str());
        }
        else {

            printf("\nInvalid path specified on commandline [%s] value [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
            printParameterHelp(argv[0],false);
            return -1;
        }
    }
    else if(config != NULL) {
    	if(config->getString("DataPath","") != "") {
    		string customPathValue = config->getString("DataPath","");
    		if(customPathValue != "") {
    			endPathWithSlash(customPathValue);
    		}
            pathCache[GameConstants::path_data_CacheLookupKey] = config->getString("DataPath","");
            if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Using ini specified data path [%s]\n",config->getString("DataPath","").c_str());
    	}
    }

    //GAME_ARG_INI_PATH
    if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_INI_PATH]) == true) {
        int foundParamIndIndex = -1;
        hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_INI_PATH]) + string("="),&foundParamIndIndex);
        if(foundParamIndIndex < 0) {
            hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_INI_PATH]),&foundParamIndIndex);
        }
        string customPath = argv[foundParamIndIndex];
        vector<string> paramPartTokens;
        Tokenize(customPath,paramPartTokens,"=");
        if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
            string customPathValue = paramPartTokens[1];
            Properties::applyTagsToValue(customPathValue);
            pathCache[GameConstants::path_ini_CacheLookupKey]=customPathValue;
            if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Using custom ini path [%s]\n",customPathValue.c_str());
        }
        else {

            printf("\nInvalid path specified on commandline [%s] value [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
            printParameterHelp(argv[0],false);
            return -1;
        }
    }

    //GAME_ARG_LOG_PATH
    if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_LOG_PATH]) == true) {
        int foundParamIndIndex = -1;
        hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_LOG_PATH]) + string("="),&foundParamIndIndex);
        if(foundParamIndIndex < 0) {
            hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_LOG_PATH]),&foundParamIndIndex);
        }
        string customPath = argv[foundParamIndIndex];
        vector<string> paramPartTokens;
        Tokenize(customPath,paramPartTokens,"=");
        if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
            string customPathValue = paramPartTokens[1];
            Properties::applyTagsToValue(customPathValue);
            pathCache[GameConstants::path_logs_CacheLookupKey]=customPathValue;
            if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Using custom logs path [%s]\n",customPathValue.c_str());
        }
        else {

            printf("\nInvalid path specified on commandline [%s] value [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
            printParameterHelp(argv[0],false);
            return -1;
        }
    }
    else if(config != NULL) {
    	if(config->getString("LogPath","") != "") {
            pathCache[GameConstants::path_logs_CacheLookupKey] = config->getString("LogPath","");
            if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Using ini specified logs path [%s]\n",config->getString("LogPath","").c_str());
    	}
    }

    Text::DEFAULT_FONT_PATH = pathCache[GameConstants::path_data_CacheLookupKey];

    return 0;
}

void setupLogging(Config &config, bool haveSpecialOutputCommandLineOption) {

    SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled      	= config.getBool("DebugMode","false");
    SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled     	= config.getBool("DebugNetwork","false");
    SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled 	= config.getBool("DebugPerformance","false");
    SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled  	= config.getBool("DebugWorldSynch","false");
    SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled  	= config.getBool("DebugUnitCommands","false");
    SystemFlags::getSystemSettingType(SystemFlags::debugPathFinder).enabled  	= config.getBool("DebugPathFinder","false");
    SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled  			= config.getBool("DebugLUA","false");
    SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled  		= config.getBool("DebugSound","false");
    SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled  		= config.getBool("DebugError","true");

    string userData = config.getString("UserData_Root","");
    if(userData != "") {
    	endPathWithSlash(userData);
    }

    string debugLogFile 			= config.getString("DebugLogFile","");
    if(getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) != "") {
        debugLogFile = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) + debugLogFile;
    }
    else {
    	debugLogFile = userData + debugLogFile;
    }

    //printf("debugLogFile [%s]\n",debugLogFile.c_str());

    string debugWorldSynchLogFile 	= config.getString("DebugLogFileWorldSynch","");
    if(debugWorldSynchLogFile == "") {
        debugWorldSynchLogFile = debugLogFile;
    }
    else if(getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) != "") {
        debugWorldSynchLogFile = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) + debugWorldSynchLogFile;
    }
    else {
    	debugWorldSynchLogFile = userData + debugWorldSynchLogFile;
    }

    string debugPerformanceLogFile = config.getString("DebugLogFilePerformance","");
    if(debugPerformanceLogFile == "") {
        debugPerformanceLogFile = debugLogFile;
    }
    else if(getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) != "") {
        debugPerformanceLogFile = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) + debugPerformanceLogFile;
    }
    else {
    	debugPerformanceLogFile = userData + debugPerformanceLogFile;
    }

    string debugNetworkLogFile = config.getString("DebugLogFileNetwork","");
    if(debugNetworkLogFile == "") {
        debugNetworkLogFile = debugLogFile;
    }
    else if(getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) != "") {
        debugNetworkLogFile = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) + debugNetworkLogFile;
    }
    else {
    	debugNetworkLogFile = userData + debugNetworkLogFile;
    }

    string debugUnitCommandsLogFile = config.getString("DebugLogFileUnitCommands","");
    if(debugUnitCommandsLogFile == "") {
        debugUnitCommandsLogFile = debugLogFile;
    }
    else if(getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) != "") {
        debugUnitCommandsLogFile = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) + debugUnitCommandsLogFile;
    }
    else {
    	debugUnitCommandsLogFile = userData + debugUnitCommandsLogFile;
    }

    string debugPathFinderLogFile = config.getString("DebugLogFilePathFinder","");
    if(debugPathFinderLogFile == "") {
    	debugPathFinderLogFile = debugLogFile;
    }
    else if(getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) != "") {
    	debugPathFinderLogFile = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) + debugPathFinderLogFile;
    }
    else {
    	debugPathFinderLogFile = userData + debugPathFinderLogFile;
    }

    string debugLUALogFile = config.getString("DebugLogFileLUA","");
    if(debugLUALogFile == "") {
        debugLUALogFile = debugLogFile;
    }
    else if(getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) != "") {
        debugLUALogFile = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) + debugLUALogFile;
    }
    else {
    	debugLUALogFile = userData + debugLUALogFile;
    }

    string debugSoundLogFile = config.getString("DebugLogFileSound","");
    if(debugSoundLogFile == "") {
        debugSoundLogFile = debugLogFile;
    }
    else if(getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) != "") {
        debugSoundLogFile = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) + debugSoundLogFile;
    }
    else {
    	debugSoundLogFile = userData + debugSoundLogFile;
    }

    string debugErrorLogFile = config.getString("DebugLogFileError","");

    SystemFlags::getSystemSettingType(SystemFlags::debugSystem).debugLogFileName      = debugLogFile;
    SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).debugLogFileName     = debugNetworkLogFile;
    SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).debugLogFileName = debugPerformanceLogFile;
    SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).debugLogFileName  = debugWorldSynchLogFile;
    SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).debugLogFileName = debugUnitCommandsLogFile;
    SystemFlags::getSystemSettingType(SystemFlags::debugPathFinder).debugLogFileName   = debugPathFinderLogFile;
    SystemFlags::getSystemSettingType(SystemFlags::debugLUA).debugLogFileName  		   = debugLUALogFile;
    SystemFlags::getSystemSettingType(SystemFlags::debugSound).debugLogFileName  	   = debugSoundLogFile;
    SystemFlags::getSystemSettingType(SystemFlags::debugError).debugLogFileName  	   = debugErrorLogFile;

    if(haveSpecialOutputCommandLineOption == false) {
        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("--- Startup log settings are ---\ndebugSystem [%d][%s]\ndebugNetwork [%d][%s]\ndebugPerformance [%d][%s]\ndebugWorldSynch [%d][%s]\ndebugUnitCommands[%d][%s]\ndebugPathFinder[%d][%s]\ndebugLUA [%d][%s]\ndebugSound [%d][%s]\ndebugError [%d][%s]\n",
                SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled,
                debugLogFile.c_str(),
                SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled,
                debugNetworkLogFile.c_str(),
                SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled,
                debugPerformanceLogFile.c_str(),
                SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled,
                debugWorldSynchLogFile.c_str(),
                SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled,
                debugUnitCommandsLogFile.c_str(),
                SystemFlags::getSystemSettingType(SystemFlags::debugPathFinder).enabled,
                debugPathFinderLogFile.c_str(),
                SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled,
                debugLUALogFile.c_str(),
                SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled,
                debugSoundLogFile.c_str(),
                SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled,
                debugErrorLogFile.c_str());
    }
}

void runTechValidationForPath(string techPath, string techName,
		const std::vector<string> &filteredFactionList, World &world,
		bool purgeUnusedFiles,bool purgeDuplicateFiles, bool showDuplicateFiles,
		bool svnPurgeFiles,double &purgedMegaBytes) {
	//Config &config = Config::getInstance();
	vector<string> factionsList;
	findDirs(techPath + techName + "/factions/", factionsList, false, false);

	if(factionsList.empty() == false) {
		Checksum checksum;
		set<string> factions;
		for(int j = 0; j < factionsList.size(); ++j) {
			if(	filteredFactionList.empty() == true ||
				std::find(filteredFactionList.begin(),filteredFactionList.end(),factionsList[j]) != filteredFactionList.end()) {
				factions.insert(factionsList[j]);
			}
		}

		printf("\n----------------------------------------------------------------");
		printf("\nChecking techPath [%s] techName [%s] total faction count = %d\n",techPath.c_str(), techName.c_str(),(int)factionsList.size());
		for(int j = 0; j < factionsList.size(); ++j) {
			if(	filteredFactionList.empty() == true ||
				std::find(filteredFactionList.begin(),filteredFactionList.end(),factionsList[j]) != filteredFactionList.end()) {
				printf("Using faction [%s]\n",factionsList[j].c_str());
			}
		}

		if(factions.empty() == false) {
			bool techtree_errors = false;

			std::map<string,vector<pair<string, string> >  > loadedFileList;
			//vector<string> pathList = config.getPathListForType(ptTechs,"");
			vector<string> pathList;
			pathList.push_back(techPath);
			world.loadTech(pathList, techName, factions, &checksum, loadedFileList);

			// Fixup paths with ..
			{
				std::map<string,vector<pair<string, string> > > newLoadedFileList;
				for( std::map<string,vector<pair<string, string> > >::iterator iterMap = loadedFileList.begin();
					iterMap != loadedFileList.end(); ++iterMap) {
					string loadedFile = iterMap->first;

					replaceAll(loadedFile,"//","/");
					replaceAll(loadedFile,"\\\\","\\");
					updatePathClimbingParts(loadedFile);

					if(newLoadedFileList.find(loadedFile) != newLoadedFileList.end()) {
						for(unsigned int xx1 = 0; xx1 < iterMap->second.size(); ++xx1) {
							newLoadedFileList[loadedFile].push_back(iterMap->second[xx1]);
						}
					}
					else {
						newLoadedFileList[loadedFile] = iterMap->second;
					}
				}
				loadedFileList = newLoadedFileList;
			}

			// Validate the faction setup to ensure we don't have any bad associations
			std::vector<std::string> resultErrors = world.validateFactionTypes();
			if(resultErrors.empty() == false) {
				techtree_errors = true;
				// Display the validation errors
				string errorText = "\nErrors were detected:\n=====================\n";
				for(int i = 0; i < resultErrors.size(); ++i) {
					if(i > 0) {
						errorText += "\n";
					}
					errorText = errorText + resultErrors[i];
				}
				errorText += "\n=====================\n";
				//throw runtime_error(errorText);
				printf("%s",errorText.c_str());
			}

			// Validate the faction resource setup to ensure we don't have any bad associations
			printf("\nChecking resources, count = %d\n",world.getTechTree()->getResourceTypeCount());

			for(int i = 0; i < world.getTechTree()->getResourceTypeCount(); ++i) {
				printf("Found techtree resource [%s]\n",world.getTechTree()->getResourceType(i)->getName().c_str());
			}

			resultErrors = world.validateResourceTypes();
			if(resultErrors.empty() == false) {
				techtree_errors = true;
				// Display the validation errors
				string errorText = "\nErrors were detected:\n=====================\n";
				for(int i = 0; i < resultErrors.size(); ++i) {
					if(i > 0) {
						errorText += "\n";
					}
					errorText = errorText + resultErrors[i];
				}
				errorText += "\n=====================\n";
				//throw runtime_error(errorText);
				printf("%s",errorText.c_str());
			}

			// Now check for unused files in the techtree
			std::map<string,vector<pair<string, string> > > foundFileList;
			for(unsigned int i = 0; i < pathList.size(); ++i) {
				string path = pathList[i];
				endPathWithSlash(path);
				path = path + techName + "/";

				vector<string> foundFiles = getFolderTreeContentsListRecursively(path + "*.", "");
				for(unsigned int j = 0; j < foundFiles.size(); ++j) {
					string file = foundFiles[j];
					if(	file.find("loading_screen") != string::npos ||
							file.find("preview_screen") != string::npos ||
							file.find("hud") != string::npos) {
						continue;
					}
					if(file.find("/factions/") != string::npos) {
						bool includeFaction = false;
						for ( set<string>::iterator it = factions.begin(); it != factions.end(); ++it ) {
							string currentFaction = *it;
							if(file.find("/factions/" + currentFaction) != string::npos) {
								includeFaction = true;
								break;
							}
						}
						if(includeFaction == false) {
							continue;
						}
					}

					replaceAll(file,"//","/");
					replaceAll(file,"\\\\","\\");

					foundFileList[file].push_back(make_pair(path,path));
				}
			}

			printf("Found techtree filecount = %lu, used = %lu\n",(unsigned long)foundFileList.size(),(unsigned long)loadedFileList.size());

//                        for( std::map<string,vector<string> >::iterator iterMap = loadedFileList.begin();
//                        	iterMap != loadedFileList.end(); ++iterMap) {
//                        	string foundFile = iterMap->first;
//
//							if(foundFile.find("golem_ack1.wav") != string::npos) {
//								printf("FOUND file [%s]\n",foundFile.c_str());
//							}
//                        }

			int purgeCount = 0;
			bool foundUnusedFile = false;
			for( std::map<string,vector<pair<string, string> > >::iterator iterMap = foundFileList.begin();
				iterMap != foundFileList.end(); ++iterMap) {
				string foundFile = iterMap->first;

				if(loadedFileList.find(foundFile) == loadedFileList.end()) {
					if(foundUnusedFile == false) {
						printf("\nWarning, unused files were detected - START:\n=====================\n");
					}
					foundUnusedFile = true;

					printf("[%s]\n",foundFile.c_str());

					string fileName = extractFileFromDirectoryPath(foundFile);
					if(loadedFileList.find(fileName) != loadedFileList.end()) {
						printf("possible match on [%s] ?\n",loadedFileList.find(fileName)->first.c_str());
					}
					else if(purgeUnusedFiles == true) {
						off_t fileSize = getFileSize(foundFile);
						// convert to MB
						purgedMegaBytes += ((double)fileSize / 1048576.0);
						purgeCount++;

						if(svnPurgeFiles == true) {
							char szBuf[4096]="";
							sprintf(szBuf,"svn delete \"%s\"",foundFile.c_str());
							bool svnOk = executeShellCommand(szBuf,0);
							if(svnOk == false) {
								throw runtime_error("Call to command failed [" + string(szBuf) + "]");
							}
						}
						else {
							removeFile(foundFile);
						}
					}
				}
			}
			if(foundUnusedFile == true) {
				if(purgedMegaBytes > 0) {
					printf("Purged %.2f MB (%d) in files\n",purgedMegaBytes,purgeCount);
				}
				printf("\nWarning, unused files were detected - END:\n");
			}

			if(showDuplicateFiles == true) {
				std::map<int32,vector<string> > mapDuplicateFiles;
				// Now check for duplicate data content
				for(std::map<string,vector<pair<string, string> > >::iterator iterMap = loadedFileList.begin();
					iterMap != loadedFileList.end(); ++iterMap) {
					string fileName = iterMap->first;
					Checksum checksum;
					checksum.addFile(fileName);
					int32 crcValue = checksum.getSum();
	//				if(crcValue == 0) {
	//					char szBuf[4096]="";
	//					sprintf(szBuf,"Error calculating CRC for file [%s]",fileName.c_str());
	//					throw runtime_error(szBuf);
	//				}
	//				else {
	//					printf("** CRC for file [%s] is [%d] and has %d parents\n",fileName.c_str(),crcValue,(int)iterMap->second.size());
	//				}
					mapDuplicateFiles[crcValue].push_back(fileName);
				}

				double duplicateMegaBytesPurged=0;
				int duplicateCountPurged=0;

				double duplicateMegaBytes=0;
				int duplicateCount=0;

				bool foundDuplicates = false;
				for(std::map<int32,vector<string> >::iterator iterMap = mapDuplicateFiles.begin();
					iterMap != mapDuplicateFiles.end(); ++iterMap) {
					vector<string> &fileList = iterMap->second;
					if(fileList.size() > 1) {
						if(foundDuplicates == false) {
							foundDuplicates = true;
							printf("\nWarning, duplicate files were detected - START:\n=====================\n");
						}

						map<string,int> parentList;
						for(unsigned int idx = 0; idx < fileList.size(); ++idx) {
							string duplicateFile = fileList[idx];
							if(idx > 0) {
								off_t fileSize = getFileSize(duplicateFile);
								// convert to MB
								duplicateMegaBytes += ((double)fileSize / 1048576.0);
								duplicateCount++;
							}
							else {
								printf("\n");
							}

							printf("[%s]\n",duplicateFile.c_str());
							std::map<string,vector<pair<string, string> > >::iterator iterFind = loadedFileList.find(duplicateFile);
							if(iterFind != loadedFileList.end()) {
								for(unsigned int jdx = 0; jdx < iterFind->second.size(); jdx++) {
									parentList[iterFind->second[jdx].first]++;
								}
							}
						}

						for(map<string,int>::iterator iterMap1 = parentList.begin();
								iterMap1 != parentList.end(); ++iterMap1) {

							if(iterMap1 == parentList.begin()) {
								printf("\tParents:\n");
							}
							printf("\t[%s]\n",iterMap1->first.c_str());
						}

						if(purgeDuplicateFiles == true) {
							string newCommonFileName = "";
							for(unsigned int idx = 0; idx < fileList.size(); ++idx) {
								string duplicateFile = fileList[idx];
								string fileExt = extractExtension(duplicateFile);
								if(fileExt == "wav" || fileExt == "ogg") {
									off_t fileSize = getFileSize(duplicateFile);
									if(idx == 0) {
										newCommonFileName = "$COMMONDATAPATH/sounds/" + extractFileFromDirectoryPath(duplicateFile);

										string expandedNewCommonFileName = newCommonFileName;

										std::map<string,string> mapExtraTagReplacementValues;
										mapExtraTagReplacementValues["$COMMONDATAPATH"] = techPath + techName + "/commondata/";
										mapExtraTagReplacementValues = Properties::getTagReplacementValues(&mapExtraTagReplacementValues);
										Properties::applyTagsToValue(expandedNewCommonFileName,&mapExtraTagReplacementValues);
										createDirectoryPaths(extractDirectoryPathFromFile(expandedNewCommonFileName));

										if(svnPurgeFiles == true) {
											copyFileTo(duplicateFile, expandedNewCommonFileName);

											char szBuf[4096]="";
											sprintf(szBuf,"svn delete \"%s\"",duplicateFile.c_str());
											bool svnOk = executeShellCommand(szBuf,0);
											if(svnOk == false) {
												throw runtime_error("Call to command failed [" + string(szBuf) + "]");
											}
											printf("*** Duplicate file:\n[%s]\nwas svn deleted and copied to:\n[%s]\n",duplicateFile.c_str(),expandedNewCommonFileName.c_str());
										}
										else {
											//int result = 0;
											int result = rename(duplicateFile.c_str(),expandedNewCommonFileName.c_str());
											if(result != 0) {
												char szBuf[4096]="";
												char *errmsg = strerror(errno);
												sprintf(szBuf,"!!! Error [%s] Could not rename [%s] to [%s]!",errmsg,duplicateFile.c_str(),expandedNewCommonFileName.c_str());
												throw runtime_error(szBuf);
											}
											else {
												printf("*** Duplicate file:\n[%s]\nwas renamed to:\n[%s]\n",duplicateFile.c_str(),expandedNewCommonFileName.c_str());
											}
										}
									}
									else {
										if(svnPurgeFiles == true) {
											char szBuf[4096]="";
											sprintf(szBuf,"svn delete \"%s\"",duplicateFile.c_str());
											bool svnOk = executeShellCommand(szBuf,0);
											if(svnOk == false) {
												throw runtime_error("Call to command failed [" + string(szBuf) + "]");
											}
											printf("*** Duplicate file:\n[%s]\nwas svn deleted\n",duplicateFile.c_str());
										}
										else {
											removeFile(duplicateFile);
										}
										printf("*** Duplicate file:\n[%s]\nwas removed\n",duplicateFile.c_str());

										// convert to MB
										duplicateMegaBytesPurged += ((double)fileSize / 1048576.0);
										duplicateCountPurged++;
									}
								}
							}

							std::map<string,int> mapUniqueParentList;

							for(unsigned int idx = 0; idx < fileList.size(); ++idx) {
								string duplicateFile = fileList[idx];
								string fileExt = extractExtension(duplicateFile);
								if(fileExt == "wav" || fileExt == "ogg") {
									std::map<string,vector<pair<string, string> > >::iterator iterFind2 = loadedFileList.find(duplicateFile);
									if(iterFind2 != loadedFileList.end()) {
										for(unsigned int jdx1 = 0; jdx1 < iterFind2->second.size(); jdx1++) {
											string parentFile = iterFind2->second[jdx1].first;
											string searchText = iterFind2->second[jdx1].second;

											if(mapUniqueParentList.find(parentFile) == mapUniqueParentList.end()) {
												printf("*** Searching parent file:\n[%s]\nfor duplicate file reference:\n[%s]\nto replace with newname:\n[%s]\n",parentFile.c_str(),searchText.c_str(),newCommonFileName.c_str());
												bool foundText = searchAndReplaceTextInFile(parentFile, searchText, newCommonFileName, false);
												printf("foundText = %d\n",foundText);
												if(foundText == false) {
													char szBuf[4096]="";
													sprintf(szBuf,"Error finding text [%s] in file [%s]",searchText.c_str(),parentFile.c_str());
													throw runtime_error(szBuf);
												}
												mapUniqueParentList[parentFile]++;
											}
										}
									}
								}
							}
						}
						else {
							string newCommonFileName = "";
							for(unsigned int idx = 0; idx < fileList.size(); ++idx) {
								string duplicateFile = fileList[idx];
								string fileExt = extractExtension(duplicateFile);
								if(fileExt == "wav" || fileExt == "ogg") {
									off_t fileSize = getFileSize(duplicateFile);
									if(idx == 0) {
										newCommonFileName = "$COMMONDATAPATH/sounds/" + extractFileFromDirectoryPath(duplicateFile);
										break;
									}
								}
							}
							for(unsigned int idx = 0; idx < fileList.size(); ++idx) {
								string duplicateFile = fileList[idx];
								string fileExt = extractExtension(duplicateFile);
								if(fileExt == "wav" || fileExt == "ogg") {
									std::map<string,vector<pair<string, string> > >::iterator iterFind4 = loadedFileList.find(duplicateFile);
									if(iterFind4 != loadedFileList.end()) {
										for(unsigned int jdx = 0; jdx < iterFind4->second.size(); jdx++) {
											string parentFile = iterFind4->second[jdx].first;
											string searchText = iterFind4->second[jdx].second;

											//printf("*** Searching parent file:\n[%s]\nfor duplicate file reference:\n[%s]\nto replace with newname:\n[%s]\n",parentFile.c_str(),searchText.c_str(),newCommonFileName.c_str());
											bool foundText = searchAndReplaceTextInFile(parentFile, searchText, newCommonFileName, true);
											//printf("foundText = %d\n",foundText);
											if(foundText == false) {
												char szBuf[4096]="";
												sprintf(szBuf,"Error finding text [%s] in file [%s]",searchText.c_str(),parentFile.c_str());
												throw runtime_error(szBuf);
											}
										}
									}
								}
							}
						}
					}
				}
				if(foundDuplicates == true) {
					printf("Duplicates %.2f MB (%d) in files\n",duplicateMegaBytes,duplicateCount);
					printf("Duplicates purged %.2f MB (%d) in files\n",duplicateMegaBytesPurged,duplicateCountPurged);

					printf("\nWarning, duplicate files were detected - END:\n");
				}
			}

			if(techtree_errors == false) {
				printf("\nValidation found NO ERRORS for techPath [%s] techName [%s] factions checked (count = %d):\n",techPath.c_str(), techName.c_str(),(int)factions.size());
				for ( set<string>::iterator it = factions.begin(); it != factions.end(); ++it ) {
					printf("Faction [%s]\n",(*it).c_str());
				}
			}
		}
		printf("----------------------------------------------------------------");
	}
}

void runTechValidationReport(int argc, char** argv) {
	//disableBacktrace=true;
	printf("====== Started Validation ======\n");

	bool purgeDuplicateFiles = false;
	bool showDuplicateFiles = true;
	bool purgeUnusedFiles = false;
	bool svnPurgeFiles = false;

	double purgedMegaBytes=0;
	Config &config = Config::getInstance();

	// Did the user pass a specific scenario to validate?
	if(hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_VALIDATE_SCENARIO]) + string("=")) == true) {
        int foundParamIndIndex = -1;
        hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_VALIDATE_SCENARIO]) + string("="),&foundParamIndIndex);

        string filterList = argv[foundParamIndIndex];
        vector<string> paramPartTokens;
        Tokenize(filterList,paramPartTokens,"=");

        if(paramPartTokens.size() >= 2) {
        	vector<string> optionList;
            string validateScenarioName = paramPartTokens[1];

			printf("Filtering scenario: %s\n",validateScenarioName.c_str());

            if(paramPartTokens.size() >= 3) {
            	if(paramPartTokens[2] == "purgeunused") {
            		purgeUnusedFiles = true;
            		printf("*NOTE All unused scenario files will be deleted!\n");
            	}
            }

            {
            printf("\n---------------- Loading scenario inside world ----------------\n");

            World world;
            double purgedMegaBytes=0;
            std::vector<string> filteredFactionList;

            vector<string> scenarioPaths = config.getPathListForType(ptScenarios);
            for(int idx = 0; idx < scenarioPaths.size(); idx++) {
                string &scenarioPath = scenarioPaths[idx];
        		endPathWithSlash(scenarioPath);

                //printf("techPath [%s]\n",techPath.c_str());

        		vector<string> scenarioList;
        		findDirs(scenarioPath, scenarioList, false, false);
                for(int idx2 = 0; idx2 < scenarioList.size(); idx2++) {
                    string &scenarioName = scenarioList[idx2];

                    //printf("Found Scenario [%s] looking for [%s]\n",scenarioName.c_str(),validateScenarioName.c_str());

                    if(scenarioName == validateScenarioName) {

                    	string file = scenarioPath + scenarioName + "/" + scenarioName + ".xml";

                        XmlTree xmlTree;
                    	xmlTree.load(file,Properties::getTagReplacementValues());
                    	const XmlNode *scenarioNode= xmlTree.getRootNode();
                    	string techName = scenarioNode->getChild("tech-tree")->getAttribute("value")->getValue();

                    	// Self Contained techtree?
                    	string scenarioTechtree = scenarioPath + scenarioName + "/" + techName + "/" + techName + ".xml";
                    	if(fileExists(scenarioTechtree) == true) {
                    		string techPath = scenarioPath + scenarioName + "/";

                    		printf("Found Scenario [%s] with custom techtree [%s] validating...\n",scenarioName.c_str(),techName.c_str());
                    		runTechValidationForPath(techPath, techName, filteredFactionList,
                    					world, 	purgeUnusedFiles, showDuplicateFiles, false, false, purgedMegaBytes);
                    	}
//
//
//                    	runTechValidationForPath(techPath, techName, filteredFactionList,
//                    			world, 	purgeUnusedFiles,purgedMegaBytes);
                    }
                }
            }

            printf("\n====== Finished Validation ======\n");
            }
            return;
        }
        else {
            printf("\nInvalid missing scenario specified on commandline [%s] value [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
            //printParameterHelp(argv[0],false);
            return;
        }
    }

    // Did the user pass a specific list of factions to validate?
    std::vector<string> filteredFactionList;
    if(hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_VALIDATE_FACTIONS]) + string("=")) == true) {
        int foundParamIndIndex = -1;
        hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_VALIDATE_FACTIONS]) + string("="),&foundParamIndIndex);

        string filterList = argv[foundParamIndIndex];
        vector<string> paramPartTokens;
        Tokenize(filterList,paramPartTokens,"=");

        if(paramPartTokens.size() >= 2) {
            string factionList = paramPartTokens[1];
            Tokenize(factionList,filteredFactionList,",");

            if(filteredFactionList.empty() == false) {
                printf("Filtering factions and only looking for the following:\n");
                for(int idx = 0; idx < filteredFactionList.size(); ++idx) {
                    filteredFactionList[idx] = trim(filteredFactionList[idx]);
                    printf("%s\n",filteredFactionList[idx].c_str());
                }
            }

            if(paramPartTokens.size() >= 3) {
            	if(paramPartTokens[2] == "purgeunused") {
            		purgeUnusedFiles = true;
            		printf("*NOTE All unused faction files will be deleted!\n");
            	}
            }
        }
    }
    vector<string> results;
    findDirs(config.getPathListForType(ptTechs), results);
    vector<string> techTreeFiles = results;
    // Did the user pass a specific list of techtrees to validate?
    std::vector<string> filteredTechTreeList;
    if(hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_VALIDATE_TECHTREES]) + string("=")) == true) {
        int foundParamIndIndex = -1;
        hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_VALIDATE_TECHTREES]) + string("="),&foundParamIndIndex);
        string filterList = argv[foundParamIndIndex];
        vector<string> paramPartTokens;
        Tokenize(filterList,paramPartTokens,"=");
        if(paramPartTokens.size() >= 2) {
            string techtreeList = paramPartTokens[1];
            Tokenize(techtreeList,filteredTechTreeList,",");

            if(filteredTechTreeList.empty() == false) {
                printf("Filtering techtrees and only looking for the following:\n");
                for(int idx = 0; idx < filteredTechTreeList.size(); ++idx) {
                    filteredTechTreeList[idx] = trim(filteredTechTreeList[idx]);
                    printf("%s\n",filteredTechTreeList[idx].c_str());
                }
            }

            if(paramPartTokens.size() >= 3) {
            	if(paramPartTokens[2] == "purgeunused") {
            		purgeUnusedFiles = true;
            		printf("*NOTE All unused techtree files will be deleted!\n");
            	}
            	else if(paramPartTokens[2] == "purgeduplicates") {
            		purgeDuplicateFiles = true;
            		printf("*NOTE All duplicate techtree files will be merged!\n");
            	}
            	else if(paramPartTokens[2] == "svndelete") {
            		svnPurgeFiles = true;
            		printf("*NOTE All unused / duplicate techtree files will be removed from svn!\n");
            	}
            	else if(paramPartTokens[2] == "hideduplicates") {
            		showDuplicateFiles = false;
            		printf("*NOTE All duplicate techtree files will NOT be shown!\n");
            	}
            }
            if(paramPartTokens.size() >= 4) {
            	if(paramPartTokens[3] == "purgeunused") {
            		purgeUnusedFiles = true;
            		printf("*NOTE All unused techtree files will be deleted!\n");
            	}
            	else if(paramPartTokens[3] == "purgeduplicates") {
            		purgeDuplicateFiles = true;
            		printf("*NOTE All duplicate techtree files will be merged!\n");
            	}
            	else if(paramPartTokens[3] == "svndelete") {
            		svnPurgeFiles = true;
            		printf("*NOTE All unused / duplicate techtree files will be removed from svn!\n");
            	}
            	else if(paramPartTokens[3] == "hideduplicates") {
            		showDuplicateFiles = false;
            		printf("*NOTE All duplicate techtree files will NOT be shown!\n");
            	}
            }
            if(paramPartTokens.size() >= 5) {
            	if(paramPartTokens[4] == "purgeunused") {
            		purgeUnusedFiles = true;
            		printf("*NOTE All unused techtree files will be deleted!\n");
            	}
            	else if(paramPartTokens[4] == "purgeduplicates") {
            		purgeDuplicateFiles = true;
            		printf("*NOTE All duplicate techtree files will be merged!\n");
            	}
            	else if(paramPartTokens[4] == "svndelete") {
            		svnPurgeFiles = true;
            		printf("*NOTE All unused / duplicate techtree files will be removed from svn!\n");
            	}
            	else if(paramPartTokens[4] == "hideduplicates") {
            		showDuplicateFiles = false;
            		printf("*NOTE All duplicate techtree files will NOT be shown!\n");
            	}
            }

        }
    }

    {
    printf("\n---------------- Loading factions inside world ----------------");
    World world;

    vector<string> techPaths = config.getPathListForType(ptTechs);
    for(int idx = 0; idx < techPaths.size(); idx++) {
        string &techPath = techPaths[idx];
		endPathWithSlash(techPath);

        //printf("techPath [%s]\n",techPath.c_str());

        for(int idx2 = 0; idx2 < techTreeFiles.size(); idx2++) {
            string &techName = techTreeFiles[idx2];

            if(	filteredTechTreeList.empty() == true ||
                std::find(filteredTechTreeList.begin(),filteredTechTreeList.end(),techName) != filteredTechTreeList.end()) {

            	runTechValidationForPath(techPath, techName, filteredFactionList,
            			world, 	purgeUnusedFiles,purgeDuplicateFiles,
            			showDuplicateFiles,svnPurgeFiles,purgedMegaBytes);
            }
        }
    }

    printf("\n====== Finished Validation ======\n");
    }

}

void ShowINISettings(int argc, char **argv,Config &config,Config &configKeys) {
    if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_SHOW_INI_SETTINGS]) == true) {
        vector<string> filteredPropertyList;
        if(hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_SHOW_INI_SETTINGS]) + string("=")) == true) {
            int foundParamIndIndex = -1;
            hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_SHOW_INI_SETTINGS]) + string("="),&foundParamIndIndex);
            string filterList = argv[foundParamIndIndex];
            vector<string> paramPartTokens;
            Tokenize(filterList,paramPartTokens,"=");
            if(paramPartTokens.size() >= 2) {
                string tokenList = paramPartTokens[1];
                Tokenize(tokenList,filteredPropertyList,",");

                if(filteredPropertyList.empty() == false) {
                    printf("Filtering properties and only looking for the following:\n");
                    for(int idx = 0; idx < filteredPropertyList.size(); ++idx) {
                        filteredPropertyList[idx] = trim(filteredPropertyList[idx]);
                        printf("%s\n",filteredPropertyList[idx].c_str());
                    }
                }
            }
        }

        printf("\nMain settings report\n");
        printf("====================\n");
        vector<pair<string,string> > mergedMainSettings = config.getMergedProperties();
        vector<pair<string,string> > mergedKeySettings = configKeys.getMergedProperties();

        // Figure out the max # of tabs we need to format display nicely
        int tabCount = 1;
        for(int i = 0; i < mergedMainSettings.size(); ++i) {
            const pair<string,string> &nameValue = mergedMainSettings[i];

            bool displayProperty = false;
            if(filteredPropertyList.empty() == false) {
                if(find(filteredPropertyList.begin(),filteredPropertyList.end(),nameValue.first) != filteredPropertyList.end()) {
                    displayProperty = true;
                }
            }
            else {
                displayProperty = true;
            }

            if(displayProperty == true) {
                int requredTabs = (nameValue.first.length() / 8)+1;
                if(nameValue.first.length() % 8) {
                    requredTabs++;
                }
                if(requredTabs > tabCount) {
                    tabCount = requredTabs;
                }
            }
        }
        for(int i = 0; i < mergedKeySettings.size(); ++i) {
            const pair<string,string> &nameValue = mergedKeySettings[i];

            bool displayProperty = false;
            if(filteredPropertyList.empty() == false) {
                if(find(filteredPropertyList.begin(),filteredPropertyList.end(),nameValue.first) != filteredPropertyList.end()) {
                    displayProperty = true;
                }
            }
            else {
                displayProperty = true;
            }

            if(displayProperty == true) {
                int requredTabs = (nameValue.first.length() / 8)+1;
                if(nameValue.first.length() % 8) {
                    requredTabs++;
                }
                if(requredTabs > tabCount) {
                    tabCount = requredTabs;
                }
            }
        }

        // Output the properties
        for(int i = 0; i < mergedMainSettings.size(); ++i) {
            const pair<string,string> &nameValue = mergedMainSettings[i];

            bool displayProperty = false;
            if(filteredPropertyList.empty() == false) {
                if(find(filteredPropertyList.begin(),filteredPropertyList.end(),nameValue.first) != filteredPropertyList.end()) {
                    displayProperty = true;
                }
            }
            else {
                displayProperty = true;
            }

            if(displayProperty == true) {
                printf("Property Name [%s]",nameValue.first.c_str());

                int tabs = (nameValue.first.length() / 8) + 1;
                for(int j = 0; j < (tabCount - tabs); ++j) {
                    printf("\t");
                }

                printf("Value [%s]\n",nameValue.second.c_str());
            }
        }

        printf("\n\nMain key binding settings report\n");
        printf("====================================\n");

        for(int i = 0; i < mergedKeySettings.size(); ++i) {
            const pair<string,string> &nameValue = mergedKeySettings[i];

            bool displayProperty = false;
            if(filteredPropertyList.empty() == false) {
                if(find(filteredPropertyList.begin(),filteredPropertyList.end(),nameValue.first) != filteredPropertyList.end()) {
                    displayProperty = true;
                }
            }
            else {
                displayProperty = true;
            }

            if(displayProperty == true) {
                printf("Property Name [%s]",nameValue.first.c_str());

                int tabs = (nameValue.first.length() / 8) + 1;
                for(int j = 0; j < (tabCount - tabs); ++j) {
                    printf("\t");
                }

                printf("Value [%s]\n",nameValue.second.c_str());
            }
        }
    }
}

void CheckForDuplicateData() {
    Config &config = Config::getInstance();

    string duplicateWarnings="";

    {
  	vector<string> maps;
  	std::vector<std::string> results;
  	vector<string> mapPaths = config.getPathListForType(ptMaps);

    findAll(mapPaths, "*.gbm", results, false, false, true);
	copy(results.begin(), results.end(), std::back_inserter(maps));

	results.clear();
    findAll(mapPaths, "*.mgm", results, false, false, true);
    copy(results.begin(), results.end(), std::back_inserter(maps));

	results.clear();
	std::sort(maps.begin(),maps.end());

	if(maps.empty()) {
        throw runtime_error("No maps were found!");
    }

	vector<string> duplicateMapsToRename;
	for(int i = 0; i < maps.size(); ++i) {
	    string map1 = maps[i];
	    for(int j = 0; j < maps.size(); ++j) {
	        if(i != j) {
                string map2 = maps[j];

                //printf("i = %d map1 [%s] j = %d map2 [%s]\n",i,map1.c_str(),j,map2.c_str());

                if(map1 == map2) {
                	if(std::find(duplicateMapsToRename.begin(),duplicateMapsToRename.end(),map1) == duplicateMapsToRename.end()) {
                		duplicateMapsToRename.push_back(map1);
                	}
                }
	        }
	    }
	}
	if(duplicateMapsToRename.empty() == false) {
		string errorMsg = "Warning duplicate maps were detected and renamed:\n";
		for(int i = 0; i < duplicateMapsToRename.size(); ++i) {
			string currentPath = mapPaths[1];
			endPathWithSlash(currentPath);

			string oldFile = currentPath + duplicateMapsToRename[i];
			string newFile = currentPath + duplicateMapsToRename[i];
			string ext = extractExtension(newFile);
			newFile = newFile.substr( 0, newFile.length()-ext.length()-1);
			newFile = newFile + "_custom." + ext;

			char szBuf[4096]="";
			int result = rename(oldFile.c_str(),newFile.c_str());
			if(result != 0) {
				char *errmsg = strerror(errno);
				sprintf(szBuf,"Error [%s]\nCould not rename [%s] to [%s]!",errmsg,oldFile.c_str(),newFile.c_str());
				throw runtime_error(szBuf);
			}
			else {
				sprintf(szBuf,"map [%s] in [%s]\nwas renamed to [%s]",duplicateMapsToRename[i].c_str(),oldFile.c_str(),newFile.c_str());
			}
			errorMsg += szBuf;
		}
		duplicateWarnings += errorMsg;

//        Program *program = Program::getInstance();
//        if(program) {
//        	program->getState()->setForceMouseRender(true);
//        }
//        ExceptionHandler::DisplayMessage(errorMsg.c_str(), false);
	}
    }

    {
	//tilesets
	std::vector<std::string> tileSets;
    vector<string> tilesetPaths = config.getPathListForType(ptTilesets);
    findDirs(tilesetPaths, tileSets, false, true);

	if (tileSets.empty()) {
        throw runtime_error("No tilesets were found!");
    }

	vector<string> duplicateTilesetsToRename;
	for(int i = 0; i < tileSets.size(); ++i) {
	    string tileSet1 = tileSets[i];
	    for(int j = 0; j < tileSets.size(); ++j) {
	        if(i != j) {
                string tileSet2= tileSets[j];
                if(tileSet1 == tileSet2) {
                	if(std::find(duplicateTilesetsToRename.begin(),duplicateTilesetsToRename.end(),tileSet1) == duplicateTilesetsToRename.end()) {
                		duplicateTilesetsToRename.push_back(tileSet1);
                	}
                }
	        }
	    }
	}
	if(duplicateTilesetsToRename.empty() == false) {
		string errorMsg = "Warning duplicate tilesets were detected and renamed:\n";

		for(int i = 0; i < duplicateTilesetsToRename.size(); ++i) {
			string currentPath = tilesetPaths[1];
			endPathWithSlash(currentPath);

			string oldFile = currentPath + duplicateTilesetsToRename[i];
			string newFile = currentPath + duplicateTilesetsToRename[i];
			newFile = newFile + "_custom";

			char szBuf[4096]="";
			int result = rename(oldFile.c_str(),newFile.c_str());
			if(result != 0) {
				char *errmsg = strerror(errno);
				sprintf(szBuf,"Error [%s]\nCould not rename [%s] to [%s]!",errmsg,oldFile.c_str(),newFile.c_str());
				throw runtime_error(szBuf);
			}
			else {
				sprintf(szBuf,"tileset [%s] in [%s]\nwas renamed to [%s]",duplicateTilesetsToRename[i].c_str(),oldFile.c_str(),newFile.c_str());

				string tilesetName = extractFileFromDirectoryPath(oldFile);
				oldFile = newFile + "/" + tilesetName + ".xml";
				newFile = newFile + "/" + tilesetName + "_custom.xml";

				//printf("\n\n\n###### RENAME [%s] to [%s]\n\n",oldFile.c_str(),newFile.c_str());
				rename(oldFile.c_str(),newFile.c_str());
			}
			errorMsg += szBuf;
		}
		duplicateWarnings += errorMsg;

//        Program *program = Program::getInstance();
//        if(program) {
//        	program->getState()->setForceMouseRender(true);
//        }
//        ExceptionHandler::DisplayMessage(errorMsg.c_str(), false);
	}
    }

    {
    vector<string> techPaths = config.getPathListForType(ptTechs);
    vector<string> techTrees;
    findDirs(techPaths, techTrees, false, true);
	if(techTrees.empty()) {
        throw runtime_error("No tech-trees were found!");
	}

	vector<string> duplicateTechtreesToRename;
	for(int i = 0; i < techTrees.size(); ++i) {
	    string techtree1 = techTrees[i];
	    for(int j = 0; j < techTrees.size(); ++j) {
	        if(i != j) {
                string techtree2 = techTrees[j];
                if(techtree1 == techtree2) {
                	if(std::find(duplicateTechtreesToRename.begin(),duplicateTechtreesToRename.end(),techtree1) == duplicateTechtreesToRename.end()) {
                		duplicateTechtreesToRename.push_back(techtree1);
                	}
                }
	        }
	    }
	}
	if(duplicateTechtreesToRename.empty() == false) {
		string errorMsg = "Warning duplicate techtrees were detected and renamed:\n";

		for(int i = 0; i < duplicateTechtreesToRename.size(); ++i) {
			string currentPath = techPaths[1];
			endPathWithSlash(currentPath);

			string oldFile = currentPath + duplicateTechtreesToRename[i];
			string newFile = currentPath + duplicateTechtreesToRename[i];
			newFile = newFile + "_custom";

			char szBuf[4096]="";
			int result = rename(oldFile.c_str(),newFile.c_str());
			if(result != 0) {
				char *errmsg = strerror(errno);
				sprintf(szBuf,"Error [%s]\nCould not rename [%s] to [%s]!",errmsg,oldFile.c_str(),newFile.c_str());
				throw runtime_error(szBuf);
			}
			else {
				sprintf(szBuf,"techtree [%s] in [%s]\nwas renamed to [%s]",duplicateTechtreesToRename[i].c_str(),oldFile.c_str(),newFile.c_str());

				string tilesetName = extractFileFromDirectoryPath(oldFile);
				oldFile = newFile + "/" + tilesetName + ".xml";
				newFile = newFile + "/" + tilesetName + "_custom.xml";

				//printf("\n\n\n###### RENAME [%s] to [%s]\n\n",oldFile.c_str(),newFile.c_str());
				rename(oldFile.c_str(),newFile.c_str());
			}
			errorMsg += szBuf;
		}
		duplicateWarnings += errorMsg;

//        Program *program = Program::getInstance();
//        if(program) {
//        	program->getState()->setForceMouseRender(true);
//        }
//        ExceptionHandler::DisplayMessage(errorMsg.c_str(), false);
	}
    }

    if(duplicateWarnings != "") {
		Program *program = Program::getInstance();
		if(program) {
			program->getState()->setForceMouseRender(true);
		}
		ExceptionHandler::DisplayMessage(duplicateWarnings.c_str(), false);
    }
}

int glestMain(int argc, char** argv) {
#ifdef SL_LEAK_DUMP
	AllocRegistry memoryLeaks = AllocRegistry::getInstance();
#endif

	application_binary= executable_path(argv[0],true);
	mg_app_name = GameConstants::application_name;
	mailStringSupport = mailString;
    SystemFlags::ENABLE_THREADED_LOGGING = false;
    disableBacktrace = false;
	bool foundInvalidArgs = false;
	preCacheThread=NULL;

	Properties::setApplicationPath(executable_path(argv[0]));
	Properties::setGameVersion(glestVersionString);

    ServerSocket::setMaxPlayerCount(GameConstants::maxPlayers);
    SystemFlags::VERBOSE_MODE_ENABLED  = false;
    if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_VERBOSE_MODE]) == true) {
        SystemFlags::VERBOSE_MODE_ENABLED  = true;
    }

	//vector<string> results;
	//findAll("/home/softcoder/Code/megaglest/trunk/mk/linux//äöüß/maps/*.gbm", results, false, true);
	//findAll("C:\\Documents and Settings\\SoftCoder\\Application Data\\人間五\\maps\\*", results, false, true);
	//for(unsigned int i = 0; i < results.size(); ++i) {
	//	string file = results[i];
	//	printf("FILE: [%s]\n",file.c_str());
	//}
	//return -1;

    if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_DISABLE_BACKTRACE]) == true) {
        disableBacktrace = true;
    }

//    UErrorCode status = U_ZERO_ERROR;
//	u_init(&status);
//	if (U_SUCCESS(status)) {
//		printf("everything is OK\n");
//	}
//	else {
//		printf("error %s opening resource\n", u_errorName(status));
//	}

    // TEST:
    //string testfile = "/home/softcoder/Code/megaglest/trunk/mk/linux/techs/megapack/factions/egypt/units/desert_camp/../../upgrades/spear_weapons/images/piercing.bmp";
    //updatePathClimbingParts(testfile);
    //return -1;
    //CHANGED relative path from [/home/softcoder/Code/megaglest/trunk/mk/linux/techs/megapack/factions/egypt/units/desert_camp/../../upgrades/spear_weapons/images/piercing.bmp] to [/home/softcoder/Code/megaglest/trunk/mk/linux/techs/megapack/factions/egypt/units/desert_camp/upgrades/spear_weapons/images/piercing.bmp]


#if defined(CUSTOM_DATA_INSTALL_PATH)
    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\n\nCUSTOM_DATA_INSTALL_PATH = [%s]\n\n",CUSTOM_DATA_INSTALL_PATH);
#endif

	const int knownArgCount = sizeof(GAME_ARGS) / sizeof(GAME_ARGS[0]);
	for(int idx = 1; idx < argc; ++idx) {
		if( hasCommandArgument(knownArgCount, (char **)&GAME_ARGS[0], argv[idx], NULL, 0, true) == false) {
			foundInvalidArgs = true;
			printf("\nInvalid argument: %s",argv[idx]);
		}
	}

	if( hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_HELP]) == true ||
		foundInvalidArgs == true) {

		printParameterHelp(argv[0],foundInvalidArgs);
		return -1;
	}

    if( hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_MASTERSERVER_MODE])) == true) {
    	isMasterServerModeEnabled = true;
    	Window::setMasterserverMode(isMasterServerModeEnabled);

    	int foundParamIndIndex = -1;
		hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_MASTERSERVER_MODE]) + string("="),&foundParamIndIndex);
		if(foundParamIndIndex < 0) {
			hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_MASTERSERVER_MODE]),&foundParamIndIndex);
		}
		string paramValue = argv[foundParamIndIndex];
		vector<string> paramPartTokens;
		Tokenize(paramValue,paramPartTokens,"=");
		if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
			string headless_command_list = paramPartTokens[1];

			vector<string> paramHeadlessCommandList;
			Tokenize(headless_command_list,paramHeadlessCommandList,",");

			for(unsigned int i = 0; i < paramHeadlessCommandList.size(); ++i) {
				string headless_command = paramHeadlessCommandList[i];
				if(headless_command == "exit") {
					printf("Forcing quit after game has compelted [%s]\n",headless_command.c_str());
					Program::setWantShutdownApplicationAfterGame(true);
				}
				else if(headless_command == "vps") {
					printf("Disabled reading from console [%s]\n",headless_command.c_str());
					disableheadless_console = true;
				}
			}
		}
    }

	//off_t fileSize = getFileSize(argv[0]);
	//double fSize = ((double)fileSize / 1048576.0);
	//printf("[%ld] [%.2f]\n",fileSize,fSize);
	//return -1;

#ifdef WIN32
	SocketManager winSockManager;
#endif

    bool haveSpecialOutputCommandLineOption = false;

	if( hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_OPENGL_INFO]) 			== true ||
		hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_SDL_INFO]) 			== true ||
		hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_LUA_INFO]) 			== true ||
        hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_CURL_INFO]) 			== true ||
		hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_VERSION]) 				== true ||
        hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_SHOW_INI_SETTINGS])    == true ||
		hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_VALIDATE_TECHTREES]) 	== true ||
		hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_VALIDATE_FACTIONS]) 	== true ||
		hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_VALIDATE_SCENARIO]) 	== true) {
		haveSpecialOutputCommandLineOption = true;
	}

	if( haveSpecialOutputCommandLineOption == false ||
		hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_VERSION]) == true) {
#ifdef USE_STREFLOP
//#	define STREFLOP_NO_DENORMALS
//	streflop_init<streflop::Simple>();
	printf("%s, SVN: [%s], [STREFLOP]\n",getNetworkVersionString().c_str(),getSVNRevisionString().c_str());
#else
	printf("%s, SVN: [%s]\n",getNetworkVersionString().c_str(),getSVNRevisionString().c_str());
#endif
	}

	if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_SDL_INFO]) == true) {
		SDL_version ver;

        // Prints the compile time version
        SDL_VERSION(&ver);
		print_SDL_version("SDL compile-time version", &ver);

        // Prints the run-time version
        ver = *SDL_Linked_Version();
        print_SDL_version("SDL runtime version", &ver);
	}

	if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_LUA_INFO]) == true) {
		printf("LUA version: %s\n", LUA_RELEASE);
	}

	if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_CURL_INFO]) == true) {
	    curl_version_info_data *curlVersion= curl_version_info(CURLVERSION_NOW);
		printf("CURL version: %s\n", curlVersion->version);
	}

	if( (hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_VERSION]) 		  == true ||
		 hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_SDL_INFO]) 		  == true ||
		 hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_LUA_INFO]) 		  == true ||
         hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_CURL_INFO]) 		  == true) &&
		hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_OPENGL_INFO]) 		  == false &&
		hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_VALIDATE_TECHTREES]) == false &&
		hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_VALIDATE_FACTIONS])  == false &&
		hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_VALIDATE_SCENARIO])  == false) {
		return -1;
	}

	if(hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_MOD])) == true) {

		int foundParamIndIndex = -1;
		hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_MOD]) + string("="),&foundParamIndIndex);
		if(foundParamIndIndex < 0) {
			hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_MOD]),&foundParamIndIndex);
		}
		string scenarioName = argv[foundParamIndIndex];
		vector<string> paramPartTokens;
		Tokenize(scenarioName,paramPartTokens,"=");
		if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
			string autoloadModName = paramPartTokens[1];
			if(Properties::applyTagsToValue(autoloadModName) == true) {
				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Property key [%s] now has value [%s]\n",Config::ACTIVE_MOD_PROPERTY_NAME,autoloadModName.c_str());
			}

			Config::setCustomRuntimeProperty(Config::ACTIVE_MOD_PROPERTY_NAME,autoloadModName);

			printf("Setting mod active [%s]\n",autoloadModName.c_str());
		}
		else {
			printf("\nInvalid mod pathname specified on commandline [%s] mod [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
			printParameterHelp(argv[0],foundInvalidArgs);
			return -1;
		}
	}

	SystemFlags::init(haveSpecialOutputCommandLineOption);
	//SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled  = true;
	//SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled = true;

	MainWindow *mainWindow= NULL;
	Program *program= NULL;
	ExceptionHandler exceptionHandler;
	exceptionHandler.install( getCrashDumpFileName() );

	int shutdownFadeSoundMilliseconds = 1000;
	Chrono chronoshutdownFadeSound;
	SimpleTaskThread *soundThreadManager = NULL;

	try {
        // Setup paths to game items (like data, logs, ini etc)
        int setupResult = setupGameItemPaths(argc, argv, NULL);
        if(setupResult != 0) {
            return setupResult;
        }

		// Attempt to read ini files
		Config &config = Config::getInstance();
		setupGameItemPaths(argc, argv, &config);

		Socket::disableNagle = config.getBool("DisableNagle","false");
		if(Socket::disableNagle) {
			printf("*WARNING users wants to disable the socket nagle algorithm.\n");
		}
		Socket::DEFAULT_SOCKET_SENDBUF_SIZE = config.getInt("DefaultSocketSendBufferSize",intToStr(Socket::DEFAULT_SOCKET_SENDBUF_SIZE).c_str());
		if(Socket::DEFAULT_SOCKET_SENDBUF_SIZE >= 0) {
			printf("*WARNING users wants to set default socket send buffer size to: %d\n",Socket::DEFAULT_SOCKET_SENDBUF_SIZE);
		}
		Socket::DEFAULT_SOCKET_RECVBUF_SIZE = config.getInt("DefaultSocketReceiveBufferSize",intToStr(Socket::DEFAULT_SOCKET_RECVBUF_SIZE).c_str());
		if(Socket::DEFAULT_SOCKET_RECVBUF_SIZE >= 0) {
			printf("*WARNING users wants to set default socket receive buffer size to: %d\n",Socket::DEFAULT_SOCKET_RECVBUF_SIZE);
		}

		shutdownFadeSoundMilliseconds = config.getInt("ShutdownFadeSoundMilliseconds",intToStr(shutdownFadeSoundMilliseconds).c_str());

	    string userData = config.getString("UserData_Root","");
	    if(userData != "") {
	    	endPathWithSlash(userData);

	        if(isdir(userData.c_str()) == false) {
	        	createDirectoryPaths(userData);
	        }
	    }
	    string crcCachePath = userData + "cache/";
        if(isdir(crcCachePath.c_str()) == false) {
        	createDirectoryPaths(crcCachePath);
        }
	    setCRCCacheFilePath(crcCachePath);

	    string tempDataPath = userData + "temp/";
        if(isdir(tempDataPath.c_str()) == true) {
        	removeFolder(tempDataPath);
        }
        createDirectoryPaths(tempDataPath);

	    if( hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_DISABLE_SOUND]) == true ||
	    	hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_MASTERSERVER_MODE])) == true) {
	    	config.setString("FactorySound","None");
	    	if(hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_MASTERSERVER_MODE])) == true) {
	    		Logger::getInstance().setMasterserverMode(true);
	    		Model::setMasterserverMode(true);
	    		Shared::Sound::Sound::setMasterserverMode(true);
	    	}
	    }

	    bool enableATIHacks = config.getBool("EnableATIHacks","false");
	    if(enableATIHacks == true) {
	    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("**WARNING** Enabling ATI video card hacks\n");
	    	TextureGl::setEnableATIHacks(enableATIHacks);
	    }

        if(config.getBool("ForceFTGLFonts","false") == true || hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_FORCE_FTGLFONTS]) == true) {
        	Font::forceFTGLFonts = true;
        	//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("**WARNING** Forcing Legacy Fonts Enabled\n");
        	printf("**WARNING** Forcing use of FTGL Fonts\n");
        }
        else {
        	Renderer::renderText3DEnabled = config.getBool("Enable3DFontRendering",intToStr(Renderer::renderText3DEnabled).c_str());
        }

        if(config.getBool("EnableLegacyFonts","false") == true || hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_ENABLE_LEGACYFONTS]) == true) {
        	Font::forceLegacyFonts = true;
        	Renderer::renderText3DEnabled = false;
        	//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("**WARNING** Forcing Legacy Fonts Enabled\n");
        	printf("**WARNING** Forcing Legacy Fonts Enabled\n");
        }
        else {
        	Renderer::renderText3DEnabled = config.getBool("Enable3DFontRendering",intToStr(Renderer::renderText3DEnabled).c_str());
        }

        if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_USE_RESOLUTION]) == true) {
			int foundParamIndIndex = -1;
			hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_USE_RESOLUTION]) + string("="),&foundParamIndIndex);
			if(foundParamIndIndex < 0) {
				hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_USE_RESOLUTION]),&foundParamIndIndex);
			}
			string paramValue = argv[foundParamIndIndex];
			vector<string> paramPartTokens;
			Tokenize(paramValue,paramPartTokens,"=");
			if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
				string settings = paramPartTokens[1];
				printf("Forcing resolution [%s]\n",settings.c_str());

				paramPartTokens.clear();
				Tokenize(settings,paramPartTokens,"x");
				if(paramPartTokens.size() >= 2) {
					int newScreenWidth 	= strToInt(paramPartTokens[0]);
					config.setInt("ScreenWidth",newScreenWidth);

					int newScreenHeight = strToInt(paramPartTokens[1]);
					config.setInt("ScreenHeight",newScreenHeight);
				}
				else {
		            printf("\nInvalid missing resolution settings specified on commandline [%s] value [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
		            //printParameterHelp(argv[0],false);
		            return -1;
				}
			}
	        else {
	            printf("\nInvalid missing resolution setting specified on commandline [%s] value [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
	            //printParameterHelp(argv[0],false);
	            return -1;
	        }
        }

		if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_USE_COLORBITS]) == true) {
			int foundParamIndIndex = -1;
			hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_USE_COLORBITS]) + string("="),&foundParamIndIndex);
			if(foundParamIndIndex < 0) {
				hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_USE_COLORBITS]),&foundParamIndIndex);
			}
			string paramValue = argv[foundParamIndIndex];
			vector<string> paramPartTokens;
			Tokenize(paramValue,paramPartTokens,"=");
			if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
				string settings = paramPartTokens[1];
				printf("Forcing colorbits [%s]\n",settings.c_str());

				int newColorBits = strToInt(settings);
				config.setInt("ColorBits",newColorBits);
			}
			else {
				printf("\nInvalid missing colorbits settings specified on commandline [%s] value [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
				//printParameterHelp(argv[0],false);
				return -1;
			}
		}

		if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_USE_DEPTHBITS]) == true) {
			int foundParamIndIndex = -1;
			hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_USE_DEPTHBITS]) + string("="),&foundParamIndIndex);
			if(foundParamIndIndex < 0) {
				hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_USE_DEPTHBITS]),&foundParamIndIndex);
			}
			string paramValue = argv[foundParamIndIndex];
			vector<string> paramPartTokens;
			Tokenize(paramValue,paramPartTokens,"=");
			if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
				string settings = paramPartTokens[1];
				printf("Forcing depthbits [%s]\n",settings.c_str());

				int newDepthBits = strToInt(settings);
				config.setInt("DepthBits",newDepthBits);
			}
			else {
				printf("\nInvalid missing depthbits setting specified on commandline [%s] value [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
				//printParameterHelp(argv[0],false);
				return -1;
			}
		}

		if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_USE_FULLSCREEN]) == true) {
			int foundParamIndIndex = -1;
			hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_USE_FULLSCREEN]) + string("="),&foundParamIndIndex);
			if(foundParamIndIndex < 0) {
				hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_USE_FULLSCREEN]),&foundParamIndIndex);
			}
			string paramValue = argv[foundParamIndIndex];
			vector<string> paramPartTokens;
			Tokenize(paramValue,paramPartTokens,"=");
			if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
				string settings = paramPartTokens[1];
				printf("Forcing fullscreen [%s]\n",settings.c_str());

				bool newFullScreenMode = strToBool(settings);
				config.setBool("Windowed",!newFullScreenMode);
			}
			else {
				printf("\nInvalid missing fullscreen setting specified on commandline [%s] value [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
				//printParameterHelp(argv[0],false);
				return -1;
			}
		}

        // Set some statics based on ini entries
		SystemFlags::ENABLE_THREADED_LOGGING = config.getBool("ThreadedLogging","true");
		FontGl::setDefault_fontType(config.getString("DefaultFont",FontGl::getDefault_fontType().c_str()));
		UPNP_Tools::isUPNP = !config.getBool("DisableUPNP","false");
		Texture::useTextureCompression = config.getBool("EnableTextureCompression","false");

		// 256 for English
		// 30000 for Chinese
		Font::charCount    		= config.getInt("FONT_CHARCOUNT",intToStr(Font::charCount).c_str());
		Font::fontTypeName 		= config.getString("FONT_TYPENAME",Font::fontTypeName.c_str());
		Font::fontIsMultibyte 	= config.getBool("FONT_MULTIBYTE",intToStr(Font::fontIsMultibyte).c_str());
		Font::fontIsRightToLeft	= config.getBool("FONT_RIGHTTOLEFT",intToStr(Font::fontIsRightToLeft).c_str());
		Font::baseSize			= config.getInt("FONT_BASE_SIZE",intToStr(Font::baseSize).c_str());
		Font::scaleFontValue				= config.getFloat("FONT_SCALE_SIZE",floatToStr(Font::scaleFontValue).c_str());
		Font::scaleFontValueCenterHFactor	= config.getFloat("FONT_SCALE_CENTERH_FACTOR",floatToStr(Font::scaleFontValueCenterHFactor).c_str());
		Font::langHeightText				= config.getString("FONT_HEIGHT_TEXT",Font::langHeightText.c_str());

		// Example values:
		// DEFAULT_CHARSET (English) = 1
		// GB2312_CHARSET (Chinese)  = 134
		Shared::Platform::charSet = config.getInt("FONT_CHARSET",intToStr(Shared::Platform::charSet).c_str());
		if(config.getBool("No2DMouseRendering","false") == false) {
			showCursor(false);
		}
		if(config.getInt("DEFAULT_HTTP_TIMEOUT",intToStr(SystemFlags::DEFAULT_HTTP_TIMEOUT).c_str()) >= 0) {
			SystemFlags::DEFAULT_HTTP_TIMEOUT = config.getInt("DEFAULT_HTTP_TIMEOUT",intToStr(SystemFlags::DEFAULT_HTTP_TIMEOUT).c_str());
		}

		bool allowAltEnterFullscreenToggle = config.getBool("AllowAltEnterFullscreenToggle",boolToStr(Window::getAllowAltEnterFullscreenToggle()).c_str());
		Window::setAllowAltEnterFullscreenToggle(allowAltEnterFullscreenToggle);

		if(config.getBool("noTeamColors","false") == true) {
			MeshCallbackTeamColor::noTeamColors = true;
		}

        // Setup debug logging etc
		setupLogging(config, haveSpecialOutputCommandLineOption);

        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Font::charCount = %d, Font::fontTypeName [%s] Shared::Platform::charSet = %d, Font::fontIsMultibyte = %d, fontIsRightToLeft = %d\n",__FILE__,__FUNCTION__,__LINE__,Font::charCount,Font::fontTypeName.c_str(),Shared::Platform::charSet,Font::fontIsMultibyte, Font::fontIsRightToLeft);

		NetworkInterface::setDisplayMessageFunction(ExceptionHandler::DisplayMessage);
		MenuStateMasterserver::setDisplayMessageFunction(ExceptionHandler::DisplayMessage);

#ifdef USE_STREFLOP
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s, STREFLOP enabled.\n",getNetworkVersionString().c_str());
#else
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s, STREFLOP NOT enabled.\n",getNetworkVersionString().c_str());
#endif

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		SystemFlags::OutputDebug(SystemFlags::debugUnitCommands,"START\n");
		SystemFlags::OutputDebug(SystemFlags::debugPathFinder,"START\n");

        // Setup hotkeys from key ini files
		Config &configKeys = Config::getInstance(
				std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys),
				std::pair<string,string>(Config::glestkeys_ini_filename,Config::glestuserkeys_ini_filename),
				std::pair<bool,bool>(true,false));

        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

        if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_SHOW_INI_SETTINGS]) == true) {
            ShowINISettings(argc,argv,config,configKeys);
            return -1;
        }

        //setVBOSupported(false);
        // Explicitly disable VBO's
        if(config.getBool("DisableVBO","false") == true || hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_DISABLE_VBO]) == true) {
        	setVBOSupported(false);
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("**WARNING** Disabling VBO's\n");
        }

        if(config.getBool("EnableVSynch","false") == true) {
        	Window::setTryVSynch(true);
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("**ENABLED OPENGL VSYNCH**\n");
        }

		if(hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_MASTERSERVER_MODE])) == true) {
			Renderer &renderer= Renderer::getInstance(true);
		}

    	if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_USE_PORTS]) == true) {
			int foundParamIndIndex = -1;
			hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_USE_PORTS]) + string("="),&foundParamIndIndex);
			if(foundParamIndIndex < 0) {
				hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_USE_PORTS]),&foundParamIndIndex);
			}
			string paramValue = argv[foundParamIndIndex];
			vector<string> paramPartTokens;
			Tokenize(paramValue,paramPartTokens,"=");
			if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
				string portsToUse = paramPartTokens[1];

				vector<string> paramPartPortsTokens;
				Tokenize(portsToUse,paramPartPortsTokens,",");
				if(paramPartPortsTokens.size() >= 2 && paramPartPortsTokens[1].length() > 0) {
					int internalPort = strToInt(paramPartPortsTokens[0]);
					int externalPort = strToInt(paramPartPortsTokens[1]);

					printf("Forcing internal port# %d, external port# %d\n",internalPort,externalPort);

					config.setInt("ServerPort",internalPort);
					config.setInt("MasterServerExternalPort",externalPort);
					config.setInt("FTPServerPort",internalPort+1);
				}
				else {
		            printf("\nInvalid ports specified on commandline [%s] value [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
		            //printParameterHelp(argv[0],false);
		            return -1;
				}
			}
	        else {
	            printf("\nInvalid missing ports specified on commandline [%s] value [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
	            //printParameterHelp(argv[0],false);
	            return -1;
	        }
    	}

		//float pingTime = Socket::getAveragePingMS("soft-haus.com");
		//printf("Ping time = %f\n",pingTime);

        // Load the language strings
        Lang &lang= Lang::getInstance();
        string language = config.getString("Lang");
    	if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_USE_LANGUAGE]) == true) {
			int foundParamIndIndex = -1;
			hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_USE_LANGUAGE]) + string("="),&foundParamIndIndex);
			if(foundParamIndIndex < 0) {
				hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_USE_LANGUAGE]),&foundParamIndIndex);
			}
			string paramValue = argv[foundParamIndIndex];
			vector<string> paramPartTokens;
			Tokenize(paramValue,paramPartTokens,"=");
			if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
				language = paramPartTokens[1];
				printf("Forcing language [%s]\n",language.c_str());
			}
	        else {
	            printf("\nInvalid missing language specified on commandline [%s] value [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
	            //printParameterHelp(argv[0],false);
	            return -1;
	        }
    	}
    	else {
    		char *lang_locale = setlocale(LC_ALL,"");
    		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Locale is: [%s]\n", lang_locale);

    		if(lang_locale != NULL && strlen(lang_locale) >= 2) {
    			if(config.getBool("AutoLocaleLanguageDetect","true") == true) {
    				language = lang_locale;
    				language = language.substr(0,2);
    				printf("Auto setting language [%s]\n",language.c_str());

    				config.setBool("AutoLocaleLanguageDetect",false);
    			}
    		}
    	}

    	Renderer &renderer= Renderer::getInstance();
        lang.loadStrings(language,false, true);

        if(	lang.hasString("FONT_HEIGHT_TEXT")) {
        	Font::langHeightText = config.getString("FONT_HEIGHT_TEXT",Font::langHeightText.c_str());
        }

    	if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_FONT_BASESIZE]) == true) {
			int foundParamIndIndex = -1;
			hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_FONT_BASESIZE]) + string("="),&foundParamIndIndex);
			if(foundParamIndIndex < 0) {
				hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_FONT_BASESIZE]),&foundParamIndIndex);
			}
			string paramValue = argv[foundParamIndIndex];
			vector<string> paramPartTokens;
			Tokenize(paramValue,paramPartTokens,"=");
			if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
				string newfontBaseSize = paramPartTokens[1];
				//printf("#1 Forcing font [%s] paramPartTokens.size() = %d, paramValue [%s]\n",newfont.c_str(),paramPartTokens.size(),paramValue.c_str());
				printf("Forcing font base size[%s]\n",newfontBaseSize.c_str());

				Font::baseSize = strToInt(newfontBaseSize);
			}
	        else {
	            printf("\nInvalid missing font base size specified on commandline [%s] value [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
	            //printParameterHelp(argv[0],false);

	            return -1;
	        }
    	}

        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Font::charCount = %d, Font::fontTypeName [%s] Shared::Platform::charSet = %d, Font::fontIsMultibyte = %d, Font::fontIsRightToLeft = %d\n",__FILE__,__FUNCTION__,__LINE__,Font::charCount,Font::fontTypeName.c_str(),Shared::Platform::charSet,Font::fontIsMultibyte,Font::fontIsRightToLeft);
        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Using Font::charCount = %d, Font::fontTypeName [%s] Shared::Platform::charSet = %d, Font::fontIsMultibyte = %d, Font::fontIsRightToLeft = %d\n",Font::charCount,Font::fontTypeName.c_str(),Shared::Platform::charSet,Font::fontIsMultibyte,Font::fontIsRightToLeft);

    	if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_USE_FONT]) == true) {
			int foundParamIndIndex = -1;
			hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_USE_FONT]) + string("="),&foundParamIndIndex);
			if(foundParamIndIndex < 0) {
				hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_USE_FONT]),&foundParamIndIndex);
			}
			string paramValue = argv[foundParamIndIndex];
			vector<string> paramPartTokens;
			Tokenize(paramValue,paramPartTokens,"=");
			if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
				string newfont = paramPartTokens[1];
				//printf("#1 Forcing font [%s] paramPartTokens.size() = %d, paramValue [%s]\n",newfont.c_str(),paramPartTokens.size(),paramValue.c_str());
				Properties::applyTagsToValue(newfont);
				printf("Forcing font [%s]\n",newfont.c_str());

#if defined(WIN32)
				string newEnvValue = "MEGAGLEST_FONT=" + newfont;
				_putenv(newEnvValue.c_str());
#else
        		setenv("MEGAGLEST_FONT",newfont.c_str(),1);
#endif
			}
	        else {
	            printf("\nInvalid missing font specified on commandline [%s] value [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
	            //printParameterHelp(argv[0],false);
	            return -1;
	        }
    	}

        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    	if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_SHOW_MAP_CRC]) == true) {
			int foundParamIndIndex = -1;
			hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_SHOW_MAP_CRC]) + string("="),&foundParamIndIndex);
			if(foundParamIndIndex < 0) {
				hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_SHOW_MAP_CRC]),&foundParamIndIndex);
			}
			string paramValue = argv[foundParamIndIndex];
			vector<string> paramPartTokens;
			Tokenize(paramValue,paramPartTokens,"=");
			if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
				string itemName = paramPartTokens[1];

				string file = Map::getMapPath(itemName,"",false);
				if(file != "") {
					Checksum checksum;
					checksum.addFile(file);
					int32 crcValue = checksum.getSum();

					printf("CRC value for map [%s] file [%s] is [%d]\n",itemName.c_str(),file.c_str(),crcValue);
				}
				else {
					printf("Map [%s] was NOT FOUND\n",itemName.c_str());
				}

				return -1;
			}
	        else {
	            printf("\nInvalid missing map specified on commandline [%s] value [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
	            //printParameterHelp(argv[0],false);

	            return -1;
	        }
    	}

    	if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_SHOW_TILESET_CRC]) == true) {
			int foundParamIndIndex = -1;
			hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_SHOW_TILESET_CRC]) + string("="),&foundParamIndIndex);
			if(foundParamIndIndex < 0) {
				hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_SHOW_TILESET_CRC]),&foundParamIndIndex);
			}
			string paramValue = argv[foundParamIndIndex];
			vector<string> paramPartTokens;
			Tokenize(paramValue,paramPartTokens,"=");
			if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
				string itemName = paramPartTokens[1];
				int32 crcValue = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTilesets,""), string("/") + itemName + string("/*"), ".xml", NULL, true);
				if(crcValue != 0) {
					printf("CRC value for tileset [%s] is [%d]\n",itemName.c_str(),crcValue);
				}
				else {
					printf("Tileset [%s] was NOT FOUND\n",itemName.c_str());
				}

				return -1;
			}
	        else {
	            printf("\nInvalid missing tileset specified on commandline [%s] value [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
	            //printParameterHelp(argv[0],false);

	            return -1;
	        }
    	}

    	if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_SHOW_TECHTREE_CRC]) == true) {
			int foundParamIndIndex = -1;
			hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_SHOW_TECHTREE_CRC]) + string("="),&foundParamIndIndex);
			if(foundParamIndIndex < 0) {
				hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_SHOW_TECHTREE_CRC]),&foundParamIndIndex);
			}
			string paramValue = argv[foundParamIndIndex];
			vector<string> paramPartTokens;
			Tokenize(paramValue,paramPartTokens,"=");
			if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
				string itemName = paramPartTokens[1];
				int32 crcValue = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,""), "/" + itemName + "/*", ".xml", NULL, true);
				if(crcValue != 0) {
					printf("CRC value for techtree [%s] is [%d]\n",itemName.c_str(),crcValue);
				}
				else {
					printf("Techtree [%s] was NOT FOUND\n",itemName.c_str());
				}

				return -1;
			}
	        else {
	            printf("\nInvalid missing techtree specified on commandline [%s] value [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
	            //printParameterHelp(argv[0],false);

	            return -1;
	        }
    	}

    	if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_SHOW_SCENARIO_CRC]) == true) {
			int foundParamIndIndex = -1;
			hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_SHOW_SCENARIO_CRC]) + string("="),&foundParamIndIndex);
			if(foundParamIndIndex < 0) {
				hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_SHOW_SCENARIO_CRC]),&foundParamIndIndex);
			}
			string paramValue = argv[foundParamIndIndex];
			vector<string> paramPartTokens;
			Tokenize(paramValue,paramPartTokens,"=");
			if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
				string itemName = paramPartTokens[1];
				int32 crcValue = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptScenarios,""), "/" + itemName + "/*", ".xml", NULL, true);
				if(crcValue != 0) {
					printf("CRC value for scenario [%s] is [%d]\n",itemName.c_str(),crcValue);
				}
				else {
					printf("Scenario [%s] was NOT FOUND\n",itemName.c_str());
				}

				return -1;
			}
	        else {
	            printf("\nInvalid missing scenario specified on commandline [%s] value [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
	            //printParameterHelp(argv[0],false);

	            return -1;
	        }
    	}

    	if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_SHOW_PATH_CRC]) == true) {
			int foundParamIndIndex = -1;
			hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_SHOW_PATH_CRC]) + string("="),&foundParamIndIndex);
			if(foundParamIndIndex < 0) {
				hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_SHOW_PATH_CRC]),&foundParamIndIndex);
			}

			string paramValue = argv[foundParamIndIndex];
			vector<string> paramPartTokens;
			Tokenize(paramValue,paramPartTokens,"=");
			if(paramPartTokens.size() >= 3 && paramPartTokens[1].length() > 0) {
				string itemName = paramPartTokens[1];
				string itemNameFilter = paramPartTokens[2];
				//printf("\n\nitemName [%s] itemNameFilter [%s]\n",itemName.c_str(),itemNameFilter.c_str());
				int32 crcValue = getFolderTreeContentsCheckSumRecursively(itemName, itemNameFilter, NULL, true);

				printf("CRC value for path [%s] filter [%s] is [%d]\n",itemName.c_str(),itemNameFilter.c_str(),crcValue);

				return -1;
			}
	        else {
	        	if(paramPartTokens.size() < 2) {
	        		printf("\nInvalid missing path and filter specified on commandline [%s] value [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
	        	}
	        	if(paramPartTokens.size() < 3) {
	        		printf("\nInvalid missing filter specified on commandline [%s] value [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 3 ? paramPartTokens[2].c_str() : NULL));
	        	}

	            //printParameterHelp(argv[0],false);

	            return -1;
	        }
    	}

		//vector<string> techPaths;
		//vector<string> techDataPaths = config.getPathListForType(ptTechs);
		//findDirs(techDataPaths, techPaths);

        //int32 techCRC = getFolderTreeContentsCheckSumRecursively(techDataPaths, string("/") + "megapack" + string("/*"), ".xml", NULL, true);
        //return -1;

        //
        //removeFolder("/home/softcoder/Code/megaglest/trunk/mk/linux/mydata/tilesets/mother_board");
        //return -1;
        //

		program= new Program();

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		mainWindow= new MainWindow(program);

		mainWindow->setUseDefaultCursorOnly(config.getBool("No2DMouseRendering","false"));

        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

        GameSettings startupGameSettings;

		//parse command line
		if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_SERVER]) == true) {
			program->initServer(mainWindow,false,true);
		}
		else if(hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_MASTERSERVER_MODE])) == true) {
			program->initServer(mainWindow,false,true,true);
		}
		else if(hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_AUTOSTART_LASTGAME])) == true) {
			program->initServer(mainWindow,true,false);
		}
		else if(hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_PREVIEW_MAP])) == true) {
			int foundParamIndIndex = -1;
			hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_PREVIEW_MAP]) + string("="),&foundParamIndIndex);
			if(foundParamIndIndex < 0) {
				hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_PREVIEW_MAP]),&foundParamIndIndex);
			}
			string mapName = argv[foundParamIndIndex];
			vector<string> paramPartTokens;
			Tokenize(mapName,paramPartTokens,"=");
			if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
				string autoloadMapName = paramPartTokens[1];

				GameSettings *gameSettings = &startupGameSettings;
				int factionCount= 0;
				gameSettings->setMap(autoloadMapName);
				gameSettings->setTileset("forest");
				gameSettings->setTech("megapack");
				gameSettings->setDefaultUnits(false);
				gameSettings->setDefaultResources(false);
				gameSettings->setDefaultVictoryConditions(true);
				gameSettings->setFogOfWar(false);
				gameSettings->setAllowObservers(true);
				gameSettings->setPathFinderType(pfBasic);

				for(int i = 0; i < GameConstants::maxPlayers; ++i) {
					ControlType ct= ctClosed;

					gameSettings->setNetworkPlayerStatuses(i, 0);
					gameSettings->setFactionControl(i, ct);
					gameSettings->setStartLocationIndex(i, i);
					gameSettings->setResourceMultiplierIndex(i, 10);
					gameSettings->setNetworkPlayerName(i, "Closed");
				}

				ControlType ct= ctHuman;

				gameSettings->setNetworkPlayerStatuses(0, 0);
				gameSettings->setFactionControl(0, ct);
				gameSettings->setFactionTypeName(0, formatString(GameConstants::OBSERVER_SLOTNAME));
				gameSettings->setTeam(0, GameConstants::maxPlayers + fpt_Observer - 1);
				gameSettings->setStartLocationIndex(0, 0);
				gameSettings->setNetworkPlayerName(0, GameConstants::OBSERVER_SLOTNAME);

				gameSettings->setFactionCount(1);

				Config &config = Config::getInstance();
				gameSettings->setEnableServerControlledAI(config.getBool("ServerControlledAI","true"));
				gameSettings->setNetworkFramePeriod(config.getInt("NetworkSendFrameCount","20"));

				program->initServer(mainWindow,gameSettings);
			}
			else {
				printf("\nInvalid map name specified on commandline [%s] map [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
				printParameterHelp(argv[0],foundInvalidArgs);
				delete mainWindow;
				return -1;
			}
		}
		else if(hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_CLIENT])) == true) {
			int foundParamIndIndex = -1;
			hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_CLIENT]) + string("="),&foundParamIndIndex);
			if(foundParamIndIndex < 0) {
				hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_CLIENT]),&foundParamIndIndex);
			}
			string serverToConnectTo = argv[foundParamIndIndex];
			vector<string> paramPartTokens;
			Tokenize(serverToConnectTo,paramPartTokens,"=");
			if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
				string autoConnectServer = paramPartTokens[1];
				program->initClient(mainWindow, autoConnectServer);
			}
			else {

				printf("\nInvalid host specified on commandline [%s] host [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
				printParameterHelp(argv[0],foundInvalidArgs);
				delete mainWindow;
				return -1;
			}
		}
		else if(hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_LOADSCENARIO])) == true) {

			int foundParamIndIndex = -1;
			hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_LOADSCENARIO]) + string("="),&foundParamIndIndex);
			if(foundParamIndIndex < 0) {
				hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_LOADSCENARIO]),&foundParamIndIndex);
			}
			string scenarioName = argv[foundParamIndIndex];
			vector<string> paramPartTokens;
			Tokenize(scenarioName,paramPartTokens,"=");
			if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
				string autoloadScenarioName = paramPartTokens[1];
				program->initScenario(mainWindow, autoloadScenarioName);
			}
			else {
				printf("\nInvalid scenario name specified on commandline [%s] scenario [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
				printParameterHelp(argv[0],foundInvalidArgs);
				delete mainWindow;
				return -1;
			}
		}
		else {
			program->initNormal(mainWindow);
		}

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

        // Initialize Renderer
		//Renderer &renderer= Renderer::getInstance();
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] OpenGL Info:\n%s\n",__FILE__,__FUNCTION__,__LINE__,renderer.getGlInfo().c_str());

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPathFinder).enabled == true) {
			renderer.setAllowRenderUnitTitles(SystemFlags::getSystemSettingType(SystemFlags::debugPathFinder).enabled);
			SystemFlags::OutputDebug(SystemFlags::debugPathFinder,"In [%s::%s Line: %d] renderer.setAllowRenderUnitTitles = %d\n",__FILE__,__FUNCTION__,__LINE__,SystemFlags::getSystemSettingType(SystemFlags::debugPathFinder).enabled);
		}
		renderer.setAllowRenderUnitTitles(true);

		if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_OPENGL_INFO]) == true) {
			//Renderer &renderer= Renderer::getInstance();
			printf("%s",renderer.getGlInfo().c_str());
			printf("%s",renderer.getGlMoreInfo().c_str());

			delete mainWindow;
			return -1;
		}

    	if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_CONVERT_MODELS]) == true) {
			int foundParamIndIndex = -1;
			hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_CONVERT_MODELS]) + string("="),&foundParamIndIndex);
			if(foundParamIndIndex < 0) {
				hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_CONVERT_MODELS]),&foundParamIndIndex);
			}
			string paramValue = argv[foundParamIndIndex];
			vector<string> paramPartTokens;
			Tokenize(paramValue,paramPartTokens,"=");
			if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
				string modelFile = paramPartTokens[1];
				printf("About to convert model(s) [%s]\n",modelFile.c_str());

				string textureFormat = "";
				if(paramPartTokens.size() >= 3 && paramPartTokens[1].length() > 0) {
					textureFormat = paramPartTokens[2];
					printf("About to convert using texture format [%s]\n",textureFormat.c_str());
				}

				bool keepsmallest = false;
				if(paramPartTokens.size() >= 4 && paramPartTokens[1].length() > 0) {
					keepsmallest = (paramPartTokens[3] == "keepsmallest");
					printf("About to convert using keepsmallest = %d\n",keepsmallest);
				}

				showCursor(true);
				mainWindow->setUseDefaultCursorOnly(true);

				const Metrics &metrics= Metrics::getInstance();
				renderer.clearBuffers();
				renderer.clearZBuffer();
				renderer.reset2d();
				renderer.renderText(
						"Please wait, converting models...",
						CoreData::getInstance().getMenuFontBig(),
						Vec3f(1.f, 1.f, 0.f), (metrics.getScreenW() / 2) - 400,
						(metrics.getScreenH() / 2), true);
			    renderer.swapBuffers();

				std::vector<string> models;
				if(isdir(modelFile.c_str()) == true) {
					models = getFolderTreeContentsListRecursively(modelFile, ".g3d");
				}
				else {
					models.push_back(modelFile);
				}

			    sleep(0);
			    Window::handleEvent();
				SDL_PumpEvents();

				char szTextBuf[1024]="";
				for(unsigned int i =0; i < models.size(); ++i) {
					string &file = models[i];
					bool modelLoadedOk = false;

					renderer.clearBuffers();
					renderer.clearZBuffer();
					renderer.reset2d();
				    sprintf(szTextBuf,"Please wait, converting models [%d of %lu] ...",i,(long int)models.size());
					renderer.renderText(
							szTextBuf,
							CoreData::getInstance().getMenuFontBig(),
							Vec3f(1.f, 1.f, 0.f), (metrics.getScreenW() / 2) - 400,
							(metrics.getScreenH() / 2), true);
				    renderer.swapBuffers();

				    sleep(0);
				    Window::handleEvent();
					SDL_PumpEvents();

					Model *model = renderer.newModel(rsGlobal);
					try {
						printf("About to load model [%s] [%d of %lu]\n",file.c_str(),i,(long int)models.size());
						model->load(file);
						modelLoadedOk = true;
					}
					catch(const exception &ex) {
						printf("ERROR loading model [%s] message [%s]\n",file.c_str(),ex.what());
					}

					if(modelLoadedOk == true) {
						printf("About to save converted model [%s]\n",file.c_str());
						model->save(file,textureFormat,keepsmallest);
					}

					Renderer::getInstance().endModel(rsGlobal, model);
				}

				delete mainWindow;
				return -1;
			}
			else {
				printf("\nInvalid model specified on commandline [%s] texture [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
				printParameterHelp(argv[0],foundInvalidArgs);
				delete mainWindow;
				return -1;
			}
    	}

		if(	hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_VALIDATE_TECHTREES]) 	== true ||
			hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_VALIDATE_FACTIONS]) 	== true ||
			hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_VALIDATE_SCENARIO])    == true) {

			runTechValidationReport(argc, argv);

		    delete mainWindow;
		    return -1;
		}

		gameInitialized = true;

        // Setup the screenshots folder
        if(userData != "") {
        	endPathWithSlash(userData);
        }

		string screenShotsPath = userData + GameConstants::folder_path_screenshots;
        if(isdir(screenShotsPath.c_str()) == false) {
        	createDirectoryPaths(screenShotsPath);
        }

        // Cache Player textures - START
        string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
		std::map<int,Texture2D *> &crcPlayerTextureCache = CacheManager::getCachedItem< std::map<int,Texture2D *> >(GameConstants::playerTextureCacheLookupKey);
        for(int index = 0; index < GameConstants::maxPlayers; ++index) {
        	string playerTexture = data_path + "data/core/faction_textures/faction" + intToStr(index) + ".tga";
        	if(fileExists(playerTexture) == true) {
        		Texture2D *texture = Renderer::getInstance().newTexture2D(rsGlobal);
        		if(texture) {
        			texture->load(playerTexture);
        		}
        		crcPlayerTextureCache[index] = texture;
        	}
        	else {
        		crcPlayerTextureCache[index] = NULL;
        	}
        }
        // Cache Player textures - END

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

        CheckForDuplicateData();

        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//throw "BLAH!";

        // START Test out SIGSEGV error handling
        //int *foo = (int*)-1; // make a bad pointer
        //printf("%d\n", *foo);       // causes segfault
        // END
        bool startCRCPrecacheThread = config.getBool("PreCacheCRCThread","true");
        //printf("### In [%s::%s Line: %d] precache thread enabled = %d SystemFlags::VERBOSE_MODE_ENABLED = %d\n",__FILE__,__FUNCTION__,__LINE__,startCRCPrecacheThread,SystemFlags::VERBOSE_MODE_ENABLED);
        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] precache thread enabled = %d\n",__FILE__,__FUNCTION__,__LINE__,startCRCPrecacheThread);
		if(startCRCPrecacheThread == true) {
			vector<string> techDataPaths = config.getPathListForType(ptTechs);
			preCacheThread = new FileCRCPreCacheThread();
			preCacheThread->setUniqueID(__FILE__);
			preCacheThread->setTechDataPaths(techDataPaths);
			//preCacheThread->setFileCRCPreCacheThreadCallbackInterface(&preCacheThreadGame);
			preCacheThread->start();
		}

        // test
        //Shared::Platform::MessageBox(NULL,"Mark's test.","Test",0);
        //throw runtime_error("test!");
        //ExceptionHandler::DisplayMessage("test!", false);

		//Lang &lang= Lang::getInstance();
		//string test = lang.get("ExitGameServer?");
		//printf("[%s]",test.c_str());

		//time_t lastTextureLoadEvent = time(NULL);

		// Check for commands being input from stdin
		string command="";

#ifndef WIN32
		pollfd cinfd[1];
#else
		HANDLE h = 0;
#endif
		if(disableheadless_console == false) {
#ifndef WIN32
			// Theoretically this should always be 0, but one fileno call isn't going to hurt, and if
			// we try to run somewhere that stdin isn't fd 0 then it will still just work
			cinfd[0].fd = fileno(stdin);
			cinfd[0].events = POLLIN;
#else
			h = GetStdHandle(STD_INPUT_HANDLE);
#endif
		}

	    if(isMasterServerModeEnabled == true) {
	    	printf("Headless server is now running...\n");
	    }

	    //throw runtime_error("Test!");

		//main loop
		while(program->isShutdownApplicationEnabled() == false && Window::handleEvent()) {
			if(isMasterServerModeEnabled == true) {

				if(disableheadless_console == false) {
				#ifndef WIN32
					int pollresult = poll(cinfd, 1, 0);
					int pollerror = errno;
					if(pollresult)
				#else
					// This is problematic because input on Windows is not line-buffered so this will return
					// even if getline may block.  I haven't found a good way to fix it, so for the moment
					// I just strongly suggest only running the server from the Python frontend, which does
					// line buffer input.  This does work okay as long as the user doesn't enter characters
					// without pressing enter, and then try to end the server another way (say a remote
					// console command), in which case we'll still be waiting for the stdin EOL and hang.
					if (WaitForSingleObject(h, 0) == WAIT_OBJECT_0)
				#endif
					{

						getline(cin, command);
						cin.clear();

						printf("server command [%s]\n",command.c_str());
						if(command == "quit") {
							break;
						}

#ifndef WIN32
						if(pollresult < 0) {
							printf("pollresult = %d errno = %d [%s]\n",pollresult,pollerror,strerror(pollerror));

							cinfd[0].fd = fileno(stdin);
							cinfd[0].events = POLLIN;
						}
#endif
					}
				}
				//printf("looping\n");
			}

			program->loop();

			// Because OpenGL really doesn't do multi-threading well
//			if(difftime(time(NULL),lastTextureLoadEvent) >= 3) {
//				lastTextureLoadEvent = time(NULL);
//				vector<Texture2D *> textureList = preCacheThread->getPendingTextureList(1);
//				for(unsigned int i = 0; i < textureList.size(); ++i) {
//					Texture2D * factionLogo = textureList[i];
//					if(factionLogo != NULL) {
//						printf("\n\n\n\n|||||||||||||||||||||||||| Load texture [%s]\n",factionLogo->getPath().c_str());
//						//Renderer::findFactionLogoTexture(factionLogo);
//						renderer.initTexture(rsGlobal,factionLogo);
//					}
//				}
//			}
		}

	    if(isMasterServerModeEnabled == true) {
	    	printf("\nHeadless server is about to quit...\n");
	    }

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] starting normal application shutdown\n",__FILE__,__FUNCTION__,__LINE__);

		if(isMasterServerModeEnabled == false) {
			soundThreadManager = program->getSoundThreadManager(true);
			if(soundThreadManager) {
				SoundRenderer &soundRenderer= SoundRenderer::getInstance();
				soundRenderer.stopAllSounds(shutdownFadeSoundMilliseconds);
				chronoshutdownFadeSound.start();
			}
		}

		cleanupCRCThread();
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	    showCursor(true);
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
	catch(const exception &e) {
		if(isMasterServerModeEnabled == false) {
			soundThreadManager = (program != NULL ? program->getSoundThreadManager(true) : NULL);
			if(soundThreadManager) {
				SoundRenderer &soundRenderer= SoundRenderer::getInstance();
				soundRenderer.stopAllSounds(shutdownFadeSoundMilliseconds);
				chronoshutdownFadeSound.start();
			}
		}

		ExceptionHandler::handleRuntimeError(e.what());
	}
	catch(const char *e) {
		if(isMasterServerModeEnabled == false) {
			soundThreadManager = (program != NULL ? program->getSoundThreadManager(true) : NULL);
			if(soundThreadManager) {
				SoundRenderer &soundRenderer= SoundRenderer::getInstance();
				soundRenderer.stopAllSounds(shutdownFadeSoundMilliseconds);
				chronoshutdownFadeSound.start();
			}
		}

		ExceptionHandler::handleRuntimeError(e);
	}
	catch(const string &ex) {
		if(isMasterServerModeEnabled == false) {
			soundThreadManager = (program != NULL ? program->getSoundThreadManager(true) : NULL);
			if(soundThreadManager) {
				SoundRenderer &soundRenderer= SoundRenderer::getInstance();
				soundRenderer.stopAllSounds(shutdownFadeSoundMilliseconds);
				chronoshutdownFadeSound.start();
			}
		}

		ExceptionHandler::handleRuntimeError(ex.c_str());
	}
	catch(...) {
		if(isMasterServerModeEnabled == false) {
			soundThreadManager = (program != NULL ? program->getSoundThreadManager(true) : NULL);
			if(soundThreadManager) {
				SoundRenderer &soundRenderer= SoundRenderer::getInstance();
				soundRenderer.stopAllSounds(shutdownFadeSoundMilliseconds);
				chronoshutdownFadeSound.start();
			}
		}

		ExceptionHandler::handleRuntimeError("Unknown error!");
	}

	cleanupCRCThread();

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	delete mainWindow;
	mainWindow = NULL;

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    //showCursor(true);
    //restoreVideoMode(true);

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	GraphicComponent::clearRegisteredComponents();

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(soundThreadManager) {
		SoundRenderer &soundRenderer= SoundRenderer::getInstance();
		if( Config::getInstance().getString("FactorySound","") != "None" &&
			soundRenderer.isVolumeTurnedOff() == false) {
			//printf("chronoshutdownFadeSound.getMillis() = %llu\n",chronoshutdownFadeSound.getMillis());
			for(;chronoshutdownFadeSound.getMillis() <= shutdownFadeSoundMilliseconds;) {
				sleep(10);
			}
		}

		BaseThread::shutdownAndWait(soundThreadManager);
		delete soundThreadManager;
		soundThreadManager = NULL;
	}

	return 0;
}

int glestMainWrapper(int argc, char** argv) {

	//setlocale(LC_ALL, "zh_TW.UTF-8");
	//setlocale(LC_ALL, "");

#if defined(__GNUC__) && !defined(__MINGW32__) && !defined(__FreeBSD__) && !defined(BSD)
//#ifdef DEBUG
	  //printf("MTRACE will be called...\n");
      //mtrace ();
//#endif
#endif

#ifdef WIN32_STACK_TRACE
__try {
#endif

    //application_binary= executable_path(argv[0],true);
    //printf("\n\nargv0 [%s] application_binary [%s]\n\n",argv[0],application_binary.c_str());

#if defined(__GNUC__) && !defined(__MINGW32__) && !defined(__FreeBSD__) && !defined(BSD)

	if(hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_DISABLE_SIGSEGV_HANDLER])) == false) {
		signal(SIGSEGV, handleSIGSEGV);
	}

    // http://developerweb.net/viewtopic.php?id=3013
    //signal(SIGPIPE, SIG_IGN);
#endif

	initSpecialStrings();
	int result = glestMain(argc, argv);

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	cleanupProcessObjects();

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    if(sdl_quitCalled == false) {
    	sdl_quitCalled = true;
    	SDL_Quit();
    }

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	return result;

#ifdef WIN32_STACK_TRACE
} __except(stackdumper(0, GetExceptionInformation()), EXCEPTION_CONTINUE_SEARCH) { return 0; }
#endif
}

}}//end namespace

MAIN_FUNCTION(Glest::Game::glestMainWrapper)
