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

#include "chat_manager.h"

#include "window.h"
#include "console.h"
#include "config.h"
#include "network_manager.h"
#include "lang.h"
#include "core_data.h"
#include "util.h"
#include <stdexcept>
#include "string_utils.h"
#include "sound_renderer.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Platform;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
//	class ChatManager
// =====================================================

ChatManager::ChatManager() {
	console= NULL;
	editEnabled= false;
	teamMode= false;
	thisTeamIndex= -1;
	disableTeamMode = false;
	xPos=75;
	yPos=155;
	maxTextLenght=90;
	textCharLength.clear();
	text="";
	font=CoreData::getInstance().getConsoleFont();
	font3D=CoreData::getInstance().getConsoleFont3D();
	inMenu=false;
	customCB = NULL;
	this->maxCustomTextLength = maxTextLenght;
}

void ChatManager::init(Console* console, int thisTeamIndex, const bool inMenu, string manualPlayerNameOverride) {
	this->console= console;
	this->thisTeamIndex= thisTeamIndex;
	this->disableTeamMode= false;
	this->inMenu=inMenu;
	this->manualPlayerNameOverride = manualPlayerNameOverride;
}

void ChatManager::setDisableTeamMode(bool value) {
	disableTeamMode = value;

	if(disableTeamMode == true) {
		teamMode = false;
	}
}

void ChatManager::keyUp(SDL_KeyboardEvent key) {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	try {
		if(editEnabled == true) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key.keysym.sym,key.keysym.sym);

			if(isKeyPressed(SDLK_ESCAPE,key,false) == true) {
				text.clear();
				textCharLength.clear();
				editEnabled= false;

				if(customCB != NULL) {
					customCB->processInputText(text,true);
					customCB = NULL;
				}
			}
		}
	}
	catch(const exception &ex) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

bool ChatManager::textInput(std::string inputText) {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] inputText [%s]\n",__FILE__,__FUNCTION__,__LINE__,inputText.c_str());

	int maxTextLenAllowed = (customCB != NULL ? this->maxCustomTextLength : maxTextLenght);
	string textToAdd = getTextWithLengthCheck(inputText,textCharLength.size(),maxTextLenAllowed);

	if(editEnabled && (int)textCharLength.size() < maxTextLenAllowed && textToAdd.size() > 0) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		appendText(textToAdd, true, true);
		updateAutoCompleteBuffer();
		return true;
	}
	return false;
}

string ChatManager::getTextWithLengthCheck(string addText, int currentLength, int maxLength) {
	string resultText = "";
	if(addText.empty() == false) {
		int utf8CharsAdded = 0;
		for(unsigned int index = 0; index < addText.size(); ) {
			int len = getUTF8_Width(&addText[index]);
			utf8CharsAdded++;
			if(currentLength + utf8CharsAdded > maxLength) {
				break;
			}
			resultText += addText.substr(index,len);
			index += len;
		}
	}
	return resultText;
}

void ChatManager::keyDown(SDL_KeyboardEvent key) {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key.keysym.sym,key.keysym.sym);

	try {
		Lang &lang= Lang::getInstance();
		Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));

		//toggle team mode
		if(editEnabled == false &&
			isKeyPressed(configKeys.getSDLKey("ChatTeamMode"),key) == true) {
			if(disableTeamMode == true) {
				if (!inMenu) {
					console->addLine(lang.getString("ChatModeDisabledToAvoidCheating") );
				}
			}
			else {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key.keysym.sym,key.keysym.sym);

				if (!inMenu) {
					if (teamMode == true) {
						teamMode = false;
						console->addLine(lang.getString("ChatMode") + ": " + lang.getString("All"));
					} else {
						teamMode = true;
						console->addLine(lang.getString("ChatMode") + ": " + lang.getString("Team"));
					}
				}
			}
		}


		if(isKeyPressed(SDLK_RETURN,key, false) == true) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key.keysym.sym,key.keysym.sym);

			SDL_keysym keystate = key.keysym;
			if(keystate.mod & (KMOD_LALT | KMOD_RALT)){
				// alt+enter is ignored
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key.keysym.sym,key.keysym.sym);
			}
			else {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key.keysym.sym,key.keysym.sym);

				if(editEnabled == true) {
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key.keysym.sym,key.keysym.sym);

					GameNetworkInterface *gameNetworkInterface= NetworkManager::getInstance().getGameNetworkInterface();
					if(text.empty() == false) {

						if(customCB == NULL) {
							//string playerName 	= gameNetworkInterface->getHumanPlayerName();
							int playerIndex 	= gameNetworkInterface->getHumanPlayerIndex();

							if(this->manualPlayerNameOverride != "") {
								console->addLine(text,false,this->manualPlayerNameOverride,Vec3f(1.f, 1.f, 1.f),teamMode);
							}
							else {
								console->addLine(text,false,playerIndex,Vec3f(1.f, 1.f, 1.f),teamMode);
							}

							gameNetworkInterface->sendTextMessage("*"+text, teamMode? thisTeamIndex: -1, false, "");
							if(inMenu == false && Config::getInstance().getBool("ChatStaysActive","false")==false ) {
								editEnabled= false;
							}
						}
					}
					else {
						editEnabled= false;
					}

					if(customCB != NULL) {
						customCB->processInputText(text,false);
						editEnabled= false;
						customCB = NULL;
					}

					text.clear();
					textCharLength.clear();
				}
				else {
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key.keysym.sym,key.keysym.sym);

					switchOnEdit();
				}
			}
		}
		else if(isKeyPressed(SDLK_TAB,key, false) == true) {
			if(text.empty() == false) {
				// First find the prefix characters to auto-complete
				string currentAutoCompleteName = "";

				int startPos = -1;
				for(int i = (int)text.size()-1; i >= 0; --i) {
					if(text[i] != ' ') {
						startPos = i;
					}
					else {
						break;
					}
				}

				if(startPos >= 0) {
					currentAutoCompleteName = text.substr(startPos);
				}

				//printf("TAB currentAutoCompleteName [%s] lastAutoCompleteSearchText [%s]\n",currentAutoCompleteName.c_str(),lastAutoCompleteSearchText.c_str());
				string autoCompleteName = lastAutoCompleteSearchText;

				// Now lookup the prefix for a match in playernames
				string autoCompleteResult = "";

				int replaceCurrentAutoCompleteName = -1;
				vector<int> matchedIndexes;

				GameNetworkInterface *gameNetworkInterface= NetworkManager::getInstance().getGameNetworkInterface();
				const GameSettings *settings = gameNetworkInterface->getGameSettings();
				for(unsigned int factionIndex = 0; factionIndex < (unsigned int)settings->getFactionCount(); ++factionIndex) {
					string playerName = settings->getNetworkPlayerName(factionIndex);
					if(playerName.length() > autoCompleteName.length() &&
						StartsWith(toLower(playerName), toLower(autoCompleteName)) == true) {
						if(toLower(playerName) == toLower(currentAutoCompleteName)) {
							replaceCurrentAutoCompleteName = factionIndex;
						}
						else {
							autoCompleteResult = playerName.substr(autoCompleteName.length());
							matchedIndexes.push_back(factionIndex);
						}
					}
				}
				if(matchedIndexes.empty() == false) {
					int newMatchedIndex = -1;
					for(unsigned int index = 0; index < (unsigned int)matchedIndexes.size(); ++index) {
						int possibleMatchIndex = matchedIndexes[index];
						if(replaceCurrentAutoCompleteName < 0 ||
							(replaceCurrentAutoCompleteName >= 0 && possibleMatchIndex > replaceCurrentAutoCompleteName)) {
							newMatchedIndex = possibleMatchIndex;
							break;
						}
					}
					if(newMatchedIndex < 0) {
						for(unsigned int index = 0; index < (unsigned int)matchedIndexes.size(); ++index) {
							int possibleMatchIndex = matchedIndexes[index];
							if(replaceCurrentAutoCompleteName < 0 ||
								(replaceCurrentAutoCompleteName >= 0 && possibleMatchIndex < replaceCurrentAutoCompleteName)) {
								newMatchedIndex = possibleMatchIndex;
								break;
							}
						}
					}

					if(newMatchedIndex >= 0) {
						autoCompleteResult = settings->getNetworkPlayerName(newMatchedIndex).substr(autoCompleteName.length());
					}
				}

				if(autoCompleteResult == "") {
					replaceCurrentAutoCompleteName = -1;
					matchedIndexes.clear();
					for(unsigned int index = 0; index < (unsigned int)autoCompleteTextList.size(); ++index) {
						string autoText = autoCompleteTextList[index];

						//printf("CHECKING #2 autoText.length() = %d [%s] autoCompleteName.length() = %d [%s]\n",autoText.length(),autoText.c_str(),autoCompleteName.length(),currentAutoCompleteName.c_str());

						if(autoText.length() > autoCompleteName.length() &&
							StartsWith(toLower(autoText), toLower(autoCompleteName)) == true) {

							if(toLower(autoText) == toLower(currentAutoCompleteName)) {
								replaceCurrentAutoCompleteName = index;
								//printf("CHECKING #2 REPLACE\n");
							}
							else {
								autoCompleteResult = autoText.substr(autoCompleteName.length());
								//printf("CHECKING #2 autoCompleteResult [%s] autoCompleteName [%s]\n",autoCompleteResult.c_str(),autoCompleteName.c_str());
								matchedIndexes.push_back(index);
							}
						}
					}
					if(matchedIndexes.empty() == false) {
						int newMatchedIndex = -1;
						for(unsigned int index = 0; index < (unsigned int)matchedIndexes.size(); ++index) {
							int possibleMatchIndex = matchedIndexes[index];
							if(replaceCurrentAutoCompleteName < 0 ||
								(replaceCurrentAutoCompleteName >= 0 && possibleMatchIndex > replaceCurrentAutoCompleteName)) {
								newMatchedIndex = possibleMatchIndex;
								break;
							}
						}
						if(newMatchedIndex < 0) {
							for(unsigned int index = 0; index < (unsigned int)matchedIndexes.size(); ++index) {
								int possibleMatchIndex = matchedIndexes[index];
								if(replaceCurrentAutoCompleteName < 0 ||
									(replaceCurrentAutoCompleteName >= 0 && possibleMatchIndex < replaceCurrentAutoCompleteName)) {
									newMatchedIndex = possibleMatchIndex;
									break;
								}
							}
						}

						if(newMatchedIndex >= 0) {
							autoCompleteResult = autoCompleteTextList[newMatchedIndex].substr(autoCompleteName.length());
						}
					}
				}

				if(autoCompleteResult != "") {
					if(replaceCurrentAutoCompleteName >= 0) {
						deleteText((int)currentAutoCompleteName.length(), false);

						autoCompleteResult = autoCompleteName + autoCompleteResult;

						//printf("REPLACE: currentAutoCompleteName [%s] autoCompleteResult [%s] text [%s]\n",currentAutoCompleteName.c_str(),autoCompleteResult.c_str(),text.c_str());
					}
					else {
						//printf("ADD: currentAutoCompleteName [%s] autoCompleteResult [%s] text [%s]\n",currentAutoCompleteName.c_str(),autoCompleteResult.c_str(),text.c_str());
					}
					appendText(autoCompleteResult, false, false);
				}
			}
		}
		else if(isKeyPressed(SDLK_BACKSPACE,key,false) == true) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key.keysym.sym,key.keysym.sym);

			deleteText(1);
		}

	}
	catch(const exception &ex) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void ChatManager::keyPress(SDL_KeyboardEvent c) {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c] [%d]\n",__FILE__,__FUNCTION__,__LINE__,c.keysym.sym,c.keysym.sym);

// no more textinput with keyPress in SDL2!
//	int maxTextLenAllowed = (customCB != NULL ? this->maxCustomTextLength : maxTextLenght);
//	if(editEnabled && (int)text.size() < maxTextLenAllowed) {
//		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c] [%d]\n",__FILE__,__FUNCTION__,__LINE__,c.keysym.sym,c.keysym.sym);
//		//space is the first meaningful code
//		wchar_t key = extractKeyPressedUnicode(c);
//		wchar_t textAppend[] = { key, 0 };
//		appendText(textAppend);
//	}
}

void ChatManager::switchOnEdit(CustomInputCallbackInterface *customCB,int maxCustomTextLength) {
	editEnabled= true;
	text.clear();
	textCharLength.clear();
	this->customCB = customCB;
	if(maxCustomTextLength > 0) {
		this->maxCustomTextLength = maxCustomTextLength;
	}
	else {
		this->maxCustomTextLength = maxTextLenght;
	}
}

void ChatManager::deleteText(int deleteCount,bool addToAutoCompleteBuffer) {
	if(text.empty() == false && deleteCount >= 0) {
		for(unsigned int i = 0; i < (unsigned int)deleteCount; ++i) {
			if(textCharLength.empty() == false) {
				//printf("BEFORE DEL textCharLength.size() = %d textCharLength[textCharLength.size()-1] = %d text.length() = %d\n",textCharLength.size(),textCharLength[textCharLength.size()-1],text.length());

				if(textCharLength[textCharLength.size()-1] > (int)text.length()) {
					textCharLength[(int)textCharLength.size()-1] = (int)text.length();
				}
				for(unsigned int i = 0; i < (unsigned int)textCharLength[textCharLength.size()-1]; ++i) {
					text.erase(text.end() -1);
				}
				//printf("AFTER DEL textCharLength.size() = %d textCharLength[textCharLength.size()-1] = %d text.length() = %d\n",textCharLength.size(),textCharLength[textCharLength.size()-1],text.length());
				textCharLength.pop_back();

				if(addToAutoCompleteBuffer == true) {
					updateAutoCompleteBuffer();
				}
			}
		}
	}

}

void ChatManager::appendText(string addText, bool validateChars, bool addToAutoCompleteBuffer) {

	for(unsigned int index = 0; index < addText.size(); ) {
		int len = getUTF8_Width(&addText[index]);
		textCharLength.push_back(len);
		text += addText.substr(index,len);
		index += len;

		if(addToAutoCompleteBuffer == true) {
			updateAutoCompleteBuffer();
		}
	}
}

void ChatManager::updateAutoCompleteBuffer() {
	if(text.empty() == false) {
		int startPos = -1;
		for(int i = (int)text.size()-1; i >= 0; --i) {
			if(text[i] != ' ') {
				startPos = i;
			}
			else {
				break;
			}
		}

		if(startPos >= 0) {
			lastAutoCompleteSearchText = text.substr(startPos);
		}
	}
}

void ChatManager::addText(string text) {
	int maxTextLenAllowed = (customCB != NULL ? this->maxCustomTextLength : maxTextLenght);
	if(editEnabled && (int)text.size() + (int)this->text.size() <= maxTextLenAllowed) {
		this->text += text;
		for(int i= 0; i<(int)text.size() ; i++){
			textCharLength.push_back(1);
		}
	}
}

void ChatManager::updateNetwork() {
	try {
		GameNetworkInterface *gameNetworkInterface= NetworkManager::getInstance().getGameNetworkInterface();
		//string text;
		//string sender;
		//Config &config= Config::getInstance();

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] gameNetworkInterface->getChatText() [%s]\n",__FILE__,__FUNCTION__,__LINE__,gameNetworkInterface->getChatText().c_str());

		if(gameNetworkInterface != NULL &&
				gameNetworkInterface->getChatTextList(false).empty() == false) {
			Lang &lang= Lang::getInstance();

			std::vector<ChatMsgInfo> chatList = gameNetworkInterface->getChatTextList(true);
			for(int idx = 0; idx < (int)chatList.size(); idx++) {
				const ChatMsgInfo msg = chatList[idx];
				int teamIndex= msg.chatTeamIndex;

				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] got nmtText [%s] for team = %d\n",__FILE__,__FUNCTION__,msg.chatText.c_str(),teamIndex);

				if(teamIndex == -1 || teamIndex == thisTeamIndex) {
					if(msg.targetLanguage == "" || lang.isLanguageLocal(msg.targetLanguage) == true) {
						bool teamMode = (teamIndex != -1 && teamIndex == thisTeamIndex);
						string playerName = gameNetworkInterface->getHumanPlayerName();
						if(this->manualPlayerNameOverride != "") {
							playerName = this->manualPlayerNameOverride;
						}

						//printf("Network chat msg from: [%d - %s] [%s]\n",msg.chatPlayerIndex,gameNetworkInterface->getHumanPlayerName().c_str(),this->manualPlayerNameOverride.c_str());

			        	if(StartsWith(msg.chatText,"*")){
			        		if(msg.chatText.find(playerName) != string::npos){
			        			CoreData &coreData= CoreData::getInstance();
			        			SoundRenderer &soundRenderer= SoundRenderer::getInstance();
			        			soundRenderer.playFx(coreData.getHighlightSound(),true);
			        		}
			        		console->addLine(msg.chatText.substr(1,msg.chatText.size()), true, msg.chatPlayerIndex,Vec3f(1.f, 1.f, 1.f),teamMode);
			        	}
			        	else {
			        		console->addLine(msg.chatText, true, msg.chatPlayerIndex,Vec3f(1.f, 1.f, 1.f),teamMode);
			        	}

					}

					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Added text to console\n",__FILE__,__FUNCTION__);
				}

				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			}
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			//gameNetworkInterface->clearChatInfo();
		}
	}
	catch(const std::exception &ex) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		throw megaglest_runtime_error(szBuf);
	}
}

}}//end namespace
