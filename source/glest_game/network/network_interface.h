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

#include <string>
#include <vector>

#include "checksum.h"
#include "network_message.h"
#include "network_types.h"

#include "game_settings.h"

#include "thread.h"
#include "types.h"

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

typedef int (*DisplayMessageFunction)(const char *msg, bool exit);

class NetworkInterface {

protected:
    static bool allowGameDataSynchCheck;
    static bool allowDownloadDataSynch;
	bool networkGameDataSynchCheckOkMap;
	bool networkGameDataSynchCheckOkTile;
	bool networkGameDataSynchCheckOkTech;
	bool networkGameDataSynchCheckOkFogOfWar;

	string chatText;
	string chatSender;
	int chatTeamIndex;
	static DisplayMessageFunction pCB_DisplayMessage;
	void DisplayErrorMessage(string sErr, bool closeSocket=true);

public:
	static const int readyWaitTimeout;
	GameSettings gameSettings;

public:
	virtual ~NetworkInterface(){}

	virtual Socket* getSocket()= 0;
	virtual const Socket* getSocket() const= 0;
	virtual void close()= 0;

    static void setDisplayMessageFunction(DisplayMessageFunction pDisplayMessage) { pCB_DisplayMessage = pDisplayMessage; }
	string getIp() const		{return getSocket()->getIp();}
	string getHostName() const	{return getSocket()->getHostName();}

	void sendMessage(const NetworkMessage* networkMessage);
	NetworkMessageType getNextMessageType(bool checkHasDataFirst = false);
	bool receiveMessage(NetworkMessage* networkMessage);

	bool isConnected();

	//virtual void setGameSettings(GameSettings *serverGameSettings) { gameSettings = *serverGameSettings; }
	const virtual GameSettings * getGameSettings() { return &gameSettings; }

	static void setAllowDownloadDataSynch(bool value)          { allowDownloadDataSynch = value; }
	static bool getAllowDownloadDataSynch()                    { return allowDownloadDataSynch; }

	static void setAllowGameDataSynchCheck(bool value)          { allowGameDataSynchCheck = value; }
	static bool getAllowGameDataSynchCheck()                    { return allowGameDataSynchCheck; }

    virtual bool getNetworkGameDataSynchCheckOk()               { return (networkGameDataSynchCheckOkMap && networkGameDataSynchCheckOkTile && networkGameDataSynchCheckOkTech && networkGameDataSynchCheckOkFogOfWar); }
    virtual void setNetworkGameDataSynchCheckOkMap(bool value)  { networkGameDataSynchCheckOkMap = value; }
    virtual void setNetworkGameDataSynchCheckOkTile(bool value) { networkGameDataSynchCheckOkTile = value; }
    virtual void setNetworkGameDataSynchCheckOkTech(bool value) { networkGameDataSynchCheckOkTech = value; }
    virtual bool getNetworkGameDataSynchCheckOkMap()            { return networkGameDataSynchCheckOkMap; }
    virtual bool getNetworkGameDataSynchCheckOkTile()           { return networkGameDataSynchCheckOkTile; }
    virtual bool getNetworkGameDataSynchCheckOkTech()           { return networkGameDataSynchCheckOkTech; }
    virtual bool getFogOfWar()=0;
    virtual bool getNetworkGameDataSynchCheckOkFogOfWar()       { return networkGameDataSynchCheckOkFogOfWar; }
    virtual void setNetworkGameDataSynchCheckOkFogOfWar(bool value)  { networkGameDataSynchCheckOkFogOfWar = value; }

	const string getChatText() const							{return chatText;}
	const string getChatSender() const							{return chatSender;}
	int getChatTeamIndex() const								{return chatTeamIndex;}
};

// =====================================================
//	class GameNetworkInterface
//
// Adds functions common to servers and clients
// but not connection slots
// =====================================================

class GameNetworkInterface: public NetworkInterface{
private:
	typedef vector<NetworkCommand> Commands;

protected:
	Commands requestedCommands;	//commands requested by the user
	Commands pendingCommands;	//commands ready to be given
	bool quit;

public:
	GameNetworkInterface();

	//message processimg
	virtual void update()= 0;
	virtual void updateLobby()= 0;
	virtual void updateKeyframe(int frameCount)= 0;
	virtual void waitUntilReady(Checksum* checksum)= 0;

	//message sending
	virtual void sendTextMessage(const string &text, int teamIndex)= 0;
	virtual void quitGame(bool userManuallyQuit)=0;

	//misc
	virtual string getNetworkStatus() const= 0;

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
    int32 filecrc;
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
