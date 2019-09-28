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

// Max SDK
#include <Scene/IPhysicalCamera.h>

namespace Max
{;
namespace RapidRTTranslator
{;

// Translates: PhysicalCamera ==> rti::Shader
class Translator_PhysicalCamera_to_RTIShader : 
    public BaseTranslator_Camera_to_RTIShader
{
public:

    static bool can_translate(const Key& key, const TimeValue t, const IRenderSessionContext& render_session_context);

    Translator_PhysicalCamera_to_RTIShader(const Key& key, TranslatorGraphNode& translator_graph_nodecan_translate);
    ~Translator_PhysicalCamera_to_RTIShader();

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

    // Fetches the camera's parameters values to be translated.
    // Returns false iff not a physical camera
    static bool EvaluateCameraParameters(CameraParameters& parameters, const TimeValue t, const IRenderSessionContext& context);

private:

    // The set of parameters, from the camera, which we use in translation
    struct CameraParameters
    {
        CameraParameters();
        bool operator==(const CameraParameters& other) const;
        bool operator!=(const CameraParameters& other) const;

        // The time and validity interval at which the parameters were last fetched
        Interval validity;

        IPoint2 camera_resolution;
        float pixel_aspect_ratio;
        float image_aspect_ratio;
        float film_width;
        float focus_distance;
        float lens_focal_length;
        float clip_near;
        Point2 film_offset_pixels;
        bool enable_dof;
        IPhysicalCamera::BokehShape bokeh_shape;
        int bokeh_num_blades;
        float bokeh_blades_rotation_radians;
        float bokeh_center_bias;
        float aperture_radial_exponent;
        float optical_vignetting;
        float bokeh_anisotropy;
        Point2 anisotropy_mult;
        IPhysicalCamera::LensDistortionType distortion_type;
        float cubic_distortion_amount;
        Texmap* distortion_texture;
        Point2 tilt_amount;
        Point2 tilt_multiplier;
        Texmap* bokeh_texture;
        bool aperture_bitmap_affects_exposure;
    };

    // Remembers the parameter values which were last translated, such that we may determine whether the camera needs to be re-translated
    CameraParameters m_last_translated_camera_parameters;
};

}}	// namespace 
