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
#include "platform_util.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;

namespace Shared { namespace Lua {

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

bool LuaScript::disableSandbox = false;

LuaScript::LuaScript() {
	Lua_STREFLOP_Wrapper streflopWrapper;

	currentLuaFunction = "";
	currentLuaFunctionIsValid = false;
	sandboxWrapperFunctionName = "";
	sandboxCode = "";
	luaState= luaL_newstate();

	luaL_openlibs(luaState);

	if(luaState == NULL) {
		throw megaglest_runtime_error("Can not allocate lua state");
	}

	argumentCount= -1;

	if(disableSandbox == false) {
		lua_getglobal(luaState, "os");
		lua_pushnil(luaState);
		lua_setfield(luaState, -2, "execute");
		lua_pushnil(luaState);
		lua_setfield(luaState, -2, "rename");
		lua_pushnil(luaState);
		lua_setfield(luaState, -2, "remove");
		lua_pushnil(luaState);
		lua_setfield(luaState, -2, "exit");

		lua_getglobal(luaState, "io");
		lua_pushnil(luaState);
		lua_setfield(luaState, -2, "open");
		lua_pushnil(luaState);
		lua_setfield(luaState, -2, "close");
		lua_pushnil(luaState);
		lua_setfield(luaState, -2, "write");
		lua_pushnil(luaState);
		lua_setfield(luaState, -2, "read");
		lua_pushnil(luaState);
		lua_setfield(luaState, -2, "flush");

		lua_pushnil(luaState);
		lua_setglobal(luaState, "loadfile");
		lua_pushnil(luaState);
		lua_setglobal(luaState, "dofile");
		lua_pushnil(luaState);
		lua_setglobal(luaState, "getfenv");
		lua_pushnil(luaState);
		lua_setglobal(luaState, "getmetatable");
		lua_pushnil(luaState);
		lua_setglobal(luaState, "load");
		lua_pushnil(luaState);
		lua_setglobal(luaState, "loadfile");
		lua_pushnil(luaState);
		lua_setglobal(luaState, "loadstring");
		lua_pushnil(luaState);
		lua_setglobal(luaState, "rawequal");
		lua_pushnil(luaState);
		lua_setglobal(luaState, "rawget");
		lua_pushnil(luaState);
		lua_setglobal(luaState, "rawset");
		lua_pushnil(luaState);
		lua_setglobal(luaState, "setfenv");
		lua_pushnil(luaState);
		lua_setglobal(luaState, "setmetatable");


		lua_pop(luaState, 1);
	}
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
#if LUA_VERSION_NUM > 501
		lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
		for (lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1)) {
#else
	while (lua_next(L, LUA_GLOBALSINDEX) != 0) {
#endif
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
#if LUA_VERSION_NUM <= 501
		lua_pop(L, 1);
#endif
	}
}

void LuaScript::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;

	const bool debugLuaDump = false;
	//try{
	LuaHandle *L = luaState;
	// push the first key (nil = beginning of table)
	lua_pushnil(L);

	// lua_next will:
	// 1 - pop the key
	// 2 - push the next key
	// 3 - push the value at that key
	// ... so the key will be at index -2 and the value at index -1

#if LUA_VERSION_NUM > 501
	lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
	for (lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1)) {
#else
	while (lua_next(L, LUA_GLOBALSINDEX) != 0) {
#endif
		// get type of key and value
		int key_type = lua_type(L, -2);
		int value_type = lua_type(L, -1);

		if(debugLuaDump == true) printf("LUA save key_type = %d, value_type = %d for var [%s]\n",key_type,value_type,lua_tostring(L, -2));

		// support only string keys
		// globals aren't likely to have a non-string key, but just to be certain ...
		if (key_type != LUA_TSTRING) {
			lua_pop(L, 1); // pop the value so that the top contains the key for the next iteration
			continue;
		}

		// support only number, boolean and string values
		if (value_type != LUA_TNUMBER &&
			value_type != LUA_TBOOLEAN &&
			value_type != LUA_TSTRING &&
			value_type != LUA_TTABLE) {
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

		bool skipTable = false;
		// The first pair is the tables key type,value
		// The second pair is the tables value type,value
		vector<pair<pair<int,string>, pair<int,string> > > tableList;
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
		case LUA_TTABLE:
			{
			if(debugLuaDump == true) printf("LUA TABLE DETECTED - START\n");
			for (lua_pushnil(L); lua_next(L, -2) ;) {
				if(debugLuaDump == true) printf("LUA TABLE loop A\n");

				int tableKeyType = lua_type(L, -2);
				int tableValueType = lua_type(L, -1);

				if(debugLuaDump == true) printf("LUA TABLE loop item type [%s]\n",lua_typename(L, tableValueType));

				switch (tableValueType) {
					case LUA_TSTRING:
					case LUA_TNUMBER:
					case LUA_TBOOLEAN:
						break;
					default:
						skipTable = true;
						break;
				}
				if(skipTable == false) {
					// Stack: value, key, table
					std :: string value = "";
					if(!lua_isnil(L, -1)) {
						if(debugLuaDump == true) printf("LUA TABLE loop B\n");

						lua_pushvalue(L, -1);

						if(debugLuaDump == true) printf("LUA TABLE loop C\n");

						value = lua_tostring (L, -1);

						if(debugLuaDump == true) printf("LUA TABLE loop D\n");

						lua_pop (L, 1);
					}
					lua_pop (L, 1);

					if(debugLuaDump == true) printf("LUA TABLE value [%s]\n",value.c_str());

					// Stack: key, table
					lua_pushvalue(L, -1);

					// Stack: key, key, table
					std :: string key = lua_tostring(L, -1);
					lua_pop(L, 1);

					// Stack: key, table
					//std :: cout << key << "" << value << "\ n";
					if(debugLuaDump == true) printf("[%s] [%s]\n",key.c_str(),value.c_str());

					if(value_string != "") {
						value_string += "|||";
					}
					char szBuf[8096]="";
					snprintf(szBuf,8096,"[%s] [%s]",key.c_str(),value.c_str());
					//value_string += szBuf;
					//vector<pair<pair<int,string>, pair<int,string>> > tableList;
					tableList.push_back(make_pair(make_pair(tableKeyType,key),make_pair(tableValueType,value)));
				}
				else {
					lua_pop(L, 1);
				}
			}
			}
			break;
		}

		// enclose the value in "" if it is a string
		//if (value_type == LUA_TSTRING) {
			//value_string = "\"" + value_string + "\"";
		//}

		if(skipTable == true) {
			if(debugLuaDump == true) printf("#2 SKIPPING TABLE\n");
		}
		else {
			//vector<pair<pair<int,string>, pair<int,string>> > tableList;
			if(tableList.empty() == false) {
				XmlNode *luaScriptNode = rootNode->addChild("LuaScript");
				luaScriptNode->addAttribute("variable",key_string, mapTagReplacements);
				luaScriptNode->addAttribute("value_type",intToStr(value_type), mapTagReplacements);

				for(unsigned int i = 0; i < tableList.size(); ++i) {
					pair<pair<int,string>, pair<int,string> > &item = tableList[i];

					XmlNode *luaScriptTableNode = luaScriptNode->addChild("Table");
					luaScriptTableNode->addAttribute("key_type",intToStr(item.first.first), mapTagReplacements);
					luaScriptTableNode->addAttribute("key",item.first.second, mapTagReplacements);
					luaScriptTableNode->addAttribute("value",item.second.second, mapTagReplacements);
					luaScriptTableNode->addAttribute("value_type",intToStr(item.second.first), mapTagReplacements);
				}
			}
			else {
				// resulting line. Somehow save this and when you need to restore it, just
				// call luaL_dostring with that line.
				//SaveLine(key_string + " = " + value_string);		// Pop the value so the index remains on top of the stack for the next iteration
				//printf("Found global LUA var: %s = %s\n",key_string.c_str(),value_string.c_str());
				XmlNode *luaScriptNode = rootNode->addChild("LuaScript");
				luaScriptNode->addAttribute("variable",key_string, mapTagReplacements);
				luaScriptNode->addAttribute("value",value_string, mapTagReplacements);
				luaScriptNode->addAttribute("value_type",intToStr(value_type), mapTagReplacements);
			}
		}
#if LUA_VERSION_NUM <= 501
		lua_pop(L, 1);
#endif
	}

	//}
	//catch(const exception &ex) {
	//	abort();
	//}
}

void LuaScript::loadGame(const XmlNode *rootNode) {
	vector<XmlNode *> luaScriptNodeList = rootNode->getChildList("LuaScript");
	for(unsigned int i = 0; i < luaScriptNodeList.size(); ++i) {
		XmlNode *node = luaScriptNodeList[i];

		string variable = node->getAttribute("variable")->getValue();
		int value_type = node->getAttribute("value_type")->getIntValue();

		switch (value_type) {
			case LUA_TSTRING:
				lua_pushstring( luaState, node->getAttribute("value")->getValue().c_str() );
				break;
			case LUA_TNUMBER:
				lua_pushnumber( luaState, node->getAttribute("value")->getIntValue() );
				break;
			case LUA_TBOOLEAN:
				lua_pushboolean( luaState, node->getAttribute("value")->getBoolValue() );
				break;
			case LUA_TTABLE:
				{
					lua_newtable(luaState);    /* We will pass a table */
					vector<XmlNode *> luaScriptTableNode = node->getChildList("Table");
					for(unsigned int j = 0; j < luaScriptTableNode.size(); ++j) {
						XmlNode *nodeTable = luaScriptTableNode[j];

						int key_type = nodeTable->getAttribute("key_type")->getIntValue();
						switch (key_type) {
							case LUA_TSTRING:
								lua_pushstring( luaState, nodeTable->getAttribute("key")->getValue().c_str() );
								break;
							case LUA_TNUMBER:
								lua_pushnumber( luaState, nodeTable->getAttribute("key")->getIntValue() );
								break;
							case LUA_TBOOLEAN:
								lua_pushboolean( luaState, nodeTable->getAttribute("key")->getIntValue() );
								break;
						}
						int value_type = nodeTable->getAttribute("value_type")->getIntValue();
						switch (value_type) {
							case LUA_TSTRING:
								lua_pushstring( luaState, nodeTable->getAttribute("value")->getValue().c_str() );
								break;
							case LUA_TNUMBER:
								lua_pushnumber( luaState, nodeTable->getAttribute("value")->getIntValue() );
								break;
							case LUA_TBOOLEAN:
								lua_pushboolean( luaState, nodeTable->getAttribute("value")->getIntValue() );
								break;
						}

						lua_rawset(luaState, -3);      /* Stores the pair in the table */
					}
				}
				break;

		}

		lua_setglobal( luaState, variable.c_str() );
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

void LuaScript::loadCode(string code, string name){
	Lua_STREFLOP_Wrapper streflopWrapper;

	//printf("Code [%s]\nName [%s]\n",code.c_str(),name.c_str());

	int errorCode= luaL_loadbuffer(luaState, code.c_str(), code.length(), name.c_str());
	if(errorCode != 0 ) {
		printf("=========================================================\n");
		printf("Error loading lua code: %s\n",errorToString(errorCode).c_str());
		printf("Function name [%s]\ncode:\n%s\n",name.c_str(),code.c_str());
		printf("=========================================================\n");

		throw megaglest_runtime_error("Error loading lua code: " + errorToString(errorCode));
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] name [%s], errorCode = %d\n",__FILE__,__FUNCTION__,__LINE__,name.c_str(),errorCode);

	//run code
	errorCode= lua_pcall(luaState, 0, 0, 0);
	if(errorCode !=0 ) {
		printf("=========================================================\n");
		printf("Error calling lua pcall: %s\n",errorToString(errorCode).c_str());
		printf("=========================================================\n");

		throw megaglest_runtime_error("Error initializing lua: " + errorToString(errorCode));
	}

	//const char *errMsg = lua_tostring(luaState, -1);

	//printf("END of call to Name [%s]\n",name.c_str());

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] name [%s], errorCode = %d\n",__FILE__,__FUNCTION__,__LINE__,name.c_str(),errorCode);
}

void LuaScript::setSandboxWrapperFunctionName(string name) {
	sandboxWrapperFunctionName = name;
}

void LuaScript::setSandboxCode(string code) {
	sandboxCode = code;
}

int LuaScript::runCode(string code) {
	Lua_STREFLOP_Wrapper streflopWrapper;

	int errorCode = luaL_dostring(luaState,code.c_str());
	return errorCode;
}

void LuaScript::beginCall(string functionName) {
	Lua_STREFLOP_Wrapper streflopWrapper;

	currentLuaFunction = functionName;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] functionName [%s]\n",__FILE__,__FUNCTION__,__LINE__,functionName.c_str());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line: %d] functionName [%s]\n",__FILE__,__FUNCTION__,__LINE__,functionName.c_str());

	//string funcLuaFunction = functionName;
//	if(sandboxWrapperFunctionName != "" && sandboxCode != "") {
//		int errorCode= runCode(sandboxCode);
//		if(errorCode !=0 ) {
//			throw megaglest_runtime_error("Error calling lua function [" + currentLuaFunction + "] error: " + errorToString(errorCode));
//		}
//		//functionName = sandboxWrapperFunctionName;
//	}
	lua_getglobal(luaState, functionName.c_str());

	currentLuaFunctionIsValid = lua_isfunction(luaState,lua_gettop(luaState));

	//printf("currentLuaFunctionIsValid = %d functionName [%s]\n",currentLuaFunctionIsValid,functionName.c_str());
	argumentCount= 0;
}

void LuaScript::endCall() {
	Lua_STREFLOP_Wrapper streflopWrapper;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugLUA).enabled) SystemFlags::OutputDebug(SystemFlags::debugLUA,"In [%s::%s Line: %d] currentLuaFunction [%s], currentLuaFunctionIsValid = %d\n",__FILE__,__FUNCTION__,__LINE__,currentLuaFunction.c_str(),currentLuaFunctionIsValid);

	if(currentLuaFunctionIsValid == true) {
		if(sandboxWrapperFunctionName != "" && sandboxCode != "") {
			//lua_pushstring(luaState, currentLuaFunction.c_str());   // push 1st argument, the real lua function
			//argumentCount = 1;

			string safeWrapper = sandboxWrapperFunctionName + " [[" + currentLuaFunction + "()]]";
			printf("Trying to execute [%s]\n",safeWrapper.c_str());
			int errorCode= runCode(safeWrapper);
			if(errorCode !=0 ) {
				throw megaglest_runtime_error("Error calling lua function [" + currentLuaFunction + "] error: " + errorToString(errorCode));
			}

			//printf("Trying to execute [%s]\n",currentLuaFunction.c_str());
			//lua_getglobal(luaState, sandboxWrapperFunctionName.c_str());
			//argumentCount = 1;
			//lua_pushstring( luaState, currentLuaFunction.c_str() );

//			int errorCode= lua_pcall(luaState, argumentCount, 0, 0);
//			if(errorCode !=0 ) {
//				throw megaglest_runtime_error("Error calling lua function [" + currentLuaFunction + "] error: " + errorToString(errorCode));
//			}
		}
		else {
			int errorCode= lua_pcall(luaState, argumentCount, 0, 0);
			if(errorCode !=0 ) {
				throw megaglest_runtime_error("Error calling lua function [" + currentLuaFunction + "] error: " + errorToString(errorCode));
			}
		}
	}
	else
	{
		lua_pcall(luaState, argumentCount, 0, 0);
	}
}

void LuaScript::registerFunction(LuaFunction luaFunction, string functionName) {
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
			error+= "Unknown LUA error" + intToStr(errorCode);
			break;
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

float LuaArguments::getFloat(int argumentIndex) const {
	Lua_STREFLOP_Wrapper streflopWrapper;

	if(!lua_isnumber(luaState, argumentIndex)) {
		throwLuaError("Can not get int from Lua state");
	}
	float result = static_cast<float>(luaL_checknumber(luaState, argumentIndex));
	return result;
}
Vec2f LuaArguments::getVec2f(int argumentIndex) const {
	Lua_STREFLOP_Wrapper streflopWrapper;

	Vec2f v;

	if(!lua_istable(luaState, argumentIndex)){
		throwLuaError("Can not get vec2f from Lua state, value on the stack is not a table");
	}

#if LUA_VERSION_NUM > 501
	if(lua_rawlen(luaState, argumentIndex)!=2){
#else
	if(luaL_getn(luaState, argumentIndex)!=2){
#endif
		throwLuaError("Can not get vec2f from Lua state, array size not 2");
	}

	//string stackString = getStackText();
	//printf("Lua Stack:\n%s\n",stackString.c_str());

	lua_rawgeti(luaState, argumentIndex, 1);
	//printf("xa = %s argumentIndex = %d\n",lua_tostring(luaState, argumentIndex),argumentIndex);

	//v.x= luaL_checkint(luaState, argumentIndex);
	v.x= static_cast<float>(lua_tonumber(luaState, argumentIndex));
	lua_pop(luaState, 1);

	//printf("X = %d\n",v.x);

	lua_rawgeti(luaState, argumentIndex, 2);
	//printf("ya = %s\n",lua_tostring(luaState, argumentIndex));

	//v.y= luaL_checkint(luaState, argumentIndex);
	v.y= static_cast<float>(lua_tonumber(luaState, argumentIndex));
	lua_pop(luaState, 1);

	//printf("Y = %d\n",v.y);

	return v;
}

Vec3f LuaArguments::getVec3f(int argumentIndex) const {
	Lua_STREFLOP_Wrapper streflopWrapper;

	Vec3f v;

	if(!lua_istable(luaState, argumentIndex)){
		throwLuaError("Can not get vec3f from Lua state, value on the stack is not a table");
	}

#if LUA_VERSION_NUM > 501
	if(lua_rawlen(luaState, argumentIndex)!=3){
#else
	if(luaL_getn(luaState, argumentIndex)!=3){
#endif
		throwLuaError("Can not get vec3f from Lua state, array size not 3");
	}

	//string stackString = getStackText();
	//printf("Lua Stack:\n%s\n",stackString.c_str());

	lua_rawgeti(luaState, argumentIndex, 1);
	//printf("xa = %s argumentIndex = %d\n",lua_tostring(luaState, argumentIndex),argumentIndex);

	//v.x= luaL_checkint(luaState, argumentIndex);
	v.x= static_cast<float>(lua_tonumber(luaState, argumentIndex));
	lua_pop(luaState, 1);

	//printf("X = %d\n",v.x);

	lua_rawgeti(luaState, argumentIndex, 2);
	//printf("ya = %s\n",lua_tostring(luaState, argumentIndex));

	//v.y= luaL_checkint(luaState, argumentIndex);
	v.y= static_cast<float>(lua_tonumber(luaState, argumentIndex));
	lua_pop(luaState, 1);

	//printf("Y = %d\n",v.y);

	lua_rawgeti(luaState, argumentIndex, 3);
	//printf("ya = %s\n",lua_tostring(luaState, argumentIndex));

	//v.y= luaL_checkint(luaState, argumentIndex);
	v.z= static_cast<float>(lua_tonumber(luaState, argumentIndex));
	lua_pop(luaState, 1);

	return v;
}

Vec4f LuaArguments::getVec4f(int argumentIndex) const {
	Lua_STREFLOP_Wrapper streflopWrapper;

	Vec4f v;

	if(!lua_istable(luaState, argumentIndex)){
		throwLuaError("Can not get vec4f from Lua state, value on the stack is not a table");
	}

#if LUA_VERSION_NUM > 501
	if(lua_rawlen(luaState, argumentIndex)!=4){
#else
	if(luaL_getn(luaState, argumentIndex)!=4){
#endif
		throwLuaError("Can not get vec4f from Lua state, array size not 4");
	}

	//string stackString = getStackText();
	//printf("Lua Stack:\n%s\n",stackString.c_str());

	lua_rawgeti(luaState, argumentIndex, 1);
	//printf("xa = %s argumentIndex = %d\n",lua_tostring(luaState, argumentIndex),argumentIndex);

	//v.x= luaL_checkint(luaState, argumentIndex);
	v.x= static_cast<float>(lua_tonumber(luaState, argumentIndex));
	lua_pop(luaState, 1);

	//printf("X = %d\n",v.x);

	lua_rawgeti(luaState, argumentIndex, 2);
	//printf("ya = %s\n",lua_tostring(luaState, argumentIndex));

	//v.y= luaL_checkint(luaState, argumentIndex);
	v.y= static_cast<float>(lua_tonumber(luaState, argumentIndex));
	lua_pop(luaState, 1);

	//printf("Y = %d\n",v.y);

	lua_rawgeti(luaState, argumentIndex, 3);
	//printf("ya = %s\n",lua_tostring(luaState, argumentIndex));

	//v.y= luaL_checkint(luaState, argumentIndex);
	v.z= static_cast<float>(lua_tonumber(luaState, argumentIndex));
	lua_pop(luaState, 1);

	lua_rawgeti(luaState, argumentIndex, 4);
	//printf("ya = %s\n",lua_tostring(luaState, argumentIndex));

	//v.y= luaL_checkint(luaState, argumentIndex);
	v.w= static_cast<float>(lua_tonumber(luaState, argumentIndex));
	lua_pop(luaState, 1);

	return v;
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

#if LUA_VERSION_NUM > 501
	if(lua_rawlen(luaState, argumentIndex)!=2){
#else
	if(luaL_getn(luaState, argumentIndex)!=2){
#endif
		throwLuaError("Can not get vec2i from Lua state, array size not 2");
	}

	//string stackString = getStackText();
	//printf("Lua Stack:\n%s\n",stackString.c_str());

	lua_rawgeti(luaState, argumentIndex, 1);
	//printf("xa = %s argumentIndex = %d\n",lua_tostring(luaState, argumentIndex),argumentIndex);

	//v.x= luaL_checkint(luaState, argumentIndex);
	v.x= lua_tointeger(luaState, argumentIndex);
	lua_pop(luaState, 1);

	//printf("X = %d\n",v.x);

	lua_rawgeti(luaState, argumentIndex, 2);
	//printf("ya = %s\n",lua_tostring(luaState, argumentIndex));

	//v.y= luaL_checkint(luaState, argumentIndex);
	v.y= lua_tointeger(luaState, argumentIndex);
	lua_pop(luaState, 1);

	//printf("Y = %d\n",v.y);

	return v;
}

Vec4i LuaArguments::getVec4i(int argumentIndex) const {
	Lua_STREFLOP_Wrapper streflopWrapper;

	Vec4i v;

	if(!lua_istable(luaState, argumentIndex)){
		throwLuaError("Can not get vec4i from Lua state, value on the stack is not a table");
	}

#if LUA_VERSION_NUM > 501
	if(lua_rawlen(luaState, argumentIndex) != 4 ) {
#else
	if(luaL_getn(luaState, argumentIndex) != 4) {
#endif
		throwLuaError("Can not get vec4i from Lua state, array size not 4");
	}

	lua_rawgeti(luaState, argumentIndex, 1);
	v.x= luaL_checkint(luaState, argumentIndex);
	lua_pop(luaState, 1);

	lua_rawgeti(luaState, argumentIndex, 2);
	v.y= luaL_checkint(luaState, argumentIndex);
	lua_pop(luaState, 1);

	lua_rawgeti(luaState, argumentIndex, 3);
	v.z= luaL_checkint(luaState, argumentIndex);
	lua_pop(luaState, 1);

	lua_rawgeti(luaState, argumentIndex, 4);
	v.w= luaL_checkint(luaState, argumentIndex);
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

void LuaArguments::returnVec4i(const Vec4i &value) {
	//Lua_STREFLOP_Wrapper streflopWrapper;

	++returnCount;

	lua_newtable(luaState);

	lua_pushnumber(luaState, value.x);
	lua_rawseti(luaState, -2, 1);

	lua_pushnumber(luaState, value.y);
	lua_rawseti(luaState, -2, 2);

	lua_pushnumber(luaState, value.z);
	lua_rawseti(luaState, -2, 3);

	lua_pushnumber(luaState, value.w);
	lua_rawseti(luaState, -2, 4);

}

void LuaArguments::returnVectorInt(const vector<int> &value) {
	//Lua_STREFLOP_Wrapper streflopWrapper;

	++returnCount;

	lua_newtable(luaState);

	for(unsigned int i = 0; i < value.size(); ++i) {
		lua_pushnumber(luaState, value[i]);
		lua_rawseti(luaState, -2, i+1);
	}
}

string LuaArguments::getStackText() const {
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

			//int tableLen = 0;
#if LUA_VERSION_NUM > 501
			int tableLen = lua_rawlen(luaState, -i);
#else
			int tableLen = luaL_getn(luaState, -i);
#endif
			stackString+= "Table (" + intToStr(tableLen) + ")\n";
//			for(unsigned int j = 1; j < tableLen; ++j) {
//				stackString+= "entry# " + intToStr(j) + ", value = " + lua_tostring(luaState, -i) + "\n";
//			}


//			int j = 1;
//			lua_pushnil(luaState);  // The initial key for the traversal.
//
//			printf("start loop\n");
//
//			while (lua_next(luaState, -2)!=0) {
//				printf("in loop j = %d\n",j);
//
//				const char* Param=lua_tostring(luaState, -1);
//
//				printf("passed in loop j = %d Param [%s]\n",j,(Param != NULL ? Param : "<nil>"));
//
//				if (Param!=NULL) {
//					stackString+= "entry# " + intToStr(j) + ", value = " + Param + "\n";
//				}
//
//				// Remove the value, keep the key for the next iteration.
//				lua_pop(luaState, 1);
//				j++;
//			}

//			const int len = lua_objlen( luaState, -i );
//			printf("Table Len = %d\n",len);
//
//			for ( int j = 1; j <= len; ++j ) {
//				printf("A Table\n");
//
//				lua_pushinteger( luaState, j );
//
//				printf("B Table\n");
//
//				lua_gettable( luaState, -2 );
//
//				printf("C Table\n");
//
//				//v.push_back( lua_tointeger( L, -1 ) );
//				const char *value = lua_tostring( luaState, -1 );
//				printf("D Table value = %s\n",(value != NULL ? value : "<nil>"));
//
//				//v.push_back( lua_tointeger( L, -1 ) );
//				const char *value2 = lua_tostring( luaState, -2 );
//				printf("E Table value2 = %s\n",(value2 != NULL ? value2 : "<nil>"));
//
//				value2 = lua_tostring( luaState, -3 );
//				printf("F Table value2 = %s\n",(value2 != NULL ? value2 : "<nil>"));
//
//				value2 = lua_tostring( luaState, 0 );
//				printf("G Table value2 = %s\n",(value2 != NULL ? value2 : "<nil>"));
//
//				value2 = lua_tostring( luaState, 1 );
//				printf("H Table value2 = %s\n",(value2 != NULL ? value2 : "<nil>"));
//
//				stackString+= "entry# " + intToStr(j) + ", value = " + (value != NULL ? value : "<nil>") + "\n";
//
//				printf("E Table\n");
//
//				lua_pop( luaState, 1 );
//			}
		}
		else
		{
			stackString+= "Unknown";
		}
		stackString+= "\n";
	}
	
	return stackString;
}
void LuaArguments::throwLuaError(const string &message) const{
	Lua_STREFLOP_Wrapper streflopWrapper;

	string stackString = getStackText();
	throw megaglest_runtime_error("Lua error: " + message + "\n\nLua Stack:\n" + stackString);
}

}}//end namespace
