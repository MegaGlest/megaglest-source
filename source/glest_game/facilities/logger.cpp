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
#include "properties.h"
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
	gameHintToShow="";
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
		throw megaglest_runtime_error("Error opening log file" + fileName);
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

void Logger::loadGameHints(string filePathEnglish,string filePathTranslation,bool clearList) {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if((filePathEnglish == "") || (filePathTranslation == "")) {
		return;
	}
	else {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] filePathEnglish = [%s]\n filePathTranslation = [%s]\n",__FILE__,__FUNCTION__,__LINE__,filePathEnglish.c_str(),filePathTranslation.c_str());
		gameHints.load(filePathEnglish,clearList);
		gameHintsTranslation.load(filePathTranslation,clearList);
		string key=gameHints.getRandomKey(true);
		string tmpString=gameHintsTranslation.getString(key,"");
		if(tmpString!=""){
			gameHintToShow=tmpString;
		}
		else {
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] key [%s] not found for [%s] hint translation\n",__FILE__,__FUNCTION__,__LINE__,key.c_str(),Lang::getInstance().getLanguage().c_str());
			tmpString=gameHints.getString(key,"");
			if(tmpString!=""){
				gameHintToShow=tmpString;
			}
			else {
				gameHintToShow="Problems to resolve hint key '"+key+"'";
			}
		}
		replaceAll(gameHintToShow, "\\n", "\n");

		Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));

		vector<pair<string,string> > mergedKeySettings = configKeys.getMergedProperties();
		for(unsigned int j = 0; j < mergedKeySettings.size(); ++j) {
            pair<string,string> &property = mergedKeySettings[j];
            replaceAll(gameHintToShow, "#"+property.first+"#", property.second);
		}
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
}

void Logger::clearHints() {
	gameHintToShow="";
	gameHints.clear();
	gameHintsTranslation.clear();
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

	if(gameHintToShow != "") {
		Lang &lang= Lang::getInstance();
		string hintText = lang.get("Hint","",true);
		char szBuf[8096]="";
		sprintf(szBuf,hintText.c_str(),gameHintToShow.c_str());
		hintText = szBuf;

		if(Renderer::renderText3DEnabled) {
			int xLocationHint =  (metrics.getVirtualW() / 2) - (coreData.getMenuFontBig3D()->getMetrics()->getTextWidth(hintText) / 2);

			renderer.renderText3D(
					hintText, coreData.getMenuFontBig3D(), Vec3f(1.f),
					//xLocation*1.5f,
					xLocationHint,
					90 * metrics.getVirtualH() / 100, false);
		}
		else {
			int xLocationHint =  (metrics.getVirtualW() / 2) - (coreData.getMenuFontBig()->getMetrics()->getTextWidth(hintText) / 2);

			renderer.renderText(
					hintText, coreData.getMenuFontBig(), Vec3f(1.f),
				//xLocation*1.5f,
				xLocationHint,
				90 * metrics.getVirtualH() / 100, false);

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
