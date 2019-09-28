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
#include <Rendering/ToneOperator.h>
#include <render.h>

#include "3dsmax_banned.h"

namespace Max
{;
namespace RenderingAPI
{;

RenderSettingsContainer_Base::RenderSettingsContainer_Base(Renderer& renderer, IImmediateInteractiveRenderingClient* const notification_client)
    : m_renderer(renderer),
    m_monitored_tone_operator(nullptr),
    m_previous_physical_scale(0.0f),
    m_notification_client(notification_client)
{

}

RenderSettingsContainer_Base::~RenderSettingsContainer_Base()
{
    // Stop monitoring the tone operator
    if((m_notification_client != nullptr) && (m_monitored_tone_operator != nullptr))
    {
        m_notification_client->StopMonitoringReferenceTarget(*m_monitored_tone_operator, ~0u, *this, nullptr);
    }
}

void RenderSettingsContainer_Base::monitor_tone_operator(ToneOperator* const tone_op)
{
    if(tone_op != m_monitored_tone_operator)
    {
        // Stop monitoring the old tone operator and monitor the new one instead
        if(m_notification_client != nullptr)
        {
            // Stop monitoring the old one
            if(m_monitored_tone_operator != nullptr)
            {
                m_notification_client->StopMonitoringReferenceTarget(*m_monitored_tone_operator, ~0u, *this, nullptr);
            }
            // Monitor the new one
            if(tone_op != nullptr)
            {
                m_notification_client->MonitorReferenceTarget(*tone_op, ~0u, *this, nullptr);
            }
        }
        m_monitored_tone_operator = tone_op;

        // Notify the registered listeners of the change(s)
        for(IChangeNotifier* notifier : m_change_notifiers)
        {
            if(notifier != nullptr)
            {
                notifier->NotifyToneOperatorSwapped();
            }
        }
    }

    // Check if the physical scale has changed
    CheckIfPhysicalScaleChanged();
}

float RenderSettingsContainer_Base::GetPhysicalScale(const TimeValue t, Interval& validity) const
{
    ToneOperator* const tone_op = GetToneOperator();
    if(tone_op != nullptr)
    {
        return tone_op->GetPhysicalUnit(t, validity);
    }
    else
    {
        // Good old default physical scale
        return 1500.0f;
    }
}

Renderer& RenderSettingsContainer_Base::GetRenderer() const 
{
    return m_renderer;

}

void RenderSettingsContainer_Base::RegisterChangeNotifier(IChangeNotifier& notifier) const
{
    m_change_notifiers.push_back(&notifier);
}

void RenderSettingsContainer_Base::UnregisterChangeNotifier(IChangeNotifier& notifier) const
{
    for(auto it = m_change_notifiers.begin(); it != m_change_notifiers.end(); ++it)
    {
        if(&notifier == *it)
        {
            m_change_notifiers.erase(it);
            return;
        }
    }

    // Shouldn't get here
    DbgAssert(false);
}

void RenderSettingsContainer_Base::NotificationCallback_NotifyEvent(const IGenericEvent& genericEvent, void* /*userData*/)
{
    switch(genericEvent.GetEventType())
    {
    default:
    case EventType_ReferenceTarget_Deleted:
        monitor_tone_operator(nullptr);
        break;
    case EventType_ReferenceTarget_ParamBlock:
    case EventType_ReferenceTarget_Uncategorized:
        // Notify the registered listeners of the change(s)
        for(IChangeNotifier* notifier : m_change_notifiers)
        {
            if(notifier != nullptr)
            {
                notifier->NotifyToneOperatorParamsChanged();
            }
        }
        break;
    }

    // Check if the physical scale changed
    CheckIfPhysicalScaleChanged();
}

void RenderSettingsContainer_Base::CheckIfPhysicalScaleChanged()
{
    // Check if the physical scale has changed
    Interval dummy_interval;
    const float new_physical_scale = GetPhysicalScale(GetCOREInterface()->GetTime(), dummy_interval);
    if(new_physical_scale != m_previous_physical_scale)
    {
        m_previous_physical_scale = new_physical_scale;

        // Notify the registered listeners of the change(s)
        for(IChangeNotifier* notifier : m_change_notifiers)
        {
            if(notifier != nullptr)
            {
                notifier->NotifyPhysicalScaleChanged();
            }
        }
    }
}


}}	// namespace 
