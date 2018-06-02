#include "steam.h"

#include <map>
#include <SDL.h>
#include "steamshim_child.h"
#include "platform_common.h"
#include "properties.h"

using Shared::PlatformCommon::fileExists;

namespace Glest{ namespace Game{

std::map<std::string,SteamStatType> Steam::SteamStatNameTypes = Steam::create_map();

// Language map
static inline std::map<std::string, std::string> gen_langToCode()
{
	std::map<std::string, std::string> map;
	map["brazilian"] = "pt_BR";
	map["bulgarian"] = "bg";
	map["czech"] = "cz";
	map["danish"] = "da";
	map["dutch"] = "nl";
	map["english"] = "en";
	map["finnish"] = "fi";
	map["french"] = "fr";
	map["german"] = "de";
	map["greek"] = "el";
	map["hungarian"] = "hu";
	map["italian"] = "it";
	map["japanese"] = "ja";
	map["koreana"] = "ko";
	map["korean"] = "ko";
	map["norwegian"] = "no";
	map["polish"] = "pl";
	map["portuguese"] = "pt";
	map["romanian"] = "ro";
	map["russian"] = "ru";
	map["schinese"] = "zh_CN";
	map["spanish"] = "es";
	map["swedish"] = "sv";
	map["tchinese"] = "zh_TW";
	map["thai"] = "th";
	map["turkish"] = "tr";
	map["ukrainian"] = "uk";
	return map;
}
static const std::map<std::string, std::string> langToCode = gen_langToCode();

static std::string steamToIsoLang(const char *steamLang) {
	//printf("Steam language [%s]\n",steamLang);
	std::map<std::string, std::string>::const_iterator it = langToCode.find(steamLang);
	if (it != langToCode.end()) {
		return it->second;
	}
	return "en";
}

// SteamPrivate
struct SteamPrivate {
	static bool debugEnabled;
	std::map<std::string, bool> achievements;
	std::map<std::string, double> stats;

	std::string userName;
	std::string lang;

	SteamPrivate() {
		if(debugEnabled) printf("\nCreating private steam state container\n");
		STEAMSHIM_getPersonaName();
		STEAMSHIM_getCurrentGameLanguage();
		STEAMSHIM_requestStats();
		STEAMSHIM_EventType statsReceived = SHIMEVENT_STATSRECEIVED;

		Shared::PlatformCommon::Chrono timerStats;
		timerStats.start();
		while(update(&statsReceived) == NULL && timerStats.getMillis() < 2500) {
			SDL_Delay(100);
		}

		refreshAllStats();
	}

	static void setDebugEnabled(bool value) {
		debugEnabled = value;
	}

	void refreshAllStats() {
		achievements.clear();
		stats.clear();

		for(int index = 0; index < EnumParser<SteamStatName>::getCount(); ++index) {
			SteamStatName statName = static_cast<SteamStatName>(index);
			string statNameStr = EnumParser<SteamStatName>::getString(statName);
			SteamStatType statType = Steam::getSteamStatNameType(statNameStr);
			switch(statType) {
				case stat_int:
					STEAMSHIM_getStatI(statNameStr.c_str());
					break;
				case stat_float:
					STEAMSHIM_getStatF(statNameStr.c_str());
					break;
				default:
					break;
			}
		}
		for(int index = 0; index < EnumParser<SteamAchievementName>::getCount(); ++index) {
			SteamAchievementName achName = static_cast<SteamAchievementName>(index);
			string achNameStr = EnumParser<SteamAchievementName>::getString(achName);
			STEAMSHIM_getAchievement(achNameStr.c_str());
		}

		Shared::PlatformCommon::Chrono timer;
		timer.start();
		while(!initialized() && timer.getMillis() < 2500) {
			SDL_Delay(100);
			update();
		}
	}

	void setAchievement(const char *name, bool set)	{
		achievements[name] = set;
		STEAMSHIM_setAchievement(name, set);
	}

	void updateAchievement(const char *name, bool isSet) {
		achievements[name] = isSet;
	}

	bool isAchievementSet(const char *name) {
		return achievements[name];
	}

	void updateStat(const char *name, double value) {
		stats[name] = value;
	}

	int getStatAsInt(const char *name) const {
		std::map<std::string, double>::const_iterator iterFind = stats.find(name);
		if(iterFind != stats.end()) {
			return iterFind->second;
		}
		return 0;
	}
	double getStatAsDouble(const char *name) const {
		std::map<std::string, double>::const_iterator iterFind = stats.find(name);
		if(iterFind != stats.end()) {
			return iterFind->second;
		}
		return 0;
	}

	void setStatAsInt(const char *name, int value) {
		STEAMSHIM_setStatI(name, value);
		update();
	}
	void setStatAsFloat(const char *name, float value) {
		STEAMSHIM_setStatF(name, value);
		update();
	}
//	void clearLocalStats() {
//		stats.clear();
//	}

	const STEAMSHIM_Event * update(STEAMSHIM_EventType *waitForEvent=NULL) {
		const STEAMSHIM_Event *e;
		while ((e = STEAMSHIM_pump()) != 0) {
			// Handle events
            switch (e->type)
			{
			case SHIMEVENT_GETACHIEVEMENT:
				if(debugEnabled) printf("\nGot Shim event SHIMEVENT_GETACHIEVEMENT name [%s] value [%d] isOk = %d\n",e->name,e->ivalue,e->okay);
				if(e->okay) {
					updateAchievement(e->name, e->ivalue);
				}
				break;
			case SHIMEVENT_SETACHIEVEMENT:
				if(debugEnabled) printf("\nGot Shim event SHIMEVENT_SETACHIEVEMENT for name [%s] value [%d] isOk = %d\n",e->name,e->ivalue,e->okay);
				break;
			case SHIMEVENT_GETPERSONANAME:
				if(debugEnabled) printf("\nGot Shim event SHIMEVENT_GETPERSONANAME isOk = %d value [%s]\n",e->okay,e->name);
				userName = e->name;
				break;
			case SHIMEVENT_GETCURRENTGAMELANGUAGE:
				if(debugEnabled) printf("\nGot Shim event SHIMEVENT_GETCURRENTGAMELANGUAGE isOk = %d value [%s]\n",e->okay,e->name);
				lang = steamToIsoLang(e->name);
				break;
			case SHIMEVENT_STATSRECEIVED:
				if(debugEnabled) printf("\nGot Shim event SHIMEVENT_STATSRECEIVED isOk = %d\n",e->okay);
				break;
			case SHIMEVENT_STATSSTORED:
				if(debugEnabled) printf("\nGot Shim event SHIMEVENT_STATSSTORED isOk = %d\n",e->okay);
				break;
			case SHIMEVENT_GETSTATI:
				if(debugEnabled) printf("\nGot Shim event SHIMEVENT_GETSTATI for stat [%s] value [%d] isOk = %d\n",e->name,e->ivalue,e->okay);
				if(e->okay) {
					updateStat(e->name, e->ivalue);
				}
				break;
			case SHIMEVENT_GETSTATF:
				if(debugEnabled) printf("\nGot Shim event SHIMEVENT_GETSTATF for stat [%s] value [%f] isOk = %d\n",e->name,e->fvalue,e->okay);
				if(e->okay) {
					updateStat(e->name, e->fvalue);
				}
				break;
			case SHIMEVENT_SETSTATI:
				if(debugEnabled) printf("\nGot Shim event SHIMEVENT_SETSTATI for stat [%s] value [%d] isOk = %d\n",e->name,e->ivalue,e->okay);
				break;
			case SHIMEVENT_SETSTATF:
				if(debugEnabled) printf("\nGot Shim event SHIMEVENT_SETSTATF for stat [%s] value [%f] isOk = %d\n",e->name,e->fvalue,e->okay);
				break;
			default:
				if(debugEnabled) printf("\nGot Shim event [%d] isOk = %d\n",e->type,e->okay);
				break;
			}
            if(waitForEvent != NULL && *waitForEvent == e->type) {
            	return e;
            }
        }
		return NULL;
	}

	bool initialized() {
		return !userName.empty()
		        && !lang.empty()
				&& (int)stats.size() >= EnumParser<SteamStatName>::getCount()
		        && (int)achievements.size() >= EnumParser<SteamAchievementName>::getCount();
	}
};

bool SteamPrivate::debugEnabled = false;

/* Steam */
Steam::Steam() : p(new SteamPrivate()) {
}

Steam::~Steam() {
	delete p;
}

const std::string &Steam::userName() const {
	return p->userName;
}

const std::string &Steam::lang() const {
	return p->lang;
}

void Steam::resetStats(const int bAlsoAchievements) const {
	STEAMSHIM_resetStats(false);
	p->update();
}

void Steam::storeStats() const {
	STEAMSHIM_storeStats();
	STEAMSHIM_EventType statsStored = SHIMEVENT_STATSSTORED;

	Shared::PlatformCommon::Chrono timer;
	timer.start();
	while(timer.getMillis() < 2500) {
		SDL_Delay(100);
		const STEAMSHIM_Event *evt = p->update(&statsStored);
		if(evt != NULL && evt->type == statsStored) {
			break;
		}
	}
}

int Steam::getStatAsInt(const char *name) const {
	return p->getStatAsInt(name);
}

double Steam::getStatAsDouble(const char *name) const {
	return p->getStatAsDouble(name);
}

void Steam::setStatAsInt(const char *name, int value) {
	p->setStatAsInt(name, value);
}
void Steam::setStatAsDouble(const char *name, double value) {
	p->setStatAsFloat(name, value);
}

void Steam::requestRefreshStats() {
	STEAMSHIM_requestStats();
	STEAMSHIM_EventType statsReceived = SHIMEVENT_STATSRECEIVED;
	Shared::PlatformCommon::Chrono timerStats;
	timerStats.start();
	while(p->update(&statsReceived) == NULL && timerStats.getMillis() < 2500) {
		SDL_Delay(100);
	}
	p->refreshAllStats();
}

SteamStatType Steam::getSteamStatNameType(string value) {
	return SteamStatNameTypes[value];
}

void Steam::unlock(const char *name) {
	p->setAchievement(name, true);
}

void Steam::lock(const char *name) {
	p->setAchievement(name, false);
}

bool Steam::isUnlocked(const char *name) {
	return p->isAchievementSet(name);
}

void Steam::setDebugEnabled(bool value) {
	SteamPrivate::setDebugEnabled(value);
}

/* SteamLocal */
SteamLocal::SteamLocal(string file) : p(new Properties()) {
	saveFilePlayerLocalStats = file;
	if(fileExists(saveFilePlayerLocalStats)) {
		p->load(saveFilePlayerLocalStats);
	}
}

SteamLocal::~SteamLocal() {
	delete p;
}

void SteamLocal::unlock(const char *name) {
	//p->setAchievement(name, true);
	p->setBool(name,true);
}

void SteamLocal::lock(const char *name) {
	//p->setAchievement(name, false);
}

bool SteamLocal::isUnlocked(const char *name) {
	//return p->isAchievementSet(name);
	return p->getBool(name,"false");
}

int SteamLocal::getStatAsInt(const char *name) const {
	return p->getInt(name,"0");
}

void SteamLocal::setStatAsInt(const char *name, int value) {
	p->setInt(name,value);
}

//void SteamLocal::save(const string &path) {
//	p->save(path);
//}

void SteamLocal::storeStats() const {
	p->save(saveFilePlayerLocalStats);
}

}}//end namespace
