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

#ifndef _GLEST_GAME_MAIN_H_
#define _GLEST_GAME_MAIN_H_

#include "leak_dumper.h"
#include "program.h"
#include "window_gl.h"
#include <ctime>

using Shared::Platform::MouseButton;
using Shared::Platform::MouseState;

namespace Glest {
namespace Game {

// =====================================================
// 	class MainWindow
//
///	Main program window
// =====================================================

class MainWindow : public WindowGl {
private:
  Program *program;
  PopupMenu popupMenu;
  int cancelLanguageSelection;
  bool triggerLanguageToggle;
  string triggerLanguage;

  void showLanguages();

public:
  explicit MainWindow(Program *program);
  ~MainWindow();

  void setProgram(Program *program);

  virtual void eventMouseDown(int x, int y, MouseButton mouseButton);
  virtual void eventMouseUp(int x, int y, MouseButton mouseButton);
  virtual void eventMouseDoubleClick(int x, int y, MouseButton mouseButton);
  virtual void eventMouseMove(int x, int y, const MouseState *mouseState);
  virtual bool eventTextInput(std::string text);
  virtual bool eventSdlKeyDown(SDL_KeyboardEvent key);
  virtual void eventKeyDown(SDL_KeyboardEvent key);
  virtual void eventMouseWheel(int x, int y, int zDelta);
  virtual void eventKeyUp(SDL_KeyboardEvent key);
  virtual void eventKeyPress(SDL_KeyboardEvent c);
  virtual void eventActivate(bool active);
  virtual void eventResize(SizeState sizeState);
  virtual void eventClose();
  virtual void eventWindowEvent(SDL_WindowEvent event);

  virtual void render();
  void toggleLanguage(string language);
  bool getTriggerLanguageToggle() const { return triggerLanguageToggle; }
  string getTriggerLanguage() const { return triggerLanguage; }

  virtual int getDesiredScreenWidth();
  virtual int getDesiredScreenHeight();

protected:
  virtual void eventToggleFullScreen(bool isFullscreen);
};

} // namespace Game
} // namespace Glest

#endif
