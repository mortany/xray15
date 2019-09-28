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

#include <Noncopyable.h>

class UnwrapMod;

class Painter2DLegacy : public MaxSDK::Util::Noncopyable
{
public:	
	virtual ~Painter2DLegacy();

	// Get the singleton instance.
	static Painter2DLegacy& Singleton();

	//collect the data from the unwrap and organize the drawing data such as vertex buffer, index buffer
	void PaintView();
	void SetUnwrapMod(UnwrapMod* pUnwrapMod);
	void DestroyOffScreenBuffer();

	IOffScreenBuf *iBuf;
	IOffScreenBuf *iTileBuf;
private:
	Painter2DLegacy();
	MaxSDK::SingleWeakRefMaker					mUnwrapModRefMaker;

	UnwrapMod* GetUnwrapModPtr();

	void DrawEdge(HDC hdc, /*w4int a,int b,*/ int vecA,int vecB, 
		IPoint2 pa, IPoint2 pb, IPoint2 pvecA, IPoint2 pvecB);

	void PaintBackground(HDC hdc);
	void PaintGrid(HDC hdc);
	void PaintTick(HDC hdc, int x, int y, BOOL largeTick);
	void PaintX(HDC hdc, int x, int y, int size);
	void PaintVertexTicks(HDC hdc);
	void GetPeelModeFaces(UnwrapMod* unwrapMod, MeshTopoData* md, BitArray& peelFaces);
	void PaintEdges(HDC hdc);
	void PaintFaces(HDC hdc);
	void PaintFreeFormGizmo(HDC hdc);
	void PaintPelt(HDC hdc);
	void PaintEdgeDistortion(HDC hdc);
};