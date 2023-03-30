#ifndef STEAM_H
#define STEAM_H

#include "game_constants.h"
#include <map>
#include <string>

namespace Shared {
namespace Util {
class Properties;
}
} // namespace Shared

namespace Glest {
namespace Game {

struct SteamPrivate;

enum SteamStatName {
  stat_online_wins,
  stat_online_loses,
  stat_online_kills,
  stat_online_kills_enemy,
  stat_online_deaths,
  stat_online_units,
  stat_online_resources_harvested,
  stat_online_quit_before_end,
  stat_online_minutes_played,
  games_played,
  network_games_played
};

enum SteamStatType { stat_int, stat_float, stat_avg };

template <> inline EnumParser<SteamStatName>::EnumParser() {
  enumMap["stat_online_wins"] = stat_online_wins;
  enumMap["stat_online_loses"] = stat_online_loses;
  enumMap["stat_online_kills"] = stat_online_kills;
  enumMap["stat_online_kills_enemy"] = stat_online_kills_enemy;
  enumMap["stat_online_deaths"] = stat_online_deaths;
  enumMap["stat_online_units"] = stat_online_units;
  enumMap["stat_online_resources_harvested"] = stat_online_resources_harvested;
  enumMap["stat_online_quit_before_end"] = stat_online_quit_before_end;
  enumMap["stat_online_minutes_played"] = stat_online_minutes_played;
  enumMap["games_played"] = games_played;
  enumMap["network_games_played"] = network_games_played;
}

enum SteamAchievementName {
  ACH_WIN_ONE_GAME,
  ACH_PLAY_FIFTY_GAMES,
  ACH_PLAY_ONE_HUNDRED_GAMES,
  ACH_PLAY_TWO_HUNDRED_FIFTY_GAMES,
  ACH_PLAY_FIVE_HUNDRED_GAMES,
  ACH_PLAY_OVER_THOUSAND_GAMES,

  ACH_WIN_ONE_GAME_ONLINE,
  ACH_PLAY_FIFTY_GAMES_ONLINE,
  ACH_PLAY_ONE_HUNDRED_GAMES_ONLINE,
  ACH_PLAY_TWO_HUNDRED_FIFTY_GAMES_ONLINE,
  ACH_PLAY_FIVE_HUNDRED_GAMES_ONLINE,
  ACH_PLAY_OVER_THOUSAND_GAMES_ONLINE

};

template <> inline EnumParser<SteamAchievementName>::EnumParser() {
  enumMap["ACH_WIN_ONE_GAME"] = ACH_WIN_ONE_GAME;
  enumMap["ACH_PLAY_FIFTY_GAMES"] = ACH_PLAY_FIFTY_GAMES;
  enumMap["ACH_PLAY_ONE_HUNDRED_GAMES"] = ACH_PLAY_ONE_HUNDRED_GAMES;
  enumMap["ACH_PLAY_TWO_HUNDRED_FIFTY_GAMES"] =
      ACH_PLAY_TWO_HUNDRED_FIFTY_GAMES;
  enumMap["ACH_PLAY_FIVE_HUNDRED_GAMES"] = ACH_PLAY_FIVE_HUNDRED_GAMES;
  enumMap["ACH_PLAY_OVER_THOUSAND_GAMES"] = ACH_PLAY_OVER_THOUSAND_GAMES;

  enumMap["ACH_WIN_ONE_GAME_ONLINE"] = ACH_WIN_ONE_GAME_ONLINE;
  enumMap["ACH_PLAY_FIFTY_GAMES_ONLINE"] = ACH_PLAY_FIFTY_GAMES_ONLINE;
  enumMap["ACH_PLAY_ONE_HUNDRED_GAMES_ONLINE"] =
      ACH_PLAY_ONE_HUNDRED_GAMES_ONLINE;
  enumMap["ACH_PLAY_TWO_HUNDRED_FIFTY_GAMES_ONLINE"] =
      ACH_PLAY_TWO_HUNDRED_FIFTY_GAMES_ONLINE;
  enumMap["ACH_PLAY_FIVE_HUNDRED_GAMES_ONLINE"] =
      ACH_PLAY_FIVE_HUNDRED_GAMES_ONLINE;
  enumMap["ACH_PLAY_OVER_THOUSAND_GAMES_ONLINE"] =
      ACH_PLAY_OVER_THOUSAND_GAMES_ONLINE;
}

//
// This interface describes the methods a callback object must implement
//
class PlayerAchievementsInterface {
public:
  virtual void unlock(const char *name) = 0;
  virtual void lock(const char *name) = 0;
  virtual bool isUnlocked(const char *name) = 0;
  virtual int getStatAsInt(const char *name) const = 0;
  virtual void setStatAsInt(const char *name, int value) = 0;
  virtual void storeStats() const = 0;

  virtual ~PlayerAchievementsInterface() {}
};

class Steam : public PlayerAchievementsInterface {
public:
  void unlock(const char *name);
  void lock(const char *name);
  bool isUnlocked(const char *name);

  static SteamStatType getSteamStatNameType(string value);

  const std::string &userName() const;
  const std::string &lang() const;

  void resetStats(const int bAlsoAchievements) const;
  void storeStats() const;
  int getStatAsInt(const char *name) const;
  double getStatAsDouble(const char *name) const;
  void setStatAsInt(const char *name, int value);
  void setStatAsDouble(const char *name, double value);

  void requestRefreshStats();
  static void setDebugEnabled(bool value);

  Steam();
  ~Steam();

private:
  // friend struct SharedStatePrivate;

  SteamPrivate *p;
  static std::map<std::string, SteamStatType> SteamStatNameTypes;

  static std::map<std::string, SteamStatType> create_map() {
    std::map<std::string, SteamStatType> steamStatNameTypes;
    steamStatNameTypes["stat_online_wins"] = stat_int;
    steamStatNameTypes["stat_online_loses"] = stat_int;
    steamStatNameTypes["stat_online_kills"] = stat_int;
    steamStatNameTypes["stat_online_kills_enemy"] = stat_int;
    steamStatNameTypes["stat_online_deaths"] = stat_int;
    steamStatNameTypes["stat_online_units"] = stat_int;
    steamStatNameTypes["stat_online_resources_harvested"] = stat_int;
    steamStatNameTypes["stat_online_quit_before_end"] = stat_int;
    steamStatNameTypes["stat_online_minutes_played"] = stat_float;
    return steamStatNameTypes;
  }
};

class SteamLocal : public PlayerAchievementsInterface {
public:
  SteamLocal(string file);
  ~SteamLocal();

  void unlock(const char *name);
  void lock(const char *name);
  bool isUnlocked(const char *name);
  int getStatAsInt(const char *name) const;

  void setStatAsInt(const char *name, int value);

  // void save(const string &path);
  void storeStats() const;

private:
  Properties *p;
  string saveFilePlayerLocalStats;
};

} // namespace Game
} // namespace Glest

#endif // STEAM_H
