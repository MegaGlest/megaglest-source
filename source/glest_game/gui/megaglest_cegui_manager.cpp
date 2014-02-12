// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2014 Mark Vejvoda
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 3 of the
//	License, or (at your option) any later version
// ==============================================================

#include "megaglest_cegui_manager.h"
#include <CEGUI/CEGUI.h>
#include "game_util.h"
#include "game_constants.h"
#include "config.h"
#include "platform_common.h"
#include "font.h"
#include "conversion.h"

#include "leak_dumper.h"

using namespace Shared::PlatformCommon;

namespace Glest { namespace Game {

class MegaGlest_CEGUI_Events_Manager {

protected:

	CEGUI::Window *ctl;
	std::string name;
	MegaGlest_CEGUIManagerBackInterface *callback;

public:

	MegaGlest_CEGUI_Events_Manager(CEGUI::Window *ctl, std::string name,
			MegaGlest_CEGUIManagerBackInterface *callback) {

		this->ctl 		= ctl;
		this->name 		= name;
		this->callback 	= callback;
	}

	bool Event(const CEGUI::EventArgs &event) {
		//printf("Line: %d this->name: %s\n",__LINE__,this->name.c_str());
		return callback->EventCallback(this->ctl,this->name);
	}

	CEGUI::Event::Subscriber getSubscriber() {
		return CEGUI::Event::Subscriber(&MegaGlest_CEGUI_Events_Manager::Event, this);
	}
};

MegaGlest_CEGUIManager::MegaGlest_CEGUIManager() {

}

MegaGlest_CEGUIManager::~MegaGlest_CEGUIManager() {
	for(std::map<string, std::vector<MegaGlest_CEGUI_Events_Manager *> >::iterator iterMap = eventManagerList.begin();
			iterMap != eventManagerList.end(); ++iterMap) {

		std::vector<MegaGlest_CEGUI_Events_Manager *> &events = iterMap->second;
		while(events.empty() == false) {
			delete events.back();
			events.pop_back();
		}
	}
	eventManagerList.clear();
}

MegaGlest_CEGUIManager & MegaGlest_CEGUIManager::getInstance() {
	static MegaGlest_CEGUIManager manager;
	return manager;
}

void MegaGlest_CEGUIManager::initializeResourceGroupDirectories() {
	// initialise the required dirs for the DefaultResourceProvider
	CEGUI::DefaultResourceProvider* rp =
		static_cast<CEGUI::DefaultResourceProvider*>
			(CEGUI::System::getSingleton().getResourceProvider());

	string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);

	string data_path_cegui = Config::getInstance().getString("CEGUI-Theme-Path","");
	if(data_path_cegui == "") {
		data_path_cegui = data_path + "/data/cegui/themes/default/";
	}

	const char* dataPathPrefix = data_path_cegui.c_str();
	char resourcePath[PATH_MAX];

	// for each resource type, set a resource group directory
	sprintf(resourcePath, "%s/%s", dataPathPrefix, "schemes/");
	rp->setResourceGroupDirectory("schemes", resourcePath);
	sprintf(resourcePath, "%s/%s", dataPathPrefix, "imagesets/");
	rp->setResourceGroupDirectory("imagesets", resourcePath);
	sprintf(resourcePath, "%s/%s", dataPathPrefix, "fonts/");
	rp->setResourceGroupDirectory("fonts", resourcePath);
	sprintf(resourcePath, "%s/%s", dataPathPrefix, "layouts/");
	rp->setResourceGroupDirectory("layouts", resourcePath);
	sprintf(resourcePath, "%s/%s", dataPathPrefix, "looknfeel/");
	rp->setResourceGroupDirectory("looknfeels", resourcePath);
	sprintf(resourcePath, "%s/%s", dataPathPrefix, "lua_scripts/");
	rp->setResourceGroupDirectory("lua_scripts", resourcePath);
	sprintf(resourcePath, "%s/%s", dataPathPrefix, "xml_schemas/");
	rp->setResourceGroupDirectory("schemas", resourcePath);
	sprintf(resourcePath, "%s/%s", dataPathPrefix, "animations/");
	rp->setResourceGroupDirectory("animations", resourcePath);
}

void MegaGlest_CEGUIManager::initializeDefaultResourceGroups() {
	// set the default resource groups to be used
	CEGUI::ImageManager::setImagesetDefaultResourceGroup("imagesets");
	CEGUI::Font::setDefaultResourceGroup("fonts");
	CEGUI::Scheme::setDefaultResourceGroup("schemes");
	CEGUI::WidgetLookManager::setDefaultResourceGroup("looknfeels");
	CEGUI::WindowManager::setDefaultResourceGroup("layouts");
	CEGUI::ScriptModule::setDefaultResourceGroup("lua_scripts");
	CEGUI::AnimationManager::setDefaultResourceGroup("animations");
	// setup default group for validation schemas
	CEGUI::XMLParser* parser = CEGUI::System::getSingleton().getXMLParser();
	if (parser->isPropertyPresent("SchemaDefaultResourceGroup")) {
		parser->setProperty("SchemaDefaultResourceGroup", "schemas");
	}
}

string MegaGlest_CEGUIManager::getThemeName() {
	string themeName = Config::getInstance().getString("CEGUI-Theme-Name","");
	if(themeName == "") {
		themeName = "TaharezLook";
	}
	return themeName;
}

string MegaGlest_CEGUIManager::getThemeCursorName() {
	string themeNameCursors = Config::getInstance().getString("CEGUI-Theme-Name-Cursors","");
	if(themeNameCursors == "") {
		themeNameCursors = "TaharezLook";
	}
	return themeNameCursors;
}

void MegaGlest_CEGUIManager::initializeTheme() {
	string themeName 		= getThemeName();
	string themeNameCursors = getThemeCursorName();

	CEGUI::SchemeManager::getSingleton().createFromFile(themeName + ".scheme");
	CEGUI::System::getSingleton().getDefaultGUIContext().getMouseCursor().setDefaultImage(themeNameCursors + "/MouseArrow");
}

void MegaGlest_CEGUIManager::setupCEGUI() {

	initializeResourceGroupDirectories();
	initializeDefaultResourceGroups();
	initializeTheme();

	string fontFile = findFont();
	printf("\nCE-GUI set default font: %s\n\n",fontFile.c_str());
	setFontDefaultFont("MEGAGLEST_FONT", fontFile, 12.0f);
}

//void MegaGlest_CEGUIManager::initializeMainMenuRoot() {
//	string themeName 		= getThemeName();
//	string themeNameCursors = getThemeCursorName();
//
//	CEGUI::WindowManager& winMgr(CEGUI::WindowManager::getSingleton());

//	CEGUI::Window* root = winMgr.createWindow("DefaultWindow", "root");


//	CEGUI::Window* fw = root->createChild(themeName + "/FrameWindow");
//	fw->setPosition(CEGUI::UVector2(CEGUI::UDim(0.25, 0), CEGUI::UDim(0.25, 0)));
//	fw->setSize(CEGUI::USize(CEGUI::UDim(0.5, 0), CEGUI::UDim(0.5, 0)));
//	fw->setText("MegaGlest CE-GUI Test");
//
//	CEGUI::Window *gTestBtnWindow = CEGUI::WindowManager::getSingleton().createWindow(themeName + "/Button","TestPushButton");
//	gTestBtnWindow->setPosition(CEGUI::UVector2(cegui_reldim(0.81f), cegui_reldim( 0.32f)));
//	gTestBtnWindow->setSize(CEGUI::USize(cegui_reldim(0.15f), cegui_reldim( 0.2f)));
//	gTestBtnWindow->setText("Test Button");
//	fw->addChild(gTestBtnWindow);
//
//	CEGUI::Window *gTestComboWindow = CEGUI::WindowManager::getSingleton().createWindow(themeName + "/Combobox","TestCombobox");
//	gTestComboWindow->setPosition(CEGUI::UVector2(cegui_reldim(0.04f), cegui_reldim( 0.06f)));
//	gTestComboWindow->setSize(CEGUI::USize(cegui_reldim(0.66f), cegui_reldim( 0.33f)));
//	gTestComboWindow->setText("Test Combo");
//	fw->addChild(gTestComboWindow);
//
//	CEGUI::Window *label = CEGUI::WindowManager::getSingleton().createWindow(themeName + "/StaticText", "Label4");
//    fw->addChild(label);
//    label->setProperty("FrameEnabled", "false");
//    label->setProperty("BackgroundEnabled", "false");
//    label->setPosition(CEGUI::UVector2(cegui_reldim(0.02f), cegui_reldim( 0.55f)));
//    label->setSize(CEGUI::USize(cegui_reldim(0.2f), cegui_reldim( 0.12f)));
//    label->setText("Player Name:");

//	CEGUI::System::getSingleton().getDefaultGUIContext().setRootWindow(root);

//	CEGUI::System::getSingleton().getDefaultGUIContext().setRootWindow(
//			winMgr.loadLayoutFromFile("MainMenuRoot.layout"));
//
//}

void MegaGlest_CEGUIManager::subscribeEventClick(std::string containerName, CEGUI::Window *ctl, std::string name, MegaGlest_CEGUIManagerBackInterface *callback) {
	//printf("Line: %d\n",__LINE__);

	MegaGlest_CEGUI_Events_Manager *mgr = new MegaGlest_CEGUI_Events_Manager(ctl,name,callback);
	eventManagerList[containerName].push_back(mgr);
	ctl->subscribeEvent(name,mgr->getSubscriber());
}

void MegaGlest_CEGUIManager::unsubscribeEvents(std::string containerName) {
	std::map<string, std::vector<MegaGlest_CEGUI_Events_Manager *> >::iterator iterMap = eventManagerList.find(containerName);
	if(iterMap != eventManagerList.end()) {
		std::vector<MegaGlest_CEGUI_Events_Manager *> &events = iterMap->second;
		while(events.empty() == false) {
			delete events.back();
			events.pop_back();
		}
		eventManagerList.erase(containerName);
	}
}

void MegaGlest_CEGUIManager::setCurrentLayout(string layoutFile) {
	CEGUI::WindowManager& winMgr(CEGUI::WindowManager::getSingleton());
	CEGUI::System::getSingleton().getDefaultGUIContext().setRootWindow(
			winMgr.loadLayoutFromFile(layoutFile));
}

void MegaGlest_CEGUIManager::setControlText(string controlName, string text) {
	CEGUI::Window *root = CEGUI::System::getSingleton().
			getDefaultGUIContext().getRootWindow();

	CEGUI::Window *ctl = root->getChild(controlName);
	ctl->setText((CEGUI::encoded_char*)text.c_str());
}

void MegaGlest_CEGUIManager::setControlEventCallback(string containerName,
		string controlName, string eventName, MegaGlest_CEGUIManagerBackInterface *cb) {

	CEGUI::Window *root = CEGUI::System::getSingleton().
			getDefaultGUIContext().getRootWindow();
	CEGUI::Window *ctl = root->getChild(controlName);
	subscribeEventClick(containerName,ctl,eventName, cb);

}

CEGUI::Window * MegaGlest_CEGUIManager::getControl(string controlName) {
	CEGUI::Window *root = CEGUI::System::getSingleton().
			getDefaultGUIContext().getRootWindow();
	return root->getChild(controlName);
}

string MegaGlest_CEGUIManager::getEventClicked() {
	string result = CEGUI::PushButton::EventClicked.c_str();
	return result;
}

void MegaGlest_CEGUIManager::setFontDefaultFont(string fontName, string fontFileName, float fontPointSize) {

	string fontPath = extractDirectoryPathFromFile(fontFileName);
	string fontFile = extractFileFromDirectoryPath(fontFileName);

	CEGUI::FontManager& fontManager(CEGUI::FontManager::getSingleton());
	string fontNameIdentifier = fontName + "-" + floatToStr(fontPointSize);

	if(fontManager.isDefined(fontNameIdentifier) == false) {
		CEGUI::Font &font = fontManager.createFreeTypeFont(
				fontNameIdentifier,
				fontPointSize,
				true,
				fontFileName,
				//CEGUI::Font::getDefaultResourceGroup(),
				fontPath,
				CEGUI::ASM_Vertical,
				CEGUI::Sizef(1280.0f, 720.0f));
		CEGUI::System::getSingleton().getDefaultGUIContext().setDefaultFont(&font);
	}
	else {
		CEGUI::Font &font(fontManager.get(fontNameIdentifier));
		CEGUI::System::getSingleton().getDefaultGUIContext().setDefaultFont(&font);
	}

}

}}//end namespace
