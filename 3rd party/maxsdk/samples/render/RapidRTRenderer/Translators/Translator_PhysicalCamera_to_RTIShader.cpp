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

#include "Translator_PhysicalCamera_to_RTIShader.h"

// Local
#include "../resource.h"
#include "Translator_BakedTexmap_to_RTITexture.h"
#include "Translator_PhysicalCameraAperture_to_RTITexture.h"

// Max SDK
#include <dllutilities.h>
#include <Scene/IPhysicalCamera.h>
#include <RenderingAPI/Renderer/IRenderingLogger.h>

namespace Max
{;
namespace RapidRTTranslator
{;

namespace
{
    float square(float v)
    {
        return v * v;
    }
}

Translator_PhysicalCamera_to_RTIShader::Translator_PhysicalCamera_to_RTIShader(const Key& key, TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_Camera_to_RTIShader(key, translator_graph_node),
    Translator(translator_graph_node)
{

}

Translator_PhysicalCamera_to_RTIShader::~Translator_PhysicalCamera_to_RTIShader()
{

}

bool Translator_PhysicalCamera_to_RTIShader::can_translate(const Key& /*key*/, const TimeValue t, const IRenderSessionContext& render_session_context)
{
    // Can translate if the node points to a IPhysicalCamera
    return (render_session_context.GetCamera().GetPhysicalCamera(t) != nullptr);
}

TranslationResult Translator_PhysicalCamera_to_RTIShader::Translate(const TimeValue translationTime, Interval& newValidity, ITranslationProgress& translationProgress, KeyframeList& /*keyframesNeeded*/) 
{
    // Evaluate the parameters
    if(EvaluateCameraParameters(m_last_translated_camera_parameters, translationTime, GetRenderSessionContext()))
    {
        const CameraParameters& camera_parameters = m_last_translated_camera_parameters;        // shortcut
        newValidity &= camera_parameters.validity;

        const rti::ShaderHandle shader_handle = initialize_output_shader("PhysicalCamera");
        if(shader_handle.isValid())
        {
            // Translate the bokeh texture
            const Translator_PhysicalCameraAperture_to_RTITexture* bokeh_texture_translator = nullptr;
            bool bokeh_texture_translation_failed = false;
            if(camera_parameters.bokeh_texture != nullptr)
            {
                TranslationResult result;
                Translator_PhysicalCameraAperture_to_RTITexture::KeyStruct key(
                    camera_parameters.bokeh_texture, 
                    camera_parameters.camera_resolution, 
                    camera_parameters.bokeh_center_bias, 
                    camera_parameters.aperture_bitmap_affects_exposure);
                bokeh_texture_translator = AcquireChildTranslator<Translator_PhysicalCameraAperture_to_RTITexture>(key, translationTime, translationProgress, result);
                if(result == TranslationResult::Failure)
                {
                    // Report error
                    const TSTR node_name = (GetRenderSessionContext().GetCamera().GetCameraNode() != nullptr) ? GetRenderSessionContext().GetCamera().GetCameraNode()->GetName() : _T("");
                    TSTR msg;
                    msg.printf(MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_BOKEH_TEXTURE_TRANSLATION_FAILED), node_name.data());
                    GetRenderSessionContext().GetLogger().LogMessage(IRenderingLogger::MessageType::Error, msg);

                    bokeh_texture_translation_failed = true;
                }
                else if(result == TranslationResult::Aborted)
                {
                    return result;
                }
            }

            // Translate the distortion texture
            const Translator_BakedTexmap_to_RTITexture* distortion_texture_translator = nullptr;
            if(camera_parameters.distortion_texture != nullptr)
            {
                TranslationResult result;
                distortion_texture_translator = AcquireChildTranslator<Translator_BakedTexmap_to_RTITexture>(camera_parameters.distortion_texture, translationTime, translationProgress, result);
                if(result == TranslationResult::Failure)
                {
                    // Report error
                    const TSTR node_name = (GetRenderSessionContext().GetCamera().GetCameraNode() != nullptr) ? GetRenderSessionContext().GetCamera().GetCameraNode()->GetName() : _T("");
                    TSTR msg;
                    msg.printf(MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_DISTORTION_TEXTURE_TRANSLATION_FAILED), node_name.data());
                    GetRenderSessionContext().GetLogger().LogMessage(IRenderingLogger::MessageType::Error, msg);
                }
                else if(result == TranslationResult::Aborted)
                {
                    return result;
                }
            }

            const rti::TextureHandle pixel_weights_texture_handle = (bokeh_texture_translator != nullptr) ? bokeh_texture_translator->get_pixel_weights_texture_handle() : rti::TextureHandle();
            const rti::TextureHandle x_selection_texture_handle = (bokeh_texture_translator != nullptr) ? bokeh_texture_translator->get_x_selection_cdf_texture_handle() : rti::TextureHandle();
            const rti::TextureHandle y_selection_texture_handle = (bokeh_texture_translator != nullptr) ? bokeh_texture_translator->get_y_selection_cdf_texture_handle() : rti::TextureHandle();
            const IPoint2 bokeh_texture_resolution = (bokeh_texture_translator != nullptr) ? bokeh_texture_translator->get_resolution() : IPoint2(0, 0);
            const rti::TextureHandle distortion_texture_handle = (distortion_texture_translator != nullptr) ? distortion_texture_translator->get_texture_handle() : rti::TextureHandle();

            // Set the parameters on the shader
            rti::TEditPtr<rti::Shader> shader(shader_handle);
            set_shader_float(*shader, "m_image_aspect_ratio", camera_parameters.image_aspect_ratio);
            set_shader_float(*shader, "m_one_over_image_aspect_ratio", 1.0f / camera_parameters.image_aspect_ratio);
            set_shader_float(*shader, "m_film_width", camera_parameters.film_width);
            set_shader_float(*shader, "m_focus_distance", camera_parameters.focus_distance);
            set_shader_float(*shader, "m_lens_focal_length", camera_parameters.lens_focal_length);
            set_shader_float(*shader, "m_near_clip_distance", camera_parameters.clip_near);
            set_shader_float2(*shader, "m_one_over_resolution", Point2(1.0f / camera_parameters.camera_resolution.x, 1.0f / camera_parameters.camera_resolution.y));
            set_shader_float2(*shader, "m_film_offset_pixels", camera_parameters.film_offset_pixels);
            set_shader_float(*shader, "m_cubic_distortion_amount", -camera_parameters.cubic_distortion_amount);
            set_shader_bool(*shader, "m_distortion_texture_enabled", distortion_texture_handle.isValid());
            set_shader_texture2d(*shader, "m_distortion_texture", distortion_texture_handle);
            set_shader_float2(*shader, "m_tilt_amount", camera_parameters.tilt_amount);
            set_shader_float2(*shader, "m_tilt_multiplier", camera_parameters.tilt_multiplier);
            set_shader_int(*shader, "m_aperture_num_blades", camera_parameters.bokeh_num_blades);
            set_shader_float(*shader, "m_aperture_blades_rotation_radians", camera_parameters.bokeh_blades_rotation_radians);
            set_shader_float(*shader, "m_aperture_radial_exponent", camera_parameters.aperture_radial_exponent);
            set_shader_float2(*shader, "m_aperture_anisotropy_mult", camera_parameters.anisotropy_mult);
            set_shader_float(*shader, "m_aperture_optical_vigneting", camera_parameters.optical_vignetting);
            set_shader_bool(*shader, "m_aperture_bitmap_enabled", (bokeh_texture_translator != nullptr));
            set_shader_texture2d(*shader, "m_aperture_bitmap_pixel_weights", pixel_weights_texture_handle);
            set_shader_texture2d(*shader, "m_aperture_bitmap_x_selection_cdf", x_selection_texture_handle);
            set_shader_texture2d(*shader, "m_aperture_bitmap_y_selection_cdf", y_selection_texture_handle);
            set_shader_int2(*shader, "m_aperture_bitmap_resolution", bokeh_texture_resolution);

            // Disable DOF if the bokeh texture failed to translate. This is to let the user clearly know that it failed
            set_shader_bool(*shader, "m_enable_dof", camera_parameters.enable_dof && !bokeh_texture_translation_failed);

            return TranslationResult::Success;
        }
        else
        {
            // Failed to instantiate shader
            return TranslationResult::Failure;
        }
    }
    else
    {
        // Not a physical camera
        return TranslationResult::Failure;
    }
}

bool Translator_PhysicalCamera_to_RTIShader::EvaluateCameraParameters(CameraParameters& parameters, const TimeValue t, const IRenderSessionContext& context)
{
    const ICameraContainer& camera_container = context.GetCamera();
    IPhysicalCamera* const physical_camera = camera_container.GetPhysicalCamera(t);

    if(physical_camera != nullptr)
    {
        parameters = CameraParameters();
        Interval& validity = parameters.validity;
        validity = FOREVER;

        // Fetch the physical camera parameters
        parameters.camera_resolution = camera_container.GetResolution();
        parameters.pixel_aspect_ratio = camera_container.GetPixelAspectRatio();
        parameters.image_aspect_ratio = parameters.pixel_aspect_ratio * float(parameters.camera_resolution.x) / float(parameters.camera_resolution.y);
        parameters.film_width = physical_camera->GetFilmWidth(t, validity);
        parameters.focus_distance = camera_container.GetFocusPlaneDistance(t, validity);
        parameters.lens_focal_length = physical_camera->GetEffectiveLensFocalLength(t, validity) * physical_camera->GetCropZoomFactor(t, validity);
        parameters.clip_near = camera_container.GetClipNear(t, validity);
        const Point2 offset = camera_container.GetImagePlaneOffset(t, validity);
        parameters.film_offset_pixels = -Point2(offset.x, -offset.y);     // Invert Y coords (in Max, top is 0 -- in Rapid, bottom is 0)

        // DOF/bokeh
        parameters.enable_dof = camera_container.GetDOFEnabled(t, validity);
        parameters.bokeh_shape = parameters.enable_dof ? physical_camera->GetBokehShape(t, validity) : IPhysicalCamera::BokehShape::Circular;
        parameters.bokeh_num_blades = (parameters.bokeh_shape == IPhysicalCamera::BokehShape::Bladed) ? physical_camera->GetBokehNumberOfBlades(t, validity) : 0;
        parameters.bokeh_blades_rotation_radians = (parameters.bokeh_shape == IPhysicalCamera::BokehShape::Bladed) ? physical_camera->GetBokehBladesRotationDegrees(t, validity) * (M_PI / 180.0f) : 0.0f;
        parameters.bokeh_center_bias = parameters.enable_dof ? physical_camera->GetBokehCenterBias(t, validity) : 0.0f;
        parameters.aperture_radial_exponent = 0.5f * ((parameters.bokeh_center_bias > 0.0f) ? (1.0f / (1.0f + parameters.bokeh_center_bias)) : (1.0f - parameters.bokeh_center_bias));
        parameters.optical_vignetting = parameters.enable_dof ? physical_camera->GetBokehOpticalVignetting(t, validity) : 0.0f;
        parameters.bokeh_anisotropy = parameters.enable_dof ? physical_camera->GetBokehAnisotropy(t, validity) : 0.0f;
        parameters.anisotropy_mult = 
            (parameters.bokeh_anisotropy > 0.0f) 
            ? Point2(1.0f / (1.0f - parameters.bokeh_anisotropy), (1.0f - parameters.bokeh_anisotropy)) 
            : Point2((1.0f + parameters.bokeh_anisotropy), 1.0f / (1.0f + parameters.bokeh_anisotropy));

        // Distortion
        parameters.distortion_type = physical_camera->GetLensDistortionType(t, validity);
        parameters.cubic_distortion_amount = (parameters.distortion_type == IPhysicalCamera::LensDistortionType::Cubic) ? physical_camera->GetLensDistortionCubicAmount(t, validity) : 0.0f;
        parameters.distortion_texture = (parameters.distortion_type == IPhysicalCamera::LensDistortionType::Texture) ? physical_camera->GetLensDistortionTexture(t, validity) : nullptr;

        // Tilt correction
        parameters.tilt_amount = physical_camera->GetTiltCorrection(t, validity);
        parameters.tilt_multiplier = Point2(1.0f / sqrtf(1.0f + square(parameters.tilt_amount.x)), 1.0f / sqrtf(1.0f + square(parameters.tilt_amount.y)));

        // Bokeh texture
        parameters.bokeh_texture = 
            (parameters.enable_dof && (parameters.bokeh_shape == IPhysicalCamera::BokehShape::Texture))
            ? physical_camera->GetBokehTexture(t, validity)
            : nullptr;
        parameters.aperture_bitmap_affects_exposure = 
            (parameters.bokeh_texture != nullptr) 
            ? physical_camera->GetBokehTextureAffectExposure(t, validity) 
            : false;

        return true;
    }
    else
    {
        // Not a physical camera: can't evaluate the parameters
        return false;
    }
}

MSTR Translator_PhysicalCamera_to_RTIShader::get_shader_name() const 
{
    return MaxSDK::GetResourceStringAsMSTR(IDS_PHYSICAL_CAMERA_SHADER_NAME);
}

Interval Translator_PhysicalCamera_to_RTIShader::CheckValidity(const TimeValue t, const Interval& previous_validity) const 
{
    // Check if any of the camera parameters, which we use and care about, has changed
    // For example, we don't care if the exposure parameters of the physical camera change - and we don't want to react to those changes.
    // We evaluate at the same time as was last evaluated, to make the comparison meaningful.
    CameraParameters camera_parameters;
    const bool is_physical_camera = EvaluateCameraParameters(camera_parameters, t, GetRenderSessionContext());

    // We compare for equality of the struct, including the validity interval it contains, as a change there may be meaningful
    if(is_physical_camera && (camera_parameters != m_last_translated_camera_parameters))
    {
        return NEVER;
    }
    else
    {
        return __super::CheckValidity(t, previous_validity) & camera_parameters.validity & previous_validity;
    }
}

void Translator_PhysicalCamera_to_RTIShader::NotifyCameraChanged() 
{
    // The camera parameters may (or may not) have changed in a way which is meaningful to us. Delay the check as it's not safe to evaluate the camera
    // node within the notification callback.
    Invalidate(true);
}

//==================================================================================================
// struct Translator_PhysicalCamera_to_RTIShader::CameraParameters
//==================================================================================================

Translator_PhysicalCamera_to_RTIShader::CameraParameters::CameraParameters()
    : validity(NEVER),
    camera_resolution(0, 0),
    pixel_aspect_ratio(0.0f),
    image_aspect_ratio(0.0f),
    film_width(0.0f),
    focus_distance(0.0f),
    lens_focal_length(0.0f),
    clip_near(0.0f),
    film_offset_pixels(0.0f, 0.0f),
    enable_dof(false),
    bokeh_shape(IPhysicalCamera::BokehShape::Circular),
    bokeh_num_blades(0),
    bokeh_blades_rotation_radians(0.0f),
    bokeh_center_bias(0.0f),
    aperture_radial_exponent(0.0f),
    optical_vignetting(0.0f),
    bokeh_anisotropy(0.0f),
    anisotropy_mult(0.0f, 0.0f),
    distortion_type(IPhysicalCamera::LensDistortionType::None),
    cubic_distortion_amount(0.0f),
    distortion_texture(nullptr),
    tilt_amount(0.0f, 0.0f),
    tilt_multiplier(0.0f, 0.0f),
    bokeh_texture(nullptr),
    aperture_bitmap_affects_exposure(false)
{

}

bool Translator_PhysicalCamera_to_RTIShader::CameraParameters::operator==(const CameraParameters& other) const
{
    return validity == other.validity
        && camera_resolution == other.camera_resolution
        && pixel_aspect_ratio == other.pixel_aspect_ratio
        && image_aspect_ratio == other.image_aspect_ratio
        && film_width == other.film_width
        && focus_distance == other.focus_distance
        && lens_focal_length == other.lens_focal_length
        && clip_near == other.clip_near
        && film_offset_pixels == other.film_offset_pixels
        && enable_dof == other.enable_dof
        && bokeh_shape == other.bokeh_shape
        && bokeh_num_blades == other.bokeh_num_blades
        && bokeh_blades_rotation_radians == other.bokeh_blades_rotation_radians
        && bokeh_center_bias == other.bokeh_center_bias
        && aperture_radial_exponent == other.aperture_radial_exponent
        && optical_vignetting == other.optical_vignetting
        && bokeh_anisotropy == other.bokeh_anisotropy
        && anisotropy_mult == other.anisotropy_mult
        && distortion_type == other.distortion_type
        && cubic_distortion_amount == other.cubic_distortion_amount
        && distortion_texture == other.distortion_texture
        && tilt_amount == other.tilt_amount
        && tilt_multiplier == other.tilt_multiplier
        && bokeh_texture == other.bokeh_texture
        && aperture_bitmap_affects_exposure == other.aperture_bitmap_affects_exposure;
}

bool Translator_PhysicalCamera_to_RTIShader::CameraParameters::operator!=(const CameraParameters& other) const
{
    return !(*this == other);
}

}}	// namespace 
