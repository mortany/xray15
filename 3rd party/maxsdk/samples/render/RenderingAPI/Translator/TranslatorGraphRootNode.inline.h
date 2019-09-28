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
// class TranslatorGraphRootNode
//=================================================================================================

inline int TranslatorGraphRootNode::increment_reference_count()
{
    return ++m_root_reference_count;
}

inline int TranslatorGraphRootNode::decrement_reference_count()
{
    DbgAssert(m_root_reference_count > 0);
    return --m_root_reference_count;
}

inline bool TranslatorGraphRootNode::can_be_garbage_collected() const
{
    return (m_root_reference_count == 0);
}

inline bool TranslatorGraphRootNode::is_scene_valid(const TimeValue t) const
{
    // Check the validity of the entire scene graph
    return !!get_subgraph_validity().InInterval(t);
}

inline bool TranslatorGraphRootNode::is_scene_maybe_invalid() const
{
    return get_subgraph_maybe_invalid();
}


}};	// namespace 
