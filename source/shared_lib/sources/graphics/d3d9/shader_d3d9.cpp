#include "shader_d3d9.h"

#include <fstream>

#include "graphics_interface.h"
#include "context_d3d9.h"
#include "texture_d3d9.h"
#include "d3d9_util.h"

#include "leak_dumper.h"

using namespace std;

namespace Shared{ namespace Graphics{ namespace D3d9{

// ===============================================
//	class ShaderD3d9
// ===============================================

ShaderProgramD3d9::ShaderProgramD3d9(){
	GraphicsInterface &gi= GraphicsInterface::getInstance();
	d3dDevice= static_cast<ContextD3d9*>(gi.getCurrentContext())->getD3dDevice();
	vertexShader= NULL;
	pixelShader= NULL;
}

void ShaderProgramD3d9::attach(VertexShader *vs, FragmentShader *fs){
	vertexShader= static_cast<VertexShaderD3d9*>(vs);
	pixelShader= static_cast<PixelShaderD3d9*>(fs);
}

bool ShaderProgramD3d9::link(string &messages){
	d3dVsConstantTable= vertexShader->getD3dConstantTable();
	d3dPsConstantTable= pixelShader->getD3dConstantTable();
	return true;
}

void ShaderProgramD3d9::activate(){
	d3dDevice->SetVertexShader(vertexShader->getD3dVertexShader());
	d3dDevice->SetPixelShader(pixelShader->getD3dPixelShader());
}

void ShaderProgramD3d9::setUniform(const string &name, int value){
	D3DXHANDLE vsHandle= d3dVsConstantTable->GetConstantByName(NULL, name.c_str());
	D3DXHANDLE psHandle= d3dPsConstantTable->GetConstantByName(NULL, name.c_str());
	HRESULT vsResult= d3dVsConstantTable->SetInt(d3dDevice, vsHandle, value);
	HRESULT psResult= d3dPsConstantTable->SetInt(d3dDevice, psHandle, value);
	if(vsResult!=D3D_OK && psResult!=D3D_OK){
		throw runtime_error("Error setting shader uniform: "+string(name));
	}
}

void ShaderProgramD3d9::setUniform(const string &name, float value){
	D3DXHANDLE vsHandle= d3dVsConstantTable->GetConstantByName(NULL, name.c_str());
	D3DXHANDLE psHandle= d3dPsConstantTable->GetConstantByName(NULL, name.c_str());
	HRESULT vsResult= d3dVsConstantTable->SetFloat(d3dDevice, vsHandle, value);
	HRESULT psResult= d3dPsConstantTable->SetFloat(d3dDevice, psHandle, value);
	if(vsResult!=D3D_OK && psResult!=D3D_OK){
		throw runtime_error("Error setting shader uniform: "+string(name));
	}
}

void ShaderProgramD3d9::setUniform(const string &name, const Vec2f &value){
	setUniform(name, Vec4f(value.x, value.y, 0.0f, 0.0f));
}

void ShaderProgramD3d9::setUniform(const string &name, const Vec3f &value){
	setUniform(name, Vec4f(value.x, value.y, value.z, 0.0f));
}

void ShaderProgramD3d9::setUniform(const string &name, const Vec4f &value){
	D3DXVECTOR4 v;
	memcpy(&v, &value, sizeof(float)*4);

	D3DXHANDLE vsHandle= d3dVsConstantTable->GetConstantByName(NULL, name.c_str());
	D3DXHANDLE psHandle= d3dPsConstantTable->GetConstantByName(NULL, name.c_str());
	HRESULT vsResult= d3dVsConstantTable->SetVector(d3dDevice, vsHandle, &v);
	HRESULT psResult= d3dPsConstantTable->SetVector(d3dDevice, psHandle, &v);
	if(vsResult!=D3D_OK && psResult!=D3D_OK){
		throw runtime_error("Error setting shader uniform: "+string(name));
	}
}

void ShaderProgramD3d9::setUniform(const string &name, const Matrix3f &value){
	throw runtime_error("Not implemented");
}

void ShaderProgramD3d9::setUniform(const string &name, const Matrix4f &value){
	D3DXMATRIX m;
	memcpy(&m, &value, sizeof(float)*16);

	D3DXHANDLE vsHandle= d3dVsConstantTable->GetConstantByName(NULL, name.c_str());
	D3DXHANDLE psHandle= d3dPsConstantTable->GetConstantByName(NULL, name.c_str());
	HRESULT vsResult= d3dVsConstantTable->SetMatrix(d3dDevice, vsHandle, &m);
	HRESULT psResult= d3dPsConstantTable->SetMatrix(d3dDevice, psHandle, &m);
	if(vsResult!=D3D_OK && psResult!=D3D_OK){
		throw runtime_error("Error setting shader uniform: "+string(name));
	}
}

/*void ShaderD3d9::setUniform(const string &name, const Texture *value){
	D3DXHANDLE handle= d3dConstantTable->GetConstantByName(NULL, name);
	D3DXCONSTANT_DESC d3dDesc;
	UINT i=1;
	IDirect3DTexture9 *d3dTexture= static_cast<const Texture2DD3d9*>(value)->getD3dTexture();
	HRESULT result= d3dConstantTable->GetConstantDesc(handle, &d3dDesc, &i);
	if(result==D3D_OK)
		d3dDevice->SetTexture(d3dDesc.RegisterIndex, d3dTexture);
	else
		throw runtime_error("Error setting shader uniform sampler: "+string(name));
}

bool ShaderD3d9::isUniform(char *name){
	D3DXCONSTANT_DESC d3dDesc;
	UINT i=1;
	D3DXHANDLE handle= d3dConstantTable->GetConstantByName(NULL, name);
	HRESULT result= d3dConstantTable->GetConstantDesc(handle, &d3dDesc, &i);
	return result==D3D_OK;
}*/

// ===============================================
//	class ShaderD3d9
// ===============================================

ShaderD3d9::ShaderD3d9(){
	GraphicsInterface &gi= GraphicsInterface::getInstance();
	d3dDevice= static_cast<ContextD3d9*>(gi.getCurrentContext())->getD3dDevice();
	d3dConstantTable= NULL;
}

void ShaderD3d9::end(){
	if(d3dConstantTable!=NULL){
		d3dConstantTable->Release();
		d3dConstantTable= NULL;
	}
}

void ShaderD3d9::load(const string &path){
	source.load(path);
}

// ===============================================
//	class VertexShaderD3d9
// ===============================================

VertexShaderD3d9::VertexShaderD3d9(){
	target= "vs_2_0";
	d3dVertexShader= NULL;
}

void VertexShaderD3d9::end(){
	ShaderD3d9::end();
	if(d3dVertexShader!=NULL){
		d3dVertexShader->Release();
		d3dVertexShader= NULL;
	}
}

bool VertexShaderD3d9::compile(string &messages){
	//compile shader
	ID3DXBuffer *code= NULL;
	ID3DXBuffer *errors= NULL;
	
	HRESULT result= D3DXCompileShader(
		source.getCode().c_str(), source.getCode().size(), NULL, NULL, 
		"main", target.c_str(), 0, &code, &errors, 
		&d3dConstantTable);

	if(errors!=NULL){
		messages+= reinterpret_cast<char*>(errors->GetBufferPointer());
	}

	if(FAILED(result)){
		return false;
	}

	//create shader
	D3DCALL(d3dDevice->CreateVertexShader((DWORD*) code->GetBufferPointer(), &d3dVertexShader));

	//release
	if(code!=NULL){
		code->Release();
	}
	if(errors!=NULL){
		errors->Release();
	}

	//set defaults
	if(d3dConstantTable!=NULL){
		d3dConstantTable->SetDefaults(d3dDevice);
	}

	return true;
}

// ===============================================
//	class PixelShaderD3d9
// ===============================================

PixelShaderD3d9::PixelShaderD3d9(){
	target= "ps_2_0";
	d3dPixelShader= NULL;
}

void PixelShaderD3d9::end(){
	ShaderD3d9::end();
	if(d3dPixelShader!=NULL){
		d3dPixelShader->Release();
		d3dPixelShader= NULL;
	}
}

bool PixelShaderD3d9::compile(string &messages){
	
	messages+= "Compiling shader: " + source.getPathInfo() + "\n";
	
	//compile shader
	ID3DXBuffer *code= NULL;
	ID3DXBuffer *errors= NULL;
	
	HRESULT result= D3DXCompileShader(
		source.getCode().c_str(), source.getCode().size(), NULL, NULL, 
		"main", target.c_str(), 0, &code, &errors, 
		&d3dConstantTable);

	if(errors!=NULL){
		messages+= reinterpret_cast<char*>(errors->GetBufferPointer());
	}

	if(FAILED(result)){
		return false;
	}

	//create shader
	D3DCALL(d3dDevice->CreatePixelShader((DWORD*) code->GetBufferPointer(), &d3dPixelShader));

	//release
	if(code!=NULL){
		code->Release();
	}
	if(errors!=NULL){
		errors->Release();
	}

	//set defaults
	if(d3dConstantTable!=NULL){
		d3dConstantTable->SetDefaults(d3dDevice);
	}

	return true;
}

}}}//end namespace