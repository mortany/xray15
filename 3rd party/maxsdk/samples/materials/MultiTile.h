//
// Copyright [2015] Autodesk, Inc.  All rights reserved. 
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc. and
// are protected under applicable copyright and trade secret law.  They may
// not be disclosed to, copied or used by any third party without the prior
// written consent of Autodesk, Inc.
//
#ifndef __MULTITILE_H__
#define __MULTITILE_H__

#include "IRefTargWrappingRefTarg.h"
#include "MultiTileFP.h"
#include "maxtypes.h"
#include "util.h"
#include "IMultiTile.h"
#include "Graphics/ITextureDisplay.h"
#include "composite.inl"

namespace MultiTileMap
{

struct Desc : public ClassDesc2
{
	int          IsPublic();
	void*        Create(BOOL);
	SClass_ID    SuperClassID();
	Class_ID     ClassID();
	HINSTANCE    HInstance();

	const TCHAR* ClassName();
	const TCHAR* Category();
	const TCHAR* InternalName();
};

enum ReferenceID
{
	ReferenceID_CompositeMap = 0,

	ReferenceID_Num
};

namespace Default 
{
	const float UVOffsetDefault( 0.0f );
	const float UVOffsetMax(  1024.0f );
	const float UVOffsetMin( -1024.0f );
}


class Texture 
	: public IMultiTile
	, public Interface
	, public MaxSDK::Graphics::ITextureDisplay
	, public IRefTargWrappingRefTarg
{
friend class Dialog;

// For Undo/Redo operation
friend class AddTileRestoreObject;
friend class DeleteTileBeforeRestoreObject;
friend class DeleteTileAfterRestoreObject;
friend class ResetTileRestoreObject;
friend class TilePatternFormatChangeRestoreObject;
friend class SetTileImageRestoreObject;
friend class UVOffsetChangeRestoreObject;
friend class ViewportQualityChangeRestoreObject;

public:

	//////////////////////////////////////////////////////////////////////////
	// ctor & dtor
	Texture(BOOL loading = FALSE);
	~Texture();

	Texture operator=(const Texture&) = delete;
	Texture(const Texture&) = delete;

	// Accessors
	Dialog*        ParamDialog( ) const { return m_ParamDialog; }
	void           ParamDialog(Dialog* v) { m_ParamDialog = v; }

	// MtlBase virtuals
	ParamDlg*      CreateParamDlg(HWND editor_wnd, IMtlParams *imp) override;
	Interval       Validity(TimeValue t) override;
	BOOL           SupportTexDisplay() override { return TRUE; }
	BOOL           SupportsMultiMapsInViewport() override { return TRUE; }
	void           ActivateTexDisplay(BOOL onoff) override { }
	void           SetupGfxMultiMaps(TimeValue t, Material *material, MtlMakerCallback &cb) override;

	// ISubMap virtuals
	void           SetSubTexmap(int i, Texmap *m) override;
	int            NumSubTexmaps() override;
	Texmap*        GetSubTexmap(int i) override;
	TSTR           GetSubTexmapSlotName(int i) override;

	// Animatable virtuals
	int            SubNumToRefNum( int subNum ) override { return -1; }
	int            NumSubs( ) override { return 0; }
	Animatable*    SubAnim(int i) override { return nullptr; }
	TSTR           SubAnimName(int i) override { return TSTR(); }
	void*          GetInterface(ULONG id) override;

	// Texmap virtuals
	AColor         EvalColor(ShadeContext& sc) override;
	Point3         EvalNormalPerturb(ShadeContext& sc) override;
	bool           IsLocalOutputMeaningful( ShadeContext& sc ) override;

	// ITextureDisplay virtuals
	void           SetupTextures(TimeValue t, MaxSDK::Graphics::DisplayTextureHelper& updater) override;

	// MultiTex virtuals
	void           SetNumSubTexmaps(int n) override;

	// IMultiTile virtuals
	void           SetPatternFormat( TilePatternFormat format ) override;
	TilePatternFormat GetPatternFormat() const override;
	TilePatternFormat GetFilePatternFormat( const MCHAR *filePath ) override;
	void           SetViewportQuality( ViewportQuality quality ) override;
	ViewportQuality GetViewportQuality() const override;
	bool           SetPatternedImageFile( const MCHAR *filePath ) override;
	bool           SetImageFile( int tileIndex, const MCHAR *filePath ) override;
	const MCHAR*   GetPatternedFilePrefix() const;
	bool           SetUVOffset( int tileIndex, int uOffset, int vOffset ) override;
	bool           GetUVOffset( int tileIndex, int *uOffset, int *vOffset ) override;

	// Implementing functions for MAXScript.
	BaseInterface* GetInterface(Interface_ID id) override;
	bool           MXS_SetPatternedImageFile(const MCHAR*) override;
	bool           MXS_SetImageFile(int, const MCHAR*) override;
	int            MXS_GetTileU(int) override;
	void           MXS_SetTileU(int, int) override;
	int            MXS_GetTileV(int) override;
	void           MXS_SetTileV(int, int) override;
	Texmap*        MXS_GetTile(int) override;
	void           MXS_SetTile(int, Texmap*) override;
	int            MXS_Count() override;
	void           MXS_AddTile() override;
	void           MXS_DeleteTile(int) override;
	int            MXS_GetPatternFormat() override;
	void           MXS_SetPatternFormat(int) override;
	int            MXS_GetViewportQuality() override;
	void           MXS_SetViewportQuality(int) override;

	// Tile entry operation, apply to custom mode
	void           AddTile() override;
	void           DeleteTile( size_t Index = 0 ) override;
	
	// Delete all rollouts except tile 0.
	// Reset tile 0 file path, UV settings.
	void           ResetTiles();
	bool           IsPatternFormat() const
	{ 
		return (GetPatternFormat() == TilePatternFormat::ZBrush || 
			    GetPatternFormat() == TilePatternFormat::Mudbox || 
				GetPatternFormat() == TilePatternFormat::UDIM); 
	}

	// ---------------------------------------------------------------------------
	Composite::Texture* GetCompositeTexture() const { return m_CompositeTexture; }

	void Update(TimeValue t, Interval& valid);
	void Reset();
	void NotifyChanged();

	// Class management
	Class_ID    ClassID() override;
	SClass_ID   SuperClassID() override;
	void        GetClassName(TSTR& s) override;
	void        DeleteThis() override;

	// From ReferenceMaker
	int             NumRefs() override { return ReferenceID_Num; }
	RefTargetHandle GetReference( int i ) override;
	
	// From IRefTargWrappingRefTarg
	ReferenceTarget* GetWrappedObject(bool recurse) const override;

private:
	void SetReference( int i, RefTargetHandle v ) override;

	// used for OGS VP display
	void ResetDisplayTexHandles();
	bool CreateViewportDisplayBitmap( PBITMAPINFO &pOutputMap,
		TimeValue time, 
		TexHandleMaker &callback, 
		Interval &validInterval, 
		Composite::Texture::iterator &it,
		bool bForceSize,
		int forceWidth,
		int forceHeight );
	int GetTextureSizeByViewportQuality() const;

	void SetPattenedFilePrefix( const _tstring &prefix );
	
	// Create a BitmapTex with tiling flag off
	static BitmapTex* CreateUntiledBitmapTexture( const MCHAR *filePath );

public:
	RefTargetHandle Clone( RemapDir &remap ) override;

	RefResult       NotifyRefChanged( 
		const Interval &changeInt,
		RefTargetHandle hTarget,
		PartID         &partID,
		RefMessage      message, 
		BOOL            propagate ) override;

	// IO
	IOResult Save( ISave *iSave ) override;
	IOResult Load( ILoad *iLoad ) override;


private:
	// Member variables
	Composite::Texture   *m_CompositeTexture;
	Dialog               *m_ParamDialog;

	// used for OGS VP display
	std::vector<TexHandle*> m_DisplayTexHandles;
	Interval                m_DisplayTexValidInterval;
	IColorCorrectionMgr::CorrectionMode mColorCorrectionMode;

	TilePatternFormat	  m_TilePatternFormat;
	_tstring			  m_PatternedFilePrefix;
	ViewportQuality       m_ViewportQuality;
};

}	// namespace MultiTileMap

ClassDesc2* GetMultiTileDesc();

#endif // __MULTITILE_H__
