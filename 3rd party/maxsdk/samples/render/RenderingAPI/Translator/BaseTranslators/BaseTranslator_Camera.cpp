//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2014 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <RenderingAPI/Translator/BaseTranslators/BaseTranslator_Camera.h>

// local
#include "../../resource.h"
// Max SDK
#include <RenderingAPI/Renderer/IRenderSessionContext.h>
#include <inode.h>
#include <dllutilities.h>

#include "3dsmax_banned.h"

namespace MaxSDK
{;
namespace RenderingAPI
{;

using namespace MaxSDK::NotificationAPI;

BaseTranslator_Camera::BaseTranslator_Camera(TranslatorGraphNode& graphNode)
    : Translator(graphNode)
{
    // Listen for changes notifications from the camera
    GetRenderSessionContext().GetCamera().RegisterChangeNotifier(*this);
}

BaseTranslator_Camera::~BaseTranslator_Camera()
{
    // Stop listening for change notifications
    GetRenderSessionContext().GetCamera().UnregisterChangeNotifier(*this);
}

Interval BaseTranslator_Camera::CheckValidity(const TimeValue /*t*/, const Interval& previous_validity) const 
{
    return previous_validity;
}

void BaseTranslator_Camera::PreTranslate(const TimeValue translationTime, Interval& /*validity*/) 
{
    // Call RenderBegin on the camera node, if applicable
    INode* const camera_node = GetRenderSessionContext().GetCamera().GetCameraNode();
    if(camera_node != nullptr)
    {
        GetRenderSessionContext().CallRenderBegin(*camera_node, translationTime);
    }
}

void BaseTranslator_Camera::PostTranslate(const TimeValue /*translationTime*/, Interval& /*validity*/) 
{
    // Nothing to do
}

void BaseTranslator_Camera::NotifyCameraChanged()
{
    Invalidate();
}

MSTR BaseTranslator_Camera::GetTimingCategory() const 
{
    return MaxSDK::GetResourceStringAsMSTR(IDS_TIMING_CAMERA);
}

}}	// namespace 

