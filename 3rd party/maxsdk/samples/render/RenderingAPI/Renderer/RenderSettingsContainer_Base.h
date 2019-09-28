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

// Max SDK
#include <RenderingAPI/Renderer/IRenderSettingsContainer.h>
#include <Noncopyable.h>
#include <NotificationAPI/NotificationAPI_Subscription.h>
#include <NotificationAPI/InteractiveRenderingAPI_Subscription.h>
// std
#include <vector>

class Renderer;
class ToneOperator;

namespace Max
{;
namespace RenderingAPI
{;

using namespace MaxSDK::RenderingAPI;
using namespace MaxSDK::NotificationAPI;

// This interface exposes access to the set of properties that affect the behaviour of the renderer and translator
class RenderSettingsContainer_Base :
    public IRenderSettingsContainer,
    public MaxSDK::Util::Noncopyable,
    private MaxSDK::NotificationAPI::INotificationCallback
{
public:

    RenderSettingsContainer_Base(Renderer& renderer, IImmediateInteractiveRenderingClient* const notification_client);
    virtual ~RenderSettingsContainer_Base();

    // -- inherited from IRenderSettingsContainer
    virtual float GetPhysicalScale(const TimeValue t, Interval& validity) const override final;
    virtual Renderer& GetRenderer() const override;
    virtual void RegisterChangeNotifier(IChangeNotifier& notifier) const  override final;
    virtual void UnregisterChangeNotifier(IChangeNotifier& notifier) const  override final;

protected:

    // Monitors the given tone operator, to notify listeners whenver it changes.
    // Must be called whenever the tone operator changes.
    void monitor_tone_operator(ToneOperator* const tone_op);

protected:

    // The list of change notifiers currently registers
    mutable std::vector<IChangeNotifier*> m_change_notifiers;

private:

    // Checks if the physical scale has changed, and notifies all listeners if it did
    void CheckIfPhysicalScaleChanged();

    // -- inherited from INotificationCallback
    virtual void NotificationCallback_NotifyEvent(const IGenericEvent& genericEvent, void* userData) override;

private:

    Renderer& m_renderer;

    // The tone operator currently being monitored
    ToneOperator* m_monitored_tone_operator;
    // Caches the previously used physical scale, such that we may know when it has changed
    float m_previous_physical_scale;

    IImmediateInteractiveRenderingClient* const m_notification_client;

};

}}	// namespace 

