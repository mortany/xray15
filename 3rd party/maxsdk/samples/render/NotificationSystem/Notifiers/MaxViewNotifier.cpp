//
// Copyright 2013 Autodesk, Inc.  All rights reserved. 
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc. and
// are protected under applicable copyright and trade secret law.  They may
// not be disclosed to, copied or used by any third party without the prior
// written consent of Autodesk, Inc.
//
//

#include "MaxViewNotifier.h"

// Local includes
#include "Events/ViewEvent.h"
// src/include
#include "../NotificationAPIUtils.h"
// Max SDK
#include <inode.h>
#include <NotificationAPI/NotificationAPIUtils.h>
#include <notify.h>

namespace Max
{;
namespace NotificationAPI
{;
using namespace MaxSDK::NotificationAPI;


MaxViewNotifier::MaxViewNotifier(int viewID) : 
m_StoredMatrix(1),
m_view_persp(false),
m_view_fov(0.0f),
m_view_focal_dist(0.0f),
m_ViewUndoID(viewID)
{
    if( viewID < 0){
        DbgAssert(0 && _T("viewID is invalid !"));
        return;
    }

    //Update our m_CurrentViewportLabel with the viewport label to track any internal change of view in the same viewport
    MaxSDK::NotificationAPIUtils::GetViewportLabelFromUndoIDIncludingExtendedViews(m_ViewUndoID, m_CurrentViewportLabel);

    //We may have a problem as extended views are not supported in the SDK yet, so the Id from the user may be wrong if he used Interface::GetActiveViewExp() and some extend. views are present...
    bool IsAnExtendedView = false;
    ViewExp* pViewExp = MaxSDK::NotificationAPIUtils::GetViewExpFromUndoIDIncludingExtendedViews(m_ViewUndoID, IsAnExtendedView);
    if (! pViewExp){
        DbgAssert(0 &&_T("pViewExp is NULL"));
        m_ViewNode = NULL;
    }else{
        m_ViewNode          = Utils::GetViewNode(*pViewExp);//Is NULL if it's a viewport
        UpdateOurStoredData(*pViewExp);//Update our members
    }
    GetCOREInterface()->RegisterRedrawViewsCallback(this);
    RegisterNotification(ProcessNotifications, this, NOTIFY_VIEWPORT_CHANGE);
}

MaxViewNotifier::~MaxViewNotifier()
{
    if (m_ViewUndoID >= 0){
        GetCOREInterface()->UnRegisterRedrawViewsCallback(this);
    }
    UnRegisterNotification(ProcessNotifications, this, NOTIFY_VIEWPORT_CHANGE);
    m_ViewUndoID = -1;
}

//Static
void MaxViewNotifier::ProcessNotifications(void *param, NotifyInfo *info)
{
    if ( ! info || ! param){
        DbgAssert(0 && _T("param or info is NULL"));
        return;
    }

	MaxViewNotifier* pViewNotifier = reinterpret_cast<MaxViewNotifier*>(param);

	switch(info->intcode)
	{
		case NOTIFY_VIEWPORT_CHANGE:
		{
            bool IsAnExtendedView = false;
            ViewExp* ActiveViewExp = MaxSDK::NotificationAPIUtils::GetActiveViewExpIncludingExtendedViews(IsAnExtendedView);
            if (! ActiveViewExp){
                DbgAssert(0 &&_T("MaxSDK::NotificationAPIUtils::GetActiveViewExpIncludingExtendedViews(IsAnExtendedView) returned NULL"));
                return;
            }

            int oldviewID           = pViewNotifier->GetMaxViewID();
            const int activeViewID  = ActiveViewExp->GetViewID();
            if (activeViewID != oldviewID){ //Is that the viewport we are monitoring ?
                //No, so the active viewport has changed, notify listeners
                pViewNotifier->UpdateOurStoredData(*ActiveViewExp);
                pViewNotifier->NotifyEvent(ViewEvent::MakeViewActiveEvent(activeViewID));
                pViewNotifier = NULL;//Be careful, the pViewNotifier may be deleted sometimes in previous function, so setting it to NULL not to use it any longer
            }
        }
        break;
    }
}

int MaxViewNotifier::GetMaxViewID()const
{
    return m_ViewUndoID;
}

void MaxViewNotifier::proc(Interface *)
{
    if (m_ViewUndoID >= 0){
        //Is the active view, the one we are monitoring
        bool IsAnExtendedView = false;
        ViewExp* activeViewExp = MaxSDK::NotificationAPIUtils::GetActiveViewExpIncludingExtendedViews(IsAnExtendedView);
        if (! activeViewExp){
            DbgAssert(0 &&_T("MaxSDK::NotificationAPIUtils::GetActiveViewExpIncludingExtendedViews(IsAnExtendedView) returned NULL"));
            return;
        }

        const int activeViewID  = activeViewExp->GetViewID();
        if (activeViewID == m_ViewUndoID){ //Is that the viewport we are monitoring ?

            //We catch the changes from pesp. to ortho views or other way round, but we need to check about switches in the same view from viewport to node view (camera or light) or other way round
            INode* activeViewNode = Utils::GetViewNode(*activeViewExp);//Is NULL if it's a viewport
            if (m_ViewNode == activeViewNode){

                // Grab new properties to see if the camera effectively changed
                Matrix3 newMatrix;
		        activeViewExp->GetAffineTM(newMatrix);
                const BOOL new_persp = activeViewExp->IsPerspView();
                const float new_fov = activeViewExp->GetFOV();
                const float new_focal_dist = activeViewExp->GetFocalDist();

                // See if properties changed
                if((m_StoredMatrix != newMatrix)
                    || (m_view_persp != new_persp)
                    || (m_view_fov != new_fov)
                    || (m_view_focal_dist != new_focal_dist))
                {
                    TSTR vportLabel (_T(""));
                    MaxSDK::NotificationAPIUtils::GetViewportLabelFromUndoIDIncludingExtendedViews(m_ViewUndoID,vportLabel);
                    if (vportLabel.Length() && vportLabel != m_CurrentViewportLabel){
                        m_CurrentViewportLabel = vportLabel; //Update to new label, the view type has changed like we switched from persp to top or a camera
                        NotifyEvent(ViewEvent::MakeViewTypeEvent(m_ViewUndoID));
                    }
                    else{
                        NotifyEvent(ViewEvent::MakeViewPropertiesEvent(Utils::GetViewNode(activeViewExp), m_ViewUndoID));
                    }

                    m_StoredMatrix = newMatrix;
                    m_view_persp = new_persp;
                    m_view_fov = new_fov;
                    m_view_focal_dist = new_focal_dist;
                }
            }else{
                m_ViewNode      = activeViewNode;
                //check if a view type changed from a camera/light to a viewport (persp/top/Left etc) or the other way round
                TSTR vportLabel(_T(""));
                MaxSDK::NotificationAPIUtils::GetViewportLabelFromUndoIDIncludingExtendedViews(m_ViewUndoID, vportLabel);
                if (vportLabel.Length() && vportLabel != m_CurrentViewportLabel) {
                    NotifyEvent(ViewEvent::MakeViewTypeEvent(m_ViewUndoID));
                }
                else {
                    //Consider this as an active view changed event
                    NotifyEvent(ViewEvent::MakeViewActiveEvent(m_ViewUndoID));
                }

                UpdateOurStoredData(*activeViewExp); //Updates m_CurrentViewportLabel among others
            }
        }
    }
}

void MaxViewNotifier::UpdateOurStoredData(ViewExp& pViewExp)
{
    pViewExp.GetAffineTM(m_StoredMatrix);
    m_view_persp = pViewExp.IsPerspView();
    m_view_fov = pViewExp.GetFOV();
    m_view_focal_dist = pViewExp.GetFocalDist();
    MaxSDK::NotificationAPIUtils::GetViewportLabelFromUndoIDIncludingExtendedViews(m_ViewUndoID, m_CurrentViewportLabel);
}

void MaxViewNotifier::DebugPrintToFile(FILE* file, size_t indent)const
{
    if (NULL == file){
		DbgAssert(0 && _T("file is NULL"));
		return;
	}

    TSTR indentString = _T("");
    for (size_t i=0;i<indent;++i){
        indentString += TSTR(_T("\t"));
    }
    _ftprintf(file, indentString);
    _ftprintf(file, _T("** View Notifier data : **\n"));

    _ftprintf(file, indentString);
    if (m_ViewUndoID >= 0){
        bool IsAnExtendedView = false;
        ViewExp* pViewExp = MaxSDK::NotificationAPIUtils::GetViewExpFromUndoIDIncludingExtendedViews(m_ViewUndoID, IsAnExtendedView);//m_ViewUndoID is the view undo ID

        if (m_CurrentViewportLabel.Length()){
            _ftprintf(file, indentString);
            _ftprintf(file, _T("** Viewport label name : %s **\n"),m_CurrentViewportLabel.data());
        }

        if (pViewExp){
            _ftprintf(file, indentString);
            if (IsAnExtendedView){
                _ftprintf(file, _T("** View is an extended view **\n"));
            }else{
                _ftprintf(file, _T("** View is not an extended view **\n"));
            }
            INode* pCamNode = pViewExp->GetViewCamera();
            if (pCamNode){
                _ftprintf(file, indentString);
                _ftprintf(file, _T("** View : is camera named \"%s\" **\n"), pCamNode->GetName() );
            }else{
                //Is a viewport
                if (pViewExp->IsPerspView()){
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("** View : is a perspective viewport **\n"));
                }else{
                    _ftprintf(file, indentString);
                    _ftprintf(file, _T("** View : is an orthographic viewport **\n"));
                }
            }
        }
    }
    else{
        _ftprintf(file, indentString);
        _ftprintf(file, _T("** View : None (ID < 0) **\n"));
    }
    
    //Print base class
   __super::DebugPrintToFile(file, ++indent);
}

} } // namespaces
    