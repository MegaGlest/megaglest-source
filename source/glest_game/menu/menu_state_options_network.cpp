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

#include "menu_state_options_network.h"

#include "config.h"
#include "core_data.h"
#include "game.h"
#include "leak_dumper.h"
#include "menu_state_keysetup.h"
#include "menu_state_options.h"
#include "menu_state_options_graphics.h"
#include "menu_state_options_network.h"
#include "menu_state_options_sound.h"
#include "menu_state_root.h"
#include "metrics.h"
#include "program.h"
#include "renderer.h"
#include "sound_renderer.h"
#include "string_utils.h"
#include "util.h"

using namespace Shared::Util;

namespace Glest {
namespace Game {

// =====================================================
// 	class MenuStateOptions
// =====================================================
MenuStateOptionsNetwork::MenuStateOptionsNetwork(Program *program,
                                                 MainMenu *mainMenu,
                                                 ProgramState **parentUI)
    : MenuState(program, mainMenu, "config"),
      buttonOk("Options_Network", "buttonOk"),
      buttonReturn("Options_Network", "buttonReturn"),

      buttonKeyboardSetup("Options_Network", "buttonKeyboardSetup"),
      buttonVideoSection("Options_Network", "buttonVideoSection"),
      buttonAudioSection("Options_Network", "buttonAudioSection"),
      buttonMiscSection("Options_Network", "buttonMiscSection"),
      buttonNetworkSettings("Options_Network", "buttonNetworkSettings"),

      mainMessageBox("Options_Network", "mainMessageBox"),

      labelExternalPort("Options_Network", "labelExternalPort"),
      labelServerPortLabel("Options_Network", "labelServerPortLabel"),

      labelPublishServerExternalPort("Options_Network",
                                     "labelPublishServerExternalPort"),
      listBoxServerPort("Options_Network", "listBoxServerPort"),

      labelEnableFTP("Options_Network", "labelEnableFTP"),
      checkBoxEnableFTP("Options_Network", "checkBoxEnableFTP"),

      labelEnableFTPServer("Options_Network", "labelEnableFTPServer"),
      checkBoxEnableFTPServer("Options_Network", "checkBoxEnableFTPServer"),

      labelFTPServerPortLabel("Options_Network", "labelFTPServerPortLabel"),
      labelFTPServerPort("Options_Network", "labelFTPServerPort"),

      labelFTPServerDataPortsLabel("Options_Network",
                                   "labelFTPServerDataPortsLabel"),
      labelFTPServerDataPorts("Options_Network", "labelFTPServerDataPorts"),

      labelEnableFTPServerInternetTilesetXfer(
          "Options_Network", "labelEnableFTPServerInternetTilesetXfer"),
      checkBoxEnableFTPServerInternetTilesetXfer(
          "Options_Network", "checkBoxEnableFTPServerInternetTilesetXfer"),

      labelEnableFTPServerInternetTechtreeXfer(
          "Options_Network", "labelEnableFTPServerInternetTechtreeXfer"),
      checkBoxEnableFTPServerInternetTechtreeXfer(
          "Options_Network", "checkBoxEnableFTPServerInternetTechtreeXfer"),

      labelEnablePrivacy("Options_Network", "labelEnablePrivacy"),
      checkBoxEnablePrivacy("Options_Network", "checkBoxEnablePrivacy")

{
  try {
    containerName = "Options_Network";
    Lang &lang = Lang::getInstance();
    Config &config = Config::getInstance();
    this->parentUI = parentUI;
    this->console.setOnlyChatMessagesInStoredLines(false);

    int leftLabelStart = 100;
    int leftColumnStart = leftLabelStart + 300;
    int buttonRowPos = 50;
    int buttonStartPos = 170;
    int lineOffset = 30;
    int tabButtonWidth = 200;
    int tabButtonHeight = 30;

    mainMessageBox.init(lang.getString("Ok"));
    mainMessageBox.setEnabled(false);
    mainMessageBoxState = 0;

    buttonAudioSection.init(0, 720, tabButtonWidth, tabButtonHeight);
    buttonAudioSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
    buttonAudioSection.setFont3D(
        CoreData::getInstance().getMenuFontVeryBig3D());
    buttonAudioSection.setText(lang.getString("Audio"));
    // Video Section
    buttonVideoSection.init(200, 720, tabButtonWidth, tabButtonHeight);
    buttonVideoSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
    buttonVideoSection.setFont3D(
        CoreData::getInstance().getMenuFontVeryBig3D());
    buttonVideoSection.setText(lang.getString("Video"));
    // currentLine-=lineOffset;
    // MiscSection
    buttonMiscSection.init(400, 720, tabButtonWidth, tabButtonHeight);
    buttonMiscSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
    buttonMiscSection.setFont3D(CoreData::getInstance().getMenuFontVeryBig3D());
    buttonMiscSection.setText(lang.getString("Misc"));
    // NetworkSettings
    buttonNetworkSettings.init(600, 700, tabButtonWidth, tabButtonHeight + 20);
    buttonNetworkSettings.setFont(CoreData::getInstance().getMenuFontVeryBig());
    buttonNetworkSettings.setFont3D(
        CoreData::getInstance().getMenuFontVeryBig3D());
    buttonNetworkSettings.setText(lang.getString("Network"));

    // KeyboardSetup
    buttonKeyboardSetup.init(800, 720, tabButtonWidth, tabButtonHeight);
    buttonKeyboardSetup.setFont(CoreData::getInstance().getMenuFontVeryBig());
    buttonKeyboardSetup.setFont3D(
        CoreData::getInstance().getMenuFontVeryBig3D());
    buttonKeyboardSetup.setText(lang.getString("Keyboardsetup"));

    int currentLine = 650;                    // reset line pos
    int currentLabelStart = leftLabelStart;   // set to right side
    int currentColumnStart = leftColumnStart; // set to right side

    // external server port
    labelPublishServerExternalPort.init(currentLabelStart, currentLine, 150);
    labelPublishServerExternalPort.setText(
        lang.getString("PublishServerExternalPort"));

    labelExternalPort.init(currentColumnStart, currentLine);
    string extPort = config.getString("PortExternal", "not set");
    if (extPort == "not set" || extPort == "0") {
      extPort = "   ---   ";
    } else {
      extPort = "!!! " + extPort + " !!!";
    }
    labelExternalPort.setText(extPort);

    currentLine -= lineOffset;
    // server port
    labelServerPortLabel.init(currentLabelStart, currentLine);
    labelServerPortLabel.setText(lang.getString("ServerPort"));

    listBoxServerPort.init(currentColumnStart, currentLine, 160);

    string portListString = config.getString(
        "PortList", intToStr(GameConstants::serverPort).c_str());
    std::vector<std::string> portList;
    Tokenize(portListString, portList, ",");

    string currentPort = config.getString(
        "PortServer", intToStr(GameConstants::serverPort).c_str());
    int portSelectionIndex = 0;
    for (int idx = 0; idx < (int)portList.size(); idx++) {
      if (portList[idx] != "" && IsNumeric(portList[idx].c_str(), false)) {
        listBoxServerPort.pushBackItem(portList[idx]);
        if (currentPort == portList[idx]) {
          portSelectionIndex = idx;
        }
      }
    }
    listBoxServerPort.setSelectedItemIndex(portSelectionIndex);

    currentLine -= lineOffset;
    labelFTPServerPortLabel.init(currentLabelStart, currentLine);
    labelFTPServerPortLabel.setText(lang.getString("FTPServerPort"));

    int FTPPort = config.getInt(
        "FTPServerPort", intToStr(ServerSocket::getFTPServerPort()).c_str());
    labelFTPServerPort.init(currentColumnStart, currentLine);
    labelFTPServerPort.setText(intToStr(FTPPort));
    currentLine -= lineOffset;
    labelFTPServerDataPortsLabel.init(currentLabelStart, currentLine);
    labelFTPServerDataPortsLabel.setText(lang.getString("FTPServerDataPort"));

    char szBuf[8096] = "";
    snprintf(szBuf, 8096, "%d - %d", FTPPort + 1,
             FTPPort + GameConstants::maxPlayers);

    labelFTPServerDataPorts.init(currentColumnStart, currentLine);
    labelFTPServerDataPorts.setText(szBuf);
    currentLine -= lineOffset;
    labelEnableFTPServer.init(currentLabelStart, currentLine);
    labelEnableFTPServer.setText(lang.getString("EnableFTPServer"));

    checkBoxEnableFTPServer.init(currentColumnStart, currentLine);
    checkBoxEnableFTPServer.setValue(config.getBool("EnableFTPServer", "true"));
    currentLine -= lineOffset;
    // FTP Config - start
    labelEnableFTP.init(currentLabelStart, currentLine);
    labelEnableFTP.setText(lang.getString("EnableFTP"));

    checkBoxEnableFTP.init(currentColumnStart, currentLine);
    checkBoxEnableFTP.setValue(config.getBool("EnableFTPXfer", "true"));
    currentLine -= lineOffset;

    labelEnableFTPServerInternetTilesetXfer.init(currentLabelStart,
                                                 currentLine);
    labelEnableFTPServerInternetTilesetXfer.setText(
        lang.getString("EnableFTPServerInternetTilesetXfer"));

    checkBoxEnableFTPServerInternetTilesetXfer.init(currentColumnStart,
                                                    currentLine);
    checkBoxEnableFTPServerInternetTilesetXfer.setValue(
        config.getBool("EnableFTPServerInternetTilesetXfer", "true"));

    currentLine -= lineOffset;

    labelEnableFTPServerInternetTechtreeXfer.init(currentLabelStart,
                                                  currentLine);
    labelEnableFTPServerInternetTechtreeXfer.setText(
        lang.getString("EnableFTPServerInternetTechtreeXfer"));

    checkBoxEnableFTPServerInternetTechtreeXfer.init(currentColumnStart,
                                                     currentLine);
    checkBoxEnableFTPServerInternetTechtreeXfer.setValue(
        config.getBool("EnableFTPServerInternetTechtreeXfer", "true"));

    currentLine -= lineOffset;

    // FTP config end

    // Privacy flag
    labelEnablePrivacy.init(currentLabelStart, currentLine);
    labelEnablePrivacy.setText(lang.getString("PrivacyPlease"));

    checkBoxEnablePrivacy.init(currentColumnStart, currentLine);
    checkBoxEnablePrivacy.setValue(config.getBool("PrivacyPlease", "false"));
    // currentLine-=lineOffset;
    //  end

    // buttons
    buttonOk.init(buttonStartPos, buttonRowPos, 100);
    buttonOk.setText(lang.getString("Save"));

    buttonReturn.setText(lang.getString("Return"));
    buttonReturn.init(buttonStartPos + 110, buttonRowPos, 100);

    GraphicComponent::applyAllCustomProperties(containerName);
  } catch (exception &e) {
    SystemFlags::OutputDebug(SystemFlags::debugError,
                             "In [%s::%s Line: %d] Error loading options: %s\n",
                             __FILE__, __FUNCTION__, __LINE__, e.what());
    throw megaglest_runtime_error(string("Error loading options msg: ") +
                                  e.what());
  }
}

void MenuStateOptionsNetwork::reloadUI() {
  Lang &lang = Lang::getInstance();

  mainMessageBox.init(lang.getString("Ok"));

  buttonAudioSection.setText(lang.getString("Audio"));
  buttonVideoSection.setText(lang.getString("Video"));
  buttonMiscSection.setText(lang.getString("Misc"));
  buttonNetworkSettings.setText(lang.getString("Network"));

  std::vector<string> listboxData;
  listboxData.push_back("None");
  listboxData.push_back("OpenAL");

  listboxData.clear();
  listboxData.push_back("Bilinear");
  listboxData.push_back("Trilinear");

  listboxData.clear();
  for (float f = 0.0; f < 2.1f; f = f + 0.1f) {
    listboxData.push_back(floatToStr(f));
  }
  listboxData.clear();
  for (int i = 0; i < Renderer::sCount; ++i) {
    listboxData.push_back(lang.getString(
        Renderer::shadowsToStr(static_cast<Renderer::Shadows>(i))));
  }

  labelServerPortLabel.setText(lang.getString("ServerPort"));
  labelPublishServerExternalPort.setText(
      lang.getString("PublishServerExternalPort"));
  labelEnableFTP.setText(lang.getString("EnableFTP"));
  labelEnableFTPServer.setText(lang.getString("EnableFTPServer"));
  labelFTPServerPortLabel.setText(lang.getString("FTPServerPort"));
  labelFTPServerDataPortsLabel.setText(lang.getString("FTPServerDataPort"));
  labelEnableFTPServerInternetTilesetXfer.setText(
      lang.getString("EnableFTPServerInternetTilesetXfer"));
  labelEnableFTPServerInternetTechtreeXfer.setText(
      lang.getString("EnableFTPServerInternetTechtreeXfer"));
  labelEnablePrivacy.setText(lang.getString("PrivacyPlease"));
  buttonOk.setText(lang.getString("Save"));
  buttonReturn.setText(lang.getString("Return"));
}

void MenuStateOptionsNetwork::mouseClick(int x, int y,
                                         MouseButton mouseButton) {
  CoreData &coreData = CoreData::getInstance();
  SoundRenderer &soundRenderer = SoundRenderer::getInstance();

  if (mainMessageBox.getEnabled()) {
    int button = 0;
    if (mainMessageBox.mouseClick(x, y, button)) {
      soundRenderer.playFx(coreData.getClickSoundA());
      if (button == 0) {
        if (mainMessageBoxState == 1) {
          mainMessageBoxState = 0;
          mainMessageBox.setEnabled(false);
          saveConfig();

          Lang &lang = Lang::getInstance();
          mainMessageBox.init(lang.getString("Ok"));
          mainMenu->setState(new MenuStateOptions(program, mainMenu));
        } else {
          mainMessageBox.setEnabled(false);

          Lang &lang = Lang::getInstance();
          mainMessageBox.init(lang.getString("Ok"));
        }
      }
    }
  } else if (buttonOk.mouseClick(x, y)) {
    soundRenderer.playFx(coreData.getClickSoundA());
    saveConfig();
    // mainMenu->setState(new MenuStateOptions(program,
    // mainMenu,this->parentUI));
    return;
  } else if (buttonReturn.mouseClick(x, y)) {
    soundRenderer.playFx(coreData.getClickSoundA());
    if (this->parentUI != NULL) {
      *this->parentUI = NULL;
      delete *this->parentUI;
    }
    mainMenu->setState(new MenuStateRoot(program, mainMenu));
    return;
  } else if (buttonAudioSection.mouseClick(x, y)) {
    soundRenderer.playFx(coreData.getClickSoundA());
    mainMenu->setState(new MenuStateOptionsSound(
        program, mainMenu,
        this->parentUI)); // open keyboard shortcuts setup screen
    return;
  } else if (buttonNetworkSettings.mouseClick(x, y)) {
    soundRenderer.playFx(coreData.getClickSoundA());
    // mainMenu->setState(new MenuStateOptionsNetwork(program,
    // mainMenu,this->parentUI)); // open keyboard shortcuts setup screen
    return;
  } else if (buttonMiscSection.mouseClick(x, y)) {
    soundRenderer.playFx(coreData.getClickSoundA());
    mainMenu->setState(new MenuStateOptions(
        program, mainMenu,
        this->parentUI)); // open keyboard shortcuts setup screen
    return;
  } else if (buttonVideoSection.mouseClick(x, y)) {
    soundRenderer.playFx(coreData.getClickSoundA());
    mainMenu->setState(new MenuStateOptionsGraphics(
        program, mainMenu,
        this->parentUI)); // open keyboard shortcuts setup screen
    return;
  } else if (buttonKeyboardSetup.mouseClick(x, y)) {
    soundRenderer.playFx(coreData.getClickSoundA());
    mainMenu->setState(new MenuStateKeysetup(
        program, mainMenu,
        this->parentUI)); // open keyboard shortcuts setup screen
    // showMessageBox("Not implemented yet", "Keyboard setup", false);
    return;
  } else {
    if (listBoxServerPort.mouseClick(x, y)) {
      int selectedPort = strToInt(listBoxServerPort.getSelectedItem());
      if (selectedPort < 10000) {
        selectedPort = GameConstants::serverPort;
      }
      // use the following ports for ftp
      char szBuf[8096] = "";
      snprintf(szBuf, 8096, "%d - %d", selectedPort + 2,
               selectedPort + 1 + GameConstants::maxPlayers);
      labelFTPServerPort.setText(intToStr(selectedPort + 1));
      labelFTPServerDataPorts.setText(szBuf);
    }

    checkBoxEnableFTP.mouseClick(x, y);
    checkBoxEnableFTPServer.mouseClick(x, y);

    checkBoxEnableFTPServerInternetTilesetXfer.mouseClick(x, y);
    checkBoxEnableFTPServerInternetTechtreeXfer.mouseClick(x, y);

    checkBoxEnablePrivacy.mouseClick(x, y);
  }
}

void MenuStateOptionsNetwork::mouseMove(int x, int y, const MouseState *ms) {
  if (mainMessageBox.getEnabled()) {
    mainMessageBox.mouseMove(x, y);
  }

  buttonOk.mouseMove(x, y);
  buttonReturn.mouseMove(x, y);
  buttonKeyboardSetup.mouseMove(x, y);
  buttonAudioSection.mouseMove(x, y);
  buttonNetworkSettings.mouseMove(x, y);
  buttonMiscSection.mouseMove(x, y);
  buttonVideoSection.mouseMove(x, y);
  listBoxServerPort.mouseMove(x, y);
  checkBoxEnableFTP.mouseMove(x, y);
  checkBoxEnableFTPServer.mouseMove(x, y);
  checkBoxEnableFTPServerInternetTilesetXfer.mouseMove(x, y);
  checkBoxEnableFTPServerInternetTechtreeXfer.mouseMove(x, y);
  checkBoxEnablePrivacy.mouseMove(x, y);
}

// bool MenuStateOptionsNetwork::isInSpecialKeyCaptureEvent() {
//	return (activeInputLabel != NULL);
// }
//
// void MenuStateOptionsNetwork::keyDown(SDL_KeyboardEvent key) {
//	if(activeInputLabel != NULL) {
//		keyDownEditLabel(key, &activeInputLabel);
//	}
// }

void MenuStateOptionsNetwork::keyPress(SDL_KeyboardEvent c) {
  //	if(activeInputLabel != NULL) {
  //	    //printf("[%d]\n",c); fflush(stdout);
  //		if( &labelPlayerName 	== activeInputLabel ||
  //			&labelTransifexUser == activeInputLabel ||
  //			&labelTransifexPwd == activeInputLabel ||
  //			&labelTransifexI18N == activeInputLabel) {
  //			textInputEditLabel(c, &activeInputLabel);
  //		}
  //	}
  //	else {
  Config &configKeys = Config::getInstance(
      std::pair<ConfigType, ConfigType>(cfgMainKeys, cfgUserKeys));
  if (isKeyPressed(configKeys.getSDLKey("SaveGUILayout"), c) == true) {
    GraphicComponent::saveAllCustomProperties(containerName);
    // Lang &lang= Lang::getInstance();
    // console.addLine(lang.getString("GUILayoutSaved") + " [" + (saved ?
    // lang.getString("Yes") : lang.getString("No"))+ "]");
  }
  //	}
}

void MenuStateOptionsNetwork::render() {
  Renderer &renderer = Renderer::getInstance();

  if (mainMessageBox.getEnabled()) {
    renderer.renderMessageBox(&mainMessageBox);
  } else {
    renderer.renderButton(&buttonOk);
    renderer.renderButton(&buttonReturn);
    renderer.renderButton(&buttonKeyboardSetup);
    renderer.renderButton(&buttonVideoSection);
    renderer.renderButton(&buttonAudioSection);
    renderer.renderButton(&buttonMiscSection);
    renderer.renderButton(&buttonNetworkSettings);
    renderer.renderLabel(&labelServerPortLabel);
    renderer.renderLabel(&labelExternalPort);
    renderer.renderLabel(&labelPublishServerExternalPort);
    renderer.renderListBox(&listBoxServerPort);

    renderer.renderLabel(&labelEnableFTP);
    renderer.renderCheckBox(&checkBoxEnableFTP);

    renderer.renderLabel(&labelEnableFTPServer);
    renderer.renderCheckBox(&checkBoxEnableFTPServer);

    renderer.renderLabel(&labelFTPServerPortLabel);
    renderer.renderLabel(&labelFTPServerPort);
    renderer.renderLabel(&labelFTPServerDataPortsLabel);
    renderer.renderLabel(&labelFTPServerDataPorts);

    renderer.renderLabel(&labelEnableFTPServerInternetTilesetXfer);
    renderer.renderCheckBox(&checkBoxEnableFTPServerInternetTilesetXfer);
    renderer.renderLabel(&labelEnableFTPServerInternetTechtreeXfer);
    renderer.renderCheckBox(&checkBoxEnableFTPServerInternetTechtreeXfer);

    renderer.renderLabel(&labelEnablePrivacy);
    renderer.renderCheckBox(&checkBoxEnablePrivacy);
  }

  renderer.renderConsole(&console);
  if (program != NULL)
    program->renderProgramMsgBox();
}

void MenuStateOptionsNetwork::saveConfig() {
  Config &config = Config::getInstance();
  Lang &lang = Lang::getInstance();
  setActiveInputLable(NULL);

  lang.loadGameStrings(config.getString("Lang"));

  config.setString("PortServer", listBoxServerPort.getSelectedItem());
  config.setInt("FTPServerPort", config.getInt("PortServer") + 1);
  config.setBool("EnableFTPXfer", checkBoxEnableFTP.getValue());
  config.setBool("EnableFTPServer", checkBoxEnableFTPServer.getValue());

  config.setBool("EnableFTPServerInternetTilesetXfer",
                 checkBoxEnableFTPServerInternetTilesetXfer.getValue());
  config.setBool("EnableFTPServerInternetTechtreeXfer",
                 checkBoxEnableFTPServerInternetTechtreeXfer.getValue());

  config.setBool("PrivacyPlease", checkBoxEnablePrivacy.getValue());

  config.save();

  Renderer::getInstance().loadConfig();
  console.addLine(lang.getString("SettingsSaved"));
}

void MenuStateOptionsNetwork::setActiveInputLable(GraphicLabel *newLable) {}

} // namespace Game
} // namespace Glest
