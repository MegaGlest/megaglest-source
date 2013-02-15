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

#ifndef _GLEST_GAME_NETWORKMESSAGE_H_
#define _GLEST_GAME_NETWORKMESSAGE_H_

#include "socket.h"
#include "game_constants.h"
#include "network_types.h"
#include "byte_order.h"
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
	nmtLoadingStatusMessage,
	nmtMarkCell,
	nmtUnMarkCell,
	nmtHighlightCell,

	nmtCount
};

enum NetworkGameStateType {
	nmgstInvalid,
	nmgstOk,
	nmgstNoSlots,

	nmgstCount
};

static const int maxLanguageStringSize= 60;

// =====================================================
//	class NetworkMessage
// =====================================================

class NetworkMessage {
public:
	static bool useOldProtocol;
	virtual ~NetworkMessage(){}
	virtual bool receive(Socket* socket)= 0;
	virtual void send(Socket* socket) = 0;
	virtual size_t getDataSize() const = 0;

	void dump_packet(string label, const void* data, int dataSize);

protected:
	//bool peek(Socket* socket, void* data, int dataSize);
	bool receive(Socket* socket, void* data, int dataSize,bool tryReceiveUntilDataSizeMet);
	void send(Socket* socket, const void* data, int dataSize);

	virtual const char * getPackedMessageFormat() const = 0;
	virtual unsigned int getPackedSize() = 0;
	virtual void unpackMessage(unsigned char *buf) = 0;
	virtual unsigned char * packMessage() = 0;
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
	struct Data {
		int8 messageType;
		int32 sessionId;
		NetworkString<maxVersionStringSize> versionString;
		NetworkString<maxNameSize> name;
		int16 playerIndex;
		int8 gameState;
		uint32 externalIp;
		uint32 ftpPort;
		NetworkString<maxLanguageStringSize> language;
		int8 gameInProgress;
	};
	void toEndian();
	void fromEndian();

private:
	Data data;

protected:

public:
	NetworkMessageIntro();
	NetworkMessageIntro(int32 sessionId, const string &versionString,
			const string &name, int playerIndex, NetworkGameStateType gameState,
			uint32 externalIp, uint32 ftpPort, const string &playerLanguage,
			int gameInProgress);


	virtual const char * getPackedMessageFormat() const;
	virtual unsigned int getPackedSize();
	virtual void unpackMessage(unsigned char *buf);
	virtual unsigned char * packMessage();


	virtual size_t getDataSize() const { return sizeof(Data); }

	int32 getSessionId() const 					{ return data.sessionId;}
	string getVersionString() const				{ return data.versionString.getString(); }
	string getName() const						{ return data.name.getString(); }
	int getPlayerIndex() const					{ return data.playerIndex; }
	NetworkGameStateType getGameState() const 	{ return static_cast<NetworkGameStateType>(data.gameState); }
	uint32 getExternalIp() const                { return data.externalIp;}
	uint32 getFtpPort() const					{ return data.ftpPort; }
	string getPlayerLanguage() const			{ return data.language.getString(); }
	uint8 getGameInProgress() const				{ return data.gameInProgress; }

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket);

	string toString() const;
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
	void toEndian();
	void fromEndian();

private:
	Data data;
	int64 pingReceivedLocalTime;

protected:
	virtual const char * getPackedMessageFormat() const;
	virtual unsigned int getPackedSize();
	virtual void unpackMessage(unsigned char *buf);
	virtual unsigned char * packMessage();

public:
	NetworkMessagePing();
	NetworkMessagePing(int32 pingFrequency, int64 pingTime);

	virtual size_t getDataSize() const { return sizeof(Data); }

	int32 getPingFrequency() const	{return data.pingFrequency;}
	int64 getPingTime() const	{return data.pingTime;}
	int64 getPingReceivedLocalTime() const { return pingReceivedLocalTime; }

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket);
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
		uint32 checksum;
	};
	void toEndian();
	void fromEndian();

private:
	Data data;

protected:
	virtual const char * getPackedMessageFormat() const;
	virtual unsigned int getPackedSize();
	virtual void unpackMessage(unsigned char *buf);
	virtual unsigned char * packMessage();

public:
	NetworkMessageReady();
	NetworkMessageReady(uint32 checksum);

	virtual size_t getDataSize() const { return sizeof(Data); }

	uint32 getChecksum() const	{return data.checksum;}

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket);
};
#pragma pack(pop)

// =====================================================
//	class NetworkMessageLaunch
//
//	Message sent from the server to the client
//	to launch the game
// =====================================================

#pragma pack(push, 1)
class NetworkMessageLaunch: public NetworkMessage {
private:
	static const int maxStringSize= 256;
	static const int maxSmallStringSize= 60;
	static const int maxFactionCRCCount= 20;

private:
	struct Data{
		int8 messageType;
		NetworkString<maxStringSize> description;
		NetworkString<maxSmallStringSize> map;
		NetworkString<maxSmallStringSize> tileset;
		NetworkString<maxSmallStringSize> tech;
		NetworkString<maxSmallStringSize> factionTypeNames[GameConstants::maxPlayers]; //faction names
		NetworkString<maxSmallStringSize> networkPlayerNames[GameConstants::maxPlayers]; //networkPlayerNames
		int32 networkPlayerStatuses[GameConstants::maxPlayers]; //networkPlayerStatuses
		NetworkString<maxSmallStringSize> networkPlayerLanguages[GameConstants::maxPlayers];

		uint32 mapCRC;
		uint32 tilesetCRC;
		uint32 techCRC;
		NetworkString<maxSmallStringSize> factionNameList[maxFactionCRCCount];
		uint32 factionCRCList[maxFactionCRCCount];

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
		uint32 flagTypes1;

		int8 aiAcceptSwitchTeamPercentChance;
		int8 cpuReplacementMultiplier;
		int32 masterserver_admin;
		int32 masterserver_admin_factionIndex;

		NetworkString<maxStringSize> scenario;
	};
	void toEndian();
	void fromEndian();

private:
	Data data;

protected:
	virtual const char * getPackedMessageFormat() const;
	virtual unsigned int getPackedSize();
	virtual void unpackMessage(unsigned char *buf);
	virtual unsigned char * packMessage();

public:
	NetworkMessageLaunch();
	NetworkMessageLaunch(const GameSettings *gameSettings,int8 messageType);

	virtual size_t getDataSize() const { return sizeof(Data); }

	void buildGameSettings(GameSettings *gameSettings) const;
	int getMessageType() const { return data.messageType; }

	int getMapCRC() const { return data.mapCRC; }
	int getTilesetCRC() const { return data.tilesetCRC; }
	int getTechCRC() const { return data.techCRC; }
	vector<pair<string,uint32> > getFactionCRCList() const;

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket);
};
#pragma pack(pop)

// =====================================================
//	class CommandList
//
//	Message to order a commands to several units
// =====================================================

#pragma pack(push, 1)
class NetworkMessageCommandList: public NetworkMessage {

private:
	//static const int maxCommandCount = 2496; // can be as large as 65535

private:
	struct DataHeader {
		int8 messageType;
		uint16 commandCount;
		int32 frameCount;
	};

	static const int32 commandListHeaderSize = sizeof(DataHeader);

	struct Data {
		DataHeader header;
		//NetworkCommand commands[maxCommandCount];
		std::vector<NetworkCommand> commands;
	};
	void toEndianHeader();
	void fromEndianHeader();
	void toEndianDetail(uint16 totalCommand);
	void fromEndianDetail();

private:
	Data data;

protected:
	virtual const char * getPackedMessageFormat() const { return NULL; }
	virtual unsigned int getPackedSize() { return 0; }
	virtual void unpackMessage(unsigned char *buf) { };
	virtual unsigned char * packMessage() { return NULL; }

	const char * getPackedMessageFormatHeader() const;
	unsigned int getPackedSizeHeader();
	void unpackMessageHeader(unsigned char *buf);
	unsigned char * packMessageHeader();

	const char * getPackedMessageFormatDetail() const;
	unsigned int getPackedSizeDetail(int count);
	void unpackMessageDetail(unsigned char *buf,int count);
	unsigned char * packMessageDetail(uint16 totalCommand);

public:
	NetworkMessageCommandList(int32 frameCount= -1);

	virtual size_t getDataSize() const { return sizeof(Data); }

	bool addCommand(const NetworkCommand* networkCommand);

	void clear()									{data.header.commandCount= 0;}
	int getCommandCount() const						{return data.header.commandCount;}
	int getFrameCount() const						{return data.header.frameCount;}
	const NetworkCommand* getCommand(int i) const	{return &data.commands[i];}

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket);
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
	static const int maxTextStringSize= 500;

private:
	struct Data{
		int8 messageType;
		NetworkString<maxTextStringSize> text;
		int8 teamIndex;
		int8 playerIndex;
		NetworkString<maxLanguageStringSize> targetLanguage;
	};
	void toEndian();
	void fromEndian();

private:
	Data data;

protected:
	virtual const char * getPackedMessageFormat() const;
	virtual unsigned int getPackedSize();
	virtual void unpackMessage(unsigned char *buf);
	virtual unsigned char * packMessage();

public:
	NetworkMessageText(){}
	NetworkMessageText(const string &text, int teamIndex, int playerIndex,
			const string targetLanguage);

	virtual size_t getDataSize() const { return sizeof(Data); }

	string getText() const		{return data.text.getString();}
	int getTeamIndex() const	{return data.teamIndex;}
	int getPlayerIndex() const  {return data.playerIndex;}
	string getTargetLanguage() const  {return data.targetLanguage.getString();}

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket);
	NetworkMessageText * getCopy() const;
};
#pragma pack(pop)

// =====================================================
//	class NetworkMessageQuit
//
//	Message sent at the beginning of the game
// =====================================================

#pragma pack(push, 1)
class NetworkMessageQuit: public NetworkMessage{
private:
	struct Data{
		int8 messageType;
	};
	void toEndian();
	void fromEndian();

private:
	Data data;

protected:
	virtual const char * getPackedMessageFormat() const;
	virtual unsigned int getPackedSize();
	virtual void unpackMessage(unsigned char *buf);
	virtual unsigned char * packMessage();

public:
	NetworkMessageQuit();

	virtual size_t getDataSize() const { return sizeof(Data); }

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket);
};
#pragma pack(pop)

// =====================================================
//	class NetworkMessageSynchNetworkGameData
//
//	Message sent at the beginning of a network game
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

		uint32 mapCRC;
		uint32 tilesetCRC;
		uint32 techCRC;

		uint32 techCRCFileCount;
	};

	static const int32 HeaderSize = sizeof(DataHeader);

	struct DataDetail {
		NetworkString<maxStringSize> techCRCFileList[maxFileCRCCount];
		uint32 techCRCFileCRCList[maxFileCRCCount];
	};

	static const int32 DetailSize1 = sizeof(NetworkString<maxStringSize>);
	static const int32 DetailSize2 = sizeof(uint32);

	struct Data {
		DataHeader header;
		DataDetail detail;
	};
	void toEndianHeader();
	void fromEndianHeader();
	void toEndianDetail(uint32 totalFileCount);
	void fromEndianDetail();

private:
	Data data;

protected:
	virtual const char * getPackedMessageFormat() const { return NULL; }
	virtual unsigned int getPackedSize() { return 0; }
	virtual void unpackMessage(unsigned char *buf) { };
	virtual unsigned char * packMessage() { return NULL; }

	const char * getPackedMessageFormatHeader() const;
	unsigned int getPackedSizeHeader();
	void unpackMessageHeader(unsigned char *buf);
	unsigned char * packMessageHeader();

	unsigned int getPackedSizeDetail();
	void unpackMessageDetail(unsigned char *buf);
	unsigned char * packMessageDetail();

public:
    NetworkMessageSynchNetworkGameData() {};
	NetworkMessageSynchNetworkGameData(const GameSettings *gameSettings);

	virtual size_t getDataSize() const { return sizeof(Data); }

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket);

	string getMap() const		{return data.header.map.getString();}
	string getTileset() const   {return data.header.tileset.getString();}
	string getTech() const		{return data.header.tech.getString();}

	uint32 getMapCRC() const		{return data.header.mapCRC;}
	uint32 getTilesetCRC() const	{return data.header.tilesetCRC;}
	uint32 getTechCRC() const	{return data.header.techCRC;}

	uint32 getTechCRCFileCount() const {return data.header.techCRCFileCount;}
	const NetworkString<maxStringSize> * getTechCRCFileList() const {return &data.detail.techCRCFileList[0];}
	const uint32 * getTechCRCFileCRCList() const {return data.detail.techCRCFileCRCList;}

	string getTechCRCFileMismatchReport(vector<std::pair<string,uint32> > &vctFileList);
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

		uint32 mapCRC;
		uint32 tilesetCRC;
		uint32 techCRC;

		uint32 techCRCFileCount;
	};
	static const int32 HeaderSize = sizeof(DataHeader);

	struct DataDetail {
		NetworkString<maxStringSize> techCRCFileList[maxFileCRCCount];
		uint32 techCRCFileCRCList[maxFileCRCCount];
	};

	static const int32 DetailSize1 = sizeof(NetworkString<maxStringSize>);
	static const int32 DetailSize2 = sizeof(uint32);

	struct Data {
		DataHeader header;
		DataDetail detail;
	};
	void toEndianHeader();
	void fromEndianHeader();
	void toEndianDetail(uint32 totalFileCount);
	void fromEndianDetail();

private:
	Data data;

protected:
	virtual const char * getPackedMessageFormat() const { return NULL; }
	virtual unsigned int getPackedSize() { return 0; }
	virtual void unpackMessage(unsigned char *buf) { };
	virtual unsigned char * packMessage() { return NULL; }

public:
    NetworkMessageSynchNetworkGameDataStatus() {};
	NetworkMessageSynchNetworkGameDataStatus(uint32 mapCRC, uint32 tilesetCRC, uint32 techCRC, vector<std::pair<string,uint32> > &vctFileList);

	virtual size_t getDataSize() const { return sizeof(Data); }

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket);

	uint32 getMapCRC() const		{return data.header.mapCRC;}
	uint32 getTilesetCRC() const	{return data.header.tilesetCRC;}
	uint32 getTechCRC() const	{return data.header.techCRC;}

	uint32 getTechCRCFileCount() const {return data.header.techCRCFileCount;}
	const NetworkString<maxStringSize> * getTechCRCFileList() const {return &data.detail.techCRCFileList[0];}
	const uint32 * getTechCRCFileCRCList() const {return data.detail.techCRCFileCRCList;}

	string getTechCRCFileMismatchReport(string techtree, vector<std::pair<string,uint32> > &vctFileList);

};
#pragma pack(pop)

// =====================================================
//	class NetworkMessageSynchNetworkGameDataFileCRCCheck
//
//	Message sent at the beginning of a network game
// =====================================================

#pragma pack(push, 1)
class NetworkMessageSynchNetworkGameDataFileCRCCheck: public NetworkMessage{

private:

static const int maxStringSize= 256;

private:
	struct Data{
		int8 messageType;

		uint32 totalFileCount;
		uint32 fileIndex;
		uint32 fileCRC;
		NetworkString<maxStringSize> fileName;
	};
	void toEndian();
	void fromEndian();

private:
	Data data;

protected:
	virtual const char * getPackedMessageFormat() const;
	virtual unsigned int getPackedSize();
	virtual void unpackMessage(unsigned char *buf);
	virtual unsigned char * packMessage();

public:
    NetworkMessageSynchNetworkGameDataFileCRCCheck() {};
	NetworkMessageSynchNetworkGameDataFileCRCCheck(uint32 totalFileCount, uint32 fileIndex, uint32 fileCRC, const string fileName);

	virtual size_t getDataSize() const { return sizeof(Data); }

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket);

	uint32 getTotalFileCount() const	{return data.totalFileCount;}
	uint32 getFileIndex() const	    {return data.fileIndex;}
	uint32 getFileCRC() const	    {return data.fileCRC;}
	string getFileName() const		{return data.fileName.getString();}
};
#pragma pack(pop)

// =====================================================
//	class NetworkMessageSynchNetworkGameDataFileGet
//
//	Message sent at the beginning of a network game
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
	void toEndian();
	void fromEndian();

private:
	Data data;

protected:
	virtual const char * getPackedMessageFormat() const;
	virtual unsigned int getPackedSize();
	virtual void unpackMessage(unsigned char *buf);
	virtual unsigned char * packMessage();

public:
    NetworkMessageSynchNetworkGameDataFileGet() {};
	NetworkMessageSynchNetworkGameDataFileGet(const string fileName);

	virtual size_t getDataSize() const { return sizeof(Data); }

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket);

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
	ssrft_NetworkPlayerName		= 0x10,
	ssrft_PlayerStatus			= 0x20
};

#pragma pack(push, 1)
class SwitchSetupRequest: public NetworkMessage{
private:
	static const int maxStringSize= 256;
	static const int maxPlayernameStringSize= 80;

private:
	struct Data {
		int8 messageType;
		NetworkString<maxStringSize> selectedFactionName; //wanted faction name
		int8 currentFactionIndex;
		int8 toFactionIndex;
		int8 toTeam;
		NetworkString<maxPlayernameStringSize> networkPlayerName;
		int8 networkPlayerStatus;
		int8 switchFlags;
		NetworkString<maxLanguageStringSize> language;
	};
	void toEndian();
	void fromEndian();

private:
	Data data;

public:
	virtual const char * getPackedMessageFormat() const;
	virtual unsigned int getPackedSize();
	virtual void unpackMessage(unsigned char *buf);
	virtual unsigned char * packMessage();

public:
	SwitchSetupRequest();
	SwitchSetupRequest( string selectedFactionName, int8 currentFactionIndex,
						int8 toFactionIndex,int8 toTeam,string networkPlayerName,
						int8 networkPlayerStatus, int8 flags,
						string language);

	virtual size_t getDataSize() const { return sizeof(Data); }

	string getSelectedFactionName() const	{return data.selectedFactionName.getString();}
	int getCurrentFactionIndex() const		{return data.currentFactionIndex;}
	int getToFactionIndex() const			{return data.toFactionIndex;}
	int getToTeam() const					{return data.toTeam;}
	string getNetworkPlayerName() const		{return data.networkPlayerName.getString(); }
	int getSwitchFlags() const				{return data.switchFlags;}
	void addSwitchFlag(SwitchSetupRequestFlagType flag) { data.switchFlags |= flag;}
	void clearSwitchFlag(SwitchSetupRequestFlagType flag) { data.switchFlags &= ~flag;}

	int getNetworkPlayerStatus() const		{ return data.networkPlayerStatus; }
	string getNetworkPlayerLanguage() const	{ return data.language.getString(); }

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket);
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
	struct Data {
		int8 messageType;
		int16 playerIndex;
	};
	void toEndian();
	void fromEndian();

private:
	Data data;

protected:
	virtual const char * getPackedMessageFormat() const;
	virtual unsigned int getPackedSize();
	virtual void unpackMessage(unsigned char *buf);
	virtual unsigned char * packMessage();

public:
	PlayerIndexMessage( int16 playerIndex);

	virtual size_t getDataSize() const { return sizeof(Data); }

	int16 getPlayerIndex() const	{return data.playerIndex;}

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket);
};
#pragma pack(pop)

// =====================================================
//	class NetworkMessageLoadingStatus
//
//	Message sent during game loading
// =====================================================

enum NetworkMessageLoadingStatusType {
	nmls_NONE = 0x00,

	nmls_PLAYER1_CONNECTED = 0x01,
	nmls_PLAYER2_CONNECTED = 0x02,
	nmls_PLAYER3_CONNECTED = 0x04,
	nmls_PLAYER4_CONNECTED = 0x08,
	nmls_PLAYER5_CONNECTED = 0x10,
	nmls_PLAYER6_CONNECTED = 0x20,
	nmls_PLAYER7_CONNECTED = 0x40,
	nmls_PLAYER8_CONNECTED = 0x80,

	nmls_PLAYER1_READY = 0x100,
	nmls_PLAYER2_READY = 0x200,
	nmls_PLAYER3_READY = 0x400,
	nmls_PLAYER4_READY = 0x1000,
	nmls_PLAYER5_READY = 0x2000,
	nmls_PLAYER6_READY = 0x4000,
	nmls_PLAYER7_READY = 0x8000,
	nmls_PLAYER8_READY = 0x10000
};

#pragma pack(push, 1)
class NetworkMessageLoadingStatus : public NetworkMessage {
private:
	struct Data {
		int8 messageType;
		uint32 status;
	};
	void toEndian();
	void fromEndian();

private:
	Data data;

protected:
	virtual const char * getPackedMessageFormat() const;
	virtual unsigned int getPackedSize();
	virtual void unpackMessage(unsigned char *buf);
	virtual unsigned char * packMessage();

public:
	NetworkMessageLoadingStatus();
	NetworkMessageLoadingStatus(uint32 status);

	virtual size_t getDataSize() const { return sizeof(Data); }

	uint32 getStatus() const	{return data.status;}

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket);
};
#pragma pack(pop)


// =====================================================
//	class NetworkMessageMarkCell
//
//	Mark a Cell message nmtMarkCell
// =====================================================

#pragma pack(push, 1)
class NetworkMessageMarkCell: public NetworkMessage {
private:
	static const int maxTextStringSize= 500;

private:
	struct Data{
		int8 messageType;

		int16 targetX;
		int16 targetY;
		int8 factionIndex;
		int8 playerIndex;
		NetworkString<maxTextStringSize> text;
	};
	void toEndian();
	void fromEndian();

private:
	Data data;

protected:
	virtual const char * getPackedMessageFormat() const;
	virtual unsigned int getPackedSize();
	virtual void unpackMessage(unsigned char *buf);
	virtual unsigned char * packMessage();

public:
	NetworkMessageMarkCell(){}
	NetworkMessageMarkCell(Vec2i target, int factionIndex, const string &text, int playerIndex);

	virtual size_t getDataSize() const { return sizeof(Data); }

	string getText() const			{ return data.text.getString(); }
	Vec2i getTarget() const		{ return Vec2i(data.targetX,data.targetY); }
	int getFactionIndex() const  { return data.factionIndex; }
	int getPlayerIndex() const { return data.playerIndex; }

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket);
	NetworkMessageMarkCell * getCopy() const;
};
#pragma pack(pop)

// =====================================================
//	class NetworkUnMessageMarkCell
//
//	Mark a Cell message nmtUnMarkCell
// =====================================================

#pragma pack(push, 1)
class NetworkMessageUnMarkCell: public NetworkMessage {

private:
	struct Data {
		int8 messageType;

		int16 targetX;
		int16 targetY;
		int8 factionIndex;
	};
	void toEndian();
	void fromEndian();

private:
	Data data;

protected:
	virtual const char * getPackedMessageFormat() const;
	virtual unsigned int getPackedSize();
	virtual void unpackMessage(unsigned char *buf);
	virtual unsigned char * packMessage();

public:
	NetworkMessageUnMarkCell(){}
	NetworkMessageUnMarkCell(Vec2i target, int factionIndex);

	virtual size_t getDataSize() const { return sizeof(Data); }

	Vec2i getTarget() const		{ return Vec2i(data.targetX,data.targetY); }
	int getFactionIndex() const  { return data.factionIndex; }

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket);
	NetworkMessageUnMarkCell * getCopy() const;
};
#pragma pack(pop)

// =====================================================
//	class NetworkHighlightCellMessage
//
//	Highlight a Cell message
// =====================================================


#pragma pack(push, 1)
class NetworkMessageHighlightCell: public NetworkMessage {
private:
	static const int maxTextStringSize= 500;

private:
	struct Data{
		int8 messageType;

		int16 targetX;
		int16 targetY;
		int8 factionIndex;
	};
	void toEndian();
	void fromEndian();

private:
	Data data;

protected:
	virtual const char * getPackedMessageFormat() const;
	virtual unsigned int getPackedSize();
	virtual void unpackMessage(unsigned char *buf);
	virtual unsigned char * packMessage();

public:
	NetworkMessageHighlightCell(){}
	NetworkMessageHighlightCell(Vec2i target, int factionIndex);

	virtual size_t getDataSize() const { return sizeof(Data); }

	Vec2i getTarget() const		{ return Vec2i(data.targetX,data.targetY); }
	int getFactionIndex() const  { return data.factionIndex; }

	virtual bool receive(Socket* socket);
	virtual void send(Socket* socket);
};
#pragma pack(pop)


}}//end namespace

#endif
