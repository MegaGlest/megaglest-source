// ==============================================================
// This file is part of MegaGlest (www.glest.org)
//
// Copyright (C) 2011 - by Mark Vejvoda <mark_vejvoda@hotmail.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "main.h"

#include <stdexcept>

#include "model_gl.h"
#include "graphics_interface.h"
#include "util.h"
#include "conversion.h"
#include "platform_common.h"
#include "xml_parser.h"
#include <iostream>
#include <wx/event.h>
#include "config.h"
#include "game_constants.h"
#include <wx/stdpaths.h>
#include <platform_util.h>
#include "common_scoped_ptr.h"

#ifndef WIN32
#include <errno.h>
#endif

#ifndef WIN32
  #define stricmp strcasecmp
  #define strnicmp strncasecmp
  #define _strnicmp strncasecmp
#endif

#if wxCHECK_VERSION(2, 9, 1)
	#define WX2CHR(x) (x.mb_str())
#else
	#define WX2CHR(x) (wxConvCurrent->cWX2MB(x))
#endif

using namespace Shared::Platform;
using namespace Shared::PlatformCommon;
using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;
using namespace Shared::Util;
using namespace Shared::Xml;

using namespace std;
using namespace Glest::Game;

#ifdef _WIN32
const char *folderDelimiter = "\\";
#else
const char *folderDelimiter = "/";
#endif

//int GameConstants::updateFps= 40;
//int GameConstants::cameraFps= 100;

const string g3dviewerVersionString= "v3.13-dev";

// Because g3d should always support alpha transparency
string fileFormat = "png";

namespace Glest { namespace Game {

string getGameReadWritePath(const string &lookupKey) {
	string path = "";
    if(path == "" && getenv("GLESTHOME") != NULL) {
        path = safeCharPtrCopy(getenv("GLESTHOME"),8096);
        if(path != "" && EndsWith(path, "/") == false && EndsWith(path, "\\") == false) {
            path += "/";
        }

        //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path to be used for read/write files [%s]\n",__FILE__,__FUNCTION__,__LINE__,path.c_str());
    }

    return path;
}
}}

namespace Shared{ namespace G3dViewer{

// ===============================================
//	class Global functions
// ===============================================

wxString ToUnicode(const char* str) {
	return wxString(str, wxConvUTF8);
}

wxString ToUnicode(const string& str) {
	return wxString(str.c_str(), wxConvUTF8);
}

const wxChar  *GAME_ARGS[] = {
	wxT("--help"),
	wxT("--auto-screenshot"),
	wxT("--load-unit"),
	wxT("--load-model"),
	wxT("--load-model-animation-value"),
	wxT("--load-particle"),
	wxT("--load-projectile-particle"),
	wxT("--load-splash-particle"),
	wxT("--load-particle-loop-value"),
	wxT("--zoom-value"),
	wxT("--rotate-x-value"),
	wxT("--rotate-y-value"),
	wxT("--screenshot-format"),
	wxT("--verbose"),
};

enum GAME_ARG_TYPE {
	GAME_ARG_HELP = 0,
	GAME_ARG_AUTO_SCREENSHOT,
	GAME_ARG_LOAD_UNIT,
	GAME_ARG_LOAD_MODEL,
	GAME_ARG_LOAD_MODEL_ANIMATION_VALUE,
	GAME_ARG_LOAD_PARTICLE,
	GAME_ARG_LOAD_PARTICLE_PROJECTILE,
	GAME_ARG_LOAD_PARTICLE_SPLASH,
	GAME_ARG_LOAD_PARTICLE_LOOP_VALUE,
	GAME_ARG_ZOOM_VALUE,
	GAME_ARG_ROTATE_X_VALUE,
	GAME_ARG_ROTATE_Y_VALUE,
	GAME_ARG_SCREENSHOT_FORMAT,
	GAME_ARG_VERBOSE,
};

bool hasCommandArgument(int argc, wxChar** argv,const string &argName,
						int *foundIndex=NULL, int startLookupIndex=1,
						bool useArgParamLen=false) {
	bool result = false;

	if(foundIndex != NULL) {
		*foundIndex = -1;
	}
	size_t compareLen = strlen(argName.c_str());

	for(int idx = startLookupIndex; idx < argc; idx++) {
		const wxWX2MBbuf tmp_buf = wxConvCurrent->cWX2MB(argv[idx]);
		//printf("tmp_buf [%s]\n",(const char *)tmp_buf);
		if(useArgParamLen == true) {
			compareLen = strlen(tmp_buf);
		}
		if(_strnicmp(argName.c_str(),tmp_buf,compareLen) == 0) {
			result = true;
			if(foundIndex != NULL) {
				*foundIndex = idx;
			}

			break;
		}
	}
	return result;
}

void printParameterHelp(const char *argv0, bool foundInvalidArgs) {
	if(foundInvalidArgs == true) {
			printf("\n");
	}

	//         "================================================================================"
	printf("\n%s %s, [Using %s]\n",extractFileFromDirectoryPath(argv0).c_str(),g3dviewerVersionString.c_str(),(const char *)wxConvCurrent->cWX2MB(wxVERSION_STRING));

	printf("\nDisplays glest 3D-models and unit/projectile/splash particle systems.\n");
	printf("\nRotate with left mouse button. Zoom with right mouse button or mousewheel.");
	printf("\nUse ctrl to load more than one particle system.");
	printf("\nPress R to restart particles, this also reloads all files if they are changed.");

	//printf("\n\noptionally you may use any of the following:\n");
	printf("\n\n%s [G3D FILE], usage",extractFileFromDirectoryPath(argv0).c_str());
	printf("\n\nCommandline Parameter:  Description:");
	printf("\n\n- - - - - - - - - - -   - - - - - - - - - - - - - - - - - - - - - - - -");
	printf("\n\n%s  \t\tDisplays this help text.",(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_HELP]));

	//         "================================================================================"
	printf("\n\n%s=x  \t\tAuto load the unit / skill information specified",(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_UNIT]));
	printf("\n\n                     \tin path/filename x.");
	printf("\n\n                     \tWhere x is a g3d filename to load separated with a");
	printf("\n\n                     \tcomma and one or more skill names to try loading.");
	printf("\n\n                     \texample:");
	printf("\n\n                     \t%s %s=techs/megapack/factions/",extractFileFromDirectoryPath(argv0).c_str(),(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_UNIT]));
	printf("\n\n                     \ttech/units/battle_machine,attack_skill,stop_skill");

	printf("\n\n%s=x  \tAuto load the model specified in path/filename x.",(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_MODEL]));
	printf("\n\n                     \tWhere x is a g3d filename to load.");
	printf("\n\n                     \texample:");
	printf("\n\n                     \t%s %s=techs/megapack/factions/",extractFileFromDirectoryPath(argv0).c_str(),(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_MODEL]));
	printf("\n\n                     \ttech/units/battle_machine/models/battle_machine_dying.g3d");

	printf("\n\n%s=x  ",(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_MODEL_ANIMATION_VALUE]));
	printf("\n\n                     \tAnimation value when loading a model.");
	printf("\n\n                     \tWhere x is a decimal value from -1.0 to 1.0");
	printf("\n\n                     \texample:");
	printf("\n\n                     \t%s %s=0.5",extractFileFromDirectoryPath(argv0).c_str(),(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_MODEL_ANIMATION_VALUE]));

	//         "================================================================================"
	printf("\n\n%s=x  \tAutomatically takes a screenshot of the items you",(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_AUTO_SCREENSHOT]));
	printf("\n\n                     \tare loading.");
	printf("\n\n                     \tWhere x is a comma-delimited list of one or more");
	printf("\n\n                     \t    of the optional settings:");
	printf("\n\n                     \t    transparent, enable_grid, enable_wireframe,");
	printf("\n\n                     \t    enable_normals, disable_grid, disable_wireframe,");
	printf("\n\n                     \t    disable_normals, saveas-<filename>, resize-wxh");
	printf("\n\n                     \texample:");
	printf("\n\n                     \t%s %s=transparent,",extractFileFromDirectoryPath(argv0).c_str(),(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_AUTO_SCREENSHOT]));
	printf("\n\n                     \tdisable_grid,saveas-test.png,resize-800x600");
	printf("\n\n                     \t%s=techs/megapack/factions/tech/units/",(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_MODEL]));
	printf("\n\n                     \tbattle_machine/models/battle_machine_dying.g3d");

	//         "================================================================================"
	printf("\n\n%s=x  \tAuto load the particle specified in path/filename x.",(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_PARTICLE]));
	printf("\n\n                     \tWhere x is a Particle XML filename to load.");
	printf("\n\n                     \texample:");
	printf("\n\n                     \t%s %s=techs/megapack/factions/",extractFileFromDirectoryPath(argv0).c_str(),(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_PARTICLE]));
	printf("\n\n                     \tpersian/units/genie/glow_particles.xml");

	printf("\n\n%s=x  ",(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_PARTICLE_PROJECTILE]));
	printf("\n\n                     \tAuto load the projectile particle specified in");
	printf("\n\n                     \tpath/filename x.");
	printf("\n\n                     \tWhere x is a Projectile Particle Definition XML");
	printf("\n\n                     \t    filename to load.");
	printf("\n\n                     \texample:");
	printf("\n\n                     \t%s %s=techs/megapack/",extractFileFromDirectoryPath(argv0).c_str(),(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_PARTICLE_PROJECTILE]));
	printf("\n\n                     \tfactions/persian/units/genie/particle_proj.xml");

	printf("\n\n%s=x  ",(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_PARTICLE_SPLASH]));
	printf("\n\n                     \tAuto load the splash particle specified in");
	printf("\n\n                     \tpath/filename x.");
	printf("\n\n                     \tWhere x is a Splash Particle Definition XML");
	printf("\n\n                     \t    filename to load.");
	printf("\n\n                     \texample:");
	printf("\n\n                     \t%s %s=techs/megapack/",extractFileFromDirectoryPath(argv0).c_str(),(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_PARTICLE_SPLASH]));
	printf("\n\n                     \tfactions/persian/units/genie/particle_splash.xml");

	printf("\n\n%s=x  ",(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_PARTICLE_LOOP_VALUE]));
	printf("\n\n                     \tParticle loop value when loading one or more");
	printf("\n\n                     \tparticles.");
	printf("\n\n                     \tWhere x is an integer value from 1 to particle count.");
	printf("\n\n                     \texample:");
	printf("\n\n                     \t%s %s=25",extractFileFromDirectoryPath(argv0).c_str(),(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_PARTICLE_LOOP_VALUE]));

	printf("\n\n%s=x  \tZoom value when loading a model.",(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_ZOOM_VALUE]));
	printf("\n\n                     \tWhere x is a decimal value from 0.1 to 10.0");
	printf("\n\n                     \texample:");
	printf("\n\n                     \t%s %s=4.2",extractFileFromDirectoryPath(argv0).c_str(),(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_ZOOM_VALUE]));

	printf("\n\n%s=x  \tX Coordinate Rotation value when loading a model.",(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_ROTATE_X_VALUE]));
	printf("\n\n                     \tWhere x is a decimal value from -10.0 to 10.0");
	printf("\n\n                     \texample:");
	printf("\n\n                     \t%s %s=2.2",extractFileFromDirectoryPath(argv0).c_str(),(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_ROTATE_X_VALUE]));

	printf("\n\n%s=x  \tY Coordinate Rotation value when loading a model.",(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_ROTATE_Y_VALUE]));
	printf("\n\n                     \tWhere x is a decimal value from -10.0 to 10.0");
	printf("\n\n                     \texample:");
	printf("\n\n                     \t%s %s=2.2",extractFileFromDirectoryPath(argv0).c_str(),(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_ROTATE_Y_VALUE]));

	printf("\n\n%s=x  \tSpecify which image format to use for screenshots.",(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_SCREENSHOT_FORMAT]));
	printf("\n\n                     \tWhere x is one of the following supported formats:");
	printf("\n\n                     \t    png,jpg,tga,bmp");
	printf("\n\n                     \t*NOTE: png is the default (and supports transparency)");
	printf("\n\n                     \texample:");
	printf("\n\n                     \t%s %s=jpg",extractFileFromDirectoryPath(argv0).c_str(),(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_SCREENSHOT_FORMAT]));

	printf("\n\n");
}

bool autoScreenShotAndExit = false;
vector<string> autoScreenShotParams;
std::pair<int,int> overrideSize(0,0);

// ===============================================
// 	class MainWindow
// ===============================================

const string MainWindow::winHeader= "G3D viewer " + g3dviewerVersionString;

const float defaultspeed = 0.025f;

MainWindow::MainWindow(	std::pair<string,vector<string> > unitToLoad,
						const string modelPath,
						const string particlePath,
						const string projectileParticlePath,
						const string splashParticlePath,
						float defaultAnimation,
						int defaultParticleLoopStart,
						float defaultZoom,float defaultXRot, float defaultYRot,
						string appPath)
    :	wxFrame(NULL, -1, ToUnicode(winHeader),
    	        wxPoint(Renderer::windowX, Renderer::windowY),
		        wxSize(Renderer::windowW, Renderer::windowH)),
		        glCanvas(NULL),
		        renderer(NULL),
		        timer(NULL),
		        menu(NULL),
		        fileDialog(NULL),
		        colorPicker(NULL),
		        model(NULL),
		        initTextureManager(true),
		        startupSettingsInited(false)

{
	this->appPath = appPath;
	Properties::setApplicationPath(executable_path(appPath));

	lastanim = 0;
	model= NULL;

	Config &config = Config::getInstance();

	isControlKeyPressed = false;

	initGlCanvas();

#if wxCHECK_VERSION(2, 9, 1)

#else
	glCanvas->SetCurrent();
#endif

	unitPath = unitToLoad;

	if(modelPath != "") {
		this->modelPathList.push_back(modelPath);
		printf("Startup Adding model [%s] list size " MG_SIZE_T_SPECIFIER "\n",modelPath.c_str(),this->modelPathList.size());
	}
	if(particlePath != "") {
		this->particlePathList.push_back(particlePath);
	}
	if(projectileParticlePath != "") {
		this->particleProjectilePathList.push_back(projectileParticlePath);
	}
	if(splashParticlePath != "") {
		this->particleSplashPathList.push_back(splashParticlePath);
	}

	resetAnimation = false;
	anim = defaultAnimation;
	particleLoopStart = defaultParticleLoopStart;
	resetAnim 				= anim;
	resetParticleLoopStart 	= particleLoopStart;
	rotX= defaultXRot;
	rotY= defaultYRot;
	zoom= defaultZoom;
	playerColor= Renderer::pcRed;

	speed= defaultspeed;

    //getGlPlatformExtensions();
	menu= new wxMenuBar();

	//menu
	menuFile= new wxMenu();
	menuFile->Append(miFileLoad, wxT("&Load G3d model\tCtrl+L"), wxT("Load 3D model"));
	menuFile->Append(miFileLoadParticleXML, wxT("Load &Particle XML\tCtrl+P"), wxT("Press ctrl before menu for keeping current particles"));
	menuFile->Append(miFileLoadProjectileParticleXML, wxT("Load Pro&jectile Particle XML\tCtrl+J"), wxT("Press ctrl before menu for keeping current projectile particles"));
	menuFile->Append(miFileLoadSplashParticleXML, wxT("Load &Splash Particle XML\tCtrl+S"), wxT("Press ctrl before menu for keeping current splash particles"));
	menuFile->Append(miFileClearAll, wxT("&Clear All\tCtrl+C"));
	menuFile->AppendCheckItem(miFileToggleScreenshotTransparent, wxT("&Transparent Screenshots\tCtrl+T"));
	menuFile->Append(miFileSaveScreenshot, wxT("Sa&ve a Screenshot\tCtrl+V"));
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT);
	menu->Append(menuFile, wxT("&File"));

	//mode
	menuMode= new wxMenu();
	menuMode->AppendCheckItem(miModeNormals, wxT("&Normals"));
	menuMode->AppendCheckItem(miModeWireframe, wxT("&Wireframe"));
	menuMode->AppendCheckItem(miModeGrid, wxT("&Grid"));
	menu->Append(menuMode, wxT("&Mode"));

	//mode
	menuSpeed= new wxMenu();
	menuSpeed->Append(miSpeedSlower, wxT("&Slower\t-"));
	menuSpeed->Append(miSpeedFaster, wxT("&Faster\t+"));
	menuSpeed->AppendSeparator();
	menuSpeed->Append(miRestart, wxT("&Restart particles\tR"), wxT("Restart particle animations, this also reloads model and particle files if they are changed"));
	menu->Append(menuSpeed, wxT("&Speed"));

	//custom color
	menuCustomColor= new wxMenu();
	menuCustomColor->Append(miChangeBackgroundColor, wxT("Change Background Color"));
	menuCustomColor->AppendCheckItem(miColorRed, wxT("&Red\t0"));
	menuCustomColor->AppendCheckItem(miColorBlue, wxT("&Blue\t1"));
	menuCustomColor->AppendCheckItem(miColorGreen, wxT("&Green\t2"));
	menuCustomColor->AppendCheckItem(miColorYellow, wxT("&Yellow\t3"));
	menuCustomColor->AppendCheckItem(miColorWhite, wxT("&White\t4"));
	menuCustomColor->AppendCheckItem(miColorCyan, wxT("&Cyan\t5"));
	menuCustomColor->AppendCheckItem(miColorOrange, wxT("&Orange\t6"));
	menuCustomColor->AppendCheckItem(miColorMagenta, wxT("&Pink\t7")); // it is called Pink everywhere else so...
	menu->Append(menuCustomColor, wxT("&Custom Color"));

	menuMode->Check(miModeGrid, true);
	menuCustomColor->Check(miColorRed, true);

	SetMenuBar(menu);

	//misc
	backBrightness= 0.3f;
	gridBrightness= 1.0f;
	lightBrightness= 0.3f;
	lastX= 0;
	lastY= 0;

	statusbarText="";
	CreateStatusBar();

	this->Layout();

	wxInitAllImageHandlers();
#ifdef WIN32

#if defined(__MINGW32__)
	wxIcon icon(ToUnicode("IDI_ICON1"));
#else
    wxIcon icon(L"IDI_ICON1");
#endif

#else
	wxIcon icon;
	string iniFilePath = extractDirectoryPathFromFile(config.getFileName(false));
	string icon_file = iniFilePath + "g3dviewer.ico";
	std::ifstream testFile(icon_file.c_str());
	if(testFile.good())	{
		testFile.close();
		icon.LoadFile(ToUnicode(icon_file.c_str()),wxBITMAP_TYPE_ICO);
	}
#endif
	SetIcon(icon);

	fileDialog = new wxFileDialog(this);
	if(modelPath != "") {
		fileDialog->SetPath(ToUnicode(modelPath));
	}
    string userData = config.getString("UserData_Root","");
    if(userData != "") {
    	endPathWithSlash(userData);
    }
    string defaultPath = userData + "techs/";
    fileDialog->SetDirectory(ToUnicode(defaultPath.c_str()));


	if(glCanvas != NULL) {
		glCanvas->SetFocus();
	}

	// For windows register g3d file extension to launch this app
#if defined(WIN32) && !defined(__MINGW32__)
	// example from: http://stackoverflow.com/questions/1387769/create-registry-entry-to-associate-file-extension-with-application-in-c
	//[HKEY_CURRENT_USER\Software\Classes\blergcorp.blergapp.v1\shell\open\command]
	//@="c:\path\to\app.exe \"%1\""
	//[HKEY_CURRENT_USER\Software\Classes\.blerg]
	//@="blergcorp.blergapp.v1"

	//Open the registry key.
	wstring subKey = L"Software\\Classes\\megaglest.g3d\\shell\\open\\command";
	HKEY keyHandle;
	DWORD dwDisposition;
	RegCreateKeyEx(HKEY_CURRENT_USER,subKey.c_str(),0, NULL, 0, KEY_ALL_ACCESS, NULL, &keyHandle, &dwDisposition);
	//Set the value.
	auto_ptr<wchar_t> wstr(Ansi2WideString(appPath.c_str()));
	
	wstring launchApp = wstring(wstr.get()) + L" \"%1\"";
	DWORD len = (launchApp.size() + 1) * sizeof(wchar_t);
	RegSetValueEx(keyHandle, NULL, 0, REG_SZ, (PBYTE)launchApp.c_str(), len);
	RegCloseKey(keyHandle);

	subKey = L"Software\\Classes\\.g3d";
	RegCreateKeyEx(HKEY_CURRENT_USER,subKey.c_str(),0, NULL, 0, KEY_ALL_ACCESS, NULL, &keyHandle, &dwDisposition);
	//Set the value.
	launchApp = L"megaglest.g3d";
	len = (launchApp.size() + 1) * sizeof(wchar_t);
	RegSetValueEx(keyHandle, NULL, 0, REG_SZ, (PBYTE)launchApp.c_str(), len);
	RegCloseKey(keyHandle);

#endif
}

void MainWindow::setupTimer() {
	timer = new wxTimer(this);
	timer->Start(100);
}

void MainWindow::setupStartupSettings() {

	//printf("In setupStartupSettings #1\n");
	if(glCanvas == NULL) {
		initGlCanvas();

#if wxCHECK_VERSION(2, 9, 1)

#else
	    glCanvas->SetCurrent();
#endif

	}
	glCanvas->setCurrentGLContext();
	//printf("In setupStartupSettings #2\n");

	GLuint err = glewInit();
	if (GLEW_OK != err) {
		fprintf(stderr, "Error [main]: glewInit failed: %s\n", glewGetErrorString(err));
		//return 1;
		throw std::runtime_error((char *)glewGetErrorString(err));
	}

	renderer= Renderer::getInstance();

	for(unsigned int i = 0; i < autoScreenShotParams.size(); ++i) {
		if(autoScreenShotParams[i] == "transparent") {
			printf("Screenshot option [%s]\n",autoScreenShotParams[i].c_str());
			menuFile->Check(miFileToggleScreenshotTransparent,true);
			float alpha = 0.0f;
			renderer->setAlphaColor(alpha);
		}
		if(autoScreenShotParams[i] == "enable_grid") {
			printf("Screenshot option [%s]\n",autoScreenShotParams[i].c_str());
			menuMode->Check(miModeGrid,true);
			if(renderer->getGrid() == false) {
				renderer->toggleGrid();
			}
		}
		if(autoScreenShotParams[i] == "enable_wireframe") {
			printf("Screenshot option [%s]\n",autoScreenShotParams[i].c_str());
			menuMode->Check(miModeWireframe,true);
			if(renderer->getWireframe() == false) {
				renderer->toggleWireframe();
			}
		}
		if(autoScreenShotParams[i] == "enable_normals") {
			printf("Screenshot option [%s]\n",autoScreenShotParams[i].c_str());
			menuMode->Check(miModeNormals,true);
			if(renderer->getNormals() == false) {
				renderer->toggleNormals();
			}
		}
		if(autoScreenShotParams[i] == "disable_grid") {
			printf("Screenshot option [%s]\n",autoScreenShotParams[i].c_str());
			menuMode->Check(miModeGrid,false);
			if(renderer->getGrid() == true) {
				renderer->toggleGrid();
			}
		}
		if(autoScreenShotParams[i] == "enable_wireframe") {
			printf("Screenshot option [%s]\n",autoScreenShotParams[i].c_str());
			menuMode->Check(miModeWireframe,false);
			if(renderer->getWireframe() == true) {
				renderer->toggleWireframe();
			}
		}
		if(autoScreenShotParams[i] == "enable_normals") {
			printf("Screenshot option [%s]\n",autoScreenShotParams[i].c_str());
			menuMode->Check(miModeNormals,false);
			if(renderer->getNormals() == true) {
				renderer->toggleNormals();
			}
		}
	}
	renderer->init();

	onMenuRestartNoEvent();
}

MainWindow::~MainWindow(){
	delete timer;
	timer = NULL;

	delete fileDialog;
	fileDialog = NULL;

	//delete model;
	//model = NULL;
	if(renderer) renderer->end();

	delete renderer;
	renderer = NULL;

	if(glCanvas) {
		glCanvas->Destroy();
	}
	glCanvas = NULL;

}

void MainWindow::initGlCanvas(){
	if(glCanvas == NULL) {
		int args[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_MIN_ALPHA, 8, 0 }; // to prevent flicker
		glCanvas = new GlCanvas(this, args);
	}
}

void MainWindow::init() {

#if wxCHECK_VERSION(2, 9, 3)

#elif wxCHECK_VERSION(2, 9, 1)

#else
	glCanvas->SetCurrent();
	//printf("setcurrent #2\n");
#endif
}

void MainWindow::onPaint(wxPaintEvent &event) {
	if(!IsShown()) {
			event.Skip();
			return;
		}

	onPaintNoEvent( );
}

void MainWindow::onPaintNoEvent( ) {
	bool isFirstWindowShownEvent = !startupSettingsInited ;
	if(startupSettingsInited == false) {
		startupSettingsInited = true;
		setupStartupSettings();
	}
	glCanvas->setCurrentGLContext();

	static float autoScreenshotRender = -1;
	if(autoScreenShotAndExit == true && autoScreenshotRender >= 0) {
		anim = autoScreenshotRender;
	}
	// notice that we use GetSize() here and not GetClientSize() because
	// the latter doesn't return correct results for the minimized windows
	// (at least not under Windows)
	int viewportW = GetClientSize().x;
	int viewportH = GetClientSize().y;

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("%d x %d\n",viewportW,viewportH);

	if(viewportW == 0 && viewportH == 0) {
		printf("#1 %d x %d\n",viewportW,viewportH);

		viewportW = GetSize().x;
		viewportH = GetSize().y;

		if(viewportH > 0) {
			//viewportH -= menu->GetSize().y;
			viewportH -= 22;
		}

		printf("#2 %d x %d\n",viewportW,viewportH);
	}

#if defined(WIN32)
	renderer->reset(viewportW, viewportH, playerColor);
#else
	renderer->reset(viewportW, viewportH, playerColor);
#endif

	renderer->transform(rotX, rotY, zoom);
	renderer->renderGrid();

	//printf("anim [%f] particleLoopStart [%d]\n",anim,particleLoopStart);

	string statusTextValue = statusbarText + " animation speed: " + floatToStr(speed * 1000.0) + " anim value: " + floatToStr(anim) + " zoom: " + floatToStr(zoom) + " rotX: " + floatToStr(rotX) + " rotY: " + floatToStr(rotY);
	GetStatusBar()->SetStatusText(ToUnicode(statusTextValue.c_str()));

	renderer->renderTheModel(model, anim);

	int updateLoops = particleLoopStart;
	particleLoopStart = 1;

	if(resetAnimation == true || ((anim - lastanim) >= defaultspeed*2)) {
		//printf("anim [%f] [%f] [%f]\n",anim,lastanim,speed);

		for(int i=0; i< updateLoops; ++i) {
			renderer->updateParticleManager();
		}
	}

	renderer->renderParticleManager();
	
	if(isFirstWindowShownEvent) {
		this->Refresh();
		glCanvas->Refresh();
		glCanvas->SetFocus();
	}

	bool haveLoadedParticles = (particleProjectilePathList.empty() == false || particleSplashPathList.empty() == false);

	if(autoScreenShotAndExit == true && viewportW > 0 && viewportH > 0) {
		printf("Auto exiting app...\n");
		fflush(stdout);

		autoScreenShotAndExit = false;

		saveScreenshot();
		Close();
		return;
	}
	
	glCanvas->SwapBuffers();

	if(autoScreenShotAndExit == true && viewportW == 0 && viewportH == 0) {
		autoScreenshotRender = anim;

		printf("Auto exiting desired but waiting for w x h > 0...\n");

		return;
	}
	if((modelPathList.empty() == false) && resetAnimation && haveLoadedParticles) {
		if(anim >= resetAnim && resetAnim > 0) {
			printf("RESETTING EVERYTHING [%f][%f]...\n",anim,resetAnim);
			fflush(stdout);

			resetAnimation 		= false;
			particleLoopStart 	= resetParticleLoopStart;

			if(unitPath.first != "") {
				//onMenuFileClearAll(event);

				modelPathList.clear();
				particlePathList.clear();
				particleProjectilePathList.clear();
				particleSplashPathList.clear(); // as above
			}
			onMenuRestartNoEvent();
		}
	}
	else if(modelPathList.empty() == true && haveLoadedParticles) {
		if(renderer->hasActiveParticleSystem(ParticleSystem::pst_ProjectileParticleSystem) == false &&
		   renderer->hasActiveParticleSystem(ParticleSystem::pst_SplashParticleSystem) == false) {

			printf("RESETTING PARTICLES...\n");
			fflush(stdout);

			resetAnimation 		= false;
			anim				= 0.f;
			particleLoopStart 	= resetParticleLoopStart;

			onMenuRestartNoEvent();
		}
	}

	lastanim = anim;
}

void MainWindow::onClose(wxCloseEvent &event){
	// release memory first (from onMenuFileClearAll)

	//printf("OnClose START\n");
	//fflush(stdout);

	modelPathList.clear();
	particlePathList.clear();
	particleProjectilePathList.clear();
	particleSplashPathList.clear(); // as above

	if(timer) timer->Stop();

	unitParticleSystems.clear();
	unitParticleSystemTypes.clear();

	projectileParticleSystems.clear();
	projectileParticleSystemTypes.clear();
	splashParticleSystems.clear(); // as above
	splashParticleSystemTypes.clear();

	//delete model;
	//model = NULL;
	if(renderer) renderer->end();

	//printf("OnClose about to END\n");
	//fflush(stdout);

	delete timer;
	timer = NULL;

	//delete model;
	//model = NULL;

	delete renderer;
	renderer = NULL;

	//delete glCanvas;
	if(glCanvas) {
		glCanvas->Destroy();
	}
	glCanvas = NULL;

	this->Destroy();
}

// for the mousewheel
void MainWindow::onMouseWheelDown(wxMouseEvent &event) {
	try {
		zoom*= 1.1f;
		zoom= clamp(zoom, 0.1f, 10.0f);

		string statusTextValue = statusbarText + " animation speed: " + floatToStr(speed * 1000.0) + " anim value: " + floatToStr(anim) + " zoom: " + floatToStr(zoom) + " rotX: " + floatToStr(rotX) + " rotY: " + floatToStr(rotY);
		GetStatusBar()->SetStatusText(ToUnicode(statusTextValue.c_str()));

		onPaintNoEvent();
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMouseWheelUp(wxMouseEvent &event) {
	try {
		zoom*= 0.90909f;
		zoom= clamp(zoom, 0.1f, 10.0f);

		string statusTextValue = statusbarText + " animation speed: " + floatToStr(speed * 1000.0) + " anim value: " + floatToStr(anim) + " zoom: " + floatToStr(zoom) + " rotX: " + floatToStr(rotX) + " rotY: " + floatToStr(rotY);
		GetStatusBar()->SetStatusText(ToUnicode(statusTextValue.c_str()));

		onPaintNoEvent();
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}


void MainWindow::onMouseMove(wxMouseEvent &event){
	try {
		int x= event.GetX();
		int y= event.GetY();


		if(event.LeftIsDown()){
			rotX+= clamp(lastX-x, -10, 10);
			rotY+= clamp(lastY-y, -10, 10);

			string statusTextValue = statusbarText + " animation speed: " + floatToStr(speed * 1000.0) + " anim value: " + floatToStr(anim) + " zoom: " + floatToStr(zoom) + " rotX: " + floatToStr(rotX) + " rotY: " + floatToStr(rotY);
			GetStatusBar()->SetStatusText(ToUnicode(statusTextValue.c_str()));

			onPaintNoEvent();
		}
		else if(event.RightIsDown()){
			zoom*= 1.0f+(lastX-x+lastY-y)/100.0f;
			zoom= clamp(zoom, 0.1f, 10.0f);

			string statusTextValue = statusbarText + " animation speed: " + floatToStr(speed * 1000.0) + " anim value: " + floatToStr(anim) + " zoom: " + floatToStr(zoom) + " rotX: " + floatToStr(rotX) + " rotY: " + floatToStr(rotY);
			GetStatusBar()->SetStatusText(ToUnicode(statusTextValue.c_str()));

			onPaintNoEvent();
		}

		lastX= x;
		lastY= y;
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuFileLoad(wxCommandEvent &event){
	try {
		//string fileName;
		fileDialog->SetWildcard(wxT("G3D files (*.g3d)|*.g3d;*.G3D"));
		fileDialog->SetMessage(wxT("Selecting Glest Model for current view."));

		if(fileDialog->ShowModal()==wxID_OK){
			modelPathList.clear();
			string file;
			file = (const char*)wxFNCONV(fileDialog->GetPath().c_str());

			//loadModel((const char*)wxFNCONV(fileDialog->GetPath().c_str()));
			loadModel(file);
		}
		isControlKeyPressed = false;
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuFileLoadParticleXML(wxCommandEvent &event){
	try {
		//string fileName;
		fileDialog->SetWildcard(wxT("XML files (*.xml)|*.xml"));

		if(isControlKeyPressed == true) {
			fileDialog->SetMessage(wxT("Adding Mega-Glest particle to current view."));
		}
		else {
			fileDialog->SetMessage(wxT("Selecting Mega-Glest particle for current view."));
		}

		if(fileDialog->ShowModal()==wxID_OK){
			//string path = (const char*)wxFNCONV(fileDialog->GetPath().c_str());
			string file;
			file = (const char*)wxFNCONV(fileDialog->GetPath().c_str());

			loadParticle(file);
		}
		isControlKeyPressed = false;
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuFileLoadProjectileParticleXML(wxCommandEvent &event){
	try {
		//string fileName;
		fileDialog->SetWildcard(wxT("XML files (*.xml)|*.xml"));

		if(isControlKeyPressed == true) {
			fileDialog->SetMessage(wxT("Adding Mega-Glest projectile particle to current view."));
		}
		else {
			fileDialog->SetMessage(wxT("Selecting Mega-Glest projectile particle for current view."));
		}

		if(fileDialog->ShowModal()==wxID_OK){
			//string path = (const char*)wxFNCONV(fileDialog->GetPath().c_str());
			string file;
			file = (const char*)wxFNCONV(fileDialog->GetPath().c_str());

			loadProjectileParticle(file);
		}
		isControlKeyPressed = false;
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuFileLoadSplashParticleXML(wxCommandEvent &event){
	try {
		//string fileName;
		fileDialog->SetWildcard(wxT("XML files (*.xml)|*.xml"));

		if(isControlKeyPressed == true) {
			fileDialog->SetMessage(wxT("Adding Mega-Glest splash particle to current view."));
		}
		else {
			fileDialog->SetMessage(wxT("Selecting Mega-Glest splash particle for current view."));
		}

		if(fileDialog->ShowModal()==wxID_OK){
			//string path = (const char*)wxFNCONV(fileDialog->GetPath().c_str());
			string file;
			file = (const char*)wxFNCONV(fileDialog->GetPath().c_str());

			loadSplashParticle(file);
		}
		isControlKeyPressed = false;
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}  // is it possible to join loadParticle(), loadProjectileParticle() and loadSplashParticle() to one method?


void MainWindow::OnChangeColor(wxCommandEvent &event) {
	try {
		//wxColour color = colorPicker->GetColour();
		wxColourData data;
		data.SetChooseFull(true);
		for (int i = 0; i < 16; i++)
		{
			wxColour colour(i*16, i*16, i*16);
			data.SetCustomColour(i, colour);
		}

		wxColourDialog dialog(this, &data);
		if (dialog.ShowModal() == wxID_OK)
		{
			wxColourData retData = dialog.GetColourData();
			wxColour col = retData.GetColour();
			renderer->setBackgroundColor(col.Red()/255.0f, col.Green()/255.0f, col.Blue()/255.0f);
		}
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenumFileToggleScreenshotTransparent(wxCommandEvent &event) {
    try {
        float alpha = (event.IsChecked() == true ? 0.0f : 1.0f);
        renderer->setAlphaColor(alpha);
        //printf("alpha = %f\n",alpha);
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::saveScreenshot() {
	try {
		int autoSaveScreenshotIndex = -1;
	    for(unsigned int i = 0; i < autoScreenShotParams.size(); ++i) {
			if(_strnicmp(autoScreenShotParams[i].c_str(),"saveas-",7) == 0) {
	    		printf("Screenshot option [%s]\n",autoScreenShotParams[i].c_str());
	    		autoSaveScreenshotIndex = i;
	    		break;
	    	}
	    }
	    if(autoSaveScreenshotIndex >= 0) {
	    	string saveAsFilename = autoScreenShotParams[autoSaveScreenshotIndex];
	    	saveAsFilename.erase(0,7);
#ifdef WIN32
			FILE*f = _wfopen(utf8_decode(saveAsFilename).c_str(), L"rb");
#else
			FILE *f= fopen(saveAsFilename.c_str(), "rb");
#endif
			if(f == NULL) {
				renderer->saveScreen(saveAsFilename.c_str(),&overrideSize);
			}
			else {
				if(f) {
					fclose(f);
				}
			}
	    }
	    else {
			//string screenShotsPath = extractDirectoryPathFromFile(appPath) + string("screens/");
	        string userData = Config::getInstance().getString("UserData_Root","");
	        if(userData != "") {
	        	endPathWithSlash(userData);
	        }
	        string screenShotsPath = userData + string("screens/");
			printf("screenShotsPath [%s]\n",screenShotsPath.c_str());

	        if(isdir(screenShotsPath.c_str()) == false) {
	        	createDirectoryPaths(screenShotsPath);
	        }

			string path = screenShotsPath;
			if(isdir(path.c_str()) == true) {
				//Config &config= Config::getInstance();
				//string fileFormat = config.getString("ScreenShotFileType","jpg");

				for(int i=0; i < 5000; ++i) {
					path = screenShotsPath;
					path += string("screen") + intToStr(i) + string(".") + fileFormat;
#ifdef WIN32
					FILE*f= _wfopen(utf8_decode(path).c_str(), L"rb");
#else
					FILE *f= fopen(path.c_str(), "rb");
#endif
					if(f == NULL) {
						renderer->saveScreen(path,&overrideSize);
						break;
					}
					else {
						if(f) {
							fclose(f);
						}
					}
				}
			}
		}
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuFileSaveScreenshot(wxCommandEvent &event) {
	saveScreenshot();
}

void MainWindow::onMenuFileClearAll(wxCommandEvent &event) {
	try {
		//printf("Start onMenuFileClearAll\n");
		//fflush(stdout);

		modelPathList.clear();
		particlePathList.clear();
		particleProjectilePathList.clear();
		particleSplashPathList.clear(); // as above

		if(timer) timer->Stop();
		if(renderer) renderer->end();

		unitParticleSystems.clear();
		unitParticleSystemTypes.clear();

		projectileParticleSystems.clear();
		projectileParticleSystemTypes.clear();
		splashParticleSystems.clear(); // as above
		splashParticleSystemTypes.clear();

		//delete model;
		//model = NULL;
		if(model != NULL && renderer != NULL) renderer->endModel(rsGlobal, model);
		model = NULL;

		loadUnit("","");
		loadModel("");
		loadParticle("");
		loadProjectileParticle("");
		loadSplashParticle(""); // as above

		GetStatusBar()->SetStatusText(ToUnicode(statusbarText.c_str()));
		isControlKeyPressed = false;

		//printf("END onMenuFileClearAll\n");
		//fflush(stdout);

		if(timer) timer->Start(100);
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuFileExit(wxCommandEvent &event) {
	Close();
}

void MainWindow::loadUnit(string path, string skillName) {
	if(path != "" && fileExists(path) == true) {
		// std::cout << "Clearing list..." << std::endl;
		this->unitPath.first = path;
		this->unitPath.second.push_back(skillName);
	}

	try{
	if(this->unitPath.first != "") {
		if(timer) timer->Stop();
		if(renderer) renderer->end();

		string titlestring = winHeader;

		string unitPath = this->unitPath.first;
		string dir = unitPath;
		string name= lastDir(dir);
		string path= dir + "/" + name + ".xml";

		titlestring = unitPath  + " - "+ titlestring;

		std::string unitXML = path;

		string skillModelFile 				= "";
		string skillParticleFile 			= "";
		string skillParticleProjectileFile 	= "";
		string skillParticleSplashFile 		= "";
		bool fileFound = fileExists(unitXML);

		printf("Loading unit from file [%s] fileFound = %d\n",unitXML.c_str(),fileFound);

		if(fileFound == true) {
			XmlTree xmlTree;
			xmlTree.load(unitXML,Properties::getTagReplacementValues());
			const XmlNode *unitNode= xmlTree.getRootNode();

			bool foundSkillName = false;
			for(unsigned int skillIdx = 0; foundSkillName == false && skillIdx < this->unitPath.second.size(); ++skillIdx) {
				string lookipForSkillName = this->unitPath.second[skillIdx];

				const XmlNode *skillsNode= unitNode->getChild("skills");
				for(unsigned int i = 0; foundSkillName == false && i < skillsNode->getChildCount(); ++i) {
					const XmlNode *sn= skillsNode->getChild("skill", i);
					//const XmlNode *typeNode= sn->getChild("type");
					const XmlNode *nameNode= sn->getChild("name");
					string skillXmlName = nameNode->getAttribute("value")->getRestrictedValue();
					if(skillXmlName == lookipForSkillName) {
						printf("Found skill [%s]\n",lookipForSkillName.c_str());
						foundSkillName = true;

						if(sn->getChild("animation") != NULL) {
							skillModelFile = sn->getChild("animation")->getAttribute("path")->getRestrictedValue(unitPath + '/');
							printf("Found skill model [%s]\n",skillModelFile.c_str());
						}

						if(sn->hasChild("particles") == true) {
							const XmlNode *particlesNode= sn->getChild("particles");
							//for(int j = 0; particlesNode != NULL && particlesNode->getAttribute("value")->getRestrictedValue() == "true" &&
							//	j < particlesNode->getChildCount(); ++j) {
							if(particlesNode != NULL && particlesNode->getAttribute("value")->getRestrictedValue() == "true" &&
									particlesNode->hasChild("particle-file") == true) {
								const XmlNode *pf= particlesNode->getChild("particle-file");
								if(pf != NULL) {
									skillParticleFile = unitPath + '/' + pf->getAttribute("path")->getRestrictedValue();
									printf("Found skill particle [%s]\n",skillParticleFile.c_str());
								}
							}
						}
						if(sn->hasChild("projectile") == true) {
							const XmlNode *particlesProjectileNode= sn->getChild("projectile");
							//for(int j = 0; particlesProjectileNode != NULL && particlesProjectileNode->getAttribute("value")->getRestrictedValue() == "true" &&
							//	j < particlesProjectileNode->getChildCount(); ++j) {
							if(particlesProjectileNode != NULL && particlesProjectileNode->getAttribute("value")->getRestrictedValue() == "true" &&
								particlesProjectileNode->hasChild("particle") == true) {
								const XmlNode *pf= particlesProjectileNode->getChild("particle");
								if(pf != NULL && pf->getAttribute("value")->getRestrictedValue() == "true") {
									skillParticleProjectileFile = unitPath + '/' + pf->getAttribute("path")->getRestrictedValue();
									printf("Found skill skill projectile particle [%s]\n",skillParticleProjectileFile.c_str());
								}
							}
						}
						if(sn->hasChild("splash") == true) {
							const XmlNode *particlesSplashNode= sn->getChild("splash");
							//for(int j = 0; particlesSplashNode != NULL && particlesSplashNode->getAttribute("value")->getRestrictedValue() == "true" &&
							//	j < particlesSplashNode->getChildCount(); ++j) {
							if(particlesSplashNode != NULL && particlesSplashNode->getAttribute("value")->getRestrictedValue() == "true" &&
								particlesSplashNode->hasChild("particle") == true) {
								const XmlNode *pf= particlesSplashNode->getChild("particle");
								if(pf != NULL && pf->getAttribute("value")->getRestrictedValue() == "true") {
									skillParticleSplashFile = unitPath + '/' + pf->getAttribute("path")->getRestrictedValue();
									printf("Found skill skill splash particle [%s]\n",skillParticleSplashFile.c_str());
								}
							}
						}
					}
				}
			}

			if(skillModelFile != "") {
				this->modelPathList.push_back(skillModelFile);
				printf("Added skill model [%s]\n",skillModelFile.c_str());
			}
			if(skillParticleFile != "") {
				this->particlePathList.push_back(skillParticleFile);
				printf("Added skill particle [%s]\n",skillParticleFile.c_str());
			}
			if(skillParticleProjectileFile != "") {
				this->particleProjectilePathList.push_back(skillParticleProjectileFile);
				printf("Added skill projectile particle [%s]\n",skillParticleProjectileFile.c_str());
			}
			if(skillParticleSplashFile != "") {
				this->particleSplashPathList.push_back(skillParticleSplashFile);
				printf("Added skill splash particle [%s]\n",skillParticleSplashFile.c_str());
			}
		}
		SetTitle(ToUnicode(titlestring));
	}
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Not a Mega-Glest particle XML file, or broken"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::loadModel(string path) {
    try {
        if(path != "" && fileExists(path) == true) {
            this->modelPathList.push_back(path);
            printf("Adding model [%s] list size " MG_SIZE_T_SPECIFIER "\n",path.c_str(),this->modelPathList.size());
        }

        string titlestring=winHeader;
        for(unsigned int idx =0; idx < this->modelPathList.size(); idx++) {
            string modelPath = this->modelPathList[idx];

            //printf("Loading model [%s] %u of " MG_SIZE_T_SPECIFIER "\n",modelPath.c_str(),idx, this->modelPathList.size());

            if(timer) timer->Stop();
            //delete model;
    		if(model != NULL && renderer != NULL) renderer->endModel(rsGlobal, model);
    		model = NULL;
            model = renderer? renderer->newModel(rsGlobal, modelPath): NULL;

            statusbarText = getModelInfo();
            string statusTextValue = statusbarText + " animation speed: " + floatToStr(speed * 1000.0) + " anim value: " + floatToStr(anim) + " zoom: " + floatToStr(zoom) + " rotX: " + floatToStr(rotX) + " rotY: " + floatToStr(rotY);
            GetStatusBar()->SetStatusText(ToUnicode(statusTextValue.c_str()));
            if(timer) timer->Start(100);
            titlestring =  extractFileFromDirectoryPath(modelPath) + " - "+ titlestring;
        }
        SetTitle(ToUnicode(titlestring));
    }
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::loadParticle(string path) {
	if(timer) timer->Stop();

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] about to load [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,path.c_str());
	if(path != "" && fileExists(path) == true) {
		renderer->end();
		unitParticleSystems.clear();
		unitParticleSystemTypes.clear();

		if(isControlKeyPressed == true) {
			// std::cout << "Adding to list..." << std::endl;
			this->particlePathList.push_back(path);
		}
		else {
			// std::cout << "Clearing list..." << std::endl;
			this->particlePathList.clear();
			this->particlePathList.push_back(path);
		}

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] added file [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,path.c_str());
	}

	try {
	if(this->particlePathList.empty() == false) {
        string titlestring=winHeader;
		for(unsigned int idx = 0; idx < this->particlePathList.size(); idx++) {
			string particlePath = this->particlePathList[idx];
			string dir= extractDirectoryPathFromFile(particlePath);

			size_t pos = dir.find_last_of(folderDelimiter);
			if(pos == dir.length() - 1 && dir.length() > 0) {
				dir.erase(dir.length() -1);
			}

			particlePath= extractFileFromDirectoryPath(particlePath);
			titlestring = particlePath  + " - "+ titlestring;

			std::string unitXML = dir + folderDelimiter + extractFileFromDirectoryPath(dir) + ".xml";

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] looking for unit XML [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,unitXML.c_str());

			//int size   = -1;
			//int height = -1;
			int size  = 0;
			int height= 0;

			if(fileExists(unitXML) == true) {
				{
				XmlTree xmlTree;
				xmlTree.load(unitXML,Properties::getTagReplacementValues());
				const XmlNode *unitNode= xmlTree.getRootNode();
				const XmlNode *parametersNode= unitNode->getChild("parameters");
				//size
				size= parametersNode->getChild("size")->getAttribute("value")->getIntValue();
				//height
				height= parametersNode->getChild("height")->getAttribute("value")->getIntValue();
				}

                // std::cout << "About to load [" << particlePath << "] from [" << dir << "] unit [" << unitXML << "]" << std::endl;
			}
			else {
				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] unit XML NOT FOUND [%s] using default position values\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,unitXML.c_str());

				size  = 1;
				height= 1;
			}

			std::map<string,vector<pair<string, string> > > loadedFileList;
            UnitParticleSystemType *unitParticleSystemType = new UnitParticleSystemType();
            unitParticleSystemType->load(NULL, dir, dir + folderDelimiter + particlePath, //### if we knew which particle it was, we could be more accurate
            		renderer,loadedFileList,"g3dviewer","");
            unitParticleSystemTypes.push_back(unitParticleSystemType);

            for(std::vector<UnitParticleSystemType *>::const_iterator it= unitParticleSystemTypes.begin(); it != unitParticleSystemTypes.end(); ++it) {
                UnitParticleSystem *ups= new UnitParticleSystem(200);

                ups->setParticleType((*it));
                (*it)->setValues(ups);
                if(size > 0) {
                    //getCurrVectorFlat() + Vec3f(0.f, type->getHeight()/2.f, 0.f);
                    Vec3f vec = Vec3f(0.f, height / 2.f, 0.f);
                    ups->setPos(vec);
                }
                //ups->setFactionColor(getFaction()->getTexture()->getPixmap()->getPixel3f(0,0));
                ups->setFactionColor(renderer->getPlayerColorTexture(playerColor)->getPixmap()->getPixel3f(0,0));
                unitParticleSystems.push_back(ups);
                renderer->manageParticleSystem(ups);

                ups->setVisible(true);
            }

            if(path != "" && fileExists(path) == true) {
                renderer->initModelManager();
                if(initTextureManager) {
                	renderer->initTextureManager();
                }
            }

		}
		SetTitle(ToUnicode(titlestring));
	}
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Not a Mega-Glest particle XML file, or broken"), wxOK | wxICON_ERROR).ShowModal();
	}
	if(timer) timer->Start(100);
}

void MainWindow::loadProjectileParticle(string path) {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] about to load [%s] particleProjectilePathList.size() = " MG_SIZE_T_SPECIFIER "\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,path.c_str(),this->particleProjectilePathList.size());

	if(timer) timer->Stop();
	if(path != "" && fileExists(path) == true) {
		renderer->end();
		projectileParticleSystems.clear();
		projectileParticleSystemTypes.clear();

		if(isControlKeyPressed == true) {
			// std::cout << "Adding to list..." << std::endl;
			this->particleProjectilePathList.push_back(path);
		}
		else {
			// std::cout << "Clearing list..." << std::endl;
			this->particleProjectilePathList.clear();
			this->particleProjectilePathList.push_back(path);
		}
	}

	try {
	if(this->particleProjectilePathList.empty() == false) {
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("this->particleProjectilePathList.size() = " MG_SIZE_T_SPECIFIER "\n",this->particleProjectilePathList.size());

        string titlestring=winHeader;
		for(unsigned int idx = 0; idx < this->particleProjectilePathList.size(); idx++) {
			string particlePath = this->particleProjectilePathList[idx];
			string dir= extractDirectoryPathFromFile(particlePath);

			size_t pos = dir.find_last_of(folderDelimiter);
			if(pos == dir.length()-1) {
				dir.erase(dir.length() -1);
			}

			particlePath= extractFileFromDirectoryPath(particlePath);
			titlestring = particlePath  + " - "+ titlestring;

			std::string unitXML = dir + folderDelimiter + extractFileFromDirectoryPath(dir) + ".xml";

			int size   = 1;
			int height = 1;

			if(fileExists(unitXML) == true) {
				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] loading [%s] idx = %u\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,unitXML.c_str(),idx);

				XmlTree xmlTree;
				xmlTree.load(unitXML,Properties::getTagReplacementValues());
				const XmlNode *unitNode= xmlTree.getRootNode();
				const XmlNode *parametersNode= unitNode->getChild("parameters");
				//size
				size= parametersNode->getChild("size")->getAttribute("value")->getIntValue();
				//height
				height= parametersNode->getChild("height")->getAttribute("value")->getIntValue();
			}

			// std::cout << "About to load [" << particlePath << "] from [" << dir << "] unit [" << unitXML << "]" << std::endl;

			string particleFile = dir + folderDelimiter + particlePath;
			{
				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] loading [%s] idx = %u\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,particleFile.c_str(),idx);
				XmlTree xmlTree;
				xmlTree.load(particleFile,Properties::getTagReplacementValues());
				//const XmlNode *particleSystemNode= xmlTree.getRootNode();

				// std::cout << "Loaded successfully, loading values..." << std::endl;
			}

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] loading [%s] idx = %u\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,particleFile.c_str(),idx);
			std::map<string,vector<pair<string, string> > > loadedFileList;
			ParticleSystemTypeProjectile *projectileParticleSystemType= new ParticleSystemTypeProjectile();
			projectileParticleSystemType->load(NULL, dir, //### we don't know if there are overrides in the unit XML
					particleFile,renderer, loadedFileList,
					"g3dviewer","");

			// std::cout << "Values loaded, about to read..." << std::endl;

			projectileParticleSystemTypes.push_back(projectileParticleSystemType);

			for(std::vector<ParticleSystemTypeProjectile *>::const_iterator it= projectileParticleSystemTypes.begin();
					it != projectileParticleSystemTypes.end(); ++it) {

				ProjectileParticleSystem *ps = (*it)->create(NULL);

				if(size > 0) {
					Vec3f vec = Vec3f(0.f, height / 2.f, 0.f);
					//ps->setPos(vec);

					Vec3f vec2 = Vec3f(size * 2.f, height * 2.f, height * 2.f);
					ps->setPath(vec, vec2);
				}
				ps->setFactionColor(renderer->getPlayerColorTexture(playerColor)->getPixmap()->getPixel3f(0,0));

				projectileParticleSystems.push_back(ps);

				ps->setVisible(true);
				renderer->manageParticleSystem(ps);
			}

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] loaded [%s] idx = %u\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,particleFile.c_str(),idx);
		}
		SetTitle(ToUnicode(titlestring));

		if(path != "" && fileExists(path) == true) {
			renderer->initModelManager();
            if(initTextureManager) {
            	renderer->initTextureManager();
            }
		}
	}
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Not a Mega-Glest projectile particle XML file, or broken"), wxOK | wxICON_ERROR).ShowModal();
	}
	if(timer) timer->Start(100);
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] after load [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,path.c_str());
}

void MainWindow::loadSplashParticle(string path) {  // uses ParticleSystemTypeSplash::load  (and own list...)
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] about to load [%s] particleSplashPathList.size() = " MG_SIZE_T_SPECIFIER "\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,path.c_str(),this->particleSplashPathList.size());

	if(timer) timer->Stop();
	if(path != "" && fileExists(path) == true) {
		renderer->end();
		splashParticleSystems.clear();
		splashParticleSystemTypes.clear();

		if(isControlKeyPressed == true) {
			// std::cout << "Adding to list..." << std::endl;
			this->particleSplashPathList.push_back(path);
		}
		else {
			// std::cout << "Clearing list..." << std::endl;
			this->particleSplashPathList.clear();
			this->particleSplashPathList.push_back(path);
		}
	}

	try {
	if(this->particleSplashPathList.empty() == false) {
        string titlestring=winHeader;
		for(unsigned int idx = 0; idx < this->particleSplashPathList.size(); idx++) {
			string particlePath = this->particleSplashPathList[idx];
			string dir= extractDirectoryPathFromFile(particlePath);

			size_t pos = dir.find_last_of(folderDelimiter);
			if(pos == dir.length()-1) {
				dir.erase(dir.length() -1);
			}

			particlePath= extractFileFromDirectoryPath(particlePath);
			titlestring = particlePath  + " - "+ titlestring;

			std::string unitXML = dir + folderDelimiter + extractFileFromDirectoryPath(dir) + ".xml";

			int size   = 1;
			//int height = 1;

			if(fileExists(unitXML) == true) {
				XmlTree xmlTree;
				xmlTree.load(unitXML,Properties::getTagReplacementValues());
				const XmlNode *unitNode= xmlTree.getRootNode();
				const XmlNode *parametersNode= unitNode->getChild("parameters");
				//size
				size= parametersNode->getChild("size")->getAttribute("value")->getIntValue();
				//height
				//height= parametersNode->getChild("height")->getAttribute("value")->getIntValue();
			}

			// std::cout << "About to load [" << particlePath << "] from [" << dir << "] unit [" << unitXML << "]" << std::endl;

			{
				XmlTree xmlTree;
				xmlTree.load(dir + folderDelimiter + particlePath,Properties::getTagReplacementValues());
				//const XmlNode *particleSystemNode= xmlTree.getRootNode();
				// std::cout << "Loaded successfully, loading values..." << std::endl;
			}

			std::map<string,vector<pair<string, string> > > loadedFileList;
			ParticleSystemTypeSplash *splashParticleSystemType= new ParticleSystemTypeSplash();
			splashParticleSystemType->load(NULL, dir, dir + folderDelimiter + particlePath,renderer, //### we don't know if there are overrides in the unit XML
					loadedFileList,"g3dviewer",""); // <---- only that must be splash...

			// std::cout << "Values loaded, about to read..." << std::endl;

			splashParticleSystemTypes.push_back(splashParticleSystemType);

                                      //ParticleSystemTypeSplash
			for(std::vector<ParticleSystemTypeSplash *>::const_iterator it= splashParticleSystemTypes.begin(); it != splashParticleSystemTypes.end(); ++it) {

				SplashParticleSystem *ps = (*it)->create(NULL);

				if(size > 0) {
					//Vec3f vec = Vec3f(0.f, height / 2.f, 0.f);
					//ps->setPos(vec);

					//Vec3f vec2 = Vec3f(size * 2.f, height * 2.f, height * 2.f);   // <------- removed relative projectile
					//ps->setPath(vec, vec2);                                       // <------- removed relative projectile
				}
				ps->setFactionColor(renderer->getPlayerColorTexture(playerColor)->getPixmap()->getPixel3f(0,0));

				splashParticleSystems.push_back(ps);

				ps->setVisible(true);
				renderer->manageParticleSystem(ps);
			}
		}
		SetTitle(ToUnicode(titlestring));

		if(path != "" && fileExists(path) == true) {
			renderer->initModelManager();
            if(initTextureManager) {
            	renderer->initTextureManager();
            }
		}
	}
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Not a Mega-Glest projectile particle XML file, or broken"), wxOK | wxICON_ERROR).ShowModal();
	}
	if(timer) timer->Start(100);
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] after load [%s] particleSplashPathList.size() = " MG_SIZE_T_SPECIFIER "\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,path.c_str(),this->particleSplashPathList.size());
}

void MainWindow::onMenuModeNormals(wxCommandEvent &event){
	try {
		renderer->toggleNormals();
		menuMode->Check(miModeNormals, renderer->getNormals());
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuModeWireframe(wxCommandEvent &event){
	try {
		renderer->toggleWireframe();
		menuMode->Check(miModeWireframe, renderer->getWireframe());
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuModeGrid(wxCommandEvent &event){
	try {
		renderer->toggleGrid();
		menuMode->Check(miModeGrid, renderer->getGrid());
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuSpeedSlower(wxCommandEvent &event){
	try {
		speed /= 1.5f;
		if(speed < 0) {
			speed = 0;
		}

		string statusTextValue = statusbarText + " animation speed: " + floatToStr(speed * 1000.0) + " anim value: " + floatToStr(anim) + " zoom: " + floatToStr(zoom) + " rotX: " + floatToStr(rotX) + " rotY: " + floatToStr(rotY);
		GetStatusBar()->SetStatusText(ToUnicode(statusTextValue.c_str()));
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuSpeedFaster(wxCommandEvent &event){
	try {
		speed *= 1.5f;
		if(speed > 1) {
			speed = 1;
		}

		string statusTextValue = statusbarText + " animation speed: " + floatToStr(speed * 1000.0 ) + " anim value: " + floatToStr(anim) + " zoom: " + floatToStr(zoom) + " rotX: " + floatToStr(rotX) + " rotY: " + floatToStr(rotY);
		GetStatusBar()->SetStatusText(ToUnicode(statusTextValue.c_str()));
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

// set menu checkboxes to what player color is used
void MainWindow::onMenuColorRed(wxCommandEvent &event) {
	try {
		playerColor= Renderer::pcRed;
		menuCustomColor->Check(miColorRed, true);
		menuCustomColor->Check(miColorBlue, false);
		menuCustomColor->Check(miColorGreen, false);
		menuCustomColor->Check(miColorYellow, false);
		menuCustomColor->Check(miColorWhite, false);
		menuCustomColor->Check(miColorCyan, false);
		menuCustomColor->Check(miColorOrange, false);
		menuCustomColor->Check(miColorMagenta, false);
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuColorBlue(wxCommandEvent &event) {
	try {
		playerColor= Renderer::pcBlue;
		menuCustomColor->Check(miColorRed, false);
		menuCustomColor->Check(miColorBlue, true);
		menuCustomColor->Check(miColorGreen, false);
		menuCustomColor->Check(miColorYellow, false);
		menuCustomColor->Check(miColorWhite, false);
		menuCustomColor->Check(miColorCyan, false);
		menuCustomColor->Check(miColorOrange, false);
		menuCustomColor->Check(miColorMagenta, false);
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuColorGreen(wxCommandEvent &event) {
	try {
		playerColor= Renderer::pcGreen;
		menuCustomColor->Check(miColorRed, false);
		menuCustomColor->Check(miColorBlue, false);
		menuCustomColor->Check(miColorGreen, true);
		menuCustomColor->Check(miColorYellow, false);
		menuCustomColor->Check(miColorWhite, false);
		menuCustomColor->Check(miColorCyan, false);
		menuCustomColor->Check(miColorOrange, false);
		menuCustomColor->Check(miColorMagenta, false);
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuColorYellow(wxCommandEvent &event) {
	try {
		playerColor= Renderer::pcYellow;
		menuCustomColor->Check(miColorRed, false);
		menuCustomColor->Check(miColorBlue, false);
		menuCustomColor->Check(miColorGreen, false);
		menuCustomColor->Check(miColorYellow, true);
		menuCustomColor->Check(miColorWhite, false);
		menuCustomColor->Check(miColorCyan, false);
		menuCustomColor->Check(miColorOrange, false);
		menuCustomColor->Check(miColorMagenta, false);
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuColorWhite(wxCommandEvent &event) {
	try {
		playerColor= Renderer::pcWhite;
		menuCustomColor->Check(miColorRed, false);
		menuCustomColor->Check(miColorBlue, false);
		menuCustomColor->Check(miColorGreen, false);
		menuCustomColor->Check(miColorYellow, false);
		menuCustomColor->Check(miColorWhite, true);
		menuCustomColor->Check(miColorCyan, false);
		menuCustomColor->Check(miColorOrange, false);
		menuCustomColor->Check(miColorMagenta, false);
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuColorCyan(wxCommandEvent &event) {
	try {
		playerColor= Renderer::pcCyan;
		menuCustomColor->Check(miColorRed, false);
		menuCustomColor->Check(miColorBlue, false);
		menuCustomColor->Check(miColorGreen, false);
		menuCustomColor->Check(miColorYellow, false);
		menuCustomColor->Check(miColorWhite, false);
		menuCustomColor->Check(miColorCyan, true);
		menuCustomColor->Check(miColorOrange, false);
		menuCustomColor->Check(miColorMagenta, false);
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuColorOrange(wxCommandEvent &event) {
	try {
		playerColor= Renderer::pcOrange;
		menuCustomColor->Check(miColorRed, false);
		menuCustomColor->Check(miColorBlue, false);
		menuCustomColor->Check(miColorGreen, false);
		menuCustomColor->Check(miColorYellow, false);
		menuCustomColor->Check(miColorWhite, false);
		menuCustomColor->Check(miColorCyan, false);
		menuCustomColor->Check(miColorOrange, true);
		menuCustomColor->Check(miColorMagenta, false);
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuColorMagenta(wxCommandEvent &event) {
	try {
		playerColor= Renderer::pcMagenta;
		menuCustomColor->Check(miColorRed, false);
		menuCustomColor->Check(miColorBlue, false);
		menuCustomColor->Check(miColorGreen, false);
		menuCustomColor->Check(miColorYellow, false);
		menuCustomColor->Check(miColorWhite, false);
		menuCustomColor->Check(miColorCyan, false);
		menuCustomColor->Check(miColorOrange, false);
		menuCustomColor->Check(miColorMagenta, true);
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}


void MainWindow::onTimer(wxTimerEvent &event) {
	anim = anim + speed;
	if(anim > 1.0f){
		anim -= 1.f;
		resetAnimation = true;
	}

	onPaintNoEvent();
}

string MainWindow::getModelInfo() {
	string str;

	if(model != NULL) {
		str+= "Meshes: "+intToStr(model->getMeshCount());
		str+= ", Vertices: "+intToStr(model->getVertexCount());
		str+= ", Triangles: "+intToStr(model->getTriangleCount());
		str+= ", Version: "+intToStr(model->getFileVersion());
	}

	return str;
}

void MainWindow::onKeyDown(wxKeyEvent &e) {

	try {
		// std::cout << "e.ControlDown() = " << e.ControlDown() << " e.GetKeyCode() = " << e.GetKeyCode() << " isCtrl = " << (e.GetKeyCode() == WXK_CONTROL) << std::endl;

		// Note: This ctrl-key handling is buggy since it never resets when ctrl is released later, so I reset it at end of loadcommands for now.
		if(e.ControlDown() == true || e.GetKeyCode() == WXK_CONTROL) {
			isControlKeyPressed = true;
		}
		else {
			isControlKeyPressed = false;
		}

		// std::cout << "isControlKeyPressed = " << isControlKeyPressed << std::endl;


		// here also because + and - hotkeys don't work for numpad automaticly
		if (e.GetKeyCode() == 388) {
			speed *= 1.5f; //numpad+
			if(speed > 1.0) {
				speed = 1.0;
			}

			string statusTextValue = statusbarText + " animation speed: " + floatToStr(speed * 1000.0) + " anim value: " + floatToStr(anim) + " zoom: " + floatToStr(zoom) + " rotX: " + floatToStr(rotX) + " rotY: " + floatToStr(rotY);
			GetStatusBar()->SetStatusText(ToUnicode(statusTextValue.c_str()));

		}
		else if (e.GetKeyCode() == 390) {
			speed /= 1.5f; //numpad-
			if(speed < 0) {
				speed = 0;
			}
			string statusTextValue = statusbarText + " animation speed: " + floatToStr(speed * 1000.0) + " anim value: " + floatToStr(anim) + " zoom: " + floatToStr(zoom) + " rotX: " + floatToStr(rotX) + " rotY: " + floatToStr(rotY);
			GetStatusBar()->SetStatusText(ToUnicode(statusTextValue.c_str()));
		}
		else if (e.GetKeyCode() == 'W') {
			glClearColor(0.6f, 0.6f, 0.6f, 1.0f); //w  key //backgroundcolor constant 0.3 -> 0.6

		}

		// some posibility to adjust brightness:
		/*
		else if (e.GetKeyCode() == 322) { // Ins  - Grid
			gridBrightness += 0.1f; if (gridBrightness >1.0) gridBrightness =1.0;
		}
		else if (e.GetKeyCode() == 127) { // Del
			gridBrightness -= 0.1f; if (gridBrightness <0) gridBrightness =0;
		}
		*/
		else if (e.GetKeyCode() == 313) { // Home  - Background
			backBrightness += 0.1f; if (backBrightness >1.0) backBrightness=1.0;
			glClearColor(backBrightness, backBrightness, backBrightness, 1.0f);
		}
		else if (e.GetKeyCode() == 312) { // End
			backBrightness -= 0.1f; if (backBrightness<0) backBrightness=0;
			glClearColor(backBrightness, backBrightness, backBrightness, 1.0f);
		}
		else if (e.GetKeyCode() == 366) { // PgUp  - Lightning of model
			lightBrightness += 0.1f; if (lightBrightness >1.0) lightBrightness =1.0;
			Vec4f ambientNEW= Vec4f(lightBrightness, lightBrightness, lightBrightness, 1.0f);
			glLightfv(GL_LIGHT0,GL_AMBIENT, ambientNEW.ptr());
		}
		else if (e.GetKeyCode() == 367) { // pgDn
			lightBrightness -= 0.1f; if (lightBrightness <0) lightBrightness =0;
			Vec4f ambientNEW= Vec4f(lightBrightness, lightBrightness, lightBrightness, 1.0f);
			glLightfv(GL_LIGHT0,GL_AMBIENT, ambientNEW.ptr());
		}

		std::cout << "pressed " << e.GetKeyCode() << std::endl;
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuRestart(wxCommandEvent &event) {
	onMenuRestartNoEvent();
}

void MainWindow::onMenuRestartNoEvent() {
	try {
		// std::cout << "pressed R (restart particle animation)" << std::endl;
		if(timer) timer->Stop();
		if(renderer) renderer->end();

		unitParticleSystems.clear();
		unitParticleSystemTypes.clear();
		projectileParticleSystems.clear();
		projectileParticleSystemTypes.clear();
		splashParticleSystems.clear(); // as above
		splashParticleSystemTypes.clear();

		loadUnit("", "");
		loadModel("");
		loadParticle("");
		loadProjectileParticle("");
		loadSplashParticle(""); // as above

		renderer->initModelManager();
        if(initTextureManager) {
        	renderer->initTextureManager();
        }
        if(timer) timer->Start(100);
	}
	catch(std::runtime_error &e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

BEGIN_EVENT_TABLE(MainWindow, wxFrame)
	EVT_TIMER(-1, MainWindow::onTimer)
	EVT_CLOSE(MainWindow::onClose)
	EVT_MENU(miFileLoad, MainWindow::onMenuFileLoad)
	EVT_MENU(miFileLoadParticleXML, MainWindow::onMenuFileLoadParticleXML)
	EVT_MENU(miFileLoadProjectileParticleXML, MainWindow::onMenuFileLoadProjectileParticleXML)
	EVT_MENU(miFileLoadSplashParticleXML, MainWindow::onMenuFileLoadSplashParticleXML)
	EVT_MENU(miFileClearAll, MainWindow::onMenuFileClearAll)
	EVT_MENU(miFileToggleScreenshotTransparent, MainWindow::onMenumFileToggleScreenshotTransparent)
	EVT_MENU(miFileSaveScreenshot, MainWindow::onMenuFileSaveScreenshot)
	EVT_MENU(wxID_EXIT, MainWindow::onMenuFileExit)

	EVT_MENU(miModeWireframe, MainWindow::onMenuModeWireframe)
	EVT_MENU(miModeNormals, MainWindow::onMenuModeNormals)
	EVT_MENU(miModeGrid, MainWindow::onMenuModeGrid)

	EVT_MENU(miSpeedFaster, MainWindow::onMenuSpeedFaster)
	EVT_MENU(miSpeedSlower, MainWindow::onMenuSpeedSlower)
	EVT_MENU(miRestart, MainWindow::onMenuRestart)

	EVT_MENU(miColorRed, MainWindow::onMenuColorRed)
	EVT_MENU(miColorBlue, MainWindow::onMenuColorBlue)
	EVT_MENU(miColorGreen, MainWindow::onMenuColorGreen)
	EVT_MENU(miColorYellow, MainWindow::onMenuColorYellow)
	EVT_MENU(miColorWhite, MainWindow::onMenuColorWhite)
	EVT_MENU(miColorCyan, MainWindow::onMenuColorCyan)
	EVT_MENU(miColorOrange, MainWindow::onMenuColorOrange)
	EVT_MENU(miColorMagenta, MainWindow::onMenuColorMagenta)

	EVT_MENU(miChangeBackgroundColor, MainWindow::OnChangeColor)
END_EVENT_TABLE()

// =====================================================
//	class GlCanvas
// =====================================================

void translateCoords(wxWindow *wnd, int &x, int &y) {
#ifdef WIN32
	int cx, cy;
	wnd->GetPosition(&cx, &cy);
	x += cx;
	y += cy;
#endif
}

// to prevent flicker
GlCanvas::GlCanvas(MainWindow *	mainWindow, int *args)
#if wxCHECK_VERSION(2, 9, 1)
		: wxGLCanvas(mainWindow, wxID_ANY, args, wxDefaultPosition, mainWindow->GetClientSize(), wxFULL_REPAINT_ON_RESIZE, wxT("GLCanvas")) {
	this->context = new wxGLContext(this);
#else
		: wxGLCanvas(mainWindow, -1, wxDefaultPosition, wxDefaultSize, 0, wxT("GLCanvas"), args) {
	this->context = NULL;
#endif
	this->mainWindow = mainWindow;
}

GlCanvas::~GlCanvas() {
	if(this->context) {
		delete this->context;
	}
	this->context = NULL;
}

void GlCanvas::setCurrentGLContext() {
#ifndef __APPLE__

#if wxCHECK_VERSION(3, 0, 0)
	//printf("Setting glcontext 3x!\n");

	//if(!IsShown()) {}
	if(this->context == NULL) {
		//printf("Make new ctx!\n");
		this->context = new wxGLContext(this);
		//printf("Set ctx [%p]\n",this->context);
	}
#elif wxCHECK_VERSION(2, 9, 1)
	//printf("Setting glcontext 29x!\n");

	//if(!IsShown()) {}
	if(this->context == NULL) {
		this->context = new wxGLContext(this);
		//printf("Set ctx [%p]\n",this->context);
	}
#endif
	//printf("Set ctx [%p]\n",this->context);
	if(this->context) {
		wxGLCanvas::SetCurrent(*this->context);
		//printf("Set ctx2 [%p]\n",this->context);
	}
#else
	this->SetCurrent();
#endif
}

// for the mousewheel
void GlCanvas::onMouseWheel(wxMouseEvent &event) {
	if(event.GetWheelRotation()>0) mainWindow->onMouseWheelDown(event);
	else mainWindow->onMouseWheelUp(event);
}

void GlCanvas::onMouseMove(wxMouseEvent &event){
    mainWindow->onMouseMove(event);
}

void GlCanvas::onKeyDown(wxKeyEvent &event) {
	int x, y;
	event.GetPosition(&x, &y);
	translateCoords(this, x, y);
	mainWindow->onKeyDown(event);
}

void GlCanvas::OnSize(wxSizeEvent&event) {

	//printf("OnSize %dx%d\n",event.m_size.GetWidth(),event.m_size.GetHeight());
	Update();
}

//    EVT_SPIN_DOWN(GlCanvas::onMouseDown)
//    EVT_SPIN_UP(GlCanvas::onMouseDown)
//    EVT_MIDDLE_DOWN(GlCanvas::onMouseWheel)
//    EVT_MIDDLE_UP(GlCanvas::onMouseWheel)

BEGIN_EVENT_TABLE(GlCanvas, wxGLCanvas)
    EVT_MOUSEWHEEL(GlCanvas::onMouseWheel)
    EVT_MOTION(GlCanvas::onMouseMove)
    EVT_KEY_DOWN(GlCanvas::onKeyDown)
    EVT_SIZE(GlCanvas::OnSize)
END_EVENT_TABLE()

// ===============================================
// 	class App
// ===============================================

bool App::OnInit() {
	SystemFlags::VERBOSE_MODE_ENABLED  = false;

#if defined(wxMAJOR_VERSION) && defined(wxMINOR_VERSION) && defined(wxRELEASE_NUMBER) && defined(wxSUBRELEASE_NUMBER)
	printf("Using wxWidgets version [%d.%d.%d.%d]\n",wxMAJOR_VERSION,wxMINOR_VERSION,wxRELEASE_NUMBER,wxSUBRELEASE_NUMBER);
#endif

	string modelPath="";
	string particlePath="";
	string projectileParticlePath="";
	string splashParticlePath="";


	bool foundInvalidArgs = false;
	const int knownArgCount = sizeof(GAME_ARGS) / sizeof(GAME_ARGS[0]);
	for(int idx = 1; idx < argc; ++idx) {
#if wxCHECK_VERSION(2, 9, 1)
		const wxWX2MBbuf tmp_buf = wxConvCurrent->cWX2MB(argv[idx].wc_str());
#else
		const wxWX2MBbuf tmp_buf = wxConvCurrent->cWX2MB(argv[idx]);
#endif
if( hasCommandArgument(knownArgCount, (wxChar**)&GAME_ARGS[0], (const char *)tmp_buf, NULL, 0, true) == false &&
			argv[idx][0] == '-') {
			foundInvalidArgs = true;

			printf("\nInvalid argument: %s",(const char*)tmp_buf);
		}
	}

    if(foundInvalidArgs == true ||
    	hasCommandArgument(argc, argv,(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_HELP])) == true) {
		printParameterHelp(static_cast<const char*>(WX2CHR(argv[0])), foundInvalidArgs);
		return false;
    }

    if(hasCommandArgument(argc, argv,(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_VERBOSE])) == true) {
    	SystemFlags::VERBOSE_MODE_ENABLED  = true;
    }

    if(hasCommandArgument(argc, argv,(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_AUTO_SCREENSHOT])) == true) {
    	autoScreenShotAndExit = true;

    	const wxWX2MBbuf param = (const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_AUTO_SCREENSHOT]);
    	//printf("param = [%s]\n",(const char*)param);

    	int foundParamIndIndex = -1;
        hasCommandArgument(argc, argv,string((const char*)param) + string("="),&foundParamIndIndex);
        if(foundParamIndIndex < 0) {
            hasCommandArgument(argc, argv,(const char*)param,&foundParamIndIndex);
        }
        //printf("foundParamIndIndex = %d\n",foundParamIndIndex);
#if wxCHECK_VERSION(2, 9, 1)
		string options = argv[foundParamIndIndex].ToStdString();
#else
        string options = (const char *)wxConvCurrent->cWX2MB(argv[foundParamIndIndex]);
#endif
        vector<string> paramPartTokens;
        Tokenize(options,paramPartTokens,"=");
        if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
            string optionsValue = paramPartTokens[1];

            autoScreenShotParams.clear();
            Tokenize(optionsValue,autoScreenShotParams,",");

			for(unsigned int i = 0; i < autoScreenShotParams.size(); ++i) {

#ifdef WIN32
				auto_ptr<wchar_t> wstr(Ansi2WideString(autoScreenShotParams[i].c_str()));
				autoScreenShotParams[i] = utf8_encode(wstr.get());
#endif

				if(_strnicmp(autoScreenShotParams[i].c_str(),"resize-",7) == 0) {
					printf("Screenshot option [%s]\n",autoScreenShotParams[i].c_str());

					string resize = autoScreenShotParams[i];
					resize = resize.erase(0,7);
					vector<string> values;
					Tokenize(resize,values,"x");
					overrideSize.first = strToInt(values[0]);
					overrideSize.second = strToInt(values[1]);

					Renderer::windowX= 0;
					Renderer::windowY= 0;
					Renderer::windowW = overrideSize.first;
					Renderer::windowH = overrideSize.second + 25;
				}
			}
        }
    }

    std::pair<string,vector<string> > unitToLoad;
    if(hasCommandArgument(argc, argv,(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_UNIT])) == true) {
    	const wxWX2MBbuf param = (const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_UNIT]);
    	//printf("param = [%s]\n",(const char*)param);

    	int foundParamIndIndex = -1;
        hasCommandArgument(argc, argv,string((const char*)param) + string("="),&foundParamIndIndex);
        if(foundParamIndIndex < 0) {
            hasCommandArgument(argc, argv,(const char*)param,&foundParamIndIndex);
        }
        //printf("foundParamIndIndex = %d\n",foundParamIndIndex);
        string customPath = static_cast<const char*>(WX2CHR(argv[foundParamIndIndex]));
        vector<string> paramPartTokens;
        Tokenize(customPath,paramPartTokens,"=");
        if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
            string customPathValue = paramPartTokens[1];
            std::vector<string> delimitedList;
            Tokenize(customPathValue,delimitedList,",");

            if(delimitedList.size() >= 2) {
            	unitToLoad.first = delimitedList[0];
  				#ifdef WIN32
            	auto_ptr<wchar_t> wstr(Ansi2WideString(unitToLoad.first.c_str()));
				unitToLoad.first = utf8_encode(wstr.get());
				#endif

            	for(unsigned int i = 1; i < delimitedList.size(); ++i) {
					string value = delimitedList[i];
  					#ifdef WIN32
					auto_ptr<wchar_t> wstr(Ansi2WideString(value.c_str()));
					value = utf8_encode(wstr.get());
					#endif

            		unitToLoad.second.push_back(value);
            	}
            }
            else {
				printf("\nInvalid path specified on commandline [%s] value [%s]\n\n", static_cast<const char*>(WX2CHR(argv[foundParamIndIndex])), (paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
				printParameterHelp(WX2CHR(argv[0]),false);
            	return false;
            }

        }
        else {
            printf("\nInvalid path specified on commandline [%s] value [%s]\n\n", static_cast<const char*>(WX2CHR(argv[foundParamIndIndex])),(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
            printParameterHelp(WX2CHR(argv[0]),false);
            return false;
        }
    }

    if(hasCommandArgument(argc, argv,(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_MODEL])) == true &&
    	hasCommandArgument(argc, argv,(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_MODEL_ANIMATION_VALUE])) == false) {
    	const wxWX2MBbuf param = (const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_MODEL]);
    	//printf("param = [%s]\n",(const char*)param);

    	int foundParamIndIndex = -1;
        hasCommandArgument(argc, argv,string((const char*)param) + string("="),&foundParamIndIndex);
        if(foundParamIndIndex < 0) {
            hasCommandArgument(argc, argv,(const char*)param,&foundParamIndIndex);
        }
        //printf("foundParamIndIndex = %d\n",foundParamIndIndex);
        string customPath = (const char*)WX2CHR(argv[foundParamIndIndex]);
        vector<string> paramPartTokens;
        Tokenize(customPath,paramPartTokens,"=");
        if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
            string customPathValue = paramPartTokens[1];
            modelPath = customPathValue;
			#ifdef WIN32
            auto_ptr<wchar_t> wstr(Ansi2WideString(modelPath.c_str()));
			modelPath = utf8_encode(wstr.get());
			#endif

        }
        else {
            printf("\nInvalid path specified on commandline [%s] value [%s]\n\n",(const char*)WX2CHR(argv[foundParamIndIndex]),(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
            printParameterHelp(WX2CHR(argv[0]),false);
            return false;
        }
    }

    if(hasCommandArgument(argc, argv,(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_PARTICLE])) == true &&
       hasCommandArgument(argc, argv,(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_PARTICLE_LOOP_VALUE])) == false) {
    	const wxWX2MBbuf param = (const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_PARTICLE]);
    	//printf("param = [%s]\n",(const char*)param);

    	int foundParamIndIndex = -1;
        hasCommandArgument(argc, argv,string((const char*)param) + string("="),&foundParamIndIndex);
        if(foundParamIndIndex < 0) {
            hasCommandArgument(argc, argv,(const char*)param,&foundParamIndIndex);
        }
        //printf("foundParamIndIndex = %d\n",foundParamIndIndex);
        string customPath = (const char*)WX2CHR(argv[foundParamIndIndex]);
        vector<string> paramPartTokens;
        Tokenize(customPath,paramPartTokens,"=");
        if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
            string customPathValue = paramPartTokens[1];
            particlePath = customPathValue;
			#ifdef WIN32
            auto_ptr<wchar_t> wstr(Ansi2WideString(particlePath.c_str()));
			particlePath = utf8_encode(wstr.get());
			#endif
        }
        else {
            printf("\nInvalid path specified on commandline [%s] value [%s]\n\n", (const char*)WX2CHR(argv[foundParamIndIndex]),(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
            printParameterHelp(WX2CHR(argv[0]),false);
            return false;
        }
    }

    if(hasCommandArgument(argc, argv,(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_PARTICLE_PROJECTILE])) == true) {
    	const wxWX2MBbuf param = (const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_PARTICLE_PROJECTILE]);
    	//printf("param = [%s]\n",(const char*)param);

    	int foundParamIndIndex = -1;
        hasCommandArgument(argc, argv,string((const char*)param) + string("="),&foundParamIndIndex);
        if(foundParamIndIndex < 0) {
            hasCommandArgument(argc, argv,(const char*)param,&foundParamIndIndex);
        }
        //printf("foundParamIndIndex = %d\n",foundParamIndIndex);
        string customPath = (const char*)WX2CHR(argv[foundParamIndIndex]);
        vector<string> paramPartTokens;
        Tokenize(customPath,paramPartTokens,"=");
        if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
            string customPathValue = paramPartTokens[1];
            projectileParticlePath = customPathValue;
			#ifdef WIN32
            auto_ptr<wchar_t> wstr(Ansi2WideString(projectileParticlePath.c_str()));
			projectileParticlePath = utf8_encode(wstr.get());
			#endif
        }
        else {
            printf("\nInvalid path specified on commandline [%s] value [%s]\n\n", (const char*)WX2CHR(argv[foundParamIndIndex]),(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
            printParameterHelp(WX2CHR(argv[0]),false);
            return false;
        }
    }

    if(hasCommandArgument(argc, argv,(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_PARTICLE_SPLASH])) == true) {
    	const wxWX2MBbuf param = (const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_PARTICLE_SPLASH]);
    	//printf("param = [%s]\n",(const char*)param);

    	int foundParamIndIndex = -1;
        hasCommandArgument(argc, argv,string((const char*)param) + string("="),&foundParamIndIndex);
        if(foundParamIndIndex < 0) {
            hasCommandArgument(argc, argv,(const char*)param,&foundParamIndIndex);
        }
        //printf("foundParamIndIndex = %d\n",foundParamIndIndex);
        string customPath = (const char*)WX2CHR(argv[foundParamIndIndex]);
        vector<string> paramPartTokens;
        Tokenize(customPath,paramPartTokens,"=");
        if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
            string customPathValue = paramPartTokens[1];
            splashParticlePath = customPathValue;
			#ifdef WIN32
            auto_ptr<wchar_t> wstr(Ansi2WideString(splashParticlePath.c_str()));
			splashParticlePath = utf8_encode(wstr.get());
			#endif
        }
        else {
            printf("\nInvalid path specified on commandline [%s] value [%s]\n\n", (const char*)WX2CHR(argv[foundParamIndIndex]),(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
            printParameterHelp(WX2CHR(argv[0]),false);
            return false;
        }
    }

	float newAnimValue = 0.0f;
    if(hasCommandArgument(argc, argv,(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_MODEL_ANIMATION_VALUE])) == true) {
    	const wxWX2MBbuf param = (const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_MODEL_ANIMATION_VALUE]);

    	int foundParamIndIndex = -1;
        hasCommandArgument(argc, argv,string((const char*)param) + string("="),&foundParamIndIndex);
        if(foundParamIndIndex < 0) {
            hasCommandArgument(argc, argv,(const char*)param,&foundParamIndIndex);
        }
        //printf("foundParamIndIndex = %d\n",foundParamIndIndex);
        string value = (const char*)WX2CHR(argv[foundParamIndIndex]);
        vector<string> paramPartTokens;
        Tokenize(value,paramPartTokens,"=");
        if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
        	newAnimValue = strToFloat(paramPartTokens[1]);
        	printf("newAnimValue = %f\n",newAnimValue);
        }
        else {
            printf("\nInvalid path specified on commandline [%s] value [%s]\n\n", (const char*)WX2CHR(argv[foundParamIndIndex]),(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
            printParameterHelp(WX2CHR(argv[0]),false);
            return false;
        }
    }

	int newParticleLoopValue = 1;
    if(hasCommandArgument(argc, argv,(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_PARTICLE_LOOP_VALUE])) == true) {
    	const wxWX2MBbuf param = (const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_PARTICLE_LOOP_VALUE]);

    	int foundParamIndIndex = -1;
        hasCommandArgument(argc, argv,string((const char*)param) + string("="),&foundParamIndIndex);
        if(foundParamIndIndex < 0) {
            hasCommandArgument(argc, argv,(const char*)param,&foundParamIndIndex);
        }
        //printf("foundParamIndIndex = %d\n",foundParamIndIndex);
        string value = (const char*)WX2CHR(argv[foundParamIndIndex]);
        vector<string> paramPartTokens;
        Tokenize(value,paramPartTokens,"=");
        if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
        	newParticleLoopValue = strToInt(paramPartTokens[1]);
        	//printf("newParticleLoopValue = %d\n",newParticleLoopValue);
        }
        else {
            printf("\nInvalid path specified on commandline [%s] value [%s]\n\n", (const char*)WX2CHR(argv[foundParamIndIndex]),(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
            printParameterHelp(WX2CHR(argv[0]),false);
            return false;
        }
    }

	float newZoomValue = 1.0f;
    if(hasCommandArgument(argc, argv,(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_ZOOM_VALUE])) == true) {
    	const wxWX2MBbuf param = (const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_ZOOM_VALUE]);

    	int foundParamIndIndex = -1;
        hasCommandArgument(argc, argv,string((const char*)param) + string("="),&foundParamIndIndex);
        if(foundParamIndIndex < 0) {
            hasCommandArgument(argc, argv,(const char*)param,&foundParamIndIndex);
        }
        //printf("foundParamIndIndex = %d\n",foundParamIndIndex);
        string value = (const char*)WX2CHR(argv[foundParamIndIndex]);
        vector<string> paramPartTokens;
        Tokenize(value,paramPartTokens,"=");
        if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
        	newZoomValue = strToFloat(paramPartTokens[1]);
        	//printf("newAnimValue = %f\n",newAnimValue);
        }
        else {
            printf("\nInvalid path specified on commandline [%s] value [%s]\n\n", (const char*)WX2CHR(argv[foundParamIndIndex]),(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
            printParameterHelp(WX2CHR(argv[0]),false);
            return false;
        }
    }

	float newXRotValue = 0.0f;
    if(hasCommandArgument(argc, argv,(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_ROTATE_X_VALUE])) == true) {
    	const wxWX2MBbuf param = (const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_ROTATE_X_VALUE]);

    	int foundParamIndIndex = -1;
        hasCommandArgument(argc, argv,string((const char*)param) + string("="),&foundParamIndIndex);
        if(foundParamIndIndex < 0) {
            hasCommandArgument(argc, argv,(const char*)param,&foundParamIndIndex);
        }
        //printf("foundParamIndIndex = %d\n",foundParamIndIndex);
        string value = (const char*)WX2CHR(argv[foundParamIndIndex]);
        vector<string> paramPartTokens;
        Tokenize(value,paramPartTokens,"=");
        if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
        	newXRotValue = strToFloat(paramPartTokens[1]);
        	//printf("newAnimValue = %f\n",newAnimValue);
        }
        else {
            printf("\nInvalid path specified on commandline [%s] value [%s]\n\n", (const char*)WX2CHR(argv[foundParamIndIndex]),(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
            printParameterHelp(WX2CHR(argv[0]),false);
            return false;
        }
    }

	float newYRotValue = 0.0f;
    if(hasCommandArgument(argc, argv,(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_ROTATE_Y_VALUE])) == true) {
    	const wxWX2MBbuf param = (const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_ROTATE_Y_VALUE]);

    	int foundParamIndIndex = -1;
        hasCommandArgument(argc, argv,string((const char*)param) + string("="),&foundParamIndIndex);
        if(foundParamIndIndex < 0) {
            hasCommandArgument(argc, argv,(const char*)param,&foundParamIndIndex);
        }
        //printf("foundParamIndIndex = %d\n",foundParamIndIndex);
        string value = static_cast<const char*>(WX2CHR(argv[foundParamIndIndex]));
        vector<string> paramPartTokens;
        Tokenize(value,paramPartTokens,"=");
        if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
        	newYRotValue = strToFloat(paramPartTokens[1]);
        	//printf("newAnimValue = %f\n",newAnimValue);
        }
        else {
            printf("\nInvalid path specified on commandline [%s] value [%s]\n\n", static_cast<const char*>(WX2CHR(argv[foundParamIndIndex])),(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
            printParameterHelp(WX2CHR(argv[0]),false);
            return false;
        }
    }

    if(hasCommandArgument(argc, argv,(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_SCREENSHOT_FORMAT])) == true) {
    	const wxWX2MBbuf param = (const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_SCREENSHOT_FORMAT]);

    	int foundParamIndIndex = -1;
        hasCommandArgument(argc, argv,string((const char*)param) + string("="),&foundParamIndIndex);
        if(foundParamIndIndex < 0) {
            hasCommandArgument(argc, argv,(const char*)param,&foundParamIndIndex);
        }
        //printf("foundParamIndIndex = %d\n",foundParamIndIndex);
        string value = static_cast<const char*>(WX2CHR(argv[foundParamIndIndex]));
        vector<string> paramPartTokens;
        Tokenize(value,paramPartTokens,"=");
        if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
        	fileFormat = paramPartTokens[1];
        }
        else {
            printf("\nInvalid value specified on commandline [%s] value [%s]\n\n", static_cast<const char*>(WX2CHR(argv[foundParamIndIndex])), (paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
            printParameterHelp(WX2CHR(argv[0]),false);
            return false;
        }
    }

	if(argc == 2 && argv[1][0] != '-') {
		modelPath = static_cast<const char*>(WX2CHR(argv[1]));
	}

//#if defined(__MINGW32__)
//		const wxWX2MBbuf tmp_buf = wxConvCurrent->cWX2MB(wxFNCONV(argv[0]));
//		appPath = tmp_buf;
//#else
//		appPath = wxFNCONV(argv[0]);
//#endif
//	printf("appPath [%s]\n",argv[0]);
	
	wxString exe_path = wxStandardPaths::Get().GetExecutablePath();
    //wxString path_separator = wxFileName::GetPathSeparator();
    //exe_path = exe_path.BeforeLast(path_separator[0]);
    //exe_path += path_separator;
	string appPath(static_cast<const char*>(WX2CHR(exe_path)));

//#else
//		appPath = wxFNCONV(exe_path);
//#endif

//	printf("#2 appPath [%s]\n",appPath.c_str());

	mainWindow= new MainWindow(	unitToLoad,
								modelPath,
								particlePath,
								projectileParticlePath,
								splashParticlePath,
								newAnimValue,
								newParticleLoopValue,
								newZoomValue,
								newXRotValue,
								newYRotValue,
								appPath);
	if(autoScreenShotAndExit == true) {
#if !defined(WIN32)
		mainWindow->Iconize(true);
#endif		
	}
	mainWindow->Show();
	mainWindow->init();
	mainWindow->Update();
	mainWindow->setupTimer();

	return true;
}

int App::MainLoop(){
	try{
		return wxApp::MainLoop();
	}
	catch(const exception &e){
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Exception"), wxOK | wxICON_ERROR).ShowModal();
		return 1;
	}
	return 0;
}

int App::OnExit(){
	SystemFlags::Close();
	SystemFlags::SHUTDOWN_PROGRAM_MODE=true;

	return 0;
}

}}//end namespace

IMPLEMENT_APP_CONSOLE(Shared::G3dViewer::App)
