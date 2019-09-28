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

#include "Translator_Camera_to_RTICamera.h"

// Local
#include "BaseTranslator_Camera_to_RTIShader.h"
#include "PBUtil.h"
#include "../Util.h"
#include "../RapidRenderSettingsWrapper.h"

// RRT
#include <rti/scene/Camera.h>

// MAX SDK
#include <ipoint2.h>
#include <Rendering/ToneOperator.h>
#include <RenderingAPI/Renderer/ISceneContainer.h>
#include <RenderingAPI/Renderer/IRenderSettingsContainer.h>

// FUBAR
#undef max
#undef min

namespace Max
{;
namespace RapidRTTranslator
{;

Translator_Camera_to_RTICamera::Translator_Camera_to_RTICamera(const Key& /*key*/, TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_Camera(translator_graph_node),
    BaseTranslator_to_RTI(translator_graph_node),
    Translator(translator_graph_node)
{
    // Register for change notifications
    GetRenderSessionContext().GetScene().RegisterChangeNotifier(*this);
}

Translator_Camera_to_RTICamera::~Translator_Camera_to_RTICamera()
{
    // Unregister for change notifications
    GetRenderSessionContext().GetScene().UnregisterChangeNotifier(*this);
}

rti::CameraHandle Translator_Camera_to_RTICamera::get_output_camera() const
{
    return get_output_handle<rti::CameraHandle>(0);
}

TranslationResult Translator_Camera_to_RTICamera::Translate(const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, KeyframeList& /*keyframesNeeded*/)
{
    // Fetch the camera parameters
    m_last_translated_camera_parameters = EvaluateCameraParameters(t, GetRenderSessionContext());
    const CameraParameters& camera_parameters = m_last_translated_camera_parameters;        // shortcut
    validity &= camera_parameters.validity;

    // Translate the camera shader
    TranslationResult result;
    const BaseTranslator_Camera_to_RTIShader* const camera_shader_translator = AcquireChildTranslator<BaseTranslator_Camera_to_RTIShader>(camera_parameters.camera_node, t, translation_progress, result);
    if(camera_shader_translator == nullptr)
    {
        return result;
    }

    // Translate the camera
    const rti::CameraHandle camera_handle = initialize_output_handle<rti::CameraHandle>(0);
    if(camera_handle.isValid())
    {
        rti::TEditPtr<rti::Camera> camera(camera_handle);

        // Setup the camera shader
        camera->setShader(camera_shader_translator->get_output_shader());

        // Translate general parameters
        camera->setResolution(camera_parameters.resolution.x, camera_parameters.resolution.y);
        camera->setAspectRatio(camera_parameters.aspect_ratio);
        camera->setWindowRegion(
            camera_parameters.region.left, 
            camera_parameters.resolution.y - camera_parameters.region.bottom - 1,      // Invert Y coords (in Max, top is 0 -- in Rapid, bottom is 0)
            camera_parameters.region.right,
            camera_parameters.resolution.y - camera_parameters.region.top - 1);        // Invert Y coords (in Max, top is 0 -- in Rapid, bottom is 0)
        camera->setOffset(camera_parameters.offset.x, -camera_parameters.offset.y);     // Invert Y coords (in Max, top is 0 -- in Rapid, bottom is 0)

        // Translate view parameters
        switch(camera_parameters.camera_projection_type)
        {
        case ICameraContainer::ProjectionType::Perspective:
            camera->setDepthOfField(camera_parameters.focus_distance, camera_parameters.aperture_radius);     // Convert lens diameter to radius
            camera->initPerspective(rti::RTI_RAD_TO_DEG * camera_parameters.fov_radians);
            break;
        case ICameraContainer::ProjectionType::Orthographic:
            camera->setDepthOfField(0.0f, 0.0f);
            camera->initOrthographic(camera_parameters.orthoghaphic_aperture_height);
            break;
        default:
            DbgAssert(false);
            return TranslationResult::Failure;
        }

        // Set transform
        {
            // Fetch the transforms
            MotionTransforms transforms = camera_parameters.transforms;

            // If using an orthographic camera without clipping planes, move the camera outside the bounding box to ensure the scene isn't clipped in any way
            // (Rapid's orthographic cameras clip everything behind them)
            if(camera_parameters.move_camera_outside_of_scene_bounding_box)
            {
                // Determine the current position of the camera
                const Point3 camera_position = transforms.shutter_open.PointTransform(Point3(0.0f, 0.0f, 0.0f));

                // Determine the scene bounding box
                const Box3 scene_bounding_box = GetRenderSessionContext().GetScene().GetSceneBoundingBox(t, validity);

                // Move the camera back enough to ensure it's outside the bounding box
                const float offset_needed = (camera_position - scene_bounding_box.Center()).Length() + (scene_bounding_box.Width()).Length();
                // Calculate what direction we have to move the camera into
                const Point3 camera_direction = Normalize(transforms.shutter_open.VectorTransform(Point3(0.0f, 0.0f, -1.0f)));
                const Point3 offset_vector = offset_needed * -camera_direction;
                transforms.shutter_open.Translate(offset_vector);

                // Offset the motion transform by the same amount. Hopefully that's OK - though it's theoretically possible for the motion camera
                // to not be completely outside the scene.
                transforms.shutter_close.Translate(offset_vector);
            }

            // Set the transforms
            camera->setTransform(RRTUtil::convertMatrix(transforms.shutter_open));
            camera->setMotionTransform(RRTUtil::convertMatrix(transforms.shutter_close));

            // Setup camera shutter
            RRTUtil::check_rti_result(camera->setParameter1f(rti::CAMERA_PARAM_SHUTTER_OPEN, camera_parameters.shutter_open));
            RRTUtil::check_rti_result(camera->setParameter1f(rti::CAMERA_PARAM_SHUTTER_CLOSE, camera_parameters.shutter_close));

            // Setup frame number (used to trigger animated noise)
            // We use the time as the frame number, such that two single-frame renders at the same time value will generate similar noise.
            const bool enable_animated_noise = RapidRenderSettingsWrapper(GetRenderSessionContext().GetRenderSettings()).get_enable_animated_noise();
            if(enable_animated_noise)
            {
                RRTUtil::check_rti_result(camera->setParameter1i(rti::CAMERA_PARAM_FRAME_NUMBER, t));
                validity &= Interval(t, t);     // Need to update at every time value
            }
        }

        RRTUtil::check_rti_result(camera->setParameter1i(rti::CAMERA_PARAM_TONEOP, rti::TONEOP_NONE));

        return TranslationResult::Success;
    }

    return TranslationResult::Failure;
}

Translator_Camera_to_RTICamera::CameraParameters Translator_Camera_to_RTICamera::EvaluateCameraParameters(const TimeValue t, const IRenderSessionContext& context)
{
    CameraParameters parameters;
    Interval& validity = parameters.validity;
    validity = FOREVER;

    const ICameraContainer& camera_container = context.GetCamera();

    parameters.camera_node = camera_container.GetCameraNode();
    parameters.resolution = camera_container.GetResolution();
    parameters.aspect_ratio = camera_container.GetPixelAspectRatio() * parameters.resolution.x / parameters.resolution.y;
    parameters.region = camera_container.GetRegion();
    parameters.offset = camera_container.GetImagePlaneOffset(t, validity);
    parameters.camera_projection_type = camera_container.GetProjectionType(t, validity);
    const bool clip_enabled = camera_container.GetClipEnabled(t, validity);
            // If using an orthographic camera without clipping planes, move the camera outside the bounding box to ensure the scene isn't clipped in any way
            // (Rapid's orthographic cameras clip everything behind them)
    //!! TODO: Move the bounding box logic into ICameraContainer.
    parameters.move_camera_outside_of_scene_bounding_box = !clip_enabled && (parameters.camera_projection_type == ICameraContainer::ProjectionType::Orthographic);

    switch(parameters.camera_projection_type)
    {
    case ICameraContainer::ProjectionType::Perspective:
        {
            // Query DOF parameters
            const bool enable_dof = camera_container.GetDOFEnabled(t, validity);
            parameters.focus_distance = enable_dof ? camera_container.GetFocusPlaneDistance(t, validity) : 0.0f;
            parameters.aperture_radius = enable_dof ? camera_container.GetLensApertureRadius(t, validity) : 0.0f;
            parameters.fov_radians = camera_container.GetPerspectiveFOVRadians(t, validity);
        }
        break;
    case ICameraContainer::ProjectionType::Orthographic:
        parameters.orthoghaphic_aperture_height = camera_container.GetOrthographicApertureHeight(t, validity);
        break;
    }

    // Fetch the transforms
    parameters.transforms = camera_container.EvaluateCameraTransform(t, validity);

    // Setup camera shutter - always 0 to 1, as we bake the actual shutter time in the scene itself.
    const bool motion_blur_enabled = (camera_container.GetGlobalMotionBlurSettings(t, validity).shutter_duration != 0);
    parameters.shutter_open = 0.0f;
    parameters.shutter_close = (motion_blur_enabled ? 1.0f : 0.0f);

    return parameters;
}

Interval Translator_Camera_to_RTICamera::CheckValidity(const TimeValue t, const Interval& previous_validity) const 
{
    // Check if any of the camera parameters, which we use and care about, has changed
    // For example, we don't care if the exposure parameters of the physical camera change - and we don't want to react to those changes.
    // We evaluate at the same time as was last evaluated, to make the comparison meaningful.
    const CameraParameters camera_parameters = EvaluateCameraParameters(t, GetRenderSessionContext());

    // We compare for equality of the struct, including the validity interval it contains, as a change there may be meaningful
    if(camera_parameters != m_last_translated_camera_parameters)
    {
        return NEVER;
    }
    else
    {
        return __super::CheckValidity(t, previous_validity) & camera_parameters.validity & previous_validity;
    }
}

void Translator_Camera_to_RTICamera::NotifyCameraChanged()
{
    // The camera parameters may (or may not) have changed in a way which is meaningful to us. Delay the check as it's not safe to evaluate the camera
    // node within the notification callback.
    Invalidate(true);
}

void Translator_Camera_to_RTICamera::NotifySceneNodesChanged()
{
    // Don't care
}

void Translator_Camera_to_RTICamera::NotifySceneBoundingBoxChanged()
{
    // Invalidate iff we're using the scene bounding box
    if(m_last_translated_camera_parameters.move_camera_outside_of_scene_bounding_box)
    {
        Invalidate();
    }
}

//==================================================================================================
// struct Translator_Camera_to_RTICamera::CameraParameters
//==================================================================================================

Translator_Camera_to_RTICamera::CameraParameters::CameraParameters()
    : validity(NEVER),
    camera_node(nullptr),
    resolution(0, 0),
    aspect_ratio(0.0f),
    region(),
    offset(0.0f, 0.0f),
    camera_projection_type(ICameraContainer::ProjectionType::Perspective),
    move_camera_outside_of_scene_bounding_box(false),
    focus_distance(0.0f),
    aperture_radius(0.0f),
    fov_radians(0.0f),
    orthoghaphic_aperture_height(0.0f),
    transforms(),
    shutter_open(0.0f),
    shutter_close(0.0f)
{

}

bool Translator_Camera_to_RTICamera::CameraParameters::operator==(const CameraParameters& other) const
{
    return validity == other.validity
        && camera_node == other.camera_node
        && resolution == other.resolution
        && aspect_ratio == other.aspect_ratio
        && region == other.region
        && offset == other.offset
        && camera_projection_type == other.camera_projection_type
        && move_camera_outside_of_scene_bounding_box == other.move_camera_outside_of_scene_bounding_box
        && focus_distance == other.focus_distance
        && aperture_radius == other.aperture_radius
        && fov_radians == other.fov_radians
        && orthoghaphic_aperture_height == other.orthoghaphic_aperture_height
        && transforms == other.transforms
        && shutter_open == other.shutter_open
        && shutter_close == other.shutter_close;
}

bool Translator_Camera_to_RTICamera::CameraParameters::operator!=(const CameraParameters& other) const
{
    return !(*this == other);
}


}}	// namespace 
