// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================
#include "intro.h"

#include "main_menu.h"
#include "util.h"
#include "game_util.h"
#include "config.h"
#include "program.h"
#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "metrics.h"
#include "auto_test.h"
#include "util.h"
//#include "glm.h"
//#include "md5util.h"
//#include "Mathlib.h"
#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Graphics;
using namespace	Shared::Xml;

namespace Glest{ namespace Game{

//struct Timer {
//public:
//  Timer ()
//    : current_time (0.0), last_time (0.0) { }
//
//public:
//  void update () {
//    last_time = current_time;
//    current_time = static_cast<double>(SDL_GetTicks ()) / 1000.0;
//  }
//
//  double deltaTime () const {
//    return (current_time - last_time);
//  }
//
//public:
//  double current_time;
//  double last_time;
//
//} animTimer;

// =====================================================
// 	class Text
// =====================================================

Text::Text(const string &text, const Vec2i &pos, int time, Font2D *font, Font3D *font3D) {
	this->text= text;
	this->pos= pos;
	this->time= time;
	this->texture= NULL;
	this->font= font;
	this->font3D = font3D;
}

Text::Text(const Texture2D *texture, const Vec2i &pos, const Vec2i &size, int time) {
	this->pos= pos;
	this->size= size;
	this->time= time;
	this->texture= texture;
	this->font= NULL;
	this->font3D=NULL;
}

// =====================================================
// 	class Intro
// =====================================================

int Intro::introTime	= 50000;
int Intro::appearTime	= 2500;
int Intro::showTime		= 3500;
int Intro::disapearTime	= 2500;

Intro::Intro(Program *program):
	ProgramState(program)
{
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	CoreData &coreData= CoreData::getInstance();
	const Metrics &metrics= Metrics::getInstance();
	int w= metrics.getVirtualW();
	int h= metrics.getVirtualH();
	timer=0;
	mouseX = 0;
	mouseY = 0;
	mouse2d = 0;

	Renderer &renderer= Renderer::getInstance();
	//renderer.init3dListMenu(NULL);
	renderer.initMenu(NULL);
	fade= 0.f;
	anim= 0.f;

	XmlTree xmlTree;
	string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
	xmlTree.load(data_path + "data/core/menu/menu.xml",Properties::getTagReplacementValues());
	const XmlNode *menuNode= xmlTree.getRootNode();
	const XmlNode *introNode= menuNode->getChild("intro");

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	targetCamera= NULL;
	t= 0.f;

	//camera
	const XmlNode *cameraNode= introNode->getChild("camera");

	//position
	const XmlNode *positionNode= cameraNode->getChild("start-position");
    startPosition.x= positionNode->getAttribute("x")->getFloatValue();
	startPosition.y= positionNode->getAttribute("y")->getFloatValue();
	startPosition.z= positionNode->getAttribute("z")->getFloatValue();
	camera.setPosition(startPosition);

	//rotation
	const XmlNode *rotationNode= cameraNode->getChild("start-rotation");
	Vec3f startRotation;
    startRotation.x= rotationNode->getAttribute("x")->getFloatValue();
	startRotation.y= rotationNode->getAttribute("y")->getFloatValue();
	startRotation.z= rotationNode->getAttribute("z")->getFloatValue();
	camera.setOrientation(Quaternion(EulerAngles(
		degToRad(startRotation.x),
		degToRad(startRotation.y),
		degToRad(startRotation.z))));

	// intro info
	const XmlNode *introTimeNode= introNode->getChild("intro-time");
	Intro::introTime = introTimeNode->getAttribute("value")->getIntValue();
	const XmlNode *appearTimeNode= introNode->getChild("appear-time");
	Intro::appearTime = appearTimeNode->getAttribute("value")->getIntValue();
	const XmlNode *showTimeNode= introNode->getChild("show-time");
	Intro::showTime = showTimeNode->getAttribute("value")->getIntValue();
	const XmlNode *disappearTimeNode= introNode->getChild("disappear-time");
	Intro::disapearTime = disappearTimeNode->getAttribute("value")->getIntValue();
	const XmlNode *showIntroPicturesNode= introNode->getChild("show-intro-pictures");
	int showIntroPics = showIntroPicturesNode->getAttribute("value")->getIntValue();
	int showIntroPicsTime = showIntroPicturesNode->getAttribute("time")->getIntValue();
	bool showIntroPicsRandom = showIntroPicturesNode->getAttribute("random")->getBoolValue();

	const XmlNode *showIntroModelsNode= introNode->getChild("show-intro-models");
	bool showIntroModels = showIntroModelsNode->getAttribute("value")->getBoolValue();
	bool showIntroModelsRandom = showIntroModelsNode->getAttribute("random")->getBoolValue();
	modelMinAnimSpeed = showIntroModelsNode->getAttribute("min-anim-speed")->getFloatValue();
	modelMaxAnimSpeed = showIntroModelsNode->getAttribute("max-anim-speed")->getFloatValue();

	//load main model
	modelIndex = 0;
	models.clear();
	if(showIntroModels == true) {
		string introPath = data_path + "data/core/menu/main_model/intro*.g3d";
		vector<string> introModels;
		findAll(introPath, introModels, false, false);
		for(int i = 0; i < introModels.size(); ++i) {
			string logo = introModels[i];
			Model *model= renderer.newModel(rsMenu);
			if(model) {
				model->load(data_path + "data/core/menu/main_model/" + logo);
				models.push_back(model);
				//printf("model [%s]\n",model->getFileName().c_str());
			}
		}

		if(showIntroModelsRandom == true) {
			std::vector<Model *> modelList;

			unsigned int seed = time(NULL);
			srand(seed);
			int failedLookups=0;
			std::map<int,bool> usedIndex;
			for(;modelList.size() < models.size();) {
				int index = rand() % models.size();
				if(usedIndex.find(index) != usedIndex.end()) {
					failedLookups++;
					seed = time(NULL) / failedLookups;
					srand(seed);
					continue;
				}
				//printf("picIndex = %d list count = %d\n",picIndex,coreData.getMiscTextureList().size());
				modelList.push_back(models[index]);
				usedIndex[index]=true;
				seed = time(NULL) / modelList.size();
				srand(seed);
			}
			models = modelList;
		}
	}

	int displayItemNumber = 1;
	int appear= Intro::appearTime;
	int disappear= Intro::showTime+Intro::appearTime+Intro::disapearTime;

	texts.push_back(new Text("based on the award winning game Glest", Vec2i(w/2, h/2), appear, coreData.getMenuFontVeryBig(),coreData.getMenuFontVeryBig3D()));
	texts.push_back(new Text("the MegaGlest team presents...", Vec2i(w/2, h/2), disappear, coreData.getMenuFontVeryBig(),coreData.getMenuFontVeryBig3D()));

	texts.push_back(new Text(coreData.getLogoTexture(), Vec2i(w/2-128, h/2-64), Vec2i(256, 128), disappear *(++displayItemNumber)));
	texts.push_back(new Text(glestVersionString, Vec2i(w/2+45, h/2-45), disappear *(displayItemNumber++), coreData.getMenuFontNormal(),coreData.getMenuFontNormal3D()));
	texts.push_back(new Text("www.megaglest.org", Vec2i(w/2, h/2), disappear *(displayItemNumber++), coreData.getMenuFontVeryBig(),coreData.getMenuFontVeryBig3D()));

	modelShowTime = disappear *(displayItemNumber);

	if(showIntroPics > 0 && coreData.getMiscTextureList().size() > 0) {
		const int showMiscTime = showIntroPicsTime;

		std::vector<Texture2D *> intoTexList;
		if(showIntroPicsRandom == true) {
			unsigned int seed = time(NULL);
			srand(seed);
			int failedLookups=0;
			std::map<int,bool> usedIndex;
			for(;intoTexList.size() < showIntroPics;) {
				int picIndex = rand() % coreData.getMiscTextureList().size();
				if(usedIndex.find(picIndex) != usedIndex.end()) {
					failedLookups++;
					seed = time(NULL) / failedLookups;
					srand(seed);
					continue;
				}
				//printf("picIndex = %d list count = %d\n",picIndex,coreData.getMiscTextureList().size());
				intoTexList.push_back(coreData.getMiscTextureList()[picIndex]);
				usedIndex[picIndex]=true;
				seed = time(NULL) / intoTexList.size();
				srand(seed);
			}
		}
		else {
			for(unsigned int i = 0;
					i < coreData.getMiscTextureList().size() &&
					i < showIntroPics; ++i) {
				Texture2D *tex = coreData.getMiscTextureList()[i];
				intoTexList.push_back(tex);
			}
		}

		for(unsigned int i = 0; i < intoTexList.size(); ++i) {
			Texture2D *tex = intoTexList[i];
			//printf("tex # %d [%s]\n",i,tex->getPath().c_str());

			Vec2i texPlacement;
			if(i == 0 || i % 9 == 0) {
				texPlacement = Vec2i(1, h-tex->getTextureHeight());
			}
			else if(i == 1 || i % 9 == 1) {
				texPlacement = Vec2i(1, 1);
			}
			else if(i == 2 || i % 9 == 2) {
				texPlacement = Vec2i(w-tex->getTextureWidth(), 1);
			}
			else if(i == 3 || i % 9 == 3) {
				texPlacement = Vec2i(w-tex->getTextureWidth(), h-tex->getTextureHeight());
			}
			else if(i == 4 || i % 9 == 4) {
				texPlacement = Vec2i(w/2 - tex->getTextureWidth()/2, h-tex->getTextureHeight());
			}
			else if(i == 5 || i % 9 == 5) {
				texPlacement = Vec2i(w/2 - tex->getTextureWidth()/2, 1);
			}
			else if(i == 6 || i % 9 == 6) {
				texPlacement = Vec2i(1, (h/2) - (tex->getTextureHeight()/2));
			}
			else if(i == 7 || i % 9 == 7) {
				texPlacement = Vec2i(w-tex->getTextureWidth(), (h/2) - (tex->getTextureHeight()/2));
			}

			texts.push_back(new Text(tex, texPlacement, Vec2i(tex->getTextureWidth(), tex->getTextureHeight()), disappear *displayItemNumber+(showMiscTime*(i+1))));
		}
	}

	//test = NULL;
	//Shared::Graphics::md5::initMD5OpenGL(data_path + "data/core/shaders/");
	//md5Test = Shared::Graphics::md5::getMD5ObjectFromLoaderScript("/home/softcoder/Code/megaglest/trunk/mk/linux/mydata/test/mv1/mv1.loader");

	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	soundRenderer.playMusic(CoreData::getInstance().getIntroMusic());

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

Intro::~Intro() {
	deleteValues(texts.begin(),texts.end());

//	if(test) {
//		glmDelete(test);
//	}

	//Shared::Graphics::md5::cleanupMD5OpenGL();
}

void Intro::update() {
	timer++;
	if(timer > introTime * GameConstants::updateFps / 1000){
	    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	    cleanup();
		return;
	}

	if(Config::getInstance().getBool("AutoTest")){
	    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		AutoTest::getInstance().updateIntro(program);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}

	mouse2d= (mouse2d+1) % Renderer::maxMouse2dAnim;

	if(targetCamera != NULL) {
		t+= ((0.01f+(1.f-t)/10.f)/20.f)*(60.f/GameConstants::updateFps);

		//interpolate position
		camera.setPosition(lastCamera.getPosition().lerp(t, targetCamera->getPosition()));

		//interpolate orientation
		Quaternion q= lastCamera.getOrientation().lerp(t, targetCamera->getOrientation());
		camera.setOrientation(q);

		if(t>=1.f){
			targetCamera= NULL;
			t= 0.f;
		}
	}

	//fade
	if(fade <= 1.f) {
		fade += 0.6f / GameConstants::updateFps;
		if(fade > 1.f){
			fade = 1.f;
		}
	}

	//animation
	//const float minSpeed = 0.015f;
	//const float minSpeed = 0.010f;
	//const float maxSpeed = 0.6f;
	const float minSpeed = modelMinAnimSpeed;
	const float maxSpeed = modelMaxAnimSpeed;
	anim += (maxSpeed / GameConstants::updateFps) / 5 + random.randRange(minSpeed, max(minSpeed + 0.0001f, (maxSpeed / GameConstants::updateFps) / 5.f));
	if(anim > 1.f) {
		anim = 0.f;
	}

	//animTimer.update();
}

void Intro::renderModelBackground() {
	// Black background
	glClearColor(0, 0, 0, 1);

	if(models.size() > 0) {
		int difTime= 1000 * timer / GameConstants::updateFps - modelShowTime;
		int totalModelShowTime = Intro::introTime - modelShowTime;
		int individualModelShowTime = totalModelShowTime / models.size();

		//printf("difTime = %d individualModelShowTime = %d modelIndex = %d\n",difTime,individualModelShowTime,modelIndex);

		//int difTime= 1;
		if(difTime > 0) {
			if(difTime > ((modelIndex+1) * individualModelShowTime)) {
				int oldmodelIndex = modelIndex;
				if(modelIndex+1 < models.size()) {
					modelIndex++;

					//position
					//nextCamera.setPosition(camera.getPosition());
//					nextCamera.setPosition(Vec3f(84,-9,11));
//
//					//rotation
//					//Vec3f startRotation(0,12,0);
//					Vec3f startRotation(0,-80,0);
//					nextCamera.setOrientation(Quaternion(EulerAngles(
//						degToRad(startRotation.x),
//						degToRad(startRotation.y),
//						degToRad(startRotation.z))));
//
//					this->targetCamera = &nextCamera;
//					this->lastCamera= camera;
//					this->t= 0.f;

				}
				//printf("oldmodelIndex = %d, modelIndex = %d\n",oldmodelIndex,modelIndex);
			}
			Renderer &renderer= Renderer::getInstance();
			vector<Model *> characterModels;
			characterModels.push_back(NULL);
			characterModels.push_back(NULL);
			characterModels.push_back(models[modelIndex]);
			const Vec3f characterPosition = startPosition;
			renderer.renderMenuBackground(&camera, fade, NULL, characterModels,characterPosition,anim);
		}
	}
}

void Intro::render() {
	Renderer &renderer= Renderer::getInstance();
	if(renderer.isMasterserverMode() == true) {
		return;
	}
	int difTime=0;

	canRender();
	incrementFps();

	renderer.clearBuffers();
	renderer.reset3dMenu();

	renderer.clearZBuffer();
	renderer.loadCameraMatrix(&camera);

//	const Vec3f &position= camera.getConstPosition();
//	Quaternion orientation= camera.getOrientation().conjugate();
//	Shared::Graphics::md5::Matrix4x4f modelViewMatrix;
//	float *mtx = orientation.toMatrix4().ptr();
//	for(unsigned int i = 0; i < 16; ++i) {
//		modelViewMatrix._m[i] = mtx[i];
//	}

	renderModelBackground();
	renderer.renderParticleManager(rsMenu);

	//printf("animTimer.deltaTime () = %f anim = %f animTimer.deltaTime() / 25.0 = %f\n",animTimer.deltaTime (),anim,animTimer.deltaTime() / 25.0);
	//double anim = animTimer.deltaTime();
	//Shared::Graphics::md5::renderMD5Object(md5Test, animTimer.deltaTime() / 30.0, &modelViewMatrix);

//	if(test == NULL) {
//		glClearColor (0.0, 0.0, 0.0, 0.0);
//		glEnable(GL_DEPTH_TEST);
//		glShadeModel (GL_SMOOTH);
//
//		test = glmReadOBJ("/home/softcoder/Code/megaglest/trunk/mk/linux/r_stack_fall.obj");
//		glmUnitize(test);
//		glmFacetNormals(test);
//		glmVertexNormals(test, 90.0);
//
//		int h = 900;
//		int w = 1680;
//		glViewport (0, 0, (GLsizei) w, (GLsizei) h);
//		glMatrixMode (GL_PROJECTION);
//		glLoadIdentity ();
//		gluPerspective(60.0, (GLfloat) w/(GLfloat) h, 1.0, 20.0);
//		glMatrixMode (GL_MODELVIEW);
//	}
//	if(test != NULL) {
//
//		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//		glLoadIdentity();
//		glTranslatef(0,0,-5);
//
//		glPushMatrix();
//		// I added these to be able to rotate the whole scene so you can see the box and textures
//		glRotatef(90,1,0,0);
//		glRotatef(0,0,1,0);
//		glRotatef(0,0,0,1);
//		glmDraw(test, GLM_SMOOTH| GLM_TEXTURE);
//		//glmDraw(test, GLM_SMOOTH| GLM_TEXTURE|GLM_COLOR);
//		//glmDraw(test, GLM_FLAT);
//		glPopMatrix();
//
//		renderer.swapBuffers();
//		return;
//		//printf("Rendering test");
//	}

	renderer.reset2d();

	for(int i = 0; i < texts.size(); ++i) {
		Text *text= texts[i];

		difTime= 1000 * timer / GameConstants::updateFps - text->getTime();

		if(difTime > 0 && difTime < appearTime + showTime + disapearTime) {
			float alpha= 1.f;
			if(difTime > 0 && difTime < appearTime) {
				//apearing
				alpha= static_cast<float>(difTime) / appearTime;
			}
			else if(difTime > 0 && difTime < appearTime + showTime + disapearTime) {
				//disappearing
				alpha= 1.f- static_cast<float>(difTime - appearTime - showTime) / disapearTime;
			}

			if(text->getText().empty() == false) {
				if(Renderer::renderText3DEnabled) {
					renderer.renderText3D(
						text->getText(), text->getFont3D(), alpha,
						text->getPos().x, text->getPos().y, true);
				}
				else {
					renderer.renderText(
						text->getText(), text->getFont(), alpha,
						text->getPos().x, text->getPos().y, true);
				}
			}

			if(text->getTexture()!=NULL){
				renderer.renderTextureQuad(
					text->getPos().x, text->getPos().y,
					text->getSize().x, text->getSize().y,
					text->getTexture(), alpha);
			}
		}
	}

	if(program != NULL) program->renderProgramMsgBox();

	if(this->forceMouseRender == true) renderer.renderMouse2d(mouseX, mouseY, mouse2d, 0.f);

	renderer.renderFPSWhenEnabled(lastFps);

	renderer.swapBuffers();
}

void Intro::keyDown(char key){
	mouseUpLeft(0, 0);
}

void Intro::mouseUpLeft(int x, int y){
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	soundRenderer.stopMusic(CoreData::getInstance().getIntroMusic());

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	soundRenderer.playMusic(CoreData::getInstance().getMenuMusic());

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	cleanup();
}

void Intro::cleanup() {
	Renderer::getInstance().endMenu();

	program->setState(new MainMenu(program));
}

void Intro::mouseMove(int x, int y, const MouseState *ms) {
	mouseX = x;
	mouseY = y;
}

}}//end namespace
