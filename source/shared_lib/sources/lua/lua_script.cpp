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

#include "lua_script.h"

#include <stdexcept>

#include "conversion.h"
#include "util.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;

namespace Shared{ namespace Lua{

//
// This class wraps streflop for LuaScript. We need to toggle the data type
// for streflop to use when calling into LUA as streflop may corrupt some
// numeric values passed from Lua otherwise
//
class Lua_STREFLOP_Wrapper {
public:
	Lua_STREFLOP_Wrapper() {
#ifdef USE_STREFLOP
	streflop_init<streflop::Double>();
#endif
	}
	~Lua_STREFLOP_Wrapper() {
#ifdef USE_STREFLOP
	streflop_init<streflop::Simple>();
#endif
	}
};

// =====================================================
//	class LuaScript
// =====================================================

LuaScript::LuaScript() {
	Lua_STREFLOP_Wrapper streflopWrapper;

	currentLuaFunction = "";
	currentLuaFunctionIsValid = false;
	luaState= luaL_newstate();

	luaL_openlibs(luaState);

	if(luaState==NULL){
		throw runtime_error("Can not allocate lua state");
	}

	argumentCount= -1;
}

void LuaScript::DumpGlobals()
{
	LuaHandle *L = luaState;
	// push the first key (nil = beginning of table)
	lua_pushnil(L);

	// lua_next will:
	// 1 - pop the key
	// 2 - push the next key
	// 3 - push the value at that key
	// ... so the key will be at index -2 and the value at index -1
	while (lua_next(L, LUA_GLOBALSINDEX) != 0) {
		// get type of key and value
		int key_type = lua_type(L, -2);
		int value_type = lua_type(L, -1);

		// support only string keys
		// globals aren't likely to have a non-string key, but just to be certain ...
		if (key_type != LUA_TSTRING) {
			lua_pop(L, 1); // pop the value so that the top contains the key for the next iteration
			continue;
		}

		// support only number, boolean and string values
		if (value_type != LUA_TNUMBER &&
			value_type != LUA_TBOOLEAN &&
			value_type != LUA_TSTRING) {
			lua_pop(L, 1); // again, pop the value before going to the next loop iteration
			continue;
		}

		// get the key as a string
		string key_string = lua_tostring(L, -2); // no copy required - we already know this is a string

		// do not support variables that start with '_'
		// lua has some predefined values like _VERSION. They all start with underscore

		if (!key_string.size()) { // this again is highly unlikely, but still ...
			lua_pop(L, 1);
			continue;
		}
		if (key_string[0] == '_') {
			lua_pop(L, 1);
			continue;
		}

		string value_string;

		// convert the value to a string. This depends on its type
		switch (value_type) {
		case LUA_TSTRING:
		case LUA_TNUMBER:
			// numbers can be converted to strings

			// get the value as a string (this requires a copy because traversing tables
			// uses the top of the stack as an index. If conversion from a number to string
			// happens, the top of the stack will be altered and the table index will become invalid)
			lua_pushvalue(L, -1);
			value_string = lua_tostring(L, -1);
			lua_pop(L, 1);
			break;
		case LUA_TBOOLEAN:
			value_string = lua_toboolean(L, -1) == 0 ? "false" : "true";
			break;
		}

		// enclose the value in "" if it is a string
		if (value_type == LUA_TSTRING) {
			value_string = "\"" + value_string + "\"";
		}

		// resulting line. Somehow save this and when you need to restore it, just
		// call luaL_dostring with that line.
		//SaveLine(key_string + " = " + value_string);		// Pop the value so the index remains on top of the stack for the next iteration
		printf("Found global LUA var: %s = %s\n",key_string.c_str(),value_string.c_str());
		lua_pop(L, 1);
	}
}

LuaScript::~LuaScript() {
	Lua_STREFLOP_Wrapper streflopWrapper;

//	LuaInterface.LuaTable luatab;
//
//	            luatab = lua.GetTable("_G");
//
//
//	            foreach (DictionaryEntry d in luatab)
//
//	            {
//
//	                Console.WriteLine("{0} -> {1}", d.Key, d.Value);
//
//	            }

	//DumpGlobals();

	lua_close(luaState);
}

void LuaScript::loadCode(const string &code, const string &name){
	Lua_STREFLOP_Wrapper streflopWrapper;

	//printf("Code [%s]\nName [%s]\n",code.c_str(),name.c_str());

	int errorCode= luaL_loadbuffer(luaState, code.c_str(), code.length(), name.c_str());
	if(errorCode != 0 ) {
		printf("=========================================================\n");
		printf("Error loading lua code: %s\n",errorToString(errorCode).c_str());
		printf("Function name [%s]\ncode:\n%s\n",name.c_str(),code.c_str());
		printf("=========================================================\n");

		throw runtime_error("Error loading lua code: " + errorToString(errorCode));
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] name [%s], errorCode = %d\n",__FILE__,__FUNCTION__,__LINE__,name.c_str(),errorCode);

	//run code
	errorCode= lua_pcall(luaState, 0, 0, 0);
	if(errorCode !=0 ) {
		printf("=========================================================\n");
		printf("Error calling lua pcall: %s\n",errorToString(errorCode).c_str());
		printf("=========================================================\n");

		throw runtime_error("Error initializing lua: " + errorToString(errorCode));
	}

	//const char *errMsg = lua_tostring(luaState, -1);

	//printf("END of call to Name [%s]\n",name.c_str());

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] name [%s], errorCode = %d\n",__FILE__,__FUNCTION__,__LINE__,name.c_str(),errorCode);
}

void LuaScript::beginCall(const string& functionName) {
	Lua_STREFLOP_Wrapper streflopWrapper;

	currentLuaFunction = functionName;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] functionName [%s]\n",__FILE__,__FUNCTION__,__LINE__,functionName.c_str());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] functionName [%s]\n",__FILE__,__FUNCTION__,__LINE__,functionName.c_str());

	lua_getglobal(luaState, functionName.c_str());
	currentLuaFunctionIsValid = lua_isfunction(luaState,lua_gettop(luaState));
	argumentCount= 0;
}

void LuaScript::endCall() {
	Lua_STREFLOP_Wrapper streflopWrapper;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] currentLuaFunction [%s], currentLuaFunctionIsValid = %d\n",__FILE__,__FUNCTION__,__LINE__,currentLuaFunction.c_str(),currentLuaFunctionIsValid);

	if(currentLuaFunctionIsValid == true) {
		int errorCode= lua_pcall(luaState, argumentCount, 0, 0);
		if(errorCode !=0 ) {
			throw runtime_error("Error calling lua function [" + currentLuaFunction + "] error: " + errorToString(errorCode));
		}
	}
	else
	{
		lua_pcall(luaState, argumentCount, 0, 0);
	}
}

void LuaScript::registerFunction(LuaFunction luaFunction, const string &functionName) {
	Lua_STREFLOP_Wrapper streflopWrapper;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] functionName [%s]\n",__FILE__,__FUNCTION__,__LINE__,functionName.c_str());

	lua_pushcfunction(luaState, luaFunction);
	lua_setglobal(luaState, functionName.c_str());
}

string LuaScript::errorToString(int errorCode) {
	Lua_STREFLOP_Wrapper streflopWrapper;

	string error;
	switch(errorCode){
		case LUA_ERRSYNTAX: 
			error+= "Syntax error"; 
			break;
		case LUA_ERRRUN: 
			error+= "Runtime error"; 
			break;
		case LUA_ERRMEM: 
			error+= "Memory allocation error"; 
			break;
		case LUA_ERRERR: 
			error+= "Error while running the error handler"; 
			break;
		default:
			error+= "Unknown error";
	}

	error += string(": ")+luaL_checkstring(luaState, -1);

	return error;
}

// =====================================================
//	class LuaArguments
// =====================================================

LuaArguments::LuaArguments(lua_State *luaState){
	Lua_STREFLOP_Wrapper streflopWrapper;

	this->luaState= luaState;
	returnCount= 0;
}

int LuaArguments::getInt(int argumentIndex) const{
	Lua_STREFLOP_Wrapper streflopWrapper;

	if(!lua_isnumber(luaState, argumentIndex)) {
		throwLuaError("Can not get int from Lua state");
	}
	int result = luaL_checkint(luaState, argumentIndex);
	return result;
}

string LuaArguments::getString(int argumentIndex) const{
	Lua_STREFLOP_Wrapper streflopWrapper;

	if(!lua_isstring(luaState, argumentIndex)){
		throwLuaError("Can not get string from Lua state");
	}
	return luaL_checkstring(luaState, argumentIndex);
}

void * LuaArguments::getGenericData(int argumentIndex) const{
	Lua_STREFLOP_Wrapper streflopWrapper;

	if(lua_isstring(luaState, argumentIndex)) {
		const char *result = luaL_checkstring(luaState, argumentIndex);
		//printf("\nGENERIC param %d is a string, %s!\n",argumentIndex,result);
		return (void *)result;
	}
	//else if(lua_isnumber(luaState, argumentIndex)) {
	//	double result = luaL_checknumber(luaState, argumentIndex);
	//	printf("\nGENERIC param %d is a double, %f!\n",argumentIndex,result);
	//	return (void *)result;
	//}
	else if(lua_isnumber(luaState, argumentIndex)) {
		lua_Integer result = luaL_checkinteger(luaState, argumentIndex);
		//printf("\nGENERIC param %d is an int, %d!\n",argumentIndex,(int)result);
		return (void *)result;
	}
	else {
		//printf("\nGENERIC param %d is a NULL!\n",argumentIndex);
		return NULL;
	}
}

Vec2i LuaArguments::getVec2i(int argumentIndex) const{
	Lua_STREFLOP_Wrapper streflopWrapper;

	Vec2i v;
	
	if(!lua_istable(luaState, argumentIndex)){
		throwLuaError("Can not get vec2i from Lua state, value on the stack is not a table");
	}

	if(luaL_getn(luaState, argumentIndex)!=2){
		throwLuaError("Can not get vec2i from Lua state, array size not 2");
	}

	lua_rawgeti(luaState, argumentIndex, 1);
	v.x= luaL_checkint(luaState, argumentIndex);
	lua_pop(luaState, 1);

	lua_rawgeti(luaState, argumentIndex, 2);
	v.y= luaL_checkint(luaState, argumentIndex);
	lua_pop(luaState, 1);

	return v;
}

void LuaArguments::returnInt(int value){
	Lua_STREFLOP_Wrapper streflopWrapper;

	++returnCount;
	lua_pushinteger(luaState, value);
}

void LuaArguments::returnString(const string &value){
	Lua_STREFLOP_Wrapper streflopWrapper;

	++returnCount;
	lua_pushstring(luaState, value.c_str());
}

void LuaArguments::returnVec2i(const Vec2i &value){
	//Lua_STREFLOP_Wrapper streflopWrapper;

	++returnCount;

	lua_newtable(luaState);

	lua_pushnumber(luaState, value.x);
	lua_rawseti(luaState, -2, 1);

	lua_pushnumber(luaState, value.y);
	lua_rawseti(luaState, -2, 2);
}

void LuaArguments::throwLuaError(const string &message) const{
	Lua_STREFLOP_Wrapper streflopWrapper;

	string stackString;
	int stackSize = lua_gettop(luaState);

	//build stack string
	for(int i= 1; i<=stackSize; ++i){
		stackString+= "-" + intToStr(i) + ": ";
		if(lua_isnumber(luaState, -i)){
			stackString+= "Number: " + doubleToStr(luaL_checknumber(luaState, -i ));
		}
		else if(lua_isstring(luaState, -i)){
			stackString+= "String: " + string(luaL_checkstring(luaState, -i));
		}
		else if(lua_istable(luaState, -i)){
			stackString+= "Table (" + intToStr(luaL_getn(luaState, -i)) + ")";
		}
		else
		{
			stackString+= "Unknown";
		}
		stackString+= "\n";
	}
	
	throw runtime_error("Lua error: " + message + "\n\nLua Stack:\n" + stackString);
}

}}//end namespace
