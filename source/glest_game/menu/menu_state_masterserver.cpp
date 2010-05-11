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

#include "menu_state_masterserver.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_connected_game.h"
#include "menu_state_root.h"
#include "metrics.h"
#include "network_manager.h"
#include "network_message.h"
#include "auto_test.h"
#include "socket.h"
#include "masterserver_info.h"
#include <curl/curl.h>

#include "leak_dumper.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class ServerLine
// =====================================================

ServerLine::ServerLine( MasterServerInfo *mServerInfo, int lineIndex)	
{
	Lang &lang= Lang::getInstance();

	index=lineIndex;
	int lineOffset=25*lineIndex;
	masterServerInfo=mServerInfo;
	int i=50;
	int startOffset=650;
	
	//general info:
	i+=10;
	glestVersionLabel.init(i,startOffset-lineOffset);
	glestVersionLabel.setText(masterServerInfo->getGlestVersion());
	i+=80;
	platformLabel.init(i,startOffset-lineOffset);
	platformLabel.setText(masterServerInfo->getPlatform());
	i+=50;
	binaryCompileDateLabel.init(i,startOffset-lineOffset);
	binaryCompileDateLabel.setText(masterServerInfo->getBinaryCompileDate());
	
	//game info:
	i+=70;
	serverTitleLabel.init(i,startOffset-lineOffset);
	serverTitleLabel.setText(masterServerInfo->getServerTitle());
	
	i+=150;
	ipAddressLabel.init(i,startOffset-lineOffset);
	ipAddressLabel.setText(masterServerInfo->getIpAddress());
	
	//game setup info:
	i+=100;
	techLabel.init(i,startOffset-lineOffset);
	techLabel.setText(masterServerInfo->getTech());
	
	i+=100;
	mapLabel.init(i,startOffset-lineOffset);
	mapLabel.setText(masterServerInfo->getMap());
	
	i+=100;
	tilesetLabel.init(i,startOffset-lineOffset);
	tilesetLabel.setText(masterServerInfo->getTileset());
	
	i+=100;
	activeSlotsLabel.init(i,startOffset-lineOffset);
	activeSlotsLabel.setText(intToStr(masterServerInfo->getActiveSlots())+"/"+intToStr(masterServerInfo->getNetworkSlots())+"/"+intToStr(masterServerInfo->getConnectedClients()));

	i+=50;
	selectButton.init(i, startOffset-lineOffset, 30);
	selectButton.setText(">");
}

ServerLine::~ServerLine(){
	delete masterServerInfo;
}

bool ServerLine::buttonMouseClick(int x, int y){
	return selectButton.mouseClick(x,y);
}

bool ServerLine::buttonMouseMove(int x, int y){
	return selectButton.mouseMove(x,y);
}

void ServerLine::render(){
	Renderer &renderer= Renderer::getInstance();

	renderer.renderButton(&selectButton);
	//general info:
	renderer.renderLabel(&glestVersionLabel);
	renderer.renderLabel(&platformLabel);
	renderer.renderLabel(&binaryCompileDateLabel);
	
	//game info:
	renderer.renderLabel(&serverTitleLabel);
	renderer.renderLabel(&ipAddressLabel);
	
	//game setup info:
	renderer.renderLabel(&techLabel);
	renderer.renderLabel(&mapLabel); 
	renderer.renderLabel(&tilesetLabel);
	renderer.renderLabel(&activeSlotsLabel);	
}

// =====================================================
// 	class MenuStateMasterserver
// =====================================================

MenuStateMasterserver::MenuStateMasterserver(Program *program, MainMenu *mainMenu):
	MenuState(program, mainMenu, "root")
{
	Lang &lang= Lang::getInstance();

	mainMessageBox.init(lang.get("Ok"));
	mainMessageBox.setEnabled(false);
	mainMessageBoxState=0;

	// header
	labelTitle.init(330, 700);
	labelTitle.setText(lang.get("Available Servers"));

    buttonRefresh.init(450, 70, 150);
    buttonReturn.init(150, 70, 150);

	buttonRefresh.setText(lang.get("Refresh List"));
	buttonReturn.setText(lang.get("Return"));

	NetworkManager::getInstance().end();
	NetworkManager::getInstance().init(nrClient);
	updateServerInfo();
}

MenuStateMasterserver::~MenuStateMasterserver() {
	clearServerLines();
}

void MenuStateMasterserver::clearServerLines(){
	while(!serverLines.empty()){
		delete serverLines.back();
		serverLines.pop_back();
	}
}

void MenuStateMasterserver::mouseClick(int x, int y, MouseButton mouseButton){

	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	if(mainMessageBox.getEnabled()){
		int button= 1;
		if(mainMessageBox.mouseClick(x, y, button))
		{
			soundRenderer.playFx(coreData.getClickSoundA());
			if(button==1)
			{
				mainMessageBox.setEnabled(false);
			}
		}
	}
	else if(buttonRefresh.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		updateServerInfo();
    }
    else if(buttonReturn.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
    }
    else{ 
	    for(int i=0; i<serverLines.size(); ++i){
	    	if(serverLines[i]->buttonMouseClick(x, y)){
	    		soundRenderer.playFx(coreData.getClickSoundB());
	    		connectToServer(serverLines[i]->getMasterServerInfo()->getIpAddress());
	    		break;
	    	}
	    }
    }
}

void MenuStateMasterserver::mouseMove(int x, int y, const MouseState *ms){
	if (mainMessageBox.getEnabled()) {
		mainMessageBox.mouseMove(x, y);
	}
    buttonRefresh.mouseMove(x, y);
    buttonReturn.mouseMove(x, y);
    for(int i=0; i<serverLines.size(); ++i){
    	serverLines[i]->buttonMouseMove(x, y);
    }
}

void MenuStateMasterserver::render(){
	Renderer &renderer= Renderer::getInstance();

	if(mainMessageBox.getEnabled()){
		renderer.renderMessageBox(&mainMessageBox);
	}
	else
	{
		renderer.renderButton(&buttonRefresh);
		renderer.renderButton(&buttonReturn);
		renderer.renderLabel(&labelTitle);
		for(int i=0; i<serverLines.size(); ++i){
	    	serverLines[i]->render();
	    }
	}	
}

void MenuStateMasterserver::update(){
}

void MenuStateMasterserver::updateServerInfo() {
	//MasterServerInfos masterServerInfos;
	clearServerLines();

	std::string serverInfo = SystemFlags::getHTTP("http://soft-haus.com/glest/cgi-bin/mega-glest-master-query.php");

	std::vector<std::string> serverList;
	Tokenize(serverInfo,serverList,"\n");
	for(int i=0; i < serverList.size(); i++) {
		string &server = serverList[i];
		std::vector<std::string> serverEntities;
		Tokenize(server,serverEntities,"|");

		const int MIN_FIELDS_EXPECTED = 11;
		if(serverEntities.size() >= MIN_FIELDS_EXPECTED) {
			MasterServerInfo *masterServerInfo=new MasterServerInfo();

			//general info:
			masterServerInfo->setGlestVersion(serverEntities[0]);
			masterServerInfo->setPlatform(serverEntities[1]);
			masterServerInfo->setBinaryCompileDate(serverEntities[2]);

			//game info:
			masterServerInfo->setServerTitle(serverEntities[3]);
			masterServerInfo->setIpAddress(serverEntities[4]);

			//game setup info:
			masterServerInfo->setTech(serverEntities[5]);
			masterServerInfo->setMap(serverEntities[6]);
			masterServerInfo->setTileset(serverEntities[7]);
			masterServerInfo->setActiveSlots(strToInt(serverEntities[8]));
			masterServerInfo->setNetworkSlots(strToInt(serverEntities[9]));
			masterServerInfo->setConnectedClients(strToInt(serverEntities[10]));

			serverLines.push_back(new ServerLine( masterServerInfo, i));
		}
	}
/*
	for(int i=0; i<10;i++){
		MasterServerInfo *masterServerInfo=new MasterServerInfo();
		
		
			//general info:
		masterServerInfo->setGlestVersion("3.3.5-dev");
		masterServerInfo->setPlatform("Linux");
		masterServerInfo->setBinaryCompileDate("11.05.69");
		
		//game info:
		masterServerInfo->setServerTitle("Titis Server");
		masterServerInfo->setIpAddress("172.20.100.101");
		
		//game setup info:
		masterServerInfo->setTech("Indian");
		masterServerInfo->setMap("Conflict"); 
		masterServerInfo->setTileset("Autumn");
		masterServerInfo->setActiveSlots(8);
		masterServerInfo->setNetworkSlots(4);
		masterServerInfo->setConnectedClients(0);
		
		
		serverLines.push_back(new ServerLine( masterServerInfo, i));
	}
*/
	
	//masterServerInfos.push_back(masterServerInfo);
}


void MenuStateMasterserver::connectToServer(string ipString)
{
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] START  ipString='%s'\n",__FILE__,__FUNCTION__,ipString.c_str());

	ClientInterface* clientInterface= NetworkManager::getInstance().getClientInterface();
	Config& config= Config::getInstance();
	Ip serverIp(ipString);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] try to connect to [%s]\n",__FILE__,__FUNCTION__,serverIp.getString().c_str());
	clientInterface->connect(serverIp, GameConstants::serverPort);
	if(!clientInterface->isConnected())
	{
		NetworkManager::getInstance().end();
		NetworkManager::getInstance().init(nrClient);
		
		mainMessageBoxState=1;
		Lang &lang= Lang::getInstance();
		showMessageBox(lang.get("Couldnt connect"), lang.get("Connection failed"), false);
		return;
		
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] connection failed\n",__FILE__,__FUNCTION__);	
	}
	else
	{
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] connected ro [%s]\n",__FILE__,__FUNCTION__,serverIp.getString().c_str());
	
		//save server ip
		//config.setString("ServerIp", serverIp.getString());
		//config.save();
		
		mainMenu->setState(new MenuStateConnectedGame(program, mainMenu,jmMasterserver));
		
	}
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

void MenuStateMasterserver::showMessageBox(const string &text, const string &header, bool toggle){
	if(!toggle){
		mainMessageBox.setEnabled(false);
	}

	if(!mainMessageBox.getEnabled()){
		mainMessageBox.setText(text);
		mainMessageBox.setHeader(header);
		mainMessageBox.setEnabled(true);
	}
	else{
		mainMessageBox.setEnabled(false);
	}
}


}}//end namespace
