// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "menu_state_join_game.h"

#include "menu_state_connected_game.h"
#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_new_game.h"
#include "menu_state_custom_game.h"
#include "metrics.h"
#include "network_manager.h"
#include "network_message.h"
#include "client_interface.h"
#include "conversion.h"
#include "game.h"
#include "string_utils.h"
#include "socket.h"

#include "leak_dumper.h"

namespace Glest{ namespace Game{

using namespace Shared::Util;

// ===============================
// 	class MenuStateJoinGame
// ===============================

const int MenuStateJoinGame::newServerIndex= 0;
const int MenuStateJoinGame::newPrevServerIndex= 1;
const int MenuStateJoinGame::foundServersIndex= 2;

const string MenuStateJoinGame::serverFileName= "servers.ini";

MenuStateJoinGame::MenuStateJoinGame(Program *program, MainMenu *mainMenu, bool *autoFindHost) :
			MenuState(program, mainMenu, "join-game") {
	CommonInit(false,Ip(),-1);

	if(autoFindHost != NULL && *autoFindHost == true) {
		//if(clientInterface->isConnected() == false) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			buttonAutoFindServers.setEnabled(false);
			buttonConnect.setEnabled(false);

			NetworkManager &networkManager= NetworkManager::getInstance();
			ClientInterface* clientInterface= networkManager.getClientInterface();
			clientInterface->discoverServers(this);
		//}
	}
}
MenuStateJoinGame::MenuStateJoinGame(Program *program, MainMenu *mainMenu,
		bool connect, Ip serverIp,int portNumberOverride) :
	MenuState(program, mainMenu, "join-game") {
	CommonInit(connect, serverIp,portNumberOverride);
}

void MenuStateJoinGame::CommonInit(bool connect, Ip serverIp,int portNumberOverride) {
	containerName = "JoinGame";
	abortAutoFind = false;
	autoConnectToServer = false;
	Lang &lang= Lang::getInstance();
	Config &config= Config::getInstance();
	NetworkManager &networkManager= NetworkManager::getInstance();
	networkManager.end();
	networkManager.init(nrClient);

	serversSavedFile = serverFileName;
    string userData = config.getString("UserData_Root","");
    if(userData != "") {
    	endPathWithSlash(userData);
    }
    serversSavedFile = userData + serversSavedFile;

    if(fileExists(serversSavedFile) == true) {
    	servers.load(serversSavedFile);
    }

	//buttons
	buttonReturn.registerGraphicComponent(containerName,"buttonReturn");
	buttonReturn.init(300, 300, 125);
	buttonReturn.setText(lang.get("Return"));

	buttonConnect.registerGraphicComponent(containerName,"buttonConnect");
	buttonConnect.init(450, 300, 125);
	buttonConnect.setText(lang.get("Connect"));

	buttonCreateGame.registerGraphicComponent(containerName,"buttonCreateGame");
	buttonCreateGame.init(450, 250, 125);
	buttonCreateGame.setText(lang.get("HostGame"));

	buttonAutoFindServers.registerGraphicComponent(containerName,"buttonAutoFindServers");
	buttonAutoFindServers.init(595, 300, 225);
	buttonAutoFindServers.setText(lang.get("FindLANGames"));
	buttonAutoFindServers.setEnabled(true);

	//server type label
	labelServerType.registerGraphicComponent(containerName,"labelServerType");
	labelServerType.init(330, 490);
	labelServerType.setText(lang.get("ServerType") + ":");

	//server type list box
	listBoxServerType.registerGraphicComponent(containerName,"listBoxServerType");
	listBoxServerType.init(465, 490);
	listBoxServerType.pushBackItem(lang.get("ServerTypeNew"));
	listBoxServerType.pushBackItem(lang.get("ServerTypePrevious"));
	listBoxServerType.pushBackItem(lang.get("ServerTypeFound"));

	//server label
	labelServer.registerGraphicComponent(containerName,"labelServer");
	labelServer.init(330, 460);
	labelServer.setText(lang.get("Server") + ": ");

	//server listbox
	listBoxServers.registerGraphicComponent(containerName,"listBoxServers");
	listBoxServers.init(465, 460);
	for(int i= 0; i<servers.getPropertyCount(); ++i){
		listBoxServers.pushBackItem(servers.getKey(i));
	}

	// found servers listbox
	listBoxFoundServers.registerGraphicComponent(containerName,"listBoxFoundServers");
	listBoxFoundServers.init(465, 460);

	//server ip
	labelServerIp.registerGraphicComponent(containerName,"labelServerIp");
	labelServerIp.init(465, 460);

	// server port
	labelServerPortLabel.registerGraphicComponent(containerName,"labelServerPortLabel");
	labelServerPortLabel.init(330,430);
	labelServerPortLabel.setText(lang.get("ServerPort"));

	labelServerPort.registerGraphicComponent(containerName,"labelServerPort");
	labelServerPort.init(465,430);

	string host = labelServerIp.getText();
	int portNumber = config.getInt("PortServer",intToStr(GameConstants::serverPort).c_str());
	std::vector<std::string> hostPartsList;
	Tokenize(host,hostPartsList,":");
	if(hostPartsList.size() > 1) {
		host = hostPartsList[0];
		replaceAll(hostPartsList[1],"_","");
		portNumber = strToInt(hostPartsList[1]);
	}

	string port = " ("+intToStr(portNumber)+")";
	labelServerPort.setText(port);

	labelStatus.registerGraphicComponent(containerName,"labelStatus");
	labelStatus.init(330, 400);
	labelStatus.setText("");

	labelInfo.registerGraphicComponent(containerName,"labelInfo");
	labelInfo.init(330, 370);
	labelInfo.setText("");

	connected= false;
	playerIndex= -1;

	//server ip
	if(connect == true) 	{
		string hostIP = serverIp.getString();
		if(portNumberOverride > 0) {
			hostIP += ":" + intToStr(portNumberOverride);
		}

		labelServerIp.setText(hostIP + "_");

		autoConnectToServer = true;
	}
	else {
		string hostIP = config.getString("ServerIp");
		if(portNumberOverride > 0) {
			hostIP += ":" + intToStr(portNumberOverride);
		}

		labelServerIp.setText(hostIP + "_");
	}

	host = labelServerIp.getText();
	portNumber = config.getInt("PortServer",intToStr(GameConstants::serverPort).c_str());
	hostPartsList.clear();
	Tokenize(host,hostPartsList,":");
	if(hostPartsList.size() > 1) {
		host = hostPartsList[0];
		replaceAll(hostPartsList[1],"_","");
		portNumber = strToInt(hostPartsList[1]);
	}

	port = " ("+intToStr(portNumber)+")";
	labelServerPort.setText(port);

	GraphicComponent::applyAllCustomProperties(containerName);

	chatManager.init(&console, -1);
}

void MenuStateJoinGame::reloadUI() {
	Lang &lang= Lang::getInstance();
	Config &config= Config::getInstance();

	console.resetFonts();

	buttonReturn.setText(lang.get("Return"));
	buttonConnect.setText(lang.get("Connect"));
	buttonCreateGame.setText(lang.get("HostGame"));
	buttonAutoFindServers.setText(lang.get("FindLANGames"));
	labelServerType.setText(lang.get("ServerType") + ":");

	std::vector<string> listboxData;
	listboxData.push_back(lang.get("ServerTypeNew"));
	listboxData.push_back(lang.get("ServerTypePrevious"));
	listboxData.push_back(lang.get("ServerTypeFound"));
	listBoxServerType.setItems(listboxData);

	labelServer.setText(lang.get("Server") + ": ");

	labelServerPortLabel.setText(lang.get("ServerPort"));

	string host = labelServerIp.getText();
	int portNumber = config.getInt("PortServer",intToStr(GameConstants::serverPort).c_str());
	std::vector<std::string> hostPartsList;
	Tokenize(host,hostPartsList,":");
	if(hostPartsList.size() > 1) {
		host = hostPartsList[0];
		replaceAll(hostPartsList[1],"_","");
		portNumber = strToInt(hostPartsList[1]);
	}

	string port = " ("+intToStr(portNumber)+")";
	labelServerPort.setText(port);

	chatManager.init(&console, -1);

	GraphicComponent::reloadFontsForRegisterGraphicComponents(containerName);
}

MenuStateJoinGame::~MenuStateJoinGame() {
	abortAutoFind = true;
}

void MenuStateJoinGame::DiscoveredServers(std::vector<string> serverList) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(abortAutoFind == true) {
		return;
	}
	// Testing multi-server
	//serverList.push_back("test1");
	//serverList.push_back("test2");
	//

	autoConnectToServer = false;
	buttonAutoFindServers.setEnabled(true);
	buttonConnect.setEnabled(true);
	if(serverList.empty() == false) {
		Config &config= Config::getInstance();
		string bestIPMatch = "";
		int serverGamePort = config.getInt("PortServer",intToStr(GameConstants::serverPort).c_str());
		std::vector<std::string> localIPList = Socket::getLocalIPAddressList();

		for(int idx = 0; idx < serverList.size(); idx++) {

			vector<string> paramPartPortsTokens;
			Tokenize(serverList[idx],paramPartPortsTokens,":");
			if(paramPartPortsTokens.size() >= 2 && paramPartPortsTokens[1].length() > 0) {
				serverGamePort = strToInt(paramPartPortsTokens[1]);
			}

			bestIPMatch = serverList[idx];

			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] bestIPMatch = [%s] localIPList[0] = [%s]\n",__FILE__,__FUNCTION__,__LINE__,bestIPMatch.c_str(),localIPList[0].c_str());

			if(strncmp(localIPList[0].c_str(),serverList[idx].c_str(),4) == 0) {
				break;
			}
		}

		if(bestIPMatch != "") {
			bestIPMatch += ":" + intToStr(serverGamePort);
		}
		labelServerIp.setText(bestIPMatch);

		if(serverList.size() > 1) {
			listBoxServerType.setSelectedItemIndex(MenuStateJoinGame::foundServersIndex);
			listBoxFoundServers.setItems(serverList);
		}
		else {
			autoConnectToServer = true;
		}
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateJoinGame::mouseClick(int x, int y, MouseButton mouseButton) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	NetworkManager &networkManager= NetworkManager::getInstance();
	ClientInterface* clientInterface= networkManager.getClientInterface();

	if(clientInterface->isConnected() == false) {
		//server type
		if(listBoxServerType.mouseClick(x, y)){
			if(!listBoxServers.getText().empty()){
				labelServerIp.setText(servers.getString(listBoxServers.getText())+"_");
			}
		}
		//server list
		else if(listBoxServerType.getSelectedItemIndex() == newPrevServerIndex){
			if(listBoxServers.mouseClick(x, y)) {
				labelServerIp.setText(servers.getString(listBoxServers.getText())+"_");
			}
		}
		else if(listBoxServerType.getSelectedItemIndex() == foundServersIndex){
			if(listBoxFoundServers.mouseClick(x, y)) {
				labelServerIp.setText(listBoxFoundServers.getText());
			}
		}

		string host = labelServerIp.getText();
		Config &config= Config::getInstance();
		int portNumber = config.getInt("PortServer",intToStr(GameConstants::serverPort).c_str());
		std::vector<std::string> hostPartsList;
		Tokenize(host,hostPartsList,":");
		if(hostPartsList.size() > 1) {
			host = hostPartsList[0];
			replaceAll(hostPartsList[1],"_","");
			portNumber = strToInt(hostPartsList[1]);
		}

		string port = " ("+intToStr(portNumber)+")";
		labelServerPort.setText(port);

	}

	//return
	if(buttonReturn.mouseClick(x, y)) {
		soundRenderer.playFx(coreData.getClickSoundA());

		clientInterface->stopServerDiscovery();

		if(clientInterface->getSocket() != NULL) {
		    //if(clientInterface->isConnected() == true) {
            //    string sQuitText = Config::getInstance().getString("NetPlayerName",Socket::getHostName().c_str()) + " has chosen to leave the game!";
            //    clientInterface->sendTextMessage(sQuitText,-1);
		    //}
		    clientInterface->close();
		}
		abortAutoFind = true;
		mainMenu->setState(new MenuStateNewGame(program, mainMenu));
    }

	//connect
	else if(buttonConnect.mouseClick(x, y) && buttonConnect.getEnabled() == true) {
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
    else if(buttonCreateGame.mouseClick(x, y)){
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		soundRenderer.playFx(coreData.getClickSoundB());
		clientInterface->stopServerDiscovery();
		if(clientInterface->getSocket() != NULL) {
		    clientInterface->close();
		}
		abortAutoFind = true;
		mainMenu->setState(new MenuStateCustomGame(program, mainMenu,true,pLanGame));
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    }

	else if(buttonAutoFindServers.mouseClick(x, y) && buttonAutoFindServers.getEnabled() == true) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		ClientInterface* clientInterface= networkManager.getClientInterface();
		soundRenderer.playFx(coreData.getClickSoundA());

		// Triggers a thread which calls back into MenuStateJoinGame::DiscoveredServers
		// with the results
		if(clientInterface->isConnected() == false) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			buttonAutoFindServers.setEnabled(false);
			buttonConnect.setEnabled(false);
			clientInterface->discoverServers(this);
		}
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

void MenuStateJoinGame::mouseMove(int x, int y, const MouseState *ms){
	buttonReturn.mouseMove(x, y);
	buttonConnect.mouseMove(x, y);
	buttonAutoFindServers.mouseMove(x, y);
	buttonCreateGame.mouseMove(x, y);
	listBoxServerType.mouseMove(x, y);

	//hide-show options depending on the selection
	if(listBoxServers.getSelectedItemIndex() == newServerIndex) {
		labelServerIp.mouseMove(x, y);
	}
	else if(listBoxServers.getSelectedItemIndex() == newPrevServerIndex) {
		listBoxServers.mouseMove(x, y);
	}
	else {
		listBoxFoundServers.mouseMove(x, y);
	}
}

void MenuStateJoinGame::render(){
	Renderer &renderer= Renderer::getInstance();

	renderer.renderButton(&buttonReturn);
	renderer.renderLabel(&labelServer);
	renderer.renderLabel(&labelServerType);
	renderer.renderLabel(&labelStatus);
	renderer.renderLabel(&labelInfo);
	renderer.renderLabel(&labelServerPort);
	renderer.renderLabel(&labelServerPortLabel);
	renderer.renderButton(&buttonConnect);
	renderer.renderButton(&buttonCreateGame);

	renderer.renderButton(&buttonAutoFindServers);
	renderer.renderListBox(&listBoxServerType);

	if(listBoxServerType.getSelectedItemIndex() == newServerIndex) {
		renderer.renderLabel(&labelServerIp);
	}
	else if(listBoxServerType.getSelectedItemIndex() == newPrevServerIndex) {
		renderer.renderListBox(&listBoxServers);
	}
	else {
		renderer.renderListBox(&listBoxFoundServers);
	}

	renderer.renderChatManager(&chatManager);
	renderer.renderConsole(&console);

	if(program != NULL) program->renderProgramMsgBox();
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
		string connectedStatus = lang.get("NotConnected");
		if(buttonAutoFindServers.getEnabled() == false) {
			connectedStatus += " - searching for servers, please wait...";
		}
		labelStatus.setText(connectedStatus);
		labelInfo.setText("");
	}

	//process network messages
	if(clientInterface->isConnected()) {
		//update lobby
		clientInterface->updateLobby();

		clientInterface= NetworkManager::getInstance().getClientInterface();
		if(clientInterface != NULL && clientInterface->isConnected()) {
			//call the chat manager
			chatManager.updateNetwork();

			//console
			console.update();

			//intro
			if(clientInterface->getIntroDone()) {
				labelInfo.setText(lang.get("WaitingHost"));

				string host = labelServerIp.getText();
				std::vector<std::string> hostPartsList;
				Tokenize(host,hostPartsList,":");
				if(hostPartsList.size() > 1) {
					host = hostPartsList[0];
					replaceAll(hostPartsList[1],"_","");
				}
				string saveHost = Ip(host).getString();
				if(hostPartsList.size() > 1) {
					saveHost += ":" + hostPartsList[1];
				}

				servers.setString(clientInterface->getServerName(), saveHost);
			}

			//launch
			if(clientInterface->getLaunchGame())
			{
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] clientInterface->getLaunchGame() - A\n",__FILE__,__FUNCTION__);

				servers.save(serversSavedFile);

				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] clientInterface->getLaunchGame() - B\n",__FILE__,__FUNCTION__);

				abortAutoFind = true;
				clientInterface->stopServerDiscovery();
				program->setState(new Game(program, clientInterface->getGameSettings(),false));

				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] clientInterface->getLaunchGame() - C\n",__FILE__,__FUNCTION__);
			}
		}
	}
	else if(autoConnectToServer == true) {
		autoConnectToServer = false;
		connectToServer();
	}

    if(clientInterface != NULL && clientInterface->getLaunchGame()) if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] clientInterface->getLaunchGame() - D\n",__FILE__,__FUNCTION__);
}

void MenuStateJoinGame::keyDown(SDL_KeyboardEvent key) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c][%d]\n",__FILE__,__FUNCTION__,__LINE__,key.keysym.sym,key.keysym.sym);

	ClientInterface* clientInterface= NetworkManager::getInstance().getClientInterface();
	if(clientInterface->isConnected() == false)	{
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));

		string text = labelServerIp.getText();
		if(isKeyPressed(SDLK_BACKSPACE,key) == true && text.length() > 0) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			size_t found = text.find_last_of("_");
			if (found == string::npos) {
				text.erase(text.end() - 1);
			}
			else {
				if(text.size() > 1) {
					text.erase(text.end() - 2);
				}
			}

			labelServerIp.setText(text);
		}
		//else if(key == configKeys.getCharKey("SaveGUILayout")) {
		else if(isKeyPressed(configKeys.getSDLKey("SaveGUILayout"),key) == true) {
			bool saved = GraphicComponent::saveAllCustomProperties(containerName);
			Lang &lang= Lang::getInstance();
			console.addLine(lang.get("GUILayoutSaved") + " [" + (saved ? lang.get("Yes") : lang.get("No"))+ "]");
		}
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

        //send key to the chat manager
        chatManager.keyDown(key);

        if(chatManager.getEditEnabled() == false) {
        	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
			//if(key == configKeys.getCharKey("SaveGUILayout")) {
        	if(isKeyPressed(configKeys.getSDLKey("SaveGUILayout"),key) == true) {
				bool saved = GraphicComponent::saveAllCustomProperties(containerName);
				Lang &lang= Lang::getInstance();
				console.addLine(lang.get("GUILayoutSaved") + " [" + (saved ? lang.get("Yes") : lang.get("No"))+ "]");
			}
        }
	}
}

void MenuStateJoinGame::keyPress(SDL_KeyboardEvent c) {
	ClientInterface* clientInterface= NetworkManager::getInstance().getClientInterface();

	if(clientInterface->isConnected() == false)	{
		int maxTextSize= 16;

		//Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));

		SDLKey key = extractKeyPressed(c);

		//if(c>='0' && c<='9') {
		if( (key >= SDLK_0 && key <= SDLK_9) ||
			(key >= SDLK_KP0 && key <= SDLK_KP9)) {
			if(labelServerIp.getText().size() < maxTextSize) {
				string text= labelServerIp.getText();
				//text.insert(text.end()-1, key);
				char szCharText[20]="";
				snprintf(szCharText,20,"%c",key);
				char *utfStr = String::ConvertToUTF8(&szCharText[0]);
				if(text.size() > 0) {
					text.insert(text.end() -1, utfStr[0]);
				}
				else {
					text = utfStr[0];
				}

				delete [] utfStr;

				labelServerIp.setText(text);
			}
		}
		//else if (c=='.') {
		else if (key == SDLK_PERIOD) {
			if(labelServerIp.getText().size() < maxTextSize) {
				string text= labelServerIp.getText();
				if(text.size() > 0) {
					text.insert(text.end() -1, '.');
				}
				else {
					text = ".";
				}

				labelServerIp.setText(text);
			}
		}
	}
	else {
	    chatManager.keyPress(c);
	}
}

void MenuStateJoinGame::connectToServer() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

	Config& config= Config::getInstance();
	string host = labelServerIp.getText();
	int port = config.getInt("PortServer",intToStr(GameConstants::serverPort).c_str());
	std::vector<std::string> hostPartsList;
	Tokenize(host,hostPartsList,":");
	if(hostPartsList.size() > 1) {
		host = hostPartsList[0];
		replaceAll(hostPartsList[1],"_","");
		port = strToInt(hostPartsList[1]);
	}
	Ip serverIp(host);

	ClientInterface* clientInterface= NetworkManager::getInstance().getClientInterface();
	clientInterface->connect(serverIp, port);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] server - [%s]\n",__FILE__,__FUNCTION__,serverIp.getString().c_str());

	labelServerIp.setText(serverIp.getString() + '_');
	labelInfo.setText("");

	//save server ip
	if(config.getString("ServerIp") != serverIp.getString()) {
		config.setString("ServerIp", serverIp.getString());
		config.save();
	}

	for(time_t elapsedWait = time(NULL);
		clientInterface->getIntroDone() == false &&
		clientInterface->isConnected() &&
		difftime(time(NULL),elapsedWait) <= 8;) {
		if(clientInterface->isConnected()) {
			//update lobby
			clientInterface->updateLobby();
			sleep(0);
			//this->render();
		}
	}
	if( clientInterface->isConnected() == true &&
		clientInterface->getIntroDone() == true) {

		string saveHost = Ip(host).getString();
		if(hostPartsList.size() > 1) {
			saveHost += ":" + hostPartsList[1];
		}
		servers.setString(clientInterface->getServerName(), saveHost);
		servers.save(serversSavedFile);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] Using FTP port #: %d\n",__FILE__,__FUNCTION__,__LINE__,clientInterface->getServerFTPPort());
		abortAutoFind = true;
		clientInterface->stopServerDiscovery();
		mainMenu->setState(new MenuStateConnectedGame(program, mainMenu));
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

}}//end namespace
