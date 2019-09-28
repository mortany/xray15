//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "Translator_RTIScene.h"

// local
#include "../resource.h"
#include "Translator_SceneRoot_to_RTIGroup.h"
#include "BaseTranslator_Environment_to_RTIShader.h"
#include "Translator_Camera_to_RTICamera.h"
#include "Translator_RTIRenderOptions.h"
#include "Translator_to_RTIFrameBuffer.h"
// max sdk
#include <dllutilities.h>
#include <RenderingAPI/Renderer/ISceneContainer.h>
#include <RenderingAPI/Renderer/IEnvironmentContainer.h>
#include <RenderingAPI/Translator/ITranslationManager.h>

using namespace MaxSDK::NotificationAPI;

namespace Max
{;
namespace RapidRTTranslator
{;

Translator_RTIScene::Translator_RTIScene(const Key& /*key*/, TranslatorGraphNode& translator_graph_node)
    : BaseTranslator_to_RTI(translator_graph_node),
    Translator(translator_graph_node),
    m_monitored_environment(nullptr)
{

}

Translator_RTIScene::~Translator_RTIScene()
{
    // Stop monitoring the environment
    monitor_environment(nullptr);
}

TranslationResult Translator_RTIScene::Translate(const TimeValue t, Interval& /*validity*/, ITranslationProgress& translation_progress, KeyframeList& /*keyframesNeeded*/)
{
	TranslationResult result;

    // Translate the root group
	const Translator_SceneRoot_to_RTIGroup* root_group_translator = AcquireChildTranslator<Translator_SceneRoot_to_RTIGroup>(Translator_SceneRoot_to_RTIGroup::Key(), t, translation_progress, result);
	if(root_group_translator == nullptr)
	{
        return result;
	}

    // Translate the scene environment
    const BaseTranslator_Environment_to_RTIShader* environment_translator = AcquireChildTranslator<BaseTranslator_Environment_to_RTIShader>(BaseTranslator_Environment_to_RTIShader::Key(GetRenderSessionContext().GetEnvironment()), t, translation_progress, result);
    if(environment_translator == nullptr)
    {
        return result;
    }
    // Monitor the environment, to re-acquire it if it changes
    monitor_environment(GetRenderSessionContext().GetEnvironment());

    // Translate the scene camera
    const Translator_Camera_to_RTICamera* camera_translator = AcquireChildTranslator<Translator_Camera_to_RTICamera>(Translator_Camera_to_RTICamera::Key(), t, translation_progress, result);
    if(camera_translator == nullptr)
    {
        return result;
    }

    // Translate the render options
    const Translator_RTIRenderOptions* render_options_translator = AcquireChildTranslator<Translator_RTIRenderOptions>(Translator_RTIRenderOptions::Key(), t, translation_progress, result);
    if(render_options_translator == nullptr)
    {
        return result;
    }

    // Translate the frame buffer
    const Translator_to_RTIFrameBuffer* frame_buffer_translator = AcquireChildTranslator<Translator_to_RTIFrameBuffer>(Translator_to_RTIFrameBuffer::Key(), t, translation_progress, result);
    if(frame_buffer_translator == nullptr)
    {
        return result;
    }

    // Create the rti::Scene
    const rti::SceneHandle scene_handle = initialize_output_handle<rti::SceneHandle>(0);
    if(scene_handle.isValid())
    {
        rti::TEditPtr<rti::Scene> scene(scene_handle);
        scene->setSceneRoot(root_group_translator->get_output_group());
        scene->setCamera(camera_translator->get_output_camera());
        scene->setFramebuffer(frame_buffer_translator->get_output_frame_buffer());
        scene->setEnvironmentShader(environment_translator->get_output_shader());
        scene->setRenderOptions(render_options_translator->get_output_render_options());
        return TranslationResult::Success;
    }
    else
    {
        return TranslationResult::Failure;
    }
}

rti::SceneHandle Translator_RTIScene::get_output_scene() const
{
    return get_output_handle<rti::SceneHandle>(0);
}

Interval Translator_RTIScene::CheckValidity(const TimeValue /*t*/, const Interval& previous_validity) const
{
    return previous_validity;
}

void Translator_RTIScene::PreTranslate(const TimeValue /*translationTime*/, Interval& /*validity*/) 
{
    // Do nothing
}

void Translator_RTIScene::PostTranslate(const TimeValue /*translationTime*/, Interval& /*validity*/) 
{
    // Do nothing
}

void Translator_RTIScene::force_invalidate()
{
    Invalidate();
}

void Translator_RTIScene::monitor_environment(IEnvironmentContainer* const environment)
{
    IImmediateInteractiveRenderingClient* const notification_client = GetRenderSessionContext().GetNotificationClient();
    if((notification_client != nullptr) && (environment != m_monitored_environment))
    {
        // Stop monitoring old environment
        if(m_monitored_environment != nullptr)
        {
            notification_client->StopMonitoringReferenceTarget(*m_monitored_environment, ~size_t(0), *this, nullptr);
        }

        // Monitor the new environment
        if(environment != nullptr)
        {
            notification_client->MonitorReferenceTarget(*environment, ~size_t(0), *this, nullptr);
        }

        m_monitored_environment = environment;
    }
}

void Translator_RTIScene::NotificationCallback_NotifyEvent(const MaxSDK::NotificationAPI::IGenericEvent& genericEvent, void* /*userData*/) 
{
    const IReferenceTargetEvent* ref_targ_event = dynamic_cast<const IReferenceTargetEvent*>(&genericEvent);
    if((ref_targ_event != nullptr) && (ref_targ_event->GetReferenceTarget() == m_monitored_environment))
    {
        // Need to re-acquire the environment translator, in case its type changed
        Invalidate();
    }
}

MSTR Translator_RTIScene::GetTimingCategory() const 
{
    return MaxSDK::GetResourceStringAsMSTR(IDS_TIMING_SCENE);
}

}}		// namespace 
