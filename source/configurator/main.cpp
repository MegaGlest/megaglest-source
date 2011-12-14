// ==============================================================
// This file is part of MegaGlest (www.glest.org)
//
// Copyright (C) 2011 - by Mark Vejvoda <mark_vejvoda@hotmail.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "main.h"

#include <stdexcept>

#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/icon.h>
#include "platform_common.h"
#include "config.h"
#include "game_constants.h"
#include "util.h"
#include <wx/stdpaths.h>
#include <platform_util.h>

using namespace std;
using namespace Shared::PlatformCommon;
using namespace Glest::Game;
using namespace Shared::Util;

namespace Glest { namespace Game {
string getGameReadWritePath(string lookupKey) {
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

namespace Configurator {

// ===============================================
// 	class MainWindow
// ===============================================

const int MainWindow::margin= 10;
const int MainWindow::panelMargin= 20;
const int MainWindow::gridMarginHorizontal= 30;

const string MainWindow::versionString= "v1.3.5";
const string MainWindow::winHeader= "Mega-Glest config " + versionString + " - Built: " + __DATE__;

MainWindow::MainWindow(string appPath) :
    wxFrame(
		NULL, -1, Configuration::ToUnicode(winHeader),
		wxDefaultPosition, wxSize(800, 600)){

	SetExtraStyle(wxFRAME_EX_CONTEXTHELP);

	this->appPath = appPath;
	Properties::setApplicationPath(executable_path(appPath));
	Config &config = Config::getInstance();
	string iniFilePath = extractDirectoryPathFromFile(config.getFileName(false));
	configuration.load(iniFilePath + "configuration.xml");

	//Create(NULL, -1, "", wxDefaultPosition,  wxDefaultSize, wxCAPTION | wxSYSTEM_MENU);

	SetTitle(Configuration::ToUnicode(("Configurator - " + configuration.getTitle() + " - Editing " + configuration.getFileName()).c_str()));

	if(configuration.getIcon()) {
		string iconPath = configuration.getIconPath();
	    printf("In [%s::%s] icon = [%s]\n",__FILE__,__FUNCTION__,iconPath.c_str());

        wxInitAllImageHandlers();
		wxIcon icon;
		icon.LoadFile(Configuration::ToUnicode(iconPath.c_str()), wxBITMAP_TYPE_ICO);
		SetIcon(icon);
	}

	notebook= new wxNotebook(this, -1);

	wxSizer *mainSizer= new wxBoxSizer(wxVERTICAL);
	wxSizer *topSizer= new wxBoxSizer(wxHORIZONTAL);
	topSizer->Add(notebook, 0, wxALL, 0);
	mainSizer->Add(topSizer, 0, wxALL, margin);

	for(unsigned int i=0; i<configuration.getFieldGroupCount(); ++i){

		//create page
		FieldGroup *fg= configuration.getFieldGroup(i);
		wxPanel *panel= new wxPanel(notebook, -1);
		notebook->AddPage(panel, Configuration::ToUnicode(fg->getName().c_str()));

		//sizers
		wxSizer *gridSizer= new wxFlexGridSizer(2, margin, gridMarginHorizontal);
		wxSizer *panelSizer= new wxBoxSizer(wxVERTICAL);
		panelSizer->Add(gridSizer, 0, wxALL, panelMargin);
		panel->SetSizer(panelSizer);

		for(unsigned int j=0; j<fg->getFieldCount(); ++j){
			Field *f= fg->getField(j);
			FieldText *staticText= new FieldText(panel, this, f);
			staticText->SetAutoLayout(true);
			gridSizer->Add(staticText);
			f->createControl(panel, gridSizer);
			idMap.insert(IdPair(staticText->GetId(), staticText));
		}
	}

	//buttons
	wxSizer *bottomSizer= new wxBoxSizer(wxHORIZONTAL);

	buttonOk= new wxButton(this, biOk, wxT("OK"));
	buttonApply= new wxButton(this, biApply, wxT("Apply"));
	buttonCancel= new wxButton(this, biCancel, wxT("Cancel"));
	buttonDefault= new wxButton(this, biDefault, wxT("Default"));
	bottomSizer->Add(buttonOk, 0, wxALL, margin);
	bottomSizer->Add(buttonApply, 0, wxRIGHT | wxDOWN | wxUP, margin);
	bottomSizer->Add(buttonCancel, 0, wxRIGHT | wxDOWN | wxUP, margin);
	bottomSizer->Add(buttonDefault, 0, wxRIGHT | wxDOWN | wxUP, margin);

	infoText= new wxTextCtrl(this, -1, Configuration::ToUnicode("Info text."), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
	infoText->SetSize(infoText->GetSize().x, 2*infoText->GetSize().y/3);
	infoText->SetBackgroundColour(buttonOk->GetBackgroundColour());

	mainSizer->Add(infoText, 1, wxGROW | wxALL | wxALIGN_CENTER, margin);
	mainSizer->Add(bottomSizer, 0, wxALIGN_CENTER);

	SetBackgroundColour(buttonOk->GetBackgroundColour());

	SetSizerAndFit(mainSizer);

	Refresh();
}

void MainWindow::init(){
}

void MainWindow::onButtonOk(wxCommandEvent &event){
	configuration.save();
	Close();
}

void MainWindow::onButtonApply(wxCommandEvent &event){
	configuration.save();
}

void MainWindow::onButtonCancel(wxCommandEvent &event){
	Close();
}

void MainWindow::onButtonDefault(wxCommandEvent &event){
	for(unsigned int i=0; i<configuration.getFieldGroupCount(); ++i){
		FieldGroup *fg= configuration.getFieldGroup(i);
		for(unsigned int j=0; j<fg->getFieldCount(); ++j){
			Field *f= fg->getField(j);
			f->setValue(f->getDefaultValue());
			f->updateControl();
		}
	}
}

void MainWindow::onClose(wxCloseEvent &event){
	Destroy();
}

void MainWindow::onMouseDown(wxMouseEvent &event){
	setInfoText("");
}

void MainWindow::setInfoText(const string &str){
	infoText->SetValue(Configuration::ToUnicode(str.c_str()));
}

BEGIN_EVENT_TABLE(MainWindow, wxFrame)
	EVT_BUTTON(biOk, MainWindow::onButtonOk)
	EVT_BUTTON(biApply, MainWindow::onButtonApply)
	EVT_BUTTON(biCancel, MainWindow::onButtonCancel)
	EVT_BUTTON(biDefault, MainWindow::onButtonDefault)
	EVT_CLOSE(MainWindow::onClose)
	EVT_LEFT_DOWN(MainWindow::onMouseDown)
END_EVENT_TABLE()

// ===============================================
// 	class FieldText
// ===============================================

FieldText::FieldText(wxWindow *parent, MainWindow *mainWindow, const Field *field):
	wxStaticText(parent, -1, Configuration::ToUnicode(field->getName().c_str()))
{
	this->mainWindow= mainWindow;
	this->field= field;
}

void FieldText::onHelp(wxHelpEvent &event){
	string str= field->getInfo()+".";
	if(!field->getDescription().empty()){
		str+= "\n"+field->getDescription()+".";
	}
	mainWindow->setInfoText(str);
}


BEGIN_EVENT_TABLE(FieldText, wxStaticText)
	EVT_HELP(-1, FieldText::onHelp)
END_EVENT_TABLE()

// ===============================================
// 	class App
// ===============================================

bool App::OnInit(){
	try {
		SystemFlags::VERBOSE_MODE_ENABLED  = true;
		string appPath = "";
		wxString exe_path = wxStandardPaths::Get().GetExecutablePath();

#ifdef WIN32
		const wxWX2MBbuf tmp_buf = wxConvCurrent->cWX2MB(wxFNCONV(exe_path));
		appPath = tmp_buf;

		std::auto_ptr<wchar_t> wstr(Ansi2WideString(appPath.c_str()));
		appPath = utf8_encode(wstr.get());
#else
		appPath = wxFNCONV(exe_path);
#endif

	//#if defined(__MINGW32__)
	//		const wxWX2MBbuf tmp_buf = wxConvCurrent->cWX2MB(wxFNCONV(exe_path));
	//		appPath = tmp_buf;
	//#else
	//		appPath = wxFNCONV(exe_path);
	//#endif

		mainWindow= new MainWindow(appPath);
		mainWindow->Show();
	}
	catch(const exception &e){
		wxMessageDialog(NULL, Configuration::ToUnicode(e.what()), Configuration::ToUnicode("Exception"), wxOK | wxICON_ERROR).ShowModal();
		return 0;
	}
	return true;
}

int App::MainLoop(){
	try{
		return wxApp::MainLoop();
	}
	catch(const exception &e){
		wxMessageDialog(NULL, Configuration::ToUnicode(e.what()), Configuration::ToUnicode("Exception"), wxOK | wxICON_ERROR).ShowModal();
		return 0;
	}
}

int App::OnExit(){
	SystemFlags::Close();
	SystemFlags::SHUTDOWN_PROGRAM_MODE=true;

	return 0;
}

}//end namespace

IMPLEMENT_APP(Configurator::App)
