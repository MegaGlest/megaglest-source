#ifndef _SHARED_PLATFORM_POPUPMENU_H_
#define _SHARED_PLATFORM_POPUPMENU_H_

#include <vector>
#include <string>
#include <windows.h>

using std::vector;
using std::string;

namespace Shared{ namespace Platform{

class Menu;

// =====================================================
//	class MenuBase
// =====================================================

class MenuBase{
private:
	static int nextId;

protected:
	int id;
	string text;
	HMENU handle;

public:
	void init(const string &text="");
	virtual ~MenuBase(){};

	virtual void create(Menu *parent)= 0;
	virtual void destroy(){};

	int getId() const				{return id;}
	const string &getText() const	{return text;}
	HMENU getHandle() const			{return handle;}
};

// =====================================================
//	class Menu
// =====================================================

class Menu: public MenuBase{
private:
	typedef vector<MenuBase*> MenuChildren; 

private:
	MenuChildren children;

public:
	virtual void create(Menu *parent= NULL);
	virtual void destroy();

	int getChildCount() const		{return children.size();}
	MenuBase *getChild(int i) const	{return children[i];}

	void addChild(MenuBase *menu)	{children.push_back(menu);}
};

// =====================================================
//	class MenuItem
// =====================================================

class MenuItem: public MenuBase{
private:
	bool isChecked;
	Menu *parent;

public:
	virtual void create(Menu *parent);

	void setChecked(bool checked);

	Menu *getParent() const		{return parent;}
	bool getChecked() const		{return isChecked;}
};

// =====================================================
//	class MenuSeparator
// =====================================================

class MenuSeparator: public MenuBase{
public:
	virtual void create(Menu *parent);
};

}}//end namespace

#endif
