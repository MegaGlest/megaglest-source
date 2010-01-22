#ifndef _GLEST_MAPEDITOR_PROGRAM_H_
#define _GLEST_MAPEDITOR_PROGRAM_H_

#include "map.h"
#include "renderer.h"

namespace Glest{ namespace MapEditor{

class MainWindow;

// ===============================================
//	class Program
// ===============================================

class Program{
private:
	Renderer renderer;
    int ofsetX, ofsetY;
    int cellSize;
    Map *map;

public:
    Program(int w, int h);
    ~Program();

	//map cell change
	void changeMapHeight(int x, int y, int Height, int radius);
    void changeMapSurface(int x, int y, int surface, int radius);
    void changeMapObject(int x, int y, int object, int radius);
    void changeMapResource(int x, int y, int resource, int radius);
    void changeStartLocation(int x, int y, int player);

	//map ops
    void reset(int w, int h, int alt, int surf);
    void resize(int w, int h, int alt, int surf);
    void resetPlayers(int maxPlayers);
	void setRefAlt(int x, int y);
	void flipX();
	void flipY();
	void randomizeMapHeights();
	void randomizeMap();
	void switchMapSurfaces(int surf1, int surf2);
    void loadMap(const string &path);
    void saveMap(const string &path);

	//map misc
	void setMapTitle(const string &title);
    void setMapDesc(const string &desc);
    void setMapAuthor(const string &author);
    void setMapAdvanced(int altFactor, int waterLevel);

	//misc
	void renderMap(int w, int h);
	void setOfset(int x, int y);
    void incCellSize(int i);
    void resetOfset();

	const Map *getMap() {return map;}
};

}}// end namespace

#endif
