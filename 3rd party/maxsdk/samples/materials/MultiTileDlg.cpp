//
// Copyright [2015] Autodesk, Inc.  All rights reserved. 
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc. and
// are protected under applicable copyright and trade secret law.  They may
// not be disclosed to, copied or used by any third party without the prior
// written consent of Autodesk, Inc.
//

#include "MultiTileDlg.h"
#include "MultiTileLayerDlg.h"
#include "3dsmaxdlport.h"
#include "mtlres.h"
#include "mtlhdr.h"

#include <sstream>

template <typename Type> 
struct ReferenceTargetDestructor
{
	static void Delete(Type *ptr) 
	{ 
		ptr->MaybeAutoDelete();
	}
};

namespace MultiTileMap 
{
////////////////////////////////////////////////////////////////////////////////

Dialog::Dialog(HWND editor_wnd, IMtlParams *mtl_param, Texture *tex)
	: m_MtlParams        ( mtl_param       )
	, m_Texture          ( tex             )
	, m_Redraw           ( false           )
	, m_EditorWnd        ( editor_wnd      )
	, m_Reloading        ( false           )
	, m_ButtonImageList  ( ImageList_Create( 13, 12, ILC_MASK, 0, 1 ) )
{
	m_DragManager.Init( this );
	m_DialogWindow = m_MtlParams->AddRollupPage( hInstance, 
		MAKEINTRESOURCE(IDD_MULTITILEMAP),
		PanelDlgProc,
		GetString( IDS_MULTITILEMAP_TILES ),
		(LPARAM)this );

	// We cannot directly construct m_LayerRollupWnd since we need to call
	// AddRollupPage before.
	// Note that the corresponding ReleaseIRollup is called when m_LayerRollupWnd is
	// destroyed.
	m_LayerRollupWnd = ::GetIRollup( ::GetDlgItem(m_DialogWindow, IDC_MULTITILEMAP_LIST) );

	// Create the button image list
	HBITMAP hBitmap, hMask;
	hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_COMP_BUTTONS));
	hMask   = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_COMP_MASKBUTTONS));
	ImageList_Add( m_ButtonImageList, hBitmap, hMask );
	DeleteObject(hBitmap);
	DeleteObject(hMask);

	// Initialize the values inside the Combo.
	HWND hCombo = GetDlgItem( m_DialogWindow, IDC_MULTITILEMAP_PATTERN_FORMAT );
	::SendMessage( hCombo, CB_ADDSTRING, 0, LPARAM( GetString( IDS_MULTITILE_PATTERN_ZBRUSH ) ) );
	::SendMessage( hCombo, CB_ADDSTRING, 0, LPARAM( GetString( IDS_MULTITILE_PATTERN_MUDBOX ) ) );
	::SendMessage( hCombo, CB_ADDSTRING, 0, LPARAM( GetString( IDS_MULTITILE_PATTERN_UDIM ) ) );
	::SendMessage( hCombo, CB_ADDSTRING, 0, LPARAM( GetString( IDS_MULTITILE_PATTERN_CUSTOM ) ) );
	::SendMessage( hCombo, CB_SETCURSEL, static_cast<WPARAM>(GetPatternFormat()), 0 );

	hCombo = GetDlgItem( m_DialogWindow, IDC_MULTITILEMAP_VIEWPORT_QUALITY );
	::SendMessage( hCombo, CB_ADDSTRING, 0, LPARAM( GetString( IDS_MULTITILE_VIEWPORT_QUALITY_LOW ) ) );
	::SendMessage( hCombo, CB_ADDSTRING, 0, LPARAM( GetString( IDS_MULTITILE_VIEWPORT_QUALITY_MIDDLE ) ) );
	::SendMessage( hCombo, CB_ADDSTRING, 0, LPARAM( GetString( IDS_MULTITILE_VIEWPORT_QUALITY_HIGH ) ) );
	::SendMessage( hCombo, CB_SETCURSEL, static_cast<WPARAM>(GetViewportQuality()), 0 );

	// Create rollups and dialog classes for all layers already available
	Composite::Texture *compTex = m_Texture->GetCompositeTexture();
	for( auto it = compTex->begin(); it != compTex->end(); ++it ) {
		CreateRollup( &*it );
	}

	EditPtr tileNum( m_DialogWindow, IDC_MULTITILEMAP_TILE_NUM );
	tileNum->Disable();

	// Set the Add layer button.
	using namespace Composite::ToolBar;

	ButtonPtr add_button( m_DialogWindow, IDC_MULTITILEMAP_ADD_ENTRY );
	add_button->Execute( I_EXEC_CB_NO_BORDER );
	add_button->Execute( I_EXEC_BUTTON_DAD_ENABLE, 0 );
	add_button->SetImage( m_ButtonImageList, AddIcon,    AddIcon,
											 AddIconDis, AddIconDis,
											 IconWidth,  IconHeight );

	// Validate or change the control state
	EnableDisableInterface();
}

// Create a rollout.
void Dialog::CreateRollup( Composite::Layer* layer ) {
	// Does NOT add a rollup if one is already present.
	if (find(layer) != end())
		return;

	// see if we are inserting layer. This happens on undo/redo.
	bool insertLayer = false;
	iterator it = begin();
	if (layer != NULL && it != end() && begin()->Layer()->Index() > layer->Index()) {
		for( ; it != end(); ++it ) {
			if (it->Layer()->Index() > layer->Index()) {
				insertLayer = true;
			}
		}
	}

	// Add the rollup to the window
	const int i = m_LayerRollupWnd->AppendRollup( hInstance,
		MAKEINTRESOURCE( IDD_MULTITILEMAP_TILE ),
		PanelDlgProc,
		GetString(IDS_TILE),
		LPARAM( this ) );

	HWND wnd( m_LayerRollupWnd->GetPanelDlg(i) );
	::IRollupPanel* panel( m_LayerRollupWnd->GetPanel( wnd ) );

	// Set opening of the rollup if necessary.
	if (layer != NULL)
		m_LayerRollupWnd->SetPanelOpen( i, layer->DialogOpened() );

	if ( layer ) {
		if ( IsPatternFormat() && (size() >= 1) ) {
			size_t a = layer->Index();
			m_LayerRollupWnd->Hide( i );
		}
		else {
			m_LayerRollupWnd->Show( i );
			m_LayerRollupWnd->UpdateLayout();
		}
	}

	// Create and push the new Layer on the list
	if (insertLayer) {
		insert(it, LayerDialog( this, layer, panel, wnd, &m_DragManager, m_ButtonImageList ) );
	}
	else {
		push_front( LayerDialog( this, layer, panel, wnd, &m_DragManager, m_ButtonImageList ) );
	}
	Invalidate();
}

// Delete the first layer or the specified one.
void Dialog::DeleteRollup( Composite::Layer* layer ) 
{
	iterator it( layer ? find(layer) : begin() );
	if (it == end()) {
		return;
	}
	HWND wnd( it->HWnd() );
	int index( m_LayerRollupWnd->GetPanelIndex( wnd ) );
	m_LayerRollupWnd->DeleteRollup( index, 1 );
	erase( it );
}

void Dialog::Rewire( ) 
{
	Composite::Texture *compTex = m_Texture->GetCompositeTexture();
	DbgAssert( size() == compTex->size() );
	if (size() != compTex->size()) {
		return;
	}
	ValueGuard<bool> reloading_guard( m_Reloading, true );
	reverse_iterator dlg_it( rbegin() );
	Composite::Texture::iterator lay_it( compTex->begin() );

	for( ; (dlg_it != rend()) && (lay_it != compTex->end()); ++dlg_it, ++lay_it ) {
		dlg_it->Layer( &*lay_it );
	}

	m_LayerRollupWnd->UpdateLayout();
	compTex->NotifyChanged();
}

void Dialog::ReloadDialog() {
	Composite::Texture *compTex = m_Texture->GetCompositeTexture();
	// Ensure m_Reloading returns to false when we go out.
	ValueGuard<bool> reloading_guard( m_Reloading, true );

	Interval valid;
	m_Texture->Update(m_MtlParams->GetTime(), valid);

	// Update number of layers
	EditPtr layer_edit( m_DialogWindow, IDC_MULTITILEMAP_TILE_NUM );
	DbgAssert(false == layer_edit.IsNull());

	if (layer_edit.IsNull()) {
		return;
	}

	_tostringstream oss;
	oss << compTex->nb_layer();
	layer_edit->SetText( oss.str().c_str() );

	// Update sub-dialogs (in reverse order so that the Name is correct)
	for( iterator it  = begin(); it != end(); ++it ) {
		Composite::Layer* layer( it->Layer() );
		if ( layer == NULL )
			continue;

		int Index( m_LayerRollupWnd->GetPanelIndex( it->HWnd() ) );
		m_LayerRollupWnd->SetPanelOpen ( Index, layer->DialogOpened() );
		m_LayerRollupWnd->SetPanelTitle( Index, const_cast<LPTSTR>(it->Title().c_str()) );
		it->UpdateDialog();
	}
}

void Dialog::SetTime(TimeValue t) {
	Composite::Texture *compTex = m_Texture->GetCompositeTexture();
	int ct =  compTex->GetParamBlock(0)->Count( Composite::BlockParam::Opacity );
	for(iterator it = begin(); it != end(); it++) {
		it->SetTime( t );

		Interval iv;
		iv.SetInfinite();
		Texmap *map = it->Layer()->Texture();
		if (map) {
			iv.SetInfinite();
			iv = map->Validity(t);
			//if it is animated we need to invalidate the swatch on time change
			if (!(iv == FOREVER)) {
				it->UpdateTexture();
			}
		}
	}

	if (!compTex->GetValidity().InInterval(t)) {
		UpdateMtlDisplay();
	}
}

// Nothing to do here since everything is managed by SmartPtr or MultiTiled.
Dialog::~Dialog() {

	m_Texture->ParamDialog( NULL );

	// Delete rollup from the material editor
	m_MtlParams->DeleteRollupPage( m_DialogWindow );
}

TilePatternFormat Dialog::GetPatternFormat() const 
{ 
	DbgAssert( m_Texture );
	if ( m_Texture ) {
		return m_Texture->GetPatternFormat();
	}
	return TilePatternFormat::Invalid;
}

void Dialog::SetPatternFormat( const TilePatternFormat format )
{ 
	HWND hCombo = GetDlgItem( m_DialogWindow, IDC_MULTITILEMAP_PATTERN_FORMAT );
	if ( hCombo ) {
		::SendMessage( hCombo, CB_SETCURSEL, static_cast<WPARAM>(format), 0 );
	}
}

bool Dialog::IsPatternFormat() const
{
	DbgAssert( m_Texture );
	if ( m_Texture ) {
		return m_Texture->IsPatternFormat();
	}
	return false;
}

ViewportQuality Dialog::GetViewportQuality() const 
{
	DbgAssert( m_Texture );
	if ( m_Texture ) {
		return m_Texture->GetViewportQuality();
	}
	return ViewportQuality::Invalid;
}

void Dialog::SetViewportQuality( ViewportQuality quality )
{
	HWND hCombo = GetDlgItem( m_DialogWindow, IDC_MULTITILEMAP_VIEWPORT_QUALITY );
	if ( hCombo ) {
		::SendMessage( hCombo, CB_SETCURSEL, static_cast<WPARAM>(quality), 0 );
	}
}

bool Dialog::GetUVOffset( int tileIndex, int *uOffset, int *vOffset ) const 
{
	DbgAssert( m_Texture );
	if ( m_Texture ) {
		m_Texture->GetUVOffset( tileIndex, uOffset, vOffset );
	}
	return false;
}

bool Dialog::SetUVOffset( int tileIndex, int uOffset, int vOffset ) 
{
	DbgAssert( m_Texture );
	if ( m_Texture ) {
		m_Texture->SetUVOffset( tileIndex, uOffset, vOffset );
	}
	return false;
}

void Dialog::OnUOffsetSpinChanged( HWND handle, iterator LayerDialog ) {
	SpinPtr spin( handle, IDC_MULTITILEMAP_U_OFFSET_SPIN );
	const int tileIndex = static_cast<int>( LayerDialog->Layer()->Index() );
	int vOffset;
	m_Texture->GetUVOffset( tileIndex, nullptr, &vOffset );
	m_Texture->SetUVOffset( tileIndex, spin->GetIVal(), vOffset );
	m_Texture->NotifyChanged();
	LayerDialog->UpdateUOffset();
}

void Dialog::OnUOffsetEditChanged( HWND handle ) {
	EditPtr edit( handle, IDC_MULTITILEMAP_U_OFFSET );
	if (edit->GotReturn()) {
		if ( m_oldUOffsetVal.isNull() ) {
			return;
		}
		OnUOffsetFocusLost( handle );
		edit->GetText(m_oldUOffsetVal);
	}
}

void Dialog::OnUOffsetFocusLost( HWND handle ) {
	iterator it ( find( handle ) );
	DbgAssert( it != end() );
	if (it == end()) {
		return;
	}

	EditPtr edit( handle, IDC_MULTITILEMAP_U_OFFSET );
	BOOL valid( TRUE );
	int uOffset( edit->GetInt( &valid ) );
	if (valid) {
		int vOffset;
		const int tileIndex = static_cast<int>( it->Layer()->Index() );
		m_Texture->GetUVOffset( tileIndex, nullptr, &vOffset );
		m_Texture->SetUVOffset( tileIndex, uOffset, vOffset );
		m_Texture->NotifyChanged();
		it->UpdateUOffset( );
	}
}

void Dialog::OnVOffsetSpinChanged( HWND handle, iterator LayerDialog ) {
	SpinPtr spin( handle, IDC_MULTITILEMAP_V_OFFSET_SPIN );
	const int tileIndex = static_cast<int>( LayerDialog->Layer()->Index() );
	int uOffset;
	m_Texture->GetUVOffset( tileIndex, &uOffset, nullptr );
	m_Texture->SetUVOffset( tileIndex, uOffset, spin->GetIVal() );
	m_Texture->NotifyChanged();
	LayerDialog->UpdateVOffset();
}

void Dialog::OnVOffsetEditChanged( HWND handle ) {
	EditPtr edit( handle, IDC_MULTITILEMAP_V_OFFSET );
	if (edit->GotReturn()) {
		if ( m_oldVOffsetVal.isNull() ) {
			return;
		}
		OnVOffsetFocusLost( handle );
		edit->GetText(m_oldVOffsetVal);
	}
}

void Dialog::OnVOffsetFocusLost( HWND handle ) {
	iterator it ( find( handle ) );
	DbgAssert( it != end() );
	if (it == end()) {
		return;
	}

	EditPtr edit( handle, IDC_MULTITILEMAP_V_OFFSET );
	BOOL valid( TRUE );
	int vOffset( edit->GetInt( &valid ) );
	if (valid) {
		int uOffset;
		const int tileIndex = static_cast<int>( it->Layer()->Index() );
		m_Texture->GetUVOffset( tileIndex, &uOffset, nullptr );
		m_Texture->SetUVOffset( tileIndex, uOffset, vOffset );
		m_Texture->NotifyChanged();
		it->UpdateVOffset( );
	}
}

// Window Proc for all the layers and the main dialog.  This could really use a clean
// up and add a method by event, then calling them. Ideally put those in a map
// outside this function, thus reducing it to less than a couple of lines.
INT_PTR Dialog::PanelProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) {
	int id( LOWORD(wParam) );
	int code( HIWORD(wParam) );
	bool updateViewPort = false;
	Composite::Texture *compTex = m_Texture ? m_Texture->GetCompositeTexture() : nullptr;

	switch (msg) {
	case WM_PAINT: {
			if (m_Redraw && hWnd == m_DialogWindow) {
				m_Redraw = false;
				ReloadDialog();
			}
		}
		break;
	// When recalculating the layout of the rollups (happening when a rollup is opened/closed
	// by the user), we update the state of the rollup in the ParamBlock.
	case WM_CUSTROLLUP_RECALCLAYOUT: {
			if (!m_Reloading) {
				StartChange();
				for(iterator it = begin(); it != end(); it++) {
					int i( m_LayerRollupWnd->GetPanelIndex( it->HWnd() ) );
					if (it->Layer()->DialogOpened() != m_LayerRollupWnd->IsPanelOpen( i )) {
						it->Layer()->DialogOpened( m_LayerRollupWnd->IsPanelOpen( i ) );
					}
				}
				StopChange( IDS_DS_UNDO_DLG_OPENED );
			}
		}
		break;
	case WM_COMMAND: {
			iterator it( find( hWnd ) );
			switch( id ) {
			case IDC_MULTITILEMAP_TEX: { // Texture button
					DbgAssert( it != end() );
					if (it == end()) {
						break;
					}
					// Create a temp BitmapTex for open dialog and retrieve file path
					MaxSDK::AutoPtr<BitmapTex, ReferenceTargetDestructor<BitmapTex>> dummy = 
						static_cast<BitmapTex*>(CreateInstance( TEXMAP_CLASS_ID, Class_ID(BMTEX_CLASS_ID, 0) ));
					if ( dummy.IsNull() ) { 
						break; 
					}
					dummy->BitmapLoadDlg();
					const MCHAR *mapFullPath = dummy->GetMapName();
					if ( !_tcscmp(mapFullPath, _T("")) ) {
						// User press cancel button, ignore it.
						break;
					}
					if ( IsPatternFormat() ) {
						if ( m_Texture && !m_Texture->SetPatternedImageFile(mapFullPath) ) {
							// invalid file pattern, do nothing
							MessageBox( 0, GetString(IDS_MULTTILE_INVALID_FILE_PATTERN),
								GetString(IDS_ERROR_TITLE),
								MB_APPLMODAL | MB_OK | MB_ICONERROR );
							break;
						}
					}
					else if ( m_Texture ) {
						m_Texture->SetImageFile( static_cast<int>(it->Layer()->Index()), mapFullPath );
					}
					updateViewPort = true;
				}
				break;
			case IDC_MULTITILEMAP_DEL_ENTRY: { // Delete button in toolbar.
					DbgAssert( it != end() );
					if (it == end()) {
						break;
					}
					if ( !IsPatternFormat() && m_Texture ) {
						StartChange();
						m_Texture->DeleteTile( it->Layer()->Index() );
						StopChange( IDS_MULTTILE_UNDO_DEL_TILE );
					}
					DbgAssert( compTex && compTex->nb_layer() > 0 );
					updateViewPort = true;
				}
				break;
			case IDC_MULTITILEMAP_U_OFFSET: { // u offset edit in toolbar.
					EditPtr edit( hWnd, IDC_MULTITILEMAP_U_OFFSET );
					if (code == EN_SETFOCUS) {
						edit->GetText( m_oldUOffsetVal );
					}
					else if( code == EN_KILLFOCUS || code == EN_CHANGE ) {
						DbgAssert( it != end() );
						if (it == end()) {
							break;
						}
						StartChange();
						if (code == EN_KILLFOCUS) {
							TSTR newUOffsetVal;
							edit->GetText(newUOffsetVal);
							if ( m_oldUOffsetVal != newUOffsetVal ) {
								OnUOffsetFocusLost( hWnd );
							}
						}
						else {
							OnUOffsetEditChanged( hWnd );
						}
						StopChange( IDS_MULTTILE_UNDO_UOFFSET );
						updateViewPort = true;
					}
				}
				break;
			case IDC_MULTITILEMAP_V_OFFSET: { // v offset edit in toolbar.
					EditPtr edit( hWnd, IDC_MULTITILEMAP_V_OFFSET );
					if (code == EN_SETFOCUS) {
						edit->GetText( m_oldVOffsetVal );
					}
					else if( code == EN_KILLFOCUS || code == EN_CHANGE ) {
						DbgAssert( it != end() );
						if (it == end()) {
							break;
						}
						StartChange();
						if (code == EN_KILLFOCUS) {
							TSTR newVOffsetVal;
							edit->GetText(newVOffsetVal);
							if ( m_oldVOffsetVal != newVOffsetVal ) {
								OnVOffsetFocusLost( hWnd );
							}
						}
						else {
							OnVOffsetEditChanged( hWnd );
						}
						StopChange( IDS_MULTTILE_UNDO_VOFFSET );
						updateViewPort = true;
					}
				}
				break;
			case IDC_MULTITILEMAP_PATTERN_FORMAT: { // Tiling mode combo in toolbar
					if (code == CBN_SELCHANGE) {
						const TilePatternFormat newTileMode = 
							static_cast<TilePatternFormat>( SendMessage(HWND(lParam), CB_GETCURSEL, 0, 0) );
						if ( m_Texture ) {
							if ( m_Texture->GetPatternFormat() != newTileMode ) {
								StartChange();
								m_Texture->SetPatternFormat( newTileMode );
								StopChange( IDS_MULTTILE_UNDO_SET_PATTERN_FORMAT );
							}
						}
					}
					updateViewPort = true;
				}
				break;
			case IDC_MULTITILEMAP_VIEWPORT_QUALITY: { // Viewport quality in toolbar.
					if (code == CBN_SELCHANGE) {
						const ViewportQuality newViewportQuality = 
							static_cast<ViewportQuality>( SendMessage(HWND(lParam), CB_GETCURSEL, 0, 0) );
						if ( m_Texture ) {
							if ( m_Texture->GetViewportQuality() != newViewportQuality ) {
								StartChange();
								m_Texture->SetViewportQuality( newViewportQuality );
								StopChange( IDS_MULTTILE_UNDO_SET_VIEWPORT_QUALITY );
							}
						}
					}
					updateViewPort = true;
				}
				break;
			case IDC_MULTITILEMAP_ADD_ENTRY: { // Add tile
					StartChange();
					if ( m_Texture ) {
						m_Texture->AddTile();
					}
					StopChange( IDS_MULTTILE_UNDO_ADD_TILE );
					updateViewPort = true;
				}
				break;
			default:
				break;
			}
		}
		break;

	// When a spinner is pressed
	case CC_SPINNER_BUTTONDOWN:
		StartChange();
		break;

	// When a spinner is released, check if it's the opacity spinner and make change.
	case CC_SPINNER_BUTTONUP: { 
			switch( id ) {
			case IDC_MULTITILEMAP_U_OFFSET_SPIN: {
					StopChange( IDS_MULTTILE_UNDO_UOFFSET );
					//we only update the viewport on spinner up since it slows things down alot
					//on very simple cases
					updateViewPort = true;
				}
				break;
			case IDC_MULTITILEMAP_V_OFFSET_SPIN: {
					StopChange( IDS_MULTTILE_UNDO_VOFFSET );
					//we only update the viewport on spinner up since it slows things down alot
					//on very simple cases
					updateViewPort = true;
				}
				break;
			}
		}
		break;

	// If value was changed, update layer.
	case CC_SPINNER_CHANGE: {
			iterator it( find( hWnd ) );
			DbgAssert( it != end() );
			if (it == end()) {
				break;
			}
			switch( id ) {
			case IDC_MULTITILEMAP_U_OFFSET_SPIN: {
					OnUOffsetSpinChanged( hWnd, it );
				}
				break;
			case IDC_MULTITILEMAP_V_OFFSET_SPIN: {
					OnVOffsetSpinChanged( hWnd, it );
				}
				break;
			}
		}
		break;

	case WM_DESTROY:
		Destroy(hWnd);
		break;
	}

	if (updateViewPort) {
		TimeValue t = GetCOREInterface()->GetTime();
		GetCOREInterface()->RedrawViews(t);
	}
	return FALSE;
}

ReferenceTarget* Dialog::GetThing() 
{
	return (ReferenceTarget*)m_Texture;
}

void Dialog::SetThing(ReferenceTarget *m) 
{
	if ( m == nullptr || 
		 m->ClassID() != m_Texture->ClassID() || 
		 m->SuperClassID() != m_Texture->SuperClassID() ) {
		DbgAssert( false );
		return;
	}

	// Change current MultiTileMap, then switch.
	m_Texture->ParamDialog( NULL );

	m_Texture = static_cast<Texture*>(m);
	Composite::Texture *compTex = m_Texture->GetCompositeTexture();
	size_t nb( compTex->nb_layer() );

	// Update the number of rollup shown
	while( size() != nb ) {
		if (size() < nb)
			CreateRollup( NULL );
		else
			DeleteRollup( NULL );
	}

	// Finally set the paramdialog of the new material.
	m_Texture->ParamDialog( this );

	// Validate or change the control state
	Rewire();
	EnableDisableInterface();
	Invalidate();
}

void Dialog::UpdateMtlDisplay() 
{
	m_MtlParams->MtlChanged();
}

void Dialog::EnableDisableInterface() 
{
	if (!m_Texture) {
		return;
	}
	Composite::Texture *compTex = m_Texture->GetCompositeTexture();
	if ( !compTex ) {
		return;
	}
	if ( IsPatternFormat() || (compTex->nb_layer() == 1) ) {
		std::for_each( begin(), end(), [](LayerDialog& dlg) {
			dlg.DisableRemove();
		});
	}
	else {
		// Enable delete entry button in Custom mode, and layer num > 1
		std::for_each( begin(), end(), [](LayerDialog& dlg) {
			dlg.EnableRemove();
		});
	}

	if ( IsPatternFormat() ) {
		std::for_each( begin(), end(), [](LayerDialog& dlg) {
			dlg.DisableUVOffset();
		});
	}
	else {
		std::for_each( begin(), end(), [](LayerDialog& dlg) {
			dlg.EnableUVOffset();
		});
	}

	// If we're in custom pattern mode, re-enable the add button.
	ButtonPtr add_button( m_DialogWindow, IDC_MULTITILEMAP_ADD_ENTRY );
	if (compTex->nb_layer() >= Composite::Param::LayerMax) {
		add_button->Disable();
	} 
	else if ( IsPatternFormat() ) {
		add_button->Disable();
	}
	else {
		add_button->Enable();
	}

	EditPtr layer_edit( m_DialogWindow, IDC_MULTITILEMAP_TILE_NUM );
	if ( !layer_edit.IsNull() )
	{
		_tostringstream oss;
		oss << compTex->nb_layer();
		layer_edit->SetText(oss.str().c_str());
	}
}

Dialog::iterator MultiTileMap::Dialog::find( HWND handle )
{
	if (!handle) {
		return end();
	}

	return find_if( begin(), end(), [&](LayerDialog& d) -> bool { 
		return d == handle; 
	});
}

Dialog::iterator MultiTileMap::Dialog::find( Composite::Layer* layer )
{
	if (!layer) {
		return end();
	}

	return find_if( begin(), end(), [&](LayerDialog &d) -> bool { 
		return *(d.Layer()) == *layer; 
	});
}

Dialog::iterator MultiTileMap::Dialog::find( int index )
{
	if (index < 0) {
		return end();
	}

	return find_if( begin(), end(), [&](LayerDialog& d) -> bool { 
		return d.Layer()->Index() == index; 
	});
}

void Dialog::Invalidate()
{
	m_Redraw = true;
	::InvalidateRect( m_EditorWnd, 0, FALSE );
}

INT_PTR Dialog::PanelDlgProc(HWND hWnd, UINT msg,
										WPARAM wParam,
										LPARAM lParam)
{
	Dialog *dlg = DLGetWindowLongPtr<Dialog*>( hWnd );
	if (msg == WM_INITDIALOG && dlg == 0) {
		dlg = (Dialog*)lParam;
		DLSetWindowLongPtr( hWnd, lParam );
	}

	if (dlg)
		return dlg->PanelProc( hWnd, msg, wParam, lParam );
	else
		return FALSE;
}

void Dialog::StartChange()
{
	theHold.Begin(); 
}

void Dialog::StopChange( int resource_id ) 
{ 
	theHold.Accept( ::GetString( resource_id ) ); 
}

void Dialog::CancelChange() 
{
	theHold.Cancel(); 
}

}	// namespace MultiTileMap
