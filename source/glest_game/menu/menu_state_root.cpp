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

#include "menu_state_root.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_new_game.h"
#include "menu_state_load_game.h"
#include "menu_state_options.h"
#include "menu_state_about.h"
#include "menu_state_mods.h"
#include "metrics.h"
#include "network_manager.h"
#include "network_message.h"
#include "socket.h"
#include "auto_test.h"
#include <stdio.h>

#include "leak_dumper.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class MenuStateRoot
// =====================================================

bool MenuStateRoot::gameUpdateChecked = false;

MenuStateRoot::MenuStateRoot(Program *program, MainMenu *mainMenu):
	MenuState(program, mainMenu, "root"), updatesHttpServerThread(NULL)
{
	containerName = "MainMenu";

	ftpClientThread 		= NULL;
	lastDownloadProgress	= 0;

	Lang &lang= Lang::getInstance();
	int yPos=440;


	labelVersion.registerGraphicComponent(containerName,"labelVersion");
	if(EndsWith(glestVersionString, "-dev") == false){
		labelVersion.init(525, yPos);
		labelVersion.setText(glestVersionString);
	}
	else {
		labelVersion.init(405, yPos);
		labelVersion.setText(glestVersionString + " [" + getCompileDateTime() + ", " + getGITRevisionString() + "]");
	}

	yPos-=55;
	buttonNewGame.registerGraphicComponent(containerName,"buttonNewGame");
	buttonNewGame.init(425, yPos, 150);
    yPos-=40;
    buttonLoadGame.registerGraphicComponent(containerName,"buttonLoadGame");
    buttonLoadGame.init(425, yPos, 150);
    yPos-=40;
    buttonMods.registerGraphicComponent(containerName,"buttonMods");
    buttonMods.init(425, yPos, 150);
    yPos-=40;
    buttonOptions.registerGraphicComponent(containerName,"buttonOptions");
    buttonOptions.init(425, yPos, 150);
    yPos-=40;
    buttonAbout.registerGraphicComponent(containerName,"buttonAbout");
    buttonAbout.init(425, yPos , 150);
    yPos-=40;
    buttonExit.registerGraphicComponent(containerName,"buttonExit");
    buttonExit.init(425, yPos, 150);

	buttonNewGame.setText(lang.getString("NewGame"));
	buttonLoadGame.setText(lang.getString("LoadGame"));
	buttonMods.setText(lang.getString("Mods"));
	buttonOptions.setText(lang.getString("Options"));
	buttonAbout.setText(lang.getString("About"));
	buttonExit.setText(lang.getString("Exit"));
	
	//mesage box
	mainMessageBox.registerGraphicComponent(containerName,"mainMessageBox");
	mainMessageBox.init(lang.getString("Yes"), lang.getString("No"));
	mainMessageBox.setEnabled(false);

	errorMessageBox.registerGraphicComponent(containerName,"errorMessageBox");
	errorMessageBox.init(lang.getString("Ok"));
	errorMessageBox.setEnabled(false);

	ftpMessageBox.registerGraphicComponent(containerName,"ftpMessageBox");
	ftpMessageBox.init(lang.getString("Yes"), lang.getString("No"));
	ftpMessageBox.setEnabled(false);

	//PopupMenu popupMenu;
	std::vector<string> menuItems;
	menuItems.push_back("1");
	menuItems.push_back("2");
	menuItems.push_back("3");
	popupMenu.setW(100);
	popupMenu.setH(100);
	popupMenu.init("Test Menu",menuItems);
	popupMenu.setEnabled(false);
	popupMenu.setVisible(false);

	GraphicComponent::applyAllCustomProperties(containerName);
}

MenuStateRoot::~MenuStateRoot() {
	if(updatesHttpServerThread != NULL) {
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		updatesHttpServerThread->setSimpleTaskInterfaceValid(false);
		updatesHttpServerThread->signalQuit();
		updatesHttpServerThread->setThreadOwnerValid(false);

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		if( updatesHttpServerThread->canShutdown(true) == true &&
				updatesHttpServerThread->shutdownAndWait() == true) {
			delete updatesHttpServerThread;
		}
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
		updatesHttpServerThread = NULL;
	}

	if(ftpClientThread != NULL) {
		ftpClientThread->setCallBackObject(NULL);
		ftpClientThread->signalQuit();
		sleep(0);
		if(ftpClientThread->canShutdown(true) == true &&
				ftpClientThread->shutdownAndWait() == true) {
			delete ftpClientThread;
		}
		else {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"In [%s::%s %d] Error cannot shutdown ftpClientThread\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			//SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("%s",szBuf);
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

			//publishToMasterserverThread->cleanup();
		}
		ftpClientThread = NULL;

//		ftpClientThread->signalQuit();
//	    ftpClientThread->setCallBackObject(NULL);
//	    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
//	    if( ftpClientThread->shutdownAndWait() == true) {
//	    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
//            delete ftpClientThread;
//	    }
//	    ftpClientThread = NULL;
//	    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}

}

void MenuStateRoot::reloadUI() {
	Lang &lang= Lang::getInstance();

	console.resetFonts();

	if(EndsWith(glestVersionString, "-dev") == false){
		labelVersion.setText(glestVersionString);
	}
	else {
		labelVersion.setText(glestVersionString + " [" + getCompileDateTime() + ", " + getGITRevisionString() + "]");
	}

	buttonNewGame.setText(lang.getString("NewGame"));
	buttonLoadGame.setText(lang.getString("LoadGame"));
	buttonMods.setText(lang.getString("Mods"));
	buttonOptions.setText(lang.getString("Options"));
	buttonAbout.setText(lang.getString("About"));
	buttonExit.setText(lang.getString("Exit"));

	mainMessageBox.init(lang.getString("Yes"), lang.getString("No"));
	errorMessageBox.init(lang.getString("Ok"));
	ftpMessageBox.init(lang.getString("Yes"), lang.getString("No"));

	console.resetFonts();

	GraphicComponent::reloadFontsForRegisterGraphicComponents(containerName);
}

void MenuStateRoot::mouseClick(int x, int y, MouseButton mouseButton){
	try {
		CoreData &coreData=  CoreData::getInstance();
		SoundRenderer &soundRenderer= SoundRenderer::getInstance();

		if(popupMenu.mouseClick(x, y)) {
			//std::pair<int,string> result = popupMenu.mouseClickedMenuItem(x, y);
			popupMenu.mouseClickedMenuItem(x, y);

			//printf("In popup callback menuItemSelected [%s] menuIndexSelected = %d\n",result.second.c_str(),result.first);
		}
		//exit message box, has to be the last thing to do in this function
		else if(mainMessageBox.getEnabled()){
			int button= 0;
			if(mainMessageBox.mouseClick(x, y, button)) {
				if(button==0) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					soundRenderer.playFx(coreData.getClickSoundA());
					program->exit();
				}
				else {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					//close message box
					mainMessageBox.setEnabled(false);
				}
			}
		}
		//exit message box, has to be the last thing to do in this function
		else if(errorMessageBox.getEnabled()){
			int button= 0;
			if(mainMessageBox.mouseClick(x, y, button)) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				//close message box
				errorMessageBox.setEnabled(false);
			}
		}

		else if(ftpMessageBox.getEnabled()) {
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

			int button= 0;
			if(ftpMessageBox.mouseClick(x, y, button)) {
				ftpMessageBox.setEnabled(false);
				if(button == 0) {
					startFTPClientIfRequired();

					lastDownloadProgress = 0;
					printf("Adding ftpFileName [%s] ftpFileURL [%s]\n",ftpFileName.c_str(),ftpFileURL.c_str());
					if(ftpClientThread != NULL) ftpClientThread->addTempFileToRequests(ftpFileName,ftpFileURL);

					static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
					MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),mutexOwnerId);
					if(ftpClientThread != NULL && ftpClientThread->getProgressMutex() != NULL) ftpClientThread->getProgressMutex()->setOwnerId(mutexOwnerId);
					fileFTPProgressList[ftpFileName] = pair<int,string>(0,"");
					safeMutexFTPProgress.ReleaseLock();
				}
			}
		}
		else if(mainMessageBox.getEnabled() == false && buttonNewGame.mouseClick(x, y)){
			soundRenderer.playFx(coreData.getClickSoundB());
			mainMenu->setState(new MenuStateNewGame(program, mainMenu));
		}
		else if(mainMessageBox.getEnabled() == false && buttonLoadGame.mouseClick(x, y)){
			soundRenderer.playFx(coreData.getClickSoundB());
			mainMenu->setState(new MenuStateLoadGame(program, mainMenu));
		}
		else if(mainMessageBox.getEnabled() == false && buttonMods.mouseClick(x, y)){
			soundRenderer.playFx(coreData.getClickSoundB());
			mainMenu->setState(new MenuStateMods(program, mainMenu));
		}
		else if(mainMessageBox.getEnabled() == false && buttonOptions.mouseClick(x, y)){
			soundRenderer.playFx(coreData.getClickSoundB());
			mainMenu->setState(new MenuStateOptions(program, mainMenu));
		}
		else if(mainMessageBox.getEnabled() == false && buttonAbout.mouseClick(x, y)){
			soundRenderer.playFx(coreData.getClickSoundB());
			mainMenu->setState(new MenuStateAbout(program, mainMenu));
		}
		else if(buttonExit.mouseClick(x, y)){
			soundRenderer.playFx(coreData.getClickSoundA());
			program->exit();
		}
	}
	catch(exception &e) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d]\nError in menu event:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,e.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		showErrorMessageBox(szBuf, "", true);
	}
}

void MenuStateRoot::startFTPClientIfRequired() {
	if(ftpClientThread == NULL) {
		// Setup File Transfer thread
		Config &config = Config::getInstance();

		vector<string> tilesetFiles;
		vector<string> tilesetFilesUserData;

		vector<string> techTreeFiles;
		vector<string> techTreeFilesUserData;


		findDirs(config.getPathListForType(ptTilesets), tilesetFiles);
		findDirs(config.getPathListForType(ptTechs), techTreeFiles);

		vector<string> mapPathList = config.getPathListForType(ptMaps);
		std::pair<string,string> mapsPath;
		if(mapPathList.empty() == false) {
			mapsPath.first = mapPathList[0];
		}
		if(mapPathList.size() > 1) {
			mapsPath.second = mapPathList[1];
		}
		std::pair<string,string> tilesetsPath;
		vector<string> tilesetsList = Config::getInstance().getPathListForType(ptTilesets);
		if(tilesetsList.empty() == false) {
			tilesetsPath.first = tilesetsList[0];
			if(tilesetsList.size() > 1) {
				tilesetsPath.second = tilesetsList[1];
			}
		}

		std::pair<string,string> techtreesPath;
		vector<string> techtreesList = Config::getInstance().getPathListForType(ptTechs);
		if(techtreesList.empty() == false) {
			techtreesPath.first = techtreesList[0];
			if(techtreesList.size() > 1) {
				techtreesPath.second = techtreesList[1];
			}
		}

		std::pair<string,string> scenariosPath;
		vector<string> scenariosList = Config::getInstance().getPathListForType(ptScenarios);
		if(scenariosList.empty() == false) {
			scenariosPath.first = scenariosList[0];
			if(scenariosList.size() > 1) {
				scenariosPath.second = scenariosList[1];
			}
		}

		string fileArchiveExtension = config.getString("FileArchiveExtension","");
		string fileArchiveExtractCommand = config.getString("FileArchiveExtractCommand","");
		string fileArchiveExtractCommandParameters = config.getString("FileArchiveExtractCommandParameters","");
		int32 fileArchiveExtractCommandSuccessResult = config.getInt("FileArchiveExtractCommandSuccessResult","0");

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		console.setOnlyChatMessagesInStoredLines(false);

		// Get path to temp files
		string tempFilePath = "temp/";
		if(getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) != "") {
			tempFilePath = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) + tempFilePath;
		}
		else {
			string userData = config.getString("UserData_Root","");
			if(userData != "") {
				endPathWithSlash(userData);
			}
			tempFilePath = userData + tempFilePath;
		}
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Temp files path [%s]\n",tempFilePath.c_str());

		ftpClientThread = new FTPClientThread(-1,"",
				mapsPath,tilesetsPath,techtreesPath,scenariosPath,
				this,fileArchiveExtension,fileArchiveExtractCommand,
				fileArchiveExtractCommandParameters,
				fileArchiveExtractCommandSuccessResult,
				tempFilePath);
		ftpClientThread->start();


		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
}

void MenuStateRoot::FTPClient_CallbackEvent(string itemName,
		FTP_Client_CallbackType type, pair<FTP_Client_ResultType,string> result,void *userdata) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

    Lang &lang= Lang::getInstance();
    if(type == ftp_cct_DownloadProgress) {
        FTPClientCallbackInterface::FtpProgressStats *stats = (FTPClientCallbackInterface::FtpProgressStats *)userdata;
        if(stats != NULL) {
            int fileProgress = 0;
            if(stats->download_total > 0) {
                fileProgress = ((stats->download_now / stats->download_total) * 100.0);
            }
            //if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Got FTP Callback for [%s] current file [%s] fileProgress = %d [now = %f, total = %f]\n",itemName.c_str(),stats->currentFilename.c_str(), fileProgress,stats->download_now,stats->download_total);

            static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
            MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),mutexOwnerId);
            if(ftpClientThread != NULL && ftpClientThread->getProgressMutex() != NULL) ftpClientThread->getProgressMutex()->setOwnerId(mutexOwnerId);
            pair<int,string> lastProgress = fileFTPProgressList[itemName];
            fileFTPProgressList[itemName] = pair<int,string>(fileProgress,stats->currentFilename);
            safeMutexFTPProgress.ReleaseLock();

            if(itemName != "" && (lastDownloadProgress < fileProgress && fileProgress % 25 == 0)) {
            	lastDownloadProgress = fileProgress;

    			char szBuf[8096]="";
    			snprintf(szBuf,8096,"Downloaded %d%% of file: %s",fileProgress,itemName.c_str());
            	console.addLine(szBuf);
            }
        }
    }
    else if(type == ftp_cct_ExtractProgress) {
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Got FTP extract Callback for [%s] result = %d [%s]\n",itemName.c_str(),result.first,result.second.c_str());
    	printf("Got FTP extract Callback for [%s] result = %d [%s]\n",itemName.c_str(),result.first,result.second.c_str());

    	if(userdata == NULL) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,lang.getString("DataMissingExtractDownloadMod").c_str(),itemName.c_str());
			//printf("%s\n",szBuf);
			console.addLine(szBuf,true);
    	}
    	else {
			char *szBuf = (char *)userdata;
			//printf("%s\n",szBuf);
			console.addLine(szBuf);
    	}
    }
    else if(type == ftp_cct_TempFile) {
        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Got FTP Callback for [%s] result = %d [%s]\n",itemName.c_str(),result.first,result.second.c_str());

        static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
        MutexSafeWrapper safeMutexFTPProgress((ftpClientThread != NULL ? ftpClientThread->getProgressMutex() : NULL),mutexOwnerId);
        if(ftpClientThread != NULL && ftpClientThread->getProgressMutex() != NULL) ftpClientThread->getProgressMutex()->setOwnerId(mutexOwnerId);
        fileFTPProgressList.erase(itemName);
        safeMutexFTPProgress.ReleaseLock();

        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("### downloaded TEMP file [%s] result = %d\n",itemName.c_str(),result.first);

        if(result.first == ftp_crt_SUCCESS) {
    		// Get path to temp files
    		string tempFilePath = "temp/";
    		if(getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) != "") {
    			tempFilePath = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) + tempFilePath;
    		}
    		else {
    			Config &config = Config::getInstance();
    			string userData = config.getString("UserData_Root","");
    			if(userData != "") {
    				endPathWithSlash(userData);
    			}
    			tempFilePath = userData + tempFilePath;
    		}
    		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Temp files path [%s]\n",tempFilePath.c_str());

    		// Delete the downloaded archive
    		if(fileExists(tempFilePath + itemName)) {
    			removeFile(tempFilePath + itemName);
    		}

    		bool result = upgradeFilesInTemp();
    		if(result == false) {
    			string binaryName = Properties::getApplicationPath() + extractFileFromDirectoryPath(PlatformExceptionHandler::application_binary);
    			string binaryNameOld = Properties::getApplicationPath() + extractFileFromDirectoryPath(PlatformExceptionHandler::application_binary) + "__REMOVE";
    			bool resultRename = renameFile(binaryName,binaryNameOld);
    			//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Rename: [%s] to [%s] result = %d\n",binaryName.c_str(),binaryNameOld.c_str(),resultRename);
    			printf("#1 Rename: [%s] to [%s] result = %d errno = %d\n",binaryName.c_str(),binaryNameOld.c_str(),resultRename, errno);

    			//result = upgradeFilesInTemp();
    			binaryName = Properties::getApplicationPath() + extractFileFromDirectoryPath(PlatformExceptionHandler::application_binary);
    			binaryNameOld = tempFilePath + extractFileFromDirectoryPath(PlatformExceptionHandler::application_binary);
    			resultRename = renameFile(binaryNameOld, binaryName);

    			//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Rename: [%s] to [%s] result = %d\n",binaryName.c_str(),binaryNameOld.c_str(),resultRename);
    			printf("#2 Rename: [%s] to [%s] result = %d errno = %d\n",binaryNameOld.c_str(),binaryName.c_str(),resultRename, errno);
    		}

    		console.addLine("Successfully updated, please restart!",true);
        }
        else {
			curl_version_info_data *curlVersion= curl_version_info(CURLVERSION_NOW);

			char szBuf[8096]="";
			snprintf(szBuf,8096,"FAILED to download the updates: [%s] using CURL version [%s] [%s]",itemName.c_str(),curlVersion->version,result.second.c_str());
			console.addLine(szBuf,true);
			showErrorMessageBox(szBuf, "ERROR", false);
        }
    }
}


void MenuStateRoot::mouseMove(int x, int y, const MouseState *ms){
	popupMenu.mouseMove(x, y);
	buttonNewGame.mouseMove(x, y);
	buttonLoadGame.mouseMove(x, y);
    buttonMods.mouseMove(x, y);
    buttonOptions.mouseMove(x, y);
    buttonAbout.mouseMove(x, y);
    buttonExit.mouseMove(x,y);
	if (mainMessageBox.getEnabled()) {
		mainMessageBox.mouseMove(x, y);
	}
	if (errorMessageBox.getEnabled()) {
		errorMessageBox.mouseMove(x, y);
	}
	if (ftpMessageBox.getEnabled()) {
		ftpMessageBox.mouseMove(x, y);
	}
}

bool MenuStateRoot::isMasterserverMode() const {
	return GlobalStaticFlags::getIsNonGraphicalModeEnabled();
}

void MenuStateRoot::render() {
	if(isMasterserverMode() == true) {
		return;
	}
	Renderer &renderer= Renderer::getInstance();
	CoreData &coreData= CoreData::getInstance();
	const Metrics &metrics= Metrics::getInstance();

	int w= 300;
	int h= 150;
	int yPos=495;

	int logoMainX = (metrics.getVirtualW()-w)/2;
	int logoMainY = yPos-h/2;
	int logoMainW = w;
	int logoMainH = h;
	logoMainX = Config::getInstance().getInt(string(containerName) + "_MainLogo_x",intToStr(logoMainX).c_str());
	logoMainY = Config::getInstance().getInt(string(containerName) + "_MainLogo_y",intToStr(logoMainY).c_str());
	logoMainW = Config::getInstance().getInt(string(containerName) + "_MainLogo_w",intToStr(logoMainW).c_str());
	logoMainH = Config::getInstance().getInt(string(containerName) + "_MainLogo_h",intToStr(logoMainH).c_str());

	renderer.renderTextureQuad(
			logoMainX, logoMainY, logoMainW, logoMainH,
		coreData.getLogoTexture(), GraphicComponent::getFade());

	int maxLogoWidth=0;
	for(int idx = 0; idx < (int)coreData.getLogoTextureExtraCount(); ++idx) {
		Texture2D *extraLogo = coreData.getLogoTextureExtra(idx);
		maxLogoWidth += extraLogo->getPixmap()->getW();
	}

	int currentX = (metrics.getVirtualW()-maxLogoWidth)/2;
	int currentY = 50;
	for(int idx = 0; idx < (int)coreData.getLogoTextureExtraCount(); ++idx) {
		Texture2D *extraLogo = coreData.getLogoTextureExtra(idx);

		logoMainX = currentX;
		logoMainY = currentY;
		logoMainW = extraLogo->getPixmap()->getW();
		logoMainH = extraLogo->getPixmap()->getH();

		string logoTagName = string(containerName) + "_ExtraLogo" + intToStr(idx+1) + "_";
		logoMainX = Config::getInstance().getInt(logoTagName + "x",intToStr(logoMainX).c_str());
		logoMainY = Config::getInstance().getInt(logoTagName + "y",intToStr(logoMainY).c_str());
		logoMainW = Config::getInstance().getInt(logoTagName + "w",intToStr(logoMainW).c_str());
		logoMainH = Config::getInstance().getInt(logoTagName + "h",intToStr(logoMainH).c_str());

		renderer.renderTextureQuad(
				logoMainX, logoMainY,
				logoMainW, logoMainH,
				extraLogo, GraphicComponent::getFade());

		currentX += extraLogo->getPixmap()->getW();
	}

	renderer.renderButton(&buttonNewGame);
	renderer.renderButton(&buttonLoadGame);
	renderer.renderButton(&buttonMods);
	renderer.renderButton(&buttonOptions);
	renderer.renderButton(&buttonAbout);
	renderer.renderButton(&buttonExit);
	renderer.renderLabel(&labelVersion);

	renderer.renderConsole(&console);

	renderer.renderPopupMenu(&popupMenu);

	//exit message box
	if(mainMessageBox.getEnabled()) {
		renderer.renderMessageBox(&mainMessageBox);
	}
	if(errorMessageBox.getEnabled()) {
		renderer.renderMessageBox(&errorMessageBox);
	}
	if(ftpMessageBox.getEnabled()) {
		renderer.renderMessageBox(&ftpMessageBox);
	}

	if(program != NULL) program->renderProgramMsgBox();
}

void MenuStateRoot::update() {
	if(Config::getInstance().getBool("AutoTest")) {
		if(AutoTest::getInstance().mustExitGame() == false) {
			AutoTest::getInstance().updateRoot(program, mainMenu);
		}
		else {
			program->exit();
		}
		return;
	}

	if(gameUpdateChecked == false) {
		gameUpdateChecked = true;

		string updateCheckURL = Config::getInstance().getString("UpdateCheckURL","");
		if(updateCheckURL != "") {
		    static string mutexOwnerId = string(extractFileFromDirectoryPath(__FILE__).c_str()) + string("_") + intToStr(__LINE__);
		    updatesHttpServerThread = new SimpleTaskThread(this,1,200);
		    updatesHttpServerThread->setUniqueID(mutexOwnerId);
		    updatesHttpServerThread->start();
		}
	}

	console.update();
}

void MenuStateRoot::simpleTask(BaseThread *callingThread,void *userdata) {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	static string mutexOwnerId = string(__FILE__) + string("_") + intToStr(__LINE__);
    MutexSafeWrapper safeMutexThreadOwner(callingThread->getMutexThreadOwnerValid(),mutexOwnerId);
    if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
        return;
    }

    callingThread->getMutexThreadOwnerValid()->setOwnerId(mutexOwnerId);
    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);


	string updateCheckURL = Config::getInstance().getString("UpdateCheckURL","");
	if(updateCheckURL != "") {

		string baseURL = updateCheckURL;

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d] About to call first http url, base [%s]..\n",__FILE__,__FUNCTION__,__LINE__,baseURL.c_str());

		CURL *handle = SystemFlags::initHTTP();
		CURLcode curlResult = CURLE_OK;
		string updateMetaData = SystemFlags::getHTTP(baseURL,handle,-1,&curlResult);

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("techsMetaData [%s] curlResult = %d\n",updateMetaData.c_str(),curlResult);

		if(callingThread->getQuitStatus() == true || safeMutexThreadOwner.isValidMutex() == false) {
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);
			return;
		}

		if(curlResult != CURLE_OK) {
			string curlError = curl_easy_strerror(curlResult);

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d] curlError [%s]..\n",__FILE__,__FUNCTION__,__LINE__,curlError.c_str());

			char szMsg[8096]="";
			snprintf(szMsg,8096,"An error was detected while checking for new updates\n%s",curlError.c_str());
			showErrorMessageBox(szMsg, "ERROR", false);
		}

		if(curlResult == CURLE_OK ||
			(curlResult != CURLE_COULDNT_RESOLVE_HOST &&
			 curlResult != CURLE_COULDNT_CONNECT)) {

			Properties props;
			props.loadFromText(updateMetaData);

			int compareResult = compareMajorMinorVersion(glestVersionString, props.getString("LatestGameVersion",""));
			if(compareResult==0) {
				if(glestVersionString != props.getString("LatestGameVersion","")) {
					compareResult = -1;
				}
			}
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("compareResult = %d local [%s] remote [%s]\n",compareResult,glestVersionString.c_str(),props.getString("LatestGameVersion","").c_str());

			if(compareResult < 0) {

				string downloadBinaryKey = "LatestGameBinaryUpdateArchiveURL-" + getPlatformTypeNameString() + getPlatformArchTypeNameString();
				if(props.hasString(downloadBinaryKey)) {
					ftpFileName = extractFileFromDirectoryPath(props.getString(downloadBinaryKey));
					ftpFileURL = props.getString(downloadBinaryKey);
				}

				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Checking update key downloadBinaryKey [%s] ftpFileURL [%s]\n",downloadBinaryKey.c_str(),ftpFileURL.c_str());

				if(props.getBool("AllowUpdateDownloads","false") == false || ftpFileURL == "") {
					char szMsg[8096]="";
					snprintf(szMsg,8096,"A new update was detected: %s\nUpdate Date: %s\nPlease visit megaglest.org for details!",
							props.getString("LatestGameVersion","?").c_str(),
							props.getString("LatestGameVersionReleaseDate","?").c_str());
					showFTPMessageBox(szMsg, "Update", false, true);
				}
				else {
					char szMsg[8096]="";
					snprintf(szMsg,8096,"A new update was detected: %s\nUpdate Date: %s\nDownload update now?",
							props.getString("LatestGameVersion","?").c_str(),
							props.getString("LatestGameVersionReleaseDate","?").c_str());
					showFTPMessageBox(szMsg, "Update", false, false);
				}
			}
		}
		SystemFlags::cleanupHTTP(&handle);
	}
}

void MenuStateRoot::keyDown(SDL_KeyboardEvent key) {

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key.keysym.sym,key.keysym.sym);
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] key = [%d - %c]\n",__FILE__,__FUNCTION__,__LINE__,key.keysym.sym,key.keysym.sym);

	//printf("\n\n\nIN MENU STATE ROOT KEYDOWN!!!\n\n\n");

	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
	//exit
	//if(key == configKeys.getCharKey("ExitKey")) {
	if(isKeyPressed(configKeys.getSDLKey("ExitKey"),key) == true) {
		Lang &lang= Lang::getInstance();
		showMessageBox(lang.getString("ExitGame?"), "", true);
	}
	//else if(mainMessageBox.getEnabled() == true && key == vkReturn) {
	else if(mainMessageBox.getEnabled() == true && isKeyPressed(SDLK_RETURN,key) == true) {
		//SDL_keysym keystate = Window::getKeystate();
		SDL_keysym keystate = key.keysym;
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] keystate.mod [%d]\n",__FILE__,__FUNCTION__,__LINE__,keystate.mod);

		//printf("---> keystate.mod [%d]\n",keystate.mod);
		if(keystate.mod & (KMOD_LALT | KMOD_RALT)) {
		}
		else {
			//printf("EXITING ---> keystate.mod [%d]\n",keystate.mod);
			program->exit();
		}
	}
	//else if(key == configKeys.getCharKey("SaveGUILayout")) {
	else if(isKeyPressed(configKeys.getSDLKey("SaveGUILayout"),key) == true) {
		GraphicComponent::saveAllCustomProperties(containerName);
		//Lang &lang= Lang::getInstance();
		//console.addLine(lang.getString("GUILayoutSaved") + " [" + (saved ? lang.getString("Yes") : lang.getString("No"))+ "]");
	}

}

void MenuStateRoot::showMessageBox(const string &text, const string &header, bool toggle) {
	if(toggle == false) {
		mainMessageBox.setEnabled(false);
	}

	if(mainMessageBox.getEnabled() == false) {
		mainMessageBox.setText(text);
		mainMessageBox.setHeader(header);
		mainMessageBox.setEnabled(true);
	}
	else {
		mainMessageBox.setEnabled(false);
	}
}

void MenuStateRoot::showErrorMessageBox(const string &text, const string &header, bool toggle) {
	if(toggle == false) {
		errorMessageBox.setEnabled(false);
	}

	if(errorMessageBox.getEnabled() == false) {
		errorMessageBox.setText(text);
		errorMessageBox.setHeader(header);
		errorMessageBox.setEnabled(true);
	}
	else {
		errorMessageBox.setEnabled(false);
	}
}

void MenuStateRoot::showFTPMessageBox(const string &text, const string &header, bool toggle, bool okOnly) {
	if(toggle == false) {
		ftpMessageBox.setEnabled(false);
	}

	Lang &lang= Lang::getInstance();
	if(okOnly) {
		ftpMessageBox.init(lang.getString("Ok"));
	}
	else {
		ftpMessageBox.init(lang.getString("Yes"), lang.getString("No"));
	}

	if(ftpMessageBox.getEnabled() == false) {
		ftpMessageBox.setText(text);
		ftpMessageBox.setHeader(header);
		ftpMessageBox.setEnabled(true);
	}
	else {
		ftpMessageBox.setEnabled(false);
	}
}


}}//end namespace
