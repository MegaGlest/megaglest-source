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

	void initializeMainMenuRoot();

protected:

	std::map<string, std::vector<MegaGlest_CEGUI_Events_Manager *> > eventManagerList;
	MegaGlest_CEGUIManager();

	void initializeResourceGroupDirectories();
	void initializeDefaultResourceGroups();
	void initializeTheme();

	string getThemeName();
	string getThemeCursorName();

public:

	static MegaGlest_CEGUIManager & getInstance();
	virtual ~MegaGlest_CEGUIManager();

	void setupCEGUI();

	void subscribeEventClick(std::string containerName, CEGUI::Window *ctl, std::string name, MegaGlest_CEGUIManagerBackInterface *cb);
	void unsubscribeEvents(std::string containerName);
};

}} //end namespace

#endif
