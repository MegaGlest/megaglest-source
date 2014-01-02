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

#include "data_types.h"
#include "util.h"
#include "game_settings.h"
#include "checksum.h"
#include "platform_util.h"
#include "config.h"
#include "network_protocol.h"
#include <algorithm>
#include <cassert>
#include <stdexcept>

#include "leak_dumper.h"

using namespace Shared::Platform;
using namespace Shared::Util;
using namespace std;
using std::min;

namespace Glest{ namespace Game{

bool NetworkMessage::useOldProtocol = true;

auto_ptr<Mutex> NetworkMessage::mutexMessageStats(new Mutex(CODE_AT_LINE));
Chrono NetworkMessage::statsTimer;
Chrono NetworkMessage::lastSend;
Chrono NetworkMessage::lastRecv;
std::map<NetworkMessageStatisticType,int64> NetworkMessage::mapMessageStats;

// =====================================================
//	class NetworkMessage
// =====================================================

bool NetworkMessage::receive(Socket* socket, void* data, int dataSize, bool tryReceiveUntilDataSizeMet) {
	if(socket != NULL) {
		int dataReceived = socket->receive(data, dataSize, tryReceiveUntilDataSizeMet);
		if(dataReceived != dataSize) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] WARNING, dataReceived = %d dataSize = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,dataReceived,dataSize);
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\nIn [%s::%s Line: %d] WARNING, dataReceived = %d dataSize = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,dataReceived,dataSize);

			if(socket != NULL && socket->getSocketId() > 0) {
				throw megaglest_runtime_error("Error receiving NetworkMessage, dataReceived = " + intToStr(dataReceived) + ", dataSize = " + intToStr(dataSize));
			}
			else {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] socket has been disconnected\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			}
		}
		else {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] dataSize = %d, dataReceived = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,dataSize,dataReceived);

			dump_packet("\nINCOMING PACKET:\n",data, dataSize, false);
			return true;
		}
	}
	return false;

}

void NetworkMessage::send(Socket* socket, const void* data, int dataSize) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] socket = %p, data = %p, dataSize = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,socket,data,dataSize);

	if(socket != NULL) {
		dump_packet("\nOUTGOING PACKET:\n",data, dataSize, true);
		int sendResult = socket->send(data, dataSize);
		if(sendResult != dataSize) {
			if(socket != NULL && socket->getSocketId() > 0) {
				char szBuf[8096]="";
				snprintf(szBuf,8096,"Error sending NetworkMessage, sendResult = %d, dataSize = %d",sendResult,dataSize);
				throw megaglest_runtime_error(szBuf);
			}
			else {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d socket has been disconnected\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			}
		}
	}
}

void NetworkMessage::resetNetworkPacketStats() {
	NetworkMessage::statsTimer.stop();
	NetworkMessage::lastSend.stop();
	NetworkMessage::lastRecv.stop();
	NetworkMessage::mapMessageStats.clear();
}

string  NetworkMessage::getNetworkPacketStats() {
	string result = "Current Timer Milliseconds: " + intToStr(NetworkMessage::statsTimer.getMillis()) + "\n";

	for(std::map<NetworkMessageStatisticType,int64>::iterator iterMap = mapMessageStats.begin();
			iterMap != mapMessageStats.end(); ++iterMap) {
		switch(iterMap->first) {
			case netmsgstPacketsPerMillisecondSend:
				result += "send p / msec: " + intToStr(iterMap->second) + (iterMap->second > 10 ? "  ****WARNING WIN32 LIMIT EXCEEDED****" : "") + "\n";
				break;
			case netmsgstPacketsPerSecondSend:
				result += "send p / sec: " + intToStr(iterMap->second) + "\n";
				break;
			case netmsgstAverageSendSize:
				result += "send avg size: " + intToStr(iterMap->second) + "\n";
				break;

			case netmsgstPacketsPerMillisecondRecv:
				result += "recv p / msec: " + intToStr(iterMap->second) + "\n";
				break;
			case netmsgstPacketsPerSecondRecv:
				result += "recv p / sec: " + intToStr(iterMap->second) + (iterMap->second > 10 ? "  ****WARNING WIN32 LIMIT EXCEEDED****" : "") + "\n";
				break;
			case netmsgstAverageRecvSize:
				result += "recv avg size: " + intToStr(iterMap->second) + "\n";
				break;

		}
	}
	return result;
}

void NetworkMessage::dump_packet(string label, const void* data, int dataSize, bool isSend) {
	Config &config = Config::getInstance();
	if( config.getBool("DebugNetworkPacketStats","false") == true) {

		MutexSafeWrapper safeMutex(NetworkMessage::mutexMessageStats.get());

		if(NetworkMessage::statsTimer.isStarted() == false) {
			NetworkMessage::statsTimer.start();
		}

		bool secondChanged = false;
		if(NetworkMessage::statsTimer.getSeconds() - NetworkMessage::mapMessageStats[netmsgstLastEvent] >= 3) {

			NetworkMessage::mapMessageStats[netmsgstLastEvent] = NetworkMessage::statsTimer.getSeconds();
			secondChanged = true;
		}

		if(isSend == true) {
			if(NetworkMessage::lastSend.isStarted() == false) {
				NetworkMessage::lastSend.start();
			}
			int64 millisecondsSinceLastSend = NetworkMessage::lastSend.getMillis();
			int64 secondsSinceLastSend 		= NetworkMessage::lastSend.getSeconds();

			//char szBuf[8096]="";
			//snprintf(szBuf,8095,"\nSEND check cur [%lld] last [%lld]\n", (long long)millisecondsSinceLastSend,(long long)NetworkMessage::mapMessageStats[netmsgstPacketsPerMillisecondSend_last] );
			//printf("%s",szBuf);

			if(millisecondsSinceLastSend == NetworkMessage::mapMessageStats[netmsgstPacketsPerMillisecondSend_last]) {
				NetworkMessage::mapMessageStats[netmsgstPacketsPerMillisecondSend_current_count]++;

			}
			else {
				NetworkMessage::mapMessageStats[netmsgstPacketsPerMillisecondSend_last] = millisecondsSinceLastSend;
				NetworkMessage::mapMessageStats[netmsgstPacketsPerMillisecondSend] =
						(NetworkMessage::mapMessageStats[netmsgstPacketsPerMillisecondSend] +
						 NetworkMessage::mapMessageStats[netmsgstPacketsPerMillisecondSend_current_count]) / 2;
			}

			if(secondsSinceLastSend == NetworkMessage::mapMessageStats[netmsgstPacketsPerSecondSend_last]) {
				NetworkMessage::mapMessageStats[netmsgstPacketsPerSecondSend_current_count]++;

			}
			else {
				NetworkMessage::mapMessageStats[netmsgstPacketsPerSecondSend_last] = secondsSinceLastSend;
				NetworkMessage::mapMessageStats[netmsgstPacketsPerSecondSend] =
						(NetworkMessage::mapMessageStats[netmsgstPacketsPerSecondSend] +
						 NetworkMessage::mapMessageStats[netmsgstPacketsPerSecondSend_current_count]) / 2;
			}

			NetworkMessage::mapMessageStats[netmsgstAverageSendSize] =
					(NetworkMessage::mapMessageStats[netmsgstAverageSendSize] +
					 dataSize) / 2;
		}
		else {
			if(NetworkMessage::lastRecv.isStarted() == false) {
				NetworkMessage::lastRecv.start();
			}
			int64 millisecondsSinceLastRecv = NetworkMessage::lastRecv.getMillis();
			int64 secondsSinceLastRecv 		= NetworkMessage::lastRecv.getSeconds();

			//char szBuf[8096]="";
			//snprintf(szBuf,8095,"\nRECV check cur [%lld] last [%lld]\n", (long long)millisecondsSinceLastRecv,(long long)NetworkMessage::mapMessageStats[netmsgstPacketsPerMillisecondRecv_last] );
			//printf("%s",szBuf);

			if(millisecondsSinceLastRecv == NetworkMessage::mapMessageStats[netmsgstPacketsPerMillisecondRecv_last]) {
				NetworkMessage::mapMessageStats[netmsgstPacketsPerMillisecondRecv_current_count]++;

			}
			else {
				NetworkMessage::mapMessageStats[netmsgstPacketsPerMillisecondRecv_last] = millisecondsSinceLastRecv;
				NetworkMessage::mapMessageStats[netmsgstPacketsPerMillisecondRecv] =
						(NetworkMessage::mapMessageStats[netmsgstPacketsPerMillisecondRecv] +
						 NetworkMessage::mapMessageStats[netmsgstPacketsPerMillisecondRecv_current_count]) / 2;
			}

			if(secondsSinceLastRecv == NetworkMessage::mapMessageStats[netmsgstPacketsPerSecondRecv_last]) {
				NetworkMessage::mapMessageStats[netmsgstPacketsPerSecondRecv_current_count]++;

			}
			else {
				NetworkMessage::mapMessageStats[netmsgstPacketsPerSecondRecv_last] = secondsSinceLastRecv;
				NetworkMessage::mapMessageStats[netmsgstPacketsPerSecondRecv] =
						(NetworkMessage::mapMessageStats[netmsgstPacketsPerSecondRecv] +
						 NetworkMessage::mapMessageStats[netmsgstPacketsPerSecondRecv_current_count]) / 2;
			}

			NetworkMessage::mapMessageStats[netmsgstAverageRecvSize] =
					(NetworkMessage::mapMessageStats[netmsgstAverageRecvSize] +
					 dataSize) / 2;
		}

		if(secondChanged == true) {
			printf("%s",NetworkMessage::getNetworkPacketStats().c_str());
		}
	}

	if( config.getBool("DebugNetworkPackets","false") == true ||
		config.getBool("DebugNetworkPacketSizes","false") == true) {

		printf("%s DataSize = %d",label.c_str(),dataSize);

		if(config.getBool("DebugNetworkPackets","false") == true) {

			printf("\n");
			const char *buf = static_cast<const char *>(data);
			for(unsigned int index = 0; index < (unsigned int)dataSize; ++index) {

				printf("%u[%X][%d] ",index,buf[index],buf[index]);
				if(index % 10 == 0) {
					printf("\n");
				}
			}
		}
		printf("\n======= END =======\n");
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
	data.ftpPort = 0;
	data.gameInProgress = 0;
}

NetworkMessageIntro::NetworkMessageIntro(int32 sessionId,const string &versionString,
										const string &name, int playerIndex,
										NetworkGameStateType gameState,
										uint32 externalIp,
										uint32 ftpPort,
										const string &playerLanguage,
										int gameInProgress, const string &playerUUID,
										const string &platform) {
	data.messageType	= nmtIntro;
	data.sessionId		= sessionId;
	data.versionString	= versionString;
	data.name			= name;
	data.playerIndex	= static_cast<int16>(playerIndex);
	data.gameState		= static_cast<int8>(gameState);
	data.externalIp     = externalIp;
	data.ftpPort		= ftpPort;
	data.language		= playerLanguage;
	data.gameInProgress = gameInProgress;
	data.playerUUID		= playerUUID;
	data.platform		= platform;
}

const char * NetworkMessageIntro::getPackedMessageFormat() const {
	return "cl128s32shcLL60sc60s60s";
}

unsigned int NetworkMessageIntro::getPackedSize() {
	static unsigned int result = 0;
	if(result == 0) {
		Data packedData;
		packedData.externalIp = 0;
		packedData.ftpPort = 0;
		packedData.gameInProgress = 0;
		packedData.gameState = 0;
		packedData.messageType = nmtIntro;
		packedData.playerIndex = 0;
		packedData.sessionId = 0;

		unsigned char *buf = new unsigned char[sizeof(packedData)*3];
		result = pack(buf, getPackedMessageFormat(),
				packedData.messageType,
				packedData.sessionId,
				packedData.versionString.getBuffer(),
				packedData.name.getBuffer(),
				packedData.playerIndex,
				packedData.gameState,
				packedData.externalIp,
				packedData.ftpPort,
				packedData.language.getBuffer(),
				data.gameInProgress,
				packedData.playerUUID.getBuffer(),
				packedData.platform.getBuffer());
		delete [] buf;
	}
	return result;
}
void NetworkMessageIntro::unpackMessage(unsigned char *buf) {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\nIn [%s] about to unpack...\n",__FUNCTION__);
	unpack(buf, getPackedMessageFormat(),
			&data.messageType,
			&data.sessionId,
			data.versionString.getBuffer(),
			data.name.getBuffer(),
			&data.playerIndex,
			&data.gameState,
			&data.externalIp,
			&data.ftpPort,
			data.language.getBuffer(),
			&data.gameInProgress,
			data.playerUUID.getBuffer(),
			data.platform.getBuffer());
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s] unpacked data:\n%s\n",__FUNCTION__,this->toString().c_str());
}

unsigned char * NetworkMessageIntro::packMessage() {
	unsigned char *buf = new unsigned char[getPackedSize()+1];

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\nIn [%s] about to pack...\n",__FUNCTION__);
	pack(buf, getPackedMessageFormat(),
			data.messageType,
			data.sessionId,
			data.versionString.getBuffer(),
			data.name.getBuffer(),
			data.playerIndex,
			data.gameState,
			data.externalIp,
			data.ftpPort,
			data.language.getBuffer(),
			data.gameInProgress,
			data.playerUUID.getBuffer(),
			data.platform.getBuffer());
	return buf;
}

string NetworkMessageIntro::toString() const {
	string result = "messageType = " + intToStr(data.messageType);
	result += " sessionId = " + intToStr(data.sessionId);
	result += " versionString = " + data.versionString.getString();
	result += " name = " + data.name.getString();
	result += " playerIndex = " + intToStr(data.playerIndex);
	result += " gameState = " + intToStr(data.gameState);
	result += " externalIp = " + uIntToStr(data.externalIp);
	result += " ftpPort = " + uIntToStr(data.ftpPort);
	result += " language = " + data.language.getString();
	result += " gameInProgress = " + uIntToStr(data.gameInProgress);
	result += " playerUUID = " + data.playerUUID.getString();
	result += " platform = " + data.platform.getString();

	return result;
}

bool NetworkMessageIntro::receive(Socket* socket) {
	bool result = false;
	if(useOldProtocol == true) {
		result = NetworkMessage::receive(socket, &data, sizeof(data), true);
	}
	else {
		unsigned char *buf = new unsigned char[getPackedSize()+1];
		result = NetworkMessage::receive(socket, buf, getPackedSize(), true);
		unpackMessage(buf);
		//printf("Got packet size = %u data.messageType = %d\n%s\n",getPackedSize(),data.messageType,buf);
		delete [] buf;
	}
	fromEndian();

	data.name.nullTerminate();
	data.versionString.nullTerminate();
	data.language.nullTerminate();
	data.playerUUID.nullTerminate();
	data.platform.nullTerminate();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] get nmtIntro, data.playerIndex = %d, data.sessionId = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,data.playerIndex,data.sessionId);
	return result;
}

void NetworkMessageIntro::send(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] sending nmtIntro, data.playerIndex = %d, data.sessionId = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,data.playerIndex,data.sessionId);
	assert(data.messageType == nmtIntro);
	toEndian();

	if(useOldProtocol == true) {
		NetworkMessage::send(socket, &data, sizeof(data));
	}
	else {
		unsigned char *buf = packMessage();
		//printf("Send packet size = %u data.messageType = %d\n[%s]\n",getPackedSize(),data.messageType,buf);
		//NetworkMessage::send(socket, &data, sizeof(data));
		NetworkMessage::send(socket, buf, getPackedSize());
		delete [] buf;
	}
}

void NetworkMessageIntro::toEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
		data.sessionId = Shared::PlatformByteOrder::toCommonEndian(data.sessionId);
		data.playerIndex = Shared::PlatformByteOrder::toCommonEndian(data.playerIndex);
		data.gameState = Shared::PlatformByteOrder::toCommonEndian(data.gameState);
		data.externalIp = Shared::PlatformByteOrder::toCommonEndian(data.externalIp);
		data.ftpPort = Shared::PlatformByteOrder::toCommonEndian(data.ftpPort);

		data.gameInProgress = Shared::PlatformByteOrder::toCommonEndian(data.gameInProgress);
	}
}
void NetworkMessageIntro::fromEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.messageType);
		data.sessionId = Shared::PlatformByteOrder::fromCommonEndian(data.sessionId);
		data.playerIndex = Shared::PlatformByteOrder::fromCommonEndian(data.playerIndex);
		data.gameState = Shared::PlatformByteOrder::fromCommonEndian(data.gameState);
		data.externalIp = Shared::PlatformByteOrder::fromCommonEndian(data.externalIp);
		data.ftpPort = Shared::PlatformByteOrder::fromCommonEndian(data.ftpPort);

		data.gameInProgress = Shared::PlatformByteOrder::fromCommonEndian(data.gameInProgress);
	}
}

// =====================================================
//	class NetworkMessagePing
// =====================================================

NetworkMessagePing::NetworkMessagePing(){
	data.messageType = nmtPing;
	data.pingFrequency = 0;
	data.pingTime = 0;
	pingReceivedLocalTime = 0;
}

NetworkMessagePing::NetworkMessagePing(int32 pingFrequency, int64 pingTime){
	data.messageType= nmtPing;
	data.pingFrequency= pingFrequency;
	data.pingTime= pingTime;
	pingReceivedLocalTime=0;
}

const char * NetworkMessagePing::getPackedMessageFormat() const {
	return "clq";
}

unsigned int NetworkMessagePing::getPackedSize() {
	static unsigned int result = 0;
	if(result == 0) {
		Data packedData;
		packedData.messageType = 0;
		packedData.pingFrequency = 0;
		packedData.pingTime = 0;
		unsigned char *buf = new unsigned char[sizeof(packedData)*3];
		result = pack(buf, getPackedMessageFormat(),
				packedData.messageType,
				packedData.pingFrequency,
				packedData.pingTime);
		delete [] buf;
	}
	return result;
}
void NetworkMessagePing::unpackMessage(unsigned char *buf) {
	unpack(buf, getPackedMessageFormat(),
			&data.messageType,
			&data.pingFrequency,
			&data.pingTime);
}

unsigned char * NetworkMessagePing::packMessage() {
	unsigned char *buf = new unsigned char[getPackedSize()+1];
	pack(buf, getPackedMessageFormat(),
			data.messageType,
			data.pingFrequency,
			data.pingTime);
	return buf;
}

bool NetworkMessagePing::receive(Socket* socket){
	bool result = false;
	if(useOldProtocol == true) {
		result = NetworkMessage::receive(socket, &data, sizeof(data), true);
	}
	else {
		unsigned char *buf = new unsigned char[getPackedSize()+1];
		result = NetworkMessage::receive(socket, buf, getPackedSize(), true);
		unpackMessage(buf);
		//printf("Got packet size = %u data.messageType = %d\n%s\n",getPackedSize(),data.messageType,buf);
		delete [] buf;
	}
	fromEndian();

	pingReceivedLocalTime = time(NULL);
	return result;
}

void NetworkMessagePing::send(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtPing\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	assert(data.messageType==nmtPing);
	toEndian();

	if(useOldProtocol == true) {
		NetworkMessage::send(socket, &data, sizeof(data));
	}
	else {
		unsigned char *buf = packMessage();
		//printf("Send packet size = %u data.messageType = %d\n[%s]\n",getPackedSize(),data.messageType,buf);
		//NetworkMessage::send(socket, &data, sizeof(data));
		NetworkMessage::send(socket, buf, getPackedSize());
		delete [] buf;
	}
}

void NetworkMessagePing::toEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
		data.pingFrequency = Shared::PlatformByteOrder::toCommonEndian(data.pingFrequency);
		data.pingTime = Shared::PlatformByteOrder::toCommonEndian(data.pingTime);
	}
}
void NetworkMessagePing::fromEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.messageType);
		data.pingFrequency = Shared::PlatformByteOrder::fromCommonEndian(data.pingFrequency);
		data.pingTime = Shared::PlatformByteOrder::fromCommonEndian(data.pingTime);
	}
}

// =====================================================
//	class NetworkMessageReady
// =====================================================

NetworkMessageReady::NetworkMessageReady() {
	data.messageType= nmtReady;
	data.checksum= 0;
}

NetworkMessageReady::NetworkMessageReady(uint32 checksum) {
	data.messageType= nmtReady;
	data.checksum= checksum;
}

const char * NetworkMessageReady::getPackedMessageFormat() const {
	return "cL";
}

unsigned int NetworkMessageReady::getPackedSize() {
	static unsigned int result = 0;
	if(result == 0) {
		Data packedData;
		packedData.checksum = 0;
		packedData.messageType = 0;
		unsigned char *buf = new unsigned char[sizeof(packedData)*3];
		result = pack(buf, getPackedMessageFormat(),
				packedData.messageType,
				packedData.checksum);
		delete [] buf;
	}
	return result;
}
void NetworkMessageReady::unpackMessage(unsigned char *buf) {
	unpack(buf, getPackedMessageFormat(),
			&data.messageType,
			&data.checksum);
}

unsigned char * NetworkMessageReady::packMessage() {
	unsigned char *buf = new unsigned char[getPackedSize()+1];
	pack(buf, getPackedMessageFormat(),
			data.messageType,
			data.checksum);
	return buf;
}

bool NetworkMessageReady::receive(Socket* socket){
	bool result = false;
	if(useOldProtocol == true) {
		result = NetworkMessage::receive(socket, &data, sizeof(data), true);
	}
	else {
		unsigned char *buf = new unsigned char[getPackedSize()+1];
		//bool result = NetworkMessage::receive(socket, buf, getPackedSize(), true);
		NetworkMessage::receive(socket, buf, getPackedSize(), true);
		unpackMessage(buf);
		//printf("Got packet size = %u data.messageType = %d\n%s\n",getPackedSize(),data.messageType,buf);
		delete [] buf;
	}
	fromEndian();
	return result;
}

void NetworkMessageReady::send(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtReady\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	assert(data.messageType==nmtReady);
	toEndian();

	if(useOldProtocol == true) {
		NetworkMessage::send(socket, &data, sizeof(data));
	}
	else {
		unsigned char *buf = packMessage();
		//printf("Send packet size = %u data.messageType = %d\n[%s]\n",getPackedSize(),data.messageType,buf);
		NetworkMessage::send(socket, buf, getPackedSize());
		delete [] buf;
	}
}

void NetworkMessageReady::toEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
		data.checksum = Shared::PlatformByteOrder::toCommonEndian(data.checksum);
	}
}
void NetworkMessageReady::fromEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.messageType);
		data.checksum = Shared::PlatformByteOrder::fromCommonEndian(data.checksum);
	}
}

// =====================================================
//	class NetworkMessageLaunch
// =====================================================

NetworkMessageLaunch::NetworkMessageLaunch() {
	data.messageType=-1;
	for(unsigned int i = 0; i < (unsigned int)maxFactionCRCCount; ++i) {
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

	for(unsigned int i = 0; i < (unsigned int)maxFactionCRCCount; ++i) {
		data.factionNameList[i] = "";
		data.factionCRCList[i] = 0;
	}

	vector<pair<string,uint32> > factionCRCList = gameSettings->getFactionCRCList();
	for(unsigned int i = 0; i < factionCRCList.size() && i < (unsigned int)maxFactionCRCCount; ++i) {
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
		data.networkPlayerPlatform[i]= gameSettings->getNetworkPlayerPlatform(i);
		data.networkPlayerStatuses[i] = gameSettings->getNetworkPlayerStatuses(i);
		data.networkPlayerLanguages[i] = gameSettings->getNetworkPlayerLanguages(i);
		data.factionControls[i]= gameSettings->getFactionControl(i);
		data.resourceMultiplierIndex[i]= gameSettings->getResourceMultiplierIndex(i);
		data.teams[i]= gameSettings->getTeam(i);
		data.startLocationIndex[i]= gameSettings->getStartLocationIndex(i);

		data.networkPlayerUUID[i] = gameSettings->getNetworkPlayerUUID(i);
		//printf("Build netmsg for index: %d [%s]\n",i,data.networkPlayerUUID[i].getString().c_str());
	}
	data.cpuReplacementMultiplier = gameSettings->getFallbackCpuMultiplier();
	data.aiAcceptSwitchTeamPercentChance = gameSettings->getAiAcceptSwitchTeamPercentChance();
	data.masterserver_admin = gameSettings->getMasterserver_admin();
	data.masterserver_admin_factionIndex = gameSettings->getMasterserver_admin_faction_index();

	data.scenario = gameSettings->getScenario();
	data.gameUUID = gameSettings->getGameUUID();

	data.networkAllowNativeLanguageTechtree = gameSettings->getNetworkAllowNativeLanguageTechtree();
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
	for(unsigned int i = 0; i < (unsigned int)maxFactionCRCCount; ++i) {
		if(data.factionNameList[i].getString() != "") {
			factionCRCList.push_back(make_pair(data.factionNameList[i].getString(),data.factionCRCList[i]));
		}
	}
	gameSettings->setFactionCRCList(factionCRCList);

	//for(int i= 0; i < data.factionCount; ++i) {
	for(int i= 0; i < GameConstants::maxPlayers; ++i) {
		gameSettings->setFactionTypeName(i, data.factionTypeNames[i].getString());
		gameSettings->setNetworkPlayerName(i,data.networkPlayerNames[i].getString());
		gameSettings->setNetworkPlayerPlatform(i,data.networkPlayerPlatform[i].getString());
		gameSettings->setNetworkPlayerStatuses(i, data.networkPlayerStatuses[i]);
		gameSettings->setNetworkPlayerLanguages(i, data.networkPlayerLanguages[i].getString());
		gameSettings->setFactionControl(i, static_cast<ControlType>(data.factionControls[i]));
		gameSettings->setResourceMultiplierIndex(i,data.resourceMultiplierIndex[i]);
		gameSettings->setTeam(i, data.teams[i]);
		gameSettings->setStartLocationIndex(i, data.startLocationIndex[i]);

		gameSettings->setNetworkPlayerUUID(i,data.networkPlayerUUID[i].getString());
		gameSettings->setNetworkPlayerPlatform(i,data.networkPlayerPlatform[i].getString());
		//printf("Build game settings for index: %d [%s]\n",i,data.networkPlayerUUID[i].getString().c_str());
	}

	gameSettings->setAiAcceptSwitchTeamPercentChance(data.aiAcceptSwitchTeamPercentChance);
	gameSettings->setFallbackCpuMultiplier(data.cpuReplacementMultiplier);

	gameSettings->setMasterserver_admin(data.masterserver_admin);
	gameSettings->setMasterserver_admin_faction_index(data.masterserver_admin_factionIndex);

	gameSettings->setScenario(data.scenario.getString());

	gameSettings->setGameUUID(data.gameUUID.getString());

	gameSettings->setNetworkAllowNativeLanguageTechtree((data.networkAllowNativeLanguageTechtree != 0));
}

vector<pair<string,uint32> > NetworkMessageLaunch::getFactionCRCList() const {

	vector<pair<string,uint32> > factionCRCList;
	for(unsigned int i = 0; i < (unsigned int)maxFactionCRCCount; ++i) {
		if(data.factionNameList[i].getString() != "") {
			factionCRCList.push_back(make_pair(data.factionNameList[i].getString(),data.factionCRCList[i]));
		}
	}
	return factionCRCList;
}

const char * NetworkMessageLaunch::getPackedMessageFormat() const {
	return "c256s60s60s60s60s60s60s60s60s60s60s60s60s60s60s60s60s60s60s60s60s60s60s60s60s60s60s60sllllllll60s60s60s60s60s60s60s60sLLL60s60s60s60s60s60s60s60s60s60s60s60s60s60s60s60s60s60s60s60sLLLLLLLLLLLLLLLLLLLLcccccccccccccccccccccccccccccccccccccccccCccLccll256s60s60s60s60s60s60s60s60sc60s";
}

unsigned int NetworkMessageLaunch::getPackedSize() {
	static unsigned int result = 0;
	if(result == 0) {
		Data packedData;
		packedData.aiAcceptSwitchTeamPercentChance = 0;
		packedData.allowObservers = 0;
		packedData.cpuReplacementMultiplier = 0;
		packedData.defaultResources = 0;
		packedData.defaultUnits = 0;
		packedData.defaultVictoryConditions = 0;
		packedData.enableObserverModeAtEndGame = 0;
		packedData.enableServerControlledAI = 0;
		for(unsigned int index =0; index < (unsigned int)maxFactionCRCCount; ++index) {
			packedData.factionCRCList[index] = 0;
		}
		for(unsigned int index =0; index < (unsigned int)GameConstants::maxPlayers; ++index) {
			packedData.factionControls[index] = 0;
			packedData.networkPlayerStatuses[index] = 0;
			packedData.resourceMultiplierIndex[index] = 0;
			packedData.startLocationIndex[index] = 0;
			packedData.teams[index] = 0;
		}
		packedData.factionCount = 0;
		packedData.flagTypes1 = 0;
		packedData.fogOfWar = 0;
		packedData.mapCRC = 0;
		packedData.masterserver_admin = 0;
		packedData.masterserver_admin_factionIndex = 0;
		packedData.messageType = 0;
		packedData.networkAllowNativeLanguageTechtree = 0;
		packedData.networkFramePeriod = 0;
		packedData.networkPauseGameForLaggedClients = 0;
		packedData.pathFinderType = 0;
		packedData.techCRC = 0;
		packedData.thisFactionIndex = 0;
		packedData.tilesetCRC = 0;

		unsigned char *buf = new unsigned char[sizeof(packedData)*3];
		result = pack(buf, getPackedMessageFormat(),
				packedData.messageType,
				packedData.description.getBuffer(),
				packedData.map.getBuffer(),
				packedData.tileset.getBuffer(),
				packedData.tech.getBuffer(),
				packedData.factionTypeNames[0].getBuffer(),
				packedData.factionTypeNames[1].getBuffer(),
				packedData.factionTypeNames[2].getBuffer(),
				packedData.factionTypeNames[3].getBuffer(),
				packedData.factionTypeNames[4].getBuffer(),
				packedData.factionTypeNames[5].getBuffer(),
				packedData.factionTypeNames[6].getBuffer(),
				packedData.factionTypeNames[7].getBuffer(),
				packedData.networkPlayerNames[0].getBuffer(),
				packedData.networkPlayerNames[1].getBuffer(),
				packedData.networkPlayerNames[2].getBuffer(),
				packedData.networkPlayerNames[3].getBuffer(),
				packedData.networkPlayerNames[4].getBuffer(),
				packedData.networkPlayerNames[5].getBuffer(),
				packedData.networkPlayerNames[6].getBuffer(),
				packedData.networkPlayerNames[7].getBuffer(),
				packedData.networkPlayerPlatform[0].getBuffer(),
				packedData.networkPlayerPlatform[1].getBuffer(),
				packedData.networkPlayerPlatform[2].getBuffer(),
				packedData.networkPlayerPlatform[3].getBuffer(),
				packedData.networkPlayerPlatform[4].getBuffer(),
				packedData.networkPlayerPlatform[5].getBuffer(),
				packedData.networkPlayerPlatform[6].getBuffer(),
				packedData.networkPlayerPlatform[7].getBuffer(),
				packedData.networkPlayerStatuses[0],
				packedData.networkPlayerStatuses[1],
				packedData.networkPlayerStatuses[2],
				packedData.networkPlayerStatuses[3],
				packedData.networkPlayerStatuses[4],
				packedData.networkPlayerStatuses[5],
				packedData.networkPlayerStatuses[6],
				packedData.networkPlayerStatuses[7],
				packedData.networkPlayerLanguages[0].getBuffer(),
				packedData.networkPlayerLanguages[1].getBuffer(),
				packedData.networkPlayerLanguages[2].getBuffer(),
				packedData.networkPlayerLanguages[3].getBuffer(),
				packedData.networkPlayerLanguages[4].getBuffer(),
				packedData.networkPlayerLanguages[5].getBuffer(),
				packedData.networkPlayerLanguages[6].getBuffer(),
				packedData.networkPlayerLanguages[7].getBuffer(),
				packedData.mapCRC,
				packedData.tilesetCRC,
				packedData.techCRC,
				packedData.factionNameList[0].getBuffer(),
				packedData.factionNameList[1].getBuffer(),
				packedData.factionNameList[2].getBuffer(),
				packedData.factionNameList[3].getBuffer(),
				packedData.factionNameList[4].getBuffer(),
				packedData.factionNameList[5].getBuffer(),
				packedData.factionNameList[6].getBuffer(),
				packedData.factionNameList[7].getBuffer(),
				packedData.factionNameList[8].getBuffer(),
				packedData.factionNameList[9].getBuffer(),
				packedData.factionNameList[10].getBuffer(),
				packedData.factionNameList[11].getBuffer(),
				packedData.factionNameList[12].getBuffer(),
				packedData.factionNameList[13].getBuffer(),
				packedData.factionNameList[14].getBuffer(),
				packedData.factionNameList[15].getBuffer(),
				packedData.factionNameList[16].getBuffer(),
				packedData.factionNameList[17].getBuffer(),
				packedData.factionNameList[18].getBuffer(),
				packedData.factionNameList[19].getBuffer(),
				packedData.factionCRCList[0],
				packedData.factionCRCList[1],
				packedData.factionCRCList[2],
				packedData.factionCRCList[3],
				packedData.factionCRCList[4],
				packedData.factionCRCList[5],
				packedData.factionCRCList[6],
				packedData.factionCRCList[7],
				packedData.factionCRCList[8],
				packedData.factionCRCList[9],
				packedData.factionCRCList[10],
				packedData.factionCRCList[11],
				packedData.factionCRCList[12],
				packedData.factionCRCList[13],
				packedData.factionCRCList[14],
				packedData.factionCRCList[15],
				packedData.factionCRCList[16],
				packedData.factionCRCList[17],
				packedData.factionCRCList[18],
				packedData.factionCRCList[19],
				packedData.factionControls[0],
				packedData.factionControls[1],
				packedData.factionControls[2],
				packedData.factionControls[3],
				packedData.factionControls[4],
				packedData.factionControls[5],
				packedData.factionControls[6],
				packedData.factionControls[7],
				packedData.resourceMultiplierIndex[0],
				packedData.resourceMultiplierIndex[1],
				packedData.resourceMultiplierIndex[2],
				packedData.resourceMultiplierIndex[3],
				packedData.resourceMultiplierIndex[4],
				packedData.resourceMultiplierIndex[5],
				packedData.resourceMultiplierIndex[6],
				packedData.resourceMultiplierIndex[7],
				packedData.thisFactionIndex,
				packedData.factionCount,
				packedData.teams[0],
				packedData.teams[1],
				packedData.teams[2],
				packedData.teams[3],
				packedData.teams[4],
				packedData.teams[5],
				packedData.teams[6],
				packedData.teams[7],
				packedData.startLocationIndex[0],
				packedData.startLocationIndex[1],
				packedData.startLocationIndex[2],
				packedData.startLocationIndex[3],
				packedData.startLocationIndex[4],
				packedData.startLocationIndex[5],
				packedData.startLocationIndex[6],
				packedData.startLocationIndex[7],
				packedData.defaultResources,
				packedData.defaultUnits,
				packedData.defaultVictoryConditions,
				packedData.fogOfWar,
				packedData.allowObservers,
				packedData.enableObserverModeAtEndGame,
				packedData.enableServerControlledAI,
				packedData.networkFramePeriod,
				packedData.networkPauseGameForLaggedClients,
				packedData.pathFinderType,
				packedData.flagTypes1,
				packedData.aiAcceptSwitchTeamPercentChance,
				packedData.cpuReplacementMultiplier,
				packedData.masterserver_admin,
				packedData.masterserver_admin_factionIndex,
				packedData.scenario.getBuffer(),
				packedData.networkPlayerUUID[0].getBuffer(),
				packedData.networkPlayerUUID[1].getBuffer(),
				packedData.networkPlayerUUID[2].getBuffer(),
				packedData.networkPlayerUUID[3].getBuffer(),
				packedData.networkPlayerUUID[4].getBuffer(),
				packedData.networkPlayerUUID[5].getBuffer(),
				packedData.networkPlayerUUID[6].getBuffer(),
				packedData.networkPlayerUUID[7].getBuffer(),
				packedData.networkAllowNativeLanguageTechtree,
				packedData.gameUUID.getBuffer()
		);
		delete [] buf;
	}
	return result;
}
void NetworkMessageLaunch::unpackMessage(unsigned char *buf) {
	unpack(buf, getPackedMessageFormat(),
			&data.messageType,
			data.description.getBuffer(),
			data.map.getBuffer(),
			data.tileset.getBuffer(),
			data.tech.getBuffer(),
			data.factionTypeNames[0].getBuffer(),
			data.factionTypeNames[1].getBuffer(),
			data.factionTypeNames[2].getBuffer(),
			data.factionTypeNames[3].getBuffer(),
			data.factionTypeNames[4].getBuffer(),
			data.factionTypeNames[5].getBuffer(),
			data.factionTypeNames[6].getBuffer(),
			data.factionTypeNames[7].getBuffer(),
			data.networkPlayerNames[0].getBuffer(),
			data.networkPlayerNames[1].getBuffer(),
			data.networkPlayerNames[2].getBuffer(),
			data.networkPlayerNames[3].getBuffer(),
			data.networkPlayerNames[4].getBuffer(),
			data.networkPlayerNames[5].getBuffer(),
			data.networkPlayerNames[6].getBuffer(),
			data.networkPlayerNames[7].getBuffer(),
			data.networkPlayerPlatform[0].getBuffer(),
			data.networkPlayerPlatform[1].getBuffer(),
			data.networkPlayerPlatform[2].getBuffer(),
			data.networkPlayerPlatform[3].getBuffer(),
			data.networkPlayerPlatform[4].getBuffer(),
			data.networkPlayerPlatform[5].getBuffer(),
			data.networkPlayerPlatform[6].getBuffer(),
			data.networkPlayerPlatform[7].getBuffer(),
			&data.networkPlayerStatuses[0],
			&data.networkPlayerStatuses[1],
			&data.networkPlayerStatuses[2],
			&data.networkPlayerStatuses[3],
			&data.networkPlayerStatuses[4],
			&data.networkPlayerStatuses[5],
			&data.networkPlayerStatuses[6],
			&data.networkPlayerStatuses[7],
			data.networkPlayerLanguages[0].getBuffer(),
			data.networkPlayerLanguages[1].getBuffer(),
			data.networkPlayerLanguages[2].getBuffer(),
			data.networkPlayerLanguages[3].getBuffer(),
			data.networkPlayerLanguages[4].getBuffer(),
			data.networkPlayerLanguages[5].getBuffer(),
			data.networkPlayerLanguages[6].getBuffer(),
			data.networkPlayerLanguages[7].getBuffer(),
			&data.mapCRC,
			&data.tilesetCRC,
			&data.techCRC,
			data.factionNameList[0].getBuffer(),
			data.factionNameList[1].getBuffer(),
			data.factionNameList[2].getBuffer(),
			data.factionNameList[3].getBuffer(),
			data.factionNameList[4].getBuffer(),
			data.factionNameList[5].getBuffer(),
			data.factionNameList[6].getBuffer(),
			data.factionNameList[7].getBuffer(),
			data.factionNameList[8].getBuffer(),
			data.factionNameList[9].getBuffer(),
			data.factionNameList[10].getBuffer(),
			data.factionNameList[11].getBuffer(),
			data.factionNameList[12].getBuffer(),
			data.factionNameList[13].getBuffer(),
			data.factionNameList[14].getBuffer(),
			data.factionNameList[15].getBuffer(),
			data.factionNameList[16].getBuffer(),
			data.factionNameList[17].getBuffer(),
			data.factionNameList[18].getBuffer(),
			data.factionNameList[19].getBuffer(),
			&data.factionCRCList[0],
			&data.factionCRCList[1],
			&data.factionCRCList[2],
			&data.factionCRCList[3],
			&data.factionCRCList[4],
			&data.factionCRCList[5],
			&data.factionCRCList[6],
			&data.factionCRCList[7],
			&data.factionCRCList[8],
			&data.factionCRCList[9],
			&data.factionCRCList[10],
			&data.factionCRCList[11],
			&data.factionCRCList[12],
			&data.factionCRCList[13],
			&data.factionCRCList[14],
			&data.factionCRCList[15],
			&data.factionCRCList[16],
			&data.factionCRCList[17],
			&data.factionCRCList[18],
			&data.factionCRCList[19],
			&data.factionControls[0],
			&data.factionControls[1],
			&data.factionControls[2],
			&data.factionControls[3],
			&data.factionControls[4],
			&data.factionControls[5],
			&data.factionControls[6],
			&data.factionControls[7],
			&data.resourceMultiplierIndex[0],
			&data.resourceMultiplierIndex[1],
			&data.resourceMultiplierIndex[2],
			&data.resourceMultiplierIndex[3],
			&data.resourceMultiplierIndex[4],
			&data.resourceMultiplierIndex[5],
			&data.resourceMultiplierIndex[6],
			&data.resourceMultiplierIndex[7],
			&data.thisFactionIndex,
			&data.factionCount,
			&data.teams[0],
			&data.teams[1],
			&data.teams[2],
			&data.teams[3],
			&data.teams[4],
			&data.teams[5],
			&data.teams[6],
			&data.teams[7],
			&data.startLocationIndex[0],
			&data.startLocationIndex[1],
			&data.startLocationIndex[2],
			&data.startLocationIndex[3],
			&data.startLocationIndex[4],
			&data.startLocationIndex[5],
			&data.startLocationIndex[6],
			&data.startLocationIndex[7],
			&data.defaultResources,
			&data.defaultUnits,
			&data.defaultVictoryConditions,
			&data.fogOfWar,
			&data.allowObservers,
			&data.enableObserverModeAtEndGame,
			&data.enableServerControlledAI,
			&data.networkFramePeriod,
			&data.networkPauseGameForLaggedClients,
			&data.pathFinderType,
			&data.flagTypes1,
			&data.aiAcceptSwitchTeamPercentChance,
			&data.cpuReplacementMultiplier,
			&data.masterserver_admin,
			&data.masterserver_admin_factionIndex,
			data.scenario.getBuffer(),
			data.networkPlayerUUID[0].getBuffer(),
			data.networkPlayerUUID[1].getBuffer(),
			data.networkPlayerUUID[2].getBuffer(),
			data.networkPlayerUUID[3].getBuffer(),
			data.networkPlayerUUID[4].getBuffer(),
			data.networkPlayerUUID[5].getBuffer(),
			data.networkPlayerUUID[6].getBuffer(),
			data.networkPlayerUUID[7].getBuffer(),
			&data.networkAllowNativeLanguageTechtree,
			data.gameUUID.getBuffer()
	);
}

unsigned char * NetworkMessageLaunch::packMessage() {
	unsigned char *buf = new unsigned char[getPackedSize()+1];
	pack(buf, getPackedMessageFormat(),
			data.messageType,
			data.description.getBuffer(),
			data.map.getBuffer(),
			data.tileset.getBuffer(),
			data.tech.getBuffer(),
			data.factionTypeNames[0].getBuffer(),
			data.factionTypeNames[1].getBuffer(),
			data.factionTypeNames[2].getBuffer(),
			data.factionTypeNames[3].getBuffer(),
			data.factionTypeNames[4].getBuffer(),
			data.factionTypeNames[5].getBuffer(),
			data.factionTypeNames[6].getBuffer(),
			data.factionTypeNames[7].getBuffer(),
			data.networkPlayerNames[0].getBuffer(),
			data.networkPlayerNames[1].getBuffer(),
			data.networkPlayerNames[2].getBuffer(),
			data.networkPlayerNames[3].getBuffer(),
			data.networkPlayerNames[4].getBuffer(),
			data.networkPlayerNames[5].getBuffer(),
			data.networkPlayerNames[6].getBuffer(),
			data.networkPlayerNames[7].getBuffer(),
			data.networkPlayerPlatform[0].getBuffer(),
			data.networkPlayerPlatform[1].getBuffer(),
			data.networkPlayerPlatform[2].getBuffer(),
			data.networkPlayerPlatform[3].getBuffer(),
			data.networkPlayerPlatform[4].getBuffer(),
			data.networkPlayerPlatform[5].getBuffer(),
			data.networkPlayerPlatform[6].getBuffer(),
			data.networkPlayerPlatform[7].getBuffer(),
			data.networkPlayerStatuses[0],
			data.networkPlayerStatuses[1],
			data.networkPlayerStatuses[2],
			data.networkPlayerStatuses[3],
			data.networkPlayerStatuses[4],
			data.networkPlayerStatuses[5],
			data.networkPlayerStatuses[6],
			data.networkPlayerStatuses[7],
			data.networkPlayerLanguages[0].getBuffer(),
			data.networkPlayerLanguages[1].getBuffer(),
			data.networkPlayerLanguages[2].getBuffer(),
			data.networkPlayerLanguages[3].getBuffer(),
			data.networkPlayerLanguages[4].getBuffer(),
			data.networkPlayerLanguages[5].getBuffer(),
			data.networkPlayerLanguages[6].getBuffer(),
			data.networkPlayerLanguages[7].getBuffer(),
			data.mapCRC,
			data.tilesetCRC,
			data.techCRC,
			data.factionNameList[0].getBuffer(),
			data.factionNameList[1].getBuffer(),
			data.factionNameList[2].getBuffer(),
			data.factionNameList[3].getBuffer(),
			data.factionNameList[4].getBuffer(),
			data.factionNameList[5].getBuffer(),
			data.factionNameList[6].getBuffer(),
			data.factionNameList[7].getBuffer(),
			data.factionNameList[8].getBuffer(),
			data.factionNameList[9].getBuffer(),
			data.factionNameList[10].getBuffer(),
			data.factionNameList[11].getBuffer(),
			data.factionNameList[12].getBuffer(),
			data.factionNameList[13].getBuffer(),
			data.factionNameList[14].getBuffer(),
			data.factionNameList[15].getBuffer(),
			data.factionNameList[16].getBuffer(),
			data.factionNameList[17].getBuffer(),
			data.factionNameList[18].getBuffer(),
			data.factionNameList[19].getBuffer(),
			data.factionCRCList[0],
			data.factionCRCList[1],
			data.factionCRCList[2],
			data.factionCRCList[3],
			data.factionCRCList[4],
			data.factionCRCList[5],
			data.factionCRCList[6],
			data.factionCRCList[7],
			data.factionCRCList[8],
			data.factionCRCList[9],
			data.factionCRCList[10],
			data.factionCRCList[11],
			data.factionCRCList[12],
			data.factionCRCList[13],
			data.factionCRCList[14],
			data.factionCRCList[15],
			data.factionCRCList[16],
			data.factionCRCList[17],
			data.factionCRCList[18],
			data.factionCRCList[19],
			data.factionControls[0],
			data.factionControls[1],
			data.factionControls[2],
			data.factionControls[3],
			data.factionControls[4],
			data.factionControls[5],
			data.factionControls[6],
			data.factionControls[7],
			data.resourceMultiplierIndex[0],
			data.resourceMultiplierIndex[1],
			data.resourceMultiplierIndex[2],
			data.resourceMultiplierIndex[3],
			data.resourceMultiplierIndex[4],
			data.resourceMultiplierIndex[5],
			data.resourceMultiplierIndex[6],
			data.resourceMultiplierIndex[7],
			data.thisFactionIndex,
			data.factionCount,
			data.teams[0],
			data.teams[1],
			data.teams[2],
			data.teams[3],
			data.teams[4],
			data.teams[5],
			data.teams[6],
			data.teams[7],
			data.startLocationIndex[0],
			data.startLocationIndex[1],
			data.startLocationIndex[2],
			data.startLocationIndex[3],
			data.startLocationIndex[4],
			data.startLocationIndex[5],
			data.startLocationIndex[6],
			data.startLocationIndex[7],
			data.defaultResources,
			data.defaultUnits,
			data.defaultVictoryConditions,
			data.fogOfWar,
			data.allowObservers,
			data.enableObserverModeAtEndGame,
			data.enableServerControlledAI,
			data.networkFramePeriod,
			data.networkPauseGameForLaggedClients,
			data.pathFinderType,
			data.flagTypes1,
			data.aiAcceptSwitchTeamPercentChance,
			data.cpuReplacementMultiplier,
			data.masterserver_admin,
			data.masterserver_admin_factionIndex,
			data.scenario.getBuffer(),
			data.networkPlayerUUID[0].getBuffer(),
			data.networkPlayerUUID[1].getBuffer(),
			data.networkPlayerUUID[2].getBuffer(),
			data.networkPlayerUUID[3].getBuffer(),
			data.networkPlayerUUID[4].getBuffer(),
			data.networkPlayerUUID[5].getBuffer(),
			data.networkPlayerUUID[6].getBuffer(),
			data.networkPlayerUUID[7].getBuffer(),
			data.networkAllowNativeLanguageTechtree,
			data.gameUUID.getBuffer()
			);
	return buf;
}

bool NetworkMessageLaunch::receive(Socket* socket) {
	//printf("Receive NetworkMessageLaunch\n");
	bool result = false;
	if(useOldProtocol == true) {
		result = NetworkMessage::receive(socket, &data, sizeof(data), true);
	}
	else {
		unsigned char *buf = new unsigned char[getPackedSize()+1];
		result = NetworkMessage::receive(socket, buf, getPackedSize(), true);
		unpackMessage(buf);
		//printf("Got packet size = %u data.messageType = %d\n%s\n",getPackedSize(),data.messageType,buf);
		delete [] buf;
	}
	fromEndian();

	data.description.nullTerminate();
	data.map.nullTerminate();
	data.tileset.nullTerminate();
	data.tech.nullTerminate();
	for(int i= 0; i < GameConstants::maxPlayers; ++i){
		data.factionTypeNames[i].nullTerminate();
		data.networkPlayerNames[i].nullTerminate();
		data.networkPlayerPlatform[i].nullTerminate();
		data.networkPlayerLanguages[i].nullTerminate();

		data.networkPlayerUUID[i].nullTerminate();
	}
	for(unsigned int i = 0; i < (unsigned int)maxFactionCRCCount; ++i) {
		data.factionNameList[i].nullTerminate();
	}

	data.scenario.nullTerminate();

	data.gameUUID.nullTerminate();

	//for(int i= 0; i < GameConstants::maxPlayers; ++i){
	//	printf("Receive index: %d resource multiplier index: %d sizeof(data): %d\n",i,data.resourceMultiplierIndex[i],sizeof(data));
	//}

	return result;
}

void NetworkMessageLaunch::send(Socket* socket) {
	//printf("Sending NetworkMessageLaunch\n");

	//for(int i= 0; i < GameConstants::maxPlayers; ++i){
	//	printf("Send index: %d resource multiplier index: %d sizeof(data): %d\n",i,data.resourceMultiplierIndex[i],sizeof(data));
	//}

	if(data.messageType == nmtLaunch) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtLaunch\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] messageType = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,data.messageType);
	}
	toEndian();

	if(useOldProtocol == true) {
		NetworkMessage::send(socket, &data, sizeof(data));
	}
	else {
		unsigned char *buf = packMessage();
		//printf("Send packet size = %u data.messageType = %d\n[%s]\n",getPackedSize(),data.messageType,buf);
		NetworkMessage::send(socket, buf, getPackedSize());
		delete [] buf;
	}
}

void NetworkMessageLaunch::toEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
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

		data.networkAllowNativeLanguageTechtree = Shared::PlatformByteOrder::toCommonEndian(data.networkAllowNativeLanguageTechtree);
	}
}

void NetworkMessageLaunch::fromEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
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

		data.networkAllowNativeLanguageTechtree = Shared::PlatformByteOrder::fromCommonEndian(data.networkAllowNativeLanguageTechtree);
	}
}

// =====================================================
//	class NetworkMessageLaunch
// =====================================================

NetworkMessageCommandList::NetworkMessageCommandList(int32 frameCount) {
	data.header.messageType= nmtCommandList;
	data.header.frameCount= frameCount;
	data.header.commandCount= 0;
	for(int index = 0; index < GameConstants::maxPlayers; ++index) {
		data.header.networkPlayerFactionCRC[index]=0;
	}
}

bool NetworkMessageCommandList::addCommand(const NetworkCommand* networkCommand){
	data.commands.push_back(*networkCommand);
	data.header.commandCount++;
	return true;
}

const char * NetworkMessageCommandList::getPackedMessageFormatHeader() const {
	return "cHlLLLLLLLL";
}

unsigned int NetworkMessageCommandList::getPackedSizeHeader() {
	static unsigned int result = 0;
	if(result == 0) {
		Data packedData;
		init(packedData);
		unsigned char *buf = new unsigned char[sizeof(packedData)*3];
		result = pack(buf, getPackedMessageFormatHeader(),
				packedData.header.messageType,
				packedData.header.commandCount,
				packedData.header.frameCount,
				packedData.header.networkPlayerFactionCRC[0],
				packedData.header.networkPlayerFactionCRC[1],
				packedData.header.networkPlayerFactionCRC[2],
				packedData.header.networkPlayerFactionCRC[3],
				packedData.header.networkPlayerFactionCRC[4],
				packedData.header.networkPlayerFactionCRC[5],
				packedData.header.networkPlayerFactionCRC[6],
				packedData.header.networkPlayerFactionCRC[7]);
		delete [] buf;
	}
	return result;
}
void NetworkMessageCommandList::unpackMessageHeader(unsigned char *buf) {
	unpack(buf, getPackedMessageFormatHeader(),
			&data.header.messageType,
			&data.header.commandCount,
			&data.header.frameCount,
			&data.header.networkPlayerFactionCRC[0],
			&data.header.networkPlayerFactionCRC[1],
			&data.header.networkPlayerFactionCRC[2],
			&data.header.networkPlayerFactionCRC[3],
			&data.header.networkPlayerFactionCRC[4],
			&data.header.networkPlayerFactionCRC[5],
			&data.header.networkPlayerFactionCRC[6],
			&data.header.networkPlayerFactionCRC[7]);
}

unsigned char * NetworkMessageCommandList::packMessageHeader() {
	unsigned char *buf = new unsigned char[getPackedSizeHeader()+1];
	pack(buf, getPackedMessageFormatHeader(),
			data.header.messageType,
			data.header.commandCount,
			data.header.frameCount,
			data.header.networkPlayerFactionCRC[0],
			data.header.networkPlayerFactionCRC[1],
			data.header.networkPlayerFactionCRC[2],
			data.header.networkPlayerFactionCRC[3],
			data.header.networkPlayerFactionCRC[4],
			data.header.networkPlayerFactionCRC[5],
			data.header.networkPlayerFactionCRC[6],
			data.header.networkPlayerFactionCRC[7]);
	return buf;
}

const char * NetworkMessageCommandList::getPackedMessageFormatDetail() const {
	return "hlhhhhlccHccll";
}

unsigned int NetworkMessageCommandList::getPackedSizeDetail(int count) {
	unsigned int result = 0;
	if(result == 0) {
		for(unsigned int i = 0; i < (unsigned int)count; ++i) {
			NetworkCommand packedData;
			unsigned char *buf = new unsigned char[sizeof(NetworkCommand)*3];
			result += pack(buf, getPackedMessageFormatDetail(),
					packedData.networkCommandType,
					packedData.unitId,
					packedData.unitTypeId,
					packedData.commandTypeId,
					packedData.positionX,
					packedData.positionY,
					packedData.targetId,
					packedData.wantQueue,
					packedData.fromFactionIndex,
					packedData.unitFactionUnitCount,
					packedData.unitFactionIndex,
					packedData.commandStateType,
					packedData.commandStateValue,
					packedData.unitCommandGroupId);
			delete [] buf;
		}
	}
	return result;
}
void NetworkMessageCommandList::unpackMessageDetail(unsigned char *buf,int count) {
	data.commands.clear();
	data.commands.resize(count);
	//unsigned int bytes_processed_total = 0;
	unsigned char *bufMove = buf;
	for(unsigned int i = 0; i < (unsigned int)count; ++i) {
		unsigned int bytes_processed = unpack(bufMove, getPackedMessageFormatDetail(),
				&data.commands[i].networkCommandType,
				&data.commands[i].unitId,
				&data.commands[i].unitTypeId,
				&data.commands[i].commandTypeId,
				&data.commands[i].positionX,
				&data.commands[i].positionY,
				&data.commands[i].targetId,
				&data.commands[i].wantQueue,
				&data.commands[i].fromFactionIndex,
				&data.commands[i].unitFactionUnitCount,
				&data.commands[i].unitFactionIndex,
				&data.commands[i].commandStateType,
				&data.commands[i].commandStateValue,
				&data.commands[i].unitCommandGroupId);
		bufMove += bytes_processed;
		//bytes_processed_total += bytes_processed;
	}
	//printf("\nUnPacked detail size = %u\n",bytes_processed_total);
}

unsigned char * NetworkMessageCommandList::packMessageDetail(uint16 totalCommand) {
	int packetSize = getPackedSizeDetail(totalCommand) +1;
	unsigned char *buf = new unsigned char[packetSize];
	unsigned char *bufMove = buf;
	//unsigned int bytes_processed_total = 0;
	for(unsigned int i = 0; i < totalCommand; ++i) {
		unsigned int bytes_processed = pack(bufMove, getPackedMessageFormatDetail(),
				data.commands[i].networkCommandType,
				data.commands[i].unitId,
				data.commands[i].unitTypeId,
				data.commands[i].commandTypeId,
				data.commands[i].positionX,
				data.commands[i].positionY,
				data.commands[i].targetId,
				data.commands[i].wantQueue,
				data.commands[i].fromFactionIndex,
				data.commands[i].unitFactionUnitCount,
				data.commands[i].unitFactionIndex,
				data.commands[i].commandStateType,
				data.commands[i].commandStateValue,
				data.commands[i].unitCommandGroupId);
		bufMove += bytes_processed;
		//bytes_processed_total += bytes_processed;
	}
	//printf("\nPacked detail size = %u, allocated = %d\n",bytes_processed_total,packetSize);
	return buf;
}

bool NetworkMessageCommandList::receive(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	unsigned char *buf = NULL;
	bool result = false;
	if(useOldProtocol == true) {
		result = NetworkMessage::receive(socket, &data.header, commandListHeaderSize, true);
		//printf("!!! =====> IN Network hdr cmd get frame: %d data.header.commandCount: %u\n",data.header.frameCount,data.header.commandCount);
	}
	else {
		buf = new unsigned char[getPackedSizeHeader()+1];
		result = NetworkMessage::receive(socket, buf, getPackedSizeHeader(), true);
		unpackMessageHeader(buf);
		//if(data.header.commandCount) printf("\n\nGot packet size = %u data.messageType = %d\n%s\ncommandcount [%u] framecount [%d]\n",getPackedSizeHeader(),data.header.messageType,buf,data.header.commandCount,data.header.frameCount);
		delete [] buf;
	}
	fromEndianHeader();

	if(result == true) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got header, messageType = %d, commandCount = %u, frameCount = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,data.header.messageType,data.header.commandCount,data.header.frameCount);

		//printf("!!! =====> IN Network cmd get frame: %d data.header.commandCount: %u\n",data.header.frameCount,data.header.commandCount);

		if(data.header.commandCount > 0) {
			data.commands.resize(data.header.commandCount);

			if(useOldProtocol == true) {
				int totalMsgSize = (sizeof(NetworkCommand) * data.header.commandCount);
				result = NetworkMessage::receive(socket, &data.commands[0], totalMsgSize, true);

//				if(data.commands[0].getNetworkCommandType() == nctPauseResume) {
//					printf("=====> IN Network cmd type: %d [%d] frame: %d\n",data.commands[0].getNetworkCommandType(),nctPauseResume,data.header.frameCount);
//				}
			}
			else {
				//int totalMsgSize = (sizeof(NetworkCommand) * data.header.commandCount);
				//result = NetworkMessage::receive(socket, &data.commands[0], totalMsgSize, true);
				buf = new unsigned char[getPackedSizeDetail(data.header.commandCount)+1];
				result = NetworkMessage::receive(socket, buf, getPackedSizeDetail(data.header.commandCount), true);
				unpackMessageDetail(buf,data.header.commandCount);
				//printf("Got packet size = %u data.messageType = %d\n%s\n",getPackedSize(),data.messageType,buf);
				delete [] buf;
			}
			fromEndianDetail();

//	        for(int idx = 0 ; idx < data.header.commandCount; ++idx) {
//	            const NetworkCommand &cmd = data.commands[idx];
//	            printf("========> Got index = %d / %u, got networkCommand [%s]\n",idx, data.header.commandCount,cmd.toString().c_str());
//	        }

			if(result == true) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled == true) {
					for(int idx = 0 ; idx < data.header.commandCount; ++idx) {
						const NetworkCommand &cmd = data.commands[idx];

						SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] index = %d, received networkCommand [%s]\n",
								extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,idx, cmd.toString().c_str());
					}
				}
			}
			else {
				//if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ERROR Failed to get command data, totalMsgSize = %d.\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,totalMsgSize);
			}
		}
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ERROR header not received as expected\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	    SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] ERROR header not received as expected\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	}
	return result;

}

void NetworkMessageCommandList::send(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtCommandList, frameCount = %d, data.header.commandCount = %d, data.header.messageType = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,data.header.frameCount,data.header.commandCount,data.header.messageType);

	assert(data.header.messageType==nmtCommandList);
	uint16 totalCommand = data.header.commandCount;
	toEndianHeader();

	unsigned char *buf = NULL;
	//bool result = false;
	if(useOldProtocol == true) {
		//printf("<===== OUT Network hdr cmd type: frame: %d totalCommand: %u [%u]\n",data.header.frameCount,totalCommand,data.header.commandCount);
		NetworkMessage::send(socket, &data.header, commandListHeaderSize);
	}
	else {
		//NetworkMessage::send(socket, &data.header, commandListHeaderSize);
		buf = packMessageHeader();
		//if(totalCommand) printf("\n\nSend packet size = %u data.messageType = %d\n%s\ncommandcount [%u] framecount [%d]\n",getPackedSizeHeader(),data.header.messageType,buf,totalCommand,data.header.frameCount);
		NetworkMessage::send(socket, buf, getPackedSizeHeader());
		delete [] buf;
	}

	if(totalCommand > 0) {
		//printf("\n#2 Send packet commandcount [%u] framecount [%d]\n",totalCommand,data.header.frameCount);
		toEndianDetail(totalCommand);
		//printf("\n#3 Send packet commandcount [%u] framecount [%d]\n",totalCommand,data.header.frameCount);

		//bool result = false;
		if(useOldProtocol == true) {
//			if(data.commands[0].getNetworkCommandType() == nctPauseResume) {
//				printf("<===== OUT Network cmd type: %d [%d] frame: %d totalCommand: %u [%u]\n",data.commands[0].getNetworkCommandType(),nctPauseResume,data.header.frameCount,totalCommand,data.header.commandCount);
//			}
			NetworkMessage::send(socket, &data.commands[0], (sizeof(NetworkCommand) * totalCommand));
		}
		else {
			buf = packMessageDetail(totalCommand);
			//printf("\n#4 Send packet commandcount [%u] framecount [%d]\n",totalCommand,data.header.frameCount);
			//printf("Send packet size = %u data.messageType = %d\n[%s]\n",getPackedSize(),data.messageType,buf);
			NetworkMessage::send(socket, buf, getPackedSizeDetail(totalCommand));
			//printf("\n#5 Send packet commandcount [%u] framecount [%d]\n",totalCommand,data.header.frameCount);
			delete [] buf;
			//printf("\n#6 Send packet commandcount [%u] framecount [%d]\n",totalCommand,data.header.frameCount);

	//        for(int idx = 0 ; idx < totalCommand; ++idx) {
	//            const NetworkCommand &cmd = data.commands[idx];
	//            printf("========> Send index = %d / %u, sent networkCommand [%s]\n",idx, totalCommand,cmd.toString().c_str());
	//        }
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled == true) {
	    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] messageType = %d, frameCount = %d, data.commandCount = %d\n",
                extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,data.header.messageType,data.header.frameCount,data.header.commandCount);

        if (totalCommand > 0) {
            for(int idx = 0 ; idx < totalCommand; ++idx) {
                const NetworkCommand &cmd = data.commands[idx];

                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] index = %d, sent networkCommand [%s]\n",
                        extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,idx, cmd.toString().c_str());
            }

            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] END of loop, nmtCommandList, frameCount = %d, data.header.commandCount = %d, data.header.messageType = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,data.header.frameCount,totalCommand,data.header.messageType);
        }
	}
}

void NetworkMessageCommandList::toEndianHeader() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.header.messageType = Shared::PlatformByteOrder::toCommonEndian(data.header.messageType);
		data.header.commandCount = Shared::PlatformByteOrder::toCommonEndian(data.header.commandCount);
		data.header.frameCount = Shared::PlatformByteOrder::toCommonEndian(data.header.frameCount);
		for(int index = 0; index < GameConstants::maxPlayers; ++index) {
			data.header.networkPlayerFactionCRC[index] = Shared::PlatformByteOrder::toCommonEndian(data.header.networkPlayerFactionCRC[index]);
		}
	}
}
void NetworkMessageCommandList::fromEndianHeader() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.header.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.header.messageType);
		data.header.commandCount = Shared::PlatformByteOrder::fromCommonEndian(data.header.commandCount);
		data.header.frameCount = Shared::PlatformByteOrder::fromCommonEndian(data.header.frameCount);
		for(int index = 0; index < GameConstants::maxPlayers; ++index) {
			data.header.networkPlayerFactionCRC[index] = Shared::PlatformByteOrder::fromCommonEndian(data.header.networkPlayerFactionCRC[index]);
		}
	}
}

void NetworkMessageCommandList::toEndianDetail(uint16 totalCommand) {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		if(totalCommand > 0) {
			for(int idx = 0 ; idx < totalCommand; ++idx) {
				NetworkCommand &cmd = data.commands[idx];
				cmd.toEndian();
			}
		}
	}
}

void NetworkMessageCommandList::fromEndianDetail() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		if(data.header.commandCount > 0) {
			for(int idx = 0 ; idx < data.header.commandCount; ++idx) {
				NetworkCommand &cmd = data.commands[idx];
				cmd.fromEndian();
			}
		}
	}
}

// =====================================================
//	class NetworkMessageText
// =====================================================

NetworkMessageText::NetworkMessageText(const string &text, int teamIndex, int playerIndex,
										const string targetLanguage) {
	if((int)text.length() >= maxTextStringSize) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] WARNING / ERROR - text [%s] length = %d, max = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,text.c_str(),text.length(),maxTextStringSize);
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

const char * NetworkMessageText::getPackedMessageFormat() const {
	return "c500scc60s";
}

unsigned int NetworkMessageText::getPackedSize() {
	static unsigned int result = 0;
	if(result == 0) {
		Data packedData;
		packedData.messageType = nmtText;
		packedData.playerIndex = 0;
		packedData.teamIndex = 0;
		unsigned char *buf = new unsigned char[sizeof(packedData)*3];
		result = pack(buf, getPackedMessageFormat(),
				packedData.messageType,
				packedData.text.getBuffer(),
				packedData.teamIndex,
				packedData.playerIndex,
				packedData.targetLanguage.getBuffer());
		delete [] buf;
	}
	return result;
}
void NetworkMessageText::unpackMessage(unsigned char *buf) {
	unpack(buf, getPackedMessageFormat(),
			&data.messageType,
			data.text.getBuffer(),
			&data.teamIndex,
			&data.playerIndex,
			data.targetLanguage.getBuffer());
}

unsigned char * NetworkMessageText::packMessage() {
	unsigned char *buf = new unsigned char[getPackedSize()+1];
	pack(buf, getPackedMessageFormat(),
			data.messageType,
			data.text.getBuffer(),
			data.teamIndex,
			data.playerIndex,
			data.targetLanguage.getBuffer());
	return buf;
}

bool NetworkMessageText::receive(Socket* socket) {
	bool result = false;
	if(useOldProtocol == true) {
		result = NetworkMessage::receive(socket, &data, sizeof(data), true);
	}
	else {
		unsigned char *buf = new unsigned char[getPackedSize()+1];
		result = NetworkMessage::receive(socket, buf, getPackedSize(), true);
		unpackMessage(buf);
		//printf("Got packet size = %u data.messageType = %d\n%s\n",getPackedSize(),data.messageType,buf);
		delete [] buf;
	}
	fromEndian();

	data.text.nullTerminate();
	data.targetLanguage.nullTerminate();

	return result;
}

void NetworkMessageText::send(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtText\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	assert(data.messageType==nmtText);
	toEndian();

	if(useOldProtocol == true) {
		NetworkMessage::send(socket, &data, sizeof(data));
	}
	else {
		unsigned char *buf = packMessage();
		//printf("Send packet size = %u data.messageType = %d\n[%s]\n",getPackedSize(),data.messageType,buf);
		NetworkMessage::send(socket, buf, getPackedSize());
		delete [] buf;
	}
}

void NetworkMessageText::toEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
		data.teamIndex = Shared::PlatformByteOrder::toCommonEndian(data.teamIndex);
		data.playerIndex = Shared::PlatformByteOrder::toCommonEndian(data.playerIndex);
	}
}
void NetworkMessageText::fromEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.messageType);
		data.teamIndex = Shared::PlatformByteOrder::fromCommonEndian(data.teamIndex);
		data.playerIndex = Shared::PlatformByteOrder::fromCommonEndian(data.playerIndex);
	}
}

// =====================================================
//	class NetworkMessageQuit
// =====================================================

NetworkMessageQuit::NetworkMessageQuit(){
	data.messageType= nmtQuit;
}

const char * NetworkMessageQuit::getPackedMessageFormat() const {
	return "c";
}

unsigned int NetworkMessageQuit::getPackedSize() {
	static unsigned int result = 0;
	if(result == 0) {
		Data packedData;
		packedData.messageType = 0;
		unsigned char *buf = new unsigned char[sizeof(packedData)*3];
		result = pack(buf, getPackedMessageFormat(),
				packedData.messageType);
		delete [] buf;
	}
	return result;
}
void NetworkMessageQuit::unpackMessage(unsigned char *buf) {
	unpack(buf, getPackedMessageFormat(),
			&data.messageType);
}

unsigned char * NetworkMessageQuit::packMessage() {
	unsigned char *buf = new unsigned char[getPackedSize()+1];
	pack(buf, getPackedMessageFormat(),
			data.messageType);
	return buf;
}

bool NetworkMessageQuit::receive(Socket* socket) {
	bool result = false;
	if(useOldProtocol == true) {
		result = NetworkMessage::receive(socket, &data, sizeof(data),true);
	}
	else {
		//fromEndian();
		unsigned char *buf = new unsigned char[getPackedSize()+1];
		result = NetworkMessage::receive(socket, buf, getPackedSize(), true);
		unpackMessage(buf);
		//printf("Got packet size = %u data.messageType = %d\n%s\n",getPackedSize(),data.messageType,buf);
		delete [] buf;
	}
	fromEndian();

	return result;
}

void NetworkMessageQuit::send(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtQuit\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	assert(data.messageType==nmtQuit);
	toEndian();

	if(useOldProtocol == true) {
		NetworkMessage::send(socket, &data, sizeof(data));
	}
	else {
		unsigned char *buf = packMessage();
		//printf("Send packet size = %u data.messageType = %d\n[%s]\n",getPackedSize(),data.messageType,buf);
		NetworkMessage::send(socket, buf, getPackedSize());
		delete [] buf;
	}
}

void NetworkMessageQuit::toEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
	}
}
void NetworkMessageQuit::fromEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.messageType);
	}
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

        if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] gameSettings.getScenarioDir() = [%s] gameSettings.getScenario() = [%s] scenarioDir = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,gameSettings->getScenarioDir().c_str(),gameSettings->getScenario().c_str(),scenarioDir.c_str());
    }

    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

    data.header.tilesetCRC = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTilesets,scenarioDir), string("/") + gameSettings->getTileset() + string("/*"), ".xml", NULL);

    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] data.tilesetCRC = %d, [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__, data.header.tilesetCRC,gameSettings->getTileset().c_str());

    //tech, load before map because of resources
	data.header.techCRC = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,scenarioDir), string("/") + gameSettings->getTech() + string("/*"), ".xml", NULL);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] data.techCRC = %d, [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__, data.header.techCRC,gameSettings->getTech().c_str());

	vector<std::pair<string,uint32> > vctFileList;
	vctFileList = getFolderTreeContentsCheckSumListRecursively(config.getPathListForType(ptTechs,scenarioDir),string("/") + gameSettings->getTech() + string("/*"), ".xml",&vctFileList);
	data.header.techCRCFileCount = min((int)vctFileList.size(),(int)maxFileCRCCount);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] vctFileList.size() = %d, maxFileCRCCount = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__, vctFileList.size(),maxFileCRCCount);

	for(int idx =0; idx < (int)data.header.techCRCFileCount; ++idx) {
		const std::pair<string,uint32> &fileInfo = vctFileList[idx];
		data.detail.techCRCFileList[idx] = fileInfo.first;
		data.detail.techCRCFileCRCList[idx] = fileInfo.second;
	}

    //map
    Checksum checksum;
    string file = Config::getMapPath(gameSettings->getMap(),scenarioDir,false);
	checksum.addFile(file);
	data.header.mapCRC = checksum.getSum();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] data.mapCRC = %d, [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__, data.header.mapCRC,gameSettings->getMap().c_str());
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
		for(int idx = 0; idx < (int)vctFileList.size(); ++idx) {
			std::pair<string,uint32> &fileInfo = vctFileList[idx];
			bool fileFound = false;
			uint32 remoteCRC = 0;
			for(int j = 0; j < (int)data.header.techCRCFileCount; ++j) {
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

		for(int i = 0; i < (int)data.header.techCRCFileCount; ++i) {
			string networkFile = data.detail.techCRCFileList[i].getString();
			uint32 &networkFileCRC = data.detail.techCRCFileCRCList[i];
			bool fileFound = false;
			uint32 localCRC = 0;
			for(int idx = 0; idx < (int)vctFileList.size(); ++idx) {
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

const char * NetworkMessageSynchNetworkGameData::getPackedMessageFormatHeader() const {
	return "c255s255s255sLLLL";
}

unsigned int NetworkMessageSynchNetworkGameData::getPackedSizeHeader() {
	static unsigned int result = 0;
	if(result == 0) {
		Data packedData;
		unsigned char *buf = new unsigned char[sizeof(packedData)*3];
		result = pack(buf, getPackedMessageFormatHeader(),
				packedData.header.messageType,
				packedData.header.map.getBuffer(),
				packedData.header.tileset.getBuffer(),
				packedData.header.tech.getBuffer(),
				packedData.header.mapCRC,
				packedData.header.tilesetCRC,
				packedData.header.techCRC,
				packedData.header.techCRCFileCount);
		delete [] buf;
	}
	return result;
}
void NetworkMessageSynchNetworkGameData::unpackMessageHeader(unsigned char *buf) {
	unpack(buf, getPackedMessageFormatHeader(),
			&data.header.messageType,
			data.header.map.getBuffer(),
			data.header.tileset.getBuffer(),
			data.header.tech.getBuffer(),
			&data.header.mapCRC,
			&data.header.tilesetCRC,
			&data.header.techCRC,
			&data.header.techCRCFileCount);
}

unsigned char * NetworkMessageSynchNetworkGameData::packMessageHeader() {
	unsigned char *buf = new unsigned char[getPackedSizeHeader()+1];
	pack(buf, getPackedMessageFormatHeader(),
			data.header.messageType,
			data.header.map.getBuffer(),
			data.header.tileset.getBuffer(),
			data.header.tech.getBuffer(),
			data.header.mapCRC,
			data.header.tilesetCRC,
			data.header.techCRC,
			data.header.techCRCFileCount);

	return buf;
}

unsigned int NetworkMessageSynchNetworkGameData::getPackedSizeDetail() {
	static unsigned int result = 0;
	if(result == 0) {
		DataDetail packedData;
		for(unsigned int index = 0; index < (unsigned int)maxFileCRCCount; ++index) {
			packedData.techCRCFileCRCList[index] = 0;
		}
		unsigned char *buf = new unsigned char[sizeof(DataDetail)*3];

		for(unsigned int i = 0; i < (unsigned int)maxFileCRCCount; ++i) {
			result += pack(buf, "255s",
					packedData.techCRCFileList[i].getBuffer());
			buf += result;
		}
		for(unsigned int i = 0; i < (unsigned int)maxFileCRCCount; ++i) {
			result += pack(buf, "L",
					packedData.techCRCFileCRCList[i]);
			buf += result;
		}

		delete [] buf;
	}
	return result;
}
void NetworkMessageSynchNetworkGameData::unpackMessageDetail(unsigned char *buf) {
	for(unsigned int i = 0; i < (unsigned int)maxFileCRCCount; ++i) {
		unsigned int bytes_processed = unpack(buf, "255s",
				data.detail.techCRCFileList[i].getBuffer());
		buf += bytes_processed;
	}
	for(unsigned int i = 0; i < (unsigned int)maxFileCRCCount; ++i) {
		unsigned int bytes_processed = unpack(buf, "L",
				&data.detail.techCRCFileCRCList[i]);
		buf += bytes_processed;
	}
}

unsigned char * NetworkMessageSynchNetworkGameData::packMessageDetail() {
	unsigned char *buf = new unsigned char[sizeof(DataDetail)*3 +1];
	unsigned char *bufMove = buf;
	for(unsigned int i = 0; i < (unsigned int)maxFileCRCCount; ++i) {
		unsigned int bytes_processed = pack(bufMove, "255s",
				data.detail.techCRCFileList[i].getBuffer());
		bufMove += bytes_processed;
	}
	for(unsigned int i = 0; i < (unsigned int)maxFileCRCCount; ++i) {
		unsigned int bytes_processed = pack(bufMove, "L",
				data.detail.techCRCFileCRCList[i]);
		bufMove += bytes_processed;
	}

	return buf;
}


bool NetworkMessageSynchNetworkGameData::receive(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] about to get nmtSynchNetworkGameData\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	data.header.techCRCFileCount = 0;
	bool result = NetworkMessage::receive(socket, &data, HeaderSize, true);
	fromEndianHeader();
	if(result == true && data.header.techCRCFileCount > 0) {
		data.header.map.nullTerminate();
		data.header.tileset.nullTerminate();
		data.header.tech.nullTerminate();

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] messageType = %d, data.techCRCFileCount = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,data.header.messageType,data.header.techCRCFileCount);



		// Here we loop possibly multiple times
		int packetLoopCount = 1;
		if(data.header.techCRCFileCount > (uint32)NetworkMessageSynchNetworkGameData::maxFileCRCPacketCount) {
			packetLoopCount = (data.header.techCRCFileCount / NetworkMessageSynchNetworkGameData::maxFileCRCPacketCount);
			if(data.header.techCRCFileCount % NetworkMessageSynchNetworkGameData::maxFileCRCPacketCount > 0) {
				packetLoopCount++;
			}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] packetLoopCount = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,packetLoopCount);

		for(int iPacketLoop = 0; result == true && iPacketLoop < packetLoopCount; ++iPacketLoop) {

			int packetIndex = iPacketLoop * NetworkMessageSynchNetworkGameData::maxFileCRCPacketCount;
			int maxFileCountPerPacket = maxFileCRCPacketCount;
			int packetFileCount = min((uint32)maxFileCountPerPacket,data.header.techCRCFileCount - packetIndex);
			int packetDetail1DataSize = (DetailSize1 * packetFileCount);
			int packetDetail2DataSize = (DetailSize2 * packetFileCount);

			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] iPacketLoop = %d, packetIndex = %d, maxFileCountPerPacket = %d, packetFileCount = %d, packetDetail1DataSize = %d, packetDetail2DataSize = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,iPacketLoop,packetIndex,maxFileCountPerPacket,packetFileCount,packetDetail1DataSize,packetDetail2DataSize);

            // Wait a max of x seconds for this message
			result = NetworkMessage::receive(socket, &data.detail.techCRCFileList[packetIndex], packetDetail1DataSize, true);
			if(result == true) {
				for(unsigned int i = 0; i < data.header.techCRCFileCount; ++i) {
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
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] about to send nmtSynchNetworkGameData\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	assert(data.header.messageType==nmtSynchNetworkGameData);
	uint32 totalFileCount = data.header.techCRCFileCount;
	toEndianHeader();
	NetworkMessage::send(socket, &data, HeaderSize);
	if(totalFileCount > 0) {
		// Here we loop possibly multiple times
		int packetLoopCount = 1;
		if(totalFileCount > (uint32)NetworkMessageSynchNetworkGameData::maxFileCRCPacketCount) {
			packetLoopCount = (totalFileCount / NetworkMessageSynchNetworkGameData::maxFileCRCPacketCount);
			if(totalFileCount % NetworkMessageSynchNetworkGameData::maxFileCRCPacketCount > 0) {
				packetLoopCount++;
			}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] packetLoopCount = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,packetLoopCount);

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
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.header.messageType = Shared::PlatformByteOrder::toCommonEndian(data.header.messageType);
		data.header.mapCRC = Shared::PlatformByteOrder::toCommonEndian(data.header.mapCRC);
		data.header.tilesetCRC = Shared::PlatformByteOrder::toCommonEndian(data.header.tilesetCRC);
		data.header.techCRC = Shared::PlatformByteOrder::toCommonEndian(data.header.techCRC);
		data.header.techCRCFileCount = Shared::PlatformByteOrder::toCommonEndian(data.header.techCRCFileCount);
	}
}
void NetworkMessageSynchNetworkGameData::fromEndianHeader() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.header.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.header.messageType);
		data.header.mapCRC = Shared::PlatformByteOrder::fromCommonEndian(data.header.mapCRC);
		data.header.tilesetCRC = Shared::PlatformByteOrder::fromCommonEndian(data.header.tilesetCRC);
		data.header.techCRC = Shared::PlatformByteOrder::fromCommonEndian(data.header.techCRC);
		data.header.techCRCFileCount = Shared::PlatformByteOrder::fromCommonEndian(data.header.techCRCFileCount);
	}
}

void NetworkMessageSynchNetworkGameData::toEndianDetail(uint32 totalFileCount) {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		for(unsigned int i = 0; i < totalFileCount; ++i) {
			data.detail.techCRCFileCRCList[i] = Shared::PlatformByteOrder::toCommonEndian(data.detail.techCRCFileCRCList[i]);
		}
	}
}
void NetworkMessageSynchNetworkGameData::fromEndianDetail() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		for(unsigned int i = 0; i < data.header.techCRCFileCount; ++i) {
			data.detail.techCRCFileCRCList[i] = Shared::PlatformByteOrder::fromCommonEndian(data.detail.techCRCFileCRCList[i]);
		}
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
	for(unsigned int idx =0; idx < data.header.techCRCFileCount; ++idx) {
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
		for(int idx = 0; idx < (int)vctFileList.size(); ++idx) {
			std::pair<string,uint32> &fileInfo = vctFileList[idx];
			bool fileFound = false;
			uint32 remoteCRC = 0;
			for(int j = 0; j < (int)data.header.techCRCFileCount; ++j) {
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

		for(int i = 0; i < (int)data.header.techCRCFileCount; ++i) {
			string networkFile = data.detail.techCRCFileList[i].getString();
			uint32 &networkFileCRC = data.detail.techCRCFileCRCList[i];
			bool fileFound = false;
			uint32 localCRC = 0;
			for(int idx = 0; idx < (int)vctFileList.size(); ++idx) {
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
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] about to get nmtSynchNetworkGameDataStatus\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	data.header.techCRCFileCount = 0;

	bool result = NetworkMessage::receive(socket, &data, HeaderSize, true);
	if(result == true && data.header.techCRCFileCount > 0) {
		fromEndianHeader();
		// Here we loop possibly multiple times
		uint32 packetLoopCount = 1;
		if(data.header.techCRCFileCount > NetworkMessageSynchNetworkGameDataStatus::maxFileCRCPacketCount) {
			packetLoopCount = (data.header.techCRCFileCount / NetworkMessageSynchNetworkGameDataStatus::maxFileCRCPacketCount);
			if(data.header.techCRCFileCount % NetworkMessageSynchNetworkGameDataStatus::maxFileCRCPacketCount > 0) {
				packetLoopCount++;
			}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] packetLoopCount = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,packetLoopCount);

		for(uint32 iPacketLoop = 0; iPacketLoop < packetLoopCount; ++iPacketLoop) {

			uint32 packetIndex = iPacketLoop * NetworkMessageSynchNetworkGameDataStatus::maxFileCRCPacketCount;
			uint32 maxFileCountPerPacket = maxFileCRCPacketCount;
			uint32 packetFileCount = min((uint32)maxFileCountPerPacket,data.header.techCRCFileCount - packetIndex);

			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] iPacketLoop = %u, packetIndex = %u, packetFileCount = %u\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,iPacketLoop,packetIndex,packetFileCount);

			result = NetworkMessage::receive(socket, &data.detail.techCRCFileList[packetIndex], ((uint32)DetailSize1 * packetFileCount),true);
			if(result == true) {
				for(int i = 0; i < (int)data.header.techCRCFileCount; ++i) {
					data.detail.techCRCFileList[i].nullTerminate();
				}

                // Wait a max of x seconds for this message
				result = NetworkMessage::receive(socket, &data.detail.techCRCFileCRCList[packetIndex], ((uint32)DetailSize2 * packetFileCount),true);
			}
		}
		fromEndianDetail();
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] result = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,result);

	return result;
}

void NetworkMessageSynchNetworkGameDataStatus::send(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] about to send nmtSynchNetworkGameDataStatus, data.header.techCRCFileCount = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,data.header.techCRCFileCount);

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

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] packetLoopCount = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,packetLoopCount);

		toEndianDetail(totalFileCount);
		for(int iPacketLoop = 0; iPacketLoop < packetLoopCount; ++iPacketLoop) {

			int packetIndex = iPacketLoop * NetworkMessageSynchNetworkGameDataStatus::maxFileCRCPacketCount;
			int maxFileCountPerPacket = maxFileCRCPacketCount;
			int packetFileCount = min((uint32)maxFileCountPerPacket,totalFileCount - packetIndex);

			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] packetLoop = %d, packetIndex = %d, packetFileCount = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,iPacketLoop,packetIndex,packetFileCount);

			NetworkMessage::send(socket, &data.detail.techCRCFileList[packetIndex], (DetailSize1 * packetFileCount));
			NetworkMessage::send(socket, &data.detail.techCRCFileCRCList[packetIndex], (DetailSize2 * packetFileCount));
		}
	}
}

void NetworkMessageSynchNetworkGameDataStatus::toEndianHeader() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.header.messageType = Shared::PlatformByteOrder::toCommonEndian(data.header.messageType);
		data.header.mapCRC = Shared::PlatformByteOrder::toCommonEndian(data.header.mapCRC);
		data.header.tilesetCRC = Shared::PlatformByteOrder::toCommonEndian(data.header.tilesetCRC);
		data.header.techCRC = Shared::PlatformByteOrder::toCommonEndian(data.header.techCRC);
		data.header.techCRCFileCount = Shared::PlatformByteOrder::toCommonEndian(data.header.techCRCFileCount);
	}
}

void NetworkMessageSynchNetworkGameDataStatus::fromEndianHeader() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.header.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.header.messageType);
		data.header.mapCRC = Shared::PlatformByteOrder::fromCommonEndian(data.header.mapCRC);
		data.header.tilesetCRC = Shared::PlatformByteOrder::fromCommonEndian(data.header.tilesetCRC);
		data.header.techCRC = Shared::PlatformByteOrder::fromCommonEndian(data.header.techCRC);
		data.header.techCRCFileCount = Shared::PlatformByteOrder::fromCommonEndian(data.header.techCRCFileCount);
	}
}

void NetworkMessageSynchNetworkGameDataStatus::toEndianDetail(uint32 totalFileCount) {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		for(unsigned int i = 0; i < totalFileCount; ++i) {
			data.detail.techCRCFileCRCList[i] = Shared::PlatformByteOrder::toCommonEndian(data.detail.techCRCFileCRCList[i]);
		}
	}
}
void NetworkMessageSynchNetworkGameDataStatus::fromEndianDetail() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		for(unsigned int i = 0; i < data.header.techCRCFileCount; ++i) {
			data.detail.techCRCFileCRCList[i] = Shared::PlatformByteOrder::fromCommonEndian(data.detail.techCRCFileCRCList[i]);
		}
	}
}

// =====================================================
//	class NetworkMessageSynchNetworkGameDataFileCRCCheck
// =====================================================

NetworkMessageSynchNetworkGameDataFileCRCCheck::NetworkMessageSynchNetworkGameDataFileCRCCheck(
		uint32 totalFileCount, uint32 fileIndex, uint32 fileCRC, const string fileName)
{
	data.messageType= nmtSynchNetworkGameDataFileCRCCheck;

    data.totalFileCount = totalFileCount;
    data.fileIndex      = fileIndex;
    data.fileCRC        = fileCRC;
    data.fileName       = fileName;
}

const char * NetworkMessageSynchNetworkGameDataFileCRCCheck::getPackedMessageFormat() const {
	return "cLLL256s";
}

unsigned int NetworkMessageSynchNetworkGameDataFileCRCCheck::getPackedSize() {
	static unsigned int result = 0;
	if(result == 0) {
		Data packedData;
		packedData.fileCRC = 0;
		packedData.fileIndex = 0;
		packedData.messageType = nmtSynchNetworkGameDataFileCRCCheck;
		packedData.totalFileCount = 0;

		unsigned char *buf = new unsigned char[sizeof(packedData)*3];
		result = pack(buf, getPackedMessageFormat(),
				packedData.messageType,
				packedData.totalFileCount,
				packedData.fileIndex,
				packedData.fileCRC,
				packedData.fileName.getBuffer());
		delete [] buf;
	}
	return result;
}
void NetworkMessageSynchNetworkGameDataFileCRCCheck::unpackMessage(unsigned char *buf) {
	unpack(buf, getPackedMessageFormat(),
			&data.messageType,
			&data.totalFileCount,
			&data.fileIndex,
			&data.fileCRC,
			data.fileName.getBuffer());
}

unsigned char * NetworkMessageSynchNetworkGameDataFileCRCCheck::packMessage() {
	unsigned char *buf = new unsigned char[getPackedSize()+1];
	pack(buf, getPackedMessageFormat(),
			data.messageType,
			data.totalFileCount,
			data.fileIndex,
			data.fileCRC,
			data.fileName.getBuffer());
	return buf;
}

bool NetworkMessageSynchNetworkGameDataFileCRCCheck::receive(Socket* socket) {
	bool result = false;
	if(useOldProtocol == true) {
		result = NetworkMessage::receive(socket, &data, sizeof(data),true);
	}
	else {
		unsigned char *buf = new unsigned char[getPackedSize()+1];
		result = NetworkMessage::receive(socket, buf, getPackedSize(), true);
		unpackMessage(buf);
		//printf("Got packet size = %u data.messageType = %d\n%s\n",getPackedSize(),data.messageType,buf);
		delete [] buf;
	}
	fromEndian();
	data.fileName.nullTerminate();

	return result;
}

void NetworkMessageSynchNetworkGameDataFileCRCCheck::send(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtSynchNetworkGameDataFileCRCCheck\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	assert(data.messageType==nmtSynchNetworkGameDataFileCRCCheck);
	toEndian();

	if(useOldProtocol == true) {
		NetworkMessage::send(socket, &data, sizeof(data));
	}
	else {
		unsigned char *buf = packMessage();
		//printf("Send packet size = %u data.messageType = %d\n[%s]\n",getPackedSize(),data.messageType,buf);
		NetworkMessage::send(socket, buf, getPackedSize());
		delete [] buf;
	}
}

void NetworkMessageSynchNetworkGameDataFileCRCCheck::toEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
		data.totalFileCount = Shared::PlatformByteOrder::toCommonEndian(data.totalFileCount);
		data.fileIndex = Shared::PlatformByteOrder::toCommonEndian(data.fileIndex);
		data.fileCRC = Shared::PlatformByteOrder::toCommonEndian(data.fileCRC);
	}
}

void NetworkMessageSynchNetworkGameDataFileCRCCheck::fromEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.messageType);
		data.totalFileCount = Shared::PlatformByteOrder::fromCommonEndian(data.totalFileCount);
		data.fileIndex = Shared::PlatformByteOrder::fromCommonEndian(data.fileIndex);
		data.fileCRC = Shared::PlatformByteOrder::fromCommonEndian(data.fileCRC);
	}
}
// =====================================================
//	class NetworkMessageSynchNetworkGameDataFileGet
// =====================================================

NetworkMessageSynchNetworkGameDataFileGet::NetworkMessageSynchNetworkGameDataFileGet(const string fileName) {
	data.messageType= nmtSynchNetworkGameDataFileGet;
    data.fileName       = fileName;
}

const char * NetworkMessageSynchNetworkGameDataFileGet::getPackedMessageFormat() const {
	return "c256s";
}

unsigned int NetworkMessageSynchNetworkGameDataFileGet::getPackedSize() {
	static unsigned int result = 0;
	if(result == 0) {
		Data packedData;
		packedData.messageType = nmtSynchNetworkGameDataFileGet;
		unsigned char *buf = new unsigned char[sizeof(packedData)*3];
		result = pack(buf, getPackedMessageFormat(),
				packedData.messageType,
				packedData.fileName.getBuffer());
		delete [] buf;
	}
	return result;
}
void NetworkMessageSynchNetworkGameDataFileGet::unpackMessage(unsigned char *buf) {
	unpack(buf, getPackedMessageFormat(),
			&data.messageType,
			data.fileName.getBuffer());
}

unsigned char * NetworkMessageSynchNetworkGameDataFileGet::packMessage() {
	unsigned char *buf = new unsigned char[getPackedSize()+1];
	pack(buf, getPackedMessageFormat(),
			data.messageType,
			data.fileName.getBuffer());
	return buf;
}

bool NetworkMessageSynchNetworkGameDataFileGet::receive(Socket* socket) {
	bool result = false;
	if(useOldProtocol == true) {
		result = NetworkMessage::receive(socket, &data, sizeof(data),true);
	}
	else {
		unsigned char *buf = new unsigned char[getPackedSize()+1];
		result = NetworkMessage::receive(socket, buf, getPackedSize(), true);
		unpackMessage(buf);
		//printf("Got packet size = %u data.messageType = %d\n%s\n",getPackedSize(),data.messageType,buf);
		delete [] buf;
	}
	fromEndian();
	data.fileName.nullTerminate();

	return result;
}

void NetworkMessageSynchNetworkGameDataFileGet::send(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtSynchNetworkGameDataFileGet\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	assert(data.messageType==nmtSynchNetworkGameDataFileGet);
	toEndian();
	if(useOldProtocol == true) {
		NetworkMessage::send(socket, &data, sizeof(data));
	}
	else {
		unsigned char *buf = packMessage();
		//printf("Send packet size = %u data.messageType = %d\n[%s]\n",getPackedSize(),data.messageType,buf);
		NetworkMessage::send(socket, buf, getPackedSize());
		delete [] buf;
	}
}

void NetworkMessageSynchNetworkGameDataFileGet::toEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
	}
}
void NetworkMessageSynchNetworkGameDataFileGet::fromEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.messageType);
	}
}


// =====================================================
//	class SwitchSetupRequest
// =====================================================

SwitchSetupRequest::SwitchSetupRequest() {
	data.messageType= nmtSwitchSetupRequest;
	data.selectedFactionName="";
	data.currentSlotIndex=-1;
	data.toSlotIndex=-1;
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
	data.currentSlotIndex=currentFactionIndex;
	data.toSlotIndex=toFactionIndex;
    data.toTeam = toTeam;
    data.networkPlayerName=networkPlayerName;
    data.networkPlayerStatus=networkPlayerStatus;
    data.switchFlags = flags;
    data.language = language;
}

const char * SwitchSetupRequest::getPackedMessageFormat() const {
	return "c256sccc80scc60s";
}

unsigned int SwitchSetupRequest::getPackedSize() {
	static unsigned int result = 0;
	if(result == 0) {
		Data packedData;
		packedData.currentSlotIndex = 0;
		packedData.messageType = nmtSwitchSetupRequest;
		packedData.networkPlayerStatus = 0;
		packedData.switchFlags = 0;
		packedData.toSlotIndex = 0;
		packedData.toTeam = 0;

		unsigned char *buf = new unsigned char[sizeof(Data)*3];
		result = pack(buf, getPackedMessageFormat(),
				packedData.messageType,
				packedData.selectedFactionName.getBuffer(),
				packedData.currentSlotIndex,
				packedData.toSlotIndex,
				packedData.toTeam,
				packedData.networkPlayerName.getBuffer(),
				packedData.networkPlayerStatus,
				packedData.switchFlags,
				packedData.language.getBuffer());
		delete [] buf;
	}
	return result;
}
void SwitchSetupRequest::unpackMessage(unsigned char *buf) {
	unpack(buf, getPackedMessageFormat(),
			&data.messageType,
			data.selectedFactionName.getBuffer(),
			&data.currentSlotIndex,
			&data.toSlotIndex,
			&data.toTeam,
			data.networkPlayerName.getBuffer(),
			&data.networkPlayerStatus,
			&data.switchFlags,
			data.language.getBuffer());
}

unsigned char * SwitchSetupRequest::packMessage() {
	unsigned char *buf = new unsigned char[getPackedSize()+1];
	pack(buf, getPackedMessageFormat(),
			data.messageType,
			data.selectedFactionName.getBuffer(),
			data.currentSlotIndex,
			data.toSlotIndex,
			data.toTeam,
			data.networkPlayerName.getBuffer(),
			data.networkPlayerStatus,
			data.switchFlags,
			data.language.getBuffer());
	return buf;
}

bool SwitchSetupRequest::receive(Socket* socket) {
	bool result = false;
	if(useOldProtocol == true) {
		result = NetworkMessage::receive(socket, &data, sizeof(data), true);
	}
	else {
		//fromEndian();
		unsigned char *buf = new unsigned char[getPackedSize()+1];
		result = NetworkMessage::receive(socket, buf, getPackedSize(), true);
		unpackMessage(buf);
		//printf("Got packet size = %u data.messageType = %d\n%s\nTeam = %d faction [%s] currentFactionIndex = %d toFactionIndex = %d\n",getPackedSize(),data.messageType,buf,data.toTeam,data.selectedFactionName.getBuffer(),data.currentFactionIndex,data.toFactionIndex);
		delete [] buf;
	}
	fromEndian();

	data.selectedFactionName.nullTerminate();
	data.networkPlayerName.nullTerminate();
	data.language.nullTerminate();

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d] data.networkPlayerName [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,data.networkPlayerName.getString().c_str());

	return result;
}

void SwitchSetupRequest::send(Socket* socket) {
	assert(data.messageType==nmtSwitchSetupRequest);

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line %d] data.networkPlayerName [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,data.networkPlayerName.getString().c_str());
	toEndian();

	if(useOldProtocol == true) {
		NetworkMessage::send(socket, &data, sizeof(data));
	}
	else {
		unsigned char *buf = packMessage();
		//printf("Send packet size = %u data.messageType = %d\n%s\nTeam = %d faction [%s] currentFactionIndex = %d toFactionIndex = %d\n",getPackedSize(),data.messageType,buf,data.toTeam,data.selectedFactionName.getBuffer(),data.currentFactionIndex,data.toFactionIndex);
		NetworkMessage::send(socket, buf, getPackedSize());
		delete [] buf;
	}
}

void SwitchSetupRequest::toEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
		data.currentSlotIndex = Shared::PlatformByteOrder::toCommonEndian(data.currentSlotIndex);
		data.toSlotIndex = Shared::PlatformByteOrder::toCommonEndian(data.toSlotIndex);
		data.toTeam = Shared::PlatformByteOrder::toCommonEndian(data.toTeam);
		data.networkPlayerStatus = Shared::PlatformByteOrder::toCommonEndian(data.networkPlayerStatus);
		data.switchFlags = Shared::PlatformByteOrder::toCommonEndian(data.switchFlags);
	}
}
void SwitchSetupRequest::fromEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.messageType);
		data.currentSlotIndex = Shared::PlatformByteOrder::fromCommonEndian(data.currentSlotIndex);
		data.toSlotIndex = Shared::PlatformByteOrder::fromCommonEndian(data.toSlotIndex);
		data.toTeam = Shared::PlatformByteOrder::fromCommonEndian(data.toTeam);
		data.networkPlayerStatus = Shared::PlatformByteOrder::fromCommonEndian(data.networkPlayerStatus);
		data.switchFlags = Shared::PlatformByteOrder::fromCommonEndian(data.switchFlags);
	}
}

// =====================================================
//	class PlayerIndexMessage
// =====================================================
PlayerIndexMessage::PlayerIndexMessage(int16 playerIndex) {
	data.messageType= nmtPlayerIndexMessage;
	data.playerIndex=playerIndex;
}

const char * PlayerIndexMessage::getPackedMessageFormat() const {
	return "ch";
}

unsigned int PlayerIndexMessage::getPackedSize() {
	static unsigned int result = 0;
	if(result == 0) {
		Data packedData;
		packedData.messageType = 0;
		packedData.playerIndex = 0;
		unsigned char *buf = new unsigned char[sizeof(packedData)*3];
		result = pack(buf, getPackedMessageFormat(),
				packedData.messageType,
				packedData.playerIndex);
		delete [] buf;
	}
	return result;
}
void PlayerIndexMessage::unpackMessage(unsigned char *buf) {
	unpack(buf, getPackedMessageFormat(),
			&data.messageType,
			&data.playerIndex);
}

unsigned char * PlayerIndexMessage::packMessage() {
	unsigned char *buf = new unsigned char[getPackedSize()+1];
	pack(buf, getPackedMessageFormat(),
			data.messageType,
			data.playerIndex);
	return buf;
}

bool PlayerIndexMessage::receive(Socket* socket) {
	bool result = false;
	if(useOldProtocol == true) {
		result = NetworkMessage::receive(socket, &data, sizeof(data), true);
	}
	else {
		//fromEndian();
		unsigned char *buf = new unsigned char[getPackedSize()+1];
		result = NetworkMessage::receive(socket, buf, getPackedSize(), true);
		unpackMessage(buf);
		//printf("Got packet size = %u data.messageType = %d\n%s\n",getPackedSize(),data.messageType,buf);
		delete [] buf;
	}
	fromEndian();

	return result;
}

void PlayerIndexMessage::send(Socket* socket) {
	assert(data.messageType==nmtPlayerIndexMessage);
	toEndian();

	if(useOldProtocol == true) {
		NetworkMessage::send(socket, &data, sizeof(data));
	}
	else {
		unsigned char *buf = packMessage();
		//printf("Send packet size = %u data.messageType = %d\n[%s]\n",getPackedSize(),data.messageType,buf);
		//NetworkMessage::send(socket, &data, sizeof(data));
		NetworkMessage::send(socket, buf, getPackedSize());
		delete [] buf;
	}
}

void PlayerIndexMessage::toEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
		data.playerIndex = Shared::PlatformByteOrder::toCommonEndian(data.playerIndex);
	}
}
void PlayerIndexMessage::fromEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.messageType);
		data.playerIndex = Shared::PlatformByteOrder::fromCommonEndian(data.playerIndex);
	}
}

// =====================================================
//	class NetworkMessageLoadingStatus
// =====================================================
NetworkMessageLoadingStatus::NetworkMessageLoadingStatus(uint32 status)
{
	data.messageType= nmtLoadingStatusMessage;
	data.status=status;
}

const char * NetworkMessageLoadingStatus::getPackedMessageFormat() const {
	return "cL";
}

unsigned int NetworkMessageLoadingStatus::getPackedSize() {
	static unsigned int result = 0;
	if(result == 0) {
		Data packedData;
		packedData.messageType = 0;
		packedData.status = 0;
		unsigned char *buf = new unsigned char[sizeof(packedData)*3];
		result = pack(buf, getPackedMessageFormat(),
				packedData.messageType,
				packedData.status);
		delete [] buf;
	}
	return result;
}
void NetworkMessageLoadingStatus::unpackMessage(unsigned char *buf) {
	unpack(buf, getPackedMessageFormat(),
			&data.messageType,
			&data.status);
}

unsigned char * NetworkMessageLoadingStatus::packMessage() {
	unsigned char *buf = new unsigned char[getPackedSize()+1];
	pack(buf, getPackedMessageFormat(),
			data.messageType,
			data.status);
	return buf;
}

bool NetworkMessageLoadingStatus::receive(Socket* socket) {
	bool result = false;
	if(useOldProtocol == true) {
		result = NetworkMessage::receive(socket, &data, sizeof(data), true);
	}
	else {
		//fromEndian();
		unsigned char *buf = new unsigned char[getPackedSize()+1];
		result = NetworkMessage::receive(socket, buf, getPackedSize(), true);
		unpackMessage(buf);
		//printf("Got packet size = %u data.messageType = %d\n%s\n",getPackedSize(),data.messageType,buf);
		delete [] buf;
	}
	fromEndian();

	return result;
}

void NetworkMessageLoadingStatus::send(Socket* socket) {
	assert(data.messageType==nmtLoadingStatusMessage);
	toEndian();

	if(useOldProtocol == true) {
		NetworkMessage::send(socket, &data, sizeof(data));
	}
	else {
		unsigned char *buf = packMessage();
		//printf("Send packet size = %u data.messageType = %d\n[%s]\n",getPackedSize(),data.messageType,buf);
		NetworkMessage::send(socket, buf, getPackedSize());
		delete [] buf;
	}
}

void NetworkMessageLoadingStatus::toEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
		data.status = Shared::PlatformByteOrder::toCommonEndian(data.status);
	}
}
void NetworkMessageLoadingStatus::fromEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.messageType);
		data.status = Shared::PlatformByteOrder::fromCommonEndian(data.status);
	}
}

// =====================================================
//	class NetworkMessageMarkCell
// =====================================================

NetworkMessageMarkCell::NetworkMessageMarkCell(Vec2i target, int factionIndex, const string &text, int playerIndex) {
	if((int)text.length() >= maxTextStringSize) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] WARNING / ERROR - text [%s] length = %d, max = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,text.c_str(),text.length(),maxTextStringSize);
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

const char * NetworkMessageMarkCell::getPackedMessageFormat() const {
	return "chhcc500s";
}

unsigned int NetworkMessageMarkCell::getPackedSize() {
	static unsigned int result = 0;
	if(result == 0) {
		Data packedData;
		packedData.factionIndex = 0;
		packedData.messageType = nmtMarkCell;
		packedData.playerIndex = 0;
		packedData.targetX = 0;
		packedData.targetY = 0;

		unsigned char *buf = new unsigned char[sizeof(packedData)*3];
		result = pack(buf, getPackedMessageFormat(),
				packedData.messageType,
				packedData.targetX,
				packedData.targetY,
				packedData.factionIndex,
				packedData.playerIndex,
				packedData.text.getBuffer());
		delete [] buf;
	}
	return result;
}
void NetworkMessageMarkCell::unpackMessage(unsigned char *buf) {
	unpack(buf, getPackedMessageFormat(),
			&data.messageType,
			&data.targetX,
			&data.targetY,
			&data.factionIndex,
			&data.playerIndex,
			data.text.getBuffer());

}

unsigned char * NetworkMessageMarkCell::packMessage() {
	unsigned char *buf = new unsigned char[getPackedSize()+1];
	pack(buf, getPackedMessageFormat(),
			data.messageType,
			data.targetX,
			data.targetY,
			data.factionIndex,
			data.playerIndex,
			data.text.getBuffer());

	return buf;
}

bool NetworkMessageMarkCell::receive(Socket* socket){
	bool result = false;
	if(useOldProtocol == true) {
		result = NetworkMessage::receive(socket, &data, sizeof(data), true);
	}
	else {
		unsigned char *buf = new unsigned char[getPackedSize()+1];
		result = NetworkMessage::receive(socket, buf, getPackedSize(), true);
		unpackMessage(buf);
		//printf("Got packet size = %u data.messageType = %d\n%s\n",getPackedSize(),data.messageType,buf);
		delete [] buf;
	}
	fromEndian();

	data.text.nullTerminate();
	return result;
}

void NetworkMessageMarkCell::send(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtMarkCell\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	assert(data.messageType == nmtMarkCell);
	toEndian();

	if(useOldProtocol == true) {
		NetworkMessage::send(socket, &data, sizeof(data));
	}
	else {
		unsigned char *buf = packMessage();
		//printf("Send packet size = %u data.messageType = %d\n[%s]\n",getPackedSize(),data.messageType,buf);
		NetworkMessage::send(socket, buf, getPackedSize());
		delete [] buf;
	}
}

void NetworkMessageMarkCell::toEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
		data.targetX = Shared::PlatformByteOrder::toCommonEndian(data.targetX);
		data.targetY = Shared::PlatformByteOrder::toCommonEndian(data.targetY);
		data.factionIndex = Shared::PlatformByteOrder::toCommonEndian(data.factionIndex);
		data.playerIndex = Shared::PlatformByteOrder::toCommonEndian(data.playerIndex);
	}
}
void NetworkMessageMarkCell::fromEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.messageType);
		data.targetX = Shared::PlatformByteOrder::fromCommonEndian(data.targetX);
		data.targetY = Shared::PlatformByteOrder::fromCommonEndian(data.targetY);
		data.factionIndex = Shared::PlatformByteOrder::fromCommonEndian(data.factionIndex);
		data.playerIndex = Shared::PlatformByteOrder::fromCommonEndian(data.playerIndex);
	}
}

// =====================================================
//	class NetworkMessageUnMarkCell
// =====================================================

NetworkMessageUnMarkCell::NetworkMessageUnMarkCell() {
	data.messageType = nmtUnMarkCell;
	data.targetX		= 0;
	data.targetY		= 0;
	data.factionIndex 	= 0;
}

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

const char * NetworkMessageUnMarkCell::getPackedMessageFormat() const {
	return "chhc";
}

unsigned int NetworkMessageUnMarkCell::getPackedSize() {
	static unsigned int result = 0;
	if(result == 0) {
		Data packedData;
		packedData.factionIndex = 0;
		packedData.messageType = 0;
		packedData.targetX = 0;
		packedData.targetY = 0;
		unsigned char *buf = new unsigned char[sizeof(packedData)*3];
		result = pack(buf, getPackedMessageFormat(),
				packedData.messageType,
				packedData.targetX,
				packedData.targetY,
				packedData.factionIndex);
		delete [] buf;
	}
	return result;
}
void NetworkMessageUnMarkCell::unpackMessage(unsigned char *buf) {
	unpack(buf, getPackedMessageFormat(),
			&data.messageType,
			&data.targetX,
			&data.targetY,
			&data.factionIndex);

}

unsigned char * NetworkMessageUnMarkCell::packMessage() {
	unsigned char *buf = new unsigned char[getPackedSize()+1];
	pack(buf, getPackedMessageFormat(),
			data.messageType,
			data.targetX,
			data.targetY,
			data.factionIndex);

	return buf;
}

bool NetworkMessageUnMarkCell::receive(Socket* socket){
	bool result = false;
	if(useOldProtocol == true) {
		result = NetworkMessage::receive(socket, &data, sizeof(data), true);
	}
	else {
		unsigned char *buf = new unsigned char[getPackedSize()+1];
		result = NetworkMessage::receive(socket, buf, getPackedSize(), true);
		unpackMessage(buf);
		//printf("Got packet size = %u data.messageType = %d\n%s\n",getPackedSize(),data.messageType,buf);
		delete [] buf;
	}
	fromEndian();

	return result;
}

void NetworkMessageUnMarkCell::send(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtUnMarkCell\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	assert(data.messageType == nmtUnMarkCell);
	toEndian();

	if(useOldProtocol == true) {
		NetworkMessage::send(socket, &data, sizeof(data));
	}
	else {
		unsigned char *buf = packMessage();
		//printf("Send packet size = %u data.messageType = %d\n[%s]\n",getPackedSize(),data.messageType,buf);
		NetworkMessage::send(socket, buf, getPackedSize());
		delete [] buf;
	}
}

void NetworkMessageUnMarkCell::toEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
		data.targetX = Shared::PlatformByteOrder::toCommonEndian(data.targetX);
		data.targetY = Shared::PlatformByteOrder::toCommonEndian(data.targetY);
		data.factionIndex = Shared::PlatformByteOrder::toCommonEndian(data.factionIndex);
	}
}
void NetworkMessageUnMarkCell::fromEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.messageType);
		data.targetX = Shared::PlatformByteOrder::fromCommonEndian(data.targetX);
		data.targetY = Shared::PlatformByteOrder::fromCommonEndian(data.targetY);
		data.factionIndex = Shared::PlatformByteOrder::fromCommonEndian(data.factionIndex);
	}
}

// =====================================================
//	class NetworkMessageHighlightCell
// =====================================================

NetworkMessageHighlightCell::NetworkMessageHighlightCell() {
	data.messageType	= nmtHighlightCell;
	data.targetX		= 0;
	data.targetY		= 0;
	data.factionIndex 	= 0;
}

NetworkMessageHighlightCell::NetworkMessageHighlightCell(Vec2i target, int factionIndex) {
	data.messageType	= nmtHighlightCell;
	data.targetX		= target.x;
	data.targetY		= target.y;
	data.factionIndex 	= factionIndex;
}

const char * NetworkMessageHighlightCell::getPackedMessageFormat() const {
	return "chhc";
}

unsigned int NetworkMessageHighlightCell::getPackedSize() {
	static unsigned int result = 0;
	if(result == 0) {
		Data packedData;
		packedData.factionIndex = 0;
		packedData.messageType = 0;
		packedData.targetX = 0;
		packedData.targetY = 0;
		unsigned char *buf = new unsigned char[sizeof(packedData)*3];
		result = pack(buf, getPackedMessageFormat(),
				packedData.messageType,
				packedData.targetX,
				packedData.targetY,
				packedData.factionIndex);
		delete [] buf;
	}
	return result;
}
void NetworkMessageHighlightCell::unpackMessage(unsigned char *buf) {
	unpack(buf, getPackedMessageFormat(),
			&data.messageType,
			&data.targetX,
			&data.targetY,
			&data.factionIndex);

}

unsigned char * NetworkMessageHighlightCell::packMessage() {
	unsigned char *buf = new unsigned char[getPackedSize()+1];
	pack(buf, getPackedMessageFormat(),
			data.messageType,
			data.targetX,
			data.targetY,
			data.factionIndex);

	return buf;
}

bool NetworkMessageHighlightCell::receive(Socket* socket) {
	bool result = false;
	if(useOldProtocol == true) {
		result = NetworkMessage::receive(socket, &data, sizeof(data), true);
	}
	else {
		unsigned char *buf = new unsigned char[getPackedSize()+1];
		result = NetworkMessage::receive(socket, buf, getPackedSize(), true);
		unpackMessage(buf);
		//printf("Got packet size = %u data.messageType = %d\n%s\n",getPackedSize(),data.messageType,buf);
		delete [] buf;
	}
	fromEndian();
	return result;
}

void NetworkMessageHighlightCell::send(Socket* socket) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] nmtMarkCell\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	assert(data.messageType == nmtHighlightCell);
	toEndian();

	if(useOldProtocol == true) {
		NetworkMessage::send(socket, &data, sizeof(data));
	}
	else {
		unsigned char *buf = packMessage();
		//printf("Send packet size = %u data.messageType = %d\n[%s]\n",getPackedSize(),data.messageType,buf);
		NetworkMessage::send(socket, buf, getPackedSize());
		delete [] buf;
	}
}

void NetworkMessageHighlightCell::toEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.messageType = Shared::PlatformByteOrder::toCommonEndian(data.messageType);
		data.targetX = Shared::PlatformByteOrder::toCommonEndian(data.targetX);
		data.targetY = Shared::PlatformByteOrder::toCommonEndian(data.targetY);
		data.factionIndex = Shared::PlatformByteOrder::toCommonEndian(data.factionIndex);
	}
}
void NetworkMessageHighlightCell::fromEndian() {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		data.messageType = Shared::PlatformByteOrder::fromCommonEndian(data.messageType);
		data.targetX = Shared::PlatformByteOrder::fromCommonEndian(data.targetX);
		data.targetY = Shared::PlatformByteOrder::fromCommonEndian(data.targetY);
		data.factionIndex = Shared::PlatformByteOrder::fromCommonEndian(data.factionIndex);
	}
}

}}//end namespace
