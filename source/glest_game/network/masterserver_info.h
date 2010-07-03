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

#ifndef _GLEST_GAME_MASTERSERVERINFO_H_
#define _GLEST_GAME_MASTERSERVERINFO_H_

#include <string>
using std::string;

namespace Glest{ namespace Game{

// ===========================================================
//	class ParticleSystemType 
//
///	A type of particle system
// ===========================================================

class MasterServerInfo{
protected:
	//general info:
	string glestVersion;
	string platform;
	string binaryCompileDate;
	
	//game info:
	string serverTitle;
	string ipAddress;
	
	//game setup info:
	string tech;
	string map; 
	string tileset;
	int activeSlots;
	int networkSlots;
	int connectedClients;
	int externalconnectport;

public:
	const string &getGlestVersion() const	{return glestVersion;}
	const string &getPlatform() const	{return platform;}
	const string &getBinaryCompileDate() const	{return binaryCompileDate;}
	
	const string &getServerTitle() const	{return serverTitle;}
	const string &getIpAddress() const	{return ipAddress;}
	
	const string &getTech() const	{return tech;}
	const string &getMap() const	{return map;}
	const string &getTileset() const	{return tileset;}
	const int getActiveSlots() const	{return activeSlots;}
	const int getNetworkSlots() const	{return networkSlots;}
	const int getConnectedClients() const	{return connectedClients;}
	const int getExternalConnectPort() const	{return externalconnectport;}
	
	
	
	//general info:
	void setGlestVersion(string value) { glestVersion = value; }
	void setPlatform(string value) { platform = value; }
	void setBinaryCompileDate(string value) { binaryCompileDate = value; }
	
	//game info:
	void setServerTitle(string value) { serverTitle = value; }
	void setIpAddress(string value) { ipAddress = value; }
	
	//game setup info:
	void setTech(string value) { tech = value; }
	void setMap(string value) { map = value; }
	void setTileset(string value) { tileset = value; }
	
	void setActiveSlots(int value) { activeSlots = value; }
	void setNetworkSlots(int value) { networkSlots = value; }
	void setConnectedClients(int value) { connectedClients = value; }	
	void setExternalConnectPort(int value) { externalconnectport = value; }
};

}}//end namespace

#endif
