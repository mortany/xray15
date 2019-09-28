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

// Max SDK
#include <render.h>

namespace Max
{;
namespace RenderingAPI
{;

using namespace MaxSDK::RenderingAPI;

// This interface exposes access to the set of properties that affect the behaviour of the renderer and translator
class RenderSettingsContainer_Offline :
    public RenderSettingsContainer_Base
{
public:

    RenderSettingsContainer_Offline(Renderer& renderer, IImmediateInteractiveRenderingClient* const notification_client);

    void set_rend_params(RendParams& rend_params);

    // -- inherited from IRenderSettingsContainer
    virtual bool GetRenderOnlySelected() const override;
    virtual bool GetRenderHiddenObjects() const override;
    virtual bool GetHideFrozenObjects() const override;
    virtual bool GetDisplacementEnabled() const override;
    virtual TimeValue GetFrameDuration() const override;
    virtual RenderTargetType GetRenderTargetType() const override;
    virtual ToneOperator* GetToneOperator() const override;
private:

    RenderTargetType m_render_type;

    RendParams m_rend_params;
};

}}	// namespace 

