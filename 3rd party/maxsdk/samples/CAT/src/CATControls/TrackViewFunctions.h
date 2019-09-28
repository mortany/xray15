//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2009 Autodesk, Inc.  All rights reserved.
//  Copyright 2003 Character Animation Technologies.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "tvnode.h"
#include "itreevw.h"

#define CAT_TRACKVIEW_TVCLSID		Class_ID(0xCa7Ca701, 0x23Ca7Ca7)

#ifdef _DEBUG
#define CAT_HIDE_TV_ROOT_TRACKS		FALSE
#else
#define CAT_HIDE_TV_ROOT_TRACKS		TRUE
#endif

extern BOOL OpenTrackViewWindow(const TCHAR *name);
extern int GetNumAvailableTrackViews();
extern BOOL CloseTrackView(int index);
extern BOOL IsTrackViewOpen(int index);
extern void DeleteTrackView(int index);

extern ITreeViewOps *GetTrackView(const TCHAR *name);
extern ITreeViewOps *GetTrackView(int index);

extern void ITreeViewUI_ShowControllerWindow(ITreeViewOps *pTreeViewOps);
