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
#ifndef VIDEO_PLAYER_H_
#define VIDEO_PLAYER_H_

#include <string>

struct SDL_Surface;
class ctx;

using namespace std;

namespace Shared{ namespace Graphics{

class VideoLoadingCallbackInterface {
public:
	virtual ~VideoLoadingCallbackInterface() {}
	/** a value from 1 to 100 representing % done */
	virtual void renderVideoLoading(int progressPercent) = 0;
};

class VideoPlayer {
protected:

	string filename;
	string filenameFallback;
	SDL_Surface *surface;
	int x;
	int y;
	int width;
	int height;
	int colorBits;

	bool successLoadingLib;
	string pluginsPath;
	bool verboseEnabled;

	bool stop;
	bool finished;
	bool loop;

	VideoLoadingCallbackInterface *loadingCB;
	ctx *ctxPtr;

	static bool disabled;
	void init();

	void cleanupPlayer();
	bool initPlayer(string mediaURL);

public:
	VideoPlayer(VideoLoadingCallbackInterface *loadingCB,
				 string filename,
				 string filenameFallback,
				 SDL_Surface *surface, int x, int y,
				 int width, int height, int colorBits,
				 bool loop, string pluginsPath,bool verboseEnabled=false);
	virtual ~VideoPlayer();

	static void setDisabled(bool value) { disabled = value; }
	static bool getDisabled() { return disabled; }

	void PlayVideo();
	void StopVideo() { stop = true; }

	bool initPlayer();
	void closePlayer();

	bool playFrame(bool swapBuffers = true);
	bool isPlaying() const;

	static bool hasBackEndVideoPlayer();

	void RestartVideo();
};

}}

#endif /* VIDEO_PLAYER_H_ */
