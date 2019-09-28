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

// Local includes
#include "BaseTranslator_Environment_to_RTIShader.h"
#include <RenderingAPI/Renderer/ICameraContainer.h>

namespace Max
{;
namespace RapidRTTranslator
{;

// Translates: Standard environment ==> rti::Shader
class Translator_StandardEnv_to_RTIShader :
	public BaseTranslator_Environment_to_RTIShader,     // Translate to environment rti::Shader
    protected ICameraContainer::IChangeNotifier,
    protected IRenderSessionContext::IChangeNotifier
{
public:
    
	// Constructs this translator with the key passed to the translator factory
	Translator_StandardEnv_to_RTIShader(const Key& key, TranslatorGraphNode& translator_graph_node);
	~Translator_StandardEnv_to_RTIShader();

	// -- inherited from Translator
	virtual TranslationResult Translate(const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, KeyframeList& keyframesNeeded) override;
    virtual Interval CheckValidity(const TimeValue t, const Interval& previous_validity) const override;

protected:

    // -- inherited from ICameraContainer::IChangeNotifier
    virtual void NotifyCameraChanged();

    // -- inherited from IRenderSessionContext::IChangeNotifier
    virtual void NotifyDefaultLightingMaybeChanged();

private:

    // Processes the scene's directional lights into parameters for the environment shader
    void process_directional_lights(int& num_lights, std::vector<rti::Vec4f>& dir_light_directional_spread, std::vector<rti::Vec4f>& dir_light_color_primvisb, const TimeValue t, Interval& validity);

private:
	
    // Set whenever we register ourselves for notifications from the camera
    bool m_camera_notifications_registered;

    // Whether default lights were enabled by the last translation
    bool m_last_translation_enabled_default_lights;
};

}}	// namespace 
