//**************************************************************************/
// Copyright (c) 1998-2013 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/
// DESCRIPTION: Implementation
// AUTHOR: David Lanier
//***************************************************************************/

#include "ImmediateNotificationClient.h"

#include "NotificationManager.h"

namespace Max{
namespace NotificationAPI{
;

//-------------------------------------------------------------------
// ----------			class ImmediateNotificationClient			---------
//-------------------------------------------------------------------

ImmediateNotificationClient::ImmediateNotificationClient() 
{
    m_NotificationsEnabled = true;//receive notifications by default
}

ImmediateNotificationClient::~ImmediateNotificationClient()
{
    // Unregister this client from all notifiers
    NotificationManager::GetInstance().UnregisterNotificationClient(*this);
}

bool ImmediateNotificationClient::MonitorNode(INode& node, const NotifierType type, const size_t monitoredEvents, INotificationCallback& callback, void* const user_data)
{
    // Register this client and callback
    NotificationManager::GetInstance().RegisterWithNodeNotifier(node, type, *this, callback, user_data, monitoredEvents);
    return true;
}

bool ImmediateNotificationClient::MonitorMaterial(Mtl& mtl, size_t monitoredEvents, INotificationCallback& callback, void* user_data)
{
    NotificationManager::GetInstance().RegisterWithMaterialNotifier(mtl, *this, callback, user_data, monitoredEvents);
    return true;
}

bool ImmediateNotificationClient::MonitorTexmap(Texmap& texmap,	size_t monitoredEvents, INotificationCallback& callback, void* user_data)
{	
    NotificationManager::GetInstance().RegisterWithTexmapNotifier(texmap, *this, callback, user_data, monitoredEvents);
    return true;
}

bool ImmediateNotificationClient::MonitorReferenceTarget(ReferenceTarget& refTarg,	size_t monitoredEvents, INotificationCallback& callback, void* user_data)
{	
    NotificationManager::GetInstance().RegisterWithReferenceTargetNotifier(refTarg, *this, callback, user_data, monitoredEvents);
    return true;
}

bool ImmediateNotificationClient::MonitorViewport(const int viewID, const size_t monitoredEvents, INotificationCallback& callback, void* const user_data)
{
    NotificationManager::GetInstance().RegisterWithViewNotifier(viewID, *this, callback, user_data, monitoredEvents);
    return true;
}

bool ImmediateNotificationClient::MonitorRenderEnvironment(size_t monitoredEvents, INotificationCallback& callback, void* user_data)
{
    NotificationManager::GetInstance().RegisterWithRenderEnvironmentNotifier(*this, callback, user_data, monitoredEvents);
    return true;
}

bool ImmediateNotificationClient::MonitorRenderSettings(size_t monitoredEvents, INotificationCallback& callback, void* user_data)
{
    NotificationManager::GetInstance().RegisterWithRenderSettingsNotifier(*this, callback, user_data, monitoredEvents);
    return true;
}

bool ImmediateNotificationClient::MonitorScene(size_t monitoredEvents, INotificationCallback& callback, void* user_data)
{
    NotificationManager::GetInstance().RegisterWithSceneNotifier(*this, callback, user_data, monitoredEvents);
    return true;
}

bool ImmediateNotificationClient::StopMonitoringNode(INode& node, size_t monitoredEvents, INotificationCallback& callback, void* user_data)
{
    NotificationManager::GetInstance().UnRegisterWithNodeNotifier(node, *this, callback, user_data, monitoredEvents);
    return true;
}

bool ImmediateNotificationClient::StopMonitoringMaterial(Mtl& mtl, size_t monitoredEvents, INotificationCallback& callback, void* user_data)
{
    NotificationManager::GetInstance().UnRegisterWithMaterialNotifier(mtl, *this, callback, user_data, monitoredEvents);
    return false;
}

bool ImmediateNotificationClient::StopMonitoringTexmap(Texmap& texmap, size_t monitoredEvents, INotificationCallback& callback, void* user_data)
{
    NotificationManager::GetInstance().UnRegisterWithTexmapNotifier(texmap, *this, callback, user_data, monitoredEvents);
    return true;
}

bool ImmediateNotificationClient::StopMonitoringReferenceTarget(ReferenceTarget& refTarg, size_t monitoredEvents, INotificationCallback& callback, void* user_data)
{
    NotificationManager::GetInstance().UnRegisterWithReferenceTargetNotifier(refTarg, *this, callback, user_data, monitoredEvents);
    return true;
}

bool ImmediateNotificationClient::StopMonitoringViewport(const int viewID, const size_t monitoredEvents, INotificationCallback& callback, void* const user_data)
{
    NotificationManager::GetInstance().UnRegisterWithViewNotifier(viewID, *this, callback, user_data, monitoredEvents);
    return true;
}

bool ImmediateNotificationClient::StopMonitoringRenderEnvironment(size_t monitoredEvents, INotificationCallback& callback, void* user_data)
{
    NotificationManager::GetInstance().UnRegisterWithRenderEnvironmentNotifier(*this, callback, user_data, monitoredEvents);
    return true;
}

bool ImmediateNotificationClient::StopMonitoringRenderSettings(size_t monitoredEvents, INotificationCallback& callback, void* user_data)
{
    NotificationManager::GetInstance().UnRegisterWithRenderSettingsNotifier(*this, callback, user_data, monitoredEvents);
    return true;
}

bool ImmediateNotificationClient::StopMonitoringScene(size_t monitoredEvents, INotificationCallback& callback, void* user_data)
{
    NotificationManager::GetInstance().UnRegisterWithSceneNotifier(*this, callback, user_data, monitoredEvents);
    return true;
}

void ImmediateNotificationClient::DebugPrintToFile(FILE* file)const
{
	if (NULL == file ){
		DbgAssert(0 && _T("file is NULL"));
		return;
	}
	
	_ftprintf(file, _T("** NotificationClient debugging **\n\n"));

	_ftprintf(file, _T("** NotificationClient version : %d **\n"), VersionNumber());
}

};//end of namespace NotificationAPI
};//end of namespace Max