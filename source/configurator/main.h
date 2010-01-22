#ifndef _CONFIGURATOR_MAIN_H_
#define _CONFIGURATOR_MAIN_H_

#include <map>

#include <wx/wx.h>
#include <wx/cshelp.h>
#include <wx/notebook.h>
#include <wx/sizer.h>

#include "configuration.h"

using std::pair;
using std::map;

namespace Configurator{

// ===============================
// 	class MainWindow
// ===============================

class MainWindow: public wxFrame{
private:
	DECLARE_EVENT_TABLE()

private:
	typedef pair<int, wxWindow*> IdPair;
	typedef map<int, wxWindow*> IdMap;

private:
	static const int margin;
	static const int panelMargin;
	static const int gridMarginHorizontal;

	enum ButtonId{
		biOk,
		biApply,
		biCancel,
		biDefault
	};

private:
	IdMap idMap;
	Configuration configuration;
	wxButton *buttonOk;
	wxButton *buttonApply;
	wxButton *buttonCancel;
	wxButton *buttonDefault;
	wxNotebook *notebook;
	wxTextCtrl *infoText;

public:
	MainWindow();

	void onButtonOk(wxCommandEvent &event);
	void onButtonApply(wxCommandEvent &event);
	void onButtonCancel(wxCommandEvent &event);
	void onButtonDefault(wxCommandEvent &event);
	void onClose(wxCloseEvent &event);
	void onMouseDown(wxMouseEvent &event);

	void setInfoText(const string &str);
};

// ===============================
// 	class FieldText
// ===============================

class FieldText: public wxStaticText{
private:
	MainWindow *mainWindow;
	const Field *field;

private:
	DECLARE_EVENT_TABLE()

public:
	FieldText(wxWindow *parentm, MainWindow *mainWindow, const Field *field);

	void onHelp(wxHelpEvent &event);
};

// ===============================
// 	class App
// ===============================

class App: public wxApp{
private:
	MainWindow *mainWindow;

public:
	virtual bool OnInit();
	virtual int MainLoop();
	virtual int OnExit();
};

}//end namespace

DECLARE_APP(Configurator::App)

#endif