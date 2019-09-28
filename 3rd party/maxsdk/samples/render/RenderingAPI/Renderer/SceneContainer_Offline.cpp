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

#include "SceneContainer_Offline.h"

#include "3dsmax_banned.h"

namespace Max
{;
namespace RenderingAPI
{;

SceneContainer_Offline::SceneContainer_Offline(const IRenderSettingsContainer& render_settings_container, IImmediateInteractiveRenderingClient* const notification_client)
    : SceneContainer_Base(render_settings_container, notification_client)
{

}

SceneContainer_Offline::~SceneContainer_Offline()
{

}

}}	// namespace

