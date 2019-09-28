/**********************************************************************
 *<
	FILE: mtlhdr.h

	DESCRIPTION:

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __MTLHDR__H
#define __MTLHDR__H

#ifdef BLD_MTL
#define MtlExport __declspec( dllexport )
#else
#define MtlExport __declspec( dllimport )
#endif

#include "max.h"
#include "imtl.h"
#include "texutil.h"
#include "buildver.h"
#include "AssetManagement\iassetmanager.h"
#include "composite.h"
#include "MultiTile.h"
#include "color_correction.h"

extern ClassDesc* GetStdMtl2Desc();
extern ClassDesc* GetBMTexDesc();
extern ClassDesc* GetTexmapsDesc();
extern ClassDesc* GetOldTexmapsDesc();
extern ClassDesc* GetCMtlDesc();
extern ClassDesc* GetCheckerDesc();
extern ClassDesc* GetMixDesc();
extern ClassDesc* GetMarbleDesc();
extern ClassDesc* GetMaskDesc();
extern ClassDesc* GetTintDesc();
extern ClassDesc* GetNoiseDesc();
extern ClassDesc* GetMultiDesc();
extern ClassDesc* GetDoubleSidedDesc();
extern ClassDesc* GetMixMatDesc();
extern ClassDesc* GetEmptyMultiDesc();
extern ClassDesc* GetACubicDesc();
extern ClassDesc* GetMirrorDesc();
extern ClassDesc* GetGradientDesc();
extern ClassDesc* GetMatteDesc();
extern ClassDesc* GetRGBMultDesc();
extern ClassDesc* GetOutputDesc();
extern ClassDesc* GetFalloffDesc();
extern ClassDesc* GetVColDesc();
extern ClassDesc* GetPlateDesc();
extern ClassDesc* GetPartAgeDesc();
extern ClassDesc* GetPartBlurDesc();
extern ClassDesc* GetCompositeMatDesc();
extern ClassDesc* GetCellTexDesc();

// old shaders are here, mostly to guarantee the existance of the default shader
extern ClassDesc* GetConstantShaderCD();
extern ClassDesc* GetPhongShaderCD();
extern ClassDesc* GetMetalShaderCD();
extern ClassDesc* GetBlinnShaderCD();
extern ClassDesc* GetBakeShellDesc();

TCHAR *GetString(int id);

#endif
