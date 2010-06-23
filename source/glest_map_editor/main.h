// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _MAPEDITOR_MAIN_H_
#define _MAPEDITOR_MAIN_H_

#include <string>
#include <vector>

#include <wx/wx.h>
#include <wx/glcanvas.h>

#include "program.h"
#include "util.h"

using std::string;
using std::vector;
using std::pair;

namespace MapEditor {

class GlCanvas;

enum BrushType {
	btHeight,
	btGradient,
	btSurface,
	btObject,
	btResource,
	btStartLocation
};

enum StatusItems {
	siNULL_ENTRY,
	siFILE_NAME,
	siFILE_TYPE,
	siCURR_OBJECT,
	siBRUSH_TYPE,
	siBRUSH_VALUE,
	siBRUSH_RADIUS,
	siCOUNT
};

const char *object_descs[] = {
	"None (Erase)",
	"Tree",
	"Dead Tree",
	"Stone",
	"Bush",
	"Water Object",
	"Big/Dead Tree",
	"Trophy Corpse",
	"Statues",
	"Big Rock",
	"Invisible Blocking"
};

const char *resource_descs[] = {
	"None (Erase)", "Gold", "Stone", "Custom", "Custom", "Custom"
};


const char *surface_descs[] = {
	"Grass", "Alt. Grass", "Road", "Stone", "Ground"
};

// =====================================================
//	class MainWindow
// =====================================================

class MainWindow: public wxFrame {
private:
	DECLARE_EVENT_TABLE()

private:
	static const string versionString;
	static const string winHeader;
	static const int heightCount = 11;
	static const int surfaceCount = 5;
	static const int objectCount = 11;
	static const int resourceCount = 6;
	static const int startLocationCount = 8;
	static const int radiusCount = 9;

private:
	enum MenuId {
		miFileLoad,
		miFileSave,
		miFileSaveAs,
		miFileExit,

		miEditUndo,
		miEditRedo,
		miEditReset,
		miEditResetPlayers,
		miEditResize,
		miEditFlipX,
		miEditFlipY,
		miEditRandomizeHeights,
		miEditRandomize,
		miEditSwitchSurfaces,
		miEditInfo,
		miEditAdvanced,

		miMiscResetZoomAndPos,
		miMiscAbout,
		miMiscHelp,

		toolPlayer,

		miBrushHeight,
		miBrushGradient = miBrushHeight + heightCount + 1,
		miBrushSurface = miBrushGradient + heightCount + 1,
		miBrushObject = miBrushSurface + surfaceCount + 1,
		miBrushResource = miBrushObject + objectCount + 1,
		miBrushStartLocation = miBrushResource + resourceCount + 1,

		miRadius = miBrushStartLocation + startLocationCount + 1
	};

private:
	GlCanvas *glCanvas;
	Program *program;
	int lastX, lastY;

	wxPanel *panel;
	
	wxTimer *timer;

	wxMenuBar *menuBar;
	wxMenu *menuFile;
	wxMenu *menuEdit;
	wxMenu *menuMisc;
	wxMenu *menuBrush;
	wxMenu *menuBrushHeight;
	wxMenu *menuBrushGradient;

	wxMenu *menuBrushSurface;
	wxMenu *menuBrushObject;
	wxMenu *menuBrushResource;
	wxMenu *menuBrushStartLocation;
	wxMenu *menuRadius;

	string currentFile;

	BrushType currentBrush;
	int height;
	int surface;
	int radius;
	int object;
	int resource;
	int startLocation;
	int resourceUnderMouse;
	int objectUnderMouse;
	
	ChangeType enabledGroup;

	string fileName;
	bool fileModified;

public:
	MainWindow();
	~MainWindow();

	void init(string fname);

	void onClose(wxCloseEvent &event);

	void onMouseDown(wxMouseEvent &event, int x, int y);
	void onMouseMove(wxMouseEvent &event, int x, int y);

	void onPaint(wxPaintEvent &event);
	void onKeyDown(wxKeyEvent &e);

	void onMenuFileLoad(wxCommandEvent &event);
	void onMenuFileSave(wxCommandEvent &event);
	void onMenuFileSaveAs(wxCommandEvent &event);
	void onMenuFileExit(wxCommandEvent &event);

	void onMenuEditUndo(wxCommandEvent &event);
	void onMenuEditRedo(wxCommandEvent &event);
	void onMenuEditReset(wxCommandEvent &event);
	void onMenuEditResetPlayers(wxCommandEvent &event);
	void onMenuEditResize(wxCommandEvent &event);
	void onMenuEditFlipX(wxCommandEvent &event);
	void onMenuEditFlipY(wxCommandEvent &event);
	void onMenuEditRandomizeHeights(wxCommandEvent &event);
	void onMenuEditRandomize(wxCommandEvent &event);
	void onMenuEditSwitchSurfaces(wxCommandEvent &event);
	void onMenuEditInfo(wxCommandEvent &event);
	void onMenuEditAdvanced(wxCommandEvent &event);

	void onMenuMiscResetZoomAndPos(wxCommandEvent &event);
	void onMenuMiscAbout(wxCommandEvent &event);
	void onMenuMiscHelp(wxCommandEvent &event);

	void onMenuBrushHeight(wxCommandEvent &event);
	void onMenuBrushGradient(wxCommandEvent &event);
	void onMenuBrushSurface(wxCommandEvent &event);
	void onMenuBrushObject(wxCommandEvent &event);
	void onMenuBrushResource(wxCommandEvent &event);
	void onMenuBrushStartLocation(wxCommandEvent &event);
	void onMenuRadius(wxCommandEvent &event);
	
	void onToolPlayer(wxCommandEvent &event);

	void onTimer(wxTimerEvent &event);

	void change(int x, int y);

	void uncheckBrush();
	void uncheckRadius();

private:
	bool isDirty() const	{ return fileModified; }
	void setDirty(bool val=true);
	void setExtension();
};

// =====================================================
//	class GlCanvas
// =====================================================

class GlCanvas: public wxGLCanvas {
private:
	DECLARE_EVENT_TABLE()

public:
	GlCanvas(MainWindow *mainWindow, wxWindow *parent, int *args);

	void onMouseDown(wxMouseEvent &event);
	void onMouseMove(wxMouseEvent &event);
	void onKeyDown(wxKeyEvent &event);

private:
	MainWindow *mainWindow;
};

// =====================================================
//	class SimpleDialog
// =====================================================

class SimpleDialog: public wxDialog {
private:
	typedef vector<pair<string, string> > Values;

private:
	Values values;

public:
	void addValue(const string &key, const string &value);
	string getValue(const string &key);

	void show();
};

// =====================================================
//	class SimpleDialog
// =====================================================

class MsgDialog: public wxDialog {

private:

	wxSizer *m_sizerText;

public:
	MsgDialog(wxWindow *parent,
                     const wxString& message,
                     const wxString& caption,
                     long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxOK | wxCENTRE,
                     const wxPoint& pos = wxDefaultPosition);
	virtual ~MsgDialog();
};

// =====================================================
//	class App
// =====================================================

// =====================================================
//	class App
// =====================================================

class App: public wxApp {
private:
	MainWindow *mainWindow;

public:
	virtual bool OnInit();
	virtual int MainLoop();
	virtual int OnExit();
};

}// end namespace

DECLARE_APP(MapEditor::App)

#endif
