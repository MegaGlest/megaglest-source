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

#ifndef _SHARED_UTIL_LOGGER_H_
#define _SHARED_UTIL_LOGGER_H_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

#include <string>
#include <deque>

#include "texture.h"
#include "components.h"
#include "leak_dumper.h"

using std::string;
using std::deque;
using Shared::Graphics::Texture2D;

namespace Glest{ namespace Game{

// =====================================================
//	class Logger
//
/// Interface to write log files
// =====================================================

class Logger {
private:
	static const int logLineCount;

private:
	typedef deque<string> Strings;

private:
	string fileName;
	string state;
	string subtitle;
	string current;
	Texture2D *loadingTexture;
	int progress;
	bool showProgressBar;

	string statusText;
	bool cancelSelected;
	GraphicButton buttonCancel;

	//bool masterserverMode;

private:
	Logger();
	~Logger();

public:
	static Logger & getInstance();

	//void setMasterserverMode(bool value) { masterserverMode = value; }

	void setFile(const string &fileName)		{this->fileName= fileName;}
	void setState(const string &state)			{this->state= state;}
	void setSubtitle(const string &subtitle)	{this->subtitle= subtitle;}
	void setProgress(int value)                 { this->progress = value; }
    int getProgress() const                     {return progress;}
    void showProgress() { showProgressBar = true;}
    void hideProgress() { showProgressBar = false;}

	void add(const string str, bool renderScreen= false, const string statusText="");
	void loadLoadingScreen(string filepath);
	void renderLoadingScreen();

	void setCancelLoadingEnabled(bool value);
	bool getCancelLoading() const { return cancelSelected; }
	void setCancelLoading(bool value) { cancelSelected = value; }
	void handleMouseClick(int x, int y);

	void clear();

};

}}//end namespace

#endif
