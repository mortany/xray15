/******************************************************************************
* Copyright 2013 Autodesk, Inc.  All rights reserved. 
* This computer source code and related instructions and comments are the 
* unpublished confidential and proprietary information of Autodesk, Inc. and 
* are protected under applicable copyright and trade secret law.  They may not 
* be disclosed to, copied or used by any third party without the prior written 
* consent of Autodesk, Inc.
******************************************************************************/
//Author : David Lanier

#include "InteractiveRenderingViewData.h"

// max sdk
#include <strclass.h>
#include <inode.h>

using namespace Max::NotificationAPI;

//-------------------------------------------------------------------
// ----------			class InteractiveRenderingViewData	---------
//-------------------------------------------------------------------

InteractiveRenderingViewData::InteractiveRenderingViewData()
{
    m_ViewID                            = -1;
    m_CameraOrLightNode                 = NULL;
}


InteractiveRenderingViewData::InteractiveRenderingViewData(int viewID, INode* pCameraOrLightNode)
{
    UpdateViewAndNode(viewID, pCameraOrLightNode);
}
		
InteractiveRenderingViewData::~InteractiveRenderingViewData()
{
    m_ViewID                            = -1;
    m_CameraOrLightNode                 = NULL;
}

void InteractiveRenderingViewData::UpdateViewAndNode(int viewID, INode* pCameraOrLightNode)
{
    DbgAssert(viewID >= 0);
    m_ViewID                            = viewID;
    m_CameraOrLightNode                 = pCameraOrLightNode;//May be NULL if we are monitoring a viewport
}

void InteractiveRenderingViewData::DebugPrintToFile(FILE* file)const
{
    if (! file){
        DbgAssert(0 &&_T("file is NULL"));
        return;
    }

    _ftprintf(file, _T("** InteractiveRenderingViewData debugging **\n"));

	size_t indent = 1; //Start with one tab char
    TSTR indentString = _T("");
    for (size_t i=0;i<indent;++i){
        indentString += TSTR(_T("\t"));
    }
    
    _ftprintf(file, indentString);
    _ftprintf(file, _T("** m_ViewID : %d **\n"), m_ViewID);

    _ftprintf(file, indentString);
    if (m_CameraOrLightNode){
        _ftprintf(file, _T("** m_CameraOrLightNode name : %s **\n"), m_CameraOrLightNode->GetName());
    }else{
        _ftprintf(file, _T("** m_CameraOrLightNode : NULL (Is a viewport) **\n"));
    }
    
}
