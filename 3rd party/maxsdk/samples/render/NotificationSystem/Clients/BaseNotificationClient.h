#pragma once

//**************************************************************************/
// Copyright (c) 2014 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/

#include "Listeners/NotificationDataListener.h"

namespace Max
{ 
namespace NotificationAPI
{;

// Base class for both on-demand and immediate notification clients; implements common functionality.
class BaseNotificationClient :
    protected NotificationDataListener::IEventProcessor
{
public:

    //!! TODO: The contents of this class - all moved to ImmediateNotificationClient
    bool MonitorNode(INode& node, const NotifierType type, const size_t monitoredEvents);
    bool MonitorMaterial(Mtl& pMtl, const NotifierType type, const size_t monitoredEvents);
    bool MonitorTexmap(Texmap& texmap, const NotifierType type, const size_t monitoredEvents);
    bool MonitorViewport(const int viewID, const size_t monitoredEvents);
    bool MonitorRenderEnvironment(const size_t monitoredEvents);
    bool MonitorRenderSettings(const size_t monitoredEvents);
    bool MonitorScene(const size_t monitoredEvents);
    bool StopMonitoringNode(INode& node, const size_t monitoredEvents);
    bool StopMonitoringMaterial(Mtl& mtl, const size_t monitoredEvents);
    bool StopMonitoringTexmap(Texmap& texmap, const size_t monitoredEvents);
    bool StopMonitoringViewport(const int viewID, const size_t monitoredEvents);
    bool StopMonitoringRenderEnvironment(const size_t monitoredEvents);
    bool StopMonitoringRenderSettings(const size_t monitoredEvents);
    bool StopMonitoringScene(const size_t monitoredEvents);
};
	
}}      // namespaces