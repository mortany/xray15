//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#pragma once

// Max sdk
#include <imtl.h>
#include <RenderingAPI/Translator/GenericTranslatorKeys.h>
#include <RenderingAPI/Translator/BaseTranslators/BaseTranslator_MtlBase.h>
#include <stdmat.h>
// local
#include "BaseTranslator_to_RTITexture.h"

class Texmap;

namespace Max
{;
namespace RapidRTTranslator
{;

// Translates: Rasterized/Baked Texmap ==> rti::Texture
// Generic texture translator which rasterizes procedurals to a bitmap.
class Translator_TextureOutput_to_RTITexture :
    public BaseTranslator_MtlBase,         // Translate from MtlBase type object
    public BaseTranslator_to_RTITexture    // Translate to rti::Shader
{
public:
    typedef GenericTranslatorKey_SinglePointer<Translator_TextureOutput_to_RTITexture, MtlBase> Key;

    // Constructs this translator with the key passed to the translator factory
	Translator_TextureOutput_to_RTITexture(const Key& key, TranslatorGraphNode& translator_graph_node);
	~Translator_TextureOutput_to_RTITexture();

	// Helper function: Find what I need in the param block
	static StdTexoutGen *Translator_TextureOutput_to_RTITexture::CreateFromOutputParameter(ReferenceTarget& ref_targ, const MSTR param_name, const TimeValue t, Interval& validity);

    // Returns the texture handle for the baked texture
    rti::TextureHandle get_texture_handle() const;
    IPoint2 get_texture_resolution() const;

	// -- inherited from Translator
	virtual TranslationResult Translate(const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, KeyframeList& keyframesNeeded) override;
};

}}	// namespace 
