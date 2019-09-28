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

class ViewExp;
class ViewParams;
class INode;

namespace Max
{;
namespace NotificationAPI
{;

//Misc. utilities functions
namespace Utils
{
    INode*  GetViewNode                 (ViewExp &viewportExp);
    bool    IsActiveShadeViewLocked     (void);
    void    GetViewParams               (ViewParams& outViewParams, ViewExp &viewportExp);
};


}}; //end of namespace 