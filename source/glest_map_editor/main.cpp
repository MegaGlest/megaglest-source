#include "main.h"

#include <ctime>

#include "conversion.h"

using namespace Shared::Util;

using namespace std;

namespace Glest{ namespace MapEditor{

const string MainWindow::versionString= "v1.3.0";
const string MainWindow::winHeader= "Glest Map Editor " + versionString + " - Built: " + __DATE__;

// ===============================================
//	class Global functions
// ===============================================

wxString ToUnicode(const char* str){
	return wxString(str, wxConvUTF8);
}

wxString ToUnicode(const string& str){
	return wxString(str.c_str(), wxConvUTF8);
}

// ===============================================
//	class MainWindow
// ===============================================

MainWindow::MainWindow():
	wxFrame(NULL, -1,  ToUnicode(winHeader), wxDefaultPosition, wxSize(800, 600))
{
	lastX= 0;
	lastY= 0;

	radius= 1;
	height= 5;
	surface= 1;
	object= 0;
	resource= 0;
	startLocation= 1;
	enabledGroup= 0;

	
	//gl canvas
	glCanvas = new GlCanvas(this);

	//menus
	menuBar= new wxMenuBar();
	
	//file
	menuFile= new wxMenu();
	menuFile->Append(miFileLoad, wxT("Load"));
	menuFile->AppendSeparator();
	menuFile->Append(miFileSave, wxT("Save"));
	menuFile->Append(miFileSaveAs, wxT("Save As"));
	menuFile->AppendSeparator();
	menuFile->Append(miFileExit, wxT("Exit"));
	menuBar->Append(menuFile, wxT("File"));

	//edit
	menuEdit= new wxMenu();
	menuEdit->Append(miEditReset, wxT("Reset"));
	menuEdit->Append(miEditResetPlayers, wxT("Reset Players"));
	menuEdit->Append(miEditResize, wxT("Resize"));
	menuEdit->Append(miEditFlipX, wxT("Flip X"));
	menuEdit->Append(miEditFlipY, wxT("Flip Y"));
	menuEdit->Append(miEditRandomizeHeights, wxT("Randomize Heights"));
	menuEdit->Append(miEditRandomize, wxT("Randomize"));
	menuEdit->Append(miEditSwitchSurfaces, wxT("Switch Surfaces"));
	menuEdit->Append(miEditInfo, wxT("Info"));
	menuEdit->Append(miEditAdvanced, wxT("Advanced"));
	menuBar->Append(menuEdit, wxT("Edit"));

	//misc
	menuMisc= new wxMenu();
	menuMisc->Append(miMiscResetZoomAndPos, wxT("Reset zoom and pos"));
	menuMisc->Append(miMiscAbout, wxT("About"));
	menuMisc->Append(miMiscHelp, wxT("Help"));
	menuBar->Append(menuMisc, wxT("Misc"));

	//brush
	menuBrush= new wxMenu();

	//height
	menuBrushHeight= new wxMenu();
	for(int i=0; i<heightCount; ++i){
		menuBrushHeight->AppendCheckItem(miBrushHeight+i+1, ToUnicode(intToStr(i-heightCount/2)));
	}	
	menuBrushHeight->Check(miBrushHeight + 1 + heightCount/2, true);
	menuBrush->Append(miBrushHeight, wxT("Height"), menuBrushHeight);
	
	//surface
	menuBrushSurface= new wxMenu();
	menuBrushSurface->AppendCheckItem(miBrushSurface+1, wxT("1 - Grass"));
	menuBrushSurface->AppendCheckItem(miBrushSurface+2, wxT("2 - Secondary Grass"));
	menuBrushSurface->AppendCheckItem(miBrushSurface+3, wxT("3 - Road"));
	menuBrushSurface->AppendCheckItem(miBrushSurface+4, wxT("4 - Stone"));
	menuBrushSurface->AppendCheckItem(miBrushSurface+5, wxT("5 - Custom"));
	menuBrush->Append(miBrushSurface, wxT("Surface"), menuBrushSurface);
	
	//objects
	menuBrushObject= new wxMenu();
	menuBrushObject->AppendCheckItem(miBrushObject+1, wxT("0 - None"));
	menuBrushObject->AppendCheckItem(miBrushObject+2, wxT("1 - Tree"));
	menuBrushObject->AppendCheckItem(miBrushObject+3, wxT("2 - Dead Tree"));
	menuBrushObject->AppendCheckItem(miBrushObject+4, wxT("3 - Stone"));
	menuBrushObject->AppendCheckItem(miBrushObject+5, wxT("4 - Bush"));
	menuBrushObject->AppendCheckItem(miBrushObject+6, wxT("5 - Water Object"));
	menuBrushObject->AppendCheckItem(miBrushObject+7, wxT("6 - Custom 1"));
	menuBrushObject->AppendCheckItem(miBrushObject+8, wxT("7 - Custom 2"));
	menuBrushObject->AppendCheckItem(miBrushObject+9, wxT("8 - Custom 3"));
	menuBrushObject->AppendCheckItem(miBrushObject+10, wxT("9 - Custom 4"));
	menuBrushObject->AppendCheckItem(miBrushObject+11, wxT("10 - Custom 5"));
	menuBrush->Append(miBrushObject, wxT("Object"), menuBrushObject);

	//resources
	menuBrushResource= new wxMenu();
	menuBrushResource->AppendCheckItem(miBrushResource+1, wxT("0 - None"));
	menuBrushResource->AppendCheckItem(miBrushResource+2, wxT("1 - Custom 1"));
	menuBrushResource->AppendCheckItem(miBrushResource+3, wxT("2 - Custom 2"));
	menuBrushResource->AppendCheckItem(miBrushResource+4, wxT("3 - Custom 3"));
	menuBrushResource->AppendCheckItem(miBrushResource+5, wxT("4 - Custom 4"));
	menuBrushResource->AppendCheckItem(miBrushResource+6, wxT("5 - Custom 5"));
	menuBrush->Append(miBrushResource, wxT("Resource"), menuBrushResource);

	//players
	menuBrushStartLocation= new wxMenu();
	menuBrushStartLocation->AppendCheckItem(miBrushStartLocation+1, wxT("1 - Player 1"));
	menuBrushStartLocation->AppendCheckItem(miBrushStartLocation+2, wxT("2 - Player 2"));
	menuBrushStartLocation->AppendCheckItem(miBrushStartLocation+3, wxT("3 - Player 3"));
	menuBrushStartLocation->AppendCheckItem(miBrushStartLocation+4, wxT("4 - Player 4"));
	menuBrush->Append(miBrushStartLocation, wxT("Player"), menuBrushStartLocation);

	menuBar->Append(menuBrush, wxT("Brush"));

	//radius
	menuRadius= new wxMenu();
	for(int i=0; i<radiusCount; ++i){
		menuRadius->AppendCheckItem(miRadius+i, ToUnicode(intToStr(i+1)));
	}
	menuRadius->Check(miRadius, true);
	menuBar->Append(menuRadius, wxT("Radius"));

	SetMenuBar(menuBar);
}

void MainWindow::init(){
	glCanvas->SetCurrent();
	program= new Program(GetClientSize().x, GetClientSize().y);
}

void MainWindow::onClose(wxCloseEvent &event){
	delete this;
}

MainWindow::~MainWindow(){
	delete program;
	delete glCanvas;
}

void MainWindow::onMouseDown(wxMouseEvent &event){
	if(event.LeftIsDown()){
		program->setRefAlt(event.GetX(), event.GetY());	
		change(event.GetX(), event.GetY());
	}
	wxPaintEvent ev;
	onPaint(ev);
}

void MainWindow::onMouseMove(wxMouseEvent &event){
	int dif;
	
	int x= event.GetX();
	int y= event.GetY();

	if(event.LeftIsDown()){
		change(x, y);
	}
	else if(event.MiddleIsDown()){
		dif= (y-lastY);
		if(dif!=0){
			program->incCellSize(dif/abs(dif));     
		}
	}
	else if(event.RightIsDown()){
		program->setOfset(x-lastX, y-lastY);        
	}
	lastX= x;
	lastY= y;
	wxPaintEvent ev;
	onPaint(ev);
}

void MainWindow::onPaint(wxPaintEvent &event){
	program->renderMap(GetClientSize().x, GetClientSize().y);
	
	glCanvas->SwapBuffers();
}

void MainWindow::onMenuFileLoad(wxCommandEvent &event){
	string fileName;
	
	wxFileDialog fileDialog(this);
	fileDialog.SetWildcard(wxT("Glest Binary Map (*.gbm)|*.gbm"));
	if(fileDialog.ShowModal()==wxID_OK){
		fileName= fileDialog.GetPath().ToAscii();
		program->loadMap(fileName);
	}

	currentFile= fileName;
	SetTitle(ToUnicode(winHeader + "; " + currentFile));
}

void MainWindow::onMenuFileSave(wxCommandEvent &event){
	if(currentFile.empty()){
		wxCommandEvent ev;
		onMenuFileSaveAs(ev);
	}
	else{
		program->saveMap(currentFile);
	}
}

void MainWindow::onMenuFileSaveAs(wxCommandEvent &event){
	string fileName;
	
	wxFileDialog fileDialog(this, wxT("Select file"), wxT(""), wxT(""), wxT("*.gbm"), wxSAVE);
	fileDialog.SetWildcard(wxT("Glest Binary Map (*.gbm)|*.gbm"));
	if(fileDialog.ShowModal()==wxID_OK){
		fileName= fileDialog.GetPath().ToAscii();
		program->saveMap(fileName);
	}

	currentFile= fileName;
	SetTitle(ToUnicode(winHeader + "; " + currentFile));
}

void MainWindow::onMenuFileExit(wxCommandEvent &event){
	Close();
}

void MainWindow::onMenuEditReset(wxCommandEvent &event){
	SimpleDialog simpleDialog;
	simpleDialog.addValue("Altitude", "10");
	simpleDialog.addValue("Surface", "1");
	simpleDialog.addValue("Height", "64");
	simpleDialog.addValue("Width", "64");
	simpleDialog.show();

	try{
		program->reset(
			strToInt(simpleDialog.getValue("Height")),
			strToInt(simpleDialog.getValue("Width")),
			strToInt(simpleDialog.getValue("Altitude")),
			strToInt(simpleDialog.getValue("Surface")));
	}
	catch(const exception &e){
		wxMessageDialog(NULL, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}

}

void MainWindow::onMenuEditResetPlayers(wxCommandEvent &event){
	SimpleDialog simpleDialog;
	simpleDialog.addValue("Players", intToStr(program->getMap()->getMaxPlayers()));
	simpleDialog.show();

	try{
		program->resetPlayers(strToInt(simpleDialog.getValue("Players")));
	}
	catch(const exception &e){
		wxMessageDialog(NULL, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuEditResize(wxCommandEvent &event){
	SimpleDialog simpleDialog;
	simpleDialog.addValue("Altitude", "10");
	simpleDialog.addValue("Surface", "1");
	simpleDialog.addValue("Height", "64");
	simpleDialog.addValue("Width", "64");
	simpleDialog.show();

	try{
		program->resize(
			strToInt(simpleDialog.getValue("Height")),
			strToInt(simpleDialog.getValue("Width")),
			strToInt(simpleDialog.getValue("Altitude")),
			strToInt(simpleDialog.getValue("Surface")));
	}
	catch(const exception &e){
		wxMessageDialog(NULL, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuEditFlipX(wxCommandEvent &event){
	program->flipX();
}

void MainWindow::onMenuEditFlipY(wxCommandEvent &event){
	program->flipY();
}

void MainWindow::onMenuEditRandomizeHeights(wxCommandEvent &event){
	program->randomizeMapHeights();
}

void MainWindow::onMenuEditRandomize(wxCommandEvent &event){
	program->randomizeMap();
}

void MainWindow::onMenuEditSwitchSurfaces(wxCommandEvent &event){
	SimpleDialog simpleDialog;
	simpleDialog.addValue("Surface1", "1");
	simpleDialog.addValue("Surface2", "2");
	simpleDialog.show();

	try{
		program->switchMapSurfaces(
			strToInt(simpleDialog.getValue("Surface1")),
			strToInt(simpleDialog.getValue("Surface2")));
	}
	catch(const exception &e){
		wxMessageDialog(NULL, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuEditInfo(wxCommandEvent &event){
	SimpleDialog simpleDialog;
	simpleDialog.addValue("Title", program->getMap()->getTitle());
	simpleDialog.addValue("Desc", program->getMap()->getDesc());
	simpleDialog.addValue("Author", program->getMap()->getAuthor());
	
	simpleDialog.show();

	program->setMapTitle(simpleDialog.getValue("Title"));
	program->setMapDesc(simpleDialog.getValue("Desc"));
	program->setMapAuthor(simpleDialog.getValue("Author"));
}

void MainWindow::onMenuEditAdvanced(wxCommandEvent &event){
	SimpleDialog simpleDialog;
	simpleDialog.addValue("Height Factor", intToStr(program->getMap()->getHeightFactor()));
	simpleDialog.addValue("Water Level", intToStr(program->getMap()->getWaterLevel()));
	
	simpleDialog.show();

	try{
		program->setMapAdvanced(
			strToInt(simpleDialog.getValue("Height Factor")),
			strToInt(simpleDialog.getValue("Water Level")));
	}
	catch(const exception &e){
		wxMessageDialog(NULL, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuMiscResetZoomAndPos(wxCommandEvent &event){
	program->resetOfset();
}

void MainWindow::onMenuMiscAbout(wxCommandEvent &event){
	wxMessageDialog(NULL, wxT("Glest Map Editor - Copyright 2004 The Glest Team"), wxT("About")).ShowModal();
}

void MainWindow::onMenuMiscHelp(wxCommandEvent &event){
	wxMessageDialog(
		NULL, 
		wxT("Left mouse click: draw\nRight mouse drag: move\nCenter mouse drag: zoom"), 
		wxT("Help")).ShowModal();
}

void MainWindow::onMenuBrushHeight(wxCommandEvent &event){
	uncheckBrush();
	menuBrushHeight->Check(event.GetId(), true);
	height= event.GetId() - miBrushHeight - heightCount/2 - 1;
	enabledGroup= 0;
}

void MainWindow::onMenuBrushSurface(wxCommandEvent &event){
	uncheckBrush();
	menuBrushSurface->Check(event.GetId(), true);
	surface= event.GetId() - miBrushSurface;
	enabledGroup= 1;
}

void MainWindow::onMenuBrushObject(wxCommandEvent &event){
	uncheckBrush();
	menuBrushObject->Check(event.GetId(), true);
	object= event.GetId() - miBrushObject - 1;
	enabledGroup= 2;
}

void MainWindow::onMenuBrushResource(wxCommandEvent &event){
	uncheckBrush();
	menuBrushResource->Check(event.GetId(), true);
	resource= event.GetId() - miBrushResource - 1;
	enabledGroup= 3;
}

void MainWindow::onMenuBrushStartLocation(wxCommandEvent &event){
	uncheckBrush();
	menuBrushStartLocation->Check(event.GetId(), true);
	startLocation= event.GetId() - miBrushStartLocation - 1;
	enabledGroup= 4;
}

void MainWindow::onMenuRadius(wxCommandEvent &event){
	uncheckRadius();
	menuRadius->Check(event.GetId(), true);
	radius= event.GetId() - miRadius + 1;
}

void MainWindow::change(int x, int y){
	switch(enabledGroup){
	case 0:
		program->changeMapHeight(x, y, height, radius);
		break;
	case 1:
		program->changeMapSurface(x, y, surface, radius);
		break;
	case 2:
		program->changeMapObject(x, y, object, radius);
		break;
	case 3:
		program->changeMapResource(x, y, resource, radius);
		break;
	case 4:
		program->changeStartLocation(x, y, startLocation);
		break;
	}
}

void MainWindow::uncheckBrush(){
	for(int i=0; i<heightCount; ++i){
		menuBrushHeight->Check(miBrushHeight+i+1, false);
	}
	for(int i=0; i<surfaceCount; ++i){
		menuBrushSurface->Check(miBrushSurface+i+1, false);
	}
	for(int i=0; i<objectCount; ++i){
		menuBrushObject->Check(miBrushObject+i+1, false);
	}
	for(int i=0; i<resourceCount; ++i){
		menuBrushResource->Check(miBrushResource+i+1, false);
	}
	for(int i=0; i<startLocationCount; ++i){
		menuBrushStartLocation->Check(miBrushStartLocation+i+1, false);
	}
}

void MainWindow::uncheckRadius(){
	for(int i=0; i<radiusCount; ++i){
		menuRadius->Check(miRadius+i, false);
	}	
}

BEGIN_EVENT_TABLE(MainWindow, wxFrame)
	EVT_CLOSE(MainWindow::onClose)
	EVT_LEFT_DOWN(MainWindow::onMouseDown)
	EVT_MOTION(MainWindow::onMouseMove)

	EVT_MENU(miFileLoad, MainWindow::onMenuFileLoad)
	EVT_MENU(miFileSave, MainWindow::onMenuFileSave)
	EVT_MENU(miFileSaveAs, MainWindow::onMenuFileSaveAs)
	EVT_MENU(miFileExit, MainWindow::onMenuFileExit)

	EVT_MENU(miEditReset, MainWindow::onMenuEditReset)
	EVT_MENU(miEditResetPlayers, MainWindow::onMenuEditResetPlayers)
	EVT_MENU(miEditResize, MainWindow::onMenuEditResize)
	EVT_MENU(miEditFlipX, MainWindow::onMenuEditFlipX)
	EVT_MENU(miEditFlipY, MainWindow::onMenuEditFlipY)
	EVT_MENU(miEditRandomizeHeights, MainWindow::onMenuEditRandomizeHeights)
	EVT_MENU(miEditRandomize, MainWindow::onMenuEditRandomize)
	EVT_MENU(miEditSwitchSurfaces, MainWindow::onMenuEditSwitchSurfaces)
	EVT_MENU(miEditInfo, MainWindow::onMenuEditInfo)
	EVT_MENU(miEditAdvanced, MainWindow::onMenuEditAdvanced)

	EVT_MENU(miMiscResetZoomAndPos, MainWindow::onMenuMiscResetZoomAndPos)
	EVT_MENU(miMiscAbout, MainWindow::onMenuMiscAbout)
	EVT_MENU(miMiscHelp, MainWindow::onMenuMiscHelp)

	EVT_MENU_RANGE(miBrushHeight+1, miBrushHeight+heightCount, MainWindow::onMenuBrushHeight)
	EVT_MENU_RANGE(miBrushSurface+1, miBrushSurface+surfaceCount, MainWindow::onMenuBrushSurface)
	EVT_MENU_RANGE(miBrushObject+1, miBrushObject+objectCount, MainWindow::onMenuBrushObject)
	EVT_MENU_RANGE(miBrushResource+1, miBrushResource+resourceCount, MainWindow::onMenuBrushResource)
	EVT_MENU_RANGE(miBrushStartLocation+1, miBrushStartLocation+startLocationCount, MainWindow::onMenuBrushStartLocation)
	EVT_MENU_RANGE(miRadius, miRadius+radiusCount, MainWindow::onMenuRadius)
END_EVENT_TABLE()

// =====================================================
//	class GlCanvas
// =====================================================

GlCanvas::GlCanvas(MainWindow *	mainWindow):
	wxGLCanvas(mainWindow, -1, wxDefaultPosition)
{
	this->mainWindow = mainWindow;
}

void GlCanvas::onMouseDown(wxMouseEvent &event){
	mainWindow->onMouseDown(event);
}

void GlCanvas::onMouseMove(wxMouseEvent &event){
	mainWindow->onMouseMove(event);
}

BEGIN_EVENT_TABLE(GlCanvas, wxGLCanvas)
	EVT_LEFT_DOWN(GlCanvas::onMouseDown)
	EVT_MOTION(GlCanvas::onMouseMove)
END_EVENT_TABLE()

// ===============================================
// 	class SimpleDialog
// ===============================================

void SimpleDialog::addValue(const string &key, const string &value){
	values.push_back(pair<string, string>(key, value));
}

string SimpleDialog::getValue(const string &key){
	for(int i=0; i<values.size(); ++i){
		if(values[i].first==key){
			return values[i].second;
		}
	}
	return "";
}

void SimpleDialog::show(){

	Create(NULL, -1, wxT("Edit Values"));

	wxSizer *sizer= new wxFlexGridSizer(2);

	vector<wxTextCtrl*> texts;

	for(Values::iterator it= values.begin(); it!=values.end(); ++it){
		sizer->Add(new wxStaticText(this, -1, ToUnicode(it->first)), 0, wxALL, 5);
		wxTextCtrl *text= new wxTextCtrl(this, -1, ToUnicode(it->second));
		sizer->Add(text, 0, wxALL, 5);
		texts.push_back(text);
	}
	SetSizerAndFit(sizer);

	ShowModal();

	for(int i=0; i<texts.size(); ++i){
		values[i].second= texts[i]->GetValue().ToAscii();
	}
}

// ===============================================
// 	class App
// ===============================================

bool App::OnInit(){
	mainWindow= new MainWindow();
	mainWindow->Show();
	mainWindow->init();
	return true;
}

int App::MainLoop(){
	try{
		return wxApp::MainLoop();
	}
	catch(const exception &e){
		wxMessageDialog(NULL, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
	return 0;
}

int App::OnExit(){
	return 0;
}

}}// end namespace

IMPLEMENT_APP(Glest::MapEditor::App)
