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

struct ctx {
	GLuint textureId; // Texture ID
    SDL_Surface *surf;
    SDL_mutex *mutex;
	int width;
	int height;
	void *rawData;
};

// Load a texture
static void loadTexture(struct ctx *ctx) {
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

    SDL_LockMutex(ctx->mutex);
    SDL_LockSurface(ctx->surf);
    *p_pixels = ctx->surf->pixels;

    return NULL; /* picture identifier, not needed here */
}

static void unlock(void *data, void *id, void *const *p_pixels) {
    struct ctx *ctx = (struct ctx *)data;

    /* VLC just rendered the video, but we can also render stuff */
    SDL_UnlockSurface(ctx->surf);
    SDL_UnlockMutex(ctx->mutex);
}

static void display(void *data, void *id) {
    /* VLC wants to display the video */
    (void) data;
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

VideoPlayer::VideoPlayer(string filename, SDL_Surface *surface,
		int width, int height,int colorBits,string pluginsPath,
		bool verboseEnabled) {
	this->filename = filename;
	this->surface = surface;
	this->width = width;
	this->height = height;
	this->colorBits = colorBits;
	this->pluginsPath = pluginsPath;
	this->verboseEnabled = verboseEnabled;

	this->stop = false;
}

VideoPlayer::~VideoPlayer() {

}

bool VideoPlayer::hasBackEndVideoPlayer() {
#ifdef HAS_LIBVLC
	return true;
#endif
	return false;
}
void VideoPlayer::PlayVideo() {
	struct ctx ctx;
	ctx.width = width;
	ctx.height = height;
	ctx.rawData = (void *) malloc(width * height * 4);

#ifdef HAS_LIBVLC
	libvlc_instance_t *libvlc = NULL;
	libvlc_media_t *m = NULL;
	libvlc_media_player_t *mp = NULL;

//	string pluginParam = "";
//	if(pluginsPath != "") {
//		pluginParam = "--plugin-path=" + pluginsPath;
//	}

	std::vector<const char *> vlc_argv;
	vlc_argv.push_back("--no-xlib" /* tell VLC to not use Xlib */);
	vlc_argv.push_back("--no-video-title-show");
#if defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_13)
	char clock[64], cunlock[64], cdata[64];
    	char cwidth[32], cheight[32], cpitch[32];
	/*
         *  Initialise libVLC
	 */
	sprintf(clock, "%lld", (long long int)(intptr_t)lock);
	sprintf(cunlock, "%lld", (long long int)(intptr_t)unlock);
	sprintf(cdata, "%lld", (long long int)(intptr_t)&ctx);
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
#if defined(WIN32)
	if(verboseEnabled) _putenv("VLC_VERBOSE=2");
#endif
	int vlc_argc = vlc_argv.size();

//	char const *vlc_argv[] =
//	{
//		//"--no-audio", /* skip any audio track */
//		"--no-xlib", /* tell VLC to not use Xlib */
//		"--no-video-title-show",
//		pluginParam.c_str(),
//	};
//	int vlc_argc = sizeof(vlc_argv) / sizeof(*vlc_argv);
#endif

	SDL_Surface *empty = NULL;
	SDL_Event event;

	int done = 0, action = 0, pause = 0, n = 0;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);

	// Init Texture
	glGenTextures(1, &ctx.textureId);
	glBindTexture(GL_TEXTURE_2D, ctx.textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	empty = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height,
			colorBits, 0, 0, 0, 0);
	ctx.surf = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height,
			colorBits, 0x001f, 0x07e0, 0xf800, 0);
	ctx.mutex = SDL_CreateMutex();

	bool successLoadingVLC = false;
#ifdef HAS_LIBVLC
	/*
	 *  Initialize libVLC
	 */
	if(verboseEnabled) printf("Trying [%s]\n",getenv("VLC_PLUGIN_PATH"));

#if defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_13)
	libvlc_exception_t ex;
	libvlc_exception_init(&ex);

	libvlc = libvlc_new(vlc_argc, &vlc_argv[0],&ex);
	catchError(&ex);
#else
	libvlc = libvlc_new(vlc_argc, &vlc_argv[0]);
#endif
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

	if(libvlc != NULL) {
#if defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_13)
		m = libvlc_media_new(libvlc, filename.c_str(), &ex);
		catchError(&ex);

#else
		mp = libvlc_media_player_new_from_media(m);
#endif
		libvlc_media_release(m);

#if !defined(LIBVLC_VERSION_PRE_2) && !defined(LIBVLC_VERSION_PRE_1_1_13)
		libvlc_video_set_callbacks(mp, lock, unlock, display, &ctx);
		libvlc_video_set_format(mp, "RV16", width, height, this->surface->pitch);
#endif
		
#if defined(LIBVLC_VERSION_PRE_2) && defined(LIBVLC_VERSION_PRE_1_1_13)
		libvlc_media_player_play(mp,&ex);
#else
		libvlc_media_player_play(mp);
#endif

		successLoadingVLC = true;
	}
#endif

	/*
	 *  Main loop
	 */

	bool needToQuit = false;
	while(successLoadingVLC == true && !done && stop == false) {
		action = 0;

		/* Keys: enter (fullscreen), space (pause), escape (quit) */
		while( SDL_PollEvent( &event ) ) {
			switch(event.type) {
			case SDL_QUIT:
				done = 1;
				needToQuit = true;
				break;
			case SDL_KEYDOWN:
				action = event.key.keysym.sym;
				break;
			case SDL_MOUSEBUTTONDOWN:
				done = 1;
				break;
			}
		}

		if(!done && stop == false) {
			switch(action) {
			case SDLK_ESCAPE:
				done = 1;
				break;
			case SDLK_RETURN:
				//options ^= SDL_FULLSCREEN;
				//screen = SDL_SetVideoMode(WIDTH, HEIGHT, 0, options);
				done = 1;
				break;
			case ' ':
				//pause = !pause;
				break;
			}

			if(!pause) {
				n++;
			}

			loadTexture(&ctx); // Loading the texture

			// Square
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

			SDL_GL_SwapBuffers();
		}
	}

#ifdef HAS_LIBVLC
	if(libvlc != NULL) {
		/*
		 * Stop stream and clean up libVLC
		 */
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

	/*
	 * Close window and clean up libSDL
	 */
	SDL_DestroyMutex(ctx.mutex);
	SDL_FreeSurface(ctx.surf);
	SDL_FreeSurface(empty);

	glDeleteTextures(1, &ctx.textureId);
	free(ctx.rawData);

	if(needToQuit == true) {
		SDL_Event quit_event = {SDL_QUIT};
		SDL_PushEvent(&quit_event);
	}
}
