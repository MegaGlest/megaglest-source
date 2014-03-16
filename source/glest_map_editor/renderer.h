#ifndef RENDERER_H
#define RENDERER_H

#include <QGraphicsScene>
#include <QAction>
#include "map_preview.h"
#include <string>
#include <vector>

using namespace std;

using namespace Shared::Map;

class RenderTile;

class Renderer: public QObject{
	Q_OBJECT
	public:
		Renderer(/*MainWindow *win*/);
		~Renderer();
		void reduceHeight(int height);
		void changeSurface(int surface);
		QGraphicsScene *getScene() const;
		RenderTile* at(int column, int row) const;
		void recalculateAll();
		void updateMap();
		int getHeight() const;
		int getWidth() const;
		void open(string path);
		MapPreview *getMap() const;
		//MainWindow *getWin() const;
		void newMap();
		int getRadius() const;
	public slots:
		void setRadius(QAction *radius);
	private:
		void removeTiles();
		void createTiles();
		QGraphicsScene *scene;
		RenderTile*** tiles;
		MapPreview *map;
		//MainWindow *win;
		int height;
		int width;
		vector<MapPreview*> history;
		int historyPos;
		void addHistory();
		void redo();
		void undo();
		int radius;
};

#endif
