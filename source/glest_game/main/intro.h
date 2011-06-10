// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_INTRO_H_
#define _GLEST_GAME_INTRO_H_

#include <vector>

#include "program.h"
#include "font.h"
#include "vec.h"
#include "texture.h"
#include "leak_dumper.h"

using std::vector;

using Shared::Graphics::Vec2i;
using Shared::Graphics::Vec2f;
using Shared::Graphics::Vec3f;
using Shared::Graphics::Font2D;
using Shared::Graphics::Font3D;
using Shared::Graphics::Texture2D;

namespace Glest{ namespace Game{

// =====================================================
//	class Text
// =====================================================

class Text {
private:
	string text;
	Vec2i pos;
    Vec2i size;
	int time;
	Font2D *font;
	Font3D *font3D;
	const Texture2D *texture;

public:
	Text(const string &text, const Vec2i &pos, int time, Font2D *font, Font3D *font3D);
	Text(const Texture2D *texture, const Vec2i &pos, const Vec2i &size, int time);

	const string &getText() const		{return text;}
	Font2D *getFont() 					{return font;}
	Font3D *getFont3D() 				{return font3D;}
	const Vec2i &getPos() const			{return pos;}
	const Vec2i &getSize() const		{return size;}
	int getTime() const					{return time;}
	const Texture2D *getTexture() const	{return texture;}
};

// =====================================================
// 	class Intro  
//
///	ProgramState representing the intro
// =====================================================

class Intro: public ProgramState{
private:
	static const int introTime;
	static const int appearTime;
	static const int showTime;
	static const int disapearTime;

private:
	vector<Text> texts;
	int timer;
	int mouseX;
	int mouseY;
	int mouse2d;

public:
	Intro(Program *program);
	virtual void update();
	virtual void render();
	virtual void keyDown(char key);
	virtual void mouseUpLeft(int x, int y);
	void mouseMove(int x, int y, const MouseState *ms);
};

}}//end namespace

#endif
