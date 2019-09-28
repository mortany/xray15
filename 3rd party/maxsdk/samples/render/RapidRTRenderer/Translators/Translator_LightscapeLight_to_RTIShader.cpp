//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "Translator_LightscapeLight_to_RTIShader.h"

// local includes
#include "../resource.h"
#include "TypeUtil.h"
#include "Translator_LightscaleLightWeb_to_RTITexture.h"
// max SDK
#include <object.h>
#include <inode.h>
#include <lslights.h>
#include <units.h>
#include <dllutilities.h>
#include <trig.h>
#include <RenderingAPI/Renderer/IRenderSettingsContainer.h>
#include <RenderingAPI/Renderer/IRenderingLogger.h>

// std includes
#include <math.h>
#include <algorithm>

#undef min
#undef max

namespace Max
{;
namespace RapidRTTranslator
{;

Translator_LightscapeLight_to_RTIShader::Translator_LightscapeLight_to_RTIShader(const Key& key, TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_Object(key, translator_graph_node),
    BaseTranslator_to_RTIShader(translator_graph_node),
    Translator(translator_graph_node)
{
    // Monitor for changes in the scene physical scale
    GetRenderSessionContext().GetRenderSettings().RegisterChangeNotifier(*this);
}

Translator_LightscapeLight_to_RTIShader::~Translator_LightscapeLight_to_RTIShader()
{
    GetRenderSessionContext().GetRenderSettings().UnregisterChangeNotifier(*this);
}

TranslationResult Translator_LightscapeLight_to_RTIShader::Translate(const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, KeyframeList& /*keyframesNeeded*/)
{
    INode* const node = GetNode();
    if(node != nullptr)
    {
        const ObjectState& object_state = node->EvalWorldState(t);
        LightscapeLight* ls_light = TypeUtil::to_LightscapeLight(object_state.obj);
        if(ls_light != nullptr)
        {
            // Fetch the required parameters;
            Color flux(0.0f, 0.0f, 0.0f);
            if(ls_light->GetUseLight())
            {
                // Get the intensity
                flux = ls_light->GetRGBColor(t, validity);
                flux *= ls_light->GetRGBFilter(t, validity);
                flux *= ls_light->GetResultingIntensity(t, validity);

                // Physical scale converts candela to generic rgb value (to be consistent with standard lights; values get converted back to candelas
                // in the tone operator)
                flux /= GetRenderSessionContext().GetRenderSettings().GetPhysicalScale(t,validity);

                // Adjust with the scene unit scale to keep the emission consistent regardless of internal unit representation
                const float meters_per_internal_unit = GetMasterScale(UNITS_METERS);
                flux /= (meters_per_internal_unit * meters_per_internal_unit);

                // Convert intensity to flux
                switch(ls_light->GetDistribution())
                {
                default:
                    DbgAssert(false);
                    // Fall into...
                case LightscapeLight::ISOTROPIC_DIST:
                    // Equal lighting in all directions of the sphere: integration of constant distribution over sphere is 4PI
                    flux *= (4.0f * PI);

                    // Planar lights with isotropic distributions are two-sided
                    switch(ls_light->Type())
                    {
                    case LightscapeLight::TARGET_DISC_TYPE:
                    case LightscapeLight::DISC_TYPE:
                    case LightscapeLight::TARGET_AREA_TYPE:
                    case LightscapeLight::AREA_TYPE:
                        // Planar isotropic lights are physically impossible: output an error
                        {
                            MSTR msg;
                            msg.printf(MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_PLANAR_ISOTROPIC_LIGHT), node->GetName());
                            GetRenderSessionContext().GetLogger().LogMessage(IRenderingLogger::MessageType::Error, msg);
                        }
                        break;
                    }

                    break;
                case LightscapeLight::DIFFUSE_DIST:
                    // Lighting in one hemisphere only, with cosine falloff: integration of cosine in hemisphere is PI.
                    flux *= PI;
                    break;
                case LightscapeLight::SPOTLIGHT_DIST:
                    // Lighting in one hemisphere only, with cosine falloff: integration of cosine in hemisphere is PI.
                    flux *= PI;
                    break;
                case LightscapeLight::WEB_DIST:
                    // Web distributions cover the entire sphere
                    flux *= (4.0f * PI);
                    break;
                }
            }

            // Setup the RapidRT shader
            // The shader to be used depends on the distribution (we use several small shaders rather than one uber shader, for performance)
            switch(ls_light->GetDistribution())
            {
            default:
                DbgAssert(false);
                // Fall into...
            case LightscapeLight::DIFFUSE_DIST:
            case LightscapeLight::ISOTROPIC_DIST:
                // Isotropic and diffuse are the same: Diffuse is merely a side-effect of surface visibility being dependent on the cosine of the viewing
                // angle.
                {
                    const rti::ShaderHandle shader_handle = initialize_output_shader("PhotometricLightUniform");
                    if(shader_handle.isValid())
                    {
                        rti::TEditPtr<rti::Shader> shader(shader_handle);

                        set_shader_float3(*shader, "m_flux", flux);
                        set_output_is_emitter(true);
                        return TranslationResult::Success;
                    }
                }
                break;
            case LightscapeLight::WEB_DIST:
                // Isotropic and diffuse are the same: Diffuse is merely a side-effect of surface visibility being dependent on the cosine of the viewing
                // angle.
                {
                    TranslationResult result;
                    // Translate the web distribution data as a texture
                    std::shared_ptr<const INodeInstancingPool> node_pool = GetNodePool();
                    const Translator_LightscaleLightWeb_to_RTITexture* web_translator = AcquireChildTranslator<Translator_LightscaleLightWeb_to_RTITexture>(node_pool, t, translation_progress, result);
                    if(result == TranslationResult::Success)
                    {
                        const rti::ShaderHandle shader_handle = initialize_output_shader("PhotometricLightWeb");
                        if(shader_handle.isValid())
                        {
                            rti::TEditPtr<rti::Shader> shader(shader_handle);

                            set_shader_float3(*shader, "m_flux", flux);
                            set_shader_texture2d(*shader, "m_web_map", web_translator->get_web_texture_handle());

                            set_output_is_emitter(true);
                            return TranslationResult::Success;
                        }
                    }
                    else
                    {
                        // Failure to translate photometric data equates to failure to translate shader.
                        return result;
                    }
                }
                break;
            case LightscapeLight::SPOTLIGHT_DIST:
                {
                    const rti::ShaderHandle shader_handle = initialize_output_shader("PhotometricLightSpot");
                    if(shader_handle.isValid())
                    {
                        rti::TEditPtr<rti::Shader> shader(shader_handle);

                        const float hotspot_radians = ls_light->GetHotspot(t, validity) * (M_PI / 180.0f);
                        const float cosbeam = std::min(cosf(hotspot_radians * 0.5f), 1.0f); // Clamp to 90 degrees
                        const float decay = logf(0.5f) / logf(cosbeam);
                        const float falloff_radians = ls_light->GetFallsize(t, validity) * (M_PI / 180.0f);
                        const float cos_falloff = cosf(falloff_radians * 0.5f);
                        const float fade_width = cosbeam - cos_falloff;
                        const float cos_edge = cos_falloff - fade_width;

                        set_shader_float3(*shader, "m_flux", flux);
                        // Subtract 1 from cosine exponent: this makes the area spot behave like a point spot, by negating the cosine falloff inherent to area lights.
                        set_shader_float(*shader, "m_cosine_exponent", decay - 1.0f);
                        set_shader_float(*shader, "m_cos_falloff", cos_falloff);
                        set_shader_float(*shader, "m_fade_factor", (1.0f / fade_width));
                        set_shader_float(*shader, "m_cos_edge", std::max(cos_edge, 0.0f));      // Clamp to 90 degrees
                        set_output_is_emitter(true);
                        return TranslationResult::Success;
                    }
                }
                break;
            }
        }

        return TranslationResult::Failure;
    }
    else
    {
        // Nothing to translate
        SetNumOutputs(0);
        return TranslationResult::Success;
    }        
}

void Translator_LightscapeLight_to_RTIShader::NotifyToneOperatorSwapped() 
{
    // Don't care, tone operator not used by this translator
}

void Translator_LightscapeLight_to_RTIShader::NotifyToneOperatorParamsChanged() 
{
    // Don't care, tone operator not used by this translator
}

void Translator_LightscapeLight_to_RTIShader::NotifyPhysicalScaleChanged() 
{
    // Physical scale change means we have to re-translate
    Invalidate();
}

MSTR Translator_LightscapeLight_to_RTIShader::get_shader_name() const
{
    INode* node = GetNode();
    if(DbgVerify(node != nullptr))
    {
        MSTR name;
        name.printf(MaxSDK::GetResourceStringAsMSTR(IDS_NODE_LIGHTSHADER_NAME_FORMAT), node->GetName());
        return name;   
    }
    else
    {
        return _T("");
    }
}

MSTR Translator_LightscapeLight_to_RTIShader::GetTimingCategory() const 
{
    return MaxSDK::GetResourceStringAsMSTR(IDS_TIMING_LIGHTS);
}

bool Translator_LightscapeLight_to_RTIShader::CareAboutMaterialDisplacement() const
{
    return false;
}

}}	// namespace 
