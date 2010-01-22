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

#include <vec.h>

using std::string;

using Shared::Graphics::Vec2i;

namespace Shared{ namespace Lua{

typedef lua_State LuaHandle;
typedef int(*LuaFunction)(LuaHandle*);

// =====================================================
//	class LuaScript
// =====================================================

class LuaScript{
private:
	LuaHandle *luaState;
	int argumentCount;

public:
	LuaScript();
	~LuaScript();

	void loadCode(const string &code, const string &name);

	void beginCall(const string& functionName);
	void endCall();

	void registerFunction(LuaFunction luaFunction, const string &functionName);

private:
	string errorToString(int errorCode);
};

// =====================================================
//	class LuaArguments
// =====================================================

class LuaArguments{
private:
	lua_State *luaState;
	int returnCount;

public:
	LuaArguments(lua_State *luaState);

	int getInt(int argumentIndex) const;
	string getString(int argumentIndex) const;
	Vec2i getVec2i(int argumentIndex) const;
	int getReturnCount() const					{return returnCount;}

	void returnInt(int value);
	void returnString(const string &value);
	void returnVec2i(const Vec2i &value);

private:
	void throwLuaError(const string &message) const;
};

}}//end namespace

#endif
