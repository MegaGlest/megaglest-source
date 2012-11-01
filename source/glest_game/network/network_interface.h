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

#ifndef _GLEST_GAME_NETWORKINTERFACE_H_
#define _GLEST_GAME_NETWORKINTERFACE_H_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

#include <string>
#include <vector>
#include "checksum.h"
#include "network_message.h"
#include "network_types.h"
#include "game_settings.h"
#include "thread.h"
#include "data_types.h"
#include <time.h>
#include "leak_dumper.h"

using std::string;
using std::vector;
using Shared::Util::Checksum;
using namespace Shared::Util;
using namespace Shared::Platform;

namespace Glest{ namespace Game{

//class GameSettings;

// =====================================================
//	class NetworkInterface
// =====================================================

class ChatMsgInfo {

protected:

	void copyAll(const ChatMsgInfo &obj) {
		this->chatText			= obj.chatText.c_str();
		this->chatTeamIndex		= obj.chatTeamIndex;
		this->chatPlayerIndex 	= obj.chatPlayerIndex;
		this->targetLanguage	= obj.targetLanguage;
	}
public:

	ChatMsgInfo() {
		this->chatText			= "";
		this->chatTeamIndex		= -1;
		this->chatPlayerIndex	= -1;
		this->targetLanguage	= "";
	}
	ChatMsgInfo(string chatText, int chatTeamIndex, int chatPlayerIndex,
			string targetLanguage) {
		this->chatText		= chatText;
		this->chatTeamIndex	= chatTeamIndex;
		this->chatPlayerIndex = chatPlayerIndex;
		this->targetLanguage = targetLanguage;
	}
	ChatMsgInfo(const ChatMsgInfo& obj) {
		copyAll(obj);
	}
	ChatMsgInfo & operator=(const ChatMsgInfo & obj) {
		copyAll(obj);
		return *this;
	}

	string chatText;
	int chatTeamIndex;
	int chatPlayerIndex;
	string targetLanguage;

};

class MarkedCell {
protected:
	Vec2i targetPos;
	const Faction *faction;
	int factionIndex;
	int playerIndex;
	string note;
	int aliveCount;

public:
	static Vec3f static_system_marker_color;

	MarkedCell() {
		faction = NULL;
		factionIndex = -1;
		playerIndex = -1;
		note = "";
		aliveCount=200;
	}
	MarkedCell(Vec2i targetPos,const Faction *faction,string note, int playerIndex) {
		this->targetPos = targetPos;
		this->faction = faction;
		this->factionIndex = -1;
		this->playerIndex = playerIndex;
		this->note = note;
		aliveCount=200;
	}
	MarkedCell(Vec2i targetPos,int factionIndex,string note, int playerIndex) {
		this->targetPos = targetPos;
		this->faction = NULL;
		this->factionIndex = factionIndex;
		this->playerIndex = playerIndex;
		this->note = note;
		aliveCount=200;
	}

	Vec2i getTargetPos() const { return targetPos; }
	const Faction * getFaction() const { return faction; }
	void setFaction(const Faction *faction) { this->faction = faction; }
	int getFactionIndex() const { return factionIndex; }
	string getNote() const { return note; }
	void decrementAliveCount() { this->aliveCount--; }
	int getAliveCount() const { return aliveCount; }
	void setAliveCount(int value) { this->aliveCount = value; }
	int getPlayerIndex() const { return playerIndex; }

	void setNote(string value) { note = value; }
	void setPlayerIndex(int value) { playerIndex = value; }
};

class UnMarkedCell {
protected:
	Vec2i targetPos;
	const Faction *faction;
	int factionIndex;

public:
	UnMarkedCell() {
		faction = NULL;
		factionIndex = -1;
	}
	UnMarkedCell(Vec2i targetPos,const Faction *faction) {
		this->targetPos = targetPos;
		this->faction = faction;
		this->factionIndex = -1;
	}
	UnMarkedCell(Vec2i targetPos,int factionIndex) {
		this->targetPos = targetPos;
		this->faction = NULL;
		this->factionIndex = factionIndex;
	}

	Vec2i getTargetPos() const { return targetPos; }
	const Faction * getFaction() const { return faction; }
	void setFaction(const Faction *faction) { this->faction = faction; }
	int getFactionIndex() const { return factionIndex; }
};

typedef int (*DisplayMessageFunction)(const char *msg, bool exit);

class NetworkInterface {

protected:
    static bool allowGameDataSynchCheck;
    static bool allowDownloadDataSynch;
	bool networkGameDataSynchCheckOkMap;
	bool networkGameDataSynchCheckOkTile;
	bool networkGameDataSynchCheckOkTech;
	string networkGameDataSynchCheckTechMismatchReport;
	bool receivedDataSynchCheck;

	std::vector<ChatMsgInfo> chatTextList;
	NetworkMessagePing lastPingInfo;
	std::vector<MarkedCell> markedCellList;
	std::vector<UnMarkedCell> unmarkedCellList;

	static DisplayMessageFunction pCB_DisplayMessage;
	void DisplayErrorMessage(string sErr, bool closeSocket=true);

	virtual Mutex * getServerSynchAccessor() = 0;

	std::vector<MarkedCell> highlightedCellList;

public:
	static const int readyWaitTimeout;
	GameSettings gameSettings;

public:
	virtual ~NetworkInterface(){}

	virtual Socket* getSocket(bool mutexLock=true)= 0;
	virtual void close()= 0;
	virtual string getHumanPlayerName(int index=-1) = 0;
	virtual int getHumanPlayerIndex() const = 0;

    static void setDisplayMessageFunction(DisplayMessageFunction pDisplayMessage) { pCB_DisplayMessage = pDisplayMessage; }
    static DisplayMessageFunction getDisplayMessageFunction() { return pCB_DisplayMessage; }

	string getIp() const		{return Socket::getIp();}
	string getHostName() const	{return Socket::getHostName();}

	virtual void sendMessage(NetworkMessage* networkMessage);
	NetworkMessageType getNextMessageType();
	bool receiveMessage(NetworkMessage* networkMessage);

	virtual bool isConnected();

	const virtual GameSettings * getGameSettings() { return &gameSettings; }
	GameSettings * getGameSettingsPtr() { return &gameSettings; }

	static void setAllowDownloadDataSynch(bool value)          { allowDownloadDataSynch = value; }
	static bool getAllowDownloadDataSynch()                    { return allowDownloadDataSynch; }

	static void setAllowGameDataSynchCheck(bool value)          { allowGameDataSynchCheck = value; }
	static bool getAllowGameDataSynchCheck()                    { return allowGameDataSynchCheck; }

    virtual bool getNetworkGameDataSynchCheckOk()               { return (networkGameDataSynchCheckOkMap && networkGameDataSynchCheckOkTile && networkGameDataSynchCheckOkTech); }
    virtual void setNetworkGameDataSynchCheckOkMap(bool value)  { networkGameDataSynchCheckOkMap = value; }
    virtual void setNetworkGameDataSynchCheckOkTile(bool value) { networkGameDataSynchCheckOkTile = value; }
    virtual void setNetworkGameDataSynchCheckOkTech(bool value) { networkGameDataSynchCheckOkTech = value; }
    virtual bool getNetworkGameDataSynchCheckOkMap()            { return networkGameDataSynchCheckOkMap; }
    virtual bool getNetworkGameDataSynchCheckOkTile()           { return networkGameDataSynchCheckOkTile; }
    virtual bool getNetworkGameDataSynchCheckOkTech()           { return networkGameDataSynchCheckOkTech; }

    std::vector<ChatMsgInfo> getChatTextList(bool clearList);
	void clearChatInfo();
	void addChatInfo(const ChatMsgInfo &msg) { chatTextList.push_back(msg); }

    std::vector<MarkedCell> getMarkedCellList(bool clearList);
	void clearMarkedCellList();
	void addMarkedCell(const MarkedCell &msg) { markedCellList.push_back(msg); }

    std::vector<UnMarkedCell> getUnMarkedCellList(bool clearList);
	void clearUnMarkedCellList();
	void addUnMarkedCell(const UnMarkedCell &msg) { unmarkedCellList.push_back(msg); }

    std::vector<MarkedCell> getHighlightedCellList(bool clearList);
	void clearHighlightedCellList();
	void setHighlightedCell(const MarkedCell &msg);

	virtual bool getConnectHasHandshaked() const= 0;

	NetworkMessagePing getLastPingInfo() const { return lastPingInfo; }
	double getLastPingLag() const {
		return difftime((long int)time(NULL),lastPingInfo.getPingReceivedLocalTime());
	}

	std::string getIpAddress();
	float getThreadedPingMS(std::string host);

	string getNetworkGameDataSynchCheckTechMismatchReport() const {return networkGameDataSynchCheckTechMismatchReport;}
	void setNetworkGameDataSynchCheckTechMismatchReport(string value) {networkGameDataSynchCheckTechMismatchReport = value;}

	bool getReceivedDataSynchCheck() const {return receivedDataSynchCheck;}
	void setReceivedDataSynchCheck(bool value) { receivedDataSynchCheck = value; }

	virtual void saveGame(XmlNode *rootNode) = 0;
	//static void loadGame(string name);

};

// =====================================================
//	class GameNetworkInterface
//
// Adds functions common to servers and clients
// but not connection slots
// =====================================================

class GameNetworkInterface: public NetworkInterface {

protected:
	typedef vector<NetworkCommand> Commands;

	Commands requestedCommands;	//commands requested by the user
	Commands pendingCommands;	//commands ready to be given
	bool quit;

public:
	GameNetworkInterface();

	//message processimg
	virtual void update()= 0;
	virtual void updateLobby()= 0;
	virtual void updateKeyframe(int frameCount)= 0;
	virtual void setKeyframe(int frameCount)= 0;
	virtual void waitUntilReady(Checksum* checksum)= 0;

	//message sending
	virtual void sendTextMessage(const string &text, int teamIndex,bool echoLocal,
			string targetLanguage)= 0;
	virtual void quitGame(bool userManuallyQuit)=0;

	virtual void sendMarkCellMessage(Vec2i targetPos, int factionIndex, string note,int playerIndex) = 0;
	virtual void sendUnMarkCellMessage(Vec2i targetPos, int factionIndex) = 0;
	virtual void sendHighlightCellMessage(Vec2i targetPos, int factionIndex) = 0;


	//misc
	virtual string getNetworkStatus() = 0;

	//access functions
	void requestCommand(const NetworkCommand *networkCommand, bool insertAtStart=false);
	int getPendingCommandCount() const							{return pendingCommands.size();}
	NetworkCommand* getPendingCommand(int i) 		            {return &pendingCommands[i];}
	void clearPendingCommands()									{pendingCommands.clear();}
	bool getQuit() const										{return quit;}
};

// =====================================================
//	class FileTransferSocketThread
// =====================================================

enum FileTransferHostType
{
    eClient,
    eServer
};

enum FileTransferOperationType
{
    eSend,
    eReceive
};

class FileTransferInfo
{
private:

    void CopyAll(const FileTransferInfo &obj)
    {
        hostType    = obj.hostType;
        serverIP    = obj.serverIP;
        serverPort  = obj.serverPort;
        opType      = obj.opType;
        fileName    = obj.fileName;
    }

public:
    FileTransferInfo()
    {
        hostType    = eClient;
        serverIP    = "";
        serverPort  = 0;
        opType      = eSend;
        fileName    = "";
    }
    FileTransferInfo(const FileTransferInfo &obj)
    {
        CopyAll(obj);
    }
    FileTransferInfo &operator=(const FileTransferInfo &obj)
    {
        CopyAll(obj);
        return *this;
    }

    FileTransferHostType hostType;
    string serverIP;
    int32  serverPort;
    FileTransferOperationType opType;
    string fileName;
};

class FileInfo
{
public:
    string fileName;
    int64 filesize;
    uint32 filecrc;
};

class FileTransferSocketThread : public Thread
{
private:
    FileTransferInfo info;

public:
    FileTransferSocketThread(FileTransferInfo fileInfo);
    virtual void execute();
};


}}//end namespace

#endif
