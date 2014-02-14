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

#ifndef MEGAGLEST_CEGUI_MANAGER_H_
#define MEGAGLEST_CEGUI_MANAGER_H_

#include <string>
#include <map>
#include <vector>

#include "leak_dumper.h"

using namespace std;

namespace CEGUI {
	class Window;
};

namespace Glest { namespace Game {

class MegaGlest_CEGUI_Events_Manager;

class MegaGlest_CEGUIManagerBackInterface {
public:
	MegaGlest_CEGUIManagerBackInterface() {}
	virtual ~MegaGlest_CEGUIManagerBackInterface() {};

	virtual bool EventCallback(CEGUI::Window *ctl, std::string name) = 0;
};

class MegaGlest_CEGUIManager {

private:

	//void initializeMainMenuRoot();

protected:

	CEGUI::Window *emptyMainWindowRoot;
	CEGUI::Window *messageBoxRoot;
	CEGUI::Window *errorMessageBoxRoot;
	string containerName;
	std::map<string, std::vector<MegaGlest_CEGUI_Events_Manager *> > eventManagerList;
	std::map<string, CEGUI::Window *> layoutCache;

	MegaGlest_CEGUIManager();

	void initializeResourceGroupDirectories();
	void initializeDefaultResourceGroups();
	void initializeTheme();

	string getThemeName();
	string getThemeCursorName();
	string getLookName();

	CEGUI::Window * getMessageBoxRoot();
	CEGUI::Window * getErrorMessageBoxRoot();

public:

	static MegaGlest_CEGUIManager & getInstance();
	virtual ~MegaGlest_CEGUIManager();

	void setupCEGUI();
	void clearRootWindow();

	CEGUI::Window * loadLayoutFromFile(string layoutFile);
	CEGUI::Window * setCurrentLayout(string layoutFile, string containerName);
	void setControlText(string controlName, string text, bool disableFormatting=false);
	void setControlText(CEGUI::Window *ctl, string text, bool disableFormatting=false);
	void setControlEventCallback(string containerName, string controlName,
			string eventName, MegaGlest_CEGUIManagerBackInterface *cb);

	CEGUI::Window * getRootControl();
	CEGUI::Window * getControl(string controlName, bool recursiveScan=false);
	CEGUI::Window * getChildControl(CEGUI::Window *parentCtl,string controlNameChild, bool recursiveScan=false);
	void dumpWindowNames(string dumpTag);

	string getEventClicked();

	void subscribeEventClick(std::string containerName, CEGUI::Window *ctl,
			std::string name, MegaGlest_CEGUIManagerBackInterface *cb);
	void unsubscribeEvents(std::string containerName);

	void setDefaultFont(string fontName, string fontFileName, float fontPointSize);
	void setImageFileForControl(string imageName, string imageFileName, string controlName);

	void subscribeMessageBoxEventClicks(std::string containerName, MegaGlest_CEGUIManagerBackInterface *cb);
	void displayMessageBox(string title, string text, string buttonTextOk, string buttonTextCancel);
	bool isMessageBoxShowing();
	void hideMessageBox();
	bool isControlMessageBoxOk(CEGUI::Window *ctl);
	bool isControlMessageBoxCancel(CEGUI::Window *ctl);

	void subscribeErrorMessageBoxEventClicks(std::string containerName, MegaGlest_CEGUIManagerBackInterface *cb);
	void displayErrorMessageBox(string title, string text, string buttonTextOk);
	bool isErrorMessageBoxShowing();
	void hideErrorMessageBox();
	bool isControlErrorMessageBoxOk(CEGUI::Window *ctl);
	bool isControlErrorMessageBoxCancel(CEGUI::Window *ctl);

	void addTabPageToTabControl(string tabControlName, CEGUI::Window *ctl);

	void addItemToComboBoxControl(CEGUI::Window *ctl, string value, int index, bool disableFormatting=false);
	void addItemsToComboBoxControl(CEGUI::Window *ctl, vector<string> valueList, bool disableFormatting=false);
	void setSelectedItemInComboBoxControl(CEGUI::Window *ctl, int index);
	void setSelectedItemInComboBoxControl(CEGUI::Window *ctl, string value, bool disableFormatting=false);

	void addItemToListBoxControl(CEGUI::Window *ctl, string value, int index, bool disableFormatting=false);
	void addItemsToListBoxControl(CEGUI::Window *ctl, vector<string> valueList, bool disableFormatting=false);
	void setSelectedItemInListBoxControl(CEGUI::Window *ctl, int index);
	void setSelectedItemInListBoxControl(CEGUI::Window *ctl, string value, bool disableFormatting=false);

};

}} //end namespace

#endif
