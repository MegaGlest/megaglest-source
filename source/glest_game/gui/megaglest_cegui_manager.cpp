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
#include "string_utils.h"

#include "leak_dumper.h"

using namespace Shared::PlatformCommon;

namespace Glest { namespace Game {

// =========================================================================
//
// This class wraps CEGUI event management
//
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

	CEGUI::Window * getControl() { return ctl; }
	std::string getEventName() { return name; }
	MegaGlest_CEGUIManagerBackInterface * getCallback() { return callback; }

	bool Event(const CEGUI::EventArgs &event) {
		//printf("Line: %d this->name: %s\n",__LINE__,this->name.c_str());
		return callback->EventCallback(this->ctl,this->name);
	}

	CEGUI::Event::Subscriber getSubscriber() {
		return CEGUI::Event::Subscriber(&MegaGlest_CEGUI_Events_Manager::Event, this);
	}
};

// =========================================================================
//
// This is the main CEGUI Wrapper class
//
MegaGlest_CEGUIManager::MegaGlest_CEGUIManager() {
	emptyMainWindowRoot = NULL;
	messageBoxRoot 		= NULL;
	errorMessageBoxRoot = NULL;
	containerName 		= "";
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
		themeName = "GlossySerpent";
	}
	return themeName;
}

string MegaGlest_CEGUIManager::getThemeCursorName() {
	string themeNameCursors = Config::getInstance().getString("CEGUI-Theme-Name-Cursors","");
	if(themeNameCursors == "") {
		themeNameCursors = "GlossySerpentCursors";
	}
	return themeNameCursors;
}

string MegaGlest_CEGUIManager::getLookName() {
	string themeName = Config::getInstance().getString("CEGUI-Look-Name","");
	if(themeName == "") {
		themeName = "GlossySerpent";
	}
	return themeName;
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
	//printf("\nCE-GUI set default font: %s\n\n",fontFile.c_str());
	setDefaultFont("MEGAGLEST_FONT", fontFile, 10.0f);

	emptyMainWindowRoot = loadLayoutFromFile("EmptyRoot.layout");
	messageBoxRoot 		= loadLayoutFromFile("MessageBox.layout");
	errorMessageBoxRoot = loadLayoutFromFile("ErrorMessageBox.layout");
}

void MegaGlest_CEGUIManager::clearRootWindow() {

	CEGUI::System::getSingleton().getDefaultGUIContext().
			setRootWindow(emptyMainWindowRoot);
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
	//printf("SUBSCRIBE Line: %d containerName [%s] ctl [%s] name [%s]\n",__LINE__,containerName.c_str(),ctl->getName().c_str(),name.c_str());

	MegaGlest_CEGUI_Events_Manager *mgr = new MegaGlest_CEGUI_Events_Manager(ctl,name,callback);
	eventManagerList[containerName].push_back(mgr);
	ctl->subscribeEvent(name,mgr->getSubscriber());
}

void MegaGlest_CEGUIManager::unsubscribeEvents(std::string containerName) {
	std::map<string, std::vector<MegaGlest_CEGUI_Events_Manager *> >::iterator iterMap = eventManagerList.find(containerName);
	if(iterMap != eventManagerList.end()) {
		std::vector<MegaGlest_CEGUI_Events_Manager *> &events = iterMap->second;
		while(events.empty() == false) {
			MegaGlest_CEGUI_Events_Manager *event = events.back();
			if(event != NULL) {
				//printf("UNSUBSCRIBE Line: %d containerName [%s] ctl [%s] name [%s]\n",__LINE__,containerName.c_str(),event->getControl()->getName().c_str(),event->getEventName().c_str());
				event->getControl()->removeEvent(event->getEventName());
			}
			delete event;
			events.pop_back();
		}
		eventManagerList.erase(containerName);
	}
}

CEGUI::Window * MegaGlest_CEGUIManager::loadLayoutFromFile(string layoutFile) {
	CEGUI::WindowManager& winMgr(CEGUI::WindowManager::getSingleton());
	if(layoutCache.find(layoutFile) == layoutCache.end()) {
		layoutCache[layoutFile] = winMgr.loadLayoutFromFile(layoutFile);
	}
	return layoutCache[layoutFile];
}

CEGUI::Window * MegaGlest_CEGUIManager::setCurrentLayout(string layoutFile, string containerName) {

	CEGUI::Window *ctl = loadLayoutFromFile(layoutFile);
	CEGUI::System::getSingleton().getDefaultGUIContext().setRootWindow(ctl);
	this->containerName = containerName;

	return ctl;
}

void MegaGlest_CEGUIManager::setControlText(string controlName, string text, bool disableFormatting) {
	CEGUI::Window *root = CEGUI::System::getSingleton().
			getDefaultGUIContext().getRootWindow();

	CEGUI::Window *ctl = root->getChild(controlName);
	setControlText(ctl,text, disableFormatting);
}

void MegaGlest_CEGUIManager::setControlText(CEGUI::Window *ctl, string text, bool disableFormatting) {

	ctl->setTextParsingEnabled(disableFormatting == false);
	ctl->setText((CEGUI::encoded_char*)text.c_str());
}

void MegaGlest_CEGUIManager::setControlEventCallback(string containerName,
		string controlName, string eventName, MegaGlest_CEGUIManagerBackInterface *cb) {

	CEGUI::Window *root = CEGUI::System::getSingleton().
			getDefaultGUIContext().getRootWindow();
	CEGUI::Window *ctl = root->getChild(controlName);
	subscribeEventClick(containerName,ctl,eventName, cb);

}

void MegaGlest_CEGUIManager::dumpWindowNames(string dumpTag) {
	CEGUI::WindowManager &winMgr(CEGUI::WindowManager::getSingleton());
	winMgr.DEBUG_dumpWindowNames(dumpTag);
}

CEGUI::Window * MegaGlest_CEGUIManager::getRootControl() {
	return CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
}

CEGUI::Window * MegaGlest_CEGUIManager::getControl(string controlName, bool recursiveScan) {
	CEGUI::Window *root = CEGUI::System::getSingleton().
			getDefaultGUIContext().getRootWindow();
	if(recursiveScan == false) {
		return root->getChild(controlName);
	}
	else {
		CEGUI::NamedElement *element = root->getChildElementRecursive(controlName);
		if(element != NULL) {
			return root->getChild(element->getNamePath());
		}
		return NULL;
	}
}

CEGUI::Window * MegaGlest_CEGUIManager::getChildControl(CEGUI::Window *parentCtl,string controlNameChild, bool recursiveScan) {
	if(recursiveScan == false) {
		return parentCtl->getChild(controlNameChild);
	}
	CEGUI::NamedElement *element = parentCtl->getChildElementRecursive(controlNameChild);
	if(element != NULL) {
		return parentCtl->getChild(element->getNamePath());
	}
	return NULL;
}

string MegaGlest_CEGUIManager::getEventClicked() {
	string result = CEGUI::PushButton::EventClicked.c_str();
	return result;
}

void MegaGlest_CEGUIManager::setDefaultFont(string fontName, string fontFileName, float fontPointSize) {

	string fontPath = extractDirectoryPathFromFile(fontFileName);
	string fontFile = extractFileFromDirectoryPath(fontFileName);

	CEGUI::FontManager &fontManager(CEGUI::FontManager::getSingleton());
	string fontNameIdentifier = fontName + "-" + floatToStr(fontPointSize);

	printf("\nCE-GUI set default font: [%s] fontNameIdentifier [%s] fontPointSize: %f\n\n",fontFileName.c_str(),fontNameIdentifier.c_str(),fontPointSize);

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

void MegaGlest_CEGUIManager::setImageFileForControl(string imageName, string imageFileName, string controlName) {

    if( CEGUI::ImageManager::getSingleton().isDefined(imageName) == false ) {
    	string filePath = extractDirectoryPathFromFile(imageFileName);
    	CEGUI::ImageManager::getSingleton().addFromImageFile(imageName, imageFileName, filePath);
    }
	CEGUI::DefaultWindow* staticImage = static_cast<CEGUI::DefaultWindow*>(getControl(controlName));

	staticImage->setProperty("Image", imageName);
}

CEGUI::Window * MegaGlest_CEGUIManager::getMessageBoxRoot() {

	CEGUI::Window *root = CEGUI::System::getSingleton().
			getDefaultGUIContext().getRootWindow();

	//printf("messageBoxRoot->getName() [%s]\n",messageBoxRoot->getName().c_str());

	if(root->isChild(messageBoxRoot->getName()) == false) {
		CEGUI::Window *ctl = messageBoxRoot->clone(true);
		ctl->setVisible(false);
		ctl->setAlwaysOnTop(true);
		root->addChild(ctl);
	}
	return root->getChild(messageBoxRoot->getName());
}
void MegaGlest_CEGUIManager::displayMessageBox(string title, string text,
		string buttonTextOk, string buttonTextCancel) {

	CEGUI::Window *ctlMsg = getMessageBoxRoot()->getChild("MessageBox");
	ctlMsg->setText(title);
	ctlMsg->getChild("MessageText")->setText(text);
	ctlMsg->getChild("ButtonOk")->setText(buttonTextOk);
	ctlMsg->getChild("ButtonCancel")->setText(buttonTextCancel);

	getMessageBoxRoot()->setVisible(true);
}

bool MegaGlest_CEGUIManager::isMessageBoxShowing() {
	return getMessageBoxRoot()->isVisible();
}

void MegaGlest_CEGUIManager::hideMessageBox() {

	getMessageBoxRoot()->setVisible(false);
}

void MegaGlest_CEGUIManager::subscribeMessageBoxEventClicks(std::string containerName,
		MegaGlest_CEGUIManagerBackInterface *cb) {

	CEGUI::Window *ctlOk = getMessageBoxRoot()->getChild("MessageBox")->getChild("ButtonOk");
	subscribeEventClick(containerName, ctlOk, getEventClicked(), cb);
	CEGUI::Window *ctlCancel = getMessageBoxRoot()->getChild("MessageBox")->getChild("ButtonCancel");
	subscribeEventClick(containerName, ctlCancel, getEventClicked(), cb);
}

bool MegaGlest_CEGUIManager::isControlMessageBoxOk(CEGUI::Window *ctl) {
	bool result = false;

	if(getMessageBoxRoot()->isVisible() == true && ctl != NULL) {
		CEGUI::Window *ctlOk = getMessageBoxRoot()->getChild("MessageBox")->getChild("ButtonOk");
		result = (ctl == ctlOk);
	}
	return result;
}

bool MegaGlest_CEGUIManager::isControlMessageBoxCancel(CEGUI::Window *ctl) {

	bool result = false;

	if(getMessageBoxRoot()->isVisible() == true && ctl != NULL) {
		CEGUI::Window *ctlCancel = getMessageBoxRoot()->getChild("MessageBox")->getChild("ButtonCancel");
		result = (ctl == ctlCancel);
	}
	return result;
}

CEGUI::Window * MegaGlest_CEGUIManager::getErrorMessageBoxRoot() {

	CEGUI::Window *root = CEGUI::System::getSingleton().
			getDefaultGUIContext().getRootWindow();

	if(root->isChild(errorMessageBoxRoot->getName()) == false) {
		CEGUI::Window *ctl = errorMessageBoxRoot->clone(true);
		ctl->setVisible(false);
		ctl->setAlwaysOnTop(true);
		root->addChild(ctl);
	}
	return root->getChild(errorMessageBoxRoot->getName());
}
void MegaGlest_CEGUIManager::displayErrorMessageBox(string title, string text,
		string buttonTextOk) {

	CEGUI::Window *ctlMsg = getErrorMessageBoxRoot()->getChild("ErrorMessageBox");
	ctlMsg->setText(title);
	ctlMsg->getChild("MessageText")->setText(text);
	ctlMsg->getChild("ButtonOk")->setText(buttonTextOk);

	getErrorMessageBoxRoot()->setVisible(true);
}

bool MegaGlest_CEGUIManager::isErrorMessageBoxShowing() {
	return getErrorMessageBoxRoot()->isVisible();
}

void MegaGlest_CEGUIManager::hideErrorMessageBox() {

	getErrorMessageBoxRoot()->setVisible(false);
}

void MegaGlest_CEGUIManager::subscribeErrorMessageBoxEventClicks(std::string containerName,
		MegaGlest_CEGUIManagerBackInterface *cb) {

	CEGUI::Window *ctlOk = getErrorMessageBoxRoot()->getChild("ErrorMessageBox")->getChild("ButtonOk");
	subscribeEventClick(containerName, ctlOk, getEventClicked(), cb);
}

bool MegaGlest_CEGUIManager::isControlErrorMessageBoxOk(CEGUI::Window *ctl) {
	bool result = false;

	if(getErrorMessageBoxRoot()->isVisible() == true && ctl != NULL) {
		CEGUI::Window *ctlOk = getErrorMessageBoxRoot()->getChild("ErrorMessageBox")->getChild("ButtonOk");
		result = (ctl == ctlOk);
	}
	return result;
}

void MegaGlest_CEGUIManager::addTabPageToTabControl(string tabControlName, CEGUI::Window *ctl) {

	CEGUI::Window *genericCtl = getControl(tabControlName);
	CEGUI::TabControl *tabCtl = static_cast<CEGUI::TabControl *>(genericCtl);

	tabCtl->addTab(ctl);
}

void MegaGlest_CEGUIManager::addItemToComboBoxControl(CEGUI::Window *ctl, string value, int index, bool disableFormatting) {
	CEGUI::Combobox *combobox = static_cast<CEGUI::Combobox*>(ctl);
	bool wasReadOnly = combobox->isReadOnly();
	if(wasReadOnly == true) {
		combobox->setReadOnly(false);
	}

	CEGUI::String cegui_value((CEGUI::encoded_char*)value.c_str());
	//printf("\nCE-GUI add item to combobox: [%s] [%s]\n",cegui_value.c_str(),value.c_str());

	CEGUI::ListboxTextItem *itemCombobox = new CEGUI::ListboxTextItem(cegui_value, index);

	string selectionImageName = getLookName() + "/MultiListSelectionBrush";
	const CEGUI::Image *selectionImage = &CEGUI::ImageManager::getSingleton().get(selectionImageName);
	itemCombobox->setSelectionBrushImage(selectionImage);

	combobox->setTextParsingEnabled(disableFormatting == false);
	combobox->addItem(itemCombobox);
	if(wasReadOnly == true) {
		combobox->setReadOnly(true);
	}
}

void MegaGlest_CEGUIManager::addItemsToComboBoxControl(CEGUI::Window *ctl, vector<string> valueList, bool disableFormatting) {
	CEGUI::Combobox *combobox = static_cast<CEGUI::Combobox*>(ctl);
	int previousItemCount = combobox->getItemCount();

	for(unsigned int index = 0; index < valueList.size(); ++index) {

		string &value = valueList[index];
		int position = previousItemCount + index;
		addItemToComboBoxControl(ctl, value, position, disableFormatting);
	}
}

void MegaGlest_CEGUIManager::setSelectedItemInComboBoxControl(CEGUI::Window *ctl, int index) {
	CEGUI::Combobox *combobox = static_cast<CEGUI::Combobox*>(ctl);
	bool wasReadOnly = combobox->isReadOnly();
	if(wasReadOnly == true) {
		combobox->setReadOnly(false);
	}

	combobox->setItemSelectState(index,true);

	if(wasReadOnly == true) {
		combobox->setReadOnly(true);
	}
}

void MegaGlest_CEGUIManager::setSelectedItemInComboBoxControl(CEGUI::Window *ctl, string value, bool disableFormatting) {
	CEGUI::Combobox *combobox = static_cast<CEGUI::Combobox*>(ctl);
	bool wasReadOnly = combobox->isReadOnly();
	if(wasReadOnly == true) {
		combobox->setReadOnly(false);
	}

	CEGUI::String cegui_value((CEGUI::encoded_char*)value.c_str());
	combobox->setTextParsingEnabled(disableFormatting == false);
	combobox->setText(cegui_value);

	if(wasReadOnly == true) {
		combobox->setReadOnly(true);
	}
}

void MegaGlest_CEGUIManager::addItemToListBoxControl(CEGUI::Window *ctl, string value, int index, bool disableFormatting) {
	CEGUI::Listbox *listbox = static_cast<CEGUI::Listbox*>(ctl);

	CEGUI::String cegui_value((CEGUI::encoded_char*)value.c_str());
	CEGUI::ListboxTextItem *itemCombobox = new CEGUI::ListboxTextItem(cegui_value, index);

	string selectionImageName = getLookName() + "/MultiListSelectionBrush";
	const CEGUI::Image *selectionImage = &CEGUI::ImageManager::getSingleton().get(selectionImageName);
	itemCombobox->setSelectionBrushImage(selectionImage);

	listbox->setTextParsingEnabled(disableFormatting == false);
	listbox->addItem(itemCombobox);
}

void MegaGlest_CEGUIManager::addItemsToListBoxControl(CEGUI::Window *ctl, vector<string> valueList, bool disableFormatting) {
	CEGUI::Listbox *listbox = static_cast<CEGUI::Listbox*>(ctl);
	int previousItemCount = listbox->getItemCount();

	for(unsigned int index = 0; index < valueList.size(); ++index) {

		string &value = valueList[index];
		int position = previousItemCount + index;
		addItemToListBoxControl(ctl, value, position, disableFormatting);
	}
}

void MegaGlest_CEGUIManager::setSelectedItemInListBoxControl(CEGUI::Window *ctl, int index) {
	CEGUI::Listbox *listbox = static_cast<CEGUI::Listbox*>(ctl);

	listbox->setItemSelectState(index,true);
}

void MegaGlest_CEGUIManager::setSelectedItemInListBoxControl(CEGUI::Window *ctl, string value, bool disableFormatting) {
	CEGUI::Combobox *listbox = static_cast<CEGUI::Combobox*>(ctl);

	CEGUI::String cegui_value((CEGUI::encoded_char*)value.c_str());
	listbox->setTextParsingEnabled(disableFormatting == false);
	listbox->setText(cegui_value);
}

}}//end namespace

