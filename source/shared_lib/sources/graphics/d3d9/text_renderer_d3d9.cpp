#include "text_renderer_d3d9.h"

#include <d3d9.h>
#include <d3d9types.h>

#include "font_d3d9.h"
#include "d3d9_util.h"
#include "leak_dumper.h"

namespace Shared{ namespace Graphics{ namespace D3d9{

// ===============================================
//	class TextRenderer2DD3d9
// ===============================================

void TextRenderer2DD3d9::begin(const Font2D *font){
	this->font= font;
	this->color= Vec4f(1.0f);
}

void TextRenderer2DD3d9::render(const string &text, int x, int y, bool centered){
   	RECT rect;
	rect.bottom= y;
	rect.left= x;
	rect.top= y;
	rect.right= x;
	
	D3DCOLOR d3dColor= D3DCOLOR_ARGB(
		static_cast<int>(color.w*255),
		static_cast<int>(color.x*255),
		static_cast<int>(color.y*255),
		static_cast<int>(color.z*255));

	static_cast<const Font2DD3d9*>(font)->getD3dFont()->DrawText(text.c_str(), -1, &rect, DT_NOCLIP, d3dColor);
}

void TextRenderer2DD3d9::end(){
}

}}}//end namespace