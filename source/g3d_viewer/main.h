#ifndef _SHADER_G3DVIEWER_MAIN_H_
#define _SHADER_G3DVIEWER_MAIN_H_

#include <string>

#include <wx/wx.h>
#include <wx/glcanvas.h>

#include "renderer.h"
//#include "util.h"

//using Shared::Platform::Window;
//using Shared::Platform::MouseState;

using std::string;

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
	string modelPath;

	float speed;
	float anim;
	float rotX, rotY, zoom;
	int lastX, lastY;
	Renderer::PlayerColor playerColor;

public:
	MainWindow(const string &modelPath);
	~MainWindow();
	void init();

	void Notify();

	void onPaint(wxPaintEvent &event);
	void onClose(wxCloseEvent &event);
	void onMenuFileLoad(wxCommandEvent &event);
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
