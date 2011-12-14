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

#include "logger.h"

#include "util.h"
#include "renderer.h"
#include "core_data.h"
#include "metrics.h"
#include "lang.h"
#include "graphics_interface.h"
#include "game_constants.h"
#include "game_util.h"
#include "platform_util.h"
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
	//masterserverMode = false;
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

	cancelSelected = false;
	buttonCancel.setEnabled(false);
}

Logger::~Logger() {
}

Logger & Logger::getInstance() {
	static Logger logger;
	return logger;
}

void Logger::add(const string str,  bool renderScreen, const string statusText) {
#ifdef WIN32
	FILE *f= _wfopen(utf8_decode(fileName).c_str(), L"at+");
#else
	FILE *f = fopen(fileName.c_str(), "at+");
#endif
	if(f != NULL){
		fprintf(f, "%s\n", str.c_str());
		fclose(f);
	}
	this->current= str;
	this->statusText = statusText;

	if(renderScreen == true && GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false) {
		renderLoadingScreen();
	}
}

void Logger::clear() {
    string s = "Log file\n";

#ifdef WIN32
	FILE *f= _wfopen(utf8_decode(fileName).c_str(), L"wt+");
#else
	FILE *f= fopen(fileName.c_str(), "wt+");
#endif
	if(f == NULL){
		throw runtime_error("Error opening log file" + fileName);
	}

    fprintf(f, "%s", s.c_str());
	fprintf(f, "\n");

    fclose(f);
}

void Logger::loadLoadingScreen(string filepath) {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(filepath == "") {
		loadingTexture = NULL;
	}
	else {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] filepath = [%s]\n",__FILE__,__FUNCTION__,__LINE__,filepath.c_str());
		loadingTexture = Renderer::findFactionLogoTexture(filepath);
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
}

void Logger::handleMouseClick(int x, int y) {
	if(buttonCancel.getEnabled() == true) {
		if(buttonCancel.mouseClick(x, y)) {
			cancelSelected = true;
		}
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
	else {
		renderer.renderBackground(loadingTexture);
	}

    if(showProgressBar == true) {
    	if(Renderer::renderText3DEnabled) {
            renderer.renderProgressBar3D(
                progress,
                metrics.getVirtualW() / 4,
                59 * metrics.getVirtualH() / 100,
                coreData.getDisplayFontSmall3D(),
                350,""); // no string here, because it has to be language specific and does not give much information
    	}
    	else {
			renderer.renderProgressBar(
				progress,
				metrics.getVirtualW() / 4,
				59 * metrics.getVirtualH() / 100,
				coreData.getDisplayFontSmall(),
				350,""); // no string here, because it has to be language specific and does not give much information
    	}
    }

    int xLocation = metrics.getVirtualW() / 4;
	if(Renderer::renderText3DEnabled) {

		renderer.renderText3D(
			state, coreData.getMenuFontBig3D(), Vec3f(1.f),
			xLocation,
			65 * metrics.getVirtualH() / 100, false);

		renderer.renderText3D(
			current, coreData.getMenuFontNormal3D(), Vec3f(1.f),
			xLocation,
			62 * metrics.getVirtualH() / 100, false);

	    if(this->statusText != "") {
	    	renderer.renderText3D(
	    		this->statusText, coreData.getMenuFontNormal3D(), 1.0f,
	    		xLocation,
	    		56 * metrics.getVirtualH() / 100, false);
	    }
	}
	else {
		renderer.renderText(
			state, coreData.getMenuFontBig(), Vec3f(1.f),
			xLocation,
			65 * metrics.getVirtualH() / 100, false);

		renderer.renderText(
			current, coreData.getMenuFontNormal(), 1.0f,
			xLocation,
			62 * metrics.getVirtualH() / 100, false);

		if(this->statusText != "") {
			renderer.renderText(
				this->statusText, coreData.getMenuFontNormal(), 1.0f,
				xLocation,
				56 * metrics.getVirtualH() / 100, false);
		}
	}

    if(buttonCancel.getEnabled() == true) {
    	renderer.renderButton(&buttonCancel);
    }

	renderer.swapBuffers();
}

void Logger::setCancelLoadingEnabled(bool value) {
	Lang &lang= Lang::getInstance();
	const Metrics &metrics= Metrics::getInstance();
	string containerName = "logger";
	//buttonCancel.registerGraphicComponent(containerName,"buttonCancel");
	buttonCancel.init((metrics.getVirtualW() / 2) - (125 / 2), 50 * metrics.getVirtualH() / 100, 125);
	buttonCancel.setText(lang.get("Cancel"));
	buttonCancel.setEnabled(value);
	//GraphicComponent::applyAllCustomProperties(containerName);
}

}}//end namespace
