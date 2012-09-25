// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_BATTLEEND_H_
#define _GLEST_GAME_BATTLEEND_H_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

#include "program.h"
#include "stats.h"
#include "leak_dumper.h"

namespace Shared { namespace Graphics {
	class VideoPlayer;
}}

namespace Glest{ namespace Game{

class GameSettings;
// =====================================================
// 	class BattleEnd  
//
///	ProgramState representing the end of the game
// =====================================================

class BattleEnd: public ProgramState{
private:
	Stats stats;

	GraphicButton buttonExit;
	int mouseX;
	int mouseY;
	int mouse2d;
	GraphicMessageBox mainMessageBox;
	Texture2D *renderToTexture;
	ProgramState *originState;
	const char *containerName;

	Shared::Graphics::VideoPlayer *menuBackgroundVideo;
	GameSettings *gameSettings;
	StrSound battleEndMusic;

	void showMessageBox(const string &text, const string &header, bool toggle);

public:
	BattleEnd(Program *program, const Stats *stats, ProgramState *originState);
	~BattleEnd();

	virtual void update();
	virtual void render();
	virtual void keyDown(SDL_KeyboardEvent key);
	virtual void mouseDownLeft(int x, int y);
	virtual void mouseMove(int x, int y, const MouseState *ms);
	//virtual void tick();
	virtual void reloadUI();

private:
	const string getTimeString(int frames);

	void initBackgroundVideo();
	std::pair<string,string> getBattleEndVideo(bool won);
	string getBattleEndMusic(bool won);
	void initBackgroundMusic();
};

}}//end namespace

#endif
