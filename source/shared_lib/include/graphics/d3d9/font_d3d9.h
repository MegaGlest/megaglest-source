#ifndef _SHARED_D3D9_FONTD3D9_H_
#define _SHARED_D3D9_FONTD3D9_H_

#include "font.h"

#include <d3dx9.h>

namespace Shared{ namespace Graphics{ namespace D3d9{

// ===============================
//	class FontD3d9  
// ===============================

class Font2DD3d9: public Font2D{
private:
	LPD3DXFONT d3dFont;

public:
	LPD3DXFONT getD3dFont() const	{return d3dFont;}
	virtual void init();
	virtual void end();
};

}}}//end namespace

#endif
