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

#include <RenderingAPI/Translator/BaseTranslators/BaseTranslator_Mtl.h>

// Max SDK
#include <RenderingAPI/Renderer/IRenderSessionContext.h>
#include <imtl.h>

#include "3dsmax_banned.h"

namespace MaxSDK
{;
namespace RenderingAPI
{;
using namespace NotificationAPI;

BaseTranslator_Mtl::BaseTranslator_Mtl(Mtl* mtl, TranslatorGraphNode& graphNode)
    : Translator(graphNode),
    BaseTranslator_MtlBase(mtl, false, graphNode),
    m_mtl(mtl) 
{
    // Monitor the material for changes
    if(m_mtl != nullptr)
    {
        IImmediateInteractiveRenderingClient* const notifications = GetRenderSessionContext().GetNotificationClient();
        if(notifications != nullptr)
        {
            notifications->MonitorMaterial(
                *m_mtl, 
                EventType_Material_Deleted | EventType_Material_ParamBlock | EventType_Material_Uncategorized,
                *this,
                nullptr);
        }
    }
}

BaseTranslator_Mtl::~BaseTranslator_Mtl()
{
    // Stop monitoring the material for changes
    if(m_mtl != nullptr)
    {
        IImmediateInteractiveRenderingClient* const notifications = GetRenderSessionContext().GetNotificationClient();
        if(notifications != nullptr)
        {
            notifications->StopMonitoringMaterial(*m_mtl, ~size_t(0), *this, nullptr);
        }
    }
}

void BaseTranslator_Mtl::NotificationCallback_NotifyEvent(const IGenericEvent& genericEvent, void* userData) 
{
    switch(genericEvent.GetEventType())
    {
    case EventType_Material_Deleted:
        TranslatedObjectDeleted();
        break;
    case EventType_Material_ParamBlock:
    case EventType_Material_Uncategorized:
        Invalidate();
        break;
    default:
        // Ignore these events
        break;
    }
    
    __super::NotificationCallback_NotifyEvent(genericEvent, userData);
}

Mtl* BaseTranslator_Mtl::GetMaterial() const
{
    return m_mtl;
}

}}	// namespace 


