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


	class MaxRenderEnvironmentNotifier : public MaxNotifier
	{
	public:
		MaxRenderEnvironmentNotifier();

        void NotifyEvent_BackgroundColor();
        void NotifyEvent_EnvironmentMap();
        void NotifyEvent_EnvironmentMapState();

        //From MaxNotifier
        virtual void DebugPrintToFile(FILE* f, size_t indent)const;

    protected:
        // Protected destructor forces going through delete_this()
        virtual ~MaxRenderEnvironmentNotifier();

	};


} } // namespaces
