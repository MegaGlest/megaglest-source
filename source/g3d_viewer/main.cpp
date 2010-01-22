#include "main.h"

#include <stdexcept>

#include "graphics_factory_basic_gl.h"
#include "graphics_interface.h"
#include "util.h"

using namespace Shared::Platform; 
using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;
using namespace Shared::Util;

using namespace std;

namespace Shared{ namespace G3dViewer{

// ===============================================
// 	class MainWindow
// ===============================================

const string MainWindow::versionString= "v1.3.4";
const string MainWindow::winHeader= "G3D viewer " + versionString + " - Built: " + __DATE__;

MainWindow::MainWindow(const string &modelPath): 
	wxFrame(
		NULL, -1, winHeader.c_str(), 
		wxPoint(Renderer::windowX, Renderer::windowY), 
		wxSize(Renderer::windowW, Renderer::windowH))
{
	renderer= Renderer::getInstance();
	this->modelPath= modelPath;
	model= NULL;
	playerColor= Renderer::pcRed;

	speed= 0.025f;
	
	glCanvas = new GlCanvas(this);

	glCanvas->SetCurrent();

	renderer->init();

	
	menu= new wxMenuBar();

	//menu
	menuFile= new wxMenu();
	menuFile->Append(miFileLoad, "Load");
	menu->Append(menuFile, "File");

	//mode
	menuMode= new wxMenu();
	menuMode->AppendCheckItem(miModeNormals, "Normals");
	menuMode->AppendCheckItem(miModeWireframe, "Wireframe");
	menuMode->AppendCheckItem(miModeGrid, "Grid");
	menu->Append(menuMode, "Mode");

	//mode
	menuSpeed= new wxMenu();
	menuSpeed->Append(miSpeedSlower, "Slower");
	menuSpeed->Append(miSpeedFaster, "Faster");
	menu->Append(menuSpeed, "Speed");

	//custom color
	menuCustomColor= new wxMenu();
	menuCustomColor->AppendCheckItem(miColorRed, "Red");
	menuCustomColor->AppendCheckItem(miColorBlue, "Blue");
	menuCustomColor->AppendCheckItem(miColorYellow, "Yellow");
	menuCustomColor->AppendCheckItem(miColorGreen, "Green");
	menu->Append(menuCustomColor, "Custom Color");

	menuMode->Check(miModeGrid, true);
	menuCustomColor->Check(miColorRed, true);

	SetMenuBar(menu);

	//misc
	model= NULL;
	rotX= 0.0f;
	rotY= 0.0f;
	zoom= 1.0f;
	lastX= 0;
	lastY= 0;
	anim= 0.0f;

	CreateStatusBar();

	timer = new wxTimer(this);
	timer->Start(40);

	if(!modelPath.empty()){
		Model *tmpModel= new ModelGl();
		renderer->loadTheModel(tmpModel, modelPath);
		model= tmpModel;
		GetStatusBar()->SetStatusText(getModelInfo().c_str());
	}
}

MainWindow::~MainWindow(){
	delete renderer;
	delete model;
	delete timer;
	delete glCanvas;
}

void MainWindow::onPaint(wxPaintEvent &event){
	renderer->reset(GetClientSize().x, GetClientSize().y, playerColor);
	renderer->transform(rotX, rotY, zoom);
	renderer->renderGrid();
	
	renderer->renderTheModel(model, anim);
	glCanvas->SwapBuffers();
}

void MainWindow::onClose(wxCloseEvent &event){
	delete this;
}

void MainWindow::onMouseMove(wxMouseEvent &event){
	
	int x= event.GetX();
	int y= event.GetY();
	wxPaintEvent paintEvent;

	if(event.LeftIsDown()){
		rotX+= clamp(lastX-x, -10, 10);
		rotY+= clamp(lastY-y, -10, 10);
		onPaint(paintEvent);
	} 
	else if(event.RightIsDown()){
		zoom*= 1.0f+(lastX-x+lastY-y)/100.0f;
		zoom= clamp(zoom, 0.1f, 10.0f);
		onPaint(paintEvent);
	}

	lastX= x;
	lastY= y;
}

void MainWindow::onMenuFileLoad(wxCommandEvent &event){
	string fileName;
	wxFileDialog fileDialog(this);
	fileDialog.SetWildcard("G3D files (*.g3d)|*.g3d");
	if(fileDialog.ShowModal()==wxID_OK){
		delete model;
		Model *tmpModel= new ModelGl();
		renderer->loadTheModel(tmpModel, fileDialog.GetPath().c_str());
		model= tmpModel;
		GetStatusBar()->SetStatusText(getModelInfo().c_str());
	}
}

void MainWindow::onMenuModeNormals(wxCommandEvent &event){
	renderer->toggleNormals();
	menuMode->Check(miModeNormals, renderer->getNormals());
}

void MainWindow::onMenuModeWireframe(wxCommandEvent &event){
	renderer->toggleWireframe();
	menuMode->Check(miModeWireframe, renderer->getWireframe());
}

void MainWindow::onMenuModeGrid(wxCommandEvent &event){
	renderer->toggleGrid();
	menuMode->Check(miModeGrid, renderer->getGrid());
}

void MainWindow::onMenuSpeedSlower(wxCommandEvent &event){
	speed/= 1.5f;
}

void MainWindow::onMenuSpeedFaster(wxCommandEvent &event){
	speed*= 1.5f;
}

void MainWindow::onMenuColorRed(wxCommandEvent &event){
	playerColor= Renderer::pcRed;
	menuCustomColor->Check(miColorRed, true);
	menuCustomColor->Check(miColorBlue, false);
	menuCustomColor->Check(miColorYellow, false);
	menuCustomColor->Check(miColorGreen, false);
}

void MainWindow::onMenuColorBlue(wxCommandEvent &event){
	playerColor= Renderer::pcBlue;
	menuCustomColor->Check(miColorRed, false);
	menuCustomColor->Check(miColorBlue, true);
	menuCustomColor->Check(miColorYellow, false);
	menuCustomColor->Check(miColorGreen, false);
}

void MainWindow::onMenuColorYellow(wxCommandEvent &event){
	playerColor= Renderer::pcYellow;
	menuCustomColor->Check(miColorRed, false);
	menuCustomColor->Check(miColorBlue, false);
	menuCustomColor->Check(miColorYellow, true);
	menuCustomColor->Check(miColorGreen, false);
}

void MainWindow::onMenuColorGreen(wxCommandEvent &event){
	playerColor= Renderer::pcGreen;
	menuCustomColor->Check(miColorRed, false);
	menuCustomColor->Check(miColorBlue, false);
	menuCustomColor->Check(miColorYellow, false);
	menuCustomColor->Check(miColorGreen, true);
}

void MainWindow::onTimer(wxTimerEvent &event){
	wxPaintEvent paintEvent;
	
	anim= anim+speed;
	if(anim>1.0f){
		anim-= 1.f;
	}
	onPaint(paintEvent);
}	

string MainWindow::getModelInfo(){
	string str;

	if(model!=NULL){
		str+= "Meshes: "+intToStr(model->getMeshCount());
		str+= ", Vertices: "+intToStr(model->getVertexCount());
		str+= ", Triangles: "+intToStr(model->getTriangleCount());
		str+= ", Version: "+intToStr(model->getFileVersion());
	}

	return str;
}

BEGIN_EVENT_TABLE(MainWindow, wxFrame)
	EVT_TIMER(-1, MainWindow::onTimer)
	EVT_CLOSE(MainWindow::onClose)
	EVT_MENU(miFileLoad, MainWindow::onMenuFileLoad)

	EVT_MENU(miModeWireframe, MainWindow::onMenuModeWireframe)
	EVT_MENU(miModeNormals, MainWindow::onMenuModeNormals)
	EVT_MENU(miModeGrid, MainWindow::onMenuModeGrid)

	EVT_MENU(miSpeedFaster, MainWindow::onMenuSpeedFaster)
	EVT_MENU(miSpeedSlower, MainWindow::onMenuSpeedSlower)

	EVT_MENU(miColorRed, MainWindow::onMenuColorRed)
	EVT_MENU(miColorBlue, MainWindow::onMenuColorBlue)
	EVT_MENU(miColorYellow, MainWindow::onMenuColorYellow)
	EVT_MENU(miColorGreen, MainWindow::onMenuColorGreen)
END_EVENT_TABLE()

// =====================================================
//	class GlCanvas
// =====================================================

GlCanvas::GlCanvas(MainWindow *	mainWindow):
	wxGLCanvas(mainWindow, -1)
{
	this->mainWindow = mainWindow;
}

void GlCanvas::onMouseMove(wxMouseEvent &event){
	mainWindow->onMouseMove(event);
}

void GlCanvas::onPaint(wxPaintEvent &event){
	mainWindow->onPaint(event);
}

BEGIN_EVENT_TABLE(GlCanvas, wxGLCanvas)
	EVT_MOTION(GlCanvas::onMouseMove)
	EVT_PAINT(GlCanvas::onPaint)
END_EVENT_TABLE()

// ===============================================
// 	class App
// ===============================================

bool App::OnInit(){
	string modelPath;
	if(argc==2){
		modelPath= argv[1];
	}
	
	mainWindow= new MainWindow(modelPath);
	mainWindow->Show();
	return true;
}

int App::MainLoop(){
	try{
		return wxApp::MainLoop();
	}
	catch(const exception &e){
		wxMessageDialog(NULL, e.what(), "Exception", wxOK | wxICON_ERROR).ShowModal();
		return 0;
	}
}

int App::OnExit(){
	return 0;
}

}}//end namespace

IMPLEMENT_APP(Shared::G3dViewer::App)
