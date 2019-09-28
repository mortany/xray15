//
// Copyright [2015] Autodesk, Inc.  All rights reserved. 
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc. and
// are protected under applicable copyright and trade secret law.  They may
// not be disclosed to, copied or used by any third party without the prior
// written consent of Autodesk, Inc.
//

#ifndef __COMPOSITE_INL__
#define __COMPOSITE_INL__

// Standard headers
#include <list>
#include <vector>

// Max headers
#include "stdmat.h"

// Custom headers
#include "util.h"
#include "composite.h"
#include "Graphics/ITextureDisplay.h"

// A Layer description.
// Contains code to handle the Dialog as well.
namespace Composite {
	// Used to diff between a function which acts on cache and real data and
	// cache only.
	struct CacheOnly {};

	// Fore declaration of Layer
	class Layer;

	/////////////////////////////////////////////////////////////////////////////
#pragma region Layer Data

	// A class encapsulating one single parameter of one Layer
	template< typename Type, ParamID Id >
	class LayerParam {
	private:
		mutable Type m_Cache;
		void SetCache( const Type& value ) const {
			m_Cache = value;
		}

		Layer *m_Layer;

		LayerParam();
	public:
		LayerParam( Layer* aLayer             ) : m_Layer(aLayer), m_Cache(Type()) {}
		LayerParam( Layer* aLayer, Type value ) : m_Layer(aLayer) {
			operator=(value);
		}
		LayerParam( Layer* aLayer, const LayerParam<Type,Id>& value )
			: m_Layer( aLayer )
			, m_Cache( value.m_Cache )
		{}

		// We can make a copy-ction from a Param with a different ID,
		// as long as the type is same.
		template< ParamID IdSrc >
		LayerParam( const LayerParam<Type,IdSrc>& src )
			: m_Layer   ( src.m_Layer   )
			, m_Cache    ( src.m_Cache   )
		{}

		template< ParamID IdSrc >
		void swap( LayerParam< Type, IdSrc >& src ) {
			// Swap the values, the layers and the controllers.
			DbgAssert(m_Layer->ParamBlock() == src.m_Layer->ParamBlock());
			IParamBlock2 *iPB2 = m_Layer->ParamBlock();

			int       idx1 ( (int)    m_Layer->Index() );
			int       idx2 ( (int)src.m_Layer->Index() );
			iPB2->SwapControllers(Id, idx1, IdSrc, idx2);

			std::swap( m_Layer, src.m_Layer );
		}

		// Operators
		const LayerParam& operator=( const LayerParam& src ) {
			LayerParam cpy( src ); swap( cpy ); return *this;
		}

		operator Type() const { 
			UpdateCache(); return m_Cache; 
		}

		const LayerParam& operator =(const Type& value) {
			SetCache( value );
			UpdateParamBlock();
			return *this;
		}

		bool IsKeyFrame() const {
			return IsKeyFrame( m_Layer->CurrentTime() );
		}

		bool IsKeyFrame( TimeValue t ) const {
			return FALSE != m_Layer->ParamBlock()->KeyFrameAtTimeByID( Id, t, int(m_Layer->Index()) );
		}

		// This function copy the tracks and is called notably during cloning of the layer.
		// This is templated since we can copy tracks from basically any params.
		template< typename TypeSrc, ParamID IdSrc >
		void SetController( const LayerParam<TypeSrc,IdSrc>* src, RemapDir &remap ) {
			IParamBlock2* pbsrc( src->m_Layer->ParamBlock() );
			IParamBlock2* pbdst(      m_Layer->ParamBlock() );
			size_t        isrc ( src->m_Layer->Index() );
			size_t        idst (      m_Layer->Index() );

			Control *ctrl = pbsrc->GetControllerByID( IdSrc, int(isrc) );
			pbdst->SetControllerByID( Id, int(idst), (Control*)remap.CloneRef(ctrl), false );
		}

		bool HasController( ) const {
			return m_Layer->ParamBlock()->GetControllerByID( Id, int(m_Layer->Index()) ) != NULL;
		}

		void Layer( Layer* layer ) { 
			m_Layer = layer;
		}

		void UpdateCache( ) const {
			m_Layer->Update(m_Layer->CurrentTime(), Interval());
		}

		void Update( TimeValue t, Interval& valid ) const {
			if (m_Layer->ParamBlock() && m_Layer->ParamBlock()->Count( Id ) > m_Layer->Index()) {
				m_Layer->ParamBlock()->GetValue( Id, t,
					m_Cache,
					valid,
					int(m_Layer->Index()) );
			}
		}

		void UpdateParamBlock() const {
			DbgAssert(m_Layer->ParamBlock() && m_Layer->ParamBlock()->Count( Id ) > m_Layer->Index());
			if (m_Layer->ParamBlock() && m_Layer->ParamBlock()->Count( Id ) > m_Layer->Index()) {
				m_Layer->ParamBlock()->SetValue( Id, m_Layer->CurrentTime(),
					m_Cache,
					int(m_Layer->Index()) );
			}
		}
	};

	/////////////////////////////////////////////////////////////////////////////
#pragma region LayerParam Specializations

	// Specialization for texmaps
	template< ParamID Id >
	class LayerParam< Texmap*, Id > {
	private:
		Layer             *m_Layer;
		mutable Texmap    *m_Cache;
		void SetCache( Texmap *value ) const {
			m_Cache = value;
		}

		LayerParam();
	public:
		LayerParam( Layer* aLayer                ) : m_Layer(aLayer), m_Cache(NULL) {}
		LayerParam( Layer* aLayer, Texmap* value ) : m_Layer(aLayer) {
			operator=(value);
		}
		LayerParam( Layer* aLayer, const LayerParam<Texmap*,Id>& value )
			: m_Layer( aLayer )
			, m_Cache( value.m_Cache )
		{}

		// We can make a copy-ction from a Param with a different ID,
		// as long as the type is same.
		template< ParamID IdSrc >
		LayerParam( const LayerParam<Texmap*,IdSrc>& src )
			: m_Layer    ( src.m_Layer   )
			, m_Cache    ( src.m_Cache   )
		{}

		template< ParamID IdSrc >
		void swap( LayerParam< Texmap*, IdSrc >& src ) {

			int       idx1 ( (int)    m_Layer->Index() );
			int       idx2 ( (int)src.m_Layer->Index() );
			Texmap  *map1 = NULL;
			m_Layer->ParamBlock()->GetValue( Id,0,map1, FOREVER,   idx1 );
			Texmap  *map2 = NULL;
			src.m_Layer->ParamBlock()->GetValue( IdSrc,0,map2, FOREVER, idx2 );
			if (map1) map1->SetAFlag(A_LOCK_TARGET);
			if (map2) map2->SetAFlag(A_LOCK_TARGET);
			m_Layer->ParamBlock()->SetValue(Id,0,map2,idx1);
			src.m_Layer->ParamBlock()->SetValue(IdSrc,0,map1,idx2);
			if (map1) map1->ClearAFlag(A_LOCK_TARGET);
			if (map2) map2->ClearAFlag(A_LOCK_TARGET);

			std::swap( m_Layer, src.m_Layer   );
		}

		// Operators
		const LayerParam& operator=( const LayerParam& src ) {
			LayerParam cpy( src ); swap( cpy ); return *this;
		}

		operator Texmap*()       { UpdateCache(); return m_Cache; }
		operator Texmap*() const { UpdateCache(); return m_Cache; }

		const LayerParam& operator =(Texmap* value) {
			if ( m_Cache == value )
			{
				// no need to proceed.
				return *this;
			}
			if ( m_Cache != NULL ) {
				SetCache(NULL);
				UpdateParamBlock();
			}
			SetCache(value);
			UpdateParamBlock();
			return *this;
		}

		void        Layer( Layer* l ) {
			m_Layer = l;
		}

		void UpdateCache( ) const {
			m_Layer->Update(m_Layer->CurrentTime(), Interval());
		}

		void Update( TimeValue t, Interval& valid ) const {
			if (m_Layer->ParamBlock() && m_Layer->ParamBlock()->Count( Id ) > m_Layer->Index()) {
				m_Layer->ParamBlock()->GetValue( Id,t,
					m_Cache,
					valid,
					int(m_Layer->Index()) );
			}
		}

		void UpdateParamBlock() const {
			DbgAssert(m_Layer->ParamBlock() && m_Layer->ParamBlock()->Count( Id ) > m_Layer->Index());
			if (m_Layer->ParamBlock() && m_Layer->ParamBlock()->Count( Id ) > m_Layer->Index()) {
				m_Layer->ParamBlock()->SetValue( Id, m_Layer->CurrentTime(),
					m_Cache,
					int(m_Layer->Index()) );
			}
		}
	};

	// Specialization for strings
	template< ParamID Id >
	class LayerParam< LPTSTR, Id > {
	private:
		mutable _tstring   m_Cache;
		void SetCache( const _tstring& value ) const {
			m_Cache = value;
		}

		Layer             *m_Layer;

		LayerParam();
	public:
		LayerParam( Layer* aLayer               ) : m_Layer(aLayer), m_Cache(_tstring()) {}
		LayerParam( Layer* aLayer, LPTSTR value ) : m_Layer(aLayer) {
			operator=(value);
		}

		LayerParam( Layer* aLayer, const LayerParam<LPTSTR,Id>& value )
			: m_Layer( aLayer )
			, m_Cache( value.m_Cache )
		{}

		// We can make a copy-ction from a Param with a different ID,
		// as long as the type is same.
		template< ParamID IdSrc >
		LayerParam( const LayerParam<LPTSTR,IdSrc>& src )
			: m_Layer    ( src.m_Layer   )
			, m_Cache    ( src.m_Cache   )
		{}

		template< ParamID IdSrc >
		void swap( LayerParam< LPTSTR, IdSrc >& src ) {
			// Make sure operators get called and
			// pblock stays valid.
			_tstring tmp( (LPCTSTR)*this );
			operator=((LPCTSTR)src);
			src = tmp.c_str();

			std::swap( m_Layer, src.m_Layer   );
		}

		// Operators
		const LayerParam& operator=( const LayerParam& src ) {
			LayerParam cpy( src ); swap( cpy ); return *this;
		}

		operator LPCTSTR() const {
			UpdateCache();
			return m_Cache.c_str();
		}
		operator const _tstring() const {
			UpdateCache();
			return m_Cache;
		}

		const LayerParam& operator =(const LPCTSTR value) {
			SetCache( _tstring(value) );
			UpdateParamBlock();
			return *this;
		}

		const LayerParam& operator =(const _tstring value) {
			SetCache( value );
			UpdateParamBlock();
			return *this;
		}

		void Layer( Layer* l ) {
			m_Layer = l;
			UpdateParamBlock();
		}

		void UpdateCache( ) const {
			m_Layer->Update(m_Layer->CurrentTime(), Interval());
		}

		void Update( TimeValue t, Interval& valid ) const {
			if (m_Layer->ParamBlock() && m_Layer->ParamBlock()->Count( Id ) > m_Layer->Index()) {
				const TCHAR* retVal;
				m_Layer->ParamBlock()->GetValue( Id, t,
					retVal,
					valid,
					int(m_Layer->Index()) );

				// Old version might not have had a value, so we keep default
				// if the pointer returned is NULL.
				if (retVal != NULL)
					m_Cache = retVal;
			}
		}

		void UpdateParamBlock() const {
			DbgAssert(m_Layer->ParamBlock() && m_Layer->ParamBlock()->Count( Id ) > m_Layer->Index());
			if (m_Layer->ParamBlock() && m_Layer->ParamBlock()->Count( Id ) > m_Layer->Index()) {
				m_Layer->ParamBlock()->SetValue( Id, m_Layer->CurrentTime(),
					const_cast<LPTSTR>(m_Cache.c_str()),
					int(m_Layer->Index()) );
			}
		}
	};

#pragma endregion

}

#pragma endregion

	/////////////////////////////////////////////////////////////////////////////

#pragma region Composite Layer

namespace Composite {
	class Layer {
	public:
		// Public typedefs
		typedef Texmap* TextureType;
		typedef BOOL    VisibleType;
		typedef float   OpacityType;
		typedef int     BlendType;
		typedef LPTSTR  NameType;
		typedef BOOL    DialogOpenedType;

	private:
		IParamBlock2	*m_ParamBlock;
		size_t			m_Index;
		TimeValue		m_CurrentTime;
		Interval		m_Validity;

		LayerParam< VisibleType,      BlockParam::Visible        > m_Visible;
		LayerParam< VisibleType,      BlockParam::VisibleMask    > m_VisibleMask;
		LayerParam< BlendType,        BlockParam::BlendMode      > m_Blend;
		LayerParam< NameType,         BlockParam::Name           > m_Name;
		LayerParam< DialogOpenedType, BlockParam::DialogOpened   > m_DialogOpened;
		LayerParam< OpacityType,      BlockParam::Opacity        > m_Opacity;
		LayerParam< TextureType,      BlockParam::Texture        > m_Texture;
		LayerParam< TextureType,      BlockParam::Mask           > m_Mask;

		// This cache is used specifically to return a pointer to a managed buffer
		// that stays in-scope.  Basically, there is the cache of the m_Name itself,
		// but since there is a modification to the Name (to include Texture Name,
		// etc), we need another cache.
		mutable _tstring m_NameCache;

	public:
		Layer( IParamBlock2 *pblock, size_t Index );

		// Copy-constructor
		Layer( const Layer& src );
		////////////////////////////////////////////////////////////////////////////////

		// Accessors
		// Name
		NameType Name( )      const;
		void     Name( const NameType v );

		// Return the name with all the special variable replaced by their values.
		// This uses a stringstream to pipe every character except if they correspond
		// to the special tags. If so, it pipes the value instead. Returns the content
		// of the pipe in a cache to ensure the pointer stays valid outside the
		// scope of this function.
		NameType ProcessName( ) const;

		// Texture
		TextureType Texture( )               const { return m_Texture; }
		void        Texture( const TextureType v ) { m_Texture = v;    }

		// Mask
		TextureType Mask( )               const { return m_Mask; }
		void        Mask( const TextureType v ) { m_Mask = v; }

		// Visible
		VisibleType Visible( )               const { return m_Visible; }
		void        Visible( const VisibleType v ) { m_Visible = v;    }
		void        ToggleVisible( )         { m_Visible = !m_Visible; }

		// VisibleMask
		VisibleType VisibleMask( )               const { return m_VisibleMask; }
		void        VisibleMask( const VisibleType v ) { m_VisibleMask = v;    }
		void        ToggleVisibleMask( )         { m_VisibleMask = !m_VisibleMask; }

		// Opacity
		// Here we clamp on read because the value in ParamBlock might not be
		// clamped (when using a Controller or MXS, for example), without our
		// setter being called.
		OpacityType Opacity( ) const;
		void        Opacity( const OpacityType v ) { m_Opacity = v; }
		float       NormalizedOpacity() const;
		bool        IsOpacityKeyFrame() const {
			return m_Opacity.IsKeyFrame();
		}

		// BlendMode
		// The Blend is not clamped, but instead defaulted to a value.
		BlendType Blend( ) const { return m_Blend; }
		void      Blend( const BlendType v );

		// Dialog Opened
		BlendType DialogOpened( )              const { return m_DialogOpened; }
		void      DialogOpened( const DialogOpenedType v ) { m_DialogOpened = v; }

		// Index
		size_t Index( )    const { return m_Index; }
		void   Index( const size_t v ) { m_Index = v; }

		// ParamBlock
		void          ParamBlock( IParamBlock2 *pblock ) { m_ParamBlock = pblock; }
		IParamBlock2 *ParamBlock( )                const { return m_ParamBlock; }

		// Validators for texture, mask and visibility
		bool HasTexture   () const { return Texture() != NULL; }
		bool HasMask      () const { return Mask() != NULL; }
		bool IsMaskVisible() const { return VisibleMask() && HasMask(); }
		bool IsVisible    () const { return Visible() && Opacity() > 0.0f && HasTexture(); }

		TimeValue CurrentTime(             ) const { return m_CurrentTime; }
		void      CurrentTime( const TimeValue v ) { m_CurrentTime = v; }

		// Common functions
		void swap( Layer& src );

		// Swap only the values of the layers.
		void SwapValue( Layer& src );

		void CopyValue( const Layer& src );

		// Operators
		Layer& operator=( const Layer& src ) {
			Layer cpy( src ); swap( cpy ); return *this;
		}
		bool operator==( const Layer& rhv ) const {
			return rhv.m_Index      == m_Index
				&& rhv.m_ParamBlock == m_ParamBlock;
		}

		// Update the Texmaps.
		void Update( TimeValue t, Interval& valid );

		void Invalidate() {
			m_Validity.SetEmpty();
		}
	};
}

#pragma endregion

/////////////////////////////////////////////////////////////////////////////

#pragma region Composite Texture

class IHardwareMaterial;

namespace Composite {
	// Forward declaration
	class Dialog;

	class Texture : 
		public MultiTex, 
		public Composite::Interface,
		public MaxSDK::Graphics::ITextureDisplay
	{
	public:
		// Public Sequence Types
		typedef Layer                       value_type;
		typedef std::list<value_type>       list_type;
		typedef list_type::iterator         iterator;
		typedef list_type::const_iterator   const_iterator;
		typedef list_type::reverse_iterator reverse_iterator;
		typedef list_type::difference_type  difference_type;

		// Public Sequence methods
		iterator          begin()        { return m_LayerList.begin();  }
		iterator          end()          { return m_LayerList.end();    }
		const_iterator    begin()  const { return m_LayerList.begin();  }
		const_iterator    end()    const { return m_LayerList.end();    }
		reverse_iterator  rbegin()       { return m_LayerList.rbegin(); }
		reverse_iterator  rend()         { return m_LayerList.rend();   }
		size_t            size()   const { return m_LayerList.size();   }

		void              push_back( const value_type& value ) {
			m_LayerList.push_back( value ); }
		// does not drop refs
		void              erase    ( iterator it ) { m_LayerList.erase( it ); }
		void              insert   ( iterator where, const value_type& value ) { 
			m_LayerList.insert( where, value ); }

		//////////////////////////////////////////////////////////////////////////
		// Utility Functions
		// There's a lot here. Mainly, if you want to change the reference index
		// system, you would just change the following functions.
		size_t nb_layer  (            ) const throw() { return size(); }
		size_t nb_texture(            ) const throw() { return nb_layer() * 2; }

		// Return true if the index is the one of a texture or a mask.
		bool   is_mask   ( size_t idx ) const throw() { return idx >= nb_layer(); }
		bool   is_tex    ( size_t idx ) const throw() { return idx <  nb_layer(); }

		// Is it a valid texture index?
		bool   is_valid  ( size_t idx ) const throw() {
			return is_mask(idx) ? (mask_idx_to_lyr(idx)< nb_layer())
								: (tex_idx_to_lyr(idx) < nb_layer());
		}

		// Return the index of a texture/mask from the index of a layer
		size_t lyr_to_tex_idx  ( size_t lyr ) const throw() { return lyr; }
		size_t lyr_to_mask_id  ( size_t lyr ) const throw() { return lyr + nb_layer(); }

		// Return the index of a layer from the index of a texture/mask.
		size_t tex_idx_to_lyr ( size_t idx ) const throw() { return idx; }
		size_t mask_idx_to_lyr( size_t idx ) const throw() { return tex_idx_to_lyr( idx - nb_layer() ); }

		// Return the index of a layer from an index (generic)
		size_t idx_to_lyr ( size_t idx ) const throw() {
			return is_mask(idx) ? mask_idx_to_lyr(idx)
								: tex_idx_to_lyr (idx); }

		// Return true if the index is the one of a texture or a mask.
		bool   is_subtexmap_idx_mask   ( size_t idx ) const throw() { return ((idx%2) == 1); }
		bool   is_subtexmap_idx_tex    ( size_t idx ) const throw() { return !is_subtexmap_idx_mask(idx); }

		// Is it a valid SubTexmap index?
		bool   is_subtexmap_idx_valid  ( size_t idx ) const throw() {
			return idx < (nb_layer()*2);
		}

		// Return the index of a layer from a SubTexmap index
		size_t subtexmap_idx_to_lyr ( size_t idx ) const throw() {
			return idx/2; }

		// Get an iterator to an indice. This is to prevent problems if changing the
		// type of iterator to a BidirectionalIterator instead of a RandomIterator.
		iterator LayerIterator( size_t lyr ) throw() {
			iterator ret( begin() );
			std::advance( ret, lyr );
			return ret;
		}
		const_iterator LayerIterator( size_t lyr ) const throw() {
			const_iterator ret( begin() );
			std::advance( ret, lyr );
			return ret;
		}

		// Return the number of Visible layers
		size_t CountVisibleLayer() const throw();

		// Return the number of masks
		size_t CountMaskedLayer() const throw();

		//////////////////////////////////////////////////////////////////////////
		// ctor & dtor
		Texture(BOOL loading = FALSE);
		~Texture();

		// Accessors
		_tstring      DefaultLayerName( )       const;
		Dialog*       ParamDialog( )            const { return m_ParamDialog; }
		void          ParamDialog(Dialog* v)          { m_ParamDialog = v; }

		// MultiTex virtuals
		void           SetNumSubTexmaps(int n)        { SetNumMaps(n); }

		// MtlBase virtuals
		ParamDlg*      CreateParamDlg(HWND editor_wnd, IMtlParams *imp);

		// ISubMap virtuals
		void           SetSubTexmap(int i, Texmap *m);
		int            NumSubTexmaps() { return int(nb_texture()); }
		Texmap*        GetSubTexmap(int i);
		TSTR           GetSubTexmapSlotName(int i);

		// Animatable virtuals
		int            SubNumToRefNum( int subNum ) { return subNum ; }
		int            NumSubs( ) { return 1;}
		Animatable*    SubAnim(int i) {
			if (i == 0)
				return m_ParamBlock;
			if (m_FileVersion != kMax2009)
			{
				DbgAssert(0);  //I dont think we should get here
				return NULL;
			}
			return NULL;
		}
		TSTR SubAnimName(int i);

		// Texmap virtuals
		AColor         EvalColor(ShadeContext& sc);
		Point3         EvalNormalPerturb(ShadeContext& sc);
		bool           IsLocalOutputMeaningful( ShadeContext& sc );

		// Implementing functions for MAXScript.
		BaseInterface* GetInterface(Interface_ID id);

		void MXS_AddLayer();

		int MXS_Count() {
			return (int)nb_layer();
		}

		void MXS_DeleteLayer( int index );

		void MXS_DuplicateLayer( int index );

		void MXS_MoveLayer ( int from_index, int to_index, bool before );

		// Public functions
		void AddLayer( );
		void DeleteLayer( size_t Index = 0 );
		void InsertLayer( size_t Index = 0 );

		void MoveLayer( size_t Index, size_t dest, bool before );
		void DuplicateLayer( size_t Index );

		void Update(TimeValue t, Interval& valid);
		void Init();
		void Reset();
		Interval GetValidity() { return m_Validity; }
		Interval Validity(TimeValue t) {
			Interval v = FOREVER;
			Update(t,v);
			return v;
		}
		void NotifyChanged();
		void SetNumMaps( int n,  bool setName = true );

		void UpdateParamBlock   ( size_t count );
		void OnParamBlockChanged( ParamID id );
		void ColorCorrection    ( Composite::Layer* layer, bool mask );

		void ValidateParamBlock();

		// Class management
		Class_ID    ClassID()             { return GetCompositeDesc()->ClassID();      }
		SClass_ID   SuperClassID()        { return GetCompositeDesc()->SuperClassID(); }
		void        GetClassName(TSTR& s) { s = GetCompositeDesc()->ClassName();       }
		void        DeleteThis()          { delete this;                               }

		// From ReferenceMaker
		int             NumRefs();
		RefTargetHandle GetReference( int i );
private:
		virtual void            SetReference( int i, RefTargetHandle v );
public:
		RefTargetHandle Clone( RemapDir &remap );
		RefResult       NotifyRefChanged( 
			const Interval &changeInt,
			RefTargetHandle hTarget,
			PartID         &partID,
			RefMessage      message, 
			BOOL            propagate );

		// IO
		IOResult Save(ISave *);
		IOResult Load(ILoad *);

		// return number of ParamBlocks in this instance
		int           NumParamBlocks   (     ) { return 1; }
		IParamBlock2 *GetParamBlock    (int i) { return m_ParamBlock; }

		// return id'd ParamBlock
		IParamBlock2 *GetParamBlockByID(BlockID id) {
			return	(m_ParamBlock && m_ParamBlock->ID() == id)
					? m_ParamBlock
					: NULL;
		}

		// Multiple map in vp support -- DS 5/4/00
		BOOL SupportTexDisplay()                 { return TRUE; }
		BOOL SupportsMultiMapsInViewport()       { return TRUE; }

		void ActivateTexDisplay(BOOL onoff);
		void SetupGfxMultiMaps(TimeValue t, ::Material *material, MtlMakerCallback &cb);

		//this is used to load old file
		//2008 files just get remapped ok
		//	the maps gets mapped into the owner ref ID of the param block and there are no masks
		//JohnsonBeta files up to J049 are hosed up and need to have major reference reworked
		//	the maps still go into the owner ref of the param block
		//	mask get moved from owner ref to paramblock controlled
		//this flag is set on load when a johnson beta file is loaded so we can get all the references
		//then it is unset in the post load call back after all the mask have been moved into the correct
		//paramblock
		enum FileVersion {
			kMax2008 = 0,
			kJohnsonBeta,
			kMax2009
		};
		FileVersion	m_FileVersion;
		//this is the list of owner ref maps we load from older files
		int		    m_OldMapCount_2008;
		int		    m_OldMapCount_JohnsonBeta;

		//on load if we are loading a 2008 or JohnsonBeta file all the maps and masks will end up 
		//here and in the post load call back we will move them into the paramblock
		Tab<Texmap*>  m_oldMapList;

		// From ITextureDisplay
		virtual void SetupTextures(TimeValue t, MaxSDK::Graphics::DisplayTextureHelper &cb);
	private:
		// Invalid calls.
		template< typename T > void operator=(T);
		template< typename T > void operator=(T) const;
		Texture(const Texture&);

		// Member variables
		IParamBlock2         *m_ParamBlock;
		Interval              m_Validity;
		Interval			  m_mapValidity;

		list_type             m_LayerList;
		Dialog               *m_ParamDialog;

		// Saved members
		_tstring              m_DefaultLayerName;

		// Guards
		bool                  m_Updating;
		bool                  m_SettingNumMaps;

		// used for OGS VP display
		std::vector<TexHandle*>    m_DisplayTexHandles;
		Interval      m_DisplayTexValidInterval;

		IColorCorrectionMgr::CorrectionMode mColorCorrectionMode;

	public:
		// Pushing and removing layers from the vector (not the param block)
		void PushLayer( bool setName );
		void PopLayer ( size_t Index = 0, bool dragndrop = false );

	private:
		/////////////////////////////////////////////////////////////////////////
		// For the interactive renderer
		struct viewport_texture {
			viewport_texture( TexHandlePtr h, iterator l, bool mask = false )
				: m_hWnd ( h )
				, m_Layer( l )
				, m_Mask ( mask )
			{}

			TexHandlePtr  handle() { return m_hWnd;  }
			iterator      Layer()  { return m_Layer; }
			bool          IsMask() { return m_Mask;  }

		private:
			TexHandlePtr  m_hWnd;  // The handle to the Texture
			bool          m_Mask;  // The mask

			// Yes, adding or removing a layer to/from the list is going to
			// invalidate this iterator, but it will trigger a REFMSG_CHANGE
			// (if using the methods) that will destroy the reference.
			iterator      m_Layer; // The Layer shown
		};

		typedef std::vector< viewport_texture > ViewportTextureListType;

		ViewportTextureListType m_ViewportTextureList;
		Interval                m_ViewportIval;

		void InitMultimap      ( TimeValue  time,
								 MtlMakerCallback &callback );
		void PrepareHWMultimap ( TimeValue  time,
								 IHardwareMaterial *hw_material,
								 ::Material *material,
								 MtlMakerCallback &callback  );
		void PrepareSWMultimap ( TimeValue  time,
								 ::Material *material,
								 MtlMakerCallback &callback  );

		void ResetDisplayTexHandles();
		bool CreateViewportDisplayBitmap(PBITMAPINFO& pOutputMap,
			TimeValue time, 
			TexHandleMaker& callback, 
			Interval& validInterval, 
			iterator& it,
			bool bForceSize,
			int forceWidth,
			int forceHeight);
	};
}

#pragma endregion

////////////////////////////////////////////////////////////////////////////////
#pragma region Layer Dialog
namespace Composite {

	// A class representing the view of Layer in a Dialog.
	// Does not need to be hooked to a rollup, but needs to have
	// a valid Layer class for parameters.
	// This means that you could pop a Dialog or put the Layer
	// in any window without modifying this class.
	class LayerDialog {
		// Internal class to ease tooltip management for the combo.
		struct ComboToolTipManager {
			ComboToolTipManager( HWND hWnd, int resTextId );
			~ComboToolTipManager();
		private:
			HWND m_hComboToolTip;
		};

	public:
		LayerDialog( Layer* layer, IRollupPanel* panel, HWND handle, DADMgr* dadManager, HIMAGELISTPtr imlButtons );
		LayerDialog( const LayerDialog& src );

		// Accessors
		Layer          *Layer()                  const { return m_Layer; }
		void            Layer( Composite::Layer* v )   { m_Layer = v; }
		IRollupPanel   *Panel()                  const { return m_RollupPanel; }
		const _tstring  Title()                  const {
			if (m_Layer) return m_Layer->ProcessName();
			else return _tstring();
		}

		bool IsTextureButton( HWND handle ) const;
		bool IsMaskButton( HWND handle ) const;

		void swap( LayerDialog& src );

		bool operator ==( HWND handle ) const {
			return m_hWnd == handle;
		}

		// Update the content of the fields
		void        UpdateDialog()                      const;
		void        UpdateOpacity( bool edit = true )   const;

		void        UpdateTexture( )                    const;
		void        UpdateMask   ( )                    const;

		void        UpdateVisibility( )                 const;
		void        UpdateMaskVisibility   ( )          const;

		void        SetTime(TimeValue t) {
			Layer()->CurrentTime( t );
		}

		void        DisableRemove();
		void        EnableRemove();
		bool        IsRemoveEnabled();

	private:
		LayerDialog();

		Composite::Layer    *m_Layer;
		IRollupPanel        *m_RollupPanel;
		HWND                 m_hWnd;
		HIMAGELISTPtr        m_TextureImageList;
		HIMAGELISTPtr        m_ButtonImageList;
		SmartPtr<ComboToolTipManager> m_ComboToolTip;

		// operators
		LayerDialog& operator=( const LayerDialog& );
		bool operator ==( const LayerDialog& ) const;

		void init( DADMgr* dragManager );

		// Initialize the values inside the Combo.
		void initCombo( );

		// Initialize the mask and texture buttons.
		void initButton( DADMgr* dragManager );

		// Initialize ALL the buttons.
		void initToolbar();
	};
}

/////////////////////////////////////////////////////////////////////////////
// Composite Dialog

namespace Composite {
	class Texture;
	class LayerDialog;

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
		iterator find( Layer* l );
		//find the layer by its index
		iterator find( int index );

		// Public methods
		// Create a new rollup for a layer. If the corresponding dialoglayer already exists
		// does nothing.
		void                  CreateRollup  ( Layer* l );
		// Delete the rollup for the layer, if it exists.
		void                  DeleteRollup  ( Layer* l );
		void                  HideRollup(Layer * layer);
		// Rename the layer (using the dialog box).
		void                  RenameLayer   ( LayerDialog* l );
		// Rewire all rollups to conform to m_Texture internal list of layers, adding
		// and deleting rollups if necessary.
		void                  Rewire        ( );
		Texture*              GetTexture    ( ) { return m_Texture; }

		// ParamDlg virtual methods
		INT_PTR               PanelProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
		void				  updateLayersCat();
		void				  updateLayersName();
		void                  openAllRollups();
		void                  ReloadDialog();
		void                  UpdateMtlDisplay();
		void                  ActivateDlg(BOOL onOff) {}

		void                  Invalidate();
		void                  Destroy(HWND hWnd) {}

		// methods inherited from ParamDlg:
		Class_ID              ClassID() { return GetCompositeDesc()->ClassID(); }
		void                  SetThing(ReferenceTarget *m);
		ReferenceTarget*      GetThing();
		virtual void          DeleteThis();
		void                  SetTime(TimeValue t);
		int                   FindSubTexFromHWND(HWND handle);

		void                  EnableDisableInterface();

	private:
		// Invalid calls.
		template< typename T > void operator=(T);
		template< typename T > void operator=(T) const;
		Dialog();
		Dialog(const Dialog&);

		static INT_PTR CALLBACK PanelDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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

		// The Composite Texture being edited.
		Texture						*m_Texture;

		std::list<LayerDialog>		m_DialogList;
		unsigned int				m_CategoryCounter;

		HIMAGELISTPtr				m_ButtonImageList;

		SmartPtr<IRollupCallback>	m_RollupCallback;

		TSTR						m_oldOpacityVal;

		// Private methods
		void OnOpacitySpinChanged( HWND handle, iterator LayerDialog );
		void OnOpacityEditChanged( HWND handle );
		void OnOpacityFocusLost  ( HWND handle );

		// Private utility methods
		void StartChange() { theHold.Begin(); }
		void StopChange( int resource_id );// { theHold.Accept( ::GetString( resource_id ) ); }
		void CancelChange() { theHold.Cancel(); }
	};

	// Some constants
	namespace Param {
		// Maximum number of layers that can be constructed in ui or through FPS methods.
		const unsigned short LayerMax    ( 1024 );

		// Maximum number of layers that can exist, created by reading in from file
		// or by direct access to the PB2 properties via maxscript
		const unsigned int CategoryCounterMax ( 2<<30 );

		// The maximum number of Layer to show in the
		// interactive renderer.
		const unsigned short LayerMaxShown( 4 );
	}

	// Some interface constants
	namespace ToolBar {
		const int   IconHeight           ( 13 );
		const int   IconWidth            ( 13 );

		// Indices for the ToolBar icons in the image list
		const int   RemoveIcon           (  0 );
		const int   VisibleIconOn        (  1 );
		const int   VisibleIconOff       (  2 );
		const int   AddIcon              (  3 );
		const int   RenameIcon           (  4 );
		const int   DuplicateIcon        (  5 );
		const int   ColorCorrectionIcon  (  6 );
		const int   RemoveIconDis        (  7 );
		const int   AddIconDis           (  8 );
	}
}

////////////////////////////////////////////////////////////////////////////////

#endif // __COMPOSITE_INL__