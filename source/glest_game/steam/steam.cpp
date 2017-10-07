#include "steam.h"

#include <map>
#include <SDL.h>
#include "steamshim_child.h"
#include "platform_common.h"

namespace Glest{ namespace Game{

std::map<std::string,SteamStatType> Steam::SteamStatNameTypes = Steam::create_map();

// Achievements
//static const char *const achievementNames[] = {
//    "X",
//};
//#define NUM_ACHIEVEMENTS (sizeof(achievementNames) / sizeof(achievementNames[0]))

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
	//std::map<std::string, bool> achievements;
	std::map<std::string, double> stats;

	std::string userName;
	std::string lang;

	SteamPrivate() {
		//printf("\ncreating private steam state container\n");

		STEAMSHIM_getPersonaName();
		STEAMSHIM_getCurrentGameLanguage();
		STEAMSHIM_requestStats();
//		for (size_t i = 0; i < NUM_ACHIEVEMENTS; ++i)
//			STEAMSHIM_getAchievement(achievementNames[i]);

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

		Shared::PlatformCommon::Chrono timer;
		timer.start();
		while(!initialized() && timer.getMillis() < 2500) {
			SDL_Delay(100);
			update();
		}
	}

//	void setAchievement(const char *name, bool set)
//	{
//		achievements[name] = set;
//		STEAMSHIM_setAchievement(name, set);
//		STEAMSHIM_storeStats();
//	}
//
//	void updateAchievement(const char *name, bool isSet)
//	{
//		achievements[name] = isSet;
//	}
//
//	bool isAchievementSet(const char *name)
//	{
//		return achievements[name];
//	}

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
	void clearLocalStats() {
		stats.clear();
	}

	const STEAMSHIM_Event * update(STEAMSHIM_EventType *waitForEvent=NULL) {
		const STEAMSHIM_Event *e;
		while ((e = STEAMSHIM_pump()) != 0) {
			/* Handle events */
            switch (e->type)
			{
//			case SHIMEVENT_GETACHIEVEMENT:
//				updateAchievement(e->name, e->ivalue);
//				break;
			case SHIMEVENT_GETPERSONANAME:
				//printf("\nGot Shim event SHIMEVENT_GETPERSONANAME isOk = %d\n",e->okay);
				userName = e->name;
				break;
			case SHIMEVENT_GETCURRENTGAMELANGUAGE:
				//printf("\nGot Shim event SHIMEVENT_GETCURRENTGAMELANGUAGE isOk = %d\n",e->okay);
				lang = steamToIsoLang(e->name);
				break;
			case SHIMEVENT_STATSRECEIVED:
				//printf("\nGot Shim event SHIMEVENT_STATSRECEIVED isOk = %d\n",e->okay);
				break;
			case SHIMEVENT_STATSSTORED:
				//printf("\nGot Shim event SHIMEVENT_STATSSTORED isOk = %d\n",e->okay);
				break;

			//case SHIMEVENT_SETSTATI:
			case SHIMEVENT_GETSTATI:
				//printf("\nGot Shim event SHIMEVENT_GETSTATI for stat [%s] value [%d] isOk = %d\n",e->name,e->ivalue,e->okay);
				if(e->okay) {
					updateStat(e->name, e->ivalue);
				}
				break;
			case SHIMEVENT_GETSTATF:
				//printf("\nGot Shim event SHIMEVENT_GETSTATF for stat [%s] value [%f] isOk = %d\n",e->name,e->fvalue,e->okay);
				if(e->okay) {
					updateStat(e->name, e->fvalue);
				}
				break;

			case SHIMEVENT_SETSTATI:
				//printf("\nGot Shim event SHIMEVENT_SETSTATI for stat [%s] value [%d] isOk = %d\n",e->name,e->ivalue,e->okay);
				break;
			case SHIMEVENT_SETSTATF:
				//printf("\nGot Shim event SHIMEVENT_SETSTATF for stat [%s] value [%f] isOk = %d\n",e->name,e->fvalue,e->okay);
				break;

			default:
				//printf("\nGot Shim event [%d] isOk = %d\n",e->type,e->okay);
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
				&& (int)stats.size() >= EnumParser<SteamStatName>::getCount();
		        //&& achievements.size() == NUM_ACHIEVEMENTS;
	}
};

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
	p->clearLocalStats();
	STEAMSHIM_requestStats();
	STEAMSHIM_EventType statsReceived = SHIMEVENT_STATSRECEIVED;
	p->update(&statsReceived);

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

	Shared::PlatformCommon::Chrono timer;
	timer.start();
	while(!p->initialized() && timer.getMillis() < 2500) {
		SDL_Delay(100);
		p->update();
	}

}

SteamStatType Steam::getSteamStatNameType(string value) {
	return SteamStatNameTypes[value];
}

//void Steam::unlock(const char *name)
//{
//	p->setAchievement(name, true);
//}
//
//void Steam::lock(const char *name)
//{
//	p->setAchievement(name, false);
//}
//
//bool Steam::isUnlocked(const char *name)
//{
//	return p->isAchievementSet(name);
//}


}}//end namespace
