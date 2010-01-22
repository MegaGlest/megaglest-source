#include "main.h"

#include <stdexcept>

#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/icon.h>

using namespace std;

namespace Configurator{

// ===============================================
// 	class MainWindow
// ===============================================


const int MainWindow::margin= 10;
const int MainWindow::panelMargin= 20;
const int MainWindow::gridMarginHorizontal= 30;

MainWindow::MainWindow(){
	
	SetExtraStyle(wxFRAME_EX_CONTEXTHELP);
	
	configuration.load("configuration.xml");

	Create(NULL, -1, "", wxDefaultPosition,  wxDefaultSize, wxCAPTION | wxSYSTEM_MENU);

	SetTitle(("Configurator - " + configuration.getTitle() + " - Editing " + configuration.getFileName()).c_str());

	if(configuration.getIcon()){
		wxIcon icon;
		icon.LoadFile(configuration.getIconPath().c_str(), wxBITMAP_TYPE_ICO);
		SetIcon(icon);
	}

	notebook= new wxNotebook(this, -1);

	wxSizer *mainSizer= new wxBoxSizer(wxVERTICAL);
	wxSizer *topSizer= new wxBoxSizer(wxHORIZONTAL);
	topSizer->Add(notebook, 0, wxALL, 0);
	mainSizer->Add(topSizer, 0, wxALL, margin);

	for(int i=0; i<configuration.getFieldGroupCount(); ++i){

		//create page
		FieldGroup *fg= configuration.getFieldGroup(i);
		wxPanel *panel= new wxPanel(notebook, -1);
		notebook->AddPage(panel, fg->getName().c_str());

		//sizers
		wxSizer *gridSizer= new wxFlexGridSizer(2, margin, gridMarginHorizontal);
		wxSizer *panelSizer= new wxBoxSizer(wxVERTICAL);
		panelSizer->Add(gridSizer, 0, wxALL, panelMargin);
		panel->SetSizer(panelSizer);
			
		for(int j=0; j<fg->getFieldCount(); ++j){
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
	
	buttonOk= new wxButton(this, biOk, "OK");
	buttonApply= new wxButton(this, biApply, "Apply");
	buttonCancel= new wxButton(this, biCancel, "Cancel");
	buttonDefault= new wxButton(this, biDefault, "Default");
	bottomSizer->Add(buttonOk, 0, wxALL, margin);
	bottomSizer->Add(buttonApply, 0, wxRIGHT | wxDOWN | wxUP, margin);
	bottomSizer->Add(buttonCancel, 0, wxRIGHT | wxDOWN | wxUP, margin);
	bottomSizer->Add(buttonDefault, 0, wxRIGHT | wxDOWN | wxUP, margin);

	infoText= new wxTextCtrl(this, -1, "Info text.", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
	infoText->SetSize(infoText->GetSize().x, 2*infoText->GetSize().y/3);
	infoText->SetBackgroundColour(buttonOk->GetBackgroundColour());
	
	mainSizer->Add(infoText, 1, wxGROW | wxALL | wxALIGN_CENTER, margin);
	mainSizer->Add(bottomSizer, 0, wxALIGN_CENTER);

	SetBackgroundColour(buttonOk->GetBackgroundColour());
	
	SetSizerAndFit(mainSizer);
	
	Refresh();
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
	for(int i=0; i<configuration.getFieldGroupCount(); ++i){
		FieldGroup *fg= configuration.getFieldGroup(i);
		for(int j=0; j<fg->getFieldCount(); ++j){
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
	infoText->SetValue(str.c_str());
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
	wxStaticText(parent, -1, field->getName().c_str())
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
	try{
		mainWindow= new MainWindow();
		mainWindow->Show();
	}
	catch(const exception &e){
		wxMessageDialog(NULL, e.what(), "Exception", wxOK | wxICON_ERROR).ShowModal();
		return 0;
	}
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

}//end namespace

IMPLEMENT_APP(Configurator::App)