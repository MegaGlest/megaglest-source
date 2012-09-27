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
#include <time.h>

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

const string HTTP_PREFIX 					= "http";
const double MAX_VIDEO_START_MILLISECONDS	= 10.0;

class ctx {
public:
	ctx() {
		loadingCB = NULL;
		empty = NULL;
	    textureId = 0; // Texture ID
	    surf = NULL;
	    mutex = NULL;
	    x= 0;
	    y = 0;
	    width = 0;
	    height = 0;
	    rawData = NULL;
	    started = false;
	    error = false;
	    stopped = false;
	    end_of_media = false;
	    isPlaying = 0;
	    needToQuit = false;
	    verboseEnabled = 0;

#ifdef HAS_LIBVLC
		libvlc = NULL;
		m = NULL;
		mp = NULL;
		ml = NULL;
		mlp = NULL;

		vlc_argv.clear();
		vlc_argv_str.clear();
#endif
	}

	Shared::Graphics::VideoLoadingCallbackInterface *loadingCB;
	SDL_Surface *empty;
    GLuint textureId; // Texture ID
    SDL_Surface *surf;
    SDL_mutex *mutex;
    int x;
    int y;
    int width;
    int height;
    void *rawData;
    bool started;
    bool error;
    bool stopped;
    bool end_of_media;
    bool isPlaying;
    bool needToQuit;
    bool verboseEnabled;

#ifdef HAS_LIBVLC
	libvlc_instance_t *libvlc;
	libvlc_media_t *m;
	libvlc_media_player_t *mp;

	libvlc_media_list_t *ml;
	libvlc_media_list_player_t *mlp;

	std::vector<const char *> vlc_argv;
	std::vector<string> vlc_argv_str;
#endif
};

namespace Shared{ namespace Graphics{

bool VideoPlayer::disabled = false;

// Load a texture
static inline void loadTexture(class ctx *ctx) {
   if(ctx->verboseEnabled) printf("In [%s] Line: %d\n",__FUNCTION__,__LINE__);

   void *rawData = ctx->rawData;
   Uint8 * pixelSource = 0;
   Uint8 * pixelDestination = (Uint8 *) rawData;
   Uint32 pix = 0;

   for ( int i = ctx->height; i > 0; i--) {
      for (int j = 0; j < ctx->width; j++) {
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
    class ctx *ctx = (class ctx *)data;
    if(ctx->verboseEnabled) printf("In [%s] Line: %d\n",__FUNCTION__,__LINE__);

    SDL_LockMutex(ctx->mutex);
    SDL_LockSurface(ctx->surf);
    *p_pixels = ctx->surf->pixels;

    return NULL; /* picture identifier, not needed here */
}

static void unlock(void *data, void *id, void *const *p_pixels) {
    class ctx *ctx = (class ctx *)data;
    if(ctx->verboseEnabled) printf("In [%s] Line: %d\n",__FUNCTION__,__LINE__);

    /* VLC just rendered the video, but we can also render stuff */
    SDL_UnlockSurface(ctx->surf);
    SDL_UnlockMutex(ctx->mutex);
}

static void display(void *data, void *id) {
    // VLC wants to display the video
    class ctx *ctx = (class ctx *)data;
    if(ctx->verboseEnabled) printf("In [%s] Line: %d\n",__FUNCTION__,__LINE__);
}

#if defined(HAS_LIBVLC) && defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_0)
static void catchError(libvlc_exception_t *ex) {
    if(libvlc_exception_raised(ex)) {
        fprintf(stderr, "exception: %s\n", libvlc_exception_get_message(ex));
        //exit(1);
    }

    libvlc_exception_clear(ex);
}
#endif

#if defined(HAS_LIBVLC)

/*
void trapPlayingEvent(const libvlc_event_t *evt, void *data) {
    class ctx *ctx = (class ctx *)data;
    if(ctx->verboseEnabled) printf("In [%s] Line: %d\n",__FUNCTION__,__LINE__);
    ctx->isPlaying = true;
    ctx->started = true;
}

void trapEndReachedEvent(const libvlc_event_t *evt, void *data) {
    class ctx *ctx = (class ctx *)data;
    if(ctx->verboseEnabled) printf("In [%s] Line: %d\n",__FUNCTION__,__LINE__);
    ctx->isPlaying = false;
}

void trapBufferingEvent(const libvlc_event_t *evt, void *data) {
    class ctx *ctx = (class ctx *)data;
    if(ctx->verboseEnabled) printf("In [%s] Line: %d\n",__FUNCTION__,__LINE__);
    ctx->isPlaying = true;
}

void trapErrorEvent(const libvlc_event_t *evt, void *data) {
    class ctx *ctx = (class ctx *)data;
    if(ctx->verboseEnabled) printf("In [%s] Line: %d\n",__FUNCTION__,__LINE__);
    ctx->isPlaying = false;
}
*/
void callbacks( const libvlc_event_t* event, void* data ) {
	class ctx *ctx = (class ctx *)data;
	if(ctx->verboseEnabled) printf("In [%s] Line: %d event [%d]\n",__FUNCTION__,__LINE__,event->type);
	switch ( event->type ) {
	    case libvlc_MediaPlayerPlaying:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaPlayerPlaying\n");
	    	ctx->end_of_media = false;
	    	ctx->error = false;
	    	ctx->stopped = false;
	    	ctx->isPlaying = true;
	        ctx->started = true;

	        break;
	    case libvlc_MediaPlayerPaused:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaPlayerPaused\n");
	        break;
	    case libvlc_MediaPlayerStopped:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaPlayerStopped\n");
	    	ctx->stopped = true;

	        break;
	    case libvlc_MediaPlayerEndReached:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaPlayerEndReached\n");
	    	if(ctx->started == true && ctx->error == false) {
	    		ctx->isPlaying = false;
	    	}

	    	ctx->end_of_media = true;

	        break;
	    case libvlc_MediaPlayerTimeChanged:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaPlayerTimeChanged\n");
	        break;
	    case libvlc_MediaPlayerPositionChanged:
	        //qDebug() << self << "position changed : " << event->u.media_player_position_changed.new_position;
	        //self->emit positionChanged( event->u.media_player_position_changed.new_position );
	    	if(ctx->verboseEnabled) printf("libvlc_MediaPlayerPositionChanged\n");
	        break;
	    case libvlc_MediaPlayerLengthChanged:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaPlayerLengthChanged\n");
	        break;
	    case libvlc_MediaPlayerSnapshotTaken:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaPlayerSnapshotTaken\n");
	        break;
	    case libvlc_MediaPlayerEncounteredError:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaPlayerEncounteredError\n");
	    	if(ctx->started == false) {
	    		ctx->isPlaying = false;
	    	}
	    	ctx->error = true;


	        break;
	    case libvlc_MediaPlayerSeekableChanged:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaPlayerSeekableChanged\n");
	        break;
	    case libvlc_MediaPlayerPausableChanged:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaPlayerPausableChanged\n");
	        break;
	    case libvlc_MediaPlayerTitleChanged:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaPlayerTitleChanged\n");
	        break;
	    case libvlc_MediaPlayerNothingSpecial:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaPlayerNothingSpecial\n");
	        break;
	    case libvlc_MediaPlayerOpening:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaPlayerOpening\n");
	        break;
	    case libvlc_MediaPlayerBuffering:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaPlayerBuffering\n");
	        break;
	    case libvlc_MediaPlayerForward:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaPlayerForward\n");
	        break;
	    case libvlc_MediaPlayerBackward:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaPlayerBackward\n");
	        break;
	    case libvlc_MediaStateChanged:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaStateChanged\n");
	        break;
	    case libvlc_MediaParsedChanged:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaParsedChanged\n");
	        break;

#if defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_0)
	    case libvlc_MediaPlayerVout:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaPlayerVout\n");
	        break;
#endif
	    case libvlc_MediaListItemAdded:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaListItemAdded\n");
	        break;
	    case libvlc_MediaListWillAddItem:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaListWillAddItem\n");
	        break;
	    case libvlc_MediaListItemDeleted:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaListItemDeleted\n");
	        break;
	    case libvlc_MediaListWillDeleteItem:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaListWillDeleteItem\n");
	        break;
	    case libvlc_MediaListViewItemAdded:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaListViewItemAdded\n");
	        break;
	    case libvlc_MediaListViewWillAddItem:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaListViewWillAddItem\n");
	        break;
	    case libvlc_MediaListViewItemDeleted:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaListViewItemDeleted\n");
	        break;
	    case libvlc_MediaListViewWillDeleteItem:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaListViewWillDeleteItem\n");
	        break;
	    case libvlc_MediaListPlayerPlayed:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaListPlayerPlayed\n");
	        break;
	    case libvlc_MediaListPlayerNextItemSet:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaListPlayerNextItemSet\n");
	        //ctx->isPlaying = true;
	        //ctx->started = true;
	    	ctx->end_of_media = false;
	    	ctx->error = false;
	    	ctx->stopped = false;

	        break;
	    case libvlc_MediaListPlayerStopped:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaListPlayerStopped\n");
	    	ctx->stopped = true;

	        break;

	    case libvlc_MediaDiscovererStarted:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaDiscovererStarted\n");
	        break;
	    case libvlc_MediaDiscovererEnded:
	    	if(ctx->verboseEnabled) printf("libvlc_MediaDiscovererEndedvvvvvvvvv\n");
	        break;
	    case libvlc_VlmMediaAdded:
	    	if(ctx->verboseEnabled) printf("libvlc_VlmMediaAdded\n");
	        break;
	    case libvlc_VlmMediaRemoved:
	    	if(ctx->verboseEnabled) printf("libvlc_VlmMediaRemoved\n");
	        break;
	    case libvlc_VlmMediaChanged:
	    	if(ctx->verboseEnabled) printf("libvlc_VlmMediaChanged\n");
	        break;
	    case libvlc_VlmMediaInstanceStarted:
	    	if(ctx->verboseEnabled) printf("libvlc_VlmMediaInstanceStarted\n");
	        break;
	    case libvlc_VlmMediaInstanceStopped:
	    	if(ctx->verboseEnabled) printf("libvlc_VlmMediaInstanceStopped\n");
	    	ctx->stopped = true;

	        break;
	    case libvlc_VlmMediaInstanceStatusInit:
	    	if(ctx->verboseEnabled) printf("libvlc_VlmMediaInstanceStatusInit\n");
	        break;
	    case libvlc_VlmMediaInstanceStatusOpening:
	    	if(ctx->verboseEnabled) printf("libvlc_VlmMediaInstanceStatusOpening\n");
	        break;
	    case libvlc_VlmMediaInstanceStatusPlaying:
	    	if(ctx->verboseEnabled) printf("libvlc_VlmMediaInstanceStatusPlaying\n");
	        break;
	    case libvlc_VlmMediaInstanceStatusPause:
	    	if(ctx->verboseEnabled) printf("libvlc_VlmMediaInstanceStatusPause\n");
	        break;
	    case libvlc_VlmMediaInstanceStatusEnd:
	    	if(ctx->verboseEnabled) printf("libvlc_VlmMediaInstanceStatusEnd\n");
	        break;
	    case libvlc_VlmMediaInstanceStatusError:
	    	if(ctx->verboseEnabled) printf("libvlc_VlmMediaInstanceStatusError\n");
	        break;

	    default:
	//        qDebug() << "Unknown mediaPlayerEvent: " << event->type;
	        break;
	}
}

#endif

VideoPlayer::VideoPlayer(VideoLoadingCallbackInterface *loadingCB,
							string filename,
							string filenameFallback,
							SDL_Surface *surface,
							int x, int y,int width, int height,int colorBits,
							bool loop, string pluginsPath, bool verboseEnabled)
	: ctxPtr(NULL) {

	this->loadingCB = loadingCB;
	this->filename = filename;
	this->filenameFallback = filenameFallback;
	this->surface = surface;
	this->x = x;
	this->y = y;
	this->width = width;
	this->height = height;
	this->colorBits = colorBits;
	this->loop = loop;
	this->pluginsPath = pluginsPath;
	//this->verboseEnabled = true;
	this->verboseEnabled = verboseEnabled;
	this->stop = false;
	this->finished = false;
	this->successLoadingLib = false;

	init();
}

void VideoPlayer::init() {
	if(VideoPlayer::disabled == true) {
		return;
	}
	cleanupPlayer();
	ctxPtr = new ctx();
	ctxPtr->loadingCB = loadingCB;
	ctxPtr->x = x;
	ctxPtr->y = y;
	ctxPtr->width = width;
	ctxPtr->height = height;
	ctxPtr->rawData = (void *) malloc(width * height * 4);
	ctxPtr->verboseEnabled = verboseEnabled;
}

VideoPlayer::~VideoPlayer() {
	cleanupPlayer();
}

bool VideoPlayer::hasBackEndVideoPlayer() {
	if(VideoPlayer::disabled == true) {
		return false;
	}

#ifdef HAS_LIBVLC
	return true;
#endif
	return false;
}

void VideoPlayer::cleanupPlayer() {
	if(ctxPtr != NULL) {
		if(ctxPtr->rawData != NULL) {
			free(ctxPtr->rawData);
			ctxPtr->rawData = NULL;
		}

		delete ctxPtr;
		ctxPtr = NULL;
	}
}

bool VideoPlayer::initPlayer(string mediaURL) {
	if(VideoPlayer::disabled == true) {
		return true;
	}

#ifdef HAS_LIBVLC
	ctxPtr->libvlc = NULL;
	ctxPtr->m = NULL;
	ctxPtr->mp = NULL;
	ctxPtr->vlc_argv.clear();
	ctxPtr->vlc_argv.push_back("--intf=dummy");
	//ctxPtr->vlc_argv.push_back("--intf=http");
	//ctxPtr->vlc_argv.push_back("--no-media-library");
	ctxPtr->vlc_argv.push_back("--ignore-config"); /* Don't use VLC's config */
	ctxPtr->vlc_argv.push_back("--no-xlib"); /* tell VLC to not use Xlib */
	ctxPtr->vlc_argv.push_back("--no-video-title-show");
	//ctxPtr->vlc_argv.push_back("--network-caching=10000");

	if(loop == true) {
		ctxPtr->vlc_argv.push_back("--loop");
		ctxPtr->vlc_argv.push_back("--repeat");
	}

#if defined(LIBVLC_VERSION_PRE_2)
	ctxPtr->vlc_argv_str.push_back("--plugin-path=" + pluginsPath);
	ctxPtr->vlc_argv.push_back(ctxPtr->vlc_argv_str[ctxPtr->vlc_argv_str.size()-1].c_str());
#endif

#if defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_0)
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

#if defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_0)
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
#if defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_0)
		ctxPtr->m = libvlc_media_new(ctxPtr->libvlc, mediaURL.c_str(), &ex);
		if(verboseEnabled) printf("In [%s] Line: %d, m [%p]\n",__FUNCTION__,__LINE__,ctxPtr->m);

		catchError(&ex);
		ctxPtr->mp = libvlc_media_player_new_from_media(ctxPtr->m);
		if(verboseEnabled) printf("In [%s] Line: %d, mp [%p]\n",__FUNCTION__,__LINE__,ctxPtr->mp);

		libvlc_media_release(ctxPtr->m);
#else
		if(mediaURL.find(HTTP_PREFIX) == 0) {
			ctxPtr->mlp = libvlc_media_list_player_new(ctxPtr->libvlc);
			ctxPtr->ml = libvlc_media_list_new(ctxPtr->libvlc);
			ctxPtr->m = libvlc_media_new_location(ctxPtr->libvlc, mediaURL.c_str());

			libvlc_media_list_add_media(ctxPtr->ml, ctxPtr->m);
		}
		else {
			ctxPtr->m = libvlc_media_new_path(ctxPtr->libvlc, mediaURL.c_str());
		}

		/* Create a new item */
		if(verboseEnabled) printf("In [%s] Line: %d, m [%p]\n",__FUNCTION__,__LINE__,ctxPtr->m);

		if(loop == true) {
			libvlc_media_add_option(ctxPtr->m, "input-repeat=-1");
		}

		if(mediaURL.find(HTTP_PREFIX) == 0) {
			ctxPtr->mp = libvlc_media_player_new(ctxPtr->libvlc);
		}
		else {
			ctxPtr->mp = libvlc_media_player_new_from_media(ctxPtr->m);
		}
		if(verboseEnabled) printf("In [%s] Line: %d, mp [%p]\n",__FUNCTION__,__LINE__,ctxPtr->mp);

		libvlc_media_player_set_media(ctxPtr->mp, ctxPtr->m);

		libvlc_media_release(ctxPtr->m);

		if(mediaURL.find(HTTP_PREFIX) == 0) {
			// Use our media list
			libvlc_media_list_player_set_media_list(ctxPtr->mlp, ctxPtr->ml);

			// Use a given media player
			libvlc_media_list_player_set_media_player(ctxPtr->mlp, ctxPtr->mp);
		}

		// Get an event manager for the media player.
		if(mediaURL.find(HTTP_PREFIX) == 0) {
			libvlc_event_manager_t *eventManager = libvlc_media_list_player_event_manager(ctxPtr->mlp);

			if(eventManager) {
//				libvlc_event_attach(eventManager, libvlc_MediaPlayerPlaying, (libvlc_callback_t)trapPlayingEvent, NULL, &ex);
//				libvlc_event_attach(eventManager, libvlc_MediaPlayerEndReached, (libvlc_callback_t)trapEndReachedEvent, NULL, &ex);
//				libvlc_event_attach(eventManager, libvlc_MediaPlayerBuffering, (libvlc_callback_t)trapBufferingEvent, NULL, &ex);
//				libvlc_event_attach(eventManager, libvlc_MediaPlayerEncounteredError, (libvlc_callback_t)trapErrorEvent, NULL, &ex);

				int event_added = libvlc_event_attach( eventManager, libvlc_MediaListPlayerNextItemSet, callbacks, ctxPtr );
				if(event_added != 0) {
					printf("ERROR CANNOT ADD EVENT [libvlc_MediaListPlayerNextItemSet]\n");
				}
			}
		}
		//else {
			//libvlc_event_manager_t *eventManager = libvlc_media_player_event_manager(mp, &ex);
			libvlc_event_manager_t *eventManager = libvlc_media_player_event_manager(ctxPtr->mp);
			if(eventManager) {
				int event_added = libvlc_event_attach( eventManager, libvlc_MediaPlayerSnapshotTaken,   callbacks, ctxPtr );
				if(event_added != 0) {
					printf("ERROR CANNOT ADD EVENT [libvlc_MediaPlayerSnapshotTaken]\n");
				}
//				event_added = libvlc_event_attach( eventManager, libvlc_MediaPlayerTimeChanged,     callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_MediaPlayerTimeChanged]\n");
//				}

				event_added = libvlc_event_attach( eventManager, libvlc_MediaPlayerPlaying,         callbacks, ctxPtr );
				if(event_added != 0) {
					printf("ERROR CANNOT ADD EVENT [libvlc_MediaPlayerPlaying]\n");
				}

				event_added = libvlc_event_attach( eventManager, libvlc_MediaPlayerPaused,          callbacks, ctxPtr );
				if(event_added != 0) {
					printf("ERROR CANNOT ADD EVENT [libvlc_MediaPlayerPaused]\n");
				}

				event_added = libvlc_event_attach( eventManager, libvlc_MediaPlayerStopped,         callbacks, ctxPtr );
				if(event_added != 0) {
					printf("ERROR CANNOT ADD EVENT [libvlc_MediaPlayerStopped]\n");
				}

				event_added = libvlc_event_attach( eventManager, libvlc_MediaPlayerEndReached,      callbacks, ctxPtr );
				if(event_added != 0) {
					printf("ERROR CANNOT ADD EVENT [libvlc_MediaPlayerEndReached]\n");
				}

				event_added = libvlc_event_attach( eventManager, libvlc_MediaPlayerPositionChanged, callbacks, ctxPtr );
				if(event_added != 0) {
					printf("ERROR CANNOT ADD EVENT [libvlc_MediaPlayerPositionChanged]\n");
				}

				event_added = libvlc_event_attach( eventManager, libvlc_MediaPlayerLengthChanged,   callbacks, ctxPtr );
				if(event_added != 0) {
					printf("ERROR CANNOT ADD EVENT [libvlc_MediaPlayerLengthChanged]\n");
				}

				event_added = libvlc_event_attach( eventManager, libvlc_MediaPlayerEncounteredError,callbacks, ctxPtr );
				if(event_added != 0) {
					printf("ERROR CANNOT ADD EVENT [libvlc_MediaPlayerEncounteredError]\n");
				}

				event_added = libvlc_event_attach( eventManager, libvlc_MediaPlayerPausableChanged, callbacks, ctxPtr );
				if(event_added != 0) {
					printf("ERROR CANNOT ADD EVENT [libvlc_MediaPlayerPausableChanged]\n");
				}

				event_added = libvlc_event_attach( eventManager, libvlc_MediaPlayerSeekableChanged, callbacks, ctxPtr );
				if(event_added != 0) {
					printf("ERROR CANNOT ADD EVENT [libvlc_MediaPlayerSeekableChanged]\n");
				}

//				event_added = libvlc_event_attach( eventManager, libvlc_MediaStateChanged, callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_MediaStateChanged]\n");
//				}

//				event_added = libvlc_event_attach( eventManager, libvlc_MediaParsedChanged, callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_MediaParsedChanged]\n");
//				}

#if defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_0)
				event_added = libvlc_event_attach( eventManager, libvlc_MediaPlayerVout, callbacks, ctxPtr );
				if(event_added != 0) {
					printf("ERROR CANNOT ADD EVENT [libvlc_MediaPlayerVout]\n");
				}
#endif

//				event_added = libvlc_event_attach( eventManager, libvlc_MediaListItemAdded, callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_MediaListItemAdded]\n");
//				}
//
//				event_added = libvlc_event_attach( eventManager, libvlc_MediaListWillAddItem, callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_MediaListWillAddItem]\n");
//				}
//
//				event_added = libvlc_event_attach( eventManager, libvlc_MediaListItemDeleted, callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_MediaListItemDeleted]\n");
//				}
//
//				event_added = libvlc_event_attach( eventManager, libvlc_MediaListWillDeleteItem, callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_MediaListWillDeleteItem]\n");
//				}
//
//				event_added = libvlc_event_attach( eventManager, libvlc_MediaListViewItemAdded, callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_MediaListViewItemAdded]\n");
//				}
//
//				event_added = libvlc_event_attach( eventManager, libvlc_MediaListViewWillAddItem, callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_MediaListViewWillAddItem]\n");
//				}
//
//				event_added = libvlc_event_attach( eventManager, libvlc_MediaListViewItemDeleted, callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_MediaListViewItemDeleted]\n");
//				}
//
//				event_added = libvlc_event_attach( eventManager, libvlc_MediaListViewWillDeleteItem, callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_MediaListViewWillDeleteItem]\n");
//				}
//
//				event_added = libvlc_event_attach( eventManager, libvlc_MediaListPlayerPlayed, callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_MediaListPlayerPlayed]\n");
//				}
//
//				event_added = libvlc_event_attach( eventManager, libvlc_MediaListPlayerNextItemSet, callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_MediaListPlayerNextItemSet]\n");
//				}
//
//				event_added = libvlc_event_attach( eventManager, libvlc_MediaListPlayerStopped, callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_MediaListPlayerStopped]\n");
//				}
//
//				event_added = libvlc_event_attach( eventManager, libvlc_MediaDiscovererStarted, callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_MediaDiscovererStarted]\n");
//				}
//
//				event_added = libvlc_event_attach( eventManager, libvlc_MediaDiscovererEnded, callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_MediaDiscovererEnded]\n");
//				}
//
//				event_added = libvlc_event_attach( eventManager, libvlc_VlmMediaAdded, callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_VlmMediaAdded]\n");
//				}
//
//				event_added = libvlc_event_attach( eventManager, libvlc_VlmMediaRemoved, callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_VlmMediaRemoved]\n");
//				}
//
//				event_added = libvlc_event_attach( eventManager, libvlc_VlmMediaChanged, callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_VlmMediaChanged]\n");
//				}
//
//				event_added = libvlc_event_attach( eventManager, libvlc_VlmMediaInstanceStarted, callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_VlmMediaInstanceStarted]\n");
//				}
//
//				event_added = libvlc_event_attach( eventManager, libvlc_VlmMediaInstanceStopped, callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_VlmMediaInstanceStopped]\n");
//				}
//
//				event_added = libvlc_event_attach( eventManager, libvlc_VlmMediaInstanceStatusInit, callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_VlmMediaInstanceStatusInit]\n");
//				}
//
//				event_added = libvlc_event_attach( eventManager, libvlc_VlmMediaInstanceStatusOpening, callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_VlmMediaInstanceStatusOpening]\n");
//				}
//
//				event_added = libvlc_event_attach( eventManager, libvlc_VlmMediaInstanceStatusPlaying, callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_VlmMediaInstanceStatusPlaying]\n");
//				}
//
//				event_added = libvlc_event_attach( eventManager, libvlc_VlmMediaInstanceStatusPause, callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_VlmMediaInstanceStatusPause]\n");
//				}
//
//				event_added = libvlc_event_attach( eventManager, libvlc_VlmMediaInstanceStatusEnd, callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_VlmMediaInstanceStatusEnd]\n");
//				}
//
//				event_added = libvlc_event_attach( eventManager, libvlc_VlmMediaInstanceStatusError, callbacks, ctxPtr );
//				if(event_added != 0) {
//					printf("ERROR CANNOT ADD EVENT [libvlc_VlmMediaInstanceStatusError]\n");
//				}

			}
		//}

		//libvlc_media_release(ctxPtr->m);
#endif


#if !defined(LIBVLC_VERSION_PRE_2) && !defined(LIBVLC_VERSION_PRE_1_1_0)
		libvlc_video_set_callbacks(ctxPtr->mp, lock, unlock, display, ctxPtr);
		libvlc_video_set_format(ctxPtr->mp, "RV16", width, height, this->surface->pitch);

#endif

		ctxPtr->isPlaying = true;

#if defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_0)
		int play_result = libvlc_media_player_play(ctxPtr->mp,&ex);
#else
		int play_result = 0;
		if(mediaURL.find(HTTP_PREFIX) == 0) {
			libvlc_media_list_player_play(ctxPtr->mlp);
		}
		else {
			play_result = libvlc_media_player_play(ctxPtr->mp);
		}
		// Play
		//int play_result = 0;
		//libvlc_media_list_player_play(ctxPtr->mlp);
#endif

		//SDL_Delay(5);
		if(verboseEnabled) printf("In [%s] Line: %d, play_result [%d]\n",__FUNCTION__,__LINE__,play_result);

		successLoadingLib = (play_result == 0);

	}
#endif

	return successLoadingLib;
}

bool VideoPlayer::initPlayer() {
	if(VideoPlayer::disabled == true) {
		return true;
	}

#ifdef HAS_LIBVLC

	bool result = initPlayer(this->filename);
	if(result == true) {
		time_t waitStart = time(NULL);
		for(;difftime(time(NULL),waitStart) <= MAX_VIDEO_START_MILLISECONDS &&
			 successLoadingLib == true &&
			 (ctxPtr->error == false || ctxPtr->stopped == false) &&
			 ctxPtr->started == false;) {
			if(ctxPtr->started == true) {
				break;
			}
			if(this->loadingCB != NULL) {
				int progress = ((difftime(time(NULL),waitStart) / MAX_VIDEO_START_MILLISECONDS) * 100.0);
				this->loadingCB->renderVideoLoading(progress);
			}
			SDL_Delay(0);
		}
	}

	if(isPlaying() == false && this->filenameFallback != "") {
		closePlayer();
		cleanupPlayer();

		init();

		result = initPlayer(this->filenameFallback);
		if(result == true) {
			time_t waitStart = time(NULL);
			for(;difftime(time(NULL),waitStart) <= MAX_VIDEO_START_MILLISECONDS &&
				 successLoadingLib == true &&
				 (ctxPtr->error == false || ctxPtr->stopped == false) &&
				 ctxPtr->started == false;) {
				if(ctxPtr->started == true) {
					break;
				}
				if(this->loadingCB != NULL) {
					int progress = ((difftime(time(NULL),waitStart) / MAX_VIDEO_START_MILLISECONDS) * 100.0);
					this->loadingCB->renderVideoLoading(progress);
				}
				SDL_Delay(0);
			}
		}
	}

#endif

	return successLoadingLib;
}

void VideoPlayer::closePlayer() {
#ifdef HAS_LIBVLC
	if(ctxPtr != NULL && ctxPtr->libvlc != NULL) {
		//
		// Stop stream and clean up libVLC
		//
#if defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_0)
		libvlc_media_player_stop(ctxPtr->mp,&ex);
		catchError(&ex);
#else
		libvlc_media_player_stop(ctxPtr->mp);
#endif
		if(ctxPtr->mlp != NULL) {
			libvlc_media_list_player_release(ctxPtr->mlp);
			ctxPtr->mlp = NULL;
		}
		libvlc_media_player_release(ctxPtr->mp);
		ctxPtr->mp = NULL;
		libvlc_release(ctxPtr->libvlc);
	}
#endif

	//
	// Close window and clean up libSDL
	//
	if(ctxPtr != NULL) {
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
}

void VideoPlayer::PlayVideo() {
	if(VideoPlayer::disabled == true) {
		return;
	}

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

#if defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_0)
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

#if defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_0)
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
#if defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_0)
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

#if !defined(LIBVLC_VERSION_PRE_2) && !defined(LIBVLC_VERSION_PRE_1_1_0)
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
#if defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_0)
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
#if defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_0)
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
					//ctxPtr->error == false &&
					(ctxPtr->error == false || ctxPtr->stopped == false) &&
					finished == false && stop == false);

	if(ctxPtr != NULL && ctxPtr->verboseEnabled) printf("isPlaying isPlaying = %d,error = %d, stopped = %d, end_of_media = %d\n",ctxPtr->isPlaying,ctxPtr->error,ctxPtr->stopped,ctxPtr->end_of_media);
	return result;
}

bool VideoPlayer::playFrame(bool swapBuffers) {
	if(VideoPlayer::disabled == true) {
		return false;
	}

	int action = 0, pause = 0, n = 0;
	if(successLoadingLib == true &&
		ctxPtr != NULL && ctxPtr->isPlaying == true &&
		finished == false && stop == false) {
		action = 0;

		SDL_Event event;
		/* Keys: enter (fullscreen), space (pause), escape (quit) */
		//while( SDL_PollEvent( &event ) ) {
		if( SDL_PollEvent( &event ) ) {
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

			// Loading the texture
			loadTexture(ctxPtr);

			if(ctxPtr->x == -1 || ctxPtr->y == -1) {
				glBegin(GL_QUADS);
				 glTexCoord2d(0, 1);
				 glVertex2f(-0.85f, 0.85f);
				 glTexCoord2d(1, 1);
				 glVertex2f(0.85f, 0.85f);
				 glTexCoord2d(1, 0);
				 glVertex2f(0.85f, -0.85f);
				 glTexCoord2d(0, 0);
				 glVertex2f(-0.85f, -0.85f);
				glEnd();
			}
			else {
				const double HEIGHT_DEFAULT 	= 768;
				const double WIDTH_DEFAULT 	= 1024;

				const double HEIGHT_MULTIPLIER 	= HEIGHT_DEFAULT / ctxPtr->height;
				const double WIDTH_MULTIPLIER 		= WIDTH_DEFAULT / ctxPtr->width;

				//printf("w x h = %d x %d\n",ctxPtr->width,ctxPtr->height);

				glBegin(GL_TRIANGLE_STRIP);
					glTexCoord2i(0, 1);
					glVertex2i(ctxPtr->x, ctxPtr->y + ctxPtr->height * HEIGHT_MULTIPLIER);
					glTexCoord2i(0, 0);
					glVertex2i(ctxPtr->x, ctxPtr->y);
					glTexCoord2i(1, 1);
					glVertex2i(ctxPtr->x+ctxPtr->width * WIDTH_MULTIPLIER, ctxPtr->y+ctxPtr->height * HEIGHT_MULTIPLIER);
					glTexCoord2i(1, 0);
					glVertex2i(ctxPtr->x+ctxPtr->width * WIDTH_MULTIPLIER, ctxPtr->y);
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

void VideoPlayer::RestartVideo() {
	//printf("Restart video\n");

	//this->stop = false;
	//this->finished = false;
	//ctxPtr->started = true;
	//ctxPtr->error = false;
	//ctxPtr->stopped = false;
	//ctxPtr->end_of_media = false;
	//ctxPtr->isPlaying = true;
	//ctxPtr->needToQuit = false;

	//return;

	this->closePlayer();

	this->stop = false;
	this->finished = false;
	this->successLoadingLib = false;

	this->initPlayer();
}

}}
