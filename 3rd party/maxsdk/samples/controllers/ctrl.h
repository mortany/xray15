/**********************************************************************
 *<
	FILE: ctrl.h

	DESCRIPTION:

	CREATED BY: Rolf Berteig

	HISTORY: created 13 June 1995

	         added independent scale controller (ScaleXYZ)
			   mjm - 9.15.98

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __CTRL__H
#define __CTRL__H

#include "Max.h"
#include "resource.h"


extern ClassDesc* GetPathCtrlDesc();
extern ClassDesc* GetEulerCtrlDesc();
extern ClassDesc* GetLocalEulerCtrlDesc();
extern ClassDesc* GetAdditiveEulerCtrlDesc();
extern ClassDesc* GetExprPosCtrlDesc();
extern ClassDesc* GetExprP3CtrlDesc();
extern ClassDesc* GetExprFloatCtrlDesc();
extern ClassDesc* GetExprScaleCtrlDesc();
extern ClassDesc* GetExprRotCtrlDesc();
extern ClassDesc* GetFloatNoiseDesc();
extern ClassDesc* GetPositionNoiseDesc();
extern ClassDesc* GetPoint3NoiseDesc();
extern ClassDesc* GetRotationNoiseDesc();
extern ClassDesc* GetScaleNoiseDesc();
extern ClassDesc* GetBoolControlDesc();
extern ClassDesc* GetIPosCtrlDesc();
extern ClassDesc* GetAttachControlDesc();
extern ClassDesc* GetIPoint3CtrlDesc();
extern ClassDesc* GetIColorCtrlDesc();
extern ClassDesc* GetLinkCtrlDesc();
extern ClassDesc* GetLinkTimeCtrlDesc();
extern ClassDesc* GetFollowUtilDesc();
extern ClassDesc* GetSurfCtrlDesc();
extern ClassDesc* GetLODControlDesc();
extern ClassDesc* GetLODUtilDesc();
extern ClassDesc* GetIScaleCtrlDesc();  // mjm 9.15.98
extern ClassDesc* GetPosConstDesc(); // AG added for Position Constraint, 4/21/2000
extern ClassDesc* GetOrientConstDesc(); // AG added for Orientation Constraint, 5/04/2000
extern ClassDesc* GetLookAtConstDesc(); // AG added for LookAt Constraint, 6/26/2000
extern ClassDesc* GetFloatListDesc();
extern ClassDesc* GetPoint3ListDesc();
extern ClassDesc* GetPositionListDesc();
extern ClassDesc* GetRotationListDesc();
extern ClassDesc* GetScaleListDesc();
extern ClassDesc* GetMasterListDesc();
extern ClassDesc* GetIPoint4CtrlDesc();
extern ClassDesc* GetIAColorCtrlDesc();
extern ClassDesc* GetPoint4ListDesc();
extern ClassDesc* GetNodeTransformMonitorDesc();
extern ClassDesc* GetNodeMonitorDesc();

extern ClassDesc* GetFloatLayerDesc();
extern ClassDesc* GetPoint3LayerDesc();
extern ClassDesc* GetPositionLayerDesc();
extern ClassDesc* GetRotationLayerDesc();
extern ClassDesc* GetScaleLayerDesc();
extern ClassDesc* GetPoint4LayerDesc();
extern ClassDesc* GetMasterLayerDesc();
extern ClassDesc* GetMasterLayerControlManagerDesc();
extern ClassDesc* GetLayerOutputDesc();
extern ClassDesc* GetRefTargMonitorDesc();

TCHAR *GetString(int id);
extern HINSTANCE hInstance;

extern int computeHorizontalExtent(HWND hListBox, 
			BOOL useTabs=FALSE, int cTabs=0, LPINT lpnTabs=NULL);
#endif

