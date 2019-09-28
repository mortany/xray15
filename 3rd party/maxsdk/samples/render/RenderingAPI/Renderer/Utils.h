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

#include <maxtypes.h>
#include <RenderingAPI/Renderer/UnifiedRenderer.h>

namespace Max
{;
namespace RenderingAPI
{;
namespace Utils
{;

using namespace MaxSDK::RenderingAPI;

// Modifies the given motion blur settings with the node's object properties.
MotionBlurSettings ApplyMotionBlurSettingsFromNode(
    INode& node, 
    const TimeValue t, 
    Interval& validity, 
    const MotionBlurSettings& input_motion_blur_settings,
    const UnifiedRenderer& renderer);

// Evaluates the node's motion blur transforms, using the given motion blur settings (NOT taking into account any of the node's own motion blur properties).
MotionTransforms EvaluateMotionTransformsForNode(
    INode& node, 
    const TimeValue t, 
    Interval& validity, 
    const MotionBlurSettings& node_motion_blur_settings);

}}}	// namespace 

