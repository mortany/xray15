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

#include "RenderSettingsContainer_Offline.h"

// max sdk
#include <render.h>

#include "3dsmax_banned.h"

namespace Max
{;
namespace RenderingAPI
{;

RenderSettingsContainer_Offline::RenderSettingsContainer_Offline(Renderer& renderer, IImmediateInteractiveRenderingClient* const notification_client)
    : RenderSettingsContainer_Base(renderer, notification_client),
    m_render_type(RenderTargetType::AnimationOrStill)
{

}

void RenderSettingsContainer_Offline::set_rend_params(RendParams& rend_params)
{
    m_rend_params = rend_params;
    if(rend_params.inMtlEdit)
    {
        m_render_type = RenderTargetType::MaterialEditor;
    }
    else if(rend_params.IsToneOperatorPreviewRender())
    {
        m_render_type = RenderTargetType::ToneOperatorPreview;
    }
    else
    {
        m_render_type = RenderTargetType::AnimationOrStill;
    }

    // Monitor the tone operator
    monitor_tone_operator(GetToneOperator());
}

bool RenderSettingsContainer_Offline::GetRenderOnlySelected() const
{
    return (m_rend_params.rendType == RENDTYPE_SELECT);
}

bool RenderSettingsContainer_Offline::GetRenderHiddenObjects() const 
{
    return !!m_rend_params.rendHidden;
}

bool RenderSettingsContainer_Offline::GetHideFrozenObjects() const 
{
    return !!(m_rend_params.extraFlags & RENDER_HIDE_FROZEN);
}

bool RenderSettingsContainer_Offline::GetDisplacementEnabled() const 
{
    return !!m_rend_params.useDisplacement;
}

ToneOperator* RenderSettingsContainer_Offline::GetToneOperator() const
{
    return m_rend_params.pToneOp;
}

TimeValue RenderSettingsContainer_Offline::GetFrameDuration() const
{
    return m_rend_params.frameDur;
}

RenderSettingsContainer_Offline::RenderTargetType RenderSettingsContainer_Offline::GetRenderTargetType() const
{
    return m_render_type;
}

}}	// namespace
