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


namespace Max
{;
namespace NotificationAPI
{;
using namespace MaxSDK::NotificationAPI;



	class MaxSceneNotifier : public MaxNotifier
	{
    protected:
        static void ProcessNotifications(void *param, NotifyInfo *info); //To receive events from RegisterNotification(ProcessNotifications, this, NOTIFY_NODE_CREATED);

	public:
		MaxSceneNotifier();

        void Notify_SceneNodeAdded(INode& node);
        void Notify_SceneNodeRemoved(INode& node);

        //From MaxNotifier
        virtual void DebugPrintToFile(FILE* f, size_t indent)const;

    protected:

        // Protected destructor forces going through delete_this()
        virtual ~MaxSceneNotifier();

	};


} } // namespaces
