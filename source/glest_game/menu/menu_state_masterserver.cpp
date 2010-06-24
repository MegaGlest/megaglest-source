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
#include "menu_state_custom_game.h"
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

DisplayMessageFunction MenuStateMasterserver::pCB_DisplayMessage = NULL;

// =====================================================
// 	class ServerLine
// =====================================================

ServerLine::ServerLine( MasterServerInfo *mServerInfo, int lineIndex)	
{
	Lang &lang= Lang::getInstance();

	index=lineIndex;
	int lineOffset=25*lineIndex;
	masterServerInfo=mServerInfo;
	int i=10;
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
	i+=130;
	serverTitleLabel.init(i,startOffset-lineOffset);
	serverTitleLabel.setText(masterServerInfo->getServerTitle());
	
	i+=160;
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
	if(glestVersionString!=masterServerInfo->getGlestVersion())
	{
		selectButton.setEnabled(false);
		selectButton.setEditable(false);
	}
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
	MenuState(program, mainMenu, "masterserver") 
{
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	Lang &lang= Lang::getInstance();
	
	autoRefreshTime=0;
	playServerFoundSound=false;
	
	mainMessageBox.init(lang.get("Ok"));
	mainMessageBox.setEnabled(false);
	mainMessageBoxState=0;
	
	lastRefreshTimer= time(NULL);	
	

	// header
	labelTitle.init(330, 700);
	labelTitle.setText(lang.get("AvailableServers"));

	if(Config::getInstance().getString("Masterserver","") == "") {
		labelTitle.setText("*** " + lang.get("AvailableServers"));
	}

	// bottom
    buttonReturn.init(50, 70, 150);
    buttonCreateGame.init(300, 70, 150);
    buttonRefresh.init(550, 70, 150);

	buttonRefresh.setText(lang.get("RefreshList"));
	buttonReturn.setText(lang.get("Return"));
	buttonCreateGame.setText(lang.get("CustomGame"));
	labelAutoRefresh.setText(lang.get("AutoRefreshRate"));
	labelAutoRefresh.init(800,100);
	listBoxAutoRefresh.init(800,70);
	listBoxAutoRefresh.pushBackItem(lang.get("Off"));
	listBoxAutoRefresh.pushBackItem("10 s");
	listBoxAutoRefresh.pushBackItem("20 s");
	listBoxAutoRefresh.pushBackItem("30 s");
	listBoxAutoRefresh.setSelectedItemIndex(1);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	NetworkManager::getInstance().end();
	NetworkManager::getInstance().init(nrClient);
	//updateServerInfo();

	needUpdateFromServer = true;
	updateFromMasterserverThread = new SimpleTaskThread(this,0,100);
	updateFromMasterserverThread->setUniqueID(__FILE__);
	updateFromMasterserverThread->start();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

MenuStateMasterserver::~MenuStateMasterserver() {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(updateFromMasterserverThread != NULL) {
		MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		needUpdateFromServer = false;
		safeMutex.ReleaseLock();

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//BaseThread::shutdownAndWait(updateFromMasterserverThread);
		delete updateFromMasterserverThread;
		updateFromMasterserverThread = NULL;
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	clearServerLines();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] END\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateMasterserver::clearServerLines(){
	while(!serverLines.empty()){
		delete serverLines.back();
		serverLines.pop_back();
	}
}

void MenuStateMasterserver::mouseClick(int x, int y, MouseButton mouseButton){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

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
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		soundRenderer.playFx(coreData.getClickSoundB());
		//updateServerInfo();
		needUpdateFromServer = true;

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    }
    else if(buttonReturn.mouseClick(x, y)){
    	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    	MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		soundRenderer.playFx(coreData.getClickSoundB());

		//BaseThread::shutdownAndWait(updateFromMasterserverThread);
		delete updateFromMasterserverThread;
		updateFromMasterserverThread = NULL;

		safeMutex.ReleaseLock();

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		mainMenu->setState(new MenuStateRoot(program, mainMenu));

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    }
    else if(buttonCreateGame.mouseClick(x, y)){
    	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    	MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		soundRenderer.playFx(coreData.getClickSoundB());
		needUpdateFromServer = false;
		safeMutex.ReleaseLock();
		//BaseThread::shutdownAndWait(updateFromMasterserverThread);
		delete updateFromMasterserverThread;
		updateFromMasterserverThread = NULL;

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		mainMenu->setState(new MenuStateCustomGame(program, mainMenu,true,true));

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    }
    else if(listBoxAutoRefresh.mouseClick(x, y)){
    	MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		soundRenderer.playFx(coreData.getClickSoundA());
		autoRefreshTime=10*listBoxAutoRefresh.getSelectedItemIndex();
    }
    else {
    	MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
	    for(int i=0; i<serverLines.size(); ++i){
	    	if(serverLines[i]->buttonMouseClick(x, y)) {
	    		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	    		soundRenderer.playFx(coreData.getClickSoundB());
	    		string connectServerIP = serverLines[i]->getMasterServerInfo()->getIpAddress();
	    		connectToServer(connectServerIP);
	    		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	    		safeMutex.ReleaseLock();

	    		BaseThread::shutdownAndWait(updateFromMasterserverThread);
	    		delete updateFromMasterserverThread;
	    		updateFromMasterserverThread = NULL;

	    		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	    		mainMenu->setState(new MenuStateConnectedGame(program, mainMenu,jmMasterserver));
	    		break;
	    	}
	    }
    }
}

void MenuStateMasterserver::mouseMove(int x, int y, const MouseState *ms){
	MutexSafeWrapper safeMutex(&masterServerThreadAccessor);

	if (mainMessageBox.getEnabled()) {
		mainMessageBox.mouseMove(x, y);
	}
    buttonRefresh.mouseMove(x, y);
    buttonReturn.mouseMove(x, y);
    buttonCreateGame.mouseMove(x, y);
    listBoxAutoRefresh.mouseMove(x, y);
        
    for(int i=0; i<serverLines.size(); ++i){
    	serverLines[i]->buttonMouseMove(x, y);
    }
}

void MenuStateMasterserver::render(){
	Renderer &renderer= Renderer::getInstance();

	MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
	if(mainMessageBox.getEnabled()){
		renderer.renderMessageBox(&mainMessageBox);
	}
	else
	{
		renderer.renderButton(&buttonRefresh);
		renderer.renderButton(&buttonReturn);
		renderer.renderLabel(&labelTitle);
		renderer.renderLabel(&labelAutoRefresh);
		renderer.renderButton(&buttonCreateGame);
		renderer.renderListBox(&listBoxAutoRefresh);
		
		for(int i=0; i<serverLines.size(); ++i){
	    	serverLines[i]->render();
	    }
	}	
}

void MenuStateMasterserver::update(){
	MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
	if(autoRefreshTime!=0 && difftime(time(NULL),lastRefreshTimer) >= autoRefreshTime ) {
		needUpdateFromServer = true;
		lastRefreshTimer= time(NULL);
	}
	
	if(playServerFoundSound)
	{
		SoundRenderer::getInstance().playFx(CoreData::getInstance().getAttentionSound());
		playServerFoundSound=false;
	}

	if(threadedErrorMsg != "") {
		std::string sError = threadedErrorMsg;
		threadedErrorMsg = "";

		if(pCB_DisplayMessage != NULL) {
			pCB_DisplayMessage(sError.c_str(),false);
		}
		else {
			throw runtime_error(sError.c_str());
		}
	}
}

void MenuStateMasterserver::simpleTask() {
	if( updateFromMasterserverThread == NULL ||
		updateFromMasterserverThread->getQuitStatus() == true) {
		return;
	}
	MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
	if(needUpdateFromServer == true) {
		updateServerInfo();
	}
}

void MenuStateMasterserver::updateServerInfo() {
	try {
		needUpdateFromServer = false;

		int numberOfOldServerLines=serverLines.size();
		clearServerLines();


		if(Config::getInstance().getString("Masterserver","") != "") {
			std::string serverInfo = SystemFlags::getHTTP(Config::getInstance().getString("Masterserver")+"showServersForGlest.php");

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

					//printf("Getting Ping time for host %s\n",masterServerInfo->getIpAddress().c_str());
					float pingTime = Socket::getAveragePingMS(masterServerInfo->getIpAddress().c_str(),1);
					//printf("Ping time = %f\n",pingTime);
					char szBuf[1024]="";
					sprintf(szBuf,"%s, %.2fms",masterServerInfo->getServerTitle().c_str(),pingTime);
					masterServerInfo->setServerTitle(szBuf);

					serverLines.push_back(new ServerLine( masterServerInfo, i));
				}
			}
		}
		
		if(serverLines.size()>numberOfOldServerLines)
		{
			playServerFoundSound=true;
		}
	}
	catch(const exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d, error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		threadedErrorMsg = e.what();
	}
}


bool MenuStateMasterserver::connectToServer(string ipString)
{
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] START  ipString='%s'\n",__FILE__,__FUNCTION__,ipString.c_str());

	ClientInterface* clientInterface= NetworkManager::getInstance().getClientInterface();
	Config& config= Config::getInstance();
	Ip serverIp(ipString);

	int serverPort = Config::getInstance().getInt("ServerPort",intToStr(GameConstants::serverPort).c_str());
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] try to connect to [%s] serverPort = %d\n",__FILE__,__FUNCTION__,serverIp.getString().c_str(),serverPort);
	clientInterface->connect(serverIp, serverPort);
	if(!clientInterface->isConnected())
	{
		NetworkManager::getInstance().end();
		NetworkManager::getInstance().init(nrClient);
		
		mainMessageBoxState=1;
		Lang &lang= Lang::getInstance();
		showMessageBox(lang.get("Couldnt connect"), lang.get("Connection failed"), false);
		return false;
		
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] connection failed\n",__FILE__,__FUNCTION__);	
	}
	else
	{
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] connected to [%s]\n",__FILE__,__FUNCTION__,serverIp.getString().c_str());
	
		//save server ip
		//config.setString("ServerIp", serverIp.getString());
		//config.save();

		return true;
		
	}
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
