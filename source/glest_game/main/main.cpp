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
#include "cache_manager.h"

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

bool gameInitialized = false;

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
	"--validate-techtrees",
	"--validate-factions",
	"--data-path",
	"--ini-path",
	"--log-path"

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
	GAME_ARG_VALIDATE_TECHTREES,
	GAME_ARG_VALIDATE_FACTIONS,
	GAME_ARG_DATA_PATH,
	GAME_ARG_INI_PATH,
	GAME_ARG_LOG_PATH
};

string runtimeErrorMsg = "";

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
		string msg = "#1 An error ocurred and " + string(GameConstants::application_name) + " will close.\nPlease report this bug to "+mailString;
#ifdef WIN32
		msg += ", attaching the generated " + getCrashDumpFileName()+ " file.";
#endif
		SystemFlags::OutputDebug(SystemFlags::debugError,"%s\n",msg.c_str());
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s\n",msg.c_str());

        Program *program = Program::getInstance();
        if(program && gameInitialized == true) {
			//SystemFlags::Close();
            program->showMessage(msg.c_str());
        }

        message(msg.c_str());
	}

	static void handleRuntimeError(const char *msg) {
		Program *program = Program::getInstance();

		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] [%s] gameInitialized = %d, program = %p\n",__FILE__,__FUNCTION__,__LINE__,msg,gameInitialized,program);
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] [%s] gameInitialized = %d, program = %p\n",__FILE__,__FUNCTION__,__LINE__,msg,gameInitialized,program);

        if(program && gameInitialized == true) {
			//SystemFlags::Close();
            program->showMessage(msg);
        }
        else {
            string err = "#2 An error ocurred and " + string(GameConstants::application_name) + " will close.\nError msg = [" + (msg != NULL ? string(msg) : string("?")) + "]\n\nPlease report this bug to "+mailString;
#ifdef WIN32
            err += string(", attaching the generated ") + getCrashDumpFileName() + string(" file.");
#endif
			message(err);
        }
        showCursor(true);
        restoreVideoMode(true);

#ifdef WIN32

		runtimeErrorMsg = "";
		if(msg != NULL) {
			runtimeErrorMsg = msg;
		}
		throw runtimeErrorMsg;
#endif
		//SystemFlags::Close();
        exit(0);
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
        	showCursor(true);
            restoreVideoMode(true);
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s\n",msg);
			//SystemFlags::Close();
            exit(0);
        }

	    return 0;
	}
};

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
			bool showDebugUI = renderer.getShowDebugUI();
			renderer.setShowDebugUI(!showDebugUI);
		}
		else if(key == configKeys.getCharKey("ReloadINI")) {
			Config &config = Config::getInstance();
			config.reload();
		}
		else if(key == configKeys.getCharKey("Screenshot")) {
			string path = GameConstants::folder_path_screenshots;
			if(isdir(path.c_str()) == true) {
				Config &config= Config::getInstance();
				string fileFormat = config.getString("ScreenShotFileType","png");

				unsigned int queueSize = Renderer::getInstance().getSaveScreenQueueSize();

				for(int i=0; i < 250; ++i) {
					path = GameConstants::folder_path_screenshots;
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
    if(program == NULL) {
    	throw runtime_error("In [MainWindow::eventKeyUp] ERROR, program == NULL!");
    }

	program->keyUp(key);
}

void MainWindow::eventKeyPress(char c){
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
	printf("\n\n");
}

int glestMain(int argc, char** argv) {
#ifdef SL_LEAK_DUMP
	AllocRegistry memoryLeaks = AllocRegistry::getInstance();
#endif

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

	bool haveSpecialOutputCommandLineOption = false;

	if( hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_OPENGL_INFO]) 			== true ||
		hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_SDL_INFO]) 			== true ||
		hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_LUA_INFO]) 			== true ||
		hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_VERSION]) 				== true ||
		hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_VALIDATE_TECHTREES]) 	== true ||
		hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_VALIDATE_FACTIONS]) 	== true) {
		haveSpecialOutputCommandLineOption = true;
	}

	if( haveSpecialOutputCommandLineOption == false ||
		hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_VERSION]) == true) {
#ifdef USE_STREFLOP
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

	if( (hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_VERSION]) 		  == true ||
		 hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_SDL_INFO]) 		  == true ||
		 hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_LUA_INFO]) 		  == true) &&
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
				printf("Using custom data path [%s]\n",customPathValue.c_str());
			}
			else {

				printf("\nInvalid path specified on commandline [%s] value [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
				printParameterHelp(argv[0],foundInvalidArgs);
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
				printf("Using custom ini path [%s]\n",customPathValue.c_str());
			}
			else {

				printf("\nInvalid path specified on commandline [%s] value [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
				printParameterHelp(argv[0],foundInvalidArgs);
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
				printf("Using custom logs path [%s]\n",customPathValue.c_str());
			}
			else {

				printf("\nInvalid path specified on commandline [%s] value [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
				printParameterHelp(argv[0],foundInvalidArgs);
				return -1;
			}
        }


		std::auto_ptr<FileCRCPreCacheThread> preCacheThread;
		Config &config = Config::getInstance();
		FontGl::setDefault_fontType(config.getString("DefaultFont",FontGl::getDefault_fontType().c_str()));

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled      	= config.getBool("DebugMode","false");
		SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled     	= config.getBool("DebugNetwork","false");
		SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled 	= config.getBool("DebugPerformance","false");
		SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled  	= config.getBool("DebugWorldSynch","false");
		SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled  	= config.getBool("DebugUnitCommands","false");
		SystemFlags::getSystemSettingType(SystemFlags::debugPathFinder).enabled  	= config.getBool("DebugPathFinder","false");
		SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled  			= config.getBool("DebugLUA","false");
		SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled  		= config.getBool("DebugError","true");

		string debugLogFile 			= config.getString("DebugLogFile","");
        if(getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) != "") {
            debugLogFile = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) + debugLogFile;
        }
		string debugWorldSynchLogFile 	= config.getString("DebugLogFileWorldSynch","");
        if(debugWorldSynchLogFile == "") {
        	debugWorldSynchLogFile = debugLogFile;
        }
        else {
            debugWorldSynchLogFile = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) + debugWorldSynchLogFile;
        }

		string debugPerformanceLogFile = config.getString("DebugLogFilePerformance","");
        if(debugPerformanceLogFile == "") {
        	debugPerformanceLogFile = debugLogFile;
        }
        else {
            debugPerformanceLogFile = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) + debugPerformanceLogFile;
        }

		string debugNetworkLogFile = config.getString("DebugLogFileNetwork","");
        if(debugNetworkLogFile == "") {
        	debugNetworkLogFile = debugLogFile;
        }
        else {
            debugNetworkLogFile = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) + debugNetworkLogFile;
        }

		string debugUnitCommandsLogFile = config.getString("DebugLogFileUnitCommands","");
        if(debugUnitCommandsLogFile == "") {
        	debugUnitCommandsLogFile = debugLogFile;
        }
        else {
            debugUnitCommandsLogFile = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) + debugUnitCommandsLogFile;
        }

		string debugPathFinderLogFile = config.getString("DebugLogFilePathFinder","");
        if(debugUnitCommandsLogFile == "") {
        	debugUnitCommandsLogFile = debugLogFile;
        }
        else {
            debugUnitCommandsLogFile = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) + debugUnitCommandsLogFile;
        }

		string debugLUALogFile = config.getString("DebugLogFileLUA","");
        if(debugLUALogFile == "") {
        	debugLUALogFile = debugLogFile;
        }
        else {
            debugLUALogFile = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) + debugLUALogFile;
        }

		string debugErrorLogFile = config.getString("DebugLogFileError","");

        SystemFlags::getSystemSettingType(SystemFlags::debugSystem).debugLogFileName      = debugLogFile;
        SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).debugLogFileName     = debugNetworkLogFile;
        SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).debugLogFileName = debugPerformanceLogFile;
        SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).debugLogFileName  = debugWorldSynchLogFile;
        SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).debugLogFileName = debugUnitCommandsLogFile;
        SystemFlags::getSystemSettingType(SystemFlags::debugPathFinder).debugLogFileName   = debugPathFinderLogFile;
        SystemFlags::getSystemSettingType(SystemFlags::debugLUA).debugLogFileName  		   = debugLUALogFile;
        SystemFlags::getSystemSettingType(SystemFlags::debugError).debugLogFileName  	   = debugErrorLogFile;

        if(haveSpecialOutputCommandLineOption == false) {
        	printf("Startup settings are: debugSystem [%d], debugNetwork [%d], debugPerformance [%d], debugWorldSynch [%d], debugUnitCommands[%d], debugPathFinder[%d], debugLUA [%d], debugError [%d]\n",
        			SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled,
        			SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled,
        			SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled,
        			SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled,
        			SystemFlags::getSystemSettingType(SystemFlags::debugUnitCommands).enabled,
        			SystemFlags::getSystemSettingType(SystemFlags::debugPathFinder).enabled,
        			SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled,
        			SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled);
        }

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

		Texture::useTextureCompression = config.getBool("EnableTextureCompression","false");

		// 256 for English
		// 30000 for Chinese
		Font::charCount    = config.getInt("FONT_CHARCOUNT",intToStr(Font::charCount).c_str());
		Font::fontTypeName = config.getString("FONT_TYPENAME",Font::fontTypeName.c_str());
		// Example values:
		// DEFAULT_CHARSET (English) = 1
		// GB2312_CHARSET (Chinese)  = 134
		Shared::Platform::charSet = config.getInt("FONT_CHARSET",intToStr(Shared::Platform::charSet).c_str());

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Font::charCount = %d, Font::fontTypeName [%s] Shared::Platform::charSet = %d\n",__FILE__,__FUNCTION__,__LINE__,Font::charCount,Font::fontTypeName.c_str(),Shared::Platform::charSet);

		Config &configKeys = Config::getInstance(
				std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys),
				std::pair<string,string>("glestkeys.ini","glestuserkeys.ini"),
				std::pair<bool,bool>(true,false));

        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

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

        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		// Over-ride default network command framecount
		//GameConstants::networkFramePeriod = config.getInt("NetworkFramePeriod",intToStr(GameConstants::networkFramePeriod).c_str());

		//float pingTime = Socket::getAveragePingMS("soft-haus.com");
		//printf("Ping time = %f\n",pingTime);

        Lang &lang= Lang::getInstance();
        lang.loadStrings(config.getString("Lang"));

        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

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

		    delete mainWindow;
		    return -1;
		}

		gameInitialized = true;

		string screenShotsPath = GameConstants::folder_path_screenshots;
		//printf("In [%s::%s Line: %d] screenShotsPath [%s]\n",__FILE__,__FUNCTION__,__LINE__,screenShotsPath.c_str());
        if(isdir(screenShotsPath.c_str()) == false) {
        	createDirectoryPaths(screenShotsPath);
			//printf("In [%s::%s Line: %d] screenShotsPath [%s]\n",__FILE__,__FUNCTION__,__LINE__,screenShotsPath.c_str());
        }

        string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
        // Cache Player textures - START
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

		//throw "BLAH!";

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
		while(Window::handleEvent()){
			program->loop();
		}

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

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	GraphicComponent::clearRegisteredComponents();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//SystemFlags::Close();

	return 0;
}

int glestMainWrapper(int argc, char** argv) {
#ifdef WIN32_STACK_TRACE
__try {
#endif

	return glestMain(argc, argv);

#ifdef WIN32_STACK_TRACE
} __except(stackdumper(0, GetExceptionInformation()), EXCEPTION_CONTINUE_SEARCH) { return 0; }
#endif
}

}}//end namespace

MAIN_FUNCTION(Glest::Game::glestMainWrapper)

/*
int main(int argc, char *argv[])
{
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0)  {
        std::cerr << "Couldn't initialize SDL: " << SDL_GetError() << "\n";
        return 1;
    }
	atexit(SDL_Quit);
	SDL_EnableUNICODE(1);
    int result = Glest::Game::glestMainWrapper(argc, argv);
    return result;
}
*/

