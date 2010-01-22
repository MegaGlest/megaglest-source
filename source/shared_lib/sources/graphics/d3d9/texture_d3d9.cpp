#include "texture_d3d9.h"

#include <stdexcept>
#include <cassert>

#include "graphics_interface.h"
#include "context_d3d9.h"
#include "d3d9_util.h"

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Graphics;

namespace Shared{ namespace Graphics{ namespace D3d9{

// ===============================================
//	class Texture2DD3d9
// ===============================================

D3DFORMAT toFormatD3d(Texture::Format format, int components){
	switch(format){
	case Texture::fAuto:
		switch(components){
		case 1:
			return D3DFMT_L8;
		case 3:
			return D3DFMT_X8R8G8B8;
		case 4:
			return D3DFMT_A8R8G8B8;
		default:
			assert(false);
			return D3DFMT_A8R8G8B8;
		}
		break;
	case Texture::fLuminance:
		return D3DFMT_L8;
	case Texture::fAlpha:
		return D3DFMT_A8;
	case Texture::fRgb:
		return D3DFMT_X8R8G8B8;
	case Texture::fRgba:
		return D3DFMT_A8R8G8B8;
	default:
		assert(false);
		return D3DFMT_A8R8G8B8;
	}
} 

void fillPixels(uint8 *texturePixels, const Pixmap2D *pixmap){

	for(int i=0; i<pixmap->getW(); ++i){
		for(int j=0; j<pixmap->getH(); ++j){
			int k= j*pixmap->getW()+i;
			
			Vec4<uint8> pixel;

			pixmap->getPixel(i, j, pixel.ptr());
			switch(pixmap->getComponents()){
			case 1:
				texturePixels[k]= pixel.x;
				break;
			case 3:
				texturePixels[k*4]= pixel.z;
				texturePixels[k*4+1]= pixel.y;
				texturePixels[k*4+2]= pixel.x;
				break;
			case 4:
				texturePixels[k*4]= pixel.z;
				texturePixels[k*4+1]= pixel.y;
				texturePixels[k*4+2]= pixel.x;
				texturePixels[k*4+3]= pixel.w;
				break;
			default:
				assert(false);
			}
		}
	}
}

void Texture2DD3d9::init(Filter textureFilter, int maxAnisotropy){
	if(!inited){

		//get device
		GraphicsInterface &gi= GraphicsInterface::getInstance();
		ContextD3d9 *context= static_cast<ContextD3d9*>(gi.getCurrentContext());
		IDirect3DDevice9 *d3dDevice= context->getD3dDevice();
		
		bool mipmapCaps= (context->getCaps()->TextureCaps & D3DPTEXTURECAPS_MIPCUBEMAP) != 0;
		bool autogenMipmap= mipmapCaps && mipmap;

		int w= pixmapInit? pixmap.getW(): defaultSize;
		int h= pixmapInit? pixmap.getH(): defaultSize;
		
		//create texture
		D3DCALL(d3dDevice->CreateTexture(
			w,
			h,
			autogenMipmap? 0: 1,
			autogenMipmap? D3DUSAGE_AUTOGENMIPMAP: 0,
			toFormatD3d(format, pixmap.getComponents()),
			D3DPOOL_MANAGED,
			&d3dTexture,
			NULL));

		if(pixmapInit){
			//lock 
			D3DLOCKED_RECT lockedRect;
			D3DCALL(d3dTexture->LockRect(0, &lockedRect, NULL, 0));

			//copy
			fillPixels(reinterpret_cast<uint8*>(lockedRect.pBits), &pixmap);

			//unlock
			D3DCALL(d3dTexture->UnlockRect(0));
		}
		inited= true;
	}
}

void Texture2DD3d9::end(){
	if(inited){
		d3dTexture->Release();
	}
}

// ===============================================
//	class TextureCubeD3d9
// ===============================================

void TextureCubeD3d9::init(Filter textureFilter, int maxAnisotropy){
	//get device
	if(!inited){
		GraphicsInterface &gi= GraphicsInterface::getInstance();
		ContextD3d9 *context= static_cast<ContextD3d9*>(gi.getCurrentContext());
		IDirect3DDevice9 *d3dDevice= context->getD3dDevice();
		
		const Pixmap2D *face0= pixmap.getFace(0);
		int l= pixmapInit? face0->getW(): defaultSize;
		int components= face0->getComponents();
		
		//check dimensions and face components
		if(pixmapInit){
			for(int i=0; i<6; ++i){	
				const Pixmap2D *currentFace= pixmap.getFace(i);
				if(currentFace->getW()!=l || currentFace->getH()!=l){
					throw runtime_error("Can't create Direct3D cube texture: dimensions don't agree");
				}
				if(currentFace->getComponents()!=components){
					throw runtime_error("Can't create Direct3D cube texture: components don't agree");
				}
			}
		}

		bool mipmapCaps= (context->getCaps()->TextureCaps & D3DPTEXTURECAPS_MIPCUBEMAP) != 0;
		bool autogenMipmap= mipmapCaps && mipmap;

		//create texture
		D3DCALL(d3dDevice->CreateCubeTexture(
			l,
			autogenMipmap? 0: 1,
			autogenMipmap? D3DUSAGE_AUTOGENMIPMAP: 0,
			toFormatD3d(format, components),
			D3DPOOL_MANAGED,
			&d3dCubeTexture,
			NULL));

		if(pixmapInit){
			for(int i=0; i<6; ++i){

				//lock
				D3DLOCKED_RECT lockedRect;
				D3DCALL(d3dCubeTexture->LockRect(static_cast<D3DCUBEMAP_FACES>(i), 0, &lockedRect, NULL, 0));

				//copy
				fillPixels(reinterpret_cast<uint8*>(lockedRect.pBits), pixmap.getFace(i));

				//unlock
				D3DCALL(d3dCubeTexture->UnlockRect(static_cast<D3DCUBEMAP_FACES>(i), 0));
			}
		}
		inited= true;
	}
}

void TextureCubeD3d9::end(){
	if(inited){
		d3dCubeTexture->Release();
	}
}

}}}//end namespace