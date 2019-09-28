/**********************************************************************
 *<
	FILE: procmaps.h

	DESCRIPTION:

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __PROCMAPS__H
#define __PROCMAPS__H

#include "max.h"
#include "imtl.h"
#include "stdmat.h"
#include "texutil.h"
#include "buildver.h"

extern HINSTANCE hInstance;

extern ClassDesc* GetPlanetDesc();
extern ClassDesc* GetStuccoDesc();
extern ClassDesc* GetSplatDesc();
extern ClassDesc* GetWaterDesc();
extern ClassDesc* GetSpeckleDesc();
extern ClassDesc* GetSmokeDesc();

extern TCHAR *GetString(int id);

#endif
