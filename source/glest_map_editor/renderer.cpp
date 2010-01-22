#include "renderer.h"

#include <cassert>

#include "opengl.h"

#include "vec.h"

using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;

namespace Glest{ namespace MapEditor{

// ===============================================
//	class Renderer
// ===============================================

void Renderer::init(int clientW, int clientH){
	assertGl();

	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT, GL_FILL);
	glClearColor(0.5, 0.5, 0.5, 1.0);
	
	assertGl();
}

void Renderer::renderMap(Map *map, int x, int y, int clientW, int clientH, int cellSize){
	float alt; 

	assertGl();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, clientW,0, clientH,1,-1);
	glViewport(0, 0, clientW, clientH);
    glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glPushAttrib(GL_CURRENT_BIT);

	glTranslatef(static_cast<float>(x), static_cast<float>(y), 0.0f);
	glLineWidth(1);
	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(0, 0, 0);

	for (int j=0; j<map->getH(); j++){
		for (int i=0; i<map->getW(); i++){
			if (i*cellSize+x>-cellSize && i*cellSize+x<clientW && clientH-cellSize-j*cellSize+y>-cellSize && clientH-cellSize-j*cellSize+y<clientH){
				
				//surface
				alt= map->getHeight(i, j)/20.f; 
				Vec3f surfColor;
				switch (map->getSurface(i, j)){
					case 1: surfColor= Vec3f(0.0, 0.8f*alt, 0.f); break; 
					case 2: surfColor= Vec3f(0.4f*alt, 0.6f*alt, 0.f); break; 
					case 3: surfColor= Vec3f(0.6f*alt, 0.3f*alt, 0.f); break; 
					case 4: surfColor= Vec3f(0.7f*alt, 0.7f*alt, 0.7f*alt); break; 
					case 5: surfColor= Vec3f(1.0f*alt, 0.f, 0.f); break; 
				}

				glColor3fv(surfColor.ptr());
	    
				glBegin(GL_TRIANGLE_STRIP);
					glVertex2i(i*cellSize, clientH-j*cellSize-cellSize);
					glVertex2i(i*cellSize, clientH-j*cellSize);
					glVertex2i(i*cellSize+cellSize, clientH-j*cellSize-cellSize);
					glVertex2i(i*cellSize+cellSize, clientH-j*cellSize);
				glEnd();

				//objects
				switch(map->getObject(i,j)){
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
					case 10:glColor3f(1.f, 0.2f, 0.8f); break;
				}

				if(map->getObject(i, j)!=0){
					glPointSize(cellSize/2.f);
					glBegin(GL_POINTS);
						glVertex2i(i*cellSize+cellSize/2, clientH-j*cellSize-cellSize/2);
					glEnd();
				}

				bool found= false;

				//height lines
				if(!found){
					glColor3fv((surfColor*0.5f).ptr());
					//left
					if(i>0 && map->getHeight(i-1, j)>map->getHeight(i, j)){
						glBegin(GL_LINES);
							glVertex2i(i*cellSize, clientH-(j+1)*cellSize);
							glVertex2i(i*cellSize, clientH-j*cellSize);
						glEnd();
					}
					//down
					if(j>0 && map->getHeight(i, j-1)>map->getHeight(i, j)){
						glBegin(GL_LINES);
							glVertex2i(i*cellSize, clientH-j*cellSize);
							glVertex2i((i+1)*cellSize, clientH-j*cellSize);
						glEnd();
					}
					
					glColor3fv((surfColor*2.f).ptr());
					//left
					if(i>0 && map->getHeight(i-1, j)<map->getHeight(i, j)){
						glBegin(GL_LINES);
							glVertex2i(i*cellSize, clientH-(j+1)*cellSize);
							glVertex2i(i*cellSize, clientH-j*cellSize);
						glEnd();
					}					
					if(j>0 && map->getHeight(i, j-1)<map->getHeight(i, j)){
						glBegin(GL_LINES);
							glVertex2i(i*cellSize, clientH-j*cellSize);
							glVertex2i((i+1)*cellSize, clientH-j*cellSize);
						glEnd();
					}
				}
	    
				//resources
				switch(map->getResource(i,j)){
					case 1: glColor3f(1.f, 1.f, 0.f); break;
					case 2: glColor3f(0.5f, 0.5f, 0.5f); break;
					case 3: glColor3f(1.f, 0.f, 0.f); break;
					case 4: glColor3f(0.f, 0.f, 1.f); break;
					case 5: glColor3f(0.5f, 0.5f, 1.f); break;
				}

				if (map->getResource(i,j)!=0){ 
				glBegin(GL_LINES);
					glVertex2i(i*cellSize, clientH-j*cellSize-cellSize);
					glVertex2i(i*cellSize+cellSize, clientH-j*cellSize);
					glVertex2i(i*cellSize, clientH-j*cellSize);
					glVertex2i(i*cellSize+cellSize, clientH-j*cellSize-cellSize);
				glEnd();
				}
			}
		}
	}

	//start locations
	glLineWidth(3);
	for (int i=0; i<map->getMaxPlayers(); i++){
		switch(i){
		case 0: glColor3f(1.f, 1.f, 0.f); break;
		case 1: glColor3f(0.5f, 0.5f, 0.5f); break;
		case 2: glColor3f(1.f, 0.f, 0.f); break;
		case 3: glColor3f(0.f, 0.f, 1.f); break;
		}
		glBegin(GL_LINES);
				glVertex2i((map->getStartLocationX(i)-1)*cellSize, clientH- (map->getStartLocationY(i)-1)*cellSize);
				glVertex2i((map->getStartLocationX(i)+1)*cellSize+cellSize, clientH- (map->getStartLocationY(i)+1)*cellSize-cellSize);
				glVertex2i((map->getStartLocationX(i)-1)*cellSize, clientH- (map->getStartLocationY(i)+1)*cellSize-cellSize);
				glVertex2i((map->getStartLocationX(i)+1)*cellSize+cellSize, clientH- (map->getStartLocationY(i)-1)*cellSize);
		glEnd();         
	}

	glPopMatrix();
	glPopAttrib();

	assertGl();
}

}}// end namespace
