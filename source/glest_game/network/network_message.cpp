// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "network_message.h"

#include <cassert>
#include <stdexcept>

#include "types.h"
#include "util.h"
#include "game_settings.h"

#include "leak_dumper.h"

#include "checksum.h"
#include "map.h"
#include "platform_util.h"
#include "config.h"

using namespace Shared::Platform;
using namespace Shared::Util;
using namespace std;

namespace Glest{ namespace Game{

// =====================================================
//	class NetworkMessage
// =====================================================

bool NetworkMessage::peek(Socket* socket, void* data, int dataSize)
{
	if(socket != NULL) {
		int ipeekdatalen = socket->getDataToRead();
		if(ipeekdatalen >= dataSize)
		{
			if(socket->peek(data, dataSize)!=dataSize)
			{
				if(socket != NULL && socket->getSocketId() > 0)
				{
					throw runtime_error("Error peeking NetworkMessage");
				}
				else
				{
					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d socket has been disconnected\n",__FILE__,__FUNCTION__,__LINE__);
				}
			}
			else
			{
				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] dataSize = %d\n",__FILE__,__FUNCTION__,dataSize);
			}
			return true;
		}
		else
		{
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] socket->getDataToRead() returned %d\n",__FILE__,__FUNCTION__,ipeekdatalen);
		}
	}
	return false;
}

bool NetworkMessage::receive(Socket* socket, void* data, int dataSize)
{
	if(socket != NULL) {
		int ipeekdatalen = socket->getDataToRead();
		if(ipeekdatalen >= dataSize)
		{
			if(socket->receive(data, dataSize)!=dataSize)
			{
				if(socket != NULL && socket->getSocketId() > 0)
				{
					throw runtime_error("Error receiving NetworkMessage");
				}
				else
				{
					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] socket has been disconnected\n",__FILE__,__FUNCTION__);
				}
			}
			else
			{
				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] dataSize = %d\n",__FILE__,__FUNCTION__,dataSize);
			}
			return true;
		}
		else
		{
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] socket->getDataToRead() returned %d\n",__FILE__,__FUNCTION__,ipeekdatalen);
		}
	}
	return false;
}

void NetworkMessage::send(Socket* socket, const void* data, int dataSize) const
{
	if(socket != NULL) {
		int sendResult = socket->send(data, dataSize);
		if(sendResult != dataSize) {
			if(socket != NULL && socket->getSocketId() > 0) {
				char szBuf[1024]="";
				sprintf(szBuf,"Error sending NetworkMessage, sendResult = %d, dataSize = %d",sendResult,dataSize);
				throw runtime_error(szBuf);
			}
			else {
				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d socket has been disconnected\n",__FILE__,__FUNCTION__,__LINE__);
			}
		}
	}
}

// =====================================================
//	class NetworkMessageIntro
// =====================================================

NetworkMessageIntro::NetworkMessageIntro(){
	data.messageType= -1;
	data.playerIndex= -1;
	data.gameState	= nmgstInvalid;
}

NetworkMessageIntro::NetworkMessageIntro(const string &versionString, const string &name, int playerIndex, NetworkGameStateType gameState) {
	data.messageType	= nmtIntro;
	data.versionString	= versionString;
	data.name			= name;
	data.playerIndex	= static_cast<int16>(playerIndex);
	data.gameState		= static_cast<int8>(gameState);
}

bool NetworkMessageIntro::receive(Socket* socket){
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void NetworkMessageIntro::send(Socket* socket) const{
	assert(data.messageType==nmtIntro);
	NetworkMessage::send(socket, &data, sizeof(data));
}

// =====================================================
//	class NetworkMessageReady
// =====================================================

NetworkMessageReady::NetworkMessageReady(){
	data.messageType= nmtReady;
}

NetworkMessageReady::NetworkMessageReady(int32 checksum){
	data.messageType= nmtReady;
	data.checksum= checksum;
}

bool NetworkMessageReady::receive(Socket* socket){
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void NetworkMessageReady::send(Socket* socket) const{
	assert(data.messageType==nmtReady);
	NetworkMessage::send(socket, &data, sizeof(data));
}

// =====================================================
//	class NetworkMessageLaunch
// =====================================================

NetworkMessageLaunch::NetworkMessageLaunch(){
	data.messageType=-1;
}

NetworkMessageLaunch::NetworkMessageLaunch(const GameSettings *gameSettings,int8 messageType){
	data.messageType=messageType;

	data.description= gameSettings->getDescription();
	data.map= gameSettings->getMap();
	data.tileset= gameSettings->getTileset();
	data.tech= gameSettings->getTech();
	data.factionCount= gameSettings->getFactionCount();
	data.thisFactionIndex= gameSettings->getThisFactionIndex();
	data.defaultResources= gameSettings->getDefaultResources();
    data.defaultUnits= gameSettings->getDefaultUnits();
    data.defaultVictoryConditions= gameSettings->getDefaultVictoryConditions();
	data.fogOfWar = gameSettings->getFogOfWar();
	data.enableObserverModeAtEndGame = gameSettings->getEnableObserverModeAtEndGame();
	data.enableServerControlledAI = gameSettings->getEnableServerControlledAI();
	data.networkFramePeriod = gameSettings->getNetworkFramePeriod();

	for(int i= 0; i<data.factionCount; ++i){
		data.factionTypeNames[i]= gameSettings->getFactionTypeName(i);
		data.networkPlayerNames[i]= gameSettings->getNetworkPlayerName(i);
		data.factionControls[i]= gameSettings->getFactionControl(i);
		data.teams[i]= gameSettings->getTeam(i);
		data.startLocationIndex[i]= gameSettings->getStartLocationIndex(i);
	}
}

void NetworkMessageLaunch::buildGameSettings(GameSettings *gameSettings) const{
	gameSettings->setDescription(data.description.getString());
	gameSettings->setMap(data.map.getString());
	gameSettings->setTileset(data.tileset.getString());
	gameSettings->setTech(data.tech.getString());
	gameSettings->setFactionCount(data.factionCount);
	gameSettings->setThisFactionIndex(data.thisFactionIndex);
	gameSettings->setDefaultResources(data.defaultResources);
    gameSettings->setDefaultUnits(data.defaultUnits);
    gameSettings->setDefaultVictoryConditions(data.defaultVictoryConditions);
	gameSettings->setFogOfWar(data.fogOfWar);

	gameSettings->setFogOfWar(data.fogOfWar);
	gameSettings->setEnableObserverModeAtEndGame(data.enableObserverModeAtEndGame);
	gameSettings->setEnableServerControlledAI(data.enableServerControlledAI);
	gameSettings->setNetworkFramePeriod(data.networkFramePeriod);

	for(int i= 0; i<data.factionCount; ++i){
		gameSettings->setFactionTypeName(i, data.factionTypeNames[i].getString());
		gameSettings->setNetworkPlayerName(i,data.networkPlayerNames[i].getString());
		gameSettings->setFactionControl(i, static_cast<ControlType>(data.factionControls[i]));
		gameSettings->setTeam(i, data.teams[i]);
		gameSettings->setStartLocationIndex(i, data.startLocationIndex[i]);
	}
}

bool NetworkMessageLaunch::receive(Socket* socket){
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void NetworkMessageLaunch::send(Socket* socket) const{
	NetworkMessage::send(socket, &data, sizeof(data));
}

// =====================================================
//	class NetworkMessageLaunch
// =====================================================

NetworkMessageCommandList::NetworkMessageCommandList(int32 frameCount){
	data.header.messageType= nmtCommandList;
	data.header.frameCount= frameCount;
	data.header.commandCount= 0;
}

bool NetworkMessageCommandList::addCommand(const NetworkCommand* networkCommand){
	if(data.header.commandCount < maxCommandCount){
		data.commands[static_cast<int>(data.header.commandCount)]= *networkCommand;
		data.header.commandCount++;
		return true;
	}
	return false;
}

bool NetworkMessageCommandList::receive(Socket* socket) {
    // _peek_ type, commandCount & frame num first.
	for(int peekAttempt = 1; peekAttempt < 5; peekAttempt++) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] peekAttempt = %d\n",__FILE__,__FUNCTION__,__LINE__,peekAttempt);

		if (NetworkMessage::peek(socket, &data, commandListHeaderSize) == true) {
			break;
		}
		else {
			sleep(1); // sleep 1 ms to wait for socket data
		}
	}

	if (NetworkMessage::peek(socket, &data, commandListHeaderSize) == false) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ERROR / WARNING!!! NetworkMessage::peek failed!\n",__FILE__,__FUNCTION__,__LINE__);
		return false;
	}

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] messageType = %d, frameCount = %d, data.commandCount = %d\n",
        __FILE__,__FUNCTION__,__LINE__,data.header.messageType,data.header.frameCount,data.header.commandCount);

	// read header + data.commandCount commands.
	int totalMsgSize = commandListHeaderSize + (sizeof(NetworkCommand) * data.header.commandCount);
	if (socket->getDataToRead() < totalMsgSize) {
	    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ERROR / WARNING!!! Insufficient data to read entire command list [need %d bytes, only %d available].\n",
			__FILE__,__FUNCTION__,__LINE__, totalMsgSize, socket->getDataToRead());
		return false;
	}
	bool result = NetworkMessage::receive(socket, &data, totalMsgSize);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled == true) {
        for(int idx = 0 ; idx < data.header.commandCount; ++idx) {
            const NetworkCommand &cmd = data.commands[idx];

            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] index = %d, received networkCommand [%s]\n",
                    __FILE__,__FUNCTION__,__LINE__,idx, cmd.toString().c_str());
        }
	}
	return result;
}

void NetworkMessageCommandList::send(Socket* socket) const{
	assert(data.header.messageType==nmtCommandList);
	int totalMsgSize = commandListHeaderSize + (sizeof(NetworkCommand) * data.header.commandCount);
	NetworkMessage::send(socket, &data, totalMsgSize);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled == true) {
	    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] messageType = %d, frameCount = %d, data.commandCount = %d\n",
                __FILE__,__FUNCTION__,__LINE__,data.header.messageType,data.header.frameCount,data.header.commandCount);

        if (data.header.commandCount > 0) {
            for(int idx = 0 ; idx < data.header.commandCount; ++idx) {
                const NetworkCommand &cmd = data.commands[idx];

                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] index = %d, sent networkCommand [%s]\n",
                        __FILE__,__FUNCTION__,__LINE__,idx, cmd.toString().c_str());
            }
        }
	}
}

// =====================================================
//	class NetworkMessageText
// =====================================================

NetworkMessageText::NetworkMessageText(const string &text, const string &sender, int teamIndex){
	data.messageType= nmtText;
	data.text= text;
	data.sender= sender;
	data.teamIndex= teamIndex;
}

bool NetworkMessageText::receive(Socket* socket){
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void NetworkMessageText::send(Socket* socket) const{
	assert(data.messageType==nmtText);
	NetworkMessage::send(socket, &data, sizeof(data));
}

// =====================================================
//	class NetworkMessageQuit
// =====================================================

NetworkMessageQuit::NetworkMessageQuit(){
	data.messageType= nmtQuit;
}

bool NetworkMessageQuit::receive(Socket* socket){
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void NetworkMessageQuit::send(Socket* socket) const{
	assert(data.messageType==nmtQuit);
	NetworkMessage::send(socket, &data, sizeof(data));
}

// =====================================================
//	class NetworkMessageSynchNetworkGameData
// =====================================================

NetworkMessageSynchNetworkGameData::NetworkMessageSynchNetworkGameData(const GameSettings *gameSettings)
{
	data.messageType= nmtSynchNetworkGameData;

	data.map     = gameSettings->getMap();
	data.tileset = gameSettings->getTileset();
	data.tech    = gameSettings->getTech();

    Config &config = Config::getInstance();
    string scenarioDir = "";
    if(gameSettings->getScenarioDir() != "") {
        scenarioDir = gameSettings->getScenarioDir();
        if(EndsWith(scenarioDir, ".xml") == true) {
            scenarioDir = scenarioDir.erase(scenarioDir.size() - 4, 4);
            scenarioDir = scenarioDir.erase(scenarioDir.size() - gameSettings->getScenario().size(), gameSettings->getScenario().size() + 1);
        }

        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] gameSettings.getScenarioDir() = [%s] gameSettings.getScenario() = [%s] scenarioDir = [%s]\n",__FILE__,__FUNCTION__,__LINE__,gameSettings->getScenarioDir().c_str(),gameSettings->getScenario().c_str(),scenarioDir.c_str());
    }

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    //Checksum checksum;
    //data.tilesetCRC = getFolderTreeContentsCheckSumRecursively(string(GameConstants::folder_path_tilesets) + "/" + gameSettings->getTileset() + "/*", "xml", NULL);
	data.tilesetCRC = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTilesets,scenarioDir), string("/") + gameSettings->getTileset() + string("/*"), ".xml", NULL);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] data.tilesetCRC = %d, [%s]\n",__FILE__,__FUNCTION__,__LINE__, data.tilesetCRC,gameSettings->getTileset().c_str());

    //tech, load before map because of resources
    //data.techCRC  = getFolderTreeContentsCheckSumRecursively(string(GameConstants::folder_path_techs) + "/" + gameSettings->getTech() + "/*", "xml", NULL);
	data.techCRC = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,scenarioDir), string("/") + gameSettings->getTech() + string("/*"), ".xml", NULL);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] data.techCRC = %d, [%s]\n",__FILE__,__FUNCTION__,__LINE__, data.techCRC,gameSettings->getTech().c_str());

    //map
    Checksum checksum;
    string file = Map::getMapPath(gameSettings->getMap(),scenarioDir);
	checksum.addFile(file);
	data.mapCRC = checksum.getSum();
	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] file = [%s] checksum = %d\n",__FILE__,__FUNCTION__,file.c_str(),data.mapCRC);
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] data.mapCRC = %d, [%s]\n",__FILE__,__FUNCTION__,__LINE__, data.mapCRC,gameSettings->getMap().c_str());
}

bool NetworkMessageSynchNetworkGameData::receive(Socket* socket)
{
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void NetworkMessageSynchNetworkGameData::send(Socket* socket) const
{
	assert(data.messageType==nmtSynchNetworkGameData);
	NetworkMessage::send(socket, &data, sizeof(data));
}


// =====================================================
//	class NetworkMessageSynchNetworkGameDataStatus
// =====================================================

NetworkMessageSynchNetworkGameDataStatus::NetworkMessageSynchNetworkGameDataStatus(int32 mapCRC, int32 tilesetCRC, int32 techCRC)
{
	data.messageType= nmtSynchNetworkGameDataStatus;

    data.tilesetCRC     = tilesetCRC;
    data.techCRC        = techCRC;
	data.mapCRC         = mapCRC;
}

bool NetworkMessageSynchNetworkGameDataStatus::receive(Socket* socket)
{
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void NetworkMessageSynchNetworkGameDataStatus::send(Socket* socket) const
{
	assert(data.messageType==nmtSynchNetworkGameDataStatus);
	NetworkMessage::send(socket, &data, sizeof(data));
}

// =====================================================
//	class NetworkMessageSynchNetworkGameDataFileCRCCheck
// =====================================================

NetworkMessageSynchNetworkGameDataFileCRCCheck::NetworkMessageSynchNetworkGameDataFileCRCCheck(int32 totalFileCount, int32 fileIndex, int32 fileCRC, const string fileName)
{
	data.messageType= nmtSynchNetworkGameDataFileCRCCheck;

    data.totalFileCount = totalFileCount;
    data.fileIndex      = fileIndex;
    data.fileCRC        = fileCRC;
    data.fileName       = fileName;
}

bool NetworkMessageSynchNetworkGameDataFileCRCCheck::receive(Socket* socket)
{
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void NetworkMessageSynchNetworkGameDataFileCRCCheck::send(Socket* socket) const
{
	assert(data.messageType==nmtSynchNetworkGameDataFileCRCCheck);
	NetworkMessage::send(socket, &data, sizeof(data));
}

// =====================================================
//	class NetworkMessageSynchNetworkGameDataFileGet
// =====================================================

NetworkMessageSynchNetworkGameDataFileGet::NetworkMessageSynchNetworkGameDataFileGet(const string fileName)
{
	data.messageType= nmtSynchNetworkGameDataFileGet;

    data.fileName       = fileName;
}

bool NetworkMessageSynchNetworkGameDataFileGet::receive(Socket* socket)
{
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void NetworkMessageSynchNetworkGameDataFileGet::send(Socket* socket) const
{
	assert(data.messageType==nmtSynchNetworkGameDataFileGet);
	NetworkMessage::send(socket, &data, sizeof(data));
}



// =====================================================
//	class NetworkMessageSynchNetworkGameDataFileGet
// =====================================================

SwitchSetupRequest::SwitchSetupRequest()
{
	data.messageType= nmtSwitchSetupRequest;
	data.selectedFactionName="";
	data.currentFactionIndex=-1;
	data.toFactionIndex=-1;
    data.toTeam = -1;
}


SwitchSetupRequest::SwitchSetupRequest(string selectedFactionName, int8 currentFactionIndex, int8 toFactionIndex,int8 toTeam)
{
	data.messageType= nmtSwitchSetupRequest;
	data.selectedFactionName=selectedFactionName;
	data.currentFactionIndex=currentFactionIndex;
	data.toFactionIndex=toFactionIndex;
    data.toTeam = toTeam;
}

bool SwitchSetupRequest::receive(Socket* socket)
{
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void SwitchSetupRequest::send(Socket* socket) const
{
	assert(data.messageType==nmtSwitchSetupRequest);
	NetworkMessage::send(socket, &data, sizeof(data));
}


// =====================================================
//	class PlayerIndexMessage
// =====================================================


PlayerIndexMessage::PlayerIndexMessage(int16 playerIndex)
{
	data.messageType= nmtPlayerIndexMessage;
	data.playerIndex=playerIndex;
}

bool PlayerIndexMessage::receive(Socket* socket)
{
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void PlayerIndexMessage::send(Socket* socket) const
{
	assert(data.messageType==nmtPlayerIndexMessage);
	NetworkMessage::send(socket, &data, sizeof(data));
}


}}//end namespace
