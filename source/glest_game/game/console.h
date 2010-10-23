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

#ifndef _GLEST_GAME_CONSOLE_H_
#define _GLEST_GAME_CONSOLE_H_

#include <utility>
#include <string>
#include <vector>
#include <stdexcept>
#include "leak_dumper.h"

using std::string;
using std::vector;
using std::pair;
using namespace std;

namespace Glest { namespace Game {

// =====================================================
// 	class Console
//
//	In-game console that shows various types of messages
// =====================================================

class ConsoleLineInfo {
public:
	string text;
	float timeStamp;
	int PlayerIndex;
	string originalPlayerName;
};

class Console {
private:
	static const int consoleLines= 5;

public:

	typedef vector<ConsoleLineInfo> Lines;
	typedef Lines::const_iterator LineIterator;

private:
	float timeElapsed; 
	Lines lines;
	Lines storedLines;

	//config
	int maxLines;
	int maxStoredLines;
	float timeout;

public:
	Console();
	
	int getStoredLineCount() const		{return storedLines.size();}
	int getLineCount() const			{return lines.size();}
	string getLine(int i) const;
	string getStoredLine(int i) const;
	ConsoleLineInfo getLineItem(int i) const;
	ConsoleLineInfo getStoredLineItem(int i) const;

	void clearStoredLines();
	void addStdMessage(const string &s);
	void addLine(string line, bool playSound= false,int playerIndex=-1);
	void update();
	bool isEmpty();
};

}}//end namespace

#endif
