// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2010-  by Titus Tscharntke
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_SERVERLINE_H_
#define _GLEST_GAME_SERVERLINE_H_

#include "masterserver_info.h"
#include "components.h"
#include "lang.h"
#include "world.h"

#include "leak_dumper.h"

namespace Glest{ namespace Game{

// ===============================
// 	ServerLine
// ===============================

class ServerLine {
private:

	MasterServerInfo masterServerInfo;
	int lineHeight;
	int baseY;
	bool compatible;
	GraphicButton selectButton;
	GraphicLabel wrongVersionLabel;

	//general info:
	GraphicLabel glestVersionLabel;
	GraphicLabel platformLabel;
	//GraphicLabel binaryCompileDateLabel;

	//game info:
	GraphicLabel serverTitleLabel;
	GraphicLabel ipAddressLabel;

	//game setup info:
	GraphicLabel techLabel;
	GraphicLabel mapLabel;
	GraphicLabel tilesetLabel;
	GraphicLabel activeSlotsLabel;

	GraphicLabel externalConnectPort;

	GraphicLabel country;
	GraphicLabel status;

	Texture2D *countryTexture;

	const char * containerName;

public:
	ServerLine( MasterServerInfo *mServerInfo, int lineIndex, int baseY, int lineHeight, const char *containerName);
	virtual ~ServerLine();
	MasterServerInfo *getMasterServerInfo() {return &masterServerInfo;}
	const int getLineHeight() const	{return lineHeight;}
	bool buttonMouseClick(int x, int y);
	bool buttonMouseMove(int x, int y);
	void setY(int y);
	//void setIndex(int value);
	void render();
};

}}//end namespace

#endif
