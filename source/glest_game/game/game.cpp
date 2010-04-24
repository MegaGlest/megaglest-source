// ==============================================================
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

// =====================================================
// 	class Game
// =====================================================

// ===================== PUBLIC ========================

Game::Game(Program *program, const GameSettings *gameSettings):
	ProgramState(program), lastMousePos(0)
{
	this->gameSettings= *gameSettings;
	scrollSpeed = Config::getInstance().getFloat("UiScrollSpeed","1.5");

	mouseX=0;
	mouseY=0;
	mouse2d= 0;
	loadingText="";
	weatherParticleSystem= NULL;
	updateFps=0;
	renderFps=0;
	lastUpdateFps=0;
	lastRenderFps=0;
	paused= false;
	gameOver= false;
	renderNetworkStatus= false;
	speed= sNormal;
	showFullConsole= false;
}

Game::~Game(){
    Logger &logger= Logger::getInstance();
	Renderer &renderer= Renderer::getInstance();
	
	logger.loadLoadingScreen("");
	logger.setState(Lang::getInstance().get("Deleting"));
	logger.add("Game", true);

	renderer.endGame();
	SoundRenderer::getInstance().stopAllSounds();

	deleteValues(aiInterfaces.begin(), aiInterfaces.end());

	gui.end();		//selection must be cleared before deleting units
	world.end();	//must die before selection because of referencers
}


// ==================== init and load ====================

void Game::load(){
	Logger &logger= Logger::getInstance();
	string mapName= gameSettings.getMap();
	string tilesetName= gameSettings.getTileset();
	string techName= gameSettings.getTech();
	string scenarioName= gameSettings.getScenario();
	bool loadingImageUsed=false;
	
	logger.setState(Lang::getInstance().get("Loading"));

	if(scenarioName.empty()){
		logger.setSubtitle(formatString(mapName)+" - "+formatString(tilesetName)+" - "+formatString(techName));
	}
	else{
		logger.setSubtitle(formatString(scenarioName));
	}

    Config &config = Config::getInstance();
    good_fpu_control_registers(NULL,__FILE__,__FUNCTION__,__LINE__);

    string scenarioDir = "";
    if(gameSettings.getScenarioDir() != "") {
        scenarioDir = gameSettings.getScenarioDir();
        if(EndsWith(scenarioDir, ".xml") == true) {
            scenarioDir = scenarioDir.erase(scenarioDir.size() - 4, 4);
            scenarioDir = scenarioDir.erase(scenarioDir.size() - gameSettings.getScenario().size(), gameSettings.getScenario().size() + 1);
        }
        // use a scenario based loading screen
        vector<string> loadScreenList;
        findAll(scenarioDir + "loading_screen.*", loadScreenList, false, false);
        if(loadScreenList.size() > 0) {
			//string senarioLogo = scenarioDir + "/" + "loading_screen.jpg";
        	string senarioLogo = scenarioDir + loadScreenList[0];
			if(fileExists(senarioLogo) == true) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] found scenario loading screen '%s'\n",__FILE__,__FUNCTION__,senarioLogo.c_str());

				logger.loadLoadingScreen(senarioLogo);
				loadingImageUsed=true;
			}
        }
        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] gameSettings.getScenarioDir() = [%s] gameSettings.getScenario() = [%s] scenarioDir = [%s]\n",__FILE__,__FUNCTION__,__LINE__,gameSettings.getScenarioDir().c_str(),gameSettings.getScenario().c_str(),scenarioDir.c_str());
    }
	
	if(loadingImageUsed == false){
		// try to use a faction related loading screen
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Searching for faction loading screen\n",__FILE__,__FUNCTION__);
		for ( int i=0; i < gameSettings.getFactionCount(); ++i ) {
			if( gameSettings.getFactionControl(i) == ctHuman || 
				(gameSettings.getFactionControl(i) == ctNetwork && gameSettings.getThisFactionIndex() == i)){
				vector<string> pathList=config.getPathListForType(ptTechs,scenarioDir);	
				for(int idx = 0; idx < pathList.size(); idx++) {
					const string path = pathList[idx]+ "/" +techName+ "/"+ "factions"+ "/"+ gameSettings.getFactionTypeName(i);
		        	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] possible loading screen dir '%s'\n",__FILE__,__FUNCTION__,path.c_str());
		        	if(isdir(path.c_str()) == true) {
				        vector<string> loadScreenList;
				        findAll(path + "/" + "loading_screen.*", loadScreenList, false, false);
				        if(loadScreenList.size() > 0) {
							//string factionLogo = path + "/" + "loading_screen.jpg";
				        	string factionLogo = path + "/" + loadScreenList[0];

							SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] looking for loading screen '%s'\n",__FILE__,__FUNCTION__,factionLogo.c_str());

							if(fileExists(factionLogo) == true) {
								SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] found loading screen '%s'\n",__FILE__,__FUNCTION__,factionLogo.c_str());

								logger.loadLoadingScreen(factionLogo);
								loadingImageUsed = true;
								break;
							}
				        }
		        	}

		        	if(loadingImageUsed == true) {
		        		break;
		        	}
		        }
		        break;
			}
	    }
	}
	if(loadingImageUsed == false){
		// try to use a tech related loading screen
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Searching for tech loading screen\n",__FILE__,__FUNCTION__);
		
		vector<string> pathList=config.getPathListForType(ptTechs,scenarioDir);	
		for(int idx = 0; idx < pathList.size(); idx++) {
			const string path = pathList[idx]+ "/" +techName;
        	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] possible loading screen dir '%s'\n",__FILE__,__FUNCTION__,path.c_str());
        	if(isdir(path.c_str()) == true) {
		        vector<string> loadScreenList;
		        findAll(path + "/" + "loading_screen.*", loadScreenList, false, false);
		        if(loadScreenList.size() > 0) {
					//string factionLogo = path + "/" + "loading_screen.jpg";
		        	string factionLogo = path + "/" + loadScreenList[0];

					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] looking for loading screen '%s'\n",__FILE__,__FUNCTION__,factionLogo.c_str());

					if(fileExists(factionLogo) == true) {
						SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] found loading screen '%s'\n",__FILE__,__FUNCTION__,factionLogo.c_str());

						logger.loadLoadingScreen(factionLogo);
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
	
	//throw runtime_error("Test!");

	//tileset
    world.loadTileset(config.getPathListForType(ptTilesets,scenarioDir), tilesetName, &checksum);

	set<string> factions;
	for ( int i=0; i < gameSettings.getFactionCount(); ++i ) {
		factions.insert(gameSettings.getFactionTypeName(i));
	}

    //tech, load before map because of resources
    world.loadTech(config.getPathListForType(ptTechs,scenarioDir), techName, factions, &checksum);
	
    //map
	world.loadMap(Map::getMapPath(mapName,scenarioDir), &checksum);

    //scenario
	if(!scenarioName.empty()){
		Lang::getInstance().loadScenarioStrings(gameSettings.getScenarioDir(), scenarioName);
		world.loadScenario(gameSettings.getScenarioDir(), &checksum);
	}

    good_fpu_control_registers(NULL,__FILE__,__FUNCTION__,__LINE__);
}

void Game::init()
{
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

	Lang &lang= Lang::getInstance();
	Logger &logger= Logger::getInstance();
	CoreData &coreData= CoreData::getInstance();
	Renderer &renderer= Renderer::getInstance();
	Map *map= world.getMap();
	NetworkManager &networkManager= NetworkManager::getInstance();

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Initializing\n",__FILE__,__FUNCTION__);
	logger.setState(lang.get("Initializing"));

	//mesage box
	mainMessageBox.init(lang.get("Yes"), lang.get("No"));
	mainMessageBox.setEnabled(false);

	//init world, and place camera
	commander.init(&world);
	world.init(this, gameSettings.getDefaultUnits());
	gui.init(this);
	chatManager.init(&console, world.getThisTeamIndex());
	console.clearStoredLines();
	const Vec2i &v= map->getStartLocation(world.getThisFaction()->getStartLocationIndex());
	gameCamera.init(map->getW(), map->getH());
	gameCamera.setPos(Vec2f(v.x, v.y));
	scriptManager.init(&world, &gameCamera);

    good_fpu_control_registers(NULL,__FILE__,__FUNCTION__,__LINE__);

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] creating AI's\n",__FILE__,__FUNCTION__);

	//create IAs
	aiInterfaces.resize(world.getFactionCount());
	for(int i=0; i<world.getFactionCount(); ++i){
		Faction *faction= world.getFaction(i);
		if(faction->getCpuControl()){
			aiInterfaces[i]= new AiInterface(*this, i, faction->getTeam());
			logger.add("Creating AI for faction " + intToStr(i), true);
		}
		else{
			aiInterfaces[i]= NULL;
		}
	}

	//wheather particle systems
	if(world.getTileset()->getWeather() == wRainy){
		logger.add("Creating rain particle system", true);
		weatherParticleSystem= new RainParticleSystem();
		weatherParticleSystem->setSpeed(12.f/GameConstants::updateFps);
		weatherParticleSystem->setPos(gameCamera.getPos());
		renderer.manageParticleSystem(weatherParticleSystem, rsGame);
	}
	else if(world.getTileset()->getWeather() == wSnowy){
		logger.add("Creating snow particle system", true);
		weatherParticleSystem= new SnowParticleSystem(1200);
		weatherParticleSystem->setSpeed(1.5f/GameConstants::updateFps);
		weatherParticleSystem->setPos(gameCamera.getPos());
		weatherParticleSystem->setTexture(coreData.getSnowTexture());
		renderer.manageParticleSystem(weatherParticleSystem, rsGame);
	}

	//init renderer state
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Initializing renderer\n",__FILE__,__FUNCTION__);
	logger.add("Initializing renderer", true);
	renderer.initGame(this);

    good_fpu_control_registers(NULL,__FILE__,__FUNCTION__,__LINE__);

	//sounds
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	Tileset *tileset= world.getTileset();
	AmbientSounds *ambientSounds= tileset->getAmbientSounds();

	//rain
	if(tileset->getWeather()==wRainy && ambientSounds->isEnabledRain()){
		logger.add("Starting ambient stream", true);
		soundRenderer.playAmbient(ambientSounds->getRain());
	}

	//snow
	if(tileset->getWeather()==wSnowy && ambientSounds->isEnabledSnow()){
		logger.add("Starting ambient stream", true);
		soundRenderer.playAmbient(ambientSounds->getSnow());
	}

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Waiting for network\n",__FILE__,__FUNCTION__);
	logger.add("Waiting for network", true);
	networkManager.getGameNetworkInterface()->waitUntilReady(&checksum);

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Starting music stream\n",__FILE__,__FUNCTION__);
	logger.add("Starting music stream", true);
	StrSound *gameMusic= world.getThisFaction()->getType()->getMusic();
	soundRenderer.playMusic(gameMusic);

	logger.add("Launching game");

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}


// ==================== update ====================

//update
void Game::update(){

	// a) Updates non dependant on speed

	//misc
	updateFps++;
	mouse2d= (mouse2d+1) % Renderer::maxMouse2dAnim;

	//console
	console.update();

	// b) Updates depandant on speed

	int updateLoops= getUpdateLoops();

	//update
	for(int i=0; i<updateLoops; ++i){
		Renderer &renderer= Renderer::getInstance();

		//AiInterface
		for(int i=0; i<world.getFactionCount(); ++i){
			if(world.getFaction(i)->getCpuControl() && scriptManager.getPlayerModifiers(i)->getAiEnabled()){
				aiInterfaces[i]->update();
			}
		}

		//World
		world.update();

		// Commander
		commander.updateNetwork();

		//Gui
		gui.update();

		//Particle systems
		if(weatherParticleSystem != NULL){
			weatherParticleSystem->setPos(gameCamera.getPos());
		}
		renderer.updateParticleManager(rsGame);

	    good_fpu_control_registers(NULL,__FILE__,__FUNCTION__,__LINE__);
	}

	//call the chat manager
	chatManager.updateNetwork();

	//check for quiting status
	if(NetworkManager::getInstance().getGameNetworkInterface()->getQuit()){
		quitGame();
	}

	//update auto test
	if(Config::getInstance().getBool("AutoTest")){
		AutoTest::getInstance().updateGame(this);
	}
}

void Game::updateCamera(){
	gameCamera.update();
}


// ==================== render ====================

//render
void Game::render(){
	renderFps++;
	render3d();
	render2d();
	Renderer::getInstance().swapBuffers();
}

// ==================== tick ====================

void Game::tick(){
	lastUpdateFps= updateFps;
	lastRenderFps= renderFps;
	updateFps= 0;
	renderFps= 0;

	//Win/lose check
	checkWinner();
	gui.tick();
}


// ==================== events ====================

void Game::mouseDownLeft(int x, int y){

	Map *map= world.getMap();
	const Metrics &metrics= Metrics::getInstance();
	NetworkManager &networkManager= NetworkManager::getInstance();
	bool messageBoxClick= false;

	//scrip message box, only if the exit box is not enabled
	if(!mainMessageBox.getEnabled() && scriptManager.getMessageBox()->getEnabled()){
		int button= 1;
		if(scriptManager.getMessageBox()->mouseClick(x, y, button)){
			scriptManager.onMessageBoxOk();
			messageBoxClick= true;
		}
	}

	//minimap panel
	if(!messageBoxClick){
		if(metrics.isInMinimap(x, y) && !gui.isSelectingPos()){

			int xm= x - metrics.getMinimapX();
			int ym= y - metrics.getMinimapY();
			int xCell= static_cast<int>(xm * (static_cast<float>(map->getW()) / metrics.getMinimapW()));
			int yCell= static_cast<int>(map->getH() - ym * (static_cast<float>(map->getH()) / metrics.getMinimapH()));

			if(map->isInside(xCell, yCell)){
				if(!gui.isSelectingPos()){
					gameCamera.setPos(Vec2f(static_cast<float>(xCell), static_cast<float>(yCell)));
				}
			}
		}

		//display panel
		else if(metrics.isInDisplay(x, y) && !gui.isSelectingPos()){
			int xd= x - metrics.getDisplayX();
			int yd= y - metrics.getDisplayY();
			if(gui.mouseValid(xd, yd)){
				gui.mouseDownLeftDisplay(xd, yd);
			}
			else{
				gui.mouseDownLeftGraphics(x, y);
			}
		}

		//graphics panel
		else{
			gui.mouseDownLeftGraphics(x, y);
		}
	}

	//exit message box, has to be the last thing to do in this function
	if(mainMessageBox.getEnabled()){
		int button= 1;
		if(mainMessageBox.mouseClick(x, y, button))
		{
			if(button==1)
			{
				networkManager.getGameNetworkInterface()->quitGame(true);
				quitGame();
			}
			else
			{
				//close message box
				mainMessageBox.setEnabled(false);
			}
		}
	}
}

void Game::mouseDownRight(int x, int y){
	gui.mouseDownRightGraphics(x, y);
}

void Game::mouseUpLeft(int x, int y){
	gui.mouseUpLeftGraphics(x, y);
}

void Game::mouseDoubleClickLeft(int x, int y){
	const Metrics &metrics= Metrics::getInstance();

	//display panel
	if(metrics.isInDisplay(x, y) && !gui.isSelectingPos()){
		int xd= x - metrics.getDisplayX();
		int yd= y - metrics.getDisplayY();
		if(gui.mouseValid(xd, yd)){
			return;
		}
	}

	//graphics panel
	gui.mouseDoubleClickLeftGraphics(x, y);
}

void Game::mouseMove(int x, int y, const MouseState *ms){
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
        {
			//float ymult = Config::getInstance().getCameraInvertYAxis() ? -0.2f : 0.2f;
			//float xmult = Config::getInstance().getCameraInvertXAxis() ? -0.2f : 0.2f;
			float ymult = 0.2f;
			float xmult = 0.2f;

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			gameCamera.transitionVH(-(y - lastMousePos.y) * ymult, (lastMousePos.x - x) * xmult);
			mouseX=lastMousePos.x;
			mouseY=lastMousePos.y;
			Window::revertMousePos();

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			return;
		}
	}
	else {
		//main window
		if(Window::isKeyDown() == false) {
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
	if (metrics.isInDisplay(x, y) && !gui.isSelecting() && !gui.isSelectingPos()) {
		if (!gui.isSelectingPos()) {
			gui.mouseMoveDisplay(x - metrics.getDisplayX(), y - metrics.getDisplayY());
		}
	}

	lastMousePos.x = mouseX;
	lastMousePos.y = mouseY;
}

void Game::eventMouseWheel(int x, int y, int zDelta) {
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	//gameCamera.transitionXYZ(0.0f, -(float)zDelta / 30.0f, 0.0f);
	gameCamera.zoom((float)zDelta / 60.0f);
	//gameCamera.setMoveY(1);
}

void Game::keyDown(char key){

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = %d\n",__FILE__,__FUNCTION__,__LINE__,key);

	Lang &lang= Lang::getInstance();
	bool speedChangesAllowed= !NetworkManager::getInstance().isNetworkGame();

	//send key to the chat manager
	chatManager.keyDown(key);

	if(!chatManager.getEditEnabled()){
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = %d\n",__FILE__,__FUNCTION__,__LINE__,key);

		if(key=='N'){
			renderNetworkStatus= true;
		}
		else if(key=='M'){
			showFullConsole= true;
		}
		else if(key=='E'){
			for(int i=0; i<100; ++i){
				string path= "screens/screen" + intToStr(i) + ".tga";

				FILE *f= fopen(path.c_str(), "rb");
				if(f==NULL){
					Renderer::getInstance().saveScreen(path);
					break;
				}
				else{
					fclose(f);
				}
			}
		}

		//move camera left
		else if(key==vkLeft){
			gameCamera.setMoveX(-1);
		}

		//move camera right
		else if(key==vkRight){
			gameCamera.setMoveX(1);
		}

		//move camera up
		else if(key==vkUp){
			gameCamera.setMoveZ(1);
		}

		//move camera down
		else if(key==vkDown){
			gameCamera.setMoveZ(-1);
		}

		//change camera mode
		else if(key=='F'){
			gameCamera.switchState();
			string stateString= gameCamera.getState()==GameCamera::sGame? lang.get("GameCamera"): lang.get("FreeCamera");
			console.addLine(lang.get("CameraModeSet")+" "+ stateString);
		}
		
		//reset camera mode to normal
		else if(key==' '){
			gameCamera.resetPosition();
		}
		//pause
		else if(key=='P'){
			if(speedChangesAllowed){
				if(paused){
					console.addLine(lang.get("GameResumed"));
					paused= false;
				}
				else{
					console.addLine(lang.get("GamePaused"));
					paused= true;
				}
			}
		}
		//switch display color
		else if(key=='C'){
			gui.switchToNextDisplayColor();
		}
		
		//increment speed
		else if(key==vkAdd){
			if(speedChangesAllowed){
				incSpeed();
			}
		}

		//decrement speed
		else if(key==vkSubtract){
			if(speedChangesAllowed){
				decSpeed();
			}
		}

		//exit
		else if(key==vkEscape){
			showMessageBox(lang.get("ExitGame?"), "", true);
		}

		//group
		else if(key>='0' && key<'0'+Selection::maxGroups){
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = %d\n",__FILE__,__FUNCTION__,__LINE__,key);

			gui.groupKey(key-'0');
		}

		//hotkeys
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] gameCamera.getState() = %d\n",__FILE__,__FUNCTION__,__LINE__,gameCamera.getState());

		if(gameCamera.getState() == GameCamera::sGame){
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = %d\n",__FILE__,__FUNCTION__,__LINE__,key);

			gui.hotKey(key);
		}
		else{
			//rotate camera leftt
			if(key=='A'){
				gameCamera.setRotate(-1);
			}

			//rotate camera right
			else if(key=='D'){
				gameCamera.setRotate(1);
			}

			//camera up
			else if(key=='S'){
				gameCamera.setMoveY(1);
			}

			//camera down
			else if(key=='W'){
				gameCamera.setMoveY(-1);
			}
		}
	}
}

void Game::keyUp(char key){

	if(chatManager.getEditEnabled()){
		//send key to the chat manager
		chatManager.keyUp(key);
	}
	else{
		switch(key){
		case 'N':
			renderNetworkStatus= false;
			break;
		case 'M':
			showFullConsole= false;
			break;
		case 'A':
		case 'D':
			gameCamera.setRotate(0);
			break;

		case 'W':
		case 'S':
			gameCamera.setMoveY(0);
			break;

		case vkUp:
		case vkDown:
			gameCamera.setMoveZ(0);
			break;

		case vkLeft:
		case vkRight:
			gameCamera.setMoveX(0);
			break;
		}
	}
}

void Game::keyPress(char c){
	chatManager.keyPress(c);
}

void Game::quitGame(){
	program->setState(new BattleEnd(program, world.getStats()));
}

// ==================== PRIVATE ====================

// ==================== render ====================

void Game::render3d(){

	Renderer &renderer= Renderer::getInstance();

	//init
	renderer.reset3d();
	renderer.computeVisibleQuad();
	renderer.loadGameCameraMatrix();
	renderer.setupLighting();

	//shadow map
	renderer.renderShadowsToTexture();

	//clear buffers
	renderer.clearBuffers();

	//surface
	renderer.renderSurface();

	//selection circles
	renderer.renderSelectionEffects();

	//units
	renderer.renderUnits();

	//objects
	renderer.renderObjects();

	//water
	renderer.renderWater();
	renderer.renderWaterEffects();

	//particles
	renderer.renderParticleManager(rsGame);

	//mouse 3d
	renderer.renderMouse3d();
}

void Game::render2d(){
	Renderer &renderer= Renderer::getInstance();
	Config &config= Config::getInstance();
	CoreData &coreData= CoreData::getInstance();

	//init
	renderer.reset2d();

	//display
	renderer.renderDisplay();

	//minimap
	if(!config.getBool("PhotoMode")){
        renderer.renderMinimap();
	}

    //selection
	renderer.renderSelectionQuad();

	//exit message box
	if(mainMessageBox.getEnabled()){
		renderer.renderMessageBox(&mainMessageBox);
	}

	//script message box
	if(!mainMessageBox.getEnabled() && scriptManager.getMessageBoxEnabled()){
		renderer.renderMessageBox(scriptManager.getMessageBox());
	}

	//script display text
	if(!scriptManager.getDisplayText().empty() && !scriptManager.getMessageBoxEnabled()){
		renderer.renderText(
			scriptManager.getDisplayText(), coreData.getMenuFontNormal(),
			Vec3f(1.0f), 200, 680, false);
	}


	renderer.renderChatManager(&chatManager);

    //debug info
	
    string str;
	if(config.getBool("DebugMode") || difftime(time(NULL),lastRenderLog2d) >= 1){
		str+= "MouseXY: " + intToStr(mouseX) + "," + intToStr(mouseY)+"\n";
		str+= "PosObjWord: " + intToStr(gui.getPosObjWorld().x) + "," + intToStr(gui.getPosObjWorld().y)+"\n";
		str+= "Render FPS: "+intToStr(lastRenderFps)+"\n";
		str+= "Update FPS: "+intToStr(lastUpdateFps)+"\n";
		str+= "GameCamera pos: "+floatToStr(gameCamera.getPos().x)+","+floatToStr(gameCamera.getPos().y)+","+floatToStr(gameCamera.getPos().z)+"\n";
		str+= "Time: "+floatToStr(world.getTimeFlow()->getTime())+"\n";
		str+= "Triangle count: "+intToStr(renderer.getTriangleCount())+"\n";
		str+= "Vertex count: "+intToStr(renderer.getPointCount())+"\n";
		str+= "Frame count:"+intToStr(world.getFrameCount())+"\n";

		//visible quad
		Quad2i visibleQuad= renderer.getVisibleQuad();

		str+= "Visible quad: ";
		for(int i= 0; i<4; ++i){
			str+= "(" + intToStr(visibleQuad.p[i].x) + "," +intToStr(visibleQuad.p[i].y) + ") ";
		}
		str+= "\n";
		str+= "Visible quad area: " + floatToStr(visibleQuad.area()) +"\n";

		// resources
		for(int i=0; i<world.getFactionCount(); ++i){
			str+= "Player "+intToStr(i)+" res: ";
			for(int j=0; j<world.getTechTree()->getResourceTypeCount(); ++j){
				str+= intToStr(world.getFaction(i)->getResource(j)->getAmount());
				str+=" ";
			}
			str+="\n";
		}
	}

	if(config.getBool("DebugMode")){
		renderer.renderText(
			str, coreData.getMenuFontNormal(),
			Vec3f(1.0f), 10, 500, false);
	}

	//network status
	if(renderNetworkStatus){
		renderer.renderText(
			NetworkManager::getInstance().getGameNetworkInterface()->getNetworkStatus(),
			coreData.getMenuFontNormal(),
			Vec3f(1.0f), 20, 500, false);
	}

    //resource info
	if(!config.getBool("PhotoMode")){
        renderer.renderResourceStatus();
		renderer.renderConsole(&console,showFullConsole);
    }

    //2d mouse
	renderer.renderMouse2d(mouseX, mouseY, mouse2d, gui.isSelectingPos()? 1.f: 0.f);

	if(difftime(time(NULL),lastRenderLog2d) >= 1) {
		lastRenderLog2d = time(NULL);
		SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d Statistics: %s\n",__FILE__,__FUNCTION__,__LINE__,str.c_str());
	}
}


// ==================== misc ====================

void Game::checkWinner(){
	if(!gameOver){
		if(gameSettings.getDefaultVictoryConditions()){
			checkWinnerStandard();
		}
		else
		{
			checkWinnerScripted();
		}
	}
}

void Game::checkWinnerStandard(){
	//lose
	bool lose= false;
	if(!hasBuilding(world.getThisFaction())){
		lose= true;
		for(int i=0; i<world.getFactionCount(); ++i){
			if(!world.getFaction(i)->isAlly(world.getThisFaction())){
				world.getStats()->setVictorious(i);
			}
		}
		gameOver= true;
		showLoseMessageBox();
	}

	//win
	if(!lose){
		bool win= true;
		for(int i=0; i<world.getFactionCount(); ++i){
			if(i!=world.getThisFactionIndex()){
				if(hasBuilding(world.getFaction(i)) && !world.getFaction(i)->isAlly(world.getThisFaction())){
					win= false;
				}
			}
		}

		//if win
		if(win){
			for(int i=0; i< world.getFactionCount(); ++i){
				if(world.getFaction(i)->isAlly(world.getThisFaction())){
					world.getStats()->setVictorious(i);
				}
			}
			gameOver= true;
			showWinMessageBox();
		}
	}
}

void Game::checkWinnerScripted(){
	if(scriptManager.getGameOver()){
		gameOver= true;
		for(int i= 0; i<world.getFactionCount(); ++i){
			if(scriptManager.getPlayerModifiers(i)->getWinner()){
				world.getStats()->setVictorious(i);
			}
		}
		if(scriptManager.getPlayerModifiers(world.getThisFactionIndex())->getWinner()){
			showWinMessageBox();
		}
		else{
			showLoseMessageBox();
		}
	}
}

bool Game::hasBuilding(const Faction *faction){
	for(int i=0; i<faction->getUnitCount(); ++i){
		if(faction->getUnit(i)->getType()->hasSkillClass(scBeBuilt)){
			return true;
		}
	}
	return false;
}

void Game::incSpeed(){
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

void Game::decSpeed(){
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

int Game::getUpdateLoops(){
	if(paused){
		return 0;
	}
	else if(speed==sFast){
		return Config::getInstance().getInt("FastSpeedLoops");
	}
	else if(speed==sSlow){
		return updateFps % 2 == 0? 1: 0;
	}
	return 1;
}

void Game::showLoseMessageBox(){
	Lang &lang= Lang::getInstance();
	showMessageBox(lang.get("YouLose")+", "+lang.get("ExitGame?"), lang.get("BattleOver"), false);
}

void Game::showWinMessageBox(){
	Lang &lang= Lang::getInstance();
	showMessageBox(lang.get("YouWin")+", "+lang.get("ExitGame?"), lang.get("BattleOver"), false);
}

void Game::showMessageBox(const string &text, const string &header, bool toggle){
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
