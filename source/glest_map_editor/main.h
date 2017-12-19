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

#ifndef _MAPEDITOR_MAIN_H_
#define _MAPEDITOR_MAIN_H_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

#include <string>
#include <vector>

#include <wx/wx.h>
#include <wx/glcanvas.h>

#include "program.h"
#include "util.h"
#include "platform_common.h"
#include "icons.h"

using std::string;
using std::vector;
using std::pair;
using namespace Shared::PlatformCommon;

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
	siPOS_VALUE,
	siCOUNT
};

struct obj
{
  const char *brushDesc;
  const char **brush;
  const char *objDesc;
};

#define OBJECT_COUNT 11

struct obj objects[OBJECT_COUNT] = {
  { _("brush_none"), brush_none, _("None") },
  { _("brush_tree"), brush_object_tree, _("Tree (unwalkable/harvestable)") },
  { _("brush_dead_tree"), brush_object_dead_tree, _("Dead tree/Cactuses/Thornbush (unwalkable)") },
  { _("brush_stone"), brush_object_stone, _("Stone (unwalkable/not harvestable)") },
  { _("brush_bush"), brush_object_bush, _("Bush/Grass/Fern (walkable)") },
  { _("brush_water"), brush_object_water_object, _("Water object/Reed/Papyrus (walkable)") },
  { _("brush_c1_bigtree"), brush_object_c1_bigtree, _("Big tree/Old palm (unwalkable/not harvestable)") },
  { _("brush_c2_hanged"), brush_object_c2_hanged, _("Hanged/Impaled (unwalkable)") },
  { _("brush_c3_statue"), brush_object_c3_statue, _("Statues (unwalkable)") },
  { _("brush_c4_bigrock"), brush_object_c4_bigrock, _("Mountain (unwalkable)") },
  { _("brush_c5_blocking"), brush_object_c5_blocking, _("Invisible blocking object (unwalkable)") }
};

const char *resource_descs[] = {
	"None (Erase)", "Gold", "Stone", "Custom 4", "Custom 5", "Custom 6"
};


const char *surface_descs[] = {
	"Grass", "Sec. grass", "Road", "Stone", "Ground"
};


class MainToolBar : public wxToolBar {
private:
	DECLARE_EVENT_TABLE()

public:

    MainToolBar(wxWindow *parent,
              wxWindowID id) : wxToolBar(parent,id) {}

	void onMouseMove(wxMouseEvent &event);
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
	static const int objectCount = OBJECT_COUNT;
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
		miEditFlipDiagonal,
		miEditFlipX,
		miEditFlipY,

		miEditMirrorX,
		miEditMirrorY,
		miEditMirrorXY,
		miEditRotatecopyX,
		miEditRotatecopyY,
		miEditRotatecopyXY,
		miEditRotatecopyCorner,
		miEditMirror,

		miEditRandomizeHeights,
		miEditRandomize,
		miEditSwitchSurfaces,
		miEditInfo,
		miEditAdvanced,

		miViewResetZoomAndPos,
		miViewGrid,
		miViewHeightMap,
		miHideWater,
		miViewAbout,
		miViewHelp,

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

	wxMenuBar *menuBar;
	wxMenu *menuFile;
	wxMenu *menuEdit;
	wxMenu *menuEditMirror;
	wxMenu *menuView;
	wxMenu *menuBrush;
	wxMenu *menuBrushHeight;
	wxMenu *menuBrushGradient;

	wxMenu *menuBrushSurface;
	wxMenu *menuBrushObject;
	wxMenu *menuBrushResource;
	wxMenu *menuBrushStartLocation;
	wxMenu *menuRadius;

	wxFileDialog *fileDialog;

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

	bool randomWithReset;
	int randomMinimumHeight;
	int randomMaximumHeight;
	int randomChanceDivider;
	int randomRecursions;

	ChangeType enabledGroup;

	string fileName;
	bool fileModified;
	Chrono lastPaintEvent;

	wxBoxSizer *boxsizer;

	bool startupSettingsInited;
	string appPath;

public:
	explicit MainWindow(string appPath);
	~MainWindow();

	void refreshMapRender();
	void init(string fname);

	void onClose(wxCloseEvent &event);

	void onMouseDown(wxMouseEvent &event, int x, int y);
	void onMouseMove(wxMouseEvent &event, int x, int y);
	void onMouseWheelDown(wxMouseEvent &event);
	void onMouseWheelUp(wxMouseEvent &event);

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
	void onMenuEditFlipDiagonal(wxCommandEvent &event);
	void onMenuEditFlipX(wxCommandEvent &event);
	void onMenuEditFlipY(wxCommandEvent &event);

	void onMenuEditMirrorX(wxCommandEvent &event); // copy left to right
	void onMenuEditMirrorY(wxCommandEvent &event); // copy top to bottom
	void onMenuEditMirrorXY(wxCommandEvent &event); // copy bottomleft to topright
	void onMenuEditRotatecopyX(wxCommandEvent &event); // copy left to right, rotated
	void onMenuEditRotatecopyY(wxCommandEvent &event); // copy top to bottom, rotated
	void onMenuEditRotatecopyXY(wxCommandEvent &event);  // copy bottomleft to topright, rotated
	void onMenuEditRotatecopyCorner(wxCommandEvent &event); // copy top left 1/4 to top right 1/4, rotated

	void onMenuEditRandomizeHeights(wxCommandEvent &event);
	void onMenuEditRandomize(wxCommandEvent &event);
	void onMenuEditSwitchSurfaces(wxCommandEvent &event);
	void onMenuEditInfo(wxCommandEvent &event);
	void onMenuEditAdvanced(wxCommandEvent &event);

	void onMenuViewResetZoomAndPos(wxCommandEvent &event);
	void onMenuViewGrid(wxCommandEvent &event);
	void onMenuViewHeightMap(wxCommandEvent &event);
	void onMenuHideWater(wxCommandEvent &event);
	void onMenuViewAbout(wxCommandEvent &event);
	void onMenuViewHelp(wxCommandEvent &event);

	void onMenuBrushHeight(wxCommandEvent &event);
	void onMenuBrushGradient(wxCommandEvent &event);
	void onMenuBrushSurface(wxCommandEvent &event);
	void onMenuBrushObject(wxCommandEvent &event);
	void onMenuBrushResource(wxCommandEvent &event);
	void onMenuBrushStartLocation(wxCommandEvent &event);
	void onMenuRadius(wxCommandEvent &event);

	void onToolPlayer(wxCommandEvent &event);

	void change(int x, int y);

	void uncheckBrush();
	void uncheckRadius();

private:

	bool isDirty() const	{ return fileModified; }
	void setDirty(bool val=true);
	void setExtension();
	void setupStartupSettings();
	void initGlCanvas();
};

// =====================================================
//	class GlCanvas
// =====================================================

class GlCanvas: public wxGLCanvas {
private:
	DECLARE_EVENT_TABLE()

public:
	GlCanvas(MainWindow *mainWindow, wxWindow *parent, int *args);
	~GlCanvas();

	void onMouseDown(wxMouseEvent &event);
	void onMouseMove(wxMouseEvent &event);
	void onMouseWheel(wxMouseEvent &event);
	void onKeyDown(wxKeyEvent &event);
    void onPaint(wxPaintEvent &event);

    void setCurrentGLContext();
private:
	MainWindow *mainWindow;
	wxGLContext *context;
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
	void addValue(const string &key, const string &value, const string &help="");
	string getValue(const string &key);

	bool show(const string &title="Edit values", bool wide=false);
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
	App() : wxApp() {
		mainWindow = NULL;
	}
	virtual ~App() {}
	virtual bool OnInit();
	virtual int MainLoop();
	virtual int OnExit();
};

}// end namespace

DECLARE_APP(MapEditor::App)

#endif
