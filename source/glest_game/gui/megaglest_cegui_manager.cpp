// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2014 Mark Vejvoda
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 3 of the
//	License, or (at your option) any later version
// ==============================================================

// START:
// This section purely to avoid stupid winsock compile errors in vc++
//
#ifdef WIN32

#include <winsock2.h>
#include <winsock.h>
#include <windows.h>

#endif
// END:

#include <GL/glew.h>
#include <CEGUI/RendererModules/OpenGL/GLRenderer.h>
#include <CEGUI/RendererModules/OpenGL/Texture.h>
#include <CEGUI/CEGUI.h>

#include "megaglest_cegui_manager.h"

#include "game_util.h"
//#include "game_constants.h"
#include "config.h"
#include "platform_common.h"
//#include "font.h"
#include "conversion.h"
//#include "string_utils.h"

#include "platform_util.h"
#include "texture_gl.h"

#include "leak_dumper.h"

//using Shared::Graphics::Texture::Format;
using namespace Shared::PlatformCommon;

namespace Glest { namespace Game {

// =========================================================================
//
// This class wraps CEGUI event management
//
class MegaGlest_CEGUI_Events_Manager {
protected:

	CEGUI::Window *ctl;
	std::string name;
	MegaGlest_CEGUIManagerBackInterface *callback;

public:

	MegaGlest_CEGUI_Events_Manager(CEGUI::Window *ctl, std::string name,
			MegaGlest_CEGUIManagerBackInterface *callback) {

		this->ctl 		= ctl;
		this->name 		= name;
		this->callback 	= callback;
	}

	CEGUI::Window * getControl() { return ctl; }
	std::string getEventName() { return name; }
	MegaGlest_CEGUIManagerBackInterface * getCallback() { return callback; }

	bool Event(const CEGUI::EventArgs &event) {
		//printf("Line: %d this->name: %s\n",__LINE__,this->name.c_str());

		if(callback != NULL) {
			//printf("Line: %d this->name: %s\n",__LINE__,this->name.c_str());

			bool result = callback->EventCallback(this->ctl,this->name);
			//printf("Line: %d this->name: %s\n",__LINE__,this->name.c_str());

			return result;
		}
		//printf("Line: %d this->name: %s\n",__LINE__,this->name.c_str());
		return false;
	}

	CEGUI::Event::Subscriber getSubscriber() {
		return CEGUI::Event::Subscriber(&MegaGlest_CEGUI_Events_Manager::Event, this);
	}
};

// =========================================================================
//
// This is the main CEGUI Wrapper class
//
MegaGlest_CEGUIManager::MegaGlest_CEGUIManager() {
	emptyMainWindowRoot = NULL;
	messageBoxRoot 		= NULL;
	errorMessageBoxRoot = NULL;
	containerName 		= "";
	isBootStrapped		= false;
	last_time_pulse		= 0.001 * static_cast<double>(SDL_GetTicks());
	//guiRenderer			= NULL;

	initialized 		= false;
}

MegaGlest_CEGUIManager::~MegaGlest_CEGUIManager() {
	for(std::map<string, std::vector<MegaGlest_CEGUI_Events_Manager *> >::iterator iterMap = eventManagerList.begin();
			iterMap != eventManagerList.end(); ++iterMap) {

		std::vector<MegaGlest_CEGUI_Events_Manager *> &events = iterMap->second;
		while(events.empty() == false) {
			delete events.back();
			events.pop_back();
		}
	}
	eventManagerList.clear();
}

void MegaGlest_CEGUIManager::setupRenderer(int width, int height) {
	// Bootstrap CEGUI::System with an OpenGLRenderer object that uses the
	// current GL viewport, the DefaultResourceProvider, and the default
	// ImageCodec.
	//
	// NB: Your OpenGL context must already be initialised when you call this; CEGUI
	// will not create the OpenGL context itself.
	if(isBootStrapped == false) {
		CEGUI::OpenGLRenderer::bootstrapSystem();
		isBootStrapped = true;
	}
	else {
		CEGUI::Sizef newSize((float)width,(float)height);
		CEGUI::System::getSingleton().getRenderer()->setDisplaySize(newSize);
	}
}
void MegaGlest_CEGUIManager::renderAllContexts() {
	CEGUI::System::getSingleton().renderAllGUIContexts();
}

void MegaGlest_CEGUIManager::handleMouseMotion(float x, float y) {
	CEGUI::System::getSingleton().getDefaultGUIContext().
		injectMousePosition(x,y);
}

/************************************************************************
     Translate a SDLKey to the proper CEGUI::Key
     I added this because i noticed in SDL 1.2 that BACKSPACE scancode
     was not nackspacing in CEGUI, so added this for protection.
 *************************************************************************/
CEGUI::Key::Scan SDLKeyToCEGUIKey(SDL_keysym key) {
     using namespace CEGUI;
     switch (key.sym) {
     	  case SDLK_BACKSPACE:    return Key::Backspace;
          case SDLK_TAB:          return Key::Tab;
          case SDLK_RETURN:       return Key::Return;
          case SDLK_PAUSE:        return Key::Pause;
          case SDLK_ESCAPE:       return Key::Escape;
          case SDLK_SPACE:        return Key::Space;
          case SDLK_COMMA:        return Key::Comma;
          case SDLK_MINUS:        return Key::Minus;
          case SDLK_PERIOD:       return Key::Period;
          case SDLK_SLASH:        return Key::Slash;
          case SDLK_0:            return Key::Zero;
          case SDLK_1:            return Key::One;
          case SDLK_2:            return Key::Two;
          case SDLK_3:            return Key::Three;
          case SDLK_4:            return Key::Four;
          case SDLK_5:            return Key::Five;
          case SDLK_6:            return Key::Six;
          case SDLK_7:            return Key::Seven;
          case SDLK_8:            return Key::Eight;
          case SDLK_9:            return Key::Nine;
          case SDLK_COLON:        return Key::Colon;
          case SDLK_SEMICOLON:    return Key::Semicolon;
          case SDLK_EQUALS:       return Key::Equals;
          case SDLK_LEFTBRACKET:  return Key::LeftBracket;
          case SDLK_BACKSLASH:    return Key::Backslash;
          case SDLK_RIGHTBRACKET: return Key::RightBracket;
          case SDLK_a:            return Key::A;
          case SDLK_b:            return Key::B;
          case SDLK_c:            return Key::C;
          case SDLK_d:            return Key::D;
          case SDLK_e:            return Key::E;
          case SDLK_f:            return Key::F;
          case SDLK_g:            return Key::G;
          case SDLK_h:            return Key::H;
          case SDLK_i:            return Key::I;
          case SDLK_j:            return Key::J;
          case SDLK_k:            return Key::K;
          case SDLK_l:            return Key::L;
          case SDLK_m:            return Key::M;
          case SDLK_n:            return Key::N;
          case SDLK_o:            return Key::O;
          case SDLK_p:            return Key::P;
          case SDLK_q:            return Key::Q;
          case SDLK_r:            return Key::R;
          case SDLK_s:            return Key::S;
          case SDLK_t:            return Key::T;
          case SDLK_u:            return Key::U;
          case SDLK_v:            return Key::V;
          case SDLK_w:            return Key::W;
          case SDLK_x:            return Key::X;
          case SDLK_y:            return Key::Y;
          case SDLK_z:            return Key::Z;
          case SDLK_DELETE:       return Key::Delete;
          case SDLK_KP0:          return Key::Numpad0;
          case SDLK_KP1:          return Key::Numpad1;
          case SDLK_KP2:          return Key::Numpad2;
          case SDLK_KP3:          return Key::Numpad3;
          case SDLK_KP4:          return Key::Numpad4;
          case SDLK_KP5:          return Key::Numpad5;
          case SDLK_KP6:          return Key::Numpad6;
          case SDLK_KP7:          return Key::Numpad7;
          case SDLK_KP8:          return Key::Numpad8;
          case SDLK_KP9:          return Key::Numpad9;
          case SDLK_KP_PERIOD:    return Key::Decimal;
          case SDLK_KP_DIVIDE:    return Key::Divide;
          case SDLK_KP_MULTIPLY:  return Key::Multiply;
          case SDLK_KP_MINUS:     return Key::Subtract;
          case SDLK_KP_PLUS:      return Key::Add;
          case SDLK_KP_ENTER:     return Key::NumpadEnter;
          case SDLK_KP_EQUALS:    return Key::NumpadEquals;
          case SDLK_UP:           return Key::ArrowUp;
          case SDLK_DOWN:         return Key::ArrowDown;
          case SDLK_RIGHT:        return Key::ArrowRight;
          case SDLK_LEFT:         return Key::ArrowLeft;
          case SDLK_INSERT:       return Key::Insert;
          case SDLK_HOME:         return Key::Home;
          case SDLK_END:          return Key::End;
          case SDLK_PAGEUP:       return Key::PageUp;
          case SDLK_PAGEDOWN:     return Key::PageDown;
          case SDLK_F1:           return Key::F1;
          case SDLK_F2:           return Key::F2;
          case SDLK_F3:           return Key::F3;
          case SDLK_F4:           return Key::F4;
          case SDLK_F5:           return Key::F5;
          case SDLK_F6:           return Key::F6;
          case SDLK_F7:           return Key::F7;
          case SDLK_F8:           return Key::F8;
          case SDLK_F9:           return Key::F9;
          case SDLK_F10:          return Key::F10;
          case SDLK_F11:          return Key::F11;
          case SDLK_F12:          return Key::F12;
          case SDLK_F13:          return Key::F13;
          case SDLK_F14:          return Key::F14;
          case SDLK_F15:          return Key::F15;
          case SDLK_NUMLOCK:      return Key::NumLock;
          case SDLK_SCROLLOCK:    return Key::ScrollLock;
          case SDLK_RSHIFT:       return Key::RightShift;
          case SDLK_LSHIFT:       return Key::LeftShift;
          case SDLK_RCTRL:        return Key::RightControl;
          case SDLK_LCTRL:        return Key::LeftControl;
          case SDLK_RALT:         return Key::RightAlt;
          case SDLK_LALT:         return Key::LeftAlt;
          case SDLK_LSUPER:       return Key::LeftWindows;
          case SDLK_RSUPER:       return Key::RightWindows;
          case SDLK_SYSREQ:       return Key::SysRq;
          case SDLK_MENU:         return Key::AppMenu;
          case SDLK_POWER:        return Key::Power;
          default:                return (Key::Scan)key.scancode;
     }

     return (Key::Scan)key.scancode;
}

void MegaGlest_CEGUIManager::handleMouseButtonDown(Uint8 button) {
	switch ( button ) {
		// handle real mouse buttons
		case SDL_BUTTON_LEFT:
			CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonDown(
				CEGUI::LeftButton);
			break;
		case SDL_BUTTON_MIDDLE:
			CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonDown(
				CEGUI::MiddleButton);
			break;
		case SDL_BUTTON_RIGHT:
			CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonDown(
				CEGUI::RightButton);
			break;

		// handle the mouse wheel
		case SDL_BUTTON_WHEELDOWN:
			CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseWheelChange( -1 );
			break;
		case SDL_BUTTON_WHEELUP:
			CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseWheelChange( +1 );
			break;
		}
}

void MegaGlest_CEGUIManager::handleMouseButtonUp(Uint8 button) {
	switch ( button ) {
		case SDL_BUTTON_LEFT:
			CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(
				CEGUI::LeftButton);
			break;
		case SDL_BUTTON_MIDDLE:
			CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(
				CEGUI::MiddleButton);
			break;
		case SDL_BUTTON_RIGHT:
			CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(
				CEGUI::RightButton);
			break;
		}
}

void MegaGlest_CEGUIManager::handleTimePulse() {
	// get current "run-time" in seconds
	double current_time_pulse = 0.001 * SDL_GetTicks();

	// inject the time that passed since the last call
	CEGUI::System::getSingleton().injectTimePulse( 
		float(current_time_pulse - last_time_pulse) );

	// store the new time as the last time
	last_time_pulse = current_time_pulse;
}

void MegaGlest_CEGUIManager::handleKeyDown(SDL_keysym key) {
	// to tell CEGUI that a key was pressed, we inject the scancode.
	CEGUI::Key::Scan cegui_keycode = SDLKeyToCEGUIKey(key);
	CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyDown(cegui_keycode);

	// as for the character it's a litte more complicated. we'll use for translated unicode value.
	// this is described in more detail below.
	if ((key.unicode & 0xFF80) == 0) {
		CEGUI::System::getSingleton().getDefaultGUIContext().injectChar(
			key.unicode & 0x7F);
	}
}

void MegaGlest_CEGUIManager::handleKeyUp(SDL_keysym key){
	CEGUI::Key::Scan cegui_keycode = SDLKeyToCEGUIKey(key);
	CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyUp(cegui_keycode);
}

MegaGlest_CEGUIManager & MegaGlest_CEGUIManager::getInstance() {
	static MegaGlest_CEGUIManager manager;
	return manager;
}

void MegaGlest_CEGUIManager::initializeResourceGroupDirectories() {
	// initialise the required dirs for the DefaultResourceProvider
	CEGUI::DefaultResourceProvider* rp =
		static_cast<CEGUI::DefaultResourceProvider*>
			(CEGUI::System::getSingleton().getResourceProvider());

	string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);

	string data_path_cegui = Config::getInstance().getString("CEGUI-Theme-Path","");
	if(data_path_cegui == "") {
		data_path_cegui = data_path + "/data/cegui/themes/default/";
	}

	const char* dataPathPrefix = data_path_cegui.c_str();
	char resourcePath[4096];

	// for each resource type, set a resource group directory
	sprintf(resourcePath, "%s/%s", dataPathPrefix, "schemes/");
	rp->setResourceGroupDirectory("schemes", resourcePath);
	sprintf(resourcePath, "%s/%s", dataPathPrefix, "imagesets/");
	rp->setResourceGroupDirectory("imagesets", resourcePath);
	sprintf(resourcePath, "%s/%s", dataPathPrefix, "fonts/");
	rp->setResourceGroupDirectory("fonts", resourcePath);
	sprintf(resourcePath, "%s/%s", dataPathPrefix, "layouts/");
	rp->setResourceGroupDirectory("layouts", resourcePath);
	sprintf(resourcePath, "%s/%s", dataPathPrefix, "looknfeel/");
	rp->setResourceGroupDirectory("looknfeels", resourcePath);
	sprintf(resourcePath, "%s/%s", dataPathPrefix, "lua_scripts/");
	rp->setResourceGroupDirectory("lua_scripts", resourcePath);
	sprintf(resourcePath, "%s/%s", dataPathPrefix, "xml_schemas/");
	rp->setResourceGroupDirectory("schemas", resourcePath);
	sprintf(resourcePath, "%s/%s", dataPathPrefix, "animations/");
	rp->setResourceGroupDirectory("animations", resourcePath);
}

void MegaGlest_CEGUIManager::initializeDefaultResourceGroups() {
	// set the default resource groups to be used
	CEGUI::ImageManager::setImagesetDefaultResourceGroup("imagesets");
	CEGUI::Font::setDefaultResourceGroup("fonts");
	CEGUI::Scheme::setDefaultResourceGroup("schemes");
	CEGUI::WidgetLookManager::setDefaultResourceGroup("looknfeels");
	CEGUI::WindowManager::setDefaultResourceGroup("layouts");
	CEGUI::ScriptModule::setDefaultResourceGroup("lua_scripts");
	CEGUI::AnimationManager::setDefaultResourceGroup("animations");
	// setup default group for validation schemas
	CEGUI::XMLParser* parser = CEGUI::System::getSingleton().getXMLParser();
	if (parser->isPropertyPresent("SchemaDefaultResourceGroup")) {
		parser->setProperty("SchemaDefaultResourceGroup", "schemas");
	}
}

string MegaGlest_CEGUIManager::getThemeName() {
	string themeName = Config::getInstance().getString("CEGUI-Theme-Name","");
	if(themeName == "") {
		themeName = "GlossySerpent";
	}
	return themeName;
}

string MegaGlest_CEGUIManager::getThemeCursorName() {
	string themeNameCursors = Config::getInstance().getString("CEGUI-Theme-Name-Cursors","");
	if(themeNameCursors == "") {
		themeNameCursors = "GlossySerpentCursors";
	}
	return themeNameCursors;
}

string MegaGlest_CEGUIManager::getLookName() {
	string themeName = Config::getInstance().getString("CEGUI-Look-Name","");
	if(themeName == "") {
		themeName = "GlossySerpent";
	}
	return themeName;
}

void MegaGlest_CEGUIManager::initializeTheme() {
	string themeName 		= getThemeName();
	string themeNameCursors = getThemeCursorName();

	CEGUI::SchemeManager::getSingleton().createFromFile(themeName + ".scheme");
	CEGUI::System::getSingleton().getDefaultGUIContext().getMouseCursor().setDefaultImage(themeNameCursors + "/MouseArrow");
}

void MegaGlest_CEGUIManager::setupCEGUI() {

	initializeResourceGroupDirectories();
	initializeDefaultResourceGroups();
	initializeTheme();

	string fontFile = findFont();
	//printf("\nCE-GUI set default font: %s\n\n",fontFile.c_str());
	addFont("MEGAGLEST_FONT", fontFile, 8.0f);
	addFont("MEGAGLEST_FONT", fontFile, 10.0f);
	addFont("MEGAGLEST_FONT", fontFile, 12.0f);
	addFont("MEGAGLEST_FONT", fontFile, 14.0f);
	addFont("MEGAGLEST_FONT", fontFile, 16.0f);
	addFont("MEGAGLEST_FONT", fontFile, 18.0f);
	addFont("MEGAGLEST_FONT", fontFile, 20.0f);
	setDefaultFont("MEGAGLEST_FONT", fontFile, 10.0f);

	emptyMainWindowRoot = loadLayoutFromFile("EmptyRoot.layout");
	messageBoxRoot 		= loadLayoutFromFile("MessageBox.layout");
	errorMessageBoxRoot = loadLayoutFromFile("ErrorMessageBox.layout");

	initialized = true;
}

void MegaGlest_CEGUIManager::clearRootWindow() {

	CEGUI::System::getSingleton().getDefaultGUIContext().
			setRootWindow(emptyMainWindowRoot);
}

//void MegaGlest_CEGUIManager::initializeMainMenuRoot() {
//	string themeName 		= getThemeName();
//	string themeNameCursors = getThemeCursorName();
//
//	CEGUI::WindowManager& winMgr(CEGUI::WindowManager::getSingleton());

//	CEGUI::Window* root = winMgr.createWindow("DefaultWindow", "root");


//	CEGUI::Window* fw = root->createChild(themeName + "/FrameWindow");
//	fw->setPosition(CEGUI::UVector2(CEGUI::UDim(0.25, 0), CEGUI::UDim(0.25, 0)));
//	fw->setSize(CEGUI::USize(CEGUI::UDim(0.5, 0), CEGUI::UDim(0.5, 0)));
//	fw->setText("MegaGlest CE-GUI Test");
//
//	CEGUI::Window *gTestBtnWindow = CEGUI::WindowManager::getSingleton().createWindow(themeName + "/Button","TestPushButton");
//	gTestBtnWindow->setPosition(CEGUI::UVector2(cegui_reldim(0.81f), cegui_reldim( 0.32f)));
//	gTestBtnWindow->setSize(CEGUI::USize(cegui_reldim(0.15f), cegui_reldim( 0.2f)));
//	gTestBtnWindow->setText("Test Button");
//	fw->addChild(gTestBtnWindow);
//
//	CEGUI::Window *gTestComboWindow = CEGUI::WindowManager::getSingleton().createWindow(themeName + "/Combobox","TestCombobox");
//	gTestComboWindow->setPosition(CEGUI::UVector2(cegui_reldim(0.04f), cegui_reldim( 0.06f)));
//	gTestComboWindow->setSize(CEGUI::USize(cegui_reldim(0.66f), cegui_reldim( 0.33f)));
//	gTestComboWindow->setText("Test Combo");
//	fw->addChild(gTestComboWindow);
//
//	CEGUI::Window *label = CEGUI::WindowManager::getSingleton().createWindow(themeName + "/StaticText", "Label4");
//    fw->addChild(label);
//    label->setProperty("FrameEnabled", "false");
//    label->setProperty("BackgroundEnabled", "false");
//    label->setPosition(CEGUI::UVector2(cegui_reldim(0.02f), cegui_reldim( 0.55f)));
//    label->setSize(CEGUI::USize(cegui_reldim(0.2f), cegui_reldim( 0.12f)));
//    label->setText("Player Name:");

//	CEGUI::System::getSingleton().getDefaultGUIContext().setRootWindow(root);

//	CEGUI::System::getSingleton().getDefaultGUIContext().setRootWindow(
//			winMgr.loadLayoutFromFile("MainMenuRoot.layout"));
//
//}

void MegaGlest_CEGUIManager::subscribeEventClick(std::string containerName, CEGUI::Window *ctl, std::string name, MegaGlest_CEGUIManagerBackInterface *callback) {
	//printf("SUBSCRIBE Line: %d containerName [%s] ctl [%s] name [%s]\n",__LINE__,containerName.c_str(),ctl->getName().c_str(),name.c_str());

	MegaGlest_CEGUI_Events_Manager *mgr = new MegaGlest_CEGUI_Events_Manager(ctl,name,callback);
	eventManagerList[containerName].push_back(mgr);
	ctl->subscribeEvent(name,mgr->getSubscriber());
}

void MegaGlest_CEGUIManager::unsubscribeEvents(std::string containerName) {
	std::map<string, std::vector<MegaGlest_CEGUI_Events_Manager *> >::iterator iterMap = eventManagerList.find(containerName);
	if(iterMap != eventManagerList.end()) {
		std::vector<MegaGlest_CEGUI_Events_Manager *> &events = iterMap->second;
		while(events.empty() == false) {
			MegaGlest_CEGUI_Events_Manager *event = events.back();
			if(event != NULL) {
				//printf("UNSUBSCRIBE Line: %d containerName [%s] ctl [%s] name [%s]\n",__LINE__,containerName.c_str(),event->getControl()->getName().c_str(),event->getEventName().c_str());
				event->getControl()->removeEvent(event->getEventName());
			}
			delete event;
			events.pop_back();
		}
		eventManagerList.erase(containerName);
	}
}

CEGUI::Window * MegaGlest_CEGUIManager::loadLayoutFromFile(string layoutFile) {
	CEGUI::WindowManager& winMgr(CEGUI::WindowManager::getSingleton());
	if(layoutCache.find(layoutFile) == layoutCache.end()) {
		layoutCache[layoutFile] = winMgr.loadLayoutFromFile(layoutFile);
	}
	return layoutCache[layoutFile];
}

CEGUI::Window * MegaGlest_CEGUIManager::setCurrentLayout(string layoutFile, string containerName) {

	CEGUI::Window *ctl = loadLayoutFromFile(layoutFile);
	CEGUI::System::getSingleton().getDefaultGUIContext().setRootWindow(ctl);
	this->containerName = containerName;

	return ctl;
}

void MegaGlest_CEGUIManager::setControlText(string controlName, string text, bool disableFormatting) {
	CEGUI::Window *root = CEGUI::System::getSingleton().
			getDefaultGUIContext().getRootWindow();

	CEGUI::Window *ctl = root->getChild(controlName);
	setControlText(ctl,text, disableFormatting);
}

void MegaGlest_CEGUIManager::setControlText(CEGUI::Window *ctl, string text, bool disableFormatting) {

	ctl->setTextParsingEnabled(disableFormatting == false);
	ctl->setText((CEGUI::encoded_char*)text.c_str());
}

void MegaGlest_CEGUIManager::setControlTextColor(string controlName, float red, float green, float blue, float alpha) {
	CEGUI::Window *root = CEGUI::System::getSingleton().
			getDefaultGUIContext().getRootWindow();

	CEGUI::Window *ctl = root->getChild(controlName);

	setControlTextColor(ctl, red, green, blue, alpha);
}

void MegaGlest_CEGUIManager::setControlTextColor(CEGUI::Window *ctl, float red, float green, float blue, float alpha) {
	CEGUI::Colour color(red, green, blue, alpha);

	if(dynamic_cast<CEGUI::Editbox *>(ctl) != NULL) {
		ctl->setProperty("NormalTextColour", CEGUI::PropertyHelper<CEGUI::Colour>::toString(color));
	}
	else {
		ctl->setProperty("TextColours", CEGUI::PropertyHelper<CEGUI::Colour>::toString(color));
	}
}

string MegaGlest_CEGUIManager::getTextColorFromRGBA(float red, float green, float blue, float alpha) {
	CEGUI::Colour color(red, green, blue, alpha);
	return CEGUI::PropertyHelper<CEGUI::Colour>::toString(color).c_str();
}

string MegaGlest_CEGUIManager::getControlText(string controlName) {
	CEGUI::Window *root = CEGUI::System::getSingleton().
			getDefaultGUIContext().getRootWindow();

	CEGUI::Window *ctl = root->getChild(controlName);
	return getControlText(ctl);
}
string MegaGlest_CEGUIManager::getControlText(CEGUI::Window *ctl) {

	return ctl->getText().c_str();
}

void MegaGlest_CEGUIManager::setControlEventCallback(string containerName,
		string controlName, string eventName, MegaGlest_CEGUIManagerBackInterface *cb) {

	CEGUI::Window *root = CEGUI::System::getSingleton().
			getDefaultGUIContext().getRootWindow();
	CEGUI::Window *ctl = root->getChild(controlName);
	//printf("In [%s] controlName [%s] ctl: %p\n",__FUNCTION__,controlName.c_str(),ctl);
	subscribeEventClick(containerName,ctl,eventName, cb);

}

void MegaGlest_CEGUIManager::setControlEventCallback(string containerName,
		CEGUI::Window *ctl, string eventName, MegaGlest_CEGUIManagerBackInterface *cb) {

	//CEGUI::Window *root = CEGUI::System::getSingleton().
	//		getDefaultGUIContext().getRootWindow();
	//CEGUI::Window *ctl = root->getChild(controlName);
	subscribeEventClick(containerName,ctl,eventName, cb);

}

void MegaGlest_CEGUIManager::dumpWindowNames(string dumpTag) {
	CEGUI::WindowManager &winMgr(CEGUI::WindowManager::getSingleton());
	winMgr.DEBUG_dumpWindowNames(dumpTag);
}

CEGUI::Window * MegaGlest_CEGUIManager::getRootControl() {
	return CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
}

CEGUI::Window * MegaGlest_CEGUIManager::getControl(string controlName, bool recursiveScan) {
	CEGUI::Window *root = CEGUI::System::getSingleton().
			getDefaultGUIContext().getRootWindow();
	if(recursiveScan == false) {
		return root->getChild(controlName);
	}
	else {
		CEGUI::NamedElement *element = root->getChildElementRecursive(controlName);
		if(element != NULL) {
			return root->getChild(element->getNamePath());
		}
		return NULL;
	}
}

CEGUI::Window * MegaGlest_CEGUIManager::getChildControl(CEGUI::Window *parentCtl,string controlNameChild, bool recursiveScan) {
	if(recursiveScan == false) {
		return parentCtl->getChild(controlNameChild);
	}
	CEGUI::NamedElement *element = parentCtl->getChildElementRecursive(controlNameChild);
	if(element != NULL) {
		return parentCtl->getChild(element->getNamePath());
	}
	return NULL;
}

bool MegaGlest_CEGUIManager::isChildControl(CEGUI::Window *parentCtl,string controlNameChild) {
	return parentCtl->isChild(controlNameChild);
}

string MegaGlest_CEGUIManager::getControlFullPathName(CEGUI::Window *ctl) {
	return ctl->getNamePath().c_str();
}

string MegaGlest_CEGUIManager::getEventButtonClicked() {
	string result = CEGUI::PushButton::EventClicked.c_str();
	return result;
}

string MegaGlest_CEGUIManager::getEventComboboxClicked() {
	string result = CEGUI::Combobox::EventListSelectionChanged.c_str();
	return result;
}

string MegaGlest_CEGUIManager::getEventComboboxChangeAccepted() {
	string result = CEGUI::Combobox::EventListSelectionAccepted.c_str();
	return result;
}

string MegaGlest_CEGUIManager::getEventCheckboxClicked() {
	string result = CEGUI::ToggleButton::EventSelectStateChanged.c_str();
	return result;
}

string MegaGlest_CEGUIManager::getEventTabControlSelectionChanged() {
	string result = CEGUI::TabControl::EventSelectionChanged.c_str();
	return result;
}

string MegaGlest_CEGUIManager::getEventSpinnerValueChanged() {
	string result = CEGUI::Spinner::EventValueChanged.c_str();
	return result;
}

string MegaGlest_CEGUIManager::getEventMultiColumnListSelectionChanged() {
	string result = CEGUI::MultiColumnList::EventSelectionChanged.c_str();
	return result;
}

void MegaGlest_CEGUIManager::addFont(string fontName, string fontFileName, float fontPointSize) {
	string fontPath = extractDirectoryPathFromFile(fontFileName);
	string fontFile = extractFileFromDirectoryPath(fontFileName);

	CEGUI::FontManager &fontManager(CEGUI::FontManager::getSingleton());
	string fontNameIdentifier = fontName + "-" + floatToStr(fontPointSize,2);

	//printf("\nCE-GUI set default font: [%s] fontNameIdentifier [%s] fontPointSize: %f\n\n",fontFileName.c_str(),fontNameIdentifier.c_str(),fontPointSize);

	if(fontManager.isDefined(fontNameIdentifier) == false) {
		fontManager.createFreeTypeFont(
				fontNameIdentifier,
				fontPointSize,
				true,
				fontFileName,
				//CEGUI::Font::getDefaultResourceGroup(),
				//fontPath,
				"MEGAGLEST_DYNAMIC",
				CEGUI::ASM_Vertical,
				CEGUI::Sizef(1280.0f, 720.0f));
	}
}

void MegaGlest_CEGUIManager::setDefaultFont(string fontName, string fontFileName, float fontPointSize) {

	addFont(fontName, fontFileName, fontPointSize);
	string fontNameIdentifier = fontName + "-" + floatToStr(fontPointSize,2);

	CEGUI::FontManager &fontManager(CEGUI::FontManager::getSingleton());
	CEGUI::Font &font(fontManager.get(fontNameIdentifier));
	CEGUI::System::getSingleton().getDefaultGUIContext().setDefaultFont(&font);
}

void MegaGlest_CEGUIManager::setControlFont(CEGUI::Window *ctl, string fontName, float fontPointSize) {
	if(fontName == "") {
		fontName = "MEGAGLEST_FONT";
	}
	string fontNameIdentifier = fontName + "-" + floatToStr(fontPointSize,2);
	//printf("fontNameIdentifier: %s\n",fontNameIdentifier.c_str());
	CEGUI::FontManager &fontManager(CEGUI::FontManager::getSingleton());
	CEGUI::Font &font(fontManager.get(fontNameIdentifier));

	ctl->setFont(&font);
}

void MegaGlest_CEGUIManager::setImageFileForControl(string imageName, string imageFileName, string controlName) {

	CEGUI::DefaultWindow* staticImage = static_cast<CEGUI::DefaultWindow*>(getControl(controlName));
	if(imageName != "" || imageFileName != "") {
		if( CEGUI::ImageManager::getSingleton().isDefined(imageName) == false ) {
			string filePath = extractDirectoryPathFromFile(imageFileName);
			CEGUI::ImageManager::getSingleton().addFromImageFile(imageName, imageFileName, filePath);
		}
		staticImage->setProperty("Image", imageName);
		staticImage->setVisible(true);
	}
	else {
		staticImage->setVisible(false);
	}
}

void MegaGlest_CEGUIManager::setImageForControl(string textureName,Texture2D *gameTexture, string controlName,
												bool isTextureTargetVerticallyFlipped) {

	string imageType 		= "BasicImage";
	string imageName 		= "BasicImage_" + textureName;
	string imageGroupName 	= "MegaGlestImageGroup/" + imageName;

	CEGUI::DefaultWindow* staticImage = static_cast<CEGUI::DefaultWindow*>(getControl(controlName));
	if(gameTexture != NULL) {

		CEGUI::BasicImage *image = NULL;
		if( CEGUI::ImageManager::getSingleton().isDefined(imageGroupName) == false ) {
			image = static_cast<CEGUI::BasicImage*>(&CEGUI::ImageManager::getSingleton().create(imageType, imageGroupName));
		}
		else {
			image = static_cast<CEGUI::BasicImage*>(&CEGUI::ImageManager::getSingleton().get(imageGroupName));
		}

		CEGUI::Texture *texture = NULL;
		if(CEGUI::System::getSingletonPtr()->getRenderer()->isTextureDefined(textureName.c_str()) == false) {

			texture = &CEGUI::System::getSingletonPtr()->getRenderer()->createTexture(textureName.c_str());
		}
		else {
			CEGUI::System::getSingletonPtr()->getRenderer()->destroyTexture(textureName.c_str());
			texture = &CEGUI::System::getSingletonPtr()->getRenderer()->createTexture(textureName.c_str());
		}

		CEGUI::Sizef buffer_size(gameTexture->getPixmap()->getW(),gameTexture->getPixmap()->getH());
		CEGUI::Rectf rect(CEGUI::Vector2f(0.0f, 0.0f), buffer_size);

		Gl::TextureGl *glTex = dynamic_cast<Gl::TextureGl *>(gameTexture);
		if(glTex) {
			CEGUI::OpenGLTexture *ceGuiTex = (CEGUI::OpenGLTexture *)(texture);
			ceGuiTex->setOpenGLTexture(glTex->getHandle(),buffer_size);

			image->setTexture(texture);

			//Flipping is necessary due to differences between renderers regarding top or bottom being the origin
			if(isTextureTargetVerticallyFlipped) {
				rect = CEGUI::Rectf(0.0f, buffer_size.d_height, buffer_size.d_width, 0.0f);
			}
			image->setArea(rect);
			image->setAutoScaled(CEGUI::ASM_Both);

			staticImage->setProperty("Image", imageGroupName);
			staticImage->setVisible(true);
		}
	}
	else {
		if( CEGUI::ImageManager::getSingleton().isDefined(imageGroupName) == true ) {
			if(CEGUI::System::getSingletonPtr()->getRenderer()->isTextureDefined(textureName.c_str()) == true) {
				CEGUI::System::getSingletonPtr()->getRenderer()->destroyTexture(textureName.c_str());
			}
		}
		staticImage->setVisible(false);
	}
}

CEGUI::Window * MegaGlest_CEGUIManager::cloneMessageBoxControl(string newMessageBoxControlName,
		CEGUI::Window *rootControlToAddTo) {

	CEGUI::Window *msgRoot = getMessageBoxRoot();
	CEGUI::Window *ctl = msgRoot->clone(true);
	ctl->setName(newMessageBoxControlName);
	ctl->setVisible(false);
	ctl->setAlwaysOnTop(true);

	if(rootControlToAddTo != NULL) {
		rootControlToAddTo->addChild(ctl);
		ctl = rootControlToAddTo->getChild(newMessageBoxControlName);
	}
	return ctl;
}

CEGUI::Window * MegaGlest_CEGUIManager::getMessageBoxRoot(string rootMessageBoxFullName) {

	CEGUI::Window *root = CEGUI::System::getSingleton().
			getDefaultGUIContext().getRootWindow();

	CEGUI::Window *msgRoot = NULL;
	if(rootMessageBoxFullName != "") {
		msgRoot = getControl(rootMessageBoxFullName);
	}
	else {
		if(root->isChild(messageBoxRoot->getName()) == false) {
			CEGUI::Window *ctl = messageBoxRoot->clone(true);
			ctl->setVisible(false);
			ctl->setAlwaysOnTop(true);
			root->addChild(ctl);
		}
		msgRoot = root->getChild(messageBoxRoot->getName());
	}
	//printf("messageBoxRoot->getName() [%s]\n",messageBoxRoot->getName().c_str());


	return msgRoot;
}
void MegaGlest_CEGUIManager::displayMessageBox(string title, string text,
		string buttonTextOk, string buttonTextCancel, string rootMessageBoxFullName) {

	CEGUI::Window *ctlMsg = getMessageBoxRoot(rootMessageBoxFullName)->getChild("MessageBox");
	ctlMsg->setText((CEGUI::encoded_char*)title.c_str());
	ctlMsg->getChild("MessageText")->setText((CEGUI::encoded_char*)text.c_str());
	ctlMsg->getChild("ButtonOk")->setText((CEGUI::encoded_char*)buttonTextOk.c_str());
	ctlMsg->getChild("ButtonCancel")->setText((CEGUI::encoded_char*)buttonTextCancel.c_str());

	if(buttonTextCancel == "") {
		CEGUI::UDim xPos(0.35f,0);
		ctlMsg->getChild("ButtonOk")->setXPosition(xPos);
		ctlMsg->getChild("ButtonCancel")->setVisible(false);
	}
	else {
		CEGUI::UDim xPos(0.17388f,0);
		ctlMsg->getChild("ButtonOk")->setXPosition(xPos);
		ctlMsg->getChild("ButtonCancel")->setVisible(true);
	}

	getMessageBoxRoot(rootMessageBoxFullName)->setVisible(true);
}

void MegaGlest_CEGUIManager::setMessageBoxButtonText(string buttonTextOk,
		string buttonTextCancel, string rootMessageBoxFullName) {

	CEGUI::Window *ctlMsg = getMessageBoxRoot(rootMessageBoxFullName)->getChild("MessageBox");
	ctlMsg->getChild("ButtonOk")->setText((CEGUI::encoded_char*)buttonTextOk.c_str());
	ctlMsg->getChild("ButtonCancel")->setText((CEGUI::encoded_char*)buttonTextCancel.c_str());

	if(buttonTextCancel == "") {
		CEGUI::UDim xPos(0.35f,0);
		ctlMsg->getChild("ButtonOk")->setXPosition(xPos);
		ctlMsg->getChild("ButtonCancel")->setVisible(false);
	}
	else {
		CEGUI::UDim xPos(0.17388f,0);
		ctlMsg->getChild("ButtonOk")->setXPosition(xPos);
		ctlMsg->getChild("ButtonCancel")->setVisible(true);
	}

	getMessageBoxRoot(rootMessageBoxFullName)->setVisible(true);
}

bool MegaGlest_CEGUIManager::isMessageBoxShowing(string rootMessageBoxFullName) {
	return getMessageBoxRoot(rootMessageBoxFullName)->isVisible();
}

void MegaGlest_CEGUIManager::hideMessageBox(string rootMessageBoxFullName) {

	getMessageBoxRoot(rootMessageBoxFullName)->setVisible(false);
}

void MegaGlest_CEGUIManager::subscribeMessageBoxEventClicks(std::string containerName,
		MegaGlest_CEGUIManagerBackInterface *cb,string rootMessageBoxFullName) {

	CEGUI::Window *ctlOk = getMessageBoxRoot(rootMessageBoxFullName)->getChild("MessageBox")->getChild("ButtonOk");
	subscribeEventClick(containerName, ctlOk, getEventButtonClicked(), cb);
	CEGUI::Window *ctlCancel = getMessageBoxRoot(rootMessageBoxFullName)->getChild("MessageBox")->getChild("ButtonCancel");
	subscribeEventClick(containerName, ctlCancel, getEventButtonClicked(), cb);
}

bool MegaGlest_CEGUIManager::isControlMessageBoxOk(CEGUI::Window *ctl,string rootMessageBoxFullName) {

	bool result = false;

	if(getMessageBoxRoot(rootMessageBoxFullName)->isVisible() == true && ctl != NULL) {
		CEGUI::Window *ctlOk = getMessageBoxRoot(rootMessageBoxFullName)->getChild("MessageBox")->getChild("ButtonOk");

		//printDebugControlInfo(ctl);
		//printDebugControlInfo(ctlOk);

		result = (ctl == ctlOk);

	}
	//printf("isControlMessageBoxOk result = %d\n",result);
	return result;
}

bool MegaGlest_CEGUIManager::isControlMessageBoxCancel(CEGUI::Window *ctl,string rootMessageBoxFullName) {

	bool result = false;

	if(getMessageBoxRoot(rootMessageBoxFullName)->isVisible() == true && ctl != NULL) {
		CEGUI::Window *ctlCancel = getMessageBoxRoot(rootMessageBoxFullName)->getChild("MessageBox")->getChild("ButtonCancel");
		result = (ctl == ctlCancel);
	}
	return result;
}

CEGUI::Window * MegaGlest_CEGUIManager::getErrorMessageBoxRoot() {

	CEGUI::Window *root = CEGUI::System::getSingleton().
			getDefaultGUIContext().getRootWindow();

	if(root->isChild(errorMessageBoxRoot->getName()) == false) {
		CEGUI::Window *ctl = errorMessageBoxRoot->clone(true);
		ctl->setVisible(false);
		ctl->setAlwaysOnTop(true);
		root->addChild(ctl);
	}
	return root->getChild(errorMessageBoxRoot->getName());
}
void MegaGlest_CEGUIManager::displayErrorMessageBox(string title, string text,
		string buttonTextOk) {

	CEGUI::Window *ctlMsg = getErrorMessageBoxRoot()->getChild("ErrorMessageBox");
	ctlMsg->setText((CEGUI::encoded_char*)title.c_str());
	ctlMsg->getChild("MessageText")->setText((CEGUI::encoded_char*)text.c_str());
	ctlMsg->getChild("ButtonOk")->setText((CEGUI::encoded_char*)buttonTextOk.c_str());

	getErrorMessageBoxRoot()->setVisible(true);
}

bool MegaGlest_CEGUIManager::isErrorMessageBoxShowing() {
	return getErrorMessageBoxRoot()->isVisible();
}

void MegaGlest_CEGUIManager::hideErrorMessageBox() {

	getErrorMessageBoxRoot()->setVisible(false);
}

void MegaGlest_CEGUIManager::subscribeErrorMessageBoxEventClicks(std::string containerName,
		MegaGlest_CEGUIManagerBackInterface *cb) {

	CEGUI::Window *ctlOk = getErrorMessageBoxRoot()->getChild("ErrorMessageBox")->getChild("ButtonOk");
	subscribeEventClick(containerName, ctlOk, getEventButtonClicked(), cb);
}

bool MegaGlest_CEGUIManager::isControlErrorMessageBoxOk(CEGUI::Window *ctl) {
	bool result = false;

	if(getErrorMessageBoxRoot()->isVisible() == true && ctl != NULL) {
		CEGUI::Window *ctlOk = getErrorMessageBoxRoot()->getChild("ErrorMessageBox")->getChild("ButtonOk");
		result = (ctl == ctlOk);
	}
	return result;
}

void MegaGlest_CEGUIManager::addTabPageToTabControl(string tabControlName, CEGUI::Window *ctl, string fontName, float fontPointSize) {

	CEGUI::Window *genericCtl = getControl(tabControlName);
	CEGUI::TabControl *tabCtl = static_cast<CEGUI::TabControl *>(genericCtl);

	if(fontPointSize > 0) {
		setControlFont(tabCtl,fontName, fontPointSize);
	}
	//printf("Adding tab page [%p] [%s]\n",ctl,(ctl != NULL ? ctl->getName().c_str() : "NULL)"));
	tabCtl->addTab(ctl);
	tabCtl->makeTabVisible(ctl->getName());
}

void MegaGlest_CEGUIManager::setSelectedTabPage(string tabControlName, string tabPageName) {

	CEGUI::Window *genericCtl = getControl(tabControlName);
	CEGUI::TabControl *tabCtl = static_cast<CEGUI::TabControl *>(genericCtl);

	tabCtl->setSelectedTab(tabPageName);
}

bool MegaGlest_CEGUIManager::isSelectedTabPage(string tabControlName, string tabPageName) {

	CEGUI::Window *genericCtl = getControl(tabControlName);
	CEGUI::TabControl *tabCtl = static_cast<CEGUI::TabControl *>(genericCtl);

	CEGUI::Window *tabPage = tabCtl->getTabContents(tabPageName);
	return tabCtl->isTabContentsSelected(tabPage);

}

void MegaGlest_CEGUIManager::addItemToComboBoxControl(CEGUI::Window *ctl, string value, int id, bool disableFormatting) {
	addItemToComboBoxControl(ctl, value, id, NULL, disableFormatting);
}

void MegaGlest_CEGUIManager::addItemToComboBoxControl(CEGUI::Window *ctl, string value, int id, void *userData,bool disableFormatting) {
	CEGUI::Combobox *combobox = static_cast<CEGUI::Combobox*>(ctl);
	bool wasReadOnly = combobox->isReadOnly();
	if(wasReadOnly == true) {
		combobox->setReadOnly(false);
	}

	CEGUI::String cegui_value((CEGUI::encoded_char*)value.c_str());
	//printf("\nCE-GUI add item to combobox: [%s] [%s]\n",cegui_value.c_str(),value.c_str());

	CEGUI::ListboxTextItem *itemCombobox = new CEGUI::ListboxTextItem(cegui_value, id);

	string selectionImageName = getLookName() + "/MultiListSelectionBrush";
	const CEGUI::Image *selectionImage = &CEGUI::ImageManager::getSingleton().get(selectionImageName);
	itemCombobox->setSelectionBrushImage(selectionImage);
	itemCombobox->setTextParsingEnabled(disableFormatting == false);
	itemCombobox->setUserData(userData);

	combobox->setTextParsingEnabled(disableFormatting == false);
	combobox->addItem(itemCombobox);
	if(wasReadOnly == true) {
		combobox->setReadOnly(true);
	}
}

void MegaGlest_CEGUIManager::addItemsToComboBoxControl(CEGUI::Window *ctl, vector<string> valueList, bool disableFormatting) {
	CEGUI::Combobox *combobox = static_cast<CEGUI::Combobox*>(ctl);
	int previousItemCount = combobox->getItemCount();

	combobox->resetList();
	for(unsigned int index = 0; index < valueList.size(); ++index) {

		string &value = valueList[index];
		int position = previousItemCount + index;
		addItemToComboBoxControl(ctl, value, position, disableFormatting);
	}
}

void MegaGlest_CEGUIManager::addItemsToComboBoxControl(CEGUI::Window *ctl, map<string,int> mapList, bool disableFormatting) {

	CEGUI::Combobox *combobox = static_cast<CEGUI::Combobox*>(ctl);
	combobox->resetList();

	for(map<string,int>::iterator iterMap = mapList.begin();
			iterMap != mapList.end(); ++iterMap) {

		string value = iterMap->first;
		int position = iterMap->second;
		addItemToComboBoxControl(ctl, value, position, disableFormatting);
	}
}

void MegaGlest_CEGUIManager::addItemsToComboBoxControl(CEGUI::Window *ctl, map<string,void*> mapList, bool disableFormatting) {

	CEGUI::Combobox *combobox = static_cast<CEGUI::Combobox*>(ctl);
	combobox->resetList();

	int previousItemCount = 0;
	for(map<string,void*>::iterator iterMap = mapList.begin();
			iterMap != mapList.end(); ++iterMap) {

		string value = iterMap->first;
		void *userData = iterMap->second;
		int position = previousItemCount;

		addItemToComboBoxControl(ctl, value, position, userData, disableFormatting);

		previousItemCount++;
	}
}

string MegaGlest_CEGUIManager::getSelectedItemFromComboBoxControl(CEGUI::Window *ctl) {

	CEGUI::Combobox *combobox = static_cast<CEGUI::Combobox*>(ctl);
	CEGUI::ListboxItem *itemCombobox = combobox->getSelectedItem();
	return itemCombobox->getText().c_str();
}

string MegaGlest_CEGUIManager::getItemFromComboBoxControl(CEGUI::Window *ctl, int index) {
	CEGUI::Combobox *combobox = static_cast<CEGUI::Combobox*>(ctl);
	CEGUI::ListboxItem *itemCombobox = combobox->getListboxItemFromIndex(index);
	return itemCombobox->getText().c_str();
}

void * MegaGlest_CEGUIManager::getSelectedUserDataItemFromComboBoxControl(CEGUI::Window *ctl) {
	CEGUI::Combobox *combobox = static_cast<CEGUI::Combobox*>(ctl);
	CEGUI::ListboxItem *itemCombobox = combobox->getSelectedItem();

	return itemCombobox->getUserData();
}

int MegaGlest_CEGUIManager::getSelectedItemIndexFromComboBoxControl(CEGUI::Window *ctl) {

	CEGUI::Combobox *combobox = static_cast<CEGUI::Combobox*>(ctl);
	CEGUI::ListboxItem *itemCombobox = combobox->getSelectedItem();

	//printf("Line: %d [%p - %s] [%p]\n",__LINE__,combobox,combobox->getName().c_str(),itemCombobox);

	return (itemCombobox != NULL ? combobox->getItemIndex(itemCombobox) : -1);
}

int MegaGlest_CEGUIManager::getSelectedItemIdFromComboBoxControl(CEGUI::Window *ctl) {

	CEGUI::Combobox *combobox = static_cast<CEGUI::Combobox*>(ctl);
	CEGUI::ListboxItem *itemCombobox = combobox->getSelectedItem();

	//printf("Line: %d [%p - %s] [%p]\n",__LINE__,combobox,combobox->getName().c_str(),itemCombobox);
	return (itemCombobox != NULL ? itemCombobox->getID() : -1);
}

int MegaGlest_CEGUIManager::getItemCountInComboBoxControl(CEGUI::Window *ctl) {
	CEGUI::Combobox *combobox = static_cast<CEGUI::Combobox*>(ctl);
	return combobox->getItemCount();
}

void MegaGlest_CEGUIManager::setSelectedItemInComboBoxControl(CEGUI::Window *ctl, int index) {
	CEGUI::Combobox *combobox = static_cast<CEGUI::Combobox*>(ctl);
	bool wasReadOnly = combobox->isReadOnly();
	if(wasReadOnly == true) {
		combobox->setReadOnly(false);
	}

	combobox->setItemSelectState(index,true);

	if(wasReadOnly == true) {
		combobox->setReadOnly(true);
	}
}

void MegaGlest_CEGUIManager::setSelectedItemInComboBoxControl(CEGUI::Window *ctl, string value, bool disableFormatting) {
	CEGUI::Combobox *combobox = static_cast<CEGUI::Combobox*>(ctl);
	bool wasReadOnly = combobox->isReadOnly();
	if(wasReadOnly == true) {
		combobox->setReadOnly(false);
	}

	CEGUI::String cegui_value((CEGUI::encoded_char*)value.c_str());
	combobox->setTextParsingEnabled(disableFormatting == false);
	//combobox->setText(cegui_value);
	CEGUI::ListboxItem *lbItem = combobox->findItemWithText(cegui_value,NULL);
	//printf("value [%s] [%s] [%p]\n",value.c_str(),cegui_value.c_str(),lbItem);
	combobox->setItemSelectState(lbItem,true);

	if(wasReadOnly == true) {
		combobox->setReadOnly(true);
	}
}

void MegaGlest_CEGUIManager::setSelectedUserDataItemInComboBoxControl(CEGUI::Window *ctl, void *value, bool disableFormatting) {
	CEGUI::Combobox *combobox = static_cast<CEGUI::Combobox*>(ctl);
	bool wasReadOnly = combobox->isReadOnly();
	if(wasReadOnly == true) {
		combobox->setReadOnly(false);
	}

	combobox->setTextParsingEnabled(disableFormatting == false);

	for(size_t index = 0; index < combobox->getItemCount(); ++index) {
		CEGUI::ListboxItem *lbItem = combobox->getListboxItemFromIndex(index);
		if(lbItem->getUserData() == value) {
			combobox->setItemSelectState(lbItem,true);
			break;
		}
	}

	if(wasReadOnly == true) {
		combobox->setReadOnly(true);
	}
}

void MegaGlest_CEGUIManager::addItemToListBoxControl(CEGUI::Window *ctl, string value, int index, bool disableFormatting) {
	CEGUI::Listbox *listbox = static_cast<CEGUI::Listbox*>(ctl);

	CEGUI::String cegui_value((CEGUI::encoded_char*)value.c_str());
	CEGUI::ListboxTextItem *itemCombobox = new CEGUI::ListboxTextItem(cegui_value, index);

	string selectionImageName = getLookName() + "/MultiListSelectionBrush";
	const CEGUI::Image *selectionImage = &CEGUI::ImageManager::getSingleton().get(selectionImageName);
	itemCombobox->setSelectionBrushImage(selectionImage);
	itemCombobox->setTextParsingEnabled(disableFormatting == false);

	listbox->setTextParsingEnabled(disableFormatting == false);
	listbox->addItem(itemCombobox);
}

void MegaGlest_CEGUIManager::addItemsToListBoxControl(CEGUI::Window *ctl, vector<string> valueList, bool disableFormatting) {
	CEGUI::Listbox *listbox = static_cast<CEGUI::Listbox*>(ctl);
	int previousItemCount = listbox->getItemCount();

	listbox->resetList();
	for(unsigned int index = 0; index < valueList.size(); ++index) {

		string &value = valueList[index];
		int position = previousItemCount + index;
		addItemToListBoxControl(ctl, value, position, disableFormatting);
	}
}

void MegaGlest_CEGUIManager::setSelectedItemInListBoxControl(CEGUI::Window *ctl, int index) {
	CEGUI::Listbox *listbox = static_cast<CEGUI::Listbox*>(ctl);

	listbox->setItemSelectState(index,true);
}

void MegaGlest_CEGUIManager::setSelectedItemInListBoxControl(CEGUI::Window *ctl, string value, bool disableFormatting) {
	CEGUI::Combobox *listbox = static_cast<CEGUI::Combobox*>(ctl);

	CEGUI::String cegui_value((CEGUI::encoded_char*)value.c_str());
	listbox->setTextParsingEnabled(disableFormatting == false);
	listbox->setText(cegui_value);
}

void MegaGlest_CEGUIManager::setSpinnerControlValues(CEGUI::Window *ctl, double minValue, double maxValue, double curValue,double interval) {

	CEGUI::Spinner *spinner = static_cast<CEGUI::Spinner*>(ctl);
	spinner->setMinimumValue(minValue);
	spinner->setMaximumValue(maxValue);
	spinner->setCurrentValue(curValue);
	spinner->setStepSize(interval);
}

void MegaGlest_CEGUIManager::setSpinnerControlValue(CEGUI::Window *ctl, double curValue) {
	CEGUI::Spinner *spinner = static_cast<CEGUI::Spinner*>(ctl);
	spinner->setCurrentValue(curValue);
}

double MegaGlest_CEGUIManager::getSpinnerControlValue(CEGUI::Window *ctl) {

	CEGUI::Spinner *spinner = static_cast<CEGUI::Spinner*>(ctl);
	return spinner->getCurrentValue();
}

void MegaGlest_CEGUIManager::setSliderControlValues(CEGUI::Window *ctl, float minValue, float maxValue, float curValue,float interval) {
	CEGUI::Scrollbar *scrollbar = static_cast<CEGUI::Scrollbar*>(ctl);

	scrollbar->setUserString("MG_MIN",floatToStr(minValue));
	scrollbar->setUserString("MG_MAX",floatToStr(maxValue));
	float realMax = maxValue-minValue + 1;
	scrollbar->setDocumentSize(realMax);
	scrollbar->setPageSize(interval);
	float realScrollPosition = curValue;
//	if(minValue < 0) {
//		realScrollPosition += (minValue * -1.0);
//	}
//	else if(minValue > 0) {
//		realScrollPosition -= minValue;
//	}
	if(minValue != 0) {
		realScrollPosition -= minValue;
	}
	scrollbar->setScrollPosition(realScrollPosition);
	scrollbar->setStepSize(interval);

	//printf("Line: %d realMax = %f curValue = %f realScrollPosition = %f\n",__LINE__,realMax,curValue,realScrollPosition);
}

double MegaGlest_CEGUIManager::getSliderControlValue(CEGUI::Window *ctl) {
	CEGUI::Scrollbar *scrollbar = static_cast<CEGUI::Scrollbar*>(ctl);
	float realScrollPosition = scrollbar->getScrollPosition();

	string strMinValue = scrollbar->getUserString("MG_MIN").c_str();
	float minValue = strToFloat(strMinValue);

	float curValue = realScrollPosition;
//	if(minValue < 0) {
//		curValue -= (minValue * -1.0);
//	}
//	else if(minValue > 0) {
//		curValue += minValue;
//	}
	if(minValue != 0) {
		curValue += minValue;
	}

	//printf("Line: %d curValue = %f realScrollPosition = %f\n",__LINE__,curValue,realScrollPosition);

	return curValue;
}

void MegaGlest_CEGUIManager::setCheckboxControlChecked(CEGUI::Window *ctl, bool checked, bool disableEventsTrigger) {
	CEGUI::ToggleButton *checkbox = static_cast<CEGUI::ToggleButton*>(ctl);

	bool wasDisable = checkbox->isMuted();
	if(disableEventsTrigger == true && wasDisable == false) {
		checkbox->setMutedState(true);
	}
	checkbox->setSelected(checked);

	if(disableEventsTrigger == true && wasDisable == false) {
		checkbox->setMutedState(false);
	}
}

bool MegaGlest_CEGUIManager::getCheckboxControlChecked(CEGUI::Window *ctl) {
	CEGUI::ToggleButton *checkbox = static_cast<CEGUI::ToggleButton*>(ctl);
	return checkbox->isSelected();
}

void MegaGlest_CEGUIManager::setControlVisible(string controlName, bool visible) {
	CEGUI::Window *root = CEGUI::System::getSingleton().
			getDefaultGUIContext().getRootWindow();

	CEGUI::Window *ctl = root->getChild(controlName);
	setControlVisible(ctl,visible);
}

void MegaGlest_CEGUIManager::setControlVisible(CEGUI::Window *ctl, bool visible) {
	ctl->setVisible(visible);
}

bool MegaGlest_CEGUIManager::getControlVisible(CEGUI::Window *ctl) {
	return ctl->isVisible();
}

void MegaGlest_CEGUIManager::setControlReadOnly(CEGUI::Window *ctl, bool readOnly) {

	if(dynamic_cast<CEGUI::Combobox*>(ctl)) {
		CEGUI::Combobox *combobox = dynamic_cast<CEGUI::Combobox*>(ctl);
		combobox->setReadOnly(readOnly);
	}
//	else if(dynamic_cast<CEGUI::MultiColumnList*>(ctl)) {
//		CEGUI::MultiColumnList *ctlType = dynamic_cast<CEGUI::MultiColumnList*>(ctl);
//		ctlType->setReadOnly(readOnly);
//	}
//	else if(dynamic_cast<CEGUI::ToggleButton*>(ctl)) {
//		CEGUI::ToggleButton *ctlType = dynamic_cast<CEGUI::ToggleButton*>(ctl);
//		ctlType->setReadOnly(readOnly);
//	}
//	else if(dynamic_cast<CEGUI::Scrollbar*>(ctl)) {
//		CEGUI::Scrollbar *ctlType = dynamic_cast<CEGUI::Scrollbar*>(ctl);
//		ctlType->setReadOnly(readOnly);
//	}
//	else if(dynamic_cast<CEGUI::TabControl*>(ctl)) {
//		CEGUI::TabControl *ctlType = dynamic_cast<CEGUI::TabControl*>(ctl);
//		ctlType->setReadOnly(readOnly);
//	}
//	else if(dynamic_cast<CEGUI::Listbox*>(ctl)) {
//		CEGUI::Listbox *ctlType = dynamic_cast<CEGUI::Listbox*>(ctl);
//		ctlType->setReadOnly(readOnly);
//	}
//	else if(dynamic_cast<CEGUI::Spinner*>(ctl)) {
//		CEGUI::Spinner *ctlType = dynamic_cast<CEGUI::Spinner*>(ctl);
//		ctlType->setReadOnly(readOnly);
//	}
//	else if(dynamic_cast<CEGUI::Scrollbar*>(ctl)) {
//		CEGUI::Scrollbar *ctlType = dynamic_cast<CEGUI::Spinner*>(ctl);
//		ctlType->setReadOnly(readOnly);
//	}
	else if(dynamic_cast<CEGUI::Editbox*>(ctl)) {
		CEGUI::Editbox *ctlType = dynamic_cast<CEGUI::Editbox*>(ctl);
		ctlType->setReadOnly(readOnly);
	}
}

void MegaGlest_CEGUIManager::setControlEnabled(CEGUI::Window *ctl, bool enabled) {
	ctl->setEnabled(enabled);
}

bool MegaGlest_CEGUIManager::getControlEnabled(CEGUI::Window *ctl) {
	return ctl->isDisabled() == false;
}

void MegaGlest_CEGUIManager::setColumnsForMultiColumnListControl(CEGUI::Window *ctl, vector<pair<string, float> > columnValues) {
	CEGUI::MultiColumnList *listbox = static_cast<CEGUI::MultiColumnList*>(ctl);

	for(int index = listbox->getColumnCount()-1; index >= 0; --index) {
		listbox->removeColumn(index);
	}
	// Add some column headers
	for(unsigned int index = 0; index < columnValues.size(); ++index) {
		pair<string, float> &columnInfo = columnValues[index];
		CEGUI::String cegui_value((CEGUI::encoded_char*)columnInfo.first.c_str());
		listbox->addColumn(cegui_value,index,cegui_absdim(columnInfo.second));
	}
}

void MegaGlest_CEGUIManager::addItemToMultiColumnListControl(CEGUI::Window *ctl, vector<string> columnValues, int id, bool disableFormatting) {
	CEGUI::MultiColumnList *listbox = static_cast<CEGUI::MultiColumnList*>(ctl);
//	bool wasReadOnly = listbox->isReadOnly();
//	if(wasReadOnly == true) {
//		listbox->setReadOnly(false);
//	}

	unsigned int rowId = listbox->addRow();

	for(unsigned int index = 0; index < columnValues.size(); ++index) {
		CEGUI::String cegui_value((CEGUI::encoded_char*)columnValues[index].c_str());
		//printf("\nCE-GUI add item to combobox: [%s] [%s]\n",cegui_value.c_str(),value.c_str());

		CEGUI::ListboxTextItem *itemCombobox = new CEGUI::ListboxTextItem(cegui_value, id);

		string selectionImageName = getLookName() + "/MultiListSelectionBrush";
		const CEGUI::Image *selectionImage = &CEGUI::ImageManager::getSingleton().get(selectionImageName);
		itemCombobox->setSelectionBrushImage(selectionImage);
		itemCombobox->setTextParsingEnabled(disableFormatting == false);

		listbox->setTextParsingEnabled(disableFormatting == false);

		//listbox->addItem(itemCombobox);
		listbox->setItem(itemCombobox,index,rowId);
	}
//	if(wasReadOnly == true) {
//		listbox->setReadOnly(true);
//	}
}

void MegaGlest_CEGUIManager::addItemsToMultiColumnListControl(CEGUI::Window *ctl, vector<vector<string> > valueList, bool disableFormatting) {
	CEGUI::MultiColumnList *listbox = static_cast<CEGUI::MultiColumnList*>(ctl);
	int previousItemCount = listbox->getRowCount();

	listbox->resetList();
	for(unsigned int index = 0; index < valueList.size(); ++index) {

		vector<string> &columnValues = valueList[index];
		int position = previousItemCount + index;
		addItemToMultiColumnListControl(ctl, columnValues, position, disableFormatting);
	}
}

void MegaGlest_CEGUIManager::setSelectedItemTextForMultiColumnListControl(CEGUI::Window *ctl, string value, int id, bool disableFormatting) {
	CEGUI::MultiColumnList *listbox = static_cast<CEGUI::MultiColumnList*>(ctl);


	CEGUI::ListboxItem *row 	= listbox->getFirstSelectedItem();
	CEGUI::MCLGridRef cellRef 	= listbox->getItemGridReference(row);
	cellRef.column 				= id;
	CEGUI::ListboxItem *cell 	= listbox->getItemAtGridReference(cellRef);

	//row->setTextParsingEnabled(disableFormatting == false);
	CEGUI::String cegui_value((CEGUI::encoded_char*)value.c_str());
	cell->setText(cegui_value);
	listbox->handleUpdatedItemData();
}

int MegaGlest_CEGUIManager::getSelectedRowForMultiColumnListControl(CEGUI::Window *ctl) {
	CEGUI::MultiColumnList *listbox = static_cast<CEGUI::MultiColumnList*>(ctl);

	int rowIndex = -1;
	CEGUI::ListboxItem *row = listbox->getFirstSelectedItem();
	if(row != NULL) {
		CEGUI::MCLGridRef cellRef = listbox->getItemGridReference(row);
		rowIndex = cellRef.row;
	}
	return rowIndex;
}

void MegaGlest_CEGUIManager::printDebugControlInfo(CEGUI::Window *ctl) {
	printf("ctl [%p] name [%s]\n",ctl,ctl->getNamePath().c_str());
}

}}//end namespace

