#include "configuration.h"

#include <stdexcept>

#include "xml/xml_parser.h"
#include "util/util.h"
#include "util/properties.h"
#include "util/conversion.h"

using namespace std;
using namespace Shared::Xml;
using namespace Shared::Util;

namespace Configurator{

// ===============================================
// 	class Configuration
// ===============================================

Configuration::~Configuration(){
	for(int i= 0; i<fieldGroups.size(); ++i){
		delete fieldGroups[i];
	}
}

void Configuration::load(const string &path){
	loadStructure(path);
	loadValues(fileName);
}

void Configuration::loadStructure(const string &path){
	
	XmlTree xmlTree;
	xmlTree.load(path);

	const XmlNode *configurationNode= xmlTree.getRootNode();

	//title
	title= configurationNode->getChild("title")->getAttribute("value")->getValue();
	
	//fileName
	fileName= configurationNode->getChild("file-name")->getAttribute("value")->getValue();
	
	//icon
	XmlNode *iconNode= configurationNode->getChild("icon");
	icon= iconNode->getAttribute("value")->getBoolValue();
	if(icon){
		iconPath= iconNode->getAttribute("path")->getValue();
	}

	const XmlNode *fieldGroupsNode= configurationNode->getChild("field-groups");

	fieldGroups.resize(fieldGroupsNode->getChildCount());

	for(int i=0; i<fieldGroups.size(); ++i){
		const XmlNode *fieldGroupNode= fieldGroupsNode->getChild("field-group", i);
		FieldGroup *fieldGroup= new FieldGroup();
		fieldGroup->load(fieldGroupNode);
		fieldGroups[i]= fieldGroup;
	}
}

void Configuration::loadValues(const string &path){
	Properties properties;

	properties.load(path);

	for(int i=0; i<fieldGroups.size(); ++i){
		FieldGroup *fg= fieldGroups[i];
		for(int j=0; j<fg->getFieldCount(); ++j){
			Field *f= fg->getField(j);
			f->setValue(properties.getString(f->getVariableName()));
		}
	}
}

void Configuration::save(){
	Properties properties;

	properties.load(fileName);

	for(int i=0; i<fieldGroups.size(); ++i){
		FieldGroup *fg= fieldGroups[i];
		for(int j=0; j<fg->getFieldCount(); ++j){
			Field *f= fg->getField(j);
			f->updateValue();
			if(!f->isValueValid(f->getValue())){
				f->setValue(f->getDefaultValue());
				f->updateControl();
			}
			properties.setString(f->getVariableName(), f->getValue());
		}
	}

	properties.save(fileName);
}

string Field::getInfo() const{
	return name+" (default: " + defaultValue + ")";
}

// ===============================================
// 	class FieldGroup
// ===============================================

FieldGroup::~FieldGroup(){
	for(int i= 0; i<fields.size(); ++i){
		delete fields[i];
	}
}

void FieldGroup::load(const XmlNode *groupNode){

	name= groupNode->getAttribute("name")->getValue();

	fields.resize(groupNode->getChildCount());
	for(int i=0; i<fields.size(); ++i){
		const XmlNode *fieldNode= groupNode->getChild("field", i); 

		Field *f= newField(fieldNode->getAttribute("type")->getValue());
			
		//name
		const XmlNode *nameNode= fieldNode->getChild("name");
		f->setName(nameNode->getAttribute("value")->getValue());

		//variableName
		const XmlNode *variableNameNode= fieldNode->getChild("variable-name");
		f->setVariableName(variableNameNode->getAttribute("value")->getValue());

		//description
		const XmlNode *descriptionNode= fieldNode->getChild("description");
		f->setDescription(descriptionNode->getAttribute("value")->getValue());

		//default
		const XmlNode *defaultNode= fieldNode->getChild("default");
		f->setDefaultValue(defaultNode->getAttribute("value")->getValue());

		f->loadSpecific(fieldNode);

		if(!f->isValueValid(f->getDefaultValue())){
			throw runtime_error("Default value not valid in field: " + f->getName());
		}

		fields[i]= f;
	}
}

Field *FieldGroup::newField(const string &type){
	if(type=="Bool"){
		return new BoolField();
	}
	else if(type=="Int"){
		return new IntField();
	}
	else if(type=="Float"){
		return new FloatField();
	}
	else if(type=="String"){
		return new StringField();
	}
	else if(type=="Enum"){
		return new EnumField();
	}
	else if(type=="IntRange"){
		return new IntRangeField();
	}
	else if(type=="FloatRange"){
		return new FloatRangeField();
	}
	else{
		throw runtime_error("Unknown field type: " + type);
	}
}

// ===============================================
// 	class BoolField
// ===============================================

void BoolField::createControl(wxWindow *parent, wxSizer *sizer){
	checkBox= new wxCheckBox(parent, -1, "");
	checkBox->SetValue(strToBool(value));
	sizer->Add(checkBox);
}

void BoolField::updateValue(){
	value= boolToStr(checkBox->GetValue());
}

void BoolField::updateControl(){
	checkBox->SetValue(strToBool(value));
}

bool BoolField::isValueValid(const string &value){
	try{
		strToBool(value);
	}
	catch(const exception &){
		return false;
	}
	return true;
}

// ===============================================
// 	class IntField
// ===============================================

void IntField::createControl(wxWindow *parent, wxSizer *sizer){
	textCtrl= new wxTextCtrl(parent, -1, value.c_str());
	sizer->Add(textCtrl);
}

void IntField::updateValue(){
	value= textCtrl->GetValue();
}

void IntField::updateControl(){
	textCtrl->SetValue(value.c_str());
}

bool IntField::isValueValid(const string &value){
	try{
		strToInt(value);
	}
	catch(const exception &){
		return false;
	}
	return true;
}

// ===============================================
// 	class FloatField
// ===============================================

void FloatField::createControl(wxWindow *parent, wxSizer *sizer){
	textCtrl= new wxTextCtrl(parent, -1, value.c_str());
	sizer->Add(textCtrl);
}

void FloatField::updateValue(){
	value= textCtrl->GetValue();
}

void FloatField::updateControl(){
	textCtrl->SetValue(value.c_str());
}

bool FloatField::isValueValid(const string &value){
	try{
		strToFloat(value);
	}
	catch(const exception &){
		return false;
	}
	return true;
}

// ===============================================
// 	class StringField
// ===============================================

void StringField::createControl(wxWindow *parent, wxSizer *sizer){
	textCtrl= new wxTextCtrl(parent, -1, value.c_str());
	textCtrl->SetSize(wxSize(3*textCtrl->GetSize().x/2, textCtrl->GetSize().y));
	sizer->Add(textCtrl);
}

void StringField::updateValue(){
	value= textCtrl->GetValue();
}

void StringField::updateControl(){
	textCtrl->SetValue(value.c_str());
}

bool StringField::isValueValid(const string &value){
	return true;
}

// ===============================================
// 	class EnumField
// ===============================================

void EnumField::createControl(wxWindow *parent, wxSizer *sizer){
	comboBox= new wxComboBox(parent, -1, "", wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);
	for(int i=0; i<enumerants.size(); ++i){
		comboBox->Append(enumerants[i].c_str());
	}
	comboBox->SetValue(value.c_str());
	sizer->Add(comboBox);
}

void EnumField::updateValue(){
	value= comboBox->GetValue();
}

void EnumField::updateControl(){
	comboBox->SetValue(value.c_str());
}

bool EnumField::isValueValid(const string &value){
	return true;
}

void EnumField::loadSpecific(const XmlNode *fieldNode){
	const XmlNode *enumsNode= fieldNode->getChild("enums");
	for(int i=0; i<enumsNode->getChildCount(); ++i){
		const XmlNode *enumNode= enumsNode->getChild("enum", i);
		enumerants.push_back(enumNode->getAttribute("value")->getValue());
	}
};

// ===============================================
// 	class IntRange
// ===============================================

void IntRangeField::createControl(wxWindow *parent, wxSizer *sizer){
	slider= new wxSlider(parent, -1, strToInt(value), min, max, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
	sizer->Add(slider);
}

void IntRangeField::updateValue(){
	value= intToStr(slider->GetValue());
}

void IntRangeField::updateControl(){
	slider->SetValue(strToInt(value));
}

bool IntRangeField::isValueValid(const string &value){
	try{
		strToInt(value);
	}
	catch(const exception &){
		return false;
	}
	return true;
}

void IntRangeField::loadSpecific(const XmlNode *fieldNode){
	const XmlNode *minNode= fieldNode->getChild("min");
	min= strToInt(minNode->getAttribute("value")->getValue());

	const XmlNode *maxNode= fieldNode->getChild("max");
	max= strToInt(maxNode->getAttribute("value")->getValue());
}

string IntRangeField::getInfo() const{
	return name + " (min: " + intToStr(min)+ ", max: " + intToStr(max) + ", default: " + defaultValue + ")";
}

// ===============================================
// 	class FloatRangeField
// ===============================================

void FloatRangeField::createControl(wxWindow *parent, wxSizer *sizer){
	textCtrl= new wxTextCtrl(parent, -1, value.c_str());
	sizer->Add(textCtrl);
}

void FloatRangeField::updateValue(){
	value= textCtrl->GetValue();
}

void FloatRangeField::updateControl(){
	textCtrl->SetValue(value.c_str());
}

bool FloatRangeField::isValueValid(const string &value){
	try{
		float f= strToFloat(value);
		return f>=min && f<=max;
	}
	catch(const exception &){
		return false;
	}
	return true;
}

void FloatRangeField::loadSpecific(const XmlNode *fieldNode){
	const XmlNode *minNode= fieldNode->getChild("min");
	min= strToFloat(minNode->getAttribute("value")->getValue());

	const XmlNode *maxNode= fieldNode->getChild("max");
	max= strToFloat(maxNode->getAttribute("value")->getValue());
};

string FloatRangeField::getInfo() const{
	return name + " (min: " + floatToStr(min)+ ", max: " + floatToStr(max) + ", default: " + defaultValue + ")";
}

}//end namespace