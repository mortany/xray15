//
// Copyright [2015] Autodesk, Inc.  All rights reserved. 
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc. and
// are protected under applicable copyright and trade secret law.  They may
// not be disclosed to, copied or used by any third party without the prior
// written consent of Autodesk, Inc.
//


#include <bmmlib.h>
#include <iparamm2.h>
#include <macrorec.h>
#include <ifnpub.h>
#include <AssetManagement\iassetmanager.h>
#include <Graphics/IParameterTranslator.h>
#include <graphics/IShaderManager.h>
#include "resource.h"

#define NSUBTEX		1 
#define PBLOCK_REF	0
#define MAP_REF		1
#define NUM_REFS	2

#define COLORMAP_CLASS_ID	Class_ID(0x139f22c6, 0x13f6a914)


class ColorMap :public Texmap, public MaxSDK::Graphics::IParameterTranslator
{
	public:
		ColorMap();
		~ColorMap();		

		void	Reset();
		void	Init();
		void	InvalidateUI();

		//-- From Animatable
		void	DeleteThis(){ delete this; }
		void	GetClassName(TSTR& s);
		Class_ID	ClassID(){ return COLORMAP_CLASS_ID;}		
		SClass_ID	SuperClassID(){ return TEXMAP_CLASS_ID; }

		int		NumSubs(){ return NSUBTEX; }
		Animatable*		SubAnim(int i); 
		TSTR	SubAnimName(int i);
		int		SubNumToRefNum(int subNum) { return subNum; }


		int	NumParamBlocks(){ return 1; } // return number of ParamBlocks in this instance
		IParamBlock2*	GetParamBlock(int i)	{ return mpPblock; } // return i'th ParamBlock
		IParamBlock2*	GetParamBlockByID(BlockID id) { return (mpPblock->ID() == id) ? mpPblock : NULL; } // return id'd ParamBlock

		//-- From ReferenceMaker
		IOResult	Load(ILoad *iload);
		IOResult	Save(ISave *isave);

		int		NumRefs(){ return NUM_REFS; }
		RefTargetHandle		GetReference(int i);


		RefResult	NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);

		//-- From ReferenceTarget
		RefTargetHandle		Clone( RemapDir &remap );

		//-- From ISubMap
		int		NumSubTexmaps(){ return NSUBTEX; }
		Texmap*	GetSubTexmap(int i){ return mpSubTex; }
		void	SetSubTexmap(int i, Texmap *m);
		TSTR	GetSubTexmapSlotName(int i);

		//-- From MtlBase
		ULONG	LocalRequirements(int subMtlNum);
		void	LocalMappingsRequired(int subMtlNum, BitArray & mapreq, BitArray &bumpreq);

		void	Update(TimeValue t, Interval& valid);
		Interval	Validity(TimeValue t)	{Interval v = FOREVER; Update(t,v); return v;}
		ParamDlg*	CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp);

		void	DiscardTexHandle();
		BOOL	SupportTexDisplay()		{ return TRUE; }
		DWORD_PTR	GetActiveTexHandle(TimeValue t, TexHandleMaker& thmaker);
		void	ActivateTexDisplay(BOOL onoff);
		BITMAPINFO *GetVPDisplayDIB(TimeValue t, TexHandleMaker& thmaker, Interval &valid, BOOL mono=FALSE, BOOL forceW=0, BOOL forceH=0);

		//-- From Texmap
		AColor	EvalColor(ShadeContext& sc);
		float	EvalMono(ShadeContext& sc);
		Point3	EvalNormalPerturb(ShadeContext& sc);

		// -- from IParameterTranslator --
		virtual bool GetParameterValue(
			const TimeValue t, 
			const MCHAR* shaderParamName, 
			void* value, 
			MaxSDK::Graphics::IParameterTranslator::ShaderParameterType shaderParamType);
		virtual bool GetShaderInputParameterName(SubMtlBaseType type, int subMtlBaseIndex, MSTR& shaderInputParamName);
		virtual bool OnPreParameterTranslate();

		BaseInterface* ColorMap::GetInterface(Interface_ID id);

	private:
		virtual void SetReference(int i, RefTargetHandle rtarg);
		IParamBlock2	*mpPblock;	//ref 0
		Texmap		*mpSubTex;	//ref 1
		Interval	ivalid;
		Interval	mapValid;
		BOOL	mMapOn;
		BOOL	mReverseGamma;
		AColor	mSolidColor;
		float	mGamma;
		float	mGain;
		TexHandle*	texHandle;
		Interval	texHandleValid;
		MaxSDK::Graphics::IShaderManager* mpShaderManager;
		MaxSDK::Graphics::IShaderManager* GetShaderManager();
};

TCHAR *GetString(int id);
extern HINSTANCE hInstance;