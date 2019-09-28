//
// Copyright [2015] Autodesk, Inc.  All rights reserved. 
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc. and
// are protected under applicable copyright and trade secret law.  They may
// not be disclosed to, copied or used by any third party without the prior
// written consent of Autodesk, Inc.
//

#include "MultiTileLayerDlg.h"
#include "MultiTileDlg.h"
#include "max.h"
#include "util.h"
#include "mtlres.h"
#include "mtlhdr.h"
#include "FilePatternParser.h"
#include <sstream>

namespace MultiTileMap 
{

LayerDialog::LayerDialog( Dialog *dialog, Composite::Layer* layer, IRollupPanel* panel, HWND handle, DADMgr* dadManager, HIMAGELISTPtr imlButtons )
	: m_Dialog( dialog )
	, m_Layer( layer )
	, m_hWnd( handle )
	, m_ButtonImageList( imlButtons )
{
	init( dadManager );
}

LayerDialog::LayerDialog( const LayerDialog& src )
	: m_Dialog( src.m_Dialog )
	, m_Layer( src.m_Layer )
	, m_hWnd( src.m_hWnd )
	, m_ButtonImageList( src.m_ButtonImageList )
{
	UpdateDialog();
}

_tstring LayerDialog::Title() const
{
	if (m_Layer) {
		_tostringstream oss;
		oss << GetString(IDS_TILE) << (m_Layer->Index() + 1);
		return oss.str();
	}
	return _tstring();
}

void LayerDialog::SetTextureButtonText( const MCHAR *text ) 
{
	if ( text ) {
		ButtonPtr texButton( GetDlgItem(m_hWnd, IDC_MULTITILEMAP_TEX) );
		if ( !texButton.IsNull() )
		{
			texButton->SetText( text );
		}
	}
}

void LayerDialog::swap( LayerDialog& src ) 
{
	std::swap( m_Layer, src.m_Layer );
	std::swap( m_ButtonImageList,    src.m_ButtonImageList   );
}

void LayerDialog::DisableRemove() 
{
	ButtonPtr button( m_hWnd, IDC_MULTITILEMAP_DEL_ENTRY );
	button->Disable();
}

void LayerDialog::EnableRemove() 
{
	ButtonPtr button( m_hWnd, IDC_MULTITILEMAP_DEL_ENTRY );
	button->Enable();
}

void LayerDialog::DisableUVOffset()
{
	EditPtr uButton( m_hWnd, IDC_MULTITILEMAP_U_OFFSET );
	uButton->Disable();
	SpinPtr uSpin( m_hWnd, IDC_MULTITILEMAP_U_OFFSET_SPIN );
	uSpin->Disable();
	EditPtr vButton( m_hWnd, IDC_MULTITILEMAP_V_OFFSET );
	vButton->Disable();
	SpinPtr vSpin( m_hWnd, IDC_MULTITILEMAP_V_OFFSET_SPIN );
	vSpin->Disable();
}

void LayerDialog::EnableUVOffset()
{
	EditPtr uButton( m_hWnd, IDC_MULTITILEMAP_U_OFFSET );
	uButton->Enable();
	SpinPtr uSpin( m_hWnd, IDC_MULTITILEMAP_U_OFFSET_SPIN );
	uSpin->Enable();
	EditPtr vButton( m_hWnd, IDC_MULTITILEMAP_V_OFFSET );
	vButton->Enable();
	SpinPtr vSpin( m_hWnd, IDC_MULTITILEMAP_V_OFFSET_SPIN );
	vSpin->Enable();
}

void LayerDialog::init( DADMgr* dragManager ) 
{
	// Set the Texture buttons and image list.
	initButton( dragManager );

	// Set the custom buttons.
	initToolbar();

	// Set the spinner for u offset
	SpinPtr UOffsetSpin( m_hWnd, IDC_MULTITILEMAP_U_OFFSET_SPIN );
	UOffsetSpin->SetLimits ( Default::UVOffsetMin, Default::UVOffsetMax  );
	EditPtr UOffsetEdit( m_hWnd, IDC_MULTITILEMAP_U_OFFSET );
	UOffsetEdit->WantReturn( TRUE );
	
	// Set the spinner for v offset
	SpinPtr VOffsetSpin( m_hWnd, IDC_MULTITILEMAP_V_OFFSET_SPIN );
	VOffsetSpin->SetLimits ( Default::UVOffsetMin, Default::UVOffsetMax  );
	EditPtr VOffsetEdit( m_hWnd, IDC_MULTITILEMAP_V_OFFSET );
	VOffsetEdit->WantReturn( TRUE );

	// Update the controls.
	UpdateDialog();
}

// Initialize the mask and texture buttons.
void LayerDialog::initButton( DADMgr* dragManager ) 
{
	ButtonPtr tex_button( m_hWnd, IDC_MULTITILEMAP_TEX  );
	tex_button->SetDADMgr( dragManager );
}

// Update the texture button (render the texmap and reshow it)
void LayerDialog::UpdateTexture( ) const 
{
	ButtonPtr tex_button( m_hWnd, IDC_MULTITILEMAP_TEX );
	if ( tex_button.IsNull() ) {
		return;
	}
	// No Update when no Texture
	if ( m_Layer ) {
		if ( m_Layer->Texture() ) {
			BitmapTex *bmTex = dynamic_cast<BitmapTex*>( m_Layer->Texture() );
			if ( bmTex ) {
				_tstring fileName = FilePatternParser::ExtractFileName( bmTex->GetMapName() );
				if ( fileName.empty() ) {
					tex_button->SetText( GetString( IDS_DS_NO_TEXTURE ) );
				}
				else {
					tex_button->SetText( fileName.c_str() );
				}
			}
		} 
		else {
			tex_button->SetText( GetString( IDS_DS_NO_TEXTURE ) );
		}
	}
}

void LayerDialog::UpdateUOffset() const
{
	EditPtr edit( m_hWnd, IDC_MULTITILEMAP_U_OFFSET );
	SpinPtr spin( m_hWnd, IDC_MULTITILEMAP_U_OFFSET_SPIN);
	Texture *tex = m_Dialog ? m_Dialog->GetTexture() : nullptr;
	if ( m_Layer && tex && edit && spin ) {
		int uOffset;
		const bool valid = tex->GetUVOffset( static_cast<int>(m_Layer->Index()), &uOffset, nullptr );
		edit->SetText ( valid ? uOffset : 0 );
		spin->SetValue( valid ? uOffset : 0, FALSE );
	}
}

void LayerDialog::UpdateVOffset() const
{
	EditPtr edit( m_hWnd, IDC_MULTITILEMAP_V_OFFSET );
	SpinPtr spin( m_hWnd, IDC_MULTITILEMAP_V_OFFSET_SPIN);
	Texture *tex = m_Dialog ? m_Dialog->GetTexture() : nullptr;
	if ( m_Layer && tex && edit && spin ) {
		int vOffset;
		const bool valid = tex->GetUVOffset( static_cast<int>(m_Layer->Index()), nullptr, &vOffset );
		edit->SetText ( valid ? vOffset : 0 );
		spin->SetValue( valid ? vOffset : 0, FALSE );
	}
}

void LayerDialog::SetUOffset( int uOffset )
{
	EditPtr edit( m_hWnd, IDC_MULTITILEMAP_U_OFFSET );
	SpinPtr spin( m_hWnd, IDC_MULTITILEMAP_U_OFFSET_SPIN);
	Texture *tex = m_Dialog ? m_Dialog->GetTexture() : nullptr;
	if ( m_Layer && tex && edit && spin ) {
		edit->SetText ( uOffset );
		spin->SetValue( uOffset, FALSE );
	}
}

void LayerDialog::SetVOffset( int vOffset )
{
	EditPtr edit( m_hWnd, IDC_MULTITILEMAP_V_OFFSET );
	SpinPtr spin( m_hWnd, IDC_MULTITILEMAP_V_OFFSET_SPIN);
	Texture *tex = m_Dialog ? m_Dialog->GetTexture() : nullptr;
	if ( m_Layer && tex && edit && spin ) {
		edit->SetText ( vOffset );
		spin->SetValue( vOffset, FALSE );
	}
}

// Updates all the fields in the dialog.
void LayerDialog::UpdateDialog() const 
{
	if (m_Layer == nullptr) {
		return;
	}

	UpdateTexture();
	UpdateUOffset();
	UpdateVOffset();
}

void LayerDialog::initToolbar()
{
	using namespace Composite::ToolBar;

	ButtonPtr deleteBtn( m_hWnd, IDC_MULTITILEMAP_DEL_ENTRY );
	deleteBtn->SetImage( m_ButtonImageList, RemoveIcon,    RemoveIcon,
											RemoveIconDis, RemoveIconDis,
											IconHeight,    IconWidth );
	deleteBtn->Execute( I_EXEC_CB_NO_BORDER );
	deleteBtn->Execute( I_EXEC_BUTTON_DAD_ENABLE, 0 );

	if ( m_Dialog ) {
		m_Dialog->IsPatternFormat() ? DisableRemove() : EnableRemove();
	}
}

}	// namespace MultiTileMap
