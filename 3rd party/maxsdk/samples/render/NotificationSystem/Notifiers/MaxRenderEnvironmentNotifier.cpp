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

#include "MaxRenderEnvironmentNotifier.h"

#include "Events/GenericEvent.h"

namespace Max
{;
namespace NotificationAPI
{;
using namespace MaxSDK::NotificationAPI;


MaxRenderEnvironmentNotifier::MaxRenderEnvironmentNotifier()
{
	
}

MaxRenderEnvironmentNotifier::~MaxRenderEnvironmentNotifier()
{
}

void MaxRenderEnvironmentNotifier::NotifyEvent_BackgroundColor()
{
    NotifyEvent(GenericEvent(NotifierType_RenderEnvironment, EventType_RenderEnvironment_BackgroundColor));
}

void MaxRenderEnvironmentNotifier::NotifyEvent_EnvironmentMap()
{
    NotifyEvent(GenericEvent(NotifierType_RenderEnvironment, EventType_RenderEnvironment_EnvironmentMap));
}

void MaxRenderEnvironmentNotifier::NotifyEvent_EnvironmentMapState()
{
    NotifyEvent(GenericEvent(NotifierType_RenderEnvironment, EventType_RenderEnvironment_EnvironmentMapState));
}

void MaxRenderEnvironmentNotifier::DebugPrintToFile(FILE* file, size_t indent)const
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
    _ftprintf(file, _T("** Render Environment Notifier data : **\n"));
    
    //Print base class
   __super::DebugPrintToFile(file, ++indent);
}

} } // namespaces
