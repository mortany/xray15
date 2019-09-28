//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "TranslatorGraphNode.h"

// local
#include "../resource.h"
#include "ITranslationManager_Internal.h"
// max sdk
#include <assert1.h>
#include <dllutilities.h>
#include <RenderingAPI/Translator/TranslatorKey.h>
#include <RenderingAPI/Renderer/IRenderSessionContext.h>
#include <RenderingAPI/Renderer/IRenderingProcess.h>
#include <RenderingAPI/Renderer/IRenderingLogger.h>

#include "3dsmax_banned.h"

namespace Max
{;
namespace RenderingAPI
{;

//==================================================================================================
// class TranslatorGraphNode
//==================================================================================================

TranslatorGraphNode::TranslatorGraphNode(
    const TranslatorKey& translator_key, 
    ITranslationManager_Internal& translation_manager,
    IRenderSessionContext& render_session_context,
    const TimeValue initial_time)
    // Allocate the translator for this node
	: m_translation_manager(translation_manager),
	m_node_validity(NEVER),		// Initially invalid
    m_subgraph_validity(NEVER),
    m_deferred_invalidation(false),
    m_subgraph_deferred_invalidation(false),
	m_traversed(false),
    m_exclude_from_updates(false),
    m_last_translation_result(TranslationResult::Failure),
    m_render_session_context(render_session_context)
{
    // Allocate the translator managed by this graph node
    m_translator = std::unique_ptr<Translator>(translator_key.AllocateNewTranslator(*this, render_session_context, initial_time));

    // Initialize the category string used for reporting timing statistics
    if(m_translator != nullptr)
    {
        m_timing_category = m_translator->GetTimingCategory();
        m_timing_category = 
            MSTR(MaxSDK::GetResourceStringAsMSTR(IDS_TIMING_TRANSLATION_PREFIX))
            + (!m_timing_category.isNull() ? m_timing_category : MaxSDK::GetResourceStringAsMSTR(IDS_TIMING_MISC_TRANSLATION));
    }
}

TranslatorGraphNode::~TranslatorGraphNode()
{
    unlink_all_children();

    // Undo all parent dependencies (though it's wrong to delete a node that is not orphaned - hence we assert)
    DbgAssert(m_parent_nodes.empty());
    for(auto& parent : m_parent_nodes)
    {
#ifdef MAX_ASSERTS_ACTIVE
        const size_t num_erased_elements = 
#endif
            parent->m_child_nodes.erase(this);
        DbgAssert(num_erased_elements == 1);
    }
    m_parent_nodes.clear();
}

void TranslatorGraphNode::unlink_all_children()
{
	// Undo all child dependencies
	for(auto& child : m_child_nodes)
	{
#ifdef MAX_ASSERTS_ACTIVE
		const size_t num_erased_elements = 
#endif
			child->m_parent_nodes.erase(this);
		DbgAssert(num_erased_elements == 1);
	}
	m_child_nodes.clear();
}

TranslationResult TranslatorGraphNode::udpate_this_node(const TimeValue t, ITranslationProgress& translation_progress)
{
    // First check the validity interval, to avoid re-translating if the translated data is still valid the the current time.
	if(!m_node_validity.InInterval(t))
	{
		// Release all the children, to ensure that our list is perfectly up to date after we've re-translated. This allows now-unneeded children
		// to be garbage collected later.
		unlink_all_children();

		if(m_translator != NULL)
		{
            // Save the old outputs before we translate - just to make sure they stay valid for the entire duration of the call to Translator::Translate().
            // This may help avoid crashes with naive implementations of Translate().
            std::vector<std::shared_ptr<const ITranslatorOutput> > old_translator_outputs(m_translator_outputs);

			// Re-translate
            {
                // Time individual translation categories
                IRenderingProcess::NamedTimerGuard translation_timer_guard2(m_timing_category, m_render_session_context.GetRenderingProcess());

			    m_node_validity.SetInfinite();
                m_keyframes_needed.clear();
                m_deferred_invalidation = false;

                m_translator->PreTranslate(t, m_node_validity);
			    m_last_translation_result = m_translator->Translate(t, m_node_validity, translation_progress, m_keyframes_needed);
                m_translator->PostTranslate(t, m_node_validity);
            }

            // Share the references to all of the children's outputs. This is to make sure we don't get a dangling reference to child outputs, should
            // the children be updated before their parents are.
            m_saved_child_ouputs.clear();
            for(TranslatorGraphNode* child : m_child_nodes)
            {
                if(child != nullptr)
                {
                    m_saved_child_ouputs.insert(m_saved_child_ouputs.end(), child->m_translator_outputs.begin(), child->m_translator_outputs.end());
                }
            }

            switch(m_last_translation_result)
            {
            case TranslationResult::Success:
                // There are plugins which don't play nicely and output empty validity intervals. Force these to be valid for the current time.
                if(!m_node_validity.InInterval(t))
                {
                    DbgAssert(m_node_validity.Empty());     // The interval should be empty: if it contains something, but it's not the current time, there could be a bug.
                    m_node_validity.SetInstant(t);
                }
                break;
            case TranslationResult::Aborted:
                // Client aborted: make sure to re-translate next time
                m_node_validity.SetEmpty();
                m_keyframes_needed.clear();
                // Reset outputs to avoid half-translated stuff being preserved
                SetNumOutputs(0);
                break;
            default:
                DbgAssert(false);
                // Fall into...
            case TranslationResult::Failure:
                // Failed to translate. We do not output an error; it's the translator's job to output an error if it so wishes.
                // Set validity to current time. This avoids re-translating until the time change, at which point we'll try translating again just
                // in case the time is enough to make the translation succeed;
                m_node_validity.SetInstant(t);
                m_keyframes_needed.clear();
                // Reset outputs to account for the failure to translate, also forcing parents to re-translate
                // without this translator's outputs
                SetNumOutputs(0);
                break;
            }
		}
		else
		{
            // No translator is attached to this node, fail.
            // The validity to infinite to avoid failing continuously on this node.
			m_node_validity.SetInfinite();
			m_last_translation_result = TranslationResult::Failure;
		}
	}
	else
	{
        // No need for update: just return the result of the last translation
        DbgAssert(m_last_translation_result != TranslationResult::Aborted); // Aborts shouldn't be cached
	}

    DbgAssert(m_node_validity.InInterval(t) || (m_last_translation_result == TranslationResult::Aborted));
    return m_last_translation_result;
}
	
Translator* TranslatorGraphNode::get_translator() const
{
	return m_translator.get();
}

bool TranslatorGraphNode::is_ancestor(TranslatorGraphNode& node) const
{
    for(TranslatorGraphNode* parent : m_parent_nodes)
    {
        if((parent == &node) || parent->is_ancestor(node))
        {
            return true;
        }
    }
    
    return false;
}

TranslationResult TranslatorGraphNode::update_subgraph(const TimeValue t, ITranslationProgress& translation_progress)
{
    // Don't update nodes which are explicitly excluded from updates
    if(!m_exclude_from_updates)
    {
        // Update this subgraph as necessary
        if(!m_subgraph_validity.InInterval(t))
        {
            m_subgraph_validity.SetInfinite();
            m_subgraph_deferred_invalidation = false;

			// Perform a depth-first translation of every node in the graph.
            // This enables the translation of children to update their output before the parent gets updated. It may result in unnecessary child
            // translations if the parent were to no longer need the child, but doing otherwise might require double translations of parents if, after
            // updating the parent, it were found to require another update because the output of its children changed.
			for(TranslatorGraphNode* child : m_child_nodes)
			{
				const TranslationResult child_result = child->update_subgraph(t, translation_progress);
				if(child_result == TranslationResult::Failure)
				{
                    // Invalidate 'this' node to force it to take into account the now-failed child translator.
					m_node_validity.SetEmpty();
				}
				else if(child_result == TranslationResult::Aborted)
				{
                    m_subgraph_validity.SetEmpty();
					return TranslationResult::Aborted;
				}
			}

		    // Update 'this' node as needed
		    const TranslationResult result = udpate_this_node(t, translation_progress);

            // Build the subgraph validity from the children's
            m_subgraph_validity = m_node_validity;
            for(TranslatorGraphNode* child : m_child_nodes)
            {
                m_subgraph_validity &= child->m_subgraph_validity;
            }
            DbgAssert(m_subgraph_validity.InInterval(t) || (result == TranslationResult::Aborted));

            return result;
        }
        else
        {
            return m_last_translation_result;
        }
    }
    else
    {
        // Skipping excluded nodes is considered a success
        return TranslationResult::Success;
    }
}

void TranslatorGraphNode::check_subgraph_validity(const TimeValue t)
{
    // Don't update nodes which are explicitly excluded from updates
    if(!m_exclude_from_updates)
    {
        if(m_subgraph_deferred_invalidation)
        {
            m_subgraph_deferred_invalidation = false;

            // First, check children and descendants recursively
            for(TranslatorGraphNode* child : m_child_nodes)
            {
                child->check_subgraph_validity(t);
            }

            // Finally, check 'this'
            if(m_deferred_invalidation) 
            {
                m_deferred_invalidation = false;
                if(m_translator != nullptr)
                {
                    const Interval new_validity = m_translator->CheckValidity(t, m_node_validity);
                    if(new_validity != m_node_validity)
                    {
                        m_node_validity = new_validity;
                        // Propagate this node's validity up the graph
                        propagate_validity();
                    }
                }
            }
        }
    }
}

void TranslatorGraphNode::Invalidate(const bool defer_invalidation_check)
{
    // Do nothing if already invalid
    if(!m_node_validity.Empty())
    {
        if(!defer_invalidation_check)
        {
            // Invalidate immediately
	        m_node_validity.SetEmpty();
            // Deferred invalidation becomes irrelevant
            m_deferred_invalidation = false;

            // Propagate this node's validity up the graph
            propagate_validity();
        }
        else if(!m_deferred_invalidation)
        {
            // Defer the validity check
            m_deferred_invalidation = true;

            // Propagate this node's validity up the graph
            propagate_validity();
        }
    }
}

void TranslatorGraphNode::propagate_validity()
{
    propagate_subgraph_validity_recursive(m_node_validity, m_deferred_invalidation);
}

void TranslatorGraphNode::propagate_subgraph_validity_recursive(const Interval& validity, const bool deferred_invalidation)
{
    // Propagate only if necessary
    if(!validity.IsSubset(m_subgraph_validity) || (deferred_invalidation && !m_subgraph_deferred_invalidation))
    {
        m_subgraph_validity &= validity;
        m_subgraph_deferred_invalidation |= deferred_invalidation;

        for(TranslatorGraphNode* parent : m_parent_nodes)
        {
            parent->propagate_subgraph_validity_recursive(validity, deferred_invalidation);
        }
    }
}

void TranslatorGraphNode::reset_subgraph_traversal_flag() const
{
	m_traversed = false;

	for(const auto& child : m_child_nodes)
	{
		child->reset_subgraph_traversal_flag();
	}
}

void TranslatorGraphNode::get_keyframes_for_subgraph(std::set<TimeValue>& keyframes) const
{
    // Add this node's keyframes
    keyframes.insert(m_keyframes_needed.begin(), m_keyframes_needed.end());

    for(TranslatorGraphNode* child : m_child_nodes)
    {
        if(child != nullptr)
        {
            child->get_keyframes_for_subgraph(keyframes);
        }
    }
}

TranslationResult TranslatorGraphNode::translate_keyframe_for_subgraph(const TimeValue frame_time, const TimeValue keyframe_time, ITranslationProgress& translation_progress)
{
    // Translate the keyframe for this node, if applicable
    if((m_translator != nullptr) && !m_keyframes_needed.empty())
    {
        // Note that, as an optimization, we only compare the first keyframe in the list. This scheme assumes the
        // keyframes are processed in ascending order
        const TimeValue first_keyframe_needed = *(m_keyframes_needed.begin());
        DbgAssert(keyframe_time <= first_keyframe_needed);   // Sanity check that no keyframes were skipped
        if(first_keyframe_needed == keyframe_time)
        {
            // Time individual the translation of this keyframe
            IRenderingProcess::NamedTimerGuard translation_timer_guard2(m_timing_category, m_render_session_context.GetRenderingProcess());

            // Translate the keyframe, disabling the acquisition of child translators in the process, as that
            // is disallowed by design.
            m_child_acquisition_disabled = true;
            m_last_translation_result = m_translator->TranslateKeyframe(frame_time, keyframe_time, translation_progress);
            m_child_acquisition_disabled = false;

            switch(m_last_translation_result)
            {
            case TranslationResult::Success:
                // Remove the keyframe from the list
                m_keyframes_needed.erase(m_keyframes_needed.begin());
                break;
            case TranslationResult::Aborted:
                // Client aborted: make sure to re-translate next time
                m_node_validity.SetEmpty();
                m_keyframes_needed.clear();
                // Reset outputs to avoid half-translated stuff being preserved
                SetNumOutputs(0);
                return m_last_translation_result;
            default:
                DbgAssert(false);
                // Fall into...
            case TranslationResult::Failure:
                // Set validity to current time. This avoids re-translating until the time change, at which point we'll try translating again just
                // in case the time is enough to make the translation succeed;
                m_node_validity.SetInstant(frame_time);
                m_keyframes_needed.clear();
                // Reset outputs to account for the failure to translate, also forcing parents to re-translate
                // without this translator's outputs
                SetNumOutputs(0);
                break;
            }
        }
    }

    // Recursively process descendant nodes
    for(TranslatorGraphNode* child : m_child_nodes)
    {
        if(child != nullptr)
        {
            const TranslationResult result = child->translate_keyframe_for_subgraph(frame_time, keyframe_time, translation_progress);
            if(result == TranslationResult::Aborted)
            {
                // Immediately interrupt the process if an abort was requested
                return result;
            }
        }
    }

    return TranslationResult::Success;
}

size_t TranslatorGraphNode::GetNumOutputs() const
{
    return m_translator_outputs.size();
}

std::shared_ptr<const ITranslatorOutput> TranslatorGraphNode::GetOutputInternal(const size_t index) const
{
    if(index < m_translator_outputs.size())
    {
        return m_translator_outputs[index];
    }
    else
    {
        // Return empty pointer (nullptr)
        return std::shared_ptr<const ITranslatorOutput>();
    }
}

void TranslatorGraphNode::SetNumOutputs(const size_t num)
{
    if(num != m_translator_outputs.size())
    {
        m_translator_outputs.resize(num);

        // Invalidate parents as the outputs (to which the parents most likely depend) have changed
        invalidate_parents();
    }
}

void TranslatorGraphNode::SetOutput(const size_t index, std::shared_ptr<const ITranslatorOutput> output)
{
	// Ignore the call if the output is the same (especially likely if setting an output to null
	// while it's already null)
	if(output != GetOutputInternal(index))
	{
		// Resize the array if needed
		if(index >= m_translator_outputs.size())
		{
			if(output != nullptr)
			{
				m_translator_outputs.resize(index + 1);
			}
			else
			{
				// Output is null: don't bother resizing the array, as any output index that's out of bounds is considered null already.
				return;
			}
		}

		m_translator_outputs[index] = output;

		// Invalidate parents as the outputs (to which the parents most likely depend) have changed
		invalidate_parents();
	}
}

void TranslatorGraphNode::invalidate_parents() const
{
    for(TranslatorGraphNode* parent : m_parent_nodes)
    {
        if(parent != nullptr)
        {
            parent->Invalidate(false);
        }
    }
}

void TranslatorGraphNode::TranslatedObjectDeleted()
{
    // Clear the outputs
    SetNumOutputs(0);

    // Flag this node to exclude it from translation
    m_exclude_from_updates = true;

    // Also invalidate, since outputs have been deleted
    Invalidate(false);
}

void TranslatorGraphNode::AccumulateGraphStatistics(TranslatorStatistics& statistics) const
{
    reset_subgraph_traversal_flag();
    return accumulate_graph_statistics_recursive(statistics);
}

void TranslatorGraphNode::accumulate_graph_statistics_recursive(TranslatorStatistics& statistics) const
{
    // Traversed the subgraph, if it hasn't been done already.
    if(!m_traversed)
    {
        // Set the traversed flag to avoid processing this node more than once
        m_traversed = true;

        if(m_translator != nullptr)
        {
            m_translator->AccumulateStatistics(statistics);
        }

        for(const auto& child : m_child_nodes)
        {
            child->accumulate_graph_statistics_recursive(statistics);
        }
    }
}

const Translator* TranslatorGraphNode::AcquireChildTranslatorInternal(const TranslatorKey& key, const TimeValue t, ITranslationProgress& translation_progress, TranslationResult& result)
{
    if(!m_child_acquisition_disabled)
    {
        // Pause the timer for this translator
        IRenderingProcess::NamedTimerGuard translation_timer_guard2(m_timing_category, m_render_session_context.GetRenderingProcess(), true);

        TranslatorGraphNode& child_node = m_translation_manager.acquire_graph_node(key, t);

        // Upon being re-acquired, nodes are automatically no longer excluded from scene updates
        child_node.m_exclude_from_updates = false;

        // Translate/update the child translator and its entire sub-graph 
        result = child_node.update_subgraph(t, translation_progress);

        // If translation of child was successful, establish the parent->child relationship. Otherwise, ignore the child (let it be garbage collected later).
        if(result == TranslationResult::Success)
        {
            DbgAssert((&child_node != this) && !is_ancestor(child_node));
            // Add child to this.
            m_child_nodes.insert(&child_node);
            // Add parent to child.
            child_node.m_parent_nodes.insert(this);

            return child_node.get_translator();
        }
        else
        {
            return nullptr;
        }
    }
    else
    {
        // Programmer error: tried acquiring a dependency while it's not allowed. Output an error for the programmer's
        // sake.
        DbgAssert(false);
        m_render_session_context.GetLogger().LogMessage(IRenderingLogger::MessageType::Error, MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_ACQUIREWHILEKEYFRAME));
        return nullptr;
    }
}

}};	// namespace 
    