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

// For gcc backtrace on crash!
#if defined(__GNUC__) && !defined(__MINGW32__) && !defined(__FreeBSD__) && !defined(BSD)
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

#include "leak_dumper.h"

#ifndef WIN32
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

namespace Glest{ namespace Game{

bool disableBacktrace = false;
bool gameInitialized = false;
static char *application_binary=NULL;

const char  *GAME_ARGS[] = {
	"--help",
	"--autostart-lastgame",
	"--connecthost",
	"--starthost",
	"--load-scenario",
	"--version",
	"--opengl-info",
	"--sdl-info",
	"--lua-info",
	"--curl-info",
	"--validate-techtrees",
	"--validate-factions",
	"--data-path",
	"--ini-path",
	"--log-path",
	"--show-ini-settings",
	"--disable-backtrace",
	"--disable-vbo",
	"--verbose"

};

enum GAME_ARG_TYPE {
	GAME_ARG_HELP = 0,
	GAME_ARG_AUTOSTART_LASTGAME,
	GAME_ARG_CLIENT,
	GAME_ARG_SERVER,
	GAME_ARG_LOADSCENARIO,
	GAME_ARG_VERSION,
	GAME_ARG_OPENGL_INFO,
	GAME_ARG_SDL_INFO,
	GAME_ARG_LUA_INFO,
	GAME_ARG_CURL_INFO,
	GAME_ARG_VALIDATE_TECHTREES,
	GAME_ARG_VALIDATE_FACTIONS,
	GAME_ARG_DATA_PATH,
	GAME_ARG_INI_PATH,
	GAME_ARG_LOG_PATH,
	GAME_ARG_SHOW_INI_SETTINGS,
	GAME_ARG_DISABLE_BACKTRACE,
	GAME_ARG_DISABLE_VBO,
	GAME_ARG_VERBOSE_MODE
};

string runtimeErrorMsg = "";

static void cleanupProcessObjects() {
    showCursor(true);
    restoreVideoMode(true);
    Renderer::getInstance().end();
	SystemFlags::Close();
	SystemFlags::SHUTDOWN_PROGRAM_MODE=true;
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
	    string sErr = string(GameConstants::application_name) + " fatal error";
		SystemFlags::OutputDebug(SystemFlags::debugError,"%s\n",errText.c_str());
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s\n",errText.c_str());

		if(errors <= 1) { // avoid recursion
            if(SDL_WasInit(SDL_INIT_VIDEO)) {
                SDL_ShowCursor(1);
                SDL_WM_GrabInput(SDL_GRAB_OFF);
                SDL_SetGamma(1, 1, 1);
            }
            #ifdef WIN32
                MessageBox(NULL, errText.c_str(), sErr.c_str(), MB_OK|MB_SYSTEMMODAL);
            #endif
            //SDL_Quit();
        }
    }

    // Now try to shutdown threads if possible
	Program *program = Program::getInstance();
    delete program;
    program = NULL;
    // END

    SDL_Quit();
    exit(EXIT_FAILURE);
}

void stackdumper(unsigned int type, EXCEPTION_POINTERS *ep) {
    if(!ep) fatal("unknown type");
    EXCEPTION_RECORD *er = ep->ExceptionRecord;
    CONTEXT *context = ep->ContextRecord;
    stringType out, t;
    formatstring(out)("%s Exception: 0x%x [0x%x]\n\n", GameConstants::application_name, er->ExceptionCode, er->ExceptionCode==EXCEPTION_ACCESS_VIOLATION ? er->ExceptionInformation[1] : -1);
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
		string msg = "#1 An error occurred and " + string(GameConstants::application_name) + " will close.\nPlease report this bug to "+mailString;
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
        static char buf[256]="";
        char *p=NULL;

        // prepare command to be executed
        // our program need to be passed after the -e parameter
        //sprintf (buf, "/usr/bin/addr2line -C -e ./a.out -f -i %lx", addr);
        sprintf (buf, "addr2line -C -e %s -f -i %p",application_binary,address);

        FILE* f = popen (buf, "r");

        if (f == NULL) {
            perror (buf);
            return 0;
        }

        // get function name
        char *ret = fgets (buf, 256, f);
        // get file and line
        ret = fgets (buf, 256, f);

        if (buf[0] != '?') {
            int l;
            char *p = buf;

            // file name is until ':'
            while (*p != ':')
            {
                p++;
            }

            *p++ = 0;
            // after file name follows line number
            strcpy (file , buf);
            sscanf (p,"%d", &line);
        }
        else {
            strcpy (file,"unkown");
            line = 0;
        }
        pclose(f);

        return line;
    }
#endif

	static void handleRuntimeError(const char *msg) {
	    //printf("In [%s::%s Line: %d] [%s] gameInitialized = %d\n",__FILE__,__FUNCTION__,__LINE__,msg,gameInitialized);

		Program *program = Program::getInstance();

        //printf("In [%s::%s Line: %d] [%s] gameInitialized = %d\n",__FILE__,__FUNCTION__,__LINE__,msg,gameInitialized);

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] [%s] gameInitialized = %d, program = %p\n",__FILE__,__FUNCTION__,__LINE__,msg,gameInitialized,program);
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] [%s] gameInitialized = %d, program = %p\n",__FILE__,__FUNCTION__,__LINE__,msg,gameInitialized,program);

        string errMsg = (msg != NULL ? msg : "null");

        #if defined(__GNUC__) && !defined(__MINGW32__) && !defined(__FreeBSD__) && !defined(BSD)
        if(disableBacktrace == false) {
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

        //printf("In [%s::%s Line: %d] [%s] gameInitialized = %d\n",__FILE__,__FUNCTION__,__LINE__,msg,gameInitialized);

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

        //printf("In [%s::%s Line: %d] [%s] gameInitialized = %d\n",__FILE__,__FUNCTION__,__LINE__,msg,gameInitialized);

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] [%s]\n",__FILE__,__FUNCTION__,__LINE__,errMsg.c_str());
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] [%s]\n",__FILE__,__FUNCTION__,__LINE__,errMsg.c_str());

        if(program && gameInitialized == true) {
			//printf("\nprogram->getState() [%p]\n",program->getState());
			if(program->getState() != NULL) {
                program->showMessage(errMsg.c_str());
                for(;program->isMessageShowing();) {
                    //program->getState()->render();
                    Window::handleEvent();
                    program->loop();
                }
			}
			else {
                program->showMessage(errMsg.c_str());
                for(;program->isMessageShowing();) {
                    //program->renderProgramMsgBox();
                    Window::handleEvent();
                    program->loop();
                }
			}
        }
        else {
            string err = "#2 An error occurred and " +
                        string(GameConstants::application_name) +
                        " will close.\nError msg = [" +
                        errMsg +
                        "]\n\nPlease report this bug to "+mailString;
#ifdef WIN32
            err += string(", attaching the generated ") + getCrashDumpFileName() + string(" file.");
#endif
			message(err);
        }

        // Now try to shutdown threads if possible
        delete program;
        program = NULL;
        // END

#ifdef WIN32

        showCursor(true);
        restoreVideoMode(true);

		runtimeErrorMsg = errMsg;
		throw runtimeErrorMsg;
#endif
		//printf("In [%s::%s Line: %d] [%s] gameInitialized = %d\n",__FILE__,__FUNCTION__,__LINE__,msg,gameInitialized);

		cleanupProcessObjects();
		SDL_Quit();
        exit(-1);
	}

	static int DisplayMessage(const char *msg, bool exitApp) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] msg [%s] exitApp = %d\n",__FILE__,__FUNCTION__,__LINE__,msg,exitApp);

        Program *program = Program::getInstance();
        if(program && gameInitialized == true) {
            program->showMessage(msg);
        }
        else {
            message(msg);
        }

        if(exitApp == true) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s\n",msg);

		    // Now try to shutdown threads if possible
			Program *program = Program::getInstance();
		    delete program;
		    program = NULL;
		    // END

		    cleanupProcessObjects();
		    SDL_Quit();
		    exit(-1);
        }

	    return 0;
	}
};

#if defined(__GNUC__)  && !defined(__FreeBSD__) && !defined(BSD)
void handleSIGSEGV(int sig) {
    char szBuf[4096]="";
    sprintf(szBuf, "In [%s::%s Line: %d] Error detected: signal %d:\n",__FILE__,__FUNCTION__,__LINE__, sig);
    printf("%s",szBuf);

    ExceptionHandler::handleRuntimeError(szBuf);
}
#endif


// =====================================================
// 	class MainWindow
// =====================================================

MainWindow::MainWindow(Program *program){
	this->program= program;
}

MainWindow::~MainWindow(){
	delete program;
	program = NULL;
}

void MainWindow::eventMouseDown(int x, int y, MouseButton mouseButton){
    const Metrics &metrics = Metrics::getInstance();
    int vx = metrics.toVirtualX(x);
    int vy = metrics.toVirtualY(getH() - y);

    if(program == NULL) {
    	throw runtime_error("In [MainWindow::eventMouseDown] ERROR, program == NULL!");
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

void MainWindow::eventKeyDown(char key){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key);

	SDL_keysym keystate = Window::getKeystate();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c][%d]\n",__FILE__,__FUNCTION__,__LINE__,key,key);

    if(program == NULL) {
    	throw runtime_error("In [MainWindow::eventKeyDown] ERROR, program == NULL!");
    }

    //{
    //Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
    //printf("----------------------- key [%d] CameraModeLeft [%d]\n",key,configKeys.getCharKey("CameraModeLeft"));
    //}

	program->keyDown(key);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(keystate.mod & (KMOD_LALT | KMOD_RALT)) {
		if(key == vkReturn) {
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
		if(key == configKeys.getCharKey("HotKeyShowDebug")) {

			Renderer &renderer= Renderer::getInstance();
			if(keystate.mod & (KMOD_LCTRL | KMOD_RCTRL)) {
				renderer.cycleShowDebugUILevel();
			}
			else {
				bool showDebugUI = renderer.getShowDebugUI();
				renderer.setShowDebugUI(!showDebugUI);
			}
		}
		else if(key == configKeys.getCharKey("ReloadINI")) {
			Config &config = Config::getInstance();
			config.reload();
		}
		else if(key == configKeys.getCharKey("Screenshot")) {
	        string userData = Config::getInstance().getString("UserData_Root","");
	        if(userData != "") {
	            if(userData != "" && EndsWith(userData, "/") == false && EndsWith(userData, "\\") == false) {
	            	userData += "/";
	            }
	        }

			string path = userData + GameConstants::folder_path_screenshots;
			if(isdir(path.c_str()) == true) {
				Config &config= Config::getInstance();
				string fileFormat = config.getString("ScreenShotFileType","png");

				unsigned int queueSize = Renderer::getInstance().getSaveScreenQueueSize();

				for(int i=0; i < 5000; ++i) {
					path = userData + GameConstants::folder_path_screenshots;
					path += string("screen") + intToStr(i + queueSize) + string(".") + fileFormat;
					FILE *f= fopen(path.c_str(), "rb");
					if(f == NULL) {
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

void MainWindow::eventKeyUp(char key){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key);
    if(program == NULL) {
    	throw runtime_error("In [MainWindow::eventKeyUp] ERROR, program == NULL!");
    }

	program->keyUp(key);
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key);
}

void MainWindow::eventKeyPress(char c){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] [%d]\n",__FILE__,__FUNCTION__,__LINE__,c);
    if(program == NULL) {
    	throw runtime_error("In [MainWindow::eventKeyPress] ERROR, program == NULL!");
    }

	program->keyPress(c);

	if(program != NULL && program->isInSpecialKeyCaptureEvent() == false) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
		if(c == configKeys.getCharKey("HotKeyToggleOSMouseEnabled")) {
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

void MainWindow::eventActivate(bool active){
	if(!active){
		//minimize();
	}
}

void MainWindow::eventResize(SizeState sizeState){
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

bool hasCommandArgument(int argc, char** argv,const string argName, int *foundIndex=NULL, int startLookupIndex=1,bool useArgParamLen=false) {
	bool result = false;

	if(foundIndex != NULL) {
		*foundIndex = -1;
	}
	int compareLen = strlen(argName.c_str());

	for(int idx = startLookupIndex; idx < argc; idx++) {
		if(useArgParamLen == true) {
			compareLen = strlen(argv[idx]);
		}
		if(_strnicmp(argName.c_str(),argv[idx],compareLen) == 0) {
			result = true;
			if(foundIndex != NULL) {
				*foundIndex = idx;
			}

			break;
		}
	}
	return result;
}

void print_SDL_version(const char *preamble, SDL_version *v) {
	printf("%s %u.%u.%u\n", preamble, v->major, v->minor, v->patch);
}
void printParameterHelp(const char *argv0, bool foundInvalidArgs) {
	if(foundInvalidArgs == true) {
			printf("\n");
	}
	printf("\n%s, usage\n\n",argv0);
	printf("Commandline Parameter:\t\tDescription:");
	printf("\n----------------------\t\t------------");
	printf("\n%s\t\t\t\tdisplays this help text.",GAME_ARGS[GAME_ARG_HELP]);
	printf("\n%s\t\tAutomatically starts a game with the last game settings you played.",GAME_ARGS[GAME_ARG_AUTOSTART_LASTGAME]);
	printf("\n%s=x\t\t\tAuto connects to a network server at IP or hostname x",GAME_ARGS[GAME_ARG_CLIENT]);
	printf("\n%s\t\t\tAuto creates a network server.",GAME_ARGS[GAME_ARG_SERVER]);
	printf("\n%s=x\t\tAuto loads the specified scenario by scenario name.",GAME_ARGS[GAME_ARG_LOADSCENARIO]);
	printf("\n%s\t\t\tdisplays the version string of this program.",GAME_ARGS[GAME_ARG_VERSION]);
	printf("\n%s\t\t\tdisplays your video driver's OpenGL information.",GAME_ARGS[GAME_ARG_OPENGL_INFO]);
	printf("\n%s\t\t\tdisplays your SDL version information.",GAME_ARGS[GAME_ARG_SDL_INFO]);
	printf("\n%s\t\t\tdisplays your LUA version information.",GAME_ARGS[GAME_ARG_LUA_INFO]);
	printf("\n%s\t\t\tdisplays your CURL version information.",GAME_ARGS[GAME_ARG_CURL_INFO]);
	printf("\n%s=x\t\tdisplays a report detailing any known problems related",GAME_ARGS[GAME_ARG_VALIDATE_TECHTREES]);
	printf("\n                     \t\tto your selected techtrees game data.");
	printf("\n                     \t\tWhere x is a comma-delimited list of techtrees to validate.");
	printf("\n                     \t\texample: %s %s=megapack,vbros_pack_5",argv0,GAME_ARGS[GAME_ARG_VALIDATE_TECHTREES]);
	printf("\n%s=x\t\tdisplays a report detailing any known problems related",GAME_ARGS[GAME_ARG_VALIDATE_FACTIONS]);
	printf("\n                     \t\tto your selected factions game data.");
	printf("\n                     \t\tWhere x is a comma-delimited list of factions to validate.");
	printf("\n                     \t\t*NOTE: leaving the list empty is the same as running");
	printf("\n                     \t\t%s",GAME_ARGS[GAME_ARG_VALIDATE_TECHTREES]);
	printf("\n                     \t\texample: %s %s=tech,egypt",argv0,GAME_ARGS[GAME_ARG_VALIDATE_FACTIONS]);
	printf("\n%s=x\t\t\tSets the game data path to x",GAME_ARGS[GAME_ARG_DATA_PATH]);
	printf("\n                     \t\texample: %s %s=/usr/local/game_data/",argv0,GAME_ARGS[GAME_ARG_DATA_PATH]);
	printf("\n%s=x\t\t\tSets the game ini path to x",GAME_ARGS[GAME_ARG_INI_PATH]);
	printf("\n                     \t\texample: %s %s=~/game_config/",argv0,GAME_ARGS[GAME_ARG_INI_PATH]);
	printf("\n%s=x\t\t\tSets the game logs path to x",GAME_ARGS[GAME_ARG_LOG_PATH]);
	printf("\n                     \t\texample: %s %s=~/game_logs/",argv0,GAME_ARGS[GAME_ARG_LOG_PATH]);
	printf("\n%s=x\t\t\tdisplays merged ini settings information.",GAME_ARGS[GAME_ARG_SHOW_INI_SETTINGS]);
	printf("\n                     \t\tWhere x is an optional property name to filter (default shows all).");
	printf("\n                     \t\texample: %s %s=DebugMode",argv0,GAME_ARGS[GAME_ARG_SHOW_INI_SETTINGS]);
	printf("\n%s\t\tdisables stack backtrace on errors.",GAME_ARGS[GAME_ARG_DISABLE_BACKTRACE]);
	printf("\n%s\t\tdisables trying to use Vertex Buffer Objects.",GAME_ARGS[GAME_ARG_DISABLE_VBO]);
	printf("\n%s\t\t\tdisplays verbose information in the console.",GAME_ARGS[GAME_ARG_VERBOSE_MODE]);

	printf("\n\n");
}

int setupGameItemPaths(int argc, char** argv) {
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
            pathCache[GameConstants::path_data_CacheLookupKey]=customPathValue;
            if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Using custom data path [%s]\n",customPathValue.c_str());
        }
        else {

            printf("\nInvalid path specified on commandline [%s] value [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
            printParameterHelp(argv[0],false);
            return -1;
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
            pathCache[GameConstants::path_logs_CacheLookupKey]=customPathValue;
            if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Using custom logs path [%s]\n",customPathValue.c_str());
        }
        else {

            printf("\nInvalid path specified on commandline [%s] value [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
            printParameterHelp(argv[0],false);
            return -1;
        }
    }

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
        if(userData != "" && EndsWith(userData, "/") == false && EndsWith(userData, "\\") == false) {
        	userData += "/";
        }
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

void runTechValidationReport(int argc, char** argv) {
	//disableBacktrace=true;

	printf("====== Started Validation ======\n");

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

            if(filteredFactionList.size() > 0) {
                printf("Filtering factions and only looking for the following:\n");
                for(int idx = 0; idx < filteredFactionList.size(); ++idx) {
                    filteredFactionList[idx] = trim(filteredFactionList[idx]);
                    printf("%s\n",filteredFactionList[idx].c_str());
                }
            }
        }
    }

    Config &config = Config::getInstance();
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

            if(filteredTechTreeList.size() > 0) {
                printf("Filtering techtrees and only looking for the following:\n");
                for(int idx = 0; idx < filteredTechTreeList.size(); ++idx) {
                    filteredTechTreeList[idx] = trim(filteredTechTreeList[idx]);
                    printf("%s\n",filteredTechTreeList[idx].c_str());
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
        //printf("techPath [%s]\n",techPath.c_str());

        for(int idx2 = 0; idx2 < techTreeFiles.size(); idx2++) {
            string &techName = techTreeFiles[idx2];

            if(	filteredTechTreeList.size() == 0 ||
                std::find(filteredTechTreeList.begin(),filteredTechTreeList.end(),techName) != filteredTechTreeList.end()) {

                vector<string> factionsList;
                findAll(techPath + "/" + techName + "/factions/*.", factionsList, false, false);

                if(factionsList.size() > 0) {
                    Checksum checksum;
                    set<string> factions;
                    for(int j = 0; j < factionsList.size(); ++j) {
                        if(	filteredFactionList.size() == 0 ||
                            std::find(filteredFactionList.begin(),filteredFactionList.end(),factionsList[j]) != filteredFactionList.end()) {
                            factions.insert(factionsList[j]);
                        }
                    }

                    printf("\n----------------------------------------------------------------");
                    printf("\nChecking techPath [%s] techName [%s] total faction count = %d\n",techPath.c_str(), techName.c_str(),(int)factionsList.size());
                    for(int j = 0; j < factionsList.size(); ++j) {
                        if(	filteredFactionList.size() == 0 ||
                            std::find(filteredFactionList.begin(),filteredFactionList.end(),factionsList[j]) != filteredFactionList.end()) {
                            printf("Using faction [%s]\n",factionsList[j].c_str());
                        }
                    }

                    if(factions.size() > 0) {
                        bool techtree_errors = false;
                        world.loadTech(config.getPathListForType(ptTechs,""), techName, factions, &checksum);
                        // Validate the faction setup to ensure we don't have any bad associations
                        std::vector<std::string> resultErrors = world.validateFactionTypes();
                        if(resultErrors.size() > 0) {
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
                        if(resultErrors.size() > 0) {
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

                if(filteredPropertyList.size() > 0) {
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
            if(filteredPropertyList.size() > 0) {
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
            if(filteredPropertyList.size() > 0) {
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
            if(filteredPropertyList.size() > 0) {
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
            if(filteredPropertyList.size() > 0) {
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
	if(duplicateMapsToRename.size() > 0) {
		string errorMsg = "Warning duplicate maps were detected and renamed:\n";
		for(int i = 0; i < duplicateMapsToRename.size(); ++i) {
			string oldFile = mapPaths[1] + "/" + duplicateMapsToRename[i];
			string newFile = mapPaths[1] + "/" + duplicateMapsToRename[i];
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
        //throw runtime_error(szBuf);
        Program *program = Program::getInstance();
        if(program) {
        	program->getState()->setForceMouseRender(true);
        }
        ExceptionHandler::DisplayMessage(errorMsg.c_str(), false);
	}

	//tilesets
	std::vector<std::string> tileSets;
    vector<string> tilesetPaths = config.getPathListForType(ptTilesets);
    findDirs(tilesetPaths, tileSets, false, true);

	if (tileSets.empty()) {
        throw runtime_error("No tilesets were found!");
    }

	for(int i = 0; i < tileSets.size(); ++i) {
	    string tileset1 = tileSets[i];
	    for(int j = 0; j < tileSets.size(); ++j) {
	        if(i != j) {
                string tileset2 = tileSets[j];
                if(tileset1 == tileset2) {
                    char szBuf[4096]="";
                    sprintf(szBuf,"You have duplicate tilesets for tileset [%s] in [%s] and [%s]",tileset1.c_str(),tilesetPaths[0].c_str(),tilesetPaths[1].c_str());
                    throw runtime_error(szBuf);

                }
	        }
	    }
	}

    vector<string> techPaths = config.getPathListForType(ptTechs);
    vector<string> techTrees;
    findDirs(techPaths, techTrees, false, true);
	if(techTrees.empty()) {
        throw runtime_error("No tech-trees were found!");
	}

	for(int i = 0; i < techTrees.size(); ++i) {
	    string techtree1 = techTrees[i];
	    for(int j = 0; j < techTrees.size(); ++j) {
	        if(i != j) {
                string techtree2 = techTrees[j];
                if(techtree1 == techtree2) {
                    char szBuf[4096]="";
                    sprintf(szBuf,"You have duplicate techtrees for techtree [%s] in [%s] and [%s]",techtree1.c_str(),techTrees[0].c_str(),techTrees[1].c_str());
                    throw runtime_error(szBuf);

                }
	        }
	    }
	}
}

int glestMain(int argc, char** argv) {
#ifdef SL_LEAK_DUMP
	AllocRegistry memoryLeaks = AllocRegistry::getInstance();
#endif

    SystemFlags::ENABLE_THREADED_LOGGING = false;
    disableBacktrace = false;

	bool foundInvalidArgs = false;
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

    ServerSocket::setMaxPlayerCount(GameConstants::maxPlayers);
    SystemFlags::VERBOSE_MODE_ENABLED  = false;
    if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_VERBOSE_MODE]) == true) {
        SystemFlags::VERBOSE_MODE_ENABLED  = true;
    }

    if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_DISABLE_BACKTRACE]) == true) {
        disableBacktrace = true;
    }

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
		hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_VALIDATE_FACTIONS]) 	== true) {
		haveSpecialOutputCommandLineOption = true;
	}

	if( haveSpecialOutputCommandLineOption == false ||
		hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_VERSION]) == true) {
#ifdef USE_STREFLOP
#	define STREFLOP_NO_DENORMALS
	streflop_init<streflop::Simple>();
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
		hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_VALIDATE_FACTIONS])  == false) {
		return -1;
	}

	SystemFlags::init(haveSpecialOutputCommandLineOption);
	SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled  = true;
	SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled = true;

	MainWindow *mainWindow= NULL;
	Program *program= NULL;
	ExceptionHandler exceptionHandler;
	exceptionHandler.install( getCrashDumpFileName() );

	try {
        // Setup paths to game items (like data, logs, ini etc)
        int setupResult = setupGameItemPaths(argc, argv);
        if(setupResult != 0) {
            return setupResult;
        }

		// Attempt to read ini files
		Config &config = Config::getInstance();

        // Set some statics based on ini entries
		SystemFlags::ENABLE_THREADED_LOGGING = config.getBool("ThreadedLogging","true");
		FontGl::setDefault_fontType(config.getString("DefaultFont",FontGl::getDefault_fontType().c_str()));
		UPNP_Tools::isUPNP = !config.getBool("DisableUPNP","false");
		Texture::useTextureCompression = config.getBool("EnableTextureCompression","false");
		// 256 for English
		// 30000 for Chinese
		Font::charCount    = config.getInt("FONT_CHARCOUNT",intToStr(Font::charCount).c_str());
		Font::fontTypeName = config.getString("FONT_TYPENAME",Font::fontTypeName.c_str());
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

        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Font::charCount = %d, Font::fontTypeName [%s] Shared::Platform::charSet = %d\n",__FILE__,__FUNCTION__,__LINE__,Font::charCount,Font::fontTypeName.c_str(),Shared::Platform::charSet);

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
				std::pair<string,string>("glestkeys.ini","glestuserkeys.ini"),
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

        // Setup the file crc thread
		std::auto_ptr<FileCRCPreCacheThread> preCacheThread;

		//float pingTime = Socket::getAveragePingMS("soft-haus.com");
		//printf("Ping time = %f\n",pingTime);

        // Load the language strings
        Lang &lang= Lang::getInstance();
        lang.loadStrings(config.getString("Lang"));

        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

        //
        //removeFolder("/home/softcoder/Code/megaglest/trunk/mk/linux/mydata/tilesets/mother_board");
        //return -1;
        //

		program= new Program();

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		mainWindow= new MainWindow(program);

		mainWindow->setUseDefaultCursorOnly(config.getBool("No2DMouseRendering","false"));

        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//parse command line
		if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_SERVER]) == true) {
			program->initServer(mainWindow,false,true);
		}
		else if(hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_AUTOSTART_LASTGAME])) == true) {
			program->initServer(mainWindow,true,false);
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
		Renderer &renderer= Renderer::getInstance();
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] OpenGL Info:\n%s\n",__FILE__,__FUNCTION__,__LINE__,renderer.getGlInfo().c_str());

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPathFinder).enabled == true) {
			renderer.setAllowRenderUnitTitles(SystemFlags::getSystemSettingType(SystemFlags::debugPathFinder).enabled);
			SystemFlags::OutputDebug(SystemFlags::debugPathFinder,"In [%s::%s Line: %d] renderer.setAllowRenderUnitTitles = %d\n",__FILE__,__FUNCTION__,__LINE__,SystemFlags::getSystemSettingType(SystemFlags::debugPathFinder).enabled);
		}

		if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_OPENGL_INFO]) == true) {
			//Renderer &renderer= Renderer::getInstance();
			printf("%s",renderer.getGlInfo().c_str());
			printf("%s",renderer.getGlMoreInfo().c_str());

			delete mainWindow;
			return -1;
		}
		if(	hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_VALIDATE_TECHTREES]) 	== true ||
			hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_VALIDATE_FACTIONS]) 	== true) {

			runTechValidationReport(argc, argv);

		    delete mainWindow;
		    return -1;
		}

		gameInitialized = true;

        // Setup the screenshots folder
        string userData = config.getString("UserData_Root","");
        if(userData != "") {
            if(userData != "" && EndsWith(userData, "/") == false && EndsWith(userData, "\\") == false) {
            	userData += "/";
            }
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
        		texture->load(playerTexture);
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

		if(config.getBool("AllowGameDataSynchCheck","false") == true) {
			vector<string> techDataPaths = config.getPathListForType(ptTechs);
			preCacheThread.reset(new FileCRCPreCacheThread());
			preCacheThread->setUniqueID(__FILE__);
			preCacheThread->setTechDataPaths(techDataPaths);
			preCacheThread->start();
		}

        // test
        //Shared::Platform::MessageBox(NULL,"Mark's test.","Test",0);
        //throw runtime_error("test!");
        //ExceptionHandler::DisplayMessage("test!", false);

		//Lang &lang= Lang::getInstance();
		//string test = lang.get("ExitGameServer?");
		//printf("[%s]",test.c_str());

		//main loop
		while(Window::handleEvent()) {
			program->loop();
		}

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	    showCursor(true);
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
	catch(const exception &e){
		ExceptionHandler::handleRuntimeError(e.what());
	}
	catch(const char *e){
		ExceptionHandler::handleRuntimeError(e);
	}
	catch(const string &ex){
		ExceptionHandler::handleRuntimeError(ex.c_str());
	}
	catch(...){
		ExceptionHandler::handleRuntimeError("Unknown error!");
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	delete mainWindow;
	mainWindow = NULL;

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    //showCursor(true);
    //restoreVideoMode(true);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	GraphicComponent::clearRegisteredComponents();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	return 0;
}

int glestMainWrapper(int argc, char** argv) {
#ifdef WIN32_STACK_TRACE
__try {
#endif
    application_binary=argv[0];

#if defined(__GNUC__) && !defined(__MINGW32__) && !defined(__FreeBSD__) && !defined(BSD)
    signal(SIGSEGV, handleSIGSEGV);

    // http://developerweb.net/viewtopic.php?id=3013
    //signal(SIGPIPE, SIG_IGN);
#endif

	int result = glestMain(argc, argv);
	cleanupProcessObjects();
	SDL_Quit();
	return result;

#ifdef WIN32_STACK_TRACE
} __except(stackdumper(0, GetExceptionInformation()), EXCEPTION_CONTINUE_SEARCH) { return 0; }
#endif
}

}}//end namespace

MAIN_FUNCTION(Glest::Game::glestMainWrapper)

