//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////

#include "Translator_SceneRoot_to_RTIGroup.h"

// local includes
#include "BaseTranslator_INode_to_RTIObject.h"
#include "../resource.h"
#include "TypeUtil.h"
// max sdk
#include <inode.h>
#include <object.h>
#include <dllutilities.h>
#include <iparamb2.h>
#include <DaylightSimulation/ISunPositioner.h>
#include <RenderingAPI/Translator/ITranslationManager.h>
#include <RenderingAPI/Translator/ITranslationProgress.h>
#include <RenderingAPI/Renderer/ISceneContainer.h>
#include <RenderingAPI/Renderer/IRenderingLogger.h>

namespace Max
{;
namespace RapidRTTranslator
{;

namespace
{
    // Returns whether the given node should be ignored by RapidRT when traversing the scene
    bool should_ignore_node(INode& node, const TimeValue tranlsation_time)
    {
        Object* const object = node.EvalWorldState(tranlsation_time).obj;
        LightObject* lightObject = TypeUtil::to_LightObject(object);
        if(lightObject != nullptr)
        {
            if((lightObject->ClassID() == MR_PHYSSUN_CLASS_ID) || (lightObject->ClassID() == MRPHYSSKY_LIGHT_OBJECT_CLASSID))
            {
                // Ignore mr sun & sky objects - we don't care about them, we use the mr physical sky env instead.
                return true;
            }
            else if(dynamic_cast<MaxSDK::ISunPositioner*>(lightObject) != nullptr)
            {
                // Ignore: Sun positioner doesn't need translation, it doesn't do anything by itself
                return true;
            }
            else if(lightObject->ClassID() == Class_ID(0x7bf61478, 0x522e4705)) // Textured skylight
            {
                // Special case: Check if it's a standard skylight in "environment" mode
                if(lightObject->GetParamBlockByID(0)->GetInt(7, tranlsation_time) == 0)
                {
                    return true;
                }
            }
        }

        return false;
    }
}

Translator_SceneRoot_to_RTIGroup::Translator_SceneRoot_to_RTIGroup(const Key& /*key*/, TranslatorGraphNode& translator_graph_node)
	: BaseTranslator_to_RTI(translator_graph_node),
    Translator(translator_graph_node)
{
    // Monitor interactive changes in the scene
    GetRenderSessionContext().GetScene().RegisterChangeNotifier(*this);
}

Translator_SceneRoot_to_RTIGroup::~Translator_SceneRoot_to_RTIGroup()
{
    // Stop monitoring interactive changes
    GetRenderSessionContext().GetScene().UnregisterChangeNotifier(*this);
}

TranslationResult Translator_SceneRoot_to_RTIGroup::Translate(const TimeValue t, Interval& validity, ITranslationProgress& translation_progress, KeyframeList& /*keyframesNeeded*/)
{
    const ISceneContainer& scene_container = GetRenderSessionContext().GetScene();
    const std::vector<INode*> geom_nodes = scene_container.GetGeometricNodes(t, validity);
    const std::vector<INode*> light_nodes = scene_container.GetLightNodes(t, validity);

    // Unify all nodes we care about in a single list, for simplicity
    std::vector<INode*> nodes_to_translate; 
    nodes_to_translate.reserve(geom_nodes.size() + light_nodes.size());
    nodes_to_translate.insert(nodes_to_translate.end(), geom_nodes.begin(), geom_nodes.end());
    nodes_to_translate.insert(nodes_to_translate.end(), light_nodes.begin(), light_nodes.end());

    // Initialize progress
    {
        bool abort_immediately = false;
        translation_progress.SetTranslationProgress(0, nodes_to_translate.size(), abort_immediately);
        if(abort_immediately)
        {
            return TranslationResult::Aborted;
        }
    }
        
    // This is the list of all acquired child entity handles
    std::vector<rti::EntityHandle> child_entity_handles;
    child_entity_handles.reserve(nodes_to_translate.size() * 2);    // Generously reserve enough space for most cases

    // Process the scene nodes individually
	size_t num_nodes_translated = 0;
    TSTR progress_string;
    for(INode* const node : nodes_to_translate)
    {
        if((node != nullptr) && !should_ignore_node(*node, t))
        {
            progress_string.printf(MaxSDK::GetResourceStringAsMSTR(IDS_RENDER_TRANSLATING_NODE), static_cast<int>(num_nodes_translated+1), static_cast<int>(nodes_to_translate.size()), node->GetName());
            {
                bool abort_immediately = false;
                translation_progress.SetTranslationProgressTitle(progress_string, abort_immediately);  
                if(abort_immediately)
                {
                    return TranslationResult::Aborted;
                }
            }

            TranslationResult child_translation_result;
            const BaseTranslator_INode_to_RTIObject* child_translator = AcquireChildTranslator<BaseTranslator_INode_to_RTIObject>(BaseTranslator_INode_to_RTIObject::Key(*node), t, translation_progress, child_translation_result);
            if(child_translator != nullptr)
            {
                // Add the child translator's objects to the list
                child_translator->append_output_objects(child_entity_handles);
            }
            else if(child_translation_result == TranslationResult::Aborted)
            {
                // Abort the traversal if requested
                return TranslationResult::Aborted;
            }
            else
            {
                // Output warning, but continue traversal - one node failure shouldn't stop rendering
                TSTR msg;
                msg.printf(MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_INODE_TRANSLATION_FAILED), node->GetName());
                // This is just a warning - if an error is to be reported, it'll be reported by the node's translator.
                // (we don't want to report an error if the node is explicitly ignored).
                GetRenderSessionContext().GetLogger().LogMessage(IRenderingLogger::MessageType::Warning, msg);
            }
        }

        // Report progress from translating the scene
        {
            bool abort_immediately = false;
            translation_progress.SetTranslationProgress(++num_nodes_translated, nodes_to_translate.size(), abort_immediately);
            if(abort_immediately)
            {
                return TranslationResult::Aborted;
            }
        }
    }

    // Reset the progress title to avoid it sticking around and providing false information
    {
        bool abort_immediately = false;
        translation_progress.SetTranslationProgressTitle(_T(""), abort_immediately);
        if(abort_immediately)
        {
            return TranslationResult::Aborted;
        }
    }

    // Setup the group object
    rti::GroupHandle group_handle = initialize_output_handle<rti::GroupHandle>(0);
    if(group_handle.isValid())
    {
        rti::TEditPtr<rti::Group> group(group_handle);
        group->removeAllChildren();
        for(const rti::EntityHandle& object_handle : child_entity_handles)
        {
            group->addChild(object_handle);
        }
    }

    // Validity is forever as the list of scene nodes isn't animated (and I don't believe that any of the parameters which might affect the inclusion
    // of a node, such as "renderable", can be animated).
    validity = FOREVER;
    return TranslationResult::Success;
}


rti::GroupHandle Translator_SceneRoot_to_RTIGroup::get_output_group() const
{
    return get_output_handle<rti::GroupHandle>(0);
}

void Translator_SceneRoot_to_RTIGroup::NotifySceneNodesChanged()
{
    Invalidate();
}

void Translator_SceneRoot_to_RTIGroup::NotifySceneBoundingBoxChanged()
{
    // Don't care
}

Interval Translator_SceneRoot_to_RTIGroup::CheckValidity(const TimeValue /*t*/, const Interval& previous_validity) const
{
    return previous_validity;
}

void Translator_SceneRoot_to_RTIGroup::PreTranslate(const TimeValue /*translationTime*/, Interval& /*validity*/) 
{
    // Do nothing
}

void Translator_SceneRoot_to_RTIGroup::PostTranslate(const TimeValue /*translationTime*/, Interval& /*validity*/) 
{
    // Do nothing
}

MSTR Translator_SceneRoot_to_RTIGroup::GetTimingCategory() const 
{
    return MaxSDK::GetResourceStringAsMSTR(IDS_TIMING_SCENE);
}

}}	// namespace 