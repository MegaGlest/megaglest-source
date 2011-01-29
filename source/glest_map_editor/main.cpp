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

#include "main.h"
#include <ctime>
#include "conversion.h"
#include "icons.h"
#include "platform_common.h"
#include <iostream>

using namespace Shared::Util;
using namespace Shared::PlatformCommon;
using namespace std;

namespace MapEditor {



const string mapeditorVersionString = "v1.5.1";
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

MainWindow::MainWindow()
		: wxFrame(NULL, -1,  ToUnicode(winHeader), wxPoint(0,0), wxSize(1024, 768))
		, lastX(0), lastY(0)
		, currentBrush(btHeight)
		, height(0)
		, surface(1)
		, radius(1)
		, object(0)
		, resource(0)
		, startLocation(1)
		, enabledGroup(ctHeight)
		, fileModified(false)
		, menuBar(NULL)
		, panel(NULL)
        , glCanvas(NULL)
        , program(NULL) {

	this->panel = new wxPanel(this, wxID_ANY);

	//gl canvas
	int args[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_MIN_ALPHA,  8 };
	glCanvas = new GlCanvas(this, this->panel, args);

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
	menuEdit->Append(miEditRandomize, wxT("Randomi&ze Heights/Players"));
	menuEdit->Append(miEditSwitchSurfaces, wxT("Switch Sur&faces..."));
	menuEdit->Append(miEditInfo, wxT("&Info..."));
	menuEdit->Append(miEditAdvanced, wxT("&Advanced..."));
	menuBar->Append(menuEdit, wxT("&Edit"));

	//view
	menuView = new wxMenu();
	menuView->Append(miViewResetZoomAndPos, wxT("&Reset zoom and pos"));
    menuView->AppendCheckItem(miViewGrid, wxT("&Grid"));
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
	menuBrushObject->AppendCheckItem(miBrushObject+4, wxT("&Stone"));
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
		-2, // File name
		-1, // File type
		-2, // Current Object
		-2, // Brush Type
		-2, // Brush 'Value'
		-1, // Brush Radius
	};
	CreateStatusBar(siCOUNT);
	GetStatusBar()->SetStatusWidths(siCOUNT, status_widths);

	SetStatusText(wxT("File: ") + ToUnicode(fileName), siFILE_NAME);
	SetStatusText(wxT(".gbm"), siFILE_TYPE);
	SetStatusText(wxT("Object: None (Erase)"), siCURR_OBJECT);
	SetStatusText(wxT("Brush: Height"), siBRUSH_TYPE);
	SetStatusText(wxT("Value: 0"), siBRUSH_VALUE);
	SetStatusText(wxT("Radius: 1"), siBRUSH_RADIUS);

	wxToolBar *toolbar = new wxToolBar(this->panel, wxID_ANY);
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

	wxToolBar *toolbar2 = new wxToolBar(this->panel, wxID_ANY);
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

	wxBoxSizer *boxsizer = new wxBoxSizer(wxVERTICAL);
	boxsizer->Add(toolbar, 0, wxEXPAND);
	boxsizer->Add(toolbar2, 0, wxEXPAND);
	boxsizer->Add(glCanvas, 1, wxEXPAND);

	this->panel->SetSizer(boxsizer);
	this->Layout();

	//std::cout << "A" << std::endl;
	wxInitAllImageHandlers();
#ifdef WIN32
	//std::cout << "B" << std::endl;
#if defined(__MINGW32__)
	wxIcon icon(ToUnicode("IDI_ICON1"));
#else
    wxIcon icon("IDI_ICON1");
#endif

#else
	//std::cout << "B" << std::endl;
	wxIcon icon;
	std::ifstream testFile("editor.ico");
	if(testFile.good())	{
		testFile.close();
		icon.LoadFile(wxT("editor.ico"),wxBITMAP_TYPE_ICO);
	}
#endif
	//std::cout << "C" << std::endl;
	SetIcon(icon);
	fileDialog = new wxFileDialog(this);
	lastPaintEvent.start();

	glCanvas->SetFocus();
}

void MainWindow::onToolPlayer(wxCommandEvent& event){
	PopupMenu(menuBrushStartLocation);
}

void MainWindow::init(string fname) {
	glCanvas->SetCurrent();

	program = new Program(glCanvas->GetClientSize().x, glCanvas->GetClientSize().y);

	fileName = "New (unsaved) Map";
	if (!fname.empty() && fileExists(fname)) {
		program->loadMap(fname);
		currentFile = fname;
		fileName = cutLastExt(extractFileFromDirectoryPath(fname.c_str()));
		fileDialog->SetPath(ToUnicode(fname));
	}
	SetTitle(ToUnicode(currentFile + " - " + winHeader));
	setDirty(false);
	setExtension();
}

void MainWindow::onClose(wxCloseEvent &event) {
	if( wxMessageDialog(NULL, ToUnicode("Do you want to save the current map?"),
		ToUnicode("Question"), wxYES_NO | wxYES_DEFAULT).ShowModal() == wxID_YES) {
		wxCommandEvent ev;
		MainWindow::onMenuFileSave(ev);
	}
	delete this;
}

MainWindow::~MainWindow() {
	delete program;
	program = NULL;

	delete glCanvas;
	glCanvas = NULL;
}

void MainWindow::setDirty(bool val) {
	wxPaintEvent ev;
	onPaint(ev);
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
	if (currentFile.empty()) {
		return;
	}
	string extnsn = ext(currentFile);
	if (extnsn == "gbm" || extnsn == "mgm") {
		currentFile = cutLastExt(currentFile);
	}
	if (Program::getMap()->getMaxFactions() <= 4) {
		SetStatusText(wxT(".gbm"), siFILE_TYPE);
		currentFile += ".gbm";
	} else {
		SetStatusText(wxT(".mgm"), siFILE_TYPE);
		currentFile += ".mgm";
	}
}

void MainWindow::onMouseDown(wxMouseEvent &event, int x, int y) {
	if (event.LeftIsDown()) {
		program->setUndoPoint(enabledGroup);
		program->setRefAlt(x, y);
		change(x, y);
		if (!isDirty()) {
			setDirty(true);
		}
		wxPaintEvent ev;
		onPaint(ev);
	}
	event.Skip();
}

// for the mousewheel
void MainWindow::onMouseWheelDown(wxMouseEvent &event) {
	wxPaintEvent ev;
	program->incCellSize(1);
	onPaint(ev);
}

void MainWindow::onMouseWheelUp(wxMouseEvent &event) {
	wxPaintEvent ev;
	program->incCellSize(-1);
	onPaint(ev);
}

void MainWindow::onMouseMove(wxMouseEvent &event, int x, int y) {
	bool repaint = false;
	int dif;
	if (event.LeftIsDown()) {
		change(x, y);
		repaint = true;
	} else if (event.MiddleIsDown()) {
		dif = (y - lastY);
		if (dif != 0) {
			program->incCellSize(dif / abs(dif));
			repaint = true;
		}
	} else if (event.RightIsDown()) {
		program->setOfset(x - lastX, y - lastY);
		repaint = true;
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
	}
	lastX = x;
	lastY = y;

	if (repaint) {
		wxPaintEvent ev;
		onPaint(ev);
	}
	event.Skip();
}

void MainWindow::onPaint(wxPaintEvent &event) {
	if(lastPaintEvent.getMillis() < 70) {
		sleep(1);
		return;
	}

	wxPaintDC dc(this); // "In a paint event handler must always create a wxPaintDC object even if you do not use it.  (?)
	                    //  Otherwise, under MS Windows, refreshing for this and other windows will go wrong"
                        //  http://docs.wxwidgets.org/2.6/wx_wxpaintevent.html

	lastPaintEvent.start();

	if(panel) panel->Update();
	if(menuBar) menuBar->Update();

	if(program && glCanvas) {
		program->renderMap(glCanvas->GetClientSize().x, glCanvas->GetClientSize().y);
		glCanvas->SwapBuffers();
	}
	event.Skip();
}

void MainWindow::onMenuFileLoad(wxCommandEvent &event) {
	fileDialog->SetMessage(wxT("Select Glestmap to load"));
	fileDialog->SetWildcard(wxT("Glest&Mega Map (*.gbm *.mgm)|*.gbm;*.mgm|Glest Map (*.gbm)|*.gbm|Mega Map (*.mgm)|*.mgm"));
	if (fileDialog->ShowModal() == wxID_OK) {
		currentFile = fileDialog->GetPath().ToAscii();
		program->loadMap(currentFile);
		fileName = cutLastExt(extractFileFromDirectoryPath(currentFile.c_str()));
		setDirty(false);
		setExtension();
		SetTitle(ToUnicode(winHeader + "; " + currentFile));
	}
}

void MainWindow::onMenuFileSave(wxCommandEvent &event) {
	if (currentFile.empty()) {
		wxCommandEvent ev;
		onMenuFileSaveAs(ev);
	} else {
		setExtension();
		program->saveMap(currentFile);
		setDirty(false);
	}
}

void MainWindow::onMenuFileSaveAs(wxCommandEvent &event) {
	wxFileDialog fd(this, wxT("Select file"), wxT(""), wxT(""), wxT("*.gbm|*.mgm"), wxSAVE);

	fd.SetPath(fileDialog->GetPath());
	fd.SetWildcard(wxT("Glest Map (*.gbm)|*.gbm|MegaGlest Map (*.mgm)|*.mgm"));
	if (fd.ShowModal() == wxID_OK) {
		currentFile = fd.GetPath().ToAscii();
		fileDialog->SetPath(fd.GetPath());
		setExtension();
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
	// std::cout << "Undo Pressed" << std::endl;
	if (program->undo()) {
		wxPaintEvent e;
		onPaint(e);
		setDirty();
	}
}

void MainWindow::onMenuEditRedo(wxCommandEvent &event) {
	// std::cout << "Redo Pressed" << std::endl;
	if (program->redo()) {
		wxPaintEvent e;
		onPaint(e);
		setDirty();
	}
}

void MainWindow::onMenuEditReset(wxCommandEvent &event) {
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
}

void MainWindow::onMenuEditResetPlayers(wxCommandEvent &event) {
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

void MainWindow::onMenuEditResize(wxCommandEvent &event) {
	SimpleDialog simpleDialog;
	simpleDialog.addValue("Width", intToStr(program->getMap()->getW()),"(must be 16,32,64,128,256,512...)");
	simpleDialog.addValue("Height", intToStr(program->getMap()->getH()),"(must be 16,32,64,128,256,512...)");
	simpleDialog.addValue("Surface", "1","(surface material for new area around map)");
	simpleDialog.addValue("Altitude", "10","(surface height for new area around map)");
	if (!simpleDialog.show("Resize - expand around, shrink to topleft")) return;

	try {
		program->resize(
			strToInt(simpleDialog.getValue("Width")),
			strToInt(simpleDialog.getValue("Height")),
			strToInt(simpleDialog.getValue("Altitude")),
			strToInt(simpleDialog.getValue("Surface")));
	}
	catch (const exception &e) {
		MsgDialog(this, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
	setDirty();
}

void MainWindow::onMenuEditFlipX(wxCommandEvent &event) {
    program->setUndoPoint(ctAll);
	program->flipX();
	setDirty();
}

void MainWindow::onMenuEditFlipY(wxCommandEvent &event) {
    program->setUndoPoint(ctAll);
	program->flipY();
	setDirty();
}

void MainWindow::onMenuEditMirrorX(wxCommandEvent &event) { // copy left to right
    program->setUndoPoint(ctAll);
	program->mirrorX();
	setDirty();
}

void MainWindow::onMenuEditMirrorY(wxCommandEvent &event) { // copy top to bottom
    program->setUndoPoint(ctAll);
	program->mirrorY();
	setDirty();
}

void MainWindow::onMenuEditMirrorXY(wxCommandEvent &event) { // copy bottomleft tp topright
    program->setUndoPoint(ctAll);
	program->mirrorXY();
	setDirty();
}

void MainWindow::onMenuEditRotatecopyX(wxCommandEvent &event) { // copy left to right, rotated
    program->setUndoPoint(ctAll);
	program->rotatecopyX();
	setDirty();
}

void MainWindow::onMenuEditRotatecopyY(wxCommandEvent &event) { // copy top to bottom, rotated
    program->setUndoPoint(ctAll);
	program->rotatecopyY();
	setDirty();
}

void MainWindow::onMenuEditRotatecopyXY(wxCommandEvent &event) { // copy bottomleft to topright, rotated
    program->setUndoPoint(ctAll);
	program->rotatecopyXY();
	setDirty();
}

void MainWindow::onMenuEditRotatecopyCorner(wxCommandEvent &event) { // copy top left 1/4 to top right 1/4, rotated
    program->setUndoPoint(ctAll);
	program->rotatecopyCorner();
	setDirty();
}


void MainWindow::onMenuEditRandomizeHeights(wxCommandEvent &event) {
    program->setUndoPoint(ctAll);
	program->randomizeMapHeights();
	setDirty();
}

void MainWindow::onMenuEditRandomize(wxCommandEvent &event) {
    program->setUndoPoint(ctAll);
	program->randomizeMap();
	setDirty();
}

void MainWindow::onMenuEditSwitchSurfaces(wxCommandEvent &event) {
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
	SimpleDialog simpleDialog;
	simpleDialog.addValue("Title", program->getMap()->getTitle());
	simpleDialog.addValue("Description", program->getMap()->getDesc());
	simpleDialog.addValue("Author", program->getMap()->getAuthor());
	if (!simpleDialog.show("Info",true)) return;

    bool ischanged = false;
	ischanged = program->setMapTitle(simpleDialog.getValue("Title"));
	ischanged = (program->setMapDesc(simpleDialog.getValue("Description")) || ischanged);
	ischanged = (program->setMapAuthor(simpleDialog.getValue("Author")) || ischanged);
	if (ischanged)
		if (!isDirty()) {
			setDirty(true);
		}

}

void MainWindow::onMenuEditAdvanced(wxCommandEvent &event) {
	SimpleDialog simpleDialog;
	simpleDialog.addValue("Height Factor", intToStr(program->getMap()->getHeightFactor()),"(lower means map is more more zoomed in)");
	simpleDialog.addValue("Water Level", intToStr(program->getMap()->getWaterLevel()),"(water is visible below this, and walkable until 1.5 less)");
	if (!simpleDialog.show("Advanced")) return;

	try {
		program->setMapAdvanced(
			strToInt(simpleDialog.getValue("Height Factor")),
			strToInt(simpleDialog.getValue("Water Level")));
	}
	catch (const exception &e) {
		MsgDialog(this, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
	setDirty();
}

void MainWindow::onMenuViewResetZoomAndPos(wxCommandEvent &event) {
	program->resetOfset();
	wxPaintEvent e;
	onPaint(e);
}

void MainWindow::onMenuViewGrid(wxCommandEvent &event) {
	menuView->Check(miViewGrid, program->setGridOnOff());    // miViewGrid event.GetId()
	wxPaintEvent e;
	onPaint(e);
}


void MainWindow::onMenuViewAbout(wxCommandEvent &event) {
	MsgDialog(
		this,
		wxT("Glest Map Editor - Copyright 2004 The Glest Team\n(with improvements by others, 2010)."),
		wxT("About")).ShowModal();
}

void MainWindow::onMenuViewHelp(wxCommandEvent &event) {
	MsgDialog(this,
		wxT("Draw with left mouse\nMove viewport with right mouse drag\nZoom with center mouse drag, or mousewheel\n\n\
You can change brush in the same category with key 1-9\n\
and change category with their first letter (keys S, R, O, G, H)\n\
Press Space to set brush to the resource or object under the mouse cursor\n\
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
	switch (enabledGroup) {
	case ctHeight:
		program->glestChangeMapHeight(x, y, height, radius);
		break;
	case ctSurface:
		program->changeMapSurface(x, y, surface, radius);
		break;
	case ctObject:
		program->changeMapObject(x, y, object, radius);
		break;
	case ctResource:
		program->changeMapResource(x, y, resource, radius);
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
 	if (currentBrush == btHeight || currentBrush == btGradient) { // 'height' brush
 		if (e.GetKeyCode() >= '0' && e.GetKeyCode() <= '5') {
 			height = e.GetKeyCode() - 48; // '0'-'5' == 0-5
 			if (e.GetModifiers() == wxMOD_CONTROL) { // Ctrl means negative
 				height  = -height ;
 			}
 			int id_offset = heightCount / 2 + height + 1;
 			if (currentBrush == btHeight) {
 				wxCommandEvent evt(wxEVT_NULL, miBrushHeight + id_offset);
 				onMenuBrushHeight(evt);
 			} else {
 				wxCommandEvent evt(wxEVT_NULL, miBrushGradient + id_offset);
 				onMenuBrushGradient(evt);
 			}
 			return;
 		}
 	}
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
 	} else if (e.GetKeyCode() == WXK_BACK && e.GetModifiers() == wxMOD_ALT) { // undo
 	    wxCommandEvent evt(wxEVT_NULL, 0);
        onMenuEditUndo(evt);
 	} else {
 		e.Skip();
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
	EVT_MENU(miEditRandomize, MainWindow::onMenuEditRandomize)
	EVT_MENU(miEditSwitchSurfaces, MainWindow::onMenuEditSwitchSurfaces)
	EVT_MENU(miEditInfo, MainWindow::onMenuEditInfo)
	EVT_MENU(miEditAdvanced, MainWindow::onMenuEditAdvanced)

	EVT_MENU(miViewResetZoomAndPos, MainWindow::onMenuViewResetZoomAndPos)
	EVT_MENU(miViewGrid, MainWindow::onMenuViewGrid)
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
		: wxGLCanvas(parent, -1, wxDefaultPosition, wxDefaultSize, 0, wxT("GLCanvas"), args) {
	this->mainWindow = mainWindow;
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

void GlCanvas::onPaint(wxPaintEvent &event) {
//	wxPaintDC dc(this); //N "In a paint event handler must always create a wxPaintDC object even if you do not use it.  (?)
//    mainWindow->program->renderMap(GetClientSize().x, GetClientSize().y);
//	SwapBuffers();
//	event.Skip();
  mainWindow->onPaint(event);
}

BEGIN_EVENT_TABLE(GlCanvas, wxGLCanvas)
	EVT_KEY_DOWN(GlCanvas::onKeyDown)
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
	    int helptextpos = it->second.find_first_of('|');
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
	string fileparam;
	if(argc==2){
		if(argv[1][0]=='-') {   // any flag gives help and exits program.
			std::cout << "MegaGlest map editor " << mapeditorVersionString << " [Using " << (const char *)wxConvCurrent->cWX2MB(wxVERSION_STRING) << "]" << std::endl << std::endl;
			std::cout << "glest_map_editor [GBM OR MGM FILE]" << std::endl << std::endl;
			std::cout << "Creates or edits glest/megaglest maps."  << std::endl;
			std::cout << "Draw with left mouse button (select what and how large area in menu or toolbar)"  << std::endl;
			std::cout << "Pan trough the map with right mouse button"  << std::endl;
			std::cout << "Zoom with middle mouse button or mousewheel"  << std::endl;

//			std::cout << " ~ more helps should be written here ~"  << std::endl;
			std::cout << std::endl;
			exit (0);
		}

#if defined(__MINGW32__)
		const wxWX2MBbuf tmp_buf = wxConvCurrent->cWX2MB(wxFNCONV(argv[1]));
		fileparam = tmp_buf;
#else
        fileparam = wxFNCONV(argv[1]);
#endif
	}

	mainWindow = new MainWindow();
	mainWindow->Show();
	mainWindow->init(fileparam);
	mainWindow->Update();

#ifdef WIN32
	wxPoint pos = mainWindow->GetScreenPosition();
	wxSize size = mainWindow->GetSize();
	mainWindow->SetSize(pos.x, pos.y, size.x-1, size.y, wxSIZE_FORCE);
	mainWindow->Update();
#endif

	return true;
}

int App::MainLoop() {
	try {
		//throw runtime_error("test");
		return wxApp::MainLoop();
	}
	catch (const exception &e) {
		MsgDialog(NULL, ToUnicode(e.what()), wxT("Exception")).ShowModal();
	}
	return 0;
}

int App::OnExit() {
	return 0;
}

MsgDialog::MsgDialog(wxWindow *parent,
                     const wxString& message,
                     const wxString& caption,
                     long style,
					 const wxPoint& pos) {

	m_sizerText = NULL;
	// TODO: should we use main frame as parent by default here?
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
