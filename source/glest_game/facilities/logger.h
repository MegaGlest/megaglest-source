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

#include <string>
#include <deque>

using std::string;
using std::deque;

namespace Glest{ namespace Game{

// =====================================================
//	class Logger  
//
/// Interface to write log files
// =====================================================

class Logger{
private:
	static const int logLineCount;

private:
	typedef deque<string> Strings;

private:
	string fileName;
	string state;
	string subtitle;
	string current;

private:
	Logger();

public:
	static Logger & getInstance();
	
	void setFile(const string &fileName)		{this->fileName= fileName;}
	void setState(const string &state)			{this->state= state;}
	void setSubtitle(const string &subtitle)	{this->subtitle= subtitle;}

	void add(const string &str, bool renderScreen= false);
	void renderLoadingScreen();

	void clear();
};

}}//end namespace

#endif
