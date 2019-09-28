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

#include "Max.h"
#include "TrackViewFunctions.h"

BOOL OpenTrackViewWindow(const TCHAR *name)
{
	ITrackViewArray * parray = (ITrackViewArray*)GetCOREInterface(ITRACKVIEWS);
	return parray->OpenTrackViewWindow(name);
}

int GetNumAvailableTrackViews()
{
	ITrackViewArray * parray = (ITrackViewArray*)GetCOREInterface(ITRACKVIEWS);
	return parray->GetNumAvailableTrackViews();
}

BOOL CloseTrackView(int index)
{
	ITrackViewArray * parray = (ITrackViewArray*)GetCOREInterface(ITRACKVIEWS);
	return parray->CloseTrackView(index);
}

BOOL IsTrackViewOpen(int index)
{
	ITrackViewArray * parray = (ITrackViewArray*)GetCOREInterface(ITRACKVIEWS);
	return parray->IsTrackViewOpen(index);
}

void DeleteTrackView(int index)
{
	ITrackViewArray * parray = (ITrackViewArray*)GetCOREInterface(ITRACKVIEWS);
	parray->DeleteTrackView(index);
}

ITreeViewOps *GetTrackView(const TCHAR *name)
{
	ITrackViewArray * parray = (ITrackViewArray*)GetCOREInterface(ITRACKVIEWS);
	return parray->GetTrackView(name);
}

ITreeViewOps *GetTrackView(int index)
{
	ITrackViewArray * parray = (ITrackViewArray*)GetCOREInterface(ITRACKVIEWS);
	return parray->GetTrackView(index);
}

void ITreeViewUI_ShowControllerWindow(ITreeViewOps *pTreeViewOps)
{
	pTreeViewOps->fpGetUIInterface()->ShowTrackWindow(TRUE);
}
