//
// Copyright [2015] Autodesk, Inc.  All rights reserved. 
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc. and
// are protected under applicable copyright and trade secret law.  They may
// not be disclosed to, copied or used by any third party without the prior
// written consent of Autodesk, Inc.
//

#include "MultiTileFP.h"
#include "MultiTile.h"

namespace MultiTileMap
{

enum PublishedEnum
{
	TilePatternFormat = 0,
	ViewportQuality,
};

FPInterfaceDesc InterfaceDesc(
	MULTITILEMAP_INTERFACE, // ID,
	_T("MultiTiles"),       // int_name
	0,                      // local_name
	GetMultiTileDesc(),     // ClassDesc
	FP_MIXIN,               // flags

	// ID, int_name, local_name, return type, flags, num args,
	//     arg_name, flags, arg type
	FunctionId::SetPatternedImageFile, _T("setPatternedImageFile"), 0, TYPE_bool, 0, 1,
		_T("file path"), 0, TYPE_FILENAME,
	FunctionId::SetImageFile, _T("setImageFile"), 0, TYPE_bool, 0, 2,
		_T("tile index"), 0, TYPE_INDEX,
		_T("file path"), 0, TYPE_FILENAME,

	FunctionId::GetTileU, _T("getTileUOffset"), 0, TYPE_INT, 0, 1,
		_T("tile index"), 0, TYPE_INDEX,
	FunctionId::SetTileU, _T("setTileUOffset"), 0, TYPE_VOID, 0, 2,
		_T("tile index"), 0, TYPE_INDEX,
		_T("u index"), 0, TYPE_INT,
	FunctionId::GetTileV, _T("getTileVOffset"), 0, TYPE_INT, 0, 1,
		_T("tile index"), 0, TYPE_INDEX,
	FunctionId::SetTileV, _T("setTileVOffset"), 0, TYPE_VOID, 0, 2,
		_T("tile index"), 0, TYPE_INDEX,
		_T("v index"), 0, TYPE_INT,
	FunctionId::GetTile, _T("getTileTexmap"), 0, TYPE_TEXMAP, 0, 1,
		_T("tile index"), 0, TYPE_INDEX,
	FunctionId::SetTile, _T("setTileTexmap"), 0, TYPE_VOID, 0, 2,
		_T("tile index"), 0, TYPE_INDEX,
		_T("texmap"), 0, TYPE_TEXMAP,
	FunctionId::CountTiles, _T("tileCount"), 0, TYPE_INT, 0, 0, 
	FunctionId::AddTile, _T("addTile"), 0, TYPE_VOID, 0, 0, 
	FunctionId::DeleteTile, _T("deleteTile"), 0, TYPE_VOID, 0, 1, 
		_T("tile index"), 0, TYPE_INDEX,

	properties,
		FunctionId::GetPatternFormat, FunctionId::SetPatternFormat, _T("PatternFormat"), 0, TYPE_ENUM, PublishedEnum::TilePatternFormat,
		FunctionId::GetViewportQuality, FunctionId::SetViewportQuality, _T("ViewportQuality"), 0, TYPE_ENUM, PublishedEnum::ViewportQuality,

	enums,
		PublishedEnum::TilePatternFormat, TilePatternFormat::Num,
			_T("ZBrush"), TilePatternFormat::ZBrush,
			_T("Mudbox"), TilePatternFormat::Mudbox,
			_T("UDIM"), TilePatternFormat::UDIM,
			_T("Custom"), TilePatternFormat::Custom,
		PublishedEnum::ViewportQuality, ViewportQuality::Num,
			_T("Low"), ViewportQuality::Low,
			_T("Middle"), ViewportQuality::Middle,
			_T("High"), ViewportQuality::High,
	p_end
);

FPInterfaceDesc* Interface::GetDesc() 
{
   return &InterfaceDesc;
}

}

