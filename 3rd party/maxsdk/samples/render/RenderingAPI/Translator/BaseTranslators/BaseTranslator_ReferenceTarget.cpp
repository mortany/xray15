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

#include <RenderingAPI/Translator/BaseTranslators/BaseTranslator_ReferenceTarget.h>

// Max SDK
#include <RenderingAPI/Renderer/IRenderSessionContext.h>
#include <imtl.h>

#include "3dsmax_banned.h"

namespace MaxSDK
{;
namespace RenderingAPI
{;
using namespace NotificationAPI;

BaseTranslator_ReferenceTarget::BaseTranslator_ReferenceTarget(
    ReferenceTarget* reference_target, 
    const bool monitor_as_referencetarget, 
    TranslatorGraphNode& graphNode)

    : m_reference_target(reference_target),
    m_monitor_as_referencetarget(monitor_as_referencetarget),
    Translator(graphNode)
{
    // Monitor the reference target for changes
    if(m_monitor_as_referencetarget && (m_reference_target != nullptr))
    {
        IImmediateInteractiveRenderingClient* const notifications = GetRenderSessionContext().GetNotificationClient();
        if(notifications != nullptr)
        {
            notifications->MonitorReferenceTarget(
                *m_reference_target, 
                EventType_ReferenceTarget_Deleted | EventType_ReferenceTarget_ParamBlock | EventType_ReferenceTarget_Uncategorized,
                *this,
                nullptr);
        }
    }
}

BaseTranslator_ReferenceTarget::~BaseTranslator_ReferenceTarget()
{
    // Stop monitoring the material for changes
    if(m_monitor_as_referencetarget && (m_reference_target != nullptr))
    {
        IImmediateInteractiveRenderingClient* const notifications = GetRenderSessionContext().GetNotificationClient();
        if(notifications != nullptr)
        {
            notifications->StopMonitoringReferenceTarget(*m_reference_target, ~size_t(0), *this, nullptr);
        }
    }
}

Interval BaseTranslator_ReferenceTarget::CheckValidity(const TimeValue /*t*/, const Interval& previous_validity) const 
{
    return previous_validity;
}

void BaseTranslator_ReferenceTarget::NotificationCallback_NotifyEvent(const IGenericEvent& genericEvent, void* /*userData*/) 
{
    const IReferenceTargetEvent* ref_targ_event = dynamic_cast<const IReferenceTargetEvent*>(&genericEvent);
    if((ref_targ_event != nullptr) && (ref_targ_event->GetReferenceTarget() == m_reference_target))
    {
        switch(genericEvent.GetEventType())
        {
        case EventType_ReferenceTarget_Deleted:
            TranslatedObjectDeleted();
            break;
        case EventType_ReferenceTarget_ParamBlock:
        case EventType_ReferenceTarget_Uncategorized:
            Invalidate();
            break;
        default:
            // Ignore these events
            break;
        }
    }
}

ReferenceTarget* BaseTranslator_ReferenceTarget::GetReferenceTarget() const
{
    return m_reference_target;
}

void BaseTranslator_ReferenceTarget::PreTranslate(const TimeValue translationTime, Interval& /*validity*/) 
{
    // Call RenderBegin on the reference_target
    if(m_reference_target != nullptr)
    {
        GetRenderSessionContext().CallRenderBegin(*m_reference_target, translationTime);
    }
}

void BaseTranslator_ReferenceTarget::PostTranslate(const TimeValue /*translationTime*/, Interval& /*validity*/) 
{
    // Nothing to do
}

}}	// namespace 


