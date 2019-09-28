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

// local includes
#include "TranslatorGraphNode.h"
// max sdk
#include <RenderingAPI/Translator/Translator.h>
#include <RenderingAPI/Translator/ITranslationProgress.h>
#include <Noncopyable.h>
// std includes
#include <set>
#include <vector>

namespace Max
{;
namespace RenderingAPI
{;

class ITranslationManager_Internal;

// The root node adds a reference count, since we can't garbage collect it based on it being an orphan (as it never has parents)
class TranslatorGraphRootNode : 
    public TranslatorGraphNode,
    // The root implements the progress callback interface to monitor potential scene changes caused by processing window messages in the
    // render progress callback.
    public ITranslationProgress
{
public:

	// See comments for constructor of base class for details.
	TranslatorGraphRootNode(
        const TranslatorKey& translator_key, 
        ITranslationManager_Internal& translation_manager, 
        IRenderSessionContext& render_session_context,
        const TimeValue initial_time);
    virtual ~TranslatorGraphRootNode();

	// Walks the entire graph under this node, re-translating every invalid node.
	// Returns false iff 'this' node failed to translate (return value doesn't directly take into account failures of descendant nodes).
	// The translation context is passed in order to check for abort during the traversal.
	TranslationResult update_scene_graph(const TimeValue t);

    // Returns whether the scene graph might be invalid, at which point check_scene_validity() needs to be called to verify whether the scene
    // is actually invalid.
    bool is_scene_maybe_invalid() const;
    // Checks the scene for validity, evaluating any deferred validity checks required by calls to Invalidate(true).
    // Only to be called from the main thread.
    void check_scene_validity(const TimeValue t);

    // Returns whether the scene is valid at the given time.
    // The interval is build when translating/updating the graph, making this call cheap.
    bool is_scene_valid(const TimeValue t) const;

	// Both return the *new* reference count.
	int increment_reference_count();
	int decrement_reference_count();

    // -- inherited from TranslatorGraphNode
    virtual void Invalidate(const bool defer_invalidation_check) override;
    virtual bool can_be_garbage_collected() const;

    // -- inherited from ITranslationProgress
    virtual void SetTranslationProgressTitle(const MCHAR* title, bool& abort_immediately) override;
    virtual void SetTranslationProgress(const size_t done, const size_t total, bool& abort_immediately) override;

private:

    // The reference count for this root graph node
	int m_root_reference_count;

    // Used to record an invalidation which occurs during a progress update
    bool m_invalidation_occured;
    // Records when a client abort has caused the translation to be aborted
    bool m_client_abort_received;
};


}};	// namespace 

#include "TranslatorGraphRootNode.inline.h"