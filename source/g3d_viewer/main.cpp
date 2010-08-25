#include "main.h"

#include <stdexcept>

#include "model_gl.h"
#include "graphics_interface.h"
#include "util.h"
#include "conversion.h"
#include "platform_common.h"
#include "xml_parser.h"
#include <iostream>
#include <wx/event.h>
#include "game_constants.h"

using namespace Shared::Platform;
using namespace Shared::PlatformCommon;
using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;
using namespace Shared::Util;
using namespace Shared::Xml;

using namespace std;
using namespace Glest::Game;

#ifdef _WIN32
const char *folderDelimiter = "\\";
#else
const char *folderDelimiter = "/";
#endif

int GameConstants::updateFps= 40;
int GameConstants::cameraFps= 100;

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

const string MainWindow::versionString= "v1.3.5";
const string MainWindow::winHeader= "G3D viewer " + versionString + " - Built: " + __DATE__;

MainWindow::MainWindow(const string &modelPath)
    :	wxFrame(NULL, -1, ToUnicode(winHeader),wxPoint(Renderer::windowX, Renderer::windowY),
		wxSize(Renderer::windowW, Renderer::windowH))
{
    //getGlPlatformExtensions();
	renderer= Renderer::getInstance();
	if(modelPath != "") {
		this->modelPathList.push_back(modelPath);
	}
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
	menuFile->Append(miFileLoadParticleXML, wxT("Load Particle XML"));
	menuFile->Append(miFileLoadProjectileParticleXML, wxT("Load Projectile Particle XML"));
	menuFile->Append(miFileClearAll, wxT("Clear All"));
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

	//std::cout << "A" << std::endl;
	wxInitAllImageHandlers();
#ifdef WIN32
	//std::cout << "B" << std::endl;
	wxIcon icon("IDI_ICON1");
#else
	//std::cout << "B" << std::endl;
	wxIcon icon;
	icon.LoadFile(wxT("g3dviewer.ico"),wxBITMAP_TYPE_ICO);
#endif
	//std::cout << "C" << std::endl;
	SetIcon(icon);

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

	loadModel("");

    //SetTitle(ToUnicode(winHeader + "; " + modelPath));
}

void MainWindow::onPaint(wxPaintEvent &event){
	renderer->reset(GetClientSize().x, GetClientSize().y, playerColor);

	renderer->transform(rotX, rotY, zoom);
	renderer->renderGrid();

	renderer->renderTheModel(model, anim);

	//int updateLoops = 100;
	int updateLoops = 1;
	for(int i=0; i< updateLoops; ++i) {
		renderer->updateParticleManager();
	}

	renderer->renderParticleManager();

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

	fileDialog.SetMessage(wxT("Selecting Glest Model for current view."));

	if(fileDialog.ShowModal()==wxID_OK){
		modelPathList.clear();
		loadModel((const char*)wxFNCONV(fileDialog.GetPath().c_str()));
	}
}

void MainWindow::onMenuFileLoadParticleXML(wxCommandEvent &event){
	string fileName;
	wxFileDialog fileDialog(this);
	fileDialog.SetWildcard(wxT("XML files (*.xml)|*.xml"));

	if(isControlKeyPressed == true) {
		fileDialog.SetMessage(wxT("Adding Mega-Glest particle to current view."));
	}
	else {
		fileDialog.SetMessage(wxT("Selecting Mega-Glest particle for current view."));
	}

	if(fileDialog.ShowModal()==wxID_OK){
		string path = (const char*)wxFNCONV(fileDialog.GetPath().c_str());
		loadParticle(path);
	}
}

void MainWindow::onMenuFileLoadProjectileParticleXML(wxCommandEvent &event){
	string fileName;
	wxFileDialog fileDialog(this);
	fileDialog.SetWildcard(wxT("XML files (*.xml)|*.xml"));

	if(isControlKeyPressed == true) {
		fileDialog.SetMessage(wxT("Adding Mega-Glest projectile particle to current view."));
	}
	else {
		fileDialog.SetMessage(wxT("Selecting Mega-Glest projectile particle for current view."));
	}

	if(fileDialog.ShowModal()==wxID_OK){
		string path = (const char*)wxFNCONV(fileDialog.GetPath().c_str());
		loadProjectileParticle(path);
	}
}

void MainWindow::onMenuFileClearAll(wxCommandEvent &event){
	modelPathList.clear();
	particlePathList.clear();
	particleProjectilePathList.clear();

	timer->Stop();
	renderer->end();

	unitParticleSystems.clear();
	unitParticleSystemTypes.clear();
	
	projectileParticleSystems.clear();
	projectileParticleSystemTypes.clear();

	delete model;
	model = NULL;

	loadModel("");
	loadParticle("");
	loadProjectileParticle("");

	timer->Start(100);
}

void MainWindow::loadModel(string path) {
	if(path != "" && fileExists(path) == true) {
		this->modelPathList.push_back(path);
	}

	for(int idx =0; idx < this->modelPathList.size(); idx++) {
		string modelPath = this->modelPathList[idx];

		timer->Stop();
		delete model;
		Model *tmpModel= new ModelGl();
		renderer->loadTheModel(tmpModel, modelPath);
		model= tmpModel;
		GetStatusBar()->SetStatusText(ToUnicode(getModelInfo().c_str()));
		timer->Start(100);
	}
}

void MainWindow::loadParticle(string path) {
	timer->Stop();
	if(path != "" && fileExists(path) == true) {
		renderer->end();
		unitParticleSystems.clear();
		unitParticleSystemTypes.clear();

		if(isControlKeyPressed == true) {
			std::cout << "Adding to list..." << std::endl;
			this->particlePathList.push_back(path);
		}
		else {
			std::cout << "Clearing list..." << std::endl;
			this->particlePathList.clear();
			this->particlePathList.push_back(path);
		}
	}

	if(this->particlePathList.size() > 0) {
		for(int idx = 0; idx < this->particlePathList.size(); idx++) {
			string particlePath = this->particlePathList[idx];
			string dir= extractDirectoryPathFromFile(particlePath);

			size_t pos = dir.find_last_of(folderDelimiter);
			if(pos == dir.length()-1) {
				dir.erase(dir.length() -1);
			}

			particlePath= extractFileFromDirectoryPath(particlePath);

			std::string unitXML = dir + folderDelimiter + extractFileFromDirectoryPath(dir) + ".xml";

			int size   = -1;
			int height = -1;

			if(fileExists(unitXML) == true) {
				XmlTree xmlTree;
				xmlTree.load(unitXML);
				const XmlNode *unitNode= xmlTree.getRootNode();
				const XmlNode *parametersNode= unitNode->getChild("parameters");
				//size
				size= parametersNode->getChild("size")->getAttribute("value")->getIntValue();
				//height
				height= parametersNode->getChild("height")->getAttribute("value")->getIntValue();
			}

			std::cout << "About to load [" << particlePath << "] from [" << dir << "] unit [" << unitXML << "]" << std::endl;

			UnitParticleSystemType *unitParticleSystemType= new UnitParticleSystemType();
			unitParticleSystemType->load(dir,  dir + folderDelimiter + particlePath, renderer);
			unitParticleSystemTypes.push_back(unitParticleSystemType);

			for(std::vector<UnitParticleSystemType *>::const_iterator it= unitParticleSystemTypes.begin(); it != unitParticleSystemTypes.end(); ++it) {
				UnitParticleSystem *ups= new UnitParticleSystem(200);
				(*it)->setValues(ups);
				if(size > 0) {
					//getCurrVectorFlat() + Vec3f(0.f, type->getHeight()/2.f, 0.f);
					Vec3f vec = Vec3f(0.f, height / 2.f, 0.f);
					ups->setPos(vec);
				}
				//ups->setFactionColor(getFaction()->getTexture()->getPixmap()->getPixel3f(0,0));
				ups->setFactionColor(renderer->getPlayerColorTexture(playerColor)->getPixmap()->getPixel3f(0,0));
				unitParticleSystems.push_back(ups);
				renderer->manageParticleSystem(ups);

				ups->setVisible(true);
			}

			if(path != "" && fileExists(path) == true) {
				renderer->initModelManager();
				renderer->initTextureManager();
			}
		}
	}

	timer->Start(100);
}

void MainWindow::loadProjectileParticle(string path) {
	timer->Stop();
	if(path != "" && fileExists(path) == true) {
		renderer->end();
		projectileParticleSystems.clear();
		projectileParticleSystemTypes.clear();

		if(isControlKeyPressed == true) {
			std::cout << "Adding to list..." << std::endl;
			this->particleProjectilePathList.push_back(path);
		}
		else {
			std::cout << "Clearing list..." << std::endl;
			this->particleProjectilePathList.clear();
			this->particleProjectilePathList.push_back(path);
		}
	}

	if(this->particleProjectilePathList.size() > 0) {

		for(int idx = 0; idx < this->particleProjectilePathList.size(); idx++) {
			string particlePath = this->particleProjectilePathList[idx];
			string dir= extractDirectoryPathFromFile(particlePath);

			size_t pos = dir.find_last_of(folderDelimiter);
			if(pos == dir.length()-1) {
				dir.erase(dir.length() -1);
			}

			particlePath= extractFileFromDirectoryPath(particlePath);

			std::string unitXML = dir + folderDelimiter + extractFileFromDirectoryPath(dir) + ".xml";

			int size   = -1;
			int height = -1;

			if(fileExists(unitXML) == true) {
				XmlTree xmlTree;
				xmlTree.load(unitXML);
				const XmlNode *unitNode= xmlTree.getRootNode();
				const XmlNode *parametersNode= unitNode->getChild("parameters");
				//size
				size= parametersNode->getChild("size")->getAttribute("value")->getIntValue();
				//height
				height= parametersNode->getChild("height")->getAttribute("value")->getIntValue();
			}

			std::cout << "About to load [" << particlePath << "] from [" << dir << "] unit [" << unitXML << "]" << std::endl;

			XmlTree xmlTree;
			xmlTree.load(dir + folderDelimiter + particlePath);
			const XmlNode *particleSystemNode= xmlTree.getRootNode();

			std::cout << "Loaded successfully, loading values..." << std::endl;

			ParticleSystemTypeProjectile *projectileParticleSystemType= new ParticleSystemTypeProjectile();
			projectileParticleSystemType->load(dir,  dir + folderDelimiter + particlePath,renderer);

			std::cout << "Values loaded, about to read..." << std::endl;

			projectileParticleSystemTypes.push_back(projectileParticleSystemType);

			for(std::vector<ParticleSystemTypeProjectile *>::const_iterator it= projectileParticleSystemTypes.begin(); it != projectileParticleSystemTypes.end(); ++it) {

				ProjectileParticleSystem *ps = (*it)->create();

				if(size > 0) {
					Vec3f vec = Vec3f(0.f, height / 2.f, 0.f);
					//ps->setPos(vec);

					Vec3f vec2 = Vec3f(size * 2.f, height * 2.f, height * 2.f);
					ps->setPath(vec, vec2);
				}
				ps->setFactionColor(renderer->getPlayerColorTexture(playerColor)->getPixmap()->getPixel3f(0,0));

				projectileParticleSystems.push_back(ps);

				ps->setVisible(true);
				renderer->manageParticleSystem(ps);

				//MessageBox(NULL,"hi","hi",MB_OK);
				//psProj= pstProj->create();
				//psProj->setPath(startPos, endPos);
				//psProj->setObserver(new ParticleDamager(unit, this, gameCamera));
				//psProj->setVisible(visible);
				//psProj->setFactionColor(unit->getFaction()->getTexture()->getPixmap()->getPixel3f(0,0));
				//renderer.manageParticleSystem(psProj, rsGame);
			}
		}

		if(path != "" && fileExists(path) == true) {
			renderer->initModelManager();
			renderer->initTextureManager();
		}
	}
	timer->Start(100);
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

void MainWindow::onKeyDown(wxKeyEvent &e) {

	std::cout << "e.ControlDown() = " << e.ControlDown() << " e.GetKeyCode() = " << e.GetKeyCode() << " isCtrl = " << (e.GetKeyCode() == WXK_CONTROL) << std::endl;

	if(e.ControlDown() == true || e.GetKeyCode() == WXK_CONTROL) {
		isControlKeyPressed = true;
	}
	else {
		isControlKeyPressed = false;
	}

	std::cout << "isControlKeyPressed = " << isControlKeyPressed << std::endl;

	/*
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
 	} else {
 		e.Skip();
	}
*/

	if (e.GetKeyCode() == 'R') {

		std::cout << "pressed R..." << std::endl;

		renderer->end();

		//renderer->end();

		unitParticleSystems.clear();
		unitParticleSystemTypes.clear();
		
		projectileParticleSystems.clear();
		projectileParticleSystemTypes.clear();

		loadModel("");
		loadParticle("");
		loadProjectileParticle("");

		renderer->initModelManager();
		renderer->initTextureManager();
	}
}

BEGIN_EVENT_TABLE(MainWindow, wxFrame)
	EVT_TIMER(-1, MainWindow::onTimer)
	EVT_CLOSE(MainWindow::onClose)
	EVT_MENU(miFileLoad, MainWindow::onMenuFileLoad)
	EVT_MENU(miFileLoadParticleXML, MainWindow::onMenuFileLoadParticleXML)
	EVT_MENU(miFileLoadProjectileParticleXML, MainWindow::onMenuFileLoadProjectileParticleXML)
	EVT_MENU(miFileClearAll, MainWindow::onMenuFileClearAll)

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

void translateCoords(wxWindow *wnd, int &x, int &y) {
#ifdef WIN32
	int cx, cy;
	wnd->GetPosition(&cx, &cy);
	x += cx;
	y += cy;
#endif
}

GlCanvas::GlCanvas(MainWindow *	mainWindow):
    wxGLCanvas(mainWindow, -1, wxDefaultPosition)
{
    this->mainWindow = mainWindow;
}

void GlCanvas::onMouseMove(wxMouseEvent &event){
    mainWindow->onMouseMove(event);
}

void GlCanvas::onKeyDown(wxKeyEvent &event) {
	int x, y;
	event.GetPosition(&x, &y);
	translateCoords(this, x, y);
	mainWindow->onKeyDown(event);
}

BEGIN_EVENT_TABLE(GlCanvas, wxGLCanvas)
    EVT_MOTION(GlCanvas::onMouseMove)
    EVT_KEY_DOWN(GlCanvas::onKeyDown)
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
