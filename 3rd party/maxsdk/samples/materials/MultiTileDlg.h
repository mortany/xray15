//
// Copyright [2015] Autodesk, Inc.  All rights reserved. 
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc. and
// are protected under applicable copyright and trade secret law.  They may
// not be disclosed to, copied or used by any third party without the prior
// written consent of Autodesk, Inc.
//
#ifndef __MULTITILEDLG_H__
#define __MULTITILEDLG_H__

#include "Multitile.h"
#include "composite.inl"

namespace MultiTileMap 
{

class LayerDialog;
class Texture;

class Dialog : public ParamDlg
{
public:
	typedef LayerDialog                    value_type;
	typedef std::list<value_type>          list_type;
	typedef list_type::iterator            iterator;
	typedef list_type::const_iterator      const_iterator;
	typedef list_type::reverse_iterator    reverse_iterator;

	// Public STD Sequence methods
	iterator          begin()        { return m_DialogList.begin();  }
	iterator          end()          { return m_DialogList.end();    }
	const_iterator    begin()  const { return m_DialogList.begin();  }
	const_iterator    end()    const { return m_DialogList.end();    }
	reverse_iterator  rbegin()       { return m_DialogList.rbegin(); }
	reverse_iterator  rend()         { return m_DialogList.rend();   }
	size_t            size()         { return m_DialogList.size();   }
	void              push_back ( const value_type& value )              { m_DialogList.push_back( value );     }
	void              push_front( const value_type& value )              { m_DialogList.push_front( value );    }
	void              erase     ( iterator it )                          { m_DialogList.erase( it );            }
	void              insert ( iterator where, const value_type& value ) { m_DialogList.insert( where, value ); }

	Dialog(HWND editor_wnd, IMtlParams *imp, Texture *m);
	~Dialog();

	// Find a layerdialog by its HWND.
	iterator find( HWND handle );
	// Find a layerdialog by its layer.
	iterator find( Composite::Layer* l );
	//find the layer by its index
	iterator find( int index );

	// Public methods
	// Create a new rollup for a layer. If the corresponding dialoglayer already exists
	// does nothing.
	void                  CreateRollup  ( Composite::Layer* l );
	// Delete the rollup for the layer, if it exists.
	void                  DeleteRollup  ( Composite::Layer* l );
	// Rewire all rollups to conform to m_Texture internal list of layers, adding
	// and deleting rollups if necessary.
	void                  Rewire        ( );

	Texture*              GetTexture( ) const { return m_Texture; }

	TilePatternFormat     GetPatternFormat() const;
	void                  SetPatternFormat( const TilePatternFormat format );
	bool                  IsPatternFormat() const;

	ViewportQuality       GetViewportQuality() const;
	void                  SetViewportQuality( ViewportQuality quality );

	// Get/Set specified tile's UV offset
	bool                  GetUVOffset( int tileIndex, int *uOffset, int *vOffset ) const;
	bool                  SetUVOffset( int tileIndex, int uOffset, int vOffset );
	void                  OnUOffsetSpinChanged( HWND handle, iterator LayerDialog );
	void                  OnUOffsetEditChanged( HWND handle );
	void                  OnUOffsetFocusLost( HWND handle );
	void                  OnVOffsetSpinChanged( HWND handle, iterator LayerDialog );
	void                  OnVOffsetEditChanged( HWND handle );
	void                  OnVOffsetFocusLost( HWND handle );

	// ParamDlg virtual methods
	INT_PTR               PanelProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
	void                  ReloadDialog() override;
	void                  UpdateMtlDisplay();
	void                  ActivateDlg(BOOL onOff) override {}

	void                  Invalidate();
	void                  Destroy(HWND hWnd) {}

	// methods inherited from ParamDlg:
	Class_ID              ClassID() override { return GetMultiTileDesc()->ClassID(); }
	void                  SetThing(ReferenceTarget *m) override;
	ReferenceTarget*      GetThing() override;
	virtual void          DeleteThis() override { delete this; }
	void                  SetTime(TimeValue t) override;

	void                  EnableDisableInterface();

private:
	// Invalid calls.
	template< typename T > void operator=(T);
	template< typename T > void operator=(T) const;
	Dialog();
	Dialog(const Dialog&);

	static INT_PTR CALLBACK PanelDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	bool						m_Redraw;
	bool						m_Reloading;

	// Texture Editor properties.
	IMtlParams					*m_MtlParams;
	HWND						m_EditorWnd;

	// Rollup Window for Layer list
	RollupWindowPtr				m_LayerRollupWnd;
	HWND						m_DialogWindow;

	// Drag&Drop Texture manager.
	TexDADMgr					m_DragManager;

	// The MultiTileMap Texture being edited.
	Texture						*m_Texture;

	list_type					m_DialogList;

	HIMAGELISTPtr				m_ButtonImageList;

	TSTR						m_oldUOffsetVal;
	TSTR						m_oldVOffsetVal;

	// Private utility methods
	void StartChange();
	void StopChange( int resource_id );
	void CancelChange();
};

}	// namespace MultiTileMap

#endif // __MULTITILEDLG_H__
