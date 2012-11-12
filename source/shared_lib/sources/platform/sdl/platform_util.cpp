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
}

#if defined(HAS_GCC_BACKTRACE)
static int getFileAndLine(char *function, void *address, char *file, size_t flen) {
        int line=-1;
        if(PlatformExceptionHandler::application_binary != "") {
			const int maxbufSize = 8096;
			char buf[maxbufSize+1]="";

			// prepare command to be executed
			// our program need to be passed after the -e parameter
			snprintf(buf, 8096,"addr2line -C -e %s -f -i %p",PlatformExceptionHandler::application_binary.c_str(),address);

			FILE* f = popen (buf, "r");
			if (f == NULL) {
				perror (buf);
				return 0;
			}

			if(function != NULL && function[0] != '\0') {
				line = 0;
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
						sscanf (p,"%d", &line);
					}
					else {
						strcpy (file,"unknown");
						line = 0;
					}
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
				sscanf (p,"%d", &line);
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
