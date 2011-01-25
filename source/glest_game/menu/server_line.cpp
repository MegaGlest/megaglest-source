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
	this->countryTexture = NULL;
	Lang &lang= Lang::getInstance();

	this->lineHeight=lineHeight;
	int lineOffset = lineHeight * lineIndex;
	masterServerInfo = *mServerInfo;
	int i=10;
	this->baseY=baseY;

	//general info:
	i+=10;
	glestVersionLabel.init(i,baseY-lineOffset);
	glestVersionLabel.setText(masterServerInfo.getGlestVersion());

	i+=80;
	platformLabel.init(i,baseY-lineOffset);
	platformLabel.setText(masterServerInfo.getPlatform());

//	i+=50;
//	registeredObjNameList.push_back("binaryCompileDateLabel" + intToStr(lineIndex));
//	binaryCompileDateLabel.registerGraphicComponent(containerName,"binaryCompileDateLabel" + intToStr(lineIndex));
//	binaryCompileDateLabel.init(i,baseY-lineOffset);
//	binaryCompileDateLabel.setText(masterServerInfo.getBinaryCompileDate());

	//game info:
	i+=80;
	serverTitleLabel.init(i,baseY-lineOffset);
	serverTitleLabel.setText(masterServerInfo.getServerTitle());

	i+=80;
	country.init(i,baseY-lineOffset);
	country.setText(masterServerInfo.getCountry());

    string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
	string countryLogoPath = data_path + "data/core/misc_textures/";

	Config &config = Config::getInstance();
	if(config.getString("CountryTexturePath","") != "") {
		countryLogoPath = config.getString("CountryTexturePath","");
	}

	if(fileExists(countryLogoPath + "/" + masterServerInfo.getCountry() + ".png") == true) {
		countryTexture=GraphicsInterface::getInstance().getFactory()->newTexture2D();
		//loadingTexture = renderer.newTexture2D(rsGlobal);
		countryTexture->setMipmap(true);
		//loadingTexture->getPixmap()->load(filepath);
		countryTexture->load(countryLogoPath + "/" + masterServerInfo.getCountry() + ".png");

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		Renderer &renderer= Renderer::getInstance();
		renderer.initTexture(rsGlobal,countryTexture);
	}

	i+=80;
	status.init(i,baseY-lineOffset);
	status.setText(lang.get("MGGameStatus" + intToStr(masterServerInfo.getStatus())));

	i+=90;
	ipAddressLabel.init(i,baseY-lineOffset);
	ipAddressLabel.setText(masterServerInfo.getIpAddress());


	wrongVersionLabel.init(i,baseY-lineOffset);
	wrongVersionLabel.setText(lang.get("IncompatibleVersion"));

	//game setup info:
	i+=100;
	techLabel.init(i,baseY-lineOffset);
	techLabel.setText(masterServerInfo.getTech());

	i+=100;
	mapLabel.init(i,baseY-lineOffset);
	mapLabel.setText(masterServerInfo.getMap());

	i+=100;
	tilesetLabel.init(i,baseY-lineOffset);
	tilesetLabel.setText(masterServerInfo.getTileset());

	i+=100;
	activeSlotsLabel.init(i,baseY-lineOffset);
	activeSlotsLabel.setText(intToStr(masterServerInfo.getActiveSlots())+"/"+intToStr(masterServerInfo.getNetworkSlots())+"/"+intToStr(masterServerInfo.getConnectedClients()));

	i+=50;
	externalConnectPort.init(i,baseY-lineOffset);
	externalConnectPort.setText(intToStr(masterServerInfo.getExternalConnectPort()));

	i+=50;
	selectButton.init(i, baseY-lineOffset, 30);
	selectButton.setText(">");

	//printf("glestVersionString [%s] masterServerInfo->getGlestVersion() [%s]\n",glestVersionString.c_str(),masterServerInfo->getGlestVersion().c_str());
	compatible = checkVersionComptability(glestVersionString, masterServerInfo.getGlestVersion());
	selectButton.setEnabled(compatible);
	selectButton.setEditable(compatible);

	gameFull.init(i, baseY-lineOffset);
	gameFull.setText(lang.get("MGGameSlotsFull"));
	gameFull.setEnabled(!compatible);
	gameFull.setEditable(!compatible);

}

ServerLine::~ServerLine() {
	//delete masterServerInfo;

	if(countryTexture != NULL) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		countryTexture->end();
		delete countryTexture;

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//delete loadingTexture;
		countryTexture=NULL;
	}
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
	if(countryTexture != NULL) {
		renderer.renderTextureQuad(country.getX() + 20,country.getY(),countryTexture->getTextureWidth(),countryTexture->getTextureHeight(),countryTexture,0.7f);
	}
	else {
		renderer.renderLabel(&country);
	}

	renderer.renderLabel(&status);
	if(gameFull.getEnabled() == false) {
		if (compatible) {
			renderer.renderLabel(&ipAddressLabel);

			//game setup info:
			renderer.renderLabel(&techLabel);
			renderer.renderLabel(&mapLabel);
			renderer.renderLabel(&tilesetLabel);
			renderer.renderLabel(&activeSlotsLabel);
			renderer.renderLabel(&externalConnectPort);
		}
		else {
			renderer.renderLabel(&wrongVersionLabel);
		}

	}
}

void ServerLine::setY(int y) {
	selectButton.setY(y);
	gameFull.setY(y);

	//general info:
	glestVersionLabel.setY(y);
	platformLabel.setY(y);
	//binaryCompileDateLabel.setY(y);

	//game info:
	serverTitleLabel.setY(y);
	country.setY(y);
	status.setY(y);
	ipAddressLabel.setY(y);

	//game setup info:
	techLabel.setY(y);
	mapLabel.setY(y);
	tilesetLabel.setY(y);
	activeSlotsLabel.setY(y);

	externalConnectPort.setY(y);

}

}}//end namespace
