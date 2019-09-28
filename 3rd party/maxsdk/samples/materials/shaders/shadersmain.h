/**********************************************************************
 *<
	FILE: shadersMain.h

	DESCRIPTION:

	CREATED BY: Kells Elmquist

	HISTORY:

 *>	Copyright (c) 1999, All Rights Reserved.
 **********************************************************************/

#ifndef __SHADERS_MAIN__H
#define __SHADERS_MAIN__H
#include "buildver.h"

extern ClassDesc* GetOrenNayarShaderCD();
extern ClassDesc* GetOrenNayarBlinnShaderCD();
extern ClassDesc* GetWardShaderCD();
extern ClassDesc* GetAnisoShaderCD();
extern ClassDesc* GetLaFortuneShaderCD();
extern ClassDesc* GetLayeredBlinnShaderCD();
extern ClassDesc* GetPhysicalBlinnShaderCD();
extern ClassDesc* GetCookShaderCD();
extern ClassDesc* GetStraussShaderCD();
extern ClassDesc* GetCompositeMatDesc();
extern ClassDesc * GetSchlickShaderCD();
extern ClassDesc * GetMultiLayerShaderCD();
extern ClassDesc * GetTranslucentShaderCD();

TCHAR *GetString(int id);

#endif
