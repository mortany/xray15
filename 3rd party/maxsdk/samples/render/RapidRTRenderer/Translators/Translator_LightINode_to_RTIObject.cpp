//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "Translator_LightINode_to_RTIObject.h"

// local
#include "../resource.h"
// Max SDK
#include <inode.h>
#include <dllutilities.h>
#include <RenderingAPI/Translator/TranslatorStatistics.h>

namespace Max
{;
namespace RapidRTTranslator
{;

Translator_LightINode_to_RTIObject::Translator_LightINode_to_RTIObject(
    const Key& key, 
    TranslatorGraphNode& translator_graph_node)

    : BaseTranslator_INode_to_RTIObject(key, MaxSDK::NotificationAPI::NotifierType_Node_Light, translator_graph_node),
    Translator(translator_graph_node)
{

}

Translator_LightINode_to_RTIObject::~Translator_LightINode_to_RTIObject()
{

}

void Translator_LightINode_to_RTIObject::AccumulateStatistics(TranslatorStatistics& stats) const
{
    stats.AddLights(get_num_output_objects());
}

bool Translator_LightINode_to_RTIObject::is_visible_to_primary_rays(const TimeValue t, Interval& validity)
{
    return is_light_visible(t, validity);
}

bool Translator_LightINode_to_RTIObject::CareAboutNotificationEvent(const MaxSDK::NotificationAPI::NodeEventType eventType) const
{
    switch(eventType)
    {
    case EventType_Node_ParamBlock:
    case EventType_Node_Uncategorized:
        // We need to invalidate on parameter changes (param block or non-param-block), even if they don't affect the light instance. 
        // When the light's distribution is changed, the render isn't correct until the object is re-translated. I suspect that only an 
        // update to the object triggers an update to the light acceleration structures.
        return  true;
    default:
        return __super::CareAboutNotificationEvent(eventType);
    }
}

MSTR Translator_LightINode_to_RTIObject::GetTimingCategory() const 
{
    return MaxSDK::GetResourceStringAsMSTR(IDS_TIMING_LIGHTS);
}

bool Translator_LightINode_to_RTIObject::CareAboutMissingUVWChannels() const
{
    // Don't care about missing UVW channels, as this is a light and doesn't have them.
    return false;
}

std::vector<unsigned int> Translator_LightINode_to_RTIObject::GetMeshUVWChannelIDs() const
{
    return std::vector<unsigned int>();
}

std::vector<MtlID> Translator_LightINode_to_RTIObject::GetMeshMaterialIDs() const
{
    return std::vector<MtlID>();
}


}}	// namespace 
