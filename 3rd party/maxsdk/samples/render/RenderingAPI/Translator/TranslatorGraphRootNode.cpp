//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "TranslatorGraphRootNode.h"

// local includes
#include "ITranslationManager_Internal.h"
// max sdk
#include <assert1.h>
#include <RenderingAPI/Renderer/IRenderSessionContext.h>
#include <RenderingAPI/Renderer/IRenderingProcess.h>

#include "3dsmax_banned.h"

namespace Max
{;
namespace RenderingAPI
{;

//==================================================================================================
// class TranslatorGraphRootNode
//==================================================================================================

TranslatorGraphRootNode::TranslatorGraphRootNode(
    const TranslatorKey& translator_key, 
    ITranslationManager_Internal& translation_manager,
    IRenderSessionContext& render_session_context,
    const TimeValue initial_time)
	: TranslatorGraphNode(translator_key, translation_manager, render_session_context, initial_time),
	m_root_reference_count(0),
    m_invalidation_occured(false),
    m_client_abort_received(false)
{

}

TranslatorGraphRootNode::~TranslatorGraphRootNode()
{
	DbgAssert(m_root_reference_count == 0);
}

TranslationResult TranslatorGraphRootNode::update_scene_graph(const TimeValue t)
{
    // Use a loop to re-try updating the scene whenever an update is internally aborted by a change which happens during a call to the render
    // progress callback.
    // A re-translation may also be necessary if an invalidation occurs as a result of an output being set by
    // a call to TranslateKeyframe().
    TranslationResult update_result = TranslationResult::Failure;
    do
    {
        // Perform deferred validity check
        check_scene_validity(t);

        // Reset the invalidation flag, to catch any invalidations that might happen while we're translating the graph (typically caused
        // by the progress callback allowing windows message to be processed and thus scene changes to occur)
        m_invalidation_occured = false;

        // Translate/update the scene
        update_result = update_subgraph(t, *this);

        // Translate the scene at the keyframes that were requested by individual translators
        if(update_result == TranslationResult::Success)
        {
            // Accumulate the global set of keyframes requested by individual translators
            std::set<TimeValue> global_keyframes_list;
            get_keyframes_for_subgraph(global_keyframes_list);

            // Recursively translate the scene for each requested keyframe
            for(const TimeValue keyframe : global_keyframes_list)
            {
                update_result = translate_keyframe_for_subgraph(t, keyframe, *this);
                // Stop processing the keyframes on abort or failure
                if(update_result != TranslationResult::Success)
                {
                    break;
                }
            }
        }

        // If m_client_abort_received is true, then we requested an abort, and this abort must be propagated back to here (otherwise there's a bug)
        DbgAssert(!m_client_abort_received || (update_result == TranslationResult::Aborted));    

    } while(m_invalidation_occured && (update_result == TranslationResult::Aborted) && !m_client_abort_received);

    DbgAssert(m_client_abort_received == (update_result == TranslationResult::Aborted));    // all aborts must be propagated (otherwise there's a bug)

    return update_result;
}

void TranslatorGraphRootNode::check_scene_validity(const TimeValue t)
{
    check_subgraph_validity(t);
}

void TranslatorGraphRootNode::Invalidate(const bool defer_invalidation_check)
{
    m_invalidation_occured = true;
    __super::Invalidate(defer_invalidation_check);
}

void TranslatorGraphRootNode::SetTranslationProgressTitle(const MCHAR* title, bool& abort_immediately)
{
    abort_immediately = false;

    IRenderingProcess& rendering_process = GetRenderSessionContext().GetRenderingProcess();
    rendering_process.SetRenderingProgressTitle(title);
    if(rendering_process.HasAbortBeenRequested())
    {
        // User abort
        m_client_abort_received = true;
        abort_immediately = true;
        return;
    }

    // Updating the progress callback results in the max windows message queue to be processed, which could end up in changing the scene.
    // We keep track of any such changes in order to abort (and then restart) translation if a change happens, to avoid accessing a then-invalid scene
    // (e.g. if a node gets deleted).
    abort_immediately = m_invalidation_occured;
}

void TranslatorGraphRootNode::SetTranslationProgress(const size_t done, const size_t total, bool& abort_immediately)
{
    abort_immediately = false;

    IRenderingProcess& rendering_process = GetRenderSessionContext().GetRenderingProcess();
    rendering_process.SetRenderingProgress(done, total, IRenderingProcess::ProgressType::Translation);
    if(rendering_process.HasAbortBeenRequested())
    {
        // User abort
        m_client_abort_received = true;
        abort_immediately = true;
        return;
    }

    // Updating the progress callback results in the max windows message queue to be processed, which could end up in changing the scene.
    // We keep track of any such changes in order to abort (and then restart) translation if a change happens, to avoid accessing a then-invalid scene
    // (e.g. if a node gets deleted).
    abort_immediately = m_invalidation_occured;
}


}};	// namespace 
    