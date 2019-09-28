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

#include "SceneContainer_Base.h"

namespace Max
{;
namespace RenderingAPI
{;

// The interactive (active shade) version of ISceneContainer
class SceneContainer_Interactive :
    public SceneContainer_Base
{
public:

	SceneContainer_Interactive(
        IImmediateInteractiveRenderingClient* notification_client, 
        IRenderSettingsContainer& render_settings_container, 
        INode* const root_node);
    ~SceneContainer_Interactive();

};

}}	// namespace 

