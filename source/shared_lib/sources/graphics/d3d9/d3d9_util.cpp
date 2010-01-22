#include "d3d9_util.h"

namespace Shared{ namespace Graphics{ namespace D3d9{

string d3dErrorToStr(HRESULT result){
	switch(result){
	case D3D_OK: return "D3D_OK";
	case D3DERR_DEVICELOST: return "D3DERR_DEVICELOST";
	case D3DERR_DEVICENOTRESET: return "D3DERR_DEVICENOTRESET";
	case D3DERR_DRIVERINTERNALERROR: return "D3DERR_DRIVERINTERNALERROR";
	case D3DERR_INVALIDCALL: return "D3DERR_INVALIDCALL";
	case D3DERR_MOREDATA: return "D3DERR_MOREDATA";
	case D3DERR_NOTFOUND: return "D3DERR_NOTFOUND";
	case D3DERR_OUTOFVIDEOMEMORY: return "D3DERR_OUTOFVIDEOMEMORY";
	case E_OUTOFMEMORY: return "E_OUTOFMEMORY";
	default:
		return "Unkown D3D error";
	}
}

}}}//end namespace
