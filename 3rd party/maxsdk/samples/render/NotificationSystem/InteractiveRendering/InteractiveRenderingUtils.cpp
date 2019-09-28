/******************************************************************************
* Copyright 2013 Autodesk, Inc.  All rights reserved. 
* This computer source code and related instructions and comments are the 
* unpublished confidential and proprietary information of Autodesk, Inc. and 
* are protected under applicable copyright and trade secret law.  They may not 
* be disclosed to, copied or used by any third party without the prior written 
* consent of Autodesk, Inc.
******************************************************************************/
//Author : David Lanier

#include "InteractiveRenderingUtils.h"

// max sdk
#include <maxapi.h>
#include <render.h>
#include <NotificationAPI/NotificationAPIUtils.h>

using namespace Max::NotificationAPI;

bool Utils::IsActiveShadeViewLocked(void)
{
	// If we are not using the active view that means we are locked to another view
	return (!MaxSDK::NotificationAPIUtils::IsUsingActiveView(RS_IReshade));
}

void Utils::GetViewParams(ViewParams& outViewParams, ViewExp &viewportExp)
{
	outViewParams.projType = viewportExp.IsPerspView() ? PROJ_PERSPECTIVE : PROJ_PARALLEL;
	int perspective;
	float mat[4][4], hither, yon;
	Matrix3 invTM;
	viewportExp.getGW()->getCameraMatrix(mat, &invTM, &perspective, &hither, &yon);
	outViewParams.hither = hither;
	outViewParams.yon = yon;
	if (outViewParams.projType==PROJ_PERSPECTIVE)
	{
		outViewParams.fov = viewportExp.GetFOV();
	}
	else
	{
		const float view_default_width = 400.0;
		GraphicsWindow* gw = viewportExp.getGW();
		outViewParams.zoom = viewportExp.GetScreenScaleFactor(
			Point3(0.0f, 0.0f, 0.0f));
		outViewParams.zoom *= gw->getWinSizeX() / ( gw->getWinSizeY() * view_default_width);
	}
	viewportExp.GetAffineTM(outViewParams.affineTM);
	outViewParams.prevAffineTM = outViewParams.affineTM;
}

