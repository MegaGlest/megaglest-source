#ifndef _GLEST_MAPEDITOR_MAP_H_
#define _GLEST_MAPEDITOR_MAP_H_

#include "util.h"
#include "types.h"
#include "random.h"

using Shared::Platform::int8;
using Shared::Platform::int32;
using Shared::Platform::float32;
using Shared::Util::Random;

struct MapFileHeader{
	int32 version;
	int32 maxPlayers;
	int32 width;
	int32 height;
	int32 altFactor;
	int32 waterLevel;
	int8 title[128];
	int8 author[128];
	int8 description[256];
};

namespace Glest{ namespace MapEditor{

// ===============================================
//	class Map
// ===============================================

class Map{
public:
	static const int maxHeight= 20;
	static const int minHeight= 0;

private:
    struct Cell{
        int surface;
        int object;
        int resource;
        float height;
    };
    
    struct StartLocation{
        int x;
        int y;
    }; 

	Random random;
    string title;
    string author;
    string desc;
    string recScn;
    int type;
    int h;
	int w;
    int altFactor;
    int waterLevel;
    Cell **cells;
    int maxPlayers;
    StartLocation *startLocations;
	int refAlt;

public:
    Map();
    ~Map();      
    float getHeight(int x, int y) const;
    int getSurface(int x, int y) const;
    int getObject(int x, int y) const;
    int getResource(int x, int y) const;
    int getStartLocationX(int index) const;
    int getStartLocationY(int index) const;
    int getHeightFactor() const;
    int getWaterLevel() const;
    bool inside(int x, int y);

	void setRefAlt(int x, int y); 
	void setAdvanced(int altFactor, int waterLevel);
    void setTitle(const string &title);
    void setDesc(const string &desc);
    void setAuthor(const string &author);
	
	int getH() const			{return h;}
	int getW() const			{return w;}
	int getMaxPlayers() const	{return maxPlayers;}
	string getTitle() const		{return title;}
	string getDesc() const		{return desc;}
	string getAuthor() const	{return author;}

    void changeHeight(int x, int y, int height, int radius);
	void changeSurface(int x, int y, int surface, int radius);
    void changeObject(int x, int y, int object, int radius);
    void changeResource(int x, int y, int resource, int radius);
    void changeStartLocation(int x, int y, int player);
    
	void flipX();
	void flipY();
	void reset(int w, int h, float alt, int surf);
    void resize(int w, int h, float alt, int surf);
    void resetPlayers(int maxPlayers);
	void randomizeHeights();
	void randomize();
	void switchSurfaces(int surf1, int surf2);

    void loadFromFile(const string &path);
    void saveToFile(const string &path);

public:
	void resetHeights(int height);
	void sinRandomize(int strenght);
	void decalRandomize(int strenght);
	void applyNewHeight(float newHeight, int x, int y, int strenght);
};

}}// end namespace

#endif
