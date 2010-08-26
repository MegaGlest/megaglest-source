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
#include <algorithm>

using namespace Shared::Platform;
using namespace Shared::Util;
using namespace std;
using std::min;

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

void NetworkMessage::send(Socket* socket, const void* data, int dataSize) const {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] socket = %p, data = %p, dataSize = %d\n",__FILE__,__FUNCTION__,__LINE__,socket,data,dataSize);

	if(socket != NULL) {
		//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] socket = %p, data = %p, dataSize = %d\n",__FILE__,__FUNCTION__,__LINE__,socket,data,dataSize);
		int sendResult = socket->send(data, dataSize);
		//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] socket = %p, data = %p, dataSize = %d\n",__FILE__,__FUNCTION__,__LINE__,socket,data,dataSize);
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
	data.sessionId=	  -1;
	data.playerIndex= -1;
	data.gameState	= nmgstInvalid;
}

NetworkMessageIntro::NetworkMessageIntro(int32 sessionId,const string &versionString,
										const string &name, int playerIndex,
										NetworkGameStateType gameState) {
	data.messageType	= nmtIntro;
	data.sessionId		= sessionId;
	data.versionString	= versionString;
	data.name			= name;
	data.playerIndex	= static_cast<int16>(playerIndex);
	data.gameState		= static_cast<int8>(gameState);
}

bool NetworkMessageIntro::receive(Socket* socket){
	bool result = NetworkMessage::receive(socket, &data, sizeof(data));
	data.name.nullTerminate();
	data.versionString.nullTerminate();
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] get nmtIntro, data.playerIndex = %d, data.sessionId = %d\n",__FILE__,__FUNCTION__,__LINE__,data.playerIndex,data.sessionId);
	return result;
}

void NetworkMessageIntro::send(Socket* socket) const{
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] sending nmtIntro, data.playerIndex = %d, data.sessionId = %d\n",__FILE__,__FUNCTION__,__LINE__,data.playerIndex,data.sessionId);
	assert(data.messageType==nmtIntro);
	NetworkMessage::send(socket, &data, sizeof(data));
}

// =====================================================
//	class NetworkMessagePing
// =====================================================

NetworkMessagePing::NetworkMessagePing(){
	data.messageType= nmtPing;
	pingReceivedLocalTime = 0;
}

NetworkMessagePing::NetworkMessagePing(int32 pingFrequency, int64 pingTime){
	data.messageType= nmtPing;
	data.pingFrequency= pingFrequency;
	data.pingTime= pingTime;
}

bool NetworkMessagePing::receive(Socket* socket){
	bool result = NetworkMessage::receive(socket, &data, sizeof(data));
	pingReceivedLocalTime = time(NULL);
	return result;
}

void NetworkMessagePing::send(Socket* socket) const{
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtPing\n",__FILE__,__FUNCTION__,__LINE__);
	assert(data.messageType==nmtPing);
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
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtReady\n",__FILE__,__FUNCTION__,__LINE__);
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
	data.networkPauseGameForLaggedClients = gameSettings->getNetworkPauseGameForLaggedClients();
	data.pathFinderType = gameSettings->getPathFinderType();

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
	gameSettings->setNetworkPauseGameForLaggedClients(data.networkPauseGameForLaggedClients);
	gameSettings->setPathFinderType(static_cast<PathFinderType>(data.pathFinderType));

	for(int i= 0; i<data.factionCount; ++i){
		gameSettings->setFactionTypeName(i, data.factionTypeNames[i].getString());
		gameSettings->setNetworkPlayerName(i,data.networkPlayerNames[i].getString());
		gameSettings->setFactionControl(i, static_cast<ControlType>(data.factionControls[i]));
		gameSettings->setTeam(i, data.teams[i]);
		gameSettings->setStartLocationIndex(i, data.startLocationIndex[i]);
	}
}

bool NetworkMessageLaunch::receive(Socket* socket){
	bool result = NetworkMessage::receive(socket, &data, sizeof(data));
	data.description.nullTerminate();
	data.map.nullTerminate();
	data.tileset.nullTerminate();
	data.tech.nullTerminate();
	for(int i= 0; i<GameConstants::maxPlayers; ++i){
		data.factionTypeNames[i].nullTerminate();
		data.networkPlayerNames[i].nullTerminate();
	}

	return result;
}

void NetworkMessageLaunch::send(Socket* socket) const{
	if(data.messageType == nmtLaunch) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtLaunch\n",__FILE__,__FUNCTION__,__LINE__);
	}
	else {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] messageType = %d\n",__FILE__,__FUNCTION__,__LINE__,data.messageType);
	}
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
	for(int peekAttempt = 1; peekAttempt < 15; peekAttempt++) {
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

    // _peek_ type, commandCount & frame num first.
	for(int peekAttempt = 1; peekAttempt < 15; peekAttempt++) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] peekAttempt = %d\n",__FILE__,__FUNCTION__,__LINE__,peekAttempt);

		if (NetworkMessage::peek(socket, &data, totalMsgSize) == true) {
			break;
		}
		else {
			sleep(1); // sleep 1 ms to wait for socket data
		}
	}


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
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtCommandList, frameCount = %d, data.header.commandCount = %d, data.header.messageType = %d\n",__FILE__,__FUNCTION__,__LINE__,data.header.frameCount,data.header.commandCount,data.header.messageType);

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

	if(text.length() >= maxTextStringSize) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] WARNING / ERROR - text [%s] length = %d, max = %d\n",__FILE__,__FUNCTION__,__LINE__,text.c_str(),text.length(),maxTextStringSize);
		//throw runtime_error("NetworkMessageText - text.length() >= maxStringSize");
	}
	if(sender.length() >= maxSenderStringSize) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] WARNING / ERROR - sender [%s] length = %d, max = %d\n",__FILE__,__FUNCTION__,__LINE__,sender.c_str(),sender.length(),maxSenderStringSize);
		//throw runtime_error("NetworkMessageText - sender.length() >= maxSenderStringSize");
	}

	data.messageType= nmtText;
	data.text= text;
	data.sender= sender;
	data.teamIndex= teamIndex;
}

bool NetworkMessageText::receive(Socket* socket){
	bool result = NetworkMessage::receive(socket, &data, sizeof(data));

	data.text.nullTerminate();
	data.sender.nullTerminate();

	return result;
}

void NetworkMessageText::send(Socket* socket) const{
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtText\n",__FILE__,__FUNCTION__,__LINE__);

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
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtQuit\n",__FILE__,__FUNCTION__,__LINE__);

	assert(data.messageType==nmtQuit);
	NetworkMessage::send(socket, &data, sizeof(data));
}

// =====================================================
//	class NetworkMessageSynchNetworkGameData
// =====================================================

NetworkMessageSynchNetworkGameData::NetworkMessageSynchNetworkGameData(const GameSettings *gameSettings)
{
	data.header.messageType= nmtSynchNetworkGameData;

	if(gameSettings == NULL) {
		throw std::runtime_error("gameSettings == NULL");
	}
	data.header.map     = gameSettings->getMap();
	data.header.tileset = gameSettings->getTileset();
	data.header.tech    = gameSettings->getTech();

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

    data.header.tilesetCRC = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTilesets,scenarioDir), string("/") + gameSettings->getTileset() + string("/*"), ".xml", NULL);

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] data.tilesetCRC = %d, [%s]\n",__FILE__,__FUNCTION__,__LINE__, data.header.tilesetCRC,gameSettings->getTileset().c_str());

    //tech, load before map because of resources
	data.header.techCRC = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,scenarioDir), string("/") + gameSettings->getTech() + string("/*"), ".xml", NULL);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] data.techCRC = %d, [%s]\n",__FILE__,__FUNCTION__,__LINE__, data.header.techCRC,gameSettings->getTech().c_str());

	vector<std::pair<string,int32> > vctFileList;
	vctFileList = getFolderTreeContentsCheckSumListRecursively(config.getPathListForType(ptTechs,scenarioDir),string("/") + gameSettings->getTech() + string("/*"), ".xml",&vctFileList);
	data.header.techCRCFileCount = min((int)vctFileList.size(),(int)maxFileCRCCount);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] vctFileList.size() = %d, maxFileCRCCount = %d\n",__FILE__,__FUNCTION__,__LINE__, vctFileList.size(),maxFileCRCCount);

	for(int idx =0; idx < data.header.techCRCFileCount; ++idx) {
		const std::pair<string,int32> &fileInfo = vctFileList[idx];
		data.detail.techCRCFileList[idx] = fileInfo.first;
		data.detail.techCRCFileCRCList[idx] = fileInfo.second;
	}

    //map
    Checksum checksum;
    string file = Map::getMapPath(gameSettings->getMap(),scenarioDir);
	checksum.addFile(file);
	data.header.mapCRC = checksum.getSum();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] data.mapCRC = %d, [%s]\n",__FILE__,__FUNCTION__,__LINE__, data.header.mapCRC,gameSettings->getMap().c_str());
}

string NetworkMessageSynchNetworkGameData::getTechCRCFileMismatchReport(vector<std::pair<string,int32> > &vctFileList) {
	string result = "Techtree: [" + data.header.tech.getString() + "] Filecount local: " + intToStr(vctFileList.size()) + " remote: " + intToStr(data.header.techCRCFileCount) + "\n";
	for(int idx = 0; idx < vctFileList.size(); ++idx) {
		std::pair<string,int32> &fileInfo = vctFileList[idx];
		bool fileFound = false;
		int32 remoteCRC = -1;
		for(int j = 0; j < data.header.techCRCFileCount; ++j) {
			string networkFile = data.detail.techCRCFileList[j].getString();
			int32 &networkFileCRC = data.detail.techCRCFileCRCList[j];
			if(fileInfo.first == networkFile) {
				fileFound = true;
				remoteCRC = networkFileCRC;
				break;
			}
		}

		if(fileFound == false) {
			result = result + "local file [" + fileInfo.first + "] missing remotely.\n";
		}
		else if(fileInfo.second != remoteCRC) {
			result = result + "local file [" + fileInfo.first + "] CRC mismatch.\n";
		}
	}

	for(int i = 0; i < data.header.techCRCFileCount; ++i) {
		string networkFile = data.detail.techCRCFileList[i].getString();
		int32 &networkFileCRC = data.detail.techCRCFileCRCList[i];
		bool fileFound = false;
		int32 localCRC = -1;
		for(int idx = 0; idx < vctFileList.size(); ++idx) {
			std::pair<string,int32> &fileInfo = vctFileList[idx];
			if(networkFile == fileInfo.first) {
				fileFound = true;
				localCRC = fileInfo.second;
				break;
			}
		}

		if(fileFound == false) {
			result = result + "remote file [" + networkFile + "] missing locally.\n";
		}
		else if(networkFileCRC != localCRC) {
			result = result + "remote file [" + networkFile + "] CRC mismatch.\n";
		}
	}

	return result;
}

bool NetworkMessageSynchNetworkGameData::receive(Socket* socket) {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] about to get nmtSynchNetworkGameData\n",__FILE__,__FUNCTION__,__LINE__);

	data.header.techCRCFileCount = 0;

	for(int peekAttempt = 1; peekAttempt < 5000; peekAttempt++) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] peekAttempt = %d\n",__FILE__,__FUNCTION__,__LINE__,peekAttempt);

		if (NetworkMessage::peek(socket, &data, HeaderSize) == true) {
			break;
		}
		else {
			sleep(1); // sleep 1 ms to wait for socket data
		}
	}

	if (NetworkMessage::peek(socket, &data, HeaderSize) == false) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ERROR / WARNING!!! NetworkMessage::peek failed!\n",__FILE__,__FUNCTION__,__LINE__);
		return false;
	}

	data.header.map.nullTerminate();
	data.header.tileset.nullTerminate();
	data.header.tech.nullTerminate();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] messageType = %d, data.techCRCFileCount = %d\n",__FILE__,__FUNCTION__,__LINE__,data.header.messageType,data.header.techCRCFileCount);

	bool result = NetworkMessage::receive(socket, &data, HeaderSize);
	if(result == true && data.header.techCRCFileCount > 0) {

		// Here we loop possibly multiple times
		int packetLoopCount = 1;
		if(data.header.techCRCFileCount > NetworkMessageSynchNetworkGameData::maxFileCRCPacketCount) {
			packetLoopCount = (data.header.techCRCFileCount / NetworkMessageSynchNetworkGameData::maxFileCRCPacketCount);
			if(data.header.techCRCFileCount % NetworkMessageSynchNetworkGameData::maxFileCRCPacketCount > 0) {
				packetLoopCount++;
			}
		}

		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] packetLoopCount = %d\n",__FILE__,__FUNCTION__,__LINE__,packetLoopCount);

		for(int iPacketLoop = 0; iPacketLoop < packetLoopCount; ++iPacketLoop) {

			int packetIndex = iPacketLoop * NetworkMessageSynchNetworkGameData::maxFileCRCPacketCount;
			int maxFileCountPerPacket = maxFileCRCPacketCount;
			int packetFileCount = min(maxFileCountPerPacket,data.header.techCRCFileCount - packetIndex);

			for(int peekAttempt = 1; peekAttempt < 5000; peekAttempt++) {
				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] peekAttempt = %d\n",__FILE__,__FUNCTION__,__LINE__,peekAttempt);

				if (NetworkMessage::peek(socket, &data.detail.techCRCFileList[packetIndex], (DetailSize1 * packetFileCount)) == true) {
					break;
				}
				else {
					sleep(1); // sleep 1 ms to wait for socket data
				}
			}

			result = NetworkMessage::receive(socket, &data.detail.techCRCFileList[packetIndex], (DetailSize1 * packetFileCount));
			if(result == true) {
				for(int i = 0; i < data.header.techCRCFileCount; ++i) {
					data.detail.techCRCFileList[i].nullTerminate();
					//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] data.detail.techCRCFileList[i] = [%s]\n",__FILE__,__FUNCTION__,__LINE__,data.detail.techCRCFileList[i].getString().c_str());
				}

				for(int peekAttempt = 1; peekAttempt < 5000; peekAttempt++) {
					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] peekAttempt = %d\n",__FILE__,__FUNCTION__,__LINE__,peekAttempt);
					if (NetworkMessage::peek(socket, &data.detail.techCRCFileCRCList[packetIndex], (DetailSize2 * packetFileCount)) == true) {
						break;
					}
					else {
						sleep(1); // sleep 1 ms to wait for socket data
					}
				}

				result = NetworkMessage::receive(socket, &data.detail.techCRCFileCRCList[packetIndex], (DetailSize2 * packetFileCount));
			}
		}
	}

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] result = %d\n",__FILE__,__FUNCTION__,__LINE__,result);
	return result;
}

void NetworkMessageSynchNetworkGameData::send(Socket* socket) const {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] about to send nmtSynchNetworkGameData\n",__FILE__,__FUNCTION__,__LINE__);

	assert(data.header.messageType==nmtSynchNetworkGameData);
	NetworkMessage::send(socket, &data, HeaderSize);
	if(data.header.techCRCFileCount > 0) {
		// Here we loop possibly multiple times
		int packetLoopCount = 1;
		if(data.header.techCRCFileCount > NetworkMessageSynchNetworkGameData::maxFileCRCPacketCount) {
			packetLoopCount = (data.header.techCRCFileCount / NetworkMessageSynchNetworkGameData::maxFileCRCPacketCount);
			if(data.header.techCRCFileCount % NetworkMessageSynchNetworkGameData::maxFileCRCPacketCount > 0) {
				packetLoopCount++;
			}
		}

		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] packetLoopCount = %d\n",__FILE__,__FUNCTION__,__LINE__,packetLoopCount);

		for(int iPacketLoop = 0; iPacketLoop < packetLoopCount; ++iPacketLoop) {

			int packetIndex = iPacketLoop * NetworkMessageSynchNetworkGameData::maxFileCRCPacketCount;
			int maxFileCountPerPacket = maxFileCRCPacketCount;
			int packetFileCount = min(maxFileCountPerPacket,data.header.techCRCFileCount - packetIndex);

			NetworkMessage::send(socket, &data.detail.techCRCFileList[packetIndex], (DetailSize1 * packetFileCount));
			NetworkMessage::send(socket, &data.detail.techCRCFileCRCList[packetIndex], (DetailSize2 * packetFileCount));
		}
	}
}

// =====================================================
//	class NetworkMessageSynchNetworkGameDataStatus
// =====================================================

NetworkMessageSynchNetworkGameDataStatus::NetworkMessageSynchNetworkGameDataStatus(int32 mapCRC, int32 tilesetCRC, int32 techCRC, vector<std::pair<string,int32> > &vctFileList)
{
	data.header.messageType= nmtSynchNetworkGameDataStatus;

    data.header.tilesetCRC     = tilesetCRC;
    data.header.techCRC        = techCRC;
	data.header.mapCRC         = mapCRC;

	data.header.techCRCFileCount = min((int)vctFileList.size(),(int)maxFileCRCCount);
	for(int idx =0; idx < data.header.techCRCFileCount; ++idx) {
		const std::pair<string,int32> &fileInfo = vctFileList[idx];
		data.detail.techCRCFileList[idx] = fileInfo.first;
		data.detail.techCRCFileCRCList[idx] = fileInfo.second;
	}
}

string NetworkMessageSynchNetworkGameDataStatus::getTechCRCFileMismatchReport(string techtree, vector<std::pair<string,int32> > &vctFileList) {
	string result = "Techtree: [" + techtree + "] Filecount local: " + intToStr(vctFileList.size()) + " remote: " + intToStr(data.header.techCRCFileCount) + "\n";
	for(int idx = 0; idx < vctFileList.size(); ++idx) {
		std::pair<string,int32> &fileInfo = vctFileList[idx];
		bool fileFound = false;
		int32 remoteCRC = -1;
		for(int j = 0; j < data.header.techCRCFileCount; ++j) {
			string networkFile = data.detail.techCRCFileList[j].getString();
			int32 &networkFileCRC = data.detail.techCRCFileCRCList[j];
			if(fileInfo.first == networkFile) {
				fileFound = true;
				remoteCRC = networkFileCRC;
				break;
			}
		}

		if(fileFound == false) {
			result = result + "local file [" + fileInfo.first + "] missing remotely.\n";
		}
		else if(fileInfo.second != remoteCRC) {
			result = result + "local file [" + fileInfo.first + "] CRC mismatch.\n";
		}
	}

	for(int i = 0; i < data.header.techCRCFileCount; ++i) {
		string networkFile = data.detail.techCRCFileList[i].getString();
		int32 &networkFileCRC = data.detail.techCRCFileCRCList[i];
		bool fileFound = false;
		int32 localCRC = -1;
		for(int idx = 0; idx < vctFileList.size(); ++idx) {
			std::pair<string,int32> &fileInfo = vctFileList[idx];

			if(networkFile == fileInfo.first) {
				fileFound = true;
				localCRC = fileInfo.second;
				break;
			}
		}

		if(fileFound == false) {
			result = result + "remote file [" + networkFile + "] missing locally.\n";
		}
		else if(networkFileCRC != localCRC) {
			result = result + "remote file [" + networkFile + "] CRC mismatch.\n";
		}
	}

	return result;
}

bool NetworkMessageSynchNetworkGameDataStatus::receive(Socket* socket) {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] about to get nmtSynchNetworkGameDataStatus\n",__FILE__,__FUNCTION__,__LINE__);
	data.header.techCRCFileCount = 0;

	for(int peekAttempt = 1; peekAttempt < 5000; peekAttempt++) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] peekAttempt = %d\n",__FILE__,__FUNCTION__,__LINE__,peekAttempt);
		if (NetworkMessage::peek(socket, &data, HeaderSize) == true) {
			break;
		}
		else {
			sleep(1); // sleep 1 ms to wait for socket data
		}
	}

	if (NetworkMessage::peek(socket, &data, HeaderSize) == false) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ERROR / WARNING!!! NetworkMessage::peek failed!\n",__FILE__,__FUNCTION__,__LINE__);
		return false;
	}

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] messageType = %d, data.techCRCFileCount = %d\n",__FILE__,__FUNCTION__,__LINE__,data.header.messageType,data.header.techCRCFileCount);

	bool result = NetworkMessage::receive(socket, &data, HeaderSize);
	if(result == true && data.header.techCRCFileCount > 0) {
		// Here we loop possibly multiple times
		int packetLoopCount = 1;
		if(data.header.techCRCFileCount > NetworkMessageSynchNetworkGameDataStatus::maxFileCRCPacketCount) {
			packetLoopCount = (data.header.techCRCFileCount / NetworkMessageSynchNetworkGameDataStatus::maxFileCRCPacketCount);
			if(data.header.techCRCFileCount % NetworkMessageSynchNetworkGameDataStatus::maxFileCRCPacketCount > 0) {
				packetLoopCount++;
			}
		}

		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] packetLoopCount = %d\n",__FILE__,__FUNCTION__,__LINE__,packetLoopCount);

		for(int iPacketLoop = 0; iPacketLoop < packetLoopCount; ++iPacketLoop) {

			int packetIndex = iPacketLoop * NetworkMessageSynchNetworkGameDataStatus::maxFileCRCPacketCount;
			int maxFileCountPerPacket = maxFileCRCPacketCount;
			int packetFileCount = min(maxFileCountPerPacket,data.header.techCRCFileCount - packetIndex);

			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] iPacketLoop = %d, packetIndex = %d, packetFileCount = %d\n",__FILE__,__FUNCTION__,__LINE__,iPacketLoop,packetIndex,packetFileCount);

			for(int peekAttempt = 1; peekAttempt < 5000; peekAttempt++) {
				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] peekAttempt = %d\n",__FILE__,__FUNCTION__,__LINE__,peekAttempt);

				if (NetworkMessage::peek(socket, &data.detail.techCRCFileList[packetIndex], (DetailSize1 * packetFileCount)) == true) {
					break;
				}
				else {
					sleep(1); // sleep 1 ms to wait for socket data
				}
			}

			result = NetworkMessage::receive(socket, &data.detail.techCRCFileList[packetIndex], (DetailSize1 * packetFileCount));
			if(result == true) {
				for(int i = 0; i < data.header.techCRCFileCount; ++i) {
					data.detail.techCRCFileList[i].nullTerminate();
					//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] data.detail.techCRCFileList[i] = [%s]\n",__FILE__,__FUNCTION__,__LINE__,data.detail.techCRCFileList[i].getString().c_str());
				}

				for(int peekAttempt = 1; peekAttempt < 5000; peekAttempt++) {
					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] peekAttempt = %d\n",__FILE__,__FUNCTION__,__LINE__,peekAttempt);
					if (NetworkMessage::peek(socket, &data.detail.techCRCFileCRCList[packetIndex], (DetailSize2 * packetFileCount)) == true) {
						break;
					}
					else {
						sleep(1); // sleep 1 ms to wait for socket data
					}
				}

				result = NetworkMessage::receive(socket, &data.detail.techCRCFileCRCList[packetIndex], (DetailSize2 * packetFileCount));
			}
		}
	}

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] result = %d\n",__FILE__,__FUNCTION__,__LINE__,result);

	return result;
}

void NetworkMessageSynchNetworkGameDataStatus::send(Socket* socket) const {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] about to send nmtSynchNetworkGameDataStatus, data.header.techCRCFileCount = %d\n",__FILE__,__FUNCTION__,__LINE__,data.header.techCRCFileCount);

	assert(data.header.messageType==nmtSynchNetworkGameDataStatus);
	//int totalMsgSize = HeaderSize + (sizeof(DataDetail) * data.header.techCRCFileCount);
	NetworkMessage::send(socket, &data, HeaderSize);
	if(data.header.techCRCFileCount > 0) {
		// Here we loop possibly multiple times
		int packetLoopCount = 1;
		if(data.header.techCRCFileCount > NetworkMessageSynchNetworkGameDataStatus::maxFileCRCPacketCount) {
			packetLoopCount = (data.header.techCRCFileCount / NetworkMessageSynchNetworkGameDataStatus::maxFileCRCPacketCount);
			if(data.header.techCRCFileCount % NetworkMessageSynchNetworkGameDataStatus::maxFileCRCPacketCount > 0) {
				packetLoopCount++;
			}
		}

		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] packetLoopCount = %d\n",__FILE__,__FUNCTION__,__LINE__,packetLoopCount);

		for(int iPacketLoop = 0; iPacketLoop < packetLoopCount; ++iPacketLoop) {

			int packetIndex = iPacketLoop * NetworkMessageSynchNetworkGameDataStatus::maxFileCRCPacketCount;
			int maxFileCountPerPacket = maxFileCRCPacketCount;
			int packetFileCount = min(maxFileCountPerPacket,data.header.techCRCFileCount - packetIndex);

			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] packetLoop = %d, packetIndex = %d, packetFileCount = %d\n",__FILE__,__FUNCTION__,__LINE__,iPacketLoop,packetIndex,packetFileCount);

			NetworkMessage::send(socket, &data.detail.techCRCFileList[packetIndex], (DetailSize1 * packetFileCount));
			NetworkMessage::send(socket, &data.detail.techCRCFileCRCList[packetIndex], (DetailSize2 * packetFileCount));
		}
	}
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

bool NetworkMessageSynchNetworkGameDataFileCRCCheck::receive(Socket* socket) {
	bool result = NetworkMessage::receive(socket, &data, sizeof(data));

	data.fileName.nullTerminate();

	return result;
}

void NetworkMessageSynchNetworkGameDataFileCRCCheck::send(Socket* socket) const {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtSynchNetworkGameDataFileCRCCheck\n",__FILE__,__FUNCTION__,__LINE__);

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

bool NetworkMessageSynchNetworkGameDataFileGet::receive(Socket* socket) {
	bool result = NetworkMessage::receive(socket, &data, sizeof(data));

	data.fileName.nullTerminate();

	return result;
}

void NetworkMessageSynchNetworkGameDataFileGet::send(Socket* socket) const {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtSynchNetworkGameDataFileGet\n",__FILE__,__FUNCTION__,__LINE__);

	assert(data.messageType==nmtSynchNetworkGameDataFileGet);
	NetworkMessage::send(socket, &data, sizeof(data));
}



// =====================================================
//	class SwitchSetupRequest
// =====================================================

SwitchSetupRequest::SwitchSetupRequest() {
	data.messageType= nmtSwitchSetupRequest;
	data.selectedFactionName="";
	data.currentFactionIndex=-1;
	data.toFactionIndex=-1;
    data.toTeam = -1;
    data.networkPlayerName="";
    data.switchFlags = ssrft_None;
}

SwitchSetupRequest::SwitchSetupRequest(string selectedFactionName, int8 currentFactionIndex,
										int8 toFactionIndex,int8 toTeam,string networkPlayerName,
										int8 flags) {
	data.messageType= nmtSwitchSetupRequest;
	data.selectedFactionName=selectedFactionName;
	data.currentFactionIndex=currentFactionIndex;
	data.toFactionIndex=toFactionIndex;
    data.toTeam = toTeam;
    data.networkPlayerName=networkPlayerName;
    data.switchFlags = flags;
}

bool SwitchSetupRequest::receive(Socket* socket) {
	bool result = NetworkMessage::receive(socket, &data, sizeof(data));

	data.selectedFactionName.nullTerminate();
	data.networkPlayerName.nullTerminate();

	return result;
}

void SwitchSetupRequest::send(Socket* socket) const {
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
