#include "steam.h"

#include <map>
#include <SDL.h>
#include "steamshim_child.h"

/* Achievements */
//static const char *const achievementNames[] = {
//    "X",
//};
//#define NUM_ACHIEVEMENTS (sizeof(achievementNames) / sizeof(achievementNames[0]))

/* Language map */
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

static std::string steamToIsoLang(const char *steamLang)
{
	//printf("Steam language [%s]\n",steamLang);
	std::map<std::string, std::string>::const_iterator it = langToCode.find(steamLang);
	if (it != langToCode.end())
		return it->second;
	return "en";
}

/* SteamPrivate */
struct SteamPrivate
{
	//std::map<std::string, bool> achievements;

	std::string userName;
	std::string lang;

	SteamPrivate()
	{
		STEAMSHIM_getPersonaName();
		STEAMSHIM_getCurrentGameLanguage();
//		for (size_t i = 0; i < NUM_ACHIEVEMENTS; ++i)
//			STEAMSHIM_getAchievement(achievementNames[i]);
		while (!initialized())
		{
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

	void update()
	{
		const STEAMSHIM_Event *e;
		while ((e = STEAMSHIM_pump()) != 0)
		{
			/* Handle events */
            switch (e->type)
			{
//			case SHIMEVENT_GETACHIEVEMENT:
//				updateAchievement(e->name, e->ivalue);
//				break;
			case SHIMEVENT_GETPERSONANAME:
				userName = e->name;
				break;
			case SHIMEVENT_GETCURRENTGAMELANGUAGE:
				lang = steamToIsoLang(e->name);
			default:
				break;
			}
        }
	}

	bool initialized()
	{
		return !userName.empty()
		        && !lang.empty();
		        //&& achievements.size() == NUM_ACHIEVEMENTS;
	}
};

/* Steam */
Steam::Steam()
    : p(new SteamPrivate())
{
}

Steam::~Steam()
{
	delete p;
}

const std::string &Steam::userName() const
{
	return p->userName;
}

const std::string &Steam::lang() const
{
	return p->lang;
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
