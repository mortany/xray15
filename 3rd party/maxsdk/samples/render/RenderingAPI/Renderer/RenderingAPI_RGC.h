//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#pragma	once

#include <imtl.h>
#include <render.h>
#include <RenderingAPI/Renderer/IRenderSessionContext.h>

namespace Max
{;
namespace RenderingAPI
{;

using namespace MaxSDK::RenderingAPI;

// The rendering API's implementation of RenderGlobalContext
class RenderingAPI_RGC : public RenderGlobalContext
{
public:

    // Sets up the context using the given scene
    RenderingAPI_RGC(
        Renderer& renderer, 
        const RendParams& rend_params,
        const IRenderSessionContext& render_session_context, 
        const TimeValue t);

    // -- inherited from RenderGlobalContext
    virtual ViewParams* GetViewParams();        // This needs to be implemented or ShaveMax crashes

private:

    ViewParams m_view_params;
};


}}  // namespace