//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "Translator_LightscapeLight_to_RTIGeometry.h"

#include "TypeUtil.h"
#include "../Util.h"
#include "../RapidRenderSettingsWrapper.h"
#include "../resource.h"

// max sdk
#include <lslights.h>
#include <dllutilities.h>

namespace Max
{;
namespace RapidRTTranslator
{;

Translator_LightscapeLight_to_RTIGeometry::Translator_LightscapeLight_to_RTIGeometry(const Key& key, TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_Object(key, translator_graph_node),
    BaseTranslator_to_RTIGeometry(translator_graph_node),
    Translator(translator_graph_node)
{

}

Translator_LightscapeLight_to_RTIGeometry::~Translator_LightscapeLight_to_RTIGeometry()
{

}

TranslationResult Translator_LightscapeLight_to_RTIGeometry::Translate(const TimeValue t, Interval& validity, ITranslationProgress& /*translation_progress*/, KeyframeList& /*keyframesNeeded*/)
{
    INode* const node = GetNode();
    if(node != nullptr)
    {
        // Initialize the output
        allocate_geometry_outputs(1);
        const rti::GeometryHandle geometry_handle = initialize_geometry_output(0, 0);

        if(geometry_handle.isValid())
        {
            // Determine the shape of the emitter
            const ObjectState& object_state = node->EvalWorldState(t);
            LightscapeLight* ls_light = TypeUtil::to_LightscapeLight(object_state.obj);
            if(ls_light != nullptr)
            {
                const float point_light_radius = RapidRenderSettingsWrapper(GetRenderSessionContext().GetRenderSettings()).get_point_light_radius();
                switch(ls_light->Type())
                {
                case LightscapeLight::TARGET_POINT_TYPE:
                case LightscapeLight::POINT_TYPE:
                    // Point shape
                    switch(ls_light->GetDistribution())
                    {
                    default:
                        DbgAssert(false);
                        // Fall into...
                    case LightscapeLight::WEB_DIST:
                    case LightscapeLight::ISOTROPIC_DIST:
                        // Sphere
                        RRTUtil::CreateRapidSphere(geometry_handle, point_light_radius);
                        break;
                    case LightscapeLight::DIFFUSE_DIST:
                    case LightscapeLight::SPOTLIGHT_DIST:
                        // Disc
                        RRTUtil::CreateRapidDisc(geometry_handle, point_light_radius * 2.0f);
                        break;
                    }
                    break;
                case LightscapeLight::TARGET_LINEAR_TYPE:
                case LightscapeLight::LINEAR_TYPE:
                    // Linear shape
                    {
                        const float cylinder_length = ls_light->GetLength(t, validity);
                        switch(ls_light->GetDistribution())
                        {
                        default:
                            DbgAssert(false);
                            // Fall into...
                        case LightscapeLight::WEB_DIST:
                        case LightscapeLight::ISOTROPIC_DIST:
                            // Use cylinder with small radius for isotropic
                            RRTUtil::CreateRapidCylinder(geometry_handle, point_light_radius * 2.0f, cylinder_length);
                            break;
                        case LightscapeLight::DIFFUSE_DIST:
                        case LightscapeLight::SPOTLIGHT_DIST:
                            // Use rectangle with small width
                            RRTUtil::CreateRapidPlane(geometry_handle, point_light_radius, cylinder_length);
                            break;
                        }
                    }
                    break;
                case LightscapeLight::TARGET_AREA_TYPE:
                case LightscapeLight::AREA_TYPE:
                    // Rectangle shape
                    RRTUtil::CreateRapidPlane(geometry_handle, ls_light->GetWidth(t, validity), ls_light->GetLength(t, validity));
                    break;
                case LightscapeLight::TARGET_DISC_TYPE:
                case LightscapeLight::DISC_TYPE:
                    // Disc shape
                    RRTUtil::CreateRapidDisc(geometry_handle, ls_light->GetRadius(t, validity) * 2.0f);
                    break;
                case LightscapeLight::TARGET_SPHERE_TYPE:
                case LightscapeLight::SPHERE_TYPE:
                    // Sphere shape
                    RRTUtil::CreateRapidSphere(geometry_handle, ls_light->GetRadius(t, validity));
                    break;
                case LightscapeLight::TARGET_CYLINDER_TYPE:
                case LightscapeLight::CYLINDER_TYPE:
                    // Cylinder shape
                    RRTUtil::CreateRapidCylinder(geometry_handle, ls_light->GetRadius(t, validity) * 2.0f, ls_light->GetLength(t, validity));
                    break;
                default:
                    DbgAssert(false);
                    break;
                }
            }

            return TranslationResult::Success;
        }
        else
        {
            return TranslationResult::Failure;
        }
    }
    else
    {
        // Nothing to translate
        SetNumOutputs(0);
        return TranslationResult::Success;
    }
}

MSTR Translator_LightscapeLight_to_RTIGeometry::GetTimingCategory() const 
{
    return MaxSDK::GetResourceStringAsMSTR(IDS_TIMING_LIGHTS);
}

bool Translator_LightscapeLight_to_RTIGeometry::CareAboutMaterialDisplacement() const
{
    return false;
}

}}	// namespace 
