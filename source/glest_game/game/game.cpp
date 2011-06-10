//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "game.h"

#include "config.h"
#include "renderer.h"
#include "particle_renderer.h"
#include "commander.h"
#include "battle_end.h"
#include "sound_renderer.h"
#include "profiler.h"
#include "core_data.h"
#include "metrics.h"
#include "faction.h"
#include "network_manager.h"
#include "checksum.h"
#include "auto_test.h"
#include "FPUCheck.h"
#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Shared::Platform;

namespace Glest{ namespace Game{

Game *thisGamePtr = NULL;

// =====================================================
// 	class Game
// =====================================================

// ===================== PUBLIC ========================

const float PHOTO_MODE_MAXHEIGHT = 500.0;

Game::Game() : ProgramState(NULL) {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	originalDisplayMsgCallback = NULL;
	aiInterfaces.clear();
}

Game::Game(Program *program, const GameSettings *gameSettings):
	ProgramState(program), lastMousePos(0), isFirstRender(true)
{
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	this->program = program;
	Unit::setGame(this);
	gameStarted = false;

	original_updateFps = GameConstants::updateFps;
	original_cameraFps = GameConstants::cameraFps;
	GameConstants::updateFps= 40;
	GameConstants::cameraFps= 100;
	captureAvgTestStatus = false;
	lastRenderLog2d		 = 0;
	totalRenderFps       = 0;
	lastMaxUnitCalcTime  = 0;

	mouseMoved= false;
	quitTriggeredIndicator = false;
	originalDisplayMsgCallback = NULL;
	thisGamePtr = this;

	this->gameSettings= *gameSettings;
	scrollSpeed = Config::getInstance().getFloat("UiScrollSpeed","1.5");
	photoModeEnabled = Config::getInstance().getBool("PhotoMode","false");
	visibleHUD = Config::getInstance().getBool("VisibleHud","true");
	//MIN_RENDER_FPS_ALLOWED = Config::getInstance().getInt("MIN_RENDER_FPS_ALLOWED",intToStr(MIN_RENDER_FPS_ALLOWED).c_str());

	mouseX=0;
	mouseY=0;
	mouse2d= 0;
	loadingText="";
	weatherParticleSystem= NULL;
	updateFps=0;
	renderFps=0;
	lastUpdateFps=0;
	lastRenderFps=-1;
	avgUpdateFps=-1;
	avgRenderFps=-1;
	currentAvgRenderFpsTotal=0;
	tickCount=0;
	paused= false;
	gameOver= false;
	renderNetworkStatus= false;
	speed= sNormal;
	showFullConsole= false;

	 camLeftButtonDown=false;
	 camRightButtonDown=false;
	 camUpButtonDown=false;
	 camDownButtonDown=false;

	Object::setStateCallback(&gui);

    Logger &logger= Logger::getInstance();
	logger.showProgress();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

Game::~Game() {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	Object::setStateCallback(NULL);
	thisGamePtr = NULL;
	if(originalDisplayMsgCallback != NULL) {
		NetworkInterface::setDisplayMessageFunction(originalDisplayMsgCallback);
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    Logger &logger= Logger::getInstance();
	Renderer &renderer= Renderer::getInstance();

	logger.loadLoadingScreen("");
	logger.setState(Lang::getInstance().get("Deleting"));
	//logger.add("Game", true);
	logger.add("Game", false);
	logger.hideProgress();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	SoundRenderer::getInstance().stopAllSounds();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	deleteValues(aiInterfaces.begin(), aiInterfaces.end());
	aiInterfaces.clear();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	gui.end();		//selection must be cleared before deleting units

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	world.end();	//must die before selection because of referencers

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] aiInterfaces.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,aiInterfaces.size());

	// MUST DO THIS LAST!!!! Because objects above have pointers to things like
	// unit particles and fade them out etc and this end method deletes the original
	// object pointers.
	renderer.endGame();

	GameConstants::updateFps = original_updateFps;
	GameConstants::cameraFps = original_cameraFps;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	Unit::setGame(NULL);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] ==== END GAME ==== getCurrentPixelByteCount() = %llu\n",__FILE__,__FUNCTION__,__LINE__,(long long unsigned int)renderer.getCurrentPixelByteCount());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled) SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"==== END GAME ====\n");

	//this->program->reInitGl();
	//renderer.reinitAll();
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

bool Game::quitTriggered() {
	return quitTriggeredIndicator;
}

Stats Game::quitAndToggleState() {
	//quitGame();
	//Program *program = game->getProgram();
	return quitGame();
	//Game::exitGameState(program, endStats);
}

// ==================== init and load ====================

int Game::ErrorDisplayMessage(const char *msg, bool exitApp) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] %s\n",__FILE__,__FUNCTION__,__LINE__,msg);

	if(thisGamePtr != NULL) {
		string text = msg;
		thisGamePtr->showErrorMessageBox(text, "Error detected", false);
	}

    return 0;
}

Texture2D * Game::findFactionLogoTexture(const GameSettings *settings, Logger *logger,string factionLogoFilter, bool useTechDefaultIfFilterNotFound) {
	Texture2D *result = NULL;
	string logoFilename = Game::findFactionLogoFile(settings, logger,factionLogoFilter);
	if(logoFilename == "" && factionLogoFilter != "" && useTechDefaultIfFilterNotFound == true) {
		logoFilename = Game::findFactionLogoFile(settings, logger);
	}

	result = Renderer::findFactionLogoTexture(logoFilename);

	return result;
}

string Game::extractScenarioLogoFile(const GameSettings *settings, string &result,
		bool &loadingImageUsed, Logger *logger, string factionLogoFilter) {
    string scenarioDir = "";
    if(settings->getScenarioDir() != "") {
		scenarioDir = settings->getScenarioDir();
		if(EndsWith(scenarioDir, ".xml") == true) {
			scenarioDir = scenarioDir.erase(scenarioDir.size() - 4, 4);
			scenarioDir = scenarioDir.erase(scenarioDir.size() - settings->getScenario().size(), settings->getScenario().size() + 1);
		}

		//printf("!!! extractScenarioLogoFile scenarioDir [%s] factionLogoFilter [%s]\n",scenarioDir.c_str(),factionLogoFilter.c_str());

		vector<string> loadScreenList;
		findAll(scenarioDir + factionLogoFilter, loadScreenList, false, false);
		if(loadScreenList.size() > 0) {
			string senarioLogo = scenarioDir + loadScreenList[0];
			if(fileExists(senarioLogo) == true) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] found scenario loading screen '%s'\n",__FILE__,__FUNCTION__,senarioLogo.c_str());

				result = senarioLogo;
				if(logger != NULL) {
					logger->loadLoadingScreen(result);
				}
				loadingImageUsed=true;
			}
		}
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] gameSettings.getScenarioDir() = [%s] gameSettings.getScenario() = [%s] scenarioDir = [%s]\n",__FILE__,__FUNCTION__,__LINE__,settings->getScenarioDir().c_str(),settings->getScenario().c_str(),scenarioDir.c_str());
	}
    return scenarioDir;
}

string Game::extractFactionLogoFile(bool &loadingImageUsed, string factionName,
		string scenarioDir, string techName, Logger *logger, string factionLogoFilter) {
	string result = "";
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Searching for faction loading screen\n",__FILE__,__FUNCTION__,__LINE__);

	if(factionName == formatString(GameConstants::OBSERVER_SLOTNAME)) {
		string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
		const string factionLogo = data_path + "data/core/misc_textures/observer.jpg";
		//printf("In [%s::%s Line: %d] looking for loading screen '%s'\n",__FILE__,__FUNCTION__,__LINE__,factionLogo.c_str());

		if(fileExists(factionLogo) == true) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found loading screen '%s'\n",__FILE__,__FUNCTION__,__LINE__,factionLogo.c_str());

			result = factionLogo;
			if(logger != NULL) {
				logger->loadLoadingScreen(result);
			}
			loadingImageUsed = true;
		}
	}
	//else if(settings->getFactionTypeName(i) == formatString(GameConstants::RANDOMFACTION_SLOTNAME)) {
	else if(factionName == formatString(GameConstants::RANDOMFACTION_SLOTNAME)) {
		string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
		const string factionLogo = data_path + "data/core/misc_textures/random.jpg";

		if(fileExists(factionLogo) == true) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found loading screen '%s'\n",__FILE__,__FUNCTION__,__LINE__,factionLogo.c_str());

			result = factionLogo;
			if(logger != NULL) {
				logger->loadLoadingScreen(result);
			}
			loadingImageUsed = true;
		}
	}
	else {
		Config &config = Config::getInstance();
		vector<string> pathList=config.getPathListForType(ptTechs,scenarioDir);
		for(int idx = 0; idx < pathList.size(); idx++) {
			string currentPath = pathList[idx];
			endPathWithSlash(currentPath);
			//string path = currentPath + techName + "/" + "factions" + "/" + settings->getFactionTypeName(i);
			string path = currentPath + techName + "/" + "factions" + "/" + factionName;
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] possible loading screen dir '%s'\n",__FILE__,__FUNCTION__,__LINE__,path.c_str());
			if(isdir(path.c_str()) == true) {
				endPathWithSlash(path);

				vector<string> loadScreenList;
				findAll(path + factionLogoFilter, loadScreenList, false, false);
				if(loadScreenList.size() > 0) {
					string factionLogo = path + loadScreenList[0];
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] looking for loading screen '%s'\n",__FILE__,__FUNCTION__,__LINE__,factionLogo.c_str());

					if(fileExists(factionLogo) == true) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found loading screen '%s'\n",__FILE__,__FUNCTION__,__LINE__,factionLogo.c_str());

						result = factionLogo;
						if(logger != NULL) {
							logger->loadLoadingScreen(result);
						}
						loadingImageUsed = true;
						break;
					}
				}
			}

			if(loadingImageUsed == true) {
				break;
			}
		}
	}
		//break;
		//}
	//}
	return result;
}

string Game::extractTechLogoFile(string scenarioDir, string techName,
		bool &loadingImageUsed, Logger *logger,string factionLogoFilter) {
	string result = "";
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Searching for tech loading screen\n",__FILE__,__FUNCTION__,__LINE__);
    Config &config = Config::getInstance();
    vector<string> pathList = config.getPathListForType(ptTechs, scenarioDir);
    for(int idx = 0; idx < pathList.size(); idx++) {
		string currentPath = pathList[idx];
		endPathWithSlash(currentPath);
		string path = currentPath + techName;
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] possible loading screen dir '%s'\n",__FILE__,__FUNCTION__,__LINE__,path.c_str());
		if(isdir(path.c_str()) == true) {
			endPathWithSlash(path);

			vector<string> loadScreenList;
			findAll(path + factionLogoFilter, loadScreenList, false, false);
			if(loadScreenList.size() > 0) {
				string factionLogo = path + loadScreenList[0];
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] looking for loading screen '%s'\n",__FILE__,__FUNCTION__,__LINE__,factionLogo.c_str());

				if(fileExists(factionLogo) == true) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found loading screen '%s'\n",__FILE__,__FUNCTION__,__LINE__,factionLogo.c_str());

					result = factionLogo;
					if(logger != NULL) {
						logger->loadLoadingScreen(result);
					}
					loadingImageUsed = true;
					break;
				}
			}
		}
		if(loadingImageUsed == true) {
			break;
		}
	}

    return result;
}

void Game::loadHudTexture(const GameSettings *settings)
{
	string factionName = "";
	string techName			= settings->getTech();
	string scenarioDir =settings->getScenarioDir();

	for(int i=0; i < settings->getFactionCount(); ++i ) {
		if(settings->getFactionControl(i) == ctHuman){
			factionName= settings->getFactionTypeName(i);
			break;
		}
	}
	if(factionName != ""){
		Config &config= Config::getInstance();
		vector<string> pathList= config.getPathListForType(ptTechs, scenarioDir);
		for(int idx= 0; idx < pathList.size(); idx++){
			string currentPath= pathList[idx];
			endPathWithSlash(currentPath);

			vector<string> hudList;
			string path= currentPath + techName + "/" + "factions" + "/" + factionName;
			endPathWithSlash(path);
			findAll(path + "hud.*", hudList, false, false);
			if(hudList.size() > 0){
				string hudImageFileName= path + hudList[0];
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] looking for a HUD '%s'\n",__FILE__,__FUNCTION__,__LINE__,hudImageFileName.c_str());

				if(fileExists(hudImageFileName) == true){
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
						SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found HUD image '%s'\n",__FILE__,__FUNCTION__,__LINE__,hudImageFileName.c_str());

					Texture2D* texture= Renderer::findFactionLogoTexture(hudImageFileName);
					gui.setHudTexture(texture);
					//printf("Hud texture found! \n");
					break;
				}
			}
		}
	}
}

string Game::findFactionLogoFile(const GameSettings *settings, Logger *logger,
		string factionLogoFilter) {
	string result = "";
	if(settings == NULL) {
		result = "";
	}
	string mapName			= settings->getMap();
	string tilesetName		= settings->getTileset();
	string techName			= settings->getTech();
	string scenarioName		= settings->getScenario();
	bool loadingImageUsed	= false;

	if(logger != NULL) {
		logger->setState(Lang::getInstance().get("Loading"));

		if(scenarioName.empty()) {
			logger->setSubtitle(formatString(mapName) + " - " +
					formatString(tilesetName) + " - " + formatString(techName));
		}
		else {
			logger->setSubtitle(formatString(scenarioName));
		}
	}

	string scenarioDir = "";
	bool skipCustomLoadScreen = false;
	if(skipCustomLoadScreen == false) {
		scenarioDir = extractScenarioLogoFile(settings, result, loadingImageUsed,
				logger, factionLogoFilter);
	}
	// try to use a faction related loading screen
	if(skipCustomLoadScreen == false && loadingImageUsed == false) {
		for(int i=0; i < settings->getFactionCount(); ++i ) {
			if( settings->getFactionControl(i) == ctHuman ||
				(settings->getFactionControl(i) == ctNetwork && settings->getThisFactionIndex() == i)) {

				result = extractFactionLogoFile(loadingImageUsed, settings->getFactionTypeName(i),
					scenarioDir, techName, logger, factionLogoFilter);
				break;
			}
		}
	}

	// try to use a tech related loading screen
	if(skipCustomLoadScreen == false && loadingImageUsed == false){
		result = extractTechLogoFile(scenarioDir, techName,
				loadingImageUsed, logger, factionLogoFilter);
	}

	return result;
}

vector<Texture2D *> Game::processTech(string techName) {
	vector<Texture2D *> logoFiles;
	bool enableFactionTexturePreview = Config::getInstance().getBool("FactionPreview","true");
	if(enableFactionTexturePreview) {
		string currentTechName_factionPreview = techName;

		vector<string> factions;
		vector<string> techPaths = Config::getInstance().getPathListForType(ptTechs);
		for(int idx = 0; idx < techPaths.size(); idx++) {
			string &techPath = techPaths[idx];
			endPathWithSlash(techPath);
			findAll(techPath + techName + "/factions/*.", factions, false, false);

			if(factions.size() > 0) {
				for(unsigned int factionIdx = 0; factionIdx < factions.size(); ++factionIdx) {
					bool loadingImageUsed = false;
					string factionLogo = "";
					string currentFactionName_factionPreview = factions[factionIdx];

					factionLogo = Game::extractFactionLogoFile(
							loadingImageUsed,
							currentFactionName_factionPreview,
							"",
							techName,
							NULL,
							"preview_screen.*");

					if(factionLogo == "") {
						factionLogo = Game::extractFactionLogoFile(
								loadingImageUsed,
								currentFactionName_factionPreview,
								"",
								techName,
								NULL,
								"loading_screen.*");
					}
					if(factionLogo != "") {
						Texture2D *texture = Renderer::preloadTexture(factionLogo);
						logoFiles.push_back(texture);
					}
				}
			}
		}
	}

	return logoFiles;
}

void Game::load() {
	load(lgt_All);
}

void Game::load(LoadGameItem loadTypes) {
	std::map<string,vector<pair<string, string> > > loadedFileList;
	originalDisplayMsgCallback = NetworkInterface::getDisplayMessageFunction();
	NetworkInterface::setDisplayMessageFunction(ErrorDisplayMessage);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] loadTypes = %d, gameSettings = [%s]\n",__FILE__,__FUNCTION__,__LINE__,loadTypes,this->gameSettings.toString().c_str());

	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	soundRenderer.stopAllSounds();

	Config &config = Config::getInstance();
	Logger &logger= Logger::getInstance();

	string mapName= gameSettings.getMap();
	string tilesetName= gameSettings.getTileset();
	string techName= gameSettings.getTech();
	string scenarioName= gameSettings.getScenario();

	if((loadTypes & lgt_FactionPreview) == lgt_FactionPreview) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		Game::findFactionLogoFile(&gameSettings, &logger);

		Window::handleEvent();
		SDL_PumpEvents();
	}

	loadHudTexture(&gameSettings);

    string scenarioDir = "";
    if(gameSettings.getScenarioDir() != "") {
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
        scenarioDir = gameSettings.getScenarioDir();
        if(EndsWith(scenarioDir, ".xml") == true) {
            scenarioDir = scenarioDir.erase(scenarioDir.size() - 4, 4);
            scenarioDir = scenarioDir.erase(scenarioDir.size() - gameSettings.getScenario().size(), gameSettings.getScenario().size() + 1);
        }
    }

	//throw runtime_error("Test!");
    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//tileset
	if((loadTypes & lgt_TileSet) == lgt_TileSet) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		world.loadTileset(	config.getPathListForType(ptTilesets,scenarioDir),
    						tilesetName, &checksum, loadedFileList);
	}

    // give CPU time to update other things to avoid apperance of hanging
    sleep(0);
    Window::handleEvent();
	SDL_PumpEvents();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    set<string> factions;
    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	for ( int i=0; i < gameSettings.getFactionCount(); ++i ) {
		factions.insert(gameSettings.getFactionTypeName(i));
	}

    if((loadTypes & lgt_TechTree) == lgt_TechTree) {
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//tech, load before map because of resources
		world.loadTech(	config.getPathListForType(ptTechs,scenarioDir), techName,
						factions, &checksum,loadedFileList);

		// Validate the faction setup to ensure we don't have any bad associations
		/*
		std::vector<std::string> results = world.validateFactionTypes();
		if(results.size() > 0) {
			// Display the validation errors
			string errorText = "Errors were detected:\n";
			for(int i = 0; i < results.size(); ++i) {
				if(i > 0) {
					errorText += "\n";
				}
				errorText += results[i];
			}
			throw runtime_error(errorText);
		}
		*/
    }

    // give CPU time to update other things to avoid apperance of hanging
    sleep(0);
    Window::handleEvent();
	SDL_PumpEvents();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    //map
    if((loadTypes & lgt_Map) == lgt_Map) {
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    	world.loadMap(Map::getMapPath(mapName,scenarioDir), &checksum);
    }

    // give CPU time to update other things to avoid apperance of hanging
    sleep(0);
    Window::handleEvent();
	SDL_PumpEvents();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    //scenario
    if((loadTypes & lgt_Scenario) == lgt_Scenario) {
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		if(scenarioName.empty() == false) {
			Lang::getInstance().loadScenarioStrings(gameSettings.getScenarioDir(), scenarioName);
			world.loadScenario(gameSettings.getScenarioDir(), &checksum);
		}
    }

    // give CPU time to update other things to avoid apperance of hanging
    sleep(0);
	SDL_PumpEvents();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    //good_fpu_control_registers(NULL,__FILE__,__FUNCTION__,__LINE__);
}

void Game::init() {
	init(false);
}

void Game::init(bool initForPreviewOnly)
{
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] initForPreviewOnly = %d\n",__FILE__,__FUNCTION__,__LINE__,initForPreviewOnly);

	Lang &lang= Lang::getInstance();
	Logger &logger= Logger::getInstance();
	CoreData &coreData= CoreData::getInstance();
	Renderer &renderer= Renderer::getInstance();
	Map *map= world.getMap();
	NetworkManager &networkManager= NetworkManager::getInstance();

	if(map == NULL) {
		throw runtime_error("map == NULL");
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(initForPreviewOnly == false) {
		logger.setState(lang.get("Initializing"));

		//mesage box
		mainMessageBox.init(lang.get("Yes"), lang.get("No"));
		mainMessageBox.setEnabled(false);

		//mesage box
		errorMessageBox.init(lang.get("Ok"));
		errorMessageBox.setEnabled(false);
		errorMessageBox.setY(mainMessageBox.getY() - mainMessageBox.getH() - 10);

		//init world, and place camera
		commander.init(&world);

		// give CPU time to update other things to avoid apperance of hanging
		sleep(0);
		Window::handleEvent();
		SDL_PumpEvents();
	}

	world.init(this, gameSettings.getDefaultUnits());

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(initForPreviewOnly == false) {
		// give CPU time to update other things to avoid apperance of hanging
		sleep(0);
		Window::handleEvent();
		SDL_PumpEvents();

		gui.init(this);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		// give CPU time to update other things to avoid apperance of hanging
		sleep(0);
		//SDL_PumpEvents();

		chatManager.init(&console, world.getThisTeamIndex());
		console.clearStoredLines();
	}

	gameCamera.init(map->getW(), map->getH());

	// camera default height calculation
	if(map->getCameraHeight()>0 && gameCamera.getCalculatedDefault()<map->getCameraHeight()){
		gameCamera.setCalculatedDefault(map->getCameraHeight());
	}
	else if(gameCamera.getCalculatedDefault()<map->getMaxMapHeight()+13.0f){
		gameCamera.setCalculatedDefault(map->getMaxMapHeight()+13.0f);
	}

	if(world.getThisFaction() != NULL) {
		const Vec2i &v= map->getStartLocation(world.getThisFaction()->getStartLocationIndex());
		gameCamera.setPos(Vec2f(v.x, v.y));
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(initForPreviewOnly == false) {
		// give CPU time to update other things to avoid apperance of hanging
		sleep(0);
		Window::handleEvent();
		SDL_PumpEvents();

		scriptManager.init(&world, &gameCamera);

		//good_fpu_control_registers(NULL,__FILE__,__FUNCTION__,__LINE__);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] creating AI's\n",__FILE__,__FUNCTION__,__LINE__);

		//create AIs

		bool enableServerControlledAI 	= this->gameSettings.getEnableServerControlledAI();
		bool isNetworkGame 				= this->gameSettings.isNetworkGame();
		NetworkRole role 				= networkManager.getNetworkRole();

		deleteValues(aiInterfaces.begin(), aiInterfaces.end());

		aiInterfaces.resize(world.getFactionCount());
		for(int i=0; i < world.getFactionCount(); ++i) {
			Faction *faction= world.getFaction(i);
			if(faction->getCpuControl(enableServerControlledAI,isNetworkGame,role) == true) {
				aiInterfaces[i]= new AiInterface(*this, i, faction->getTeam());
				logger.add("Creating AI for faction " + intToStr(i), true);
			}
			else {
				aiInterfaces[i]= NULL;
			}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);


		// give CPU time to update other things to avoid apperance of hanging
		sleep(0);
		Window::handleEvent();
		SDL_PumpEvents();

		//weather particle systems
		if(world.getTileset()->getWeather() == wRainy) {
			logger.add("Creating rain particle system", true);
			weatherParticleSystem= new RainParticleSystem();
			weatherParticleSystem->setSpeed(12.f/GameConstants::updateFps);
			weatherParticleSystem->setPos(gameCamera.getPos());
			renderer.manageParticleSystem(weatherParticleSystem, rsGame);
		}
		else if(world.getTileset()->getWeather() == wSnowy) {
			logger.add("Creating snow particle system", true);
			weatherParticleSystem= new SnowParticleSystem(1200);
			weatherParticleSystem->setSpeed(1.5f/GameConstants::updateFps);
			weatherParticleSystem->setPos(gameCamera.getPos());
			weatherParticleSystem->setTexture(coreData.getSnowTexture());
			renderer.manageParticleSystem(weatherParticleSystem, rsGame);
		}
    }

	//init renderer state
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Initializing renderer\n",__FILE__,__FUNCTION__);
	logger.add("Initializing renderer", true);
	renderer.initGame(this);

	for(int i=0; i < world.getFactionCount(); ++i) {
		Faction *faction= world.getFaction(i);
		if(faction != NULL) {
			faction->deletePixels();
		}
	}

	if(initForPreviewOnly == false) {
		//good_fpu_control_registers(NULL,__FILE__,__FUNCTION__,__LINE__);

		// give CPU time to update other things to avoid apperance of hanging
		sleep(0);
		Window::handleEvent();
		SDL_PumpEvents();

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Waiting for network\n",__FILE__,__FUNCTION__);
		logger.add("Waiting for network players", true);
		networkManager.getGameNetworkInterface()->waitUntilReady(&checksum);

		//std::string worldLog = world.DumpWorldToLog(true);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Starting music stream\n",__FILE__,__FUNCTION__,__LINE__);
		logger.add("Starting music stream", true);

		if(world.getThisFaction() == NULL) {
			throw runtime_error("world.getThisFaction() == NULL");
		}
		if(world.getThisFaction()->getType() == NULL) {
			throw runtime_error("world.getThisFaction()->getType() == NULL");
		}
		//if(world.getThisFaction()->getType()->getMusic() == NULL) {
		//	throw runtime_error("world.getThisFaction()->getType()->getMusic() == NULL");
		//}

		//sounds
		SoundRenderer &soundRenderer= SoundRenderer::getInstance();
		soundRenderer.stopAllSounds();
		soundRenderer= SoundRenderer::getInstance();

		Tileset *tileset= world.getTileset();
		AmbientSounds *ambientSounds= tileset->getAmbientSounds();

		//rain
		if(tileset->getWeather() == wRainy && ambientSounds->isEnabledRain()) {
			logger.add("Starting ambient stream", true);
			soundRenderer.playAmbient(ambientSounds->getRain());
		}

		//snow
		if(tileset->getWeather() == wSnowy && ambientSounds->isEnabledSnow()) {
			logger.add("Starting ambient stream", true);
			soundRenderer.playAmbient(ambientSounds->getSnow());
		}

		StrSound *gameMusic= world.getThisFaction()->getType()->getMusic();
		soundRenderer.playMusic(gameMusic);

		logger.add("Launching game");
	}

	logger.setCancelLoadingEnabled(false);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"================ STARTING GAME ================\n");
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPathFinder).enabled) SystemFlags::OutputDebug(SystemFlags::debugPathFinder,"================ STARTING GAME ================\n");
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPathFinder).enabled) SystemFlags::OutputDebug(SystemFlags::debugPathFinder,"PathFinderType: %s\n", (getGameSettings()->getPathFinderType() ? "RoutePlanner" : "PathFinder"));

	gameStarted = true;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] ==== START GAME ==== getCurrentPixelByteCount() = %llu\n",__FILE__,__FUNCTION__,__LINE__,(long long unsigned int)renderer.getCurrentPixelByteCount());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled) SystemFlags::OutputDebug(SystemFlags::debugWorldSynch,"==== START GAME ====\n");
}

// ==================== update ====================

//update
void Game::update() {
	try {
		Chrono chrono;
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

		// a) Updates non dependent on speed

		if(NetworkManager::getInstance().getGameNetworkInterface() != NULL &&
			NetworkManager::getInstance().getGameNetworkInterface()->getQuit() &&
		   mainMessageBox.getEnabled() == false &&
		   errorMessageBox.getEnabled() == false) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			quitTriggeredIndicator = true;
			return;
		}

		//misc
		updateFps++;
		mouse2d= (mouse2d+1) % Renderer::maxMouse2dAnim;

		//console
		console.update();

		// b) Updates depandant on speed
		int updateLoops= getUpdateLoops();

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

		NetworkManager &networkManager= NetworkManager::getInstance();
		bool enableServerControlledAI 	= this->gameSettings.getEnableServerControlledAI();
		bool isNetworkGame 				= this->gameSettings.isNetworkGame();
		NetworkRole role 				= networkManager.getNetworkRole();

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld [before ReplaceDisconnectedNetworkPlayersWithAI]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

		// Check to see if we are playing a network game and if any players
		// have disconnected?
		ReplaceDisconnectedNetworkPlayersWithAI(isNetworkGame, role);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld [after ReplaceDisconnectedNetworkPlayersWithAI]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());

		if(updateLoops>0){
			// update the frame based timer in the stats with at least one step
			world.getStats()->addFramesToCalculatePlaytime();
		}
		//update
		for(int i = 0; i < updateLoops; ++i) {
			chrono.start();
			//AiInterface
			for(int j = 0; j < world.getFactionCount(); ++j) {
				Faction *faction = world.getFaction(j);
				if(	faction->getCpuControl(enableServerControlledAI,isNetworkGame,role) == true &&
					scriptManager.getPlayerModifiers(j)->getAiEnabled() == true) {

					if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] [i = %d] faction = %d, factionCount = %d, took msecs: %lld [before AI updates]\n",__FILE__,__FUNCTION__,__LINE__,i,j,world.getFactionCount(),chrono.getMillis());

					aiInterfaces[j]->update();

					if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] [i = %d] faction = %d, factionCount = %d, took msecs: %lld [after AI updates]\n",__FILE__,__FUNCTION__,__LINE__,i,j,world.getFactionCount(),chrono.getMillis());
				}
			}

			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld [AI updates]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

			//World
			world.update();
			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld [world update i = %d]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),i);
			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

			// Commander
			//commander.updateNetwork();
			commander.signalNetworkUpdate(this);

			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld [commander updateNetwork i = %d]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),i);
			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

			//Gui
			gui.update();
			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld [gui updating i = %d]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),i);
			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

			//Particle systems
			if(weatherParticleSystem != NULL) {
				weatherParticleSystem->setPos(gameCamera.getPos());
			}

			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld [weather particle updating i = %d]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),i);
			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

			Renderer &renderer= Renderer::getInstance();
			renderer.updateParticleManager(rsGame,avgRenderFps);
			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld [particle manager updating i = %d]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),i);
			if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

			//good_fpu_control_registers(NULL,__FILE__,__FUNCTION__,__LINE__);
		}

		//call the chat manager
		chatManager.updateNetwork();
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld [chatManager.updateNetwork]\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

		//check for quiting status
		if(NetworkManager::getInstance().getGameNetworkInterface() != NULL &&
			NetworkManager::getInstance().getGameNetworkInterface()->getQuit() &&
		   mainMessageBox.getEnabled() == false &&
		   errorMessageBox.getEnabled() == false) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			quitTriggeredIndicator = true;
			return;
		}

		//update auto test
		if(Config::getInstance().getBool("AutoTest")){
			AutoTest::getInstance().updateGame(this);
		}
	}
	catch(const exception &ex) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());

		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,szBuf);

		NetworkManager &networkManager= NetworkManager::getInstance();
		if(networkManager.getGameNetworkInterface() != NULL) {
			GameNetworkInterface *networkInterface = NetworkManager::getInstance().getGameNetworkInterface();
			networkInterface->sendTextMessage(szBuf,-1,true,"");
			sleep(10);
			networkManager.getGameNetworkInterface()->quitGame(true);
		}
		if(errorMessageBox.getEnabled() == false) {
            ErrorDisplayMessage(ex.what(),true);
		}
	}
}

void Game::ReplaceDisconnectedNetworkPlayersWithAI(bool isNetworkGame, NetworkRole role) {
	if(role == nrServer && isNetworkGame == true) {
		Logger &logger= Logger::getInstance();
		ServerInterface *server = NetworkManager::getInstance().getServerInterface();

		for(int i = 0; i < world.getFactionCount(); ++i) {
			Faction *faction = world.getFaction(i);
			if(	faction->getFactionDisconnectHandled() == false &&
				(faction->getControlType() == ctNetwork ||
				faction->getControlType() == ctNetworkCpuEasy ||
				faction->getControlType() == ctNetworkCpu ||
				faction->getControlType() == ctNetworkCpuUltra ||
				faction->getControlType() == ctNetworkCpuMega)) {
				ConnectionSlot *slot =  server->getSlot(faction->getStartLocationIndex());
				if(aiInterfaces[i] == NULL && (slot == NULL || slot->isConnected() == false)) {
					faction->setFactionDisconnectHandled(true);

					char szBuf[255]="";
					if(faction->getType()->getPersonalityType() != fpt_Observer) {
						faction->setControlType(ctCpu);
						aiInterfaces[i] = new AiInterface(*this, i, faction->getTeam(), faction->getStartLocationIndex());
						logger.add("Creating AI for faction " + intToStr(i), true);

						Lang &lang= Lang::getInstance();
						string msg = "Player #%d [%s] has disconnected, switching player to AI mode!";
						if(lang.hasString("GameSwitchPlayerToAI")) {
							msg = lang.get("GameSwitchPlayerToAI");
						}
						sprintf(szBuf,msg.c_str(),i+1,this->gameSettings.getNetworkPlayerName(i).c_str());
					}
					else {
						Lang &lang= Lang::getInstance();
						string msg = "Player #%d [%s] has disconnected, but player was only an observer!";
						if(lang.hasString("GameSwitchPlayerObserverToAI")) {
							msg = lang.get("GameSwitchPlayerObserverToAI");
						}
						sprintf(szBuf,msg.c_str(),i+1,this->gameSettings.getNetworkPlayerName(i).c_str());
					}

					Lang &lang= Lang::getInstance();
					const vector<string> languageList = this->gameSettings.getUniqueNetworkPlayerLanguages();
					for(unsigned int j = 0; j < languageList.size(); ++j) {
						bool localEcho = (languageList[j] == lang.getLanguage());
						server->sendTextMessage(szBuf,-1,localEcho,languageList[j]);
					}
				}
			}
		}
	}
}

void Game::updateCamera(){
	gameCamera.update();
}


// ==================== render ====================

//render
void Game::render() {
	// Ensure the camera starts in the right position
	if(isFirstRender == true) {
		isFirstRender = false;

		gameCamera.resetPosition();
		this->restoreToStartXY();
	}

	renderFps++;
	totalRenderFps++;
	renderWorker();
}

void Game::renderWorker() {
	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	render3d();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] renderFps = %d took msecs: %d [render3d]\n",__FILE__,__FUNCTION__,__LINE__,renderFps,chrono.getMillis());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

	render2d();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] renderFps = %d took msecs: %d [render2d]\n",__FILE__,__FUNCTION__,__LINE__,renderFps,chrono.getMillis());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

	Renderer::getInstance().swapBuffers();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] renderFps = %d took msecs: %d [swap buffers]\n",__FILE__,__FUNCTION__,__LINE__,renderFps,chrono.getMillis());
}

// ==================== tick ====================

void Game::tick() {
	tickCount++;

	if(avgUpdateFps == -1) {
		avgUpdateFps = updateFps;
	}
	else {
		avgUpdateFps = (avgUpdateFps + updateFps) / 2;
	}
	currentAvgRenderFpsTotal += renderFps;

	if(avgRenderFps == -1) {
		avgRenderFps = renderFps;
	}
	// Update the average every x game ticks
	const int CHECK_AVG_FPS_EVERY_X_TICKS = 15;
	if(tickCount % CHECK_AVG_FPS_EVERY_X_TICKS == 0) {
		avgRenderFps = currentAvgRenderFpsTotal / CHECK_AVG_FPS_EVERY_X_TICKS;
		currentAvgRenderFpsTotal = 0;
	}

	if(captureAvgTestStatus == true) {
		if(updateFpsAvgTest == -1) {
			updateFpsAvgTest = updateFps;
		}
		else {
			updateFpsAvgTest = (updateFpsAvgTest + updateFps) / 2;
		}
		if(renderFpsAvgTest == -1) {
			renderFpsAvgTest = renderFps;
		}
		else {
			renderFpsAvgTest = (renderFpsAvgTest + renderFps) / 2;
		}
	}

	lastUpdateFps= updateFps;
	lastRenderFps= renderFps;
	updateFps= 0;
	renderFps= 0;

	//Win/lose check
	checkWinner();
	gui.tick();
}


// ==================== events ====================

void Game::mouseDownLeft(int x, int y) {
	try {
		if(gameStarted == false) {
			Logger::getInstance().handleMouseClick(x, y);
			return;
		}

		Map *map= world.getMap();
		const Metrics &metrics= Metrics::getInstance();
		NetworkManager &networkManager= NetworkManager::getInstance();
		bool messageBoxClick= false;

		//scrip message box, only if the exit box is not enabled
		if( mainMessageBox.getEnabled() == false &&
			errorMessageBox.getEnabled() == false &&
			scriptManager.getMessageBox()->getEnabled()) {
			int button= 1;
			if(scriptManager.getMessageBox()->mouseClick(x, y, button)){
				scriptManager.onMessageBoxOk();
				messageBoxClick= true;
			}
		}

		//minimap panel
		if(messageBoxClick == false) {
			if(metrics.isInMinimap(x, y)){
				int xm= x - metrics.getMinimapX();
				int ym= y - metrics.getMinimapY();
				int xCell= static_cast<int>(xm * (static_cast<float>(map->getW()) / metrics.getMinimapW()));
				int yCell= static_cast<int>(map->getH() - ym * (static_cast<float>(map->getH()) / metrics.getMinimapH()));

				if(map->isInside(xCell, yCell) && map->isInsideSurface(map->toSurfCoords(Vec2i(xCell,yCell)))) {
					if(gui.isSelectingPos()){
						gui.mouseDownLeftGraphics(xCell, yCell, true);
					}
					else
					{
						gameCamera.setPos(Vec2f(static_cast<float>(xCell), static_cast<float>(yCell)));
					}
				}
			}
			//display panel
			else if(metrics.isInDisplay(x, y) && !gui.isSelectingPos()) {
				int xd= x - metrics.getDisplayX();
				int yd= y - metrics.getDisplayY();
				if(gui.mouseValid(xd, yd)) {
					gui.mouseDownLeftDisplay(xd, yd);
				}
				else {
					gui.mouseDownLeftGraphics(x, y, false);
				}
			}
			//graphics panel
			else {
				gui.mouseDownLeftGraphics(x, y, false);
			}
		}

		//exit message box, has to be the last thing to do in this function
		if(errorMessageBox.getEnabled() == true) {
			if(errorMessageBox.mouseClick(x, y)) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				//close message box
				errorMessageBox.setEnabled(false);
			}
		}
		if(mainMessageBox.getEnabled()){
			int button= 1;
			if(mainMessageBox.mouseClick(x, y, button)) {
				if(button==1) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					if(networkManager.getGameNetworkInterface() != NULL) {
						networkManager.getGameNetworkInterface()->quitGame(true);
					}
					quitTriggeredIndicator = true;
					return;
				}
				else {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					//close message box
					mainMessageBox.setEnabled(false);
				}
			}
		}
	}
	catch(const exception &ex) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());

		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,szBuf);

		NetworkManager &networkManager= NetworkManager::getInstance();
		if(networkManager.getGameNetworkInterface() != NULL) {
			GameNetworkInterface *networkInterface = NetworkManager::getInstance().getGameNetworkInterface();
			networkInterface->sendTextMessage(szBuf,-1,true,"");
			sleep(10);
			networkManager.getGameNetworkInterface()->quitGame(true);
		}
		ErrorDisplayMessage(ex.what(),true);
	}
}

void Game::mouseDownRight(int x, int y) {
	try {
		if(gameStarted == false) {
			Logger::getInstance().handleMouseClick(x, y);
			return;
		}

		Map *map= world.getMap();
		const Metrics &metrics= Metrics::getInstance();

		if(metrics.isInMinimap(x, y) ){
			int xm= x - metrics.getMinimapX();
			int ym= y - metrics.getMinimapY();
			int xCell= static_cast<int>(xm * (static_cast<float>(map->getW()) / metrics.getMinimapW()));
			int yCell= static_cast<int>(map->getH() - ym * (static_cast<float>(map->getH()) / metrics.getMinimapH()));

			if(map->isInside(xCell, yCell) && map->isInsideSurface(map->toSurfCoords(Vec2i(xCell,yCell)))) {
				gui.mouseDownRightGraphics(xCell, yCell,true);
			}
		}
		else {
			Vec2i targetPos;
			Vec2i screenPos(x,y);
			Renderer &renderer= Renderer::getInstance();
			renderer.computePosition(screenPos, targetPos);
			if(renderer.computePosition(screenPos, targetPos) == true &&
				map->isInsideSurface(map->toSurfCoords(targetPos)) == true) {
				gui.mouseDownRightGraphics(x, y,false);
			}
		}
	}
	catch(const exception &ex) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] Error [%s] x = %d y = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what(),x,y);

		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,szBuf);

		NetworkManager &networkManager= NetworkManager::getInstance();
		if(networkManager.getGameNetworkInterface() != NULL) {
			GameNetworkInterface *networkInterface = NetworkManager::getInstance().getGameNetworkInterface();
			networkInterface->sendTextMessage(szBuf,-1,true,"");
			sleep(10);
			networkManager.getGameNetworkInterface()->quitGame(true);
		}
		ErrorDisplayMessage(ex.what(),true);
	}
}

 void Game::mouseUpCenter(int x, int y) {
	if(gameStarted == false) {
		return;
	}

 	if(mouseMoved == false) {
 		gameCamera.resetPosition();
 	}
 	else {
 		mouseMoved = false;
 	}
}

void Game::mouseUpLeft(int x, int y) {
	try {
		if(gameStarted == false) {
			return;
		}

		gui.mouseUpLeftGraphics(x, y);
	}
	catch(const exception &ex) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());

		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,szBuf);

		NetworkManager &networkManager= NetworkManager::getInstance();
		if(networkManager.getGameNetworkInterface() != NULL) {
			GameNetworkInterface *networkInterface = NetworkManager::getInstance().getGameNetworkInterface();
			networkInterface->sendTextMessage(szBuf,-1,true,"");
			sleep(10);
			networkManager.getGameNetworkInterface()->quitGame(true);
		}
		ErrorDisplayMessage(ex.what(),true);
	}
}

void Game::mouseDoubleClickLeft(int x, int y) {
	try {
		if(gameStarted == false) {
			return;
		}

		const Metrics &metrics= Metrics::getInstance();

		//display panel
		if(metrics.isInDisplay(x, y) && !gui.isSelectingPos()) {
			int xd= x - metrics.getDisplayX();
			int yd= y - metrics.getDisplayY();
			if(gui.mouseValid(xd, yd)){
				return;
			}
		}

		//graphics panel
		gui.mouseDoubleClickLeftGraphics(x, y);
	}
	catch(const exception &ex) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());

		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,szBuf);

		NetworkManager &networkManager= NetworkManager::getInstance();
		if(networkManager.getGameNetworkInterface() != NULL) {
			GameNetworkInterface *networkInterface = NetworkManager::getInstance().getGameNetworkInterface();
			networkInterface->sendTextMessage(szBuf,-1,true,"");
			sleep(10);
			networkManager.getGameNetworkInterface()->quitGame(true);
		}
		ErrorDisplayMessage(ex.what(),true);
	}
}

void Game::mouseMove(int x, int y, const MouseState *ms) {
	try {
		if(gameStarted == false) {
			return;
		}

		const Metrics &metrics = Metrics::getInstance();

		mouseX = x;
		mouseY = y;

		if (ms->get(mbCenter)) {
			//if (input.isCtrlDown()) {
			//	float speed = input.isShiftDown() ? 1.f : 0.125f;
			//	float response = input.isShiftDown() ? 0.1875f : 0.0625f;
			//	gameCamera.moveForwardH((y - lastMousePos.y) * speed, response);
			//	gameCamera.moveSideH((x - lastMousePos.x) * speed, response);
			//} else
			mouseMoved = true;
			{
				//float ymult = Config::getInstance().getCameraInvertYAxis() ? -0.2f : 0.2f;
				//float xmult = Config::getInstance().getCameraInvertXAxis() ? -0.2f : 0.2f;
				float ymult = 0.2f;
				float xmult = 0.2f;

				//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

				gameCamera.transitionVH(-(y - lastMousePos.y) * ymult, (lastMousePos.x - x) * xmult);
				mouseX=lastMousePos.x;
				mouseY=lastMousePos.y;
				Window::revertMousePos();

				//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				return;
			}
		}
		else {
			//main window
			//if(Window::isKeyDown() == false)
			if(!camLeftButtonDown && !camRightButtonDown && !camUpButtonDown && !camDownButtonDown)
			{
				if (y < 10) {
					gameCamera.setMoveZ(-scrollSpeed);
				}
				else if (y > metrics.getVirtualH() - 10) {
					gameCamera.setMoveZ(scrollSpeed);
				}
				else {
					gameCamera.setMoveZ(0);
				}

				if (x < 10) {
					gameCamera.setMoveX(-scrollSpeed);
				}
				else if (x > metrics.getVirtualW() - 10) {
					gameCamera.setMoveX(scrollSpeed);
				}
				else {
					gameCamera.setMoveX(0);
				}
			}

			if (mainMessageBox.getEnabled()) {
				mainMessageBox.mouseMove(x, y);
			}
			if (errorMessageBox.getEnabled()) {
				errorMessageBox.mouseMove(x, y);
			}
			if (scriptManager.getMessageBox()->getEnabled()) {
				scriptManager.getMessageBox()->mouseMove(x, y);
			}
			//else if (saveBox) {
			//	saveBox->mouseMove(x, y);
			//} else {
			//	//graphics
			gui.mouseMoveGraphics(x, y);
			//}
		}

		//display
		if ( !gui.isSelecting() && !gui.isSelectingPos()) {
			if (!gui.isSelectingPos()) {
				if(metrics.isInDisplay(x, y)){
					gui.mouseMoveDisplay(x - metrics.getDisplayX(), y - metrics.getDisplayY());
				}
				else {
					gui.mouseMoveOutsideDisplay();
				}
			}
		}

		lastMousePos.x = mouseX;
		lastMousePos.y = mouseY;
	}
	catch(const exception &ex) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());

		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,szBuf);

		NetworkManager &networkManager= NetworkManager::getInstance();
		if(networkManager.getGameNetworkInterface() != NULL) {
			GameNetworkInterface *networkInterface = NetworkManager::getInstance().getGameNetworkInterface();
			networkInterface->sendTextMessage(szBuf,-1,true,"");
			sleep(10);
			networkManager.getGameNetworkInterface()->quitGame(true);
		}
		ErrorDisplayMessage(ex.what(),true);
	}
}

void Game::eventMouseWheel(int x, int y, int zDelta) {
	try {
		//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		//gameCamera.transitionXYZ(0.0f, -(float)zDelta / 30.0f, 0.0f);
		gameCamera.zoom((float)zDelta / 60.0f);
		//gameCamera.setMoveY(1);
	}
	catch(const exception &ex) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());

		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,szBuf);

		NetworkManager &networkManager= NetworkManager::getInstance();
		if(networkManager.getGameNetworkInterface() != NULL) {
			GameNetworkInterface *networkInterface = NetworkManager::getInstance().getGameNetworkInterface();
			networkInterface->sendTextMessage(szBuf,-1,true,"");
			sleep(10);
			networkManager.getGameNetworkInterface()->quitGame(true);
		}
		ErrorDisplayMessage(ex.what(),true);
	}
}

void Game::keyDown(char key) {
	try {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c] [%d] gameStarted [%d]\n",__FILE__,__FUNCTION__,__LINE__,key,key, gameStarted);
		if(gameStarted == false) {
			return;
		}

		Lang &lang= Lang::getInstance();

		//send key to the chat manager
		chatManager.keyDown(key);

		if(chatManager.getEditEnabled() == false) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%d - %c]\n",__FILE__,__FUNCTION__,__LINE__,key,key);

			Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
			//if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] key = [%d - %c] pausegame [%d]\n",__FILE__,__FUNCTION__,__LINE__,key,key,configKeys.getCharKey("PauseGame"));

			if(key == configKeys.getCharKey("RenderNetworkStatus")) {
				renderNetworkStatus= !renderNetworkStatus;
			}
			else if(key == configKeys.getCharKey("ShowFullConsole")) {
				showFullConsole= true;
			}
			else if(key == configKeys.getCharKey("TogglePhotoMode")) {
				photoModeEnabled = !photoModeEnabled;
				if(	photoModeEnabled == true &&
					this->gameSettings.isNetworkGame() == false) {
					gameCamera.setMaxHeight(PHOTO_MODE_MAXHEIGHT);
				}
				else if(photoModeEnabled == false) {
					gameCamera.setMaxHeight(-1);
				}

			}
			//Toggle music
			else if(key == configKeys.getCharKey("ToggleMusic")) {
				Config &config = Config::getInstance();
				StrSound *gameMusic = world.getThisFaction()->getType()->getMusic();
				if(gameMusic != NULL) {
					float configVolume = (config.getInt("SoundVolumeMusic") / 100.f);
					float currentVolume = gameMusic->getVolume();
					if(currentVolume > 0) {
						gameMusic->setVolume(0);
						console.addLine(lang.get("GameMusic") + " " + lang.get("Off"));
					}
					else {
						//If the config says zero, use the default music volume
						gameMusic->setVolume(configVolume ? configVolume : 0.9);
						console.addLine(lang.get("GameMusic"));
					}
				}
			}
			//move camera left
			else if(key == configKeys.getCharKey("CameraModeLeft")) {
				gameCamera.setMoveX(-1);
				camLeftButtonDown=true;
			}
			//move camera right
			else if(key == configKeys.getCharKey("CameraModeRight")) {
				gameCamera.setMoveX(1);
				camRightButtonDown=true;
			}
			//move camera up
			else if(key == configKeys.getCharKey("CameraModeUp")) {
				gameCamera.setMoveZ(1);
				camUpButtonDown=true;
			}
			//move camera down
			else if(key == configKeys.getCharKey("CameraModeDown")) {
				gameCamera.setMoveZ(-1);
				camDownButtonDown=true;
			}
			//change camera mode
			else if(key == configKeys.getCharKey("FreeCameraMode")) {
				gameCamera.switchState();
				string stateString= gameCamera.getState()==GameCamera::sGame? lang.get("GameCamera"): lang.get("FreeCamera");
				console.addLine(lang.get("CameraModeSet")+" "+ stateString);
			}
			//reset camera mode to normal
			else if(key == configKeys.getCharKey("ResetCameraMode")) {
				gameCamera.resetPosition();
			}
			//pause
			else if(key == configKeys.getCharKey("PauseGame")) {
				//printf("Toggle pause paused = %d\n",paused);
				setPaused(!paused);
			}
			//switch display color
			else if(key == configKeys.getCharKey("ChangeFontColor")) {
				gui.switchToNextDisplayColor();
			}
			//increment speed
			else if(key == configKeys.getCharKey("GameSpeedIncrease")) {
				bool speedChangesAllowed= !NetworkManager::getInstance().isNetworkGame();
				if(speedChangesAllowed){
					incSpeed();
				}
			}
			//decrement speed
			else if(key == configKeys.getCharKey("GameSpeedDecrease")) {
				bool speedChangesAllowed= !NetworkManager::getInstance().isNetworkGame();
				if(speedChangesAllowed){
					decSpeed();
				}
			}
			//exit
			else if(key == configKeys.getCharKey("ExitKey")) {
				showMessageBox(lang.get("ExitGame?"), "", true);
			}
			//group
			//else if(key>='0' && key<'0'+Selection::maxGroups){
			else {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = %d\n",__FILE__,__FUNCTION__,__LINE__,key);

				for(int idx = 1; idx <= Selection::maxGroups; idx++) {
					string keyName = "GroupUnitsKey" + intToStr(idx);
					char groupHotKey = configKeys.getCharKey(keyName.c_str());
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] keyName [%s] group index = %d, key = [%c] [%d]\n",__FILE__,__FUNCTION__,__LINE__,keyName.c_str(),idx,groupHotKey,groupHotKey);

					if(key == groupHotKey) {
						//gui.groupKey(key-'0');
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
						gui.groupKey(idx-1);
						break;
					}
				}
			}

			//hotkeys
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] gameCamera.getState() = %d\n",__FILE__,__FUNCTION__,__LINE__,gameCamera.getState());

			if(gameCamera.getState() == GameCamera::sGame){
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = %d\n",__FILE__,__FUNCTION__,__LINE__,key);

				gui.hotKey(key);
			}
			else {
				//rotate camera leftt
				if(key == configKeys.getCharKey("CameraRotateLeft")) {
					gameCamera.setRotate(-1);
				}
				//rotate camera right
				else if(key == configKeys.getCharKey("CameraRotateRight")){
					gameCamera.setRotate(1);
				}
				//camera up
				else if(key == configKeys.getCharKey("CameraRotateUp")) {
					gameCamera.setMoveY(1);
				}
				//camera down
				else if(key == configKeys.getCharKey("CameraRotateDown")) {
					gameCamera.setMoveY(-1);
				}
			}
		}

		//throw runtime_error("Test Error!");
	}
	catch(const exception &ex) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());

		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,szBuf);

		NetworkManager &networkManager= NetworkManager::getInstance();
		if(networkManager.getGameNetworkInterface() != NULL) {
			GameNetworkInterface *networkInterface = NetworkManager::getInstance().getGameNetworkInterface();
			networkInterface->sendTextMessage(szBuf,-1,true,"");
			sleep(10);
			networkManager.getGameNetworkInterface()->quitGame(true);
		}
		ErrorDisplayMessage(ex.what(),true);
	}
}

void Game::keyUp(char key){
	try {
		if(gameStarted == false) {
			return;
		}

		if(chatManager.getEditEnabled()) {
			//send key to the chat manager
			chatManager.keyUp(key);
		}
		else {
			Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));

			if(key == configKeys.getCharKey("ShowFullConsole")) {
				showFullConsole= false;
			}
			else if(key == configKeys.getCharKey("CameraRotateLeft") ||
					key == configKeys.getCharKey("CameraRotateRight")) {
				gameCamera.setRotate(0);
			}
			else if(key == configKeys.getCharKey("CameraRotateDown") ||
					key == configKeys.getCharKey("CameraRotateUp")) {
				gameCamera.setMoveY(0);
			}
			else if(key == configKeys.getCharKey("CameraModeUp")){
				gameCamera.setMoveZ(0);
				camUpButtonDown= false;
				calcCameraMoveZ();
			}
			else if(key == configKeys.getCharKey("CameraModeDown")){
				gameCamera.setMoveZ(0);
				camDownButtonDown= false;
				calcCameraMoveZ();
			}

			else if(key == configKeys.getCharKey("CameraModeLeft")){
				gameCamera.setMoveX(0);
				camLeftButtonDown= false;
				calcCameraMoveX();
			}
			else if(key == configKeys.getCharKey("CameraModeRight")){
				gameCamera.setMoveX(0);
				camRightButtonDown= false;
				calcCameraMoveX();
			}
		}
	}
	catch(const exception &ex) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());

		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,szBuf);

		NetworkManager &networkManager= NetworkManager::getInstance();
		if(networkManager.getGameNetworkInterface() != NULL) {
			GameNetworkInterface *networkInterface = NetworkManager::getInstance().getGameNetworkInterface();
			networkInterface->sendTextMessage(szBuf,-1,true,"");
			sleep(10);
			networkManager.getGameNetworkInterface()->quitGame(true);
		}
		ErrorDisplayMessage(ex.what(),true);
	}
}

void Game::calcCameraMoveX(){
	//move camera left
	if(camLeftButtonDown == true){
		gameCamera.setMoveX(-1);
	}
	//move camera right
	else if(camRightButtonDown == true){
		gameCamera.setMoveX(1);
	}
}
void Game::calcCameraMoveZ(){
	//move camera up
	if(camUpButtonDown == true){
		gameCamera.setMoveZ(1);
	}
	//move camera down
	else if(camDownButtonDown == true){
		gameCamera.setMoveZ(-1);
	}

}

void Game::keyPress(char c){
	if(gameStarted == false) {
		return;
	}

	chatManager.keyPress(c);
}

Stats Game::quitGame() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled == true) {
        world.DumpWorldToLog();
    }

	//Stats stats = *(world.getStats());
	Stats endStats;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	endStats = *(world.getStats());

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	NetworkManager::getInstance().end();
	//sleep(0);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//ProgramState *newState = new BattleEnd(program, endStats);

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//program->setState(newState);

	return endStats;
}

void Game::exitGameState(Program *program, Stats &endStats) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	ProgramState *newState = new BattleEnd(program, &endStats);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	program->setState(newState);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

// ==================== PRIVATE ====================

// ==================== render ====================

void Game::render3d(){
	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	Renderer &renderer= Renderer::getInstance();

	//init
	renderer.reset3d();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] renderFps = %d took msecs: %lld [reset3d]\n",__FILE__,__FUNCTION__,__LINE__,renderFps,chrono.getMillis());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

	renderer.computeVisibleQuad();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] renderFps = %d took msecs: %lld [computeVisibleQuad]\n",__FILE__,__FUNCTION__,__LINE__,renderFps,chrono.getMillis());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

	renderer.loadGameCameraMatrix();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] renderFps = %d took msecs: %lld [loadGameCameraMatrix]\n",__FILE__,__FUNCTION__,__LINE__,renderFps,chrono.getMillis());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

	renderer.setupLighting();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] renderFps = %d took msecs: %lld [setupLighting]\n",__FILE__,__FUNCTION__,__LINE__,renderFps,chrono.getMillis());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

	//shadow map
	renderer.renderShadowsToTexture(avgRenderFps);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] renderFps = %d took msecs: %lld [renderShadowsToTexture]\n",__FILE__,__FUNCTION__,__LINE__,renderFps,chrono.getMillis());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

	//clear buffers
	renderer.clearBuffers();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d renderFps = %d took msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,renderFps,chrono.getMillis());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

	//surface
	renderer.renderSurface(avgRenderFps);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] renderFps = %d took msecs: %lld [renderSurface]\n",__FILE__,__FUNCTION__,__LINE__,renderFps,chrono.getMillis());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

	//selection circles
	renderer.renderSelectionEffects();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] renderFps = %d took msecs: %lld [renderSelectionEffects]\n",__FILE__,__FUNCTION__,__LINE__,renderFps,chrono.getMillis());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

	//units
	renderer.renderUnits(avgRenderFps);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] renderFps = %d took msecs: %lld [renderUnits]\n",__FILE__,__FUNCTION__,__LINE__,renderFps,chrono.getMillis());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

	//objects
	renderer.renderObjects(avgRenderFps);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] renderFps = %d took msecs: %lld [renderObjects]\n",__FILE__,__FUNCTION__,__LINE__,renderFps,chrono.getMillis());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

	//water
	renderer.renderWater();
	renderer.renderWaterEffects();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] renderFps = %d took msecs: %lld [renderWater]\n",__FILE__,__FUNCTION__,__LINE__,renderFps,chrono.getMillis());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

	//particles
	renderer.renderParticleManager(rsGame);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] renderFps = %d took msecs: %lld [renderParticleManager]\n",__FILE__,__FUNCTION__,__LINE__,renderFps,chrono.getMillis());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) chrono.start();

	//mouse 3d
	renderer.renderMouse3d();
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] renderFps = %d took msecs: %lld [renderMouse3d]\n",__FILE__,__FUNCTION__,__LINE__,renderFps,chrono.getMillis());

	renderer.setLastRenderFps(lastRenderFps);
}

void Game::render2d(){
	Renderer &renderer= Renderer::getInstance();
	Config &config= Config::getInstance();
	CoreData &coreData= CoreData::getInstance();

	//init
	renderer.reset2d();

	//HUD
	if(visibleHUD == true) {
		renderer.renderHud();
	}
	//display
	renderer.renderDisplay();

	//minimap
	if(photoModeEnabled == false) {
        renderer.renderMinimap();
	}

    //selection
	renderer.renderSelectionQuad();

	//exit message box
	if(errorMessageBox.getEnabled()) {
		renderer.renderMessageBox(&errorMessageBox);
	}
	if(mainMessageBox.getEnabled()) {
		renderer.renderMessageBox(&mainMessageBox);
	}

	//script message box
	if( mainMessageBox.getEnabled() == false &&
		errorMessageBox.getEnabled() == false &&
		scriptManager.getMessageBoxEnabled()) {
		renderer.renderMessageBox(scriptManager.getMessageBox());
	}

	//script display text
	if(!scriptManager.getDisplayText().empty() && !scriptManager.getMessageBoxEnabled()){
		Vec4f fontColor = getGui()->getDisplay()->getColor();

		if(Renderer::renderText3DEnabled == true) {
			renderer.renderText3D(
				scriptManager.getDisplayText(), coreData.getMenuFontNormal3D(),
				Vec3f(fontColor.x,fontColor.y,fontColor.z), 200, 680, false);
		}
		else {
			renderer.renderText(
				scriptManager.getDisplayText(), coreData.getMenuFontNormal(),
				Vec3f(fontColor.x,fontColor.y,fontColor.z), 200, 680, false);
		}
	}

	if(program != NULL) program->renderProgramMsgBox();

	renderer.renderChatManager(&chatManager);

    //debug info
	bool perfLogging = false;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled == true ||
	   SystemFlags::getSystemSettingType(SystemFlags::debugWorldSynch).enabled == true) {
		perfLogging = true;
	}

    string str="";
    std::map<int,string> factionDebugInfo;

    world.getStats()->setWorldTimeElapsed(world.getTimeFlow()->getTime());

    if(difftime(time(NULL),lastMaxUnitCalcTime) >= 1) {
    	lastMaxUnitCalcTime = time(NULL);

		int totalUnitcount = 0;
		for(int i = 0; i < world.getFactionCount(); ++i) {
			Faction *faction= world.getFaction(i);
			totalUnitcount += faction->getUnitCount();
		}

		if(world.getStats()->getMaxConcurrentUnitCount() < totalUnitcount) {
			world.getStats()->setMaxConcurrentUnitCount(totalUnitcount);
		}
		world.getStats()->setTotalEndGameConcurrentUnitCount(totalUnitcount);
		world.getStats()->setFramesPlayed(world.getFrameCount());
    }

	if( renderer.getShowDebugUI() == true ||
		(perfLogging == true && difftime(time(NULL),lastRenderLog2d) >= 1)) {
		str+= "MouseXY: "        + intToStr(mouseX) + "," + intToStr(mouseY)+"\n";
		str+= "PosObjWord: "     + intToStr(gui.getPosObjWorld().x) + "," + intToStr(gui.getPosObjWorld().y)+"\n";
		str+= "Render FPS: "     + intToStr(lastRenderFps) + "[" + intToStr(avgRenderFps) + "]\n";
		str+= "Update FPS: "     + intToStr(lastUpdateFps) + "[" + intToStr(avgUpdateFps) + "]\n";
		str+= "GameCamera pos: " + floatToStr(gameCamera.getPos().x)+","+floatToStr(gameCamera.getPos().y)+","+floatToStr(gameCamera.getPos().z)+"\n";
		//str+= "Cached surfacedata: " +  intToStr(renderer.getCachedSurfaceDataSize())+"\n";
		str+= "Time: "           + floatToStr(world.getTimeFlow()->getTime(),2)+"\n";
		if(SystemFlags::getThreadedLoggerRunning() == true) {
            str+= "Log buffer count: " + intToStr(SystemFlags::getLogEntryBufferCount())+"\n";
		}

		//str+= "AttackWarningCount: " + intToStr(world.getUnitUpdater()->getAttackWarningCount()) + "\n";

		str+= "Map: " + gameSettings.getMap() +"\n";
		str+= "Tileset: " + gameSettings.getTileset() +"\n";
		str+= "Techtree: " + gameSettings.getTech() +"\n";

		str+= "Triangle count: " + intToStr(renderer.getTriangleCount())+"\n";
		str+= "Vertex count: "   + intToStr(renderer.getPointCount())+"\n";
		str+= "Frame count:"     + intToStr(world.getFrameCount())+"\n";

		//visible quad
		Quad2i visibleQuad= renderer.getVisibleQuad();

		str+= "Visible quad: ";
		for(int i= 0; i<4; ++i){
			str+= "(" + intToStr(visibleQuad.p[i].x) + "," +intToStr(visibleQuad.p[i].y) + ") ";
		}
		str+= "\n";
		str+= "Visible quad area: " + floatToStr(visibleQuad.area()) +"\n";

		int totalUnitcount = 0;
		for(int i = 0; i < world.getFactionCount(); ++i) {
			Faction *faction= world.getFaction(i);
			totalUnitcount += faction->getUnitCount();
		}

		VisibleQuadContainerCache &qCache =renderer.getQuadCache();
		int visibleUnitCount = qCache.visibleQuadUnitList.size();
		str+= "Visible unit count: " + intToStr(visibleUnitCount) + " total: " + intToStr(totalUnitcount) + "\n";

		int visibleObjectCount = qCache.visibleObjectList.size();
		str+= "Visible object count: " + intToStr(visibleObjectCount) +"\n";

		// resources
		for(int i = 0; i < world.getFactionCount(); ++i) {
			string factionInfo = this->gameSettings.getNetworkPlayerName(i);
			switch(this->gameSettings.getFactionControl(i)) {
				case ctCpuEasy:
					factionInfo += " CPU Easy";
					break;
				case ctCpu:
					factionInfo += " CPU Normal";
					break;
				case ctCpuUltra:
					factionInfo += " CPU Ultra";
					break;
				case ctCpuMega:
					factionInfo += " CPU Mega";
					break;
			}

			factionInfo +=	" [" + formatString(this->gameSettings.getFactionTypeName(i)) +
					" team: " + intToStr(this->gameSettings.getTeam(i)) + "] res: ";
			for(int j = 0; j < world.getTechTree()->getResourceTypeCount(); ++j) {
				factionInfo += intToStr(world.getFaction(i)->getResource(j)->getAmount());
				factionInfo += " ";
			}

			factionDebugInfo[i] = factionInfo;
		}

	}

	if(renderer.getShowDebugUI() == true) {
		const Metrics &metrics= Metrics::getInstance();
		int mx= metrics.getMinimapX();
		int my= metrics.getMinimapY();
		int mw= metrics.getMinimapW();
		int mh= metrics.getMinimapH();
		const Vec4f fontColor=getGui()->getDisplay()->getColor();

		if(Renderer::renderText3DEnabled == true) {
			renderer.renderTextShadow3D(str, coreData.getMenuFontNormal3D(),
					fontColor, 10, metrics.getVirtualH() - mh - 60, false);
		}
		else {
			renderer.renderTextShadow(str, coreData.getMenuFontNormal(),
					fontColor, 10, metrics.getVirtualH() - mh - 60, false);
		}

		for(int i = 0; i < world.getFactionCount(); ++i) {
			string factionInfo = factionDebugInfo[i];
			Vec3f playerColor = world.getFaction(i)->getTexture()->getPixmapConst()->getPixel3f(0, 0);

			if(Renderer::renderText3DEnabled == true) {
				renderer.renderText3D(factionInfo, coreData.getMenuFontBig3D(),
						Vec4f(playerColor.x,playerColor.y,playerColor.z,1.0),
						10, metrics.getVirtualH() - mh - 90 - 280 - (i * 16), false);
			}
			else {
				renderer.renderText(factionInfo, coreData.getMenuFontBig(),
						Vec4f(playerColor.x,playerColor.y,playerColor.z,1.0),
						10, metrics.getVirtualH() - mh - 90 - 280 - (i * 16), false);
			}
		}

		if((renderer.getShowDebugUILevel() & debugui_unit_titles) == debugui_unit_titles) {
			if(renderer.getAllowRenderUnitTitles() == false) {
				renderer.setAllowRenderUnitTitles(true);
			}
			renderer.renderUnitTitles(coreData.getMenuFontNormal(),Vec3f(1.0f));
		}
	}

	//network status
	if(renderNetworkStatus == true) {
		if(NetworkManager::getInstance().getGameNetworkInterface() != NULL) {
			const Metrics &metrics= Metrics::getInstance();
			int mx= metrics.getMinimapX();
			int my= metrics.getMinimapY();
			int mw= metrics.getMinimapW();
			int mh= metrics.getMinimapH();
			const Vec4f fontColor=getGui()->getDisplay()->getColor();

			if(Renderer::renderText3DEnabled == true) {
				renderer.renderTextShadow3D(
								NetworkManager::getInstance().getGameNetworkInterface()->getNetworkStatus(),
								coreData.getMenuFontNormal3D(),
								fontColor, mx + mw + 5 , metrics.getVirtualH()-30-20, false);
			}
			else {
				renderer.renderTextShadow(
					NetworkManager::getInstance().getGameNetworkInterface()->getNetworkStatus(),
					coreData.getMenuFontNormal(),
					fontColor, mx + mw + 5 , metrics.getVirtualH()-30-20, false);
			}
		}
	}

    //resource info
	if(photoModeEnabled == false) {
        renderer.renderResourceStatus();
		renderer.renderConsole(&console,showFullConsole);
    }

    //2d mouse
	renderer.renderMouse2d(mouseX, mouseY, mouse2d, gui.isSelectingPos()? 1.f: 0.f);

	if(perfLogging == true && difftime(time(NULL),lastRenderLog2d) >= 1) {
		lastRenderLog2d = time(NULL);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] Statistics: %s\n",__FILE__,__FUNCTION__,__LINE__,str.c_str());
	}
}


// ==================== misc ====================

void Game::checkWinner() {
	if(gameOver == false) {
		if(gameSettings.getDefaultVictoryConditions()) {
			checkWinnerStandard();
		}
		else {
			checkWinnerScripted();
		}
	}
}

void Game::checkWinnerStandard() {
	if(world.getThisFaction()->getType()->getPersonalityType() == fpt_Observer) {
		// lookup int is team #, value is players alive on team
		std::map<int,int> teamsAlive;
		for(int i = 0; i < world.getFactionCount(); ++i) {
			if(i != world.getThisFactionIndex()) {
				if(hasBuilding(world.getFaction(i))) {
					teamsAlive[world.getFaction(i)->getTeam()] = teamsAlive[world.getFaction(i)->getTeam()] + 1;
				}
			}
		}

		// did some team win
		if(teamsAlive.size() <= 1) {
			for(int i=0; i< world.getFactionCount(); ++i) {
				if(i != world.getThisFactionIndex() && teamsAlive.find(world.getFaction(i)->getTeam()) != teamsAlive.end()) {
					world.getStats()->setVictorious(i);
				}
			}
			gameOver= true;
			if( this->gameSettings.isNetworkGame() == false ||
				this->gameSettings.getEnableObserverModeAtEndGame() == true) {
				// Let the happy winner view everything left in the world

				// This caused too much LAG for network games
				if(this->gameSettings.isNetworkGame() == false) {
					Renderer::getInstance().setPhotoMode(true);
					gameCamera.setMaxHeight(PHOTO_MODE_MAXHEIGHT);
				}
				// END
			}

			scriptManager.onGameOver(true);
			showWinMessageBox();
		}
	}
	else {
		//lose
		bool lose= false;
		if(hasBuilding(world.getThisFaction()) == false) {
			lose= true;
			for(int i=0; i<world.getFactionCount(); ++i) {
				if(world.getFaction(i)->getType()->getPersonalityType() != fpt_Observer) {
					if(world.getFaction(i)->isAlly(world.getThisFaction()) == false) {
						world.getStats()->setVictorious(i);
					}
				}
			}
			gameOver= true;
			if( this->gameSettings.isNetworkGame() == false ||
				this->gameSettings.getEnableObserverModeAtEndGame() == true) {
				// Let the poor user watch everything unfold

				// This caused too much LAG for network games
				if(this->gameSettings.isNetworkGame() == false) {
					Renderer::getInstance().setPhotoMode(true);
					gameCamera.setMaxHeight(PHOTO_MODE_MAXHEIGHT);
				}
				// END

				// but don't let him cheat via teamchat
				chatManager.setDisableTeamMode(true);
			}

			scriptManager.onGameOver(!lose);

			showLoseMessageBox();
		}

		//win
		if(lose == false) {
			bool win= true;
			for(int i = 0; i < world.getFactionCount(); ++i) {
				if(i != world.getThisFactionIndex()) {
					if(world.getFaction(i)->getType()->getPersonalityType() != fpt_Observer) {
						if(hasBuilding(world.getFaction(i)) && world.getFaction(i)->isAlly(world.getThisFaction()) == false) {
							win= false;
						}
					}
				}
			}

			//if win
			if(win) {
				for(int i=0; i< world.getFactionCount(); ++i) {
					if(world.getFaction(i)->getType()->getPersonalityType() != fpt_Observer) {
						if(world.getFaction(i)->isAlly(world.getThisFaction())) {
							world.getStats()->setVictorious(i);
						}
					}
				}
				gameOver= true;
				if( this->gameSettings.isNetworkGame() == false ||
					this->gameSettings.getEnableObserverModeAtEndGame() == true) {
					// Let the happy winner view everything left in the world
					//world.setFogOfWar(false);

					// This caused too much LAG for network games
					if(this->gameSettings.isNetworkGame() == false) {
						Renderer::getInstance().setPhotoMode(true);
						gameCamera.setMaxHeight(PHOTO_MODE_MAXHEIGHT);
					}
					// END
				}

				scriptManager.onGameOver(win);

				showWinMessageBox();
			}
		}
	}
}

void Game::checkWinnerScripted() {
	if(scriptManager.getGameOver()) {
		gameOver= true;
		for(int i= 0; i<world.getFactionCount(); ++i) {
			if(scriptManager.getPlayerModifiers(i)->getWinner()) {
				world.getStats()->setVictorious(i);
			}
		}

		if( this->gameSettings.isNetworkGame() == false ||
			this->gameSettings.getEnableObserverModeAtEndGame() == true) {
			// Let the happy winner view everything left in the world
			//world.setFogOfWar(false);

			// This caused too much LAG for network games
			if(this->gameSettings.isNetworkGame() == false) {
				Renderer::getInstance().setPhotoMode(true);
				gameCamera.setMaxHeight(PHOTO_MODE_MAXHEIGHT);
			}
			// END
		}

		scriptManager.onGameOver(scriptManager.getPlayerModifiers(world.getThisFactionIndex())->getWinner());

		if(scriptManager.getPlayerModifiers(world.getThisFactionIndex())->getWinner()){
			showWinMessageBox();
		}
		else{
			showLoseMessageBox();
		}
	}
}

bool Game::hasBuilding(const Faction *faction) {
	for(int i=0; i<faction->getUnitCount(); ++i) {
		if(faction->getUnit(i)->getType()->hasSkillClass(scBeBuilt)) {
			return true;
		}
	}
	return false;
}

void Game::incSpeed() {
	Lang &lang= Lang::getInstance();
	switch(speed){
	case sSlow:
		speed= sNormal;
		console.addLine(lang.get("GameSpeedSet")+" "+lang.get("Normal"));
		break;
	case sNormal:
		speed= sFast;
		console.addLine(lang.get("GameSpeedSet")+" "+lang.get("Fast"));
		break;
	default:
		break;
	}
}

void Game::decSpeed() {
	Lang &lang= Lang::getInstance();
	switch(speed){
	case sNormal:
		speed= sSlow;
		console.addLine(lang.get("GameSpeedSet")+" "+lang.get("Slow"));
		break;
	case sFast:
		speed= sNormal;
		console.addLine(lang.get("GameSpeedSet")+" "+lang.get("Normal"));
		break;
	default:
		break;
	}
}

void Game::setPaused(bool value) {
	bool speedChangesAllowed= !NetworkManager::getInstance().isNetworkGame();
	//printf("Toggle pause value = %d, speedChangesAllowed = %d\n",value,speedChangesAllowed);

	if(speedChangesAllowed) {
		Lang &lang= Lang::getInstance();
		if(value == false) {
			console.addLine(lang.get("GameResumed"));
			paused= false;
		}
		else {
			console.addLine(lang.get("GamePaused"));
			paused= true;
		}
	}
}

int Game::getUpdateLoops() {
	if(paused) {
		return 0;
	}
	else if(speed == sFast) {
		return Config::getInstance().getInt("FastSpeedLoops");
	}
	else if(speed == sSlow) {
		return updateFps % 2 == 0? 1: 0;
	}
	return 1;
}

void Game::showLoseMessageBox() {
	Lang &lang= Lang::getInstance();

	NetworkManager &networkManager= NetworkManager::getInstance();
	if(networkManager.isNetworkGame() == true && networkManager.getNetworkRole() == nrServer) {
		showMessageBox(lang.get("YouLose")+", "+lang.get("ExitGameServer?"), lang.get("BattleOver"), false);
	}
	else {
		showMessageBox(lang.get("YouLose")+", "+lang.get("ExitGame?"), lang.get("BattleOver"), false);
	}
}

void Game::showWinMessageBox() {
	Lang &lang= Lang::getInstance();

	if(world.getThisFaction()->getType()->getPersonalityType() == fpt_Observer) {
		showMessageBox(lang.get("GameOver")+", "+lang.get("ExitGame?"), lang.get("BattleOver"), false);
	}
	else {
		showMessageBox(lang.get("YouWin")+", "+lang.get("ExitGame?"), lang.get("BattleOver"), false);
	}
}

void Game::showMessageBox(const string &text, const string &header, bool toggle) {
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

void Game::showErrorMessageBox(const string &text, const string &header, bool toggle) {
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

void Game::startPerformanceTimer() {
	captureAvgTestStatus = true;
	updateFpsAvgTest = -1;
	renderFpsAvgTest = -1;
}

void Game::endPerformanceTimer() {
	captureAvgTestStatus = false;
}

Vec2i Game::getPerformanceTimerResults() {
	Vec2i results(this->updateFpsAvgTest,this->renderFpsAvgTest);
	return results;
}

void Game::consoleAddLine(string line) {
	console.addLine(line);
}

}}//end namespace
