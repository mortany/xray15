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
#include "CameraContainer_Offline.h"
#include "RenderSettingsContainer_Offline.h"

#include <Noncopyable.h>

namespace Max
{;
namespace RenderingAPI
{;

using namespace MaxSDK::RenderingAPI;

// The offline (non active shade) version of ISceneContainer
// The scene context stores high-level information about the scene - everything that is needed to translate the root of a scene
// (camera, instances, environment, options, etc.). It's the root to which all of which gets translated is attached.
class SceneContainer_Offline :
    public SceneContainer_Base
{
public:

	SceneContainer_Offline(const IRenderSettingsContainer& render_settings_container, IImmediateInteractiveRenderingClient* const notification_client);
    ~SceneContainer_Offline();

};

}}	// namespace

