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

#include "BaseTranslator_to_RTITexture.h"

// Max SDK
#include <NotificationAPI/NotificationAPI_Subscription.h>
#include <RenderingAPI/Translator/GenericTranslatorKeys.h>
#include <RenderingAPI/Translator/BaseTranslators/BaseTranslator_Object.h>

class INode;

namespace Max
{;
namespace RapidRTTranslator
{;

using namespace MaxSDK::RenderingAPI::TranslationHelpers;

// Translates: LightscapeLight Photometric Web ==> rti::Texture
class Translator_LightscaleLightWeb_to_RTITexture :
    public BaseTranslator_Object,       // Translate from Object
    public BaseTranslator_to_RTITexture        // Translate to rti::Texture
{
public:

    typedef GenericTranslatorKey_Struct<Translator_LightscaleLightWeb_to_RTITexture, std::shared_ptr<const INodeInstancingPool>> Key;

	// Constructs this translator with the key passed to the translator factory
	Translator_LightscaleLightWeb_to_RTITexture(const Key& key, TranslatorGraphNode& translator_graph_node);
	~Translator_LightscaleLightWeb_to_RTITexture();

	// -- inherited from Translator
	virtual TranslationResult Translate(const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, KeyframeList& keyframesNeeded) override;
    virtual MSTR GetTimingCategory() const override;

    rti::TextureHandle get_web_texture_handle() const;

protected:

    // -- inherited from BaseTranslator_Object
    virtual bool CareAboutMaterialDisplacement() const override;
};

}}	// namespace 
