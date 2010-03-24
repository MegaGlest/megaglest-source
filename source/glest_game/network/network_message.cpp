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
	return false;
}

bool NetworkMessage::receive(Socket* socket, void* data, int dataSize)
{
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
	return false;
}

void NetworkMessage::send(Socket* socket, const void* data, int dataSize) const
{
	if(socket->send(data, dataSize)!=dataSize)
	{
	    if(socket != NULL && socket->getSocketId() > 0)
	    {
            throw runtime_error("Error sending NetworkMessage");
	    }
	    else
	    {
	        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] socket has been disconnected\n",__FILE__,__FUNCTION__);
	    }
	}
}

// =====================================================
//	class NetworkMessageIntro
// =====================================================

NetworkMessageIntro::NetworkMessageIntro(){
	data.messageType= -1;
	data.playerIndex= -1;
}

NetworkMessageIntro::NetworkMessageIntro(const string &versionString, const string &name, int playerIndex){
	data.messageType=nmtIntro;
	data.versionString= versionString;
	data.name= name;
	data.playerIndex= static_cast<int16>(playerIndex);
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

NetworkMessageLaunch::NetworkMessageLaunch(const GameSettings *gameSettings){
	data.messageType=nmtLaunch;

	data.description= gameSettings->getDescription();
	data.map= gameSettings->getMap();
	data.tileset= gameSettings->getTileset();
	data.tech= gameSettings->getTech();
	data.factionCount= gameSettings->getFactionCount();
	data.thisFactionIndex= gameSettings->getThisFactionIndex();
	data.defaultResources= gameSettings->getDefaultResources();
    data.defaultUnits= gameSettings->getDefaultUnits();
    data.defaultVictoryConditions= gameSettings->getDefaultVictoryConditions();

	for(int i= 0; i<data.factionCount; ++i){
		data.factionTypeNames[i]= gameSettings->getFactionTypeName(i);
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

	for(int i= 0; i<data.factionCount; ++i){
		gameSettings->setFactionTypeName(i, data.factionTypeNames[i].getString());
		gameSettings->setFactionControl(i, static_cast<ControlType>(data.factionControls[i]));
		gameSettings->setTeam(i, data.teams[i]);
		gameSettings->setStartLocationIndex(i, data.startLocationIndex[i]);
	}
}

bool NetworkMessageLaunch::receive(Socket* socket){
	return NetworkMessage::receive(socket, &data, sizeof(data));
}

void NetworkMessageLaunch::send(Socket* socket) const{
	assert(data.messageType==nmtLaunch);
	NetworkMessage::send(socket, &data, sizeof(data));
}

// =====================================================
//	class NetworkMessageLaunch
// =====================================================

NetworkMessageCommandList::NetworkMessageCommandList(int32 frameCount){
	data.messageType= nmtCommandList;
	data.frameCount= frameCount;
	data.commandCount= 0;
}

bool NetworkMessageCommandList::addCommand(const NetworkCommand* networkCommand){
	if(data.commandCount<maxCommandCount){
		data.commands[static_cast<int>(data.commandCount)]= *networkCommand;
		data.commandCount++;
		return true;
	}
	return false;
}

bool NetworkMessageCommandList::receive(Socket* socket){
    // _peek_ type, commandCount & frame num first.
	if (!NetworkMessage::peek(socket, &data, commandListHeaderSize)) {
	    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s %d] NetworkMessage::peek failed!\n",__FILE__,__FUNCTION__,__LINE__);
		return false;
	}

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s %d] messageType = %d, frameCount = %d, data.commandCount = %d\n",
        __FILE__,__FUNCTION__,__LINE__,data.messageType,data.frameCount,data.commandCount);

	// read header + data.commandCount commands.
	int totalMsgSize = commandListHeaderSize + sizeof(NetworkCommand) * data.commandCount;
	if (socket->getDataToRead() < totalMsgSize) {
	    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s %d] Insufficient data to read entire command list [need %d bytes, only %d available].\n",
			__FILE__,__FUNCTION__,__LINE__, totalMsgSize, socket->getDataToRead());
		return false;
	}
	bool result = NetworkMessage::receive(socket, &data, totalMsgSize);
	if(SystemFlags::enableNetworkDebugInfo) {
        for(int idx = 0 ; idx < data.commandCount; ++idx) {
            const NetworkCommand &cmd = data.commands[idx];

            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s %d] index = %d, networkCommandType = %d, unitId = %d, commandTypeId = %d, positionX = %d, positionY = %d, unitTypeId = %d, targetId = %d\n",
                    __FILE__,__FUNCTION__,__LINE__,idx, cmd.getNetworkCommandType(),cmd.getUnitId(), cmd.getCommandTypeId(),
                    cmd.getPosition().x,cmd.getPosition().y, cmd.getUnitTypeId(), cmd.getTargetId());
        }
	}
	return result;
}

void NetworkMessageCommandList::send(Socket* socket) const{
	assert(data.messageType==nmtCommandList);
	NetworkMessage::send(socket, &data, commandListHeaderSize + sizeof(NetworkCommand) * data.commandCount);

	if(SystemFlags::enableNetworkDebugInfo) {
	    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s %d] messageType = %d, frameCount = %d, data.commandCount = %d\n",
                __FILE__,__FUNCTION__,__LINE__,data.messageType,data.frameCount,data.commandCount);

        if (data.commandCount) {
            for(int idx = 0 ; idx < data.commandCount; ++idx) {
                const NetworkCommand &cmd = data.commands[idx];

                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s %d] index = %d, networkCommandType = %d, unitId = %d, commandTypeId = %d, positionX = %d, positionY = %d, unitTypeId = %d, targetId = %d\n",
                        __FILE__,__FUNCTION__,__LINE__,idx, cmd.getNetworkCommandType(),cmd.getUnitId(), cmd.getCommandTypeId(),
                        cmd.getPosition().x,cmd.getPosition().y, cmd.getUnitTypeId(), cmd.getTargetId());
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

    //Checksum checksum;
    data.tilesetCRC = getFolderTreeContentsCheckSumRecursively(string(GameConstants::folder_path_tilesets) + "/" + gameSettings->getTileset() + "/*", "xml", NULL);

    //tech, load before map because of resources
    data.techCRC  = getFolderTreeContentsCheckSumRecursively(string(GameConstants::folder_path_techs) + "/" + gameSettings->getTech() + "/*", "xml", NULL);

    //map
    Checksum checksum;
    string file = Map::getMapPath(gameSettings->getMap());
	checksum.addFile(file);
	data.mapCRC = checksum.getSum();
	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] file = [%s] checksum = %d\n",__FILE__,__FUNCTION__,file.c_str(),data.mapCRC);

	data.hasFogOfWar = Config::getInstance().getBool("FogOfWar");;
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

NetworkMessageSynchNetworkGameDataStatus::NetworkMessageSynchNetworkGameDataStatus(int32 mapCRC, int32 tilesetCRC, int32 techCRC, int8 hasFogOfWar)
{
	data.messageType= nmtSynchNetworkGameDataStatus;

    data.tilesetCRC     = tilesetCRC;
    data.techCRC        = techCRC;
	data.mapCRC         = mapCRC;

	data.hasFogOfWar    = hasFogOfWar;
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


}}//end namespace
