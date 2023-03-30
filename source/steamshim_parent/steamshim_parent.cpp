#ifndef GAME_LAUNCH_NAME
#define GAME_LAUNCH_NAME "megaglest"
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#define UNICODE
#include <windows.h>
typedef PROCESS_INFORMATION ProcessType;
typedef HANDLE PipeType;
#define NULLPIPE NULL
#define LLUFMT "%I64u"
#else
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
typedef pid_t ProcessType;
typedef int PipeType;
#define NULLPIPE -1
#define LLUFMT "%llu"
#endif
#include <stdlib.h>

#include "steam/steam_api.h"

#ifdef STEAMSHIM_DEBUG
#define dbgpipe printf
#else
static inline void dbgpipe(const char *fmt, ...) {}
#endif

/* platform-specific mainline calls this. */
static int mainline(void);

/* Windows and Unix implementations of this stuff below. */
static void fail(const char *err);
static bool writePipe(PipeType fd, const void *buf, const unsigned int _len);
static int readPipe(PipeType fd, void *buf, const unsigned int _len);
static bool createPipes(PipeType *pPipeParentRead, PipeType *pPipeParentWrite,
                        PipeType *pPipeChildRead, PipeType *pPipeChildWrite);
static void closePipe(PipeType fd);
static bool setEnvVar(const char *key, const char *val);
static bool launchChild(ProcessType *pid);
static int closeProcess(ProcessType *pid);

#ifdef _WIN32
bool fileExists(char *path) {
  FILE *file = fopen(path, "rb");
  if (file != NULL) {
    fclose(file);
    return true;
  }
  return false;
}

static void fail(const char *err) {
  MessageBoxA(NULL, err, "ERROR", MB_ICONERROR | MB_OK);
  ExitProcess(1);
} // fail

static bool writePipe(PipeType fd, const void *buf, const unsigned int _len) {
  const DWORD len = (DWORD)_len;
  DWORD bw = 0;
  return ((WriteFile(fd, buf, len, &bw, NULL) != 0) && (bw == len));
} // writePipe

static int readPipe(PipeType fd, void *buf, const unsigned int _len) {
  const DWORD len = (DWORD)_len;
  DWORD br = 0;
  return ReadFile(fd, buf, len, &br, NULL) ? (int)br : -1;
} // readPipe

static bool createPipes(PipeType *pPipeParentRead, PipeType *pPipeParentWrite,
                        PipeType *pPipeChildRead, PipeType *pPipeChildWrite) {
  SECURITY_ATTRIBUTES pipeAttr;

  pipeAttr.nLength = sizeof(pipeAttr);
  pipeAttr.lpSecurityDescriptor = NULL;
  pipeAttr.bInheritHandle = TRUE;
  if (!CreatePipe(pPipeParentRead, pPipeChildWrite, &pipeAttr, 0))
    return 0;

  pipeAttr.nLength = sizeof(pipeAttr);
  pipeAttr.lpSecurityDescriptor = NULL;
  pipeAttr.bInheritHandle = TRUE;
  if (!CreatePipe(pPipeChildRead, pPipeParentWrite, &pipeAttr, 0)) {
    CloseHandle(*pPipeParentRead);
    CloseHandle(*pPipeChildWrite);
    return 0;
  } // if

  return 1;
} // createPipes

static void closePipe(PipeType fd) { CloseHandle(fd); } // closePipe

static bool setEnvVar(const char *key, const char *val) {
  return (SetEnvironmentVariableA(key, val) != 0);
} // setEnvVar

static LPWSTR genCommandLine() {
  // Construct a command line with the appropriate filename
  LPWSTR cmdline = GetCommandLineW();

  // Find the index of the first argument after 0
  int iFirstArg = -1;
  bool quote = false;
  bool whitespace = false;
  for (int i = 0; cmdline[i]; ++i) {
    if (cmdline[i] == '"' && (i == 0 || cmdline[i - 1] != '\\')) {
      quote = !quote;
      whitespace = false;
    } else if (!quote && (cmdline[i] == ' ' || cmdline[i] == '\t')) {
      whitespace = true;
    } else {
      if (whitespace) {
        iFirstArg = i;
        break;
      }
      whitespace = false;
    }
  }

  // If it doesn't exist, that must mean there are no arguments,
  // so just return GAME_LAUNCH_NAME
  bool default_file_exists = true;
  if (!fileExists(GAME_LAUNCH_NAME ".exe")) {
    default_file_exists = false;
  }
  if (iFirstArg == -1) {
    if (default_file_exists) {
      return _wcsdup(TEXT("\".\\" GAME_LAUNCH_NAME ".exe\""));
    }
    return _wcsdup(TEXT("\".\\" GAME_LAUNCH_NAME "x64.exe\""));
  }

  // Create the new string
  // (`".\.exe" ` == +9
  LPWSTR newcmdline = NULL;
  if (default_file_exists) {
    newcmdline =
        (LPWSTR)malloc(sizeof(TEXT(GAME_LAUNCH_NAME)) +
                       sizeof(WCHAR) * (wcslen(cmdline) - iFirstArg + 9));
    wsprintf(newcmdline, TEXT("\".\\" GAME_LAUNCH_NAME ".exe\" %s"),
             cmdline + iFirstArg);
  } else {
    newcmdline =
        (LPWSTR)malloc(sizeof(TEXT(GAME_LAUNCH_NAME "x64")) +
                       sizeof(WCHAR) * (wcslen(cmdline) - iFirstArg + 9));
    wsprintf(newcmdline, TEXT("\".\\" GAME_LAUNCH_NAME "x64.exe\" %s"),
             cmdline + iFirstArg);
  }
  return newcmdline;
}

static bool launchChild(ProcessType *pid) {
  bool default_file_exists = true;
  if (!fileExists(GAME_LAUNCH_NAME ".exe")) {
    default_file_exists = false;
  }

  STARTUPINFOW si;
  memset(&si, 0, sizeof(si));
  if (default_file_exists) {
    return CreateProcessW(TEXT(".\\" GAME_LAUNCH_NAME ".exe"), genCommandLine(),
                          NULL, NULL, TRUE, 0, NULL, NULL, &si, pid);
  }
  return CreateProcessW(TEXT(".\\" GAME_LAUNCH_NAME "x64.exe"),
                        genCommandLine(), NULL, NULL, TRUE, 0, NULL, NULL, &si,
                        pid);

} // launchChild

static int closeProcess(ProcessType *pid) {
  CloseHandle(pid->hProcess);
  CloseHandle(pid->hThread);
  return 0;
} // closeProcess

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nCmdShow) {
  mainline();
  ExitProcess(0);
  return 0; // just in case.
} // WinMain

#else // everyone else that isn't Windows.

static void fail(const char *err) {
  // !!! FIXME: zenity or something.
  fprintf(stderr, "%s\n", err);
  _exit(1);
} // fail

static bool writePipe(PipeType fd, const void *buf, const unsigned int _len) {
  const ssize_t len = (ssize_t)_len;
  ssize_t bw;
  while (((bw = write(fd, buf, len)) == -1) && (errno == EINTR)) { /*spin*/
  }
  return (bw == len);
} // writePipe

static int readPipe(PipeType fd, void *buf, const unsigned int _len) {
  const ssize_t len = (ssize_t)_len;
  ssize_t br;
  while (((br = read(fd, buf, len)) == -1) && (errno == EINTR)) { /*spin*/
  }
  return (int)br;
} // readPipe

static bool createPipes(PipeType *pPipeParentRead, PipeType *pPipeParentWrite,
                        PipeType *pPipeChildRead, PipeType *pPipeChildWrite) {
  int fds[2];
  if (pipe(fds) == -1)
    return 0;
  fcntl(fds[0], F_SETFL, 0);
  fcntl(fds[1], F_SETFL, 0);
  *pPipeParentRead = fds[0];
  *pPipeChildWrite = fds[1];

  if (pipe(fds) == -1) {
    close(*pPipeParentRead);
    close(*pPipeChildWrite);
    return 0;
  } // if

  fcntl(fds[0], F_SETFL, 0);
  fcntl(fds[1], F_SETFL, 0);
  *pPipeChildRead = fds[0];
  *pPipeParentWrite = fds[1];

  return 1;
} // createPipes

static void closePipe(PipeType fd) { close(fd); } // closePipe

static bool setEnvVar(const char *key, const char *val) {
  return (setenv(key, val, 1) != -1);
} // setEnvVar

static int GArgc = 0;
static char **GArgv = NULL;

static bool launchChild(ProcessType *pid) {
  dbgpipe("***IN launchChild START.\n");

  *pid = fork();

  dbgpipe("***IN launchChild pid = %d.\n", *pid);
  if (*pid == -1) // failed
    return false;
  else if (*pid != 0) // we're the parent
    return true;      // we'll let the pipe fail if this didn't work.

  // we're the child.
  GArgv[0] = strdup("./" GAME_LAUNCH_NAME);

  dbgpipe("***IN launchChild [%s].\n", GArgv[0]);

  int ret = execvp(GArgv[0], GArgv);

  dbgpipe("***IN launchChild [%s] got ret = %d errorno = %d.\n", GArgv[0], ret,
          errno);
  // still here? It failed! Terminate, closing child's ends of the pipes.
  _exit(1);
} // launchChild

static int closeProcess(ProcessType *pid) {
  int rc = 0;
  while ((waitpid(*pid, &rc, 0) == -1) && (errno == EINTR)) { /*spin*/
  }
  if (!WIFEXITED(rc))
    return 1; // oh well.
  return WEXITSTATUS(rc);
} // closeProcess

int main(int argc, char **argv) {
  printf("#1 =============> Megaglest Parent starting mainline.\n");

  signal(SIGPIPE, SIG_IGN);
  GArgc = argc;
  GArgv = argv;
  return mainline();
} // main

#endif

// THE ACTUAL PROGRAM.

class SteamBridge;

static ISteamUserStats *GSteamStats = NULL;
static ISteamUtils *GSteamUtils = NULL;
static ISteamUser *GSteamUser = NULL;
static ISteamFriends *GSteamFriends = NULL;
static ISteamApps *GSteamApps = NULL;
static AppId_t GAppID = 0;
static uint64 GUserID = 0;
static SteamBridge *GSteamBridge = NULL;

class SteamBridge {
public:
  explicit SteamBridge(PipeType _fd);
  STEAM_CALLBACK(SteamBridge, OnUserStatsReceived, UserStatsReceived_t,
                 m_CallbackUserStatsReceived);
  STEAM_CALLBACK(SteamBridge, OnUserStatsStored, UserStatsStored_t,
                 m_CallbackUserStatsStored);

private:
  PipeType fd;
};

typedef enum ShimCmd {
  SHIMCMD_BYE,
  SHIMCMD_PUMP,
  SHIMCMD_REQUESTSTATS,
  SHIMCMD_STORESTATS,
  SHIMCMD_SETACHIEVEMENT,
  SHIMCMD_GETACHIEVEMENT,
  SHIMCMD_RESETSTATS,
  SHIMCMD_SETSTATI,
  SHIMCMD_GETSTATI,
  SHIMCMD_SETSTATF,
  SHIMCMD_GETSTATF,
  SHIMCMD_GETPERSONANAME,
  SHIMCMD_GETCURRENTGAMELANGUAGE,
} ShimCmd;

typedef enum ShimEvent {
  SHIMEVENT_BYE,
  SHIMEVENT_STATSRECEIVED,
  SHIMEVENT_STATSSTORED,
  SHIMEVENT_SETACHIEVEMENT,
  SHIMEVENT_GETACHIEVEMENT,
  SHIMEVENT_RESETSTATS,
  SHIMEVENT_SETSTATI,
  SHIMEVENT_GETSTATI,
  SHIMEVENT_SETSTATF,
  SHIMEVENT_GETSTATF,
  SHIMEVENT_GETPERSONANAME,
  SHIMEVENT_GETCURRENTGAMELANGUAGE,
} ShimEvent;

static bool write1ByteCmd(PipeType fd, const uint8 b1) {
  const uint8 buf[] = {1, b1};
  return writePipe(fd, buf, sizeof(buf));
} // write1ByteCmd

static bool write2ByteCmd(PipeType fd, const uint8 b1, const uint8 b2) {
  const uint8 buf[] = {2, b1, b2};
  return writePipe(fd, buf, sizeof(buf));
} // write2ByteCmd

static bool write3ByteCmd(PipeType fd, const uint8 b1, const uint8 b2,
                          const uint8 b3) {
  const uint8 buf[] = {3, b1, b2, b3};
  return writePipe(fd, buf, sizeof(buf));
} // write3ByteCmd

static bool writeString(PipeType fd, ShimEvent event, const char *str) {
  uint8 buf[256];
  buf[0] = strlen(str) + 2;
  buf[1] = (uint8)event;
  strcpy((char *)buf + 2, str);
  return writePipe(fd, buf, buf[0] + 1);
} // writeString

static inline bool writeBye(PipeType fd) {
  dbgpipe("Parent sending SHIMEVENT_BYE().\n");
  return write1ByteCmd(fd, SHIMEVENT_BYE);
} // writeBye

static inline bool writeStatsReceived(PipeType fd, const bool okay) {
  dbgpipe("Parent sending SHIMEVENT_STATSRECEIVED(%sokay).\n", okay ? "" : "!");
  return write2ByteCmd(fd, SHIMEVENT_STATSRECEIVED, okay ? 1 : 0);
} // writeStatsReceived

static inline bool writeStatsStored(PipeType fd, const bool okay) {
  dbgpipe("Parent sending SHIMEVENT_STATSSTORED(%sokay).\n", okay ? "" : "!");
  return write2ByteCmd(fd, SHIMEVENT_STATSSTORED, okay ? 1 : 0);
} // writeStatsStored

static bool writeAchievementSet(PipeType fd, const char *name,
                                const bool enable, const bool okay) {
  uint8 buf[256];
  uint8 *ptr = buf + 1;
  dbgpipe("Parent sending SHIMEVENT_SETACHIEVEMENT('%s', %senable, %sokay).\n",
          name, enable ? "" : "!", okay ? "" : "!");
  *(ptr++) = (uint8)SHIMEVENT_SETACHIEVEMENT;
  *(ptr++) = enable ? 1 : 0;
  *(ptr++) = okay ? 1 : 0;
  strcpy((char *)ptr, name);
  ptr += strlen(name) + 1;
  buf[0] = (uint8)((ptr - 1) - buf);
  return writePipe(fd, buf, buf[0] + 1);
} // writeAchievementSet

static bool writeAchievementGet(PipeType fd, const char *name, const int status,
                                const uint64 time) {
  uint8 buf[256];
  uint8 *ptr = buf + 1;
  dbgpipe(
      "Parent sending SHIMEVENT_GETACHIEVEMENT('%s', status %d, time " LLUFMT
      ").\n",
      name, status, (unsigned long long)time);
  *(ptr++) = (uint8)SHIMEVENT_GETACHIEVEMENT;
  *(ptr++) = (uint8)status;
  memcpy(ptr, &time, sizeof(time));
  ptr += sizeof(time);
  strcpy((char *)ptr, name);
  ptr += strlen(name) + 1;
  buf[0] = (uint8)((ptr - 1) - buf);
  return writePipe(fd, buf, buf[0] + 1);
} // writeAchievementGet

static inline bool writeResetStats(PipeType fd, const bool alsoAch,
                                   const bool okay) {
  dbgpipe("Parent sending SHIMEVENT_RESETSTATS(%salsoAchievements, %sokay).\n",
          alsoAch ? "" : "!", okay ? "" : "!");
  return write3ByteCmd(fd, SHIMEVENT_RESETSTATS, alsoAch ? 1 : 0, okay ? 1 : 0);
} // writeResetStats

static bool writeStatThing(PipeType fd, const ShimEvent ev, const char *name,
                           const void *val, const size_t vallen,
                           const bool okay) {
  uint8 buf[256];
  uint8 *ptr = buf + 1;
  *(ptr++) = (uint8)ev;
  *(ptr++) = okay ? 1 : 0;
  memcpy(ptr, val, vallen);
  ptr += vallen;
  strcpy((char *)ptr, name);
  ptr += strlen(name) + 1;
  buf[0] = (uint8)((ptr - 1) - buf);
  return writePipe(fd, buf, buf[0] + 1);
} // writeStatThing

static inline bool writeSetStatI(PipeType fd, const char *name, const int32 val,
                                 const bool okay) {
  dbgpipe("Parent sending SHIMEVENT_SETSTATI('%s', val %d, %sokay).\n", name,
          (int)val, okay ? "" : "!");
  return writeStatThing(fd, SHIMEVENT_SETSTATI, name, &val, sizeof(val), okay);
} // writeSetStatI

static inline bool writeSetStatF(PipeType fd, const char *name, const float val,
                                 const bool okay) {
  dbgpipe("Parent sending SHIMEVENT_SETSTATF('%s', val %f, %sokay).\n", name,
          val, okay ? "" : "!");
  return writeStatThing(fd, SHIMEVENT_SETSTATF, name, &val, sizeof(val), okay);
} // writeSetStatF

static inline bool writeGetStatI(PipeType fd, const char *name, const int32 val,
                                 const bool okay) {
  dbgpipe("Parent sending SHIMEVENT_GETSTATI('%s', val %d, %sokay).\n", name,
          (int)val, okay ? "" : "!");
  return writeStatThing(fd, SHIMEVENT_GETSTATI, name, &val, sizeof(val), okay);
} // writeGetStatI

static inline bool writeGetStatF(PipeType fd, const char *name, const float val,
                                 const bool okay) {
  dbgpipe("Parent sending SHIMEVENT_GETSTATF('%s', val %f, %sokay).\n", name,
          val, okay ? "" : "!");
  return writeStatThing(fd, SHIMEVENT_GETSTATF, name, &val, sizeof(val), okay);
} // writeGetStatF

SteamBridge::SteamBridge(PipeType _fd)
    : m_CallbackUserStatsReceived(this, &SteamBridge::OnUserStatsReceived),
      m_CallbackUserStatsStored(this, &SteamBridge::OnUserStatsStored),
      fd(_fd) {} // SteamBridge::SteamBridge

void SteamBridge::OnUserStatsReceived(UserStatsReceived_t *pCallback) {
  dbgpipe("***IN OnUserStatsReceived GAppID = %d, GUserID = %lld\n", GAppID,
          GUserID);

  if (GAppID != pCallback->m_nGameID)
    return;
  if (GUserID != pCallback->m_steamIDUser.ConvertToUint64())
    return;
  writeStatsReceived(fd, pCallback->m_eResult == k_EResultOK);
} // SteamBridge::OnUserStatsReceived

void SteamBridge::OnUserStatsStored(UserStatsStored_t *pCallback) {
  dbgpipe("***IN OnUserStatsStored GAppID = %d, GUserID = %lld\n", GAppID,
          GUserID);

  if (GAppID != pCallback->m_nGameID)
    return;
  writeStatsStored(fd, pCallback->m_eResult == k_EResultOK);
} // SteamBridge::OnUserStatsStored

static bool processCommand(const uint8 *buf, unsigned int buflen, PipeType fd) {
  dbgpipe("***IN processCommand GAppID = %d, GUserID = %lld\n", GAppID,
          GUserID);

  if (buflen == 0)
    return true;

  const ShimCmd cmd = (ShimCmd) * (buf++);
  buflen--;

#if STEAMSHIM_DEBUG
  if (false) {
  }
#define PRINTGOTCMD(x) else if (cmd == x) printf("Parent got " #x ".\n")
  PRINTGOTCMD(SHIMCMD_BYE);
  PRINTGOTCMD(SHIMCMD_PUMP);
  PRINTGOTCMD(SHIMCMD_REQUESTSTATS);
  PRINTGOTCMD(SHIMCMD_STORESTATS);
  PRINTGOTCMD(SHIMCMD_SETACHIEVEMENT);
  PRINTGOTCMD(SHIMCMD_GETACHIEVEMENT);
  PRINTGOTCMD(SHIMCMD_RESETSTATS);
  PRINTGOTCMD(SHIMCMD_SETSTATI);
  PRINTGOTCMD(SHIMCMD_GETSTATI);
  PRINTGOTCMD(SHIMCMD_SETSTATF);
  PRINTGOTCMD(SHIMCMD_GETSTATF);
  PRINTGOTCMD(SHIMCMD_GETPERSONANAME);
  PRINTGOTCMD(SHIMCMD_GETCURRENTGAMELANGUAGE);
#undef PRINTGOTCMD
  else printf("Parent got unknown shimcmd %d.\n", (int)cmd);
#endif

  switch (cmd) {
  case SHIMCMD_PUMP:
    SteamAPI_RunCallbacks();
    break;

  case SHIMCMD_BYE:
    writeBye(fd);
    return false;

  case SHIMCMD_REQUESTSTATS:
    if ((!GSteamStats) || (!GSteamStats->RequestCurrentStats()))
      writeStatsReceived(fd, false);
    // callback later.
    break;

  case SHIMCMD_STORESTATS:
    if ((!GSteamStats) || (!GSteamStats->StoreStats()))
      writeStatsStored(fd, false);
    // callback later.
    break;

  case SHIMCMD_SETACHIEVEMENT:
    if (buflen >= 2) {
      const bool enable = (*(buf++) != 0);
      const char *name =
          (const char *)buf; // !!! FIXME: buffer overflow possible.
      if (!GSteamStats)
        writeAchievementSet(fd, name, enable, false);
      else if (enable && !GSteamStats->SetAchievement(name))
        writeAchievementSet(fd, name, enable, false);
      else if (!enable && !GSteamStats->ClearAchievement(name))
        writeAchievementSet(fd, name, enable, false);
      else
        writeAchievementSet(fd, name, enable, true);
    } // if
    break;

  case SHIMCMD_GETACHIEVEMENT:
    if (buflen) {
      const char *name =
          (const char *)buf; // !!! FIXME: buffer overflow possible.
      bool ach = false;
      uint32 t = 0;
      if ((GSteamStats) &&
          (GSteamStats->GetAchievementAndUnlockTime(name, &ach, &t)))
        writeAchievementGet(fd, name, ach ? 1 : 0, t);
      else
        writeAchievementGet(fd, name, 2, 0);
    } // if
    break;

  case SHIMCMD_RESETSTATS:
    if (buflen) {
      const bool alsoAch = (*(buf++) != 0);
      writeResetStats(fd, alsoAch,
                      (GSteamStats) && (GSteamStats->ResetAllStats(alsoAch)));
    } // if
    break;

  case SHIMCMD_SETSTATI:
    if (buflen >= 5) {
      const int32 val = *((int32 *)buf);
      buf += sizeof(int32);
      const char *name =
          (const char *)buf; // !!! FIXME: buffer overflow possible.
      writeSetStatI(fd, name, val,
                    (GSteamStats) && (GSteamStats->SetStat(name, val)));
    } // if
    break;

  case SHIMCMD_GETSTATI:
    if (buflen) {
      const char *name =
          (const char *)buf; // !!! FIXME: buffer overflow possible.
      int32 val = 0;
      if ((GSteamStats) && (GSteamStats->GetStat(name, &val)))
        writeGetStatI(fd, name, val, true);
      else
        writeGetStatI(fd, name, 0, false);
    } // if
    break;

  case SHIMCMD_SETSTATF:
    if (buflen >= 5) {
      const float val = *((float *)buf);
      buf += sizeof(float);
      const char *name =
          (const char *)buf; // !!! FIXME: buffer overflow possible.
      writeSetStatF(fd, name, val,
                    (GSteamStats) && (GSteamStats->SetStat(name, val)));
    } // if
    break;

  case SHIMCMD_GETSTATF:
    if (buflen) {
      const char *name =
          (const char *)buf; // !!! FIXME: buffer overflow possible.
      float val = 0;
      if ((GSteamStats) && (GSteamStats->GetStat(name, &val)))
        writeGetStatF(fd, name, val, true);
      else
        writeGetStatF(fd, name, 0.0f, false);
    } // if
    break;

  case SHIMCMD_GETPERSONANAME:
    dbgpipe("Parent sending SHIMEVENT_GETPERSONANAME.\n");
    writeString(fd, SHIMEVENT_GETPERSONANAME, GSteamFriends->GetPersonaName());
    break;

  case SHIMCMD_GETCURRENTGAMELANGUAGE:
    dbgpipe("Parent sending SHIMEVENT_GETCURRENTGAMELANGUAGE.\n");
    writeString(fd, SHIMEVENT_GETCURRENTGAMELANGUAGE,
                GSteamApps->GetCurrentGameLanguage());
    break;
  } // switch

  return true; // keep going.
} // processCommand

static void processCommands(PipeType pipeParentRead, PipeType pipeParentWrite) {
  bool quit = false;
  uint8 buf[256];
  int br;

  dbgpipe("***START processCommands.\n");

  // this read blocks.
  while (!quit && ((br = readPipe(pipeParentRead, buf, sizeof(buf))) > 0)) {
    dbgpipe("***IN processCommands readPipe found something.\n");
    while (br > 0) {
      const int cmdlen = (int)buf[0];
      if ((br - 1) >= cmdlen) {
        dbgpipe("***IN processCommands processCommand.\n");
        if (!processCommand(buf + 1, cmdlen, pipeParentWrite)) {
          quit = true;
          break;
        } // if

        br -= cmdlen + 1;
        if (br > 0)
          memmove(buf, buf + cmdlen + 1, br);
      }    // if
      else // get more data.
      {
        dbgpipe("***IN processCommands readPipe#2....\n");
        const int morebr = readPipe(pipeParentRead, buf + br, sizeof(buf) - br);
        if (morebr <= 0) {
          quit = true; // uhoh.
          break;
        } // if
        br += morebr;
      } // else
    }   // while
  }     // while
  dbgpipe("***END processCommands.\n");
} // processCommands

static bool setEnvironmentVars(PipeType pipeChildRead,
                               PipeType pipeChildWrite) {
  char buf[64];
  snprintf(buf, sizeof(buf), LLUFMT, (unsigned long long)pipeChildRead);
  if (!setEnvVar("STEAMSHIM_READHANDLE", buf))
    return false;

  snprintf(buf, sizeof(buf), LLUFMT, (unsigned long long)pipeChildWrite);
  if (!setEnvVar("STEAMSHIM_WRITEHANDLE", buf))
    return false;

  return true;
} // setEnvironmentVars

static bool initSteamworks(PipeType fd) {
  dbgpipe("***IN initSteamworks Start\n");

  // this can fail for many reasons:
  //  - you forgot a steam_appid.txt in the current working directory.
  //  - you don't have Steam running
  //  - you don't own the game listed in steam_appid.txt
  if (!SteamAPI_Init())
    return 0;

  GSteamStats = SteamUserStats();
  GSteamUtils = SteamUtils();
  GSteamUser = SteamUser();
  GSteamFriends = SteamFriends();
  GSteamApps = SteamApps();

  GAppID = GSteamUtils ? GSteamUtils->GetAppID() : 0;
  GUserID = GSteamUser ? GSteamUser->GetSteamID().ConvertToUint64() : 0;

  dbgpipe("***IN initSteamworks GAppID = %d, GUserID = %lld\n", GAppID,
          GUserID);

  GSteamBridge = new SteamBridge(fd);

  return 1;
} // initSteamworks

static void deinitSteamworks(void) {
  dbgpipe("***IN deinitSteamworks GAppID = %d, GUserID = %lld\n", GAppID,
          GUserID);

  SteamAPI_Shutdown();
  delete GSteamBridge;
  GSteamBridge = NULL;
  GSteamStats = NULL;
  GSteamUtils = NULL;
  GSteamUser = NULL;
} // deinitSteamworks

static int mainline(void) {
  PipeType pipeParentRead = NULLPIPE;
  PipeType pipeParentWrite = NULLPIPE;
  PipeType pipeChildRead = NULLPIPE;
  PipeType pipeChildWrite = NULLPIPE;
  ProcessType childPid;

  printf("#2 =============> Megaglest Parent starting mainline.\n");

#ifdef STEAMSHIM_DEBUG
  printf("=============> Megaglest Parent starting mainline.\n");

#endif
  dbgpipe("Parent starting mainline.\n");

  if (!createPipes(&pipeParentRead, &pipeParentWrite, &pipeChildRead,
                   &pipeChildWrite))
    fail("Failed to create application pipes");
  else if (!initSteamworks(pipeParentWrite))
    fail("Failed to initialize Steamworks");
  else if (!setEnvironmentVars(pipeChildRead, pipeChildWrite))
    fail("Failed to set environment variables");
  else if (!launchChild(&childPid))
    fail("Failed to launch application");

  dbgpipe("Parent ending mainline\n");

  // Close the ends of the pipes that the child will use; we don't need them.
  closePipe(pipeChildRead);
  closePipe(pipeChildWrite);
  pipeChildRead = pipeChildWrite = NULLPIPE;

  dbgpipe("Parent in command processing loop.\n");

  // Now, we block for instructions until the pipe fails (child closed it or
  //  terminated/crashed).
  processCommands(pipeParentRead, pipeParentWrite);

  dbgpipe("Parent shutting down.\n");

  // Close our ends of the pipes.
  writeBye(pipeParentWrite);
  closePipe(pipeParentRead);
  closePipe(pipeParentWrite);

  deinitSteamworks();

  dbgpipe("Parent waiting on child process.\n");

  // Wait for the child to terminate, close the child process handles.
  const int retval = closeProcess(&childPid);

  dbgpipe("Parent exiting mainline (child exit code %d).\n", retval);

  return retval;
} // mainline

// end of steamshim_parent.cpp ...
