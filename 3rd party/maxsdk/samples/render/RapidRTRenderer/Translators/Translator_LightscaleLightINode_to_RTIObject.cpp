//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "Translator_LightscaleLightINode_to_RTIObject.h"

#include "Translator_GeomObject_to_RTIGeometry.h"
#include "Translator_LightscapeLight_to_RTIGeometry.h"
#include "Translator_LightscapeLight_to_RTIShader.h"
#include "TypeUtil.h"
#include <RenderingAPI/Renderer/IRenderSettingsContainer.h>
#include <RenderingAPI/Translator/ITranslationManager.h>

#include <lslights.h>
#include <icustattribcontainer.h>

namespace Max
{;
namespace RapidRTTranslator
{;

Translator_LightscaleLightINode_to_RTIObject::Translator_LightscaleLightINode_to_RTIObject(
    const Key& key, 
    TranslatorGraphNode& translator_graph_node)
    
    : Translator_LightINode_to_RTIObject(key, translator_graph_node),
    Translator(translator_graph_node)
{


}

Translator_LightscaleLightINode_to_RTIObject::~Translator_LightscaleLightINode_to_RTIObject()
{

}

bool Translator_LightscaleLightINode_to_RTIObject::can_translate(const Key& key, const TimeValue t)
{
    INode& node = key;
    const ObjectState& object_state = node.EvalWorldState(t);
    LightscapeLight* ls_light = TypeUtil::to_LightscapeLight(object_state.obj);
    return (ls_light != nullptr);
}

const BaseTranslator_to_RTIGeometry* Translator_LightscaleLightINode_to_RTIObject::acquire_geometry_translator(const TimeValue t, Interval& /*validity*/, ITranslationProgress& translation_progress, TranslationResult& result)
{
    std::shared_ptr<const INodeInstancingPool> node_pool = GetNodePool(t);
    return AcquireChildTranslator<Translator_LightscapeLight_to_RTIGeometry>(node_pool, t, translation_progress, result);
}

const BaseTranslator_to_RTIShader* Translator_LightscaleLightINode_to_RTIObject::acquire_shader_translator(const MtlID /*material_id*/, const TimeValue t, Interval& /*validity*/, ITranslationProgress& translation_progress, TranslationResult& result)
{
    std::shared_ptr<const INodeInstancingPool> node_pool = GetNodePool(t);
    return AcquireChildTranslator<Translator_LightscapeLight_to_RTIShader>(node_pool, t, translation_progress, result);
}

bool Translator_LightscaleLightINode_to_RTIObject::is_light_visible(const TimeValue t, Interval& validity) const
{
    const ObjectState& object_state = GetNode().EvalWorldState(t);
    LightscapeLight* ls_light = TypeUtil::to_LightscapeLight(object_state.obj);
    if(ls_light != nullptr)
    {
        switch(ls_light->Type())
        {
        case LightscapeLight::TARGET_POINT_TYPE:
        case LightscapeLight::POINT_TYPE:
            // Point lights should be invisible
            return false;
        case LightscapeLight::TARGET_LINEAR_TYPE:
        case LightscapeLight::LINEAR_TYPE:
        case LightscapeLight::TARGET_AREA_TYPE:
        case LightscapeLight::AREA_TYPE:
        case LightscapeLight::TARGET_DISC_TYPE:
        case LightscapeLight::DISC_TYPE:
        case LightscapeLight::TARGET_SPHERE_TYPE:
        case LightscapeLight::SPHERE_TYPE:
        case LightscapeLight::TARGET_CYLINDER_TYPE:
        case LightscapeLight::CYLINDER_TYPE:
            {
                // Area lights can be visible. Look at custom attributes to know
                // Try to get the custom attribute from the light
                ICustAttribContainer* custAttribCont = ls_light->GetCustAttribContainer();
                if(custAttribCont != NULL) 
                {
                    int count = custAttribCont->GetNumCustAttribs();
                    for(int i = 0; i < count; ++i) {
                        CustAttrib* custAttrib = custAttribCont->GetCustAttrib(i);
                        if(custAttrib != NULL) {
                            LightscapeLight::AreaLightCustAttrib* lightCustAttrib = LightscapeLight::GetAreaLightCustAttrib(custAttrib);
                            if(lightCustAttrib != NULL) 
                            {
                                return !!lightCustAttrib->IsLightShapeRenderingEnabled(t, &validity);
                            }
                        }
                    }
                }
            }
            break;
        default:
            DbgAssert(false);
            break;
        }
    }

    return false;
}

}}	// namespace 
