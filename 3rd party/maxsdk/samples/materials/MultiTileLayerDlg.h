//
// Copyright [2015] Autodesk, Inc.  All rights reserved. 
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc. and
// are protected under applicable copyright and trade secret law.  They may
// not be disclosed to, copied or used by any third party without the prior
// written consent of Autodesk, Inc.
//
#ifndef __MULTITILE_LAYERDLG_H__
#define __MULTITILE_LAYERDLG_H__

#include "composite.inl"

namespace MultiTileMap 
{

class Dialog;

// A class representing the view of Layer in a Dialog.
// Does not need to be hooked to a rollup, but needs to have
// a valid Layer class for parameters.
// This means that you could pop a Dialog or put the Layer
// in any window without modifying this class.
class LayerDialog 
{
public:
	LayerDialog( Dialog *dialog, Composite::Layer* layer, IRollupPanel* panel, HWND handle, DADMgr* dadManager, HIMAGELISTPtr imlButtons );

	LayerDialog( const LayerDialog& src );

	// Accessors
	Composite::Layer *Layer() const { return m_Layer; }
	void Layer( Composite::Layer* v ) { m_Layer = v; }
	HWND HWnd() const { return m_hWnd; }
	_tstring Title() const;

	void SetTextureButtonText( const MCHAR *text );

	void swap( LayerDialog& src );

	bool operator ==( HWND handle ) const {
		return m_hWnd == handle;
	}

	// Update the content of the fields
	void UpdateDialog() const;
	void UpdateTexture() const;
	// Update UV offset through texmap value
	void UpdateUOffset() const;
	void UpdateVOffset() const;
	// Set UV offset UI controller value
	void SetUOffset( int uOffset );
	void SetVOffset( int vOffset );

	void SetTime(TimeValue t) {
		Layer()->CurrentTime( t );
	}

	void DisableRemove();
	void EnableRemove();

	void DisableUVOffset();
	void EnableUVOffset();

private:
	LayerDialog();

	Composite::Layer *m_Layer;
	HWND m_hWnd;
	HIMAGELISTPtr        m_ButtonImageList;
	Dialog *m_Dialog;

	// operators
	LayerDialog& operator=( const LayerDialog& );
	bool operator ==( const LayerDialog& ) const;

	void init( DADMgr* dragManager );

	// Initialize the mask and texture buttons.
	void initButton( DADMgr* dragManager );

	// Initialize ALL the buttons.
	void initToolbar();
};

}	// namespace MultiTileMap

#endif // __MULTITILE_LAYERDLG_H__
