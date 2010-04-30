// ==============================================================
//This file is part of Glest Shared Library (www.glest.org)
//Copyright (C) 2005 Matthias Braun <matze@braunis.de>

//You can redistribute this code and/or modify it under
//the terms of the GNU General Public License as published by the Free Software
//Foundation; either version 2 of the License, or (at your option) any later
//version.
#include "platform_util.h"
#include <iostream>
#include <string.h>
#include <unistd.h>

#include "leak_dumper.h"

using namespace Shared::Util;
using namespace std;

namespace Shared{ namespace Platform{

// This was the simplest, most portable solution i could find in 5 mins for linux
int MessageBox(int handle, const char *msg, const char *title, int buttons) {
    char cmd[1024]="";
    //sprintf(cmd, "xmessage -center \"%s\"", msg);
    sprintf(cmd, "gdialog --title \"%s\" --msgbox \"%s\"", title, msg);

    //if(fork()==0){
        //close(1); close(2);
    int ret = system(cmd);
        //exit(0);
    //}
}

void message(string message) {
	std::cerr << "******************************************************\n";
	std::cerr << "    " << message << "\n";
	std::cerr << "******************************************************\n";
}

bool ask(string message) {
	std::cerr << "Confirmation: " << message << "\n";
	int res;
	std::cin >> res;
	return res != 0;
}

void exceptionMessage(const exception &excp) {
	std::cerr << "Exception: " << excp.what() << std::endl;
	//int result = MessageBox(NULL, excp.what(), "Error", 0);
}

}}//end namespace
