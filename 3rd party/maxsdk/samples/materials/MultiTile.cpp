//
// Copyright [2015] Autodesk, Inc.  All rights reserved. 
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc. and
// are protected under applicable copyright and trade secret law.  They may
// not be disclosed to, copied or used by any third party without the prior
// written consent of Autodesk, Inc.
//

#pragma region Includes

// Max headers
#include "MultiTile.h"
#include "MultiTileDlg.h"
#include "MultiTileLayerDlg.h"
#include "FilePatternParser.h"
#include "mtlhdr.h"
#include "mtlres.h"
#include "stdmat.h"
#include "iparamm2.h"
#include "iFnPub.h"

// Custom headers
#include "util.h"

// Standard headers
#include <list>
#include <tchar.h>
#include <windows.h>
#include <tuple>
#include <sstream>

#pragma endregion

////////////////////////////////////////////////////////////////////////////////

ClassDesc2* GetMultiTileDesc() {
	static MultiTileMap::Desc ClassDescInstance;
	return &ClassDescInstance;
}

using namespace MaxSDK::Graphics;
////////////////////////////////////////////////////////////////////////////////

namespace MultiTileMap
{

enum class ChunkID : USHORT
{
	Header,
	PatternFormat,
	PatternPrefix,
	ViewportQuality
};

#pragma region Restore Objects

class AddTileRestoreObject: public RestoreObj 
{
	Texture *m_Texture;

	AddTileRestoreObject() { }

public:
	AddTileRestoreObject( Texture *texture )
		: m_Texture( texture )
	{
	
	}

	~AddTileRestoreObject() {}

	void Restore( int isUndo ) override
	{
		if (isUndo) {
			if ( m_Texture ) {
				Dialog *paramDlg = m_Texture->m_ParamDialog;
				Composite::Texture *compTex = m_Texture->m_CompositeTexture;
				if ( paramDlg && compTex ) {
					int index = static_cast<int>(compTex->nb_layer() - 1);
					auto it = paramDlg->find( index );
					if ( it != paramDlg->end() ) {
						paramDlg->DeleteRollup( it->Layer() );
						paramDlg->EnableDisableInterface();
					}
				}
			}
		}
	}

	void Redo() override
	{
		if ( m_Texture ) {
			Dialog *paramDlg = m_Texture->m_ParamDialog;
			Composite::Texture *compTex = m_Texture->m_CompositeTexture;
			if ( paramDlg && compTex ) {
				auto layer = compTex->LayerIterator( compTex->nb_layer() - 1 );
				paramDlg->CreateRollup( &*layer );
				paramDlg->EnableDisableInterface();
			}
		}
	}

	TSTR Description() override 
	{
		return(_T("MultiTileTexmapAddTileRestore"));
	}
};

class DeleteTileBeforeRestoreObject: public RestoreObj {
	Texture *m_Texture;
	size_t m_TileIndex;

	DeleteTileBeforeRestoreObject() { }

public:
	DeleteTileBeforeRestoreObject( Texture *texture, size_t tileIndex )
		: m_Texture( texture )
		, m_TileIndex( tileIndex )
	{
	
	}

	~DeleteTileBeforeRestoreObject() { }

	void Restore(int isUndo) override
	{
		if (isUndo) {
			if ( m_Texture ) {
				Dialog *paramDlg = m_Texture->m_ParamDialog;
				Composite::Texture *compTex = m_Texture->m_CompositeTexture;
				if ( paramDlg && compTex ) {
					auto layer = compTex->LayerIterator ( m_TileIndex );
					paramDlg->CreateRollup( &*layer );
					paramDlg->EnableDisableInterface();
				}
			}
		}
	}

	void Redo() override
	{
		if ( m_Texture ) {
			Dialog *paramDlg = m_Texture->m_ParamDialog;
			if ( paramDlg ) {
				auto it = paramDlg->find( static_cast<int>(m_TileIndex) );
				if ( it != paramDlg->end() ) {
					paramDlg->DeleteRollup( it->Layer() );
				}
			}
		}
	}

	TSTR Description() override
	{
		return(_T("MultiTileTexmapDeleteTileBeforeRestore"));
	}
};

class DeleteTileAfterRestoreObject: public RestoreObj {
	Texture *m_Texture;

	DeleteTileAfterRestoreObject() { }

public:
	DeleteTileAfterRestoreObject( Texture *texture )
		: m_Texture( texture )
	{
	
	}

	~DeleteTileAfterRestoreObject() { }

	void Restore(int isUndo) override
	{
		// do nothing
	}

	void Redo() override
	{
		if ( m_Texture ) {
			Dialog *paramDlg = m_Texture->m_ParamDialog;
			if ( paramDlg ) {
				paramDlg->Rewire();
			}
		}
	}

	TSTR Description() override
	{
		return(_T("MultiTileTexmapDeleteTileAfterRestore"));
	}
};

class ResetTileRestoreObject: public RestoreObj {
	Texture *m_Texture;
	TSTR m_MapName;
	_tstring m_PrevFilePrefix;

	static const int layer_zero = 0;

	ResetTileRestoreObject() { }

public:
	ResetTileRestoreObject( Texture *texture )
		: m_Texture( texture )
		, m_MapName( TSTR() )
		, m_PrevFilePrefix( _tstring() )
	{
		if ( m_Texture ) {
			auto a = m_Texture->m_CompositeTexture->begin();
			auto c = a->Texture();
			auto b = m_Texture->m_CompositeTexture->rbegin();
			auto d = b->Texture();
			Texmap *tex = m_Texture->GetSubTexmap(layer_zero);
			BitmapTex *bmTex = dynamic_cast<BitmapTex*>( tex );
			if ( bmTex ) {
				m_MapName = bmTex->GetMapName();
			}

			m_PrevFilePrefix = m_Texture->m_PatternedFilePrefix;
		}
	}

	~ResetTileRestoreObject() { }

	void Restore(int isUndo) override
	{
		if (isUndo) {
			if ( m_Texture ) {
				std::swap( m_PrevFilePrefix, m_Texture->m_PatternedFilePrefix );
				Dialog *paramDlg = m_Texture->m_ParamDialog;
				if ( paramDlg ) {
					Dialog::iterator it = paramDlg->find( layer_zero );
					if ( it != paramDlg->end() ) {
						_tstring fileName = FilePatternParser::ExtractFileName( m_MapName );
						if ( fileName.empty() ) {
							it->SetTextureButtonText( GetString(IDS_DS_NO_TEXTURE) );
						}
						else {
							it->SetTextureButtonText( fileName.c_str() );
						}
					}
				}
			}
		}
	}

	void Redo() override
	{
		if ( m_Texture ) {
			std::swap( m_PrevFilePrefix, m_Texture->m_PatternedFilePrefix );
			Dialog *paramDlg = m_Texture->m_ParamDialog;
			if ( paramDlg ) {
				Dialog::iterator it = paramDlg->find( layer_zero );
				if ( it != paramDlg->end() ) {
					it->SetTextureButtonText( GetString(IDS_DS_NO_TEXTURE) );
				}
			}
		}
	}

	TSTR Description() override
	{
		return(_T("MultiTileTexmapResetTileRestore"));
	}
};

class TilePatternFormatChangeRestoreObject: public RestoreObj {
	Texture *m_Texture;
	TilePatternFormat m_PrevFormat;

	TilePatternFormatChangeRestoreObject() { }

private:
	void Exec()
	{
		if ( m_Texture ) {  // swap and keep both old and new format for later undo/redo operation
			std::swap( m_Texture->m_TilePatternFormat, m_PrevFormat );
			Dialog *paramDlg = m_Texture->m_ParamDialog;
			if ( paramDlg ) {
				paramDlg->SetPatternFormat( m_Texture->m_TilePatternFormat );
				paramDlg->EnableDisableInterface();
			}
		}
	}

public:
	TilePatternFormatChangeRestoreObject( Texture *texture )
		: m_Texture( texture )
		, m_PrevFormat( TilePatternFormat::Invalid )
	{
		if ( texture ) {
			m_PrevFormat = texture->m_TilePatternFormat;
		}
	}

	~TilePatternFormatChangeRestoreObject() { }

	void Restore(int isUndo) override
	{
		if (isUndo) {
			Exec();
		}
	}

	void Redo() override
	{
		Exec();
	}

	TSTR Description() override
	{
		return(_T("MultiTileTexmapTilePatternFormatChangeRestore"));
	}
};

class SetTileImageRestoreObject: public RestoreObj {
	Texture *m_Texture;
	int m_TileIndex;
    _tstring m_OldFilePath;
	_tstring m_NewFilePath;

	SetTileImageRestoreObject() { }

private:
	void Exec( bool isUndo )
	{
		if ( m_Texture ) {
			Dialog *paramDlg = m_Texture->m_ParamDialog;
			if ( paramDlg ) {
				Dialog::iterator it = paramDlg->find( m_TileIndex );
				if ( it != paramDlg->end() ) {
					if ( isUndo ) {
						_tstring fileName = FilePatternParser::ExtractFileName( m_OldFilePath.c_str() );
						if ( fileName.empty() ) {
							it->SetTextureButtonText( GetString(IDS_DS_NO_TEXTURE) );
						}
						else {
							it->SetTextureButtonText( fileName.c_str() );
						}
					}
					else {
						_tstring fileName = FilePatternParser::ExtractFileName( m_NewFilePath.c_str() );
						if ( fileName.empty() ) {
							it->SetTextureButtonText( GetString(IDS_DS_NO_TEXTURE) );
						}
						else {
							it->SetTextureButtonText( fileName.c_str() );
						}
					}
				}
			}
		}
	}

public:
	SetTileImageRestoreObject( Texture *texture, int tileIndex, const MCHAR *filePath )
		: m_Texture( texture )
		, m_TileIndex( tileIndex )
		, m_OldFilePath( _tstring() )
		, m_NewFilePath( filePath )
	{
		if ( m_Texture ) {
			Texmap *tex = m_Texture->GetSubTexmap(m_TileIndex);
			BitmapTex *bmTex = dynamic_cast<BitmapTex*>( tex );
			if ( bmTex ) {
				m_OldFilePath = bmTex->GetMapName();
			}
		}
	}

	~SetTileImageRestoreObject() { }

	void Restore(int isUndo) override
	{
		if (isUndo) {
			Exec( true );
		}
	}

	void Redo() override
	{
		Exec( false );
	}

	TSTR Description() override
	{
		return(_T("MultiTileTexmapResetTileRestore"));
	}
};

class UVOffsetChangeRestoreObject: public RestoreObj {
	Texture *m_Texture;
	int m_TileIndex;
    int m_UOffset;
    int m_VOffset;

	UVOffsetChangeRestoreObject() { }

private:
	void Exec()
	{
	}

public:
	UVOffsetChangeRestoreObject( Texture *texture, int tileIndex, int uOffset, int vOffset )
		: m_Texture( texture )
		, m_TileIndex( tileIndex )
		, m_UOffset( uOffset )
		, m_VOffset( vOffset )
	{
	}

	~UVOffsetChangeRestoreObject() { }

	void Restore(int isUndo) override
	{
		if (isUndo) {
			Dialog *paramDialog = m_Texture ? m_Texture->m_ParamDialog : nullptr;
			if ( paramDialog ) {
				auto it = paramDialog->find( m_TileIndex );
				if ( it != paramDialog->end() ) {
					it->UpdateUOffset();
					it->UpdateVOffset();
				}
			}
		}
	}

	void Redo() override
	{
		Dialog *paramDialog = m_Texture ? m_Texture->m_ParamDialog : nullptr;
		if ( paramDialog ) {
			auto it = paramDialog->find( m_TileIndex );
			if ( it != paramDialog->end() ) {
				it->SetUOffset( m_UOffset );
				it->SetVOffset( m_VOffset );
			}
		}
	}

	TSTR Description() override
	{
		return(_T("MultiTileTexmapUVOffsetChangeRestore"));
	}
};

class ViewportQualityChangeRestoreObject : public RestoreObj {
	Texture *m_Texture;
	ViewportQuality m_oldQuality;
	ViewportQuality m_newQuality;

	ViewportQualityChangeRestoreObject() { }

private:
	void Exec( bool isUndo )
	{
		if (m_Texture)
		{
			m_Texture->m_ViewportQuality = isUndo ? m_oldQuality : m_newQuality;
			// Invalidate m_DisplayTexValidInterval and request viewport redraw
			m_Texture->ResetDisplayTexHandles();
			m_Texture->NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);

			Dialog *paramDialog = m_Texture->m_ParamDialog;
			if (paramDialog)
			{
				paramDialog->SetViewportQuality(m_Texture->m_ViewportQuality);
			}
		}
	}

public:
	ViewportQualityChangeRestoreObject(Texture *texture, ViewportQuality quality)
		: m_Texture(texture)
		, m_oldQuality(ViewportQuality::Invalid)
		, m_newQuality(quality)
	{
		if (m_Texture)
		{
			m_oldQuality = m_Texture->m_ViewportQuality;
		}
	}

	~ViewportQualityChangeRestoreObject() { }

	void Restore(int isUndo) override
	{
		if (isUndo) {
			Exec(true);
		}
	}

	void Redo() override
	{
		Exec(false);
	}

	TSTR Description() override
	{
		return(_T("MultiTileTexmapViewportQualityChangeRestore"));
	}
};

#pragma endregion

#pragma region MultiTileMap Texture

Texture::Texture(BOOL loading)
	: m_CompositeTexture( nullptr )
	, m_ParamDialog( nullptr )
	, m_TilePatternFormat( TilePatternFormat::ZBrush )
	, m_ViewportQuality  ( ViewportQuality::High )
{
	if (!loading)
		GetMultiTileDesc()->MakeAutoParamBlocks( this );   // make and intialize paramblock2

	ReplaceReference( ReferenceID_CompositeMap, new Composite::Texture(loading) );

	m_DisplayTexHandles.clear();
	m_DisplayTexValidInterval.SetEmpty();
	mColorCorrectionMode = GetMaxColorCorrectionMode();
}

Texture::~Texture()
{
	if ( m_CompositeTexture )
	{
		m_CompositeTexture->DeleteMe();
		m_CompositeTexture = nullptr;
	}

	ResetDisplayTexHandles();
}

// MtlBase virtuals
ParamDlg* Texture::CreateParamDlg(HWND editor_wnd, IMtlParams *imp) 
{ 
	m_ParamDialog = new Dialog( editor_wnd, imp, this );
	return m_ParamDialog;
}

// ISubMap virtuals
void Texture::SetSubTexmap(int i, Texmap *m) 
{ 
	if ( m_CompositeTexture && (i >= 0) && (i < NumSubTexmaps()) ) {
		m_CompositeTexture->SetSubTexmap(i * 2, m); // skip composite's mask texture
	}
}

int Texture::NumSubTexmaps() 
{ 
	if ( m_CompositeTexture ) {
		return m_CompositeTexture->NumSubTexmaps() / 2; // skip composite's mask texture
	}
	return 0;
}

Texmap* Texture::GetSubTexmap(int i) 
{ 
	if ( m_CompositeTexture && (i >= 0) && (i < NumSubTexmaps()) ) {
		return m_CompositeTexture->GetSubTexmap(i * 2); // skip composite's mask texture
	}
	return nullptr;
}

TSTR Texture::GetSubTexmapSlotName(int i) 
{ 
	if ( m_CompositeTexture && (i >= 0) && (i < NumSubTexmaps()) ) {
		_tostringstream oss;
		oss << GetString(IDS_TILE) << (i + 1);
		return TSTR( oss.str().c_str() );
	}
	return TSTR();
}


void* Texture::GetInterface(ULONG id)
{
	if (id == I_REFTARGWRAPPINGREFTARG)
	{
		return static_cast<IRefTargWrappingRefTarg*>(this);
	}
	return Texmap::GetInterface(id);
}

// Texmap virtuals
AColor Texture::EvalColor(ShadeContext& sc) 
{ 
	if ( m_CompositeTexture ) {
		return m_CompositeTexture->EvalColor(sc); 
	}
	return AColor( 0.0f, 0.0f, 0.0f );
}

Point3 Texture::EvalNormalPerturb(ShadeContext& sc) 
{ 
	if ( m_CompositeTexture ) {
		return m_CompositeTexture->EvalNormalPerturb(sc); 
	}
	return Point3( 0.0f, 0.0f, 0.0f );
}

bool Texture::IsLocalOutputMeaningful( ShadeContext& sc ) 
{ 
	if ( m_CompositeTexture ) {
		return m_CompositeTexture->IsLocalOutputMeaningful(sc); 
	}
	return false;
}

void Texture::SetupTextures(TimeValue t, MaxSDK::Graphics::DisplayTextureHelper& updater)
{
	ISimpleMaterial *pISimpleMaterial = (ISimpleMaterial *)GetProperty(PROPID_SIMPLE_MATERIAL);
	if ( pISimpleMaterial == nullptr )
	{
		return;
	}
	ISimpleMaterialExt* pSimpleMaterialExt = static_cast<ISimpleMaterialExt*>(pISimpleMaterial->GetInterface(ISIMPLE_MATERIAL_EXT_INTERFACE_ID));
	if ( pSimpleMaterialExt == nullptr )
	{
		return;
	}
	if ( !m_CompositeTexture )
	{
		return;
	}

	const bool colorCorrectionModeChanged = UpdateColorCorrectionMode(mColorCorrectionMode);
	// Are we still valid?  If not, initialize...
	if ( !m_DisplayTexValidInterval.InInterval(t) || colorCorrectionModeChanged )
	{
		// Set the interval and initialize the map.
		ResetDisplayTexHandles();
		m_DisplayTexValidInterval.SetInfinite();

		const int forceWidth = GetTextureSizeByViewportQuality();
		const int forceHeight = GetTextureSizeByViewportQuality();
		Composite::Texture::iterator it( m_CompositeTexture->begin() );
		int texCount = 0;
		for ( ; it != m_CompositeTexture->end(); ++it, ++texCount )
		{
			if ( texCount >= pSimpleMaterialExt->GetMaxTextureStageCount() )
			{   // The textures that exceed ISimpleMaterialExt::GetMaxTextureStageCount()
				// will not be shown in viewport.
				break;
			}

			BITMAPINFO* bmi = nullptr;
			Interval tempInterval;
			tempInterval.SetInfinite();
			if ( !CreateViewportDisplayBitmap(bmi, t, updater, tempInterval, it, true, forceWidth, forceHeight) || bmi == nullptr )
			{
				continue;
			}

			m_DisplayTexValidInterval &= tempInterval;
			TexHandle* pTextureHandle = updater.MakeHandle( bmi );
			m_DisplayTexHandles.push_back(pTextureHandle);
		}
	}

	pSimpleMaterialExt->ClearTextures();
	if ( m_DisplayTexHandles.size() > 0 )
	{
		int texmapIndex = 0;
		Composite::Texture::iterator it( m_CompositeTexture->begin() );
		DisplayTextureHelperExt* pDisplayTextureHelperExt = dynamic_cast<DisplayTextureHelperExt*>(&updater);
		for( ; it != m_CompositeTexture->end(); ++it, ++texmapIndex )
		{
			if ( texmapIndex < m_DisplayTexHandles.size() )
			{
				pSimpleMaterialExt->SetStageTexture(texmapIndex, m_DisplayTexHandles[texmapIndex]); 

				if ( pDisplayTextureHelperExt != nullptr )
				{
					pDisplayTextureHelperExt->UpdateStageTextureMapInfo( t, texmapIndex, it->Texture() );
				}

				if (texmapIndex == 0)
				{
					pSimpleMaterialExt->SetTextureColorOperation(texmapIndex, ISimpleMaterialExt::TextureOperationModulate);
				}
				else
				{
					pSimpleMaterialExt->SetTextureColorOperation(texmapIndex, ISimpleMaterialExt::TextureOperationBlendTextureAlpha);
				}
			}
		}
	}

	pSimpleMaterialExt->SetTextureStageCount(static_cast<int>(m_DisplayTexHandles.size()));
}

void Texture::SetNumSubTexmaps(int n)
{
	if ( m_CompositeTexture ) {
		m_CompositeTexture->SetNumMaps( n ); 
	}
}

// Implementing functions for MAXScript.
BaseInterface* Texture::GetInterface( Interface_ID id )
{
	if (id == ITEXTURE_DISPLAY_INTERFACE_ID)
	{
		return static_cast<ITextureDisplay*>(this);
	}
	if (id == MULTITILEMAP_INTERFACE)
	{
		return static_cast<Interface*>(this);
	}
	else
	{
		return MultiTex::GetInterface(id);
	}
}

bool Texture::MXS_SetPatternedImageFile( const MCHAR* filePath )
{
	return SetPatternedImageFile( filePath );
}

bool Texture::MXS_SetImageFile( int tileIndex, const MCHAR *filePath )
{
	if ( tileIndex < 0 || tileIndex >= NumSubTexmaps() ) {
		throw MAXException(GetString( IDS_ERROR_TILE_INDEX ));
	}

	return SetImageFile( tileIndex, filePath );
}

int Texture::MXS_GetTileU( int tileIndex )
{
	if ( tileIndex < 0 || tileIndex >= NumSubTexmaps() ) {
		throw MAXException(GetString( IDS_ERROR_TILE_INDEX ));
	}

	int uOffset;
	GetUVOffset( tileIndex, &uOffset, nullptr );
	return uOffset;
}

void Texture::MXS_SetTileU( int tileIndex, int uOffset )
{
	if ( tileIndex < 0 || tileIndex >= NumSubTexmaps() ) {
		throw MAXException(GetString( IDS_ERROR_TILE_INDEX ));
	}

	if ( !IsPatternFormat() ) {
		int vOffset;
		GetUVOffset( tileIndex, nullptr, &vOffset );
		SetUVOffset( tileIndex, uOffset, vOffset );
	}
}

int Texture::MXS_GetTileV( int tileIndex )
{
	if ( tileIndex < 0 || tileIndex >= NumSubTexmaps() ) {
		throw MAXException(GetString( IDS_ERROR_TILE_INDEX ));
	}

	int vOffset;
	GetUVOffset( tileIndex, nullptr, &vOffset );
	return vOffset;
}

void Texture::MXS_SetTileV( int tileIndex, int vOffset )
{
	if ( tileIndex < 0 || tileIndex >= NumSubTexmaps() ) {
		throw MAXException(GetString( IDS_ERROR_TILE_INDEX ));
	}

	if ( !IsPatternFormat() ) {
		int uOffset;
		GetUVOffset( tileIndex, &uOffset, nullptr );
		SetUVOffset( tileIndex, uOffset, vOffset );
	}
}

Texmap* Texture::MXS_GetTile( int tileIndex )
{
	if ( tileIndex < 0 || tileIndex >= NumSubTexmaps() ) {
		throw MAXException(GetString( IDS_ERROR_TILE_INDEX ));
	}

	return GetSubTexmap( tileIndex );
}

void Texture::MXS_SetTile( int tileIndex, Texmap* map )
{
	if ( tileIndex < 0 || tileIndex >= NumSubTexmaps() ) {
		throw MAXException(GetString( IDS_ERROR_TILE_INDEX ));
	}

	if ( !IsPatternFormat() ) {
		SetSubTexmap( tileIndex, map );
	}
}

int Texture::MXS_Count()
{
	return NumSubTexmaps();
}

void Texture::MXS_AddTile()
{
	if ( NumSubTexmaps() >= Composite::Param::LayerMax) {
		throw MAXException(GetString( IDS_ERROR_MAXTILE ));
	}

	if ( !IsPatternFormat() ) {
		AddTile();
	}
}

void Texture::MXS_DeleteTile(int tileIndex )
{
	if ( tileIndex < 0 || tileIndex >= NumSubTexmaps() ) {
		throw MAXException(GetString( IDS_ERROR_TILE_INDEX ));
	}
	if ( NumSubTexmaps() == 1 ) {
		throw MAXException(GetString( IDS_ERROR_MINTILE ));
	}

	if ( !IsPatternFormat() ) {
		DeleteTile( tileIndex );
	}
}

int Texture::MXS_GetPatternFormat()
{
	return static_cast<int>( GetPatternFormat() );
}

void Texture::MXS_SetPatternFormat( int format )
{
	SetPatternFormat( static_cast<TilePatternFormat>(format) );
}

int Texture::MXS_GetViewportQuality()
{
	return static_cast<int>( GetViewportQuality() );
}

void Texture::MXS_SetViewportQuality( int quality )
{
	SetViewportQuality( static_cast<ViewportQuality>(quality) );
}

void Texture::AddTile( )
{
	if ( m_CompositeTexture )
	{
		if ( m_CompositeTexture->nb_layer() >= Composite::Param::LayerMax ) {
			if ( !GetCOREInterface()->GetQuietMode() ) {
				MessageBox( 0, GetString( IDS_ERROR_MAXTILE ),
					GetString( IDS_ERROR_TITLE ),
					MB_APPLMODAL | MB_OK | MB_ICONERROR );
			}
			return;
		}

		m_CompositeTexture->AddLayer();

		if ( theHold.Holding() ) {
			theHold.Put( new AddTileRestoreObject(this) );
		}

		if ( m_ParamDialog ) {
			auto layer = m_CompositeTexture->LayerIterator( m_CompositeTexture->nb_layer() - 1 );
			m_ParamDialog->CreateRollup( &*layer );
			m_ParamDialog->EnableDisableInterface();
		}
	}
}

void Texture::DeleteTile( size_t Index/* = 0*/ )
{
	if ( m_CompositeTexture )
	{
		if ( m_CompositeTexture->nb_layer() == 1) {
			if ( !GetCOREInterface()->GetQuietMode() ) {
				MessageBox( 0, GetString( IDS_ERROR_MINTILE ),
					GetString( IDS_ERROR_TITLE ),
					MB_APPLMODAL | MB_OK | MB_ICONERROR );
			}
			return;
		}
		else if ( Index >= m_CompositeTexture->nb_layer() ) {
			if ( !GetCOREInterface()->GetQuietMode() ) {
				MessageBox( 0, GetString( IDS_ERROR_TILE_INDEX ),
					GetString( IDS_ERROR_TITLE ),
					MB_APPLMODAL | MB_OK | MB_ICONERROR );
			}
			return;
		}

		if ( m_ParamDialog ) {
			auto layerDlg = m_ParamDialog->find( static_cast<int>(Index) );
			if ( layerDlg != m_ParamDialog->end() ) {
				m_ParamDialog->DeleteRollup( layerDlg->Layer() );
			}
		}
		if (theHold.Holding()) {
			theHold.Put(new DeleteTileBeforeRestoreObject(this, Index));
		}
		m_CompositeTexture->DeleteLayer(Index);

		if (theHold.Holding()) {
			theHold.Put(new DeleteTileAfterRestoreObject(this));
		}
		if ( m_ParamDialog ) {
			m_ParamDialog->EnableDisableInterface();
		}
	}
}

TilePatternFormat Texture::GetPatternFormat() const 
{ 
	return m_TilePatternFormat; 
}

TilePatternFormat Texture::GetFilePatternFormat( const MCHAR *filePath )
{
	return FilePatternParser::GetFilePatternFormat( filePath );
}

void Texture::SetPatternFormat( const TilePatternFormat format )
{ 
	if ( format == GetPatternFormat() ) 
	{
		return; 
	}

	if ( (format >= TilePatternFormat::First) && (format < TilePatternFormat::Last) )
	{
		ResetTiles();

		if ( theHold.Holding() ) {
			theHold.Put( new TilePatternFormatChangeRestoreObject(this) );
		}
		m_TilePatternFormat = format;
		if ( m_ParamDialog )
		{
			m_ParamDialog->SetPatternFormat( format );
			m_ParamDialog->EnableDisableInterface();
		}
	}
}

ViewportQuality Texture::GetViewportQuality() const 
{
	return m_ViewportQuality; 
}

void Texture::SetViewportQuality( ViewportQuality quality )
{
	if ( quality == GetViewportQuality() ) 
	{
		return; 
	}

	if ( (quality >= ViewportQuality::First) && (quality < ViewportQuality::Last) )
	{
		if (theHold.Holding()) {
			theHold.Put(new ViewportQualityChangeRestoreObject(this, quality));
		}

		m_ViewportQuality = quality;
		// Invalidate m_DisplayTexValidInterval and request viewport redraw
		ResetDisplayTexHandles();
		NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);

		if ( m_ParamDialog )
		{
			m_ParamDialog->SetViewportQuality( quality );
		}
	}
}

bool Texture::SetPatternedImageFile( const MCHAR *filePath )
{
	if ( IsPatternFormat() )
	{
		return SetImageFile( 0, filePath );
	}
	return false;
}

bool Texture::SetImageFile( int tileIndex, const MCHAR *filePath )
{
	DbgAssert( m_CompositeTexture );
	if ( m_CompositeTexture )
	{
		if ( IsPatternFormat() && (0 == tileIndex) )
		{   
			const TilePatternFormat curFormat = GetPatternFormat();
			FilePatternParser parser( curFormat, filePath );
			if ( !parser.CheckFilePattern() ) {
				return false;
			}

			theHold.Begin();
			// Clear the tile list first.
			ResetTiles();

			SetPattenedFilePrefix( parser.GetPattenedFilePrefix() );
			auto matchedFiles = parser.FindMatchedFiles();

			auto minmaxU = std::minmax_element( matchedFiles.cbegin(), matchedFiles.cend(), 
				[](const FilePatternParser::UVMapFileEntry &lhs, 
				   const FilePatternParser::UVMapFileEntry &rhs) -> bool {
					   return ( std::get<0>(lhs) < std::get<0>(rhs) );
				});
			const int minUIndex = std::get<0>(*minmaxU.first);
			const int maxUIndex = std::get<0>(*minmaxU.second);

			auto minmaxV = std::minmax_element( matchedFiles.cbegin(), matchedFiles.cend(), 
				[](const FilePatternParser::UVMapFileEntry &lhs, 
				   const FilePatternParser::UVMapFileEntry &rhs) -> bool {
					   return ( std::get<1>(lhs) < std::get<1>(rhs) );
				});
			const int minVIndex = std::get<1>(*minmaxV.first);
			const int maxVIndex = std::get<1>(*minmaxV.second);

			auto lastEntry = matchedFiles.crbegin();
			const int lastVIndex = std::get<1>(*lastEntry);

			// The file matched may contains only a few of files, there may be 'hole' in the file pattern.
			// We create the placeholder textures to keep these entries.
			// For example, there exist U0_V2, U2_V1, 2 files.
			// We should loop for 3 * 3 times, create any intermediate missing texture, such as U0_V1, U1_V2,
			// but should stop when U = 2, V = 1, the U2_V2 should not create.
			int count = 0;
			bool quitLoop = false;
			for ( int u = minUIndex; (u <= maxUIndex) && !quitLoop; ++u ) {
				for ( int v = minVIndex; v <= maxVIndex; ++v ) {
					if ( (u == maxUIndex) && (v > lastVIndex) ) {
						// index has beyond the last entry, no need to create more texture.
						quitLoop = true;
						break;
					}

					// get image path
					const auto entry = std::find_if( matchedFiles.cbegin(), matchedFiles.cend(), 
						[&]( const FilePatternParser::UVMapFileEntry &item ) -> bool {
							return (std::get<0>(item) == u) && (std::get<1>(item) == v);
						});
					_tstring fullPath;
					if ( entry != matchedFiles.end() ) {
						fullPath = std::get<3>(*entry);
					}
					BitmapTex *bmTex = CreateUntiledBitmapTexture( fullPath.empty() ? nullptr : fullPath.c_str() );
					if ( bmTex ) {
						if ( 0 == count ) { // first tile is always created, no need to call AddTile()
							if ( theHold.Holding() ) {
								theHold.Put( new SetTileImageRestoreObject(this, 0, filePath) );
							}
							if ( m_ParamDialog ) {
								Dialog::iterator it = m_ParamDialog->find( count );
								if ( it != m_ParamDialog->end() ) {
									it->SetTextureButtonText( std::get<2>(*entry).c_str() );
								}
							}
						}
						else {
							AddTile();
						}
						const int tileIndex = static_cast<int>(m_CompositeTexture->nb_layer() - 1);
						SetSubTexmap( tileIndex, bmTex );
						SetUVOffset( tileIndex, u, v );
					}
					++count;
				}
			}
			theHold.Accept( ::GetString(IDS_MULTTILE_UNDO_SETTILE) );
			return true;
		}
		else if ( (tileIndex >= 0) && (tileIndex < m_CompositeTexture->nb_layer()) )
		{
			theHold.Begin();
			if ( theHold.Holding() ) {
				theHold.Put( new SetTileImageRestoreObject(this, tileIndex, filePath) );
			}
			BitmapTex *bmTex = CreateUntiledBitmapTexture( filePath );
			if ( bmTex ) {
				SetSubTexmap( tileIndex, bmTex );
				if ( m_ParamDialog ) {
					Dialog::iterator it = m_ParamDialog->find( tileIndex );
					if ( it != m_ParamDialog->end() ) {
						_tstring fileName = FilePatternParser::ExtractFileName( filePath );
						if ( !fileName.empty() ) {
							it->SetTextureButtonText( fileName.c_str() );
						}
					}
				}
			}
			theHold.Accept( ::GetString(IDS_MULTTILE_UNDO_SETTILE) );
			return true;
		}
	}
	return false;
}

const MCHAR* Texture::GetPatternedFilePrefix() const
{
	if ( IsPatternFormat() && !m_PatternedFilePrefix.empty() )
	{
		m_PatternedFilePrefix.c_str();
	}
	return nullptr;
}

bool Texture::SetUVOffset( int tileIndex, int uOffset, int vOffset )
{
	Texmap *tex = GetSubTexmap( tileIndex );
	if ( tex ) {
		StdUVGen *uvGen = dynamic_cast<StdUVGen*>(tex->GetTheUVGen());
		if ( uvGen ) {
			const TimeValue t = GetCOREInterface()->GetTime();
			if ( theHold.Holding() ) {
				theHold.Put( new UVOffsetChangeRestoreObject(this, tileIndex, uOffset, vOffset) );
			}
			uvGen->SetUOffs(uOffset, t);
			uvGen->SetVOffs(vOffset, t);
			return true;
		}
		StdXYZGen *xyzGen = dynamic_cast<StdXYZGen*>(tex->GetTheXYZGen());
		if ( xyzGen ) {
			const TimeValue t = GetCOREInterface()->GetTime();
			static const int X = 0;
			static const int Y = 1;
			if ( theHold.Holding() ) {
				theHold.Put( new UVOffsetChangeRestoreObject(this, tileIndex, uOffset, vOffset) );
			}
			xyzGen->SetOffs(X, uOffset, t);
			xyzGen->SetOffs(Y, vOffset, t);
			return true;
		}
	}
	return false;
}

bool Texture::GetUVOffset( int tileIndex, int *uOffset, int *vOffset )
{
	Texmap *tex = GetSubTexmap( tileIndex );
	if ( tex ) {
		StdUVGen *uvGen = dynamic_cast<StdUVGen*>(tex->GetTheUVGen());
		if ( uvGen ) {
			const TimeValue t = GetCOREInterface()->GetTime();
			if ( uOffset ) {
				*uOffset = static_cast<int>( uvGen->GetUOffs(t) );
			}
			if ( vOffset ) {
				*vOffset = static_cast<int>( uvGen->GetVOffs(t) );
			}
			return true;
		}
		StdXYZGen *xyzGen = dynamic_cast<StdXYZGen*>(tex->GetTheXYZGen());
		if ( xyzGen ) {
			const TimeValue t = GetCOREInterface()->GetTime();
			static const int X = 0;
			static const int Y = 1;
			if ( uOffset ) {
				*uOffset = static_cast<int>( xyzGen->GetOffs(X, t) );
			}
			if ( vOffset ) {
				*vOffset = static_cast<int>( xyzGen->GetOffs(Y, t) );
			}
			return true;
		}
	}
	return false;
}

void Texture::Update(TimeValue t, Interval& valid) { 
	if ( m_CompositeTexture ) {
		m_CompositeTexture->Update(t, valid); 
	}
}

void Texture::Reset() { 
	DeleteAllRefsFromMe();
	if ( m_CompositeTexture ) {
		m_CompositeTexture->Reset();
	}
	GetCompositeDesc()->MakeAutoParamBlocks(this);   // make and intialize paramblock2
}

Interval Texture::Validity(TimeValue t) {
	Interval v = FOREVER;
	Update(t,v);
	return v;
}

void Texture::SetupGfxMultiMaps(TimeValue t, Material *material, MtlMakerCallback &cb)
{
	if ( m_CompositeTexture ) {
		m_CompositeTexture->SetupGfxMultiMaps( t, material, cb );
	}
}

void Texture::NotifyChanged() {
	if ( m_CompositeTexture ) {
		m_CompositeTexture->NotifyChanged();
	}
}

Class_ID Texture::ClassID()
{
	return GetMultiTileDesc()->ClassID();
}

SClass_ID Texture::SuperClassID()
{ 
	return GetMultiTileDesc()->SuperClassID();
}

void Texture::GetClassName(TSTR& s)
{ 
	s = GetMultiTileDesc()->ClassName();
}

void Texture::DeleteThis()
{ 
	delete this;
}

RefTargetHandle Texture::Clone( RemapDir &remap ) 
{ 
	Texture *NewMtl = new Texture( TRUE );
	*((MtlBase*)NewMtl) = *((MtlBase*)this);  // copy superclass stuff

	NewMtl->ReplaceReference( ReferenceID_CompositeMap, remap.CloneRef( m_CompositeTexture ));

	BaseClone(this, NewMtl, remap);

	return (RefTargetHandle)NewMtl;
}

RefTargetHandle Texture::GetReference( int i )
{
	switch(i)
	{
	case ReferenceID_CompositeMap: {
			return m_CompositeTexture;
		}
		break;
	default: {
			return nullptr;
		}
	}
}

ReferenceTarget * Texture::GetWrappedObject(bool recurse) const
{
	return m_CompositeTexture;
}

void Texture::SetReference( int i, RefTargetHandle v ) 
{ 
	switch ( i ) 
	{
	case ReferenceID_CompositeMap: {
			m_CompositeTexture = dynamic_cast<Composite::Texture*>(v); 
		}
		break;
	}
}

void Texture::ResetDisplayTexHandles()
{
	for ( auto hTex : m_DisplayTexHandles )
	{
		if ( hTex )
		{
			hTex->DeleteThis();
			hTex = nullptr;
		}
	}
	m_DisplayTexHandles.clear();
	m_DisplayTexValidInterval.SetEmpty();
}

bool Texture::CreateViewportDisplayBitmap( PBITMAPINFO &pOutputMap,
										   TimeValue time, 
										   TexHandleMaker &callback, 
										   Interval &validInterval, 
										   Composite::Texture::iterator &it,
										   bool bForceSize,
										   int forceWidth,
										   int forceHeight )
{
	if ( !bForceSize )
	{
		forceWidth = 0;
		forceHeight = 0;
	}
	Interval theInterval;
	theInterval.SetEmpty();
    Texmap * map = it->Texture();
    if (map == nullptr)
    {
        return false;
    }
	pOutputMap = map->GetVPDisplayDIB(time, callback, theInterval, false, forceWidth, forceHeight);
	if ( !pOutputMap ) {
		return false;
	}
	validInterval &= theInterval;
	return true;
}

int Texture::GetTextureSizeByViewportQuality() const
{
	static const int defaultRes = 256;
	switch (m_ViewportQuality)
	{
	case ViewportQuality::Low: return defaultRes / 2;
	case ViewportQuality::Middle: return defaultRes;
	case ViewportQuality::High: return defaultRes * 2;
	default: return defaultRes;
	}
}

void Texture::SetPattenedFilePrefix( const _tstring &prefix )
{
	m_PatternedFilePrefix = prefix;
}

BitmapTex* Texture::CreateUntiledBitmapTexture( const MCHAR *filePath )
{
	BitmapTex *bmTex = static_cast<BitmapTex*>(CreateInstance( TEXMAP_CLASS_ID, Class_ID(BMTEX_CLASS_ID, 0) ));
	if ( bmTex ) {
		StdUVGen *uvGen = bmTex->GetUVGen();
		if ( uvGen ) {
			// remove tiling flag
			int tileFlag = uvGen->GetTextureTiling();
			tileFlag &= ~(U_WRAP | V_WRAP);
			uvGen->SetTextureTiling(tileFlag);
		}
		bmTex->SetMapName( filePath ? filePath : _T("") );
	}
	return bmTex;
}

void Texture::ResetTiles()
{
	if ( theHold.Holding() ) {
		theHold.Put( new ResetTileRestoreObject(this) );
	}

	static const int layer_zero = 0;
	if ( m_CompositeTexture ) {
		size_t layerNum = m_CompositeTexture->size();
		while ( --layerNum > 0 ) {
			DeleteTile( layerNum );
		}
		Texmap *tex = GetSubTexmap(layer_zero);
		BitmapTex *bmTex = dynamic_cast<BitmapTex*>( tex );
		if ( bmTex ) {
			// Clear image file path
			bmTex->SetMapName( _T("") );
		}
		if ( tex ) {
			// Reset UV settings
			const TimeValue t = GetCOREInterface()->GetTime();
			StdUVGen *uvGen = dynamic_cast<StdUVGen*>( tex->GetTheUVGen() );
			if ( uvGen ) {
				uvGen->SetUOffs( 0.0f, t );
				uvGen->SetVOffs( 0.0f, t );
				// remove tiling flag
				int tileFlag = uvGen->GetTextureTiling();
				tileFlag &= ~(U_WRAP | V_WRAP); 
				uvGen->SetTextureTiling(tileFlag);
			}
			StdXYZGen *xyzGen = dynamic_cast<StdXYZGen*>( tex->GetTheXYZGen() );
			if ( xyzGen ) {
				static const int X = 0;
				static const int Y = 1;
				xyzGen->SetOffs(X, 0.0f, t);
				xyzGen->SetOffs(Y, 0.0f, t);
			}
		}
	}

	if ( m_ParamDialog ) {
		// Clear bitmap button text
		Dialog::iterator it = m_ParamDialog->find( layer_zero );
		if ( it != m_ParamDialog->end() ) {
			it->SetTextureButtonText( GetString(IDS_DS_NO_TEXTURE) );
		}
	}

	SetPattenedFilePrefix( _T("") );
}

RefResult Texture::NotifyRefChanged( 
	const Interval &changeInt,
	RefTargetHandle hTarget,
	PartID         &partID,
	RefMessage      message, 
	BOOL            propagate )
{
	switch ( message )
	{
	case REFMSG_TARGET_DELETED:
		{
			if ( hTarget == m_CompositeTexture ) {
				m_CompositeTexture = nullptr;
			}
		}
		break;
	}
	return REF_SUCCEED;
}

IOResult Texture::Save( ISave *iSave )
{
	IOResult res( IO_OK );

	iSave->BeginChunk( static_cast<USHORT>(ChunkID::Header) );
	res = MtlBase::Save( iSave );
	if ( res != IO_OK ) return res;
	iSave->EndChunk();

	ULONG nBytes = 0;
	iSave->BeginChunk( static_cast<USHORT>(ChunkID::PatternFormat) );
	res = iSave->WriteEnum( &m_TilePatternFormat, sizeof(m_TilePatternFormat), &nBytes );	
	if (res != IO_OK) return res;
	iSave->EndChunk();

	iSave->BeginChunk( static_cast<USHORT>(ChunkID::PatternPrefix) );
	res = iSave->WriteWString( m_PatternedFilePrefix.c_str() );
	if (res != IO_OK) return res;
	iSave->EndChunk();

	iSave->BeginChunk( static_cast<USHORT>(ChunkID::ViewportQuality) );
	res = iSave->WriteEnum( &m_ViewportQuality, sizeof(m_ViewportQuality), &nBytes );
	if (res != IO_OK) return res;
	iSave->EndChunk();

	return res;
}

IOResult Texture::Load( ILoad *iLoad )
{
	IOResult res( IO_OK );

	while( IO_OK == (res = iLoad->OpenChunk()) ) 
	{
		USHORT chunk_id = iLoad->CurChunkID();
		switch( chunk_id ) 
		{
		case ChunkID::Header:
			{
				res = MtlBase::Load( iLoad );
				DbgAssert( IO_OK == res );
			}
			break;
		case ChunkID::PatternFormat:
			{
				ULONG nb;
				res = iLoad->ReadEnum( &m_TilePatternFormat, sizeof(m_TilePatternFormat), &nb );
				DbgAssert( IO_OK == res );
			}
			break;
		case ChunkID::PatternPrefix:
			{
				MCHAR *buffer;
				res = iLoad->ReadWStringChunk( &buffer );
				DbgAssert( IO_OK == res );
				m_PatternedFilePrefix = buffer;
			}
			break;
		case ChunkID::ViewportQuality: {
			{
				ULONG nb;
				res = iLoad->ReadEnum( &m_ViewportQuality, sizeof(m_ViewportQuality), &nb );
				DbgAssert( IO_OK == res );
			}
			break;
			}
		 default:
			 {  // Unknown chunk.  You may want to break here for debugging.
				res = IO_OK;
			 }
			 break;
		}
		if (res != IO_OK) 
		{
			return res;
		}
		res = iLoad->CloseChunk();
		if (res != IO_OK) 
		{
			return res;
		}
	}
	return IO_OK;
}

#pragma endregion

////////////////////////////////////////////////////////////////////////////////

#pragma region Class Descriptors

void*        Desc::Create(BOOL loading)   { return new MultiTileMap::Texture(loading); }
int          Desc::IsPublic()     { return TRUE; }
SClass_ID    Desc::SuperClassID() { return TEXMAP_CLASS_ID; }
Class_ID     Desc::ClassID()      { return Class_ID( MULTITILE_CLASS_ID, 0 ); }
HINSTANCE    Desc::HInstance()    { return hInstance; }
const TCHAR* Desc::ClassName()    { return GetString( IDS_RB_MULTITILE_CDESC ); }
const TCHAR* Desc::Category()     { return TEXMAP_CAT_COMP; }
const TCHAR* Desc::InternalName() { return _T("MultiTileMap"); }

#pragma endregion

}

