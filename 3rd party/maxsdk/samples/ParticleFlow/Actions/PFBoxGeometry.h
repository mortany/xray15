//
// Copyright 2015 Autodesk, Inc.  All rights reserved. 
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc. and
// are protected under applicable copyright and trade secret law.  They may
// not be disclosed to, copied or used by any third party without the prior
// written consent of Autodesk, Inc.
//
//

#pragma once
#include <Graphics/IRenderGeometry.h>
#include <Graphics/IndexBufferHandle.h>
#include <box3.h>

class PFBoxGeometry : public MaxSDK::Graphics::IRenderGeometry
{
public:
	PFBoxGeometry(Box3& box);
	virtual ~PFBoxGeometry();

	/** Display the geometry defined by the index buffer and vertex buffer
		\param[in] drawContext the context for display
		\param[in] start start primitive to render
		\param[in] count primitive count to render
		\param[in] lod current lod value from AD system
	*/
	void Display(MaxSDK::Graphics::DrawContext& drawContext, int start, int count, int lod);

	/** Get the type of primitives in the geometry.
	\return the geometry's primitive type
	*/
	MaxSDK::Graphics::PrimitiveType GetPrimitiveType();
	/** Sets type of primitives in the geometry.
		\param[in] type the geometry's primitive type
	*/
	void SetPrimitiveType(MaxSDK::Graphics::PrimitiveType type);

	/** Number of primitives the mesh represents.
		\return geometry's primitive count
	*/
	size_t GetPrimitiveCount();
	
	/** Number of vertices in the mesh.
		\return number of vertices in the mesh.
	*/
	virtual size_t GetVertexCount();
	
	/** Get the stream requirement of this render geometry.
		\return the stream requirement which this geometry built with.
	*/
	MaxSDK::Graphics::MaterialRequiredStreams& GetSteamRequirement();

	/** Get index buffer of this geometry.
		\return index buffer of this geometry. 
	*/
	MaxSDK::Graphics::IndexBufferHandle& GetIndexBuffer();

	MaxSDK::Graphics::VertexBufferHandleArray& GetVertexBuffers();

	/** Get the start primitive offset
	\return The index of the start primitive
	*/
	int GetStartPrimitive() const;
private:
	MaxSDK::Graphics::IndexBufferHandle mBoxIB;
	MaxSDK::Graphics::VertexBufferHandleArray mBoxVB;
};