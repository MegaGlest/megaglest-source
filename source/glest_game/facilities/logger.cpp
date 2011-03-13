// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "logger.h"

#include "util.h"
#include "renderer.h"
#include "core_data.h"
#include "metrics.h"
#include "lang.h"
#include "graphics_interface.h"
#include "game_constants.h"
#include "game_util.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
//	class Logger
// =====================================================

const int Logger::logLineCount= 15;

// ===================== PUBLIC ========================

Logger::Logger() {
	string logs_path = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey);
	if(logs_path != "") {
		fileName= logs_path + "log.txt";
	}
	else {
        string userData = Config::getInstance().getString("UserData_Root","");
        if(userData != "") {
        	endPathWithSlash(userData);
        }
        fileName= userData + "log.txt";
	}
	loadingTexture=NULL;
	showProgressBar = false;
}

Logger::~Logger() {
}

Logger & Logger::getInstance() {
	static Logger logger;
	return logger;
}

void Logger::add(const string &str,  bool renderScreen) {
	FILE *f=fopen(fileName.c_str(), "at+");
	if(f!=NULL){
		fprintf(f, "%s\n", str.c_str());
		fclose(f);
	}
	current= str;
	if(renderScreen){
		renderLoadingScreen();
	}
}

void Logger::clear() {
    string s="Log file\n";

	FILE *f= fopen(fileName.c_str(), "wt+");
	if(f==NULL){
		throw runtime_error("Error opening log file"+ fileName);
	}

    fprintf(f, "%s", s.c_str());
	fprintf(f, "\n");

    fclose(f);
}

void Logger::loadLoadingScreen(string filepath) {

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(filepath=="")
	{
		loadingTexture=NULL;
	}
	else
	{
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] filepath = [%s]\n",__FILE__,__FUNCTION__,__LINE__,filepath.c_str());
		loadingTexture = Renderer::findFactionLogoTexture(filepath);
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
}

// ==================== PRIVATE ====================

void Logger::renderLoadingScreen() {

	Renderer &renderer= Renderer::getInstance();
	CoreData &coreData= CoreData::getInstance();
	const Metrics &metrics= Metrics::getInstance();

	renderer.reset2d();
	renderer.clearBuffers();
	if(loadingTexture == NULL) {
		renderer.renderBackground(CoreData::getInstance().getBackgroundTexture());
	}
	else{
		renderer.renderBackground(loadingTexture);
	}
	renderer.renderText(
		state, coreData.getMenuFontBig(), Vec3f(1.f),
		metrics.getVirtualW()/4, 65*metrics.getVirtualH()/100, false);

	renderer.renderText(
		current, coreData.getMenuFontNormal(), 1.0f,
		metrics.getVirtualW()/4,
		62*metrics.getVirtualH()/100, false);

    if(showProgressBar == true) {
        renderer.renderProgressBar(
            progress,
            metrics.getVirtualW()/4,
            59*metrics.getVirtualH()/100,
            coreData.getDisplayFontSmall(),
            350,""); // no string here, because it has to be language specific and does not give much information
    }

	renderer.swapBuffers();
}

}}//end namespace
