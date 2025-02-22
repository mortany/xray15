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
#include "BaseTranslator_to_RTI.h"
// Max SDK
#include <RenderingAPI/Translator/GenericTranslatorKeys.h>
#include <RenderingAPI/Renderer/ISceneContainer.h>
#include <RenderingAPI/Translator/BaseTranslators/BaseTranslator_Camera.h>
#include <RenderingAPI/Renderer/ICameraContainer.h>
// rti
#include <rti/scene/camera.h>

class INode;
class Interval;
class ViewParams;
class ToneOperator;

namespace Max
{;
namespace RapidRTTranslator
{;

// Translates: Camera ==> rti::Camera

// Translates the camera from a SceneContainer into a rti::Camera
class Translator_Camera_to_RTICamera :
    public BaseTranslator_Camera,       // Translate from Camera
	public BaseTranslator_to_RTI,       // Translate to rti::Camera
    private ISceneContainer::IChangeNotifier
{
public:

    
    typedef GenericTranslatorKey_Empty<Translator_Camera_to_RTICamera> Key;

	Translator_Camera_to_RTICamera(const Key& key, TranslatorGraphNode& translator_graph_node);
	~Translator_Camera_to_RTICamera();

    // Accesses the outputs generated by this translator
    rti::CameraHandle get_output_camera() const;

    // -- inherited from Translator
    virtual TranslationResult Translate(const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, KeyframeList& keyframesNeeded) override;
    virtual Interval CheckValidity(const TimeValue t, const Interval& previous_validity) const override;

private:

    struct CameraParameters;

    // Fetches the camera's parameters values to be translated
    static CameraParameters EvaluateCameraParameters(const TimeValue t, const IRenderSessionContext& context);

    // -- inherited from CameraContainer::IChangeNotifier
    virtual void NotifyCameraChanged() override;

    // -- inherited from ISceneContainer::IChangeNotifier
    virtual void NotifySceneNodesChanged();
    virtual void NotifySceneBoundingBoxChanged();

private:

    // The set of parameters, from the camera, which we use in translation
    struct CameraParameters
    {
        CameraParameters();
        bool operator==(const CameraParameters& other) const;
        bool operator!=(const CameraParameters& other) const;

        // The time and validity interval at which the parameters were last fetched
        Interval validity;

        INode* camera_node;
        IPoint2 resolution;
        float aspect_ratio;
        Box2 region;
        Point2 offset;
        ICameraContainer::ProjectionType camera_projection_type;
        bool move_camera_outside_of_scene_bounding_box;
        float focus_distance;
        float aperture_radius;
        float fov_radians;
        float orthoghaphic_aperture_height;
        MotionTransforms transforms;
        float shutter_open;
        float shutter_close;
    };

    // Remembers the parameter values which were last translated, such that we may determine whether the camera needs to be re-translated
    CameraParameters m_last_translated_camera_parameters;
};

}}	// namespace 
