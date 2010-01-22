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

#include "profiler.h"

#ifdef SL_PROFILE

#include <stdexcept>

using namespace std;

namespace Shared{ namespace Util{

// =====================================================
//	class Section
// =====================================================

Section::Section(const string &name){
	this->name= name;
	milisElapsed= 0;
	parent= NULL;
}

Section *Section::getChild(const string &name){
	SectionContainer::iterator it;
	for(it= children.begin(); it!=children.end(); ++it){
		if((*it)->getName()==name){
			return *it;
		}
	}

	return NULL;
}

void Section::print(FILE *outStream, int tabLevel){

	float percent= (parent==NULL || parent->milisElapsed==0)? 100.0f: 100.0f*milisElapsed/parent->milisElapsed;
	string pname= parent==NULL? "": parent->getName();
	
	for(int i=0; i<tabLevel; ++i)
		fprintf(outStream, "\t");
	
	fprintf(outStream, "%s: ", name.c_str()); 
	fprintf(outStream, "%d ms, ", milisElapsed); 
	fprintf(outStream, "%.1f%s\n", percent, "%"); 

	SectionContainer::iterator it;
	for(it= children.begin(); it!=children.end(); ++it){
		(*it)->print(outStream, tabLevel+1);
	}
}

// =====================================================
//	class Profiler
// =====================================================

Profiler::Profiler(){
	rootSection= new Section("Root");
	currSection= rootSection;
	rootSection->start();
}

Profiler::~Profiler(){
	rootSection->stop();

	FILE *f= fopen("profiler.log", "w");
	if(f==NULL)
		throw runtime_error("Can not open file: profiler.log");

	fprintf(f, "Profiler Results\n\n");

	rootSection->print(f);

	fclose(f);
}

Profiler &Profiler::getInstance(){
	static Profiler profiler;
	return profiler;
}

void Profiler::sectionBegin(const string &name){
	Section *childSection= currSection->getChild(name);
	if(childSection==NULL){
		childSection= new Section(name);
		currSection->addChild(childSection);
		childSection->setParent(currSection);
	}
	currSection= childSection;
	childSection->start();
}

void Profiler::sectionEnd(const string &name){
	if(name==currSection->getName()){
		currSection->stop();
		currSection= currSection->getParent();
	}
	else{
		throw runtime_error("Profile: Leaving section is not current section: "+name);
	}
}

}};//end namespace

#endif
