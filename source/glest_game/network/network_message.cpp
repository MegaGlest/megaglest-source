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

#include "data_types.h"
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

bool NetworkMessage::receive(Socket* socket, void* data, int dataSize, bool tryReceiveUntilDataSizeMet) {
	if(socket != NULL) {
		int dataReceived = socket->receive(data, dataSize, tryReceiveUntilDataSizeMet);
		if(dataReceived != dataSize) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] WARNING, dataReceived = %d dataSize = %d\n",__FILE__,__FUNCTION__,__LINE__,dataReceived,dataSize);
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\nIn [%s::%s Line: %d] WARNING, dataReceived = %d dataSize = %d\n",__FILE__,__FUNCTION__,__LINE__,dataReceived,dataSize);

			if(socket != NULL && socket->getSocketId() > 0) {
				throw megaglest_runtime_error("Error receiving NetworkMessage, dataReceived = " + intToStr(dataReceived) + ", dataSize = " + intToStr(dataSize));
			}
			else {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] socket has been disconnected\n",__FILE__,__FUNCTION__,__LINE__);
			}
		}
		else {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] dataSize = %d, dataReceived = %d\n",__FILE__,__FUNCTION__,__LINE__,dataSize,dataReceived);
			return true;
		}
	}
	return false;

}

void NetworkMessage::send(Socket* socket, const void* data, int dataSize) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] socket = %p, data = %p, dataSize = %d\n",__FILE__,__FUNCTION__,__LINE__,socket,data,dataSize);

	if(socket != NULL) {
		int sendResult = socket->send(data, dataSize);
		if(sendResult != dataSize) {
			if(socket != NULL && socket->getSocketId() > 0) {
				char szBuf[8096]="";
				snprintf(szBuf,8096,"Error sending NetworkMessage, sendResult = %d, dataSize = %d",sendResult,dataSize);
				throw megaglest_runtime_error(szBuf);
			}
			else {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d socket has been disconnected\n",__FILE__,__FUNCTION__,__LINE__);
			}
		}
	}
}

// =====================================================
//	class NetworkMessageIntro
// =====================================================

NetworkMessageIntro::NetworkMessageIntro() {
	data.messageType= -1;
	data.sessionId=	  -1;
	data.playerIndex= -1;
	data.gameState	= nmgstInvalid;
	data.externalIp = 0;
}

NetworkMessageIntro::NetworkMessageIntro(int32 sessionId,const string &versionString,
										const string &name, int playerIndex,
										NetworkGameStateType gameState,
										uint32 externalIp,
										uint32 ftpPort,
										const string &playerLanguage) {
	data.messageType	= nmtIntro;
	data.sessionId		= sessionId;
	data.versionString	= versionString;
	data.name			= name;
	data.playerIndex	= static_cast<int16>(playerIndex);
	data.gameState		= static_cast<int8>(gameState);
	data.externalIp     = externalIp;
	data.ftpPort		= ftpPort;
	data.language		= playerLanguage;
}

bool NetworkMessageIntro::receive(Socket* socket) {
	bool result = NetworkMessage::receive(socket, &data, sizeof(data), true);
	fromEndian();
	data.name.nullTerminate();
	data.versionString.nullTerminate();
	data.language.nullTerminate();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] get nmtIntro, data.playerIndex = %d, data.sessionId = %d\n",__FILE__,__FUNCTION__,__LINE__,data.playerIndex,data.sessionId);
	return result;
}

void NetworkMessageIntro::send(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] sending nmtIntro, data.playerIndex = %d, data.sessionId = %d\n",__FILE__,__FUNCTION__,__LINE__,data.playerIndex,data.sessionId);
	assert(data.messageType == nmtIntro);
	toEndian();
	NetworkMessage::send(socket, &data, sizeof(data));
}

void NetworkMessageIntro::toEndian() {
	data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
	data.sessionId = Shared::PlatformByteOrder::toCommonEndian(data.sessionId);
	data.playerIndex = Shared::PlatformByteOrder::toCommonEndian(data.playerIndex);
	data.gameState = Shared::PlatformByteOrder::toCommonEndian(data.gameState);
	data.externalIp = Shared::PlatformByteOrder::toCommonEndian(data.externalIp);
	data.ftpPort = Shared::PlatformByteOrder::toCommonEndian(data.ftpPort);
}
void NetworkMessageIntro::fromEndian() {
	data.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.messageType);
	data.sessionId = Shared::PlatformByteOrder::fromCommonEndian(data.sessionId);
	data.playerIndex = Shared::PlatformByteOrder::fromCommonEndian(data.playerIndex);
	data.gameState = Shared::PlatformByteOrder::fromCommonEndian(data.gameState);
	data.externalIp = Shared::PlatformByteOrder::fromCommonEndian(data.externalIp);
	data.ftpPort = Shared::PlatformByteOrder::fromCommonEndian(data.ftpPort);
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
	pingReceivedLocalTime=0;
}

bool NetworkMessagePing::receive(Socket* socket){
	bool result = NetworkMessage::receive(socket, &data, sizeof(data), true);
	fromEndian();
	pingReceivedLocalTime = time(NULL);
	return result;
}

void NetworkMessagePing::send(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtPing\n",__FILE__,__FUNCTION__,__LINE__);
	assert(data.messageType==nmtPing);
	toEndian();
	NetworkMessage::send(socket, &data, sizeof(data));
}

void NetworkMessagePing::toEndian() {
	data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
	data.pingFrequency = Shared::PlatformByteOrder::toCommonEndian(data.pingFrequency);
	data.pingTime = Shared::PlatformByteOrder::toCommonEndian(data.pingTime);
}
void NetworkMessagePing::fromEndian() {
	data.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.messageType);
	data.pingFrequency = Shared::PlatformByteOrder::fromCommonEndian(data.pingFrequency);
	data.pingTime = Shared::PlatformByteOrder::fromCommonEndian(data.pingTime);
}

// =====================================================
//	class NetworkMessageReady
// =====================================================

NetworkMessageReady::NetworkMessageReady() {
	data.messageType= nmtReady;
}

NetworkMessageReady::NetworkMessageReady(uint32 checksum) {
	data.messageType= nmtReady;
	data.checksum= checksum;
}

bool NetworkMessageReady::receive(Socket* socket){
	bool result = NetworkMessage::receive(socket, &data, sizeof(data), true);
	fromEndian();
	return result;
}

void NetworkMessageReady::send(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtReady\n",__FILE__,__FUNCTION__,__LINE__);
	assert(data.messageType==nmtReady);
	toEndian();
	NetworkMessage::send(socket, &data, sizeof(data));
}

void NetworkMessageReady::toEndian() {
	data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
	data.checksum = Shared::PlatformByteOrder::toCommonEndian(data.checksum);
}
void NetworkMessageReady::fromEndian() {
	data.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.messageType);
	data.checksum = Shared::PlatformByteOrder::fromCommonEndian(data.checksum);
}

// =====================================================
//	class NetworkMessageLaunch
// =====================================================

NetworkMessageLaunch::NetworkMessageLaunch() {
	data.messageType=-1;
	for(unsigned int i = 0; i < maxFactionCRCCount; ++i) {
		data.factionNameList[i] = "";
		data.factionCRCList[i] = 0;
	}
	data.aiAcceptSwitchTeamPercentChance = 0;
	data.cpuReplacementMultiplier = 10 ;
	data.masterserver_admin = -1;
	data.masterserver_admin_factionIndex = -1;
}

NetworkMessageLaunch::NetworkMessageLaunch(const GameSettings *gameSettings,int8 messageType) {
	data.messageType=messageType;

    data.mapCRC     = gameSettings->getMapCRC();
    data.tilesetCRC = gameSettings->getTilesetCRC();
    data.techCRC    = gameSettings->getTechCRC();

	for(unsigned int i = 0; i < maxFactionCRCCount; ++i) {
		data.factionNameList[i] = "";
		data.factionCRCList[i] = 0;
	}

	vector<pair<string,uint32> > factionCRCList = gameSettings->getFactionCRCList();
	for(unsigned int i = 0; i < factionCRCList.size() && i < maxFactionCRCCount; ++i) {
		data.factionNameList[i] = factionCRCList[i].first;
		data.factionCRCList[i] = factionCRCList[i].second;
	}

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
	data.allowObservers = gameSettings->getAllowObservers();
	data.enableObserverModeAtEndGame = gameSettings->getEnableObserverModeAtEndGame();
	data.enableServerControlledAI = gameSettings->getEnableServerControlledAI();
	data.networkFramePeriod = gameSettings->getNetworkFramePeriod();
	data.networkPauseGameForLaggedClients = gameSettings->getNetworkPauseGameForLaggedClients();
	data.pathFinderType = gameSettings->getPathFinderType();
	data.flagTypes1 = gameSettings->getFlagTypes1();

	//for(int i= 0; i < data.factionCount; ++i) {
	for(int i= 0; i < GameConstants::maxPlayers; ++i) {
		data.factionTypeNames[i]= gameSettings->getFactionTypeName(i);
		data.networkPlayerNames[i]= gameSettings->getNetworkPlayerName(i);
		data.networkPlayerStatuses[i] = gameSettings->getNetworkPlayerStatuses(i);
		data.networkPlayerLanguages[i] = gameSettings->getNetworkPlayerLanguages(i);
		data.factionControls[i]= gameSettings->getFactionControl(i);
		data.resourceMultiplierIndex[i]= gameSettings->getResourceMultiplierIndex(i);
		data.teams[i]= gameSettings->getTeam(i);
		data.startLocationIndex[i]= gameSettings->getStartLocationIndex(i);
	}
//	for(int i= data.factionCount; i < GameConstants::maxPlayers; ++i) {
//		data.factionTypeNames[i]= "";
//		data.networkPlayerNames[i]= "";
//		data.networkPlayerStatuses[i] = 0;
//		data.networkPlayerLanguages[i] = "";
//		data.factionControls[i]= 0;
//		data.resourceMultiplierIndex[i]= 0;
//		data.teams[i]= -1;
//		data.startLocationIndex[i]= 0;
//	}
	data.cpuReplacementMultiplier = gameSettings->getFallbackCpuMultiplier();
	data.aiAcceptSwitchTeamPercentChance = gameSettings->getAiAcceptSwitchTeamPercentChance();
	data.masterserver_admin = gameSettings->getMasterserver_admin();
	data.masterserver_admin_factionIndex = gameSettings->getMasterserver_admin_faction_index();

	data.scenario = gameSettings->getScenario();
}

void NetworkMessageLaunch::buildGameSettings(GameSettings *gameSettings) const {
	gameSettings->setDescription(data.description.getString());
	gameSettings->setMap(data.map.getString());
	gameSettings->setTileset(data.tileset.getString());
	gameSettings->setTech(data.tech.getString());
	gameSettings->setFactionCount(data.factionCount);
	gameSettings->setThisFactionIndex(data.thisFactionIndex);
	gameSettings->setDefaultResources((data.defaultResources != 0));
    gameSettings->setDefaultUnits((data.defaultUnits != 0));
    gameSettings->setDefaultVictoryConditions((data.defaultVictoryConditions != 0));
	gameSettings->setFogOfWar((data.fogOfWar != 0));
	gameSettings->setAllowObservers((data.allowObservers != 0));

	gameSettings->setEnableObserverModeAtEndGame((data.enableObserverModeAtEndGame != 0));
	gameSettings->setEnableServerControlledAI((data.enableServerControlledAI != 0));
	gameSettings->setNetworkFramePeriod(data.networkFramePeriod);
	gameSettings->setNetworkPauseGameForLaggedClients((data.networkPauseGameForLaggedClients != 0));
	gameSettings->setPathFinderType(static_cast<PathFinderType>(data.pathFinderType));
	gameSettings->setFlagTypes1(data.flagTypes1);

    gameSettings->setMapCRC(data.mapCRC);
    gameSettings->setTilesetCRC(data.tilesetCRC);
    gameSettings->setTechCRC(data.techCRC);

	vector<pair<string,uint32> > factionCRCList;
	for(unsigned int i = 0; i < maxFactionCRCCount; ++i) {
		if(data.factionNameList[i].getString() != "") {
			factionCRCList.push_back(make_pair(data.factionNameList[i].getString(),data.factionCRCList[i]));
		}
	}
	gameSettings->setFactionCRCList(factionCRCList);

	//for(int i= 0; i < data.factionCount; ++i) {
	for(int i= 0; i < GameConstants::maxPlayers; ++i) {
		gameSettings->setFactionTypeName(i, data.factionTypeNames[i].getString());
		gameSettings->setNetworkPlayerName(i,data.networkPlayerNames[i].getString());
		gameSettings->setNetworkPlayerStatuses(i, data.networkPlayerStatuses[i]);
		gameSettings->setNetworkPlayerLanguages(i, data.networkPlayerLanguages[i].getString());
		gameSettings->setFactionControl(i, static_cast<ControlType>(data.factionControls[i]));
		gameSettings->setResourceMultiplierIndex(i,data.resourceMultiplierIndex[i]);
		gameSettings->setTeam(i, data.teams[i]);
		gameSettings->setStartLocationIndex(i, data.startLocationIndex[i]);
	}

	gameSettings->setAiAcceptSwitchTeamPercentChance(data.aiAcceptSwitchTeamPercentChance);
	gameSettings->setFallbackCpuMultiplier(data.cpuReplacementMultiplier);

	gameSettings->setMasterserver_admin(data.masterserver_admin);
	gameSettings->setMasterserver_admin_faction_index(data.masterserver_admin_factionIndex);

	gameSettings->setScenario(data.scenario.getString());
}

vector<pair<string,uint32> > NetworkMessageLaunch::getFactionCRCList() const {

	vector<pair<string,uint32> > factionCRCList;
	for(unsigned int i = 0; i < maxFactionCRCCount; ++i) {
		if(data.factionNameList[i].getString() != "") {
			factionCRCList.push_back(make_pair(data.factionNameList[i].getString(),data.factionCRCList[i]));
		}
	}
	return factionCRCList;
}

bool NetworkMessageLaunch::receive(Socket* socket) {
	bool result = NetworkMessage::receive(socket, &data, sizeof(data), true);
	fromEndian();
	data.description.nullTerminate();
	data.map.nullTerminate();
	data.tileset.nullTerminate();
	data.tech.nullTerminate();
	for(int i= 0; i < GameConstants::maxPlayers; ++i){
		data.factionTypeNames[i].nullTerminate();
		data.networkPlayerNames[i].nullTerminate();
		data.networkPlayerLanguages[i].nullTerminate();
	}
	for(unsigned int i = 0; i < maxFactionCRCCount; ++i) {
		data.factionNameList[i].nullTerminate();
	}

	data.scenario.nullTerminate();
	return result;
}

void NetworkMessageLaunch::send(Socket* socket) {
	if(data.messageType == nmtLaunch) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtLaunch\n",__FILE__,__FUNCTION__,__LINE__);
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] messageType = %d\n",__FILE__,__FUNCTION__,__LINE__,data.messageType);
	}
	toEndian();
	NetworkMessage::send(socket, &data, sizeof(data));
}

void NetworkMessageLaunch::toEndian() {
	data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
	for(int i= 0; i < GameConstants::maxPlayers; ++i){
		data.networkPlayerStatuses[i] = Shared::PlatformByteOrder::toCommonEndian(data.networkPlayerStatuses[i]);
		data.factionCRCList[i] = Shared::PlatformByteOrder::toCommonEndian(data.factionCRCList[i]);
		data.factionControls[i] = Shared::PlatformByteOrder::toCommonEndian(data.factionControls[i]);
		data.resourceMultiplierIndex[i] = Shared::PlatformByteOrder::toCommonEndian(data.resourceMultiplierIndex[i]);
		data.teams[i] = Shared::PlatformByteOrder::toCommonEndian(data.teams[i]);
		data.startLocationIndex[i] = Shared::PlatformByteOrder::toCommonEndian(data.startLocationIndex[i]);
	}
	data.mapCRC = Shared::PlatformByteOrder::toCommonEndian(data.mapCRC);
	data.tilesetCRC = Shared::PlatformByteOrder::toCommonEndian(data.tilesetCRC);
	data.techCRC = Shared::PlatformByteOrder::toCommonEndian(data.techCRC);
	data.thisFactionIndex = Shared::PlatformByteOrder::toCommonEndian(data.thisFactionIndex);
	data.factionCount = Shared::PlatformByteOrder::toCommonEndian(data.factionCount);
	data.defaultResources = Shared::PlatformByteOrder::toCommonEndian(data.defaultResources);
	data.defaultUnits = Shared::PlatformByteOrder::toCommonEndian(data.defaultUnits);

	data.defaultVictoryConditions = Shared::PlatformByteOrder::toCommonEndian(data.defaultVictoryConditions);
	data.fogOfWar = Shared::PlatformByteOrder::toCommonEndian(data.fogOfWar);
	data.allowObservers = Shared::PlatformByteOrder::toCommonEndian(data.allowObservers);
	data.enableObserverModeAtEndGame = Shared::PlatformByteOrder::toCommonEndian(data.enableObserverModeAtEndGame);
	data.enableServerControlledAI = Shared::PlatformByteOrder::toCommonEndian(data.enableServerControlledAI);
	data.networkFramePeriod = Shared::PlatformByteOrder::toCommonEndian(data.networkFramePeriod);
	data.networkPauseGameForLaggedClients = Shared::PlatformByteOrder::toCommonEndian(data.networkPauseGameForLaggedClients);
	data.pathFinderType = Shared::PlatformByteOrder::toCommonEndian(data.pathFinderType);
	data.flagTypes1 = Shared::PlatformByteOrder::toCommonEndian(data.flagTypes1);

	data.aiAcceptSwitchTeamPercentChance = Shared::PlatformByteOrder::toCommonEndian(data.aiAcceptSwitchTeamPercentChance);
	data.cpuReplacementMultiplier = Shared::PlatformByteOrder::toCommonEndian(data.cpuReplacementMultiplier);
	data.masterserver_admin = Shared::PlatformByteOrder::toCommonEndian(data.masterserver_admin);
	data.masterserver_admin_factionIndex = Shared::PlatformByteOrder::toCommonEndian(data.masterserver_admin_factionIndex);
}
void NetworkMessageLaunch::fromEndian() {
	data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
	for(int i= 0; i < GameConstants::maxPlayers; ++i){
		data.networkPlayerStatuses[i] = Shared::PlatformByteOrder::fromCommonEndian(data.networkPlayerStatuses[i]);
		data.factionCRCList[i] = Shared::PlatformByteOrder::fromCommonEndian(data.factionCRCList[i]);
		data.factionControls[i] = Shared::PlatformByteOrder::fromCommonEndian(data.factionControls[i]);
		data.resourceMultiplierIndex[i] = Shared::PlatformByteOrder::fromCommonEndian(data.resourceMultiplierIndex[i]);
		data.teams[i] = Shared::PlatformByteOrder::fromCommonEndian(data.teams[i]);
		data.startLocationIndex[i] = Shared::PlatformByteOrder::fromCommonEndian(data.startLocationIndex[i]);
	}
	data.mapCRC = Shared::PlatformByteOrder::fromCommonEndian(data.mapCRC);
	data.tilesetCRC = Shared::PlatformByteOrder::fromCommonEndian(data.tilesetCRC);
	data.techCRC = Shared::PlatformByteOrder::fromCommonEndian(data.techCRC);
	data.thisFactionIndex = Shared::PlatformByteOrder::fromCommonEndian(data.thisFactionIndex);
	data.factionCount = Shared::PlatformByteOrder::fromCommonEndian(data.factionCount);
	data.defaultResources = Shared::PlatformByteOrder::fromCommonEndian(data.defaultResources);
	data.defaultUnits = Shared::PlatformByteOrder::fromCommonEndian(data.defaultUnits);

	data.defaultVictoryConditions = Shared::PlatformByteOrder::fromCommonEndian(data.defaultVictoryConditions);
	data.fogOfWar = Shared::PlatformByteOrder::fromCommonEndian(data.fogOfWar);
	data.allowObservers = Shared::PlatformByteOrder::fromCommonEndian(data.allowObservers);
	data.enableObserverModeAtEndGame = Shared::PlatformByteOrder::fromCommonEndian(data.enableObserverModeAtEndGame);
	data.enableServerControlledAI = Shared::PlatformByteOrder::fromCommonEndian(data.enableServerControlledAI);
	data.networkFramePeriod = Shared::PlatformByteOrder::fromCommonEndian(data.networkFramePeriod);
	data.networkPauseGameForLaggedClients = Shared::PlatformByteOrder::fromCommonEndian(data.networkPauseGameForLaggedClients);
	data.pathFinderType = Shared::PlatformByteOrder::fromCommonEndian(data.pathFinderType);
	data.flagTypes1 = Shared::PlatformByteOrder::fromCommonEndian(data.flagTypes1);

	data.aiAcceptSwitchTeamPercentChance = Shared::PlatformByteOrder::fromCommonEndian(data.aiAcceptSwitchTeamPercentChance);
	data.cpuReplacementMultiplier = Shared::PlatformByteOrder::fromCommonEndian(data.cpuReplacementMultiplier);
	data.masterserver_admin = Shared::PlatformByteOrder::fromCommonEndian(data.masterserver_admin);
	data.masterserver_admin_factionIndex = Shared::PlatformByteOrder::fromCommonEndian(data.masterserver_admin_factionIndex);
}

// =====================================================
//	class NetworkMessageLaunch
// =====================================================

NetworkMessageCommandList::NetworkMessageCommandList(int32 frameCount) {
	data.header.messageType= nmtCommandList;
	data.header.frameCount= frameCount;
	data.header.commandCount= 0;
}

bool NetworkMessageCommandList::addCommand(const NetworkCommand* networkCommand){
	data.commands.push_back(*networkCommand);
	data.header.commandCount++;
	return true;
}

bool NetworkMessageCommandList::receive(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	bool result = NetworkMessage::receive(socket, &data.header, commandListHeaderSize, true);
	fromEndianHeader();
	if(result == true) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got header, messageType = %d, commandCount = %u, frameCount = %d\n",__FILE__,__FUNCTION__,__LINE__,data.header.messageType,data.header.commandCount,data.header.frameCount);

		if(data.header.commandCount > 0) {
			data.commands.resize(data.header.commandCount);

			int totalMsgSize = (sizeof(NetworkCommand) * data.header.commandCount);
			result = NetworkMessage::receive(socket, &data.commands[0], totalMsgSize, true);
			if(result == true) {
				fromEndianDetail();
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled == true) {
					for(int idx = 0 ; idx < data.header.commandCount; ++idx) {
						const NetworkCommand &cmd = data.commands[idx];

						SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] index = %d, received networkCommand [%s]\n",
								__FILE__,__FUNCTION__,__LINE__,idx, cmd.toString().c_str());
					}
				}
			}
			else {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ERROR Failed to get command data, totalMsgSize = %d.\n",__FILE__,__FUNCTION__,__LINE__,totalMsgSize);
			}
		}
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ERROR header not received as expected\n",__FILE__,__FUNCTION__,__LINE__);
	    SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] ERROR header not received as expected\n",__FILE__,__FUNCTION__,__LINE__);
	}
	return result;

}

void NetworkMessageCommandList::send(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtCommandList, frameCount = %d, data.header.commandCount = %d, data.header.messageType = %d\n",__FILE__,__FUNCTION__,__LINE__,data.header.frameCount,data.header.commandCount,data.header.messageType);

	assert(data.header.messageType==nmtCommandList);
	uint16 totalCommand = data.header.commandCount;
	toEndianHeader();
	NetworkMessage::send(socket, &data.header, commandListHeaderSize);
	if(totalCommand > 0) {
		toEndianDetail(totalCommand);
		NetworkMessage::send(socket, &data.commands[0], (sizeof(NetworkCommand) * totalCommand));
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled == true) {
	    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] messageType = %d, frameCount = %d, data.commandCount = %d\n",
                __FILE__,__FUNCTION__,__LINE__,data.header.messageType,data.header.frameCount,data.header.commandCount);

        if (data.header.commandCount > 0) {
            for(int idx = 0 ; idx < data.header.commandCount; ++idx) {
                const NetworkCommand &cmd = data.commands[idx];

                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] index = %d, sent networkCommand [%s]\n",
                        __FILE__,__FUNCTION__,__LINE__,idx, cmd.toString().c_str());
            }

            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] END of loop, nmtCommandList, frameCount = %d, data.header.commandCount = %d, data.header.messageType = %d\n",__FILE__,__FUNCTION__,__LINE__,data.header.frameCount,data.header.commandCount,data.header.messageType);
        }
	}
}

void NetworkMessageCommandList::toEndianHeader() {
	data.header.messageType = Shared::PlatformByteOrder::toCommonEndian(data.header.messageType);
	data.header.commandCount = Shared::PlatformByteOrder::toCommonEndian(data.header.commandCount);
	data.header.frameCount = Shared::PlatformByteOrder::toCommonEndian(data.header.frameCount);
}
void NetworkMessageCommandList::fromEndianHeader() {
	data.header.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.header.messageType);
	data.header.commandCount = Shared::PlatformByteOrder::fromCommonEndian(data.header.commandCount);
	data.header.frameCount = Shared::PlatformByteOrder::fromCommonEndian(data.header.frameCount);
}

void NetworkMessageCommandList::toEndianDetail(uint16 totalCommand) {
	if(totalCommand > 0) {
        for(int idx = 0 ; idx < totalCommand; ++idx) {
            NetworkCommand &cmd = data.commands[idx];
            cmd.toEndian();
        }
	}
}
void NetworkMessageCommandList::fromEndianDetail() {
	if(data.header.commandCount > 0) {
        for(int idx = 0 ; idx < data.header.commandCount; ++idx) {
            NetworkCommand &cmd = data.commands[idx];
            cmd.fromEndian();
        }
	}
}

// =====================================================
//	class NetworkMessageText
// =====================================================

NetworkMessageText::NetworkMessageText(const string &text, int teamIndex, int playerIndex,
										const string targetLanguage) {
	if(text.length() >= maxTextStringSize) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] WARNING / ERROR - text [%s] length = %d, max = %d\n",__FILE__,__FUNCTION__,__LINE__,text.c_str(),text.length(),maxTextStringSize);
	}

	data.messageType	= nmtText;
	data.text			= text;
	data.teamIndex		= teamIndex;
	data.playerIndex 	= playerIndex;
	data.targetLanguage = targetLanguage;
}

NetworkMessageText * NetworkMessageText::getCopy() const {
	NetworkMessageText *copy = new NetworkMessageText();
	copy->data = this->data;
	return copy;
}

bool NetworkMessageText::receive(Socket* socket){
	bool result = NetworkMessage::receive(socket, &data, sizeof(data), true);
	fromEndian();
	data.text.nullTerminate();
	data.targetLanguage.nullTerminate();

	return result;
}

void NetworkMessageText::send(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtText\n",__FILE__,__FUNCTION__,__LINE__);

	assert(data.messageType==nmtText);
	toEndian();
	NetworkMessage::send(socket, &data, sizeof(data));
}

void NetworkMessageText::toEndian() {
	data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
	data.teamIndex = Shared::PlatformByteOrder::toCommonEndian(data.teamIndex);
	data.playerIndex = Shared::PlatformByteOrder::toCommonEndian(data.playerIndex);
}
void NetworkMessageText::fromEndian() {
	data.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.messageType);
	data.teamIndex = Shared::PlatformByteOrder::fromCommonEndian(data.teamIndex);
	data.playerIndex = Shared::PlatformByteOrder::fromCommonEndian(data.playerIndex);
}

// =====================================================
//	class NetworkMessageQuit
// =====================================================

NetworkMessageQuit::NetworkMessageQuit(){
	data.messageType= nmtQuit;
}

bool NetworkMessageQuit::receive(Socket* socket){
	bool result = NetworkMessage::receive(socket, &data, sizeof(data),true);
	fromEndian();
	return result;
}

void NetworkMessageQuit::send(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtQuit\n",__FILE__,__FUNCTION__,__LINE__);

	assert(data.messageType==nmtQuit);
	toEndian();
	NetworkMessage::send(socket, &data, sizeof(data));
}

void NetworkMessageQuit::toEndian() {
	data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
}
void NetworkMessageQuit::fromEndian() {
	data.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.messageType);
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

        if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] gameSettings.getScenarioDir() = [%s] gameSettings.getScenario() = [%s] scenarioDir = [%s]\n",__FILE__,__FUNCTION__,__LINE__,gameSettings->getScenarioDir().c_str(),gameSettings->getScenario().c_str(),scenarioDir.c_str());
    }

    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    data.header.tilesetCRC = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTilesets,scenarioDir), string("/") + gameSettings->getTileset() + string("/*"), ".xml", NULL);

    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] data.tilesetCRC = %d, [%s]\n",__FILE__,__FUNCTION__,__LINE__, data.header.tilesetCRC,gameSettings->getTileset().c_str());

    //tech, load before map because of resources
	data.header.techCRC = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,scenarioDir), string("/") + gameSettings->getTech() + string("/*"), ".xml", NULL);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] data.techCRC = %d, [%s]\n",__FILE__,__FUNCTION__,__LINE__, data.header.techCRC,gameSettings->getTech().c_str());

	vector<std::pair<string,uint32> > vctFileList;
	vctFileList = getFolderTreeContentsCheckSumListRecursively(config.getPathListForType(ptTechs,scenarioDir),string("/") + gameSettings->getTech() + string("/*"), ".xml",&vctFileList);
	data.header.techCRCFileCount = min((int)vctFileList.size(),(int)maxFileCRCCount);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] vctFileList.size() = %d, maxFileCRCCount = %d\n",__FILE__,__FUNCTION__,__LINE__, vctFileList.size(),maxFileCRCCount);

	for(int idx =0; idx < data.header.techCRCFileCount; ++idx) {
		const std::pair<string,uint32> &fileInfo = vctFileList[idx];
		data.detail.techCRCFileList[idx] = fileInfo.first;
		data.detail.techCRCFileCRCList[idx] = fileInfo.second;
	}

    //map
    Checksum checksum;
    string file = Map::getMapPath(gameSettings->getMap(),scenarioDir,false);
	checksum.addFile(file);
	data.header.mapCRC = checksum.getSum();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] data.mapCRC = %d, [%s]\n",__FILE__,__FUNCTION__,__LINE__, data.header.mapCRC,gameSettings->getMap().c_str());
}

string NetworkMessageSynchNetworkGameData::getTechCRCFileMismatchReport(vector<std::pair<string,uint32> > &vctFileList) {
	string result = "Techtree: [" + data.header.tech.getString() + "] Filecount local: " + intToStr(vctFileList.size()) + " remote: " + intToStr(data.header.techCRCFileCount) + "\n";
	if(vctFileList.size() <= 0) {
		result = result + "Local player has no files.\n";
	}
	else if(data.header.techCRCFileCount <= 0) {
		result = result + "Remote player has no files.\n";
	}
	else {
		for(int idx = 0; idx < vctFileList.size(); ++idx) {
			std::pair<string,uint32> &fileInfo = vctFileList[idx];
			bool fileFound = false;
			uint32 remoteCRC = 0;
			for(int j = 0; j < data.header.techCRCFileCount; ++j) {
				string networkFile = data.detail.techCRCFileList[j].getString();
				uint32 &networkFileCRC = data.detail.techCRCFileCRCList[j];
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
			uint32 &networkFileCRC = data.detail.techCRCFileCRCList[i];
			bool fileFound = false;
			uint32 localCRC = 0;
			for(int idx = 0; idx < vctFileList.size(); ++idx) {
				std::pair<string,uint32> &fileInfo = vctFileList[idx];
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
	}
	return result;
}

bool NetworkMessageSynchNetworkGameData::receive(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] about to get nmtSynchNetworkGameData\n",__FILE__,__FUNCTION__,__LINE__);

	data.header.techCRCFileCount = 0;
	bool result = NetworkMessage::receive(socket, &data, HeaderSize, true);
	fromEndianHeader();
	if(result == true && data.header.techCRCFileCount > 0) {
		data.header.map.nullTerminate();
		data.header.tileset.nullTerminate();
		data.header.tech.nullTerminate();

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] messageType = %d, data.techCRCFileCount = %d\n",__FILE__,__FUNCTION__,__LINE__,data.header.messageType,data.header.techCRCFileCount);



		// Here we loop possibly multiple times
		int packetLoopCount = 1;
		if(data.header.techCRCFileCount > NetworkMessageSynchNetworkGameData::maxFileCRCPacketCount) {
			packetLoopCount = (data.header.techCRCFileCount / NetworkMessageSynchNetworkGameData::maxFileCRCPacketCount);
			if(data.header.techCRCFileCount % NetworkMessageSynchNetworkGameData::maxFileCRCPacketCount > 0) {
				packetLoopCount++;
			}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] packetLoopCount = %d\n",__FILE__,__FUNCTION__,__LINE__,packetLoopCount);

		for(int iPacketLoop = 0; result == true && iPacketLoop < packetLoopCount; ++iPacketLoop) {

			int packetIndex = iPacketLoop * NetworkMessageSynchNetworkGameData::maxFileCRCPacketCount;
			int maxFileCountPerPacket = maxFileCRCPacketCount;
			int packetFileCount = min((uint32)maxFileCountPerPacket,data.header.techCRCFileCount - packetIndex);
			int packetDetail1DataSize = (DetailSize1 * packetFileCount);
			int packetDetail2DataSize = (DetailSize2 * packetFileCount);

			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] iPacketLoop = %d, packetIndex = %d, maxFileCountPerPacket = %d, packetFileCount = %d, packetDetail1DataSize = %d, packetDetail2DataSize = %d\n",__FILE__,__FUNCTION__,__LINE__,iPacketLoop,packetIndex,maxFileCountPerPacket,packetFileCount,packetDetail1DataSize,packetDetail2DataSize);

            // Wait a max of x seconds for this message
			result = NetworkMessage::receive(socket, &data.detail.techCRCFileList[packetIndex], packetDetail1DataSize, true);
			if(result == true) {
				for(int i = 0; i < data.header.techCRCFileCount; ++i) {
					data.detail.techCRCFileList[i].nullTerminate();
				}

				result = NetworkMessage::receive(socket, &data.detail.techCRCFileCRCList[packetIndex], packetDetail2DataSize, true);
			}
		}
		fromEndianDetail();
	}

	return result;
}

void NetworkMessageSynchNetworkGameData::send(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] about to send nmtSynchNetworkGameData\n",__FILE__,__FUNCTION__,__LINE__);

	assert(data.header.messageType==nmtSynchNetworkGameData);
	uint32 totalFileCount = data.header.techCRCFileCount;
	toEndianHeader();
	NetworkMessage::send(socket, &data, HeaderSize);
	if(totalFileCount > 0) {
		// Here we loop possibly multiple times
		int packetLoopCount = 1;
		if(totalFileCount > NetworkMessageSynchNetworkGameData::maxFileCRCPacketCount) {
			packetLoopCount = (totalFileCount / NetworkMessageSynchNetworkGameData::maxFileCRCPacketCount);
			if(totalFileCount % NetworkMessageSynchNetworkGameData::maxFileCRCPacketCount > 0) {
				packetLoopCount++;
			}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] packetLoopCount = %d\n",__FILE__,__FUNCTION__,__LINE__,packetLoopCount);

		for(int iPacketLoop = 0; iPacketLoop < packetLoopCount; ++iPacketLoop) {

			int packetIndex = iPacketLoop * NetworkMessageSynchNetworkGameData::maxFileCRCPacketCount;
			int maxFileCountPerPacket = maxFileCRCPacketCount;
			int packetFileCount = min((uint32)maxFileCountPerPacket,totalFileCount - packetIndex);

			NetworkMessage::send(socket, &data.detail.techCRCFileList[packetIndex], (DetailSize1 * packetFileCount));
			NetworkMessage::send(socket, &data.detail.techCRCFileCRCList[packetIndex], (DetailSize2 * packetFileCount));
		}
		toEndianDetail(totalFileCount);
	}
}

void NetworkMessageSynchNetworkGameData::toEndianHeader() {
	data.header.messageType = Shared::PlatformByteOrder::toCommonEndian(data.header.messageType);
	data.header.mapCRC = Shared::PlatformByteOrder::toCommonEndian(data.header.mapCRC);
	data.header.tilesetCRC = Shared::PlatformByteOrder::toCommonEndian(data.header.tilesetCRC);
	data.header.techCRC = Shared::PlatformByteOrder::toCommonEndian(data.header.techCRC);
	data.header.techCRCFileCount = Shared::PlatformByteOrder::toCommonEndian(data.header.techCRCFileCount);
}
void NetworkMessageSynchNetworkGameData::fromEndianHeader() {
	data.header.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.header.messageType);
	data.header.mapCRC = Shared::PlatformByteOrder::fromCommonEndian(data.header.mapCRC);
	data.header.tilesetCRC = Shared::PlatformByteOrder::fromCommonEndian(data.header.tilesetCRC);
	data.header.techCRC = Shared::PlatformByteOrder::fromCommonEndian(data.header.techCRC);
	data.header.techCRCFileCount = Shared::PlatformByteOrder::fromCommonEndian(data.header.techCRCFileCount);
}

void NetworkMessageSynchNetworkGameData::toEndianDetail(uint32 totalFileCount) {
	for(unsigned int i = 0; i < totalFileCount; ++i) {
		data.detail.techCRCFileCRCList[i] = Shared::PlatformByteOrder::toCommonEndian(data.detail.techCRCFileCRCList[i]);
	}
}
void NetworkMessageSynchNetworkGameData::fromEndianDetail() {
	for(unsigned int i = 0; i < data.header.techCRCFileCount; ++i) {
		data.detail.techCRCFileCRCList[i] = Shared::PlatformByteOrder::fromCommonEndian(data.detail.techCRCFileCRCList[i]);
	}
}

// =====================================================
//	class NetworkMessageSynchNetworkGameDataStatus
// =====================================================

NetworkMessageSynchNetworkGameDataStatus::NetworkMessageSynchNetworkGameDataStatus(uint32 mapCRC, uint32 tilesetCRC, uint32 techCRC, vector<std::pair<string,uint32> > &vctFileList)
{
	data.header.messageType= nmtSynchNetworkGameDataStatus;

    data.header.tilesetCRC     = tilesetCRC;
    data.header.techCRC        = techCRC;
	data.header.mapCRC         = mapCRC;

	data.header.techCRCFileCount = min((int)vctFileList.size(),(int)maxFileCRCCount);
	for(int idx =0; idx < data.header.techCRCFileCount; ++idx) {
		const std::pair<string,uint32> &fileInfo = vctFileList[idx];
		data.detail.techCRCFileList[idx] = fileInfo.first;
		data.detail.techCRCFileCRCList[idx] = fileInfo.second;
	}
}

string NetworkMessageSynchNetworkGameDataStatus::getTechCRCFileMismatchReport(string techtree, vector<std::pair<string,uint32> > &vctFileList) {
	string result = "Techtree: [" + techtree + "] Filecount local: " + intToStr(vctFileList.size()) + " remote: " + intToStr(data.header.techCRCFileCount) + "\n";
	if(vctFileList.size() <= 0) {
		result = result + "Local player has no files.\n";
	}
	else if(data.header.techCRCFileCount <= 0) {
		result = result + "Remote player has no files.\n";
	}
	else {
		for(int idx = 0; idx < vctFileList.size(); ++idx) {
			std::pair<string,uint32> &fileInfo = vctFileList[idx];
			bool fileFound = false;
			uint32 remoteCRC = 0;
			for(int j = 0; j < data.header.techCRCFileCount; ++j) {
				string networkFile = data.detail.techCRCFileList[j].getString();
				uint32 &networkFileCRC = data.detail.techCRCFileCRCList[j];
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
			uint32 &networkFileCRC = data.detail.techCRCFileCRCList[i];
			bool fileFound = false;
			uint32 localCRC = 0;
			for(int idx = 0; idx < vctFileList.size(); ++idx) {
				std::pair<string,uint32> &fileInfo = vctFileList[idx];

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
	}
	return result;
}

bool NetworkMessageSynchNetworkGameDataStatus::receive(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] about to get nmtSynchNetworkGameDataStatus\n",__FILE__,__FUNCTION__,__LINE__);

	data.header.techCRCFileCount = 0;

	bool result = NetworkMessage::receive(socket, &data, HeaderSize, true);
	if(result == true && data.header.techCRCFileCount > 0) {
		fromEndianHeader();
		// Here we loop possibly multiple times
		int packetLoopCount = 1;
		if(data.header.techCRCFileCount > NetworkMessageSynchNetworkGameDataStatus::maxFileCRCPacketCount) {
			packetLoopCount = (data.header.techCRCFileCount / NetworkMessageSynchNetworkGameDataStatus::maxFileCRCPacketCount);
			if(data.header.techCRCFileCount % NetworkMessageSynchNetworkGameDataStatus::maxFileCRCPacketCount > 0) {
				packetLoopCount++;
			}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] packetLoopCount = %d\n",__FILE__,__FUNCTION__,__LINE__,packetLoopCount);

		for(int iPacketLoop = 0; iPacketLoop < packetLoopCount; ++iPacketLoop) {

			int packetIndex = iPacketLoop * NetworkMessageSynchNetworkGameDataStatus::maxFileCRCPacketCount;
			int maxFileCountPerPacket = maxFileCRCPacketCount;
			int packetFileCount = min((uint32)maxFileCountPerPacket,data.header.techCRCFileCount - packetIndex);

			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] iPacketLoop = %d, packetIndex = %d, packetFileCount = %d\n",__FILE__,__FUNCTION__,__LINE__,iPacketLoop,packetIndex,packetFileCount);

			result = NetworkMessage::receive(socket, &data.detail.techCRCFileList[packetIndex], (DetailSize1 * packetFileCount),true);
			if(result == true) {
				for(int i = 0; i < data.header.techCRCFileCount; ++i) {
					data.detail.techCRCFileList[i].nullTerminate();
				}

                // Wait a max of x seconds for this message
				result = NetworkMessage::receive(socket, &data.detail.techCRCFileCRCList[packetIndex], (DetailSize2 * packetFileCount),true);
			}
		}
		fromEndianDetail();
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] result = %d\n",__FILE__,__FUNCTION__,__LINE__,result);

	return result;
}

void NetworkMessageSynchNetworkGameDataStatus::send(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] about to send nmtSynchNetworkGameDataStatus, data.header.techCRCFileCount = %d\n",__FILE__,__FUNCTION__,__LINE__,data.header.techCRCFileCount);

	assert(data.header.messageType==nmtSynchNetworkGameDataStatus);
	uint32 totalFileCount = data.header.techCRCFileCount;
	toEndianHeader();
	NetworkMessage::send(socket, &data, HeaderSize);
	if(totalFileCount > 0) {
		// Here we loop possibly multiple times
		int packetLoopCount = 1;
		if(totalFileCount > NetworkMessageSynchNetworkGameDataStatus::maxFileCRCPacketCount) {
			packetLoopCount = (totalFileCount / NetworkMessageSynchNetworkGameDataStatus::maxFileCRCPacketCount);
			if(totalFileCount % NetworkMessageSynchNetworkGameDataStatus::maxFileCRCPacketCount > 0) {
				packetLoopCount++;
			}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] packetLoopCount = %d\n",__FILE__,__FUNCTION__,__LINE__,packetLoopCount);

		toEndianDetail(totalFileCount);
		for(int iPacketLoop = 0; iPacketLoop < packetLoopCount; ++iPacketLoop) {

			int packetIndex = iPacketLoop * NetworkMessageSynchNetworkGameDataStatus::maxFileCRCPacketCount;
			int maxFileCountPerPacket = maxFileCRCPacketCount;
			int packetFileCount = min((uint32)maxFileCountPerPacket,totalFileCount - packetIndex);

			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] packetLoop = %d, packetIndex = %d, packetFileCount = %d\n",__FILE__,__FUNCTION__,__LINE__,iPacketLoop,packetIndex,packetFileCount);

			NetworkMessage::send(socket, &data.detail.techCRCFileList[packetIndex], (DetailSize1 * packetFileCount));
			NetworkMessage::send(socket, &data.detail.techCRCFileCRCList[packetIndex], (DetailSize2 * packetFileCount));
		}
	}
}

void NetworkMessageSynchNetworkGameDataStatus::toEndianHeader() {
	data.header.messageType = Shared::PlatformByteOrder::toCommonEndian(data.header.messageType);
	data.header.mapCRC = Shared::PlatformByteOrder::toCommonEndian(data.header.mapCRC);
	data.header.tilesetCRC = Shared::PlatformByteOrder::toCommonEndian(data.header.tilesetCRC);
	data.header.techCRC = Shared::PlatformByteOrder::toCommonEndian(data.header.techCRC);
	data.header.techCRCFileCount = Shared::PlatformByteOrder::toCommonEndian(data.header.techCRCFileCount);
}
void NetworkMessageSynchNetworkGameDataStatus::fromEndianHeader() {
	data.header.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.header.messageType);
	data.header.mapCRC = Shared::PlatformByteOrder::fromCommonEndian(data.header.mapCRC);
	data.header.tilesetCRC = Shared::PlatformByteOrder::fromCommonEndian(data.header.tilesetCRC);
	data.header.techCRC = Shared::PlatformByteOrder::fromCommonEndian(data.header.techCRC);
	data.header.techCRCFileCount = Shared::PlatformByteOrder::fromCommonEndian(data.header.techCRCFileCount);
}

void NetworkMessageSynchNetworkGameDataStatus::toEndianDetail(uint32 totalFileCount) {
	for(unsigned int i = 0; i < totalFileCount; ++i) {
		data.detail.techCRCFileCRCList[i] = Shared::PlatformByteOrder::toCommonEndian(data.detail.techCRCFileCRCList[i]);
	}
}
void NetworkMessageSynchNetworkGameDataStatus::fromEndianDetail() {
	for(unsigned int i = 0; i < data.header.techCRCFileCount; ++i) {
		data.detail.techCRCFileCRCList[i] = Shared::PlatformByteOrder::fromCommonEndian(data.detail.techCRCFileCRCList[i]);
	}
}

// =====================================================
//	class NetworkMessageSynchNetworkGameDataFileCRCCheck
// =====================================================

NetworkMessageSynchNetworkGameDataFileCRCCheck::NetworkMessageSynchNetworkGameDataFileCRCCheck(uint32 totalFileCount, uint32 fileIndex, uint32 fileCRC, const string fileName)
{
	data.messageType= nmtSynchNetworkGameDataFileCRCCheck;

    data.totalFileCount = totalFileCount;
    data.fileIndex      = fileIndex;
    data.fileCRC        = fileCRC;
    data.fileName       = fileName;
}

bool NetworkMessageSynchNetworkGameDataFileCRCCheck::receive(Socket* socket) {
	bool result = NetworkMessage::receive(socket, &data, sizeof(data), true);
	fromEndian();
	data.fileName.nullTerminate();

	return result;
}

void NetworkMessageSynchNetworkGameDataFileCRCCheck::send(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtSynchNetworkGameDataFileCRCCheck\n",__FILE__,__FUNCTION__,__LINE__);

	assert(data.messageType==nmtSynchNetworkGameDataFileCRCCheck);
	toEndian();
	NetworkMessage::send(socket, &data, sizeof(data));
}

void NetworkMessageSynchNetworkGameDataFileCRCCheck::toEndian() {
	data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
	data.totalFileCount = Shared::PlatformByteOrder::toCommonEndian(data.totalFileCount);
	data.fileIndex = Shared::PlatformByteOrder::toCommonEndian(data.fileIndex);
	data.fileCRC = Shared::PlatformByteOrder::toCommonEndian(data.fileCRC);
}

void NetworkMessageSynchNetworkGameDataFileCRCCheck::fromEndian() {
	data.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.messageType);
	data.totalFileCount = Shared::PlatformByteOrder::fromCommonEndian(data.totalFileCount);
	data.fileIndex = Shared::PlatformByteOrder::fromCommonEndian(data.fileIndex);
	data.fileCRC = Shared::PlatformByteOrder::fromCommonEndian(data.fileCRC);
}
// =====================================================
//	class NetworkMessageSynchNetworkGameDataFileGet
// =====================================================

NetworkMessageSynchNetworkGameDataFileGet::NetworkMessageSynchNetworkGameDataFileGet(const string fileName) {
	data.messageType= nmtSynchNetworkGameDataFileGet;
    data.fileName       = fileName;
}

bool NetworkMessageSynchNetworkGameDataFileGet::receive(Socket* socket) {
	bool result = NetworkMessage::receive(socket, &data, sizeof(data), true);
	fromEndian();
	data.fileName.nullTerminate();

	return result;
}

void NetworkMessageSynchNetworkGameDataFileGet::send(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtSynchNetworkGameDataFileGet\n",__FILE__,__FUNCTION__,__LINE__);

	assert(data.messageType==nmtSynchNetworkGameDataFileGet);
	toEndian();
	NetworkMessage::send(socket, &data, sizeof(data));
}

void NetworkMessageSynchNetworkGameDataFileGet::toEndian() {
	data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
}
void NetworkMessageSynchNetworkGameDataFileGet::fromEndian() {
	data.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.messageType);
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
    data.networkPlayerStatus = npst_None;
    data.switchFlags = ssrft_None;
    data.language = "";
}

SwitchSetupRequest::SwitchSetupRequest(string selectedFactionName, int8 currentFactionIndex,
										int8 toFactionIndex,int8 toTeam,string networkPlayerName,
										int8 networkPlayerStatus, int8 flags,
										string language) {
	data.messageType= nmtSwitchSetupRequest;
	data.selectedFactionName=selectedFactionName;
	data.currentFactionIndex=currentFactionIndex;
	data.toFactionIndex=toFactionIndex;
    data.toTeam = toTeam;
    data.networkPlayerName=networkPlayerName;
    data.networkPlayerStatus=networkPlayerStatus;
    data.switchFlags = flags;
    data.language = language;
}

bool SwitchSetupRequest::receive(Socket* socket) {
	bool result = NetworkMessage::receive(socket, &data, sizeof(data), true);
	fromEndian();
	data.selectedFactionName.nullTerminate();
	data.networkPlayerName.nullTerminate();
	data.language.nullTerminate();

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d] data.networkPlayerName [%s]\n",__FILE__,__FUNCTION__,__LINE__,data.networkPlayerName.getString().c_str());

	return result;
}

void SwitchSetupRequest::send(Socket* socket) {
	assert(data.messageType==nmtSwitchSetupRequest);

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d] data.networkPlayerName [%s]\n",__FILE__,__FUNCTION__,__LINE__,data.networkPlayerName.getString().c_str());
	toEndian();
	NetworkMessage::send(socket, &data, sizeof(data));
}

void SwitchSetupRequest::toEndian() {
	data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
	data.currentFactionIndex = Shared::PlatformByteOrder::toCommonEndian(data.currentFactionIndex);
	data.toFactionIndex = Shared::PlatformByteOrder::toCommonEndian(data.toFactionIndex);
	data.toTeam = Shared::PlatformByteOrder::toCommonEndian(data.toTeam);
	data.networkPlayerStatus = Shared::PlatformByteOrder::toCommonEndian(data.networkPlayerStatus);
	data.switchFlags = Shared::PlatformByteOrder::toCommonEndian(data.switchFlags);
}
void SwitchSetupRequest::fromEndian() {
	data.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.messageType);
	data.currentFactionIndex = Shared::PlatformByteOrder::fromCommonEndian(data.currentFactionIndex);
	data.toFactionIndex = Shared::PlatformByteOrder::fromCommonEndian(data.toFactionIndex);
	data.toTeam = Shared::PlatformByteOrder::fromCommonEndian(data.toTeam);
	data.networkPlayerStatus = Shared::PlatformByteOrder::fromCommonEndian(data.networkPlayerStatus);
	data.switchFlags = Shared::PlatformByteOrder::fromCommonEndian(data.switchFlags);
}

// =====================================================
//	class PlayerIndexMessage
// =====================================================
PlayerIndexMessage::PlayerIndexMessage(int16 playerIndex) {
	data.messageType= nmtPlayerIndexMessage;
	data.playerIndex=playerIndex;
}

bool PlayerIndexMessage::receive(Socket* socket) {
	bool result = NetworkMessage::receive(socket, &data, sizeof(data), true);
	fromEndian();
	return result;
}

void PlayerIndexMessage::send(Socket* socket) {
	assert(data.messageType==nmtPlayerIndexMessage);
	toEndian();
	NetworkMessage::send(socket, &data, sizeof(data));
}

void PlayerIndexMessage::toEndian() {
	data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
	data.playerIndex = Shared::PlatformByteOrder::toCommonEndian(data.playerIndex);
}
void PlayerIndexMessage::fromEndian() {
	data.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.messageType);
	data.playerIndex = Shared::PlatformByteOrder::fromCommonEndian(data.playerIndex);
}

// =====================================================
//	class NetworkMessageLoadingStatus
// =====================================================
NetworkMessageLoadingStatus::NetworkMessageLoadingStatus(uint32 status)
{
	data.messageType= nmtLoadingStatusMessage;
	data.status=status;
}

bool NetworkMessageLoadingStatus::receive(Socket* socket) {
	bool result = NetworkMessage::receive(socket, &data, sizeof(data), true);
	fromEndian();
	return result;
}

void NetworkMessageLoadingStatus::send(Socket* socket) {
	assert(data.messageType==nmtLoadingStatusMessage);
	toEndian();
	NetworkMessage::send(socket, &data, sizeof(data));
}

void NetworkMessageLoadingStatus::toEndian() {
	data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
	data.status = Shared::PlatformByteOrder::toCommonEndian(data.status);
}
void NetworkMessageLoadingStatus::fromEndian() {
	data.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.messageType);
	data.status = Shared::PlatformByteOrder::fromCommonEndian(data.status);
}

// =====================================================
//	class NetworkMessageMarkCell
// =====================================================

NetworkMessageMarkCell::NetworkMessageMarkCell(Vec2i target, int factionIndex, const string &text, int playerIndex) {
	if(text.length() >= maxTextStringSize) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] WARNING / ERROR - text [%s] length = %d, max = %d\n",__FILE__,__FUNCTION__,__LINE__,text.c_str(),text.length(),maxTextStringSize);
	}

	data.messageType	= nmtMarkCell;
	data.text			= text;
	data.targetX		= target.x;
	data.targetY		= target.y;
	data.factionIndex 	= factionIndex;
	data.playerIndex	= playerIndex;
}

NetworkMessageMarkCell * NetworkMessageMarkCell::getCopy() const {
	NetworkMessageMarkCell *copy = new NetworkMessageMarkCell();
	copy->data = this->data;
	return copy;
}

bool NetworkMessageMarkCell::receive(Socket* socket){
	bool result = NetworkMessage::receive(socket, &data, sizeof(data), true);
	fromEndian();
	data.text.nullTerminate();
	return result;
}

void NetworkMessageMarkCell::send(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtMarkCell\n",__FILE__,__FUNCTION__,__LINE__);

	assert(data.messageType == nmtMarkCell);
	toEndian();
	NetworkMessage::send(socket, &data, sizeof(data));
}

void NetworkMessageMarkCell::toEndian() {
	data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
	data.targetX = Shared::PlatformByteOrder::toCommonEndian(data.targetX);
	data.targetY = Shared::PlatformByteOrder::toCommonEndian(data.targetY);
	data.factionIndex = Shared::PlatformByteOrder::toCommonEndian(data.factionIndex);
	data.playerIndex = Shared::PlatformByteOrder::toCommonEndian(data.playerIndex);
}
void NetworkMessageMarkCell::fromEndian() {
	data.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.messageType);
	data.targetX = Shared::PlatformByteOrder::fromCommonEndian(data.targetX);
	data.targetY = Shared::PlatformByteOrder::fromCommonEndian(data.targetY);
	data.factionIndex = Shared::PlatformByteOrder::fromCommonEndian(data.factionIndex);
	data.playerIndex = Shared::PlatformByteOrder::fromCommonEndian(data.playerIndex);
}

// =====================================================
//	class NetworkMessageUnMarkCell
// =====================================================

NetworkMessageUnMarkCell::NetworkMessageUnMarkCell(Vec2i target, int factionIndex) {
	data.messageType	= nmtUnMarkCell;
	data.targetX		= target.x;
	data.targetY		= target.y;
	data.factionIndex 	= factionIndex;
}

NetworkMessageUnMarkCell * NetworkMessageUnMarkCell::getCopy() const {
	NetworkMessageUnMarkCell *copy = new NetworkMessageUnMarkCell();
	copy->data = this->data;
	return copy;
}

bool NetworkMessageUnMarkCell::receive(Socket* socket){
	bool result = NetworkMessage::receive(socket, &data, sizeof(data), true);
	fromEndian();
	return result;
}

void NetworkMessageUnMarkCell::send(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtUnMarkCell\n",__FILE__,__FUNCTION__,__LINE__);

	assert(data.messageType == nmtUnMarkCell);
	toEndian();
	NetworkMessage::send(socket, &data, sizeof(data));
}

void NetworkMessageUnMarkCell::toEndian() {
	data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
	data.targetX = Shared::PlatformByteOrder::toCommonEndian(data.targetX);
	data.targetY = Shared::PlatformByteOrder::toCommonEndian(data.targetY);
	data.factionIndex = Shared::PlatformByteOrder::toCommonEndian(data.factionIndex);
}
void NetworkMessageUnMarkCell::fromEndian() {
	data.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.messageType);
	data.targetX = Shared::PlatformByteOrder::fromCommonEndian(data.targetX);
	data.targetY = Shared::PlatformByteOrder::fromCommonEndian(data.targetY);
	data.factionIndex = Shared::PlatformByteOrder::fromCommonEndian(data.factionIndex);
}

// =====================================================
//	class NetworkMessageHighlightCell
// =====================================================

NetworkMessageHighlightCell::NetworkMessageHighlightCell(Vec2i target, int factionIndex) {

	data.messageType	= nmtHighlightCell;
	data.targetX		= target.x;
	data.targetY		= target.y;
	data.factionIndex 	= factionIndex;
}

bool NetworkMessageHighlightCell::receive(Socket* socket){
	bool result = NetworkMessage::receive(socket, &data, sizeof(data), true);
	fromEndian();
	return result;
}

void NetworkMessageHighlightCell::send(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtMarkCell\n",__FILE__,__FUNCTION__,__LINE__);

	assert(data.messageType == nmtHighlightCell);
	toEndian();
	NetworkMessage::send(socket, &data, sizeof(data));
}

void NetworkMessageHighlightCell::toEndian() {
	data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
	data.targetX = Shared::PlatformByteOrder::toCommonEndian(data.targetX);
	data.targetY = Shared::PlatformByteOrder::toCommonEndian(data.targetY);
	data.factionIndex = Shared::PlatformByteOrder::toCommonEndian(data.factionIndex);
}
void NetworkMessageHighlightCell::fromEndian() {
	data.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.messageType);
	data.targetX = Shared::PlatformByteOrder::fromCommonEndian(data.targetX);
	data.targetY = Shared::PlatformByteOrder::fromCommonEndian(data.targetY);
	data.factionIndex = Shared::PlatformByteOrder::fromCommonEndian(data.factionIndex);
}

}}//end namespace
