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

#include "BaseTranslator_to_RTIShader.h"

#include <RenderingAPI/Renderer/IRenderSettingsContainer.h>
#include <RenderingAPI/Translator/GenericTranslatorKeys.h>
#include <RenderingAPI/Translator/BaseTranslators/BaseTranslator_Object.h>

namespace Max
{;
namespace RapidRTTranslator
{;

using namespace MaxSDK::RenderingAPI;
using namespace MaxSDK::RenderingAPI::TranslationHelpers;

// Translates: LightscaleLight Object ==> rti::Shader
class Translator_LightscapeLight_to_RTIShader :
    public BaseTranslator_Object,       // translate from Object
    public BaseTranslator_to_RTIShader,     // translate to rti::Shader
    // For listening to changes in the scene's physical scale
    private IRenderSettingsContainer::IChangeNotifier
{
public:

    // The key holds the INode* as well as the scene's physical scale, which light sources
    typedef GenericTranslatorKey_Struct<Translator_LightscapeLight_to_RTIShader, std::shared_ptr<const INodeInstancingPool>> Key;

	// Constructs this translator with the key passed to the translator factory
	Translator_LightscapeLight_to_RTIShader(const Key& key, TranslatorGraphNode& translator_graph_node);
	~Translator_LightscapeLight_to_RTIShader();

	// -- inherited from Translator
	virtual TranslationResult Translate(const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, KeyframeList& keyframesNeeded) override;
    
    // -- inherited from IRenderSettingsContainer::IChangeNotifier
    virtual void NotifyToneOperatorSwapped() override;
    virtual void NotifyToneOperatorParamsChanged() override;
    virtual void NotifyPhysicalScaleChanged() override;
    virtual MSTR GetTimingCategory() const override;

protected:

    // -- inherited from BaseTranslator_to_RTIShader
    virtual MSTR get_shader_name() const override;

    // -- inherited from BaseTranslator_Object
    virtual bool CareAboutMaterialDisplacement() const override;
};

}}	// namespace 
