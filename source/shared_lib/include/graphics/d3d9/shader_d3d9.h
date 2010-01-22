#ifndef _SHARED_D3D9_SHADERD3D9_H_
#define _SHARED_D3D9_SHADERD3D9_H_

#include "shader.h"
#include "vec.h"

#include <d3d9.h>
#include <d3dx9shader.h>

namespace Shared{ namespace Graphics{ namespace D3d9{

// ===============================
//	class ShaderProgramD3d9  
// ===============================

class VertexShaderD3d9;
class PixelShaderD3d9;

class ShaderProgramD3d9: public virtual ShaderProgram{
protected:
	IDirect3DDevice9 *d3dDevice;
	ID3DXConstantTable *d3dVsConstantTable;
	ID3DXConstantTable *d3dPsConstantTable;

	VertexShaderD3d9 *vertexShader;
	PixelShaderD3d9 *pixelShader;

public:
	ShaderProgramD3d9();
	
	virtual void init(){};
	virtual void end(){};

	virtual void attach(VertexShader *vs, FragmentShader *fs);
	virtual void activate();

	virtual bool link(string &messages);
	
	virtual void setUniform(const string &name, int value);
	virtual void setUniform(const string &name, float value);
	virtual void setUniform(const string &name, const Vec2f &value);
	virtual void setUniform(const string &name, const Vec3f &value);
	virtual void setUniform(const string &name, const Vec4f &value);
	virtual void setUniform(const string &name, const Matrix3f &value);
	virtual void setUniform(const string &name, const Matrix4f &value);
	//virtual void setUniform(const string &name, const Texture *value);
};


// ===============================
//	class ShaderD3d9  
// ===============================

class ShaderD3d9: virtual public Shader{
protected:
	ShaderSource source;
	string target;

	IDirect3DDevice9 *d3dDevice;
	ID3DXConstantTable *d3dConstantTable;
	
public:
	ShaderD3d9();
	virtual void init(){};
	virtual void end();
	virtual void load(const string &path);

	ID3DXConstantTable *getD3dConstantTable() const	{return d3dConstantTable;}

	void setTarget(const string &target)	{this->target= target;}
};

// ===============================
//	class VertexShaderD3d9  
// ===============================

class VertexShaderD3d9: public ShaderD3d9, public VertexShader{
private:
	IDirect3DVertexShader9 *d3dVertexShader;
	
public:
	VertexShaderD3d9();
	virtual void end();

	virtual bool compile(string &messages);

	IDirect3DVertexShader9 *getD3dVertexShader() const	{return d3dVertexShader;}
};

// ===============================
//	class PixelShaderD3d9  
// ===============================

class PixelShaderD3d9: public ShaderD3d9, public FragmentShader{
private:
	IDirect3DPixelShader9 *d3dPixelShader;

public:
	PixelShaderD3d9();
	virtual void end();

	virtual bool compile(string &messages);

	IDirect3DPixelShader9 *getD3dPixelShader() const	{return d3dPixelShader;}
};

}}}//end namespace

#endif