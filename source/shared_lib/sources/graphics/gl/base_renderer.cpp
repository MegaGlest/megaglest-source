// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2010 Mark Vejvoda
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================


#include "base_renderer.h"
#include <cassert>
#include "opengl.h"
#include "vec.h"

using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;

namespace Shared { namespace Graphics {

// ===============================================
// class Renderer
// ===============================================

void BaseRenderer::initMapSurface(int clientW, int clientH) {
	assertGl();

	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT, GL_FILL);
	glClearColor(0.5, 0.5, 0.5, 1.0);

	assertGl();
}

void BaseRenderer::renderMap(MapPreview *map, int x, int y,
							 int clientW, int clientH, int cellSize) {
	float alt=0;
	float showWater=0;

	assertGl();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, clientW, 0, clientH, 1, -1);
	glViewport(0, 0, clientW, clientH);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glPushAttrib(GL_CURRENT_BIT);

	glTranslatef(static_cast<float>(x), static_cast<float>(y), 0.0f);
	glLineWidth(1);
	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(0, 0, 0);

	for (int j = 0; j < map->getH(); j++) {
		for (int i = 0; i < map->getW(); i++) {
			if (i * cellSize + x > -cellSize
					&& i * cellSize + x < clientW
					&& clientH - cellSize - j * cellSize + y > -cellSize
					&& clientH - cellSize - j * cellSize + y < clientH) {

				//surface
				alt = map->getHeight(i, j) / 20.f;
				showWater = map->getWaterLevel()/ 20.f - alt;
				showWater = (showWater > 0)? showWater:0;
				Vec3f surfColor;
				switch (map->getSurface(i, j)) {
					case st_Grass: surfColor = Vec3f(0.0, 0.8f * alt, 0.f + showWater); break;
					case st_Secondary_Grass: surfColor = Vec3f(0.4f * alt, 0.6f * alt, 0.f + showWater); break;
					case st_Road: surfColor = Vec3f(0.6f * alt, 0.3f * alt, 0.f + showWater); break;
					case st_Stone: surfColor = Vec3f(0.7f * alt, 0.7f * alt, 0.7f * alt + showWater); break;
					case st_Ground: surfColor = Vec3f(0.7f * alt, 0.5f * alt, 0.3f * alt + showWater); break;
				}

				glColor3fv(surfColor.ptr());

				glBegin(GL_TRIANGLE_STRIP);
				glVertex2i(i * cellSize, clientH - j * cellSize - cellSize);
				glVertex2i(i * cellSize, clientH - j * cellSize);
				glVertex2i(i * cellSize + cellSize, clientH - j * cellSize - cellSize);
				glVertex2i(i * cellSize + cellSize, clientH - j * cellSize);
				glEnd();

				//objects
				switch (map->getObject(i, j)) {
					case 0: glColor3f(0.f, 0.f, 0.f); break;
					case 1: glColor3f(1.f, 0.f, 0.f); break;
					case 2: glColor3f(1.f, 1.f, 1.f); break;
					case 3: glColor3f(0.5f, 0.5f, 1.f); break;
					case 4: glColor3f(0.f, 0.f, 1.f); break;
					case 5: glColor3f(0.5f, 0.5f, 0.5f); break;
					case 6: glColor3f(1.f, 0.8f, 0.5f); break;
					case 7: glColor3f(0.f, 1.f, 1.f); break;
					case 8: glColor3f(0.7f, 0.1f, 0.3f); break;
					case 9: glColor3f(0.5f, 1.f, 0.1f); break;
					case 10: glColor3f(1.f, 0.2f, 0.8f); break;
				}

				if (map->getObject(i, j) != 0) {
					glPointSize(cellSize / 2.f);
					glBegin(GL_POINTS);
					glVertex2i(i * cellSize + cellSize / 2, clientH - j * cellSize - cellSize / 2);
					glEnd();
				}

//				bool found = false;

				//height lines
//				if (!found) {
					glColor3fv((surfColor*0.5f).ptr());
					//left
					if (i > 0 && map->getHeight(i - 1, j) > map->getHeight(i, j)) {
						glBegin(GL_LINES);
						glVertex2i(i * cellSize, clientH - (j + 1) * cellSize);
						glVertex2i(i * cellSize, clientH - j * cellSize);
						glEnd();
					}
					//down
					if (j > 0 && map->getHeight(i, j - 1) > map->getHeight(i, j)) {
						glBegin(GL_LINES);
						glVertex2i(i * cellSize, clientH - j * cellSize);
						glVertex2i((i + 1) * cellSize, clientH - j * cellSize);
						glEnd();
					}

					glColor3fv((surfColor*2.f).ptr());
					//left
					if (i > 0 && map->getHeight(i - 1, j) < map->getHeight(i, j)) {
						glBegin(GL_LINES);
						glVertex2i(i * cellSize, clientH - (j + 1) * cellSize);
						glVertex2i(i * cellSize, clientH - j * cellSize);
						glEnd();
					}
					if (j > 0 && map->getHeight(i, j - 1) < map->getHeight(i, j)) {
						glBegin(GL_LINES);
						glVertex2i(i * cellSize, clientH - j * cellSize);
						glVertex2i((i + 1) * cellSize, clientH - j * cellSize);
						glEnd();
					}
//				}

				//resources
				switch (map->getResource(i, j)) {
					case 1: glColor3f(1.f, 1.f, 0.f); break;
					case 2: glColor3f(0.5f, 0.5f, 0.5f); break;
					case 3: glColor3f(1.f, 0.f, 0.f); break;
					case 4: glColor3f(0.f, 0.f, 1.f); break;
					case 5: glColor3f(0.5f, 0.5f, 1.f); break;
				}

				if (map->getResource(i, j) != 0) {
					glBegin(GL_LINES);
					glVertex2i(i * cellSize, clientH - j * cellSize - cellSize);
					glVertex2i(i * cellSize + cellSize, clientH - j * cellSize);
					glVertex2i(i * cellSize, clientH - j * cellSize);
					glVertex2i(i * cellSize + cellSize, clientH - j * cellSize - cellSize);
					glEnd();
				}
			}
		}
	}

	//start locations
	glLineWidth(3);
	for (int i = 0; i < map->getMaxFactions(); i++) {
		switch (i) {
			case 0: glColor3f(1.f, 0.f, 0.f); break;
			case 1: glColor3f(0.f, 0.f, 1.f); break;
			case 2: glColor3f(0.f, 1.f, 0.f); break;
			case 3: glColor3f(1.f, 1.f, 0.f); break;
			case 4: glColor3f(1.f, 1.f, 1.f); break;
			case 5: glColor3f(0.f, 1.f, 0.8f); break;
			case 6: glColor3f(1.f, 0.5f, 0.f); break;
			case 7: glColor3f(1.f, 0.5f, 1.f); break;
   		}
		glBegin(GL_LINES);
		glVertex2i((map->getStartLocationX(i) - 1) * cellSize, clientH - (map->getStartLocationY(i) - 1) * cellSize);
		glVertex2i((map->getStartLocationX(i) + 1) * cellSize + cellSize, clientH - (map->getStartLocationY(i) + 1) * cellSize - cellSize);
		glVertex2i((map->getStartLocationX(i) - 1) * cellSize, clientH - (map->getStartLocationY(i) + 1) * cellSize - cellSize);
		glVertex2i((map->getStartLocationX(i) + 1) * cellSize + cellSize, clientH - (map->getStartLocationY(i) - 1) * cellSize);
		glEnd();
	}

	glPopMatrix();
	glPopAttrib();

	assertGl();
}

void BaseRenderer::renderMapPreview( const MapPreview *map, int x, int y,
							 	 	 bool renderAll, int screenX, int screenY) {
	float alt=0;
	float showWater=0;
	int renderMapHeight=128;
	int renderMapWidth=128;
	float clientW=256;
	float clientH=256;
	float cellSize=2;
	float playerCrossSize=2;

	// stretch small maps to 128x128
	if(map->getW() < map->getH()) {
		cellSize = cellSize * renderMapHeight / map->getH();
	}
	else {
		cellSize = cellSize * renderMapWidth / map->getW();
	}

	assertGl();

	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);

	glMatrixMode(GL_PROJECTION);

	glLoadIdentity();

	glViewport(screenX, screenY, renderMapWidth,renderMapHeight);
	//glViewport(1, 1, renderMapWidth*4,renderMapHeight*4);
	glOrtho(0, clientW, 0, clientH, -1, 1);
	//glOrtho(-clientW, clientW, -clientH, clientH, -1000, 1000);

	//glMatrixMode(GL_MODELVIEW);

	glPushMatrix();

	glPushAttrib(GL_CURRENT_BIT);

	//glTranslatef(static_cast<float>(x), static_cast<float>(y), 0.0f);
	glLineWidth(1);
	//glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(0, 0, 0);

	for (int j = 0; j < map->getH(); j++) {
		for (int i = 0; i < map->getW(); i++) {
		//surface
		alt = map->getHeight(i, j) / 20.f;
		showWater = map->getWaterLevel()/ 20.f - alt;
		showWater = (showWater > 0)? showWater:0;
		Vec3f surfColor;
		switch (map->getSurface(i, j)) {
			case st_Grass: surfColor = Vec3f(0.0, 0.8f * alt, 0.f + showWater); break;
			case st_Secondary_Grass: surfColor = Vec3f(0.4f * alt, 0.6f * alt, 0.f + showWater); break;
			case st_Road: surfColor = Vec3f(0.6f * alt, 0.3f * alt, 0.f + showWater); break;
			case st_Stone: surfColor = Vec3f(0.7f * alt, 0.7f * alt, 0.7f * alt + showWater); break;
			case st_Ground: surfColor = Vec3f(0.7f * alt, 0.5f * alt, 0.3f * alt + showWater); break;
		}

		glColor3fv(surfColor.ptr());

		glBegin(GL_TRIANGLE_STRIP);
		glVertex2f(i * cellSize, clientH - j * cellSize - cellSize);
		glVertex2f(i * cellSize, clientH - j * cellSize);
		glVertex2f(i * cellSize + cellSize, clientH - j * cellSize - cellSize);
		glVertex2f(i * cellSize + cellSize, clientH - j * cellSize);
		glEnd();

		//objects
		if(renderAll == true) {
			switch (map->getObject(i, j)) {
				case 0: glColor3f(0.f, 0.f, 0.f); break;
				case 1: glColor3f(1.f, 0.f, 0.f); break;
				case 2: glColor3f(1.f, 1.f, 1.f); break;
				case 3: glColor3f(0.5f, 0.5f, 1.f); break;
				case 4: glColor3f(0.f, 0.f, 1.f); break;
				case 5: glColor3f(0.5f, 0.5f, 0.5f); break;
				case 6: glColor3f(1.f, 0.8f, 0.5f); break;
				case 7: glColor3f(0.f, 1.f, 1.f); break;
				case 8: glColor3f(0.7f, 0.1f, 0.3f); break;
				case 9: glColor3f(0.5f, 1.f, 0.1f); break;
				case 10: glColor3f(1.f, 0.2f, 0.8f); break;// we don't render unvisible blocking objects
			}

			if ( renderAll && (map->getObject(i, j) != 0) && (map->getObject(i, j) != 10) ){
				glPointSize(cellSize / 2.f);
				glBegin(GL_POINTS);
				glVertex2i(i * cellSize + cellSize / 2, clientH - j * cellSize - cellSize / 2);
				glEnd();
			}
		}

//		bool found = false;

		//height lines
//		if (!found) {

			//left
			if (i > 0 && map->getHeight(i - 1, j) > map->getHeight(i, j)) {
				glColor3fv((surfColor*0.5f).ptr());
				glBegin(GL_LINES);
				glVertex2f(i * cellSize, clientH - (j + 1) * cellSize);
				glVertex2f(i * cellSize, clientH - j * cellSize);
				glEnd();
			}
			//down
			if (j > 0 && map->getHeight(i, j - 1) > map->getHeight(i, j)) {
				glColor3fv((surfColor*0.5f).ptr());
				glBegin(GL_LINES);
				glVertex2f(i * cellSize, clientH - j * cellSize);
				glVertex2f((i + 1) * cellSize, clientH - j * cellSize);
				glEnd();
			}


			//left
			if (i > 0 && map->getHeight(i - 1, j) < map->getHeight(i, j)) {
				glColor3fv((surfColor*2.f).ptr());
				glBegin(GL_LINES);
				glVertex2f(i * cellSize, clientH - (j + 1)  * cellSize);
				glVertex2f(i * cellSize, clientH - j * cellSize);
				glEnd();
			}
			if (j > 0 && map->getHeight(i, j - 1) < map->getHeight(i, j)) {
				glColor3fv((surfColor*2.f).ptr());
				glBegin(GL_LINES);
				glVertex2f(i * cellSize, clientH - j * cellSize);
				glVertex2f((i + 1) * cellSize, clientH - j * cellSize);
				glEnd();
			}
//				}

			//resources
			if(renderAll == true) {
				switch (map->getResource(i, j)) {
					case 1: glColor3f(1.f, 1.f, 0.f); break;
					case 2: glColor3f(0.5f, 0.5f, 0.5f); break;
					case 3: glColor3f(1.f, 0.f, 0.f); break;
					case 4: glColor3f(0.f, 0.f, 1.f); break;
					case 5: glColor3f(0.5f, 0.5f, 1.f); break;
				}

				if (renderAll && map->getResource(i, j) != 0) {
					glBegin(GL_LINES);
					glVertex2f(i * cellSize, clientH - j * cellSize - cellSize);
					glVertex2f(i * cellSize + cellSize, clientH - j * cellSize);
					glVertex2f(i * cellSize, clientH - j * cellSize);
					glVertex2f(i * cellSize + cellSize, clientH - j * cellSize - cellSize);
					glEnd();
				}
			}
		}

	}


	//start locations
	glLineWidth(3);

	// force playerCrossSize to be at least of size 4
	if(cellSize<4)
		playerCrossSize=4;
	else
		playerCrossSize=cellSize;

	for (int i = 0; i < map->getMaxFactions(); i++) {
		switch (i) {
			case 0: glColor3f(1.f, 0.f, 0.f); break;
			case 1: glColor3f(0.f, 0.f, 1.f); break;
			case 2: glColor3f(0.f, 1.f, 0.f); break;
			case 3: glColor3f(1.f, 1.f, 0.f); break;
			case 4: glColor3f(1.f, 1.f, 1.f); break;
			case 5: glColor3f(0.f, 1.f, 0.8f); break;
			case 6: glColor3f(1.f, 0.5f, 0.f); break;
			case 7: glColor3f(1.f, 0.5f, 1.f); break;
   		}
		glBegin(GL_LINES);
		glVertex2f((map->getStartLocationX(i) - 1) * cellSize, clientH - (map->getStartLocationY(i) - 1) * cellSize);
		glVertex2f((map->getStartLocationX(i) + 1) * cellSize + playerCrossSize, clientH - (map->getStartLocationY(i) + 1) * cellSize - playerCrossSize);
		glVertex2f((map->getStartLocationX(i) - 1) * cellSize, clientH - (map->getStartLocationY(i) + 1) * cellSize - playerCrossSize);
		glVertex2f((map->getStartLocationX(i) + 1) * cellSize + playerCrossSize, clientH - (map->getStartLocationY(i) - 1) * cellSize);
		glEnd();
	}

	glPopMatrix();
	glPopAttrib();

	glViewport(0, 0, 0, 0);

	assertGl();
}


}} // end namespace
