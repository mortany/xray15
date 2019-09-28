//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "Translator_StandardEnv_to_RTIShader.h"

// local
#include "Translator_SceneRoot_to_RTIGroup.h"

// Max sdk
#include <RenderingAPI/Renderer/ISceneContainer.h>
#include <RenderingAPI/Renderer/ICameraContainer.h>
#include <RenderingAPI/Renderer/IRenderSettingsContainer.h>
#include <RenderingAPI/Renderer/IEnvironmentContainer.h>
#include <Rendering/Renderer.h>
#include <trig.h>
#include <triobj.h>

using namespace MaxSDK;

namespace
{
    static bool TMIsAllZero(const Matrix3 &m) {
        for (int i=0; i<4; i++) {
            Point3 p = m.GetRow(i);
            if (p.x!=0||p.y!=0||p.z!=0) 
                return false;
        }
        return true;
    }
}

namespace Max
{;
namespace RapidRTTranslator
{;

Translator_StandardEnv_to_RTIShader::Translator_StandardEnv_to_RTIShader(const Key& key, TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_Environment_to_RTIShader(key, translator_graph_node),
    Translator(translator_graph_node),
    m_camera_notifications_registered(false),
    m_last_translation_enabled_default_lights(false)
{
    // Register for changes to the default lights
    GetRenderSessionContext().RegisterChangeNotifier(*this);
}

Translator_StandardEnv_to_RTIShader::~Translator_StandardEnv_to_RTIShader()
{
    if(m_camera_notifications_registered)
    {
        GetRenderSessionContext().GetCamera().UnregisterChangeNotifier(*this);
    }

    GetRenderSessionContext().UnregisterChangeNotifier(*this);
}

TranslationResult Translator_StandardEnv_to_RTIShader::Translate(const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, KeyframeList& /*keyframesNeeded*/)
{
	// Stop listening for camera notifications. If needed, we'll register again while translating
    if(m_camera_notifications_registered)
    {
        GetRenderSessionContext().GetCamera().UnregisterChangeNotifier(*this);
        m_camera_notifications_registered = false;
    }

    // Determine if default lights are to be enabled
    m_last_translation_enabled_default_lights = GetRenderSessionContext().GetDefaultLightingEnabled(true, t, validity);

    // Process directional lights
    int num_directional_lights = 0;
    std::vector<rti::Vec4f> dir_light_directional_spread;
    std::vector<rti::Vec4f> dir_light_color_primvisb;
    if(m_last_translation_enabled_default_lights)
    {
        process_directional_lights(num_directional_lights, dir_light_directional_spread, dir_light_color_primvisb, t, validity);
    }

	// Translate the StandardEnvironment object in Rapid
	return AMG_Translate(GetEnvironment(), t, validity, translation_progress, num_directional_lights, &dir_light_directional_spread, &dir_light_color_primvisb);
}

void Translator_StandardEnv_to_RTIShader::NotifyCameraChanged()
{
    Invalidate();
}

void Translator_StandardEnv_to_RTIShader::process_directional_lights(int& num_lights, std::vector<rti::Vec4f>& dir_light_directional_spread, std::vector<rti::Vec4f>& dir_light_color_primvisb, const TimeValue t, Interval& validity)
{
    num_lights = 0;
    dir_light_directional_spread.clear();
    dir_light_color_primvisb.clear();

    const IRenderSessionContext& session_context = GetRenderSessionContext();
    const ICameraContainer& camera_container = session_context.GetCamera();

    // Process the default lights
    const std::vector<DefaultLight> default_lights = session_context.GetDefaultLights();
    for(const DefaultLight& default_light : default_lights)
    {
        // If the default light doesn't have a transform, then we use the view transform
        Matrix3 light_tm = default_light.tm;
        if(TMIsAllZero(default_light.tm))
        {
            light_tm = camera_container.EvaluateCameraTransform(t, validity).shutter_open;
            light_tm.Invert();
            // Register for notifications from the camera
            if(!m_camera_notifications_registered)
            {
                camera_container.RegisterChangeNotifier(*this);
                m_camera_notifications_registered = true;
            }
        }

        Point3 light_direction;
        switch(default_light.ls.type)
        {
        case SPOT_LGT:
            // We support spots as omni lights: fall through...
        case OMNI_LGT:
            light_direction = Normalize(light_tm.PointTransform(Point3(0.0f, 0.0f, 0.0f)));
            break;
        case DIRECT_LGT:
            // Directional light points towards Z
            light_direction = Normalize(light_tm.VectorTransform(Point3(0.0f, 0.0f, 1.0f)));
            break;
        case AMBIENT_LGT:
            // I don't see the mental ray integration supporting ambient default lights, so I'll assume they don't exist?
            continue;
        }
        const float light_spread_angle = DEG_TO_RAD * 0.5f;  // 0.5 degree is approximately the spread of the sun from the earth
        // Calculate the portion of the hemisphere covered by the light, and use that as a multiplier for the intensity of the light.
        // This follows from assuming that we would get the desired light intensity if the light were hit by 100% of random environment samples
        // (which it won't be so we have to compensate by the probability of hitting the light).
        const float cos_light_spread = cosf(light_spread_angle);
        const float hemisphere_coverage = 1.0f - cos_light_spread;
        // Division by PI is to convert max's intensity units into physical units.
        const Color intensity = (1.0f / PI) * default_light.ls.color * default_light.ls.intens / hemisphere_coverage;

        ++num_lights;
        dir_light_directional_spread.push_back(rti::Vec4f(light_direction.x, light_direction.y, light_direction.z, cos_light_spread));
        dir_light_color_primvisb.push_back(rti::Vec4f(intensity.r, intensity.g, intensity.b, 0.0f));
    }
}

Interval Translator_StandardEnv_to_RTIShader::CheckValidity(const TimeValue t, const Interval& previous_validity) const 
{
    // Check if the state of default lights has changed
    Interval new_validity;
    const bool new_enable_default_lights = GetRenderSessionContext().GetDefaultLightingEnabled(true, t, new_validity);
    if(new_enable_default_lights != m_last_translation_enabled_default_lights)
    {
        return NEVER;
    }
    else
    {
        return __super::CheckValidity(t, previous_validity) & new_validity & previous_validity;
    }
}

void Translator_StandardEnv_to_RTIShader::NotifyDefaultLightingMaybeChanged()
{
    // The state of the default lights may have changed
    Invalidate(true);
}

}}	// namespace 
