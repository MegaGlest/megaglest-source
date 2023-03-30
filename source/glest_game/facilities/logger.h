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

#ifndef _SHARED_UTIL_LOGGER_H_
#define _SHARED_UTIL_LOGGER_H_

#ifdef WIN32
#include <winsock.h>
#include <winsock2.h>
#endif

#include <deque>
#include <string>

#include "components.h"
#include "leak_dumper.h"
#include "properties.h"
#include "texture.h"

using Shared::Graphics::Texture2D;
using Shared::Util::Properties;
using std::deque;
using std::string;

namespace Glest {
namespace Game {

// =====================================================
//	class Logger
//
/// Interface to write log files
// =====================================================

class Logger {
private:
  static const int logLineCount;

private:
  typedef deque<string> Strings;

private:
  string fileName;
  string state;
  string subtitle;
  string current;
  Texture2D *loadingTexture;
  Properties gameHints;
  Properties gameHintsTranslation;
  string gameHintToShow;
  int progress;
  bool showProgressBar;

  string statusText;
  bool cancelSelected;
  GraphicButton buttonCancel;
  Vec4f displayColor;
  GraphicButton buttonNextHint;

private:
  Logger();
  ~Logger();

public:
  static Logger &getInstance();

  // void setMasterserverMode(bool value) { masterserverMode = value; }

  void setFile(const string &fileName) { this->fileName = fileName; }
  void setState(const string &state) { this->state = state; }
  void setSubtitle(const string &subtitle) { this->subtitle = subtitle; }
  void setProgress(int value) { this->progress = value; }
  int getProgress() const { return progress; }
  void showProgress() { showProgressBar = true; }
  void hideProgress() { showProgressBar = false; }

  void add(const string str, bool renderScreen = false,
           const string statusText = "");
  void loadLoadingScreen(string filepath);
  void loadGameHints(string filePathEnglish, string filePathTranslation,
                     bool clearList);
  void renderLoadingScreen();

  void setCancelLoadingEnabled(bool value);
  bool getCancelLoading() const { return cancelSelected; }
  void setCancelLoading(bool value) { cancelSelected = value; }
  void handleMouseClick(int x, int y);
  void clearHints();

  void clear();

private:
  void showNextHint();
};

} // namespace Game
} // namespace Glest

#endif
