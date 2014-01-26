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

#include "menu_state_load_game.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_root.h"
#include "metrics.h"
#include "network_message.h"
#include "game.h"
#include "auto_test.h"

#include "leak_dumper.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class MenuStateLoadGame
// =====================================================

MenuStateLoadGame::MenuStateLoadGame(Program *program, MainMenu *mainMenu):
	MenuState(program, mainMenu, "root")
{
	containerName = "LoadGame";
	Lang &lang= Lang::getInstance();

	int buttonWidth = 120;
	int yPos=40;
	int xPos=20;
	int xSpacing=20;
	int slotsToRender=20;
	int slotWidth=200;

	slotLinesYBase=650;
	slotsLineHeight=30;
	previewTexture=NULL;
	buttonToDelete=NULL;

	selectedButton=NULL;

    string userData = Config::getInstance().getString("UserData_Root","");
	if(getGameReadWritePath(GameConstants::path_logs_CacheLookupKey) != "") {
		userData = getGameReadWritePath(GameConstants::path_logs_CacheLookupKey);
	}
    if(userData != "") {
    	endPathWithSlash(userData);
    }
    saveGameDir = userData +"saved/";

	lines[0].init(0,slotLinesYBase+slotsLineHeight);
	lines[1].init(0, slotLinesYBase-(slotsToRender-1)*slotsLineHeight-5);

	headerLabel.registerGraphicComponent(containerName,"headerLabel");
	headerLabel.init(400, 730);
	headerLabel.setFont(CoreData::getInstance().getMenuFontBig());
	headerLabel.setFont3D(CoreData::getInstance().getMenuFontBig3D());
	headerLabel.setText(lang.getString("LoadGameMenu"));

	noSavedGamesLabel.registerGraphicComponent(containerName,"noSavedGamesLabel");
	noSavedGamesLabel.init(20, 400);
	noSavedGamesLabel.setFont(CoreData::getInstance().getMenuFontBig());
	noSavedGamesLabel.setFont3D(CoreData::getInstance().getMenuFontBig3D());
	noSavedGamesLabel.setText(lang.getString("NoSavedGames"));

	savedGamesLabel.registerGraphicComponent(containerName,"savedGamesLabel");
	savedGamesLabel.init(120, slotLinesYBase+slotsLineHeight+10);
	savedGamesLabel.setFont(CoreData::getInstance().getMenuFontBig());
	savedGamesLabel.setFont3D(CoreData::getInstance().getMenuFontBig3D());
	savedGamesLabel.setText(lang.getString("SavedGames"));

	infoHeaderLabel.registerGraphicComponent(containerName,"infoHeaderLabel");
	infoHeaderLabel.init(650, slotLinesYBase+slotsLineHeight+10);
	infoHeaderLabel.setFont(CoreData::getInstance().getMenuFontBig());
	infoHeaderLabel.setFont3D(CoreData::getInstance().getMenuFontBig3D());
	infoHeaderLabel.setText(lang.getString("SavegameInfo"));

	infoTextLabel.registerGraphicComponent(containerName,"infoTextLabel");
	infoTextLabel.init(550, 350);
	infoTextLabel.setText("");

    abortButton.registerGraphicComponent(containerName,"abortButton");
    abortButton.init(xPos, yPos, buttonWidth);
	abortButton.setText(lang.getString("Abort"));
	xPos+=buttonWidth+xSpacing;
    loadButton.registerGraphicComponent(containerName,"loadButton");
    loadButton.init(xPos, yPos, buttonWidth);
    loadButton.setText(lang.getString("LoadGame"));
	xPos+=buttonWidth+xSpacing;
    deleteButton.registerGraphicComponent(containerName,"deleteButton");
    deleteButton.init(xPos, yPos, buttonWidth);
    deleteButton.setText(lang.getString("Delete"));

    slotsScrollBar.init(500-20,slotLinesYBase-slotsLineHeight*(slotsToRender-1),false,slotWidth,20);
    slotsScrollBar.setLength(slotsLineHeight*slotsToRender);
    slotsScrollBar.setElementCount(0);
    slotsScrollBar.setVisibleSize(slotsToRender);
    slotsScrollBar.setVisibleStart(0);

    listFiles();
    slotsScrollBar.setElementCount((int)filenames.size());

    mainMessageBox.registerGraphicComponent(containerName,"mainMessageBox");
	mainMessageBox.init(lang.getString("Ok"),450);
	mainMessageBox.setEnabled(false);

	GraphicComponent::applyAllCustomProperties(containerName);
}

MenuStateLoadGame::~MenuStateLoadGame() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	clearSlots();

	cleanupTexture(&previewTexture);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] END\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void MenuStateLoadGame::cleanupTexture(Texture2D **texture) {
	if(texture != NULL && *texture != NULL) {
		(*texture)->end();
		delete *texture;
		*texture=NULL;
	}
}

void MenuStateLoadGame::clearSlots() {
	while(!slots.empty()) {
		delete slots.back();
		slots.pop_back();
		slotsGB.pop_back();
	}
}

void MenuStateLoadGame::listFiles() {
	int keyButtonsXBase = 20;
	int keyButtonsYBase = slotLinesYBase;
	int keyButtonsWidth = 460;
	int keyButtonsHeight = slotsLineHeight;

	clearSlots();
	// Save the file now
    vector<string> paths;
    paths.push_back(saveGameDir);
    filenames.clear();
    findAll(paths, "*.xml", filenames, true, false, true);
    sort(filenames.begin(),filenames.end());
    for(int i = (int)filenames.size()-1; i > -1; i--) {
    	GraphicButton *button=new GraphicButton();
    	button->init( keyButtonsXBase, keyButtonsYBase, keyButtonsWidth,keyButtonsHeight);
    	button->setText(filenames[i]);

    	slots.push_back(button);
    	slotsGB.push_back(button);
    }
}


void MenuStateLoadGame::reloadUI() {
	Lang &lang= Lang::getInstance();

	infoHeaderLabel.setText(lang.getString("SavegameInfo"));
	savedGamesLabel.setText(lang.getString("SavedGames"));
	noSavedGamesLabel.setText(lang.getString("NoSavedGames"));
	headerLabel.setText(lang.getString("LoadGameMenu"));

	abortButton.setText(lang.getString("Abort"));
	deleteButton.setText(lang.getString("Delete"));
	loadButton.setText(lang.getString("LoadGame"));

	mainMessageBox.init(lang.getString("Ok"),450);

	GraphicComponent::reloadFontsForRegisterGraphicComponents(containerName);
}

void MenuStateLoadGame::mouseClick(int x, int y, MouseButton mouseButton){

	CoreData &coreData=  CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	if(mainMessageBox.getEnabled()) {
		soundRenderer.playFx(coreData.getClickSoundA());
		int button= 0;
		if(mainMessageBox.mouseClick(x, y, button)) {
			mainMessageBox.setEnabled(false);

			Lang &lang= Lang::getInstance();
			mainMessageBox.init(lang.getString("Ok"),450);
		}
	}
    if(abortButton.mouseClick(x, y)) {
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
    }
    else if(deleteButton.mouseClick(x, y)) {
		soundRenderer.playFx(coreData.getClickSoundB());
		if(selectedButton == NULL) {
			console.addStdMessage("NothingSelected",true);
		}
		else {
			string slotname 	= selectedButton->getText();
			string filename 	= saveGameDir + selectedButton->getText() + ".xml";
			string jpgfilename 	= saveGameDir + selectedButton->getText() + ".xml.jpg";
			string replayfilename 	= saveGameDir + selectedButton->getText() + ".xml.replay";

			Lang &lang= Lang::getInstance();
			char szBuf[8096]="";
			snprintf(szBuf,8096,lang.getString("LoadGameDeletingFile","",true).c_str(),filename.c_str());
			console.addLineOnly(szBuf);

			for(int i = 0; i < (int)slots.size(); i++) {
				if(slots[i] == selectedButton) {
					if(removeFile(filename) == true) {
						removeFile(jpgfilename);
						removeFile(replayfilename);
						cleanupTexture(&previewTexture);

						infoTextLabel.setText("");
						listFiles();
						slotsScrollBar.setElementCount((int)filenames.size());

						selectedButton = NULL;
					}
					break;
				}
			}
		}
    }
    else if(loadButton.mouseClick(x, y)) {
		soundRenderer.playFx(coreData.getClickSoundB());

		if(selectedButton == NULL) {
			console.addStdMessage("NothingSelected",true);
		}
		else {
			string filename = saveGameDir + selectedButton->getText() + ".xml";

			Lang &lang= Lang::getInstance();
			char szBuf[8096]="";
			snprintf(szBuf,8096,lang.getString("LoadGameLoadingFile","",true).c_str(),filename.c_str());
			console.addLineOnly(szBuf);

			try {
				Game::loadGame(filename,program,false);
			}
			catch(const megaglest_runtime_error &ex) {
				char szBuf[8096]="";
				snprintf(szBuf,8096,"In [%s::%s Line: %d]\nError [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
				SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);

				showMessageBox(ex.what(), lang.getString("Notice"), false);
			}
			return;
		}
    }
	else if(slotsScrollBar.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
	}
    else {
    	if(slotsScrollBar.getElementCount()!=0){
    		for(int i = slotsScrollBar.getVisibleStart(); i <= slotsScrollBar.getVisibleEnd(); ++i) {
				if(slots[i]->mouseClick(x, y) && selectedButton != slots[i]) {
					soundRenderer.playFx(coreData.getClickSoundB());

					Lang &lang= Lang::getInstance();
					cleanupTexture(&previewTexture);
					selectedButton = slots[i];
					string filename	= saveGameDir + selectedButton->getText()+".xml";
					string screenShotFilename = filename + ".jpg";
					if(fileExists(screenShotFilename) == true) {
						try {
							previewTexture = GraphicsInterface::getInstance().getFactory()->newTexture2D();
							if(previewTexture) {
								previewTexture->setMipmap(true);
								previewTexture->load(screenShotFilename);
								previewTexture->init();
							}
						}
						catch(const megaglest_runtime_error &ex) {
							char szBuf[8096]="";
							snprintf(szBuf,8096,"In [%s::%s Line: %d]\nError [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
							SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);

							cleanupTexture(&previewTexture);
							showMessageBox(ex.what(), lang.getString("Notice"), false);
						}
					}
					else {
						previewTexture=NULL;
					}

					if(fileExists(filename) == true) {
						// Xerces is infinitely slower than rapidxml
						xml_engine_parser_type engine_type = XML_RAPIDXML_ENGINE;

#if defined(WANT_XERCES)

						if(Config::getInstance().getBool("ForceXMLLoadGameUsingXerces","false") == true) {
							engine_type = XML_XERCES_ENGINE;
						}

#endif

						XmlTree	xmlTree(engine_type);

						if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Before load of XML\n");
						std::map<string,string> mapExtraTagReplacementValues;
						try {
							xmlTree.load(filename, Properties::getTagReplacementValues(&mapExtraTagReplacementValues),true,false,true);

							if(SystemFlags::VERBOSE_MODE_ENABLED) printf("After load of XML\n");

							const XmlNode *rootNode= xmlTree.getRootNode();
							if(rootNode != NULL && rootNode->hasChild("megaglest-saved-game") == true) {
								rootNode = rootNode->getChild("megaglest-saved-game");
							}

							if(rootNode == NULL) {
								char szBuf[8096]="";
								snprintf(szBuf,8096,"Invalid XML saved game file: [%s]",filename.c_str());
								infoTextLabel.setText(szBuf);
								return;
							}

							const XmlNode *versionNode= rootNode;
							string gameVer = versionNode->getAttribute("version")->getValue();
							if(gameVer != glestVersionString && checkVersionComptability(gameVer, glestVersionString) == false) {
								char szBuf[8096]="";
								snprintf(szBuf,8096,lang.getString("SavedGameBadVersion").c_str(),gameVer.c_str(),glestVersionString.c_str());
								infoTextLabel.setText(szBuf);
							}
							else {
								XmlNode *gameNode = rootNode->getChild("Game");
								GameSettings newGameSettings;
								newGameSettings.loadGame(gameNode);

								char szBuf[8096]="";
								snprintf(szBuf,8096,lang.getString("LoadSavedGameInfo").c_str(),
										newGameSettings.getMap().c_str(),
										newGameSettings.getTileset().c_str(),
										newGameSettings.getTech().c_str(),
										newGameSettings.getScenario().c_str(),
										newGameSettings.getFactionCount(),
										(newGameSettings.getThisFactionIndex() >= 0 &&
										 newGameSettings.getThisFactionIndex() < newGameSettings.getFactionCount() ?
										newGameSettings.getFactionTypeName(newGameSettings.getThisFactionIndex()).c_str() : ""));
								infoTextLabel.setText(szBuf);
							}
						}
						catch(const megaglest_runtime_error &ex) {
							char szBuf[8096]="";
							snprintf(szBuf,8096,"In [%s::%s Line: %d]\nError [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
							SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);

							showMessageBox(ex.what(), lang.getString("Notice"), false);
						}
					}
					else {
						infoTextLabel.setText("");
					}

					break;
				}
    		}
    	}
    }
}

void MenuStateLoadGame::mouseUp(int x, int y, const MouseButton mouseButton) {
    if (mouseButton == mbLeft) {
        slotsScrollBar.mouseUp(x, y);
    }
}

void MenuStateLoadGame::mouseMove(int x, int y, const MouseState *ms) {
    abortButton.mouseMove(x, y);
    deleteButton.mouseMove(x, y);
    loadButton.mouseMove(x, y);
	if(slotsScrollBar.getElementCount()!=0){
		for(int i = slotsScrollBar.getVisibleStart(); i <= slotsScrollBar.getVisibleEnd(); ++i) {
			slots[i]->mouseMove(x, y);
		}
	}
	slotsScrollBar.mouseMove(x,y);
}

void MenuStateLoadGame::render() {
	Renderer &renderer= Renderer::getInstance();

	renderer.renderLabel(&headerLabel);
	renderer.renderLabel(&savedGamesLabel);
	renderer.renderLabel(&infoHeaderLabel);
	renderer.renderLabel(&infoTextLabel);

	renderer.renderButton(&abortButton);
	renderer.renderButton(&deleteButton);
	renderer.renderButton(&loadButton);

    for(int i = 0; i < (int)sizeof(lines) / (int)sizeof(lines[0]); ++i){
    	renderer.renderLine(&lines[i]);
    }

	if(slotsScrollBar.getElementCount()==0 ) {
		renderer.renderLabel(&noSavedGamesLabel);
	}
	else{
		for(int i = slotsScrollBar.getVisibleStart(); i <= slotsScrollBar.getVisibleEnd(); ++i) {
			if(slots[i]==selectedButton){
				bool lightedOverride = true;
				renderer.renderButton(slots[i],&YELLOW,&lightedOverride);
			}
			else{
				renderer.renderButton(slots[i]);
			}
		}
	}
	renderer.renderScrollBar(&slotsScrollBar);

	if(previewTexture != NULL) {
		renderer.renderTextureQuad(550,slotLinesYBase-300+slotsLineHeight,400,300,previewTexture,1.0f);
	}

	if(mainMessageBox.getEnabled()) {
		renderer.renderMessageBox(&mainMessageBox);
	}

	renderer.renderConsole(&console,false,false);
	if(program != NULL) program->renderProgramMsgBox();
}

void MenuStateLoadGame::update(){
	if(Config::getInstance().getBool("AutoTest")){
		AutoTest::getInstance().updateNewGame(program, mainMenu);
		return;
	}
	slotsScrollBar.arrangeComponents(slotsGB);
	console.update();
}

void MenuStateLoadGame::keyDown(SDL_KeyboardEvent key) {
	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
	if(isKeyPressed(configKeys.getSDLKey("SaveGUILayout"),key) == true) {
		GraphicComponent::saveAllCustomProperties(containerName);
	}
}

void MenuStateLoadGame::showMessageBox(const string &text, const string &header, bool toggle) {
	if(toggle == false) {
		mainMessageBox.setEnabled(false);
	}

	if(mainMessageBox.getEnabled() == false) {
		mainMessageBox.setText(text);
		mainMessageBox.setHeader(header);
		mainMessageBox.setEnabled(true);
	}
	else{
		mainMessageBox.setEnabled(false);
	}
}

}}//end namespace
