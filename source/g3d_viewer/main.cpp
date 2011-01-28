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
#ifndef WIN32
  #define stricmp strcasecmp
  #define strnicmp strncasecmp
  #define _strnicmp strncasecmp
#endif

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

const string g3dviewerVersionString= "v1.3.6";

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

const wxChar  *GAME_ARGS[] = {
	wxT("--help"),
	wxT("--autoscreenshot"),
	wxT("--loadmodel")
};

enum GAME_ARG_TYPE {
	GAME_ARG_HELP = 0,
	GAME_ARG_AUTO_SCREENSHOT,
	GAME_ARG_LOAD_MODEL
};

bool hasCommandArgument(int argc, wxChar** argv,const string argName,
						int *foundIndex=NULL, int startLookupIndex=1,
						bool useArgParamLen=false) {
	bool result = false;

	if(foundIndex != NULL) {
		*foundIndex = -1;
	}
	int compareLen = strlen(argName.c_str());

	for(int idx = startLookupIndex; idx < argc; idx++) {
		const wxWX2MBbuf tmp_buf = wxConvCurrent->cWX2MB(argv[idx]);
		//printf("tmp_buf [%s]\n",(const char *)tmp_buf);
		if(useArgParamLen == true) {
			compareLen = strlen(tmp_buf);
		}
		if(_strnicmp(argName.c_str(),tmp_buf,compareLen) == 0) {
			result = true;
			if(foundIndex != NULL) {
				*foundIndex = idx;
			}

			break;
		}
	}
	return result;
}

void printParameterHelp(const char *argv0, bool foundInvalidArgs) {
	if(foundInvalidArgs == true) {
			printf("\n");
	}

	printf("\n%s %s, usage\n",argv0,g3dviewerVersionString.c_str());

	printf("\n%s [G3D FILE]\n\n",argv0);
	printf("Displays glest 3D-models and unit/projectile/splash particle systems.\n");
	printf("rotate with left mouse button, zoom with right mouse button or mousewheel.\n");
	printf("Use ctrl to load more than one particle system.\n");
	printf("Press R to restart particles, this also reloads all files if they are changed.\n\n");

	printf("optionally you may use any of the following:\n");
	printf("Parameter:\t\tDescription:");
	printf("\n----------------------\t------------");
	printf("\n%s\t\t\tdisplays this help text.",(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_HELP]));
	printf("\n%s=x\t\tAuto load the model specified in path/filename x",(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_MODEL]));
	printf("\n                     \tWhere x is a G3d filename to load:");
	printf("\n                     \texample: %s %s=techs/megapack/factions/tech/units/battle_machine/models/battle_machine_dying.g3d",argv0,(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_MODEL]));
	printf("\n%s=x\tAutomatically takes a screenshot of the items you are loading.",(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_AUTO_SCREENSHOT]));
	printf("\n                     \tWhere x is a comma-delimited list of one or more of the optional settings:");
	printf("\n                     \ttransparent, enable_grid, enable_wireframe, enable_normals,");
	printf("\n                     \tdisable_grid, disable_wireframe, disable_normals, saveas-<filename>");
	printf("\n                     \texample: %s %s=transparent,disable_grid,saveas-test.png %s=techs/megapack/factions/tech/units/battle_machine/models/battle_machine_dying.g3d",argv0,(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_AUTO_SCREENSHOT]),(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_MODEL]));

	printf("\n\n");
}

bool autoScreenShotAndExit = false;
vector<string> autoScreenShotParams;

// ===============================================
// 	class MainWindow
// ===============================================

const string MainWindow::winHeader= "G3D viewer " + g3dviewerVersionString;

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

	int args[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER,  WX_GL_MIN_ALPHA,  8  }; // to prevent flicker
	glCanvas = new GlCanvas(this, args);

    //getGlPlatformExtensions();

	//glCanvas->SetCurrent();
	//renderer->init();
	menu= new wxMenuBar();

	//menu
	menuFile= new wxMenu();
	menuFile->Append(miFileLoad, wxT("&Load G3d model"), wxT("Load 3D model"));
	menuFile->Append(miFileLoadParticleXML, wxT("Load &Particle XML"), wxT("Press ctrl before menu for keeping current particles"));
	menuFile->Append(miFileLoadProjectileParticleXML, wxT("Load P&rojectile Particle XML"), wxT("Press ctrl before menu for keeping current projectile particles"));
	menuFile->Append(miFileLoadSplashParticleXML, wxT("Load &Splash Particle XML"), wxT("Press ctrl before menu for keeping current splash particles"));
	menuFile->Append(miFileClearAll, wxT("&Clear All"));
	menuFile->AppendCheckItem(miFileToggleScreenshotTransparent, wxT("&Transparent Screenshots"));
	menuFile->Append(miFileSaveScreenshot, wxT("Sa&ve a Screenshot"));
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT);
	menu->Append(menuFile, wxT("&File"));

	//mode
	menuMode= new wxMenu();
	menuMode->AppendCheckItem(miModeNormals, wxT("&Normals"));
	menuMode->AppendCheckItem(miModeWireframe, wxT("&Wireframe"));
	menuMode->AppendCheckItem(miModeGrid, wxT("&Grid"));
	menu->Append(menuMode, wxT("&Mode"));

	//mode
	menuSpeed= new wxMenu();
	menuSpeed->Append(miSpeedSlower, wxT("&Slower\t-"));
	menuSpeed->Append(miSpeedFaster, wxT("&Faster\t+"));
	menuSpeed->AppendSeparator();
	menuSpeed->Append(miRestart, wxT("&Restart particles\tR"), wxT("Restart particle animations, this also reloads model and particle files if they are changed"));
	menu->Append(menuSpeed, wxT("&Speed"));

	//custom color
	menuCustomColor= new wxMenu();
	menuCustomColor->Append(miChangeBackgroundColor, wxT("Change Background Color"));
	menuCustomColor->AppendCheckItem(miColorRed, wxT("&Red\t0"));
	menuCustomColor->AppendCheckItem(miColorBlue, wxT("&Blue\t1"));
	menuCustomColor->AppendCheckItem(miColorGreen, wxT("&Green\t2"));
	menuCustomColor->AppendCheckItem(miColorYellow, wxT("&Yellow\t3"));
	menuCustomColor->AppendCheckItem(miColorWhite, wxT("&White\t4"));
	menuCustomColor->AppendCheckItem(miColorCyan, wxT("&Cyan\t5"));
	menuCustomColor->AppendCheckItem(miColorOrange, wxT("&Orange\t6"));
	menuCustomColor->AppendCheckItem(miColorMagenta, wxT("&Pink\t7")); // it is called Pink everywhere else so...
	menu->Append(menuCustomColor, wxT("&Custom Color"));

	menuMode->Check(miModeGrid, true);
	menuCustomColor->Check(miColorRed, true);

    for(int i = 0; i < autoScreenShotParams.size(); ++i) {
    	if(autoScreenShotParams[i] == "transparent") {
    		printf("Screenshot option [%s]\n",autoScreenShotParams[i].c_str());
    		menuFile->Check(miFileToggleScreenshotTransparent,true);
            float alpha = 0.0f;
            renderer->setAlphaColor(alpha);
    	}
    	if(autoScreenShotParams[i] == "enable_grid") {
    		printf("Screenshot option [%s]\n",autoScreenShotParams[i].c_str());
    		menuMode->Check(miModeGrid,true);
    		if(renderer->getGrid() == false) {
    			renderer->toggleGrid();
    		}
    	}
    	if(autoScreenShotParams[i] == "enable_wireframe") {
    		printf("Screenshot option [%s]\n",autoScreenShotParams[i].c_str());
    		menuMode->Check(miModeWireframe,true);
    		if(renderer->getWireframe() == false) {
    			renderer->toggleWireframe();
    		}
    	}
    	if(autoScreenShotParams[i] == "enable_normals") {
    		printf("Screenshot option [%s]\n",autoScreenShotParams[i].c_str());
    		menuMode->Check(miModeNormals,true);
    		if(renderer->getNormals() == false) {
    			renderer->toggleNormals();
    		}
    	}
    	if(autoScreenShotParams[i] == "disable_grid") {
    		printf("Screenshot option [%s]\n",autoScreenShotParams[i].c_str());
    		menuMode->Check(miModeGrid,false);
    		if(renderer->getGrid() == true) {
    			renderer->toggleGrid();
    		}
    	}
    	if(autoScreenShotParams[i] == "enable_wireframe") {
    		printf("Screenshot option [%s]\n",autoScreenShotParams[i].c_str());
    		menuMode->Check(miModeWireframe,false);
    		if(renderer->getWireframe() == true) {
    			renderer->toggleWireframe();
    		}
    	}
    	if(autoScreenShotParams[i] == "enable_normals") {
    		printf("Screenshot option [%s]\n",autoScreenShotParams[i].c_str());
    		menuMode->Check(miModeNormals,false);
    		if(renderer->getNormals() == true) {
    			renderer->toggleNormals();
    		}
    	}
    }

	SetMenuBar(menu);

	//misc
	model= NULL;
	rotX= 0.0f;
	rotY= 0.0f;
	zoom= 1.0f;
	backBrightness= 0.3f;
	gridBrightness= 1.0f;
	lightBrightness= 0.3f;
	lastX= 0;
	lastY= 0;
	anim= 0.0f;
	statusbarText="";
	CreateStatusBar();

	wxInitAllImageHandlers();
#ifdef WIN32

#if defined(__MINGW32__)
	wxIcon icon(ToUnicode("IDI_ICON1"));
#else
    wxIcon icon("IDI_ICON1");
#endif

#else
	wxIcon icon;
	std::ifstream testFile("g3dviewer.ico");
	if(testFile.good())	{
		testFile.close();
		icon.LoadFile(wxT("g3dviewer.ico"),wxBITMAP_TYPE_ICO);
	}
#endif
	SetIcon(icon);

	timer = new wxTimer(this);
	timer->Start(100);

	glCanvas->SetFocus();

	fileDialog = new wxFileDialog(this);
	if(modelPath != "") {
		fileDialog->SetPath(ToUnicode(modelPath));
	}
}

MainWindow::~MainWindow(){
	delete renderer;
	delete model;
	delete timer;
	delete glCanvas;

}

void MainWindow::init() {
	glCanvas->SetCurrent();
	renderer->init();

	loadModel("");
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

	if(autoScreenShotAndExit == true) {
		autoScreenShotAndExit = false;
		saveScreenshot();
		Close();
	}
}

void MainWindow::onClose(wxCloseEvent &event){
	// release memory first (from onMenuFileClearAll)

	modelPathList.clear();
	particlePathList.clear();
	particleProjectilePathList.clear();
	particleSplashPathList.clear(); // as above

	timer->Stop();
	renderer->end();

	unitParticleSystems.clear();
	unitParticleSystemTypes.clear();

	projectileParticleSystems.clear();
	projectileParticleSystemTypes.clear();
	splashParticleSystems.clear(); // as above
	splashParticleSystemTypes.clear();

	delete model;
	model = NULL;

	delete this;
}

// for the mousewheel
void MainWindow::onMouseWheelDown(wxMouseEvent &event) {
	try {
		wxPaintEvent paintEvent;
		zoom*= 1.1f;
		zoom= clamp(zoom, 0.1f, 10.0f);
		onPaint(paintEvent);
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMouseWheelUp(wxMouseEvent &event) {
	try {
		wxPaintEvent paintEvent;
		zoom*= 0.90909f;
		zoom= clamp(zoom, 0.1f, 10.0f);
		onPaint(paintEvent);
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}


void MainWindow::onMouseMove(wxMouseEvent &event){
	try {
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
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuFileLoad(wxCommandEvent &event){
	try {
		string fileName;
		fileDialog->SetWildcard(wxT("G3D files (*.g3d)|*.g3d;*.G3D"));
		fileDialog->SetMessage(wxT("Selecting Glest Model for current view."));

		if(fileDialog->ShowModal()==wxID_OK){
			modelPathList.clear();
			loadModel((const char*)wxFNCONV(fileDialog->GetPath().c_str()));
		}
		isControlKeyPressed = false;
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuFileLoadParticleXML(wxCommandEvent &event){
	try {
		string fileName;
		fileDialog->SetWildcard(wxT("XML files (*.xml)|*.xml"));

		if(isControlKeyPressed == true) {
			fileDialog->SetMessage(wxT("Adding Mega-Glest particle to current view."));
		}
		else {
			fileDialog->SetMessage(wxT("Selecting Mega-Glest particle for current view."));
		}

		if(fileDialog->ShowModal()==wxID_OK){
			string path = (const char*)wxFNCONV(fileDialog->GetPath().c_str());
			loadParticle(path);
		}
		isControlKeyPressed = false;
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuFileLoadProjectileParticleXML(wxCommandEvent &event){
	try {
		string fileName;
		fileDialog->SetWildcard(wxT("XML files (*.xml)|*.xml"));

		if(isControlKeyPressed == true) {
			fileDialog->SetMessage(wxT("Adding Mega-Glest projectile particle to current view."));
		}
		else {
			fileDialog->SetMessage(wxT("Selecting Mega-Glest projectile particle for current view."));
		}

		if(fileDialog->ShowModal()==wxID_OK){
			string path = (const char*)wxFNCONV(fileDialog->GetPath().c_str());
			loadProjectileParticle(path);
		}
		isControlKeyPressed = false;
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuFileLoadSplashParticleXML(wxCommandEvent &event){
	try {
		string fileName;
		fileDialog->SetWildcard(wxT("XML files (*.xml)|*.xml"));

		if(isControlKeyPressed == true) {
			fileDialog->SetMessage(wxT("Adding Mega-Glest splash particle to current view."));
		}
		else {
			fileDialog->SetMessage(wxT("Selecting Mega-Glest splash particle for current view."));
		}

		if(fileDialog->ShowModal()==wxID_OK){
			string path = (const char*)wxFNCONV(fileDialog->GetPath().c_str());
			loadSplashParticle(path);
		}
		isControlKeyPressed = false;
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}  // is it possible to join loadParticle(), loadProjectileParticle() and loadSplashParticle() to one method?


void MainWindow::OnChangeColor(wxCommandEvent &event) {
	try {
		//wxColour color = colorPicker->GetColour();
		wxColourData data;
		data.SetChooseFull(true);
		for (int i = 0; i < 16; i++)
		{
			wxColour colour(i*16, i*16, i*16);
			data.SetCustomColour(i, colour);
		}

		wxColourDialog dialog(this, &data);
		if (dialog.ShowModal() == wxID_OK)
		{
			wxColourData retData = dialog.GetColourData();
			wxColour col = retData.GetColour();

			//renderer->setBackgroundColor(col.Red()/255.0f, col.Green()/255.0f, col.Blue()/255.0f, col.Alpha()/255.0f);
			renderer->setBackgroundColor(col.Red()/255.0f, col.Green()/255.0f, col.Blue()/255.0f);
			//renderer->setBackgroundColor(0.0f, 0.0f, 0.0f, 0.0f);
		}
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenumFileToggleScreenshotTransparent(wxCommandEvent &event) {
    try {
        float alpha = (event.IsChecked() == true ? 0.0f : 1.0f);
        renderer->setAlphaColor(alpha);
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::saveScreenshot() {
	try {
		int autoSaveScreenshotIndex = -1;
	    for(int i = 0; i < autoScreenShotParams.size(); ++i) {
			if(_strnicmp(autoScreenShotParams[i].c_str(),"saveas-",7) == 0) {
	    		printf("Screenshot option [%s]\n",autoScreenShotParams[i].c_str());
	    		autoSaveScreenshotIndex = i;
	    		break;
	    	}
	    }
	    if(autoSaveScreenshotIndex >= 0) {
	    	string saveAsFilename = autoScreenShotParams[autoSaveScreenshotIndex];
	    	saveAsFilename.erase(0,7);
			FILE *f= fopen(saveAsFilename.c_str(), "rb");
			if(f == NULL) {
				renderer->saveScreen(saveAsFilename.c_str());
			}
			else {
				fclose(f);
			}
	    }
	    else {
			string path = "screens/";
			if(isdir(path.c_str()) == true) {
				//Config &config= Config::getInstance();
				//string fileFormat = config.getString("ScreenShotFileType","png");
				string fileFormat = "png";

				for(int i=0; i < 1000; ++i) {
					path = "screens/";
					path += string("screen") + intToStr(i) + string(".") + fileFormat;
					FILE *f= fopen(path.c_str(), "rb");
					if(f == NULL) {
						renderer->saveScreen(path);
						break;
					}
					else {
						fclose(f);
					}
				}
			}
		}
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuFileSaveScreenshot(wxCommandEvent &event) {
	saveScreenshot();
}

void MainWindow::onMenuFileClearAll(wxCommandEvent &event) {
	try {
		modelPathList.clear();
		particlePathList.clear();
		particleProjectilePathList.clear();
		particleSplashPathList.clear(); // as above

		timer->Stop();
		renderer->end();

		unitParticleSystems.clear();
		unitParticleSystemTypes.clear();

		projectileParticleSystems.clear();
		projectileParticleSystemTypes.clear();
		splashParticleSystems.clear(); // as above
		splashParticleSystemTypes.clear();

		delete model;
		model = NULL;

		loadModel("");
		loadParticle("");
		loadProjectileParticle("");
		loadSplashParticle(""); // as above

		GetStatusBar()->SetStatusText(ToUnicode(statusbarText.c_str()));
		timer->Start(100);
		isControlKeyPressed = false;
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuFileExit(wxCommandEvent &event) {
	Close();
}

void MainWindow::loadModel(string path) {
    try {
        if(path != "" && fileExists(path) == true) {
            this->modelPathList.push_back(path);
        }

        string titlestring=winHeader;
        for(unsigned int idx =0; idx < this->modelPathList.size(); idx++) {
            string modelPath = this->modelPathList[idx];

            timer->Stop();
            delete model;
            Model *tmpModel= new ModelGl();
            renderer->loadTheModel(tmpModel, modelPath);
            model= tmpModel;

            statusbarText = getModelInfo();
            string statusTextValue = statusbarText + " animation speed: " + floatToStr(speed * 1000.0);
            GetStatusBar()->SetStatusText(ToUnicode(statusTextValue.c_str()));
            timer->Start(100);
            titlestring =  extractFileFromDirectoryPath(modelPath) + " - "+ titlestring;
        }
        SetTitle(ToUnicode(titlestring));
    }
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::loadParticle(string path) {
	timer->Stop();
	if(path != "" && fileExists(path) == true) {
		renderer->end();
		unitParticleSystems.clear();
		unitParticleSystemTypes.clear();

		if(isControlKeyPressed == true) {
			// std::cout << "Adding to list..." << std::endl;
			this->particlePathList.push_back(path);
		}
		else {
			// std::cout << "Clearing list..." << std::endl;
			this->particlePathList.clear();
			this->particlePathList.push_back(path);
		}
	}

	try{
	if(this->particlePathList.size() > 0) {
        string titlestring=winHeader;
		for(unsigned int idx = 0; idx < this->particlePathList.size(); idx++) {
			string particlePath = this->particlePathList[idx];
			string dir= extractDirectoryPathFromFile(particlePath);

			size_t pos = dir.find_last_of(folderDelimiter);
			if(pos == dir.length()-1) {
				dir.erase(dir.length() -1);
			}

			particlePath= extractFileFromDirectoryPath(particlePath);
			titlestring = particlePath  + " - "+ titlestring;

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

			// std::cout << "About to load [" << particlePath << "] from [" << dir << "] unit [" << unitXML << "]" << std::endl;

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
		SetTitle(ToUnicode(titlestring));
	}
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Not a Mega-Glest particle XML file, or broken"), wxOK | wxICON_ERROR).ShowModal();
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
			// std::cout << "Adding to list..." << std::endl;
			this->particleProjectilePathList.push_back(path);
		}
		else {
			// std::cout << "Clearing list..." << std::endl;
			this->particleProjectilePathList.clear();
			this->particleProjectilePathList.push_back(path);
		}
	}

	try {
	if(this->particleProjectilePathList.size() > 0) {
        string titlestring=winHeader;
		for(unsigned int idx = 0; idx < this->particleProjectilePathList.size(); idx++) {
			string particlePath = this->particleProjectilePathList[idx];
			string dir= extractDirectoryPathFromFile(particlePath);

			size_t pos = dir.find_last_of(folderDelimiter);
			if(pos == dir.length()-1) {
				dir.erase(dir.length() -1);
			}

			particlePath= extractFileFromDirectoryPath(particlePath);
			titlestring = particlePath  + " - "+ titlestring;

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

			// std::cout << "About to load [" << particlePath << "] from [" << dir << "] unit [" << unitXML << "]" << std::endl;

			XmlTree xmlTree;
			xmlTree.load(dir + folderDelimiter + particlePath);
			const XmlNode *particleSystemNode= xmlTree.getRootNode();

			// std::cout << "Loaded successfully, loading values..." << std::endl;

			ParticleSystemTypeProjectile *projectileParticleSystemType= new ParticleSystemTypeProjectile();
			projectileParticleSystemType->load(dir,  dir + folderDelimiter + particlePath,renderer);

			// std::cout << "Values loaded, about to read..." << std::endl;

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
		SetTitle(ToUnicode(titlestring));

		if(path != "" && fileExists(path) == true) {
			renderer->initModelManager();
			renderer->initTextureManager();
		}
	}
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Not a Mega-Glest projectile particle XML file, or broken"), wxOK | wxICON_ERROR).ShowModal();
	}
	timer->Start(100);
}

void MainWindow::loadSplashParticle(string path) {  // uses ParticleSystemTypeSplash::load  (and own list...)
	timer->Stop();
	if(path != "" && fileExists(path) == true) {
		renderer->end();
		splashParticleSystems.clear();
		splashParticleSystemTypes.clear();

		if(isControlKeyPressed == true) {
			// std::cout << "Adding to list..." << std::endl;
			this->particleSplashPathList.push_back(path);
		}
		else {
			// std::cout << "Clearing list..." << std::endl;
			this->particleSplashPathList.clear();
			this->particleSplashPathList.push_back(path);
		}
	}

	try {
	if(this->particleSplashPathList.size() > 0) {
        string titlestring=winHeader;
		for(unsigned int idx = 0; idx < this->particleSplashPathList.size(); idx++) {
			string particlePath = this->particleSplashPathList[idx];
			string dir= extractDirectoryPathFromFile(particlePath);

			size_t pos = dir.find_last_of(folderDelimiter);
			if(pos == dir.length()-1) {
				dir.erase(dir.length() -1);
			}

			particlePath= extractFileFromDirectoryPath(particlePath);
			titlestring = particlePath  + " - "+ titlestring;

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

			// std::cout << "About to load [" << particlePath << "] from [" << dir << "] unit [" << unitXML << "]" << std::endl;

			XmlTree xmlTree;
			xmlTree.load(dir + folderDelimiter + particlePath);
			const XmlNode *particleSystemNode= xmlTree.getRootNode();

			// std::cout << "Loaded successfully, loading values..." << std::endl;

			ParticleSystemTypeSplash *splashParticleSystemType= new ParticleSystemTypeSplash();
			splashParticleSystemType->load(dir,  dir + folderDelimiter + particlePath,renderer); // <---- only that must be splash...

			// std::cout << "Values loaded, about to read..." << std::endl;

			splashParticleSystemTypes.push_back(splashParticleSystemType);

                                      //ParticleSystemTypeSplash
			for(std::vector<ParticleSystemTypeSplash *>::const_iterator it= splashParticleSystemTypes.begin(); it != splashParticleSystemTypes.end(); ++it) {

				SplashParticleSystem *ps = (*it)->create();

				if(size > 0) {
					Vec3f vec = Vec3f(0.f, height / 2.f, 0.f);
					//ps->setPos(vec);

					//Vec3f vec2 = Vec3f(size * 2.f, height * 2.f, height * 2.f);   // <------- removed relative projectile
					//ps->setPath(vec, vec2);                                       // <------- removed relative projectile
				}
				ps->setFactionColor(renderer->getPlayerColorTexture(playerColor)->getPixmap()->getPixel3f(0,0));

				splashParticleSystems.push_back(ps);

				ps->setVisible(true);
				renderer->manageParticleSystem(ps);
			}
		}
		SetTitle(ToUnicode(titlestring));

		if(path != "" && fileExists(path) == true) {
			renderer->initModelManager();
			renderer->initTextureManager();
		}
	}
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Not a Mega-Glest projectile particle XML file, or broken"), wxOK | wxICON_ERROR).ShowModal();
	}
	timer->Start(100);
}

void MainWindow::onMenuModeNormals(wxCommandEvent &event){
	try {
		renderer->toggleNormals();
		menuMode->Check(miModeNormals, renderer->getNormals());
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuModeWireframe(wxCommandEvent &event){
	try {
		renderer->toggleWireframe();
		menuMode->Check(miModeWireframe, renderer->getWireframe());
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuModeGrid(wxCommandEvent &event){
	try {
		renderer->toggleGrid();
		menuMode->Check(miModeGrid, renderer->getGrid());
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuSpeedSlower(wxCommandEvent &event){
	try {
		speed /= 1.5f;
		if(speed < 0) {
			speed = 0;
		}

		string statusTextValue = statusbarText + " animation speed: " + floatToStr(speed * 1000.0);
		GetStatusBar()->SetStatusText(ToUnicode(statusTextValue.c_str()));
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuSpeedFaster(wxCommandEvent &event){
	try {
		speed *= 1.5f;
		if(speed > 1) {
			speed = 1;
		}

		string statusTextValue = statusbarText + " animation speed: " + floatToStr(speed * 1000.0 );
		GetStatusBar()->SetStatusText(ToUnicode(statusTextValue.c_str()));
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

// set menu checkboxes to what player color is used
void MainWindow::onMenuColorRed(wxCommandEvent &event) {
	try {
		playerColor= Renderer::pcRed;
		menuCustomColor->Check(miColorRed, true);
		menuCustomColor->Check(miColorBlue, false);
		menuCustomColor->Check(miColorGreen, false);
		menuCustomColor->Check(miColorYellow, false);
		menuCustomColor->Check(miColorWhite, false);
		menuCustomColor->Check(miColorCyan, false);
		menuCustomColor->Check(miColorOrange, false);
		menuCustomColor->Check(miColorMagenta, false);
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuColorBlue(wxCommandEvent &event) {
	try {
		playerColor= Renderer::pcBlue;
		menuCustomColor->Check(miColorRed, false);
		menuCustomColor->Check(miColorBlue, true);
		menuCustomColor->Check(miColorGreen, false);
		menuCustomColor->Check(miColorYellow, false);
		menuCustomColor->Check(miColorWhite, false);
		menuCustomColor->Check(miColorCyan, false);
		menuCustomColor->Check(miColorOrange, false);
		menuCustomColor->Check(miColorMagenta, false);
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuColorGreen(wxCommandEvent &event) {
	try {
		playerColor= Renderer::pcGreen;
		menuCustomColor->Check(miColorRed, false);
		menuCustomColor->Check(miColorBlue, false);
		menuCustomColor->Check(miColorGreen, true);
		menuCustomColor->Check(miColorYellow, false);
		menuCustomColor->Check(miColorWhite, false);
		menuCustomColor->Check(miColorCyan, false);
		menuCustomColor->Check(miColorOrange, false);
		menuCustomColor->Check(miColorMagenta, false);
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuColorYellow(wxCommandEvent &event) {
	try {
		playerColor= Renderer::pcYellow;
		menuCustomColor->Check(miColorRed, false);
		menuCustomColor->Check(miColorBlue, false);
		menuCustomColor->Check(miColorGreen, false);
		menuCustomColor->Check(miColorYellow, true);
		menuCustomColor->Check(miColorWhite, false);
		menuCustomColor->Check(miColorCyan, false);
		menuCustomColor->Check(miColorOrange, false);
		menuCustomColor->Check(miColorMagenta, false);
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuColorWhite(wxCommandEvent &event) {
	try {
		playerColor= Renderer::pcWhite;
		menuCustomColor->Check(miColorRed, false);
		menuCustomColor->Check(miColorBlue, false);
		menuCustomColor->Check(miColorGreen, false);
		menuCustomColor->Check(miColorYellow, false);
		menuCustomColor->Check(miColorWhite, true);
		menuCustomColor->Check(miColorCyan, false);
		menuCustomColor->Check(miColorOrange, false);
		menuCustomColor->Check(miColorMagenta, false);
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuColorCyan(wxCommandEvent &event) {
	try {
		playerColor= Renderer::pcCyan;
		menuCustomColor->Check(miColorRed, false);
		menuCustomColor->Check(miColorBlue, false);
		menuCustomColor->Check(miColorGreen, false);
		menuCustomColor->Check(miColorYellow, false);
		menuCustomColor->Check(miColorWhite, false);
		menuCustomColor->Check(miColorCyan, true);
		menuCustomColor->Check(miColorOrange, false);
		menuCustomColor->Check(miColorMagenta, false);
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuColorOrange(wxCommandEvent &event) {
	try {
		playerColor= Renderer::pcOrange;
		menuCustomColor->Check(miColorRed, false);
		menuCustomColor->Check(miColorBlue, false);
		menuCustomColor->Check(miColorGreen, false);
		menuCustomColor->Check(miColorYellow, false);
		menuCustomColor->Check(miColorWhite, false);
		menuCustomColor->Check(miColorCyan, false);
		menuCustomColor->Check(miColorOrange, true);
		menuCustomColor->Check(miColorMagenta, false);
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuColorMagenta(wxCommandEvent &event) {
	try {
		playerColor= Renderer::pcMagenta;
		menuCustomColor->Check(miColorRed, false);
		menuCustomColor->Check(miColorBlue, false);
		menuCustomColor->Check(miColorGreen, false);
		menuCustomColor->Check(miColorYellow, false);
		menuCustomColor->Check(miColorWhite, false);
		menuCustomColor->Check(miColorCyan, false);
		menuCustomColor->Check(miColorOrange, false);
		menuCustomColor->Check(miColorMagenta, true);
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}


void MainWindow::onTimer(wxTimerEvent &event) {
	wxPaintEvent paintEvent;

	anim = anim + speed;
	if(anim > 1.0f){
		anim -= 1.f;
	}
	onPaint(paintEvent);
}

string MainWindow::getModelInfo() {
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

	try {
		// std::cout << "e.ControlDown() = " << e.ControlDown() << " e.GetKeyCode() = " << e.GetKeyCode() << " isCtrl = " << (e.GetKeyCode() == WXK_CONTROL) << std::endl;

		// Note: This ctrl-key handling is buggy since it never resests when ctrl is released later, so I reset it at end of loadcommands for now.
		if(e.ControlDown() == true || e.GetKeyCode() == WXK_CONTROL) {
			isControlKeyPressed = true;
		}
		else {
			isControlKeyPressed = false;
		}

		// std::cout << "isControlKeyPressed = " << isControlKeyPressed << std::endl;


		// here also because + and - hotkeys don't work for numpad automaticly
		if (e.GetKeyCode() == 388) {
			speed *= 1.5f; //numpad+
			if(speed > 1.0) {
				speed = 1.0;
			}

			string statusTextValue = statusbarText + " animation speed: " + floatToStr(speed * 1000.0);
			GetStatusBar()->SetStatusText(ToUnicode(statusTextValue.c_str()));

		}
		else if (e.GetKeyCode() == 390) {
			speed /= 1.5f; //numpad-
			if(speed < 0) {
				speed = 0;
			}
			string statusTextValue = statusbarText + " animation speed: " + floatToStr(speed * 1000.0);
			GetStatusBar()->SetStatusText(ToUnicode(statusTextValue.c_str()));
		}
		else if (e.GetKeyCode() == 87) {
			glClearColor(0.6f, 0.6f, 0.6f, 1.0f); //w  key //backgroundcolor constant 0.3 -> 0.6

		}

		// some posibility to adjust brightness:
		/*
		else if (e.GetKeyCode() == 322) { // Ins  - Grid
			gridBrightness += 0.1f; if (gridBrightness >1.0) gridBrightness =1.0;
		}
		else if (e.GetKeyCode() == 127) { // Del
			gridBrightness -= 0.1f; if (gridBrightness <0) gridBrightness =0;
		}
		*/
		else if (e.GetKeyCode() == 313) { // Home  - Background
			backBrightness += 0.1f; if (backBrightness >1.0) backBrightness=1.0;
			glClearColor(backBrightness, backBrightness, backBrightness, 1.0f);
		}
		else if (e.GetKeyCode() == 312) { // End
			backBrightness -= 0.1f; if (backBrightness<0) backBrightness=0;
			glClearColor(backBrightness, backBrightness, backBrightness, 1.0f);
		}
		else if (e.GetKeyCode() == 366) { // PgUp  - Lightning of model
			lightBrightness += 0.1f; if (lightBrightness >1.0) lightBrightness =1.0;
			Vec4f ambientNEW= Vec4f(lightBrightness, lightBrightness, lightBrightness, 1.0f);
			glLightfv(GL_LIGHT0,GL_AMBIENT, ambientNEW.ptr());
		}
		else if (e.GetKeyCode() == 367) { // pgDn
			lightBrightness -= 0.1f; if (lightBrightness <0) lightBrightness =0;
			Vec4f ambientNEW= Vec4f(lightBrightness, lightBrightness, lightBrightness, 1.0f);
			glLightfv(GL_LIGHT0,GL_AMBIENT, ambientNEW.ptr());
		}
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

void MainWindow::onMenuRestart(wxCommandEvent &event) {
	try {
		// std::cout << "pressed R (restart particle animation)" << std::endl;
		renderer->end();

		unitParticleSystems.clear();
		unitParticleSystemTypes.clear();
		projectileParticleSystems.clear();
		projectileParticleSystemTypes.clear();
		splashParticleSystems.clear(); // as above
		splashParticleSystemTypes.clear();

		loadModel("");
		loadParticle("");
		loadProjectileParticle("");
		loadSplashParticle(""); // as above

		renderer->initModelManager();
		renderer->initTextureManager();
	}
	catch(std::runtime_error e) {
		std::cout << e.what() << std::endl;
		wxMessageDialog(NULL, ToUnicode(e.what()), ToUnicode("Error"), wxOK | wxICON_ERROR).ShowModal();
	}
}

BEGIN_EVENT_TABLE(MainWindow, wxFrame)
	EVT_TIMER(-1, MainWindow::onTimer)
	EVT_CLOSE(MainWindow::onClose)
	EVT_MENU(miFileLoad, MainWindow::onMenuFileLoad)
	EVT_MENU(miFileLoadParticleXML, MainWindow::onMenuFileLoadParticleXML)
	EVT_MENU(miFileLoadProjectileParticleXML, MainWindow::onMenuFileLoadProjectileParticleXML)
	EVT_MENU(miFileLoadSplashParticleXML, MainWindow::onMenuFileLoadSplashParticleXML)
	EVT_MENU(miFileClearAll, MainWindow::onMenuFileClearAll)
	EVT_MENU(miFileToggleScreenshotTransparent, MainWindow::onMenumFileToggleScreenshotTransparent)
	EVT_MENU(miFileSaveScreenshot, MainWindow::onMenuFileSaveScreenshot)
	EVT_MENU(wxID_EXIT, MainWindow::onMenuFileExit)

	EVT_MENU(miModeWireframe, MainWindow::onMenuModeWireframe)
	EVT_MENU(miModeNormals, MainWindow::onMenuModeNormals)
	EVT_MENU(miModeGrid, MainWindow::onMenuModeGrid)

	EVT_MENU(miSpeedFaster, MainWindow::onMenuSpeedFaster)
	EVT_MENU(miSpeedSlower, MainWindow::onMenuSpeedSlower)
	EVT_MENU(miRestart, MainWindow::onMenuRestart)

	EVT_MENU(miColorRed, MainWindow::onMenuColorRed)
	EVT_MENU(miColorBlue, MainWindow::onMenuColorBlue)
	EVT_MENU(miColorGreen, MainWindow::onMenuColorGreen)
	EVT_MENU(miColorYellow, MainWindow::onMenuColorYellow)
	EVT_MENU(miColorWhite, MainWindow::onMenuColorWhite)
	EVT_MENU(miColorCyan, MainWindow::onMenuColorCyan)
	EVT_MENU(miColorOrange, MainWindow::onMenuColorOrange)
	EVT_MENU(miColorMagenta, MainWindow::onMenuColorMagenta)

	EVT_MENU(miChangeBackgroundColor, MainWindow::OnChangeColor)
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

// to prevent flicker
GlCanvas::GlCanvas(MainWindow *	mainWindow, int *args)
		: wxGLCanvas(mainWindow, -1, wxDefaultPosition, wxDefaultSize, 0, wxT("GLCanvas"), args) {
	this->mainWindow = mainWindow;
	//
}

// for the mousewheel
void GlCanvas::onMouseWheel(wxMouseEvent &event) {
	if(event.GetWheelRotation()>0) mainWindow->onMouseWheelDown(event);
	else mainWindow->onMouseWheelUp(event);
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

//    EVT_SPIN_DOWN(GlCanvas::onMouseDown)
//    EVT_SPIN_UP(GlCanvas::onMouseDown)
//    EVT_MIDDLE_DOWN(GlCanvas::onMouseWheel)
//    EVT_MIDDLE_UP(GlCanvas::onMouseWheel)

BEGIN_EVENT_TABLE(GlCanvas, wxGLCanvas)
    EVT_MOUSEWHEEL(GlCanvas::onMouseWheel)
    EVT_MOTION(GlCanvas::onMouseMove)
    EVT_KEY_DOWN(GlCanvas::onKeyDown)
END_EVENT_TABLE()

// ===============================================
// 	class App
// ===============================================

bool App::OnInit(){
	std::string modelPath="";

/*
    if(hasCommandArgument(argc, argv,GAME_ARGS[GAME_ARG_DATA_PATH]) == true) {
        int foundParamIndIndex = -1;
        hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_DATA_PATH]) + string("="),&foundParamIndIndex);
        if(foundParamIndIndex < 0) {
            hasCommandArgument(argc, argv,string(GAME_ARGS[GAME_ARG_DATA_PATH]),&foundParamIndIndex);
        }
        string customPath = argv[foundParamIndIndex];
        vector<string> paramPartTokens;
        Tokenize(customPath,paramPartTokens,"=");
        if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
            string customPathValue = paramPartTokens[1];
            pathCache[GameConstants::path_data_CacheLookupKey]=customPathValue;
            if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Using custom data path [%s]\n",customPathValue.c_str());
        }
        else {

            printf("\nInvalid path specified on commandline [%s] value [%s]\n\n",argv[foundParamIndIndex],(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
            printParameterHelp(argv[0],false);
            return -1;
        }
    }
*/

	bool foundInvalidArgs = false;
	const int knownArgCount = sizeof(GAME_ARGS) / sizeof(GAME_ARGS[0]);
	for(int idx = 1; idx < argc; ++idx) {
		const wxWX2MBbuf tmp_buf = wxConvCurrent->cWX2MB(argv[idx]);
		if( hasCommandArgument(knownArgCount, (wxChar**)&GAME_ARGS[0], (const char *)tmp_buf, NULL, 0, true) == false &&
			argv[idx][0] == '-') {
			foundInvalidArgs = true;

			printf("\nInvalid argument: %s",(const char*)tmp_buf);
		}
	}

    if(foundInvalidArgs == true ||
    	hasCommandArgument(argc, argv,(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_HELP])) == true) {
    	printParameterHelp(wxConvCurrent->cWX2MB(argv[0]),foundInvalidArgs);
		return false;
    }

    if(hasCommandArgument(argc, argv,(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_AUTO_SCREENSHOT])) == true) {
    	autoScreenShotAndExit = true;

    	const wxWX2MBbuf param = (const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_AUTO_SCREENSHOT]);
    	//printf("param = [%s]\n",(const char*)param);

    	int foundParamIndIndex = -1;
        hasCommandArgument(argc, argv,string((const char*)param) + string("="),&foundParamIndIndex);
        if(foundParamIndIndex < 0) {
            hasCommandArgument(argc, argv,(const char*)param,&foundParamIndIndex);
        }
        //printf("foundParamIndIndex = %d\n",foundParamIndIndex);
        string options = (const char *)wxConvCurrent->cWX2MB(argv[foundParamIndIndex]);
        vector<string> paramPartTokens;
        Tokenize(options,paramPartTokens,"=");
        if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
            string optionsValue = paramPartTokens[1];

            autoScreenShotParams.clear();
            Tokenize(optionsValue,autoScreenShotParams,",");
        }
    }

    if(hasCommandArgument(argc, argv,(const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_MODEL])) == true) {
    	const wxWX2MBbuf param = (const char *)wxConvCurrent->cWX2MB(GAME_ARGS[GAME_ARG_LOAD_MODEL]);
    	//printf("param = [%s]\n",(const char*)param);

    	int foundParamIndIndex = -1;
        hasCommandArgument(argc, argv,string((const char*)param) + string("="),&foundParamIndIndex);
        if(foundParamIndIndex < 0) {
            hasCommandArgument(argc, argv,(const char*)param,&foundParamIndIndex);
        }
        //printf("foundParamIndIndex = %d\n",foundParamIndIndex);
        string customPath = (const char *)wxConvCurrent->cWX2MB(argv[foundParamIndIndex]);
        vector<string> paramPartTokens;
        Tokenize(customPath,paramPartTokens,"=");
        if(paramPartTokens.size() >= 2 && paramPartTokens[1].length() > 0) {
            string customPathValue = paramPartTokens[1];
            modelPath = customPathValue;
        }
        else {
            printf("\nInvalid path specified on commandline [%s] value [%s]\n\n",(const char *)wxConvCurrent->cWX2MB(argv[foundParamIndIndex]),(paramPartTokens.size() >= 2 ? paramPartTokens[1].c_str() : NULL));
            printParameterHelp(wxConvCurrent->cWX2MB(argv[0]),false);
            return false;
        }
    }

	if(argc == 2 && argv[1][0] != '-') {

#if defined(__MINGW32__)
		const wxWX2MBbuf tmp_buf = wxConvCurrent->cWX2MB(wxFNCONV(argv[1]));
		modelPath = tmp_buf;
#else
        modelPath = wxFNCONV(argv[1]);
#endif

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
