// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#include "components.h"

#include <cassert>
#include <algorithm>

#include "metrics.h"
#include "core_data.h"
#include "platform_util.h"
#include "util.h"
#include "conversion.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
//	class GraphicComponent
// =====================================================

float GraphicComponent::anim= 0.f;
float GraphicComponent::fade= 0.f;
const float GraphicComponent::animSpeed= 0.02f;
const float GraphicComponent::fadeSpeed= 0.01f;

std::map<std::string, std::map<std::string, GraphicComponent *> > GraphicComponent::registeredGraphicComponentList;

GraphicComponent::GraphicComponent(std::string containerName, std::string objName) {
	instanceName = "";
	if(objName != "") {
		registerGraphicComponent(containerName,objName);
	}
	enabled= true;
	editable= true;
}

void GraphicComponent::clearRegisteredComponents() {
	GraphicComponent::registeredGraphicComponentList.clear();
}

void GraphicComponent::registerGraphicComponent(std::string containerName, std::string objName) {
	instanceName = objName;
	registeredGraphicComponentList[containerName][objName] = this;
}

GraphicComponent * GraphicComponent::findRegisteredComponent(std::string containerName, std::string objName) {
	GraphicComponent *result = NULL;

	std::map<std::string, std::map<std::string, GraphicComponent *> >::iterator iterFind1 = GraphicComponent::registeredGraphicComponentList.find(containerName);
	if(iterFind1 != GraphicComponent::registeredGraphicComponentList.end()) {
		std::map<std::string, GraphicComponent *>::iterator iterFind2 = iterFind1->second.find(objName);
		if(iterFind2 != iterFind1->second.end()) {
			result = iterFind2->second;
		}
	}
	return result;
}

void GraphicComponent::applyAllCustomProperties(std::string containerName) {

	std::map<std::string, std::map<std::string, GraphicComponent *> >::iterator iterFind1 = GraphicComponent::registeredGraphicComponentList.find(containerName);
	if(iterFind1 != GraphicComponent::registeredGraphicComponentList.end()) {
		for(std::map<std::string, GraphicComponent *>::iterator iterFind2 = iterFind1->second.begin();
				iterFind2 != iterFind1->second.end(); iterFind2++) {
			iterFind2->second->applyCustomProperties(containerName);
		}
	}
}

void GraphicComponent::applyCustomProperties(std::string containerName) {
	if(instanceName != "") {
		std::map<std::string, std::map<std::string, GraphicComponent *> >::iterator iterFind1 = GraphicComponent::registeredGraphicComponentList.find(containerName);
		if(iterFind1 != GraphicComponent::registeredGraphicComponentList.end()) {
			std::map<std::string, GraphicComponent *>::iterator iterFind2 = iterFind1->second.find(instanceName);
			if(iterFind2 != iterFind1->second.end()) {
				Config &config = Config::getInstance();

				//if(dynamic_cast<GraphicButton *>(iterFind2->second) != NULL) {
				GraphicComponent *ctl = dynamic_cast<GraphicComponent *>(iterFind2->second);
				ctl->x = config.getInt(containerName + "_" + iterFind2->first + "_x",intToStr(ctl->x).c_str());
				ctl->y = config.getInt(containerName + "_" + iterFind2->first + "_y",intToStr(ctl->y).c_str());
				ctl->w = config.getInt(containerName + "_" + iterFind2->first + "_w",intToStr(ctl->w).c_str());
				ctl->h = config.getInt(containerName + "_" + iterFind2->first + "_h",intToStr(ctl->h).c_str());
				//}
			}
		}
	}
}

void GraphicComponent::init(int x, int y, int w, int h){
    this->x= x;
    this->y= y;
    this->w= w;
    this->h= h;
    font= CoreData::getInstance().getMenuFontNormal();
	enabled= true;
}

bool GraphicComponent::mouseMove(int x, int y){
    return 
        x > this->x &&
        y > this->y &&
        x < this->x + w &&
        y < this->y + h;
}

bool GraphicComponent::mouseClick(int x, int y){
	if(getEnabled()&&getEditable())
    	return mouseMove(x, y);
    else
    	return false;
}

void GraphicComponent::update(){
	fade+= fadeSpeed;
	anim+= animSpeed;
	if(fade>1.f) fade= 1.f;
	if(anim>1.f) anim= 0.f;
}

void GraphicComponent::resetFade(){
	fade= 0.f;
}

// =====================================================
//	class GraphicLabel
// =====================================================

const int GraphicLabel::defH= 20;
const int GraphicLabel::defW= 70;

void GraphicLabel::init(int x, int y, int w, int h, bool centered){
	GraphicComponent::init(x, y, w, h);
	this->centered= centered;
}

// =====================================================
//	class GraphicButton
// =====================================================

const int GraphicButton::defH= 22;
const int GraphicButton::defW= 90;

void GraphicButton::init(int x, int y, int w, int h){
	GraphicComponent::init(x, y, w, h);
    lighted= false;
}

bool GraphicButton::mouseMove(int x, int y){
    bool b= GraphicComponent::mouseMove(x, y);
    lighted= b;
    return b;
}

// =====================================================
//	class GraphicListBox
// =====================================================

const int GraphicListBox::defH= 22;
const int GraphicListBox::defW= 140;

void GraphicListBox::init(int x, int y, int w, int h){
	GraphicComponent::init(x, y, w, h);

	graphButton1.init(x, y, 22, h);
    graphButton2.init(x+w-22, y, 22, h);
    graphButton1.setText("<");
    graphButton2.setText(">");
    selectedItemIndex=-1;
}       

//queryes
void GraphicListBox::pushBackItem(string item){
    items.push_back(item);
    setSelectedItemIndex(0);
}

void GraphicListBox::setItems(const vector<string> &items){
    this->items= items; 
    setSelectedItemIndex(0);
}

void GraphicListBox::setSelectedItemIndex(int index, bool errorOnMissing){
    if(errorOnMissing == true) assert(index>=0 && index<items.size());
    selectedItemIndex= index;
    setText(getSelectedItem());
}

void GraphicListBox::setEditable(bool editable){
    graphButton1.setEditable(editable);
    graphButton2.setEditable(editable);
    GraphicComponent::setEditable(editable);
}

void GraphicListBox::setSelectedItem(string item, bool errorOnMissing){
	vector<string>::iterator iter;        

    iter= find(items.begin(), items.end(), item);

	if(iter==items.end()) {
		if(errorOnMissing == true) {
			for(int idx = 0; idx < items.size(); idx++) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] idx = %d items[idx] = [%s]\n",__FILE__,__FUNCTION__,__LINE__,idx,items[idx].c_str());
			}
			throw runtime_error("Value not found on list box: "+item);
		}
	}
	else {
		setSelectedItemIndex(iter-items.begin());
	}

}
    
bool GraphicListBox::mouseMove(int x, int y){
    return 
        graphButton1.mouseMove(x, y) || 
        graphButton2.mouseMove(x, y);
}

bool GraphicListBox::mouseClick(int x, int y){
	if(!items.empty()){
		bool b1= graphButton1.mouseClick(x, y);
		bool b2= graphButton2.mouseClick(x, y);

		if(b1){
			selectedItemIndex--;
			if(selectedItemIndex<0){
				selectedItemIndex=items.size()-1;
			}
		}
		else if(b2){
			selectedItemIndex++;
			if(selectedItemIndex>=items.size()){
				selectedItemIndex=0;
			}
		}
		setText(getSelectedItem());

		return b1 || b2;
	}
	return false;
}

// =====================================================
//	class GraphicMessageBox
// =====================================================

const int GraphicMessageBox::defH= 240;
const int GraphicMessageBox::defW= 350;

void GraphicMessageBox::init(const string &button1Str, const string &button2Str){
	init(button1Str);

	button1.init(x+(w-GraphicButton::defW)/4, y+25);
	button1.setText(button1Str);
	button2.init(x+3*(w-GraphicButton::defW)/4, y+25);
	button2.setText(button2Str);
	buttonCount= 2;
}

void GraphicMessageBox::init(const string &button1Str){
	font= CoreData::getInstance().getMenuFontNormal();

	h= defH;
	w= defW;
	
	const Metrics &metrics= Metrics::getInstance();

	x= (metrics.getVirtualW()-w)/2;
	y= (metrics.getVirtualH()-h)/2;

	button1.init(x+(w-GraphicButton::defW)/2, y+25);
	button1.setText(button1Str);
	buttonCount= 1;
}

bool GraphicMessageBox::mouseMove(int x, int y){
	return button1.mouseMove(x, y) || button2.mouseMove(x, y);
}

bool GraphicMessageBox::mouseClick(int x, int y){
    bool b1= button1.mouseClick(x, y);
	bool b2= button2.mouseClick(x, y);
	if(buttonCount==1){
		return b1;
	}
	else{
		return b1 ||b2;
	}
}

bool GraphicMessageBox::mouseClick(int x, int y, int &clickedButton){
    bool b1= button1.mouseClick(x, y);
	bool b2= button2.mouseClick(x, y);

	if(buttonCount==1){
		clickedButton= 1;
		return b1;
	}
	else{
		if(b1){
			clickedButton= 1;
			return true;
		}
		else if(b2){
			clickedButton= 2;
			return true;
		}
	}
	return false;
}

}}//end namespace
