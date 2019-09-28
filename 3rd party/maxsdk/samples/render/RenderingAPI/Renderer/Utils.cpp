//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "Utils.h"

#include "3dsmax_banned.h"

#undef min      
#undef max      

namespace Max
{;
namespace RenderingAPI
{;
namespace Utils
{;

MotionBlurSettings ApplyMotionBlurSettingsFromNode(
    INode& node, 
    const TimeValue t, 
    Interval& /*validity*/, 
    const MotionBlurSettings& input_motion_blur_settings, 
    const UnifiedRenderer& renderer)
{
    const bool ignoreNodeProperties = renderer.MotionBlurIgnoresNodeProperties();
    const bool motion_blur_enabled_for_node = ignoreNodeProperties || (node.GetMotBlurOnOff(t) && (node.MotBlur() == 1));
    return motion_blur_enabled_for_node ? input_motion_blur_settings : MotionBlurSettings();
}

MotionTransforms EvaluateMotionTransformsForNode(INode& node, const TimeValue t, Interval& validity, const MotionBlurSettings& node_motion_blur_settings)
{
    MotionTransforms transforms;
    transforms.shutter_settings = node_motion_blur_settings;

    const TimeValue offset = transforms.shutter_settings.shutter_offset;
    const TimeValue duration = transforms.shutter_settings.shutter_duration;

    // Evaluate transform at shutter open
    Interval open_validity = FOREVER;
    transforms.shutter_open = node.GetObjTMAfterWSM(t + offset, &open_validity);

    // Evaluate transform at shutter close
    Interval close_validity = FOREVER;
    if(duration == 0)
    {
        transforms.shutter_close = transforms.shutter_open;
        close_validity = open_validity;
    }
    else
    {
        transforms.shutter_close = node.GetObjTMAfterWSM(t + offset + duration, &close_validity);
    }

    // Offset the validity of the transform to overlap its validity range with the current time.
    open_validity.ApplyOffset(-offset);
    close_validity.ApplyOffset(-(offset + duration));
    DbgAssert(open_validity.InInterval(t) && close_validity.InInterval(t));
    validity &= open_validity;
    validity &= close_validity;

    return transforms;
}


}}}	// namespace 

