// ==============================================================
//	This file is part of MegaGlest Shared Library (www.glest.org)
//
//	Copyright (C) 2012 Mark Vejvoda (mark_vejvoda@hotmail.com)
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include <GL/glew.h>
#include "video_player.h"
#include <SDL.h>
#include <SDL_mutex.h>
#include <vector>

#if defined(WIN32)
#include <windows.h>
#endif

#ifdef HAS_LIBVLC

//MSVC++ 11.0 _MSC_VER = 1700 (Visual Studio 2011)
//MSVC++ 10.0 _MSC_VER = 1600 (Visual Studio 2010)
//MSVC++ 9.0  _MSC_VER = 1500 (Visual Studio 2008)
#if defined(_MSC_VER) && _MSC_VER < 1600

// Older versions of VC++ don't include stdint.h
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

#endif

#include <vlc/vlc.h>
#endif

#if defined(WIN32)
/**
* @param location The location of the registry key. For example "Software\\Bethesda Softworks\\Morrowind"
* @param name the name of the registry key, for example "Installed Path"
* @return the value of the key or an empty string if an error occured.
*/
std::string getRegKey(const std::string& location, const std::string& name){
    HKEY key;
    CHAR value[1024]; 
    DWORD bufLen = 1024*sizeof(CHAR);
    long ret;
    ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE, location.c_str(), 0, KEY_QUERY_VALUE, &key);
    if( ret != ERROR_SUCCESS ){
        return std::string();
    }
    ret = RegQueryValueExA(key, name.c_str(), 0, 0, (LPBYTE) value, &bufLen);
    RegCloseKey(key);
    if ( (ret != ERROR_SUCCESS) || (bufLen > 1024*sizeof(TCHAR)) ){
        return std::string();
    }
    string stringValue = value;
    size_t i = stringValue.length();
    while( i > 0 && stringValue[i-1] == '\0' ){
        --i;
    }
    return stringValue.substr(0,i); 
}
#endif

class ctx {
public:
	ctx() {
		empty = NULL;
	    textureId = 0; // Texture ID
	    surf = NULL;
	    mutex = NULL;
	    x= 0;
	    y = 0;
	    width = 0;
	    height = 0;
	    rawData = NULL;
	    isPlaying = 0;
	    needToQuit = false;
	    verboseEnabled = 0;

#ifdef HAS_LIBVLC
		libvlc = NULL;
		m = NULL;
		mp = NULL;
		vlc_argv.clear();
		vlc_argv_str.clear();
#endif
	}

	SDL_Surface *empty;
    GLuint textureId; // Texture ID
    SDL_Surface *surf;
    SDL_mutex *mutex;
    int x;
    int y;
    int width;
    int height;
    void *rawData;
    bool isPlaying;
    bool needToQuit;
    bool verboseEnabled;

#ifdef HAS_LIBVLC
	libvlc_instance_t *libvlc;
	libvlc_media_t *m;
	libvlc_media_player_t *mp;

	std::vector<const char *> vlc_argv;
	std::vector<string> vlc_argv_str;
#endif
};

namespace Shared{ namespace Graphics{

// Load a texture
static void loadTexture(struct ctx *ctx) {
   if(ctx->verboseEnabled) printf("In [%s] Line: %d\n",__FUNCTION__,__LINE__);

   void *rawData = ctx->rawData;
   Uint8 * pixelSource = 0;
   Uint8 * pixelDestination = (Uint8 *) rawData;
   Uint32 pix = 0;

   for (unsigned int i = ctx->height; i > 0; i--) {
      for (unsigned int j = 0; j < ctx->width; j++) {
         pixelSource = (Uint8 *) ctx->surf->pixels + (i-1) * ctx->surf->pitch + j * 2;
         pix = *(Uint16 *) pixelSource;
         SDL_GetRGBA(pix, ctx->surf->format, &(pixelDestination[0]), &(pixelDestination[1]), &(pixelDestination[2]), &(pixelDestination[3]));
         pixelDestination += 4;
      }
   }

   // Building the texture
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glBindTexture(GL_TEXTURE_2D, ctx->textureId);
   glTexImage2D(GL_TEXTURE_2D, 0, 4, ctx->width, ctx->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (Uint8 *) rawData);
}

static void *lock(void *data, void **p_pixels) {
    struct ctx *ctx = (struct ctx *)data;
    if(ctx->verboseEnabled) printf("In [%s] Line: %d\n",__FUNCTION__,__LINE__);

    SDL_LockMutex(ctx->mutex);
    SDL_LockSurface(ctx->surf);
    *p_pixels = ctx->surf->pixels;

    return NULL; /* picture identifier, not needed here */
}

static void unlock(void *data, void *id, void *const *p_pixels) {
    struct ctx *ctx = (struct ctx *)data;
    if(ctx->verboseEnabled) printf("In [%s] Line: %d\n",__FUNCTION__,__LINE__);

    /* VLC just rendered the video, but we can also render stuff */
    SDL_UnlockSurface(ctx->surf);
    SDL_UnlockMutex(ctx->mutex);
}

static void display(void *data, void *id) {
    /* VLC wants to display the video */
    struct ctx *ctx = (struct ctx *)data;
    if(ctx->verboseEnabled) printf("In [%s] Line: %d\n",__FUNCTION__,__LINE__);
}

#if defined(HAS_LIBVLC) && defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_13)
static void catchError(libvlc_exception_t *ex) {
    if(libvlc_exception_raised(ex)) {
        fprintf(stderr, "exception: %s\n", libvlc_exception_get_message(ex));
        //exit(1);
    }

    libvlc_exception_clear(ex);
}
#endif

#if defined(HAS_LIBVLC)
void trapPlayingEvent(const libvlc_event_t *evt, void *data) {
    struct ctx *ctx = (struct ctx *)data;
    if(ctx->verboseEnabled) printf("In [%s] Line: %d\n",__FUNCTION__,__LINE__);
    ctx->isPlaying = true;
}

void trapEndReachedEvent(const libvlc_event_t *evt, void *data) {
    struct ctx *ctx = (struct ctx *)data;
    if(ctx->verboseEnabled) printf("In [%s] Line: %d\n",__FUNCTION__,__LINE__);
    ctx->isPlaying = false;
}

void trapBufferingEvent(const libvlc_event_t *evt, void *data) {
    struct ctx *ctx = (struct ctx *)data;
    if(ctx->verboseEnabled) printf("In [%s] Line: %d\n",__FUNCTION__,__LINE__);
    ctx->isPlaying = true;
}

void trapErrorEvent(const libvlc_event_t *evt, void *data) {
    struct ctx *ctx = (struct ctx *)data;
    if(ctx->verboseEnabled) printf("In [%s] Line: %d\n",__FUNCTION__,__LINE__);
    ctx->isPlaying = false;
}
#endif

VideoPlayer::VideoPlayer(string filename, SDL_Surface *surface,
		int x, int y,int width, int height,int colorBits,string pluginsPath,
		bool verboseEnabled) : ctxPtr(NULL) {
	this->filename = filename;
	this->surface = surface;
	this->x = x;
	this->y = y;
	this->width = width;
	this->height = height;
	this->colorBits = colorBits;
	this->pluginsPath = pluginsPath;
	this->verboseEnabled = verboseEnabled;
	this->stop = false;
	this->finished = false;
	this->successLoadingLib = false;

	init();
}

void VideoPlayer::init() {
    //verboseEnabled = true;
	ctxPtr = new ctx();
	ctxPtr->x = x;
	ctxPtr->y = y;
	ctxPtr->width = width;
	ctxPtr->height = height;
	ctxPtr->rawData = (void *) malloc(width * height * 4);
	ctxPtr->verboseEnabled = verboseEnabled;
}

VideoPlayer::~VideoPlayer() {
	if(ctxPtr != NULL) {
		if(ctxPtr->rawData != NULL) {
			free(ctxPtr->rawData);
			ctxPtr->rawData = NULL;
		}

		delete ctxPtr;
		ctxPtr = NULL;
	}
}

bool VideoPlayer::hasBackEndVideoPlayer() {
#ifdef HAS_LIBVLC
	return true;
#endif
	return false;
}

bool VideoPlayer::initPlayer() {
#ifdef HAS_LIBVLC
	ctxPtr->libvlc = NULL;
	ctxPtr->m = NULL;
	ctxPtr->mp = NULL;
	ctxPtr->vlc_argv.clear();
	ctxPtr->vlc_argv.push_back("--intf=dummy");
	ctxPtr->vlc_argv.push_back("--no-media-library");
	ctxPtr->vlc_argv.push_back("--ignore-config"); /* Don't use VLC's config */
	ctxPtr->vlc_argv.push_back("--no-xlib"); /* tell VLC to not use Xlib */
	ctxPtr->vlc_argv.push_back("--no-video-title-show");

#if defined(LIBVLC_VERSION_PRE_2)
	ctxPtr->vlc_argv_str.push_back("--plugin-path=" + pluginsPath);
	ctxPtr->vlc_argv.push_back(ctxPtr->vlc_argv_str[ctxPtr->vlc_argv_str.size()-1].c_str());
#endif

#if defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_13)
	char clock[64], cunlock[64], cdata[64];
    char cwidth[32], cheight[32], cpitch[32];
	/*
         *  Initialise libVLC
	 */
	sprintf(clock, "%lld", (long long int)(intptr_t)lock);
	sprintf(cunlock, "%lld", (long long int)(intptr_t)unlock);
	sprintf(cdata, "%lld", (long long int)(intptr_t)ctxPtr);
	sprintf(cwidth, "%i", width);
	sprintf(cheight, "%i", height);
	sprintf(cpitch, "%i", colorBits);

	vlc_argv.push_back("--vout");
	vlc_argv.push_back("vmem");
	vlc_argv.push_back("--vmem-width");
	ctxPtr->vlc_argv_str.push_back(cwidth);
	ctxPtr->vlc_argv.push_back(ctxPtr->vlc_argv_str[ctxPtr->vlc_argv_str.size()-1].c_str());
    vlc_argv.push_back("--vmem-height");
	ctxPtr->vlc_argv_str.push_back(cheight);
	ctxPtr->vlc_argv.push_back(ctxPtr->vlc_argv_str[ctxPtr->vlc_argv_str.size()-1].c_str());
    vlc_argv.push_back("--vmem-pitch");
	ctxPtr->vlc_argv_str.push_back(cpitch);
	ctxPtr->vlc_argv.push_back(ctxPtr->vlc_argv_str[ctxPtr->vlc_argv_str.size()-1].c_str());
    vlc_argv.push_back("--vmem-chroma");
	vlc_argv.push_back("RV16");
    vlc_argv.push_back("--vmem-lock");
	ctxPtr->vlc_argv_str.push_back(clock);
	ctxPtr->vlc_argv.push_back(ctxPtr->vlc_argv_str[ctxPtr->vlc_argv_str.size()-1].c_str());
    vlc_argv.push_back("--vmem-unlock");
	ctxPtr->vlc_argv_str.push_back(cunlock);
	ctxPtr->vlc_argv.push_back(ctxPtr->vlc_argv_str[ctxPtr->vlc_argv_str.size()-1].c_str());
    vlc_argv.push_back("--vmem-data");
	ctxPtr->vlc_argv_str.push_back(cdata);
	ctxPtr->vlc_argv.push_back(ctxPtr->vlc_argv_str[ctxPtr->vlc_argv_str.size()-1].c_str());

#endif

	if(verboseEnabled) ctxPtr->vlc_argv.push_back("--verbose=2");
	if(verboseEnabled) ctxPtr->vlc_argv.push_back("--extraintf=logger"); //log anything
#if defined(WIN32)
	if(verboseEnabled) _putenv("VLC_VERBOSE=2");
#endif
	int vlc_argc = ctxPtr->vlc_argv.size();

//	char const *vlc_argv[] =
//	{
//		//"--no-audio", /* skip any audio track */
//		"--no-xlib", /* tell VLC to not use Xlib */
//		"--no-video-title-show",
//		pluginParam.c_str(),
//	};
//	int vlc_argc = sizeof(vlc_argv) / sizeof(*vlc_argv);
#endif

	ctxPtr->empty = NULL;
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	// Init Texture
	glGenTextures(1, &ctxPtr->textureId);
	glBindTexture(GL_TEXTURE_2D, ctxPtr->textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	ctxPtr->empty = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height,
			colorBits, 0, 0, 0, 0);
	ctxPtr->surf = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height,
			colorBits, 0x001f, 0x07e0, 0xf800, 0);
	ctxPtr->mutex = SDL_CreateMutex();

#ifdef HAS_LIBVLC
	/*
	 *  Initialize libVLC
	 */
	if(verboseEnabled) printf("Trying [%s]\n",getenv("VLC_PLUGIN_PATH"));

#if defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_13)
	libvlc_exception_t ex;
	libvlc_exception_init(&ex);

	ctxPtr->libvlc = libvlc_new(ctxPtr->vlc_argc, &ctxPtr->vlc_argv[0],&ex);
	catchError(&ex);
#else
	ctxPtr->libvlc = libvlc_new(vlc_argc, &ctxPtr->vlc_argv[0]);
#endif

	if(verboseEnabled) printf("In [%s] Line: %d, libvlc [%p]\n",__FUNCTION__,__LINE__,ctxPtr->libvlc);

/* It is meaningless to try all this because we have to restart mg to pickup new env vars
#if defined(WIN32)
	if(libvlc == NULL) {
		// For windows check registry for install path
		std::string strValue = getRegKey("Software\\VideoLAN\\VLC", "InstallDir");
		if(strValue != "") {
			if(strValue.length() >= 2) {
				if(strValue[0] == '"') {
					strValue = strValue.erase(0);
				}
				if(strValue[strValue.length()-1] == '"') {
					strValue = strValue.erase(strValue.length()-1);
				}
			}
			strValue = "VLC_PLUGIN_PATH=" + strValue;
			_putenv(strValue.c_str());

			if(verboseEnabled) printf("Trying [%s]\n",getenv("VLC_PLUGIN_PATH"));
			libvlc = libvlc_new(vlc_argc, &vlc_argv[0]);
		}

		if(libvlc == NULL) {
			_putenv("VLC_PLUGIN_PATH=c:\\program files\\videolan\\vlc\\plugins");

			if(verboseEnabled) printf("Trying [%s]\n",getenv("VLC_PLUGIN_PATH"));
			libvlc = libvlc_new(vlc_argc, &vlc_argv[0]);
		}
		if(libvlc == NULL) {
			_putenv("VLC_PLUGIN_PATH=\\program files\\videolan\\vlc\\plugins");

			if(verboseEnabled) printf("Trying [%s]\n",getenv("VLC_PLUGIN_PATH"));
			libvlc = libvlc_new(vlc_argc, &vlc_argv[0]);
		}
		if(libvlc == NULL) {
			_putenv("VLC_PLUGIN_PATH=c:\\program files (x86)\\videolan\\vlc\\plugins");

			if(verboseEnabled) printf("Trying [%s]\n",getenv("VLC_PLUGIN_PATH"));
			libvlc = libvlc_new(vlc_argc, &vlc_argv[0]);
		}
		if(libvlc == NULL) {
			_putenv("VLC_PLUGIN_PATH=\\program files (x86)\\videolan\\vlc\\plugins");

			if(verboseEnabled) printf("Trying [%s]\n",getenv("VLC_PLUGIN_PATH"));
			libvlc = libvlc_new(vlc_argc, &vlc_argv[0]);
		}
	}
#endif
*/

	if(ctxPtr->libvlc != NULL) {
#if defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_13)
		ctxPtr->m = libvlc_media_new(ctxPtr->libvlc, filename.c_str(), &ex);
		if(verboseEnabled) printf("In [%s] Line: %d, m [%p]\n",__FUNCTION__,__LINE__,ctxPtr->m);

		catchError(&ex);
		ctxPtr->mp = libvlc_media_player_new_from_media(ctxPtr->m);
		if(verboseEnabled) printf("In [%s] Line: %d, mp [%p]\n",__FUNCTION__,__LINE__,ctxPtr->mp);
#else
		/* Create a new item */
		ctxPtr->m = libvlc_media_new_path(ctxPtr->libvlc, filename.c_str());
		if(verboseEnabled) printf("In [%s] Line: %d, m [%p]\n",__FUNCTION__,__LINE__,ctxPtr->m);

		ctxPtr->mp = libvlc_media_player_new_from_media(ctxPtr->m);
		if(verboseEnabled) printf("In [%s] Line: %d, mp [%p]\n",__FUNCTION__,__LINE__,ctxPtr->mp);
#endif
		libvlc_media_release(ctxPtr->m);

#if !defined(LIBVLC_VERSION_PRE_2) && !defined(LIBVLC_VERSION_PRE_1_1_13)
		libvlc_video_set_callbacks(ctxPtr->mp, lock, unlock, display, ctxPtr);
		libvlc_video_set_format(ctxPtr->mp, "RV16", width, height, this->surface->pitch);

		// Get an event manager for the media player.
		//libvlc_event_manager_t *eventManager = libvlc_media_player_event_manager(mp, &ex);
		libvlc_event_manager_t *eventManager = libvlc_media_player_event_manager(ctxPtr->mp);
		if(eventManager) {
//			libvlc_event_attach(eventManager, libvlc_MediaPlayerPlaying, (libvlc_callback_t)trapPlayingEvent, NULL, &ex);
//			libvlc_event_attach(eventManager, libvlc_MediaPlayerEndReached, (libvlc_callback_t)trapEndReachedEvent, NULL, &ex);
//			libvlc_event_attach(eventManager, libvlc_MediaPlayerBuffering, (libvlc_callback_t)trapBufferingEvent, NULL, &ex);
//			libvlc_event_attach(eventManager, libvlc_MediaPlayerEncounteredError, (libvlc_callback_t)trapErrorEvent, NULL, &ex);
			libvlc_event_attach(eventManager, libvlc_MediaPlayerPlaying, (libvlc_callback_t)trapPlayingEvent, ctxPtr);
			libvlc_event_attach(eventManager, libvlc_MediaPlayerEndReached, (libvlc_callback_t)trapEndReachedEvent, ctxPtr);
			libvlc_event_attach(eventManager, libvlc_MediaPlayerBuffering, (libvlc_callback_t)trapBufferingEvent, ctxPtr);
			libvlc_event_attach(eventManager, libvlc_MediaPlayerEncounteredError, (libvlc_callback_t)trapErrorEvent, ctxPtr);
		}
#endif

		ctxPtr->isPlaying = true;
#if defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_13)
		int play_result = libvlc_media_player_play(ctxPtr->mp,&ex);
#else
		int play_result = libvlc_media_player_play(ctxPtr->mp);
#endif

		//SDL_Delay(5);
		if(verboseEnabled) printf("In [%s] Line: %d, play_result [%d]\n",__FUNCTION__,__LINE__,play_result);

		successLoadingLib = (play_result == 0);
	}
#endif

	return successLoadingLib;
}

void VideoPlayer::closePlayer() {
#ifdef HAS_LIBVLC
	if(ctxPtr->libvlc != NULL) {
		//
		// Stop stream and clean up libVLC
		//
#if defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_13)
		libvlc_media_player_stop(ctxPtr->mp,&ex);
		catchError(&ex);
#else
		libvlc_media_player_stop(ctxPtr->mp);
#endif
		libvlc_media_player_release(ctxPtr->mp);
		libvlc_release(ctxPtr->libvlc);
	}
#endif

	//
	// Close window and clean up libSDL
	//
	if(ctxPtr->mutex != NULL) {
		SDL_DestroyMutex(ctxPtr->mutex);
	}
	if(ctxPtr->surf != NULL) {
		SDL_FreeSurface(ctxPtr->surf);
	}
	if(ctxPtr->empty != NULL) {
		SDL_FreeSurface(ctxPtr->empty);
	}

	glDeleteTextures(1, &ctxPtr->textureId);

	if(ctxPtr->needToQuit == true) {
		SDL_Event quit_event = {SDL_QUIT};
		SDL_PushEvent(&quit_event);
	}
}

void VideoPlayer::PlayVideo() {
/*
#ifdef HAS_LIBVLC
	libvlc_instance_t *libvlc = NULL;
	libvlc_media_t *m = NULL;
	libvlc_media_player_t *mp = NULL;

//	string pluginParam = "";
//	if(pluginsPath != "") {
//		pluginParam = "--plugin-path=" + pluginsPath;
//	}

	std::vector<const char *> vlc_argv;
	vlc_argv.push_back("--intf=dummy");
	vlc_argv.push_back("--no-media-library");
	vlc_argv.push_back("--ignore-config"); // Don't use VLC's config
	vlc_argv.push_back("--no-xlib"); // tell VLC to not use Xlib
	vlc_argv.push_back("--no-video-title-show");
#if defined(LIBVLC_VERSION_PRE_2)
	string fullPluginsParam = "--plugin-path=" + pluginsPath;
	vlc_argv.push_back(fullPluginsParam.c_str());
#endif

#if defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_13)
	char clock[64], cunlock[64], cdata[64];
    	char cwidth[32], cheight[32], cpitch[32];
	//
    //  Initialise libVLC
	//
	sprintf(clock, "%lld", (long long int)(intptr_t)lock);
	sprintf(cunlock, "%lld", (long long int)(intptr_t)unlock);
	sprintf(cdata, "%lld", (long long int)(intptr_t)ctxPtr);
	sprintf(cwidth, "%i", width);
	sprintf(cheight, "%i", height);
	sprintf(cpitch, "%i", colorBits);

	vlc_argv.push_back("--vout");
	vlc_argv.push_back("vmem");
	vlc_argv.push_back("--vmem-width");
	vlc_argv.push_back(cwidth);
        vlc_argv.push_back("--vmem-height");
	vlc_argv.push_back(cheight);
        vlc_argv.push_back("--vmem-pitch");
	vlc_argv.push_back(cpitch);
        vlc_argv.push_back("--vmem-chroma");
	vlc_argv.push_back("RV16");
        vlc_argv.push_back("--vmem-lock");
	vlc_argv.push_back(clock);
        vlc_argv.push_back("--vmem-unlock");
	vlc_argv.push_back(cunlock);
        vlc_argv.push_back("--vmem-data");
	vlc_argv.push_back(cdata);

#endif

	if(verboseEnabled) vlc_argv.push_back("--verbose=2");
	if(verboseEnabled) vlc_argv.push_back("--extraintf=logger"); //log anything
#if defined(WIN32)
	if(verboseEnabled) _putenv("VLC_VERBOSE=2");
#endif
	int vlc_argc = vlc_argv.size();

//	char const *vlc_argv[] =
//	{
//		//"--no-audio", // skip any audio track
//		"--no-xlib", // tell VLC to not use Xlib
//		"--no-video-title-show",
//		pluginParam.c_str(),
//	};
//	int vlc_argc = sizeof(vlc_argv) / sizeof(*vlc_argv);
#endif

	SDL_Surface *empty = NULL;
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	// Init Texture
	glGenTextures(1, &ctxPtr->textureId);
	glBindTexture(GL_TEXTURE_2D, ctxPtr->textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	empty = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height,
			colorBits, 0, 0, 0, 0);
	ctxPtr->surf = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height,
			colorBits, 0x001f, 0x07e0, 0xf800, 0);
	ctxPtr->mutex = SDL_CreateMutex();

#ifdef HAS_LIBVLC
	//
	//  Initialize libVLC
	//
	if(verboseEnabled) printf("Trying [%s]\n",getenv("VLC_PLUGIN_PATH"));

#if defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_13)
	libvlc_exception_t ex;
	libvlc_exception_init(&ex);

	libvlc = libvlc_new(vlc_argc, &vlc_argv[0],&ex);
	catchError(&ex);
#else
	libvlc = libvlc_new(vlc_argc, &vlc_argv[0]);
#endif

	if(verboseEnabled) printf("In [%s] Line: %d, libvlc [%p]\n",__FUNCTION__,__LINE__,libvlc);

// It is meaningless to try all this because we have to restart mg to pickup new env vars
#if defined(WIN32)
	if(libvlc == NULL) {
		// For windows check registry for install path
		std::string strValue = getRegKey("Software\\VideoLAN\\VLC", "InstallDir");
		if(strValue != "") {
			if(strValue.length() >= 2) {
				if(strValue[0] == '"') {
					strValue = strValue.erase(0);
				}
				if(strValue[strValue.length()-1] == '"') {
					strValue = strValue.erase(strValue.length()-1);
				}
			}
			strValue = "VLC_PLUGIN_PATH=" + strValue;
			_putenv(strValue.c_str());

			if(verboseEnabled) printf("Trying [%s]\n",getenv("VLC_PLUGIN_PATH"));
			libvlc = libvlc_new(vlc_argc, &vlc_argv[0]);
		}

		if(libvlc == NULL) {
			_putenv("VLC_PLUGIN_PATH=c:\\program files\\videolan\\vlc\\plugins");

			if(verboseEnabled) printf("Trying [%s]\n",getenv("VLC_PLUGIN_PATH"));
			libvlc = libvlc_new(vlc_argc, &vlc_argv[0]);
		}
		if(libvlc == NULL) {
			_putenv("VLC_PLUGIN_PATH=\\program files\\videolan\\vlc\\plugins");

			if(verboseEnabled) printf("Trying [%s]\n",getenv("VLC_PLUGIN_PATH"));
			libvlc = libvlc_new(vlc_argc, &vlc_argv[0]);
		}
		if(libvlc == NULL) {
			_putenv("VLC_PLUGIN_PATH=c:\\program files (x86)\\videolan\\vlc\\plugins");
			
			if(verboseEnabled) printf("Trying [%s]\n",getenv("VLC_PLUGIN_PATH"));
			libvlc = libvlc_new(vlc_argc, &vlc_argv[0]);
		}
		if(libvlc == NULL) {
			_putenv("VLC_PLUGIN_PATH=\\program files (x86)\\videolan\\vlc\\plugins");

			if(verboseEnabled) printf("Trying [%s]\n",getenv("VLC_PLUGIN_PATH"));
			libvlc = libvlc_new(vlc_argc, &vlc_argv[0]);
		}
	}
#endif
//

	if(libvlc != NULL) {
#if defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_13)
		m = libvlc_media_new(libvlc, filename.c_str(), &ex);
		if(verboseEnabled) printf("In [%s] Line: %d, m [%p]\n",__FUNCTION__,__LINE__,m);

		catchError(&ex);
		mp = libvlc_media_player_new_from_media(m);
		if(verboseEnabled) printf("In [%s] Line: %d, mp [%p]\n",__FUNCTION__,__LINE__,mp);
#else
		// Create a new item
		m = libvlc_media_new_path(libvlc, filename.c_str());
		if(verboseEnabled) printf("In [%s] Line: %d, m [%p]\n",__FUNCTION__,__LINE__,m);

		mp = libvlc_media_player_new_from_media(m);
		if(verboseEnabled) printf("In [%s] Line: %d, mp [%p]\n",__FUNCTION__,__LINE__,mp);
#endif
		libvlc_media_release(m);

#if !defined(LIBVLC_VERSION_PRE_2) && !defined(LIBVLC_VERSION_PRE_1_1_13)
		libvlc_video_set_callbacks(mp, lock, unlock, display, ctxPtr);
		libvlc_video_set_format(mp, "RV16", width, height, this->surface->pitch);

		// Get an event manager for the media player.
		//libvlc_event_manager_t *eventManager = libvlc_media_player_event_manager(mp, &ex);
		libvlc_event_manager_t *eventManager = libvlc_media_player_event_manager(mp);
		if(eventManager) {
//			libvlc_event_attach(eventManager, libvlc_MediaPlayerPlaying, (libvlc_callback_t)trapPlayingEvent, NULL, &ex);
//			libvlc_event_attach(eventManager, libvlc_MediaPlayerEndReached, (libvlc_callback_t)trapEndReachedEvent, NULL, &ex);
//			libvlc_event_attach(eventManager, libvlc_MediaPlayerBuffering, (libvlc_callback_t)trapBufferingEvent, NULL, &ex);
//			libvlc_event_attach(eventManager, libvlc_MediaPlayerEncounteredError, (libvlc_callback_t)trapErrorEvent, NULL, &ex);
			libvlc_event_attach(eventManager, libvlc_MediaPlayerPlaying, (libvlc_callback_t)trapPlayingEvent, ctxPtr);
			libvlc_event_attach(eventManager, libvlc_MediaPlayerEndReached, (libvlc_callback_t)trapEndReachedEvent, ctxPtr);
			libvlc_event_attach(eventManager, libvlc_MediaPlayerBuffering, (libvlc_callback_t)trapBufferingEvent, ctxPtr);
			libvlc_event_attach(eventManager, libvlc_MediaPlayerEncounteredError, (libvlc_callback_t)trapErrorEvent, ctxPtr);
		}
#endif
		
		ctxPtr->isPlaying = true;
#if defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_13)
		int play_result = libvlc_media_player_play(mp,&ex);
#else
		int play_result = libvlc_media_player_play(mp);
#endif

		//SDL_Delay(5);
		if(verboseEnabled) printf("In [%s] Line: %d, play_result [%d]\n",__FUNCTION__,__LINE__,play_result);

		successLoadingLib = (play_result == 0);
	}
#endif

	//
	 /  Main loop
	 //
	//int done = 0, action = 0, pause = 0, n = 0;
	bool needToQuit = false;
	while(isPlaying() == true) {
		needToQuit = playFrame();
	}

#ifdef HAS_LIBVLC
	if(libvlc != NULL) {
		//
		 / Stop stream and clean up libVLC
		 //
#if defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_13)
		libvlc_media_player_stop(mp,&ex);
		catchError(&ex);
#else
		libvlc_media_player_stop(mp);
#endif
		libvlc_media_player_release(mp);
		libvlc_release(libvlc);
	}
#endif

	//
	 / Close window and clean up libSDL
	 //
	SDL_DestroyMutex(ctxPtr->mutex);
	SDL_FreeSurface(ctxPtr->surf);
	SDL_FreeSurface(empty);

	glDeleteTextures(1, &ctxPtr->textureId);

	if(needToQuit == true) {
		SDL_Event quit_event = {SDL_QUIT};
		SDL_PushEvent(&quit_event);
	}
*/

	initPlayer();
	for(;isPlaying() == true;) {
		playFrame();
	}
	closePlayer();
}

bool VideoPlayer::isPlaying() const {
	bool result = (successLoadingLib == true &&
					ctxPtr != NULL && ctxPtr->isPlaying == true &&
					finished == false && stop == false);
	return result;
}

bool VideoPlayer::playFrame(bool swapBuffers) {
	//bool needToQuit = false;
	int action = 0, pause = 0, n = 0;
	if(successLoadingLib == true &&
		ctxPtr != NULL && ctxPtr->isPlaying == true &&
		finished == false && stop == false) {
		action = 0;

		SDL_Event event;
		/* Keys: enter (fullscreen), space (pause), escape (quit) */
		while( SDL_PollEvent( &event ) ) {
			switch(event.type) {
			case SDL_QUIT:
				finished = true;
				ctxPtr->needToQuit = true;
				break;
			case SDL_KEYDOWN:
				action = event.key.keysym.sym;
				break;
			case SDL_MOUSEBUTTONDOWN:
				finished = true;
				break;
			}
		}

		if(finished == false && stop == false) {
			switch(action) {
			case SDLK_ESCAPE:
				finished = true;
				break;
			case SDLK_RETURN:
				//options ^= SDL_FULLSCREEN;
				//screen = SDL_SetVideoMode(WIDTH, HEIGHT, 0, options);
				finished = true;
				break;
			case ' ':
				//pause = !pause;
				break;
			}

			if(pause == 0) {
				n++;
			}

		    //assertGl();

			glPushAttrib(GL_ENABLE_BIT);

			//glDisable(GL_LIGHTING);
			glEnable(GL_TEXTURE_2D);
			glEnable(GL_BLEND);

			loadTexture(ctxPtr); // Loading the texture
			// Square
//			glBegin(GL_QUADS);
//			 glTexCoord2d(0, 1);
//			 glVertex2f(-0.85, 0.85);
//			 glTexCoord2d(1, 1);
//			 glVertex2f(0.85, 0.85);
//			 glTexCoord2d(1, 0);
//			 glVertex2f(0.85, -0.85);
//			 glTexCoord2d(0, 0);
//			 glVertex2f(-0.85, -0.85);
//			glEnd();

//			glBegin(GL_TRIANGLE_STRIP);
//				glTexCoord2i(0, 1);
//				glVertex2i(1, 1+600);
//				glTexCoord2i(0, 0);
//				glVertex2i(1, 1);
//				glTexCoord2i(1, 1);
//				glVertex2i(1+800, 1+600);
//				glTexCoord2i(1, 0);
//				glVertex2i(1+800, 600);
//			glEnd();

//			glBegin(GL_TRIANGLE_STRIP);
//				glTexCoord2i(0, 1);
//				glVertex2i(ctxPtr->x, ctxPtr->y + ctxPtr->height);
//				glTexCoord2i(0, 0);
//				glVertex2i(ctxPtr->x, ctxPtr->y);
//				glTexCoord2i(1, 1);
//				glVertex2i(ctxPtr->x + ctxPtr->width, ctxPtr->y + ctxPtr->height);
//				glTexCoord2i(1, 0);
//				glVertex2i(ctxPtr->x + ctxPtr->width, ctxPtr->y);
//			glEnd();

			if(ctxPtr->x == -1 || ctxPtr->y == -1) {
				glBegin(GL_QUADS);
				 glTexCoord2d(0, 1);
				 glVertex2f(-0.85, 0.85);
				 glTexCoord2d(1, 1);
				 glVertex2f(0.85, 0.85);
				 glTexCoord2d(1, 0);
				 glVertex2f(0.85, -0.85);
				 glTexCoord2d(0, 0);
				 glVertex2f(-0.85, -0.85);
				glEnd();
			}
			else {
//				glBegin(GL_QUADS);
//				 glTexCoord2d(0, 1);
//				 glVertex2f(ctxPtr->x, ctxPtr->y + ctxPtr->height);
//				 glTexCoord2d(1, 1);
//				 glVertex2f(ctxPtr->x + ctxPtr->width, ctxPtr->y + ctxPtr->height);
//				 glTexCoord2d(1, 0);
//				 glVertex2f(ctxPtr->x + ctxPtr->width, ctxPtr->y);
//				 glTexCoord2d(0, 0);
//				 glVertex2f(ctxPtr->x, ctxPtr->y);
//				glEnd();

				glBegin(GL_TRIANGLE_STRIP);
					glTexCoord2i(0, 1);
					glVertex2i(ctxPtr->x, ctxPtr->y + ctxPtr->height * 0.80);
					glTexCoord2i(0, 0);
					glVertex2i(ctxPtr->x, ctxPtr->y);
					glTexCoord2i(1, 1);
					glVertex2i(ctxPtr->x+ctxPtr->width * 0.60, ctxPtr->y+ctxPtr->height * 0.80);
					glTexCoord2i(1, 0);
					glVertex2i(ctxPtr->x+ctxPtr->width * 0.60, ctxPtr->y);
				glEnd();

			}

			glPopAttrib();

			if(swapBuffers == true) {
				SDL_GL_SwapBuffers();
			}
		}
	}

	return ctxPtr->needToQuit;
}

}}
