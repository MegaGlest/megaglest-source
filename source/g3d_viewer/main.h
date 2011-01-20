#ifndef _SHADER_G3DVIEWER_MAIN_H_
#define _SHADER_G3DVIEWER_MAIN_H_

#include <string>

#include <wx/wx.h>
#include <wx/glcanvas.h>

#include "renderer.h"
#include "util.h"
#include "particle_type.h"
#include "unit_particle_type.h"

using std::string;
using namespace Glest::Game;

namespace Shared{ namespace G3dViewer{

class GlCanvas;

// ===============================
// 	class MainWindow
// ===============================

class MainWindow: public wxFrame{
private:
	DECLARE_EVENT_TABLE()

public:
	static const string versionString;
	static const string winHeader;

	enum MenuId{
		miFileLoad,
		miFileLoadParticleXML,
		miFileLoadProjectileParticleXML,
		miFileLoadSplashParticleXML,
		miFileClearAll,
		miModeWireframe,
		miModeNormals,
		miModeGrid,
		miSpeedSlower,
		miSpeedFaster,
		miRestart,
		miColorRed,
		miColorBlue,
		miColorGreen,
		miColorYellow,
		miColorWhite,
		miColorCyan,
		miColorOrange,
		miColorMagenta
	};

private:
	GlCanvas *glCanvas;
	Renderer *renderer;

	wxTimer *timer;

	wxMenuBar *menu;
	wxMenu *menuFile;
	wxMenu *menuMode;
	wxMenu *menuSpeed;
	wxMenu *menuCustomColor;
	wxFileDialog *fileDialog;

	Model *model;

	std::vector<string> modelPathList;
	std::vector<string> particlePathList;
	std::vector<string> particleProjectilePathList;
	std::vector<string> particleSplashPathList; // as above

	float speed;
	float anim;
	float rotX, rotY, zoom;
	float backBrightness, gridBrightness, lightBrightness;
	int lastX, lastY;
	Renderer::PlayerColor playerColor;

	std::vector<UnitParticleSystemType *> unitParticleSystemTypes;
	std::vector<UnitParticleSystem *> unitParticleSystems;

	std::vector<ParticleSystemTypeProjectile *> projectileParticleSystemTypes;
	std::vector<ProjectileParticleSystem *> projectileParticleSystems;
	std::vector<ParticleSystemTypeSplash *> splashParticleSystemTypes; // as above
	std::vector<SplashParticleSystem *> splashParticleSystems;
	string statusbarText;

	bool isControlKeyPressed;
	void loadModel(string path);
	void loadParticle(string path);
	void loadProjectileParticle(string path);
	void loadSplashParticle(string path);

public:
	MainWindow(const string &modelPath);
	~MainWindow();
	void init();

	void Notify();

	void onPaint(wxPaintEvent &event);
	void onClose(wxCloseEvent &event);
	void onMenuFileLoad(wxCommandEvent &event);
	void onMenuFileLoadParticleXML(wxCommandEvent &event);
	void onMenuFileLoadProjectileParticleXML(wxCommandEvent &event);
	void onMenuFileLoadSplashParticleXML(wxCommandEvent &event);
	void onMenuFileClearAll(wxCommandEvent &event);
	void onMenuFileExit(wxCommandEvent &event);
	void onMenuModeNormals(wxCommandEvent &event);
	void onMenuModeWireframe(wxCommandEvent &event);
	void onMenuModeGrid(wxCommandEvent &event);
	void onMenuSpeedSlower(wxCommandEvent &event);
	void onMenuSpeedFaster(wxCommandEvent &event);
	void onMenuRestart(wxCommandEvent &event);
	void onMenuColorRed(wxCommandEvent &event);
	void onMenuColorBlue(wxCommandEvent &event);
	void onMenuColorGreen(wxCommandEvent &event);
	void onMenuColorYellow(wxCommandEvent &event);
	void onMenuColorWhite(wxCommandEvent &event);
	void onMenuColorCyan(wxCommandEvent &event);
	void onMenuColorOrange(wxCommandEvent &event);
	void onMenuColorMagenta(wxCommandEvent &event);
	void onMouseWheelDown(wxMouseEvent &event);
	void onMouseWheelUp(wxMouseEvent &event);
	void onMouseMove(wxMouseEvent &event);
	void onTimer(wxTimerEvent &event);

	void onKeyDown(wxKeyEvent &e);

	string getModelInfo();
};

// =====================================================
//	class GlCanvas
// =====================================================

class GlCanvas: public wxGLCanvas {
private:
	DECLARE_EVENT_TABLE()

public:
	GlCanvas(MainWindow *mainWindow);
	GlCanvas(MainWindow *mainWindow, int *args);

	void onMouseWheel(wxMouseEvent &event);
	void onMouseMove(wxMouseEvent &event);
	void onPaint(wxPaintEvent &event);
	void onKeyDown(wxKeyEvent &event);

private:
	MainWindow *mainWindow;
};


// ===============================
// 	class App
// ===============================

class App: public wxApp{
private:
	MainWindow *mainWindow;

public:
	virtual bool OnInit();
	virtual int MainLoop();
	virtual int OnExit();
};

}}//end namespace

DECLARE_APP(Shared::G3dViewer::App)

#endif
