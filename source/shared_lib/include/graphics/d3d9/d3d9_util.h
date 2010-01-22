#ifndef _SHARED_D3D9_D3D9UTIL_H_
#define _SHARED_D3D9_D3D9UTIL_H_

#include <d3d9.h>

#include <string>
#include <stdexcept>

#define D3DCALL(X) checkResult(X, #X);

using std::string;
using std::runtime_error;

namespace Shared{ namespace Graphics{ namespace D3d9{

string d3dErrorToStr(HRESULT result);

inline void checkResult(HRESULT result, const string &functionCall){
	if(result!=D3D_OK){
		throw runtime_error("Direct3D Error\nCode: " + d3dErrorToStr(result) + "\nFunction: " + functionCall);
	}
}

}}}//end namespace

#endif