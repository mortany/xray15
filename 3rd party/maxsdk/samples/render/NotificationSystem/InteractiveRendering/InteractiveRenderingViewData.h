/******************************************************************************
* Copyright 2013 Autodesk, Inc.  All rights reserved. 
* This computer source code and related instructions and comments are the 
* unpublished confidential and proprietary information of Autodesk, Inc. and 
* are protected under applicable copyright and trade secret law.  They may not 
* be disclosed to, copied or used by any third party without the prior written 
* consent of Autodesk, Inc.
******************************************************************************/
//Author : David Lanier

#pragma once

// Max SDK
#include <Noncopyable.h>
#include <assert1.h>
// STD
#include <stdio.h>

class INode;

namespace Max
{;
namespace NotificationAPI
{;

//Events monitored for camera or light viewports
#define IR_CAM_OR_LIGHT_MONITORED_EVENTS  Max::NotificationAPI::EventType_Node_ParamBlock |\
		                                  Max::NotificationAPI::EventType_Node_Transform  |\
		                                  Max::NotificationAPI::EventType_Node_Deleted

//Events monitored for viewports (non camera or lights)
#define IR_VIEWPORT_MONITORED_EVENTS Max::NotificationAPI::EventType_View_Properties  | Max::NotificationAPI::EventType_View_Active

class InteractiveRenderingViewData  : public MaxSDK::Util::Noncopyable
{
protected:
    int         m_ViewID;
    INode*      m_CameraOrLightNode;//May be NULL if we are monitoring a viewport

public:
    InteractiveRenderingViewData();//default constructor
    InteractiveRenderingViewData(int viewId, INode* pCameraOrLightNode);//!< Constructor
    virtual ~InteractiveRenderingViewData();//!< Destructor
    
    //Get 
    virtual int     GetViewID                       ()const{return m_ViewID;}
    virtual INode*  GetCameraOrLightNode            ()const{return m_CameraOrLightNode;}
    virtual bool    IsAViewport                     ()const{DbgAssert(m_ViewID >= 0); return NULL == m_CameraOrLightNode;}//Should be initialized
    
    //Set
    virtual void    UpdateViewAndNode   (int viewId, INode* pCameraOrLightNode);//When we change the monitored view to another view, we update the data

    virtual void DebugPrintToFile(FILE* file)const;
};


}}; //end of namespace 