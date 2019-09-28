#pragma once
#include <Graphics/MaterialRequiredStreams.h>
#include <Graphics/VertexBufferHandle.h>
#include <Graphics/IndexBufferHandle.h>
#include <Graphics/CustomRenderItemHandle.h>
#include <Graphics/HLSLMaterialHandle.h>
#include <Graphics/VertexColorMaterialHandle.h>
#include <bitarray.h>

class UnwrapMod;

class UnwrapRenderItem : public MaxSDK::Graphics::ICustomRenderItem
{
	UnwrapMod* mpUnwrapMod;
	//selected edge data
	MaxSDK::Graphics::VertexBufferHandleArray mUnwrapVB;
	MaxSDK::Graphics::IndexBufferHandle mUnwrapIB;
	MaxSDK::Graphics::VertexBufferHandle mUnwrapAttributeVB;
	static MaxSDK::Graphics::HLSLMaterialHandle mUnwrapShaderDX11;
	static MaxSDK::Graphics::MaterialRequiredStreams msFormat;
public:
	UnwrapRenderItem(UnwrapMod* pData,INode* pNode);

	virtual void Realize(MaxSDK::Graphics::DrawContext& drawContext);
	virtual void Display(MaxSDK::Graphics::DrawContext& drawContext);
	virtual void HitTest(MaxSDK::Graphics::HitTestContext& /*hittestContext*/, MaxSDK::Graphics::DrawContext& drawContext) {} //This unwrap gizmo should never be hit/preview
	virtual size_t GetPrimitiveCount() const;
protected:
	void RealizeDX11(MaxSDK::Graphics::DrawContext& drawContext);
	void InitializeData();
	bool	mbDX11Mode;
	Point3	mSelectedEdgeColor;
	Point3  mSeamEdgePreviewColor;
	Point3	mOpenEdgeColor;
	Point3	mOpenEdgeSelectedColor;
	Point3  mSeamEdgeColor;
	INode*  mpMaxNode;
	float	mEdgeThickness;
	BitArray mOpenGeomEdges;
	BOOL	mbShowSeamEdges;
	BOOL	mbShowOpenEdges;
};
