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
#include "util.h"
#include "conversion.h"

// For gcc backtrace on crash!
#if defined(HAS_GCC_BACKTRACE)

#include <execinfo.h>
#include <cxxabi.h>
#include <signal.h>

#endif

#include <stdexcept>
#include "leak_dumper.h"

using namespace Shared::Util;
using namespace std;

namespace Shared{ namespace Platform{

string PlatformExceptionHandler::application_binary="";
bool PlatformExceptionHandler::disableBacktrace = false;

const char * getDialogCommand() {

/*
  if (::system(NULL)) {
    if (::system("which gdialog") == 0)
      return "gdialog";
    else if (::system("which kdialog") == 0)
      return "kdialog";
  }
*/

  FILE *file = popen("which zenity","r");
  //printf("File #1 [%p]\n",file);
  if (file != NULL) {
	  pclose(file);
	  return "zenity";
  }

  file = popen("which gdialog","r");
  //printf("File #1 [%p]\n",file);
  if (file != NULL) {
	  pclose(file);
	  return "gdialog";
  }
  file = popen("which kdialog","r");
  //printf("File #2 [%p]\n",file);
  if (file != NULL) {
	  pclose(file);
	  return "kdialog";
  }

  return NULL;
}

bool showMessage(std::string warning,string writepath) {
	bool guiMessage = false;
	const char * dialogCommand = getDialogCommand();
	if (dialogCommand) {
		std::string command = dialogCommand;

		string text_file = writepath + "/mg_dialog_text.txt";
		{
			FILE *fp = fopen(text_file.c_str(),"wt");
			if (fp != NULL) {
				fputs(warning.c_str(),fp);
				fclose(fp);
			}
		}

		//command += " --title \"Error\" --msgbox \"`printf \"" + warning + "\"`\"";
		command += " --title \"Error\" --text-info --filename=" + text_file;

		//printf("\n\n\nzenity command [%s]\n\n\n",command.c_str());

		FILE *fp = popen(command.c_str(),"r");
		if (fp != 0) {
			guiMessage = true;
			pclose(fp);
		}
	}

	return guiMessage;
}

void message(string message, bool isNonGraphicalModeEnabled,string writepath) {
	std::cerr << "\n\n\n";
	std::cerr << "******************************************************\n";
	std::cerr << "    " << message << "\n";
	std::cerr << "******************************************************\n";
	std::cerr << "\n\n\n";

	if(isNonGraphicalModeEnabled == false) {
		showMessage(message,writepath);
	}
}

void exceptionMessage(const exception &excp) {
	std::cerr << "Exception: " << excp.what() << std::endl;
}

#if defined(HAS_GCC_BACKTRACE)
static int getFileAndLine(char *function, void *address, char *file, size_t flen) {
        int line=-1;
        if(PlatformExceptionHandler::application_binary != "") {
			const int maxbufSize = 8096;
			char buf[maxbufSize+1]="";

			// prepare command to be executed
			// our program need to be passed after the -e parameter
#if __APPLE_CC__
			snprintf(buf, 8096,"xcrun atos -v -o %s %p",PlatformExceptionHandler::application_binary.c_str(),address);
            fprintf(stderr,"> %s\n",buf);
#else
			snprintf(buf, 8096,"addr2line -C -e %s -f -i %p",PlatformExceptionHandler::application_binary.c_str(),address);
#endif
			FILE* f = popen (buf, "r");
			if (f == NULL) {
				perror (buf);
				return 0;
			}
#if __APPLE_CC__
            //### TODO Will: still working this out
            int len = fread(buf,1,maxbufSize,f);
            buf[len] = 0;
            fprintf(stderr,"< %s",buf);
            return -1;
#endif
            for(;function != NULL && function[0] != '\0';) {
                // get function name
                char *ret = fgets (buf, maxbufSize, f);
                if(ret == NULL) {
                    pclose(f);
                    return line;
                }
                //printf("Looking for [%s] Found [%s]\n",function,ret);
                if(strstr(ret,function) != NULL) {
                    break;
                }
                // get file and line
                ret = fgets (buf, maxbufSize, f);
                if(ret == NULL) {
                    pclose(f);
                    return line;
                }

                if(strlen(buf) > 0 && buf[0] != '?') {
                    //int l;
                    char *p = buf;
                    // file name is until ':'
                    while(*p != 0 && *p != ':') {
                        p++;
                    }

                    *p++ = 0;
                    // after file name follows line number
                    strcpy (file , buf);
                    sscanf (p,"%10d", &line);
                }
                else {
                    strcpy (file,"unknown");
                    line = 0;
                }
            }

			// get file and line
			char *ret = fgets (buf, maxbufSize, f);
			if(ret == NULL) {
				pclose(f);
				return line;
			}

			if(strlen(buf) > 0 && buf[0] != '?') {
				//int l;
				char *p = buf;

				// file name is until ':'
				while(*p != 0 && *p != ':') {
					p++;
				}

				*p++ = 0;
				// after file name follows line number
				strcpy (file , buf);
				sscanf (p,"%10d", &line);
			}
			else {
				strcpy (file,"unknown");
				line = 0;
			}
			pclose(f);
        }

        return line;
}
#endif

string PlatformExceptionHandler::getStackTrace() {
	string errMsg = "\nStack Trace:\n";
	 if(PlatformExceptionHandler::disableBacktrace == true) {
		 errMsg += "disabled...";
		 return errMsg;
	 }

#if defined(HAS_GCC_BACKTRACE)
//        if(disableBacktrace == false && sdl_quitCalled == false) {
        //errMsg = "\nStack Trace:\n";
        //errMsg += "To find line #'s use:\n";
        //errMsg += "readelf --debug-dump=decodedline %s | egrep 0xaddress-of-stack\n";

        const size_t max_depth = 25;
        void *stack_addrs[max_depth];
        size_t stack_depth = backtrace(stack_addrs, max_depth);
        char **stack_strings = backtrace_symbols(stack_addrs, stack_depth);
        //for (size_t i = 1; i < stack_depth; i++) {
        //    errMsg += string(stack_strings[i]) + "\n";
        //}

        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

        char szBuf[8096]="";
        string strBuf = "";
        for(size_t i = 1; i < stack_depth; i++) {
            void *lineAddress = stack_addrs[i]; //getStackAddress(stackIndex);

            size_t sz = 8096; // just a guess, template names will go much wider
            char *function = static_cast<char *>(malloc(sz));
            function[0] = '\0';
            char *begin = 0;
            char *end = 0;

            // find the parentheses and address offset surrounding the mangled name
            for (char *j = stack_strings[i]; *j; ++j) {
                if (*j == '(') {
                    begin = j;
                }
                else if (*j == '+') {
                    end = j;
                }
            }
            if (begin && end) {
                *begin++ = '\0';
                *end = '\0';
                // found our mangled name, now in [begin, end)

                int status;
                char *ret = abi::__cxa_demangle(begin, function, &sz, &status);
                if (ret) {
                    // return value may be a realloc() of the input
                    function = ret;
                }
                else {
                    // demangling failed, just pretend it's a C function with no args
                    strncpy(function, begin, sz);
                    strncat(function, "()", sz);
                    function[sz-1] = '\0';
                }
                //fprintf(out, "    %s:%s\n", stack.strings[i], function);

                strBuf = string(stack_strings[i]) + ":" + string(function);
                snprintf(szBuf,8096,"address [%p]",lineAddress);
                strBuf += szBuf;
            }
            else {
                // didn't find the mangled name, just print the whole line
                //fprintf(out, "    %s\n", stack.strings[i]);
            	strBuf = stack_strings[i];
                snprintf(szBuf,8096,"address [%p]",lineAddress);
                strBuf += szBuf;
            }

            //errMsg += string(szBuf);
            errMsg += strBuf;
            char file[8096]="";
            int line = getFileAndLine(function, lineAddress, file, 8096);
            if(line >= 0) {
                errMsg += " line: " + intToStr(line);
            }
            errMsg += "\n";

            free(function);
        }

        free(stack_strings); // malloc()ed by backtrace_symbols
#endif

	return errMsg;
}

megaglest_runtime_error::megaglest_runtime_error(const string& __arg,bool noStackTrace)
	: std::runtime_error(noStackTrace == false ? __arg + PlatformExceptionHandler::getStackTrace() : __arg), noStackTrace(noStackTrace) {
}

}}//end namespace
