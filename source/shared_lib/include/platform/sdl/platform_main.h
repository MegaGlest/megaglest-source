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
	"--load-scenario",
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
	"--disable-vbo",
	"--disable-sound",
	"--enable-legacyfonts",
//	"--use-video-settings",

	"--resolution",
	"--colorbits",
	"--depthbits",
	"--fullscreen",
	//"--windowed",

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
	GAME_ARG_LOADSCENARIO,
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
	GAME_ARG_DISABLE_VBO,
	GAME_ARG_DISABLE_SOUND,
	GAME_ARG_ENABLE_LEGACYFONTS,
	//GAME_ARG_USE_VIDEO_SETTINGS,
	GAME_ARG_USE_RESOLUTION,
	GAME_ARG_USE_COLORBITS,
	GAME_ARG_USE_DEPTHBITS,
	GAME_ARG_USE_FULLSCREEN,

	GAME_ARG_USE_FONT,
	GAME_ARG_FONT_BASESIZE,
	GAME_ARG_VERBOSE_MODE
};

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

	printf("\n%s=x\t\t\tRun as a headless server.",GAME_ARGS[GAME_ARG_MASTERSERVER_MODE]);
	printf("\n                     \t\tWhere x is an option command: exit which quits the application after a game has no more connected players.");

	printf("\n%s=x\t\tAuto loads the specified scenario by scenario name.",GAME_ARGS[GAME_ARG_LOADSCENARIO]);
	printf("\n%s=x\t\tAuto Preview the specified map by map name.",GAME_ARGS[GAME_ARG_PREVIEW_MAP]);
	printf("\n%s\t\t\tdisplays the version string of this program.",GAME_ARGS[GAME_ARG_VERSION]);
	printf("\n%s\t\t\tdisplays your video driver's OpenGL information.",GAME_ARGS[GAME_ARG_OPENGL_INFO]);
	printf("\n%s\t\t\tdisplays your SDL version information.",GAME_ARGS[GAME_ARG_SDL_INFO]);
	printf("\n%s\t\t\tdisplays your LUA version information.",GAME_ARGS[GAME_ARG_LUA_INFO]);
	printf("\n%s\t\t\tdisplays your CURL version information.",GAME_ARGS[GAME_ARG_CURL_INFO]);
	printf("\n%s=x=purgeunused=purgeduplicates=svndelete=hideduplicates",GAME_ARGS[GAME_ARG_VALIDATE_TECHTREES]);
	printf("\n                     \t\tdisplays a report detailing any known problems related to your selected techtrees game data.");
	printf("\n                     \t\tWhere x is a comma-delimited list of techtrees to validate.");
	printf("\n                     \t\tWhere purgeunused is an optional parameter telling the validation to delete extra files in the techtree that are not used.");
	printf("\n                     \t\tWhere purgeduplicates is an optional parameter telling the validation to merge duplicate files in the techtree.");
	printf("\n                     \t\tWhere svndelete is an optional parameter telling the validation to call svn delete on duplicate / unused files in the techtree.");
	printf("\n                     \t\tWhere hideduplicates is an optional parameter telling the validation to NOT SHOW duplicate files in the techtree.");
	printf("\n                     \t\t*NOTE: This only applies when files are purged due to the above flags being set.");
	printf("\n                     \t\texample: %s %s=megapack,vbros_pack_5",argv0,GAME_ARGS[GAME_ARG_VALIDATE_TECHTREES]);
	printf("\n%s=x=purgeunused=purgeduplicates=hideduplicates",GAME_ARGS[GAME_ARG_VALIDATE_FACTIONS]);
	printf("\n                     \t\tdisplays a report detailing any known problems related to your selected factions game data.");
	printf("\n                     \t\tWhere x is a comma-delimited list of factions to validate.");
	printf("\n                     \t\tWhere purgeunused is an optional parameter telling the validation to delete extra files in the faction that are not used.");
	printf("\n                     \t\tWhere purgeduplicates is an optional parameter telling the validation to merge duplicate files in the faction.");
	printf("\n                     \t\tWhere hideduplicates is an optional parameter telling the validation to NOT SHOW duplicate files in the techtree.");
	printf("\n                     \t\t*NOTE: leaving the list empty is the same as running");
	printf("\n                     \t\t%s",GAME_ARGS[GAME_ARG_VALIDATE_TECHTREES]);
	printf("\n                     \t\texample: %s %s=tech,egypt",argv0,GAME_ARGS[GAME_ARG_VALIDATE_FACTIONS]);

	printf("\n%s=x=purgeunused=svndelete\t\tdisplays a report detailing any known problems related",GAME_ARGS[GAME_ARG_VALIDATE_SCENARIO]);
	printf("\n                     \t\tto your selected scenario game data.");
	printf("\n                     \t\tWhere x is a single scenario to validate.");
	printf("\n                     \t\tWhere purgeunused is an optional parameter telling the validation to delete extra files in the scenario that are not used.");
	printf("\n                     \t\texample: %s %s=stranded",argv0,GAME_ARGS[GAME_ARG_VALIDATE_SCENARIO]);

	printf("\n%s=x\t\t\tSets the game data path to x",GAME_ARGS[GAME_ARG_DATA_PATH]);
	printf("\n                     \t\texample: %s %s=/usr/local/game_data/",argv0,GAME_ARGS[GAME_ARG_DATA_PATH]);
	printf("\n%s=x\t\t\tSets the game ini path to x",GAME_ARGS[GAME_ARG_INI_PATH]);
	printf("\n                     \t\texample: %s %s=~/game_config/",argv0,GAME_ARGS[GAME_ARG_INI_PATH]);
	printf("\n%s=x\t\t\tSets the game logs path to x",GAME_ARGS[GAME_ARG_LOG_PATH]);
	printf("\n                     \t\texample: %s %s=~/game_logs/",argv0,GAME_ARGS[GAME_ARG_LOG_PATH]);
	printf("\n%s=x\t\t\tdisplays merged ini settings information.",GAME_ARGS[GAME_ARG_SHOW_INI_SETTINGS]);
	printf("\n                     \t\tWhere x is an optional property name to filter (default shows all).");
	printf("\n                     \t\texample: %s %s=DebugMode",argv0,GAME_ARGS[GAME_ARG_SHOW_INI_SETTINGS]);

	printf("\n%s=x=textureformat=keepsmallest\t\tconvert a model file or folder to the current g3d version format.",GAME_ARGS[GAME_ARG_CONVERT_MODELS]);
	printf("\n                     \t\tWhere x is a filename or folder containing the g3d model(s).");
	printf("\n                     \t\tWhere textureformat is an optional supported texture format to convert to (tga,bmp,jpg,png).");
	printf("\n                     \t\tWhere keepsmallest is an optional flag indicating to keep original texture if its filesize is smaller than the converted format.");
	printf("\n                     \t\texample: %s %s=techs/megapack/factions/tech/units/castle/models/castle.g3d=png=keepsmallest",argv0,GAME_ARGS[GAME_ARG_CONVERT_MODELS]);

	printf("\n%s=x\t\tforce the language to be the language specified by x.",GAME_ARGS[GAME_ARG_USE_LANGUAGE]);
	printf("\n                     \t\tWhere x is a supported language (such as english).");
	printf("\n                     \t\texample: %s %s=english",argv0,GAME_ARGS[GAME_ARG_USE_LANGUAGE]);


	printf("\n%s=x\t\tshow the calculated CRC for the map named x.",GAME_ARGS[GAME_ARG_SHOW_MAP_CRC]);
	printf("\n                     \t\tWhere x is a map name.");
	printf("\n                     \t\texample: %s %s=four_rivers",argv0,GAME_ARGS[GAME_ARG_SHOW_MAP_CRC]);

	printf("\n%s=x\t\tshow the calculated CRC for the tileset named x.",GAME_ARGS[GAME_ARG_SHOW_TILESET_CRC]);
	printf("\n                     \t\tWhere x is a tileset name.");
	printf("\n                     \t\texample: %s %s=forest",argv0,GAME_ARGS[GAME_ARG_SHOW_TILESET_CRC]);

	printf("\n%s=x\t\tshow the calculated CRC for the techtree named x.",GAME_ARGS[GAME_ARG_SHOW_TECHTREE_CRC]);
	printf("\n                     \t\tWhere x is a techtree name.");
	printf("\n                     \t\texample: %s %s=megapack",argv0,GAME_ARGS[GAME_ARG_SHOW_TECHTREE_CRC]);

	printf("\n%s=x\t\tshow the calculated CRC for the scenario named x.",GAME_ARGS[GAME_ARG_SHOW_SCENARIO_CRC]);
	printf("\n                     \t\tWhere x is a scenario name.");
	printf("\n                     \t\texample: %s %s=storming",argv0,GAME_ARGS[GAME_ARG_SHOW_SCENARIO_CRC]);

	printf("\n%s=x=y\t\tshow the calculated CRC for files in the path located in x using file filter y.",GAME_ARGS[GAME_ARG_SHOW_PATH_CRC]);
	printf("\n                     \t\tWhere x is a path name.");
	printf("\n                     \t\tand y is file(s) filter.");
	printf("\n                     \t\texample: %s %s=techs/=megapack.7z",argv0,GAME_ARGS[GAME_ARG_SHOW_PATH_CRC]);

	printf("\n%s\t\tdisables stack backtrace on errors.",GAME_ARGS[GAME_ARG_DISABLE_BACKTRACE]);
	printf("\n%s\t\t\tdisables trying to use Vertex Buffer Objects.",GAME_ARGS[GAME_ARG_DISABLE_VBO]);
	printf("\n%s\t\t\tdisables the sound system.",GAME_ARGS[GAME_ARG_DISABLE_SOUND]);

	printf("\n%s\t\t\tenables using the legacy font system.",GAME_ARGS[GAME_ARG_ENABLE_LEGACYFONTS]);


//	printf("\n%s=x\t\t\toverride video settings.",GAME_ARGS[GAME_ARG_USE_VIDEO_SETTINGS]);
//	printf("\n                     \t\tWhere x is a string with the following format:");
//	printf("\n                     \t\twidthxheightxcolorbitsxdepthbitsxfullscreen");
//	printf("\n                     \t\twhere * indicates not to replace the default value for the parameter");
//	printf("\n                     \t\tfullscreen has possible values of true, false, 1 or 0");
//	printf("\n                     \t\tand only the width and height parameters are required (the others are optional)");
//	printf("\n                     \t\texample: %s %s=1024x768x*x*",argv0,GAME_ARGS[GAME_ARG_USE_VIDEO_SETTINGS]);
//	printf("\n                     \t\tsame result for: %s %s=1024x768",argv0,GAME_ARGS[GAME_ARG_USE_VIDEO_SETTINGS]);



	printf("\n%s=x\t\t\toverride the video resolution.",GAME_ARGS[GAME_ARG_USE_RESOLUTION]);
	printf("\n                     \t\tWhere x is a string with the following format:");
	printf("\n                     \t\twidthxheight");
	printf("\n                     \t\texample: %s %s=1024x768",argv0,GAME_ARGS[GAME_ARG_USE_RESOLUTION]);

	printf("\n%s=x\t\t\toverride the video colorbits.",GAME_ARGS[GAME_ARG_USE_COLORBITS]);
	printf("\n                     \t\tWhere x is a valid colorbits value supported by your video driver");
	printf("\n                     \t\texample: %s %s=32",argv0,GAME_ARGS[GAME_ARG_USE_COLORBITS]);

	printf("\n%s=x\t\t\toverride the video depthbits.",GAME_ARGS[GAME_ARG_USE_DEPTHBITS]);
	printf("\n                     \t\tWhere x is a valid depthbits value supported by your video driver");
	printf("\n                     \t\texample: %s %s=24",argv0,GAME_ARGS[GAME_ARG_USE_DEPTHBITS]);

	printf("\n%s=x\t\t\toverride the video fullscreen mode.",GAME_ARGS[GAME_ARG_USE_FULLSCREEN]);
	printf("\n                     \t\tWhere x either true or false");
	printf("\n                     \t\texample: %s %s=true",argv0,GAME_ARGS[GAME_ARG_USE_FULLSCREEN]);

	printf("\n%s=x\t\t\toverride the font to use.",GAME_ARGS[GAME_ARG_USE_FONT]);
	printf("\n                     \t\tWhere x is the path and name of a font file support by freetype2.");
	printf("\n                     \t\texample: %s %s=$APPLICATIONDATAPATH/data/core/fonts/Vera.ttf",argv0,GAME_ARGS[GAME_ARG_USE_FONT]);

	printf("\n%s=x\t\t\toverride the font base size.",GAME_ARGS[GAME_ARG_FONT_BASESIZE]);
	printf("\n                     \t\tWhere x is the numeric base font size to use.");
	printf("\n                     \t\texample: %s %s=5",argv0,GAME_ARGS[GAME_ARG_FONT_BASESIZE]);

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

#define MAIN_FUNCTION(X) int main(int argc, char **argv)                                       \
{																			                   \
	if(hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_HELP])) == true    ||           \
	   hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_VERSION])) == true ||           \
	   hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_SHOW_INI_SETTINGS])) == true || \
	   hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_MASTERSERVER_MODE])) == true) { \
	     if(SDL_Init(SDL_INIT_TIMER | SDL_INIT_JOYSTICK) < 0)  {                               \
	        std::cerr << "Couldn't initialize SDL: " << SDL_GetError() << "\n";                \
	        return 1;                                                                          \
	     }                                                                                     \
	}																		                   \
	else {																	                   \
		 if(SDL_Init(SDL_INIT_EVERYTHING) < 0)  {                                              \
			std::cerr << "Couldn't initialize SDL: " << SDL_GetError() << "\n";                \
			return 1;                                                                          \
		 }                                                                                     \
	}																		                   \
	SDL_EnableUNICODE(1);													                   \
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);                \
    int result = X(argc, argv);                                                                \
    return result;                                                                             \
}

#endif
