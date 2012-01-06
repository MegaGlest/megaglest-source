// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2005 Matthias Braun
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================
#ifndef _SHARED_PLATFORM_MAIN_H_
#define _SHARED_PLATFORM_MAIN_H_

#include <SDL.h>
#include <iostream>
#include <GL/glew.h>
#include "leak_dumper.h"

#ifndef WIN32
  #define stricmp strcasecmp
  #define strnicmp strncasecmp
  #define _strnicmp strncasecmp
#endif

const char  *GAME_ARGS[] = {
	"--help",

	"--autostart-lastgame",
	"--connecthost",
	"--starthost",
	"--headless-server-mode",
	"--use-ports",

	"--load-scenario",
	"--load-mod",
	"--preview-map",
	"--version",
	"--opengl-info",
	"--sdl-info",
	"--lua-info",
	"--curl-info",
	"--validate-techtrees",
	"--validate-factions",
	"--validate-scenario",
	"--data-path",
	"--ini-path",
	"--log-path",
	"--show-ini-settings",
	"--convert-models",
	"--use-language",
	"--show-map-crc",
	"--show-tileset-crc",
	"--show-techtree-crc",
	"--show-scenario-crc",
	"--show-path-crc",
	"--disable-backtrace",
	"--disable-sigsegv-handler",
	"--disable-vbo",
	"--disable-sound",
	"--enable-legacyfonts",
	"--force-ftglfonts",

	"--resolution",
	"--colorbits",
	"--depthbits",
	"--fullscreen",
	"--set-gamma",

	"--use-font",
	"--font-basesize",
	"--verbose"

};

enum GAME_ARG_TYPE {
	GAME_ARG_HELP = 0,

	GAME_ARG_AUTOSTART_LASTGAME,
	GAME_ARG_CLIENT,
	GAME_ARG_SERVER,
	GAME_ARG_MASTERSERVER_MODE,
	GAME_ARG_USE_PORTS,

	GAME_ARG_LOADSCENARIO,
	GAME_ARG_MOD,
	GAME_ARG_PREVIEW_MAP,
	GAME_ARG_VERSION,
	GAME_ARG_OPENGL_INFO,
	GAME_ARG_SDL_INFO,
	GAME_ARG_LUA_INFO,
	GAME_ARG_CURL_INFO,
	GAME_ARG_VALIDATE_TECHTREES,
	GAME_ARG_VALIDATE_FACTIONS,
	GAME_ARG_VALIDATE_SCENARIO,
	GAME_ARG_DATA_PATH,
	GAME_ARG_INI_PATH,
	GAME_ARG_LOG_PATH,
	GAME_ARG_SHOW_INI_SETTINGS,
	GAME_ARG_CONVERT_MODELS,
	GAME_ARG_USE_LANGUAGE,

	GAME_ARG_SHOW_MAP_CRC,
	GAME_ARG_SHOW_TILESET_CRC,
	GAME_ARG_SHOW_TECHTREE_CRC,
	GAME_ARG_SHOW_SCENARIO_CRC,
	GAME_ARG_SHOW_PATH_CRC,

	GAME_ARG_DISABLE_BACKTRACE,
	GAME_ARG_DISABLE_SIGSEGV_HANDLER,
	GAME_ARG_DISABLE_VBO,
	GAME_ARG_DISABLE_SOUND,
	GAME_ARG_ENABLE_LEGACYFONTS,
	GAME_ARG_FORCE_FTGLFONTS,

	GAME_ARG_USE_RESOLUTION,
	GAME_ARG_USE_COLORBITS,
	GAME_ARG_USE_DEPTHBITS,
	GAME_ARG_USE_FULLSCREEN,
	GAME_ARG_SET_GAMMA,

	GAME_ARG_USE_FONT,
	GAME_ARG_FONT_BASESIZE,
	GAME_ARG_VERBOSE_MODE,

	GAME_ARG_END
};

void printParameterHelp(const char *argv0, bool foundInvalidArgs) {
	//                           MAX WIDTH FOR MAN PAGE
	//     "================================================================================"
	if(foundInvalidArgs == true) {
			printf("\n");
	}
	printf("\n%s, usage\n\n",extractFileFromDirectoryPath(argv0).c_str());
	printf("Commandline Parameter:\t\tDescription:");
	printf("\n----------------------\t\t------------");
	printf("\n%s\t\t\t\tdisplays this help text.",GAME_ARGS[GAME_ARG_HELP]);
	printf("\n%s\t\tAutomatically starts a game with the last game",GAME_ARGS[GAME_ARG_AUTOSTART_LASTGAME]);
	printf("\n\t\t\t\tsettings you played.");
	printf("\n%s=x\t\t\tAuto connect to host server at IP or hostname x",GAME_ARGS[GAME_ARG_CLIENT]);
	printf("\n%s\t\t\tAuto create a host server.",GAME_ARGS[GAME_ARG_SERVER]);

	printf("\n%s=x,x\tRun as a headless server.",GAME_ARGS[GAME_ARG_MASTERSERVER_MODE]);
	printf("\n                     \t\tWhere x is an optional comma delimited command");
	printf("\n                     \t\t        list of one or more of the following: ");
	printf("\n                     \t\texit - which quits the application after a game");
	printf("\n                     \t               has no more connected players.");
	printf("\n                     \t\tvps  - which does NOT read commands from the");
	printf("\n                     \t               local console (for some vps's).");

	printf("\n%s=x,y\t\t\tForce hosted games to listen internally on port",GAME_ARGS[GAME_ARG_USE_PORTS]);
	printf("\n\t\t\t\tx, externally on port y.");
	printf("\n                     \t\tWhere x is the internal port # on the local");
	printf("\n                     \t\t        machine to listen for connects");
	printf("\n                     \t\t      y is the external port # on the");
	printf("\n                     \t\t        router/proxy to forward connection");
	printf("\n                     \t\t        from to the internal port #");
	printf("\n                     \t\t*NOTE: If enabled the FTP Server port #'s will");
	printf("\n                     \t\t       be set to x+1 to x+9");

	printf("\n%s=x\t\tAuto load a scenario by scenario name.",GAME_ARGS[GAME_ARG_LOADSCENARIO]);
	printf("\n%s=x\t\t\tAuto load a mod by mod pathname.",GAME_ARGS[GAME_ARG_MOD]);

	//     "================================================================================"
	printf("\n%s=x\t\t\tAuto Preview a map by map name.",GAME_ARGS[GAME_ARG_PREVIEW_MAP]);
	printf("\n%s\t\t\tdisplays the version string of this program.",GAME_ARGS[GAME_ARG_VERSION]);
	printf("\n%s\t\t\tdisplays your video driver's OpenGL info.",GAME_ARGS[GAME_ARG_OPENGL_INFO]);
	printf("\n%s\t\t\tdisplays your SDL version information.",GAME_ARGS[GAME_ARG_SDL_INFO]);
	printf("\n%s\t\t\tdisplays your LUA version information.",GAME_ARGS[GAME_ARG_LUA_INFO]);
	printf("\n%s\t\t\tdisplays your CURL version information.",GAME_ARGS[GAME_ARG_CURL_INFO]);
	printf("\n%s=x=purgeunused=purgeduplicates=svndelete=hideduplicates",GAME_ARGS[GAME_ARG_VALIDATE_TECHTREES]);
	printf("\n                     \t\tdisplay a report detailing any known problems");
	printf("\n                     \t\trelated to your selected techtrees game data.");
	printf("\n                     \t\tWhere x is a comma-delimited list of techtrees");
	printf("\n                     \t\t        to validate.");
	printf("\n                     \t\tWhere purgeunused is an optional parameter");
	printf("\n                     \t\t      telling the validation to delete");
	printf("\n                     \t\t      extra files in the techtree that are");
	printf("\n                     \t\t      not used.");
	printf("\n                     \t\tWhere purgeduplicates is an optional parameter");
	printf("\n                     \t\t      telling the validation to merge");
	printf("\n                     \t\t      duplicate files in the techtree.");
	printf("\n                     \t\tWhere svndelete is an optional parameter");
	printf("\n                     \t\t      telling the validation to call");
	printf("\n                     \t\t      svn delete on duplicate / unused");
	printf("\n                     \t\t      files in the techtree.");
	printf("\n                     \t\tWhere hideduplicates is an optional parameter");
	printf("\n                     \t\t      telling the validation to NOT SHOW");
	printf("\n                     \t\t      duplicate files in the techtree.");
	printf("\n                     \t\t*NOTE: This only applies when files are");
	printf("\n                     \t\t       purged due to the above flags being set.");
	printf("\n                     \t\texample:");
	printf("\n                     %s %s=megapack,vbros_pack_5",extractFileFromDirectoryPath(argv0).c_str(),GAME_ARGS[GAME_ARG_VALIDATE_TECHTREES]);
	printf("\n%s=x=purgeunused=purgeduplicates=hideduplicates",GAME_ARGS[GAME_ARG_VALIDATE_FACTIONS]);
	printf("\n                     \t\tdisplay a report detailing any known problems");
	printf("\n                     \t\trelated to your selected factions game data.");
	printf("\n                     \t\tWhere x is a comma-delimited list of factions");
	printf("\n                     \t\t        to validate.");
	printf("\n                     \t\tWhere purgeunused is an optional parameter");
	printf("\n                     \t\t      telling the validation to delete");
	printf("\n                     \t\t      extra files in the faction that are");
	printf("\n                     \t\t      not used.");
	printf("\n                     \t\tWhere purgeduplicates is an optional parameter");
	printf("\n                     \t\t      telling the validation to merge");
	printf("\n                     \t\t      duplicate files in the faction.");
	printf("\n                     \t\tWhere hideduplicates is an optional parameter");
	printf("\n                     \t\t      telling the validation to NOT SHOW");
	printf("\n                     \t\t      duplicate files in the techtree.");
	printf("\n                     \t\t*NOTE: leaving the list empty is the same as");
	printf("\n                     \t\trunning: %s",GAME_ARGS[GAME_ARG_VALIDATE_TECHTREES]);
	printf("\n                     \t\texample:");
	printf("\n                     %s %s=tech,egypt",extractFileFromDirectoryPath(argv0).c_str(),GAME_ARGS[GAME_ARG_VALIDATE_FACTIONS]);

	printf("\n%s=x=purgeunused=svndelete",GAME_ARGS[GAME_ARG_VALIDATE_SCENARIO]);
	printf("\n                     \t\tdisplay a report detailing any known problems");
	printf("\n                     \t\trelated to your selected scenario game data.");
	printf("\n                     \t\tWhere x is a single scenario to validate.");
	printf("\n                     \t\tWhere purgeunused is an optional parameter");
	printf("\n                     \t\t      telling the validation to delete extra");
	printf("\n                     \t\t      files in the scenario that are not used.");
	printf("\n                     \t\texample:");
	printf("\n                     %s %s=stranded",extractFileFromDirectoryPath(argv0).c_str(),GAME_ARGS[GAME_ARG_VALIDATE_SCENARIO]);

	//     "================================================================================"
	printf("\n%s=x\t\t\tSets the game data path to x",GAME_ARGS[GAME_ARG_DATA_PATH]);
	printf("\n                     \t\texample:");
	printf("\n                     %s %s=/usr/local/game_data/",extractFileFromDirectoryPath(argv0).c_str(),GAME_ARGS[GAME_ARG_DATA_PATH]);
	printf("\n%s=x\t\t\tSets the game ini path to x",GAME_ARGS[GAME_ARG_INI_PATH]);
	printf("\n                     \t\texample");
	printf("\n                     %s %s=~/game_config/",extractFileFromDirectoryPath(argv0).c_str(),GAME_ARGS[GAME_ARG_INI_PATH]);
	printf("\n%s=x\t\t\tSets the game logs path to x",GAME_ARGS[GAME_ARG_LOG_PATH]);
	printf("\n                     \t\texample:");
	printf("\n                     %s %s=~/game_logs/",extractFileFromDirectoryPath(argv0).c_str(),GAME_ARGS[GAME_ARG_LOG_PATH]);
	printf("\n%s=x\t\tdisplay merged ini settings information.",GAME_ARGS[GAME_ARG_SHOW_INI_SETTINGS]);
	printf("\n                     \t\tWhere x is an optional property name to");
	printf("\n                     \t\t        filter (default shows all).");
	printf("\n                     \t\texample:");
	printf("\n                     %s %s=DebugMode",extractFileFromDirectoryPath(argv0).c_str(),GAME_ARGS[GAME_ARG_SHOW_INI_SETTINGS]);

	printf("\n%s=x=textureformat=keepsmallest",GAME_ARGS[GAME_ARG_CONVERT_MODELS]);
	printf("\n                     \t\tConvert a model file or folder to the current g3d");
	printf("\n                     \t\tversion format.");
	printf("\n                     \t\tWhere x is a filename or folder containing the g3d");
	printf("\n                     \t\t        model(s).");
	printf("\n                     \t\tWhere textureformat is an optional supported");
	printf("\n                     \t\t      texture format to convert to (tga,bmp,jpg,png).");
	printf("\n                     \t\tWhere keepsmallest is an optional flag indicating");
	printf("\n                     \t\t      to keep original texture if its filesize is");
	printf("\n                     \t\t      smaller than the converted format.");
	printf("\n                     \t\texample:");
	printf("\n  %s %s=techs/megapack/factions/tech/units/castle/models/castle.g3d=png=keepsmallest",extractFileFromDirectoryPath(argv0).c_str(),GAME_ARGS[GAME_ARG_CONVERT_MODELS]);

	printf("\n%s=x\t\tforce the language to be the language specified by x.",GAME_ARGS[GAME_ARG_USE_LANGUAGE]);
	printf("\n                     \t\tWhere x is a language filename or ISO639-1 code.");
	printf("\n                     \t\texample: %s %s=english",extractFileFromDirectoryPath(argv0).c_str(),GAME_ARGS[GAME_ARG_USE_LANGUAGE]);
	printf("\n                     \t\texample: %s %s=en",extractFileFromDirectoryPath(argv0).c_str(),GAME_ARGS[GAME_ARG_USE_LANGUAGE]);


	printf("\n%s=x\t\tshow the calculated CRC for the map named x.",GAME_ARGS[GAME_ARG_SHOW_MAP_CRC]);
	printf("\n                     \t\tWhere x is a map name.");
	printf("\n                     \t\texample:");
	printf("\n                     %s %s=four_rivers",extractFileFromDirectoryPath(argv0).c_str(),GAME_ARGS[GAME_ARG_SHOW_MAP_CRC]);

	printf("\n%s=x\t\tshow the calculated CRC for the tileset named x.",GAME_ARGS[GAME_ARG_SHOW_TILESET_CRC]);
	printf("\n                     \t\tWhere x is a tileset name.");
	printf("\n                     \t\texample:");
	printf("\n                     %s %s=forest",extractFileFromDirectoryPath(argv0).c_str(),GAME_ARGS[GAME_ARG_SHOW_TILESET_CRC]);

	printf("\n%s=x\t\tshow the calculated CRC for the techtree named x.",GAME_ARGS[GAME_ARG_SHOW_TECHTREE_CRC]);
	printf("\n                     \t\tWhere x is a techtree name.");
	printf("\n                     \t\texample:");
	printf("\n                     %s %s=megapack",extractFileFromDirectoryPath(argv0).c_str(),GAME_ARGS[GAME_ARG_SHOW_TECHTREE_CRC]);

	printf("\n%s=x\t\tshow the calculated CRC for the scenario named x.",GAME_ARGS[GAME_ARG_SHOW_SCENARIO_CRC]);
	printf("\n                     \t\tWhere x is a scenario name.");
	printf("\n                     \t\texample:");
	printf("\n                     %s %s=storming",extractFileFromDirectoryPath(argv0).c_str(),GAME_ARGS[GAME_ARG_SHOW_SCENARIO_CRC]);

	//     "================================================================================"
	printf("\n%s=x=y",GAME_ARGS[GAME_ARG_SHOW_PATH_CRC]);
	printf("\n                     \t\tShow the calculated CRC for files in the path located");
	printf("\n                     \t\tin x using file filter y.");
	printf("\n                     \t\tWhere x is a path name.");
	printf("\n                     \t\tand y is file(s) filter.");
	printf("\n                     \t\texample:");
	printf("\n                     %s %s=techs/=megapack.7z",extractFileFromDirectoryPath(argv0).c_str(),GAME_ARGS[GAME_ARG_SHOW_PATH_CRC]);

	printf("\n%s\t\tdisables stack backtrace on errors.",GAME_ARGS[GAME_ARG_DISABLE_BACKTRACE]);

	printf("\n%s\tdisables the sigsegv error handler.",GAME_ARGS[GAME_ARG_DISABLE_SIGSEGV_HANDLER]);


	printf("\n%s\t\t\tdisables trying to use Vertex Buffer Objects.",GAME_ARGS[GAME_ARG_DISABLE_VBO]);
	printf("\n%s\t\t\tdisables the sound system.",GAME_ARGS[GAME_ARG_DISABLE_SOUND]);

	printf("\n%s\t\tenables using the legacy font system.",GAME_ARGS[GAME_ARG_ENABLE_LEGACYFONTS]);

	printf("\n%s\t\tforces use of the FTGL font system.",GAME_ARGS[GAME_ARG_FORCE_FTGLFONTS]);

//	printf("\n%s=x\t\t\toverride video settings.",GAME_ARGS[GAME_ARG_USE_VIDEO_SETTINGS]);
//	printf("\n                     \t\tWhere x is a string with the following format:");
//	printf("\n                     \t\twidthxheightxcolorbitsxdepthbitsxfullscreen");
//	printf("\n                     \t\twhere * indicates not to replace the default value for the parameter");
//	printf("\n                     \t\tfullscreen has possible values of true, false, 1 or 0");
//	printf("\n                     \t\tand only the width and height parameters are required (the others are optional)");
//	printf("\n                     \t\texample: %s %s=1024x768x*x*",extractFileFromDirectoryPath(argv0).c_str(),GAME_ARGS[GAME_ARG_USE_VIDEO_SETTINGS]);
//	printf("\n                     \t\tsame result for: %s %s=1024x768",extractFileFromDirectoryPath(argv0).c_str(),GAME_ARGS[GAME_ARG_USE_VIDEO_SETTINGS]);


	//     "================================================================================"
	printf("\n%s=x\t\t\toverride the video resolution.",GAME_ARGS[GAME_ARG_USE_RESOLUTION]);
	printf("\n                     \t\tWhere x is a string with the following format:");
	printf("\n                     \t\twidthxheight");
	printf("\n                     \t\texample: %s %s=1024x768",extractFileFromDirectoryPath(argv0).c_str(),GAME_ARGS[GAME_ARG_USE_RESOLUTION]);

	printf("\n%s=x\t\t\toverride the video colorbits.",GAME_ARGS[GAME_ARG_USE_COLORBITS]);
	printf("\n                     \t\tWhere x is a valid colorbits value supported by");
	printf("\n                     \t\t        your video driver");
	printf("\n                     \t\texample: %s %s=32",extractFileFromDirectoryPath(argv0).c_str(),GAME_ARGS[GAME_ARG_USE_COLORBITS]);

	printf("\n%s=x\t\t\toverride the video depthbits.",GAME_ARGS[GAME_ARG_USE_DEPTHBITS]);
	printf("\n                     \t\tWhere x is a valid depthbits value supported by");
	printf("\n                     \t\t        your video driver");
	printf("\n                     \t\texample: %s %s=24",extractFileFromDirectoryPath(argv0).c_str(),GAME_ARGS[GAME_ARG_USE_DEPTHBITS]);

	printf("\n%s=x\t\t\toverride the video fullscreen mode.",GAME_ARGS[GAME_ARG_USE_FULLSCREEN]);
	printf("\n                     \t\tWhere x either true or false");
	printf("\n                     \t\texample: %s %s=true",extractFileFromDirectoryPath(argv0).c_str(),GAME_ARGS[GAME_ARG_USE_FULLSCREEN]);

	printf("\n%s=x\t\t\toverride the video gamma (contrast) value.",GAME_ARGS[GAME_ARG_SET_GAMMA]);
	printf("\n                     \t\tWhere x a floating point value");
	printf("\n                     \t\texample: %s %s=1.5",extractFileFromDirectoryPath(argv0).c_str(),GAME_ARGS[GAME_ARG_SET_GAMMA]);

	printf("\n%s=x\t\t\toverride the font to use.",GAME_ARGS[GAME_ARG_USE_FONT]);
	printf("\n                     \t\tWhere x is the path and name of a font file supported");
	printf("\n                     \t\t        by freetype2.");
	printf("\n                     \t\texample:");
	printf("\n  %s %s=$APPLICATIONDATAPATH/data/core/fonts/Vera.ttf",extractFileFromDirectoryPath(argv0).c_str(),GAME_ARGS[GAME_ARG_USE_FONT]);

	printf("\n%s=x\t\t\toverride the font base size.",GAME_ARGS[GAME_ARG_FONT_BASESIZE]);
	printf("\n                     \t\tWhere x is the numeric base font size to use.");
	printf("\n                     \t\texample: %s %s=5",extractFileFromDirectoryPath(argv0).c_str(),GAME_ARGS[GAME_ARG_FONT_BASESIZE]);

	printf("\n%s\t\t\tdisplays verbose information in the console.",GAME_ARGS[GAME_ARG_VERBOSE_MODE]);

	printf("\n\n");
}

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

int mainSetup(int argc, char **argv) {
    bool haveSpecialOutputCommandLineOption = false;

    const int knownArgCount = sizeof(GAME_ARGS) / sizeof(GAME_ARGS[0]);
    if(knownArgCount != GAME_ARG_END) {
    	char szBuf[1024]="";
    	sprintf(szBuf,"Internal arg count mismatch knownArgCount = %d, GAME_ARG_END = %d",knownArgCount,GAME_ARG_END);
    	throw runtime_error("");
    }

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

	if( haveSpecialOutputCommandLineOption == false) {
#ifdef USE_STREFLOP
#define STREFLOP_NO_DENORMALS
	streflop_init<streflop::Simple>();
#endif
	}

	if(hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_HELP])) == true    ||
	   hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_VERSION])) == true ||
	   hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_SHOW_INI_SETTINGS])) == true ||
	   hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_MASTERSERVER_MODE])) == true) {
	     // Use this for masterserver mode for timers like Chrono
		 if(SDL_Init(SDL_INIT_TIMER) < 0)  {
	        std::cerr << "Couldn't initialize SDL: " << SDL_GetError() << "\n";
	        return 1;
	     }
	}
	else {
		 if(SDL_Init(SDL_INIT_EVERYTHING) < 0)  {
			std::cerr << "Couldn't initialize SDL: " << SDL_GetError() << "\n";
			return 1;
		 }
		SDL_EnableUNICODE(1);
		SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	}
	return 0;
}

#define MAIN_FUNCTION(X) int main(int argc, char **argv) {                                     \
	int result = mainSetup(argc,argv);														   \
	if(result == 0) {																		   \
		result = X(argc, argv);                                                                \
	}																						   \
    return result;                                                                             \
}

#endif
