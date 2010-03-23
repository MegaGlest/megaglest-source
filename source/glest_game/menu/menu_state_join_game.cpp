// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Marti�o Figueroa
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

	serversSavedFile = serverFileName;
    if(getGameReadWritePath() != "") {
        serversSavedFile = getGameReadWritePath() + serversSavedFile;
    }

	servers.load(serversSavedFile);

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
	if(connect)
	{
		labelServerIp.setText(serverIp.getString() + "_");
		connectToServer();
	}
	else
	{
		labelServerIp.setText(config.getString("ServerIp") + "_");
	}

	chatManager.init(&console, -1);
}

void MenuStateJoinGame::mouseClick(int x, int y, MouseButton mouseButton)
{
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

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
	if(buttonReturn.mouseClick(x, y))
	{
		soundRenderer.playFx(coreData.getClickSoundA());

		if(clientInterface->getSocket() != NULL)
		{
		    if(clientInterface->isConnected() == true)
		    {
                string sQuitText = Config::getInstance().getString("NetPlayerName",Socket::getHostName().c_str()) + " has chosen to leave the game!";
                clientInterface->sendTextMessage(sQuitText,-1);
		    }
		    clientInterface->close();
		}

		mainMenu->setState(new MenuStateRoot(program, mainMenu));
    }

	//connect
	else if(buttonConnect.mouseClick(x, y))
	{
		ClientInterface* clientInterface= networkManager.getClientInterface();

		soundRenderer.playFx(coreData.getClickSoundA());
		labelInfo.setText("");

		if(clientInterface->isConnected())
		{
			clientInterface->reset();
		}
		else
		{
			connectToServer();
		}
	}

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
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

	renderer.renderChatManager(&chatManager);
	renderer.renderConsole(&console);
}

void MenuStateJoinGame::update()
{
	ClientInterface* clientInterface= NetworkManager::getInstance().getClientInterface();
	Lang &lang= Lang::getInstance();

	//update status label
	if(clientInterface->isConnected())
	{
		buttonConnect.setText(lang.get("Disconnect"));

		if(clientInterface->getAllowDownloadDataSynch() == false)
		{
		    string label = lang.get("ConnectedToServer");

            if(!clientInterface->getServerName().empty())
            {
                label = label + " " + clientInterface->getServerName();
            }

            if(clientInterface->getAllowGameDataSynchCheck() == true &&
               clientInterface->getNetworkGameDataSynchCheckOk() == false)
            {
                label = label + " - warning synch mismatch for:";
                if(clientInterface->getNetworkGameDataSynchCheckOkMap() == false)
                {
                    label = label + " map";
                }
                if(clientInterface->getNetworkGameDataSynchCheckOkTile() == false)
                {
                    label = label + " tile";
                }
                if(clientInterface->getNetworkGameDataSynchCheckOkTech() == false)
                {
                    label = label + " techtree";
                }
                if(clientInterface->getNetworkGameDataSynchCheckOkFogOfWar() == false)
                {
                    label = label + " FogOfWar == false";
                }
            }
            else if(clientInterface->getAllowGameDataSynchCheck() == true)
            {
                label += " - data synch is ok";
            }

            labelStatus.setText(label);
		}
		else
		{
		    string label = lang.get("ConnectedToServer");

            if(!clientInterface->getServerName().empty())
            {
                label = label + " " + clientInterface->getServerName();
            }

            if(clientInterface->getAllowGameDataSynchCheck() == true &&
               clientInterface->getNetworkGameDataSynchCheckOk() == false)
            {
                label = label + " - waiting to synch:";
                if(clientInterface->getNetworkGameDataSynchCheckOkMap() == false)
                {
                    label = label + " map";
                }
                if(clientInterface->getNetworkGameDataSynchCheckOkTile() == false)
                {
                    label = label + " tile";
                }
                if(clientInterface->getNetworkGameDataSynchCheckOkTech() == false)
                {
                    label = label + " techtree";
                }
                if(clientInterface->getNetworkGameDataSynchCheckOkFogOfWar() == false)
                {
                    label = label + " FogOfWar == false";
                }
            }
            else if(clientInterface->getAllowGameDataSynchCheck() == true)
            {
                label += " - data synch is ok";
            }

            labelStatus.setText(label);
		}
	}
	else
	{
		buttonConnect.setText(lang.get("Connect"));
		labelStatus.setText(lang.get("NotConnected"));
		labelInfo.setText("");
	}

	//process network messages
	if(clientInterface->isConnected())
	{
		//update lobby
		clientInterface->updateLobby();

        //call the chat manager
        chatManager.updateNetwork();

        //console
        console.update();

		//intro
		if(clientInterface->getIntroDone())
		{
			labelInfo.setText(lang.get("WaitingHost"));
			servers.setString(clientInterface->getServerName(), Ip(labelServerIp.getText()).getString());
		}

		//launch
		if(clientInterface->getLaunchGame())
		{
		    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] clientInterface->getLaunchGame() - A\n",__FILE__,__FUNCTION__);

			servers.save(serversSavedFile);

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] clientInterface->getLaunchGame() - B\n",__FILE__,__FUNCTION__);

			program->setState(new Game(program, clientInterface->getGameSettings()));

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] clientInterface->getLaunchGame() - C\n",__FILE__,__FUNCTION__);
		}
	}

    if(clientInterface->getLaunchGame()) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] clientInterface->getLaunchGame() - D\n",__FILE__,__FUNCTION__);
}

void MenuStateJoinGame::keyDown(char key){
	ClientInterface* clientInterface= NetworkManager::getInstance().getClientInterface();

	if(!clientInterface->isConnected())
	{
		if(key==vkBack){
			string text= labelServerIp.getText();

			if(text.size()>1){
				text.erase(text.end()-2);
			}

			labelServerIp.setText(text);
		}
	}
	else
	{
        //send key to the chat manager
        chatManager.keyDown(key);
	}
}

void MenuStateJoinGame::keyPress(char c){
	ClientInterface* clientInterface= NetworkManager::getInstance().getClientInterface();

	if(!clientInterface->isConnected())
	{
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
	else
	{
	    chatManager.keyPress(c);
	}
}

void MenuStateJoinGame::connectToServer()
{
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

	ClientInterface* clientInterface= NetworkManager::getInstance().getClientInterface();
	Config& config= Config::getInstance();
	Ip serverIp(labelServerIp.getText());

	clientInterface->connect(serverIp, GameConstants::serverPort);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] server - [%s]\n",__FILE__,__FUNCTION__,serverIp.getString().c_str());

	labelServerIp.setText(serverIp.getString()+'_');
	labelInfo.setText("");

	//save server ip
	config.setString("ServerIp", serverIp.getString());
	config.save();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

}}//end namespace
