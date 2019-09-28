#pragma once

//**************************************************************************/
// Copyright (c) 2013 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/
// AUTHOR: David Lanier
//***************************************************************************/

#include <ref.h>
#include <matrix3.h>
#include <NotificationAPI/NotificationAPI_Events.h>

#ifdef NOTIFICATION_SYSTEM_MODULE
    #define NotificationAPIExport __declspec(dllexport)
#else
    #define NotificationAPIExport __declspec(dllimport)
#endif

class ViewExp;

namespace Max
{;
namespace NotificationAPI
{;
namespace Utils
{;
using namespace MaxSDK::NotificationAPI;

    void                        DebugPrintToFileEventType       (FILE* file, NotifierType notifierType, size_t updateType, size_t indent);
    void                        DebugPrintToFileNotifierType    (FILE* theFile, NotifierType type, RefTargetHandle, size_t indent);
    void                        DebugPrintToFileMonitoredEvents (FILE* file,    NotifierType notifierType, size_t monitoredEvents, size_t indent);
    INode*                      GetViewNode                     (ViewExp &viewportExp);
    INode*                      GetViewNode                     (ViewExp *viewportExp);
    void                        DebugPrintParamBlockData        (FILE* file, const ParamBlockData& pblockData, size_t indent);
    //GetLastUpdatedParamFromObject function
    //result is set in outResult, it is true if there were one parameter which was updated. false if none of the parameters was updated (when false, all ParamBlockData arrays are empty and type == UNKNOWN)
    //referenceTarget may be coming from a INode* , Mtl* , Texmpa* of whatever, 
    //outParamblockData is going to be filled by the function when outResult is true
    void                        GetLastUpdatedParamFromObject   (bool& outResult, ReferenceTarget& referenceTarget, ParamBlockData& outParamblockData);
    //Merge content of otherParamBlockData into inoutParamBlockData
    void                        MergeParamBlockDatas            (MaxSDK::Array<ParamBlockData>& inoutParamBlockDatas, const MaxSDK::Array<ParamBlockData>& otherParamBlockDatas); 
};//end of namespace Utils
};//end of namespace NotificationAPI
};//end of namespace Max