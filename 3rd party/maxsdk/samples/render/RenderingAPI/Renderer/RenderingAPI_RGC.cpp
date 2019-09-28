//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "RenderingAPI_RGC.h"

// Max SDK
#include <bitmap.h>
#include <RenderingAPI/Renderer/ICameraContainer.h>
#include <RenderingAPI/Renderer/IRenderSettingsContainer.h>
#include <RenderingAPI/Renderer/IEnvironmentContainer.h>

#include "3dsmax_banned.h"

using namespace MaxSDK;

namespace
{
    inline void RemoveScaling(Matrix3 &m) 
    {
        for(int i = 0; i < 3; i++) 
            m.SetRow(i, Normalize (m.GetRow(i)));
    }
}

namespace Max
{;
namespace RenderingAPI
{;

RenderingAPI_RGC::RenderingAPI_RGC(
    Renderer& renderer, 
    const RendParams& rend_params, 
    const IRenderSessionContext& render_session_context, 
    const TimeValue t)
{
    const ICameraContainer& camera_container = render_session_context.GetCamera();

    // Dummy validity - we don't actually care about the validity
    Interval dummy_validity;

    // Setup the RGC
    {
        //!! Lots of magic code copied from mental ray integration

        Bitmap* const bitmap = camera_container.GetBitmap();

        this->renderer = &renderer;
        this->projType = (camera_container.GetProjectionType(t, dummy_validity) == ICameraContainer::ProjectionType::Perspective) ? PROJ_PERSPECTIVE : PROJ_PARALLEL;
        this->devWidth = camera_container.GetResolution().x;
        this->devHeight = camera_container.GetResolution().y;
        this->devAspect = (bitmap != nullptr) ? bitmap->Aspect() : 1.0f;
        if (this->projType == PROJ_PERSPECTIVE) 
        {
            // Magic code copied from mental ray integration
            const float fac = -(1.0f / tanf(0.5f*camera_container.GetPerspectiveFOVRadians(t, dummy_validity)));
            this->xscale =  fac*this->xc;
        }
        else
        {
            // Magic code copied from mental ray integration
            this->xscale = this->devWidth / camera_container.GetOrthographicApertureWidth(t, dummy_validity);
        }
        this->yscale = -this->devAspect*this->xscale;
        this->xc = this->devWidth * 0.5f;
        this->yc = this->devHeight * 0.5f;

        this->antialias = false;
        this->camToWorld = camera_container.EvaluateCameraTransform(t, dummy_validity).shutter_open;
        RemoveScaling(this->worldToCam);      // mrmax does this, so I'll do it do here, I guess there are plugins which need this.
        this->worldToCam = Inverse(this->camToWorld);
        // Ignore near/far range (which I believe is used to limit global volume effects), as it's not well defined and questionably useful
        this->nearRange = 0.0f;
        this->farRange = 0.0f;
        this->frameDur = render_session_context.GetRenderSettings().GetFrameDuration();

        // Set environment map
        {
            this->envMap = nullptr;
            const IEnvironmentContainer* const env = render_session_context.GetEnvironment();
            if((env != nullptr) && (env->GetEnvironmentMode() == IEnvironmentContainer::EnvironmentMode::Texture))
            {
                this->envMap = env->GetEnvironmentTexture();
            }
        }

        const MotionBlurSettings motion_blur_settings = render_session_context.GetCamera().GetGlobalMotionBlurSettings(t, dummy_validity);

        this->globalLightLevel = Color(0.0f, 0.0f, 0.0f);   // no supported by RapidRT (deprecate feature!?)
        this->atmos = nullptr;        //!! This param isn't very useful - we can have multiple atmospherics, but this expects a pointer to the atmospheric 'container', class RenderEnvironment, which isn't even public - so why bother?
        this->pToneOp = render_session_context.GetRenderSettings().GetToneOperator();
        this->time = t;
        this->wireMode = false;       // Not supported (this is specific to the scanline renderer)
        this->wire_thick = 0;         // Not supported (this is specific to the scanline renderer)
        this->force2Side = rend_params.force2Side;
        this->inMtlEdit = render_session_context.GetRenderSettings().GetIsMEditRender();
        this->fieldRender = rend_params.fieldRender;
        this->first_field = true;     //!! NOTE: No support for field rendering
        this->objMotBlur = motion_blur_settings.IsMotionBlurEnabled();
        this->nBlurFrames = motion_blur_settings.shutter_duration;
        this->simplifyAreaLights = rend_params.simplifyAreaLights;
    }

    // Setup the ViewParams
    m_view_params = camera_container.GetViewParams(t, dummy_validity);
}

ViewParams* RenderingAPI_RGC::GetViewParams()
{
    return &m_view_params;
}


}}  // namespace