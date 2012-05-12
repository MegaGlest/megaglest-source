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

#include <SDL/SDL.h>
#include <SDL/SDL_mutex.h>

#ifdef HAS_LIBVLC
#include <vlc/vlc.h>
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

VideoPlayer::VideoPlayer(string filename, SDL_Surface *surface,
		int width, int height,int colorBits) {
	this->filename = filename;
	this->surface = surface;
	this->width = width;
	this->height = height;
	this->colorBits = colorBits;
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

#ifdef HAS_LIBVLC
	libvlc_instance_t *libvlc = NULL;
	libvlc_media_t *m = NULL;
	libvlc_media_player_t *mp = NULL;
	char const *vlc_argv[] =
	{
		//"--no-audio", /* skip any audio track */
		"--no-xlib", /* tell VLC to not use Xlib */
		"--no-video-title-show",
	};
	int vlc_argc = sizeof(vlc_argv) / sizeof(*vlc_argv);
#endif

	SDL_Surface *empty = NULL;
	SDL_Event event;

	int done = 0, action = 0, pause = 0, n = 0;

	struct ctx ctx;
	ctx.width = width;
	ctx.height = height;
	ctx.rawData = (void *) malloc(width * height * 4);

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

#ifdef HAS_LIBVLC
	/*
	 *  Initialise libVLC
	 */
	libvlc = libvlc_new(vlc_argc, vlc_argv);
	m = libvlc_media_new_path(libvlc, filename.c_str());
	mp = libvlc_media_player_new_from_media(m);
	libvlc_media_release(m);

	libvlc_video_set_callbacks(mp, lock, unlock, display, &ctx);
	libvlc_video_set_format(mp, "RV16", width, height, this->surface->pitch);
	libvlc_media_player_play(mp);
#endif

	/*
	 *  Main loop
	 */

	bool needToQuit = false;
	while(!done && stop == false) {
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
	/*
	 * Stop stream and clean up libVLC
	 */
	libvlc_media_player_stop(mp);
	libvlc_media_player_release(mp);
	libvlc_release(libvlc);
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
