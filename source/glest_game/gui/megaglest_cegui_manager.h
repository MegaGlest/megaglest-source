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

	CEGUI::Window * getMessageBoxRoot(string rootMessageBoxFullName="");
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
	string getControlText(string controlName);
	string getControlText(CEGUI::Window *ctl);
	void setControlEventCallback(string containerName, string controlName,
			string eventName, MegaGlest_CEGUIManagerBackInterface *cb);
	void setControlEventCallback(string containerName, CEGUI::Window *ctl,
			string eventName, MegaGlest_CEGUIManagerBackInterface *cb);

	void setControlVisible(string controlName, bool visible);
	void setControlVisible(CEGUI::Window *ctl, bool visible);

	CEGUI::Window * getRootControl();
	CEGUI::Window * getControl(string controlName, bool recursiveScan=false);
	CEGUI::Window * getChildControl(CEGUI::Window *parentCtl,string controlNameChild, bool recursiveScan=false);
	bool isChildControl(CEGUI::Window *parentCtl,string controlNameChild);
	string getControlFullPathName(CEGUI::Window *ctl);
	void dumpWindowNames(string dumpTag);

	string getEventButtonClicked();
	string getEventComboboxClicked();
	string getEventComboboxChangeAccepted();
	string getEventCheckboxClicked();
	string getEventTabControlSelectionChanged();
	string getEventSpinnerValueChanged();

	void subscribeEventClick(std::string containerName, CEGUI::Window *ctl,
			std::string name, MegaGlest_CEGUIManagerBackInterface *cb);
	void unsubscribeEvents(std::string containerName);

	void addFont(string fontName, string fontFileName, float fontPointSize);
	void setControlFont(CEGUI::Window *ctl, string fontName, float fontPointSize);
	void setDefaultFont(string fontName, string fontFileName, float fontPointSize);

	void setImageFileForControl(string imageName, string imageFileName, string controlName);

	CEGUI::Window * cloneMessageBoxControl(string newMessageBoxControlName, CEGUI::Window *rootControlToAddTo=NULL);
	void subscribeMessageBoxEventClicks(std::string containerName, MegaGlest_CEGUIManagerBackInterface *cb, string rootMessageBoxFullName="");
	void displayMessageBox(string title, string text, string buttonTextOk, string buttonTextCancel, string rootMessageBoxFullName="");
	void setMessageBoxButtonText(string buttonTextOk, string buttonTextCancel, string rootMessageBoxFullName="");
	bool isMessageBoxShowing(string rootMessageBoxFullName="");
	void hideMessageBox(string rootMessageBoxFullName="");
	bool isControlMessageBoxOk(CEGUI::Window *ctl, string rootMessageBoxFullName="");
	bool isControlMessageBoxCancel(CEGUI::Window *ctl, string rootMessageBoxFullName="");

	void subscribeErrorMessageBoxEventClicks(std::string containerName, MegaGlest_CEGUIManagerBackInterface *cb);
	void displayErrorMessageBox(string title, string text, string buttonTextOk);
	bool isErrorMessageBoxShowing();
	void hideErrorMessageBox();
	bool isControlErrorMessageBoxOk(CEGUI::Window *ctl);
	bool isControlErrorMessageBoxCancel(CEGUI::Window *ctl);

	void addTabPageToTabControl(string tabControlName, CEGUI::Window *ctl, string fontName="", float fontPointSize=-1);
	void setSelectedTabPage(string tabControlName, string tabPageName);
	bool isSelectedTabPage(string tabControlName, string tabPageName);

	void addItemToComboBoxControl(CEGUI::Window *ctl, string value, int id, bool disableFormatting=false);
	void addItemsToComboBoxControl(CEGUI::Window *ctl, vector<string> valueList, bool disableFormatting=false);
	void addItemsToComboBoxControl(CEGUI::Window *ctl, map<string,int> mapList, bool disableFormatting=false);
	void setSelectedItemInComboBoxControl(CEGUI::Window *ctl, int index);
	void setSelectedItemInComboBoxControl(CEGUI::Window *ctl, string value, bool disableFormatting=false);
	string getSelectedItemFromComboBoxControl(CEGUI::Window *ctl);
	int getSelectedItemIndexFromComboBoxControl(CEGUI::Window *ctl);
	int getSelectedItemIdFromComboBoxControl(CEGUI::Window *ctl);

	void addItemToListBoxControl(CEGUI::Window *ctl, string value, int index, bool disableFormatting=false);
	void addItemsToListBoxControl(CEGUI::Window *ctl, vector<string> valueList, bool disableFormatting=false);
	void setSelectedItemInListBoxControl(CEGUI::Window *ctl, int index);
	void setSelectedItemInListBoxControl(CEGUI::Window *ctl, string value, bool disableFormatting=false);

	void setSpinnerControlValues(CEGUI::Window *ctl, double minValue, double maxValue, double curValue, double interval=1);
	double getSpinnerControlValue(CEGUI::Window *ctl);

	void setCheckboxControlChecked(CEGUI::Window *ctl, bool checked, bool disableEventsTrigger=false);
	bool getCheckboxControlChecked(CEGUI::Window *ctl);

	void printDebugControlInfo(CEGUI::Window *ctl);
};

}} //end namespace

#endif
