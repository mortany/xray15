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

#include "RenderSettingsContainer_Interactive.h"
// max sdk
#include <toneop.h>
#include <units.h>
#include <maxapi.h>

#include "3dsmax_banned.h"

namespace Max
{;
namespace RenderingAPI
{;

RenderSettingsContainer_Interactive::RenderSettingsContainer_Interactive(Renderer& renderer, IImmediateInteractiveRenderingClient* const notification_client)
    : RenderSettingsContainer_Base(renderer, notification_client)
{
    // Register the tone operator change callback
    ToneOperatorInterface* const tone_op_interface = static_cast<ToneOperatorInterface*>(GetCOREInterface(TONE_OPERATOR_INTERFACE));
    if(DbgVerify(tone_op_interface != nullptr))
    {
        tone_op_interface->RegisterToneOperatorChangeNotification(ToneOperatorChangeCallback, this);
    }

    // Monitor the initial tone operator
    monitor_tone_operator(GetToneOperator());

    // Fetch the options from the common render panel
    GetCOREInterface()->SetupRendParams(m_common_rend_params, 
        nullptr, //this isn't used internally
        RENDTYPE_REGION);       // Don't care about this value, it's used to set RenderParams::rendType
}

RenderSettingsContainer_Interactive::~RenderSettingsContainer_Interactive()
{
    // Unregister the tone operator change callback
    ToneOperatorInterface* const tone_op_interface = static_cast<ToneOperatorInterface*>(GetCOREInterface(TONE_OPERATOR_INTERFACE));
    if(DbgVerify(tone_op_interface != nullptr))
    {
        tone_op_interface->UnRegisterToneOperatorChangeNotification(ToneOperatorChangeCallback, this);
    }
}

bool RenderSettingsContainer_Interactive::GetRenderOnlySelected() const
{
    return false;
}

bool RenderSettingsContainer_Interactive::GetRenderHiddenObjects() const 
{
    return !!m_common_rend_params.rendHidden;
}

bool RenderSettingsContainer_Interactive::GetHideFrozenObjects() const 
{
    return !!(m_common_rend_params.extraFlags & RENDER_HIDE_FROZEN);
}

bool RenderSettingsContainer_Interactive::GetDisplacementEnabled() const 
{
    return !!m_common_rend_params.useDisplacement;
}

ToneOperator* RenderSettingsContainer_Interactive::GetToneOperator() const
{
    ToneOperatorInterface* const tone_op_int = static_cast<ToneOperatorInterface*>(GetCOREInterface(TONE_OPERATOR_INTERFACE));
    if(DbgVerify(tone_op_int != nullptr))
    {
        return tone_op_int->GetToneOperator();
    }
    else
    {
        return nullptr;
    }
}

TimeValue RenderSettingsContainer_Interactive::GetFrameDuration() const
{
    return GetTicksPerFrame();
}

RenderSettingsContainer_Interactive::RenderTargetType RenderSettingsContainer_Interactive::GetRenderTargetType() const
{
    return RenderTargetType::Interactive;
}

void RenderSettingsContainer_Interactive::ToneOperatorChangeCallback(ToneOperator* const newOp, ToneOperator* const /*oldOp*/, void* const param)
{
    RenderSettingsContainer_Interactive* const container = static_cast<RenderSettingsContainer_Interactive*>(param);
    if(DbgVerify(container != nullptr))
    {
        container->monitor_tone_operator(newOp);
    }
}

}}	// namespace
