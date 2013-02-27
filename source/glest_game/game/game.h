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

#ifndef _GLEST_GAME_GAME_H_
#define _GLEST_GAME_GAME_H_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

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
#include "data_types.h"
#include "selection.h"
#include "leak_dumper.h"

using std::vector;
using namespace Shared::Platform;
using namespace Shared::PlatformCommon;

namespace Shared { namespace Graphics {
	class VideoPlayer;
}}

namespace Glest{ namespace Game{

class GraphicMessageBox;
class ServerInterface;

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
class Game: public ProgramState, public FileCRCPreCacheThreadCallbackInterface, public CustomInputCallbackInterface {
public:
	static const float highlightTime;

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
    int mouseX;
    int mouseY; //coords win32Api
    Vec2i mouseCellPos;

	int updateFps, lastUpdateFps, avgUpdateFps;
	int totalRenderFps, renderFps, lastRenderFps, avgRenderFps,currentAvgRenderFpsTotal;
	uint64 tickCount;
	bool paused;
	bool pauseRequestSent;
	bool resumeRequestSent;
	bool pauseStateChanged;
	bool gameOver;
	bool renderNetworkStatus;
	bool showFullConsole;
	bool setMarker;
	bool cameraDragAllowed;
	bool mouseMoved;
	float scrollSpeed;
	bool camLeftButtonDown;
	bool camRightButtonDown;
	bool camUpButtonDown;
	bool camDownButtonDown;

	int speed;
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
	bool quitPendingIndicator;

	int original_updateFps;
	int original_cameraFps;

	bool captureAvgTestStatus;
	int updateFpsAvgTest;
	int renderFpsAvgTest;

	int renderExtraTeamColor;
	static const int renderTeamColorCircleBit=1;
	static const int renderTeamColorPlaneBit=2;

	bool photoModeEnabled;
	bool visibleHUD;
	bool timeDisplay;
	bool withRainEffect;
	Program *program;

	bool gameStarted;

	time_t lastMaxUnitCalcTime;

	PopupMenu popupMenu;
	PopupMenu popupMenuSwitchTeams;
	PopupMenu popupMenuDisconnectPlayer;

	std::map<int,int> switchTeamIndexMap;
	GraphicMessageBox switchTeamConfirmMessageBox;

	std::map<int,int> disconnectPlayerIndexMap;
	int playerIndexDisconnect;
	GraphicMessageBox disconnectPlayerConfirmMessageBox;

	int exitGamePopupMenuIndex;
	int joinTeamPopupMenuIndex;
	int pauseGamePopupMenuIndex;
	int saveGamePopupMenuIndex;
	int loadGamePopupMenuIndex;
	//int markCellPopupMenuIndex;
	//int unmarkCellPopupMenuIndex;
	int keyboardSetupPopupMenuIndex;
	int disconnectPlayerPopupMenuIndex;
	//GLuint statelist3dMenu;
	ProgramState *currentUIState;

	bool isMarkCellEnabled;
	Vec2i cellMarkedPos;
	MarkedCell cellMarkedData;
	bool isMarkCellTextEnabled;

	Texture2D *markCellTexture;
	bool isUnMarkCellEnabled;
	Texture2D *unmarkCellTexture;
	std::map<Vec2i, MarkedCell> mapMarkedCellList;
	Texture2D *highlightCellTexture;
	std::vector<MarkedCell> highlightedCells;
	bool masterserverMode;

	StrSound *currentAmbientSound;

	time_t lastNetworkPlayerConnectionCheck;

	time_t lastMasterServerGameStatsDump;

	XmlNode *loadGameNode;
	int lastworldFrameCountForReplay;
	std::vector<std::pair<int,NetworkCommand> > replayCommandList;

	std::vector<string> streamingVideos;
	Shared::Graphics::VideoPlayer *videoPlayer;
	bool playingStaticVideo;

	Unit *currentCameraFollowUnit;

	std::map<int,HighlightSpecialUnitInfo> unitHighlightList;

	MasterSlaveThreadController masterController;

	bool inJoinGameLoading;

public:
	Game();
    Game(Program *program, const GameSettings *gameSettings, bool masterserverMode);
    ~Game();

    bool isMarkCellMode() const { return isMarkCellEnabled; }
    const Texture2D * getMarkCellTexture() const { return markCellTexture; }
    bool isUnMarkCellMode() const { return isUnMarkCellEnabled; }
    const Texture2D * getUnMarkCellTexture() const { return unmarkCellTexture; }

    std::map<Vec2i, MarkedCell> getMapMarkedCellList() const { return mapMarkedCellList; }

    const Texture2D * getHighlightCellTexture() const { return highlightCellTexture; }
    const std::vector<MarkedCell> * getHighlightedCells() const { return &highlightedCells; }
    void addOrReplaceInHighlightedCells(MarkedCell mc);

    bool isMasterserverMode() const { return masterserverMode; }
    //get
    GameSettings *getGameSettings() 	    		{return &gameSettings;}
    void setGameSettings(GameSettings *settings) 	{ gameSettings = *settings;}
	const GameSettings *getReadOnlyGameSettings() const	{return &gameSettings;}

	const GameCamera *getGameCamera() const	{return &gameCamera;}
	GameCamera *getGameCameraPtr()			{return &gameCamera;}
	const Commander *getCommander() const	{return &commander;}
	Gui *getGuiPtr()							{return &gui;}
	const Gui *getGui() const				{return &gui;}
	Commander *getCommander()				{return &commander;}
	Console *getConsole()					{return &console;}
	ScriptManager *getScriptManager()		{return &scriptManager;}
	World *getWorld()						{return &world;}
	const World *getWorld() const			{return &world;}

	Program *getProgram()					{return program;}

	void removeUnitFromSelection(const Unit *unit);
	bool addUnitToSelection(Unit *unit);
	void addUnitToGroupSelection(Unit *unit,int groupIndex);
	void removeUnitFromGroupSelection(int unitId,int groupIndex);
	void recallGroupSelection(int groupIndex);

	Uint64 getTickCount()					{return tickCount;}
	bool getPaused();
	void setPaused(bool value, bool forceAllowPauseStateChange=false,bool clearCaches=false);
	void tryPauseToggle(bool pause);
	void setupRenderForVideo();
	void saveGame();
	const int getTotalRenderFps() const					{return totalRenderFps;}

	void toggleTeamColorMarker();
    //init
	void resetMembers();
    virtual void load(int loadTypes);
    virtual void load();
    virtual void init();
    virtual void init(bool initForPreviewOnly);
	virtual void update();
	virtual void updateCamera();
	virtual void render();
	virtual void tick();

    //event managing
    virtual void keyDown(SDL_KeyboardEvent key);
    virtual void keyUp(SDL_KeyboardEvent key);
    virtual void keyPress(SDL_KeyboardEvent c);
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

	static Texture2D * findFactionLogoTexture(const GameSettings *settings, Logger *logger=NULL,string factionLogoFilter=GameConstants::LOADING_SCREEN_FILE_FILTER, bool useTechDefaultIfFilterNotFound=true);
	static string findFactionLogoFile(const GameSettings *settings, Logger *logger=NULL, string factionLogoFilter=GameConstants::LOADING_SCREEN_FILE_FILTER);
	static string extractScenarioLogoFile(const GameSettings *settings, string &result, bool &loadingImageUsed, Logger *logger=NULL, string factionLogoFilter=GameConstants::LOADING_SCREEN_FILE_FILTER);
	static string extractFactionLogoFile(bool &loadingImageUsed, string factionName, string scenarioDir, string techName, Logger *logger=NULL, string factionLogoFilter=GameConstants::LOADING_SCREEN_FILE_FILTER);
	static string extractTechLogoFile(string scenarioDir, string techName, bool &loadingImageUsed, Logger *logger=NULL,string factionLogoFilter=GameConstants::LOADING_SCREEN_FILE_FILTER);

	void loadHudTexture(const GameSettings *settings);

	bool getGameOver() { return gameOver; }
	bool hasGameStarted() { return gameStarted;}
	virtual vector<Texture2D *> processTech(string techName);
	virtual void consoleAddLine(string line);

	void endGame();

	void playStaticVideo(const string &playVideo);
	void playStreamingVideo(const string &playVideo);
	void stopStreamingVideo(const string &playVideo);
	void stopAllVideo();

	string saveGame(string name, string path="saved/");
	static void loadGame(string name,Program *programPtr,bool isMasterserverMode, const GameSettings *joinGameSettings=NULL);

	void addNetworkCommandToReplayList(NetworkCommand* networkCommand,int worldFrameCount);

	bool factionLostGame(int factionIndex);

	void addCellMarker(Vec2i cellPos, MarkedCell cellData);
	void removeCellMarker(Vec2i surfaceCellPos, const Faction *faction);
	void showMarker(Vec2i cellPos, MarkedCell cellData);

	void highlightUnit(int unitId,float radius, float thickness, Vec4f color);
	void unhighlightUnit(int unitId);

private:
	//render
    void render3d();
    void render2d();

	//misc
	void checkWinner();
	void checkWinnerStandard();
	void checkWinnerScripted();
	void setEndGameTeamWinnersAndLosers();

	bool hasBuilding(const Faction *faction);
	bool factionLostGame(const Faction *faction);
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
	void calcCameraMoveX();
	void calcCameraMoveZ();

	int getFirstUnusedTeamNumber();
	void updateWorldStats();

	void setupPopupMenus(bool checkClientAdminOverrideOnly);

	string getDebugStats(std::map<int,string> &factionDebugInfo);

	void renderVideoPlayer();

	void updateNetworkMarkedCells();
	void updateNetworkUnMarkedCells();
	void updateNetworkHighligtedCells();

	virtual void processInputText(string text, bool cancelled);

	void startMarkCell();
	void startCameraFollowUnit();

	bool switchSetupForSlots(ServerInterface *& serverInterface,
			int startIndex, int endIndex, bool onlyNetworkUnassigned);

};

}}//end namespace

#endif
