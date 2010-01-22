#include "font_d3d9.h"

#include <stdexcept>

#include <d3d9.h> 

#include "graphics_interface.h"
#include "context_d3d9.h"
#include "leak_dumper.h"

using namespace std;

namespace Shared{ namespace Graphics{ namespace D3d9{

// ===============================================
//	class Font2DD3d9
// ===============================================

void Font2DD3d9::init(){
	GraphicsInterface &gi= GraphicsInterface::getInstance();
	IDirect3DDevice9 *d3dDevice= static_cast<ContextD3d9*>(gi.getCurrentContext())->getD3dDevice();

	HFONT hFont=CreateFont(size, 0, 0, 0, width, 0, FALSE, FALSE, ANSI_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DRAFT_QUALITY, DEFAULT_PITCH, type.c_str());

	HRESULT result= D3DXCreateFont(d3dDevice, hFont, &d3dFont);
	if(result!=D3D_OK){
		throw runtime_error("FontD3d9::init() -> Can't create D3D font");
	}
	DeleteObject(hFont);
}

void Font2DD3d9::end(){
	d3dFont->Release();
}


}}}//end namespace