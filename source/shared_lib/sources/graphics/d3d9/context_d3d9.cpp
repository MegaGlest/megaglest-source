#include "context_d3d9.h"

#include <cassert>
#include <stdexcept>

#include "d3d9_util.h"

#include "leak_dumper.h"

using namespace std;

namespace Shared{ namespace Graphics{ namespace D3d9{

// ===============================================
//	class ContextD3d9
// ===============================================

ContextD3d9::ContextD3d9(){
	windowed= true;
	hardware= true;
}

void ContextD3d9::init(){

	//create object
	d3dObject= Direct3DCreate9(D3D_SDK_VERSION);
	if(d3dObject==NULL){
		throw runtime_error("Direct3DCreate9==NULL");
	}

	//present parameters
	memset(&d3dPresentParameters, 0, sizeof(d3dPresentParameters));
	d3dPresentParameters.Windowed = TRUE;
	d3dPresentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dPresentParameters.BackBufferFormat = D3DFMT_A8R8G8B8;
	d3dPresentParameters.EnableAutoDepthStencil= TRUE;
	d3dPresentParameters.AutoDepthStencilFormat= D3DFMT_D24X8;
	d3dPresentParameters.PresentationInterval= D3DPRESENT_INTERVAL_IMMEDIATE;

	//create device
	D3DCALL(d3dObject->CreateDevice( 
		D3DADAPTER_DEFAULT, 
		hardware? D3DDEVTYPE_HAL: D3DDEVTYPE_REF, 
		GetActiveWindow(),                
		hardware? D3DCREATE_HARDWARE_VERTEXPROCESSING: D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        &d3dPresentParameters, 
		&d3dDevice));

	//get caps
	D3DCALL(d3dDevice->GetDeviceCaps(&caps));
}

void ContextD3d9::end(){
	D3DCALL(d3dDevice->Release());
	D3DCALL(d3dObject->Release());
}

void ContextD3d9::makeCurrent(){

}

void ContextD3d9::swapBuffers(){
	D3DCALL(d3dDevice->Present(NULL, NULL, NULL, NULL));
}

void ContextD3d9::reset(){
	d3dPresentParameters.BackBufferWidth= 0;
	d3dPresentParameters.BackBufferHeight= 0;
	D3DCALL(d3dDevice->Reset(&d3dPresentParameters));
}


}}}//end namespace