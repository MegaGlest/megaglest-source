#include "platform_menu.h"

#include <cassert>

#include "leak_dumper.h"

namespace Shared{ namespace Platform{

int MenuBase::nextId= 1000;

// =====================================================
//	class MenuBase
// =====================================================

void MenuBase::init(const string &text){
	this->text= text;
	id= nextId++;
}

// =====================================================
//	class Menu
// =====================================================

void Menu::create(Menu *parent){
	handle= CreatePopupMenu();

	for(int i= 0; i<children.size(); ++i){
		children[i]->create(this);
	}	

	if(parent!=NULL){
		BOOL result = AppendMenu(parent->getHandle(), MF_POPUP | MF_STRING, reinterpret_cast<UINT_PTR>(handle), text.c_str()); 
		assert(result);
	}
}

void Menu::destroy(){
	for(int i= 0; i<children.size(); ++i){
		children[i]->destroy();
	}	

	children.clear();

	BOOL result = DestroyMenu(handle);
	assert(result);
}

// =====================================================
//	class MenuItem
// =====================================================

void MenuItem::create(Menu *parent){
	isChecked= false;
	this->parent = parent;
	assert(parent!=NULL);
	BOOL result = AppendMenu(parent->getHandle(), MF_STRING, static_cast<UINT>(id), text.c_str()); 
	assert(result);
}

void MenuItem::setChecked(bool checked){
	isChecked= checked;
	CheckMenuItem(parent->getHandle(), id, checked? MF_CHECKED: MF_UNCHECKED);
}

// =====================================================
//	class MenuSeparator
// =====================================================

void MenuSeparator::create(Menu *parent){
	assert(parent!=NULL);
	BOOL result = AppendMenu(parent->getHandle(), MF_SEPARATOR, static_cast<UINT>(id), NULL); 
	assert(result);
}

}}//end namespace
