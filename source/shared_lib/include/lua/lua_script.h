// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_LUA_LUASCRIPT_H_
#define _SHARED_LUA_LUASCRIPT_H_

#include <string>
#include <lua.hpp>
#include "vec.h"
#include "xml_parser.h"
#include "leak_dumper.h"

using std::string;

using Shared::Graphics::Vec2i;
using Shared::Graphics::Vec4i;
using Shared::Xml::XmlNode;

namespace Shared { namespace Lua {

typedef lua_State LuaHandle;
typedef int(*LuaFunction)(LuaHandle*);

// =====================================================
//	class LuaScript
// =====================================================

class LuaScript {
private:
	LuaHandle *luaState;
	int argumentCount;
	string currentLuaFunction;
	bool currentLuaFunctionIsValid;
	string sandboxWrapperFunctionName;
	string sandboxCode;

	void DumpGlobals();

public:
	LuaScript();
	~LuaScript();

	void loadCode(string code, string name);

	void beginCall(string functionName);
	void endCall();

	int runCode(const string code);
	void setSandboxWrapperFunctionName(string name);
	void setSandboxCode(string code);

	void registerFunction(LuaFunction luaFunction, string functionName);

	void saveGame(XmlNode *rootNode);
	void loadGame(const XmlNode *rootNode);

private:
	string errorToString(int errorCode);
};

// =====================================================
//	class LuaArguments
// =====================================================

class LuaArguments {
private:
	lua_State *luaState;
	int returnCount;

public:
	LuaArguments(lua_State *luaState);

	int getInt(int argumentIndex) const;
	string getString(int argumentIndex) const;
	void * getGenericData(int argumentIndex) const;
	Vec2i getVec2i(int argumentIndex) const;
	Vec4i getVec4i(int argumentIndex) const;
	int getReturnCount() const					{return returnCount;}

	void returnInt(int value);
	void returnString(const string &value);
	void returnVec2i(const Vec2i &value);
	void returnVec4i(const Vec4i &value);
	void returnVectorInt(const vector<int> &value);

private:
	void throwLuaError(const string &message) const;
};

}}//end namespace

#endif
