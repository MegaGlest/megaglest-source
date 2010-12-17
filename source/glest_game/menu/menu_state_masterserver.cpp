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

ServerLine::ServerLine( MasterServerInfo *mServerInfo, int lineIndex, const char * containerName) {
	this->containerName = containerName;
	Lang &lang= Lang::getInstance();

	index=lineIndex;
	int lineOffset = 25 * lineIndex;
	masterServerInfo = *mServerInfo;
	int i=10;
	int startOffset=600;
	
	//general info:
	i+=10;
	glestVersionLabel.registerGraphicComponent(containerName,"glestVersionLabel" + intToStr(lineIndex));
	registeredObjNameList.push_back("glestVersionLabel" + intToStr(lineIndex));
	glestVersionLabel.init(i,startOffset-lineOffset);
	glestVersionLabel.setText(masterServerInfo.getGlestVersion());

	i+=80;
	registeredObjNameList.push_back("platformLabel" + intToStr(lineIndex));
	platformLabel.registerGraphicComponent(containerName,"platformLabel" + intToStr(lineIndex));
	platformLabel.init(i,startOffset-lineOffset);
	platformLabel.setText(masterServerInfo.getPlatform());

//	i+=50;
//	registeredObjNameList.push_back("binaryCompileDateLabel" + intToStr(lineIndex));
//	binaryCompileDateLabel.registerGraphicComponent(containerName,"binaryCompileDateLabel" + intToStr(lineIndex));
//	binaryCompileDateLabel.init(i,startOffset-lineOffset);
//	binaryCompileDateLabel.setText(masterServerInfo.getBinaryCompileDate());
	
	//game info:
	i+=130;
	registeredObjNameList.push_back("serverTitleLabel" + intToStr(lineIndex));
	serverTitleLabel.registerGraphicComponent(containerName,"serverTitleLabel" + intToStr(lineIndex));
	serverTitleLabel.init(i,startOffset-lineOffset);
	serverTitleLabel.setText(masterServerInfo.getServerTitle());
	
	i+=210;
	registeredObjNameList.push_back("ipAddressLabel" + intToStr(lineIndex));
	ipAddressLabel.registerGraphicComponent(containerName,"ipAddressLabel" + intToStr(lineIndex));
	ipAddressLabel.init(i,startOffset-lineOffset);
	ipAddressLabel.setText(masterServerInfo.getIpAddress());
	
	//game setup info:
	i+=100;
	registeredObjNameList.push_back("techLabel" + intToStr(lineIndex));
	techLabel.registerGraphicComponent(containerName,"techLabel" + intToStr(lineIndex));
	techLabel.init(i,startOffset-lineOffset);
	techLabel.setText(masterServerInfo.getTech());
	
	i+=100;
	registeredObjNameList.push_back("mapLabel" + intToStr(lineIndex));
	mapLabel.registerGraphicComponent(containerName,"mapLabel" + intToStr(lineIndex));
	mapLabel.init(i,startOffset-lineOffset);
	mapLabel.setText(masterServerInfo.getMap());
	
	i+=100;
	registeredObjNameList.push_back("tilesetLabel" + intToStr(lineIndex));
	tilesetLabel.registerGraphicComponent(containerName,"tilesetLabel" + intToStr(lineIndex));
	tilesetLabel.init(i,startOffset-lineOffset);
	tilesetLabel.setText(masterServerInfo.getTileset());
	
	i+=100;
	registeredObjNameList.push_back("activeSlotsLabel" + intToStr(lineIndex));
	activeSlotsLabel.registerGraphicComponent(containerName,"activeSlotsLabel" + intToStr(lineIndex));
	activeSlotsLabel.init(i,startOffset-lineOffset);
	activeSlotsLabel.setText(intToStr(masterServerInfo.getActiveSlots())+"/"+intToStr(masterServerInfo.getNetworkSlots())+"/"+intToStr(masterServerInfo.getConnectedClients()));

	i+=50;
	registeredObjNameList.push_back("externalConnectPort" + intToStr(lineIndex));
	externalConnectPort.registerGraphicComponent(containerName,"externalConnectPort" + intToStr(lineIndex));
	externalConnectPort.init(i,startOffset-lineOffset);
	externalConnectPort.setText(intToStr(masterServerInfo.getExternalConnectPort()));

	i+=50;
	registeredObjNameList.push_back("selectButton" + intToStr(lineIndex));
	selectButton.registerGraphicComponent(containerName,"selectButton" + intToStr(lineIndex));
	selectButton.init(i, startOffset-lineOffset, 30);
	selectButton.setText(">");

	//printf("glestVersionString [%s] masterServerInfo->getGlestVersion() [%s]\n",glestVersionString.c_str(),masterServerInfo->getGlestVersion().c_str());
	bool compatible = checkVersionComptability(glestVersionString, masterServerInfo.getGlestVersion());
	selectButton.setEnabled(compatible);
	selectButton.setEditable(compatible);

	registeredObjNameList.push_back("gameFull" + intToStr(lineIndex));
	gameFull.registerGraphicComponent(containerName,"gameFull" + intToStr(lineIndex));
	gameFull.init(i, startOffset-lineOffset);
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

// =====================================================
// 	class MenuStateMasterserver
// =====================================================

MenuStateMasterserver::MenuStateMasterserver(Program *program, MainMenu *mainMenu):
	MenuState(program, mainMenu, "masterserver") 
{
	containerName = "MasterServer";
	updateFromMasterserverThread = NULL;
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	Lang &lang= Lang::getInstance();
	
	autoRefreshTime=0;
	playServerFoundSound=false;
	announcementLoaded=false;

	mainMessageBox.registerGraphicComponent(containerName,"mainMessageBox");
	mainMessageBox.init(lang.get("Ok"));
	mainMessageBox.setEnabled(false);
	mainMessageBoxState=0;
	
	lastRefreshTimer= time(NULL);	
	
	// announcement
	announcementLabel.registerGraphicComponent(containerName,"announcementLabel");
    announcementLabel.init(10, 730);
    announcementLabel.setFont(CoreData::getInstance().getMenuFontBig());
    announcementLabel.setText("");

    // versionInfo
    versionInfoLabel.registerGraphicComponent(containerName,"versionInfoLabel");
    versionInfoLabel.init(10, 680);
    versionInfoLabel.setFont(CoreData::getInstance().getMenuFontBig());
    versionInfoLabel.setText("");
	// header
	labelTitle.registerGraphicComponent(containerName,"labelTitle");
	labelTitle.init(330, 640);
	labelTitle.setFont(CoreData::getInstance().getMenuFontBig());
	labelTitle.setText(lang.get("AvailableServers"));

	if(Config::getInstance().getString("Masterserver","") == "") {
		labelTitle.setText("*** " + lang.get("AvailableServers"));
	}
    
	// bottom
	int buttonPos=130;
	
	labelChatUrl.registerGraphicComponent(containerName,"labelChatUrl");
	labelChatUrl.init(150,buttonPos-50);
	labelChatUrl.setFont(CoreData::getInstance().getMenuFontBig());
	labelChatUrl.setText(lang.get("NoServerVisitChat")+":     http://webchat.freenode.net/?channels=glest");

	// Titles for current games - START
	int lineIndex = 0;
	int lineOffset=25*lineIndex;
	int i=10;
	int startOffset=623;

	//general info:
	i+=10;
	glestVersionLabel.registerGraphicComponent(containerName,"glestVersionLabel");
	glestVersionLabel.init(i,startOffset-lineOffset);
	glestVersionLabel.setText(lang.get("MGVersion"));

	i+=80;
	platformLabel.registerGraphicComponent(containerName,"platformLabel");
	platformLabel.init(i,startOffset-lineOffset);
	platformLabel.setText(lang.get("MGPlatform"));

//	i+=50;
//	binaryCompileDateLabel.registerGraphicComponent(containerName,"binaryCompileDateLabel");
//	binaryCompileDateLabel.init(i,startOffset-lineOffset);
//	binaryCompileDateLabel.setText(lang.get("MGBuildDateTime"));

	//game info:
	i+=130;
	serverTitleLabel.registerGraphicComponent(containerName,"serverTitleLabel");
	serverTitleLabel.init(i,startOffset-lineOffset);
	serverTitleLabel.setText(lang.get("MGGameTitle"));

	i+=210;
	ipAddressLabel.registerGraphicComponent(containerName,"ipAddressLabel");
	ipAddressLabel.init(i,startOffset-lineOffset);
	ipAddressLabel.setText(lang.get("MGGameIP"));

	//game setup info:
	i+=100;
	techLabel.registerGraphicComponent(containerName,"techLabel");
	techLabel.init(i,startOffset-lineOffset);
	techLabel.setText(lang.get("TechTree"));

	i+=100;
	mapLabel.registerGraphicComponent(containerName,"mapLabel");
	mapLabel.init(i,startOffset-lineOffset);
	mapLabel.setText(lang.get("Map"));

	i+=100;
	tilesetLabel.registerGraphicComponent(containerName,"tilesetLabel");
	tilesetLabel.init(i,startOffset-lineOffset);
	tilesetLabel.setText(lang.get("Tileset"));

	i+=100;
	activeSlotsLabel.registerGraphicComponent(containerName,"activeSlotsLabel");
	activeSlotsLabel.init(i,startOffset-lineOffset);
	activeSlotsLabel.setText(lang.get("MGGameSlots"));

	i+=50;
	externalConnectPort.registerGraphicComponent(containerName,"externalConnectPort");
	externalConnectPort.init(i,startOffset-lineOffset);
	externalConnectPort.setText(lang.get("Port"));

	i+=50;
	selectButton.registerGraphicComponent(containerName,"selectButton");
	selectButton.init(i, startOffset-lineOffset);
	selectButton.setText(lang.get("MGJoinGameSlots"));

	// Titles for current games - END
	
	buttonReturn.registerGraphicComponent(containerName,"buttonReturn");
    buttonReturn.init(50, buttonPos, 150);

    buttonCreateGame.registerGraphicComponent(containerName,"buttonCreateGame");
    buttonCreateGame.init(300, buttonPos, 150);

    buttonRefresh.registerGraphicComponent(containerName,"buttonRefresh");
    buttonRefresh.init(550, buttonPos, 150);

	buttonRefresh.setText(lang.get("RefreshList"));
	buttonReturn.setText(lang.get("Return"));
	buttonCreateGame.setText(lang.get("CustomGame"));
	labelAutoRefresh.setText(lang.get("AutoRefreshRate"));

	labelAutoRefresh.registerGraphicComponent(containerName,"labelAutoRefresh");
	labelAutoRefresh.init(800,buttonPos+30);

	listBoxAutoRefresh.registerGraphicComponent(containerName,"listBoxAutoRefresh");
	listBoxAutoRefresh.init(800,buttonPos);
	listBoxAutoRefresh.pushBackItem(lang.get("Off"));
	listBoxAutoRefresh.pushBackItem("10 s");
	listBoxAutoRefresh.pushBackItem("20 s");
	listBoxAutoRefresh.pushBackItem("30 s");
	listBoxAutoRefresh.setSelectedItemIndex(1);
	autoRefreshTime=10*listBoxAutoRefresh.getSelectedItemIndex();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	NetworkManager::getInstance().end();
	NetworkManager::getInstance().init(nrClient);
	
	// write hint to console:
	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
	console.addLine(lang.get("To switch off music press")+" - \""+configKeys.getCharKey("ToggleMusic")+"\"");

	GraphicComponent::applyAllCustomProperties(containerName);


	MutexSafeWrapper safeMutexPtr(&masterServerThreadPtrChangeAccessor);
	masterServerThreadInDeletion = false;
	needUpdateFromServer = true;
	updateFromMasterserverThread = new SimpleTaskThread(this,0,100);
	updateFromMasterserverThread->setUniqueID(__FILE__);
	updateFromMasterserverThread->start();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

MenuStateMasterserver::~MenuStateMasterserver() {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(masterServerThreadInDeletion == false) {
		MutexSafeWrapper safeMutexPtr(&masterServerThreadPtrChangeAccessor);
		if(updateFromMasterserverThread != NULL) {
			MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
			needUpdateFromServer = false;
			safeMutex.ReleaseLock();

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			//BaseThread::shutdownAndWait(updateFromMasterserverThread);
			masterServerThreadInDeletion = true;
			if(updateFromMasterserverThread != NULL &&
				updateFromMasterserverThread->shutdownAndWait() == true) {
				delete updateFromMasterserverThread;
			}
			updateFromMasterserverThread = NULL;
			masterServerThreadInDeletion = false;
			safeMutexPtr.ReleaseLock();
		}
		else {
			safeMutexPtr.ReleaseLock();
		}
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
		needUpdateFromServer = true;

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    }
    else if(buttonReturn.mouseClick(x, y)){
    	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    	MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		soundRenderer.playFx(coreData.getClickSoundB());

		//BaseThread::shutdownAndWait(updateFromMasterserverThread);
		MutexSafeWrapper safeMutexPtr(&masterServerThreadPtrChangeAccessor);
		masterServerThreadInDeletion = true;
		if(updateFromMasterserverThread != NULL &&
			updateFromMasterserverThread->shutdownAndWait() == true) {
			delete updateFromMasterserverThread;
		}
		updateFromMasterserverThread = NULL;
		masterServerThreadInDeletion = false;
		safeMutexPtr.ReleaseLock();

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

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//BaseThread::shutdownAndWait(updateFromMasterserverThread);
		MutexSafeWrapper safeMutexPtr(&masterServerThreadPtrChangeAccessor);
		masterServerThreadInDeletion = true;
		if(updateFromMasterserverThread != NULL &&
			updateFromMasterserverThread->shutdownAndWait() == true) {
			delete updateFromMasterserverThread;
		}
		updateFromMasterserverThread = NULL;
		masterServerThreadInDeletion = false;
		safeMutexPtr.ReleaseLock();

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
	    		int connectServerPort = serverLines[i]->getMasterServerInfo()->getExternalConnectPort();
	    		connectToServer(connectServerIP,connectServerPort);
	    		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	    		safeMutex.ReleaseLock();

	    		MutexSafeWrapper safeMutexPtr(&masterServerThreadPtrChangeAccessor);
	    		masterServerThreadInDeletion = true;
				if(updateFromMasterserverThread != NULL &&
					updateFromMasterserverThread->shutdownAndWait() == true) {
					delete updateFromMasterserverThread;
				}
	    		updateFromMasterserverThread = NULL;
	    		masterServerThreadInDeletion = false;
	    		safeMutexPtr.ReleaseLock();

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
	if(mainMessageBox.getEnabled()) {
		renderer.renderMessageBox(&mainMessageBox);
	}
	else
	{
		renderer.renderButton(&buttonRefresh);
		renderer.renderButton(&buttonReturn);

		renderer.renderLabel(&labelTitle,&GREEN);
		renderer.renderLabel(&announcementLabel,&YELLOW);
		renderer.renderLabel(&versionInfoLabel);

		renderer.renderLabel(&labelAutoRefresh);
		renderer.renderLabel(&labelChatUrl);
		renderer.renderButton(&buttonCreateGame);
		renderer.renderListBox(&listBoxAutoRefresh);

		// Render titles for server games listed
		const Vec4f titleLabelColor = CYAN;

		//general info:
		renderer.renderLabel(&glestVersionLabel,&titleLabelColor);
		renderer.renderLabel(&platformLabel,&titleLabelColor);
		renderer.renderLabel(&binaryCompileDateLabel,&titleLabelColor);

		//game info:
		renderer.renderLabel(&serverTitleLabel,&titleLabelColor);
		renderer.renderLabel(&ipAddressLabel,&titleLabelColor);

		//game setup info:
		renderer.renderLabel(&techLabel,&titleLabelColor);
		renderer.renderLabel(&mapLabel,&titleLabelColor);
		renderer.renderLabel(&tilesetLabel,&titleLabelColor);
		renderer.renderLabel(&activeSlotsLabel,&titleLabelColor);
		renderer.renderLabel(&externalConnectPort,&titleLabelColor);
		renderer.renderLabel(&selectButton,&titleLabelColor);


		// render console
		renderer.renderConsole(&console,false,false);
		
		for(int i=0; i<serverLines.size(); ++i){
	    	serverLines[i]->render();
	    }
	}
	if(program != NULL) program->renderProgramMsgBox();
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
		//switch on music again!!
		Config &config = Config::getInstance();
		float configVolume = (config.getInt("SoundVolumeMusic") / 100.f);
		CoreData::getInstance().getMenuMusic()->setVolume(configVolume);
		
		playServerFoundSound=false;
	}
	
	console.update();

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
	if(masterServerThreadInDeletion == true) {
		return;
	}

	if( updateFromMasterserverThread == NULL ||
		updateFromMasterserverThread->getQuitStatus() == true) {
		return;
	}
	MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
	bool needUpdate = needUpdateFromServer;
	safeMutex.ReleaseLock();

	if(needUpdate == true) {
		updateServerInfo();
	}
}

void MenuStateMasterserver::updateServerInfo() {
	try {

		if(masterServerThreadInDeletion == true) {
			return;
		}

		MutexSafeWrapper safeMutexPtr(&masterServerThreadPtrChangeAccessor);
		if( updateFromMasterserverThread == NULL ||
			updateFromMasterserverThread->getQuitStatus() == true) {
			safeMutexPtr.ReleaseLock();
			return;
		}
		safeMutexPtr.ReleaseLock(true);

		MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		needUpdateFromServer = false;
		int numberOfOldServerLines=serverLines.size();
		clearServerLines();
		safeMutex.ReleaseLock(true);

		if(announcementLoaded == false) {
			string announcementURL = Config::getInstance().getString("AnnouncementURL","http://megaglest.pepper.freeit.org/announcement.txt");
			if(announcementURL != "") {
				std::string announcementTxt = SystemFlags::getHTTP(announcementURL);
				if(StartsWith(announcementTxt,"Announcement from Masterserver:") == true) {
					announcementLabel.setText(announcementTxt);
				}
			}
			string versionURL = Config::getInstance().getString("versionURL","http://megaglest.pepper.freeit.org/")+glestVersionString+".txt";
			//printf("\nversionURL=%s\n",versionURL.c_str());
			if(versionURL != "") {
				std::string versionTxt = SystemFlags::getHTTP(versionURL);
				if(StartsWith(versionTxt,"Version info:") == true) {
					versionInfoLabel.setText(versionTxt);
				}
			}
			announcementLoaded=true;
		}

		if(Config::getInstance().getString("Masterserver","") != "") {
			std::string serverInfo = SystemFlags::getHTTP(Config::getInstance().getString("Masterserver") + "showServersForGlest.php");

			if(serverInfo != "") {
				std::vector<std::string> serverList;
				Tokenize(serverInfo,serverList,"\n");
				for(int i=0; i < serverList.size(); i++) {
					string &server = serverList[i];
					std::vector<std::string> serverEntities;
					Tokenize(server,serverEntities,"|");

					const int MIN_FIELDS_EXPECTED = 12;
					if(serverEntities.size() >= MIN_FIELDS_EXPECTED) {

						Lang &lang= Lang::getInstance();
						labelTitle.setText(lang.get("AvailableServers"));

						if(Config::getInstance().getString("Masterserver","") == "") {
							labelTitle.setText("*** " + lang.get("AvailableServers"));
						}

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
						masterServerInfo->setExternalConnectPort(strToInt(serverEntities[11]));

						//printf("Getting Ping time for host %s\n",masterServerInfo->getIpAddress().c_str());
						//float pingTime = Socket::getAveragePingMS(masterServerInfo->getIpAddress().c_str(),1);
						//printf("Ping time = %f\n",pingTime);
						char szBuf[1024]="";
						//sprintf(szBuf,"%s, %.2fms",masterServerInfo->getServerTitle().c_str(),pingTime);
						sprintf(szBuf,"%s",masterServerInfo->getServerTitle().c_str());
						masterServerInfo->setServerTitle(szBuf);

						safeMutexPtr.Lock();
						if( masterServerThreadInDeletion == true ||
							updateFromMasterserverThread == NULL ||
							updateFromMasterserverThread->getQuitStatus() == true) {
							safeMutexPtr.ReleaseLock();
							return;
						}
						safeMutexPtr.ReleaseLock(true);

						safeMutex.Lock();
						serverLines.push_back(new ServerLine( masterServerInfo, i, containerName));
						safeMutex.ReleaseLock(true);
					}
					else {
						Lang &lang= Lang::getInstance();
						labelTitle.setText("*** " + lang.get("AvailableServers") + " [" + serverInfo + "]");
					}
				}
			}
		}

		safeMutexPtr.Lock();
		if( updateFromMasterserverThread == NULL ||
			updateFromMasterserverThread->getQuitStatus() == true) {
			safeMutexPtr.ReleaseLock();
			return;
		}
		safeMutexPtr.ReleaseLock();
		
		safeMutex.Lock();
		if(serverLines.size()>numberOfOldServerLines) {
			playServerFoundSound=true;
		}
		safeMutex.ReleaseLock();
	}
	catch(const exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d, error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		threadedErrorMsg = e.what();
	}
}


bool MenuStateMasterserver::connectToServer(string ipString, int port)
{
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] START  ipString='%s'\n",__FILE__,__FUNCTION__,ipString.c_str());

	ClientInterface* clientInterface= NetworkManager::getInstance().getClientInterface();
	Config& config= Config::getInstance();
	Ip serverIp(ipString);

	//int serverPort = Config::getInstance().getInt("ServerPort",intToStr(GameConstants::serverPort).c_str());
	int serverPort = port;
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] try to connect to [%s] serverPort = %d\n",__FILE__,__FUNCTION__,serverIp.getString().c_str(),serverPort);
	clientInterface->connect(serverIp, serverPort);
	if(clientInterface->isConnected() == false) {
		NetworkManager::getInstance().end();
		NetworkManager::getInstance().init(nrClient);
		
		mainMessageBoxState=1;
		Lang &lang= Lang::getInstance();
		showMessageBox(lang.get("Couldnt connect"), lang.get("Connection failed"), false);
		return false;
		
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] connection failed\n",__FILE__,__FUNCTION__);	
	}
	else {
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


void MenuStateMasterserver::keyDown(char key) {
	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));

	if(key == configKeys.getCharKey("ToggleMusic")) {
		Config &config = Config::getInstance();
		Lang &lang= Lang::getInstance();
		
		float configVolume = (config.getInt("SoundVolumeMusic") / 100.f);
		float currentVolume = CoreData::getInstance().getMenuMusic()->getVolume();
		if(currentVolume > 0) {
			CoreData::getInstance().getMenuMusic()->setVolume(0.f);
			console.addLine(lang.get("GameMusic") + " " + lang.get("Off"));
		}
		else {
			CoreData::getInstance().getMenuMusic()->setVolume(configVolume);
			//If the config says zero, use the default music volume
			//gameMusic->setVolume(configVolume ? configVolume : 0.9);
			console.addLine(lang.get("GameMusic"));
		}
	}
	else if(key == configKeys.getCharKey("SaveGUILayout")) {
		bool saved = GraphicComponent::saveAllCustomProperties(containerName);
		Lang &lang= Lang::getInstance();
		console.addLine(lang.get("GUILayoutSaved") + " [" + (saved ? lang.get("Yes") : lang.get("No"))+ "]");
	}
}

//CoreData::getInstance().getMenuMusic()->setVolume(strToInt(listBoxVolumeMusic.getSelectedItem())/100.f);

}}//end namespace
