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
		miFileClearAll,
		miModeWireframe,
		miModeNormals,
		miModeGrid,
		miSpeedSlower,
		miSpeedFaster,
		miColorRed,
		miColorBlue,
		miColorYellow,
		miColorGreen
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

	Model *model;

	std::vector<string> modelPathList;
	std::vector<string> particlePathList;
	std::vector<string> particleProjectilePathList;

	float speed;
	float anim;
	float rotX, rotY, zoom;
	int lastX, lastY;
	Renderer::PlayerColor playerColor;

	std::vector<UnitParticleSystemType *> unitParticleSystemTypes;
	std::vector<UnitParticleSystem *> unitParticleSystems;

	std::vector<ParticleSystemTypeProjectile *> projectileParticleSystemTypes;
	std::vector<ProjectileParticleSystem *> projectileParticleSystems;

	bool isControlKeyPressed;
	void loadModel(string path);
	void loadParticle(string path);
	void loadProjectileParticle(string path);

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
	void onMenuFileClearAll(wxCommandEvent &event);
	void onMenuModeNormals(wxCommandEvent &event);
	void onMenuModeWireframe(wxCommandEvent &event);
	void onMenuModeGrid(wxCommandEvent &event);
	void onMenuSpeedSlower(wxCommandEvent &event);
	void onMenuSpeedFaster(wxCommandEvent &event);
	void onMenuColorRed(wxCommandEvent &event);
	void onMenuColorBlue(wxCommandEvent &event);
	void onMenuColorYellow(wxCommandEvent &event);
	void onMenuColorGreen(wxCommandEvent &event);
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
