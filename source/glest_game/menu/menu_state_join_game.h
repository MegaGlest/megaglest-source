// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_MENUSTATEJOINGAME_H_
#define _GLEST_GAME_MENUSTATEJOINGAME_H_

#ifdef WIN32
#include <winsock.h>
#include <winsock2.h>
#endif

#include "chat_manager.h"
#include "leak_dumper.h"
#include "main_menu.h"
#include "properties.h"
#include <string>
#include <vector>

using Shared::Util::Properties;

namespace Glest {
namespace Game {

class NetworkMessageIntro;

// ===============================
// 	class MenuStateJoinGame
// ===============================

class MenuStateJoinGame : public MenuState, public DiscoveredServersInterface {
private:
  static const int newServerIndex;
  static const int newPrevServerIndex;
  static const int foundServersIndex;
  static const string serverFileName;

private:
  GraphicButton buttonReturn;
  GraphicButton buttonConnect;
  GraphicButton buttonAutoFindServers;
  GraphicButton buttonCreateGame;

  GraphicLabel labelServer;
  GraphicLabel labelServerType;
  GraphicLabel labelServerIp;
  GraphicLabel labelStatus;
  GraphicLabel labelInfo;
  GraphicListBox listBoxServerType;
  GraphicListBox listBoxServers;
  GraphicListBox listBoxFoundServers;
  GraphicLabel labelServerPort;
  GraphicLabel labelServerPortLabel;

  bool connected;
  int playerIndex;
  Properties servers;

  // Console console;
  ChatManager chatManager;

  string serversSavedFile;
  bool abortAutoFind;
  bool autoConnectToServer;

public:
  MenuStateJoinGame(Program *program, MainMenu *mainMenu, bool connect = false,
                    Ip serverIp = Ip(), int portNumberOverride = -1);
  MenuStateJoinGame(Program *program, MainMenu *mainMenu, bool *autoFindHost);
  virtual ~MenuStateJoinGame();

  void mouseClick(int x, int y, MouseButton mouseButton);
  void mouseDoubleClick(int x, int y, MouseButton mouseButton){};
  void mouseMove(int x, int y, const MouseState *mouseState);
  void render();
  void update();

  virtual bool textInput(std::string text);
  virtual void keyDown(SDL_KeyboardEvent key);
  virtual void keyPress(SDL_KeyboardEvent c);

  virtual bool isInSpecialKeyCaptureEvent() {
    return chatManager.getEditEnabled();
  }

  void reloadUI();

private:
  void CommonInit(bool connect, Ip serverIp, int portNumberOverride);
  bool connectToServer();
  virtual void DiscoveredServers(std::vector<string> serverList);
};
} // namespace Game
} // namespace Glest

#endif
