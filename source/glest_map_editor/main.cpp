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

#include "main.h"
#include <ctime>
#include "conversion.h"
#include "icons.h"
#include "platform_common.h"
#include "config.h"
#include <iostream>
#include"platform_util.h"
#include <wx/stdpaths.h>
#include "wx/image.h"
#ifndef WIN32
#include <errno.h>
#endif
#include "common_scoped_ptr.h"
//#include <memory>

using namespace Shared::Util;
using namespace Shared::PlatformCommon;
using namespace Glest::Game;
using namespace std;

namespace Glest { namespace Game {
string getGameReadWritePath(const string &lookupKey) {
	string path = "";
    if(path == "" && getenv("GLESTHOME") != NULL) {
        path = getenv("GLESTHOME");
        if(path != "" && EndsWith(path, "/") == false && EndsWith(path, "\\") == false) {
            path += "/";
        }

        //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path to be used for read/write files [%s]\n",__FILE__,__FUNCTION__,__LINE__,path.c_str());
    }

    return path;
}
}}

namespace MapEditor {

const string mapeditorVersionString = "v3.13-dev";
const string MainWindow::winHeader = "MegaGlest Map Editor " + mapeditorVersionString;

// ===============================================
//	class Global functions
// ===============================================

wxString ToUnicode(const char* str) {
	return wxString(str, wxConvUTF8);
}

wxString ToUnicode(const string& str) {
	return wxString(str.c_str(), wxConvUTF8);
}

// ===============================================
// class MainWindow
// ===============================================

MainWindow::MainWindow(string appPath)
		: wxFrame(NULL, -1,  ToUnicode(winHeader), wxPoint(0,0), wxSize(1024, 768))
		, glCanvas(NULL)
		, program(NULL)
		, lastX(0)
		, lastY(0)
		, panel(NULL)
		, menuBar(NULL)
		, fileDialog(NULL)

		, currentBrush(btHeight)
		, height(0)
		, surface(1)
		, radius(1)
		, object(0)
		, resource(0)
		, startLocation(1)
		, enabledGroup(ctHeight)
		, fileModified(false)
        , boxsizer(NULL)
        ,startupSettingsInited(false) {

	menuFile=NULL;
	menuEdit=NULL;
	menuEditMirror=NULL;
	menuView=NULL;
	menuBrush=NULL;
	menuBrushHeight=NULL;
	menuBrushGradient=NULL;
	menuBrushSurface=NULL;
	menuBrushObject=NULL;
	menuBrushResource=NULL;
	menuBrushStartLocation=NULL;
	menuRadius=NULL;
	fileDialog=NULL;
	resourceUnderMouse=0;
	objectUnderMouse=0;

    shiftModifierKey = false;

	// default values for random height calculation that turned out to be quite useful
	randomWithReset=true;
	randomMinimumHeight=-300;
	randomMaximumHeight=400;
	randomChanceDivider=30;
	randomRecursions=3;

	this->appPath = appPath;
	Properties::setApplicationPath(executable_path(appPath));

	this->panel = new wxPanel(this, wxID_ANY);
}

void MainWindow::onToolPlayer(wxCommandEvent& event){
	PopupMenu(menuBrushStartLocation);
}

void MainToolBar::onMouseMove(wxMouseEvent &event) {
#ifdef WIN32
	if(this->GetParent() != NULL && this->GetParent()->GetParent() != NULL) {
		MainWindow *mainWindow = dynamic_cast<MainWindow *>(this->GetParent()->GetParent());
		if(mainWindow != NULL) {
			mainWindow->refreshMapRender();
		}
	}
#endif
	event.Skip();
}

BEGIN_EVENT_TABLE(MainToolBar, wxToolBar)

	EVT_MOTION(MainToolBar::onMouseMove)

END_EVENT_TABLE()

void MainWindow::init(string fname) {

	//menus
	menuBar = new wxMenuBar();

	//file
	menuFile = new wxMenu();
	menuFile->Append(wxID_NEW);
	menuFile->Append(wxID_OPEN);
	menuFile->AppendSeparator();
	menuFile->Append(wxID_SAVE);
	menuFile->Append(wxID_SAVEAS);
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT);
	menuBar->Append(menuFile, wxT("&File"));

	//edit
    menuEdit = new wxMenu();
    menuEdit->Append(miEditUndo, wxT("&Undo\tCTRL+Z"));
	menuEdit->Append(miEditRedo, wxT("&Redo\tCTRL+Y"));
	menuEdit->AppendSeparator();
//	menuEdit->Append(miEditReset, wxT("Rese&t..."));
	menuEdit->Append(miEditResetPlayers, wxT("Reset &Players..."));
	menuEdit->Append(miEditResize, wxT("Re&size..."));
	menuEdit->Append(miEditFlipDiagonal, wxT("Flip &Diagonal"));
	menuEdit->Append(miEditFlipX, wxT("Flip &X"));
	menuEdit->Append(miEditFlipY, wxT("Flip &Y"));

    // Mirror submenu-------------------------------------------
	menuEditMirror = new wxMenu();
	menuEditMirror->Append(miEditMirrorX, wxT("Copy &Left to Right"));
	menuEditMirror->Append(miEditMirrorY, wxT("Copy &Top to Bottom"));
	menuEditMirror->Append(miEditMirrorXY, wxT("Copy &BottomLeft to TopRight"));
	menuEditMirror->AppendSeparator();
	menuEditMirror->Append(miEditRotatecopyX, wxT("&Rotate Left to Right"));
	menuEditMirror->Append(miEditRotatecopyY, wxT("Rotate T&op to Bottom"));
	menuEditMirror->Append(miEditRotatecopyXY, wxT("Rotate Botto&mLeft to TopRight"));
	menuEditMirror->Append(miEditRotatecopyCorner, wxT("Rotate TopLeft &corner to TopRight"));
    menuEdit->Append(miEditMirror, wxT("&Mirror"), menuEditMirror);
    // ---------------------------------------------------------

	menuEdit->Append(miEditRandomizeHeights, wxT("Randomize &Heights"));
    menuEdit->Append(miEditImportHeights, wxT("Import Heightsmap"));
    menuEdit->Append(miEditExportHeights, wxT("Export Heightsmap"));
	menuEdit->Append(miEditRandomize, wxT("Randomi&ze Players"));
	menuEdit->Append(miEditSwitchSurfaces, wxT("Switch Sur&faces..."));
	menuEdit->Append(miEditInfo, wxT("&Info..."));
	menuEdit->Append(miEditAdvanced, wxT("&Advanced..."));
	menuBar->Append(menuEdit, wxT("&Edit"));

	//view
	menuView = new wxMenu();
	menuView->Append(miViewResetZoomAndPos, wxT("&Reset zoom and pos"));
    menuView->AppendCheckItem(miViewGrid, wxT("&Grid"));
	menuView->AppendCheckItem(miViewHeightMap, wxT("H&eightMap"));
	menuView->AppendCheckItem(miHideWater, wxT("Hide&Water"));
	menuView->AppendSeparator();
	menuView->Append(miViewHelp, wxT("&Help..."));
	menuView->Append(miViewAbout, wxT("&About..."));
	menuBar->Append(menuView, wxT("&View"));

	//brush
	// menuBrush = new wxMenu();

	//surface
	menuBrushSurface = new wxMenu();
	menuBrushSurface->AppendCheckItem(miBrushSurface + 1, wxT("&Grass"));
	menuBrushSurface->AppendCheckItem(miBrushSurface + 2, wxT("S&econdary grass"));
	menuBrushSurface->AppendCheckItem(miBrushSurface + 3, wxT("&Road"));
	menuBrushSurface->AppendCheckItem(miBrushSurface + 4, wxT("&Stone"));
	menuBrushSurface->AppendCheckItem(miBrushSurface + 5, wxT("Gr&ound"));
	menuBar->Append(menuBrushSurface, wxT("&Surface"));

	//resources
	menuBrushResource = new wxMenu();
	//menuBrushResource->AppendCheckItem(miBrushResource + 1, wxT("&0 - None"));
	menuBrushResource->AppendCheckItem(miBrushResource+2, wxT("&Gold  (unwalkable)"));
	menuBrushResource->AppendCheckItem(miBrushResource+3, wxT("&Stone (unwalkable)"));
	menuBrushResource->AppendCheckItem(miBrushResource+4, wxT("&3 - custom"));
	menuBrushResource->AppendCheckItem(miBrushResource+5, wxT("&4 - custom"));
	menuBrushResource->AppendCheckItem(miBrushResource+6, wxT("&5 - custom"));
	menuBar->Append(menuBrushResource, wxT("&Resource"));

	//objects
	menuBrushObject = new wxMenu();
	menuBrushObject->AppendCheckItem(miBrushObject+1, wxT("&None (erase)\tALT+0"));
	menuBrushObject->AppendSeparator();
	menuBrushObject->AppendCheckItem(miBrushObject+2, wxT("&Tree (harvestable)"));
	menuBrushObject->AppendCheckItem(miBrushObject+3, wxT("&Dead tree/Cactuses/Thornbush"));
	menuBrushObject->AppendCheckItem(miBrushObject+4, wxT("&Stone (unharvestable)"));
	menuBrushObject->AppendCheckItem(miBrushObject+5, wxT("&Bush/Grass/Fern (walkable)"));
	menuBrushObject->AppendCheckItem(miBrushObject+6, wxT("&Water object/Reed/Papyrus (walkable)"));
	menuBrushObject->AppendCheckItem(miBrushObject+7, wxT("Big tree/&Old palm"));
	menuBrushObject->AppendCheckItem(miBrushObject+8, wxT("&Hanged/Impaled "));
	menuBrushObject->AppendCheckItem(miBrushObject+9, wxT("St&atues"));
	menuBrushObject->AppendCheckItem(miBrushObject+10, wxT("&Mountain"));
	menuBrushObject->AppendCheckItem(miBrushObject+11, wxT("&Invisible blocking object"));
	menuBar->Append(menuBrushObject, wxT("&Object"));

	// ZombiePirate height brush
	menuBrushGradient = new wxMenu();
	for (int i = 0; i < heightCount; ++i) {
		menuBrushGradient->AppendCheckItem(miBrushGradient + i + 1, ToUnicode((i>4?"&":"") +intToStr(i - heightCount / 2)));
	}
	menuBar->Append(menuBrushGradient, wxT("&Gradient"));

	// Glest height brush
	menuBrushHeight = new wxMenu();
	for (int i = 0; i < heightCount; ++i) {
		menuBrushHeight->AppendCheckItem(miBrushHeight + i + 1, ToUnicode((i>4?"&":"") +intToStr(i - heightCount / 2)));
	}
	menuBrushHeight->Check(miBrushHeight + (heightCount + 1) / 2, true);
	menuBar->Append(menuBrushHeight, wxT("&Height"));

	enabledGroup = ctHeight;

	//radius
	menuRadius = new wxMenu();
	for (int i = 1; i <= radiusCount; ++i) {
		menuRadius->AppendCheckItem(miRadius + i, ToUnicode("&" + intToStr(i) + " (diameter "+intToStr(i*2-1)+ ")"+ "\tALT+" + intToStr(i)));
	}
	menuRadius->Check(miRadius + 1, true);
	menuBar->Append(menuRadius, wxT("R&adius"));

	//players
	menuBrushStartLocation = new wxMenu();
	wxMenuItem *pmi1 = new wxMenuItem(menuBrushStartLocation, miBrushStartLocation + 1, wxT("Player &1 Red"));
	pmi1->SetBitmap(wxBitmap(brush_players_red));
	menuBrushStartLocation->Append(pmi1);
	wxMenuItem *pmi2 = new wxMenuItem(menuBrushStartLocation, miBrushStartLocation + 2, wxT("Player &2 Blue"));
	pmi2->SetBitmap(wxBitmap(brush_players_blue));
	menuBrushStartLocation->Append(pmi2);
	wxMenuItem *pmi3 = new wxMenuItem(menuBrushStartLocation, miBrushStartLocation + 3, wxT("Player &3 Green"));
	pmi3->SetBitmap(wxBitmap(brush_players_green));
	menuBrushStartLocation->Append(pmi3);
	wxMenuItem *pmi4 = new wxMenuItem(menuBrushStartLocation, miBrushStartLocation + 4, wxT("Player &4 Yellow"));
	pmi4->SetBitmap(wxBitmap(brush_players_yellow));
	menuBrushStartLocation->Append(pmi4);
	wxMenuItem *pmi5 = new wxMenuItem(menuBrushStartLocation, miBrushStartLocation + 5, wxT("Player &5 White"));
	pmi5->SetBitmap(wxBitmap(brush_players_white));
	menuBrushStartLocation->Append(pmi5);
	wxMenuItem *pmi6 = new wxMenuItem(menuBrushStartLocation, miBrushStartLocation + 6, wxT("Player &6 Cyan"));
	pmi6->SetBitmap(wxBitmap(brush_players_cyan));
	menuBrushStartLocation->Append(pmi6);
	wxMenuItem *pmi7 = new wxMenuItem(menuBrushStartLocation, miBrushStartLocation + 7, wxT("Player &7 Orange"));
	pmi7->SetBitmap(wxBitmap(brush_players_orange));
	menuBrushStartLocation->Append(pmi7);
	wxMenuItem *pmi8 = new wxMenuItem(menuBrushStartLocation, miBrushStartLocation + 8, wxT("Player &8 Pink")); // = Light Magenta :-)
	pmi8->SetBitmap(wxBitmap(brush_players_pink));
	menuBrushStartLocation->Append(pmi8);
	menuBar->Append(menuBrushStartLocation, wxT("&Player"));
	//menuBar->Append(menuBrush, wxT("&Brush"));


	SetMenuBar(menuBar);

	fileName = "New (unsaved) map";
	int status_widths[siCOUNT] = {
		10, // empty
		-25, // File name
		-6, // File type
		-20, // Current Object
		-14, // Brush Type
		-20, // Brush 'Value'
		-10, // Brush Radius
		-25, // Position
	};
	CreateStatusBar(siCOUNT);
	GetStatusBar()->SetStatusWidths(siCOUNT, status_widths);

	SetStatusText(wxT("File: ") + ToUnicode(fileName), siFILE_NAME);
	SetStatusText(wxT(".mgm"), siFILE_TYPE);
	SetStatusText(wxT("Object: None (Erase)"), siCURR_OBJECT);
	SetStatusText(wxT("Brush: Height"), siBRUSH_TYPE);
    SetStatusText(wxT("Type: Overwrite"), siBRUSH_OVERWRITE);
	SetStatusText(wxT("Value: 0"), siBRUSH_VALUE);
	SetStatusText(wxT("Radius: 1"), siBRUSH_RADIUS);
	SetStatusText(wxT("Pos (Ingame): 0"), siPOS_VALUE);

	wxToolBar *toolbar = new MainToolBar(this->panel, wxID_ANY);
	toolbar->AddTool(miEditUndo, _("undo"), wxBitmap(edit_undo), _("Undo"));
	toolbar->AddTool(miEditRedo, _("redo"), wxBitmap(edit_redo), _("Redo"));
	toolbar->AddTool(miEditRandomizeHeights, _("randomizeHeights"), wxBitmap(edit_randomize_heights), _("Randomize Heights"));
//	toolbar->AddTool(miEditRandomize, _("randomize"), wxBitmap(edit_randomize), _("Randomize"));
	toolbar->AddTool(miEditSwitchSurfaces, _("switch"), wxBitmap(edit_switch_surfaces), _("Switch Surfaces"));
	toolbar->AddSeparator();
	toolbar->AddTool(miBrushSurface + 1, _("brush_grass1"), wxBitmap(brush_surface_grass1), _("Grass"));
	toolbar->AddTool(miBrushSurface + 2, _("brush_grass2"), wxBitmap(brush_surface_grass2), _("Secondary grass"));
	toolbar->AddTool(miBrushSurface + 3, _("brush_road"), wxBitmap(brush_surface_road), _("Road"));
	toolbar->AddTool(miBrushSurface + 4, _("brush_stone"), wxBitmap(brush_surface_stone), _("Stone"));
	toolbar->AddTool(miBrushSurface + 5, _("brush_custom"), wxBitmap(brush_surface_custom), _("Ground"));
	toolbar->AddSeparator();
	toolbar->AddTool(miBrushResource + 2, _("resource1"), wxBitmap(brush_resource_1_gold), _("gold  (unwalkable)"));
	toolbar->AddTool(miBrushResource + 3, _("resource2"), wxBitmap(brush_resource_2_stone), _("stone (unwalkable)"));
	toolbar->AddTool(miBrushResource + 4, _("resource3"), wxBitmap(brush_resource_3), _("custom3"));
	toolbar->AddTool(miBrushResource + 5, _("resource4"), wxBitmap(brush_resource_4), _("custom4"));
	toolbar->AddTool(miBrushResource + 6, _("resource5"), wxBitmap(brush_resource_5), _("custom5"));
	toolbar->AddSeparator();
	toolbar->AddTool(miBrushObject + 1, _("brush_none"), wxBitmap(brush_none), _("None (erase)"));
	toolbar->AddTool(miBrushObject + 2, _("brush_tree"), wxBitmap(brush_object_tree), _("Tree (unwalkable/harvestable)"));
	toolbar->AddTool(miBrushObject + 3, _("brush_dead_tree"), wxBitmap(brush_object_dead_tree), _("Dead tree/Cactuses/Thornbush (unwalkable)"));
	toolbar->AddTool(miBrushObject + 4, _("brush_stone"), wxBitmap(brush_object_stone), _("Stone (unwalkable/not harvestable)"));
	toolbar->AddTool(miBrushObject + 5, _("brush_bush"), wxBitmap(brush_object_bush), _("Bush/Grass/Fern (walkable)"));
	toolbar->AddTool(miBrushObject + 6, _("brush_water"), wxBitmap(brush_object_water_object), _("Water object/Reed/Papyrus (walkable)"));
	toolbar->AddTool(miBrushObject + 7, _("brush_c1_bigtree"), wxBitmap(brush_object_c1_bigtree), _("Big tree/Old palm (unwalkable/not harvestable)"));
	toolbar->AddTool(miBrushObject + 8, _("brush_c2_hanged"), wxBitmap(brush_object_c2_hanged), _("Hanged/Impaled (unwalkable)"));
	toolbar->AddTool(miBrushObject + 9, _("brush_c3_statue"), wxBitmap(brush_object_c3_statue), _("Statues (unwalkable)"));
	toolbar->AddTool(miBrushObject +10, _("brush_c4_bigrock"), wxBitmap(brush_object_c4_bigrock), _("Mountain (unwalkable)"));
	toolbar->AddTool(miBrushObject +11, _("brush_c5_blocking"), wxBitmap(brush_object_c5_blocking), _("Invisible blocking object (unwalkable)"));
	toolbar->AddSeparator();
	toolbar->AddTool(toolPlayer, _("brush_player"), wxBitmap(brush_players_player),  _("Player start position"));
	toolbar->Realize();

	wxToolBar *toolbar2 = new MainToolBar(this->panel, wxID_ANY);
	toolbar2->AddTool(miBrushGradient + 1, _("brush_gradient_n5"), wxBitmap(brush_gradient_n5));
	toolbar2->AddTool(miBrushGradient + 2, _("brush_gradient_n4"), wxBitmap(brush_gradient_n4));
	toolbar2->AddTool(miBrushGradient + 3, _("brush_gradient_n3"), wxBitmap(brush_gradient_n3));
	toolbar2->AddTool(miBrushGradient + 4, _("brush_gradient_n2"), wxBitmap(brush_gradient_n2));
	toolbar2->AddTool(miBrushGradient + 5, _("brush_gradient_n1"), wxBitmap(brush_gradient_n1));
	toolbar2->AddTool(miBrushGradient + 6, _("brush_gradient_0"), wxBitmap(brush_gradient_0));
	toolbar2->AddTool(miBrushGradient + 7, _("brush_gradient_p1"), wxBitmap(brush_gradient_p1));
	toolbar2->AddTool(miBrushGradient + 8, _("brush_gradient_p2"), wxBitmap(brush_gradient_p2));
	toolbar2->AddTool(miBrushGradient + 9, _("brush_gradient_p3"), wxBitmap(brush_gradient_p3));
	toolbar2->AddTool(miBrushGradient +10, _("brush_gradient_p4"), wxBitmap(brush_gradient_p4));
	toolbar2->AddTool(miBrushGradient +11, _("brush_gradient_p5"), wxBitmap(brush_gradient_p5));
	toolbar2->AddSeparator();
	toolbar2->AddTool(miBrushHeight + 1, _("brush_height_n5"), wxBitmap(brush_height_n5));
	toolbar2->AddTool(miBrushHeight + 2, _("brush_height_n4"), wxBitmap(brush_height_n4));
	toolbar2->AddTool(miBrushHeight + 3, _("brush_height_n3"), wxBitmap(brush_height_n3));
	toolbar2->AddTool(miBrushHeight + 4, _("brush_height_n2"), wxBitmap(brush_height_n2));
	toolbar2->AddTool(miBrushHeight + 5, _("brush_height_n1"), wxBitmap(brush_height_n1));
	toolbar2->AddTool(miBrushHeight + 6, _("brush_height_0"), wxBitmap(brush_height_0));
	toolbar2->AddTool(miBrushHeight + 7, _("brush_height_p1"), wxBitmap(brush_height_p1));
	toolbar2->AddTool(miBrushHeight + 8, _("brush_height_p2"), wxBitmap(brush_height_p2));
	toolbar2->AddTool(miBrushHeight + 9, _("brush_height_p3"), wxBitmap(brush_height_p3));
	toolbar2->AddTool(miBrushHeight +10, _("brush_height_p4"), wxBitmap(brush_height_p4));
	toolbar2->AddTool(miBrushHeight +11, _("brush_height_p5"), wxBitmap(brush_height_p5));
	toolbar2->AddSeparator();
	toolbar2->AddTool(miRadius + 1, _("radius1"), wxBitmap(radius_1), _("1 (1x1)"));
	toolbar2->AddTool(miRadius + 2, _("radius2"), wxBitmap(radius_2), _("2 (3x3)"));
	toolbar2->AddTool(miRadius + 3, _("radius3"), wxBitmap(radius_3), _("3 (5x5)"));
	toolbar2->AddTool(miRadius + 4, _("radius4"), wxBitmap(radius_4), _("4 (7x7)"));
	toolbar2->AddTool(miRadius + 5, _("radius5"), wxBitmap(radius_5), _("5 (9x9)"));
	toolbar2->AddTool(miRadius + 6, _("radius6"), wxBitmap(radius_6), _("6 (11x11)"));
	toolbar2->AddTool(miRadius + 7, _("radius7"), wxBitmap(radius_7), _("7 (13x13)"));
	toolbar2->AddTool(miRadius + 8, _("radius8"), wxBitmap(radius_8), _("8 (15x15)"));
	toolbar2->AddTool(miRadius + 9, _("radius9"), wxBitmap(radius_9), _("9 (17x17)"));
	toolbar2->Realize();

	Config &config = Config::getInstance();

    string userData = config.getString("UserData_Root","");
    if(userData != "") {
    	endPathWithSlash(userData);
    }

	//std::cout << "A" << std::endl;
	wxInitAllImageHandlers();
#ifdef WIN32
	//std::cout << "B" << std::endl;
//	#if defined(__MINGW32__)
		wxIcon icon(ToUnicode("IDI_ICON1"));
//	#else
//		wxIcon icon("IDI_ICON1");
//	#endif

#else
	//std::cout << "B" << std::endl;
	wxIcon icon;
	string iniFilePath = extractDirectoryPathFromFile(config.getFileName(false));
	string icon_file = iniFilePath + "editor.ico";
	std::ifstream testFile(icon_file.c_str());
	if(testFile.good())	{
		testFile.close();
		icon.LoadFile(ToUnicode(icon_file.c_str()),wxBITMAP_TYPE_ICO);
	}
#endif
	//std::cout << "C" << std::endl;
	SetIcon(icon);
	fileDialog = new wxFileDialog(this);
    string defaultPath = userData + "maps/";
    fileDialog->SetDirectory(ToUnicode(defaultPath));
    heightMapDirectory=fileDialog->GetDirectory();

	//printf("Default Path [%s]\n",defaultPath.c_str());

	lastPaintEvent.start();

	boxsizer = new wxBoxSizer(wxVERTICAL);
	boxsizer->Add(toolbar, 0, wxEXPAND);
	boxsizer->Add(toolbar2, 0, wxEXPAND);
	//boxsizer->Add(glCanvas, 1, wxEXPAND);

	this->panel->SetSizer(boxsizer);
	//this->Layout();

	//program = new Program(glCanvas->GetClientSize().x, glCanvas->GetClientSize().y);

	fileName = "New (unsaved) Map";

	//printf("Does file exist, fname [%s]\n",fname.c_str());

	if (fname.empty() == false && fileExists(fname)) {
		//printf("YES file exist, fname [%s]\n",fname.c_str());

		//program->loadMap(fname);
		currentFile = fname;
		fileName = cutLastExt(extractFileFromDirectoryPath(fname.c_str()));
		fileDialog->SetPath(ToUnicode(fname));
	}
	SetTitle(ToUnicode(currentFile + " - " + winHeader));
	//setDirty(false);
	//setExtension();

	initGlCanvas();
	#if wxCHECK_VERSION(2, 9, 3)
		//glCanvas->setCurrentGLContext();
		//printf("setcurrent #1\n");
	#elif wxCHECK_VERSION(2, 9, 1)

	#else
		if(glCanvas) glCanvas->SetCurrent();
		//printf("setcurrent #2\n");
	#endif

	if(startupSettingsInited == false) {
		startupSettingsInited = true;
		setupStartupSettings();
	}
}

void MainWindow::onClose(wxCloseEvent &event) {
	if(program != NULL && program->getMap()->getHasChanged() == true) {
		if( wxMessageDialog(NULL, ToUnicode("Do you want to save the current map?"),
			ToUnicode("Question"), wxYES_NO | wxYES_DEFAULT).ShowModal() == wxID_YES) {
			wxCommandEvent ev;
			MainWindow::onMenuFileSave(ev);
		}
	}
	delete program;
	program = NULL;

	//delete glCanvas;
	if(glCanvas) glCanvas->Destroy();
	glCanvas = NULL;

	this->Destroy();
}

void MainWindow::initGlCanvas(){
	if(glCanvas == NULL) {
		int args[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_MIN_ALPHA, 8, 0 };
		glCanvas = new GlCanvas(this, this->panel, args);

		boxsizer->Add(glCanvas, 1, wxEXPAND);
	}
}

void MainWindow::setupStartupSettings() {

	//gl canvas
	if(glCanvas == NULL) {
		initGlCanvas();

		boxsizer->Add(glCanvas, 1, wxEXPAND);

		this->panel->SetSizer(boxsizer);
		this->Layout();
	}
	glCanvas->setCurrentGLContext();
	glCanvas->SetFocus();

	string playerName = Config::getInstance().getString("NetPlayerName","");
	program = new Program(glCanvas->GetClientSize().x, glCanvas->GetClientSize().y, playerName);
	fileName = "New (unsaved) Map";

	//printf("#0 file load [%s]\n",currentFile.c_str());

	if (!currentFile.empty() && fileExists(currentFile)) {
		//printf("#0 exists file load [%s]\n",currentFile.c_str());

		program->loadMap(currentFile);
		//currentFile = fname;
		fileName = cutLastExt(extractFileFromDirectoryPath(currentFile.c_str()));
		fileDialog->SetPath(ToUnicode(currentFile));
	}
	SetTitle(ToUnicode(currentFile + " - " + winHeader));
	setDirty(false);
	setExtension();
}

MainWindow::~MainWindow() {
	delete fileDialog;
	fileDialog = NULL;

	delete program;
	program = NULL;

	delete glCanvas;
	glCanvas = NULL;
}

void MainWindow::setDirty(bool val) {
	refreshThings();
	if (fileModified && val) {
		return;
	}
	fileModified = val;
	if (fileModified) {
		SetStatusText(wxT("File: ") + ToUnicode(fileName) + wxT("*"), siFILE_NAME);
	} else {
		SetStatusText(wxT("File: ") + ToUnicode(fileName), siFILE_NAME);
	}
}

void MainWindow::setExtension() {
	if (currentFile.empty() || program == NULL) {
		return;
	}

	string extnsn = ext(currentFile);

	//printf("#A currentFile [%s] extnsn [%s]\n",currentFile.c_str(),extnsn.c_str());

	if (extnsn == "gbm" || extnsn == "mgm") {
		currentFile = cutLastExt(currentFile);
	}
	SetStatusText(wxT(".mgm"), siFILE_TYPE);
	currentFile += ".mgm";
}

void MainWindow::onMouseDown(wxMouseEvent &event, int x, int y) {
	if (event.LeftIsDown() && program != NULL) {
		program->setUndoPoint(enabledGroup);
		program->setRefAlt(x, y);
		change(x, y);
		if (!isDirty()) {
			setDirty(true);
		}

		refreshThings();
	}
	event.Skip();
}

// for the mousewheel
void MainWindow::onMouseWheelDown(wxMouseEvent &event) {
	if(program == NULL) {
		return;
	}
	program->incCellSize(1);
	refreshThings();
}

void MainWindow::onMouseWheelUp(wxMouseEvent &event) {
	if(program == NULL) {
		return;
	}
	program->incCellSize(-1);
	refreshThings();
}

void MainWindow::onMouseMove(wxMouseEvent &event, int x, int y) {
	if(program == NULL) {
		return;
	}
    mouse_pos.first = program->getCellX(x);
    mouse_pos.second = program->getCellY(y);
	if (event.LeftIsDown()) {
		change(x, y);
	} else if (event.MiddleIsDown()) {
		int dif = (y - lastY);
		if (dif != 0) {
			program->incCellSize(dif / abs(dif));
		}
	} else if (event.RightIsDown()) {
		program->setOfset(x - lastX, y - lastY);
    } else {
		int currResource = program->getResource(x, y);
		if (currResource > 0) {
			SetStatusText(wxT("Resource: ") + ToUnicode(resource_descs[currResource]), siCURR_OBJECT);
			resourceUnderMouse = currResource;
			objectUnderMouse = 0;
		} else {
			int currObject = program->getObject(x, y);
			SetStatusText(wxT("Object: ") + ToUnicode(object_descs[currObject]), siCURR_OBJECT);
			resourceUnderMouse = 0;
			objectUnderMouse = currObject;
		}

		SetStatusText(wxT("Pos (Ingame): ")
				+ ToUnicode(intToStr(program->getCellX(x))
				+ ","
				+ intToStr(program->getCellY(y))
				+ " ("
				+ intToStr(2*(program->getCellX(x)))
				+ ","
				+ intToStr(2*(program->getCellY(y)))
				+ ")"), siPOS_VALUE);
//#ifdef WIN32
		//repaint = true;
//#endif
	}
	lastX = x;
	lastY = y;

	refreshThings();

	event.Skip();
}

void MainWindow::refreshThings() {
	if(!IsShown()) {
		return;
	}
	glCanvas->setCurrentGLContext();
	//}

	//printf("lastPaintEvent.getMillis() map\n");
	if(lastPaintEvent.getMillis() < 30) {
		sleep(1);
		return;
	}

	//printf("wxPaintDC dc map\n");
	wxPaintDC dc(this); // "In a paint event handler must always create a wxPaintDC object even if you do not use it.  (?)
	                    //  Otherwise, under MS Windows, refreshing for this and other windows will go wrong"
                        //  http://docs.wxwidgets.org/2.6/wx_wxpaintevent.html

	lastPaintEvent.start();

	if(panel) panel->Refresh(false);
	if(menuBar) menuBar->Refresh(false);

	refreshMapRender();
}
void MainWindow::onPaint(wxPaintEvent &event) {
		refreshThings();
		event.Skip();
		return;
}

void MainWindow::refreshMapRender() {
	//printf("refreshMapRender map\n");

    if(program && glCanvas) {
        if(enabledGroup == ctLocation) {
            program->renderMap(glCanvas->GetClientSize().x, glCanvas->GetClientSize().y);
        } else {
            program->renderMap(glCanvas->GetClientSize().x, glCanvas->GetClientSize().y, &mouse_pos, &radius);
        }
		glCanvas->SwapBuffers();
	}
}

void MainWindow::onMenuFileLoad(wxCommandEvent &event) {
	if(program == NULL) {
		return;
	}

	try {
		fileDialog->SetMessage(wxT("Select Glestmap to load"));
		fileDialog->SetWildcard(wxT("Glest&Mega Map (*.gbm *.mgm)|*.gbm;*.mgm|Glest Map (*.gbm)|*.gbm|Mega Map (*.mgm)|*.mgm"));
		if (fileDialog->ShowModal() == wxID_OK) {
#ifdef wxCHECK_VERSION(2, 9, 1)
			currentFile = fileDialog->GetPath().ToStdString();
#else
			const wxWX2MBbuf tmp_buf = wxConvCurrent->cWX2MB(fileDialog->GetPath());
			currentFile = tmp_buf;
#endif

			//printf("#1 file load [%s]\n",currentFile.c_str());

			program->loadMap(currentFile);
			fileName = cutLastExt(extractFileFromDirectoryPath(currentFile.c_str()));
			setDirty(false);
			setExtension();
			SetTitle(ToUnicode(winHeader + "; " + currentFile));
		}
	}
	catch (const string &e) {
		MsgDialog(this, ToUnicode(e), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
	catch (const exception &e) {
		MsgDialog(this, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}

}

void MainWindow::onMenuFileSave(wxCommandEvent &event) {
	if(program == NULL) {
		return;
	}

	if (currentFile.empty()) {
		wxCommandEvent ev;
		onMenuFileSaveAs(ev);
	}
	else {
		setExtension();

		//printf("#1 save load [%s]\n",currentFile.c_str());

		program->saveMap(currentFile);
		setDirty(false);
	}
}

void MainWindow::onMenuFileSaveAs(wxCommandEvent &event) {
	if(program == NULL) {
		return;
	}

#if wxCHECK_VERSION(2, 9, 1)
	wxFileDialog fd(this, wxT("Select file"), wxT(""), wxT(""), wxT("*.mgm|*.gbm"), wxFD_SAVE);
#else
	wxFileDialog fd(this, wxT("Select file"), wxT(""), wxT(""), wxT("*.mgm|*.gbm"), wxSAVE);
#endif

	if(fileDialog->GetPath() != ToUnicode("")) {
		fd.SetPath(fileDialog->GetPath());
	}
	else {
		Config &config = Config::getInstance();
		string userData = config.getString("UserData_Root","");
		if(userData != "") {
			endPathWithSlash(userData);
		}
		string defaultPath = userData + "maps/";
		fd.SetDirectory(ToUnicode(defaultPath));
	}

	fd.SetWildcard(wxT("MegaGlest Map (*.mgm)|*.mgm|Glest Map (*.gbm)|*.gbm"));
	if (fd.ShowModal() == wxID_OK) {

#ifdef wxCHECK_VERSION(2, 9, 1)
		currentFile = fd.GetPath().ToStdString();
#else
		const wxWX2MBbuf tmp_buf = wxConvCurrent->cWX2MB(fd.GetPath());
		currentFile = tmp_buf;
#endif

		fileDialog->SetPath(fd.GetPath());
		setExtension();

		//printf("#2 file save [%s]\n",currentFile.c_str());

		program->saveMap(currentFile);
		fileName = cutLastExt(extractFileFromDirectoryPath(currentFile.c_str()));
		setDirty(false);
	}
	SetTitle(ToUnicode(winHeader + "; " + currentFile));
}

void MainWindow::onMenuFileExit(wxCommandEvent &event) {
	Close();
}

void MainWindow::onMenuEditUndo(wxCommandEvent &event) {
	if(program == NULL) {
		return;
	}

	// std::cout << "Undo Pressed" << std::endl;
	if (program->undo()) {
		refreshThings();
		setDirty();
	}
}

void MainWindow::onMenuEditRedo(wxCommandEvent &event) {
	if(program == NULL) {
		return;
	}

	if (program->redo()) {
		refreshThings();
		setDirty();
	}
}

void MainWindow::onMenuEditReset(wxCommandEvent &event) {
	if(program == NULL) {
		return;
	}

	program->setUndoPoint(ctAll);
	SimpleDialog simpleDialog;
	simpleDialog.addValue("Width", "128","(must be 16,32,64,128,256,512...)"); // must be an exponent of two
	simpleDialog.addValue("Height", "128","(must be 16,32,64,128,256,512...)");
	simpleDialog.addValue("Surface", "1","(Default surface material)");
	simpleDialog.addValue("Altitude", "10","(Default surface height)");
	simpleDialog.addValue("Number of players", "8");
	if (!simpleDialog.show()) return;

	try {
		program->reset(
			strToInt(simpleDialog.getValue("Width")),
			strToInt(simpleDialog.getValue("Height")),
			strToInt(simpleDialog.getValue("Altitude")),
			strToInt(simpleDialog.getValue("Surface")));
			program->resetFactions(strToInt(simpleDialog.getValue("Number of players")));
	}
	catch (const exception &e) {
		MsgDialog(this, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
	currentFile = "";
	fileName = "New (unsaved) map";

	refreshThings();
}

void MainWindow::onMenuEditResetPlayers(wxCommandEvent &event) {
	if(program == NULL) {
		return;
	}

	SimpleDialog simpleDialog;
	simpleDialog.addValue("Number of players", intToStr(program->getMap()->getMaxFactions()));
	if (!simpleDialog.show("Reset players")) return;

	try {
		program->resetFactions(strToInt(simpleDialog.getValue("Number of players")));
	}
	catch (const exception &e) {
		MsgDialog(this, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
	setDirty();
	setExtension();
}


void MainWindow::onMenuEditFlipDiagonal(wxCommandEvent &event) {
	if(program == NULL) {
		return;
	}
	program->setUndoPoint(ctAll);
		program->flipDiagonal();
		setDirty();
}

void MainWindow::onMenuEditResize(wxCommandEvent &event) {
	if(program == NULL) {
		return;
	}

	SimpleDialog simpleDialog;
	simpleDialog.addValue("Width", intToStr(program->getMap()->getW()),"(must be 16,32,64,128,256,512...)");
	simpleDialog.addValue("Height", intToStr(program->getMap()->getH()),"(must be 16,32,64,128,256,512...)");
    if (!simpleDialog.show("Resize")) return;

	try {
		program->resize(
			strToInt(simpleDialog.getValue("Width")),
            strToInt(simpleDialog.getValue("Height")));
	}
	catch (const exception &e) {
		MsgDialog(this, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
	setDirty();
}

void MainWindow::onMenuEditFlipX(wxCommandEvent &event) {
	if(program == NULL) {
		return;
	}

	program->setUndoPoint(ctAll);
	program->flipX();
	setDirty();
}

void MainWindow::onMenuEditFlipY(wxCommandEvent &event) {
	if(program == NULL) {
		return;
	}

	program->setUndoPoint(ctAll);
	program->flipY();
	setDirty();
}

void MainWindow::onMenuEditMirrorX(wxCommandEvent &event) { // copy left to right
	if(program == NULL) {
		return;
	}

	program->setUndoPoint(ctAll);
	program->mirrorX();
	setDirty();
}

void MainWindow::onMenuEditMirrorY(wxCommandEvent &event) { // copy top to bottom
	if(program == NULL) {
		return;
	}

	program->setUndoPoint(ctAll);
	program->mirrorY();
	setDirty();
}

void MainWindow::onMenuEditMirrorXY(wxCommandEvent &event) { // copy bottomleft tp topright
	if(program == NULL) {
		return;
	}

	program->setUndoPoint(ctAll);
	program->mirrorXY();
	setDirty();
}

void MainWindow::onMenuEditRotatecopyX(wxCommandEvent &event) { // copy left to right, rotated
	if(program == NULL) {
		return;
	}

	program->setUndoPoint(ctAll);
	program->rotatecopyX();
	setDirty();
}

void MainWindow::onMenuEditRotatecopyY(wxCommandEvent &event) { // copy top to bottom, rotated
	if(program == NULL) {
		return;
	}

	program->setUndoPoint(ctAll);
	program->rotatecopyY();
	setDirty();
}

void MainWindow::onMenuEditRotatecopyXY(wxCommandEvent &event) { // copy bottomleft to topright, rotated
	if(program == NULL) {
		return;
	}

	program->setUndoPoint(ctAll);
	program->rotatecopyXY();
	setDirty();
}

void MainWindow::onMenuEditRotatecopyCorner(wxCommandEvent &event) { // copy top left 1/4 to top right 1/4, rotated
	if(program == NULL) {
		return;
	}

	program->setUndoPoint(ctAll);
	program->rotatecopyCorner();
	setDirty();
}


void MainWindow::onMenuEditRandomizeHeights(wxCommandEvent &event) {
	if(program == NULL) {
		return;
	}
	while(true){

		SimpleDialog simpleDialog;
		simpleDialog.addValue("Initial Reset", boolToStr(randomWithReset),"(1 = true, 0 = false) If set to '0' no height reset is done before calculating");
		simpleDialog.addValue("Min Height", intToStr(randomMinimumHeight),"Lowest random height. example: -300 or below if you want water , 0 if you don't want water.");
		simpleDialog.addValue("Max Height", intToStr(randomMaximumHeight),"Max random height. A good value is 400");
		simpleDialog.addValue("Chance Divider", intToStr(randomChanceDivider),"Defines how often you get a hill or hole default is 30. Bigger number, less hills/holes.");
		simpleDialog.addValue("Smooth Recursions", intToStr(randomRecursions),"Number of recursions cycles to smooth the hills and holes. 0<x<50 default is 3.");
		if (!simpleDialog.show("Randomize Height")) return;

		try {
			string checkValue = simpleDialog.getValue("Initial Reset");
			if(checkValue != "" && strToInt(checkValue) > 1) {
				randomWithReset = true;
			}
			else {
				randomWithReset = strToBool(simpleDialog.getValue("Initial Reset"));
			}
			randomMinimumHeight=strToInt(simpleDialog.getValue("Min Height"));
			randomMaximumHeight=strToInt(simpleDialog.getValue("Max Height"));
			randomChanceDivider=strToInt(simpleDialog.getValue("Chance Divider"));
			randomRecursions=strToInt(simpleDialog.getValue("Smooth Recursions"));

			// set insane inputs to something that does not crash
			if(randomMinimumHeight>=randomMaximumHeight) randomMinimumHeight=randomMaximumHeight-1;
			if(randomChanceDivider<1) randomChanceDivider=1;

			// set randomRecursions to something useful
			if(randomRecursions<0) randomRecursions=0;
			if(randomRecursions>50) randomRecursions=50;

			program->setUndoPoint(ctAll);
			program->randomizeMapHeights(randomWithReset, randomMinimumHeight, randomMaximumHeight,
					randomChanceDivider, randomRecursions);
		}
		catch (const exception &e) {
			MsgDialog(this, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
		}
		setDirty();
	}
}

void MainWindow::onMenuEditImportHeights(wxCommandEvent &event) {
    if(program == NULL) {
        return;
    }
    try {
        fileDialog->SetMessage(wxT("Select heigthmap image to load"));
        fileDialog->SetWildcard(wxT("All Images|*.bmp;*.png;*.jpg;*.jpeg;*.gif;.*.tga;*.tiff;*.tif|PNG-Image (*.png)|*.png|JPEG-Image (*.jpg, *.jpeg)|*.jpg;*.jpeg|BMP-Image (*.bmp)|*.bmp|GIF-Image (*.gif)|*.gif|TIFF-Image (*.tif, *.tiff)|*.tiff;*.tif"));
        wxString savedDir=fileDialog->GetDirectory();
        fileDialog->SetDirectory(heightMapDirectory);
        if (fileDialog->ShowModal() == wxID_OK) {
#ifdef wxCHECK_VERSION(2, 9, 1)
            currentFile = fileDialog->GetPath().ToStdString();
#else
            const wxWX2MBbuf tmp_buf = wxConvCurrent->cWX2MB(fileDialog->GetPath());
            currentFile = tmp_buf;
#endif

            wxImage* img=new wxImage(currentFile);
            if(img != NULL) {
                wxImage grey_img = img->ConvertToGreyscale();
                delete img;
                int w =grey_img.GetWidth();
                int h =grey_img.GetHeight();
                int map_w = program->getMap()->getW();
                int map_h = program->getMap()->getH();
                program->setUndoPoint(ctAll);
                if(w != map_w && h != map_h) {
                        grey_img.Rescale(map_w,map_h,wxIMAGE_QUALITY_HIGH);
                }
                program->importMapHeights(grey_img.GetData());
                setDirty();
            }
        }

        heightMapDirectory=fileDialog->GetDirectory();
        fileDialog->SetDirectory(savedDir);
    }
    catch (const string &e) {
        MsgDialog(this, ToUnicode(e), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
    }
    catch (const exception &e) {
        MsgDialog(this, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
    }
}

void MainWindow::onMenuEditExportHeights(wxCommandEvent &event) {
    if(program == NULL) {
        return;
    }
#if wxCHECK_VERSION(2, 9, 1)
    wxFileDialog fd(this, wxT("Select file"), wxT(""), wxT(""), wxT("All Images|*.bmp;*.png;*.jpg;*.jpeg;*.gif;.*.tga;*.tiff;*.tif|PNG-Image (*.png)|*.png|JPEG-Image (*.jpg, *.jpeg)|*.jpg;*.jpeg|BMP-Image (*.bmp)|*.bmp|GIF-Image (*.gif)|*.gif|TIFF-Image (*.tif, *.tiff)|*.tiff;*.tif"), wxFD_SAVE);
#else
    wxFileDialog fd(this, wxT("Select file"), wxT(""), wxT(""), wxT("All Images|*.bmp;*.png;*.jpg;*.jpeg;*.gif;.*.tga;*.tiff;*.tif|PNG-Image (*.png)|*.png|JPEG-Image (*.jpg, *.jpeg)|*.jpg;*.jpeg|BMP-Image (*.bmp)|*.bmp|GIF-Image (*.gif)|*.gif|TIFF-Image (*.tif, *.tiff)|*.tiff;*.tif"), wxSAVE);
#endif
    fd.SetDirectory(heightMapDirectory);
    if (fd.ShowModal() == wxID_OK) {
#if wxCHECK_VERSION(2, 9, 1)
        currentFile = fd.GetPath().ToStdString();
#else
        const wxWX2MBbuf tmp_buf = wxConvCurrent->cWX2MB(fd.GetPath());
        currentFile = tmp_buf;
#endif
        int map_w = program->getMap()->getW();
        int map_h = program->getMap()->getH();
        wxImage img(map_w, map_h);
        unsigned char* img_data = img.GetData();
        for(int i = 0; i < map_w; i++) {
            for(int j = 0; j < map_h; j++) {
                //cells[i][j].height=(data[(i+j*w)*3]*(MAX_MAP_CELL_HEIGHT-MIN_MAP_CELL_HEIGHT)/255)+MIN_MAP_CELL_HEIGHT;
                img_data[(i+j*map_w)*3] = (unsigned char) ((program->getMap()->getHeight(i,j)-MIN_MAP_CELL_HEIGHT)*255/(MAX_MAP_CELL_HEIGHT-MIN_MAP_CELL_HEIGHT)+0.5);
                img_data[(i+j*map_w)*3+1] = (unsigned char) ((program->getMap()->getHeight(i,j)-MIN_MAP_CELL_HEIGHT)*255/(MAX_MAP_CELL_HEIGHT-MIN_MAP_CELL_HEIGHT)+0.5);
                img_data[(i+j*map_w)*3+2] = (unsigned char) ((program->getMap()->getHeight(i,j)-MIN_MAP_CELL_HEIGHT)*255/(MAX_MAP_CELL_HEIGHT-MIN_MAP_CELL_HEIGHT)+0.5);
            }
        }
        img.SaveFile(currentFile);
    }
    heightMapDirectory=fd.GetDirectory();
}

void MainWindow::onMenuEditRandomize(wxCommandEvent &event) {
	if(program == NULL) {
		return;
	}

	program->setUndoPoint(ctAll);
	program->randomizeFactions();
	setDirty();
}

void MainWindow::onMenuEditSwitchSurfaces(wxCommandEvent &event) {
	if(program == NULL) {
		return;
	}

	SimpleDialog simpleDialog;
	simpleDialog.addValue("Surface1", "1","replace this surface with...");
	simpleDialog.addValue("Surface2", "2","...this and vice versa");
	if (!simpleDialog.show("Switch surfaces")) return;

	try {
        program->setUndoPoint(ctSurface);
		program->switchMapSurfaces(
			strToInt(simpleDialog.getValue("Surface1")),
			strToInt(simpleDialog.getValue("Surface2")));
	}
	catch (const exception &e) {
		MsgDialog(this, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
	setDirty();
}

void MainWindow::onMenuEditInfo(wxCommandEvent &event) {
	if(program == NULL) {
		return;
	}

	SimpleDialog simpleDialog;
	simpleDialog.addValue("Title", program->getMap()->getTitle());
	simpleDialog.addValue("Description", program->getMap()->getDesc());
	simpleDialog.addValue("Author", program->getMap()->getAuthor());
	if (!simpleDialog.show("Info",true)) return;

    bool ischanged = program->setMapTitle(simpleDialog.getValue("Title"));
	ischanged = (program->setMapDesc(simpleDialog.getValue("Description")) || ischanged);
	ischanged = (program->setMapAuthor(simpleDialog.getValue("Author")) || ischanged);
	if (ischanged)
		if (!isDirty()) {
			setDirty(true);
		}

}

void MainWindow::onMenuEditAdvanced(wxCommandEvent &event) {
	if(program == NULL) {
		return;
	}

	SimpleDialog simpleDialog;
	simpleDialog.addValue("Height Factor", intToStr(program->getMap()->getHeightFactor()),"lower means more hill effect. Numbers above 100 are handled like this:\nx=x/100 ,so a 150 will mean 1.5 in the game.");
	simpleDialog.addValue("Water Level", intToStr(program->getMap()->getWaterLevel()),"water is visible below this, and walkable until 1.5 less");
	simpleDialog.addValue("Cliff Level", intToStr(program->getMap()->getCliffLevel()),"neighboring fields with at least this heights difference are cliffs");
	simpleDialog.addValue("Camera Height", intToStr(program->getMap()->getCameraHeight()),"you can give a camera height here default is 0 ;ignored if <20");
	if (!simpleDialog.show("Advanced")) return;

	try {
		program->setMapAdvanced(
			strToInt(simpleDialog.getValue("Height Factor")),
			strToInt(simpleDialog.getValue("Water Level")),
			strToInt(simpleDialog.getValue("Cliff Level")),
			strToInt(simpleDialog.getValue("Camera Height")));
	}
	catch (const exception &e) {
		MsgDialog(this, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
	setDirty();
}

void MainWindow::onMenuViewResetZoomAndPos(wxCommandEvent &event) {
	if(program == NULL) {
		return;
	}

	program->resetOfset();
	refreshThings();
}

void MainWindow::onMenuViewGrid(wxCommandEvent &event) {
	if(program == NULL) {
		return;
	}

	menuView->Check(miViewGrid, program->setGridOnOff());    // miViewGrid event.GetId()
	refreshThings();
}


void MainWindow::onMenuViewHeightMap(wxCommandEvent &event) {
	if(program == NULL) {
		return;
	}

	menuView->Check(miViewHeightMap, program->setHeightMapOnOff());    // miViewGrid event.GetId()
	refreshThings();
}
void MainWindow::onMenuHideWater(wxCommandEvent &event) {
	if(program == NULL) {
		return;
	}

	menuView->Check(miHideWater, program->setHideWaterOnOff());    // miViewGrid event.GetId()
	refreshThings();
}
void MainWindow::onMenuViewAbout(wxCommandEvent &event) {
	MsgDialog(
		this,
        wxT("\n    Glest Map Editor\n    Copyright 2004-2010 The Glest Team\n    Copyright 2010-2021 The MegaGlest Team    \n"),
		wxT("About")).ShowModal();
}

void MainWindow::onMenuViewHelp(wxCommandEvent &event) {
	MsgDialog(this,
		wxT("Draw with left mouse\nMove viewport with right mouse drag\nZoom with center mouse drag, or mousewheel\n\n\
You can change brush in the same category with key 1-9\n\
and change category with their first letter (keys S, R, O, G, H)\n\
Press Space to set brush to the resource or object under the mouse cursor\n\
Hold Shift to fill only empty spaces with the current object or resource\ninstead of replacing everything under the brush.\n\
To center things in the map shift it with Shift-Up/Down/Left/Right keys\n\
Height tool (blue) builds with integer height steps 0-20, \nwhile Gradient tool (red) uses any real number \n\
Units can go over water as long as it is less than 1.5 deep\n\n\
A good idea is to put some stone, gold and tree near starting position\n\
Starting position needs an open area for the tower and the starting units\n"),
		wxT("Help")).ShowModal();
		/* 5 away and 10x10 empty area? */
}

void MainWindow::onMenuBrushHeight(wxCommandEvent &e) {
	uncheckBrush();
	menuBrushHeight->Check(e.GetId(), true);
	height = e.GetId() - miBrushHeight - heightCount / 2 - 1;
	enabledGroup = ctHeight;
	currentBrush = btHeight;
	SetStatusText(wxT("Brush: Height"), siBRUSH_TYPE);
	SetStatusText(wxT("Value: ") + ToUnicode(intToStr(height)), siBRUSH_VALUE);
}

void MainWindow::onMenuBrushGradient(wxCommandEvent &e) {
	uncheckBrush();
	menuBrushGradient->Check(e.GetId(), true);
	height = e.GetId() - miBrushGradient - heightCount / 2 - 1;
	enabledGroup = ctGradient;
	currentBrush = btGradient;
	SetStatusText(wxT("Brush: Gradient"), siBRUSH_TYPE);
	SetStatusText(wxT("Value: ") + ToUnicode(intToStr(height)), siBRUSH_VALUE);
}


void MainWindow::onMenuBrushSurface(wxCommandEvent &e) {
	uncheckBrush();
	menuBrushSurface->Check(e.GetId(), true);
	surface = e.GetId() - miBrushSurface;
	enabledGroup = ctSurface;
	currentBrush = btSurface;
	SetStatusText(wxT("Brush: Surface"), siBRUSH_TYPE);
	SetStatusText(
		wxT("Value: ") + ToUnicode(intToStr(surface)) + wxT(" ")
		+ ToUnicode(surface_descs[surface - 1]), siBRUSH_VALUE);
}

void MainWindow::onMenuBrushObject(wxCommandEvent &e) {
	uncheckBrush();
	menuBrushObject->Check(e.GetId(), true);
	object = e.GetId() - miBrushObject - 1;
	enabledGroup = ctObject;
	currentBrush = btObject;
	SetStatusText(wxT("Brush: Object"), siBRUSH_TYPE);
	SetStatusText(
		wxT("Value: ") + ToUnicode(intToStr(object)) + wxT(" ")
		+ ToUnicode(object_descs[object]), siBRUSH_VALUE);
}

void MainWindow::onMenuBrushResource(wxCommandEvent &e) {
	uncheckBrush();
	menuBrushResource->Check(e.GetId(), true);
	resource = e.GetId() - miBrushResource - 1;
	enabledGroup = ctResource;
	currentBrush = btResource;
	SetStatusText(wxT("Brush: Resource"), siBRUSH_TYPE);
	SetStatusText(
		wxT("Value: ") + ToUnicode(intToStr(resource)) + wxT(" ")
		+ ToUnicode(resource_descs[resource]), siBRUSH_VALUE);
}

void MainWindow::onMenuBrushStartLocation(wxCommandEvent &e) {
	uncheckBrush();
	menuBrushStartLocation->Check(e.GetId(), true);
	startLocation = e.GetId() - miBrushStartLocation;
	enabledGroup = ctLocation;
	currentBrush = btStartLocation;
	SetStatusText(wxT("Brush: Start Locations"), siBRUSH_TYPE);
	SetStatusText(wxT("Value: ") + ToUnicode(intToStr(startLocation)), siBRUSH_VALUE);
}

void MainWindow::onMenuRadius(wxCommandEvent &e) {
	uncheckRadius();
	menuRadius->Check(e.GetId(), true);
	radius = e.GetId() - miRadius;
	SetStatusText(wxT("Radius: ") + ToUnicode(intToStr(radius)), siBRUSH_RADIUS);
}

void MainWindow::change(int x, int y) {
	if(program == NULL) {
		return;
	}

	switch (enabledGroup) {
	case ctHeight:
		program->glestChangeMapHeight(x, y, height, radius);
		break;
	case ctSurface:
		program->changeMapSurface(x, y, surface, radius);
		break;
	case ctObject:
        program->changeMapObject(x, y, object, radius, !shiftModifierKey);
		break;
	case ctResource:
        program->changeMapResource(x, y, resource, radius, !shiftModifierKey);
		break;
	case ctLocation:
		program->changeStartLocation(x, y, startLocation - 1);
		break;
	case ctGradient:
		program->pirateChangeMapHeight(x, y, height, radius);
		break;
	}
}

void MainWindow::uncheckBrush() {
	for (int i = 0; i < heightCount; ++i) {
		menuBrushHeight->Check(miBrushHeight + i + 1, false);
	}
	for (int i = 0; i < heightCount; ++i) {
		menuBrushGradient->Check(miBrushGradient + i + 1, false);
	}
	for (int i = 0; i < surfaceCount; ++i) {
		menuBrushSurface->Check(miBrushSurface + i + 1, false);
	}
	for (int i = 0; i < objectCount; ++i) {
		menuBrushObject->Check(miBrushObject + i + 1, false);
	}
	for (int i = 0; i < resourceCount; ++i) {
		menuBrushResource->Check(miBrushResource + i + 1, false);
	}
	for (int i = 0; i < startLocationCount; ++i) {
		menuBrushStartLocation->Check(miBrushStartLocation + i + 1, false);
	}
}

void MainWindow::uncheckRadius() {
	for (int i = 1; i <= radiusCount; ++i) {
		menuRadius->Check(miRadius + i, false);
	}
}

void MainWindow::onKeyDown(wxKeyEvent &e) {
	if(program == NULL) {
		return;
	}

	if(e.GetModifiers() == wxMOD_CONTROL || e.GetModifiers() == wxMOD_ALT){
		e.Skip();
	}
	// WARNING: don't add any Ctrl or ALt key shortcuts below those are reserved for internal menu use.

 	if (currentBrush == btSurface) { // surface texture
 		if (e.GetKeyCode() >= '1' && e.GetKeyCode() <= '5') {
 			surface = e.GetKeyCode() - 48; // '1'-'5' == 1-5
 			wxCommandEvent evt(wxEVT_NULL, miBrushSurface + surface);
 			onMenuBrushSurface(evt);
 			return;
 		}
 	}
 	if (currentBrush == btObject) {
 		bool valid = true;
 		if (e.GetKeyCode() >= '1' && e.GetKeyCode() <= '9') {
 			object = e.GetKeyCode() - 48; // '1'-'9' == 1-9
 		} else if (e.GetKeyCode() == '0') { // '0' == 10
 			object = 10;
 		} else if (e.GetKeyCode() == '-') {	// '-' == 0
 			object = 0;
 		} else {
 			valid = false;
 		}
 		if (valid) {
 			wxCommandEvent evt(wxEVT_NULL, miBrushObject + object + 1);
 			onMenuBrushObject(evt);
 			return;
 		}
 	}
 	if (currentBrush == btResource) {
 		if (e.GetKeyCode() >= '0' && e.GetKeyCode() <= '5') {
 			resource = e.GetKeyCode() - 48;	// '0'-'5' == 0-5
 			wxCommandEvent evt(wxEVT_NULL, miBrushResource + resource + 1);
 			onMenuBrushResource(evt);
 			return;
 		}
 	}
 	if (currentBrush == btStartLocation) {
 		if (e.GetKeyCode() >= '1' && e.GetKeyCode() <= '8') {
 			startLocation = e.GetKeyCode() - 48; // '1'-'8' == 0-7
 			wxCommandEvent evt(wxEVT_NULL, miBrushStartLocation + startLocation);
 			onMenuBrushStartLocation(evt);
 			return;
 		}
 	}
 	if (e.GetKeyCode() == 'H') {
 		wxCommandEvent evt(wxEVT_NULL, miBrushHeight + height + heightCount / 2 + 1);
 		onMenuBrushHeight(evt);
 	} else if (e.GetKeyCode() == ' ') {
		if (resourceUnderMouse != 0) {
			wxCommandEvent evt(wxEVT_NULL, miBrushResource + resourceUnderMouse + 1);
 			onMenuBrushResource(evt);
		} else {
			wxCommandEvent evt(wxEVT_NULL, miBrushObject + objectUnderMouse + 1);
 			onMenuBrushObject(evt);
		}
 	} else if (e.GetKeyCode() == 'G') {
 		wxCommandEvent evt(wxEVT_NULL, miBrushGradient + height + heightCount / 2 + 1);
 		onMenuBrushGradient(evt);
 	} else if (e.GetKeyCode() == 'S') {
		wxCommandEvent evt(wxEVT_NULL, miBrushSurface + surface);
		onMenuBrushSurface(evt);
 	} else if (e.GetKeyCode() == 'O') {
 		wxCommandEvent evt(wxEVT_NULL, miBrushObject + object + 1);
 		onMenuBrushObject(evt);
 	} else if (e.GetKeyCode() == 'R') {
 		wxCommandEvent evt(wxEVT_NULL, miBrushResource + resource + 1);
 		onMenuBrushResource(evt);
 	} else if (e.GetKeyCode() == 'L') {
 		wxCommandEvent evt(wxEVT_NULL, miBrushStartLocation + startLocation + 1);
 		onMenuBrushStartLocation(evt);
 	} else if (e.GetKeyCode() == WXK_LEFT && e.GetModifiers() == wxMOD_SHIFT) { // shift-left/right/up/down to shift the map one square
 	    program->setUndoPoint(ctAll);
 	    program->shiftLeft();
 		setDirty();
 	} else if (e.GetKeyCode() == WXK_RIGHT && e.GetModifiers() == wxMOD_SHIFT) {
 	    program->setUndoPoint(ctAll);
 	    program->shiftRight();
 		setDirty();
 	} else if (e.GetKeyCode() == WXK_UP && e.GetModifiers() == wxMOD_SHIFT) {
 	    program->setUndoPoint(ctAll);
 	    program->shiftUp();
 		setDirty();
 	} else if (e.GetKeyCode() == WXK_DOWN && e.GetModifiers() == wxMOD_SHIFT) {
 	    program->setUndoPoint(ctAll);
 	    program->shiftDown();
 		setDirty();
    } else if (e.GetModifiers() == wxMOD_SHIFT) {
        shiftModifierKey = true;
        SetStatusText(wxT("Type: Fill"), siBRUSH_OVERWRITE);
    } else {
 		e.Skip();
	}
}

void MainWindow::onKeyUp(wxKeyEvent &e) {
    if(program == NULL) {
        return;
    }

    if(e.GetModifiers() == wxMOD_CONTROL || e.GetModifiers() == wxMOD_ALT){
        e.Skip();
    }
    // WARNING: don't add any Ctrl or ALt key shortcuts below those are reserved for internal menu use.
    if (e.GetModifiers() != wxMOD_SHIFT) {
            shiftModifierKey = false;
            SetStatusText(wxT("Type: Overwrite"), siBRUSH_OVERWRITE);
    }
}

BEGIN_EVENT_TABLE(MainWindow, wxFrame)

	EVT_CLOSE(MainWindow::onClose)

	// these are 'handled' by GlCanvas and funneled to these handlers. See BEGIN_EVENT_TABLE(GlCanvas, wxGLCanvas) below.
	//EVT_LEFT_DOWN(MainWindow::onMouseDown)
	//EVT_MOTION(MainWindow::onMouseMove)
	//EVT_KEY_DOWN(MainWindow::onKeyDown)

	EVT_MENU(wxID_NEW, MainWindow::onMenuEditReset)
	EVT_MENU(wxID_OPEN, MainWindow::onMenuFileLoad)
	EVT_MENU(wxID_SAVE, MainWindow::onMenuFileSave)
	EVT_MENU(wxID_SAVEAS, MainWindow::onMenuFileSaveAs)
	EVT_MENU(wxID_EXIT, MainWindow::onMenuFileExit)

	EVT_MENU(miEditUndo, MainWindow::onMenuEditUndo)
	EVT_MENU(miEditRedo, MainWindow::onMenuEditRedo)
	EVT_MENU(miEditResetPlayers, MainWindow::onMenuEditResetPlayers)
	EVT_MENU(miEditResize, MainWindow::onMenuEditResize)
	EVT_MENU(miEditFlipDiagonal, MainWindow::onMenuEditFlipDiagonal)
	EVT_MENU(miEditFlipX, MainWindow::onMenuEditFlipX)
	EVT_MENU(miEditFlipY, MainWindow::onMenuEditFlipY)

    EVT_MENU(miEditMirrorX, MainWindow::onMenuEditMirrorX)
	EVT_MENU(miEditMirrorY, MainWindow::onMenuEditMirrorY)
	EVT_MENU(miEditMirrorXY, MainWindow::onMenuEditMirrorXY)
	EVT_MENU(miEditRotatecopyX, MainWindow::onMenuEditRotatecopyX)
	EVT_MENU(miEditRotatecopyY, MainWindow::onMenuEditRotatecopyY)
	EVT_MENU(miEditRotatecopyXY, MainWindow::onMenuEditRotatecopyXY)
	EVT_MENU(miEditRotatecopyCorner, MainWindow::onMenuEditRotatecopyCorner)

	EVT_MENU(miEditRandomizeHeights, MainWindow::onMenuEditRandomizeHeights)
    EVT_MENU(miEditImportHeights, MainWindow::onMenuEditImportHeights)
    EVT_MENU(miEditExportHeights, MainWindow::onMenuEditExportHeights)
	EVT_MENU(miEditRandomize, MainWindow::onMenuEditRandomize)
	EVT_MENU(miEditSwitchSurfaces, MainWindow::onMenuEditSwitchSurfaces)
	EVT_MENU(miEditInfo, MainWindow::onMenuEditInfo)
	EVT_MENU(miEditAdvanced, MainWindow::onMenuEditAdvanced)

	EVT_MENU(miViewResetZoomAndPos, MainWindow::onMenuViewResetZoomAndPos)
	EVT_MENU(miViewGrid, MainWindow::onMenuViewGrid)
	EVT_MENU(miViewHeightMap, MainWindow::onMenuViewHeightMap)
	EVT_MENU(miHideWater, MainWindow::onMenuHideWater)
	EVT_MENU(miViewAbout, MainWindow::onMenuViewAbout)
	EVT_MENU(miViewHelp, MainWindow::onMenuViewHelp)

	EVT_MENU_RANGE(miBrushHeight + 1, miBrushHeight + heightCount, MainWindow::onMenuBrushHeight)
	EVT_MENU_RANGE(miBrushGradient + 1, miBrushGradient + heightCount, MainWindow::onMenuBrushGradient)
	EVT_MENU_RANGE(miBrushSurface + 1, miBrushSurface + surfaceCount, MainWindow::onMenuBrushSurface)
	EVT_MENU_RANGE(miBrushObject + 1, miBrushObject + objectCount, MainWindow::onMenuBrushObject)
	EVT_MENU_RANGE(miBrushResource + 1, miBrushResource + resourceCount, MainWindow::onMenuBrushResource)
	EVT_MENU_RANGE(miBrushStartLocation + 1, miBrushStartLocation + startLocationCount, MainWindow::onMenuBrushStartLocation)
	EVT_MENU_RANGE(miRadius, miRadius + radiusCount, MainWindow::onMenuRadius)

	EVT_PAINT(MainWindow::onPaint)

	EVT_TOOL(toolPlayer, MainWindow::onToolPlayer)
END_EVENT_TABLE()

// =====================================================
// class GlCanvas
// =====================================================

GlCanvas::GlCanvas(MainWindow *mainWindow, wxWindow *parent, int *args)
#if wxCHECK_VERSION(2, 9, 1)
		: wxGLCanvas(parent, -1, args, wxDefaultPosition, wxDefaultSize, 0, wxT("GLCanvas")) {
	this->context = new wxGLContext(this);
#else
		: wxGLCanvas(parent, -1, wxDefaultPosition, wxDefaultSize, 0, wxT("GLCanvas"), args) {
	this->context = NULL;
#endif

	this->mainWindow = mainWindow;
}

GlCanvas::~GlCanvas() {
	delete this->context;
	this->context = NULL;
}

void GlCanvas::setCurrentGLContext() {
#ifndef __APPLE__

#if wxCHECK_VERSION(2, 9, 1)
	if(this->context == NULL) {
		this->context = new wxGLContext(this);
	}
#endif

	if(this->context) {
		this->SetCurrent(*this->context);
	}
#else
        this->SetCurrent();
#endif
}

void translateCoords(wxWindow *wnd, int &x, int &y) {
/*
#ifdef WIN32
	int cx, cy;
	wnd->GetPosition(&cx, &cy);
	x += cx;
	y += cy;
#endif
	*/
}

// for the mousewheel
void GlCanvas::onMouseWheel(wxMouseEvent &event) {
	if(event.GetWheelRotation()>0) mainWindow->onMouseWheelDown(event);
	else mainWindow->onMouseWheelUp(event);
}

void GlCanvas::onMouseDown(wxMouseEvent &event) {
	int x, y;
	event.GetPosition(&x, &y);
	translateCoords(this, x, y);
	mainWindow->onMouseDown(event, x, y);
}

void GlCanvas::onMouseMove(wxMouseEvent &event) {
	int x, y;
	event.GetPosition(&x, &y);
	translateCoords(this, x, y);
	mainWindow->onMouseMove(event, x, y);
}

void GlCanvas::onKeyDown(wxKeyEvent &event) {
	int x, y;
	event.GetPosition(&x, &y);
	translateCoords(this, x, y);
	mainWindow->onKeyDown(event);
}

void GlCanvas::onKeyUp(wxKeyEvent &event) {
    mainWindow->onKeyUp(event);
}


void GlCanvas::onPaint(wxPaintEvent &event) {
//	wxPaintDC dc(this); //N "In a paint event handler must always create a wxPaintDC object even if you do not use it.  (?)
//    mainWindow->program->renderMap(GetClientSize().x, GetClientSize().y);
//	SwapBuffers();
//	event.Skip();
	//printf("gl onPaint skip map\n");
  mainWindow->onPaint(event);
}

BEGIN_EVENT_TABLE(GlCanvas, wxGLCanvas)
	EVT_KEY_DOWN(GlCanvas::onKeyDown)
    EVT_KEY_UP(GlCanvas::onKeyUp)
    EVT_MOUSEWHEEL(GlCanvas::onMouseWheel)
	EVT_LEFT_DOWN(GlCanvas::onMouseDown)
	EVT_MOTION(GlCanvas::onMouseMove)
	EVT_PAINT(GlCanvas::onPaint)  // Because the drawing area needs to be repainted too.
END_EVENT_TABLE()

// ===============================================
//  class SimpleDialog
// ===============================================

void SimpleDialog::addValue(const string &key, const string &value, const string &help) {
	values.push_back(pair<string, string>(key, value+"|"+help)); // I guess I need map<,> instead but I don't know how to do it
}

string SimpleDialog::getValue(const string &key) {
	for (unsigned int i = 0; i < values.size(); ++i) {
		if (values[i].first == key) {
			return values[i].second;
		}
	}
	return "";
}

bool SimpleDialog::show(const string &title, bool wide) {
	Create(NULL, -1, ToUnicode(title));
	wxSizer *sizer2 = new wxBoxSizer(wxVERTICAL);
	wxSizer *sizer = new wxFlexGridSizer(3);

	vector<wxTextCtrl*> texts;
	for (Values::iterator it = values.begin(); it != values.end(); ++it) {
	    size_t helptextpos = it->second.find_first_of('|');
		sizer->Add(new wxStaticText(this, -1, ToUnicode(it->first)), 0, wxALL, 5);
		wxTextCtrl *text = new wxTextCtrl(this, -1, ToUnicode(it->second.substr(0,helptextpos)));
		if(wide) text->SetMinSize( wxSize((text->GetSize().GetWidth())*4, text->GetSize().GetHeight()));  // 4 time as wide as default
		sizer->Add(text, 0, wxALL, 5);
		texts.push_back(text);
	    sizer->Add(new wxStaticText(this, -1, ToUnicode(it->second.substr(helptextpos+1))), 0, wxALL, 5);
	}
	sizer2->Add(sizer);
    sizer2->Add(CreateButtonSizer(wxOK|wxCANCEL),0,wxALIGN_RIGHT); // enable Cancel button
	SetSizerAndFit(sizer2);

	ShowModal();
	if(m_returnCode==wxID_CANCEL) return false; // don't change values if canceled

	for (unsigned int i = 0; i < texts.size(); ++i) {
		values[i].second = texts[i]->GetValue().ToAscii();
	}
	return true;
}


// ===============================================
//  class App
// ===============================================

bool App::OnInit() {
	SystemFlags::VERBOSE_MODE_ENABLED  = false;
	SystemFlags::ENABLE_THREADED_LOGGING = false;

	string fileparam;
	if(argc==2){
		if(argv[1][0]=='-') {   // any flag gives help and exits program.
			std::cout << std::endl << "MegaGlest map editor " << mapeditorVersionString << " [Using " << (const char *)wxConvCurrent->cWX2MB(wxVERSION_STRING) << "]" << std::endl << std::endl;
			//std::cout << "\nglest_map_editor [MGM FILE]" << std::endl << std::endl;
			std::cout << "Creates or edits megaglest/glest maps. [.mgm/.gbm]" << std::endl << std::endl;
			std::cout << "Draw with left mouse button."  << std::endl;
			std::cout << "Move map with right mouse button."  << std::endl;
			std::cout << "Zoom with middle mouse button or mousewheel."  << std::endl;

//			std::cout << " ~ more helps should be written here ~"  << std::endl;
			std::cout << std::endl;
			exit (0);
		}
//#if defined(__MINGW32__)
#if wxCHECK_VERSION(2, 9, 1)
		fileparam = argv[1].ToStdString();
#else
		const wxWX2MBbuf tmp_buf = wxConvCurrent->cWX2MB(argv[1]);
		fileparam = tmp_buf;
#endif

#ifdef WIN32
		auto_ptr<wchar_t> wstr(Ansi2WideString(fileparam.c_str()));
		fileparam = utf8_encode(wstr.get());
#endif

//#else
//        fileparam = wxFNCONV(argv[1]);
//#endif
	}

#if defined(wxMAJOR_VERSION) && defined(wxMINOR_VERSION) && defined(wxRELEASE_NUMBER) && defined(wxSUBRELEASE_NUMBER)
	printf("Using wxWidgets version [%d.%d.%d.%d]\n",wxMAJOR_VERSION,wxMINOR_VERSION,wxRELEASE_NUMBER,wxSUBRELEASE_NUMBER);
#endif

	wxString exe_path = wxStandardPaths::Get().GetExecutablePath();
    //wxString path_separator = wxFileName::GetPathSeparator();
    //exe_path = exe_path.BeforeLast(path_separator[0]);
    //exe_path += path_separator;

	string appPath;
#ifdef wxCHECK_VERSION(2, 9, 1)
	appPath = exe_path.ToStdString();
#else
	appPath = wxFNCONV(exe_path);
#endif

//#else
//		const wxWX2MBbuf tmp_buf = wxConvCurrent->cWX2MB(wxFNCONV(exe_path));
//		appPath = tmp_buf;
//#endif

	mainWindow = new MainWindow(appPath);
	mainWindow->Show();
	mainWindow->init(fileparam);
	mainWindow->Update();

#ifdef WIN32
	wxPoint pos = mainWindow->GetScreenPosition();
	wxSize size = mainWindow->GetSize();

	mainWindow->SetSize(pos.x, pos.y, 1, 1, wxSIZE_FORCE);
	//mainWindow->Update();

	mainWindow->SetSize(pos.x, pos.y, size.x-1, size.y, wxSIZE_FORCE);
	mainWindow->Update();
#endif

	return true;
}

int App::MainLoop() {
	try {
		//throw megaglest_runtime_error("test");
		return wxApp::MainLoop();
	}
	catch (const exception &e) {
		MsgDialog(NULL, ToUnicode(e.what()), wxT("Exception")).ShowModal();
	}
	return 0;
}

int App::OnExit() {
	SystemFlags::Close();
	SystemFlags::SHUTDOWN_PROGRAM_MODE=true;

	return 0;
}

MsgDialog::MsgDialog(wxWindow *parent,
                     const wxString& message,
                     const wxString& caption,
                     long style,
					 const wxPoint& pos) {

	m_sizerText = NULL;
    if ( !wxDialog::Create(parent, wxID_ANY, caption,
                           pos, wxDefaultSize,
						   style) ) {
        return;
	}
    m_sizerText = new wxBoxSizer(wxVERTICAL);
    wxStaticText *label = new wxStaticText(this, wxID_ANY, message);
    wxFont font(*wxNORMAL_FONT);
    font.SetPointSize(font.GetPointSize());
    font.SetWeight(wxFONTWEIGHT_NORMAL);
    label->SetFont(font);

    m_sizerText->Add(label, wxSizerFlags().Centre().Border());

    wxSizer *sizerIconAndText = new wxBoxSizer(wxHORIZONTAL);
    sizerIconAndText->Add(m_sizerText, wxSizerFlags(1).Expand());

    wxSizer *sizerTop = new wxBoxSizer(wxVERTICAL);
    sizerTop->Add(sizerIconAndText, wxSizerFlags(1).Expand().Border());

    wxSizer *sizerBtns = CreateButtonSizer(wxOK);
    if ( sizerBtns )
    {
        sizerTop->Add(sizerBtns, wxSizerFlags().Expand().Border());
    }

    SetSizerAndFit(sizerTop);
    CentreOnScreen();
}
MsgDialog::~MsgDialog() {

}

}// end namespace

IMPLEMENT_APP(MapEditor::App)
