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

#ifndef _GLEST_GAME_GAME_H_
#define _GLEST_GAME_GAME_H_

#include <vector>

#include "gui.h"
#include "game_camera.h"
#include "world.h"
#include "ai_interface.h"
#include "program.h"
#include "chat_manager.h"
#include "script_manager.h"
#include "game_settings.h"
#include "network_interface.h"
#include "types.h"
#include "leak_dumper.h"

using std::vector;
using namespace Shared::Platform;
using namespace Shared::PlatformCommon;

namespace Glest{ namespace Game{

class GraphicMessageBox;

enum LoadGameItem {
	lgt_FactionPreview 	= 0x01,
	lgt_TileSet 		= 0x02,
	lgt_TechTree		= 0x04,
	lgt_Map				= 0x08,
	lgt_Scenario		= 0x10,

	lgt_All				= (lgt_FactionPreview | lgt_TileSet | lgt_TechTree | lgt_Map | lgt_Scenario)
};

// =====================================================
// 	class Game
//
//	Main game class
// =====================================================

//class Game: public ProgramState, public SimpleTaskCallbackInterface {
class Game: public ProgramState, public FileCRCPreCacheThreadCallbackInterface {
public:
	enum Speed{
		sFast,
		sNormal,
		sSlow
	};

private:
	typedef vector<Ai*> Ais;
	typedef vector<AiInterface*> AiInterfaces;

private:
	//main data
	World world;
    AiInterfaces aiInterfaces;
    Gui gui;
    GameCamera gameCamera;
    Commander commander;
    Console console;
	ChatManager chatManager;
	ScriptManager scriptManager;

	//misc
	Checksum checksum;
    string loadingText;
    int mouse2d;
    int mouseX, mouseY; //coords win32Api
	int updateFps, lastUpdateFps, avgUpdateFps;
	int totalRenderFps, renderFps, lastRenderFps, avgRenderFps,currentAvgRenderFpsTotal;
	Uint64 tickCount;
	bool paused;
	bool gameOver;
	bool renderNetworkStatus;
	bool showFullConsole;
	bool mouseMoved;
	float scrollSpeed;
	Speed speed;
	GraphicMessageBox mainMessageBox;
	GraphicMessageBox errorMessageBox;

	//misc ptr
	ParticleSystem *weatherParticleSystem;
	GameSettings gameSettings;
	Vec2i lastMousePos;
	time_t lastRenderLog2d;
	DisplayMessageFunction originalDisplayMsgCallback;
	bool isFirstRender;

	bool quitTriggeredIndicator;

	int original_updateFps;
	int original_cameraFps;

	bool captureAvgTestStatus;
	int updateFpsAvgTest;
	int renderFpsAvgTest;

	bool photoModeEnabled;
	Program *program;

	bool gameStarted;

	time_t lastMaxUnitCalcTime;

public:
	Game();
    Game(Program *program, const GameSettings *gameSettings);
    ~Game();

    //get
    GameSettings *getGameSettings() 	    {return &gameSettings;}
	const GameSettings *getReadOnlyGameSettings() const	{return &gameSettings;}

	const GameCamera *getGameCamera() const	{return &gameCamera;}
	GameCamera *getGameCamera()				{return &gameCamera;}
	const Commander *getCommander() const	{return &commander;}
	Gui *getGui()							{return &gui;}
	const Gui *getGui() const				{return &gui;}
	Commander *getCommander()				{return &commander;}
	Console *getConsole()					{return &console;}
	ScriptManager *getScriptManager()		{return &scriptManager;}
	World *getWorld()						{return &world;}
	const World *getWorld() const			{return &world;}

	bool getPaused() const					{ return paused;}
	void setPaused(bool value);
	const int getTotalRenderFps() const					{return totalRenderFps;}
    //init
    virtual void load(LoadGameItem loadTypes);
    virtual void load();
    virtual void init();
    virtual void init(bool initForPreviewOnly);
	virtual void update();
	virtual void updateCamera();
	virtual void render();
	virtual void tick();

    //event managing
    virtual void keyDown(char key);
    virtual void keyUp(char key);
    virtual void keyPress(char c);
    virtual void mouseDownLeft(int x, int y);
    virtual void mouseDownRight(int x, int y);
    virtual void mouseUpCenter(int x, int y);
    virtual void mouseUpLeft(int x, int y);
    virtual void mouseDoubleClickLeft(int x, int y);
    virtual void eventMouseWheel(int x, int y, int zDelta);
    virtual void mouseMove(int x, int y, const MouseState *mouseState);

	virtual bool isInSpecialKeyCaptureEvent() { return chatManager.getEditEnabled(); }

	virtual bool quitTriggered();
	virtual Stats quitAndToggleState();
	Stats quitGame();
	static void exitGameState(Program *program, Stats &endStats);

	void startPerformanceTimer();
	void endPerformanceTimer();
	Vec2i getPerformanceTimerResults();

	static Texture2D * findFactionLogoTexture(const GameSettings *settings, Logger *logger=NULL,string factionLogoFilter="loading_screen.*", bool useTechDefaultIfFilterNotFound=true);
	static string findFactionLogoFile(const GameSettings *settings, Logger *logger=NULL, string factionLogoFilter="loading_screen.*");
	static string extractScenarioLogoFile(const GameSettings *settings, string &result, bool &loadingImageUsed, Logger *logger=NULL, string factionLogoFilter="loading_screen.*");
	static string extractFactionLogoFile(bool &loadingImageUsed, string factionName, string scenarioDir, string techName, Logger *logger=NULL, string factionLogoFilter="loading_screen.*");
	static string extractTechLogoFile(string scenarioDir, string techName, bool &loadingImageUsed, Logger *logger=NULL,string factionLogoFilter="loading_screen.*");


	bool getGameOver() { return gameOver; }
	bool hasGameStarted() { return gameStarted;}
	virtual vector<Texture2D *> processTech(string techName);
	virtual void consoleAddLine(string line);

private:
	//render
    void render3d();
    void render2d();

	//misc
	void checkWinner();
	void checkWinnerStandard();
	void checkWinnerScripted();
	bool hasBuilding(const Faction *faction);
	void incSpeed();
	void decSpeed();
	int getUpdateLoops();

	void showLoseMessageBox();
	void showWinMessageBox();
	void showMessageBox(const string &text, const string &header, bool toggle);
	void showErrorMessageBox(const string &text, const string &header, bool toggle);

	void renderWorker();
	static int ErrorDisplayMessage(const char *msg, bool exitApp);

	void ReplaceDisconnectedNetworkPlayersWithAI(bool isNetworkGame, NetworkRole role);
};

}}//end namespace

#endif
