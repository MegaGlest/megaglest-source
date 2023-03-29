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

#include "leak_dumper.h"
#include <GL/glew.h>
#include <SDL.h>
#include <iostream>

#ifndef WIN32
#define stricmp strcasecmp
#define strnicmp strncasecmp
#define _strnicmp strncasecmp
#endif

const char *GAME_ARGS[] = {"--help",

                           "--autostart-lastgame",
                           "--load-saved-game",
                           "--auto-test",
                           "--connect",
                           "--connecthost",
                           "--starthost",
                           "--headless-server-mode",
                           "--headless-server-status",
                           "--server-title",
                           "--use-ports",

                           "--load-scenario",
                           "--load-mod",
                           "--preview-map",
                           "--version",
                           "--opengl-info",
                           "--sdl-info",
                           "--lua-info",
                           "--lua-debug",
                           "--curl-info",
                           "--xerces-info",

                           "--validate-techtrees",
                           "--validate-factions",
                           "--validate-scenario",
                           "--validate-tileset",

                           "--translate-techtrees",

                           "--list-maps",
                           "--list-techtrees",
                           "--list-scenarios",
                           "--list-tilesets",
                           "--list-tutorials",

                           "--data-path",
                           "--ini-path",
                           "--log-path",
                           "--font-path",
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
                           "--disable-vertex-interpolation",
                           "--disable-sound",
                           "--enable-legacyfonts",

                           "--resolution",
                           "--colorbits",
                           "--depthbits",
                           "--fullscreen",
                           "--set-gamma",

                           "--use-font",
                           "--font-basesize",

                           "--disable-videos",

                           "--disable-opengl-checks",
                           "--disable-streflop-checks",
                           "--debug-network-packets",
                           "--debug-network-packet-sizes",
                           "--debug-network-packet-stats",
                           "--enable-new-protocol",

                           "--create-data-archives",
                           "--steam",
                           "--steam-debug",
                           "--steam-reset-stats",

                           "--verbose"

};

enum GAME_ARG_TYPE {
  GAME_ARG_HELP = 0,

  GAME_ARG_AUTOSTART_LASTGAME,
  GAME_ARG_AUTOSTART_LAST_SAVED_GAME,
  GAME_ARG_AUTO_TEST,
  GAME_ARG_CONNECT,
  GAME_ARG_CLIENT,
  GAME_ARG_SERVER,
  GAME_ARG_MASTERSERVER_MODE,
  GAME_ARG_MASTERSERVER_STATUS,
  GAME_ARG_SERVER_TITLE,
  GAME_ARG_USE_PORTS,

  GAME_ARG_LOADSCENARIO,
  GAME_ARG_MOD,
  GAME_ARG_PREVIEW_MAP,
  GAME_ARG_VERSION,
  GAME_ARG_OPENGL_INFO,
  GAME_ARG_SDL_INFO,
  GAME_ARG_LUA_INFO,
  GAME_ARG_LUA_DEBUG,
  GAME_ARG_CURL_INFO,
  GAME_ARG_XERCES_INFO,

  GAME_ARG_VALIDATE_TECHTREES,
  GAME_ARG_VALIDATE_FACTIONS,
  GAME_ARG_VALIDATE_SCENARIO,
  GAME_ARG_VALIDATE_TILESET,

  GAME_ARG_TRANSLATE_TECHTREES,

  GAME_ARG_LIST_MAPS,
  GAME_ARG_LIST_TECHTRESS,
  GAME_ARG_LIST_SCENARIOS,
  GAME_ARG_LIST_TILESETS,
  GAME_ARG_LIST_TUTORIALS,

  GAME_ARG_DATA_PATH,
  GAME_ARG_INI_PATH,
  GAME_ARG_LOG_PATH,
  GAME_ARG_FONT_PATH,
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
  GAME_ARG_DISABLE_VERTEX_INTERPOLATION,
  GAME_ARG_DISABLE_SOUND,
  GAME_ARG_ENABLE_LEGACYFONTS,

  GAME_ARG_USE_RESOLUTION,
  GAME_ARG_USE_COLORBITS,
  GAME_ARG_USE_DEPTHBITS,
  GAME_ARG_USE_FULLSCREEN,
  GAME_ARG_SET_GAMMA,

  GAME_ARG_USE_FONT,
  GAME_ARG_FONT_BASESIZE,

  GAME_ARG_DISABLE_VIDEOS,

  GAME_ARG_DISABLE_OPENGL_CAPS_CHECK,
  GAME_ARG_DISABLE_STREFLOP_CAPS_CHECK,

  GAME_ARG_DEBUG_NETWORK_PACKETS,
  GAME_ARG_DEBUG_NETWORK_PACKET_SIZES,
  GAME_ARG_DEBUG_NETWORK_PACKET_STATS,
  GAME_ARG_ENABLE_NEW_PROTOCOL,

  GAME_ARG_CREATE_DATA_ARCHIVES,
  GAME_ARG_STEAM,
  GAME_ARG_STEAM_DEBUG,
  GAME_ARG_STEAM_RESET_STATS,

  GAME_ARG_VERBOSE_MODE,

  GAME_ARG_END
};

void printParameterHelp(const char *argv0, bool foundInvalidArgs) {
  //                                     MAX WIDTH FOR MAN PAGE
  //         "================================================================================"
  if (foundInvalidArgs == true) {
    printf("\n");
  }
  printf("\n%s, usage", extractFileFromDirectoryPath(argv0).c_str());
  printf("\n\nCommandline Parameter:  Description:");
  printf("\n\n- - - - - - - - - - -   - - - - - - - - - - - - - - - - - - - - "
         "- - - -");
  printf("\n\n%s  \t\tDisplays this help text.", GAME_ARGS[GAME_ARG_HELP]);

  printf("\n\n%s  \tAutomatically starts a game with the last game",
         GAME_ARGS[GAME_ARG_AUTOSTART_LASTGAME]);
  printf("\n\n                     \tsettings you played.");

  printf("\n\n%s=x  \tLoads the last saved game.",
         GAME_ARGS[GAME_ARG_AUTOSTART_LAST_SAVED_GAME]);
  printf("\n\n                     \tWhere x is an optional name of the saved "
         "game file to load.");
  printf("\n\n                     \tIf x is not specified we load the last "
         "game that was saved.");

  printf("\n\n%s=x,y,z  \tRun in auto test mode.",
         GAME_ARGS[GAME_ARG_AUTO_TEST]);
  printf("\n\n                     \tWhere x is an optional maximum # seconds "
         "to play.");
  printf("\n\n                     \tIf x is not specified the default is 1200 "
         "seconds (20 minutes).");
  printf("\n\n                     \tWhere y is an optional game settings file "
         "to play.");
  printf("\n\n                     \tIf y is not specified (or is empty) then "
         "auto test cycles");
  printf("\n\n                     \tthrough playing scenarios.");
  printf("\n\n                     \tWhere z is the word 'exit' indicating the "
         "game should exit");
  printf("\n\n                     \tafter the game is finished or the time "
         "runs out. If z is");
  printf("\n\n                     \tnot specified (or is empty) then auto "
         "test continues to cycle.");

  printf(
      "\n\n%s=x:y  \t\tAuto connect to host server at IP or hostname x using",
      GAME_ARGS[GAME_ARG_CONNECT]);
  printf("\n\n                     \t    port y. Shortcut version of using %s "
         "and %s.",
         GAME_ARGS[GAME_ARG_CLIENT], GAME_ARGS[GAME_ARG_USE_PORTS]);
  printf("\n\n                     \t*NOTE: to automatically connect to the "
         "first LAN host you may");
  printf("\n\n                     \t    use: %s=auto-connect",
         GAME_ARGS[GAME_ARG_CONNECT]);

  printf("\n\n%s=x  \tAuto connect to host server at IP or hostname x.",
         GAME_ARGS[GAME_ARG_CLIENT]);
  printf("\n\n                     \t*NOTE: to automatically connect to the "
         "first LAN host you may");
  printf("\n\n                     \t    use: %s=auto-connect",
         GAME_ARGS[GAME_ARG_CLIENT]);

  printf("\n\n%s  \t\tAuto create a host server.", GAME_ARGS[GAME_ARG_SERVER]);

  printf("\n\n%s=x,x  ", GAME_ARGS[GAME_ARG_MASTERSERVER_MODE]);
  printf("\n\n                     \tRun as a headless server.");
  printf("\n\n                     \tWhere x is an optional comma delimited "
         "command list of one or");
  printf("\n\n                     \t    more of the following: ");
  printf("\n\n                     \t'exit' - which quits the application "
         "after a game has no more");
  printf("\n\n                     \t    connected players.");
  printf("\n\n                     \t'vps' - which does NOT read commands from "
         "the local console");
  printf("\n\n                     \t    (for some vps's).");
  printf("\n\n                     \t'lan' - which does not broadcast the "
         "hosting server to the");
  printf("\n\n                     \t    masterserver (for local LAN games).");

  printf("\n\n%s  ", GAME_ARGS[GAME_ARG_MASTERSERVER_STATUS]);
  printf("\n\n                     \tCheck the current status of a headless "
         "server.");

  printf("\n\n%s=x,y,z  \tForce hosted games to listen internally on port",
         GAME_ARGS[GAME_ARG_USE_PORTS]);
  printf("\n\n                     \t    x, externally on port y and for game "
         "status on port z.");
  printf("\n\n                     \tWhere x is the internal port # on the "
         "local machine to");
  printf("\n\n                     \t    listen for connects.");
  printf("\n\n                     \tWhere y is the external port # on the "
         "router/proxy to");
  printf("\n\n                     \t    forward connection from to the "
         "internal port #.");
  printf("\n\n                     \tWhere z is the game status port # on the "
         "local machine");
  printf("\n\n                     \t    to listen for status requests.");
  printf("\n\n                     \t*NOTE: If enabled the FTP Server port #'s "
         "will be set");
  printf("\n\n                     \t    to x+1 to x+9.");

  printf("\n\n%s=x  \tSet server title.", GAME_ARGS[GAME_ARG_SERVER_TITLE]);

  printf("\n\n%s=x  \tAuto load a scenario by scenario name.",
         GAME_ARGS[GAME_ARG_LOADSCENARIO]);
  printf("\n\n%s=x  \t\tAuto load a mod by mod pathname.",
         GAME_ARGS[GAME_ARG_MOD]);

  //         "================================================================================"
  printf("\n\n%s=Map,Tileset  ", GAME_ARGS[GAME_ARG_PREVIEW_MAP]);
  printf("\n\n                     \tAuto Preview a map by map name. (tileset "
         "is optional)");
  printf("\n\n%s  \t\tDisplays the version string of this program.",
         GAME_ARGS[GAME_ARG_VERSION]);
  printf("\n\n%s  \t\tDisplays your video driver's OpenGL info.",
         GAME_ARGS[GAME_ARG_OPENGL_INFO]);
  printf("\n\n%s  \t\tDisplays your SDL version information.",
         GAME_ARGS[GAME_ARG_SDL_INFO]);
  printf("\n\n%s  \t\tDisplays your LUA version information.",
         GAME_ARGS[GAME_ARG_LUA_INFO]);
  printf("\n\n%s  \t\tDisplays LUA debug information.",
         GAME_ARGS[GAME_ARG_LUA_DEBUG]);
  printf("\n\n%s  \t\tDisplays your CURL version information.",
         GAME_ARGS[GAME_ARG_CURL_INFO]);
  printf("\n\n%s  \t\tDisplays your XERCES version information.",
         GAME_ARGS[GAME_ARG_XERCES_INFO]);

  printf("\n\n%s=x=purgeunused=purgeduplicates=gitdelete=hideduplicates  ",
         GAME_ARGS[GAME_ARG_VALIDATE_TECHTREES]);
  printf("\n\n                     \tDisplay a report detailing any known "
         "problems related to");
  printf("\n\n                     \t    your selected techtrees game data.");
  printf("\n\n                     \tWhere x is a comma-delimited list of "
         "techtrees to validate.");
  printf("\n\n                     \tWhere 'purgeunused' is an optional "
         "parameter telling the");
  printf("\n\n                     \t    validation to delete extra files in "
         "the techtree that");
  printf("\n\n                     \t    are not used.");
  printf("\n\n                     \tWhere 'purgeduplicates' is an optional "
         "parameter telling");
  printf("\n\n                     \t    the validation to merge duplicate "
         "files in the techtree.");
  printf("\n\n                     \tWhere 'gitdelete' is an optional "
         "parameter telling the");
  printf("\n\n                     \t    validation to call 'git rm' on "
         "duplicate / unused files");
  printf("\n\n                     \t    in the techtree.");
  printf("\n\n                     \tWhere 'hideduplicates' is an optional "
         "parameter telling the");
  printf("\n\n                     \t    validation to NOT SHOW duplicate "
         "files in the techtree.");
  printf("\n\n                     \t*NOTE: This only applies when files are "
         "purged due to the");
  printf("\n\n                     \t    above flags being set.");
  printf("\n\n                     \texample:");
  printf("\n\n                     \t%s %s=megapack,vbros_pack_5",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_VALIDATE_TECHTREES]);

  printf("\n\n%s=x=purgeunused=purgeduplicates=hideduplicates  ",
         GAME_ARGS[GAME_ARG_VALIDATE_FACTIONS]);
  printf("\n\n                     \tDisplay a report detailing any known "
         "problems related to");
  printf("\n\n                     \t    your selected factions game data.");
  printf("\n\n                     \tWhere x is a comma-delimited list of "
         "factions to validate.");
  printf("\n\n                     \tWhere 'purgeunused' is an optional "
         "parameter telling the");
  printf("\n\n                     \t    validation to delete extra files in "
         "the faction that are");
  printf("\n\n                     \t    not used.");
  printf("\n\n                     \tWhere 'purgeduplicates' is an optional "
         "parameter telling the");
  printf("\n\n                     \t    validation to merge duplicate files "
         "in the faction.");
  printf("\n\n                     \tWhere 'hideduplicates' is an optional "
         "parameter telling the");
  printf("\n\n                     \t    validation to NOT SHOW duplicate "
         "files in the techtree.");
  printf("\n\n                     \t*NOTE: leaving the list empty is the same "
         "as running:");
  printf("\n\n                     \t    %s",
         GAME_ARGS[GAME_ARG_VALIDATE_TECHTREES]);
  printf("\n\n                     \texample: %s %s=tech,egypt",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_VALIDATE_FACTIONS]);

  printf("\n\n%s=x=purgeunused=gitdelete  ",
         GAME_ARGS[GAME_ARG_VALIDATE_SCENARIO]);
  printf("\n\n                     \tDisplay a report detailing any known "
         "problems related to");
  printf("\n\n                     \t    your selected scenario game data.");
  printf(
      "\n\n                     \tWhere x is a single scenario to validate.");
  printf("\n\n                     \tWhere 'purgeunused' is an optional "
         "parameter telling the");
  printf("\n\n                     \t    validation to delete extra files in "
         "the scenario that");
  printf("\n\n                     \t    are not used.");
  printf("\n\n                     \texample: %s %s=stranded",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_VALIDATE_SCENARIO]);

  printf("\n\n%s=x=purgeunused=gitdelete  ",
         GAME_ARGS[GAME_ARG_VALIDATE_TILESET]);
  printf("\n\n                     \tDisplay a report detailing any known "
         "problems related to");
  printf("\n\n                     \t    your selected tileset game data.");
  printf("\n\n                     \tWhere x is a single tileset to validate.");
  printf("\n\n                     \tWhere 'purgeunused' is an optional "
         "parameter telling the");
  printf("\n\n                     \t    validation to delete extra files in "
         "the tileset that");
  printf("\n\n                     \t    are not used.");
  printf("\n\n                     \texample: %s %s=desert2",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_VALIDATE_TILESET]);

  printf("\n\n%s=x  ", GAME_ARGS[GAME_ARG_TRANSLATE_TECHTREES]);
  printf("\n\n                     \tProduces a default lng file for the "
         "specified techtree to");
  printf("\n\n                     \t    prepare for translation into other "
         "languages.");
  printf("\n\n                     \tWhere x is a techtree name.");

  printf("\n\n%s=x  \t\tDisplay a list of game content: maps.",
         GAME_ARGS[GAME_ARG_LIST_MAPS]);
  printf("\n\n                     \tWhere x is an optional name filter.");
  printf("\n\n                     \texample: %s %s=island*",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_LIST_MAPS]);

  printf("\n\n%s=showfactions  ", GAME_ARGS[GAME_ARG_LIST_TECHTRESS]);
  printf(
      "\n\n                     \tDisplay a list of game content: techtrees.");
  printf("\n\n                     \tWhere 'showfactions' is an optional "
         "parameter to display");
  printf("\n\n                     \t    factions in each techtree.");
  printf("\n\n                     \texample: %s %s=showfactions",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_LIST_TECHTRESS]);

  printf("\n\n%s=x  \tDisplay a list of game content: scenarios.",
         GAME_ARGS[GAME_ARG_LIST_SCENARIOS]);
  printf("\n\n                     \tWhere x is an optional name filter.");
  printf("\n\n                     \texample: %s %s=beginner*",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_LIST_SCENARIOS]);

  printf("\n\n%s=x  \tDisplay a list of game content: tilesets.",
         GAME_ARGS[GAME_ARG_LIST_TILESETS]);
  printf("\n\n                     \tWhere x is an optional name filter.");
  printf("\n\n                     \texample: %s %s=f*",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_LIST_TILESETS]);

  printf("\n\n%s=x  \tDisplay a list of game content: tutorials.",
         GAME_ARGS[GAME_ARG_LIST_TUTORIALS]);
  printf("\n\n                     \tWhere x is an optional name filter.");
  printf("\n\n                     \texample: %s %s=*",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_LIST_TUTORIALS]);

  //         "================================================================================"
  printf("\n\n%s=x  \t\tSets the game data path to x.",
         GAME_ARGS[GAME_ARG_DATA_PATH]);
  printf("\n\n                     \texample:");
  printf("\n\n                     \t%s %s=/usr/local/game_data/",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_DATA_PATH]);
  printf("\n\n%s=x  \t\tSets the game ini path to x.",
         GAME_ARGS[GAME_ARG_INI_PATH]);
  printf("\n\n                     \texample: %s %s=~/game_config/",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_INI_PATH]);
  printf("\n\n%s=x  \t\tSets the game logs path to x.",
         GAME_ARGS[GAME_ARG_LOG_PATH]);
  printf("\n\n                     \texample: %s %s=~/game_logs/",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_LOG_PATH]);
  printf("\n\n%s=x  \t\tSets the game fonts path to x.",
         GAME_ARGS[GAME_ARG_FONT_PATH]);
  printf("\n\n                     \texample: %s %s=~/myfonts/",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_FONT_PATH]);

  printf("\n\n%s=x  \tDisplay merged ini settings information.",
         GAME_ARGS[GAME_ARG_SHOW_INI_SETTINGS]);
  printf("\n\n                     \tWhere x is an optional property name to "
         "filter (default");
  printf("\n\n                     \t    shows all).");
  printf("\n\n                     \texample:");
  printf("\n\n                     \t%s %s=DebugMode",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_SHOW_INI_SETTINGS]);

  printf("\n\n%s=x=textureformat=keepsmallest  ",
         GAME_ARGS[GAME_ARG_CONVERT_MODELS]);
  printf("\n\n                     \tConvert a model file or folder to the "
         "current g3d version");
  printf("\n\n                     \t    format.");
  printf("\n\n                     \tWhere x is a filename or folder "
         "containing the g3d model(s).");
  printf("\n\n                     \tWhere 'textureformat' is an optional "
         "supported texture");
  printf(
      "\n\n                     \t    format to convert to (tga,bmp,jpg,png).");
  printf("\n\n                     \tWhere 'keepsmallest' is an optional flag "
         "indicating to keep");
  printf("\n\n                     \t    the original texture if its filesize "
         "is smaller than the");
  printf("\n\n                     \t    converted format.");
  printf("\n\n                     \texample:");
  printf("\n\n                     \t%s %s=techs/megapack/factions/tech/",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_CONVERT_MODELS]);
  printf("\n\n                     "
         "\tunits/castle/models/castle.g3d=png=keepsmallest");

  printf("\n\n%s=x  \tForce the language to be the language specified",
         GAME_ARGS[GAME_ARG_USE_LANGUAGE]);
  printf("\n\n                     \t    by x. Where x is a language filename "
         "or ISO639-1 code.");
  printf("\n\n                     \texample: %s %s=english",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_USE_LANGUAGE]);
  printf("\n\n                     \texample: %s %s=en",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_USE_LANGUAGE]);

  printf("\n\n%s=x  \tShow the calculated CRC for the map named x.",
         GAME_ARGS[GAME_ARG_SHOW_MAP_CRC]);
  printf("\n\n                     \tWhere x is a map name.");
  printf("\n\n                     \texample: %s %s=four_rivers",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_SHOW_MAP_CRC]);

  printf("\n\n%s=x  \tShow the calculated CRC for the tileset named x.",
         GAME_ARGS[GAME_ARG_SHOW_TILESET_CRC]);
  printf("\n\n                     \tWhere x is a tileset name.");
  printf("\n\n                     \texample: %s %s=forest",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_SHOW_TILESET_CRC]);

  printf("\n\n%s=x  \tShow the calculated CRC for the techtree named x.",
         GAME_ARGS[GAME_ARG_SHOW_TECHTREE_CRC]);
  printf("\n\n                     \tWhere x is a techtree name.");
  printf("\n\n                     \texample: %s %s=megapack",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_SHOW_TECHTREE_CRC]);

  printf("\n\n%s=x  \tShow the calculated CRC for the scenario named x.",
         GAME_ARGS[GAME_ARG_SHOW_SCENARIO_CRC]);
  printf("\n\n                     \tWhere x is a scenario name.");
  printf("\n\n                     \texample: %s %s=storming",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_SHOW_SCENARIO_CRC]);

  //         "================================================================================"
  printf("\n\n%s=x=y  \tShow the calculated CRC for files in the path",
         GAME_ARGS[GAME_ARG_SHOW_PATH_CRC]);
  printf("\n\n                     \t    located in x using file filter y.");
  printf("\n\n                     \tWhere x is a path name and y is file(s) "
         "filter.");
  printf("\n\n                     \texample: %s %s=techs/=megapack.7z",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_SHOW_PATH_CRC]);

  printf("\n\n%s  \tDisables stack backtrace on errors.",
         GAME_ARGS[GAME_ARG_DISABLE_BACKTRACE]);

  printf("\n\n%s  ", GAME_ARGS[GAME_ARG_DISABLE_SIGSEGV_HANDLER]);
  printf("\n\n                     \tDisables the sigsegv error handler.");

  printf("\n\n%s  \t\tDisables trying to use Vertex Buffer Objects.",
         GAME_ARGS[GAME_ARG_DISABLE_VBO]);
  printf("\n\n%s  ", GAME_ARGS[GAME_ARG_DISABLE_VERTEX_INTERPOLATION]);
  printf("\n\n                     \tDisables interpolating animations to make "
         "them smoother.");
  printf("\n\n%s  \tDisables the sound system.",
         GAME_ARGS[GAME_ARG_DISABLE_SOUND]);

  printf("\n\n%s  \tEnables using the legacy font system.",
         GAME_ARGS[GAME_ARG_ENABLE_LEGACYFONTS]);

  //         "================================================================================"
  printf("\n\n%s=x  \tOverride the video resolution.",
         GAME_ARGS[GAME_ARG_USE_RESOLUTION]);
  printf("\n\n                     \tWhere x is a string with the following "
         "format: 'width x height'.");
  printf("\n\n                     \texample: %s %s=1024x768",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_USE_RESOLUTION]);

  printf("\n\n%s=x  \t\tOverride the video colorbits.",
         GAME_ARGS[GAME_ARG_USE_COLORBITS]);
  printf("\n\n                     \tWhere x is a valid colorbits value "
         "supported by your video");
  printf("\n\n                     \t    driver.");
  printf("\n\n                     \texample: %s %s=32",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_USE_COLORBITS]);

  printf("\n\n%s=x  \t\tOverride the video depthbits.",
         GAME_ARGS[GAME_ARG_USE_DEPTHBITS]);
  printf("\n\n                     \tWhere x is a valid depthbits value "
         "supported by your video");
  printf("\n\n                     \t    driver.");
  printf("\n\n                     \texample: %s %s=24",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_USE_DEPTHBITS]);

  printf("\n\n%s=x  \tOverride the video fullscreen mode.",
         GAME_ARGS[GAME_ARG_USE_FULLSCREEN]);
  printf("\n\n                     \tWhere x either true or false.");
  printf("\n\n                     \texample: %s %s=true",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_USE_FULLSCREEN]);

  printf("\n\n%s=x  \t\tOverride the video gamma (contrast) value.",
         GAME_ARGS[GAME_ARG_SET_GAMMA]);
  printf("\n\n                     \tWhere x is a floating point value.");
  printf("\n\n                     \texample: %s %s=1.5",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_SET_GAMMA]);

  printf("\n\n%s=x  \t\tOverride the font to use.",
         GAME_ARGS[GAME_ARG_USE_FONT]);
  printf("\n\n                     \tWhere x is the path and name of a font "
         "file supported by");
  printf("\n\n                     \t    freetype2.");
  printf("\n\n                     \texample:");
  printf("\n\n                     \t%s "
         "%s=$APPLICATIONDATAPATH/data/core/fonts/Vera.ttf",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_USE_FONT]);

  printf("\n\n%s=x  \tOverride the font base size.",
         GAME_ARGS[GAME_ARG_FONT_BASESIZE]);
  printf("\n\n                     \tWhere x is the numeric base font size to "
         "use.");
  printf("\n\n                     \texample: %s %s=5",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_FONT_BASESIZE]);

  printf("\n\n%s  \tDisables video playback.",
         GAME_ARGS[GAME_ARG_DISABLE_VIDEOS]);

  printf("\n\n%s  ", GAME_ARGS[GAME_ARG_DISABLE_OPENGL_CAPS_CHECK]);
  printf("\n\n                     \tDisables opengl capability checks (for "
         "corrupt or flaky video");
  printf("\n\n                     \t    drivers).");

  printf("\n\n%s=x=y  ", GAME_ARGS[GAME_ARG_CREATE_DATA_ARCHIVES]);
  printf("\n\n                     \tCompress selected game data into archives "
         "for network sharing.");
  printf("\n\n                     \tWhere x is one of the following data "
         "items to compress:");
  printf("\n\n                     \t    techtrees, tilesets or all.");
  printf("\n\n                     \tWhere y = include_main to include main "
         "(non mod) data.");
  printf("\n\n                     \texample: %s %s=all",
         extractFileFromDirectoryPath(argv0).c_str(),
         GAME_ARGS[GAME_ARG_CREATE_DATA_ARCHIVES]);

  printf("\n\n%s=x=y  ", GAME_ARGS[GAME_ARG_STEAM]);
  printf("\n\n                     \tRun with Steam Client Integration.");

  printf("\n\n%s  \t\tDisplays verbose information in the console.",
         GAME_ARGS[GAME_ARG_VERBOSE_MODE]);
  printf("\n\n");
}

bool hasCommandArgument(int argc, char **argv, const string argName,
                        int *foundIndex = NULL, int startLookupIndex = 1,
                        bool useArgParamLen = false) {
  bool result = false;

  if (foundIndex != NULL) {
    *foundIndex = -1;
  }
  int compareLen = (int)strlen(argName.c_str());

  for (int idx = startLookupIndex; idx < argc; idx++) {
    if (useArgParamLen == true) {
      compareLen = (int)strlen(argv[idx]);
    }
    if (_strnicmp(argName.c_str(), argv[idx], compareLen) == 0) {
      result = true;
      if (foundIndex != NULL) {
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
  if (knownArgCount != GAME_ARG_END) {
    char szBuf[8096] = "";
    snprintf(
        szBuf, 8096,
        "Internal arg count mismatch knownArgCount = %d, GAME_ARG_END = %d",
        knownArgCount, GAME_ARG_END);
    throw megaglest_runtime_error(szBuf);
  }

  SystemFlags::VERBOSE_MODE_ENABLED = false;
  if (hasCommandArgument(argc, argv, GAME_ARGS[GAME_ARG_VERBOSE_MODE]) ==
      true) {
    SystemFlags::VERBOSE_MODE_ENABLED = true;
  }

  if (SystemFlags::VERBOSE_MODE_ENABLED)
    printf("In [%s::%s Line: %d]\n", __FILE__, __FUNCTION__, __LINE__);

  if (hasCommandArgument(
          argc, argv,
          string(GAME_ARGS[GAME_ARG_DISABLE_STREFLOP_CAPS_CHECK])) == false) {
// Ensure at runtime that the client has SSE
#if defined(STREFLOP_SSE)
#if defined(WIN32)

    if (SystemFlags::VERBOSE_MODE_ENABLED)
      printf("In [%s::%s Line: %d] About to validate SSE support\n", __FILE__,
             __FUNCTION__, __LINE__);

    int has_x64 = false;
    int has_MMX = false;
    int has_SSE = false;
    int has_SSE2 = false;
    int has_SSE3 = false;
    int has_SSSE3 = false;
    int has_SSE41 = false;
    int has_SSE42 = false;
    int has_SSE4a = false;
    int has_AVX = false;
    int has_XOP = false;
    int has_FMA3 = false;
    int has_FMA4 = false;

    int info[4];
    __cpuid(info, 0);
    int nIds = info[0];

    __cpuid(info, 0x80000000);
    int nExIds = info[0];

    //  Detect Instruction Set
    if (nIds >= 1) {
      __cpuid(info, 0x00000001);
      has_MMX = (info[3] & ((int)1 << 23)) != 0;
      has_SSE = (info[3] & ((int)1 << 25)) != 0;
      has_SSE2 = (info[3] & ((int)1 << 26)) != 0;
      has_SSE3 = (info[2] & ((int)1 << 0)) != 0;

      has_SSSE3 = (info[2] & ((int)1 << 9)) != 0;
      has_SSE41 = (info[2] & ((int)1 << 19)) != 0;
      has_SSE42 = (info[2] & ((int)1 << 20)) != 0;

      has_AVX = (info[2] & ((int)1 << 28)) != 0;
      has_FMA3 = (info[2] & ((int)1 << 12)) != 0;
    }

    if (nExIds >= 0x80000001) {
      __cpuid(info, 0x80000001);
      has_x64 = (info[3] & ((int)1 << 29)) != 0;
      has_SSE4a = (info[2] & ((int)1 << 6)) != 0;
      has_FMA4 = (info[2] & ((int)1 << 16)) != 0;
      has_XOP = (info[2] & ((int)1 << 11)) != 0;
    }

    if (SystemFlags::VERBOSE_MODE_ENABLED)
      printf("In [%s::%s Line: %d] sse check got "
             "[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]\n",
             __FILE__, __FUNCTION__, __LINE__, has_x64, has_MMX, has_SSE,
             has_SSE2, has_SSE3, has_SSSE3, has_SSE41, has_SSE42, has_SSE4a,
             has_AVX, has_XOP, has_FMA3, has_FMA4);

    if (has_SSE == false && has_SSE2 == false && has_SSE3 == false &&
        has_SSSE3 == false && has_SSE41 == false && has_SSE42 == false &&
        has_SSE4a == false) {
      char szBuf[8096] = "";
      snprintf(szBuf, 8096,
               "Error detected, your CPU does not seem to support SSE: [%d]\n",
               has_SSE);
      throw megaglest_runtime_error(szBuf);
    }
#elif defined(__GNUC__) && !defined(__APPLE__)

    if (SystemFlags::VERBOSE_MODE_ENABLED)
      printf("In [%s::%s Line: %d] About to validate SSE support\n", __FILE__,
             __FUNCTION__, __LINE__);

#define CHECK_BIT(var, pos) ((var) & (1 << (pos)))

#define cpuid(func, ax, bx, cx, dx)                                            \
  __asm__ __volatile__("cpuid"                                                 \
                       : "=a"(ax), "=b"(bx), "=c"(cx), "=d"(dx)                \
                       : "a"(func));

    int ax = 0, bx = 0, cx = 0, dx = 0;
    cpuid(0x0000001, ax, bx, cx, dx)

        if (SystemFlags::VERBOSE_MODE_ENABLED)
            printf("In [%s::%s Line: %d] sse check got [%d,%d,%d,%d]\n",
                   __FILE__, __FUNCTION__, __LINE__, ax, bx, cx, dx);

    // Check SSE, SSE2 and SSE3 support (if all 3 fail throw exception)
    if (!CHECK_BIT(dx, 25) && !CHECK_BIT(dx, 26) && !CHECK_BIT(cx, 0)) {
      char szBuf[8096] = "";
      snprintf(szBuf, 8096,
               "Error detected, your CPU does not seem to support SSE: [%d]\n",
               CHECK_BIT(dx, 25));
      throw megaglest_runtime_error(szBuf);
    }

#endif
#endif
  }

  if (hasCommandArgument(argc, argv, GAME_ARGS[GAME_ARG_OPENGL_INFO]) == true ||
      hasCommandArgument(argc, argv, GAME_ARGS[GAME_ARG_SDL_INFO]) == true ||
      hasCommandArgument(argc, argv, GAME_ARGS[GAME_ARG_LUA_INFO]) == true ||
      hasCommandArgument(argc, argv, GAME_ARGS[GAME_ARG_CURL_INFO]) == true ||
      hasCommandArgument(argc, argv, GAME_ARGS[GAME_ARG_XERCES_INFO]) == true ||
      hasCommandArgument(argc, argv, GAME_ARGS[GAME_ARG_VERSION]) == true ||
      hasCommandArgument(argc, argv, GAME_ARGS[GAME_ARG_SHOW_INI_SETTINGS]) ==
          true ||
      hasCommandArgument(argc, argv, GAME_ARGS[GAME_ARG_VALIDATE_TECHTREES]) ==
          true ||
      hasCommandArgument(argc, argv, GAME_ARGS[GAME_ARG_VALIDATE_FACTIONS]) ==
          true ||
      hasCommandArgument(argc, argv, GAME_ARGS[GAME_ARG_VALIDATE_SCENARIO]) ==
          true ||
      hasCommandArgument(argc, argv, GAME_ARGS[GAME_ARG_VALIDATE_TILESET]) ==
          true ||
      hasCommandArgument(argc, argv, GAME_ARGS[GAME_ARG_LIST_MAPS]) == true ||
      hasCommandArgument(argc, argv, GAME_ARGS[GAME_ARG_LIST_TECHTRESS]) ==
          true ||
      hasCommandArgument(argc, argv, GAME_ARGS[GAME_ARG_LIST_SCENARIOS]) ==
          true ||
      hasCommandArgument(argc, argv, GAME_ARGS[GAME_ARG_LIST_TILESETS]) ==
          true ||
      hasCommandArgument(argc, argv, GAME_ARGS[GAME_ARG_LIST_TUTORIALS]) ==
          true) {
    haveSpecialOutputCommandLineOption = true;
  }

  if (haveSpecialOutputCommandLineOption == false) {
#ifdef USE_STREFLOP
#define STREFLOP_NO_DENORMALS
    if (SystemFlags::VERBOSE_MODE_ENABLED)
      printf("In [%s::%s Line: %d]\n", __FILE__, __FUNCTION__, __LINE__);

    streflop_init<streflop::Simple>();

    if (SystemFlags::VERBOSE_MODE_ENABLED)
      printf("In [%s::%s Line: %d]\n", __FILE__, __FUNCTION__, __LINE__);
#endif
  }

  if (hasCommandArgument(argc, argv, string(GAME_ARGS[GAME_ARG_HELP])) ==
          true ||
      hasCommandArgument(argc, argv, string(GAME_ARGS[GAME_ARG_VERSION])) ==
          true ||
      hasCommandArgument(
          argc, argv, string(GAME_ARGS[GAME_ARG_SHOW_INI_SETTINGS])) == true ||
      hasCommandArgument(
          argc, argv, string(GAME_ARGS[GAME_ARG_MASTERSERVER_MODE])) == true ||
      hasCommandArgument(argc, argv,
                         string(GAME_ARGS[GAME_ARG_MASTERSERVER_STATUS]))) {
    // Use this for masterserver mode for timers like Chrono
    if (SystemFlags::VERBOSE_MODE_ENABLED)
      printf("In [%s::%s Line: %d]\n", __FILE__, __FUNCTION__, __LINE__);

    if (SDL_Init(SDL_INIT_TIMER) < 0) {
      if (SystemFlags::VERBOSE_MODE_ENABLED)
        printf("In [%s::%s Line: %d]\n", __FILE__, __FUNCTION__, __LINE__);
      std::cerr << "Couldn't initialize SDL: " << SDL_GetError() << "\n";
      return 3;
    }
  } else {
    if (SystemFlags::VERBOSE_MODE_ENABLED)
      printf("In [%s::%s Line: %d]\n", __FILE__, __FUNCTION__, __LINE__);
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO |
                 SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER |
                 SDL_INIT_EVENTS) < 0) {
      if (SystemFlags::VERBOSE_MODE_ENABLED)
        printf("In [%s::%s Line: %d]\n", __FILE__, __FUNCTION__, __LINE__);
      std::cerr << "Couldn't initialize SDL: " << SDL_GetError() << "\n";
      return 3;
    }
    if (SystemFlags::VERBOSE_MODE_ENABLED)
      printf("In [%s::%s Line: %d]\n", __FILE__, __FUNCTION__, __LINE__);
    // SDL_EnableUNICODE(1);
    if (SystemFlags::VERBOSE_MODE_ENABLED)
      printf("In [%s::%s Line: %d]\n", __FILE__, __FUNCTION__, __LINE__);
    // SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,
    // SDL_DEFAULT_REPEAT_INTERVAL);
  }
  if (SystemFlags::VERBOSE_MODE_ENABLED)
    printf("In [%s::%s Line: %d]\n", __FILE__, __FUNCTION__, __LINE__);
  return 0;
}

#define MAIN_FUNCTION(X)                                                       \
  int main(int argc, char **argv) {                                            \
    int result = mainSetup(argc, argv);                                        \
    if (result == 0) {                                                         \
      result = X(argc, argv);                                                  \
    }                                                                          \
    return result;                                                             \
  }

#endif
