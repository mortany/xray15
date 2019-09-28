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

#pragma once

#include "Notifiers/MaxNotifier.h"
#include <maxapi.h>

namespace Max
{;
namespace NotificationAPI
{;
using namespace MaxSDK::NotificationAPI;


    //Used to get notified when the viewport TM changes
	class MaxViewNotifier : public MaxNotifier, public RedrawViewsCallback 
	{
	public:
		MaxViewNotifier(int viewID);

        int GetMaxViewID()const;

        //From MaxNotifier
        virtual void DebugPrintToFile(FILE* f, size_t indent)const;

        //From RedrawViewsCallback
		virtual void proc(Interface *ip);

        static void ProcessNotifications(void *param, NotifyInfo *info);

    protected:
        // Protected destructor forces going through delete_this()
        virtual ~MaxViewNotifier();

    protected:
        int     				m_ViewUndoID;
        INode*                  m_ViewNode;//Used to check if we have switched within the same view from a viewport to a camera/light or the other way round
        TSTR                    m_CurrentViewportLabel;//Used to track any changes of view inside the same viewport, say from perspective to Left by using the keyboard shortcuts

        // Set of properties which are monitored for change
        Matrix3                 m_StoredMatrix;
        BOOL m_view_persp;
        float m_view_fov;
        float m_view_focal_dist;

    private:

        virtual void UpdateOurStoredData(ViewExp& pViewExp);

	};


} } // namespaces
