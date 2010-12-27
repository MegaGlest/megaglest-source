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


#include "server_line.h"

#include "renderer.h"
#include "core_data.h"
#include "config.h"
#include "metrics.h"
#include "auto_test.h"
#include "masterserver_info.h"


#include "leak_dumper.h"



namespace Glest{ namespace Game{

// =====================================================
// 	class ServerLine
// =====================================================

ServerLine::ServerLine( MasterServerInfo *mServerInfo, int lineIndex, int baseY, int lineHeight, const char * containerName) {
	this->containerName = containerName;
	Lang &lang= Lang::getInstance();

	index=lineIndex;
	this->lineHeight=lineHeight;
	int lineOffset = lineHeight * lineIndex;
	masterServerInfo = *mServerInfo;
	int i=10;
	this->baseY=baseY;

	//general info:
	i+=10;
	glestVersionLabel.registerGraphicComponent(containerName,"glestVersionLabel" + intToStr(lineIndex));
	registeredObjNameList.push_back("glestVersionLabel" + intToStr(lineIndex));
	glestVersionLabel.init(i,baseY-lineOffset);
	glestVersionLabel.setText(masterServerInfo.getGlestVersion());

	i+=80;
	registeredObjNameList.push_back("platformLabel" + intToStr(lineIndex));
	platformLabel.registerGraphicComponent(containerName,"platformLabel" + intToStr(lineIndex));
	platformLabel.init(i,baseY-lineOffset);
	platformLabel.setText(masterServerInfo.getPlatform());

//	i+=50;
//	registeredObjNameList.push_back("binaryCompileDateLabel" + intToStr(lineIndex));
//	binaryCompileDateLabel.registerGraphicComponent(containerName,"binaryCompileDateLabel" + intToStr(lineIndex));
//	binaryCompileDateLabel.init(i,baseY-lineOffset);
//	binaryCompileDateLabel.setText(masterServerInfo.getBinaryCompileDate());

	//game info:
	i+=130;
	registeredObjNameList.push_back("serverTitleLabel" + intToStr(lineIndex));
	serverTitleLabel.registerGraphicComponent(containerName,"serverTitleLabel" + intToStr(lineIndex));
	serverTitleLabel.init(i,baseY-lineOffset);
	serverTitleLabel.setText(masterServerInfo.getServerTitle());

	i+=210;
	registeredObjNameList.push_back("ipAddressLabel" + intToStr(lineIndex));
	ipAddressLabel.registerGraphicComponent(containerName,"ipAddressLabel" + intToStr(lineIndex));
	ipAddressLabel.init(i,baseY-lineOffset);
	ipAddressLabel.setText(masterServerInfo.getIpAddress());

	//game setup info:
	i+=100;
	registeredObjNameList.push_back("techLabel" + intToStr(lineIndex));
	techLabel.registerGraphicComponent(containerName,"techLabel" + intToStr(lineIndex));
	techLabel.init(i,baseY-lineOffset);
	techLabel.setText(masterServerInfo.getTech());

	i+=100;
	registeredObjNameList.push_back("mapLabel" + intToStr(lineIndex));
	mapLabel.registerGraphicComponent(containerName,"mapLabel" + intToStr(lineIndex));
	mapLabel.init(i,baseY-lineOffset);
	mapLabel.setText(masterServerInfo.getMap());

	i+=100;
	registeredObjNameList.push_back("tilesetLabel" + intToStr(lineIndex));
	tilesetLabel.registerGraphicComponent(containerName,"tilesetLabel" + intToStr(lineIndex));
	tilesetLabel.init(i,baseY-lineOffset);
	tilesetLabel.setText(masterServerInfo.getTileset());

	i+=100;
	registeredObjNameList.push_back("activeSlotsLabel" + intToStr(lineIndex));
	activeSlotsLabel.registerGraphicComponent(containerName,"activeSlotsLabel" + intToStr(lineIndex));
	activeSlotsLabel.init(i,baseY-lineOffset);
	activeSlotsLabel.setText(intToStr(masterServerInfo.getActiveSlots())+"/"+intToStr(masterServerInfo.getNetworkSlots())+"/"+intToStr(masterServerInfo.getConnectedClients()));

	i+=50;
	registeredObjNameList.push_back("externalConnectPort" + intToStr(lineIndex));
	externalConnectPort.registerGraphicComponent(containerName,"externalConnectPort" + intToStr(lineIndex));
	externalConnectPort.init(i,baseY-lineOffset);
	externalConnectPort.setText(intToStr(masterServerInfo.getExternalConnectPort()));

	i+=50;
	registeredObjNameList.push_back("selectButton" + intToStr(lineIndex));
	selectButton.registerGraphicComponent(containerName,"selectButton" + intToStr(lineIndex));
	selectButton.init(i, baseY-lineOffset, 30);
	selectButton.setText(">");

	//printf("glestVersionString [%s] masterServerInfo->getGlestVersion() [%s]\n",glestVersionString.c_str(),masterServerInfo->getGlestVersion().c_str());
	bool compatible = checkVersionComptability(glestVersionString, masterServerInfo.getGlestVersion());
	selectButton.setEnabled(compatible);
	selectButton.setEditable(compatible);

	registeredObjNameList.push_back("gameFull" + intToStr(lineIndex));
	gameFull.registerGraphicComponent(containerName,"gameFull" + intToStr(lineIndex));
	gameFull.init(i, baseY-lineOffset);
	gameFull.setText(lang.get("MGGameSlotsFull"));
	gameFull.setEnabled(!compatible);
	gameFull.setEditable(!compatible);

	GraphicComponent::applyAllCustomProperties(containerName);
}

ServerLine::~ServerLine() {
	GraphicComponent::clearRegisterGraphicComponent(containerName, registeredObjNameList);
	//delete masterServerInfo;
}

bool ServerLine::buttonMouseClick(int x, int y){
	return selectButton.mouseClick(x,y);
}

bool ServerLine::buttonMouseMove(int x, int y){
	return selectButton.mouseMove(x,y);
}

void ServerLine::render() {
	Renderer &renderer= Renderer::getInstance();

	bool joinEnabled = (masterServerInfo.getNetworkSlots() > masterServerInfo.getConnectedClients());
	if(joinEnabled == true) {
		selectButton.setEnabled(true);
		selectButton.setVisible(true);

		gameFull.setEnabled(false);
		gameFull.setEditable(false);

		renderer.renderButton(&selectButton);
	}
	else {
		selectButton.setEnabled(false);
		selectButton.setVisible(false);

		gameFull.setEnabled(true);
		gameFull.setEditable(true);

		renderer.renderLabel(&gameFull);
	}

	//general info:
	renderer.renderLabel(&glestVersionLabel);
	renderer.renderLabel(&platformLabel);
	//renderer.renderLabel(&binaryCompileDateLabel);

	//game info:
	renderer.renderLabel(&serverTitleLabel);
	renderer.renderLabel(&ipAddressLabel);

	//game setup info:
	renderer.renderLabel(&techLabel);
	renderer.renderLabel(&mapLabel);
	renderer.renderLabel(&tilesetLabel);
	renderer.renderLabel(&activeSlotsLabel);
	renderer.renderLabel(&externalConnectPort);
}
}}//end namespace
