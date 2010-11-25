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

#ifndef _GLEST_GAME_NETWORKMESSAGE_H_
#define _GLEST_GAME_NETWORKMESSAGE_H_

#include "socket.h"
#include "game_constants.h"
#include "network_types.h"
#include "leak_dumper.h"

using Shared::Platform::Socket;
using Shared::Platform::int8;
using Shared::Platform::uint8;
using Shared::Platform::int16;

namespace Glest{ namespace Game{

class GameSettings;

enum NetworkMessageType {
	nmtInvalid,
	nmtIntro,
	nmtPing,
	nmtReady,
	nmtLaunch,
	nmtCommandList,
	nmtText,
	nmtQuit,
	nmtSynchNetworkGameData,
	nmtSynchNetworkGameDataStatus,
	nmtSynchNetworkGameDataFileCRCCheck,
	nmtSynchNetworkGameDataFileGet,
	nmtBroadCastSetup,
	nmtSwitchSetupRequest,
	nmtPlayerIndexMessage,

	nmtCount
};

enum NetworkGameStateType {
	nmgstInvalid,
	nmgstOk,
	nmgstNoSlots,

	nmgstCount
};

// =====================================================
//	class NetworkMessage
// =====================================================

class NetworkMessage {
public:
	virtual ~NetworkMessage(){}
	virtual bool receive(Socket* socket)= 0;
	virtual void send(Socket* socket) const = 0;

protected:
	bool peek(Socket* socket, void* data, int dataSize);
	bool receive(Socket* socket, void* data, int dataSize);
	void send(Socket* socket, const void* data, int dataSize) const;
};

// =====================================================
//	class NetworkMessageIntro
//
//	Message sent from the server to the client
//	when the client connects and vice versa
// =====================================================

#pragma pack(push, 1)
class NetworkMessageIntro: public NetworkMessage{
private:
	static const int maxVersionStringSize= 128;
	static const int maxNameSize= 32;

private:
	struct Data{
		int8 messageType;
		int32 sessionId;
		NetworkString<maxVersionStringSize> versionString;
		NetworkString<maxNameSize> name;
		int16 playerIndex;
		int8 gameState;
	};

private:
	Data data;

public:
	NetworkMessageIntro();
	NetworkMessageIntro(int32 sessionId, const string &versionString, const string &name, int playerIndex, NetworkGameStateType gameState);

	int32 getSessionId() const 					{ return data.sessionId;}
	string getVersionString() const				{ return data.versionString.getString(); }
	string getName() const						{ return data.name.getString(); }
	int getPlayerIndex() const					{ return data.playerIndex; }
	NetworkGameStateType getGameState() const 	{ return static_cast<NetworkGameStateType>(data.gameState); }

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
};
#pragma pack(pop)

// =====================================================
//	class NetworkMessagePing
//
//	Message sent at any time
// =====================================================

#pragma pack(push, 1)
class NetworkMessagePing: public NetworkMessage{
private:
	struct Data{
		int8 messageType;
		int32 pingFrequency;
		int64 pingTime;
	};

private:
	Data data;
	int64 pingReceivedLocalTime;

public:
	NetworkMessagePing();
	NetworkMessagePing(int32 pingFrequency, int64 pingTime);

	int32 getPingFrequency() const	{return data.pingFrequency;}
	int64 getPingTime() const	{return data.pingTime;}
	int64 getPingReceivedLocalTime() const { return pingReceivedLocalTime; }

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
};
#pragma pack(pop)

// =====================================================
//	class NetworkMessageReady
//
//	Message sent at the beginning of the game
// =====================================================

#pragma pack(push, 1)
class NetworkMessageReady: public NetworkMessage{
private:
	struct Data{
		int8 messageType;
		int32 checksum;
	};

private:
	Data data;

public:
	NetworkMessageReady();
	NetworkMessageReady(int32 checksum);

	int32 getChecksum() const	{return data.checksum;}

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
};
#pragma pack(pop)

// =====================================================
//	class NetworkMessageLaunch
//
//	Message sent from the server to the client
//	to launch the game
// =====================================================

#pragma pack(push, 1)
class NetworkMessageLaunch: public NetworkMessage{
private:
	static const int maxStringSize= 256;
	static const int maxSmallStringSize= 60;

private:
	struct Data{
		int8 messageType;
		NetworkString<maxStringSize> description;
		NetworkString<maxSmallStringSize> map;
		NetworkString<maxSmallStringSize> tileset;
		NetworkString<maxSmallStringSize> tech;
		NetworkString<maxSmallStringSize> factionTypeNames[GameConstants::maxPlayers]; //faction names
		NetworkString<maxSmallStringSize> networkPlayerNames[GameConstants::maxPlayers]; //networkPlayerNames

		int8 factionControls[GameConstants::maxPlayers];
		int8 resourceMultiplierIndex[GameConstants::maxPlayers];

		int8 thisFactionIndex;
		int8 factionCount;
		int8 teams[GameConstants::maxPlayers];
		int8 startLocationIndex[GameConstants::maxPlayers];
		int8 defaultResources;
        int8 defaultUnits;
        int8 defaultVictoryConditions;
		int8 fogOfWar;
		int8 allowObservers;
		int8 enableObserverModeAtEndGame;
		int8 enableServerControlledAI;
		uint8 networkFramePeriod; // allowed values 0 - 255
		int8 networkPauseGameForLaggedClients;
		int8 pathFinderType;
	};

private:
	Data data;

public:
	NetworkMessageLaunch();
	NetworkMessageLaunch(const GameSettings *gameSettings,int8 messageType);

	void buildGameSettings(GameSettings *gameSettings) const;
	int getMessageType() const { return data.messageType; }

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
};
#pragma pack(pop)

// =====================================================
//	class CommandList
//
//	Message to order a commands to several units
// =====================================================

//const int32 commandListHeaderSize = 6;

#pragma pack(push, 1)
class NetworkMessageCommandList: public NetworkMessage{
private:
	//static const int maxCommandCount= 16*4;
	//static const int maxCommandCount = (16 * 4) * 2;
	static const int maxCommandCount = 2048; // can be as large as 65535

private:
	struct DataHeader {
		int8 messageType;
		uint16 commandCount;
		//int8 commandCount;
		int32 frameCount;
	};

	static const int32 commandListHeaderSize = sizeof(DataHeader);

	struct Data {
		DataHeader header;
		NetworkCommand commands[maxCommandCount];
	};

private:
	Data data;

public:
	NetworkMessageCommandList(int32 frameCount= -1);

	bool addCommand(const NetworkCommand* networkCommand);

	void clear()									{data.header.commandCount= 0;}
	int getCommandCount() const						{return data.header.commandCount;}
	int getFrameCount() const						{return data.header.frameCount;}
	const NetworkCommand* getCommand(int i) const	{return &data.commands[i];}

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
};
#pragma pack(pop)

// =====================================================
//	class NetworkMessageText
//
//	Chat text message
// =====================================================

#pragma pack(push, 1)
class NetworkMessageText: public NetworkMessage {
private:
	static const int maxTextStringSize= 340;
	//static const int maxSenderStringSize= 60;

private:
	struct Data{
		int8 messageType;
		NetworkString<maxTextStringSize> text;
		//NetworkString<maxSenderStringSize> sender;
		int8 teamIndex;
		int8 playerIndex;
	};

private:
	Data data;

public:
	NetworkMessageText(){}
	//NetworkMessageText(const string &text, const string &sender, int teamIndex, int playerIndex);
	NetworkMessageText(const string &text, int teamIndex, int playerIndex);

	string getText() const		{return data.text.getString();}
	//string getSender() const	{return data.sender.getString();}
	int getTeamIndex() const	{return data.teamIndex;}
	int getPlayerIndex() const  {return data.playerIndex;}

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
};
#pragma pack(pop)

// =====================================================
//	class NetworkMessageQuit
//
//	Message sent at the beggining of the game
// =====================================================

#pragma pack(push, 1)
class NetworkMessageQuit: public NetworkMessage{
private:
	struct Data{
		int8 messageType;
	};

private:
	Data data;

public:
	NetworkMessageQuit();

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
};
#pragma pack(pop)

// =====================================================
//	class NetworkMessageSynchNetworkGameData
//
//	Message sent at the beggining of a network game
// =====================================================

#pragma pack(push, 1)
class NetworkMessageSynchNetworkGameData: public NetworkMessage{

private:

static const int maxStringSize= 255;
static const int maxFileCRCCount= 1500;
static const int maxFileCRCPacketCount= 25;

private:

	struct DataHeader {
		int8 messageType;

		NetworkString<maxStringSize> map;
		NetworkString<maxStringSize> tileset;
		NetworkString<maxStringSize> tech;

		int32 mapCRC;
		int32 tilesetCRC;
		int32 techCRC;

		int32 techCRCFileCount;
	};

	static const int32 HeaderSize = sizeof(DataHeader);

	struct DataDetail {
		NetworkString<maxStringSize> techCRCFileList[maxFileCRCCount];
		int32 techCRCFileCRCList[maxFileCRCCount];
	};

	static const int32 DetailSize1 = sizeof(NetworkString<maxStringSize>);
	static const int32 DetailSize2 = sizeof(int32);

	struct Data {
		DataHeader header;
		DataDetail detail;
	};

private:
	Data data;

public:
    NetworkMessageSynchNetworkGameData() {};
	NetworkMessageSynchNetworkGameData(const GameSettings *gameSettings);

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;

	string getMap() const		{return data.header.map.getString();}
	string getTileset() const   {return data.header.tileset.getString();}
	string getTech() const		{return data.header.tech.getString();}

	int32 getMapCRC() const		{return data.header.mapCRC;}
	int32 getTilesetCRC() const	{return data.header.tilesetCRC;}
	int32 getTechCRC() const	{return data.header.techCRC;}

	int32 getTechCRCFileCount() const {return data.header.techCRCFileCount;}
	const NetworkString<maxStringSize> * getTechCRCFileList() const {return &data.detail.techCRCFileList[0];}
	const int32 * getTechCRCFileCRCList() const {return data.detail.techCRCFileCRCList;}

	string getTechCRCFileMismatchReport(vector<std::pair<string,int32> > &vctFileList);
};
#pragma pack(pop)

// =====================================================
//	class NetworkMessageSynchNetworkGameDataStatus
//
//	Message sent at the beggining of a network game
// =====================================================

#pragma pack(push, 1)
class NetworkMessageSynchNetworkGameDataStatus: public NetworkMessage{

private:

static const int maxStringSize= 255;
static const int maxFileCRCCount= 1500;
static const int maxFileCRCPacketCount= 25;

private:

	struct DataHeader {
		int8 messageType;

		int32 mapCRC;
		int32 tilesetCRC;
		int32 techCRC;

		int32 techCRCFileCount;
	};
	static const int32 HeaderSize = sizeof(DataHeader);

	struct DataDetail {
		NetworkString<maxStringSize> techCRCFileList[maxFileCRCCount];
		int32 techCRCFileCRCList[maxFileCRCCount];
	};

	static const int32 DetailSize1 = sizeof(NetworkString<maxStringSize>);
	static const int32 DetailSize2 = sizeof(int32);

	struct Data {
		DataHeader header;
		DataDetail detail;
	};

private:
	Data data;

public:
    NetworkMessageSynchNetworkGameDataStatus() {};
	NetworkMessageSynchNetworkGameDataStatus(int32 mapCRC, int32 tilesetCRC, int32 techCRC, vector<std::pair<string,int32> > &vctFileList);

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;

	int32 getMapCRC() const		{return data.header.mapCRC;}
	int32 getTilesetCRC() const	{return data.header.tilesetCRC;}
	int32 getTechCRC() const	{return data.header.techCRC;}

	int32 getTechCRCFileCount() const {return data.header.techCRCFileCount;}
	const NetworkString<maxStringSize> * getTechCRCFileList() const {return &data.detail.techCRCFileList[0];}
	const int32 * getTechCRCFileCRCList() const {return data.detail.techCRCFileCRCList;}

	string getTechCRCFileMismatchReport(string techtree, vector<std::pair<string,int32> > &vctFileList);

};
#pragma pack(pop)

// =====================================================
//	class NetworkMessageSynchNetworkGameDataFileCRCCheck
//
//	Message sent at the beggining of a network game
// =====================================================

#pragma pack(push, 1)
class NetworkMessageSynchNetworkGameDataFileCRCCheck: public NetworkMessage{

private:

static const int maxStringSize= 256;

private:
	struct Data{
		int8 messageType;

		int32 totalFileCount;
		int32 fileIndex;
		int32 fileCRC;
		NetworkString<maxStringSize> fileName;
	};

private:
	Data data;

public:
    NetworkMessageSynchNetworkGameDataFileCRCCheck() {};
	NetworkMessageSynchNetworkGameDataFileCRCCheck(int32 totalFileCount, int32 fileIndex, int32 fileCRC, const string fileName);

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;

	int32 getTotalFileCount() const	{return data.totalFileCount;}
	int32 getFileIndex() const	    {return data.fileIndex;}
	int32 getFileCRC() const	    {return data.fileCRC;}
	string getFileName() const		{return data.fileName.getString();}
};
#pragma pack(pop)

// =====================================================
//	class NetworkMessageSynchNetworkGameDataFileGet
//
//	Message sent at the beggining of a network game
// =====================================================

#pragma pack(push, 1)
class NetworkMessageSynchNetworkGameDataFileGet: public NetworkMessage{

private:

static const int maxStringSize= 256;

private:
	struct Data{
		int8 messageType;

		NetworkString<maxStringSize> fileName;
	};

private:
	Data data;

public:
    NetworkMessageSynchNetworkGameDataFileGet() {};
	NetworkMessageSynchNetworkGameDataFileGet(const string fileName);

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;

	string getFileName() const		{return data.fileName.getString();}
};
#pragma pack(pop)

// =====================================================
//	class SwitchSetupRequest
//
//	Message sent from the client to the server
//	to switch its settings
// =====================================================

// Each bit represents which item in the packet has a changed value
enum SwitchSetupRequestFlagType {
	ssrft_None					= 0x00,
	ssrft_SelectedFactionName	= 0x01,
	ssrft_CurrentFactionIndex	= 0x02,
	ssrft_ToFactionIndex		= 0x04,
	ssrft_ToTeam				= 0x08,
	ssrft_NetworkPlayerName		= 0x10
};

#pragma pack(push, 1)
class SwitchSetupRequest: public NetworkMessage{
private:
	static const int maxStringSize= 256;
	static const int maxPlayernameStringSize= 80;

private:
	struct Data{
		int8 messageType;
		NetworkString<maxStringSize> selectedFactionName; //wanted faction name
		int8 currentFactionIndex;
		int8 toFactionIndex;
		int8 toTeam;
		NetworkString<maxPlayernameStringSize> networkPlayerName;
		int8 switchFlags;
	};

private:
	Data data;

public:
	SwitchSetupRequest();
	SwitchSetupRequest( string selectedFactionName, int8 currentFactionIndex,
						int8 toFactionIndex,int8 toTeam,string networkPlayerName, int8 flags);

	string getSelectedFactionName() const	{return data.selectedFactionName.getString();}
	int getCurrentFactionIndex() const		{return data.currentFactionIndex;}
	int getToFactionIndex() const			{return data.toFactionIndex;}
	int getToTeam() const					{return data.toTeam;}
	string getNetworkPlayerName() const		{return data.networkPlayerName.getString(); }
	int getSwitchFlags() const				{return data.switchFlags;}
	int addSwitchFlag(SwitchSetupRequestFlagType flag) { data.switchFlags |= flag;}
	int clearSwitchFlag(SwitchSetupRequestFlagType flag) { data.switchFlags &= ~flag;}

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
};
#pragma pack(pop)

// =====================================================
//	class PlayerIndexMessage
//
//	Message sent from the server to the clients
//	to tell them about a slot change ( caused by another client )
// =====================================================

#pragma pack(push, 1)
class PlayerIndexMessage: public NetworkMessage{

private:
	struct Data{
		int8 messageType;
		int16 playerIndex;
	};

private:
	Data data;

public:
	PlayerIndexMessage( int16 playerIndex);

	int16 getPlayerIndex() const	{return data.playerIndex;}

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket) const;
};
#pragma pack(pop)

}}//end namespace

#endif
