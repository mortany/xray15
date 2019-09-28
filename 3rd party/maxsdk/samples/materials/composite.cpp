/**********************************************************************
 *<
   FILE:        composite.cpp
   DESCRIPTION: A compositor Texture map.  Based on the ole' Composite
                Texture from Rolf Berteig.
   CREATED BY:  Hans Larsen
   HISTORY:     Created 01.aug.2007
 *>   Copyright (c) 2007, All Rights Reserved.
 **********************************************************************/

#pragma region Includes

// Max headers
#include "mtlhdr.h"
#include "mtlres.h"
#include "iparamm2.h"
#include "macrorec.h"
#include "IHardwareMaterial.h"
#include "3dsmaxport.h"
#include "hsv.h"
#include "iFnPub.h"

// Internal project dependencies
#include "color_correction.h"

// Standard headers
#include <sstream>
#include <algorithm>
#include <numeric>
#include <functional>
#include <map>

#include "../../include/Graphics/ISimpleMaterial.h"

#pragma endregion

////////////////////////////////////////////////////////////////////////////////

#pragma region Notes
/******************************************************************************
   Reference system:
      The reference Index works as follow for textures:
         [0]      - The parameter block
         [1..n(   - The Textures (1)
         [n..2n(  - The Masks    (1)
      This system is used to keep the same reference indices between the old
      version and the new one (so that loading the param block will set the
      same references, easing the backward-compatibility contract).
      (1) When using an index instead of a reference number, it is simply
          (as of the writing of this text) the index inside the
          [texture,mask( range of reference numbers.
          Ideally, if you were to add new references, you could add them at
          the beginning and change the indexing functions (see
          Texture::idx_to_lyr) so that the indexing system stays the same
          inside the range. This way the changes will stay minimal.
   ----------------------------------------------------------------------------
   A bit of explanation concerning the class diagram here.  Here it is in
   short (using ASCII UML. <>-> is association, <#>-> is composition):

      *   _____________   1  1   _______   1      *   ________________
      ,->| LayerDialog |<>----->| Layer |<#>-------->| LayerParam<Id> |
      |   ?????????????         ???????             ????????????????
      |                             ^ *
      |                             `-----------.
      |  1   ________   1    1   __________   1 |
      `--<#>| Dialog |<-------<>| Texture |<#>-'
             ????????            ??????????
   So basically, Dialog is a composition of LayerDialog which basically
   manage the update of the interface based on the properties of a Layer.
   Bear in mind that there is an operation (Rewire) that re-associate the
   LayerDialogs to the Layers, and add/remove LayerDialog if necessary. Since
   the only role of LayerDialog is to show information based on its Layer
   pointer, there is no black magic involved.
   ----------------------------------------------------------------------------
   A Layer is composited of multiple LayerParam. Due to backward compatibility,
   it was simpler to put a BlockParam2 that had many tables than one which had
   one table of BlockParam2. Basically, a Layer points to a BlockParam2 (of
   the Texture), has an index, and one LayerParam<Id> for each field
   inside this BlockParam2. The LayerParam<Id> exists only to encapsulate the
   GetValue and SetValue, the Id and a cache if necessary.
   To add a parameter to each layers, simply add the field in the BlockParam2,
   add a LayerParam<Id> to Layer, with the corresponding Id and type, and add
   accessors to this param inside the Layer class. Then, you have to update
   the functions ValidateParamBlock and UpdateParamBlock to ensure the count
   of the field inside the ParamBlock2 is still valid. If you want that
   parameter to show in the interface, change LayerDialog::Update and the
   resource file.
   ----------------------------------------------------------------------------
   If you are new to this code, understand some of the concepts in util.h,
   then start reading the simple code of LayerParam<Id>, then read on Layer
   and LayerDialog and understand them correctly. Blendmodes should be next,
   and after that you'll be able to check Texture and Dialog and their
   implementation.
   ----------------------------------------------------------------------------
   This is a gigantic file but it's separated in regions to ease reading and
   maintenance. The particular structure of the map couldn't let us use the
   param block to its full extent, and this has resulted in a lot of lines that
   mainly link the param block values to their UI elements. The design was
   separated and simplified to leave it easier to maintain.
   ----------------------------------------------------------------------------
   STL was used heavily in this file.  Be sure you have used or understand
   algorithms, streams and sequences.  Some methods from functional were used
   as well.
   ----------------------------------------------------------------------------
   Please note the use of SmartPtr for almost everything that require
   acquisition and release.  This include every GetICust... and ReleaseICust...
   for custom controls as well as many classes.
   This automates memory management and cleans the code and design.
   There's a few "new" in this file, but no delete . Don't be surprised, it's
   all done once the smart pointers pointing to that class are destroyed.
   Same goes for ReleaseICust.
   ----------------------------------------------------------------------------
   Hope this helped.
*******************************************************************************/
#pragma endregion

////////////////////////////////////////////////////////////////////////////////

#pragma region Color Blending

//CODE from hsv.cpp  the old RGBtoHSV would return black on colors < 0 or > 1
//this is a temporary fix to remove that clamp since we dont want to have to test
//all the RGBtoHSV at this stage.  This should be reevaluated for Renior.
#define MAX3(a,b,c) ((a)>(b)?((a)>(c)?(a):(c)):((b)>(c)?(b):(c)))
#define MIN3(a,b,c) ((a)<(b)?((a)<(c)?(a):(c)):((b)<(c)?(b):(c)))

Color RGBtoHSV2(Color rgb)
{
	float h, s, r, g, b, vmx, V, X;
	Color res;

	V = MAX3(rgb.r,rgb.g,rgb.b);
	X = MIN3(rgb.r,rgb.g,rgb.b);

	if (V==X) {
		h = (float)0.;
		s = (float)0.;
	} else {
		vmx = V - X;
		s = vmx/V;
		r = ( V - rgb.r )/vmx;
		g = ( V - rgb.g )/vmx;
		b = ( V - rgb.b )/vmx;
		if( rgb.r == V )
			h = ( rgb.g == X ) ?  (float)5.+b : (float)1.-g;
		else if( rgb.g == V )
			h = ( rgb.b == X ) ? (float)1.+r : (float)3.-b;
		else
			h = ( rgb.r == X ) ? (float)3.+g : (float)5.-r;
		h /= (float)6.;
	}
	res.r = h; 	// h
	res.g = s;	// s
	res.b = V;	// v
	return res;
}

// If you modify the blending modes, don't forget to adapt the ones in the 
// Mental Ray shader. Since it's only an integer that is passed, they MUST be
// in the same order in ModeArray.
namespace Blending {
   struct BaseBlendMode {
      virtual float    operator()( const float    fg,  const float     bg  ) const = 0;
      virtual ::Color  operator()( const ::Color& fg,  const ::Color & bg  ) const = 0;

      virtual _tstring Name      ( )                                      const = 0;
   };

   template< typename SubType >
   struct BlendMode : public BaseBlendMode {
      static const BaseBlendMode* i() { 
         static SubType instance;
         return &instance; 
      }

      // Builds a color, blend it, then return the mix of the result.
      // This might be used in some mono case.
      // THIS MIGHT BE INFINITELY RECURSIVE IF NEITHER OPERATOR IS DEFINED,
      // since they depend on one another, so you can define one operator
      // without the other.
      float operator()( const float fg,  const float bg ) const { 
         ::Color colfg ( fg, fg, fg ); 
         ::Color colbg ( bg, bg, bg );
         // Apply blending.
         ::Color result( (*(i()))(colfg, colbg) );
         return ( result.r + result.g + result.b ) / 3.0f;
      }

		::Color operator()( const Color& fg, const Color& bg ) const {
			return Transform( fg, bg, *(i()) ); }

		_tstring Name() const {
			return _tstring( GetString( SubType::NameResourceID ) ); }
   };

   /////////////////////////////////////////////////////////////////////////////
   // Full list of Blending modes
   // To add one, copy an existing mode, change the value of the NameResourceID
   // enum to the resource used for its Name and change the content of 
   // operator() so it applies the function and returns the value.
   // Finally, to make the Blending type appear in the combo box and be usable
   // in scripts, just add a new reference in the ModeArray initialization.
   // Don't forget to update the Mental Ray shader if you do so.  If the blend
   // mode is invalid in Mental Ray, it will return all black so you'll know.

   // If you define a [float operator()(...)], you won't need to define a
   // [Color operator()(...)] since it will call compose and call the float
   // operator on each component (RGB, A will be a mix of alphas).
   // Likewise, if you define a [Color operator()(...)], the other will only
   // apply it in cases where it needs a float (EvalMono).

   // We estimate that all colors are clamped PRIOR to the call.

   /////////////////////////////////////////////////////////////////////////////
   // Normal
   struct Normal : public BlendMode< Normal > {
      enum { NameResourceID = IDS_BLENDMODE_NORMAL };
		::Color operator()( const ::Color& fg, const ::Color& bg ) const {
			return fg; }
   };
   struct Average : public BlendMode< Average > {
      enum { NameResourceID = IDS_BLENDMODE_AVERAGE };
		float operator()( const float fg, const float bg ) const {
			return (fg + bg) / 2.0f; }
   };
   struct Add : public BlendMode< Add > {
      enum { NameResourceID = IDS_BLENDMODE_ADD };
		float operator()( const float fg, const float bg ) const {
			return  bg + fg; }
   };
   struct Subtract : public BlendMode< Subtract > {
      enum { NameResourceID = IDS_BLENDMODE_SUBTRACT };
		float operator()( const float fg, const float bg ) const {
			return bg - fg; }
   };

   /////////////////////////////////////////////////////////////////////////////
   // Darkening
   struct Darken : public BlendMode< Darken > {
      enum { NameResourceID = IDS_BLENDMODE_DARKEN };
		float operator()( const float fg, const float bg ) const {
			return min( fg, bg ); }
   };
   struct Multiply : public BlendMode< Multiply > {
      enum { NameResourceID = IDS_BLENDMODE_MULTIPLY };
		float operator()( const float fg, const float bg ) const {
			return bg * fg; }
   };
   struct ColorBurn : public BlendMode< ColorBurn > {
      enum { NameResourceID = IDS_BLENDMODE_COLORBURN };
		float operator()( const float fg, const float bg ) const {
			return (fg == 0.0f) ? 0.0f : max( 1.0f - (1.0f - bg) / fg, 0.0f ); }
   };
   struct LinearBurn : public BlendMode< LinearBurn > {
      enum { NameResourceID = IDS_BLENDMODE_LINEARBURN };
		float operator()( const float fg, const float bg ) const {
			return max(fg + bg - 1.0f, 0.0f); }
   };
   
   /////////////////////////////////////////////////////////////////////////////
   // Lightening
   struct Lighten : public BlendMode< Lighten > {
      enum { NameResourceID = IDS_BLENDMODE_LIGHTEN };
		float operator()( const float fg, const float bg ) const {
			return max( fg, bg ); }
   };
   struct Screen : public BlendMode< Screen > {
      enum { NameResourceID = IDS_BLENDMODE_SCREEN };
		float operator()( const float fg, const float bg ) const {
			return fg + bg - fg * bg; }
   };
   struct ColorDodge : public BlendMode< ColorDodge > {
      enum { NameResourceID = IDS_BLENDMODE_COLORDODGE };
		float operator()( const float fg, const float bg ) const {
			return (fg == 1.0f) ? 1.0f : min( bg / (1.0f - fg), 1.0f ); }
   };
   struct LinearDodge : public BlendMode< LinearDodge > {
      enum { NameResourceID = IDS_BLENDMODE_LINEARDODGE };
		float operator()( const float fg, const float bg ) const {
			return min( fg + bg, 1.0f ); }
   };

   /////////////////////////////////////////////////////////////////////////////
   // Spotlights
   struct Spotlight : public BlendMode< Spotlight > {
      enum { NameResourceID = IDS_BLENDMODE_SPOT };
		float operator()( const float fg, const float bg ) const {
			return min( 2.0f * fg * bg, 1.0f ); }
   };
   struct SpotlightBlend : public BlendMode< SpotlightBlend > {
      enum { NameResourceID = IDS_BLENDMODE_SPOTBLEND };
		float operator()( const float fg, const float bg ) const {
			return min( fg * bg + bg, 1.0f ); }
   };

   /////////////////////////////////////////////////////////////////////////////
   // Lighting
   struct Overlay : public BlendMode< Overlay > {
      enum { NameResourceID = IDS_BLENDMODE_OVERLAY };
		float operator()( const float fg, const float bg ) const {
			return clamp( bg <= 0.5f ? 2.0f * fg * bg
                                      : 1.0f - 2.0f * (1.0f-fg) * (1.0f-bg),
                         0.0f, 1.0f ); }
   };
   struct SoftLight : public BlendMode< SoftLight > {
      enum { NameResourceID = IDS_BLENDMODE_SOFTLIGHT };
		float operator()(const float a, const float b) const {
			return clamp( a <= 0.5f ? b * (b + 2.0f*a*(1.0f - b))
									: b + (2.0f*a - 1.0f)*(sqrt(b) - b),
						  0.0f, 1.0f ); }
   };
   struct HardLight : public BlendMode< HardLight > {
      enum { NameResourceID = IDS_BLENDMODE_HARDLIGHT };
		float operator()(const float a, const float b) const {
			return clamp( a <= 0.5f ? 2.0f*a*b
									: 1.0f - 2.0f*(1.0f - a)*(1.0f - b),
						  0.0f, 1.0f ); }
   };
   struct PinLight : public BlendMode< PinLight > {
      enum { NameResourceID = IDS_BLENDMODE_PINLIGHT };
		float operator()(const float a, const float b) const {
			return (a > 0.5f && a > b) || (a < 0.5f && a < b) ? a : b; }
   };
   struct HardMix : public BlendMode< HardMix > {
      enum { NameResourceID = IDS_BLENDMODE_HARDMIX };
		float operator()(const float a, const float b) const {
			return a + b <= 1.0f ? 0.0f : 1.0f; }
   };

   /////////////////////////////////////////////////////////////////////////////
   // Difference
   struct Difference : public BlendMode< Difference > {
      enum { NameResourceID = IDS_BLENDMODE_DIFFERENCE };
		float operator()(const float a, const float b) const {
			return abs(a - b); }
   };
   struct Exclusion : public BlendMode< Exclusion > {
      enum { NameResourceID = IDS_BLENDMODE_EXCLUSION };
		float operator()( const float fg, const float bg ) const {
			return fg + bg - 2.0f * fg * bg; }
   };

   // HSV
   struct Hue : public BlendMode< Hue > {
      enum { NameResourceID = IDS_BLENDMODE_HUE };
      ::Color operator()( const ::Color& fg, const ::Color& bg ) const {
			const ::Color hsv_fg( ::RGBtoHSV2( ::Color(fg.r, fg.g, fg.b) ) );
			const ::Color hsv_bg( ::RGBtoHSV2( ::Color(bg.r, bg.g, bg.b) ) );
         return ::Color(::HSVtoRGB( ::Color( hsv_fg[0], hsv_bg[1], hsv_bg[2] ) ));
      }
   };
   struct Saturation : public BlendMode< Saturation > {
      enum { NameResourceID = IDS_BLENDMODE_SATURATION };
      ::Color operator()( const ::Color& fg, const ::Color& bg ) const {
			const ::Color hsv_fg( ::RGBtoHSV2( ::Color(fg.r, fg.g, fg.b) ) );
			const ::Color hsv_bg( ::RGBtoHSV2( ::Color(bg.r, bg.g, bg.b) ) );
         return ::Color(HSVtoRGB( ::Color( hsv_bg[0], hsv_fg[1], hsv_bg[2] ) ));
      }
   };
   struct Color : public BlendMode< Color > {
      enum { NameResourceID = IDS_BLENDMODE_COLOR };
      ::Color operator()( const ::Color& fg, const ::Color& bg ) const {
			const ::Color hsv_fg( ::RGBtoHSV2( ::Color(fg.r, fg.g, fg.b) ) );
			const ::Color hsv_bg( ::RGBtoHSV2( ::Color(bg.r, bg.g, bg.b) ) );
         return ::Color(HSVtoRGB( ::Color( hsv_fg[0], hsv_fg[1], hsv_bg[2] ) ));
      }
   };
   struct Value : public BlendMode< Value > {
      enum { NameResourceID = IDS_BLENDMODE_VALUE };
      ::Color operator()( const ::Color& fg, const ::Color& bg ) const {
			const ::Color hsv_fg( ::RGBtoHSV2( ::Color(fg.r, fg.g, fg.b) ) );
			const ::Color hsv_bg( ::RGBtoHSV2( ::Color(bg.r, bg.g, bg.b) ) );
         return ::Color(HSVtoRGB( ::Color( hsv_bg[0], hsv_bg[1], hsv_fg[2] ) ));
      }
   };

   /////////////////////////////////////////////////////////////////////////////
   // Creating and managing the static list of blending modes.
   const BaseBlendMode* const ModeArray[] = {
         Normal    ::i(),
         Average   ::i(), Add            ::i(), Subtract  ::i(),
         Darken    ::i(), Multiply       ::i(), ColorBurn ::i(),  LinearBurn::i(),
         Lighten   ::i(), Screen         ::i(), ColorDodge::i(),  LinearDodge::i(),
         Spotlight ::i(), SpotlightBlend ::i(),
         Overlay   ::i(), SoftLight      ::i(), HardLight ::i(),  PinLight::i(), HardMix::i(),
         Difference::i(), Exclusion      ::i(),
         Hue       ::i(), Saturation     ::i(), Color     ::i(),  Value::i()
      };

   struct ModeListType {
      typedef const BaseBlendMode* const * const_iterator;
      const BaseBlendMode& operator []( int Index ) const {
         return *(ModeArray[ Index ]);
      }
      const_iterator begin() const { return &ModeArray[0]; }
      const_iterator end()   const { return &ModeArray[size()]; }
      size_t         size()  const { return _countof(ModeArray); }
   } ModeList;
}
#pragma endregion

////////////////////////////////////////////////////////////////////////////////

#pragma region Constants

// Composite namespace and all internally used global variables.
namespace Composite {

	// Chunk magic numbers.
	namespace Chunk {
		// For compatibility with the old COMPOSITE.
		namespace Compatibility {
			enum {
				Header         = 0x4000,
				Param2         = 0x4010,
				SubTexCount    = 0x0010,
				MapOffset      = 0x1000,
				MaxMapOffset   = 0x2000
			};
		};

		// We don't need the offsets and all that since everything
		// is now owned by the ParamBlock2.
		enum {
			Header            = 0x1000,
			Version           = 0x1001
		};
	};

	// Default values for new layers
	namespace Default {
		const float		Opacity			(  100.0f );
		const bool		Visible			( true );
		const bool		VisibleMask		( true );

		const float		OpacityMax		(  100.0f );
		const float		OpacityMin		(    0.0f );

		const int		BlendMode		(    0 );

		const bool		DialogOpened	( true );

		// Resource Id
		const _tstring Name				( _T("") );
	}


}

////////////////////////////////////////////////////////////////////////
// Static assertions
// These are going to generate compilation error if they are not valid.
namespace {
	// LayerMax for interactive renderer must be under the LayerMax
	// because, obviously, you cannot have more Layer shown than the
	// maximum Layer available.
	C_ASSERT( Composite::Param::LayerMaxShown <= Composite::Param::LayerMax );
}

#pragma endregion

////////////////////////////////////////////////////////////////////////////////

#pragma region Data Holders (Layers)

// Render a Texture and returns its HBITMAP.  If Texture is NULL,
// give back a big blacked HBITMAP.
// This is needed for the preview in the texture buttons.
SmartHBITMAP RenderBitmap( Texmap* Texture, TimeValue t,
						   size_t  width,   size_t    height,
						   int     type )
{
	BitmapInfo  bi;

	bi.SetWidth ( int(width)    );
	bi.SetHeight( int(height)   );
	bi.SetType  ( type          );
	bi.SetFlags ( MAP_HAS_ALPHA );
	bi.SetCustomFlag( 0 );

	Bitmap      *bm   ( TheManager->Create( &bi ) );
	HDC          hdc  ( GetDC( bm->GetWindow() )  );

	if (Texture) {
		// Unsure of why 50 works here, but it does (same as in the Bricks texture)
		Texture->RenderBitmap( t, bm, 50.0f, TRUE );
	}

	PBITMAPINFO  bitex( bm->ToDib() );
	HBITMAP      bmp  ( CreateDIBitmap( hdc, &bitex->bmiHeader,
										CBM_INIT,
										bitex->bmiColors,
										bitex,
										DIB_RGB_COLORS ) );

	// Freeing the allocated resource
	LocalFree( bitex );
	ReleaseDC( GetDesktopWindow(), hdc );
	bm->DeleteThis();

	// Return a managed resource of this...
	return SmartHBITMAP(bmp);
}

#pragma region Layer Implementation

namespace Composite {

	////////////////////////////////////////////////////////////////////////////////
	// Disabling the warning C4355 - this used in base member initializer list
	// The following code has been thought correctly and as such this warning may
	// be disabled.
#pragma warning( push )
#pragma warning( disable: 4355 )
	Layer::Layer( IParamBlock2 *pblock, size_t Index )
		: m_ParamBlock  ( pblock       )
		, m_Index       ( Index        )
		, m_Visible     ( this         )
		, m_Opacity     ( this         )
		, m_Texture     ( this         )
		, m_Mask        ( this         )
		, m_Blend       ( this         )
		, m_VisibleMask ( this         )
		, m_Name        ( this         )
		, m_DialogOpened( this         )
		, m_CurrentTime ( )
		, m_Validity    ( )
	{}

	// Copy-constructor
	Layer::Layer( const Layer& src )
		: m_ParamBlock  ( src.m_ParamBlock         )
		, m_Index       ( src.m_Index              )
		, m_Visible     ( this, src.m_Visible      )
		, m_Opacity     ( this, src.m_Opacity      )
		, m_Texture     ( this, src.m_Texture      )
		, m_Mask        ( this, src.m_Mask         )
		, m_Blend       ( this, src.m_Blend        )
		, m_VisibleMask ( this, src.m_VisibleMask  )
		, m_Name        ( this, src.m_Name         )
		, m_DialogOpened( this, src.m_DialogOpened )
		, m_CurrentTime ( src.m_CurrentTime )
		, m_Validity    ( src.m_Validity )
	{ }
#pragma warning( pop )
	////////////////////////////////////////////////////////////////////////////////

	// Accessors
	// Name
	Layer::NameType Layer::Name( ) const
	{
		return const_cast<LPTSTR>((LPCTSTR)m_Name); 
	}

	void     Layer::Name( const NameType v ) 
	{
		const _tstring tmp(v);
		// Trim the string, removing spaces at the beginning and the end (as well as tabs).
		if (tmp == _T(""))
			m_Name = _T("");
		else
			m_Name = _tstring( v + tmp.find_first_not_of( _T(" \t") ),
			v + tmp.find_last_not_of ( _T(" \t") ) + 1 ).c_str();
	}

	Layer::OpacityType Layer::Opacity( ) const
	{
		return clamp( m_Opacity, Default::OpacityMin, Default::OpacityMax );
	}

	float Layer::NormalizedOpacity() const
	{
		return   float( Opacity() - Default::OpacityMin ) / float(Default::OpacityMax);
	}

	void Layer::Blend( const BlendType v )
	{
		m_Blend = (v < 0 || v >= Blending::ModeList.size()) ? Default::BlendMode : v;
	}

	void Layer::swap( Layer& src ) 
	{
		// create no new keys while setting values...
		AnimateSuspend as (TRUE, TRUE);

		std::swap( m_ParamBlock,   src.m_ParamBlock  );
		std::swap( m_Index,        src.m_Index       );
		std::swap( m_CurrentTime,  src.m_CurrentTime );
		std::swap( m_Validity,     src.m_Validity );
		SwapValue( src );
	}

	// Swap only the values of the layers.
	void Layer::SwapValue( Layer& src ) 
	{
		m_Visible	  .swap ( src.m_Visible );		m_Visible	  .Layer( this ); src.m_Visible		.Layer( &src );
		m_VisibleMask .swap ( src.m_VisibleMask );	m_VisibleMask .Layer( this ); src.m_VisibleMask .Layer( &src );
		m_Blend		  .swap ( src.m_Blend );		m_Blend		  .Layer( this ); src.m_Blend		.Layer( &src );
		m_Name		  .swap ( src.m_Name );			m_Name		  .Layer( this ); src.m_Name		.Layer( &src );
		m_DialogOpened.swap ( src.m_DialogOpened );	m_DialogOpened.Layer( this ); src.m_DialogOpened.Layer( &src );
		m_Opacity	  .swap ( src.m_Opacity );		m_Opacity	  .Layer( this ); src.m_Opacity		.Layer( &src );
		m_Texture	  .swap ( src.m_Texture );		m_Texture	  .Layer( this ); src.m_Texture		.Layer( &src );
		m_Mask		  .swap ( src.m_Mask );			m_Mask		  .Layer( this ); src.m_Mask		.Layer( &src );
	}

	void Layer::CopyValue( const Layer& src ) 
	{
		m_CurrentTime = src.m_CurrentTime;

		m_Blend			= BlendType (		src.m_Blend );
		m_Name          = (LPCTSTR)			src.m_Name;
		m_NameCache     = 					src.m_NameCache;
		m_DialogOpened  = DialogOpenedType( src.m_DialogOpened );

		RemapDir *remap = NewRemapDir();

		if (src.m_Visible.HasController())
			m_Visible.SetController( &src.m_Visible, *remap );
		else
			m_Visible = VisibleType( src.m_Visible );

		if (src.m_VisibleMask.HasController())
			m_VisibleMask.SetController( &src.m_VisibleMask, *remap );
		else
			m_VisibleMask = VisibleType( src.m_VisibleMask );

		if (src.m_Opacity.HasController())
			m_Opacity.SetController( &src.m_Opacity, *remap );
		else
			m_Opacity = OpacityType( src.m_Opacity );

		// Clone the textures.
		HoldSuspend hs;
		Texmap *map = (Texmap *) remap->CloneRef(src.m_Texture);
		Texmap *mask = (Texmap *) remap->CloneRef(src.m_Mask);
		hs.Resume();
		m_Texture = TextureType( map );
		m_Mask =    TextureType( mask );

		remap->DeleteThis();
	}

	void Layer::Update( TimeValue t, Interval& valid ) 
	{
		CurrentTime( t );
		if (!m_Validity.InInterval(t)) {
			m_Validity.SetInfinite();

			m_Visible		.Update( t, m_Validity );
			m_VisibleMask	.Update( t, m_Validity );
			m_Blend			.Update( t, m_Validity );
			m_Name			.Update( t, m_Validity );
			m_DialogOpened	.Update( t, m_Validity );
			m_Opacity		.Update( t, m_Validity );
			m_Texture		.Update( t, m_Validity );
			m_Mask			.Update( t, m_Validity );

			if (Texture())
				Texture()->Update(t,m_Validity);
			if (Mask())
				Mask()->Update(t,m_Validity);

		}
		valid &= m_Validity;
	}

	Layer::NameType Layer::ProcessName() const
	{
		const _tstring &name( m_Name );
		_tostringstream oss;
		oss << name << GetString(IDS_LAYER) << (Index() + 1);
		m_NameCache = oss.str();
		return Layer::NameType( m_NameCache.c_str() );
	}
}

#pragma endregion

/////////////////////////////////////////////////////////////////////////////
#pragma region Layer Dialog Implementations

namespace Composite {

	LayerDialog::ComboToolTipManager::ComboToolTipManager( HWND hWnd, int resTextId )
		: m_hComboToolTip( 0 ) {
		// Hack to add the tooltip to a combo box, which doesn't support (yet) a
		// CustControl interface.
		TOOLINFO ti = {0};
		ti.cbSize		= sizeof(TOOLINFO);
		ti.uFlags		= TTF_SUBCLASS | TTF_IDISHWND;
		ti.hwnd			= hWnd;							// Control's parent window
		ti.hinst		= hInstance;
		ti.uId			= (UINT_PTR)hWnd;				// Control's window handle
		ti.lpszText		= GetString( resTextId );

		// Create a ToolTip window if it not yet created
		// This assure that the tooltip window is created only once
		if (!m_hComboToolTip) {
			m_hComboToolTip = CreateWindowEx( NULL,
				TOOLTIPS_CLASS,
				NULL,
				WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				hWnd,
				NULL,
				hInstance,
				NULL );

			DbgAssert(m_hComboToolTip);
		}

		// Before registering a tool with a ToolTip control, we try to
		// delete it to ensure that the tooltip is added only once.
		SendMessage(m_hComboToolTip, TTM_DELTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
		SendMessage(m_hComboToolTip, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
	}

	LayerDialog::ComboToolTipManager::~ComboToolTipManager() {
		if (m_hComboToolTip) {
			TOOLINFO ti = {0};
			ti.cbSize = sizeof(TOOLINFO);

			// Remove every registered ToolTip with m_hComboToolTip
			while ( SendMessage(m_hComboToolTip, TTM_ENUMTOOLS, 0, LPARAM(&ti)) ) {
				SendMessage(m_hComboToolTip, TTM_DELTOOL, 0, LPARAM(&ti) );

				ZeroMemory(&ti, sizeof(TOOLINFO));
				ti.cbSize = sizeof(TOOLINFO);
			}

			DestroyWindow(m_hComboToolTip);
			m_hComboToolTip = 0;
		}
	}

	LayerDialog::LayerDialog( Composite::Layer* layer, IRollupPanel* panel, HWND handle, DADMgr* dadManager, HIMAGELISTPtr imlButtons )
		: m_Layer            ( layer  )
		, m_RollupPanel      ( panel  )
		, m_hWnd             ( handle )
		, m_TextureImageList ( 0 )
		, m_ButtonImageList  ( imlButtons )
		, m_ComboToolTip     ( new ComboToolTipManager( GetDlgItem( m_hWnd, IDC_COMP_BLEND_TYPE ), IDS_DS_COMP_TOOLTIP_BLEND_MODE ) )
	{
		init( dadManager );
	}

	LayerDialog::LayerDialog( const LayerDialog& src )
		: m_Layer            ( src.m_Layer            )
		, m_RollupPanel      ( src.m_RollupPanel      )
		, m_hWnd             ( src.m_hWnd             )
		, m_TextureImageList ( src.m_TextureImageList )
		, m_ButtonImageList  ( src.m_ButtonImageList  )
		, m_ComboToolTip     ( src.m_ComboToolTip )
	{
		UpdateDialog();
	}

	bool LayerDialog::IsTextureButton( HWND handle ) const {
		HWND button ( ::GetDlgItem( m_hWnd, IDC_COMP_TEX  ) );
		return handle == button;
	}

	bool LayerDialog::IsMaskButton( HWND handle ) const {
		HWND button( ::GetDlgItem( m_hWnd, IDC_COMP_MASK ) );
		return handle == button;
	}

	void LayerDialog::swap( LayerDialog& src ) {
		std::swap( m_Layer,              src.m_Layer             );
		std::swap( m_RollupPanel,        src.m_RollupPanel       );
		std::swap( m_TextureImageList,   src.m_TextureImageList  );
		std::swap( m_ButtonImageList,    src.m_ButtonImageList   );
	}

	void LayerDialog::DisableRemove() {
		ButtonPtr  button( m_hWnd, IDC_COMP_LAYER_REMOVE    );
		button->Disable();
	}

	void LayerDialog::EnableRemove() {
		ButtonPtr  button( m_hWnd, IDC_COMP_LAYER_REMOVE    );
		button->Enable();
	}

	bool LayerDialog::IsRemoveEnabled() {
		ButtonPtr  button( m_hWnd, IDC_COMP_LAYER_REMOVE    );
		return button->IsEnabled() != FALSE;
	}

	void LayerDialog::init( DADMgr* dragManager ) {
		// Fill the combo box for blending modes.
		initCombo();

		// Set the Texture buttons and image list.
		initButton( dragManager );

		// Set the custom buttons.
		initToolbar();

		// Set the spinner for opacity
		SpinPtr OpacitySpin( m_hWnd, IDC_COMP_OPACITY_SPIN );
		EditPtr OpacityEdit( m_RollupPanel->GetHWnd(), IDC_COMP_OPACITY );
		OpacitySpin->SetLimits ( Default::OpacityMin, Default::OpacityMax  );
		OpacityEdit->WantReturn( TRUE );

		// Set tooltip.
		OpacitySpin->SetTooltip( TRUE, GetString( IDS_DS_COMP_TOOLTIP_OPACITY ) );
		OpacityEdit->SetTooltip( TRUE, GetString( IDS_DS_COMP_TOOLTIP_OPACITY ) );

		// Update the controls.
		UpdateDialog();
	}

	// Initialize the values inside the Combo.
	void LayerDialog::initCombo( ) {
		// Set the combo box for Blending modes
		struct predicate : std::binary_function<HWND, const Blending::BaseBlendMode*, void> {
			void operator()( HWND ctrl, const Blending::BaseBlendMode* mode ) const {
				::SendMessage( ctrl, CB_ADDSTRING, 0, LPARAM( mode->Name().c_str() ) );
			};
		} ;

		std::for_each( Blending::ModeList.begin(),
			Blending::ModeList.end(),
			std::bind1st( predicate(),
			GetDlgItem( m_hWnd, IDC_COMP_BLEND_TYPE ) ) );
	}

	// Initialize the mask and texture buttons.
	void LayerDialog::initButton( DADMgr* dragManager ) {
		ButtonPtr  tex_button( m_hWnd, IDC_COMP_TEX  );
		ButtonPtr  msk_button( m_hWnd, IDC_COMP_MASK );

        RECT rc = {};
        GetWindowRect( tex_button->GetHwnd(), &rc );

		int width ( rc.right - rc.left );
		int height( rc.bottom - rc.top );

		if (m_TextureImageList == nullptr ) 
        {
			m_TextureImageList  = ImageList_Create( width, height, ILC_COLOR24, 2, 1 );
			// Set the Texture maps.
			SmartHBITMAP tex_bmp ( RenderBitmap( nullptr, 0, width, height ) );
			SmartHBITMAP mask_bmp( RenderBitmap( nullptr, 0, width, height, BMM_GRAY_8 ) );
			ImageList_Add        ( m_TextureImageList, tex_bmp,  nullptr );
			ImageList_Add        ( m_TextureImageList, mask_bmp, nullptr );
		}

		tex_button->SetDADMgr( dragManager );
		msk_button->SetDADMgr( dragManager );

		tex_button->SetImage ( m_TextureImageList, 0, 0, 0, 0, width, height );
		msk_button->SetImage ( m_TextureImageList, 1, 1, 1, 1, width, height );

		tex_button->SetTooltip( TRUE, GetString( IDS_DS_COMP_TOOLTIP_TEXTURE ) );
		msk_button->SetTooltip( TRUE, GetString( IDS_DS_COMP_TOOLTIP_MASK    ) );
	}

	// Initialize ALL the buttons.
	void LayerDialog::initToolbar() {
		using namespace ToolBar;

		ButtonPtr  DeleteBtn       ( m_hWnd, IDC_COMP_LAYER_REMOVE    );
		ButtonPtr  VisibleBtn      ( m_hWnd, IDC_COMP_VISIBLE         );
		ButtonPtr  ColorCorrBtn    ( m_hWnd, IDC_COMP_CC              );
		ButtonPtr  MaskVisibleBtn  ( m_hWnd, IDC_COMP_MASK_VISIBLE    );
		ButtonPtr  MaskColorCorrBtn( m_hWnd, IDC_COMP_MASK_CC         );
		ButtonPtr  RenameBtn       ( m_hWnd, IDC_COMP_LAYER_RENAME    );
		ButtonPtr  DuplicateBtn    ( m_hWnd, IDC_COMP_LAYER_DUPLICATE );

		DeleteBtn      ->SetImage( m_ButtonImageList, RemoveIcon,    RemoveIcon,
			RemoveIconDis, RemoveIconDis,
			IconHeight,    IconWidth );
		VisibleBtn     ->SetImage( m_ButtonImageList, VisibleIconOn, VisibleIconOn,
			VisibleIconOn, VisibleIconOn,
			IconHeight,    IconWidth );
		ColorCorrBtn   ->SetImage( m_ButtonImageList, ColorCorrectionIcon, ColorCorrectionIcon,
			ColorCorrectionIcon, ColorCorrectionIcon,
			IconHeight,    IconWidth );
		MaskVisibleBtn ->SetImage( m_ButtonImageList, VisibleIconOn, VisibleIconOn,
			VisibleIconOn, VisibleIconOn,
			IconHeight,    IconWidth );
		MaskColorCorrBtn->SetImage(m_ButtonImageList, ColorCorrectionIcon, ColorCorrectionIcon,
			ColorCorrectionIcon, ColorCorrectionIcon,
			IconHeight,    IconWidth );
		RenameBtn      ->SetImage( m_ButtonImageList, RenameIcon,    RenameIcon,
			RenameIcon,    RenameIcon,
			IconHeight,    IconWidth );
		DuplicateBtn   ->SetImage( m_ButtonImageList, DuplicateIcon, DuplicateIcon,
			DuplicateIcon, DuplicateIcon,
			IconHeight,    IconWidth );

		DeleteBtn->Execute         ( I_EXEC_CB_NO_BORDER );
		VisibleBtn->Execute        ( I_EXEC_CB_NO_BORDER );
		ColorCorrBtn->Execute      ( I_EXEC_CB_NO_BORDER );
		MaskVisibleBtn->Execute    ( I_EXEC_CB_NO_BORDER );
		MaskColorCorrBtn->Execute  ( I_EXEC_CB_NO_BORDER );
		RenameBtn->Execute         ( I_EXEC_CB_NO_BORDER );
		DuplicateBtn->Execute      ( I_EXEC_CB_NO_BORDER );

		DeleteBtn->Execute         ( I_EXEC_BUTTON_DAD_ENABLE, 0 );
		VisibleBtn->Execute        ( I_EXEC_BUTTON_DAD_ENABLE, 0 );
		ColorCorrBtn->Execute      ( I_EXEC_BUTTON_DAD_ENABLE, 0 );
		MaskVisibleBtn->Execute    ( I_EXEC_BUTTON_DAD_ENABLE, 0 );
		MaskColorCorrBtn->Execute  ( I_EXEC_BUTTON_DAD_ENABLE, 0 );
		RenameBtn->Execute         ( I_EXEC_BUTTON_DAD_ENABLE, 0 );
		DuplicateBtn->Execute      ( I_EXEC_BUTTON_DAD_ENABLE, 0 );

		DeleteBtn->SetTooltip         ( TRUE, GetString( IDS_DS_COMP_TOOLTIP_DELETE ) );
		RenameBtn->SetTooltip         ( TRUE, GetString( IDS_DS_COMP_TOOLTIP_RENAME ) );
		DuplicateBtn->SetTooltip      ( TRUE, GetString( IDS_DS_COMP_TOOLTIP_DUPLICATE ) );
		ColorCorrBtn->SetTooltip      ( TRUE, GetString( IDS_DS_COMP_TOOLTIP_COLOR ) );
		MaskColorCorrBtn->SetTooltip  ( TRUE, GetString( IDS_DS_COMP_TOOLTIP_MASK_COLOR ) );
	}


	// Update the texture button (render the texmap and reshow it)
	void LayerDialog::UpdateTexture( ) const {
		ButtonPtr  tex_button( m_hWnd, IDC_COMP_TEX );
		if ( false == tex_button )
			return;

		// No Update when no Texture
		if (m_Layer && !m_Layer->Texture()) {
			tex_button->SetIcon( 0, 0, 0 );
			tex_button->SetText( GetString( IDS_DS_NO_TEXTURE ) );
			// button tool-tip is updated automatically when SetText is called (TexDADMgr::AutoTooltip returns TRUE),
			// We want to force the tooltip to something different.
			tex_button->SetTooltip( TRUE, GetString( IDS_DS_COMP_TOOLTIP_TEXTURE ) );
		} else {
			// Create the image.
			HWND button( tex_button->GetHwnd() );
			RECT rc; ::GetWindowRect( button, &rc );
			int width  = rc.right - rc.left;
			int height = rc.bottom - rc.top;

			SmartHBITMAP tex_bmp  ( RenderBitmap( m_Layer->Texture(),
				m_Layer->CurrentTime(),
				width, height ) );
			ImageList_Replace     ( m_TextureImageList, 0, tex_bmp, nullptr );
			tex_button->SetImage  ( m_TextureImageList, 0, 0, 0, 0, width, height );
			// Add the name of Texture to the tooltip.
			_tstring ttip( GetString( IDS_DS_COMP_TOOLTIP_TEXTURE ) );
			ttip += _T(": ");
			ttip += m_Layer->Texture()->GetName();
			tex_button->SetTooltip( TRUE, const_cast<LPTSTR>(ttip.c_str()) );
		}
	}

	// Update the mask button (render the texmap and reshow it).
	void LayerDialog::UpdateMask( ) const {
		ButtonPtr mask_button( m_hWnd, IDC_COMP_MASK );
		if ( false == mask_button )
			return;

		// No Update when no Texture
		if (m_Layer && !m_Layer->Mask()) {
			mask_button->SetIcon( 0, 0, 0 );
			mask_button->SetText( GetString( IDS_DS_NO_MASK ) );
			// button tool-tip is updated automatically when SetText is called (TexDADMgr::AutoTooltip returns TRUE),
			// We want to force the tooltip to something different.
			mask_button->SetTooltip( TRUE, GetString( IDS_DS_COMP_TOOLTIP_MASK ) );
		} else {
			// Create the image.
			HWND button( mask_button->GetHwnd() );
			RECT rc; ::GetWindowRect( button, &rc );
			int width  = rc.right - rc.left;
			int height = rc.bottom - rc.top;

			SmartHBITMAP mask_bmp( RenderBitmap( m_Layer->Mask(),
				m_Layer->CurrentTime(),
				width, height,
				BMM_GRAY_8 ) );
			ImageList_Replace    ( m_TextureImageList, 1, mask_bmp, nullptr );
			mask_button->SetImage( m_TextureImageList, 1, 1, 1, 1, width, height );
			// Add the name of Texture to the tooltip.
			_tstring ttip( GetString( IDS_DS_COMP_TOOLTIP_MASK ) );
			ttip += _T(": ");
			ttip += m_Layer->Mask()->GetName();
			mask_button->SetTooltip( TRUE, const_cast<LPTSTR>(ttip.c_str()) );
		}
	}

	// Update the opacity edit and spinner.
	void LayerDialog::UpdateOpacity( bool edit ) const {
		// Set the edit boxes
		EditPtr Opacity(m_RollupPanel->GetHWnd(), IDC_COMP_OPACITY     );
		SpinPtr spin   (m_RollupPanel->GetHWnd(), IDC_COMP_OPACITY_SPIN);
		if (m_Layer && Opacity && spin) {
			if (edit) {
				Opacity->SetText ( m_Layer->Opacity() );
			}

			spin   ->SetValue( m_Layer->Opacity(), FALSE );
			// Set the red border if spin is on key frame.
			spin   ->SetKeyBrackets( Layer()->IsOpacityKeyFrame() );
		}
	}

	// Update the texture visibility button.
	void LayerDialog::UpdateVisibility( ) const {
		using namespace ToolBar;
		ButtonPtr VisibleBtn( m_RollupPanel->GetHWnd(), IDC_COMP_VISIBLE );
		if (m_Layer && VisibleBtn) {
			if (m_Layer->Visible()) {
				VisibleBtn->SetImage( m_ButtonImageList,  VisibleIconOn,  VisibleIconOn,
					VisibleIconOn,  VisibleIconOn,
					IconWidth, IconHeight );
				VisibleBtn->SetTooltip( TRUE, GetString( IDS_DS_COMP_TOOLTIP_VISIBLE ) );
			} else {
				VisibleBtn->SetImage( m_ButtonImageList,  VisibleIconOff, VisibleIconOff,
					VisibleIconOff, VisibleIconOff,
					IconWidth, IconHeight );
				VisibleBtn->SetTooltip( TRUE, GetString( IDS_DS_COMP_TOOLTIP_HIDDEN ) );
			}
		}
	}

	// Update the mask visibility button.
	void LayerDialog::UpdateMaskVisibility( ) const {
		using namespace ToolBar;
		ButtonPtr MaskVisibleBtn( m_RollupPanel->GetHWnd(), IDC_COMP_MASK_VISIBLE );
		if (m_Layer && m_Layer->VisibleMask()) {
			MaskVisibleBtn->SetImage( m_ButtonImageList,  VisibleIconOn,  VisibleIconOn,
				VisibleIconOn,  VisibleIconOn,
				IconWidth, IconHeight );
			MaskVisibleBtn->SetTooltip( TRUE, GetString( IDS_DS_COMP_TOOLTIP_MASK_VISIBLE ) );
		} else {
			MaskVisibleBtn->SetImage( m_ButtonImageList, VisibleIconOff, VisibleIconOff,
				VisibleIconOff, VisibleIconOff,
				IconWidth, IconHeight );
			MaskVisibleBtn->SetTooltip( TRUE, GetString( IDS_DS_COMP_TOOLTIP_MASK_HIDDEN ) );
		}
	}

	// Updates all the fields in the dialog.
	void LayerDialog::UpdateDialog()  const {
		if (m_Layer == NULL)
			return;

		// Set the visibility buttonz
		UpdateVisibility();
		UpdateMaskVisibility();

		HWND combo( ::GetDlgItem( m_RollupPanel->GetHWnd(), IDC_COMP_BLEND_TYPE ) );
		::SendMessage( combo, CB_SETCURSEL, m_Layer->Blend(), 0 );

		UpdateOpacity( );
		if (m_Layer) {
			UpdateTexture( );
			UpdateMask   ( );
		}
	}
#pragma endregion

#pragma endregion
	/////////////////////////////////////////////////////////////////////////////
}

#pragma endregion

////////////////////////////////////////////////////////////////////////////////

#pragma region Parameter Block
namespace Composite {
	namespace {
		class AccessorClass : public PBAccessor
		{
		public:
			void TabChanged( tab_changes     changeCode,
				Tab<PB2Value>  *tab,
				ReferenceMaker *owner,
				ParamID         id,
				int             tabIndex,
				int             count );
		};

		AccessorClass Accessor;
	}
}

// Parameter block (module private)
namespace Composite {
	namespace {
		ParamBlockDesc2 ParamBlock (
				BlockParam::ID, _T("parameters"),              // ID, int_name
			                    0,                             // local_name
			                    GetCompositeDesc(),            // ClassDesc
			                    P_AUTO_CONSTRUCT | P_VERSION,  // flags
			                    3,                             // version
			                    0,                             // construct_id

      //////////////////////////////////////////////////////////////////////////
      // params
      // For compatibility while loading with the old Composite map, the two
      // first parameters have the same Name and are in the same order as before.
      BlockParam::Texture_DEPRECATED,    _T(""),          // ID, int_name
         TYPE_TEXMAP_TAB,     0,                      // Type, tab_size
         P_VARIABLE_SIZE | P_OWNERS_REF | P_OBSOLETE,              // flags
         IDS_DS_TEXMAPS,                              // localized Name
         p_accessor,         &Accessor,               // params
         p_refno,             1,
      p_end,
      BlockParam::Visible,    _T("mapEnabled"),
         TYPE_BOOL_TAB,       1,
         P_VARIABLE_SIZE | P_ANIMATABLE,
         IDS_JW_MAP1ENABLE,
         p_default,           Default::Visible,
         p_accessor,         &Accessor,
      p_end,
      BlockParam::Mask_DEPRECATED,       _T(""),
         TYPE_TEXMAP_TAB,     0,
         P_VARIABLE_SIZE | P_OWNERS_REF  | P_OBSOLETE,
         IDS_JW_MASKMAP,
         p_accessor,         &Accessor,
         p_refno,             1,
      p_end,
      BlockParam::VisibleMask,_T("maskEnabled"),
         TYPE_BOOL_TAB,       1,
         P_VARIABLE_SIZE | P_ANIMATABLE,
         IDS_JW_MAP2ENABLE,
         p_default,           Default::VisibleMask,
         p_accessor,         &Accessor,
      p_end,
      BlockParam::Opacity_DEPRECATED,    _T(""),
         TYPE_INT_TAB,        0,
         P_VARIABLE_SIZE | P_ANIMATABLE | P_OBSOLETE,
         0,
      p_end,
      BlockParam::BlendMode,  _T("blendMode"),
         TYPE_INT_TAB,        1,
         P_VARIABLE_SIZE,
         IDS_JW_MAP_BLENDMODE,
         p_default,           Default::BlendMode,
         p_range,             0, Blending::ModeList.size(),
         p_accessor,          &Accessor,
      p_end,
      BlockParam::Name,       _T("layerName"),
         TYPE_STRING_TAB,     1,
         P_VARIABLE_SIZE,
         IDS_JW_NAME,
		 p_default,          Default::Name.c_str(),
        p_accessor,          &Accessor,
      p_end,
      BlockParam::DialogOpened, _T("dlgOpened"),
         TYPE_BOOL_TAB,       1,
         P_VARIABLE_SIZE,
         IDS_JW_DIALOG_OPENED,
         p_default,           Default::DialogOpened,
         p_accessor,          &Accessor,
      p_end,
	  BlockParam::Opacity,    _T("opacity"),
         TYPE_FLOAT_TAB,        1,
         P_VARIABLE_SIZE | P_ANIMATABLE,
         IDS_JW_MAPOPACITY,
         p_default,           Default::Opacity,
         p_range,             Default::OpacityMin,
							  Default::OpacityMax,
         p_accessor,          &Accessor,
	  p_end,

	  BlockParam::Texture,    _T("mapList"),          // ID, int_name
         TYPE_TEXMAP_TAB,     1,                      // Type, tab_size
         P_VARIABLE_SIZE | P_SUBANIM,              // flags
         IDS_DS_TEXMAPS,                              // localized Name
         p_accessor,         &Accessor,               // params
	  p_end,
	  BlockParam::Mask,       _T("mask"),
         TYPE_TEXMAP_TAB,     1,
         P_VARIABLE_SIZE | P_SUBANIM ,
         IDS_JW_MASKMAP,
         p_accessor,         &Accessor,
	  p_end,

   p_end );
}
}
#pragma endregion

////////////////////////////////////////////////////////////////////////////////

#pragma region Function Publishing

namespace Composite {

   FPInterfaceDesc InterfaceDesc(
      COMPOSITE_INTERFACE,             _T("layers"),        // ID, int_name
                                       0,                   // local_name
                                       GetCompositeDesc(),  // ClassDesc
                                       FP_MIXIN,            // flags

		Function::CountLayers, _T("count"), 0, TYPE_INT, 0, 0, // ID, int_name, local_name, return type, flags, num args.
		Function::AddLayer, _T("add"), 0, TYPE_VOID, 0, 0, 
		Function::DeleteLayer, _T("delete"), 0, TYPE_VOID, 0, 1, 
			_T("index"), 0, TYPE_INDEX,
		Function::DuplicateLayer, _T("duplicate"), 0, TYPE_VOID, 0, 1, 
			_T("index"), 0, TYPE_INDEX,
		Function::MoveLayer, _T("move"), 0, TYPE_VOID, 0, 3, 
			_T("from index"), 0, TYPE_INDEX,
			_T("to index"),   0, TYPE_INDEX,
			_T("before"),     0, TYPE_BOOL, f_keyArgDefault, false,
      p_end
   );

   FPInterfaceDesc* Interface::GetDesc() {
      return &InterfaceDesc;
   }
}

#pragma endregion

////////////////////////////////////////////////////////////////////////////////


#pragma region Restore Objects

namespace Composite {
	class PopLayerBeforeRestoreObject: public RestoreObj {
		Texture			*m_owner;
		IParamBlock2	*m_paramblock;
		size_t			m_layer_index;
		PopLayerBeforeRestoreObject() { }
	public:
		PopLayerBeforeRestoreObject(Texture *owner, IParamBlock2 *paramblock, size_t layer_index);
		~PopLayerBeforeRestoreObject() { }
		void Restore(int isUndo);
		void Redo();
		TSTR Description() {
			return(_T("CompositeTexmapPopLayerBeforeRestore"));
		}
	};

	class PopLayerAfterRestoreObject: public RestoreObj {
		Texture			*m_owner;
		IParamBlock2	*m_paramblock;
		size_t			m_layer_index;
		PopLayerAfterRestoreObject() { }
	public:
		PopLayerAfterRestoreObject(Texture *owner, IParamBlock2 *paramblock, size_t layer_index);
		~PopLayerAfterRestoreObject() { }
		void Restore(int isUndo);
		void Redo();
		TSTR Description() {
			return(_T("CompositeTexmapPopLayerAfterRestore"));
		}
	};

	class PushLayerRestoreObject: public RestoreObj {
		Texture			*m_owner;
		IParamBlock2	*m_paramblock;
		bool			m_setName;
		PushLayerRestoreObject() { }
	public:
		PushLayerRestoreObject(Texture *owner, IParamBlock2 *paramblock, bool setName);
		~PushLayerRestoreObject(){}
		void Restore(int isUndo);
		void Redo();
		TSTR Description() {
			return(_T("CompositeTexmapPushLayerRestore"));
		}
	};

	class InsertLayerBeforeRestoreObject: public RestoreObj {
		Texture			*m_owner;
		IParamBlock2	*m_paramblock;
		size_t			m_layer_index;
		InsertLayerBeforeRestoreObject() { }
	public:
		InsertLayerBeforeRestoreObject(Texture *owner, IParamBlock2 *paramblock, size_t layer_index);
		~InsertLayerBeforeRestoreObject() { }
		void Restore(int isUndo);
		void Redo();
		TSTR Description() {
			return(_T("CompositeTexmapInsertLayerBeforeRestore"));
		}
	};

	class InsertLayerAfterRestoreObject: public RestoreObj {
		Texture			*m_owner;
		IParamBlock2	*m_paramblock;
		size_t			m_layer_index;
		InsertLayerAfterRestoreObject() { }
	public:
		InsertLayerAfterRestoreObject(Texture *owner, IParamBlock2 *paramblock, size_t layer_index);
		~InsertLayerAfterRestoreObject() { }
		void Restore(int isUndo);
		void Redo();
		TSTR Description() {
			return(_T("CompositeTexmapInsertLayerAfterRestore"));
		}
	};

}

#pragma endregion

using namespace MaxSDK::Graphics;

#pragma region Composite Texture Implementation

namespace Composite {

	_tstring Texture::DefaultLayerName( ) const 
	{ 
		return m_DefaultLayerName; 
	}
	
	TSTR Texture::SubAnimName(int i) 
	{
		return GetString(IDS_DS_PARAMETERS);
	}

	BaseInterface* Texture::GetInterface(Interface_ID id) 
	{
		if (id == ITEXTURE_DISPLAY_INTERFACE_ID)
		{
			return static_cast<ITextureDisplay*>(this);
		}
		else if (id == COMPOSITE_INTERFACE)
		{
			return (Interface*)this;
		}
		else
		{
			return MultiTex::GetInterface(id);
		}
	}

	void Texture::MXS_AddLayer() 
	{
		if (nb_layer() >= Param::LayerMax)
			throw MAXException(GetString( IDS_ERROR_MAXLAYER ));
		AddLayer();
	}

	void Texture::MXS_DeleteLayer( int index ) 
	{
		if (index < 0 || index >= nb_layer())
			throw MAXException(GetString( IDS_ERROR_LAYER_INDEX ));
		if (nb_layer() == 1)
			throw MAXException(GetString( IDS_ERROR_MINLAYER ));

		DeleteLayer( size_t( index ) );
	}

	void Texture::MXS_DuplicateLayer( int index ) 
	{
		if (nb_layer() >= Param::LayerMax)
			throw MAXException(GetString( IDS_ERROR_MAXLAYER ));
		if (index < 0 || index >= nb_layer())
			throw MAXException(GetString( IDS_ERROR_LAYER_INDEX ));
		DuplicateLayer( size_t( index ) );
	}

	void Texture::MXS_MoveLayer ( int from_index, int to_index, bool before ) 
	{
		if (from_index < 0 || from_index > nb_layer())
			throw MAXException(GetString( IDS_ERROR_LAYER_INDEX ));
		if (to_index < 0 || to_index >= nb_layer())
			throw MAXException(GetString( IDS_ERROR_LAYER_INDEX ));
		MoveLayer( size_t( from_index ), size_t( to_index ), !before );
	}

	size_t Texture::CountVisibleLayer() const throw() 
	{
		struct {
			bool operator()( const value_type& l ) const {
				return l.IsVisible(); }
		} const predicate;
		return std::count_if( begin(), end(), predicate );
	}

	size_t Texture::CountMaskedLayer() const throw() 
	{
		struct {
			bool operator()( const value_type& l ) const {
				return l.IsMaskVisible(); }
		} const predicate;
		return std::count_if( begin(), end(), predicate );
	}
}

#pragma endregion

////////////////////////////////////////////////////////////////////////////////

#pragma region Class Descriptors

void*          Composite::Desc::Create(BOOL loading)   { return new Composite::Texture(loading); }
int            Composite::Desc::IsPublic()     { return TRUE; }
SClass_ID      Composite::Desc::SuperClassID() { return TEXMAP_CLASS_ID; }
Class_ID       Composite::Desc::ClassID()      { return Class_ID( COMPOSITE_CLASS_ID, 0 ); }
HINSTANCE      Composite::Desc::HInstance()    { return hInstance; }
const TCHAR  * Composite::Desc::ClassName()    { return GetString( IDS_RB_COMPOSITE_CDESC ); }
const TCHAR  * Composite::Desc::Category()     { return TEXMAP_CAT_COMP; }
const TCHAR  * Composite::Desc::InternalName() { return _T("CompositeMap"); }

ClassDesc2* GetCompositeDesc() {
	static Composite::Desc ClassDescInstance;
	return &ClassDescInstance;
}

#pragma endregion

////////////////////////////////////////////////////////////////////////////////

#pragma region Accessors

namespace Composite {

	void AccessorClass::TabChanged( tab_changes     changeCode,
		Tab<PB2Value>  *tab,
		ReferenceMaker *owner,
		ParamID         id,
		int             tabIndex,
		int             count ) {
			if ( (owner != NULL) &&
				 (id != BlockParam::Texture_DEPRECATED) &&
				 (id != BlockParam::Mask_DEPRECATED) &&
				 (id != BlockParam::Opacity_DEPRECATED) )
				static_cast<Texture*>(owner)->OnParamBlockChanged( id );
	}
}

#pragma endregion

////////////////////////////////////////////////////////////////////////////////

#pragma region Composite Dialog Method Implementation

INT_PTR Composite::Dialog::PanelDlgProc(HWND hWnd, UINT msg,
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

int Composite::Dialog::FindSubTexFromHWND(HWND handle) {
	struct {
		HWND m_hWnd;
		bool operator()( LayerDialog& d ) {
			return d.IsTextureButton( m_hWnd ) || d.IsMaskButton( m_hWnd ); }
	} predicate = {handle};

	iterator it(find_if( begin(), end(), predicate ));
	if (it == end())
		return int(-1);

	if (it->IsTextureButton( handle ))
		return int(it->Layer()->Index() * 2 );
	else
		return int(it->Layer()->Index() * 2 + 1);
}

void Composite::Dialog::OnOpacitySpinChanged( HWND handle, iterator LayerDialog ) {
	SpinPtr spin( handle, IDC_COMP_OPACITY_SPIN );
	LayerDialog->Layer()->Opacity( spin->GetFVal() );
	LayerDialog->UpdateOpacity( );
	m_Texture->NotifyChanged();
}

// When the opacity edit box change, update the layer if it was a return.
void Composite::Dialog::OnOpacityEditChanged( HWND handle ) {
	EditPtr edit( handle, IDC_COMP_OPACITY );
	if (edit->GotReturn()) {
		TSTR newOpacityVal;
		if ( m_oldOpacityVal == newOpacityVal )
			return;
		OnOpacityFocusLost( handle );
		edit->GetText(m_oldOpacityVal);
	}
}

// Update the opacity when edit lost focus
void Composite::Dialog::OnOpacityFocusLost( HWND handle ) {
	EditPtr edit( handle, IDC_COMP_OPACITY );
	iterator it ( find( handle ) );

	DbgAssert( it != end() );
	if (it == end())
		return;

	BOOL valid  ( TRUE );
	float  Opacity( edit->GetFloat( &valid ) );
	// Update the opacity if it's valid.
	if (valid)
		it->Layer()->Opacity( Opacity );

	it->UpdateOpacity( );
	m_Texture->NotifyChanged();
}

void Composite::Dialog::StopChange( int resource_id ) 
{ 
	theHold.Accept( ::GetString( resource_id ) ); 
}

namespace {
	// Register a drag'n'drop callback to call the MoveLayer function when dragging.
	struct RollupCallback : public IRollupCallback {
		RollupCallback( Composite::Dialog& c ) : m_Dialog(c) {}
		// On drop, if we find both destination and source in our layers, we
		// move the layer
		BOOL HandleDrop( IRollupPanel *src, IRollupPanel *dst, bool before ) {
			Composite::Dialog::iterator src_it( m_Dialog.find( src->GetHWnd() ) );
			Composite::Dialog::iterator dst_it( m_Dialog.find( dst->GetHWnd() ) );

			if( src_it != m_Dialog.end() && dst_it != m_Dialog.end() && src_it != dst_it) {
				size_t s( src_it->Layer()->Index() );
				size_t d( dst_it->Layer()->Index() );
				theHold.Begin();
				m_Dialog.GetTexture()->MoveLayer( s, d, before );
				theHold.Accept( GetString(IDS_DS_UNDO_MOVE) );
				m_Dialog.updateLayersCat();
				m_Dialog.ReloadDialog();
				GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
			}
			// Never let the system handle this, since we need to completely refresh
			// the interface and we re-hooked a lot of things...
			return TRUE;
		}

	private:
		Composite::Dialog& m_Dialog;
	};
}

Composite::Dialog::Dialog(HWND editor_wnd, IMtlParams *mtl_param, Texture *comp)
							: m_MtlParams        ( mtl_param       )
							, m_Texture          ( comp            )
							, m_Redraw           ( false           )
							, m_EditorWnd        ( editor_wnd      )
							, m_CategoryCounter  ( Param::CategoryCounterMax )
							, m_Reloading        ( false           )
							, m_ButtonImageList  ( ImageList_Create( 13, 12, ILC_MASK, 0, 1 ) )
{
	m_DragManager.Init( this );
	m_DialogWindow = m_MtlParams->AddRollupPage( hInstance, MAKEINTRESOURCE(IDD_COMPOSITEMAP),
		PanelDlgProc,
		GetString( IDS_COMPOSITE_LAYERS ),
		(LPARAM)this );

	// We cannot directly construct m_LayerRollupWnd since we need to call
	// AddRollupPage before.
	// Note that the corresponding ReleaseIRollup is called when m_LayerRollupWnd is
	// destroyed.
	m_LayerRollupWnd = ::GetIRollup( ::GetDlgItem(m_DialogWindow, IDC_COMP_LAYER_LIST) );

	// Create a new managed callback and register it.
	m_RollupCallback = new RollupCallback( *this );
	m_LayerRollupWnd->RegisterRollupCallback( m_RollupCallback ); //< This is unregistered in the destructor

	// Create the button image list
	HBITMAP hBitmap, hMask;
	hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_COMP_BUTTONS));
	hMask   = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_COMP_MASKBUTTONS));
	ImageList_Add( m_ButtonImageList, hBitmap, hMask );
	DeleteObject(hBitmap);
	DeleteObject(hMask);

	// Create rollups and dialog classes for all layers already available
	for( Texture::iterator it  = m_Texture->begin(); it != m_Texture->end(); ++it )
		CreateRollup( &*it );

	// Disable the number of layers
	EditPtr( m_DialogWindow, IDC_COMP_TOTAL_LAYERS )->Disable();

	// Set the Add layer button.
	using namespace ToolBar;
	ButtonPtr add_button( m_DialogWindow, IDC_COMP_ADD );
	EditPtr   add_edit  ( m_DialogWindow, IDC_COMP_TOTAL_LAYERS );
	add_button->Execute( I_EXEC_CB_NO_BORDER );
	add_button->Execute( I_EXEC_BUTTON_DAD_ENABLE, 0 );
	add_button->SetTooltip( TRUE, GetString( IDS_DS_COMP_TOOLTIP_ADD_LAYER ) );
	add_edit  ->SetTooltip( TRUE, GetString( IDS_DS_COMP_TOOLTIP_ADD_LAYER ) );

	add_button->SetImage( m_ButtonImageList,	AddIcon,    AddIcon,
												AddIconDis, AddIconDis,
												IconWidth,  IconHeight );

	// Validate or change the control state
	EnableDisableInterface();
}

// Create a rollout.
void Composite::Dialog::CreateRollup( Layer* layer ) {
	// Does NOT add a rollup if one is already present.
	if (find(layer) != end())
		return;

	// see if we are inserting layer. This happens on undo/redo.
	int category = -1;
	bool insertLayer = false;
	iterator it = begin();
	if (layer != NULL && it != end() && begin()->Layer()->Index() > layer->Index()) {
		for( ; it != end(); ++it ) {
			if (it->Layer()->Index() > layer->Index()) {
				category = it->Panel()->GetCategory()+1;
				insertLayer = true;
			}
		}
	}

	if (!insertLayer)
		category = --m_CategoryCounter;

	if (category < 0)
		category = INT_MAX;

	// Add the rollup to the window
	int i = m_LayerRollupWnd->AppendRollup( hInstance,
		MAKEINTRESOURCE( IDD_COMPOSITEMAP_LAYER ),
		PanelDlgProc,
		_T("Layer"),
		LPARAM( this ),
		0,
		category );
	HWND            wnd  ( m_LayerRollupWnd->GetPanelDlg(i) );
	::IRollupPanel* panel( m_LayerRollupWnd->GetPanel( wnd ) );

	// Set opening of the rollup if necessary.
	if (layer != NULL)
		m_LayerRollupWnd->SetPanelOpen( i, layer->DialogOpened() );

	// Reposition categories if we have too many used (since deleting a Layer
	// doesn't free the category).  Shouldn't happen too often
	if (m_CategoryCounter < 1) {
		m_CategoryCounter = Param::CategoryCounterMax;
		// Re-categorize the layers.  Should not happen often
		for( iterator it  = begin(); it != end(); ++it, --m_CategoryCounter )
			it->Panel()->SetCategory( m_CategoryCounter );
	}

	m_LayerRollupWnd->Show( i );
	m_LayerRollupWnd->UpdateLayout();

	// Create and push the new Layer on the list
	if (insertLayer)
		insert(it, LayerDialog( layer, panel, wnd, &m_DragManager, m_ButtonImageList ) );
	else
		push_front( LayerDialog( layer, panel, wnd, &m_DragManager, m_ButtonImageList ) );
	Invalidate();
}

// Delete the first layer or the specified one.
void Composite::Dialog::DeleteRollup( Layer* layer ) {
	iterator it( layer ? find(layer) : begin() );

	if (it == end())
		return;
	
	HWND wnd  ( it->Panel()->GetHWnd() );
	int  index( m_LayerRollupWnd->GetPanelIndex( wnd ) );
	m_LayerRollupWnd->DeleteRollup(index, 1);
	erase( it );
}

// Hide the first layer or the specified one.
void Composite::Dialog::HideRollup(Layer* layer) {
	iterator it(layer ? find(layer) : begin());

	if (it == end())
		return;

	HWND wnd(it->Panel()->GetHWnd());
	int  index(m_LayerRollupWnd->GetPanelIndex(wnd));
	m_LayerRollupWnd->Hide(index);
	erase(it);
}

// Show the rename dialog to the user and change the layer's name.
void Composite::Dialog::RenameLayer( LayerDialog* layerDialog ) {
	// This is the dialog's handler class.
	// Basically it encapsulates the WinProc and call back Texture's members
	// to set or reset the layer name.
	struct rename_dialog {
		Layer     *m_Layer;
		Texture   *m_Texture;
		HWND       m_hWnd;

		static INT_PTR CALLBACK proc( HWND h, UINT msg, WPARAM w, LPARAM l ) {
			rename_dialog* Dialog(DLGetWindowLongPtr< rename_dialog* >( h ));

			switch( msg ) {
			// On init, set the user data to the rename_dialog class and
			// set the text inside, then select it all.
			case WM_INITDIALOG:
				DLSetWindowLongPtr( h, l, GWLP_USERDATA );
				Dialog = DLGetWindowLongPtr< rename_dialog* >( h );
				Dialog->m_hWnd = h;
				SetWindowText( GetDlgItem( Dialog->m_hWnd, IDC_LAYERNAME ),
					Dialog->m_Layer->Name() );
				SetFocus( GetDlgItem( Dialog->m_hWnd, IDC_LAYERNAME ) );
				SendMessage(GetDlgItem( Dialog->m_hWnd, IDC_LAYERNAME ),
					EM_SETSEL,
					0, MAKELONG(0, _tcslen(Dialog->m_Layer->Name())) );
				return FALSE;
				break;

			case WM_COMMAND:
				switch (LOWORD(w)) {
					// On OK, set the name to the editbox text.
				case IDOK: {
						TCHAR buff[ 1024 ];
						GetWindowText( GetDlgItem( Dialog->m_hWnd, IDC_LAYERNAME ),
							buff, _countof( buff ));
						Dialog->m_Layer->Name( buff );
						EndDialog( Dialog->m_hWnd, 0 );
						break;
					}
				}
				break;

			case WM_CLOSE:
				EndDialog( Dialog->m_hWnd, 0 );
				break;

			default:
				return FALSE;
			}
			return TRUE;
		}
	} proc = { layerDialog->Layer(), m_Texture, 0 };

	// Call the dialog.
	DialogBoxParam( hInstance, MAKEINTRESOURCE(IDD_COMPOSITEMAP_RENAME),
		m_DialogWindow,
		rename_dialog::proc,
		LPARAM(&proc) );

	// Update the Name of this Layer...
	HWND wnd  ( layerDialog->Panel()->GetHWnd() );
	int  index( m_LayerRollupWnd->GetPanelIndex( wnd ) );
	m_LayerRollupWnd->SetPanelTitle( index, const_cast<LPTSTR>(layerDialog->Title().c_str()) );
}

void Composite::Dialog::Rewire( ) {
	DbgAssert( size() == m_Texture->size() );
	if (size() != m_Texture->size())
		return;

	ValueGuard<bool>   reloading_guard( m_Reloading, true );
	reverse_iterator   dlg_it( rbegin() );
	Texture::iterator  lay_it( m_Texture->begin() );

	m_CategoryCounter = Param::CategoryCounterMax;

	for( ; dlg_it != rend(); ++dlg_it, ++lay_it ) {
		dlg_it->Layer( &*lay_it );
		dlg_it->Panel()->SetCategory( --m_CategoryCounter );
	}

	m_LayerRollupWnd->UpdateLayout();
	m_Texture->NotifyChanged();
}

void Composite::Dialog::updateLayersCat()
{
	// Update sub-dialogs
	for (iterator it = begin(); it != end(); ++it) {
		Layer* layer(it->Layer());
		if (layer == NULL)
			continue;

		int Index(m_LayerRollupWnd->GetPanelIndex(it->Panel()->GetHWnd()));
		it->Panel()->SetCategory(Index);
	}
	m_LayerRollupWnd->UpdateLayout();
}

void Composite::Dialog::updateLayersName()
{
	// Update sub-dialogs (in reverse order so that the Name is correct)
	for (iterator it = begin(); it != end(); ++it) {
		Layer* layer(it->Layer());
		if (layer == NULL)
			continue;

		int Index(m_LayerRollupWnd->GetPanelIndex(it->Panel()->GetHWnd()));
		m_LayerRollupWnd->SetPanelTitle(Index, const_cast<LPTSTR>(it->Title().c_str()));

		it->UpdateDialog();
	}
}

void Composite::Dialog::openAllRollups()
{
	for (iterator it = begin(); it != end(); ++it) {
		Layer* layer(it->Layer());
		if (layer == NULL)
			continue;

		int Index(m_LayerRollupWnd->GetPanelIndex(it->Panel()->GetHWnd()));
		m_LayerRollupWnd->SetPanelOpen(Index, true);
	}
}

void Composite::Dialog::ReloadDialog() {
	// Ensure m_Reloading returns to false when we go out.
	ValueGuard<bool> reloading_guard( m_Reloading, true );

	Interval valid;
	m_Texture->Update(m_MtlParams->GetTime(), valid);

	// Update number of layers
	EditPtr layer_edit( m_DialogWindow, IDC_COMP_TOTAL_LAYERS );
	DbgAssert(false == layer_edit.IsNull());

	if (layer_edit.IsNull())
		return;

	_tostringstream oss;
	oss << m_Texture->nb_layer();
	layer_edit->SetText( const_cast<LPTSTR>(oss.str().c_str()) );

	updateLayersName();
}

void Composite::Dialog::SetTime(TimeValue t) {
	//we need to check the opacity, mask, and map for any animation and if so update
	//those specific controls so we dont get excess ui redraw
	//get our map validity
//   Interval valid;
//   valid.SetInfinite();
//   m_Texture->GetParamBlock(0)->GetValidity(t,valid);

	int ct =  m_Texture->GetParamBlock(0)->Count( BlockParam::Opacity);
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
		iv.SetInfinite();
		map = it->Layer()->Mask();
		if (map) {
			iv.SetInfinite();
			iv = map->Validity(t);
			//if it is animated we need to invalidate the swatch on time change
			if (!(iv == FOREVER)) {
				it->UpdateMask();
			}
		}

		int index = (int)it->Layer()->Index();
		if ((index >= 0) && (index < ct)) {
			iv.SetInfinite();
			float opac = 0.0f;
			m_Texture->GetParamBlock(0)->GetValue(BlockParam::Opacity,t,opac,iv,index);
			if (!(iv == FOREVER)) {
				//if it is animated we need to invalidate the opacity spinnner on time change
				it->UpdateOpacity();
			}
			iv.SetInfinite();
			BOOL visible = TRUE;
			m_Texture->GetParamBlock(0)->GetValue(BlockParam::Visible,t,visible,iv,index);
			if (!(iv == FOREVER)) {
				//if it is animated we need to invalidate the button on time change
				it->UpdateVisibility();
			}
			iv.SetInfinite();
			visible = TRUE;
			m_Texture->GetParamBlock(0)->GetValue(BlockParam::VisibleMask,t,visible,iv,index);
			if (!(iv == FOREVER)) {
				//if it is animated we need to invalidate the button on time change
				it->UpdateMaskVisibility();
			}
		}
	}

	if (!m_Texture->GetValidity().InInterval(t)) {
		UpdateMtlDisplay();
	}
}

// Nothing to do here since everything is managed by SmartPtr or composited.
Composite::Dialog::~Dialog() {

	// Create a new managed callback and register it.
	DbgAssert(m_RollupCallback);
	m_LayerRollupWnd->UnRegisterRollupCallback( m_RollupCallback );

	m_Texture->ParamDialog( NULL );

	// Delete rollup from the material editor
	m_MtlParams->DeleteRollupPage( m_DialogWindow );
}

// Window Proc for all the layers and the main dialog.  This could really use a clean
// up and add a method by event, then calling them. Ideally put those in a map
// outside this function, thus reducing it to less than a couple of lines.
INT_PTR Composite::Dialog::PanelProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) {
	int      id   ( LOWORD(wParam) );
	int      code ( HIWORD(wParam) );
	bool updateViewPort = false;

	switch (msg) {
	case WM_PAINT:
		if (m_Redraw && hWnd == m_DialogWindow) {
			m_Redraw = false;
			ReloadDialog();
		}
		break;
	// When recalculating the layout of the rollups (happening when a rollup is opened/closed
	// by the user), we update the state of the rollup in the ParamBlock.
	case WM_CUSTROLLUP_RECALCLAYOUT:
		if (!m_Reloading) {
			StartChange();
			for(iterator it = begin(); it != end(); it++) {
				int i( m_LayerRollupWnd->GetPanelIndex( it->Panel()->GetHWnd() ) );
				if (it->Layer()->DialogOpened() != m_LayerRollupWnd->IsPanelOpen( i )) {
					it->Layer()->DialogOpened( m_LayerRollupWnd->IsPanelOpen( i ) );
				}
			}
			StopChange( IDS_DS_UNDO_DLG_OPENED );
		}
		break;
	case WM_COMMAND: {
		iterator it( find( hWnd ) );
		switch( id ) {
		// Texture button.
		case IDC_COMP_TEX:
			DbgAssert( it != end() );
			if (it == end())
				break;

			// max doesn't support undo of subtexture assignment. As a matter of fact it
			// flushes the undo system if doing so. Here we aren't necessarily doing an
			// assignment - if subtexture exists, we go to its ui 
//			StartChange(); 
			SendMessage(m_EditorWnd, WM_TEXMAP_BUTTON,
				(int)it->Layer()->Index()*2 ,
				LPARAM(m_Texture));
//			StopChange( IDS_DS_UNDO_TEXMAP );
			updateViewPort = true;
			break;
		// Mask Texture button.
		case IDC_COMP_MASK:
			DbgAssert( it != end() );
			if (it == end())
				break;

			// max doesn't support undo of subtexture assignment. As a matter of fact it
			// flushes the undo system if doing so. Here we aren't necessarily doing an
			// assignment - if subtexture exists, we go to its ui 
//			StartChange();
			SendMessage(m_EditorWnd, WM_TEXMAP_BUTTON,
				(int)it->Layer()->Index() * 2 + 1,
				LPARAM(m_Texture));
//			StopChange( IDS_DS_UNDO_MASK );
			updateViewPort = true;
			break;
		// Visible button in toolbar.
		case IDC_COMP_VISIBLE:
			DbgAssert( it != end() );
			if (it == end())
				break;

			// Toggle the visibility
			StartChange();
			it->Layer()->ToggleVisible( );
			it->UpdateDialog( );
			StopChange( IDS_DS_UNDO_VISIBLE );
			updateViewPort = true;
			break;
		// Mask Visible button in toolbar.
		case IDC_COMP_MASK_VISIBLE:
			DbgAssert( it != end() );
			if (it == end())
				break;

			// Toggle the visibility of the Mask
			StartChange();
			it->Layer()->ToggleVisibleMask( );
			it->UpdateDialog( );
			StopChange( IDS_DS_UNDO_MASK_VISIBLE );
			updateViewPort = true;
			break;
		// Delete button in toolbar.
		case IDC_COMP_LAYER_REMOVE:
			DbgAssert( it != end() );
			if (it == end())
				break;

			StartChange();
			m_Texture->DeleteLayer( it->Layer()->Index() );
			StopChange( IDS_DS_UNDO_REMOVE );
			DbgAssert( m_Texture->nb_layer() > 0 );
			updateViewPort = true;
			break;
		// Rename button in toolbar.
		case IDC_COMP_LAYER_RENAME:
			DbgAssert( it != end() );
			if (it == end())
				break;

			StartChange();
			RenameLayer( &*it );
			StopChange( IDS_DS_UNDO_RENAME );
			break;
		// Duplicate button in toolbar.
		case IDC_COMP_LAYER_DUPLICATE:
			DbgAssert( it != end() );
			if (it == end())
				break;

			StartChange();
			m_Texture->DuplicateLayer( it->Layer()->Index() );
			StopChange( IDS_DS_UNDO_DUPLICATE );
			updateViewPort = true;
			break;
		// Blending mode combo in toolbar.
		case IDC_COMP_BLEND_TYPE:
			DbgAssert( it != end() );
			if (it == end())
				break;

			if (code == CBN_SELCHANGE) {
				int selection( SendMessage( HWND(lParam), CB_GETCURSEL, 0, 0 ) );
				StartChange();
				it->Layer()->Blend( Layer::BlendType( selection ) );
				StopChange( IDS_DS_UNDO_BLENDMODE );
			}
			updateViewPort = true;
			break;
		// Opacity edit in toolbar.
		case IDC_COMP_OPACITY:
			if (code == EN_SETFOCUS) {
				EditPtr edit( hWnd, IDC_COMP_OPACITY );
				edit->GetText(m_oldOpacityVal);
			}
			else if( code == EN_KILLFOCUS || code == EN_CHANGE ) {
				DbgAssert( it != end() );
				if (it == end())
					break;


				StartChange();
				if (code == EN_KILLFOCUS) {
					EditPtr edit( hWnd, IDC_COMP_OPACITY );
					TSTR newOpacityVal;
					edit->GetText(newOpacityVal);
					if ( m_oldOpacityVal != newOpacityVal )
						OnOpacityFocusLost( hWnd );
				}
				else {
					OnOpacityEditChanged( hWnd );
				}
				StopChange( IDS_DS_UNDO_OPACITY );
				updateViewPort = true;
			}
			break;
		// Insert the ColorCorrection map between this and the texture
		case IDC_COMP_CC:
			DbgAssert( it != end() );
			if (it != end()) {
				// max doesn't support undo of subtexture assignment. As a matter of fact it
				// flushes the undo system if doing so. Here we aren't necessarily doing an
				// assignment - if subtexture exists, we go to its ui 
//				StartChange();
				m_Texture->ColorCorrection( it->Layer(), false );
				// Edit the new texture
				SendMessage( m_EditorWnd, WM_TEXMAP_BUTTON,
					(int)it->Layer()->Index() * 2,
					LPARAM(m_Texture) );
//				StopChange( IDS_DS_UNDO_COLOR_CORRECTION );
				updateViewPort = true;
			}
			break;
		// Insert the ColorCorrection map between this and the mask
		case IDC_COMP_MASK_CC:
			DbgAssert( it != end() );
			if (it != end()) {
//				StartChange();
				m_Texture->ColorCorrection( it->Layer(), true );
				// Edit the new texture
				SendMessage( m_EditorWnd, WM_TEXMAP_BUTTON,
					 (int)it->Layer()->Index() * 2 + 1,
					LPARAM(m_Texture) );
//				StopChange( IDS_DS_UNDO_MASK_COLOR_CORRECTION );
				updateViewPort = true;
			}
			break;

		// Generic buttons
		// Add button (add a layer)
		case IDC_COMP_ADD:
			StartChange();
			m_Texture->AddLayer();
			StopChange( IDS_DS_UNDO_ADD_LAYER );
			updateLayersName();
			updateViewPort = true;
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
	case CC_SPINNER_BUTTONUP:
		switch( id ) {
		case IDC_COMP_OPACITY_SPIN:
			StopChange( IDS_DS_UNDO_OPACITY );
			//we only update the viewport on spinner up since it slows things down alot
			//on very simple cases
			updateViewPort = true;
			break;
		}
		break;

	// If value was changed, update layer.
	case CC_SPINNER_CHANGE: {
			iterator it( find( hWnd ) );
			DbgAssert( it != end() );
			if (it == end())
				break;

			switch( id ) {
			case IDC_COMP_OPACITY_SPIN:
				OnOpacitySpinChanged( hWnd, it );
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

ReferenceTarget* Composite::Dialog::GetThing() {
	return (ReferenceTarget*)m_Texture;
}

void Composite::Dialog::DeleteThis()
{
	delete this;
}

void Composite::Dialog::SetThing(ReferenceTarget *m) {
	if (   m                   == NULL
		|| m->ClassID()        != m_Texture->ClassID()
		|| m->SuperClassID()   != m_Texture->SuperClassID())
	{
		DbgAssert( false );
		return;
	}

	// Change current Composite, then switch.
	m_Texture->ParamDialog( NULL );

	m_Texture = static_cast<Texture*>(m);
	size_t nb( m_Texture->nb_layer() );

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
	ReloadDialog();
	openAllRollups();
	Invalidate();
}

void Composite::Dialog::UpdateMtlDisplay() {
	m_MtlParams->MtlChanged();
}

void Composite::Dialog::EnableDisableInterface() {
	if (!m_Texture)
		return;

	// If we're adding a layer after the last, re-enable the remove button.
	if (m_Texture->nb_layer() == 1) {
		const struct {
			void operator()( LayerDialog& dlg ) const { dlg.DisableRemove(); }
		} disabler;
		std::for_each( begin(), end(), disabler );
	}
	else if (m_Texture->nb_layer() == 2) {
		const struct {
			void operator()( LayerDialog& dlg ) const { dlg.EnableRemove(); }
		} enabler;
		std::for_each( begin(), end(), enabler );
	}

	if (m_Texture->nb_layer() >= Param::LayerMax) {
		ButtonPtr add_button( m_DialogWindow, IDC_COMP_ADD );
		add_button->Disable();
	} else {
		ButtonPtr add_button( m_DialogWindow, IDC_COMP_ADD );
		add_button->Enable();
	}
}

Composite::Dialog::iterator Composite::Dialog::find( HWND handle )
{
	struct {
		HWND m_hWnd;
		bool operator()( LayerDialog& d ) { return d == m_hWnd; }
	} predicate = {handle};
	if (!handle)
		return end();
	else
		return find_if( begin(), end(), predicate );
}

Composite::Dialog::iterator Composite::Dialog::find( Layer* l )
{
	struct {
		Layer *m_Layer;
		bool operator()( LayerDialog& d ) { return *d.Layer() == *m_Layer; }
	} predicate = {l};
	if (!l)
		return end();
	else
		return find_if( begin(), end(), predicate );
}

Composite::Dialog::iterator Composite::Dialog::find( int index )
{
	struct {
		int m_Index;
		bool operator()( LayerDialog& d ) { return d.Layer()->Index() == m_Index; }
	} predicate = {index};
	if (index < 0)
		return end();
	else
		return find_if( begin(), end(), predicate );
}

void Composite::Dialog::Invalidate()
{
	m_Redraw = true;
}

#pragma endregion

////////////////////////////////////////////////////////////////////////////////

#pragma region Composite Texture Method Implementation

// Update the number of layers inside the ParamBlock and set the vector
// accordingly.
void Composite::Texture::UpdateParamBlock( size_t count ) {
	if (m_Updating)
		return;

	if (theHold.RestoreOrRedoing())
		return;

	if (m_ParamBlock == NULL)
		return;

	// When this guard goes out of scope, it returns Updating to its old
	// value.
	ValueGuard<bool> updating_guard( m_Updating, true );

	m_ParamBlock ->SetCount( BlockParam::Visible,        int(count) );
	macroRec->Disable();  // only record one count change
	m_ParamBlock ->SetCount( BlockParam::VisibleMask,    int(count) );
	m_ParamBlock ->SetCount( BlockParam::BlendMode,      int(count) );
	m_ParamBlock ->SetCount( BlockParam::Name,           int(count) );
	m_ParamBlock ->SetCount( BlockParam::DialogOpened,   int(count) );
	m_ParamBlock ->SetCount( BlockParam::Opacity,        int(count) );
	m_ParamBlock ->SetCount( BlockParam::Texture,        int(count) );
	m_ParamBlock ->SetCount( BlockParam::Mask,           int(count) );
	macroRec->Enable();

	SetNumMaps(int(count));
	ValidateParamBlock();
}

// When param block is changed, we readjust it to make sure we are ok.
void Composite::Texture::OnParamBlockChanged( ParamID id ) {
	if (m_ParamBlock) {
		int count( m_ParamBlock->Count( id ) );
		UpdateParamBlock( count );
	}
}

// Add a color correction map between us and the texture/mask
void Composite::Texture::ColorCorrection( Composite::Layer* layer, bool mask ) {

	int index;
	if (mask)
		index = (int)layer->Index() * 2 + 1;
	else
		index = (int)layer->Index() * 2;

	ClassDesc2 * ccdesc( GetColorCorrectionDesc() );
	Texmap     * ref   ( GetSubTexmap( index ) );

	// We don't duplicate color correction here
	if (ref == NULL || (ref->ClassID() != ccdesc->ClassID())) {
		Texmap     * cc    ( (Texmap*)CreateInstance( ccdesc->SuperClassID(), ccdesc->ClassID() ) );

		cc->SetName( ccdesc->ClassName() );

		// Set the texture
		HoldSuspend hs; // don't need to be able to undo setting of the subtexture here
		cc->SetSubTexmap( 0, ref );
		hs.Resume();
		SetSubTexmap( index, cc );
	}
}

void Composite::Texture::AddLayer() {
	// If we've attained our maximum number of layers, we don't add a new one.
	if (nb_layer() >= Param::LayerMax) {
		MessageBox( 0, GetString( IDS_ERROR_MAXLAYER ),
			GetString( IDS_ERROR_TITLE ),
			MB_APPLMODAL | MB_OK | MB_ICONERROR );
		return;
	}

	UpdateParamBlock( nb_layer() + 1 );
}

void Composite::Texture::DeleteLayer( size_t Index ) {
	// If there is only one layer, or the index is outside of range,
	// we do not delete it.
	if (nb_layer() == 1) {
		MessageBox( 0, GetString( IDS_ERROR_MINLAYER ),
			GetString( IDS_ERROR_TITLE ),
			MB_APPLMODAL | MB_OK | MB_ICONERROR );
		return;
	} else if (Index >= nb_layer()) {
		return;
	}

	PopLayer( Index );
}

// Add a new layer at the end. If loading is false, sets its name to the default one.
// If we are loading a file, don't set the name as it would overwrite the name in the
// pb2 we are loading
void Composite::Texture::PushLayer( bool setName ) {
	if (theHold.Holding())
		theHold.Put(new PushLayerRestoreObject(this, m_ParamBlock, setName));

	// We create the Layer at the end...
	push_back( Composite::Layer( m_ParamBlock, nb_layer() ) );
	Composite::Layer &new_layer( *LayerIterator( nb_layer()-1 ) );

	if (setName)
		new_layer.Name( Layer::NameType( m_DefaultLayerName.c_str() ) );

	// Create and push the new Layer on the list
	if (m_ParamDialog) {
		m_ParamDialog->EnableDisableInterface();
		m_ParamDialog->CreateRollup( &new_layer );
	}

	ValidateParamBlock();
}

// Delete a layer.
void Composite::Texture::PopLayer( size_t Index, bool dragndrop ) {
	iterator it = LayerIterator( Index );

	if( it != end() ) {
		// Update the interface
		//If the PopLayer is coming from a drag n drop we don't delete the layer, but we hide it. It will get deleted when the dialog will be destroyed.
		//This is a hack but we can't delete it as it will make Qt crash if it comes during a Drag'n drop event.
		if (m_ParamDialog)
			dragndrop ? m_ParamDialog->HideRollup(&*it) : m_ParamDialog->DeleteRollup( &*it );

		erase( it );
		// invalidates it, so we need to refetch it.
		it = LayerIterator( Index ); // if Index is size() it will be end()

		// Update all indexes in the paramblock...
		for( ; it != end(); ++it ) {
			it->Index( it->Index() - 1 );
		}

		if (theHold.Holding())
			theHold.Put(new PopLayerBeforeRestoreObject(this, m_ParamBlock, Index));

		if (m_ParamBlock && !m_Updating) {
			// This is to prevent UpdateParamBlock from recalling us.
			// if we wrapped around, we are pretty well screwed because of everything else happening in this method.
			ValueGuard<bool> updating_guard( m_Updating, true );

			// Update the paramblock
			m_ParamBlock->Delete( BlockParam::Visible,      int(Index), 1 );
			m_ParamBlock->Delete( BlockParam::VisibleMask,  int(Index), 1 );
			m_ParamBlock->Delete( BlockParam::BlendMode,    int(Index), 1 );
			m_ParamBlock->Delete( BlockParam::Name,         int(Index), 1 );
			m_ParamBlock->Delete( BlockParam::DialogOpened, int(Index), 1 );
			m_ParamBlock->Delete( BlockParam::Opacity,      int(Index), 1 );
			m_ParamBlock->Delete( BlockParam::Texture,      int(Index), 1 );
			m_ParamBlock->Delete( BlockParam::Mask,         int(Index), 1 );
		}

		if (theHold.Holding())
			theHold.Put(new PopLayerAfterRestoreObject(this, m_ParamBlock, Index));

		// Redraw
		if (m_ParamDialog) {
			m_ParamDialog->EnableDisableInterface();
			m_ParamDialog->Invalidate();
		}
	}

	ValidateParamBlock();
}

// Insert a layer.
void Composite::Texture::InsertLayer( size_t Index ) {
	// Create and push the new Layer on the list
	iterator layer = LayerIterator ( Index );
	Composite::Layer &new_layer( Composite::Layer( m_ParamBlock, Index ) );
	insert( layer, new_layer );

	// Update all indexes in the paramblock...
	for( ; layer != end(); ++layer ) {
		layer->Index( layer->Index() + 1 );
	}

	if (theHold.Holding())
		theHold.Put(new InsertLayerBeforeRestoreObject(this, m_ParamBlock, Index));

	if (m_ParamBlock && !m_Updating) {
		// This is to prevent UpdateParamBlock from recalling us.
		ValueGuard<bool> updating_guard( m_Updating, true );

		Texmap *nullTexmap = NULL;
		BOOL visible = Default::Visible;
		BOOL visibleMask = Default::VisibleMask;
		float opacity = Default::Opacity;
		int blendMode = Default::BlendMode;
		BOOL dialogOpened = Default::DialogOpened;
		TCHAR *name = const_cast<TCHAR*>(m_DefaultLayerName.c_str());

		// Update the paramblock
		m_ParamBlock->Insert( BlockParam::Visible,      int(Index), 1, &visible );
		m_ParamBlock->Insert( BlockParam::VisibleMask,  int(Index), 1, &visibleMask );
		m_ParamBlock->Insert( BlockParam::BlendMode,    int(Index), 1, &blendMode );
		m_ParamBlock->Insert( BlockParam::Name,         int(Index), 1, &name );
		m_ParamBlock->Insert( BlockParam::DialogOpened, int(Index), 1, &dialogOpened );
		m_ParamBlock->Insert( BlockParam::Opacity,      int(Index), 1, &opacity );
		m_ParamBlock->Insert( BlockParam::Texture,      int(Index), 1, &nullTexmap );
		m_ParamBlock->Insert( BlockParam::Mask,         int(Index), 1, &nullTexmap );

		if (theHold.Holding())
			theHold.Put(new InsertLayerAfterRestoreObject(this, m_ParamBlock, Index));

		// Redraw
		if (m_ParamDialog) {
			m_ParamDialog->EnableDisableInterface();
			iterator new_layer = LayerIterator ( Index );
			m_ParamDialog->CreateRollup( &(*new_layer) );
			m_ParamDialog->Invalidate();
		}
	}

	ValidateParamBlock();
}

// Move a layer by inserting a layer, copying current layer data to it, and then deleting current layer.
// Tricky part is we need to send out a REFMSG_SUBANIM_NUMBER_CHANGED message with correct mapping
// TODO: PB2 isn't sending a REFMSG_SUBANIM_NUMBER_CHANGED for many cases, including setting a 
// reftarg-derived value or causing an animation controller to be assigned.
// before arg here is in terms of ui, which is opposite of layer order
void Composite::Texture::MoveLayer( size_t index, size_t dest, bool before ) {

	// create no new keys while setting values...
	AnimateSuspend as (TRUE, TRUE);

	if (before)
		dest++;

	iterator          it_src( LayerIterator( index ) );
	iterator          it_dst( LayerIterator( dest  ) );
	int               i     ( (int)index );

	if (it_src == it_dst)
		return;

	if (it_dst != begin() && it_src == --it_dst)
		return;

	// insert new, copy data, delete original
	InsertLayer( dest );
	it_dst = LayerIterator( dest  );
	it_dst->CopyValue( *it_src );
	PopLayer(it_src->Index(), true);

	if (m_ParamDialog) {
		// Re-wire the dialogs...
		m_ParamDialog->Rewire();
		NotifyChanged();
	}
}

// Basically a duplicated Layer is a new Layer copied from the layer to duplicate
// and moved to the right position.  This is to re-use the code.
void Composite::Texture::DuplicateLayer ( size_t Index ) {
	// If we've attained our maximum number of layers, we don't add a new one.
	if (nb_layer() >= Param::LayerMax) {
		MessageBox( 0, GetString( IDS_ERROR_MAXLAYER ),
			GetString( IDS_ERROR_TITLE ),
			MB_APPLMODAL | MB_OK | MB_ICONERROR );
		return;
	}

	DbgAssert( Index < nb_layer() );
	if (Index >= nb_layer())
		return;

	// suspend macro recorder, animate, set key
	SuspendAll suspendAll (FALSE, TRUE, TRUE, TRUE);

	// Insert the layer directly below me and copy values from me to new layer 
	iterator it     ( LayerIterator( Index ) );
	InsertLayer(Index);
	iterator new_lyr( LayerIterator( Index ) );
	new_lyr->CopyValue( *it );

	// Give the layer a new name so we can distinguish it from the original
	TSTR newName;
	TSTR origName = new_lyr->Name(); //
	if (origName.isNull())
		origName = GetString(IDS_UNNAMED_LAYER);
	newName.printf(_T("%s%s"),GetString(IDS_DUPLICATE_OF), origName.data());
	new_lyr->Name(const_cast<TCHAR*>(newName.data()));

	if (m_ParamDialog) {
		// Re-wire the dialogs...
		m_ParamDialog->Rewire();
		NotifyChanged();
	}
}

void Composite::Texture::Init() {
	macroRecorder->Disable();
	m_Validity.SetEmpty();
	m_mapValidity.SetEmpty();
	m_ViewportIval.SetEmpty();
	m_ViewportTextureList.swap( ViewportTextureListType() );
	UpdateParamBlock( 1 );
	macroRecorder->Enable();
	ResetDisplayTexHandles();
	mColorCorrectionMode = GetMaxColorCorrectionMode();
}

void Composite::Texture::Reset() {
	DeleteAllRefsFromMe();
	GetCompositeDesc()->MakeAutoParamBlocks(this);   // make and intialize paramblock2
	Init();
}

void Composite::Texture::NotifyChanged() {
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

Composite::Texture::Texture(BOOL loading)
	: m_ParamDialog   ( NULL )
	, m_ParamBlock    ( NULL )
	, m_Updating      ( false )
	, m_SettingNumMaps( false )
{
	// Init default values to members
	m_DefaultLayerName = Default::Name;
	m_FileVersion = kMax2009;
	m_OldMapCount_2008 = 0;
	m_OldMapCount_JohnsonBeta = 0;
	if (!loading)
		GetCompositeDesc()->MakeAutoParamBlocks( this );   // make and intialize paramblock2
	m_DisplayTexHandles.clear();
	m_DisplayTexValidInterval.SetEmpty();
	Init();
}

Composite::Texture::~Texture()
{	// other member is managed by smart pointers.
	ResetDisplayTexHandles();
}

// Update the number of layers, either adding or deleting the last one.
void Composite::Texture::SetNumMaps( int n,  bool setName ) {

	// guard against wrap around. Pushing/popping a layer causes a REFMSG_CHANGE message to be 
	// sent which hits Texture::NotifyRefChanged which calls UpdateParamBlock, which calls SetNumMaps
	if (m_SettingNumMaps)
		return;

	ValueGuard< bool >   change( m_SettingNumMaps, true );

	while( n != nb_layer() )
		if( n < nb_layer() )
			PopLayer( nb_layer()-1 );
		else
			PushLayer( setName );
}

void Composite::Texture::ValidateParamBlock() {
	// Check the count of every parameters so that they are coherent with internal
	// data.
	// If we are setting the number of maps, this may differ...
	if( m_ParamBlock && !m_SettingNumMaps ) {
		DbgAssert( m_ParamBlock->Count(BlockParam::Visible      ) == nb_layer() );
		DbgAssert( m_ParamBlock->Count(BlockParam::VisibleMask  ) == nb_layer() );
		DbgAssert( m_ParamBlock->Count(BlockParam::BlendMode    ) == nb_layer() );
		DbgAssert( m_ParamBlock->Count(BlockParam::Name         ) == nb_layer() );
		DbgAssert( m_ParamBlock->Count(BlockParam::DialogOpened ) == nb_layer() );
		DbgAssert( m_ParamBlock->Count(BlockParam::Opacity      ) == nb_layer() );
		DbgAssert( m_ParamBlock->Count(BlockParam::Texture      ) == nb_layer() );
		DbgAssert( m_ParamBlock->Count(BlockParam::Mask         ) == nb_layer() );
	}
}

// Accumulate the layers and return the result.
// Please note that EvalMono was not implemented because calling EvalMono
// of the children would not get the Alpha channel necessary for compositing.
// That leads to weird assumptions about blending and compositing.
AColor Composite::Texture::EvalColor(ShadeContext& sc) {
	AColor res;
	if ( sc.GetCache( this, res ) )
		return res;

	if ( gbufID )
		sc.SetGBufferID(gbufID);

	// Simple accumulator that blends the layers
	class accumulator {
	  ShadeContext&  sc;

	public:
	  accumulator( ShadeContext& shade ) : sc(shade) { }

		AColor operator()( const AColor& bg, const value_type& Layer ) {
		 // If the foreground is invisible, return the background.
		 if (false == Layer.IsVisible())
			return bg;

		 // Take color, apply Opacity, check Mask, Blend, rinse and re-do
		 AColor fg( Layer.Texture()->EvalColor( sc ) );

		 //we need to pull out the premultiplied alpha coming from a texture
		 if ((fg.a != 1.0f) && (fg.a != 0.0f)) {
			 fg.r /= fg.a;
			 fg.g /= fg.a;
			 fg.b /= fg.a;
		 }

		 if (Layer.IsMaskVisible()) {
			const float mask_alpha( Layer.Mask()->EvalMono( sc ) );
			fg.a *= mask_alpha;
		 }

		 fg.a *= Layer.NormalizedOpacity();

		 // If the background is invisible, this is equal to the foreground.
		 // Technically this means that the last visible layer is ALWAYS
		 // blended with Normal.
		 if (bg.a == 0) 
			return fg;

		 // AColor has no conversion to (const Color), only non-const. So we have
		 // to do it manually.
		 const Color bg_temp( bg.r, bg.g, bg.b );
		 // Apply Blending here.  fg is fore, bg is back.
		 const Color blended( Blending::ModeList[ Layer.Blend() ]( fg, bg_temp ) );
		 const float alpha  ( fg.a + (1 - fg.a) * bg.a );

			// We composite.  This is an "over" mode of composition, same as Combustion
			// and photoshop uses.
			// On blending mode: http://www.pegtop.net/delphi/articles/blendmodes/index.htm
			// Normal, add, subtract, average and multiple are not clamped
			const float retR( (  blended.r * (fg.a *      bg.a)
				+ fg.r      * (fg.a * (1 - bg.a))
				+ bg.r      * ((1 - fg.a) * bg.a)) / alpha );
			const float retG( (  blended.g * (fg.a *      bg.a)
				+ fg.g      * (fg.a * (1 - bg.a))
				+ bg.g      * ((1 - fg.a) * bg.a)) / alpha );
			const float retB( (  blended.b * (fg.a *      bg.a)
				+ fg.b      * (fg.a * (1 - bg.a))
				+ bg.b      * ((1 - fg.a) * bg.a)) / alpha );
	     
		 return AColor( retR, retG, retB, alpha );
	  }
	};

   // We don't want to accumulate from real Black
   res = std::accumulate( begin(), end(), AColor(0, 0, 0, 0), accumulator(sc) );


   
   //we now need to premultiple our apha into the results
   if (res.a != 1.0f)
   {
	   res.r *= res.a;
	   res.g *= res.a;
	   res.b *= res.a;
   }

   sc.PutCache( this, res );
   return res;
}

inline float BlendOver( float fg, float fgAlpha, float bg, float bgAlpha, float blended, float alpha  )
// Alpha should be ( fgAlpha + (1 - fgAlpha) * bgAlpha )
{
	// This is an "over" mode of composition, same as Combustion
	// and photoshop uses.
	// On blending mode: http://www.pegtop.net/delphi/articles/blendmodes/index.htm
	// Since everything is clamped between [0,1], there is no need to clamp
	// this color.
	return ( blended	* (fgAlpha *      bgAlpha)
			 + fg       * (fgAlpha * (1 - bgAlpha))
			 + bg       * ((1 - fgAlpha) * bgAlpha)) / alpha ;
}

float ClampZeroOne( float f ) { return (f<0? 0 : (f>1? 1 : f)); }
Color ClampZeroOne( const Color& c ) { return Color( ClampZeroOne(c.r), ClampZeroOne(c.g), ClampZeroOne(c.b) ); }
Color AddGreyscale( const Color& c, float f ) { return Color( c.r+f, c.g+f, c.b+f ); }
float  GetGreyscale( const Color& c ) { return ((c.r + c.g + c.b) / 3.0f); }


//  ---------- ---------- ---------- ---------- ---------- ---------- ---------- ----------
//  Composite Bump - Determines the bump value of the Composite Map

Point3 Composite::Texture::EvalNormalPerturb(ShadeContext& sc)  {
	Point3 result;

	if (!sc.doMaps) 
		return Point3(0,0,0);
	if (gbufID) 
		sc.SetGBufferID(gbufID);
	if ( sc.GetCache( this, result ) )
		return result;

	struct BumpModeInfo {
		Point3 deriv;
		Color  color;
		float  alpha;
		BumpModeInfo() : deriv(0,0,0), color(0,0,0), alpha(0) {}
	};

	class accumulator  {
	  ShadeContext&  sc;

	public:
	  accumulator( ShadeContext& shade ) : sc(shade) { }


	  BumpModeInfo operator()( const BumpModeInfo& bg, const value_type& Layer )
	  {
		 // If the foreground is invisible, return the background.
		 if (false == Layer.IsVisible())
			return bg;

		 BumpModeInfo bumpModeInfoTemp;	 
		 BumpModeInfo& fg = bumpModeInfoTemp;

		 AColor fgColor = Layer.Texture()->EvalColor( sc );
		 fg.color = fgColor;
		 fg.alpha = fgColor.a;

		 //we need to pull out the premultiplied alpha coming from a texture
		 if ((fg.alpha != 1.0f) && (fg.alpha != 0.0f)) {
			 fg.color.r /= fg.alpha;
			 fg.color.g /= fg.alpha;
			 fg.color.b /= fg.alpha;
		 }

		 if (Layer.IsMaskVisible())
			fg.alpha *= Layer.Mask()->EvalMono( sc );

		 fg.alpha *= Layer.NormalizedOpacity();

		 fg.deriv = Layer.Texture()->EvalNormalPerturb( sc );

		 // If the background is invisible, this is equal to the foreground.
		 // Technically this means that the last visible layer is ALWAYS
		 // blended with Normal.
		 if (bg.alpha == 0) 
			return fg;

		 float  alpha  ( fg.alpha + bg.alpha * (1 - fg.alpha) );


		 // Determine Blended Color.  fg is fore, bg is back.  
		 Color colorBlended( Blending::ModeList[ Layer.Blend() ]( fg.color, bg.color ) );


		 // Compute interpolated texture colors, three each for foreground and background.
		 // The distance across which we interpolate is scaled down by a constant factor.
		 // This gives a derivative that is more accurate locally.
		 // The magnitude of the final derivate must be scaled up by an inverse factor.
		 const float magicNumber = 0.1f; // Chosen through empirical tests; from 1/4 to 1/16 is reasonable.

		 float stepScale = magicNumber; 
		 float stepScaleInv = 1.0f / magicNumber;

		 Color fgInterpX = ClampZeroOne( AddGreyscale(fg.color, -(fg.deriv.x * stepScale)) );
		 Color fgInterpY = ClampZeroOne( AddGreyscale(fg.color, -(fg.deriv.y * stepScale)) );
		 Color fgInterpZ = ClampZeroOne( AddGreyscale(fg.color, -(fg.deriv.z * stepScale)) );

		 Color bgInterpX = ClampZeroOne( AddGreyscale(bg.color, -(bg.deriv.x * stepScale)) );
		 Color bgInterpY = ClampZeroOne( AddGreyscale(bg.color, -(bg.deriv.y * stepScale)) );
		 Color bgInterpZ = ClampZeroOne( AddGreyscale(bg.color, -(bg.deriv.z * stepScale)) );

		 // Compute the interpolated composite color for each pair of interpolated texture colors
		 Color colorBlendedInterpX( Blending::ModeList[ Layer.Blend() ]( fgInterpX, bgInterpX ) );
		 Color colorBlendedInterpY( Blending::ModeList[ Layer.Blend() ]( fgInterpY, bgInterpY ) );
		 Color colorBlendedInterpZ( Blending::ModeList[ Layer.Blend() ]( fgInterpZ, bgInterpZ ) );

		 float greyInterpX = GetGreyscale( colorBlendedInterpX );
		 float greyInterpY = GetGreyscale( colorBlendedInterpY );
		 float greyInterpZ = GetGreyscale( colorBlendedInterpZ );

		 float greyOrigin = GetGreyscale( colorBlended );

		 // Compute the derivative as the difference between the original and interpolated composite colors
		 Point3 derivBlended( greyOrigin - greyInterpX, greyOrigin - greyInterpY, greyOrigin - greyInterpZ );

		 derivBlended *= stepScaleInv; //scale back up


		 // We composite.
		 // retval and fg point to the same memory - operations are ordered so each value is overwritten only when not needed later

		 BumpModeInfo& retval = bumpModeInfoTemp;
		 retval.deriv.x = ( BlendOver( fg.deriv.x, fg.alpha, bg.deriv.x, bg.alpha, derivBlended.x, alpha ) );
		 retval.deriv.y = ( BlendOver( fg.deriv.y, fg.alpha, bg.deriv.y, bg.alpha, derivBlended.y, alpha ) );
		 retval.deriv.z = ( BlendOver( fg.deriv.z, fg.alpha, bg.deriv.z, bg.alpha, derivBlended.z, alpha ) );

		 retval.color.r = ( BlendOver( fg.color.r, fg.alpha, bg.color.r, bg.alpha, colorBlended.r, alpha ) );
		 retval.color.g = ( BlendOver( fg.color.g, fg.alpha, bg.color.g, bg.alpha, colorBlended.g, alpha ) );
		 retval.color.b = ( BlendOver( fg.color.b, fg.alpha, bg.color.b, bg.alpha, colorBlended.b, alpha ) );

		 retval.alpha = alpha;

		 return retval;
	  }
	};


	BumpModeInfo bumpInfo;
	bumpInfo = BumpModeInfo(std::accumulate( begin(), end(), bumpInfo, accumulator(sc) ));
	result = bumpInfo.deriv;

	sc.PutCache( this, result );
	return result;
}

int Composite::Texture::NumRefs() {
	if (m_FileVersion == kMax2008)
		return 1 + m_OldMapCount_2008;	//just the parm block + maps
	else if (m_FileVersion == kJohnsonBeta)
		return 1 + m_OldMapCount_JohnsonBeta;	//just the parm block + maps + masks
	else 
		return 1;  //just the parm block
}

// See reference system in the {Notes} region.
RefTargetHandle Composite::Texture::GetReference(int i) {
	if (m_FileVersion == kMax2009) {
		if (i == 0)
			return m_ParamBlock;
	}
	else {
		if (i == 0)
			return m_ParamBlock;
		else {
			int mapID = i -1;
			if (mapID < m_oldMapList.Count())
				return m_oldMapList[mapID];
		}
	}
	return NULL;

}

void Composite::Texture::SetReference(int i, RefTargetHandle v) {
	if (m_FileVersion == kMax2009) {
		//we only have 1 reference now the paramblock
		if (i == 0) {
			IParamBlock2* v_ParamBlock = (IParamBlock2*)(v);

			// we are guaranteed to have one of the following 3 cases:
			//		v_ParamBlock		m_ParamBlock
			//		NULL				NULL
			//		NULL				non-NULL
			//		non-NULL			NULL
			// we will never have both v_ParamBlock and m_ParamBlock non-NULL

			DbgAssert (!(v_ParamBlock != NULL && m_ParamBlock != NULL));

			if (v_ParamBlock) {
				// Create the number of layers, don't set new layer names
				m_ParamBlock = v_ParamBlock;
				SetNumMaps( m_ParamBlock->Count( BlockParam::Texture ), false );
			}
			else {
				SetNumMaps( 0 );
				m_ParamBlock = v_ParamBlock;
			}

			// Update param blocks for all layers
			struct {
				IParamBlock2 *m_ParamBlock;
				void operator()( value_type& Layer ) {
					Layer.ParamBlock( m_ParamBlock );
				}
			} predicate = {m_ParamBlock};
			for_each( begin(), end(), predicate );
		}
		else {
			DbgAssert(0);
		}
	}
	else {
		if ( i == 0 ) {
			m_ParamBlock = (IParamBlock2*)(v);

			if (m_ParamBlock)
				m_OldMapCount_JohnsonBeta = m_ParamBlock->Count(BlockParam::Texture_DEPRECATED) * 2;
			else
				m_OldMapCount_JohnsonBeta = 0;

			struct {
				IParamBlock2 *m_ParamBlock;
				void operator()( value_type& Layer ) {
					Layer.ParamBlock( m_ParamBlock );
				}
			} predicate = {m_ParamBlock};
			for_each( begin(), end(), predicate );
		}
		else {
			int mapID = i-1;
			while (mapID >= m_oldMapList.Count()) {
				Texmap *map = NULL;
				m_oldMapList.Append(1,&map,100);
			}
			m_oldMapList[mapID] = (Texmap*) v;
		}
	}
}

RefTargetHandle Composite::Texture::Clone(RemapDir &remap) {
	Texture *NewMtl = new Texture( TRUE ); // don't need the PB2 to be created
	*((MtlBase*)NewMtl) = *((MtlBase*)this);  // copy superclass stuff

	// Replace the ParamBlock
	NewMtl->ReplaceReference( 0, remap.CloneRef( m_ParamBlock ) );

	// Copy the internal variables
	NewMtl->m_DefaultLayerName = m_DefaultLayerName;

	// Empty the interactive rendering variables
	NewMtl->m_ViewportIval.SetEmpty();
	NewMtl->m_ViewportTextureList.swap( ViewportTextureListType() );
	NewMtl->ResetDisplayTexHandles();

	BaseClone(this, NewMtl, remap);
	return (RefTargetHandle)NewMtl;
}

ParamDlg* Composite::Texture::CreateParamDlg(HWND editor_wnd, IMtlParams *imp) {
	m_ParamDialog = new Dialog( editor_wnd, imp, this );
	return m_ParamDialog;
}

void Composite::Texture::Update(TimeValue t, Interval& valid) {
	if (!m_Validity.InInterval(t)) {
		m_Validity.SetInfinite();
		m_ParamBlock->GetValidity(t,m_Validity);
		NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_DISPLAY_MATERIAL_CHANGE);
	}

	if (!m_mapValidity.InInterval(t))
	{
		m_mapValidity.SetInfinite();
		for (iterator it = begin(); it != end(); ++it)
			it->Update( t, m_mapValidity );
	}

	valid &= m_mapValidity;
	valid &= m_Validity;

	int numTextures		= m_ParamBlock->Count(BlockParam::Texture);
	int numMasks		= m_ParamBlock->Count(BlockParam::Mask);
	int numOpacities	= m_ParamBlock->Count(BlockParam::Opacity);
	int numBlendModes	= m_ParamBlock->Count(BlockParam::BlendMode);
	int numDlgOpen		= m_ParamBlock->Count(BlockParam::DialogOpened);
	int numNames		= m_ParamBlock->Count(BlockParam::Name);
	int numVisible		= m_ParamBlock->Count(BlockParam::Visible);
	int numVisibleMask	= m_ParamBlock->Count(BlockParam::VisibleMask);

	int layerCount = (int)nb_layer();
	DbgAssert( layerCount == numTextures &&
			   layerCount == numMasks &&
			   layerCount == numOpacities &&
			   layerCount == numBlendModes &&
			   layerCount == numDlgOpen &&
			   layerCount == numNames &&
			   layerCount == numVisible &&
			   layerCount == numVisibleMask
		);

	int id = 0;
	for (iterator it = begin(); it != end(); ++it) 
	{
		Interval iv;		
		DbgAssert((int)it->Index() == id);

		BOOL visible = FALSE;
		m_ParamBlock->GetValue(BlockParam::Visible,t,visible,FOREVER,id);
		DbgAssert( visible == it->Visible());

		BOOL maskVisible = FALSE;
		m_ParamBlock->GetValue(BlockParam::VisibleMask,t,maskVisible,FOREVER,id);
		DbgAssert( maskVisible == it->VisibleMask());

		int blendMode = 0;
		m_ParamBlock->GetValue(BlockParam::BlendMode,t,blendMode,FOREVER,id);
		DbgAssert( blendMode == it->Blend());

		TSTR name = m_ParamBlock->GetStr(BlockParam::Name,t,id);
		DbgAssert( name == TSTR (it->Name()));

		BOOL dialogOpened = FALSE;
		m_ParamBlock->GetValue(BlockParam::DialogOpened,t,dialogOpened,FOREVER,id);
		DbgAssert( dialogOpened == it->DialogOpened());

		float f1 = it->Opacity();
		float f2 = m_ParamBlock->GetFloat(BlockParam::Opacity,t,id);
		DbgAssert(f1==f2);

		Texmap *map = NULL;
		m_ParamBlock->GetValue(BlockParam::Texture,t,map,FOREVER,id);
		DbgAssert(map==it->Texture());

		m_ParamBlock->GetValue(BlockParam::Mask,t,map,FOREVER,id);
		DbgAssert(map==it->Mask());

		id++;
	}
}

void Composite::Texture::SetSubTexmap( int i, Texmap *m ) {
	if (m_FileVersion == kMax2009) {
		if (!is_subtexmap_idx_valid(i)) {
			DbgAssert(false);
			return ;
		}

		iterator layer = LayerIterator( subtexmap_idx_to_lyr(i) );
		DbgAssert( layer != end() );
		if (is_subtexmap_idx_tex(i))
			layer->Texture(m);
		else
			layer->Mask   (m);
	}
	else
		DbgAssert(false);
}

Texmap* Composite::Texture::GetSubTexmap(int i) {

	if (m_FileVersion == kMax2009) {
		if (!is_subtexmap_idx_valid(i)) {
			DbgAssert(false);
			return NULL;
		}

		iterator layer = LayerIterator ( subtexmap_idx_to_lyr(i) );
		DbgAssert( layer != end() );
		return is_subtexmap_idx_tex(i) ? layer->Texture() : layer->Mask();
	}
	else {
		DbgAssert(false);
		return NULL;
	}
}

TSTR Composite::Texture::GetSubTexmapSlotName( int i ) {
	if (m_FileVersion == kMax2009) {
		if (false == is_subtexmap_idx_valid(i)) {
			DbgAssert(0);
			return TSTR();
		}

		iterator layer = LayerIterator ( subtexmap_idx_to_lyr(i) );
		DbgAssert( layer != end() );
		if (is_subtexmap_idx_tex(i))
			return layer->ProcessName();
		else
			return TSTR( layer->ProcessName() ) + _T(" (Mask)");
	}
	else {
		DbgAssert(0);
		return TSTR();
	}
}

RefResult Composite::Texture::NotifyRefChanged( const Interval &changeInt,
											    RefTargetHandle hTarget,
											    PartID         &partID,
												RefMessage      message, 
												BOOL            propagate )
{
	switch (message) {
	case REFMSG_CHANGE:
		// invalidate the texture
		m_Validity.SetEmpty();
		m_mapValidity.SetEmpty();
		// invalidate the layer
		if (hTarget == m_ParamBlock) {
			int index = -1;
			//get which parameter is changing 
			ParamID changing_param = m_ParamBlock->LastNotifyParamID(index);
			if (index >= 0 && index < nb_layer()) {
				iterator it = LayerIterator(index);
				if (it != end())
					it->Invalidate();
			}
		}

		// Clean up the interactive renderer internals.
		m_ViewportIval.SetEmpty();
		// Empty the vector
		m_ViewportTextureList.swap( ViewportTextureListType() );
		ResetDisplayTexHandles();

		// Recalculate the number of layers
		if (m_ParamBlock && nb_layer() != m_ParamBlock->Count( BlockParam::Texture ))
			UpdateParamBlock( m_ParamBlock->Count( BlockParam::Texture ) );

		// Re-show everything
		if (m_ParamDialog && (hTarget == m_ParamBlock)) {
			int index = -1;
			//get which parameter is changing and do specific ui updateing
			//if we can
			ParamID changing_param = m_ParamBlock->LastNotifyParamID(index);

			if (index >= 0 && index < m_ParamDialog->size() ) {
				Composite::Dialog::iterator it = m_ParamDialog->find(index);
				if (it != m_ParamDialog->end())
				{
					if (changing_param == BlockParam::Opacity) {
						if (index < m_ParamBlock->Count(BlockParam::Opacity))
							it->UpdateOpacity();
					}
					else if (changing_param == BlockParam::Texture) {
						if (index < m_ParamBlock->Count(BlockParam::Texture))
							it->UpdateTexture();
					}
					else if (changing_param == BlockParam::Mask) {
						if (index < m_ParamBlock->Count(BlockParam::Mask))
							it->UpdateMask();
					}
					else // Re-show everything
						m_ParamDialog->Invalidate();
				}
			}
			else
				m_ParamDialog->Invalidate();

		}
		break;

	case REFMSG_GET_PARAM_NAME: {
			GetParamName *gpn = (GetParamName*)partID;
			gpn->name = GetSubTexmapSlotName(gpn->index);
			return REF_HALT;
		}

	case REFMSG_SUBANIM_STRUCTURE_CHANGED:
		if (hTarget == m_ParamBlock && theHold.RestoreOrRedoing()) {
			for( Texture::iterator it  = begin(); it != end(); ++it )
				it->Invalidate();
		}
		break;

	}
	return(REF_SUCCEED);
}

// This map is meaningful iif all of its submaps are on.
bool Composite::Texture::IsLocalOutputMeaningful( ShadeContext& sc ) {
	struct {
		bool operator()( const value_type& l ) {
			return TRUE == l.IsVisible();
		}
	} predicate;

	return std::find_if( begin(), end(), predicate ) == end();
}

#pragma region Load/Save serializers

IOResult Composite::Texture::Save(ISave* isave) {
	IOResult res( IO_OK );

	// Header and version to ensure that it is us. If those fields are missing,
	// loading will not work correctly.
	isave->BeginChunk     ( Chunk::Header );
	res = MtlBase::Save  ( isave );
	if (res != IO_OK) return res;
	isave->EndChunk       ( );

	ULONG	nBytes = 0;
	isave->BeginChunk     ( Chunk::Version );
	res = isave->WriteEnum( &m_FileVersion, sizeof(m_FileVersion), &nBytes );	
	if (res != IO_OK) return res;
	isave->EndChunk       ( );

	return res;
}

IOResult Composite::Texture::Load(ILoad *iload) {
	// Register a callback that will make sure the new layers have
	// all parameters.
	struct callback : public PostLoadCallback {
		Texture *m_Texture;

		callback( Texture* mtl )
			: m_Texture(mtl) {}

		void proc(ILoad *iload) {
			// Don't leak...  we are the sole responsible here.
			// Doing this prevents this class from leaking on exception.
			SmartPtr< callback > exception_guard( this );

			//FIX for 2008 to 2009 to ref structure
			//just need to copy owner ref maps to the paramblock and then drop the Texture's reference
			if (m_Texture->m_FileVersion == kMax2008) {
				int ct = m_Texture->m_oldMapList.Count();
				if (ct > 0) {
					m_Texture->UpdateParamBlock( ct );
					for (int i = 0; i < ct; i++) {
						Texmap *texmap = m_Texture->m_oldMapList[i];
						if (texmap) {
							m_Texture->GetParamBlock(0)->SetValue(BlockParam::Texture,0,texmap,i);	
							m_Texture->ReplaceReference(i+1, NULL);
						}
					}
				}
				else { // need at least 1 layer
					m_Texture->UpdateParamBlock( 1 );
				}

				m_Texture->GetParamBlock(0)->SetCount(BlockParam::Texture_DEPRECATED,0);
			}
			//FIX for JohnsonBeta to 2009 ref structure
			//just need to copy owner ref maps and masks to the paramblock
			else if (m_Texture->m_FileVersion == kJohnsonBeta) {
				int ct = m_Texture->m_OldMapCount_JohnsonBeta/2;
				m_Texture->UpdateParamBlock( ct );

				int maskID = ct;
				for (int i = 0; i < ct; i++) {
					if (i < m_Texture->m_oldMapList.Count()) {
						Texmap *texmap = m_Texture->m_oldMapList[i];
						if (texmap) {
							m_Texture->GetParamBlock(0)->SetValue(BlockParam::Texture,0,texmap,i);
							m_Texture->ReplaceReference(i+1, NULL);
						}
					}
					if (maskID < m_Texture->m_oldMapList.Count()) {
						Texmap *texmap = m_Texture->m_oldMapList[maskID];
						if (texmap) {
							m_Texture->GetParamBlock(0)->SetValue(BlockParam::Mask,0,texmap,i);	
							m_Texture->ReplaceReference(maskID+1, NULL);
						}
					}
					maskID++;
				}
				m_Texture->GetParamBlock(0)->SetCount(BlockParam::Texture_DEPRECATED,0);
				m_Texture->GetParamBlock(0)->SetCount(BlockParam::Mask_DEPRECATED,0);
			}

			DbgAssert(m_Texture->GetParamBlock(0)->Count(BlockParam::Texture_DEPRECATED) == 0);
			DbgAssert(m_Texture->GetParamBlock(0)->Count(BlockParam::Mask_DEPRECATED) == 0);
			
			m_Texture->m_oldMapList.SetCount(0);
			
			m_Texture->m_FileVersion = kMax2009;

			 //FIX*** for changing the opacity from int to float
			int count( m_Texture->GetParamBlock( 0 )->Count( BlockParam::Texture ) );

			//see if we have old integer based opacity values
			int oldOpacityParamCount = m_Texture->GetParamBlock( 0 )->Count( BlockParam::Opacity_DEPRECATED ) ;
			int opacityParamCount = m_Texture->GetParamBlock( 0 )->Count( BlockParam::Opacity ) ;
			if (opacityParamCount >= oldOpacityParamCount)
				oldOpacityParamCount = 0;

			if (oldOpacityParamCount > 0) {
				m_Texture->GetParamBlock( 0 )->SetCount( BlockParam::Opacity,oldOpacityParamCount ) ;
				//loop through each opac value
				for (int i = 0; i < oldOpacityParamCount; i++) {
					//see if it is animated if so we need to copy the controller
					Control *c =  m_Texture->GetParamBlock( 0 )->GetControllerByID(BlockParam::Opacity_DEPRECATED , i);
					if (c)
						m_Texture->GetParamBlock( 0 )->SetControllerByID(BlockParam::Opacity,i,c,FALSE);
					//otherwise we can just copy the value
					else {
						float opac = (float) m_Texture->GetParamBlock( 0 )->GetInt(BlockParam::Opacity_DEPRECATED,0,i);
						m_Texture->GetParamBlock( 0 )->SetValue(BlockParam::Opacity,0,opac,i);
					}
				}

				//make sure to remove those old values				
			}
			m_Texture->GetParamBlock( 0 )->SetCount( BlockParam::Opacity_DEPRECATED,0 ) ;

			
			//we have to make sure to update the pointer caches to the maps and masks are updated 
			m_Texture->UpdateParamBlock( count );

			int maskCount = m_Texture->m_ParamBlock->Count(BlockParam::Mask);			
			for(int i = 0; i < maskCount; i++) {
				 Texmap *map = NULL;
				 m_Texture->GetParamBlock(0)->GetValue(BlockParam::Mask,0,map,FOREVER,i);
				 m_Texture->LayerIterator(i)->Mask( map );
			}
			int mapCount = m_Texture->m_ParamBlock->Count(BlockParam::Texture);
			for(int i = 0; i < mapCount; i++) {
				Texmap *map = NULL;
				m_Texture->GetParamBlock(0)->GetValue(BlockParam::Texture,0,map,FOREVER,i);
				m_Texture->LayerIterator(i)->Texture( map );
			}

			m_Texture->NotifyChanged();
		}
	};

	IOResult res( IO_OK );
	bool     head_loaded( false );
	bool     compat_mode( false );

	//we set the version to JohnsonBeta since it did not have a flag
	// 2008 we can check by seeing if there is subtex count
	// 2009 will have the flag
	m_FileVersion = kJohnsonBeta;

	while( IO_OK == (res=iload->OpenChunk()) ) {
		int   chunk_id = iload->CurChunkID();
		ULONG tmp;

		switch( chunk_id ) {
		// Latest Version chunks
		case Chunk::Header:
			// Should not contain two headers or a new header
			// and a legacy tag.
			if (compat_mode || head_loaded) {
				return IO_ERROR;
			}
			iload->RegisterPostLoadCallback( new callback( this ) );

			res         = MtlBase::Load( iload );
			head_loaded = true;
			break;
		case Chunk::Version:   
			ULONG nb;
			res = iload->ReadEnum(&m_FileVersion, sizeof(m_FileVersion), &nb);
			break;
		// Legacy chunks
		case Chunk::Compatibility::Header:
			res         = MtlBase::Load( iload );
			head_loaded = true;
			compat_mode = true;
			break;
		case Chunk::Compatibility::SubTexCount: {
			compat_mode = true;
			int count;
			m_FileVersion = kMax2008;
			res = iload->Read( &count, sizeof(count), &tmp );
			m_OldMapCount_2008 = count;
			if (res != IO_OK) return res;

			iload->RegisterPostLoadCallback( new callback( this ) );
			break;
			}

		 default:
			 // Unknown chunk.  You may want to break here for debugging.
			 res = IO_OK;
		}

		if (res != IO_OK) return res;
		res = iload->CloseChunk();
		if (res != IO_OK) return res;
	}
	return IO_OK;
}

#pragma endregion

#pragma region Viewport Interactive Renderer

void Composite::Texture::ActivateTexDisplay(BOOL onoff) {}


bool Composite::Texture::CreateViewportDisplayBitmap(PBITMAPINFO& pOutputMap,
													 TimeValue time, 
													 TexHandleMaker& callback, 
													 Interval& validInterval, 
													 iterator& it,
													 bool bForceSize,
													 int forceWidth,
													 int forceHeight)
{
	pOutputMap = NULL;
	if (it->IsVisible()) 
	{
		if (bForceSize == false)
		{
			forceWidth = 0;
			forceHeight = 0;
		}

		const unsigned opacity( unsigned( it->NormalizedOpacity() * 255.0f ));

		Interval theInterval;
		theInterval.SetEmpty();
		PBITMAPINFO bmi = it->Texture()->GetVPDisplayDIB(time, callback, theInterval, false, forceWidth, forceHeight);
		if(!bmi){
			return false;
		}
		validInterval &= theInterval;
		pOutputMap = bmi;

		unsigned char *data( (unsigned char*)bmi + sizeof( BITMAPINFOHEADER ) );
		const int      w   (bmi->bmiHeader.biWidth);
		const int      h   (bmi->bmiHeader.biHeight);

		if (it->IsMaskVisible()) 
		{
			theInterval.SetEmpty();
			PBITMAPINFO pMaskBmi = it->Mask()->GetVPDisplayDIB(time, callback, theInterval, true, w, h);
			if(pMaskBmi)
			{
				const int      Maskw   (pMaskBmi->bmiHeader.biWidth);
				const int      Maskh   (pMaskBmi->bmiHeader.biHeight);
				validInterval &= theInterval;
				UBYTE* pMaskPixel = ((UBYTE *)((BYTE *)(pMaskBmi) + sizeof(BITMAPINFOHEADER)));
				for( int y = 0; y < h; ++y ) 
				{
					for( int x = 0; x < w;  ++x,data += 4 )
					{ 
						const unsigned long alpha(data[3]);
						unsigned long mask = 1;
						if(y<Maskh && x<Maskw)
						{
							pMaskPixel += 4;
							mask = pMaskPixel[0];
						}
						data[3] = (unsigned char)( alpha * mask * opacity / 255 / 255 );
					}
				}
				free(pMaskBmi); // free the mask map bitmap.
			}
		} 
		else 
		{
			for( int y = 0; y < h; ++y ) 
			{
				for( int x = 0; x < w; ++x, data += 4 ) 
				{ 
					const unsigned long alpha(data[3]);
					data[3] = (unsigned char)(alpha * opacity / 255 ); // alpha[0,255]* o[0,255] , so don't ">>8"
				}
			}
		}
		return true;
	}
	return false;
}
// Init the interactive multi-map rendering
// Creates bitmaps from the sub-maps.  Modify those bitmaps to include
// the mask and opacity, then create handle and push it on the vector.
void Composite::Texture::InitMultimap( TimeValue  time,
									   MtlMakerCallback &callback ) {
	// Emptying the viewport vector.
	m_ViewportTextureList.swap( ViewportTextureListType() );

	// Determine how many textures we're going to show.
	int nb_tex( min<int>( int(CountVisibleLayer()),
		Param::LayerMaxShown,
		callback.NumberTexturesSupported() ) );

	iterator it( begin() );
	for( int i = 0; i < nb_tex && it != end(); ++it ) {
		if (it->IsVisible()) {
			PBITMAPINFO bmi = NULL;
			if (CreateViewportDisplayBitmap(bmi, time, callback, GetValidity(), it, false, 0, 0) && bmi)
			{
				TexHandlePtr handle( callback.MakeHandle( bmi ) );
				m_ViewportTextureList.push_back( viewport_texture( handle, it ) );
			}
			++i;
		}
	}
}

// Configure max to show the texture in DirectX.
void Composite::Texture::PrepareHWMultimap( TimeValue  time,
										    ::IHardwareMaterial *hw_material,
										    ::Material *material,
										    MtlMakerCallback &callback ) {
	hw_material->SetNumTexStages( DWORD(m_ViewportTextureList.size()) );

	ViewportTextureListType::iterator it( m_ViewportTextureList.begin() );
	for (int i = 0; it != m_ViewportTextureList.end(); ++it, ++i) {
		hw_material->SetTexture( i, it->handle()->GetHandle() );

		material->texture[0].useTex = i;
		Texmap* pSubMap = it->Layer()->Mask();
		if(!pSubMap){
			pSubMap = it->Layer()->Texture();
		}

		callback.GetGfxTexInfoFromTexmap( time, material->texture[0], pSubMap?pSubMap:this);

		// Set the operations and the arguments for the Texture
		if (0 == i)
			hw_material->SetTextureColorOp    ( i, D3DTOP_MODULATE );
		else
			hw_material->SetTextureColorOp    ( i, D3DTOP_BLENDTEXTUREALPHA );

		hw_material->SetTextureColorArg      ( i, 1, D3DTA_TEXTURE  );
		hw_material->SetTextureColorArg      ( i, 2, D3DTA_CURRENT  );

		hw_material->SetTextureAlphaOp       ( i, D3DTOP_SELECTARG2 );
		hw_material->SetTextureAlphaArg      ( i, 1, D3DTA_TEXTURE  );
		hw_material->SetTextureAlphaArg      ( i, 2, D3DTA_CURRENT  );

		hw_material->SetTextureTransformFlag ( i, D3DTTFF_COUNT2    );
	}
}

// Configure max to show the texture, either in OpenGL or software.
void Composite::Texture::PrepareSWMultimap( TimeValue  time,
										    ::Material *material,
										    MtlMakerCallback &callback ) {
	material->texture.setLengthUsed( int(m_ViewportTextureList.size()) );

	ViewportTextureListType::iterator it( m_ViewportTextureList.begin() );
	for( int i = 0; it != m_ViewportTextureList.end(); ++i, ++it ) {
		Texmap* pSubMap = it->Layer()->Mask();
		if(!pSubMap){
			pSubMap = it->Layer()->Texture();
		}
		callback.GetGfxTexInfoFromTexmap( time, material->texture[i], pSubMap?pSubMap:this );
		material->texture[i].textHandle = it->handle()->GetHandle();

		material->texture[i].colorOp          = GW_TEX_ALPHA_BLEND;
		material->texture[i].colorAlphaSource = GW_TEX_TEXTURE;
		material->texture[i].colorScale       = GW_TEX_SCALE_1X;
		material->texture[i].alphaOp          = GW_TEX_MODULATE;
		material->texture[i].alphaAlphaSource = GW_TEX_TEXTURE;
		material->texture[i].alphaScale       = GW_TEX_SCALE_1X;
	}
}

void Composite::Texture::ResetDisplayTexHandles()
{
	for (int index = 0; index < m_DisplayTexHandles.size(); index++)
	{
		if (m_DisplayTexHandles[index])
		{
			m_DisplayTexHandles[index]->DeleteThis();
			m_DisplayTexHandles[index] = NULL;
		}
	}
	m_DisplayTexHandles.clear();
	
	m_DisplayTexValidInterval.SetEmpty();
}

void Composite::Texture::SetupGfxMultiMaps( TimeValue  t,
										    ::Material *mtl,
										    MtlMakerCallback &cb ) {
	bool colorCorrectionModeChanged = UpdateColorCorrectionMode(mColorCorrectionMode);

	// Are we still valid?  If not, initialize...
	if ( false == m_ViewportIval.InInterval(t) || colorCorrectionModeChanged) {
		// Set the interval and initialize the map.
		m_ViewportIval = GetValidity();
		InitMultimap( t, cb );
	}

	IHardwareMaterial *HWMaterial = (IHardwareMaterial *)GetProperty(PROPID_HARDWARE_MATERIAL);

	// Are we using hardware?
	if (HWMaterial)
		PrepareHWMultimap( t, HWMaterial, mtl, cb );
	else
		PrepareSWMultimap( t, mtl, cb );
}

void BlendBitmaps(BITMAPINFO* bitmapA, BITMAPINFO* bitmapB)
{
	unsigned char *dataA( (unsigned char*)bitmapA + sizeof( BITMAPINFOHEADER ) );
	unsigned char *dataB( (unsigned char*)bitmapB + sizeof( BITMAPINFOHEADER ) );
	const int      w   (bitmapA->bmiHeader.biWidth);
	const int      h   (bitmapA->bmiHeader.biHeight);
	const int count = w*h;
	for (int i = 0; i < count; ++i)
	{
		float alphaA = dataB[3]/255.0f;
		dataA[0] = dataB[0]*alphaA + dataA[0]*(1.0f - alphaA);
		dataA[1] = dataB[1]*alphaA + dataA[1]*(1.0f - alphaA);
		dataA[2] = dataB[2]*alphaA + dataA[2]*(1.0f - alphaA);;
		dataA += 4;
		dataB += 4;
	}
}

void Composite::Texture::SetupTextures(TimeValue t, DisplayTextureHelper &cb)
{
	ISimpleMaterial *pISimpleMaterial = (ISimpleMaterial *)GetProperty(PROPID_SIMPLE_MATERIAL);
	if (pISimpleMaterial == nullptr)
	{
		return;
	}
	ISimpleMaterialExt* pSimpleMaterialExt = static_cast<ISimpleMaterialExt*>(pISimpleMaterial->GetInterface(ISIMPLE_MATERIAL_EXT_INTERFACE_ID));
	if (pSimpleMaterialExt == nullptr)
	{
		return;
	}

	bool colorCorrectionModeChanged = UpdateColorCorrectionMode(mColorCorrectionMode);

	// Are we still valid?  If not, initialize...
	if (!m_DisplayTexValidInterval.InInterval(t) || colorCorrectionModeChanged)
	{
		// Set the interval and initialize the map.
		ResetDisplayTexHandles();
		m_DisplayTexValidInterval.SetInfinite();

		int forceWidth = 0;
		int forceHeight = 0;
		BITMAPINFO* bmiMaxLastOneCandidate = NULL;
		bool bForceSize = false;
		iterator it( begin() );
		for( ; it != end(); ++it ) 
		{
			if (!it->IsVisible()) 
			{
				continue;
			}

			BITMAPINFO* bmi = NULL;
			Interval tempInterval;
			tempInterval.SetInfinite();
			if (!CreateViewportDisplayBitmap(bmi, t, cb, tempInterval, it, bForceSize, forceWidth, forceHeight) || bmi == nullptr)
			{
				continue;
			}

			m_DisplayTexValidInterval &= tempInterval;
			if (!bForceSize)
			{
				forceWidth = bmi->bmiHeader.biWidth;
				forceHeight = bmi->bmiHeader.biHeight;
				bForceSize = true;
			}
			
			int maxLastOneIndex = pSimpleMaterialExt->GetMaxTextureStageCount() - 1;
			if (m_DisplayTexHandles.size() < maxLastOneIndex)
			{
				TexHandle* pTextureHandle = cb.MakeHandle( bmi );
				m_DisplayTexHandles.push_back(pTextureHandle);
			}
			else
			{
				if (m_DisplayTexHandles.size() == maxLastOneIndex)
				{
					bmiMaxLastOneCandidate = bmi;
				}
				else
				{
					//combine the existing bmiMaxLastOneCandidate with the new layer bitmap if possible.
					Texmap* pTexmap = it->Texture();
					if (IsSameSize(bmi, bmiMaxLastOneCandidate))
					{
						BlendBitmaps(bmiMaxLastOneCandidate, bmi);
						free(bmi);
						bmi = NULL;
					}
				}
			}
		}

		if (bmiMaxLastOneCandidate != nullptr)
		{
			TexHandle* pTextureHandle = cb.MakeHandle( bmiMaxLastOneCandidate );
			m_DisplayTexHandles.push_back(pTextureHandle);
		}
	}

	pSimpleMaterialExt->ClearTextures();
	int texmapIndex = 0;
	if (m_DisplayTexHandles.size() > 0)
	{
		iterator it( begin() );
		for( ; it != end(); ++it )
		{
			if (!it->IsVisible())
			{	
				continue;
			}

			// use the uv setting of the first visible map.
			Texmap* pSubMap = it->Mask();
			if(!pSubMap)
			{
				pSubMap = it->Texture();
			}
			if (m_DisplayTexHandles.size() > texmapIndex)
			{
				pSimpleMaterialExt->SetStageTexture(texmapIndex, m_DisplayTexHandles[texmapIndex]); 

				DisplayTextureHelperExt* pDisplayTextureHelperExt = dynamic_cast<DisplayTextureHelperExt*>(&cb);
				if (pDisplayTextureHelperExt != nullptr)
				{
					pDisplayTextureHelperExt->UpdateStageTextureMapInfo( t, texmapIndex, pSubMap);
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
			texmapIndex++;
		}
	}

	pSimpleMaterialExt->SetTextureStageCount(static_cast<int>(m_DisplayTexHandles.size()));
}

#pragma endregion

////////////////////////////////////////////////////////////////////////////////

Composite::PopLayerBeforeRestoreObject::PopLayerBeforeRestoreObject(Texture *owner, IParamBlock2 *paramblock, size_t layer_index) 
	: m_layer_index(layer_index), m_owner(owner), m_paramblock(paramblock)
{ }

void Composite::PopLayerBeforeRestoreObject::Redo() {
	Texture::iterator it = m_owner->LayerIterator ( m_layer_index );
	if (m_owner->ParamDialog())
		m_owner->ParamDialog()->DeleteRollup( &*it );

	m_owner->erase( it );
	// acquire new layer at index value
	it = m_owner->LayerIterator ( m_layer_index );

	// Update all indexes in the paramblock...
	for( ; it != m_owner->end(); ++it )
		it->Index( it->Index() - 1 );
}

void Composite::PopLayerBeforeRestoreObject::Restore(int isUndo) {
	if (isUndo) {
		// Create and push the new Layer on the list
		if (m_owner->ParamDialog()) {
			m_owner->ParamDialog()->EnableDisableInterface();
			Texture::iterator it = m_owner->LayerIterator ( m_layer_index );
			m_owner->ParamDialog()->CreateRollup( &*it );
		}

		m_owner->ValidateParamBlock();
	}
}

Composite::PopLayerAfterRestoreObject::PopLayerAfterRestoreObject(Texture *owner, IParamBlock2 *paramblock, size_t layer_index) 
	: m_layer_index(layer_index), m_owner(owner), m_paramblock(paramblock)
{ }

void Composite::PopLayerAfterRestoreObject::Restore(int isUndo) {
	if (isUndo) {
		Composite::Layer &new_layer( Composite::Layer( m_paramblock, m_layer_index ) );
		Texture::iterator it = m_owner->LayerIterator ( m_layer_index );
		m_owner->insert( it, new_layer );

		// Update all indexes in the paramblock...
		for( ; it != m_owner->end(); ++it )
			it->Index( it->Index() + 1 );
	}
}

void Composite::PopLayerAfterRestoreObject::Redo() {
	m_owner->ValidateParamBlock();

	if (m_owner->ParamDialog()) {
		// Re-wire the dialogs...
		m_owner->ParamDialog()->Rewire();
		m_owner->NotifyChanged();
	}
}

Composite::PushLayerRestoreObject::PushLayerRestoreObject(Texture *owner, IParamBlock2 *paramblock, bool setName) 
	: m_setName(setName), m_owner(owner), m_paramblock(paramblock)
{ }

void Composite::PushLayerRestoreObject::Restore(int isUndo) {
	if (isUndo) {
		Texture::iterator it = m_owner->LayerIterator ( m_owner->nb_layer()-1 );
		if (m_owner->ParamDialog())
			m_owner->ParamDialog()->DeleteRollup( &*it );

		m_owner->erase( it );
	}
}

void Composite::PushLayerRestoreObject::Redo() {
	// We create the Layer at the end...
	m_owner->push_back( Composite::Layer( m_paramblock, m_owner->nb_layer() ) );
	Composite::Layer &new_layer( *m_owner->LayerIterator( m_owner->nb_layer()-1 ) );

	if (m_setName)
		new_layer.Name( Layer::NameType( m_owner->DefaultLayerName().c_str() ) );

	// Create and push the new Layer on the list
	if (m_owner->ParamDialog()) {
		m_owner->ParamDialog()->EnableDisableInterface();
		m_owner->ParamDialog()->CreateRollup( &new_layer );
	}

	m_owner->ValidateParamBlock();
}

Composite::InsertLayerBeforeRestoreObject::InsertLayerBeforeRestoreObject(Texture *owner, IParamBlock2 *paramblock, size_t layer_index) 
	: m_layer_index(layer_index), m_owner(owner), m_paramblock(paramblock)
{ }

void Composite::InsertLayerBeforeRestoreObject::Redo() {
	Composite::Layer &new_layer( Composite::Layer( m_paramblock, m_layer_index ) );
	Texture::iterator it = m_owner->LayerIterator ( m_layer_index );
	m_owner->insert( it, new_layer );

	// Update all indexes in the paramblock...
	for( ; it != m_owner->end(); ++it )
		it->Index( it->Index() + 1 );
}

void Composite::InsertLayerBeforeRestoreObject::Restore(int isUndo) {
	if (isUndo) {
		Texture::iterator it = m_owner->LayerIterator ( m_layer_index );
		m_owner->erase( it );
		// acquire new layer at index value
		it = m_owner->LayerIterator ( m_layer_index );

		// Update all indexes in the paramblock...
		for( ; it != m_owner->end(); ++it )
			it->Index( it->Index() - 1 );

		m_owner->ValidateParamBlock();

		if (m_owner->ParamDialog()) {
			// Re-wire the dialogs...
			m_owner->ParamDialog()->Rewire();
			m_owner->NotifyChanged();
		}

	}
}

Composite::InsertLayerAfterRestoreObject::InsertLayerAfterRestoreObject(Texture *owner, IParamBlock2 *paramblock, size_t layer_index) 
	: m_layer_index(layer_index), m_owner(owner), m_paramblock(paramblock)
{ }

void Composite::InsertLayerAfterRestoreObject::Restore(int isUndo) {
	if (isUndo) {
		Texture::iterator it = m_owner->LayerIterator ( m_layer_index );
		if (m_owner->ParamDialog())
			m_owner->ParamDialog()->DeleteRollup( &*it );
	}
}

void Composite::InsertLayerAfterRestoreObject::Redo() {
	// Create and push the new Layer on the list
	if (m_owner->ParamDialog()) {
		m_owner->ParamDialog()->EnableDisableInterface();
		Texture::iterator it = m_owner->LayerIterator ( m_layer_index );
		m_owner->ParamDialog()->CreateRollup( &*it );
	}

	m_owner->ValidateParamBlock();
}

#pragma endregion

////////////////////////////////////////////////////////////////////////////////
