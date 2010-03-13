#include "main.h"

#include <stdexcept>

#include "graphics_factory_gl.h"
#include "graphics_interface.h"
#include "util.h"
#include "conversion.h"

using namespace Shared::Platform;
using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;
using namespace Shared::Util;

using namespace std;

namespace Shared{ namespace G3dViewer{

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
// 	class MainWindow
// ===============================================

const string MainWindow::versionString= "v1.3.5-beta1";
const string MainWindow::winHeader= "G3D viewer " + versionString + " - Built: " + __DATE__;

MainWindow::MainWindow(const string &modelPath)
    :	wxFrame(NULL, -1, ToUnicode(winHeader),wxPoint(Renderer::windowX, Renderer::windowY),
		wxSize(Renderer::windowW, Renderer::windowH))
{
    //getGlPlatformExtensions();
	renderer= Renderer::getInstance();
	this->modelPath= modelPath;
	model= NULL;
	playerColor= Renderer::pcRed;

	speed= 0.025f;

	glCanvas = new GlCanvas(this);

    //getGlPlatformExtensions();

	//glCanvas->SetCurrent();
	//renderer->init();

	menu= new wxMenuBar();

	//menu
	menuFile= new wxMenu();
	menuFile->Append(miFileLoad, wxT("Load"));
	menu->Append(menuFile, wxT("File"));

	//mode
	menuMode= new wxMenu();
	menuMode->AppendCheckItem(miModeNormals, wxT("Normals"));
	menuMode->AppendCheckItem(miModeWireframe, wxT("Wireframe"));
	menuMode->AppendCheckItem(miModeGrid, wxT("Grid"));
	menu->Append(menuMode, wxT("Mode"));

	//mode
	menuSpeed= new wxMenu();
	menuSpeed->Append(miSpeedSlower, wxT("Slower"));
	menuSpeed->Append(miSpeedFaster, wxT("Faster"));
	menu->Append(menuSpeed, wxT("Speed"));

	//custom color
	menuCustomColor= new wxMenu();
	menuCustomColor->AppendCheckItem(miColorRed, wxT("Red"));
	menuCustomColor->AppendCheckItem(miColorBlue, wxT("Blue"));
	menuCustomColor->AppendCheckItem(miColorYellow, wxT("Yellow"));
	menuCustomColor->AppendCheckItem(miColorGreen, wxT("Green"));
	menu->Append(menuCustomColor, wxT("Custom Color"));

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
	timer->Start(100);

	glCanvas->SetFocus();
}

MainWindow::~MainWindow(){
	delete renderer;
	delete model;
	delete timer;
	delete glCanvas;
}

void MainWindow::init(){
	glCanvas->SetCurrent();
	renderer->init();

	if(!modelPath.empty()){
		Model *tmpModel= new ModelGl();

		printf("In [%s::%s] modelPath = [%s]\n",__FILE__,__FUNCTION__,modelPath.c_str());

		renderer->loadTheModel(tmpModel, modelPath);
		model= tmpModel;

		GetStatusBar()->SetStatusText(ToUnicode(getModelInfo().c_str()));
	}

    //SetTitle(ToUnicode(winHeader + "; " + modelPath));
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
	fileDialog.SetWildcard(wxT("G3D files (*.g3d)|*.g3d"));
	if(fileDialog.ShowModal()==wxID_OK){
		delete model;
		Model *tmpModel= new ModelGl();
		renderer->loadTheModel(tmpModel, (const char*)wxFNCONV(fileDialog.GetPath().c_str()));
		model= tmpModel;
		GetStatusBar()->SetStatusText(ToUnicode(getModelInfo().c_str()));
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
    wxGLCanvas(mainWindow, -1, wxDefaultPosition)
{
    this->mainWindow = mainWindow;
}

void GlCanvas::onMouseMove(wxMouseEvent &event){
    mainWindow->onMouseMove(event);
}

BEGIN_EVENT_TABLE(GlCanvas, wxGLCanvas)
    EVT_MOTION(GlCanvas::onMouseMove)
END_EVENT_TABLE()

// ===============================================
// 	class App
// ===============================================

bool App::OnInit(){
	std::string modelPath;
	if(argc==2){
		modelPath= wxFNCONV(argv[1]);
	}

	mainWindow= new MainWindow(modelPath);
	mainWindow->Show();
	mainWindow->init();

	return true;
}

int App::MainLoop(){
	try{
		return wxApp::MainLoop();
	}
	catch(const exception &e){
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Exception"), wxOK | wxICON_ERROR).ShowModal();
		return 0;
	}
}

int App::OnExit(){
	return 0;
}

}}//end namespace

IMPLEMENT_APP(Shared::G3dViewer::App)
