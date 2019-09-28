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

#include "BaseTranslator_Camera_to_RTIShader.h"

namespace Max
{;
namespace RapidRTTranslator
{;

// Translates:standard camera ==> rti::Shader
class Translator_StandardCamera_to_RTIShader : 
    public BaseTranslator_Camera_to_RTIShader
{
public:

    Translator_StandardCamera_to_RTIShader(const Key& key, TranslatorGraphNode& translator_graph_node);
    ~Translator_StandardCamera_to_RTIShader();

    // -- inherited from Translator
    virtual TranslationResult Translate(const TimeValue translationTime, Interval& newValidity, ITranslationProgress& translationProgress, KeyframeList& keyframesNeeded) override;
    virtual Interval CheckValidity(const TimeValue t, const Interval& previous_validity) const override;

protected:

    // -- inherited from BaseTranslator_to_RTIShader
    virtual MSTR get_shader_name() const override;

    // -- inherited from ICameraContainer::IChangeNotifier
    virtual void NotifyCameraChanged() override;

private:
    
    struct CameraParameters;

    static void EvaluateCameraParameters(CameraParameters& parameters, const TimeValue t, const IRenderSessionContext& context);

private:

    // The set of parameters, from the camera, which we use in translation
    struct CameraParameters
    {
        CameraParameters();
        bool operator==(const CameraParameters& other) const;
        bool operator!=(const CameraParameters& other) const;

        // The time and validity interval at which the parameters were last fetched
        Interval validity;

        float clip_near;
    };

    // Remembers the parameter values which were last translated, such that we may determine whether the camera needs to be re-translated
    CameraParameters m_last_translated_camera_parameters;
};

}}	// namespace 
