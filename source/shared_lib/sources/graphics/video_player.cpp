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

#include <glad/gl.h>
#include "video_player.h"
#include <SDL.h>
#include <SDL_mutex.h>
#include <vector>
#include <time.h>

#if defined(WIN32)
#include <windows.h>

/**
 * @param location The location of the registry key. For example
 * "Software\\Bethesda Softworks\\Morrowind"
 * @param name the name of the registry key, for example "Installed Path"
 * @return the value of the key or an empty string if an error occured.
 */
std::string getRegKey(const std::string &location, const std::string &name) {
    HKEY key;
    CHAR value[1024];
    DWORD bufLen = 1024 * sizeof(CHAR);
    long ret;
    ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE, location.c_str(), 0, KEY_QUERY_VALUE, &key);
    if (ret != ERROR_SUCCESS) {
        return std::string();
    }
    ret = RegQueryValueExA(key, name.c_str(), 0, 0, (LPBYTE)value, &bufLen);
    RegCloseKey(key);
    if ((ret != ERROR_SUCCESS) || (bufLen > 1024 * sizeof(TCHAR))) {
        return std::string();
    }
    string stringValue = value;
    size_t i = stringValue.length();
    while (i > 0 && stringValue[i - 1] == '\0') {
        --i;
    }
    return stringValue.substr(0, i);
}
#endif

const string HTTP_PREFIX = "http";

class ctx {
  public:
    ctx() {
        loadingCB = NULL;
        empty = NULL;
        textureId = 0; // Texture ID
        surf = NULL;
        mutex = NULL;
        x = 0;
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
};

namespace Shared {
namespace Graphics {

bool VideoPlayer::disabled = false;

// Load a texture
static inline void loadTexture(class ctx *ctx) {
    if (ctx->verboseEnabled) printf("In [%s] Line: %d\n", __FUNCTION__, __LINE__);

    void *rawData = ctx->rawData;
    Uint8 *pixelSource = 0;
    Uint8 *pixelDestination = (Uint8 *)rawData;
    Uint32 pix = 0;

    for (int i = ctx->height; i > 0; i--) {
        for (int j = 0; j < ctx->width; j++) {
            pixelSource = (Uint8 *)ctx->surf->pixels + (i - 1) * ctx->surf->pitch + j * 2;
            pix = *(Uint16 *)pixelSource;
            SDL_GetRGBA(pix, ctx->surf->format, &(pixelDestination[0]), &(pixelDestination[1]), &(pixelDestination[2]), &(pixelDestination[3]));
            pixelDestination += 4;
        }
    }

    // Building the texture
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindTexture(GL_TEXTURE_2D, ctx->textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, ctx->width, ctx->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (Uint8 *)rawData);
}

VideoPlayer::VideoPlayer(VideoLoadingCallbackInterface *loadingCB, string filename, string filenameFallback, SDL_Window *window, int x, int y, int width,
                         int height, int colorBits, bool loop, bool verboseEnabled)
    : ctxPtr(NULL) {
    this->loadingCB = loadingCB;
    this->filename = filename;
    this->filenameFallback = filenameFallback;
    this->window = window;
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;
    this->colorBits = colorBits;
    this->loop = loop;
    // this->verboseEnabled = true;
    this->verboseEnabled = verboseEnabled;
    this->stop = false;
    this->finished = false;
    this->successLoadingLib = false;

    init();
}

void VideoPlayer::init() {
    if (VideoPlayer::disabled == true) {
        return;
    }
    cleanupPlayer();
    ctxPtr = new ctx();
    ctxPtr->loadingCB = loadingCB;
    ctxPtr->x = x;
    ctxPtr->y = y;
    ctxPtr->width = width;
    ctxPtr->height = height;
    ctxPtr->rawData = (void *)malloc(width * height * 4);
    ctxPtr->verboseEnabled = verboseEnabled;
}

VideoPlayer::~VideoPlayer() {
    cleanupPlayer();
}

bool VideoPlayer::hasBackEndVideoPlayer() {
    if (VideoPlayer::disabled == true) {
        return false;
    }

    return false;
}

void VideoPlayer::cleanupPlayer() {
    if (ctxPtr != NULL) {
        if (ctxPtr->rawData != NULL) {
            free(ctxPtr->rawData);
            ctxPtr->rawData = NULL;
        }

        delete ctxPtr;
        ctxPtr = NULL;
    }
}

bool VideoPlayer::initPlayer(string mediaURL) {
    if (VideoPlayer::disabled == true) {
        return true;
    }

    ctxPtr->empty = NULL;
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    // Init Texture
    glGenTextures(1, &ctxPtr->textureId);
    glBindTexture(GL_TEXTURE_2D, ctxPtr->textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    ctxPtr->empty = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, colorBits, 0, 0, 0, 0);
    ctxPtr->surf = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, colorBits, 0x001f, 0x07e0, 0xf800, 0);
    ctxPtr->mutex = SDL_CreateMutex();

    return successLoadingLib;
}

bool VideoPlayer::initPlayer() {
    if (VideoPlayer::disabled == true) {
        return true;
    }

    return successLoadingLib;
}

void VideoPlayer::closePlayer() {
    //
    // Close window and clean up libSDL
    //
    if (ctxPtr != NULL) {
        if (ctxPtr->mutex != NULL) {
            SDL_DestroyMutex(ctxPtr->mutex);
        }
        if (ctxPtr->surf != NULL) {
            SDL_FreeSurface(ctxPtr->surf);
        }
        if (ctxPtr->empty != NULL) {
            SDL_FreeSurface(ctxPtr->empty);
        }

        glDeleteTextures(1, &ctxPtr->textureId);

        if (ctxPtr->needToQuit == true) {
            SDL_Event quit_event = {SDL_QUIT};
            SDL_PushEvent(&quit_event);
        }
    }
}

void VideoPlayer::PlayVideo() {
    if (VideoPlayer::disabled == true) {
        return;
    }

    initPlayer();
    for (; isPlaying() == true;) {
        playFrame();
    }
    closePlayer();
}

bool VideoPlayer::isPlaying() const {
    bool result = (successLoadingLib == true && ctxPtr != NULL && ctxPtr->isPlaying == true &&
                   // ctxPtr->error == false &&
                   (ctxPtr->error == false || ctxPtr->stopped == false) && finished == false && stop == false);

    if (ctxPtr != NULL && ctxPtr->verboseEnabled)
        printf("isPlaying isPlaying = %d,error = %d, stopped = %d, end_of_media = "
               "%d\n",
               ctxPtr->isPlaying, ctxPtr->error, ctxPtr->stopped, ctxPtr->end_of_media);
    return result;
}

bool VideoPlayer::playFrame(bool swapBuffers) {
    if (VideoPlayer::disabled == true) {
        return false;
    }

    if (successLoadingLib == true && ctxPtr != NULL && ctxPtr->isPlaying == true && finished == false && stop == false) {
        // int action = 0, pause = 0, n = 0;
        // int action = 0, n = 0;
        int action = 0;

        SDL_Event event;
        /* Keys: enter (fullscreen), space (pause), escape (quit) */
        // while( SDL_PollEvent( &event ) ) {
        if (SDL_PollEvent(&event)) {
            switch (event.type) {
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

        if (finished == false && stop == false) {
            switch (action) {
            case SDLK_ESCAPE:
                finished = true;
                break;
            case SDLK_RETURN:
                // options ^= SDL_WINDOW_FULLSCREEN;
                // screen = SDL_SetVideoMode(WIDTH, HEIGHT, 0, options);
                finished = true;
                break;
                // case ' ':
                //	//pause = !pause;
                //	break;
            }

            // if(pause == 0) {
            // n++;
            // }

            // assertGl();

            glPushAttrib(GL_ENABLE_BIT);

            // glDisable(GL_LIGHTING);
            glEnable(GL_TEXTURE_2D);
            glEnable(GL_BLEND);

            // Loading the texture
            loadTexture(ctxPtr);

            if (ctxPtr->x == -1 || ctxPtr->y == -1) {
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
            } else {
                const double HEIGHT_DEFAULT = 768;
                const double WIDTH_DEFAULT = 1024;

                const double HEIGHT_MULTIPLIER = HEIGHT_DEFAULT / (double)ctxPtr->height;
                const double WIDTH_MULTIPLIER = WIDTH_DEFAULT / (double)ctxPtr->width;

                // printf("w x h = %d x %d\n",ctxPtr->width,ctxPtr->height);

                glBegin(GL_TRIANGLE_STRIP);
                glTexCoord2i(0, 1);
                glVertex2i(ctxPtr->x, (int)((double)ctxPtr->y + (double)ctxPtr->height * HEIGHT_MULTIPLIER));
                glTexCoord2i(0, 0);
                glVertex2i(ctxPtr->x, ctxPtr->y);
                glTexCoord2i(1, 1);
                glVertex2i((int)((double)ctxPtr->x + (double)ctxPtr->width * WIDTH_MULTIPLIER),
                           (int)((double)ctxPtr->y + (double)ctxPtr->height * HEIGHT_MULTIPLIER));
                glTexCoord2i(1, 0);
                glVertex2i((int)((double)ctxPtr->x + (double)ctxPtr->width * WIDTH_MULTIPLIER), ctxPtr->y);
                glEnd();
            }

            glPopAttrib();

            if (swapBuffers == true) {
                SDL_GL_SwapWindow(window);
            }
        }
    }

    if (ctxPtr != NULL) {
        return ctxPtr->needToQuit;
    } else {
        return false;
    }
}

void VideoPlayer::RestartVideo() {
    // printf("Restart video\n");

    // this->stop = false;
    // this->finished = false;
    // ctxPtr->started = true;
    // ctxPtr->error = false;
    // ctxPtr->stopped = false;
    // ctxPtr->end_of_media = false;
    // ctxPtr->isPlaying = true;
    // ctxPtr->needToQuit = false;

    // return;

    this->closePlayer();

    this->stop = false;
    this->finished = false;
    this->successLoadingLib = false;

    this->initPlayer();
}

} // namespace Graphics
} // namespace Shared
