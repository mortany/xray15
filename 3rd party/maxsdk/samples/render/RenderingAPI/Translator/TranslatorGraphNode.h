//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#pragma once

// max sdk
#include <RenderingAPI/Translator/Translator.h>

#include <Noncopyable.h>

#include <set>
#include <vector>

namespace Max
{;
namespace RenderingAPI
{;

using namespace MaxSDK;
using namespace MaxSDK::RenderingAPI;

class ITranslationManager_Internal;

class TranslatorGraphNode :
	public MaxSDK::Util::Noncopyable
{
public:

	// This constructor will use the factory to allocate a translator based on the given key.
	// Note that, if the factory returns a NULL translator, then this node will store that NULL translator.
	TranslatorGraphNode(
        const TranslatorKey& translator_key, 
        ITranslationManager_Internal& translation_manager,
        IRenderSessionContext& render_session_context,
        const TimeValue initial_time);
    virtual ~TranslatorGraphNode();

	// Returns the translator stored in this node, which may be NULL if the translator factory returned NULL.
	Translator* get_translator() const;

    // Returns true if the node is no longer being referenced in the scene DAG, and may therefore be garbage collected
    virtual bool can_be_garbage_collected() const;

    // Implementation of methods forwarded by Translator
    size_t GetNumOutputs() const;
    std::shared_ptr<const ITranslatorOutput> GetOutputInternal(const size_t index) const;
    void SetNumOutputs(const size_t num);
    void SetOutput(const size_t index, std::shared_ptr<const ITranslatorOutput> output);
    const Translator* AcquireChildTranslatorInternal(const TranslatorKey& key, const TimeValue t, ITranslationProgress& translation_progress, TranslationResult& result);
    void TranslatedObjectDeleted();
    void AccumulateGraphStatistics(TranslatorStatistics& statistics) const;
    virtual void Invalidate(const bool defer_invalidation_check);
    IRenderSessionContext& GetRenderSessionContext();
    const IRenderSessionContext& GetRenderSessionContext() const;

    // Resets the validity interval of all parent nodes
    void invalidate_parents() const;

protected:

    // Recursively updates this node and every descendant (children of children, etc.), recursively.
	TranslationResult update_subgraph(const TimeValue t, ITranslationProgress& translation_progress);

    // Recursively checks the subgraph for validity (for cause of m_maybe_invalid being true), invalidating
    // nodes that are deemed invalid.
    void check_subgraph_validity(const TimeValue t);

	// Removes all child dependencies
	void unlink_all_children();

	// Recursively resets m_traversed on this node and its children.
	void reset_subgraph_traversal_flag() const;

    const Interval& get_subgraph_validity() const;
    bool get_subgraph_maybe_invalid() const;

    // Recursively compiles the set of all keyframes at which individual translators need to be translated
    void get_keyframes_for_subgraph(std::set<TimeValue>& keyframes) const;
    // Recursively traverses the subgraph, translating individual translators at the given keyframe if and only
    // if they need to be translated at that keyframe.
    TranslationResult translate_keyframe_for_subgraph(
        // Time of the current animation frame
        const TimeValue frame_time, 
        // Time of the keyframe being translated (multiple keyframes per animation frame)
        const TimeValue keyframe_time, 
        ITranslationProgress& translation_progress);

private:

    void accumulate_graph_statistics_recursive(TranslatorStatistics& statistics) const;

    // Returns whether the given node is an ancestor in the graph
    bool is_ancestor(TranslatorGraphNode& node) const;

    // Translates/updates this node, as necessary based on the validity interval.
    TranslationResult udpate_this_node(const TimeValue t, ITranslationProgress& translation_progress);

    // Propagates a node's validity such that the m_subgraph_validity of every ancestor gets updated accordingly.
    void propagate_validity();
    void propagate_subgraph_validity_recursive(const Interval& validity, const bool deferred_invalidation);

private:

	// The translator associated with this node. May be NULL, if we failed to create the translator.
	std::unique_ptr<Translator> m_translator;

    ITranslationManager_Internal& m_translation_manager;

	// The validity interval associated to the data last translated by the translator.
	Interval m_node_validity;
    // The validity interval of the subgraph for which this node is the root.
    Interval m_subgraph_validity;

    // The set of keyframes at which the translator needs to be translated, following the call to Translate()
    Translator::KeyframeList m_keyframes_needed;

    // Flags that Invalidate(true) was called; that CheckValidity() needs to be called to verify whether the translator is invalid.
    bool m_deferred_invalidation;
    bool m_subgraph_deferred_invalidation;

	// Flags the last translation as having failed. This is used to avoid re-attempting a failed translation over and over. Failed
	// translations cause the validity interval to be set to FOREVER, allowing a re-translation iff the translator is invalidated.
	TranslationResult m_last_translation_result;

	// Child nodes in the DAG.
    // Guaranteed to never have a null entry.
	std::set<TranslatorGraphNode*> m_child_nodes;

	// Parent nodes in the DAG. Also stores a boolean which flags parents as "strong dependencies", which implies that an invalidation of a child
	// also invalidates the parent.
    // Guaranteed to never have a null entry.
	std::set<TranslatorGraphNode*> m_parent_nodes;

	// Flag used when traversing a graph of nodes; avoids traversing the same node multiple times.
	mutable bool m_traversed;
    // Flag used to disable the acquisition of child translators when translating keyframes
    bool m_child_acquisition_disabled = false;

    // The outputs produced by the last translation
    std::vector<std::shared_ptr<const ITranslatorOutput> > m_translator_outputs;
    
    // The outputs of the children, saved as a safety measure to avoid dangling references to child outputs if the children were to be updated
    // before the parent was (resulting in a possible release of old outputs to which the parent might still reference)
    std::vector<std::shared_ptr<const ITranslatorOutput> > m_saved_child_ouputs;

    // This flags the node as being excluded from scene udpates until it's been acquired again by a parent. This is used to flag nodes for which
    // the translated object has been deleted, signaling that updating them may result in accessing dangling pointers.
    bool m_exclude_from_updates;

    // The context in which this translation is to happen. This, of course, means that we can't share translators between render sessions - something
    // which may eventually be desired (to share memory between multiple views), but not conceivable in the foreseeable future.
    IRenderSessionContext& m_render_session_context;

    // The string used to categorize timing statistics
    MSTR m_timing_category;
};

}};	// namespace 

#include "TranslatorGraphNode.inline.h"