#ifndef _CONFIGURATOR_CONFIGURATION_H_
#define _CONFIGURATOR_CONFIGURATION_H_

#include <string>
#include <vector>

#include <wx/wx.h>

#include "xml/xml_parser.h"

using std::string;
using std::vector;

using Shared::Xml::XmlNode;

namespace Configurator{

class Field;
class FieldGroup;

// ===============================
// 	class Configuration
// ===============================

class Configuration{
public:
	typedef vector<FieldGroup*> FieldGroups;

private:
	string title;
	string fileName;
	bool icon;
	string iconPath;
	FieldGroups fieldGroups;

public:
	~Configuration();

	void load(const string &path);
	
	void loadStructure(const string &path);
	void loadValues(const string &path);

	void save();

	const string &getTitle() const		{return title;}
	const string &getFileName() const	{return fileName;}
	bool getIcon() const				{return icon;}
	const string &getIconPath() const	{return iconPath;}

	int getFieldGroupCount() const			{return fieldGroups.size();}
	FieldGroup *getFieldGroup(int i) const	{return fieldGroups[i];}
};

// ===============================
// 	class FieldGroup
// ===============================

class FieldGroup{
public:
	typedef vector<Field*> Fields;

private:
	Fields fields;
	string name;

public:
	~FieldGroup();

	int getFieldCount() const		{return fields.size();}
	Field *getField(int i) const	{return fields[i];}
	const string &getName() const	{return name;}

	void load(const XmlNode *groupNode);

private:
	Field *newField(const string &type);
};

// ===============================
// 	class Field
// ===============================

class Field{
protected:
	string name;
	string variableName;
	string description;
	string value;
	string defaultValue;

public:
	virtual ~Field()= 0{}

	const string &getName() const			{return name;}
	const string &getVariableName() const	{return variableName;}
	const string &getDescription() const	{return description;}
	const string &getValue() const			{return value;}
	const string &getDefaultValue() const	{return defaultValue;}

	void setName(const string &name)					{this->name= name;}
	void setVariableName(const string &variableName)	{this->variableName= variableName;}
	void setDescription(const string &description)		{this->description= description;}
	void setValue(const string &value)					{this->value= value;}
	void setDefaultValue(const string &defaultValue)	{this->defaultValue= defaultValue;}

	virtual void loadSpecific(const XmlNode *fieldNode){};
	virtual string getInfo() const;

	virtual void createControl(wxWindow *parent, wxSizer *sizer)= 0;
	virtual void updateValue()= 0;
	virtual void updateControl()= 0;
	virtual bool isValueValid(const string &value)= 0;
};

// ===============================
// 	class BoolField
// ===============================

class BoolField: public Field{
private:
	wxCheckBox *checkBox;

public:
	virtual void createControl(wxWindow *parent, wxSizer *sizer);
	virtual void updateValue();
	virtual void updateControl();
	virtual bool isValueValid(const string &value);
};

// ===============================
// 	class IntField
// ===============================

class IntField: public Field{
private:
	wxTextCtrl *textCtrl;

public:
	virtual void createControl(wxWindow *parent, wxSizer *sizer);
	virtual void updateValue();
	virtual void updateControl();
	virtual bool isValueValid(const string &value);
};

// ===============================
// 	class FloatField
// ===============================

class FloatField: public Field{
private:
	wxTextCtrl *textCtrl;

public:
	virtual void createControl(wxWindow *parent, wxSizer *sizer);
	virtual void updateValue();
	virtual void updateControl();
	virtual bool isValueValid(const string &value);
};

// ===============================
// 	class StringField
// ===============================

class StringField: public Field{
private:
	wxTextCtrl *textCtrl;

public:
	virtual void createControl(wxWindow *parent, wxSizer *sizer);
	virtual void updateValue();
	virtual void updateControl();
	virtual bool isValueValid(const string &value);
};

// ===============================
// 	class EnumField
// ===============================

class EnumField: public Field{
private:
	wxComboBox *comboBox;
	vector<string> enumerants;

public:
	virtual void createControl(wxWindow *parent, wxSizer *sizer);
	virtual void updateValue();
	virtual void updateControl();
	virtual bool isValueValid(const string &value);

	virtual void loadSpecific(const XmlNode *fieldNode);
};

// ===============================
// 	class IntRangeField
// ===============================

class IntRangeField: public Field{
private:
	wxSlider *slider;
	int min;
	int max;

public:
	virtual void createControl(wxWindow *parent, wxSizer *sizer);
	virtual void updateValue();
	virtual void updateControl();
	virtual bool isValueValid(const string &value);

	virtual void loadSpecific(const XmlNode *fieldNode);
	virtual string getInfo() const;
};

// ===============================
// 	class FloatRangeField
// ===============================

class FloatRangeField: public Field{
private:
	wxTextCtrl *textCtrl;
	float min;
	float max;

public:
	virtual void createControl(wxWindow *parent, wxSizer *sizer);
	virtual void updateValue();
	virtual void updateControl();
	virtual bool isValueValid(const string &value);

	virtual void loadSpecific(const XmlNode *fieldNode);
	virtual string getInfo() const;
};


}//end namespace

#endif
