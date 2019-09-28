//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "SceneContainer_Interactive.h"

// max sdk
#include <RenderingAPI/Renderer/IRenderSettingsContainer.h>

#include "3dsmax_banned.h"

namespace Max
{;
namespace RenderingAPI
{;

SceneContainer_Interactive::SceneContainer_Interactive(
    IImmediateInteractiveRenderingClient* notification_client, 
    IRenderSettingsContainer& render_settings_container, 
    INode* const root_node)

    : SceneContainer_Base(render_settings_container, notification_client)
{
    set_scene_root_node(root_node);
}

SceneContainer_Interactive::~SceneContainer_Interactive()
{

}

}}	// namespace 

