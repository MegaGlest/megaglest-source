#ifndef _GLEST_MAPEDITOR_RENDERER_H_
#define _GLEST_MAPEDITOR_RENDERER_H_

#include "map.h"

namespace Glest{ namespace MapEditor{

// ===============================================
//	class Renderer
// ===============================================

class Renderer{
public:
	void init(int clientW, int clientH);
	void renderMap(Map *map, int x, int y, int clientW, int clientH, int cellSize);
};

}}// end namespace

#endif
