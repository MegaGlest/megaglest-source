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
#include "gl_wrap.h"
#include "window.h"
#include "texture.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Platform;
using namespace Shared::Graphics;

namespace CEGUI {
	class Window;
	//class OpenGLRenderer;
};

//namespace Shared { namespace Graphics {
//	class Texture2D;
//}};

namespace Glest { namespace Game {

class MegaGlest_CEGUI_Events_Manager;

class MegaGlest_CEGUIManagerBackInterface {
public:
	MegaGlest_CEGUIManagerBackInterface() {}
	virtual ~MegaGlest_CEGUIManagerBackInterface() {};

	virtual bool EventCallback(CEGUI::Window *ctl, std::string name) = 0;
};

class MegaGlest_CEGUIManager : 
	public PlatformContextRendererInterface,
    public WindowInputInterface {

private:

	//void initializeMainMenuRoot();

protected:

	bool initialized;
	CEGUI::Window *emptyMainWindowRoot;
	CEGUI::Window *messageBoxRoot;
	CEGUI::Window *errorMessageBoxRoot;
	string containerName;
	std::map<string, std::vector<MegaGlest_CEGUI_Events_Manager *> > eventManagerList;
	std::map<string, CEGUI::Window *> layoutCache;

	bool isBootStrapped;
	double last_time_pulse;
	//CEGUI::OpenGLRenderer *guiRenderer;

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

	virtual void setupRenderer(int width, int height);
	virtual void renderAllContexts();

	virtual void handleMouseMotion(float x, float y);
	virtual void handleMouseButtonDown(Uint8 button);
	virtual void handleMouseButtonUp(Uint8 button);
	virtual void handleKeyDown(SDL_keysym key);
	virtual void handleKeyUp(SDL_keysym key);
	virtual void handleTimePulse();

	void setupCEGUI();
	bool isInitialized() const { return initialized; }
	void clearRootWindow();

	CEGUI::Window * loadLayoutFromFile(string layoutFile);
	CEGUI::Window * setCurrentLayout(string layoutFile, string containerName);
	void setControlText(string controlName, string text, bool disableFormatting=false);
	void setControlText(CEGUI::Window *ctl, string text, bool disableFormatting=false);
	void setControlTextColor(CEGUI::Window *ctl, float red, float green, float blue, float alpha=1.0f);
	void setControlTextColor(string controlName, float red, float green, float blue, float alpha=1.0f);
	string getTextColorFromRGBA(float red, float green, float blue, float alpha=1.0f);
	string getControlText(string controlName);
	string getControlText(CEGUI::Window *ctl);
	void setControlEventCallback(string containerName, string controlName,
			string eventName, MegaGlest_CEGUIManagerBackInterface *cb);
	void setControlEventCallback(string containerName, CEGUI::Window *ctl,
			string eventName, MegaGlest_CEGUIManagerBackInterface *cb);

	void setControlVisible(string controlName, bool visible);
	void setControlVisible(CEGUI::Window *ctl, bool visible);
	bool getControlVisible(CEGUI::Window *ctl);

	void setControlOnTop(string controlName, bool ontop);
	void setControlOnTop(CEGUI::Window *ctl, bool ontop);

	void setControlReadOnly(CEGUI::Window *ctl, bool readOnly);
	void setControlEnabled(CEGUI::Window *ctl, bool enabled);
	bool getControlEnabled(CEGUI::Window *ctl);

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
	string getEventMultiColumnListSelectionChanged();

	void subscribeEventClick(std::string containerName, CEGUI::Window *ctl,
			std::string name, MegaGlest_CEGUIManagerBackInterface *cb);
	void unsubscribeEvents(std::string containerName);

	void addFont(string fontName, string fontFileName, float fontPointSize);
	void setControlFont(CEGUI::Window *ctl, string fontName, float fontPointSize);
	void setDefaultFont(string fontName, string fontFileName, float fontPointSize);

	void setImageFileForControl(string imageName, string imageFileName, string controlName);
	void setImageForControl(string textureName,Texture2D *gameTexture, string controlName, bool isTextureTargetVerticallyFlipped);

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
	void addItemToComboBoxControl(CEGUI::Window *ctl, string value, int id, void *userData,bool disableFormatting=false);
	void addItemsToComboBoxControl(CEGUI::Window *ctl, vector<string> valueList, bool disableFormatting=false);
	void addItemsToComboBoxControl(CEGUI::Window *ctl, map<string,int> mapList, bool disableFormatting=false);
	void addItemsToComboBoxControl(CEGUI::Window *ctl, map<string,void*> mapList, bool disableFormatting=false);
	void setSelectedItemInComboBoxControl(CEGUI::Window *ctl, int index);
	void setSelectedItemInComboBoxControl(CEGUI::Window *ctl, string value, bool disableFormatting=false);
	void setSelectedUserDataItemInComboBoxControl(CEGUI::Window *ctl, void *value, bool disableFormatting=false);
	string getSelectedItemFromComboBoxControl(CEGUI::Window *ctl);
	string getItemFromComboBoxControl(CEGUI::Window *ctl, int index);
	int getSelectedItemIndexFromComboBoxControl(CEGUI::Window *ctl);
	int getSelectedItemIdFromComboBoxControl(CEGUI::Window *ctl);
	void * getSelectedUserDataItemFromComboBoxControl(CEGUI::Window *ctl);
	int getItemCountInComboBoxControl(CEGUI::Window *ctl);
	bool hasItemInComboBoxControl(CEGUI::Window *ctl, string value);

	void addItemToListBoxControl(CEGUI::Window *ctl, string value, int index, bool disableFormatting=false);
	void addItemsToListBoxControl(CEGUI::Window *ctl, vector<string> valueList, bool disableFormatting=false);
	void setSelectedItemInListBoxControl(CEGUI::Window *ctl, int index);
	void setSelectedItemInListBoxControl(CEGUI::Window *ctl, string value, bool disableFormatting=false);

	void setSpinnerControlValues(CEGUI::Window *ctl, double minValue, double maxValue, double curValue, double interval=1);
	void setSpinnerControlValue(CEGUI::Window *ctl, double curValue);
	double getSpinnerControlValue(CEGUI::Window *ctl);

	void setCheckboxControlChecked(CEGUI::Window *ctl, bool checked, bool disableEventsTrigger=false);
	bool getCheckboxControlChecked(CEGUI::Window *ctl);

	void setColumnsForMultiColumnListControl(CEGUI::Window *ctl, vector<pair<string, float> > columnValues);
	void addItemToMultiColumnListControl(CEGUI::Window *ctl, vector<string> columnValues, int id, bool disableFormatting=false);
	void addItemsToMultiColumnListControl(CEGUI::Window *ctl, vector<vector<string> > valueList, bool disableFormatting=false);
	void setSelectedItemTextForMultiColumnListControl(CEGUI::Window *ctl, string value, int id, bool disableFormatting=false);
	int getSelectedRowForMultiColumnListControl(CEGUI::Window *ctl);

	void setSliderControlValues(CEGUI::Window *ctl, float minValue, float maxValue, float curValue,float interval);
	double getSliderControlValue(CEGUI::Window *ctl);

	void printDebugControlInfo(CEGUI::Window *ctl);
};

}} //end namespace

#endif
