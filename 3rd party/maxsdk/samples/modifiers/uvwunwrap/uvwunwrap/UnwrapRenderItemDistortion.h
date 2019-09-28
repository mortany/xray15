#pragma once
#include <Graphics/MaterialRequiredStreams.h>
#include <Graphics/VertexBufferHandle.h>
#include <Graphics/IndexBufferHandle.h>
#include <Graphics/CustomRenderItemHandle.h>
#include <Graphics/HLSLMaterialHandle.h>
#include <Graphics/VertexColorMaterialHandle.h>

class UnwrapMod;

class UnwrapRenderItemDistortion : public MaxSDK::Graphics::ICustomRenderItem
{
	UnwrapMod* mpUnwrapMod;
	//distortion data
	MaxSDK::Graphics::VertexBufferHandleArray mBufferArrayDistortion;
	MaxSDK::Graphics::IndexBufferHandle		  mIndexBufferDistortion;
	static MaxSDK::Graphics::MaterialRequiredStreams msFormatDistortion;
	MaxSDK::Graphics::VertexColorMaterialHandle	mpVertexColorMaterialDistortion;
public:
	UnwrapRenderItemDistortion(UnwrapMod* pData,INode* pNode);

	virtual void Realize(MaxSDK::Graphics::DrawContext& drawContext);
	virtual void Display(MaxSDK::Graphics::DrawContext& drawContext);
	virtual void HitTest(MaxSDK::Graphics::HitTestContext& /*hittestContext*/, MaxSDK::Graphics::DrawContext& drawContext) {} //This unwrap gizmo should never be hit/preview
	virtual size_t GetPrimitiveCount() const;
private:
	INode*  mpMaxNode;
};
