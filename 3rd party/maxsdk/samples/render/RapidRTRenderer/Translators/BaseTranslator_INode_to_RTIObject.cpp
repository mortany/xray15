//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "BaseTranslator_INode_to_RTIObject.h"

// local includes
#include "../resource.h"
#include "../Util.h"
#include "../RapidRenderSettingsWrapper.h"
#include "Translator_LightscaleLightINode_to_RTIObject.h"
#include "Translator_GeomObjectINode_to_RTIObject.h"
#include "TypeUtil.h"
// max sdk includes
#include <inode.h>
#include <iparamb2.h>
#include <dllutilities.h>
// IAmgInterface.h is located in sdk samples folder, such that it may be shared with sdk samples plugins, without actually being exposed in the SDK.
#include <../samples/render/AmgTranslator/IAmgInterface.h>
#include <RenderingAPI/Renderer/IRenderSettingsContainer.h>
#include <RenderingAPI/Renderer/IRenderingLogger.h>

using namespace MaxSDK;

namespace Max
{;
namespace RapidRTTranslator
{;

BaseTranslator_INode_to_RTIObject::BaseTranslator_INode_to_RTIObject(
    const Key& key, 
    const NotifierType notifier_node_type,
    TranslatorGraphNode& translator_graph_node)

	: BaseTranslator_to_RTIObject(translator_graph_node),
    BaseTranslator_INode(key, notifier_node_type, translator_graph_node),
    Translator(translator_graph_node)
{

}

BaseTranslator_INode_to_RTIObject::~BaseTranslator_INode_to_RTIObject()
{

}

void BaseTranslator_INode_to_RTIObject::get_transform_matrices(const TimeValue t, Interval& validity, Matrix3& transform, Matrix3& motion_transform) const 
{
    // Evaluate the transform
    const MotionTransforms transforms = EvaluateTransforms(t, validity);
    transform = transforms.shutter_open;
    motion_transform = transforms.shutter_close;
}

bool BaseTranslator_INode_to_RTIObject::is_caustic_generator(const TimeValue /*t*/, Interval& /*validity*/)
{
    return RapidRenderSettingsWrapper(GetRenderSessionContext().GetRenderSettings()).get_enable_caustics();
}

bool BaseTranslator_INode_to_RTIObject::is_visible_to_primary_rays(const TimeValue /*t*/, Interval& /*validity*/)
{
    return (GetNode().GetPrimaryVisibility() != 0);
}

Interval BaseTranslator_INode_to_RTIObject::CheckValidity(const TimeValue t, const Interval& previous_validity) const
{
    return BaseTranslator_INode::CheckValidity(t, previous_validity);
}

Mtl* BaseTranslator_INode_to_RTIObject::ResolveMaterial(Mtl* const mtl) const
{
    // Enable AMG to support wrapper materials directly, e.g. scripted materials.
    if((mtl != nullptr) && IAMGInterface::IsSupported(_T("RapidRT"), mtl->SuperClassID(), mtl->ClassID()))
    {
        return mtl;
    }
    else
    {
        return __super::ResolveMaterial(mtl);
    }
}

bool BaseTranslator_INode_to_RTIObject::CareAboutRenderProperty(const PartID render_property_id) const
{
    switch(render_property_id)
    {
    case PART_REND_PROP_PRIMARY_INVISIBILITY:
        return true;
    default:
        return false;
    }
}

bool BaseTranslator_INode_to_RTIObject::CareAboutDisplayProperty(const PartID /*display_property_id*/) const
{
    return false;
}

bool BaseTranslator_INode_to_RTIObject::CareAboutGIProperty(const PartID /*gi_property_id*/) const
{
    return false;
}

//==================================================================================================
// class BaseTranslator_INode_to_RTIObject::Allocator
//==================================================================================================

std::unique_ptr<Translator> BaseTranslator_INode_to_RTIObject::Allocator::operator()(const Key& key, TranslatorGraphNode& translator_graph_node, const IRenderSessionContext& render_session_context, const TimeValue initial_time) const
{
    // Find the appropriate translator for this node
    if(Translator_LightscaleLightINode_to_RTIObject::can_translate(key, initial_time))
    {
        return std::unique_ptr<Translator>(new Translator_LightscaleLightINode_to_RTIObject(key, translator_graph_node));
    }
    else if(Translator_GeomObjectINode_to_RTIObject::can_translate(key, initial_time))
    {
        return std::unique_ptr<Translator>(new Translator_GeomObjectINode_to_RTIObject(key, translator_graph_node));
    }
    else
    {
        // Unsupported object: output an appropriate error message
        INode& node = key;
        Object* object = node.EvalWorldState(initial_time).obj;
        if(object != nullptr)
        {
            LightObject* const lightObject = TypeUtil::to_LightObject(object);
            const TSTR class_name = object->ClassName();

            TSTR error_message;
            error_message.printf(MaxSDK::GetResourceStringAsMSTR(
                (lightObject != nullptr) ? IDS_UNSUPPORTED_LIGHT_NODE : IDS_UNSUPPORTED_GENERIC_NODE), node.GetName(), (const MCHAR*)class_name);
            render_session_context.GetLogger().LogMessage(IRenderingLogger::MessageType::Error, error_message);
        }

        return nullptr;
    }
}

}}	// namespace 
