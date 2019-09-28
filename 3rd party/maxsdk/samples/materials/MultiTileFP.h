//
// Copyright [2015] Autodesk, Inc.  All rights reserved. 
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc. and
// are protected under applicable copyright and trade secret law.  They may
// not be disclosed to, copied or used by any third party without the prior
// written consent of Autodesk, Inc.
//

#ifndef __MULTITILE_FP_H__
#define __MULTITILE_FP_H__

#include "iFnPub.h"
#include "iparamm2.h"
#include "IMultiTile.h"

namespace MultiTileMap
{
	enum class FunctionId
	{
		SetPatternedImageFile,
		SetImageFile,
		GetTileU,
		SetTileU,
		GetTileV,
		SetTileV,
		GetTile,
		SetTile,
		CountTiles,
		AddTile,
		DeleteTile,
		GetPatternFormat,
		SetPatternFormat,
		GetViewportQuality,
		SetViewportQuality,
	};

	struct Interface : public FPMixinInterface 
	{
		BEGIN_FUNCTION_MAP
			FN_1( FunctionId::SetPatternedImageFile, TYPE_bool, MXS_SetPatternedImageFile, TYPE_FILENAME );
			FN_2( FunctionId::SetImageFile, TYPE_bool, MXS_SetImageFile, TYPE_INDEX, TYPE_FILENAME );
			FN_1( FunctionId::GetTileU, TYPE_INT, MXS_GetTileU, TYPE_INDEX );
			VFN_2( FunctionId::SetTileU, MXS_SetTileU, TYPE_INDEX, TYPE_INT );
			FN_1( FunctionId::GetTileV, TYPE_INT, MXS_GetTileV, TYPE_INDEX );
			VFN_2( FunctionId::SetTileV, MXS_SetTileV, TYPE_INDEX, TYPE_INT );
			FN_1( FunctionId::GetTile, TYPE_TEXMAP, MXS_GetTile, TYPE_INDEX );
			VFN_2( FunctionId::SetTile, MXS_SetTile, TYPE_INDEX, TYPE_TEXMAP );
			FN_0( FunctionId::CountTiles, TYPE_INT, MXS_Count );
			VFN_0( FunctionId::AddTile, MXS_AddTile);
			VFN_1( FunctionId::DeleteTile, MXS_DeleteTile, TYPE_INDEX );
			PROP_FNS( FunctionId::GetPatternFormat, MXS_GetPatternFormat, FunctionId::SetPatternFormat, MXS_SetPatternFormat, TYPE_ENUM );
			PROP_FNS( FunctionId::GetViewportQuality, MXS_GetViewportQuality, FunctionId::SetViewportQuality, MXS_SetViewportQuality, TYPE_ENUM );
		END_FUNCTION_MAP

		// functions, prefixing with MXS_ to prevent name clash
		virtual bool MXS_SetPatternedImageFile(const MCHAR*) = 0;
		virtual bool MXS_SetImageFile(int, const MCHAR*) = 0;
		virtual int MXS_GetTileU(int) = 0;
		virtual void MXS_SetTileU(int, int) = 0;
		virtual int MXS_GetTileV(int) = 0;
		virtual void MXS_SetTileV(int, int) = 0;
		virtual Texmap* MXS_GetTile(int) = 0;
		virtual void MXS_SetTile(int, Texmap*) = 0;
		virtual int MXS_Count() = 0;
		virtual void MXS_AddTile() = 0;
		virtual void MXS_DeleteTile(int) = 0;

		// props
		virtual int MXS_GetPatternFormat() = 0;
		virtual void MXS_SetPatternFormat(int) = 0;
		virtual int MXS_GetViewportQuality() = 0;
		virtual void MXS_SetViewportQuality(int) = 0;

		FPInterfaceDesc* GetDesc();
	};
}

#endif // __MULTITILE_FP_H__