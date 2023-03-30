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

#include "client_interface.h"

#include "logger.h"
#include "window.h"

#include "config.h"
#include "conversion.h"
#include "game_util.h"
#include "lang.h"
#include "platform_util.h"
#include <cassert>
#include <stdexcept>

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Platform;
using namespace Shared::Util;

#ifdef WIN32

#define snprintf _snprintf

#endif

namespace Glest {
namespace Game {

const bool debugClientInterfacePerf = false;

const int ClientInterface::messageWaitTimeout = 10000; // 10 seconds
const int ClientInterface::waitSleepTime = 10;
const int ClientInterface::maxNetworkCommandListSendTimeWait = 5;

// =====================================================
//	class ClientInterfaceThread
// =====================================================

ClientInterfaceThread::ClientInterfaceThread(ClientInterface *client)
    : BaseThread() {
  this->clientInterface = client;
  this->uniqueID = "ClientInterfaceThread";
}

ClientInterfaceThread::~ClientInterfaceThread() {
  this->clientInterface = NULL;
}

void ClientInterfaceThread::setQuitStatus(bool value) {
  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem,
                             "In [%s::%s] Line: %d value = %d\n", __FILE__,
                             __FUNCTION__, __LINE__, value);

  BaseThread::setQuitStatus(value);

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s] Line: %d\n",
                             __FILE__, __FUNCTION__, __LINE__);
}

bool ClientInterfaceThread::canShutdown(bool deleteSelfIfShutdownDelayed) {
  bool ret = (getExecutingTask() == false);
  if (ret == false && deleteSelfIfShutdownDelayed == true) {
    setDeleteSelfOnExecutionDone(deleteSelfIfShutdownDelayed);
    deleteSelfIfRequired();
    signalQuit();
  }

  return ret;
}

void ClientInterfaceThread::execute() {
  RunningStatusSafeWrapper runningStatus(this);
  try {
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line: %d]\n", __FILE__, __FUNCTION__,
                               __LINE__);
    if (SystemFlags::VERBOSE_MODE_ENABLED)
      printf("In [%s::%s Line: %d] ****************** STARTING worker thread "
             "this = %p\n",
             __FILE__, __FUNCTION__, __LINE__, this);

    // Set socket to non blocking
    if (clientInterface != NULL && clientInterface->getSocket(true) != NULL) {
      clientInterface->getSocket(true)->setBlock(false);
    }

    if (SystemFlags::VERBOSE_MODE_ENABLED)
      printf("ClientInterfaceThread::exec Line: %d\n", __LINE__);

    time_t clientSimulationLagStartTime = 0;
    Chrono chrono;
    for (; this->clientInterface != NULL;) {

      if (getQuitStatus() == true) {
        if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
          SystemFlags::OutputDebug(SystemFlags::debugSystem,
                                   "In [%s::%s Line: %d]\n", __FILE__,
                                   __FUNCTION__, __LINE__);
        break;
      }

      ExecutingTaskSafeWrapper safeExecutingTaskMutex(this);

      if (debugClientInterfacePerf == true)
        printf("START === Client thread\n");

      // printf("ClientInterfaceThread::exec Line: %d\n",__LINE__);

      uint64 loopCount = 0;
      if (debugClientInterfacePerf == true) {
        chrono.start();
      }
      while (this->getQuitStatus() == false && clientInterface != NULL) {
        // printf("ClientInterfaceThread::exec Line: %d this->getQuitStatus():
        // %d\n",__LINE__,this->getQuitStatus());

        // START: Test simulating lag for the client
        int simulateLag =
            Config::getInstance().getInt("SimulateClientLag", "0");
        if (simulateLag > 0) {
          if (clientSimulationLagStartTime == 0) {
            clientSimulationLagStartTime = time(NULL);
          }
          if (difftime((long int)time(NULL), clientSimulationLagStartTime) <=
              Config::getInstance().getInt("SimulateClientLagDurationSeconds",
                                           "0")) {
            sleep(simulateLag);
          }
        }
        // END: Test simulating lag for the client

        clientInterface->updateNetworkFrame();

        // printf("ClientInterfaceThread::exec Line: %d this->getQuitStatus():
        // %d\n",__LINE__,this->getQuitStatus());

        if (debugClientInterfacePerf == true) {
          loopCount++;
          if (chrono.getMillis() >= 1000) {
            printf("Client thread loopCount = %llu\n",
                   (long long unsigned int)loopCount);

            loopCount = 0;
            // sleep(0);
            chrono.start();
          }
        }
      }

      if (debugClientInterfacePerf == true) {
        printf("END === Client thread\n");
      }

      // printf("ClientInterfaceThread::exec Line: %d\n",__LINE__);

      if (getQuitStatus() == true) {
        if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
          SystemFlags::OutputDebug(SystemFlags::debugSystem,
                                   "In [%s::%s Line: %d]\n", __FILE__,
                                   __FUNCTION__, __LINE__);
        break;
      }

      // printf("ClientInterfaceThread::exec Line: %d\n",__LINE__);
    }

    if (SystemFlags::VERBOSE_MODE_ENABLED)
      printf("ClientInterfaceThread::exec Line: %d\n", __LINE__);
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line: %d]\n", __FILE__, __FUNCTION__,
                               __LINE__);
    if (SystemFlags::VERBOSE_MODE_ENABLED)
      printf("In [%s::%s Line: %d] ****************** ENDING worker thread "
             "this = %p\n",
             __FILE__, __FUNCTION__, __LINE__, this);
  } catch (const exception &ex) {

    SystemFlags::OutputDebug(SystemFlags::debugError,
                             "In [%s::%s Line: %d] Error [%s]\n", __FILE__,
                             __FUNCTION__, __LINE__, ex.what());
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line: %d]\n", __FILE__, __FUNCTION__,
                               __LINE__);

    if (clientInterface == NULL || clientInterface->getSocket(true) == NULL ||
        clientInterface->getSocket(true)->isConnected() == true) {
      throw megaglest_runtime_error(ex.what());
    }
  } catch (...) {
    char szBuf[8096] = "";
    snprintf(szBuf, 8096, "In [%s::%s %d] UNKNOWN error\n", __FILE__,
             __FUNCTION__, __LINE__);
    SystemFlags::OutputDebug(SystemFlags::debugError, szBuf);
    throw megaglest_runtime_error(szBuf);
  }

  if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s] Line: %d\n",
                             __FILE__, __FUNCTION__, __LINE__);
}

// =====================================================
//	class ClientInterface
// =====================================================

ClientInterface::ClientInterface() : GameNetworkInterface() {
  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,
                             "In [%s::%s Line: %d] constructor for %p\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__, this);

  networkCommandListThreadAccessor = new Mutex(CODE_AT_LINE);
  networkCommandListThread = NULL;
  cachedPendingCommandsIndex = 0;
  cachedLastPendingFrameCount = 0;
  timeClientWaitedForLastMessage = 0;

  flagAccessor = new Mutex(CODE_AT_LINE);

  clientSocket = NULL;
  sessionKey = 0;
  launchGame = false;
  introDone = false;

  this->joinGameInProgress = false;
  this->joinGameInProgressLaunch = false;
  this->readyForInGameJoin = false;
  this->resumeInGameJoin = false;

  quitThreadAccessor = new Mutex(CODE_AT_LINE);
  setQuitThread(false);

  playerIndex = -1;
  gameSettingsReceivedCount = 0;
  setGameSettingsReceived(false);
  gameSettingsReceivedCount = 0;
  connectedTime = 0;
  port = 0;
  serverFTPPort = 0;

  gotIntro = false;
  lastNetworkCommandListSendTime = 0;
  currentFrameCount = 0;
  lastSentFrameCount = 0;
  clientSimulationLagStartTime = 0;

  networkGameDataSynchCheckOkMap = false;
  networkGameDataSynchCheckOkTile = false;
  networkGameDataSynchCheckOkTech = false;
  this->setNetworkGameDataSynchCheckTechMismatchReport("");
  this->setReceivedDataSynchCheck(false);
}

void ClientInterface::shutdownNetworkCommandListThread(
    MutexSafeWrapper &safeMutexWrapper) {
  if (networkCommandListThread != NULL) {
    // printf("START === shutdownNetworkCommandListThread\n");
    if (SystemFlags::VERBOSE_MODE_ENABLED)
      printf("%s Line: %d\n", __FUNCTION__, __LINE__);

    setQuitThread(true);
    networkCommandListThread->signalQuit();
    safeMutexWrapper.ReleaseLock(true);

    if (SystemFlags::VERBOSE_MODE_ENABLED)
      printf("%s Line: %d\n", __FUNCTION__, __LINE__);

    Chrono chronoElapsed(true);
    for (; chronoElapsed.getMillis() <= 10000;) {
      safeMutexWrapper.Lock();
      if (networkCommandListThread != NULL &&
          networkCommandListThread->canShutdown(false) == false &&
          networkCommandListThread->getRunningStatus() == true) {

        safeMutexWrapper.ReleaseLock(true);
        if (chronoElapsed.getMillis() % 1000 == 0) {
          sleep(1);
          if (SystemFlags::VERBOSE_MODE_ENABLED)
            printf("%s Line: %d\n", __FUNCTION__, __LINE__);
        }
      } else {
        safeMutexWrapper.ReleaseLock(true);
        break;
      }
      // printf("%s Line: %d\n",__FUNCTION__,__LINE__);
    }

    if (SystemFlags::VERBOSE_MODE_ENABLED)
      printf("%s Line: %d\n chronoElapsed.getMillis(): %lld", __FUNCTION__,
             __LINE__, (long long int)chronoElapsed.getMillis());
    // printf("A === shutdownNetworkCommandListThread\n");

    safeMutexWrapper.Lock();
    if (networkCommandListThread != NULL &&
        networkCommandListThread->canShutdown(true)) {

      if (SystemFlags::VERBOSE_MODE_ENABLED)
        printf("%s Line: %d\n", __FUNCTION__, __LINE__);

      delete networkCommandListThread;
      networkCommandListThread = NULL;
    } else {
      networkCommandListThread = NULL;
    }

    if (SystemFlags::VERBOSE_MODE_ENABLED)
      printf("%s Line: %d\n", __FUNCTION__, __LINE__);
    // printf("END === shutdownNetworkCommandListThread\n");
  }
}

ClientInterface::~ClientInterface() {
  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,
                             "In [%s::%s Line: %d] destructor for %p\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__, this);
  // printf("START === Client destructor\n");

  if (SystemFlags::VERBOSE_MODE_ENABLED)
    printf("%s Line: %d\n", __FUNCTION__, __LINE__);

  MutexSafeWrapper safeMutex(networkCommandListThreadAccessor, CODE_AT_LINE);

  if (SystemFlags::VERBOSE_MODE_ENABLED)
    printf("%s Line: %d\n", __FUNCTION__, __LINE__);

  shutdownNetworkCommandListThread(safeMutex);
  // printf("A === Client destructor\n");

  if (SystemFlags::VERBOSE_MODE_ENABLED)
    printf("%s Line: %d\n", __FUNCTION__, __LINE__);

  if (clientSocket != NULL && clientSocket->isConnected() == true) {

    if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugNetwork,
                               "In [%s::%s Line: %d]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__);

    if (SystemFlags::VERBOSE_MODE_ENABLED)
      printf("%s Line: %d\n", __FUNCTION__, __LINE__);

    Lang &lang = Lang::getInstance();
    const vector<string> languageList =
        this->gameSettings.getUniqueNetworkPlayerLanguages();
    for (unsigned int langIndex = 0; langIndex < languageList.size();
         ++langIndex) {

      string sQuitText = "has chosen to leave the game!";
      if (lang.hasString("PlayerLeftGame", languageList[langIndex]) == true) {
        sQuitText = lang.getString("PlayerLeftGame", languageList[langIndex]);
      }

      if (clientSocket != NULL && clientSocket->isConnected() == true) {
        sendTextMessage(sQuitText, -1, false, languageList[langIndex]);
      }
    }
  }

  if (SystemFlags::VERBOSE_MODE_ENABLED)
    printf("%s Line: %d\n", __FUNCTION__, __LINE__);

  // printf("B === Client destructor\n");

  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(
        SystemFlags::debugNetwork, "In [%s::%s Line: %d]\n",
        extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__, __LINE__);

  close(false);

  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(
        SystemFlags::debugNetwork, "In [%s::%s Line: %d]\n",
        extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__, __LINE__);

  if (SystemFlags::VERBOSE_MODE_ENABLED)
    printf("%s Line: %d\n", __FUNCTION__, __LINE__);

  if (SystemFlags::VERBOSE_MODE_ENABLED)
    printf("%s Line: %d\n", __FUNCTION__, __LINE__);

  // printf("C === Client destructor\n");

  networkCommandListThreadAccessor = NULL;
  safeMutex.ReleaseLock(false, true);

  if (SystemFlags::VERBOSE_MODE_ENABLED)
    printf("%s Line: %d\n", __FUNCTION__, __LINE__);

  delete flagAccessor;
  flagAccessor = NULL;
  // printf("END === Client destructor\n");

  delete quitThreadAccessor;
  quitThreadAccessor = NULL;

  if (SystemFlags::VERBOSE_MODE_ENABLED)
    printf("%s Line: %d\n", __FUNCTION__, __LINE__);
  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(
        SystemFlags::debugNetwork, "In [%s::%s Line: %d]\n",
        extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__, __LINE__);
}

bool ClientInterface::getQuitThread() {
  MutexSafeWrapper safeMutex(quitThreadAccessor, CODE_AT_LINE);
  return this->quitThread;
}
void ClientInterface::setQuitThread(bool value) {
  MutexSafeWrapper safeMutex(quitThreadAccessor, CODE_AT_LINE);
  this->quitThread = value;
}

bool ClientInterface::getQuit() {
  MutexSafeWrapper safeMutex(quitThreadAccessor, CODE_AT_LINE);
  return this->quit;
}
void ClientInterface::setQuit(bool value) {
  MutexSafeWrapper safeMutex(quitThreadAccessor, CODE_AT_LINE);
  this->quit = value;
}

bool ClientInterface::getJoinGameInProgress() {
  MutexSafeWrapper safeMutex(flagAccessor, CODE_AT_LINE);
  return joinGameInProgress;
}
bool ClientInterface::getJoinGameInProgressLaunch() {
  MutexSafeWrapper safeMutex(flagAccessor, CODE_AT_LINE);
  return joinGameInProgressLaunch;
}

bool ClientInterface::getReadyForInGameJoin() {
  MutexSafeWrapper safeMutex(flagAccessor, CODE_AT_LINE);
  return readyForInGameJoin;
}

bool ClientInterface::getResumeInGameJoin() {
  MutexSafeWrapper safeMutex(flagAccessor, CODE_AT_LINE);
  return resumeInGameJoin;
}

void ClientInterface::connect(const Ip &ip, int port) {
  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugNetwork, "In [%s::%s] START\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__);

  this->ip = ip;
  this->port = port;

  MutexSafeWrapper safeMutex(networkCommandListThreadAccessor, CODE_AT_LINE);
  shutdownNetworkCommandListThread(safeMutex);

  delete clientSocket;
  clientSocket = NULL;

  safeMutex.ReleaseLock();

  clientSocket = new ClientSocket();
  clientSocket->setBlock(false);
  clientSocket->connect(ip, port);
  connectedTime = time(NULL);
  // clientSocket->setBlock(true);

  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,
                             "In [%s::%s] END - socket = %d\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, clientSocket->getSocketId());
}

void ClientInterface::reset() {
  if (getSocket() != NULL) {
    Lang &lang = Lang::getInstance();
    const vector<string> languageList =
        this->gameSettings.getUniqueNetworkPlayerLanguages();
    for (unsigned int langIndex = 0; langIndex < languageList.size();
         ++langIndex) {

      string sQuitText = "has chosen to leave the game!";
      if (lang.hasString("PlayerLeftGame", languageList[langIndex]) == true) {
        sQuitText = lang.getString("PlayerLeftGame", languageList[langIndex]);
      }
      sendTextMessage(sQuitText, -1, false, languageList[langIndex]);
    }
    close();
  }
}

void ClientInterface::update() {
  bool wasConnected = this->isConnected();
  if (gotIntro == true && wasConnected == false) {
    string playerNameStr = getHumanPlayerName();

    Lang &lang = Lang::getInstance();

    char szBuf1[8096] = "";
    string statusTextFormat = lang.getString("PlayerDisconnected");
    snprintf(szBuf1, 8096, statusTextFormat.c_str(), playerNameStr.c_str());

    DisplayErrorMessage(szBuf1);
    setQuit(true);
    return;
  }

  try {
    NetworkMessageCommandList networkMessageCommandList(currentFrameCount);
    for (int index = 0; index < GameConstants::maxPlayers; ++index) {
      networkMessageCommandList.setNetworkPlayerFactionCRC(
          index, this->getNetworkPlayerFactionCRC(index));
    }

    // send as many commands as we can
    while (requestedCommands.empty() == false) {
      if (networkMessageCommandList.addCommand(&requestedCommands.back())) {
        requestedCommands.pop_back();
      } else {
        break;
      }
    }

    double lastSendElapsed =
        difftime((long int)time(NULL), lastNetworkCommandListSendTime);

    // If we are on a frame that should send packets or we have commands
    // to send now, send it now.
    if ((currentFrameCount >= this->gameSettings.getNetworkFramePeriod() &&
         currentFrameCount % this->gameSettings.getNetworkFramePeriod() == 0) ||
        networkMessageCommandList.getCommandCount() > 0) {

      if (lastSentFrameCount < currentFrameCount ||
          networkMessageCommandList.getCommandCount() > 0) {

        lastSentFrameCount = currentFrameCount;
        sendMessage(&networkMessageCommandList);

        lastNetworkCommandListSendTime = time(NULL);
        lastSendElapsed = 0;
      }
    }

    // If we have not sent anything for maxNetworkCommandListSendTimeWait
    // seconds, send one now.
    if (lastNetworkCommandListSendTime > 0 &&
        lastSendElapsed >= ClientInterface::maxNetworkCommandListSendTimeWait) {

      lastSentFrameCount = currentFrameCount;
      sendMessage(&networkMessageCommandList);
      lastNetworkCommandListSendTime = time(NULL);
    }

    // Possible cause of out of synch since we have more commands that need
    // to be sent in this frame
    if (requestedCommands.empty() == false) {
      if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,
                                 "In [%s::%s Line: %d] WARNING / ERROR, "
                                 "requestedCommands.size() = %d\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__,
                                 requestedCommands.size());

      string sMsg = "may go out of synch: client requestedCommands.size() = " +
                    intToStr(requestedCommands.size());
      sendTextMessage(sMsg, -1, true, "");
      sleep(1);
    }
  } catch (const megaglest_runtime_error &ex) {
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,
                             "In [%s::%s Line: %d] Error [%s]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__, ex.what());
    SystemFlags::OutputDebug(SystemFlags::debugError,
                             "In [%s::%s Line: %d] Error [%s]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__, ex.what());

    if (this->isConnected() == false) {
      if (gotIntro == false || wasConnected == false) {
        string sErr = string(extractFileFromDirectoryPath(__FILE__).c_str()) +
                      "::" + string(__FUNCTION__) +
                      " network error: " + string(ex.what());
        DisplayErrorMessage(sErr);
      }

      setQuit(true);
    } else {
      throw megaglest_runtime_error(ex.what());
    }
  }
}

std::string ClientInterface::getIpAddress(bool mutexLock) {
  string result = "";
  // MutexSafeWrapper safeMutexSlot((mutexLock == true ? mutexSocket :
  // NULL),CODE_AT_LINE);
  if (clientSocket != NULL) {
    result = clientSocket->getIpAddress();
  }
  return result;
}

std::string ClientInterface::getServerIpAddress() {
  return this->ip.getString();
}

void ClientInterface::updateLobby() {
  Chrono chrono;
  if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled)
    chrono.start();

  NetworkMessageType networkMessageType = getNextMessageType();

  if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
          .enabled &&
      chrono.getMillis() > 0)
    SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                             "In [%s::%s Line: %d] took msecs: %lld\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__, chrono.getMillis());
  if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
          .enabled &&
      chrono.getMillis() > 0)
    chrono.start();

  switch (networkMessageType) {
  case nmtInvalid:
    break;

  case nmtIntro: {
    NetworkMessageIntro networkMessageIntro;
    if (receiveMessage(&networkMessageIntro)) {
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                                 "In [%s::%s Line: %d] took msecs: %lld\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__, chrono.getMillis());
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        chrono.start();

      gotIntro = true;
      sessionKey = networkMessageIntro.getSessionId();
      versionString = networkMessageIntro.getVersionString();
      playerIndex = networkMessageIntro.getPlayerIndex();
      serverName = networkMessageIntro.getName();
      serverUUID = networkMessageIntro.getPlayerUUID();
      serverPlatform = networkMessageIntro.getPlayerPlatform();
      serverFTPPort = networkMessageIntro.getFtpPort();

      if (playerIndex < 0 || playerIndex >= GameConstants::maxPlayers) {
        throw megaglest_runtime_error(
            "playerIndex < 0 || playerIndex >= GameConstants::maxPlayers");
      }

      MutexSafeWrapper safeMutexFlags(flagAccessor, CODE_AT_LINE);
      this->joinGameInProgress = (networkMessageIntro.getGameInProgress() != 0);
      this->joinGameInProgressLaunch = false;
      safeMutexFlags.ReleaseLock();

      // printf("Client got intro playerIndex = %d\n",playerIndex);

      if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
        SystemFlags::OutputDebug(
            SystemFlags::debugNetwork,
            "In [%s::%s Line: %d] got NetworkMessageIntro, "
            "networkMessageIntro.getGameState() = %d, versionString [%s], "
            "sessionKey = %d, playerIndex = %d, serverFTPPort = %d\n",
            extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
            __LINE__, networkMessageIntro.getGameState(), versionString.c_str(),
            sessionKey, playerIndex, serverFTPPort);

      // check consistency
      bool compatible = checkVersionComptability(
          networkMessageIntro.getVersionString(), getNetworkVersionGITString());

      if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
        SystemFlags::OutputDebug(
            SystemFlags::debugNetwork,
            "In [%s::%s Line: %d] got NetworkMessageIntro, "
            "networkMessageIntro.getGameState() = %d, versionString [%s], "
            "sessionKey = %d, playerIndex = %d, serverFTPPort = %d\n",
            extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
            __LINE__, networkMessageIntro.getGameState(), versionString.c_str(),
            sessionKey, playerIndex, serverFTPPort);

      if (compatible == false) {
        if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                .enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugNetwork, "In [%s::%s Line: %d]\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__);

        bool versionMatched = false;
        string platformFreeVersion = getNetworkPlatformFreeVersionString();
        string sErr = "";

        if (strncmp(platformFreeVersion.c_str(),
                    networkMessageIntro.getVersionString().c_str(),
                    strlen(platformFreeVersion.c_str())) != 0) {
          string playerNameStr = getHumanPlayerName();
          sErr = "Server and client binary mismatch!\nYou have to use the "
                 "exactly same binaries!\n\nServer: " +
                 networkMessageIntro.getVersionString() +
                 "\nClient: " + getNetworkVersionGITString() + " player [" +
                 playerNameStr + "]";
          printf("%s\n", sErr.c_str());

          sendTextMessage("Server and client binary mismatch!!", -1, true, "");
          sendTextMessage(" Server:" + networkMessageIntro.getVersionString(),
                          -1, true, "");
          sendTextMessage(" Client: " + getNetworkVersionGITString(), -1, true,
                          "");
          sendTextMessage(" Client player [" + playerNameStr + "]", -1, true,
                          "");
        } else {
          versionMatched = true;
          string playerNameStr = getHumanPlayerName();
          sErr = "Warning, Server and client are using the same version but "
                 "different platforms.\n\nServer: " +
                 networkMessageIntro.getVersionString() +
                 "\nClient: " + getNetworkVersionGITString() + " player [" +
                 playerNameStr + "]";
          // printf("%s\n",sErr.c_str());
        }

        // error message and disconnect only if checked
        if (Config::getInstance().getBool("PlatformConsistencyChecks",
                                          "true") &&
            versionMatched == false) {

          DisplayErrorMessage(sErr);
          sleep(1);

          setQuit(true);
          close();
          return;
        }
      }

      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                                 "In [%s::%s Line: %d] took msecs: %lld\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__, chrono.getMillis());
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        chrono.start();

      if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,
                                 "In [%s::%s Line: %d]\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__);

      if (networkMessageIntro.getGameState() == nmgstOk) {
        if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                .enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugNetwork, "In [%s::%s Line: %d]\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__);

        // send intro message
        Lang &lang = Lang::getInstance();
        NetworkMessageIntro sendNetworkMessageIntro(
            sessionKey, getNetworkVersionGITString(), getHumanPlayerName(), -1,
            nmgstOk, this->getSocket()->getConnectedIPAddress(), serverFTPPort,
            lang.getLanguage(), networkMessageIntro.getGameInProgress(),
            Config::getInstance().getString("PlayerId", ""),
            getPlatformNameString());
        sendMessage(&sendNetworkMessageIntro);

        // printf("Got intro sending client details to server\n");

        if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                .enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugNetwork, "In [%s::%s Line: %d]\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__);

        if (clientSocket == NULL || clientSocket->isConnected() == false) {

          if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugNetwork, "In [%s::%s Line: %d]\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__);

          string sErr = "Disconnected from server during intro handshake.";
          DisplayErrorMessage(sErr);
          setQuit(true);
          close();

          if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugNetwork, "In [%s::%s Line: %d]\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__);
          return;
        } else {
          if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugNetwork, "In [%s::%s Line: %d]\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__);

          introDone = true;
        }
      } else if (networkMessageIntro.getGameState() == nmgstNoSlots) {
        string sErr = "Cannot join the server because there are no open slots "
                      "for new players.";
        DisplayErrorMessage(sErr);
        setQuit(true);
        close();
        if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                .enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugNetwork, "In [%s::%s Line: %d]\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__);
        return;
      } else {
        string sErr = "Unknown response from server: " +
                      intToStr(networkMessageIntro.getGameState());
        DisplayErrorMessage(sErr);
        setQuit(true);
        close();
        if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                .enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugNetwork, "In [%s::%s Line: %d]\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__);
        return;
      }
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                                 "In [%s::%s Line: %d] took msecs: %lld\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__, chrono.getMillis());
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        chrono.start();
    }
  } break;

  case nmtPing: {
    if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
      SystemFlags::OutputDebug(
          SystemFlags::debugNetwork, "In [%s::%s] got nmtPing\n",
          extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__);

    NetworkMessagePing networkMessagePing;
    if (receiveMessage(&networkMessagePing)) {
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                                 "In [%s::%s Line: %d] took msecs: %lld\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__, chrono.getMillis());
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        chrono.start();

      if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,
                                 "In [%s::%s Line: %d]\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__);
      this->setLastPingInfo(networkMessagePing);
    }
  } break;

  case nmtSynchNetworkGameData: {
    NetworkMessageSynchNetworkGameData networkMessageSynchNetworkGameData;

    if (receiveMessage(&networkMessageSynchNetworkGameData)) {
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                                 "In [%s::%s Line: %d] took msecs: %lld\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__, chrono.getMillis());
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        chrono.start();

      if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
        SystemFlags::OutputDebug(
            SystemFlags::debugNetwork,
            "In [%s::%s Line: %d] got NetworkMessageSynchNetworkGameData, "
            "getTechCRCFileCount() = %d\n",
            extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
            __LINE__, networkMessageSynchNetworkGameData.getTechCRCFileCount());

      this->setLastPingInfoToNow();

      networkGameDataSynchCheckOkMap = false;
      networkGameDataSynchCheckOkTile = false;
      networkGameDataSynchCheckOkTech = false;
      this->setNetworkGameDataSynchCheckTechMismatchReport("");
      this->setReceivedDataSynchCheck(false);

      uint32 tilesetCRC = 0;
      uint32 techCRC = 0;
      uint32 mapCRC = 0;
      vector<std::pair<string, uint32>> vctFileList;

      try {
        Config &config = Config::getInstance();
        string scenarioDir = "";
        if (gameSettings.getScenarioDir() != "") {

          scenarioDir = gameSettings.getScenarioDir();
          if (EndsWith(scenarioDir, ".xml") == true) {
            scenarioDir = scenarioDir.erase(scenarioDir.size() - 4, 4);
            scenarioDir = scenarioDir.erase(
                scenarioDir.size() - gameSettings.getScenario().size(),
                gameSettings.getScenario().size() + 1);
          }

          if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugNetwork,
                "In [%s::%s Line: %d] gameSettings.getScenarioDir() = [%s] "
                "gameSettings.getScenario() = [%s] scenarioDir = [%s]\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__, gameSettings.getScenarioDir().c_str(),
                gameSettings.getScenario().c_str(), scenarioDir.c_str());
        }

        // check the checksum's
        tilesetCRC = getFolderTreeContentsCheckSumRecursively(
            config.getPathListForType(ptTilesets, scenarioDir),
            string("/") + networkMessageSynchNetworkGameData.getTileset() +
                string("/*"),
            ".xml", NULL);

        this->setNetworkGameDataSynchCheckOkTile(
            (tilesetCRC == networkMessageSynchNetworkGameData.getTilesetCRC()));

        if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                .enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugNetwork,
              "In [%s::%s Line: %d] tilesetCRC info, local = %d, remote = %d, "
              "networkMessageSynchNetworkGameData.getTileset() = [%s]\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__, tilesetCRC,
              networkMessageSynchNetworkGameData.getTilesetCRC(),
              networkMessageSynchNetworkGameData.getTileset().c_str());

        // tech, load before map because of resources
        techCRC = getFolderTreeContentsCheckSumRecursively(
            config.getPathListForType(ptTechs, scenarioDir),
            string("/") + networkMessageSynchNetworkGameData.getTech() +
                string("/*"),
            ".xml", NULL);

        this->setNetworkGameDataSynchCheckOkTech(
            (techCRC == networkMessageSynchNetworkGameData.getTechCRC()));

        if (this->getNetworkGameDataSynchCheckOkTech() == false) {

          if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugNetwork, "In [%s::%s Line: %d]\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__);

          string pathSearchString =
              "/" + networkMessageSynchNetworkGameData.getTech() + "/*";
          vctFileList = getFolderTreeContentsCheckSumListRecursively(
              config.getPathListForType(ptTechs, scenarioDir), pathSearchString,
              ".xml", NULL);

          if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugNetwork, "In [%s::%s Line: %d]\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__);

          string report =
              networkMessageSynchNetworkGameData.getTechCRCFileMismatchReport(
                  vctFileList);
          this->setNetworkGameDataSynchCheckTechMismatchReport(report);
        }
        if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                .enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugNetwork,
              "In [%s::%s] techCRC info, local = %d, remote = %d, "
              "networkMessageSynchNetworkGameData.getTech() = [%s]\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              techCRC, networkMessageSynchNetworkGameData.getTechCRC(),
              networkMessageSynchNetworkGameData.getTech().c_str());

        // map
        Checksum checksum;
        string file = Config::getMapPath(
            networkMessageSynchNetworkGameData.getMap(), scenarioDir, false);
        if (file != "") {
          checksum.addFile(file);
          mapCRC = checksum.getSum();
        }
        this->setNetworkGameDataSynchCheckOkMap(
            (mapCRC == networkMessageSynchNetworkGameData.getMapCRC()));
        this->setReceivedDataSynchCheck(true);

        if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                .enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugNetwork,
              "In [%s::%s Line: %d] mapCRC info, local = %d, remote = %d, file "
              "= [%s]\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__, mapCRC, networkMessageSynchNetworkGameData.getMapCRC(),
              file.c_str());
      } catch (const runtime_error &ex) {
        SystemFlags::OutputDebug(SystemFlags::debugError,
                                 "In [%s::%s Line: %d] Error [%s]\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__, ex.what());
        string sErr = ex.what();
        if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                .enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugNetwork,
              "In [%s::%s Line: %d] error during processing, sErr = [%s]\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__, sErr.c_str());

        DisplayErrorMessage(sErr);
      }

      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                                 "In [%s::%s Line: %d] took msecs: %lld\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__, chrono.getMillis());
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        chrono.start();

      NetworkMessageSynchNetworkGameDataStatus
          sendNetworkMessageSynchNetworkGameDataStatus(mapCRC, tilesetCRC,
                                                       techCRC, vctFileList);
      sendMessage(&sendNetworkMessageSynchNetworkGameDataStatus);
    }
  } break;

  case nmtSynchNetworkGameDataFileCRCCheck: {
    NetworkMessageSynchNetworkGameDataFileCRCCheck
        networkMessageSynchNetworkGameDataFileCRCCheck;
    if (receiveMessage(&networkMessageSynchNetworkGameDataFileCRCCheck)) {
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                                 "In [%s::%s Line: %d] took msecs: %lld\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__, chrono.getMillis());
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        chrono.start();

      this->setLastPingInfoToNow();

      Checksum checksum;
      string file =
          networkMessageSynchNetworkGameDataFileCRCCheck.getFileName();
      checksum.addFile(file);
      uint32 fileCRC = checksum.getSum();

      if (fileCRC !=
          networkMessageSynchNetworkGameDataFileCRCCheck.getFileCRC()) {
        if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                .enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugNetwork,
              "In [%s::%s] got nmtSynchNetworkGameDataFileCRCCheck localCRC = "
              "%d, remoteCRC = %d, file [%s]\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              fileCRC,
              networkMessageSynchNetworkGameDataFileCRCCheck.getFileCRC(),
              networkMessageSynchNetworkGameDataFileCRCCheck.getFileName()
                  .c_str());

        // Here we initiate a download of missing or mismatched content

        NetworkMessageSynchNetworkGameDataFileGet
            sendNetworkMessageSynchNetworkGameDataFileGet(
                networkMessageSynchNetworkGameDataFileCRCCheck.getFileName());
        sendMessage(&sendNetworkMessageSynchNetworkGameDataFileGet);

        FileTransferInfo fileInfo;
        fileInfo.hostType = eClient;
        fileInfo.serverIP = this->ip.getString();
        fileInfo.serverPort = this->port;
        fileInfo.fileName =
            networkMessageSynchNetworkGameDataFileCRCCheck.getFileName();

        FileTransferSocketThread *fileXferThread =
            new FileTransferSocketThread(fileInfo);
        fileXferThread->start();
      }

      if (networkMessageSynchNetworkGameDataFileCRCCheck.getFileIndex() <
          networkMessageSynchNetworkGameDataFileCRCCheck.getTotalFileCount()) {
        NetworkMessageSynchNetworkGameDataFileCRCCheck
            sendNetworkMessageSynchNetworkGameDataFileCRCCheck(
                networkMessageSynchNetworkGameDataFileCRCCheck
                    .getTotalFileCount(),
                networkMessageSynchNetworkGameDataFileCRCCheck.getFileIndex() +
                    1,
                0, "");
        sendMessage(&sendNetworkMessageSynchNetworkGameDataFileCRCCheck);
      }

      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                                 "In [%s::%s Line: %d] took msecs: %lld\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__, chrono.getMillis());
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        chrono.start();
    }
  } break;

  case nmtText: {
    NetworkMessageText networkMessageText;
    if (receiveMessage(&networkMessageText)) {
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                                 "In [%s::%s Line: %d] took msecs: %lld\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__, chrono.getMillis());
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        chrono.start();

      this->setLastPingInfoToNow();

      if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
        SystemFlags::OutputDebug(
            SystemFlags::debugNetwork, "In [%s::%s] got nmtText\n",
            extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__);

      ChatMsgInfo msg(networkMessageText.getText().c_str(),
                      networkMessageText.getTeamIndex(),
                      networkMessageText.getPlayerIndex(),
                      networkMessageText.getTargetLanguage());
      this->addChatInfo(msg);
    }
  } break;

  case nmtMarkCell: {
    NetworkMessageMarkCell networkMessageMarkCell;
    if (receiveMessage(&networkMessageMarkCell)) {
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                                 "In [%s::%s Line: %d] took msecs: %lld\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__, chrono.getMillis());
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        chrono.start();

      this->setLastPingInfoToNow();
      if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
        SystemFlags::OutputDebug(
            SystemFlags::debugNetwork, "In [%s::%s] got nmtMarkCell\n",
            extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__);

      MarkedCell msg(networkMessageMarkCell.getTarget(),
                     networkMessageMarkCell.getFactionIndex(),
                     networkMessageMarkCell.getText().c_str(),
                     networkMessageMarkCell.getPlayerIndex());
      this->addMarkedCell(msg);
    }
  } break;
  case nmtUnMarkCell: {
    NetworkMessageUnMarkCell networkMessageMarkCell;
    if (receiveMessage(&networkMessageMarkCell)) {
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                                 "In [%s::%s Line: %d] took msecs: %lld\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__, chrono.getMillis());
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        chrono.start();

      this->setLastPingInfoToNow();
      if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
        SystemFlags::OutputDebug(
            SystemFlags::debugNetwork, "In [%s::%s] got nmtMarkCell\n",
            extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__);

      UnMarkedCell msg(networkMessageMarkCell.getTarget(),
                       networkMessageMarkCell.getFactionIndex());
      this->addUnMarkedCell(msg);
    }
  } break;
  case nmtHighlightCell: {
    NetworkMessageHighlightCell networkMessageHighlightCell;
    if (receiveMessage(&networkMessageHighlightCell)) {
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                                 "In [%s::%s Line: %d] took msecs: %lld\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__, chrono.getMillis());
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        chrono.start();

      this->setLastPingInfoToNow();
      if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
        SystemFlags::OutputDebug(
            SystemFlags::debugNetwork, "In [%s::%s] got nmtHighlightCell\n",
            extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__);

      MarkedCell msg(networkMessageHighlightCell.getTarget(),
                     networkMessageHighlightCell.getFactionIndex(), "none", -1);
      this->setHighlightedCell(msg);
    }
  } break;

  case nmtLaunch:
  case nmtBroadCastSetup: {
    // printf("#1 Got new game setup playerIndex = %d!\n",playerIndex);

    NetworkMessageLaunch networkMessageLaunch;
    if (receiveMessage(&networkMessageLaunch, networkMessageType)) {
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                                 "In [%s::%s Line: %d] took msecs: %lld\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__, chrono.getMillis());
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        chrono.start();

      this->setLastPingInfoToNow();

      if (networkMessageLaunch.getMessageType() == nmtLaunch) {
        if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                .enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugNetwork,
              "In [%s::%s Lined: %d] got nmtLaunch\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__);
      } else if (networkMessageLaunch.getMessageType() == nmtBroadCastSetup) {
        if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                .enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugNetwork,
              "In [%s::%s Lined: %d] got nmtBroadCastSetup\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__);
      } else {
        if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                .enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugNetwork,
              "In [%s::%s Lined: %d] got networkMessageLaunch.getMessageType() "
              "= %d\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__, networkMessageLaunch.getMessageType());

        char szBuf[1024] = "";
        snprintf(szBuf, 1023,
                 "In [%s::%s Line: %d] Invalid "
                 "networkMessageLaunch.getMessageType() = %d",
                 extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                 __LINE__, networkMessageLaunch.getMessageType());
        throw megaglest_runtime_error(szBuf);
      }

      networkMessageLaunch.buildGameSettings(&gameSettings);

      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                                 "In [%s::%s Line: %d] took msecs: %lld\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__, chrono.getMillis());
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        chrono.start();

      // printf("Client got game settings playerIndex = %d lookingfor
      // match...\n",playerIndex);

      if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,
                                 "In [%s::%s Lined: %d] got "
                                 "networkMessageLaunch.getMessageType() = %d\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__,
                                 networkMessageLaunch.getMessageType());
      // replace server player by network
      for (int factionIndex = 0; factionIndex < gameSettings.getFactionCount();
           ++factionIndex) {

        // printf("Faction = %d start location = %d faction name =
        // %s\n",i,gameSettings.getStartLocationIndex(factionIndex),gameSettings.getFactionTypeName(factionIndex).c_str());

        // replace by network
        if (gameSettings.getFactionControl(factionIndex) == ctHuman) {
          gameSettings.setFactionControl(factionIndex, ctNetwork);
        }

        // printf("factionIndex = %d
        // gameSettings.getStartLocationIndex(factionIndex) = %d playerIndex =
        // %d, gameSettings.getFactionControl(factionIndex) =
        // %d\n",factionIndex,gameSettings.getStartLocationIndex(factionIndex),playerIndex,gameSettings.getFactionControl(i));

        // set the faction index
        if (gameSettings.getStartLocationIndex(factionIndex) == playerIndex) {
          // printf("Setting my factionindex to: %d for playerIndex:
          // %d\n",i,playerIndex);

          gameSettings.setThisFactionIndex(factionIndex);

          // printf("Client got game settings playerIndex = %d factionIndex = %d
          // control = %d name =
          // %s\n",playerIndex,factionIndex,gameSettings.getFactionControl(factionIndex),gameSettings.getFactionTypeName(i).c_str());
          if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugNetwork,
                "In [%s::%s Line: %d] "
                "gameSettings.getThisFactionIndex(factionIndex) = %d, "
                "playerIndex = %d, factionIndex = %d\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__, gameSettings.getThisFactionIndex(), playerIndex,
                factionIndex);
        }
      }

      if (networkMessageLaunch.getMessageType() == nmtLaunch) {
        launchGame = true;
      } else if (networkMessageLaunch.getMessageType() == nmtBroadCastSetup) {
        setGameSettingsReceived(true);
      }

      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                                 "In [%s::%s Line: %d] took msecs: %lld\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__, chrono.getMillis());
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        chrono.start();
    }
  } break;
  case nmtPlayerIndexMessage: {
    PlayerIndexMessage playerIndexMessage(-1);
    if (receiveMessage(&playerIndexMessage)) {
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                                 "In [%s::%s Line: %d] took msecs: %lld\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__, chrono.getMillis());
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        chrono.start();

      this->setLastPingInfoToNow();
      playerIndex = playerIndexMessage.getPlayerIndex();

      if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,
                                 "In [%s::%s Line: %d] got "
                                 "nmtPlayerIndexMessage, playerIndex = %d\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__, playerIndex);
    }

    // printf("Got player index changed msg: %d\n",playerIndex);
  } break;

  case nmtReady: {
    NetworkMessageReady networkMessageReady;
    if (receiveMessage(&networkMessageReady)) {
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                                 "In [%s::%s Line: %d] took msecs: %lld\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__, chrono.getMillis());
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        chrono.start();

      this->setLastPingInfoToNow();
      MutexSafeWrapper safeMutexFlags(flagAccessor, CODE_AT_LINE);
      this->readyForInGameJoin = true;
    }

    // printf("ClientInterface got nmtReady this->readyForInGameJoin:
    // %d\n",this->readyForInGameJoin);
  } break;

  case nmtCommandList: {

    // make sure we read the message
    // time_t receiveTimeElapsed = time(NULL);
    NetworkMessageCommandList networkMessageCommandList;
    bool gotCmd = receiveMessage(&networkMessageCommandList);
    if (gotCmd == false) {
      throw megaglest_runtime_error(
          "error retrieving nmtCommandList returned false!");
    }
    if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
            .enabled &&
        chrono.getMillis() > 0)
      SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                               "In [%s::%s Line: %d] took msecs: %lld\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__, chrono.getMillis());
    if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
            .enabled &&
        chrono.getMillis() > 0)
      chrono.start();

    this->setLastPingInfoToNow();
  } break;

  case nmtQuit: {
    // time_t receiveTimeElapsed = time(NULL);
    NetworkMessageQuit networkMessageQuit;
    bool gotCmd = receiveMessage(&networkMessageQuit);
    if (gotCmd == false) {
      throw megaglest_runtime_error("error retrieving nmtQuit returned false!");
    }
    if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
            .enabled &&
        chrono.getMillis() > 0)
      SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                               "In [%s::%s Line: %d] took msecs: %lld\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__, chrono.getMillis());
    if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
            .enabled &&
        chrono.getMillis() > 0)
      chrono.start();

    this->setLastPingInfoToNow();
    setQuit(true);
    close();
  } break;

  case nmtLoadingStatusMessage: {
    NetworkMessageLoadingStatus networkMessageLoadingStatus(nmls_NONE);
    if (receiveMessage(&networkMessageLoadingStatus)) {
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                                 "In [%s::%s Line: %d] took msecs: %lld\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__, chrono.getMillis());
      if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
              .enabled &&
          chrono.getMillis() > 0)
        chrono.start();

      this->setLastPingInfoToNow();
      if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,
                                 "In [%s::%s] Line: %d\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__);
    }
  } break;

  default: {
    string sErr =
        string(extractFileFromDirectoryPath(__FILE__).c_str()) +
        "::" + string(__FUNCTION__) +
        " Unexpected network message: " + intToStr(networkMessageType);
    // throw
    // megaglest_runtime_error(string(extractFileFromDirectoryPath(__FILE__).c_str())
    // + "::" + string(__FUNCTION__) + " Unexpected network message: " +
    // intToStr(networkMessageType));
    sendTextMessage("Unexpected network message: " +
                        intToStr(networkMessageType),
                    -1, true, "");
    DisplayErrorMessage(sErr);
    sleep(1);

    setQuit(true);
    close();
  }
  }

  if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
          .enabled &&
      chrono.getMillis() > 0)
    SystemFlags::OutputDebug(SystemFlags::debugPerformance,
                             "In [%s::%s Line: %d] took msecs: %lld\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__, chrono.getMillis());
  if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
          .enabled &&
      chrono.getMillis() > 0)
    chrono.start();

  if (clientSocket != NULL && clientSocket->isConnected() == true &&
      gotIntro == false &&
      difftime((long int)time(NULL), connectedTime) >
          GameConstants::maxClientConnectHandshakeSecs) {
    if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
      SystemFlags::OutputDebug(
          SystemFlags::debugNetwork,
          "In [%s::%s Line: %d] difftime(time(NULL),connectedTime) = %f\n",
          extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
          __LINE__, difftime((long int)time(NULL), connectedTime));
    close();
  }

  // if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled
  // && chrono.getMillis() > 0)
  // SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s Line:
  // %d] took msecs:
  // %lld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());
  // if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled
  // && chrono.getMillis() > 0) chrono.start();
}

void ClientInterface::updateNetworkFrame() { this->updateFrame(NULL); }

void ClientInterface::updateFrame(int *checkFrame) {
  // printf("#1 ClientInterface::updateFrame\n");

  // printf("In updateFrame: %d\n",(checkFrame ? *checkFrame : -1));

  if (isConnected() == true && getQuitThread() == false) {
    // printf("#2 ClientInterface::updateFrame\n");

    uint64 loopCount = 0;
    Chrono chronoPerf;
    if (debugClientInterfacePerf == true) {
      chronoPerf.start();
    }

    int waitMicroseconds = (checkFrame == NULL ? 10 : 0);

    bool done = false;
    while (done == false && getQuitThread() == false) {
      // printf("BEFORE Client get networkMessageType\n");

      // wait for the next message
      NetworkMessageType networkMessageType = waitForMessage(waitMicroseconds);

      // printf("AFTER Client got networkMessageType =
      // %d\n",networkMessageType);

      // check we have an expected message
      // NetworkMessageType networkMessageType= getNextMessageType();

      // printf("Got Network networkMessageType: %d\n",networkMessageType);

      switch (networkMessageType) {
      case nmtCommandList: {

        // make sure we read the message
        // time_t receiveTimeElapsed = time(NULL);
        NetworkMessageCommandList networkMessageCommandList;
        bool gotCmd = receiveMessage(&networkMessageCommandList);
        if (gotCmd == false) {
          SystemFlags::OutputDebug(SystemFlags::debugError,
                                   "In [%s::%s Line: %d] error retrieving "
                                   "nmtCommandList returned false!\n",
                                   __FILE__, __FUNCTION__, __LINE__);
          if (isConnected() == false) {
            setQuit(true);
            close();
            return;
          }

          throw megaglest_runtime_error(
              "error retrieving nmtCommandList returned false!");
        }

        // printf("Client Thread getFrameCount(): %d getCommandCount():
        // %d\n",networkMessageCommandList.getFrameCount(),networkMessageCommandList.getCommandCount());

        MutexSafeWrapper safeMutex(networkCommandListThreadAccessor,
                                   CODE_AT_LINE);
        cachedLastPendingFrameCount = networkMessageCommandList.getFrameCount();
        // printf("cachedLastPendingFrameCount = %lld\n",(long long
        // int)cachedLastPendingFrameCount);

        // check that we are in the right frame
        if (checkFrame != NULL) {
          if (networkMessageCommandList.getFrameCount() != *checkFrame) {
            string sErr = "Player: " + getHumanPlayerName() +
                          " got a Network synchronization error, frame counts "
                          "do not match, server frameCount = " +
                          intToStr(networkMessageCommandList.getFrameCount()) +
                          ", local frameCount = " + intToStr(*checkFrame);
            sendTextMessage(sErr, -1, true, "");
            DisplayErrorMessage(sErr);
            sleep(1);

            setQuit(true);
            close();
            return;
          }
          for (int index = 0; index < GameConstants::maxPlayers; ++index) {
            printf("Frame: %d faction: %d local CRC: %u Remote CRC: %u\n",
                   *checkFrame, index, getNetworkPlayerFactionCRC(index),
                   networkMessageCommandList.getNetworkPlayerFactionCRC(index));

            if (networkMessageCommandList.getNetworkPlayerFactionCRC(index) !=
                getNetworkPlayerFactionCRC(index)) {
              string sErr =
                  "Player: " + getHumanPlayerName() +
                  " got a Network CRC error, CRC's do not match, server CRC "
                  "= " +
                  uIntToStr(
                      networkMessageCommandList.getNetworkPlayerFactionCRC(
                          index)) +
                  ", local CRC = " +
                  uIntToStr(getNetworkPlayerFactionCRC(index));
              sendTextMessage(sErr, -1, true, "");
              DisplayErrorMessage(sErr);
              sleep(1);

              setQuit(true);
              close();
              return;
            }
          }
        }

        cachedPendingCommands[networkMessageCommandList.getFrameCount()]
            .reserve(networkMessageCommandList.getCommandCount());

        // give all commands
        for (int i = 0; i < networkMessageCommandList.getCommandCount(); ++i) {
          // pendingCommands.push_back(*networkMessageCommandList.getCommand(i));

          // if(networkMessageCommandList.getCommand(i)->getNetworkCommandType()
          // == nctPauseResume) { printf("Network cmd type: %d [%d] frame:
          // %d\n",networkMessageCommandList.getCommand(i)->getNetworkCommandType(),nctPauseResume,networkMessageCommandList.getFrameCount());
          //}

          cachedPendingCommands[networkMessageCommandList.getFrameCount()]
              .push_back(*networkMessageCommandList.getCommand(i));

          if (cachedPendingCommandCRCs.find(
                  networkMessageCommandList.getFrameCount()) ==
              cachedPendingCommandCRCs.end()) {
            cachedPendingCommandCRCs[networkMessageCommandList.getFrameCount()]
                .reserve(GameConstants::maxPlayers);
            for (int index = 0; index < GameConstants::maxPlayers; ++index) {
              cachedPendingCommandCRCs[networkMessageCommandList
                                           .getFrameCount()]
                  .push_back(
                      networkMessageCommandList.getNetworkPlayerFactionCRC(
                          index));
            }
          }
        }
        safeMutex.ReleaseLock();

        done = true;
      } break;

      case nmtPing: {
        if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                .enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugNetwork, "In [%s::%s] got nmtPing\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__);

        NetworkMessagePing networkMessagePing;
        if (receiveMessage(&networkMessagePing)) {
          if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugNetwork, "In [%s::%s Line: %d]\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__);
          this->setLastPingInfo(networkMessagePing);
        }
      } break;

      case nmtQuit: {
        NetworkMessageQuit networkMessageQuit;
        bool gotCmd = receiveMessage(&networkMessageQuit);
        if (gotCmd == false) {
          SystemFlags::OutputDebug(
              SystemFlags::debugError,
              "In [%s::%s Line: %d] error retrieving nmtQuit returned false!\n",
              __FILE__, __FUNCTION__, __LINE__);
          if (isConnected() == false) {
            setQuit(true);
            close();
            return;
          }

          // throw megaglest_runtime_error("error retrieving nmtQuit returned
          // false!");
          setQuit(true);
          done = true;
        }
        setQuit(true);
        done = true;
      } break;

      case nmtText: {
        NetworkMessageText networkMessageText;
        bool gotCmd = receiveMessage(&networkMessageText);
        if (gotCmd == false) {

          SystemFlags::OutputDebug(
              SystemFlags::debugError,
              "In [%s::%s Line: %d] error retrieving nmtText returned false!\n",
              __FILE__, __FUNCTION__, __LINE__);
          if (isConnected() == false) {

            setQuit(true);
            close();
            return;
          }

          throw megaglest_runtime_error(
              "error retrieving nmtText returned false!");
        }

        ChatMsgInfo msg(networkMessageText.getText().c_str(),
                        networkMessageText.getTeamIndex(),
                        networkMessageText.getPlayerIndex(),
                        networkMessageText.getTargetLanguage());
        this->addChatInfo(msg);
      } break;

      case nmtMarkCell: {
        NetworkMessageMarkCell networkMessageMarkCell;
        if (receiveMessage(&networkMessageMarkCell)) {
          if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugNetwork, "In [%s::%s] got nmtMarkCell\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__);

          MarkedCell msg(networkMessageMarkCell.getTarget(),
                         networkMessageMarkCell.getFactionIndex(),
                         networkMessageMarkCell.getText().c_str(),
                         networkMessageMarkCell.getPlayerIndex());
          this->addMarkedCell(msg);
        }
      } break;

      case nmtUnMarkCell: {
        NetworkMessageUnMarkCell networkMessageMarkCell;
        if (receiveMessage(&networkMessageMarkCell)) {
          if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugNetwork, "In [%s::%s] got nmtMarkCell\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__);

          UnMarkedCell msg(networkMessageMarkCell.getTarget(),
                           networkMessageMarkCell.getFactionIndex());
          this->addUnMarkedCell(msg);
        }
      } break;
      case nmtHighlightCell: {
        NetworkMessageHighlightCell networkMessageHighlightCell;
        if (receiveMessage(&networkMessageHighlightCell)) {
          if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugNetwork, "In [%s::%s] got nmtHighlightCell\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__);

          MarkedCell msg(networkMessageHighlightCell.getTarget(),
                         networkMessageHighlightCell.getFactionIndex(), "none",
                         -1);
          this->setHighlightedCell(msg);
        }
      } break;

      case nmtLaunch:
      case nmtBroadCastSetup: {
        // printf("#2 Got new game setup playerIndex = %d!\n",playerIndex);

        NetworkMessageLaunch networkMessageLaunch;
        if (receiveMessage(&networkMessageLaunch, networkMessageType)) {

          if (networkMessageLaunch.getMessageType() == nmtLaunch) {
            if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                    .enabled)
              SystemFlags::OutputDebug(
                  SystemFlags::debugNetwork,
                  "In [%s::%s Lined: %d] got nmtLaunch\n",
                  extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                  __LINE__);
          } else if (networkMessageLaunch.getMessageType() ==
                     nmtBroadCastSetup) {
            if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                    .enabled)
              SystemFlags::OutputDebug(
                  SystemFlags::debugNetwork,
                  "In [%s::%s Lined: %d] got nmtBroadCastSetup\n",
                  extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                  __LINE__);
          } else {
            if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                    .enabled)
              SystemFlags::OutputDebug(
                  SystemFlags::debugNetwork,
                  "In [%s::%s Lined: %d] got "
                  "networkMessageLaunch.getMessageType() = %d\n",
                  extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                  __LINE__, networkMessageLaunch.getMessageType());

            char szBuf[1024] = "";
            snprintf(szBuf, 1023,
                     "In [%s::%s Line: %d] Invalid "
                     "networkMessageLaunch.getMessageType() = %d",
                     extractFileFromDirectoryPath(__FILE__).c_str(),
                     __FUNCTION__, __LINE__,
                     networkMessageLaunch.getMessageType());
            throw megaglest_runtime_error(szBuf);
          }

          networkMessageLaunch.buildGameSettings(&gameSettings);

          if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugNetwork,
                "In [%s::%s Lined: %d] got "
                "networkMessageLaunch.getMessageType() = %d\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__, networkMessageLaunch.getMessageType());
          // replace server player by network
          for (int i = 0; i < gameSettings.getFactionCount(); ++i) {
            // replace by network
            if (gameSettings.getFactionControl(i) == ctHuman) {
              gameSettings.setFactionControl(i, ctNetwork);
            }

            // printf("i = %d gameSettings.getStartLocationIndex(i) = %d
            // playerIndex =
            // %d!\n",i,gameSettings.getStartLocationIndex(i),playerIndex);

            // set the faction index
            if (gameSettings.getStartLocationIndex(i) == playerIndex) {
              // printf("Setting my factionindex to: %d for playerIndex:
              // %d\n",i,playerIndex);

              gameSettings.setThisFactionIndex(i);
              if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                      .enabled)
                SystemFlags::OutputDebug(
                    SystemFlags::debugNetwork,
                    "In [%s::%s Line: %d] gameSettings.getThisFactionIndex(i) "
                    "= %d, playerIndex = %d, i = %d\n",
                    extractFileFromDirectoryPath(__FILE__).c_str(),
                    __FUNCTION__, __LINE__, gameSettings.getThisFactionIndex(),
                    playerIndex, i);
            }
          }
        }
      } break;

      case nmtLoadingStatusMessage:
        break;

      case nmtInvalid:
        break;

      default: {
        sendTextMessage("Unexpected message in client interface: " +
                            intToStr(networkMessageType),
                        -1, true, "");
        DisplayErrorMessage(
            string(extractFileFromDirectoryPath(__FILE__).c_str()) + "::" +
            string(__FUNCTION__) + " Unexpected message in client interface: " +
            intToStr(networkMessageType));
        sleep(1);

        setQuit(true);
        close();
        done = true;
      } break;
      }

      if (isConnected() == false && getQuit() == true) {
        done = true;
      }

      if (debugClientInterfacePerf == true) {
        loopCount++;
        if (chronoPerf.getMillis() >= 1000) {
          printf("Client updateFrame loopCount = %llu\n",
                 (long long unsigned int)loopCount);

          loopCount = 0;
          // sleep(0);
          chronoPerf.start();
        }
      }
    }

    if (done == true) {
      MutexSafeWrapper safeMutex(networkCommandListThreadAccessor,
                                 CODE_AT_LINE);
      cachedPendingCommandsIndex++;
    }
  }
  // printf("#3 ClientInterface::updateFrame\n");
}

uint64 ClientInterface::getCachedLastPendingFrameCount() {
  MutexSafeWrapper safeMutex(networkCommandListThreadAccessor, CODE_AT_LINE);
  uint64 result = cachedLastPendingFrameCount;
  return result;
}

int64 ClientInterface::getTimeClientWaitedForLastMessage() {
  MutexSafeWrapper safeMutex(networkCommandListThreadAccessor, CODE_AT_LINE);
  uint64 result = timeClientWaitedForLastMessage;
  return result;
}

bool ClientInterface::getNetworkCommand(int frameCount,
                                        int currentCachedPendingCommandsIndex) {
  bool result = false;
  bool waitForData = false;
  uint64 copyCachedLastPendingFrameCount = 0;
  uint64 waitCount = 0;
  uint64 frameCountAsUInt64 = frameCount;
  timeClientWaitedForLastMessage = 0;

  // printf("In getNetworkCommand: %d
  // [%d]\n",frameCount,currentCachedPendingCommandsIndex);

  if (getQuit() == false && getQuitThread() == false) {

    Chrono chrono;
    MutexSafeWrapper safeMutex(NULL, CODE_AT_LINE);

    for (; getQuit() == false && getQuitThread() == false;) {

      if (safeMutex.isValidMutex() == false) {
        safeMutex.setMutex(networkCommandListThreadAccessor, CODE_AT_LINE);
      } else {
        safeMutex.Lock();
      }
      copyCachedLastPendingFrameCount = cachedLastPendingFrameCount;

      if (cachedPendingCommands.find(frameCount) !=
          cachedPendingCommands.end()) {

        Commands &frameCmdList = cachedPendingCommands[frameCount];

        // printf("In getNetworkCommand frameCmdList.size():
        // %d\n",(int)frameCmdList.size());

        if (frameCmdList.empty() == false) {
          for (int index = 0; index < (int)frameCmdList.size(); ++index) {
            pendingCommands.push_back(frameCmdList[index]);
          }
          cachedPendingCommands[frameCount].clear();

          if (frameCount >= 0) {
            for (int index = 0; index < GameConstants::maxPlayers; ++index) {
              // printf("X**X Frame: %d faction: %d local CRC: %u Remote CRC:
              // %u\n",frameCount,index,getNetworkPlayerFactionCRC(index),cachedPendingCommandCRCs[frameCount][index]);

              if (cachedPendingCommandCRCs[frameCount][index] !=
                  getNetworkPlayerFactionCRC(index)) {

                printf(
                    "X**X Frame: %d faction: %d local CRC: %u Remote CRC: %u\n",
                    frameCount, index, getNetworkPlayerFactionCRC(index),
                    cachedPendingCommandCRCs[frameCount][index]);

                string sErr =
                    "Player: " + getHumanPlayerName() +
                    " got a Network CRC error, CRC's do not match, server CRC "
                    "= " +
                    uIntToStr(cachedPendingCommandCRCs[frameCount][index]) +
                    ", local CRC = " +
                    uIntToStr(getNetworkPlayerFactionCRC(index));
                sendTextMessage(sErr, -1, true, "");
                DisplayErrorMessage(sErr);
                sleep(1);

                setQuit(true);
                close();
              }
            }
          }
          cachedPendingCommandCRCs.erase(frameCount);
        }
        if (waitForData == true) {
          timeClientWaitedForLastMessage = chrono.getMillis();
          chrono.stop();
        }
        safeMutex.ReleaseLock(true);

        result = true;
        break;
      } else {
        safeMutex.ReleaseLock(true);
        // No data for this frame
        if (waitForData == false) {
          if (SystemFlags::VERBOSE_MODE_ENABLED)
            printf("Client waiting for packet for frame: %d, "
                   "copyCachedLastPendingFrameCount = %lld\n",
                   frameCount, (long long int)copyCachedLastPendingFrameCount);
          chrono.start();
        }
        if (copyCachedLastPendingFrameCount > frameCountAsUInt64) {
          break;
        }

        if (waitForData == false) {
          waitForData = true;
          sleep(0);
        }

        waitCount++;
        // printf("Client waiting for packet for frame: %d,
        // currentCachedPendingCommandsIndex = %d, cachedPendingCommandsIndex =
        // %lld\n",frameCount,currentCachedPendingCommandsIndex,(long long
        // int)cachedPendingCommandsIndex);
      }
    }
  }
  if (waitForData == true) {
    if (SystemFlags::VERBOSE_MODE_ENABLED)
      printf("Client waiting for packet FINISHED for frame: %d, "
             "copyCachedLastPendingFrameCount = %lld waitCount = %llu\n",
             frameCount, (long long int)copyCachedLastPendingFrameCount,
             (long long unsigned int)waitCount);
  }

  return result;
}

void ClientInterface::updateKeyframe(int frameCount) {
  currentFrameCount = frameCount;

  // printf("In updateKeyFrame: %d\n",currentFrameCount);

  if (getQuit() == false && getQuitThread() == false) {
    if (networkCommandListThread == NULL) {
      static string mutexOwnerId =
          string(extractFileFromDirectoryPath(__FILE__).c_str()) + string("_") +
          intToStr(__LINE__);
      networkCommandListThread = new ClientInterfaceThread(this);
      networkCommandListThread->setUniqueID(mutexOwnerId);
      networkCommandListThread->start();

      sleep(0);
    }

    getNetworkCommand(frameCount, cachedPendingCommandsIndex);
  }
}

bool ClientInterface::isMasterServerAdminOverride() {
  return (gameSettings.getMasterserver_admin() == this->getSessionKey());
}

void ClientInterface::waitUntilReady(Checksum *checksum) {
  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(
        SystemFlags::debugNetwork, "In [%s::%s] Line: %d\n",
        extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__, __LINE__);

  MutexSafeWrapper safeMutexFlags(flagAccessor, CODE_AT_LINE);
  bool signalServerWhenReadyToStartJoinedGame = this->readyForInGameJoin;
  this->readyForInGameJoin = false;
  safeMutexFlags.ReleaseLock();

  Logger &logger = Logger::getInstance();

  Chrono chrono;
  chrono.start();

  // FOR TESTING ONLY - delay to see the client count up while waiting
  // sleep(5000);

  // clientSocket->setBlock(true);
  // send ready message
  NetworkMessageReady networkMessageReady;
  sendMessage(&networkMessageReady);

  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(
        SystemFlags::debugNetwork, "In [%s::%s] Line: %d\n",
        extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__, __LINE__);

  NetworkMessageLoadingStatus networkMessageLoadingStatus(nmls_NONE);

  Lang &lang = Lang::getInstance();

  int64 lastMillisCheck = 0;
  uint64 waitLoopIterationCount = 0;
  uint64 MAX_LOOP_COUNT_BEFORE_SLEEP = 100;
  MAX_LOOP_COUNT_BEFORE_SLEEP = Config::getInstance().getInt(
      "NetworkClientLoopGameLoadingCap",
      intToStr(MAX_LOOP_COUNT_BEFORE_SLEEP).c_str());
  if (MAX_LOOP_COUNT_BEFORE_SLEEP == 0) {
    MAX_LOOP_COUNT_BEFORE_SLEEP = 1;
  }
  int sleepMillis = Config::getInstance().getInt(
      "NetworkClientLoopGameLoadingCapSleepMillis", "10");

  // wait until we get a ready message from the server
  while (true) {
    // FOR TESTING ONLY - delay to see the client count up while waiting
    // sleep(2000);

    waitLoopIterationCount++;
    if (waitLoopIterationCount > 0 &&
        waitLoopIterationCount % MAX_LOOP_COUNT_BEFORE_SLEEP == 0) {
      sleep(sleepMillis);
      waitLoopIterationCount = 0;
    }

    if (isConnected() == false) {
      if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,
                                 "In [%s::%s] Line: %d\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__);

      string sErr = "Error, Server has disconnected!";
      DisplayErrorMessage(sErr);

      if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,
                                 "In [%s::%s] Line: %d\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__);

      setQuit(true);
      close();

      if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,
                                 "In [%s::%s] Line: %d\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__);
      return;
    }
    NetworkMessageType networkMessageType = getNextMessageType();

    // consume old messages from the lobby
    bool discarded = shouldDiscardNetworkMessage(networkMessageType);
    if (discarded == false) {
      if (networkMessageType == nmtReady) {
        if (receiveMessage(&networkMessageReady)) {
          if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugNetwork, "In [%s::%s] Line: %d\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__);
          break;
        }
      } else if (networkMessageType == nmtLoadingStatusMessage) {
        if (receiveMessage(&networkMessageLoadingStatus)) {
          if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugNetwork, "In [%s::%s] Line: %d\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__);
        }
      } else if (networkMessageType == nmtQuit) {
        NetworkMessageQuit networkMessageQuit;
        if (receiveMessage(&networkMessageQuit)) {
          if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugNetwork, "In [%s::%s] Line: %d\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__);

          DisplayErrorMessage(lang.getString("GameCancelledByUser"));

          if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugNetwork, "In [%s::%s] Line: %d\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__);

          setQuit(true);
          close();

          if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugNetwork, "In [%s::%s] Line: %d\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__);
          return;
        }
      } else if (networkMessageType == nmtCommandList) {
        // make sure we read the message
        NetworkMessageCommandList networkMessageCommandList;
        bool gotCmd = receiveMessage(&networkMessageCommandList);
        if (gotCmd == false) {
          throw megaglest_runtime_error(
              "error retrieving nmtCommandList returned false!");
        }
      } else if (networkMessageType == nmtInvalid) {
        if (chrono.getMillis() > readyWaitTimeout) {
          if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugNetwork, "In [%s::%s] Line: %d\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__);

          Lang &lang = Lang::getInstance();
          const vector<string> languageList =
              this->gameSettings.getUniqueNetworkPlayerLanguages();
          for (unsigned int i = 0; i < languageList.size(); ++i) {
            string sErr = "Timeout waiting for server";
            if (lang.hasString("TimeoutWaitingForServer", languageList[i]) ==
                true) {
              sErr = lang.getString("TimeoutWaitingForServer", languageList[i]);
            }
            bool echoLocal = lang.isLanguageLocal(lang.getLanguage());
            sendTextMessage(sErr, -1, echoLocal, languageList[i]);

            if (echoLocal) {
              DisplayErrorMessage(sErr);
            }
          }

          if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugNetwork, "In [%s::%s] Line: %d\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__);

          sleep(1);
          setQuit(true);
          close();

          if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugNetwork, "In [%s::%s] Line: %d\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__);
          return;
        } else {
          if (chrono.getMillis() % 100 == 0) {
            lastMillisCheck = (chrono.getMillis() / 1000);

            char szBuf[8096] = "";
            string updateTextFormat = "Waiting for network: %lld seconds "
                                      "elapsed (maximum wait time: %d seconds)";
            if (lang.hasString("NetworkGameClientLoadStatus") == true) {
              updateTextFormat = lang.getString("NetworkGameClientLoadStatus");
            }

            string waitForHosts = "";
            if ((networkMessageLoadingStatus.getStatus() &
                 nmls_PLAYER1_CONNECTED) == nmls_PLAYER1_CONNECTED) {
              if ((networkMessageLoadingStatus.getStatus() &
                   nmls_PLAYER1_READY) != nmls_PLAYER1_READY) {
                if (waitForHosts != "") {
                  waitForHosts += ", ";
                }
                waitForHosts +=
                    gameSettings.getNetworkPlayerNameByPlayerIndex(0);
              }
            }
            if ((networkMessageLoadingStatus.getStatus() &
                 nmls_PLAYER2_CONNECTED) == nmls_PLAYER2_CONNECTED) {
              if ((networkMessageLoadingStatus.getStatus() &
                   nmls_PLAYER2_READY) != nmls_PLAYER2_READY) {
                if (waitForHosts != "") {
                  waitForHosts += ", ";
                }
                waitForHosts +=
                    gameSettings.getNetworkPlayerNameByPlayerIndex(1);
              }
            }
            if ((networkMessageLoadingStatus.getStatus() &
                 nmls_PLAYER3_CONNECTED) == nmls_PLAYER3_CONNECTED) {
              if ((networkMessageLoadingStatus.getStatus() &
                   nmls_PLAYER3_READY) != nmls_PLAYER3_READY) {
                if (waitForHosts != "") {
                  waitForHosts += ", ";
                }
                waitForHosts +=
                    gameSettings.getNetworkPlayerNameByPlayerIndex(2);
              }
            }
            if ((networkMessageLoadingStatus.getStatus() &
                 nmls_PLAYER4_CONNECTED) == nmls_PLAYER4_CONNECTED) {
              if ((networkMessageLoadingStatus.getStatus() &
                   nmls_PLAYER4_READY) != nmls_PLAYER4_READY) {
                if (waitForHosts != "") {
                  waitForHosts += ", ";
                }
                waitForHosts +=
                    gameSettings.getNetworkPlayerNameByPlayerIndex(3);
              }
            }
            if ((networkMessageLoadingStatus.getStatus() &
                 nmls_PLAYER5_CONNECTED) == nmls_PLAYER5_CONNECTED) {
              if ((networkMessageLoadingStatus.getStatus() &
                   nmls_PLAYER5_READY) != nmls_PLAYER5_READY) {
                if (waitForHosts != "") {
                  waitForHosts += ", ";
                }
                waitForHosts +=
                    gameSettings.getNetworkPlayerNameByPlayerIndex(4);
              }
            }
            if ((networkMessageLoadingStatus.getStatus() &
                 nmls_PLAYER6_CONNECTED) == nmls_PLAYER6_CONNECTED) {
              if ((networkMessageLoadingStatus.getStatus() &
                   nmls_PLAYER6_READY) != nmls_PLAYER6_READY) {
                if (waitForHosts != "") {
                  waitForHosts += ", ";
                }
                waitForHosts +=
                    gameSettings.getNetworkPlayerNameByPlayerIndex(5);
              }
            }
            if ((networkMessageLoadingStatus.getStatus() &
                 nmls_PLAYER7_CONNECTED) == nmls_PLAYER7_CONNECTED) {
              if ((networkMessageLoadingStatus.getStatus() &
                   nmls_PLAYER7_READY) != nmls_PLAYER7_READY) {
                if (waitForHosts != "") {
                  waitForHosts += ", ";
                }
                waitForHosts +=
                    gameSettings.getNetworkPlayerNameByPlayerIndex(6);
              }
            }
            if ((networkMessageLoadingStatus.getStatus() &
                 nmls_PLAYER8_CONNECTED) == nmls_PLAYER8_CONNECTED) {
              if ((networkMessageLoadingStatus.getStatus() &
                   nmls_PLAYER8_READY) != nmls_PLAYER8_READY) {
                if (waitForHosts != "") {
                  waitForHosts += ", ";
                }
                waitForHosts +=
                    gameSettings.getNetworkPlayerNameByPlayerIndex(7);
              }
            }

            if (waitForHosts == "") {
              waitForHosts = lang.getString("Server");
            }
            snprintf(szBuf, 8096, updateTextFormat.c_str(),
                     (long long int)lastMillisCheck,
                     int(readyWaitTimeout / 1000));

            char szBuf1[8096] = "";
            string statusTextFormat = "Waiting for players: %s";
            if (lang.hasString("NetworkGameStatusWaiting") == true) {
              statusTextFormat = lang.getString("NetworkGameStatusWaiting");
            }
            snprintf(szBuf1, 8096, statusTextFormat.c_str(),
                     waitForHosts.c_str());

            logger.add(szBuf, true, szBuf1);

            sleep(0);
          }
        }
      } else {
        if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                .enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugNetwork, "In [%s::%s] Line: %d\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__);
        sendTextMessage("Unexpected network message: " +
                            intToStr(networkMessageType),
                        -1, true, "");

        if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                .enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugNetwork, "In [%s::%s] Line: %d\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__);

        DisplayErrorMessage(
            string(extractFileFromDirectoryPath(__FILE__).c_str()) +
            "::" + string(__FUNCTION__) +
            " Unexpected network message: " + intToStr(networkMessageType));

        if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                .enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugNetwork, "In [%s::%s] Line: %d\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__);

        sleep(1);
        setQuit(true);
        close();

        if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                .enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugNetwork, "In [%s::%s] Line: %d\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__);
        return;
      }

      Shared::Platform::Window::handleEvent();
      // sleep a bit
      sleep(waitSleepTime);
    }
  }

  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(
        SystemFlags::debugNetwork, "In [%s::%s] Line: %d\n",
        extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__, __LINE__);

  // check checksum
  if (getJoinGameInProgress() == false &&
      networkMessageReady.getChecksum() != checksum->getSum()) {

    if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugNetwork,
                               "In [%s::%s] Line: %d\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__);

    Lang &lang = Lang::getInstance();
    const vector<string> languageList =
        this->gameSettings.getUniqueNetworkPlayerLanguages();
    for (unsigned int langIndex = 0; langIndex < languageList.size();
         ++langIndex) {

      string sErr =
          "Checksum error, you don't have the same data as the server";
      if (lang.hasString("CheckSumGameLoadError", languageList[langIndex]) ==
          true) {
        sErr = lang.getString("CheckSumGameLoadError", languageList[langIndex]);
      }
      bool echoLocal = lang.isLanguageLocal(lang.getLanguage());
      sendTextMessage(sErr, -1, echoLocal, languageList[langIndex]);

      string playerNameStr = "Player with error is: " + getHumanPlayerName();
      if (lang.hasString("CheckSumGameLoadPlayer", languageList[langIndex]) ==
          true) {
        playerNameStr =
            lang.getString("CheckSumGameLoadPlayer", languageList[langIndex]) +
            " " + getHumanPlayerName();
      }
      sendTextMessage(playerNameStr, -1, echoLocal, languageList[langIndex]);

      string sErr1 = "Client Checksum: " + intToStr(checksum->getSum());
      if (lang.hasString("CheckSumGameLoadClient", languageList[langIndex]) ==
          true) {
        sErr1 =
            lang.getString("CheckSumGameLoadClient", languageList[langIndex]) +
            " " + intToStr(checksum->getSum());
      }

      sendTextMessage(sErr1, -1, echoLocal, languageList[langIndex]);

      string sErr2 =
          "Server Checksum: " + intToStr(networkMessageReady.getChecksum());
      if (lang.hasString("CheckSumGameLoadServer", languageList[langIndex]) ==
          true) {
        sErr2 =
            lang.getString("CheckSumGameLoadServer", languageList[langIndex]) +
            " " + intToStr(networkMessageReady.getChecksum());
      }
      sendTextMessage(sErr2, -1, echoLocal, languageList[langIndex]);

      if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
        SystemFlags::OutputDebug(
            SystemFlags::debugNetwork, "In [%s::%s] Line: %d %s %s %s\n",
            extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
            __LINE__, sErr.c_str(), sErr1.c_str(), sErr2.c_str());

      if (echoLocal == true) {
        if (Config::getInstance().getBool("NetworkConsistencyChecks")) {
          // error message and disconnect only if checked
          if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                  .enabled)
            SystemFlags::OutputDebug(
                SystemFlags::debugNetwork, "In [%s::%s] Line: %d\n",
                extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
                __LINE__);

          string niceError = sErr + string("\n") + sErr1 + string("\n") + sErr2;
          DisplayErrorMessage(niceError);
        }
      }
    }

    if (Config::getInstance().getBool("NetworkConsistencyChecks")) {
      // error message and disconnect only if checked
      if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,
                                 "In [%s::%s] Line: %d\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__);

      sleep(1);
      setQuit(true);

      if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,
                                 "In [%s::%s] Line: %d\n",
                                 extractFileFromDirectoryPath(__FILE__).c_str(),
                                 __FUNCTION__, __LINE__);

      close();
    }
    return;
  }

  MutexSafeWrapper safeMutexFlags2(flagAccessor, CODE_AT_LINE);
  this->joinGameInProgress = false;
  this->joinGameInProgressLaunch = false;

  // printf("Client signalServerWhenReadyToStartJoinedGame =
  // %d\n",signalServerWhenReadyToStartJoinedGame);
  if (signalServerWhenReadyToStartJoinedGame == true) {

    Lang &lang = Lang::getInstance();
    const vector<string> languageList =
        this->gameSettings.getUniqueNetworkPlayerLanguages();
    for (unsigned int langIndex = 0; langIndex < languageList.size();
         ++langIndex) {

      string sText = "Player: %s is joining the game now.";
      if (lang.hasString("JoinPlayerToCurrentGameLaunchDone",
                         languageList[langIndex]) == true) {
        sText = lang.getString("JoinPlayerToCurrentGameLaunchDone",
                               languageList[langIndex]);
      }

      if (clientSocket != NULL && clientSocket->isConnected() == true) {
        string playerNameStr = getHumanPlayerName();
        char szBuf[8096] = "";
        snprintf(szBuf, 8096, sText.c_str(), playerNameStr.c_str());

        sendTextMessage(szBuf, -1, false, languageList[langIndex]);
      }
    }

    this->resumeInGameJoin = true;
    safeMutexFlags2.ReleaseLock();
  } else {
    safeMutexFlags2.ReleaseLock();
    // delay the start a bit, so clients have more room to get messages
    // This is to ensure clients don't start ahead of the server and thus
    // constantly freeze because they are waiting for the server to catch up
    sleep(120);
  }

  // This triggers LAG update packets to begin as required
  lastNetworkCommandListSendTime = time(NULL);

  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugNetwork, "In [%s::%s] END\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__);
}

void ClientInterface::sendResumeGameMessage() {
  NetworkMessageReady networkMessageReady;
  sendMessage(&networkMessageReady);
}

void ClientInterface::sendTextMessage(const string &text, int teamIndex,
                                      bool echoLocal, string targetLanguage) {

  string humanPlayerName = getHumanPlayerName();
  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(
        SystemFlags::debugNetwork,
        "In [%s::%s Line: %d] humanPlayerName = [%s] playerIndex = %d\n",
        extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__, __LINE__,
        humanPlayerName.c_str(), playerIndex);

  NetworkMessageText networkMessageText(text, teamIndex, playerIndex,
                                        targetLanguage);
  sendMessage(&networkMessageText);

  if (echoLocal == true) {
    ChatMsgInfo msg(networkMessageText.getText().c_str(),
                    networkMessageText.getTeamIndex(),
                    networkMessageText.getPlayerIndex(), targetLanguage);
    this->addChatInfo(msg);
  }
}

void ClientInterface::sendMarkCellMessage(Vec2i targetPos, int factionIndex,
                                          string note, int playerIndex) {
  string humanPlayerName = getHumanPlayerName();
  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(
        SystemFlags::debugNetwork,
        "In [%s::%s Line: %d] humanPlayerName = [%s] playerIndex = %d\n",
        extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__, __LINE__,
        humanPlayerName.c_str(), playerIndex);

  NetworkMessageMarkCell networkMessageMarkCell(targetPos, factionIndex, note,
                                                playerIndex);
  sendMessage(&networkMessageMarkCell);
}

void ClientInterface::sendHighlightCellMessage(Vec2i targetPos,
                                               int factionIndex) {
  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,
                             "In [%s::%s Line: %d] playerIndex = %d\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__, playerIndex);

  NetworkMessageHighlightCell networkMessageHighlightCell(targetPos,
                                                          factionIndex);
  sendMessage(&networkMessageHighlightCell);
}

void ClientInterface::sendUnMarkCellMessage(Vec2i targetPos, int factionIndex) {
  string humanPlayerName = getHumanPlayerName();
  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(
        SystemFlags::debugNetwork,
        "In [%s::%s Line: %d] humanPlayerName = [%s] playerIndex = %d\n",
        extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__, __LINE__,
        humanPlayerName.c_str(), playerIndex);

  NetworkMessageUnMarkCell networkMessageMarkCell(targetPos, factionIndex);
  sendMessage(&networkMessageMarkCell);
}

void ClientInterface::sendPingMessage(int32 pingFrequency, int64 pingTime) {
  NetworkMessagePing networkMessagePing(pingFrequency, pingTime);
  sendMessage(&networkMessagePing);
}

string ClientInterface::getNetworkStatus() {
  std::string label =
      Lang::getInstance().getString("Server") + ": " + serverName;
  // float pingTime = getThreadedPingMS(getServerIpAddress().c_str());
  char szBuf[8096] = "";
  snprintf(szBuf, 8096, "%s", label.c_str());

  return szBuf;
}

NetworkMessageType ClientInterface::waitForMessage(int waitMicroseconds) {
  // Debug!
  /*
      sendTextMessage("Timeout waiting for message",-1);
      DisplayErrorMessage("Timeout waiting for message");
      quit= true;
      close();
      return;
  */

  uint64 loopCount = 0;
  Chrono chronoPerf;
  if (debugClientInterfacePerf == true) {
    chronoPerf.start();
  }

  Chrono chrono;
  chrono.start();

  NetworkMessageType msg = nmtInvalid;
  while (msg == nmtInvalid && getQuitThread() == false) {

    msg = getNextMessageType(waitMicroseconds);
    if (msg == nmtInvalid) {
      if (getSocket() == NULL ||
          (chrono.getMillis() % 250 == 0 && isConnected() == false)) {
        if (getQuit() == false) {
          // throw megaglest_runtime_error("Disconnected");
          // sendTextMessage("Server has Disconnected.",-1);
          DisplayErrorMessage("Server has Disconnected.");
          setQuit(true);
        }
        close();
        return msg;
      }

      if (chrono.getMillis() > messageWaitTimeout) {
        if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork)
                .enabled)
          SystemFlags::OutputDebug(
              SystemFlags::debugNetwork, "In [%s::%s Line: %d]\n",
              extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__,
              __LINE__);

        Lang &lang = Lang::getInstance();
        const vector<string> languageList =
            this->gameSettings.getUniqueNetworkPlayerLanguages();
        for (unsigned int langIndex = 0; langIndex < languageList.size();
             ++langIndex) {

          string msg = "Timeout waiting for message.";
          if (lang.hasString("TimeoutWaitingForMessage",
                             languageList[langIndex]) == true) {
            msg = lang.getString("TimeoutWaitingForMessage",
                                 languageList[langIndex]);
          }

          sendTextMessage(msg, -1,
                          lang.isLanguageLocal(languageList[langIndex]),
                          languageList[langIndex]);
          if (lang.isLanguageLocal(languageList[langIndex]) == true) {
            DisplayErrorMessage(msg);
          }
        }

        sleep(1);
        setQuit(true);
        close();
        return msg;
      }
      // Sleep every x milli-seconds we wait to let other threads work
      else if (chrono.getMillis() % 2 == 0) {
        sleep(1);
      } else {
        sleep(0);
      }
    }

    if (debugClientInterfacePerf == true) {
      loopCount++;
      if (chronoPerf.getMillis() >= 100) {
        printf("Client waitForMessage loopCount = %llu\n",
               (long long unsigned int)loopCount);

        loopCount = 0;
        chronoPerf.start();
      }
    }
  }

  if (SystemFlags::getSystemSettingType(SystemFlags::debugPerformance)
          .enabled &&
      chrono.getMillis() > 1)
    SystemFlags::OutputDebug(
        SystemFlags::debugPerformance,
        "In [%s::%s Line: %d] waiting took %lld msecs, msg = %d\n",
        extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__, __LINE__,
        chrono.getMillis(), msg);

  return msg;
}

void ClientInterface::quitGame(bool userManuallyQuit) {
  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,
                             "In [%s::%s Line: %d] userManuallyQuit = %d\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__, userManuallyQuit);

  if (clientSocket != NULL && userManuallyQuit == true) {
    if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugNetwork,
                               "In [%s::%s Line: %d]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__);

    Lang &lang = Lang::getInstance();
    const vector<string> languageList =
        this->gameSettings.getUniqueNetworkPlayerLanguages();
    for (unsigned int langIndex = 0; langIndex < languageList.size();
         ++langIndex) {

      string msg = "has chosen to leave the game!";
      if (lang.hasString("PlayerLeftGame", languageList[langIndex]) == true) {
        msg = lang.getString("PlayerLeftGame", languageList[langIndex]);
      }

      sendTextMessage(msg, -1, lang.isLanguageLocal(languageList[langIndex]),
                      languageList[langIndex]);
    }
    sleep(1);
    close();
  }

  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(
        SystemFlags::debugNetwork, "In [%s::%s Lined: %d]\n",
        extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__, __LINE__);
}

void ClientInterface::close(bool lockMutex) {
  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,
                             "In [%s::%s Line: %d] START, clientSocket = %p\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__, clientSocket);

  MutexSafeWrapper safeMutex(NULL, CODE_AT_LINE);
  if (lockMutex == true) {
    safeMutex.setMutex(networkCommandListThreadAccessor, CODE_AT_LINE);
  }
  shutdownNetworkCommandListThread(safeMutex);

  delete clientSocket;
  clientSocket = NULL;

  safeMutex.ReleaseLock();

  connectedTime = 0;
  gotIntro = false;

  MutexSafeWrapper safeMutexFlags(flagAccessor, CODE_AT_LINE);
  this->joinGameInProgress = false;
  this->joinGameInProgressLaunch = false;
  this->readyForInGameJoin = false;

  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(
        SystemFlags::debugNetwork, "In [%s::%s Line: %d] END\n",
        extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__, __LINE__);
}

void ClientInterface::close() { close(true); }

void ClientInterface::discoverServers(DiscoveredServersInterface *cb) {
  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(
        SystemFlags::debugNetwork, "In [%s::%s Line: %d]\n",
        extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__, __LINE__);

  ClientSocket::discoverServers(cb);

  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(
        SystemFlags::debugNetwork, "In [%s::%s Line: %d]\n",
        extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__, __LINE__);
}
void ClientInterface::stopServerDiscovery() {
  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(
        SystemFlags::debugNetwork, "In [%s::%s Line: %d]\n",
        extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__, __LINE__);

  ClientSocket::stopBroadCastClientThread();

  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(
        SystemFlags::debugNetwork, "In [%s::%s Line: %d]\n",
        extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__, __LINE__);
}

void ClientInterface::sendSwitchSetupRequest(string selectedFactionName,
                                             int8 currentSlotIndex,
                                             int8 toSlotIndex, int8 toTeam,
                                             string networkPlayerName,
                                             int8 networkPlayerStatus,
                                             int8 flags, string language) {
  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(
        SystemFlags::debugNetwork,
        "In [%s::%s Line: %d] networkPlayerName [%s] flags = %d\n",
        extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__, __LINE__,
        networkPlayerName.c_str(), flags);
  SwitchSetupRequest message = SwitchSetupRequest(
      selectedFactionName, currentSlotIndex, toSlotIndex, toTeam,
      networkPlayerName, networkPlayerStatus, flags, language);
  sendMessage(&message);
  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(
        SystemFlags::debugNetwork, "In [%s::%s Line: %d]\n",
        extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__, __LINE__);
}

bool ClientInterface::shouldDiscardNetworkMessage(
    NetworkMessageType networkMessageType) {
  bool discard = false;

  switch (networkMessageType) {
  case nmtIntro: {
    discard = true;
    NetworkMessageIntro msg = NetworkMessageIntro();
    this->receiveMessage(&msg);
  } break;
  case nmtPing: {
    discard = true;
    NetworkMessagePing msg = NetworkMessagePing();
    this->receiveMessage(&msg);
    this->setLastPingInfo(msg);
  } break;
  case nmtLaunch: {
    discard = true;
    NetworkMessageLaunch msg = NetworkMessageLaunch();
    this->receiveMessage(&msg);
  } break;
  case nmtText: {
    if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugNetwork,
                               "In [%s::%s Line: %d] got nmtText\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__);
    discard = true;
    NetworkMessageText netMsg = NetworkMessageText();
    this->receiveMessage(&netMsg);

    ChatMsgInfo msg(netMsg.getText().c_str(), netMsg.getTeamIndex(),
                    netMsg.getPlayerIndex(), netMsg.getTargetLanguage());
    this->addChatInfo(msg);
  } break;

  case nmtMarkCell: {
    discard = true;
    NetworkMessageMarkCell networkMessageMarkCell;
    receiveMessage(&networkMessageMarkCell);

    MarkedCell msg(networkMessageMarkCell.getTarget(),
                   networkMessageMarkCell.getFactionIndex(),
                   networkMessageMarkCell.getText().c_str(),
                   networkMessageMarkCell.getPlayerIndex());
    this->addMarkedCell(msg);
  } break;

  case nmtUnMarkCell: {
    discard = true;
    NetworkMessageUnMarkCell networkMessageMarkCell;
    receiveMessage(&networkMessageMarkCell);

    UnMarkedCell msg(networkMessageMarkCell.getTarget(),
                     networkMessageMarkCell.getFactionIndex());
    this->addUnMarkedCell(msg);
  } break;

  case nmtHighlightCell: {
    discard = true;
    NetworkMessageHighlightCell networkMessageHighlightCell;
    receiveMessage(&networkMessageHighlightCell);

    MarkedCell msg(networkMessageHighlightCell.getTarget(),
                   networkMessageHighlightCell.getFactionIndex(), "none", -1);
    this->setHighlightedCell(msg);
  } break;

  case nmtSynchNetworkGameData: {
    discard = true;
    NetworkMessageSynchNetworkGameData msg =
        NetworkMessageSynchNetworkGameData();
    this->receiveMessage(&msg);
  } break;
  case nmtSynchNetworkGameDataStatus: {
    discard = true;
    NetworkMessageSynchNetworkGameDataStatus msg =
        NetworkMessageSynchNetworkGameDataStatus();
    this->receiveMessage(&msg);
  } break;
  case nmtSynchNetworkGameDataFileCRCCheck: {
    discard = true;
    NetworkMessageSynchNetworkGameDataFileCRCCheck msg =
        NetworkMessageSynchNetworkGameDataFileCRCCheck();
    this->receiveMessage(&msg);
  } break;
  case nmtSynchNetworkGameDataFileGet: {
    discard = true;
    NetworkMessageSynchNetworkGameDataFileGet msg =
        NetworkMessageSynchNetworkGameDataFileGet();
    this->receiveMessage(&msg);
  } break;
  case nmtSwitchSetupRequest: {
    discard = true;
    SwitchSetupRequest msg = SwitchSetupRequest();
    this->receiveMessage(&msg);
  } break;
  case nmtBroadCastSetup: {
    discard = true;
    NetworkMessageLaunch msg = NetworkMessageLaunch();
    this->receiveMessage(&msg);
  } break;

  case nmtPlayerIndexMessage: {
    discard = true;
    PlayerIndexMessage msg = PlayerIndexMessage(0);
    this->receiveMessage(&msg);
  } break;
  }

  return discard;
}

string ClientInterface::getHumanPlayerName(int index) {
  string result = Config::getInstance().getString(
      "NetPlayerName", Socket::getHostName().c_str());

  if (index >= 0 || gameSettings.getThisFactionIndex() >= 0) {
    if (index < 0) {
      index = gameSettings.getThisFactionIndex();
    }
    if (gameSettings.getNetworkPlayerName(index) != "") {
      result = gameSettings.getNetworkPlayerName(index);
    }
  }

  return result;
}

void ClientInterface::setGameSettings(GameSettings *serverGameSettings) {
  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugNetwork, "In [%s::%s] START\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__);

  gameSettings = *serverGameSettings;

  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(SystemFlags::debugNetwork, "In [%s::%s] END\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__);
}

void ClientInterface::broadcastGameSetup(const GameSettings *gameSettings) {
  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(
        SystemFlags::debugNetwork, "In [%s::%s] Line: %d\n",
        extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__, __LINE__);

  NetworkMessageLaunch networkMessageLaunch(gameSettings, nmtBroadCastSetup);
  sendMessage(&networkMessageLaunch);

  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(
        SystemFlags::debugNetwork, "In [%s::%s] Line: %d\n",
        extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__, __LINE__);
}

void ClientInterface::broadcastGameStart(const GameSettings *gameSettings) {
  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(
        SystemFlags::debugNetwork, "In [%s::%s] Line: %d\n",
        extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__, __LINE__);

  MutexSafeWrapper safeMutexFlags(flagAccessor, CODE_AT_LINE);
  if (this->joinGameInProgress == true) {
    this->joinGameInProgressLaunch = true;
  }
  safeMutexFlags.ReleaseLock();

  // printf("Sending game launch joinGameInProgress: %d\n",joinGameInProgress);

  NetworkMessageLaunch networkMessageLaunch(gameSettings, nmtLaunch);
  sendMessage(&networkMessageLaunch);

  if (SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled)
    SystemFlags::OutputDebug(
        SystemFlags::debugNetwork, "In [%s::%s] Line: %d\n",
        extractFileFromDirectoryPath(__FILE__).c_str(), __FUNCTION__, __LINE__);
}

void ClientInterface::setGameSettingsReceived(bool value) {
  // printf("In [%s:%s] Line: %d gameSettingsReceived = %d value = %d,
  // gameSettingsReceivedCount =
  // %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,gameSettingsReceived,value,gameSettingsReceivedCount);
  gameSettingsReceived = value;
  gameSettingsReceivedCount++;
}

} // namespace Game
} // namespace Glest
