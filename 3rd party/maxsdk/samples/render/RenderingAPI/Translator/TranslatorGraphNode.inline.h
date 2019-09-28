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

namespace Max
{;
namespace RenderingAPI
{;

//=================================================================================================
// class TranslatorGraphNode
//=================================================================================================

inline bool TranslatorGraphNode::can_be_garbage_collected() const
{
    return m_parent_nodes.empty();
}

inline const Interval& TranslatorGraphNode::get_subgraph_validity() const
{
    return m_subgraph_validity;
}

inline bool TranslatorGraphNode::get_subgraph_maybe_invalid() const
{
    return m_subgraph_deferred_invalidation;
}

inline IRenderSessionContext& TranslatorGraphNode::GetRenderSessionContext()
{
    return m_render_session_context;
}

inline const IRenderSessionContext& TranslatorGraphNode::GetRenderSessionContext() const
{
    return m_render_session_context;
}

}};	// namespace 
