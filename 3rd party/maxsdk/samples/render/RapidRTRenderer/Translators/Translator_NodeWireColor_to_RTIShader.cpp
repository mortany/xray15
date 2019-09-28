//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "Translator_NodeWireColor_to_RTIShader.h"

// Local includes
#include "../resource.h"
// Max SDK
#include <inode.h>
#include <dllutilities.h>

namespace Max
{;
namespace RapidRTTranslator
{;

Translator_NodeWireColor_to_RTIShader::Translator_NodeWireColor_to_RTIShader(const Key& key, TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_INode(key, NotifierType_Node_Geom, translator_graph_node),
    BaseTranslator_to_RTIShader(translator_graph_node),
    Translator(translator_graph_node)
{

}

Translator_NodeWireColor_to_RTIShader::~Translator_NodeWireColor_to_RTIShader()
{

}

TranslationResult Translator_NodeWireColor_to_RTIShader::Translate(const TimeValue /*t*/, Interval& /*validity*/, ITranslationProgress& /*translation_progress*/, KeyframeList& /*keyframesNeeded*/)
{
	// Instantiate a simple diffuse material that uses the node's wire color
	const COLORREF node_color = GetNode().GetWireColor();

    const rti::ShaderHandle shader_handle = initialize_output_shader("ArchDesign");
	if(shader_handle.isValid())
    {
		rti::TEditPtr<rti::Shader> shader(shader_handle);
		set_shader_float3(*shader, "diffuse", Color(GetRValue(node_color), GetGValue(node_color), GetBValue(node_color)) / 255.0f);
        return TranslationResult::Success;
	}

	return TranslationResult::Failure;
}

bool Translator_NodeWireColor_to_RTIShader::CareAboutNotificationEvent(const MaxSDK::NotificationAPI::NodeEventType eventType) const
{
    // We only care about the node wire color
    switch(eventType)
    {
    case EventType_Node_WireColor:
        return true;
    default:
        return false;
    }
}

MSTR Translator_NodeWireColor_to_RTIShader::get_shader_name() const
{
    MSTR name;
    name.printf(MaxSDK::GetResourceStringAsMSTR(IDS_NODE_MATERIAL_NAME_FORMAT), GetNode().GetName());
    return name;
}

MSTR Translator_NodeWireColor_to_RTIShader::GetTimingCategory() const 
{
    return MSTR();
}

bool Translator_NodeWireColor_to_RTIShader::CareAboutMissingUVWChannels() const 
{
    // Not translating a mesh, so don't care
    return false;
}

std::vector<unsigned int> Translator_NodeWireColor_to_RTIShader::GetMeshUVWChannelIDs() const 
{
    // Don't care
    return std::vector<unsigned int>();
}

std::vector<MtlID> Translator_NodeWireColor_to_RTIShader::GetMeshMaterialIDs() const 
{
    return std::vector<MtlID>();
}



}}	// namespace 
