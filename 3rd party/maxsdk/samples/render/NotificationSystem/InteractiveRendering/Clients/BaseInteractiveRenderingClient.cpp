/******************************************************************************
* Copyright 2014 Autodesk, Inc.  All rights reserved. 
* This computer source code and related instructions and comments are the 
* unpublished confidential and proprietary information of Autodesk, Inc. and 
* are protected under applicable copyright and trade secret law.  They may not 
* be disclosed to, copied or used by any third party without the prior written 
* consent of Autodesk, Inc.
******************************************************************************/

#include "BaseInteractiveRenderingClient.h"

#include "InteractiveRendering/InteractiveRenderingUtils.h"

// src/include
#include "../../NotificationAPIUtils.h"

// Max SDK
#include <object.h>
#include <inode.h>
#include <NotificationAPI/NotificationAPIUtils.h>

namespace Max
{;
namespace NotificationAPI
{;
using namespace MaxSDK::NotificationAPI;

BaseInteractiveRenderingClient::BaseInteractiveRenderingClient(NotificationAPI::IImmediateNotificationClient& notification_client)
    : m_NotificationClient(notification_client)
{

}

void BaseInteractiveRenderingClient::LockViewStateChanged(const NotificationAPI::IGenericEvent& 
#ifdef MAX_ASSERTS_ACTIVE
    genericEvent
#endif
    )
{
    DbgAssert(Max::NotificationAPI::EventType_RenderSettings_LockView == genericEvent.GetEventType());
    DbgAssert(Max::NotificationAPI::NotifierType_RenderSettings == genericEvent.GetNotifierType());

    if (Utils::IsActiveShadeViewLocked() ){
        return; //Then ignore this change, active shade view has just been locked
    }

    //Active shade view has been unlocked so we should now show in active shade current view if not already

    //Get active view
    bool IsAnExtendedView = false;

    ViewExp* viewExp = MaxSDK::NotificationAPIUtils::GetActiveViewExpIncludingExtendedViews(IsAnExtendedView);
    if (! viewExp){
        DbgAssert(0 &&_T("MaxSDK::NotificationAPIUtils::GetActiveViewExpIncludingExtendedViews(IsAnExtendedView) returned NULL"));
        return;
    }

    const int viewID = viewExp->GetViewID();
    if (viewID == m_ViewData.GetViewID() ){
        //This view is already shown so ignore
        return;
    }

    //If we are here, the active view is not shown in the active shade view, so update it.

    //So stop monitoring old viewport, camera or light node
    bool bRes = StopMonitoringViewportOrCameraOrLightNode();
    if (! bRes){
        DbgAssert(0 && _T("StopMonitoringViewportOrCameraOrLightNode didn't work !"));
        return;
    }

    //Monitor new view
    MonitorViewportOrCameraOrLightNode(false);

    ProcessEvent(ViewEvent(ViewEvent::MakeViewActiveEvent(m_ViewData.GetViewID())));
}

void BaseInteractiveRenderingClient::NotificationCallback_NotifyEvent(const Max::NotificationAPI::IGenericEvent& genericEvent, void* 
#ifdef MAX_ASSERTS_ACTIVE
    userData
#endif
    )
{
    DbgAssert(userData == nullptr);     // we're not using the user data, it should always be null

    const Max::NotificationAPI::NotifierType notifierType	= genericEvent.GetNotifierType();	
    switch(notifierType){
    case Max::NotificationAPI::NotifierType_View:
        {
            const ViewEvent* view_event = dynamic_cast<const ViewEvent*>(&genericEvent);
            DbgAssert(view_event != nullptr);
            if(view_event != nullptr)
            {
                switch(view_event->GetEventType())
                {
                case Max::NotificationAPI::EventType_View_Active:
                    //Current active viewport changed
                    ActiveViewportChanged(*view_event);
                    break;
                default:
                    ProcessEvent(*view_event);
                    break;
                }//end of switch
            }
        }
        break;
    case Max::NotificationAPI::NotifierType_Node_Light: //In our case light is only monitored when it's used in a viewport in the role of a camera
    case Max::NotificationAPI::NotifierType_Node_Camera:
        {

            const Max::NotificationAPI::INodeEvent* nodeEvent = dynamic_cast<const Max::NotificationAPI::INodeEvent*>(&genericEvent);
            if (! nodeEvent){
                DbgAssert(0 && _T("genericEvent should be a NodeEvent!"));
                return;
            }

            const Max::NotificationAPI::NodeEventType eventType = static_cast<const Max::NotificationAPI::NodeEventType>(genericEvent.GetEventType());
            switch(eventType)
            {
            case Max::NotificationAPI::EventType_Node_Deleted:
                ActiveCameraWasDeleted(*nodeEvent);
                break;
            case Max::NotificationAPI::EventType_Node_Uncategorized:
            case Max::NotificationAPI::EventType_Node_ParamBlock:
            case Max::NotificationAPI::EventType_Node_Transform:
                //Is a transform event or camera parameter event
                ProcessEvent(ViewEvent::MakeViewPropertiesEvent(nodeEvent->GetNode(), m_ViewData.GetViewID()));
                break;
            }
        }
        break;
    case Max::NotificationAPI::NotifierType_RenderSettings:
        {
            const Max::NotificationAPI::RenderSettingsEventType eventType = static_cast<const Max::NotificationAPI::RenderSettingsEventType>(genericEvent.GetEventType());
            switch(eventType)
            {
            case Max::NotificationAPI::EventType_RenderSettings_LockView:
                //lock/unlock view state changed
                LockViewStateChanged(genericEvent);
                break;
            default:
                break;
            }//end of switch
        }
        break;
    case Max::NotificationAPI::NotifierType_RenderEnvironment:
        {
            //UpdateRenderEnvironment(genericEvent);
        }
        break;
    case Max::NotificationAPI::NotifierType_SceneNode:
        {
            //UpdateScene(genericEvent);
        }
        break;
    default:
        DbgAssert(0 && _T("Unknown notifier type !"));
        break;
    }
}

void BaseInteractiveRenderingClient::ActiveCameraWasDeleted(const INodeEvent& node_event)
{
    DbgAssert(Max::NotificationAPI::EventType_Node_Deleted == node_event.GetEventType());
    DbgAssert(Max::NotificationAPI::NotifierType_Node_Camera    == node_event.GetNotifierType());

    //So stop monitoring old viewport, camera or light node
    bool bRes = StopMonitoringViewportOrCameraOrLightNode();
    if (! bRes){
        DbgAssert(0 && _T("StopMonitoringViewportOrCameraOrLightNode didn't work !"));
        return;
    }

    const bool UseOldviewID = Utils::IsActiveShadeViewLocked(); //Takes care of active shade locked
    MonitorViewportOrCameraOrLightNode(UseOldviewID);

    ProcessEvent(ViewEvent::MakeViewDeletedEvent(node_event.GetNode(), (UseOldviewID)?m_ViewData.GetViewID():-1));
}

void BaseInteractiveRenderingClient::ActiveViewportChanged(const ViewEvent& view_event)
{
    DbgAssert(Max::NotificationAPI::EventType_View_Active == view_event.GetEventType());

    if (Utils::IsActiveShadeViewLocked() ){
        return; //Then ignore this change, active shade view is locked
    }

    //View is not locked 

    //So stop monitoring old viewport, camera or light node
    bool bRes = StopMonitoringViewportOrCameraOrLightNode();
    if (! bRes){
        DbgAssert(0 && _T("StopMonitoringViewportOrCameraOrLightNode didn't work !"));
        return;
    }

    //Monitor new view
    MonitorViewportOrCameraOrLightNode(false);

    // Store a copy of the event
    ProcessEvent(view_event);
}

void BaseInteractiveRenderingClient::MonitorViewportOrCameraOrLightNode(bool bUseOldViewID /*= false*/)
{
    bool IsAnExtendedView = false;

    ViewExp* viewExp        = NULL;
    if (!MaxSDK::NotificationAPIUtils::IsUsingActiveView(RS_IReshade)){
        //So yes, view is locked, get Rend view undoID, it may not be necessarily the active view
        const int viewUndoID = GetCOREInterface16()->GetRendViewID(RS_IReshade);// this returns the view ID for the CURRENT renderer, not necessarily active shade!!
        viewExp = MaxSDK::NotificationAPIUtils::GetViewExpFromUndoIDIncludingExtendedViews(viewUndoID, IsAnExtendedView);
        if (! viewExp){
            DbgAssert(0 &&_T("NotificationSystemPrivate::GetViewExpFromUndoIDIncludingExtendedViews(viewUndoID, IsAnExtendedView) returned NULL"));
            return;
        }
    }else{
        //Get active view
        if (! bUseOldViewID){
            viewExp = MaxSDK::NotificationAPIUtils::GetActiveViewExpIncludingExtendedViews(IsAnExtendedView);
            if (! viewExp){
                DbgAssert(0 &&_T("NotificationSystemPrivate::GetActiveViewExpIncludingExtendedViews(IsAnExtendedView) returned NULL"));
                return;
            }
        }else{
            //bUseOldViewID means we should use the viewId set in m_ViewData
            viewExp = MaxSDK::NotificationAPIUtils::GetViewExpFromUndoIDIncludingExtendedViews(m_ViewData.GetViewID(), IsAnExtendedView);
            if (! viewExp){
                DbgAssert(0 &&_T("NotificationSystemPrivate::GetViewExpFromUndoIDIncludingExtendedViews(m_ViewData.GetViewID(), IsAnExtendedView) returned NULL"));
                return;
            }
        }
    }

    if (! viewExp){
        DbgAssert(0 &&_T("viewExp is NULL"));
        return;
    }

    const int viewUndoID = viewExp->GetViewID(); //Is the view UndoID

    //Is it a camera, light or viewport ?
    INode* pNode    = Utils::GetViewNode(*viewExp);

    if (bUseOldViewID){
        pNode = NULL;//if bUseOldViewID == true then we must not use the Inode that is being deleted... Only monitor the viewport,so NULL the node
    }

    if (pNode ){
        //It's a camera or light view
        TimeValue t             = GetCOREInterface()->GetTime();
        const ObjectState& os   = pNode->EvalWorldState(t);
        Object* pObj            = os.obj;
        if (NULL ==  pObj){
            DbgAssert(0 &&_T("Invalid node for the active viewport ! pObj == NULL"));
            return;
        }

        const Max::NotificationAPI::NotifierType notifierType = (pObj->SuperClassID() == CAMERA_CLASS_ID)?Max::NotificationAPI::NotifierType_Node_Camera : Max::NotificationAPI::NotifierType_Node_Light;
        m_NotificationClient.MonitorNode(*pNode, notifierType, IR_CAM_OR_LIGHT_MONITORED_EVENTS, *this, nullptr);

        //Need to monitor also for viewport changes
        m_NotificationClient.MonitorViewport(viewUndoID, Max::NotificationAPI::EventType_View_Active, *this, nullptr);
    }
    else{
        //It's a viewport
        m_NotificationClient.MonitorViewport( viewUndoID, IR_VIEWPORT_MONITORED_EVENTS, *this, nullptr);
    }

    //Update our ViewData
    m_ViewData.UpdateViewAndNode(viewUndoID, pNode);
}

bool BaseInteractiveRenderingClient::StopMonitoringViewportOrCameraOrLightNode(void)
{
    if (m_ViewData.IsAViewport()){
        return m_NotificationClient.StopMonitoringViewport(m_ViewData.GetViewID(), IR_VIEWPORT_MONITORED_EVENTS, *this, nullptr);
    }

    //Is a camera or light
    //Stop monitoring for active viewport changes and stop monitoring node
    bool bRes   = 
        m_NotificationClient.StopMonitoringViewport(m_ViewData.GetViewID(), Max::NotificationAPI::EventType_View_Active, *this, nullptr) 
        && (m_ViewData.GetCameraOrLightNode() != nullptr)
        && m_NotificationClient.StopMonitoringNode(*(m_ViewData.GetCameraOrLightNode()), IR_CAM_OR_LIGHT_MONITORED_EVENTS, *this, nullptr);
    DbgAssert(bRes);
    return bRes;
}

bool BaseInteractiveRenderingClient::MonitorActiveShadeView_Base()
{
    //Are we already monitoring active view ? Prevent from looking twice at some view with same client
    if ( IsMonitoringActiveView() ){
        DbgAssert(0 && _T("We are already monitoring the active view, can't do it twice !"));
        return false;
    }

    MonitorViewportOrCameraOrLightNode(false);

    //Monitor the lock/unlock view in the render settings dialog
    m_NotificationClient.MonitorRenderSettings(Max::NotificationAPI::EventType_RenderSettings_LockView, *this, nullptr);

    return true;
}

bool BaseInteractiveRenderingClient::StopMonitoringActiveView_Base()
{
    const bool bRes = StopMonitoringViewportOrCameraOrLightNode();
    //Also stop monitoring lock/unlock
    m_NotificationClient.StopMonitoringRenderSettings(Max::NotificationAPI::EventType_RenderSettings_LockView, *this, nullptr);
    return bRes;
}

}}      // namespace