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

#include "BaseTranslator_Camera_to_RTIShader.h"

// local includes
#include "../resource.h"
#include "Translator_PhysicalCamera_to_RTIShader.h"
#include "Translator_StandardCamera_to_RTIShader.h"
// max sdk
#include <dllutilities.h>
#include <RenderingAPI/Renderer/ISceneContainer.h>
#include <RenderingAPI/Renderer/IRenderingLogger.h>

namespace Max
{;
namespace RapidRTTranslator
{;

BaseTranslator_Camera_to_RTIShader::BaseTranslator_Camera_to_RTIShader(const Key& /*key*/, TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_Camera(translator_graph_node),
    BaseTranslator_to_RTIShader(translator_graph_node),
    Translator(translator_graph_node)
{

}

BaseTranslator_Camera_to_RTIShader::~BaseTranslator_Camera_to_RTIShader()
{

}

void BaseTranslator_Camera_to_RTIShader::PostTranslate(const TimeValue translationTime, Interval& validity) 
{
    const IRenderSessionContext& context = GetRenderSessionContext();
    const ICameraContainer& camera_container = context.GetCamera();
    
    // Far clip plane isn't supported, so output a warning if the far clipping plane is within the scene bounding box
    Interval far_clip_validity;        // Don't care about animation validity for far clip plane, since it's not supported
    if(camera_container.GetClipEnabled(translationTime, far_clip_validity))
    {
        const float clip_far = camera_container.GetClipFar(translationTime, far_clip_validity);
        Box3 scene_bounding_box = context.GetScene().GetSceneBoundingBox(translationTime, far_clip_validity);
        // Add the camera position to the scene bounding box, in case it lies outside of it
        const MotionTransforms camera_transform = camera_container.EvaluateCameraTransform(translationTime, far_clip_validity);
        scene_bounding_box += camera_transform.shutter_open.GetTrans();
        scene_bounding_box += camera_transform.shutter_close.GetTrans();
        const float scene_width = scene_bounding_box.Width().Length();
        if(clip_far < scene_width)
        {
            context.GetLogger().LogMessage(IRenderingLogger::MessageType::Error, MaxSDK::GetResourceStringAsMSTR(IDS_ERROR_FAR_CLIP_NOT_SUPPORTED));
        }
    }

    __super::PostTranslate(translationTime, validity);
}

//==================================================================================================
// class BaseTranslator_Camera_to_RTIShader::Allocator
//==================================================================================================

std::unique_ptr<Translator> BaseTranslator_Camera_to_RTIShader::Allocator::operator()(const Key& key, TranslatorGraphNode& translator_graph_node, const IRenderSessionContext& render_session_context, const TimeValue initial_time) const
{

    const Translator_PhysicalCamera_to_RTIShader::Key& camera_shader_key = static_cast<const Translator_PhysicalCamera_to_RTIShader::Key&>(key);
    if(Translator_PhysicalCamera_to_RTIShader::can_translate(camera_shader_key, initial_time, render_session_context))
    {
        return std::unique_ptr<Translator>(new Translator_PhysicalCamera_to_RTIShader(camera_shader_key, translator_graph_node));
    }
    else
    {
        // Default to the standard camera shader
        return std::unique_ptr<Translator>(new Translator_StandardCamera_to_RTIShader(camera_shader_key, translator_graph_node));
    }
}

}}	// namespace 
