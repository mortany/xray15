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

#include "RenderSettingsContainer_Base.h"

// max sdk
#include <render.h>

namespace Max
{;
namespace RenderingAPI
{;

// This interface exposes access to the set of properties that affect the behaviour of the renderer and translator
class RenderSettingsContainer_Interactive :
    public RenderSettingsContainer_Base
{
public:

    RenderSettingsContainer_Interactive(Renderer& renderer, IImmediateInteractiveRenderingClient* const notification_client);
    ~RenderSettingsContainer_Interactive();

    // -- inherited from IRenderSettingsContainer
    virtual bool GetRenderOnlySelected() const override;
    virtual bool GetRenderHiddenObjects() const override;
    virtual bool GetHideFrozenObjects() const override;
    virtual bool GetDisplacementEnabled() const override;
    virtual TimeValue GetFrameDuration() const override;
    virtual RenderTargetType GetRenderTargetType() const override;
    virtual ToneOperator* GetToneOperator() const override;

private:

    // Callback for getting notified when the tone operator is changed
    static void ToneOperatorChangeCallback(ToneOperator* newOp, ToneOperator* oldOp, void* param);

    // These are the render params fetched from the core interface. 
    // IMPORTANT: They are ONLY to be used for inspecting the settings from the common render panel (other settings may be bogus).
    RendParams m_common_rend_params;
};

}}	// namespace 

