// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#include "menu_state_join_game.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_root.h"
#include "metrics.h"
#include "network_manager.h"
#include "network_message.h"
#include "client_interface.h"
#include "conversion.h"
#include "game.h"
#include "socket.h"

#include "leak_dumper.h"

namespace Glest{ namespace Game{

using namespace Shared::Util;

// ===============================
// 	class MenuStateJoinGame  
// ===============================

const int MenuStateJoinGame::newServerIndex= 0;
const string MenuStateJoinGame::serverFileName= "servers.ini";

MenuStateJoinGame::MenuStateJoinGame(Program *program, MainMenu *mainMenu, bool connect, Ip serverIp):
	MenuState(program, mainMenu, "join-game")
{
	Lang &lang= Lang::getInstance();
	Config &config= Config::getInstance();
	NetworkManager &networkManager= NetworkManager::getInstance();

	servers.load(serverFileName);

	//buttons
	buttonReturn.init(325, 300, 125);
	buttonReturn.setText(lang.get("Return"));

	buttonConnect.init(475, 300, 125);
	buttonConnect.setText(lang.get("Connect"));

	//server type label
	labelServerType.init(330, 460);
	labelServerType.setText(lang.get("ServerType") + ":");

	//server type list box
	listBoxServerType.init(465, 460);
	listBoxServerType.pushBackItem(lang.get("ServerTypeNew"));
	listBoxServerType.pushBackItem(lang.get("ServerTypePrevious"));

	//server label
	labelServer.init(330, 430);
	labelServer.setText(lang.get("Server") + ": ");

	//server listbox
	listBoxServers.init(465, 430);

	for(int i= 0; i<servers.getPropertyCount(); ++i){
		listBoxServers.pushBackItem(servers.getKey(i));
	}

	//server ip
	labelServerIp.init(465, 430);

	labelStatus.init(330, 400);
	labelStatus.setText("");

	labelInfo.init(330, 370);
	labelInfo.setText("");

	networkManager.init(nrClient);
	connected= false;
	playerIndex= -1;

	//server ip
	if(connect){
		labelServerIp.setText(serverIp.getString() + "_");
		connectToServer();
	}
	else
	{
		labelServerIp.setText(config.getString("ServerIp") + "_");
	}
}

void MenuStateJoinGame::mouseClick(int x, int y, MouseButton mouseButton){

	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	NetworkManager &networkManager= NetworkManager::getInstance();
	ClientInterface* clientInterface= networkManager.getClientInterface();

	if(!clientInterface->isConnected()){
		//server type
		if(listBoxServerType.mouseClick(x, y)){
			if(!listBoxServers.getText().empty()){
				labelServerIp.setText(servers.getString(listBoxServers.getText())+"_");
			}
		}

		//server list
		else if(listBoxServerType.getSelectedItemIndex()!=newServerIndex){
			if(listBoxServers.mouseClick(x, y)){
				labelServerIp.setText(servers.getString(listBoxServers.getText())+"_");
			}
		}
	}

	//return
	if(buttonReturn.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
    }  

	//connect
	else if(buttonConnect.mouseClick(x, y)){
		ClientInterface* clientInterface= networkManager.getClientInterface();

		soundRenderer.playFx(coreData.getClickSoundA());
		labelInfo.setText("");
			
		if(clientInterface->isConnected()){
			clientInterface->reset();
		}
		else{
			connectToServer();
		}
	}
}

void MenuStateJoinGame::mouseMove(int x, int y, const MouseState *ms){
	buttonReturn.mouseMove(x, y);
	buttonConnect.mouseMove(x, y);
	listBoxServerType.mouseMove(x, y);

	//hide-show options depending on the selection
	if(listBoxServers.getSelectedItemIndex()==newServerIndex){
		labelServerIp.mouseMove(x, y);
	}
	else{
		listBoxServers.mouseMove(x, y);
	}
}

void MenuStateJoinGame::render(){
	Renderer &renderer= Renderer::getInstance();

	renderer.renderButton(&buttonReturn);
	renderer.renderLabel(&labelServer);
	renderer.renderLabel(&labelServerType);
	renderer.renderLabel(&labelStatus);
	renderer.renderLabel(&labelInfo);
	renderer.renderButton(&buttonConnect);
	renderer.renderListBox(&listBoxServerType);
	
	if(listBoxServerType.getSelectedItemIndex()==newServerIndex){
		renderer.renderLabel(&labelServerIp);
	}
	else
	{
		renderer.renderListBox(&listBoxServers);
	}
}

void MenuStateJoinGame::update(){
	ClientInterface* clientInterface= NetworkManager::getInstance().getClientInterface();
	Lang &lang= Lang::getInstance();

	//update status label
	if(clientInterface->isConnected()){
		buttonConnect.setText(lang.get("Disconnect"));
		if(!clientInterface->getServerName().empty()){
			labelStatus.setText(lang.get("ConnectedToServer") + " " + clientInterface->getServerName());
		}
		else{
			labelStatus.setText(lang.get("ConnectedToServer"));
		}
	}
	else{
		buttonConnect.setText(lang.get("Connect"));
		labelStatus.setText(lang.get("NotConnected"));
		labelInfo.setText("");
	}

	//process network messages
	if(clientInterface->isConnected()){

		//update lobby
		clientInterface->updateLobby();
		
		//intro
		if(clientInterface->getIntroDone()){
			labelInfo.setText(lang.get("WaitingHost"));		
			servers.setString(clientInterface->getServerName(), Ip(labelServerIp.getText()).getString());
		}

		//launch
		if(clientInterface->getLaunchGame()){
			servers.save(serverFileName);
			program->setState(new Game(program, clientInterface->getGameSettings())); 
		}
	}
}

void MenuStateJoinGame::keyDown(char key){
	ClientInterface* clientInterface= NetworkManager::getInstance().getClientInterface();
	
	if(!clientInterface->isConnected()){
		if(key==vkBack){
			string text= labelServerIp.getText();

			if(text.size()>1){
				text.erase(text.end()-2);
			}

			labelServerIp.setText(text);
		}
	}
}

void MenuStateJoinGame::keyPress(char c){
	ClientInterface* clientInterface= NetworkManager::getInstance().getClientInterface();
	
	if(!clientInterface->isConnected()){
		int maxTextSize= 16;

		if(c>='0' && c<='9'){

			if(labelServerIp.getText().size()<maxTextSize){
				string text= labelServerIp.getText();

				text.insert(text.end()-1, c);

				labelServerIp.setText(text);
			}
		}
		else if (c=='.'){
			if(labelServerIp.getText().size()<maxTextSize){
				string text= labelServerIp.getText();

				text.insert(text.end()-1, '.');

				labelServerIp.setText(text);
			}
		}
	}
}

void MenuStateJoinGame::connectToServer(){
	ClientInterface* clientInterface= NetworkManager::getInstance().getClientInterface();
	Config& config= Config::getInstance();
	Ip serverIp(labelServerIp.getText());

	clientInterface->connect(serverIp, GameConstants::serverPort);
	labelServerIp.setText(serverIp.getString()+'_');
	labelInfo.setText("");

	//save server ip
	config.setString("ServerIp", serverIp.getString());
	config.save();
}

}}//end namespace
