//
// Copyright [2015] Autodesk, Inc.  All rights reserved. 
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc. and
// are protected under applicable copyright and trade secret law.  They may
// not be disclosed to, copied or used by any third party without the prior
// written consent of Autodesk, Inc.
//

#include "ColorMap.h"

using namespace MaxSDK::Graphics;


class ColorMapClassDesc: public ClassDesc2
 {
	public:
	int						IsPublic() { return true;}
	void*					Create(BOOL loading = FALSE) { return new ColorMap(); }
	const TCHAR*			ClassName() { return GetString(IDS_CLASS_NAME); }
	SClass_ID				SuperClassID() { return TEXMAP_CLASS_ID; }
	Class_ID				ClassID() { return COLORMAP_CLASS_ID; }
	const TCHAR*			Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*			InternalName() { return _T("ColorMap"); }
	HINSTANCE				HInstance() { return hInstance;}
};

static ColorMapClassDesc ColorMapDesc;
ClassDesc2* GetColorMapDesc() { return &ColorMapDesc; }

enum
{
	colormap_solid_color,
	colormap_map,
	colormap_map_on,
	colormap_gamma,
	colormap_gain,
	colormap_reverse_gamma,
};


static ParamBlockDesc2 colormap_param_blk (gnormal_params, _T("params"),  0, &ColorMapDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_REF, 
	//rollout
	IDD_PARAM_PANEL, IDS_PARAMS, 0, 0, NULL,
	// params
		colormap_solid_color,	 _T("solidcolor"),	TYPE_FRGBA,	P_ANIMATABLE,	IDS_SOLID_COLOR,	
			p_default,		AColor(0.5f,0.5f,0.5f,1.0f), 
			p_ui,			TYPE_COLORSWATCH, IDC_SOLID_COLOR, 
		p_end,
		colormap_map,		_T("map"),TYPE_TEXMAP,P_OWNERS_REF,	IDS_MAP,
			p_refno,		1,
			p_subtexno,		0,		
			p_ui,			TYPE_TEXMAPBUTTON, IDC_MAP,
		p_end,
		colormap_map_on,	_T("mapEnabled"), TYPE_BOOL, 0,	IDS_MAP_ON,
			p_default,		TRUE,
			p_ui,			TYPE_SINGLECHECKBOX, IDC_MAP_ON,
		p_end,
		colormap_gamma,	_T("gamma"),TYPE_FLOAT,	P_ANIMATABLE,	IDS_GAMMA,
			p_default,		2.2f,
			p_range,		1.0f, 5.0f,
			p_ui, 			TYPE_SPINNER, EDITTYPE_FLOAT,  IDC_GAMMA_EDIT, IDC_GAMMA_SPIN, 0.1f,
		p_end,
		colormap_gain,	_T("gain"),   TYPE_FLOAT, P_ANIMATABLE,	IDS_GAIN,
			p_default,		1.0f,
			p_range,		0.0f, 100000.0f,
			p_ui, 			TYPE_SPINNER, EDITTYPE_FLOAT,  IDC_GAIN_EDIT, IDC_GAIN_SPIN, 0.1f,
		p_end,
		colormap_reverse_gamma,	_T("ReverseGamma"), TYPE_BOOL, P_ANIMATABLE,	IDS_REVERSE_GAMMA,
			p_default,		TRUE,
			p_ui,			TYPE_SINGLECHECKBOX, IDC_REVERSE_GAMMA,
		p_end,
	p_end
);



MaxSDK::Graphics::IShaderManager* ColorMap::GetShaderManager()
{
	if (mpShaderManager == NULL)
	{
		mpShaderManager = IShaderManagerCreator::GetInstance()->CreateShaderManager(
			IShaderManager::ShaderTypeAMG, 
			//_M("metasl_mip_gamma_gain"), //Temporary usage 
			_M("max_ColorMap"),
			_M(""), // Not needed
			this);
	}
	return mpShaderManager;
}

ColorMap::ColorMap()
{

	mpPblock = NULL;
	mpSubTex = NULL;
	texHandle = NULL;
	mpShaderManager = NULL;
	ColorMapDesc.MakeAutoParamBlocks(this);
	Init();
}

ColorMap::~ColorMap()
{
	DiscardTexHandle();
	IShaderManagerCreator::GetInstance()->DeleteShaderManager(mpShaderManager);
	mpShaderManager = NULL;
}

void ColorMap::Reset() 
{
	DeleteReference(1);
	Init();
	ivalid.SetEmpty();

}

void ColorMap::Init()
{
	ivalid.SetEmpty();
	mapValid.SetEmpty();
}

void ColorMap::InvalidateUI()
{
	colormap_param_blk.InvalidateUI(mpPblock->LastNotifyParamID());
}


//-----------------------------------------------------------------------------
//-- From Animatable

void ColorMap::GetClassName(TSTR& s)
{
	s = GetString(IDS_CLASS_NAME_IMP);
}
	 
Animatable* ColorMap::SubAnim(int i) 
{
	switch (i) 
	{
		case 0:		
			return mpPblock;

		default:	
			return mpSubTex;
	}
}

TSTR ColorMap::SubAnimName(int i) 
{
	switch (i) 
	{
		case 0:		
			return GetString(IDS_PARAMS);
		default:	
			return GetSubTexmapTVName(i);
	}
}



//-----------------------------------------------------------------------------
//-- From ReferenceMaker

#define MTL_HDR_CHUNK 0x4000

IOResult ColorMap::Save(ISave *isave) 
{
	IOResult res;
	isave->BeginChunk(MTL_HDR_CHUNK);
	res = MtlBase::Save(isave);
	if (res!=IO_OK) return res;
	isave->EndChunk();
	return IO_OK;
}

IOResult ColorMap::Load(ILoad *iload) 
{ 
	IOResult res;
	int id;
	while (IO_OK==(res=iload->OpenChunk())) {
		switch(id=iload->CurChunkID())  {
			case MTL_HDR_CHUNK:
				res = MtlBase::Load(iload);
				break;
			}
		iload->CloseChunk();
		if (res!=IO_OK) 
			return res;
		}
	return IO_OK;
}

RefTargetHandle ColorMap::GetReference(int i) 
{
	switch (i) 
	{
		case PBLOCK_REF:		
			return mpPblock;
			
		default:	
			return mpSubTex;
	}
}
void ColorMap::SetReference(int i, RefTargetHandle rtarg) 
{
	switch(i) 
	{
	case PBLOCK_REF:
		mpPblock = (IParamBlock2 *)rtarg; 
		break;
	default: 
		mpSubTex = (Texmap *)rtarg; 
		break;
	}
}

RefResult ColorMap::NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, 
   PartID& partID, RefMessage message, BOOL propagate ) 
{
	switch (message) {
	case REFMSG_CHANGE:
		ivalid.SetEmpty();
		mapValid.SetEmpty();
		if (hTarget==mpPblock)
		{
			ParamID changing_param = mpPblock->LastNotifyParamID();
			colormap_param_blk.InvalidateUI(changing_param);
			if (changing_param != -1)
			{
				DiscardTexHandle();
			}
		}
		break;
	}	
	return(REF_SUCCEED);
}

//-----------------------------------------------------------------------------
//-- From ReferenceTarget

RefTargetHandle ColorMap::Clone(RemapDir &remap) 
{
	ColorMap *mnew = new ColorMap();
	*((MtlBase*)mnew) = *((MtlBase*)this); // copy superclass stuff
    mnew->ReplaceReference(PBLOCK_REF,remap.CloneRef(mpPblock));
	mnew->ivalid.SetEmpty();
	mnew->mapValid.SetEmpty();
	mnew->mpSubTex = NULL;
	mnew->mMapOn = mMapOn;
	mnew->mReverseGamma = mReverseGamma;
	mnew->mSolidColor = mSolidColor;
	mnew->mGamma = mGamma;
	mnew->mGain = mGain;
	
	if(mpSubTex)
	{
		mnew->ReplaceReference(MAP_REF,remap.CloneRef(mpSubTex));
	}
	BaseClone(this, mnew, remap);
	return (RefTargetHandle)mnew;
}

//-----------------------------------------------------------------------------
//-- From MtlBase

ULONG ColorMap::LocalRequirements(int subMtlNum) {
	ULONG retval = 0;
	if(mpSubTex)
	{
		retval |= mpSubTex->LocalRequirements(subMtlNum);
	}
	return retval;
}

void ColorMap::LocalMappingsRequired(int subMtlNum, BitArray & mapreq, BitArray &bumpreq) 
{
	if(mpSubTex)
	{
		mpSubTex->LocalMappingsRequired(subMtlNum,mapreq,bumpreq);
	}
}

ParamDlg* ColorMap::CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp) 
{
	IAutoMParamDlg* masterDlg = ColorMapDesc.CreateParamDlgs(hwMtlEdit, imp, this);
	return masterDlg;	
}

void ColorMap::Update(TimeValue t, Interval& valid) 
{
	if (!ivalid.InInterval(t)) 
	{
		ivalid.SetInfinite();
		mpPblock->GetValue( colormap_solid_color, t, mSolidColor, ivalid );
		mpPblock->GetValue( colormap_map_on, t, mMapOn, ivalid );
		mpPblock->GetValue( colormap_gamma, t, mGamma, ivalid );
		mpPblock->GetValue( colormap_gain, t, mGain, ivalid);
		mpPblock->GetValue( colormap_reverse_gamma, t, mReverseGamma, ivalid);
		NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_DISPLAY_MATERIAL_CHANGE);
	}

	if (!mapValid.InInterval(t))
	{
		mapValid.SetInfinite();
		if (mpSubTex && mMapOn) 
		{
			mpSubTex->Update(t,mapValid);
		}
	}
	valid &= mapValid;
	valid &= ivalid;
}

void ColorMap::SetSubTexmap(int i, Texmap *m) 
{
	ReplaceReference(MAP_REF,m);
	colormap_param_blk.InvalidateUI(colormap_map);
	mapValid.SetEmpty();
}

TSTR ColorMap::GetSubTexmapSlotName(int i) 
{	
	return GetString(IDS_MAP);
}

DWORD_PTR ColorMap::GetActiveTexHandle(TimeValue t, TexHandleMaker& thmaker)
{
	if (texHandle) 
	{
		if (texHandleValid.InInterval(t))
		{
			return texHandle->GetHandle();
		}
		else
		{
			DiscardTexHandle();
		}
	}
	texHandle = thmaker.MakeHandle(GetVPDisplayDIB(t,thmaker,texHandleValid));
	return texHandle->GetHandle();
}

void ColorMap::ActivateTexDisplay(BOOL onoff)
{
	if (!onoff) 
	{
		DiscardTexHandle();
	}
}

void ColorMap::DiscardTexHandle() 
{
	if (texHandle) 
	{
		texHandle->DeleteThis();
		texHandle = NULL;
	}
}


BITMAPINFO* ColorMap::GetVPDisplayDIB(TimeValue t, TexHandleMaker& thmaker, Interval &valid, BOOL mono, BOOL forceW, BOOL forceH) 
{
	//! Hack for now to wor around the baking happening in realistic mode
	if(TestMtlFlag(MTL_HW_TEX_ENABLED))
	{
		BITMAPINFO * cache = NULL;
		//Use a cached bitmpainfo
		BitmapInfo bi;
		static MaxSDK::AssetManagement::AssetUser bitMapAssetUser;
		if (bitMapAssetUser.GetId() == MaxSDK::AssetManagement::kInvalidId)
			bitMapAssetUser = MaxSDK::AssetManagement::IAssetManager::GetInstance()->GetAsset(_T("colormapTemp"), MaxSDK::AssetManagement::kBitmapAsset);
		bi.SetAsset(bitMapAssetUser);
		bi.SetWidth(1);
		bi.SetHeight(1);
		bi.SetType(BMM_TRUE_32);
		Bitmap *bm = TheManager->Create(&bi);
		cache = thmaker.BitmapToDIB(bm,1,0,forceW,forceH);
		bm->DeleteThis();
		return cache;

	}
	Interval v;
	Update(t,v);
	BitmapInfo bi;
	static MaxSDK::AssetManagement::AssetUser bitMapAssetUser;
	if (bitMapAssetUser.GetId() == MaxSDK::AssetManagement::kInvalidId)
		bitMapAssetUser = MaxSDK::AssetManagement::IAssetManager::GetInstance()->GetAsset(_T("colormapTemp"), MaxSDK::AssetManagement::kBitmapAsset);
	bi.SetAsset(bitMapAssetUser);
	bi.SetWidth(thmaker.Size());
	bi.SetHeight(thmaker.Size());
	bi.SetType(BMM_TRUE_32);
	Bitmap *bm = TheManager->Create(&bi);
	GetCOREInterface()->RenderTexmap(this,bm);
	BITMAPINFO *bmi = thmaker.BitmapToDIB(bm,1,0,forceW,forceH);
	bm->DeleteThis();
	valid.SetInfinite();

	AColor ac;
	mpPblock->GetValue(colormap_solid_color,  t, ac, valid);
	float f;
	mpPblock->GetValue(colormap_gamma, t, f, valid);
	mpPblock->GetValue(colormap_gain, t, f, valid);
	BOOL b;
	mpPblock->GetValue(colormap_reverse_gamma,t,b,valid);

	return bmi;
}

//-----------------------------------------------------------------------------
//-- From Texmap

AColor ColorMap::EvalColor( ShadeContext& sc ) {
	AColor retval;
	float gamma = mGamma;

	retval = mSolidColor;

	if(mpSubTex && mMapOn)
	{
		retval = mpSubTex->EvalColor(sc);
	}

	if(gamma < 0.0f)
		gamma = 2.2f;

	if(!mReverseGamma)
	{
		gamma = 1.0f/gamma;
		retval.r *= mGain;
		retval.g *= mGain;
		retval.b *= mGain;
	}

	if(gamma != 1.0f)
	{
		retval.r = retval.r >= 0.0f ? pow(retval.r, gamma): -pow(-retval.r, gamma);
		retval.g = retval.g >= 0.0f ? pow(retval.g, gamma): -pow(-retval.g, gamma);
		retval.b = retval.b >= 0.0f ? pow(retval.b, gamma): -pow(-retval.b, gamma);
	}

	if(mReverseGamma && (mGain > 0.0f))
	{
		retval.r /= mGain;
		retval.g /= mGain;
		retval.b /= mGain;
	}
	return retval;
}

float ColorMap::EvalMono(ShadeContext& sc)
{
	return Intens(EvalColor(sc));
}

Point3 ColorMap::EvalNormalPerturb(ShadeContext& sc)
{
	//! Undefined currently
	if(mMapOn && mpSubTex)
	{
		return mpSubTex->EvalNormalPerturb(sc);
	}
	else
	{
		Point3 defaultNormal (0,0,0);
		return defaultNormal;
	}
}


BaseInterface* ColorMap::GetInterface(Interface_ID id) {

	if (ISHADER_MANAGER_INTERFACE_ID == id)
	{
		return GetShaderManager();
	}
	else if (IPARAMETER_TRANSLATOR_INTERFACE_ID == id)
	{ 
		return static_cast<IParameterTranslator*>(this);
	}
	else
	{
		return Texmap::GetInterface(id);
	}
}


bool ColorMap::GetParameterValue(const TimeValue t, const MCHAR* shaderParamName, void* value, 
									IParameterTranslator::ShaderParameterType shaderParamType)
{
	if(_tcscmp(shaderParamName,_M("gain"))==0)
	{
		*(float*)value = mGain;
	}
	else if(_tcscmp(shaderParamName,_M("gamma"))==0)
	{
		*(float*)value = mGamma;
	}	
	else if(_tcscmp(shaderParamName,_M("input"))==0)
	{
		//! AColor has the same layout as D3DCOLORVALUE
		*(AColor*)value = mSolidColor;
		
	}
	else if(_tcscmp(shaderParamName,_M("reverse"))==0)
	{
		*(BOOL*)value = mReverseGamma;
	}
	return TRUE;
}



bool ColorMap::GetShaderInputParameterName(SubMtlBaseType type, int subMtlBaseIndex, MSTR& shaderInputParamName)
{
	if(mMapOn && mpSubTex)
	{
		shaderInputParamName = _M("input");
	}
	
	return true;
}

bool ColorMap::OnPreParameterTranslate()
{
	Interval valid;
	Update(GetCOREInterface()->GetTime(),valid);
	return true;
}