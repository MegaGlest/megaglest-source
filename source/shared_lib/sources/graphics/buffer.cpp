#include "buffer.h"

#include "leak_dumper.h"

namespace Shared{ namespace Graphics{

// =====================================================
//	class VertexBuffer
// =====================================================

VertexBuffer::VertexBuffer(){
	positionPointer= NULL;
	normalPointer= NULL;
	for(int i= 0; i<texCoordCount; ++i){
		texCoordPointers[i]= NULL;
		texCoordCoordCounts[i]= -1;
	}
	for(int i= 0; i<attribCount; ++i){
		attribPointers[i]= NULL;
		attribCoordCounts[i]= -1;
	}
}

void VertexBuffer::setPositionPointer(void *pointer){
	positionPointer= pointer;
}

void VertexBuffer::setNormalPointer(void *pointer){
	normalPointer= pointer;
}

void VertexBuffer::setTexCoordPointer(void *pointer, int texCoordIndex, int coordCount){
	texCoordPointers[texCoordIndex]= pointer;
	texCoordCoordCounts[texCoordIndex]= coordCount;
}

void VertexBuffer::setAttribPointer(void *pointer, int attribIndex, int coordCount, const string &name){
	attribPointers[attribIndex]= pointer;
	attribCoordCounts[attribIndex]= coordCount;
	attribNames[attribIndex]= name;
}

// =====================================================
//	class IndexBuffer
// =====================================================

IndexBuffer::IndexBuffer(){
	indexPointer= NULL;
}

void IndexBuffer::setIndexPointer(void *pointer){
	indexPointer= pointer;
}

}}//end namespace
