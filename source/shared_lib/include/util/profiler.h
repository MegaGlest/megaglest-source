// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_UTIL_PROFILER_H_
#define _SHARED_UTIL_PROFILER_H_

//#define SL_PROFILE
//SL_PROFILE controls if profile is enabled or not

#include "platform_util.h"
#include <list>
#include <string>

using std::list;
using std::string;

using Shared::Platform::Chrono;

namespace Shared{ namespace Util{

#ifdef SL_PROFILE

// =====================================================
//	class Section
// =====================================================

class Section{
public:
	typedef list<Section*> SectionContainer;
private:
	string name;
	Chrono chrono;
	int64 milisElapsed;
	Section *parent;
	SectionContainer children;

public:
	Section(const string &name);

	Section *getParent()				{return parent;}
	const string &getName() const		{return name;}

	void setParent(Section *parent)	{this->parent= parent;}

	void start()	{chrono.start();}
	void stop()		{milisElapsed+=chrono.getMillis();}

	void addChild(Section *child)	{children.push_back(child);}
	Section *getChild(const string &name);

	void print(FILE *outSream, int tabLevel=0);
};

// =====================================================
//	class Profiler
// =====================================================

class Profiler{
private:
	Section *rootSection;
	Section *currSection;
private:
	Profiler();
public:
	~Profiler();
	static Profiler &getInstance();
	void sectionBegin(const string &name);
	void sectionEnd(const string &name);
};

#endif //SL_PROFILE

// =====================================================
//	class funtions
// =====================================================

inline void profileBegin(const string &sectionName){
#ifdef SL_PROFILE
	Profiler::getInstance().sectionBegin(sectionName);
#endif
}

inline void profileEnd(const string &sectionName){
#ifdef SL_PROFILE
	Profiler::getInstance().sectionEnd(sectionName);
#endif
}

}}//end namespace

#endif 
