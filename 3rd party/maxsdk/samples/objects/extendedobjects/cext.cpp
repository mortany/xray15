/**********************************************************************
 *<
	FILE: cext.cpp - parameterized c-extrusion
	Created by Audrey Peterson

 *>	Copyright (c) 1996, All Rights Reserved.
 **********************************************************************/

#include "solids.h"
#include "iparamm2.h"
#include "Simpobj.h"
#include "macroRec.h"
#include "RealWorldMapUtils.h"
#include <ReferenceSaveManager.h>
#include <Util/StaticAssert.h>

class CExtObject : public SimpleObject2, public RealWorldMapSizeInterface {
	friend class CExtWidthDlgProc;
	friend class CExtTypeInDlgProc;
	friend class CExtObjCreateCallBack;
	friend class CExtTopBotAccessor;
	friend class CExtSideAccessor;
public:
	// Class vars	
	static IObjParam *ip;
	static bool typeinCreate;

	CExtObject(BOOL loading);

	// From Object
	int CanConvertToType(Class_ID obtype) override;
	Object* ConvertToType(TimeValue t, Class_ID obtype) override;

	// From BaseObject
	CreateMouseCallBack* GetCreateMouseCallBack() override;
	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev) override;
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next) override;
	const TCHAR *GetObjectName() override { return GetString(IDS_RB_CEXT); }
	BOOL HasUVW() override;
	void SetGenUVW(BOOL sw) override;

	// Animatable methods		
	void DeleteThis() override { delete this; }
	Class_ID ClassID() override { return CEXT_CLASS_ID; }

	// From ref
	RefTargetHandle Clone(RemapDir& remap) override;
	bool SpecifySaveReferences(ReferenceSaveManager& referenceSaveManager) override;
	IOResult Load(ILoad *iload) override;
	IOResult Save(ISave* isave) override;

	// From SimpleObjectBase
	void BuildMesh(TimeValue t) override;
	BOOL OKtoDisplay(TimeValue t) override;
	void InvalidateUI() override;

	// Get/Set the UsePhyicalScaleUVs flag.
	BOOL GetUsePhysicalScaleUVs() override;
	void SetUsePhysicalScaleUVs(BOOL flag) override;
	void UpdateUI();

	//From FPMixinInterface
	BaseInterface* GetInterface(Interface_ID id) override
	{
		if (id == RWS_INTERFACE)
			return this;

		BaseInterface* pInterface = FPMixinInterface::GetInterface(id);
		if (pInterface != NULL)
		{
			return pInterface;
		}
		// return the GetInterface() of its super class
		return SimpleObject2::GetInterface(id);
	}
private:
	class CExtPLC;
};

#define MIN_SEGMENTS	1
#define MAX_SEGMENTS	200
#define DEF_SEGMENTS 	1
#define MIN_HEIGHT		float(-1.0E30)
#define MAX_HEIGHT		float( 1.0E30)
#define MIN_LENGTH		float(-1.0E30)
#define MAX_LENGTH		float( 1.0E30)
#define MIN_WIDTH		float(0.1)
#define MAX_WIDTH		float(1.0E30)

//--- ClassDescriptor and class vars ---------------------------------

// in solids.cpp  - The dll instance handle
extern HINSTANCE hInstance;

// class variables for C-Ext class.
IObjParam *CExtObject::ip = NULL;
bool CExtObject::typeinCreate = false;

#define PBLOCK_REF_NO  0

static BOOL sInterfaceAdded = FALSE;

class CExtClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE)
	{
		if (!sInterfaceAdded) {
			AddInterface(&gRealWorldMapSizeDesc);
			sInterfaceAdded = TRUE;
		}
		return new CExtObject(loading);
	}
	const TCHAR *	ClassName() override { return GetString(IDS_AP_CEXT_CLASS); }
	SClass_ID		SuperClassID() override { return GEOMOBJECT_CLASS_ID; }
	Class_ID		ClassID() override { return CEXT_CLASS_ID; }
	const TCHAR* 	Category() override { return GetString(IDS_RB_EXTENDED); }
	const TCHAR* InternalName() { return _T("C_Ext"); } // returns fixed parsable name (scripter-visible name)
	HINSTANCE  HInstance() { return hInstance; }   // returns owning module handle
};

static CExtClassDesc CExtDesc;

ClassDesc* GetCExtDesc() { return &CExtDesc; }

// ParamBlockDesc2 IDs
enum paramblockdesc2_ids { cext_creation_type, cext_type_in, cext_params, };
enum cext_creation_type_param_ids { cext_create_meth, };
enum cext_type_in_param_ids {
	cext_ti_pos, cext_ti_toplength, cext_ti_sidelength, cext_ti_botlength,
	cext_ti_topwidth, cext_ti_sidewidth, cext_ti_botwidth, cext_ti_height,
};
enum cext_param_param_ids {
	cext_toplength = CEXT_TOPLENGTH, cext_sidelength = CEXT_SIDELENGTH, cext_botlength = CEXT_BOTLENGTH,
	cext_topwidth = CEXT_TOPWIDTH, cext_sidewidth = CEXT_SIDEWIDTH, cext_botwidth = CEXT_BOTWIDTH,
	cext_height = CEXT_HEIGHT, cext_tsegs = CEXT_TSEGS, cext_ssegs = CEXT_SSEGS,
	cext_bsegs = CEXT_BSEGS, cext_wsegs = CEXT_WSEGS, cext_hsegs = CEXT_HSEGS,
	cext_mapping = CEXT_GENUVS, cext_centercreate = CEXT_CENTERCREATE,
};

namespace
{
	MaxSDK::Util::StaticAssert< (cext_params == CEXT_PARAMBLOCK_ID) > validator;
}

void FixCExtTopBotWidth(IParamBlock2 *pblock2, TimeValue t, ParamID sidelenId, ParamID width1Id, ParamID width2Id, ParamID id, PB2Value &v)
{
	float sidelen, width1, width2;

	pblock2->GetValue(sidelenId, t, sidelen, FOREVER);
	pblock2->GetValue(width1Id, t, width1, FOREVER);
	pblock2->GetValue(width2Id, t, width2, FOREVER);
	if (sidelen == 0.0f && id == width2Id) // if width is set before length
	{
		sidelen = width1 + width2;
		pblock2->SetValue(sidelenId, t, sidelen);
	}

	// Sum of top and bot width should be lower than side length
	float fmax = (float)fabs(sidelen) - width1;
	fmax = fmax > MIN_WIDTH ? fmax : MIN_WIDTH;
	IParamMap2 * pParamMap = pblock2->GetMap(width2Id);
	if (pParamMap)
		pParamMap->SetRange(width2Id, MIN_WIDTH, fmax);
	if (width2 > fmax) {
		if (id == width2Id)
			v.f = fmax;
		else
			pblock2->SetValue(width2Id, t, fmax);
	}
}

void FixCExtSideWidth(IParamBlock2 *pblock2, TimeValue t, ParamID sidewidthId, ParamID toplenId, ParamID botlenId, ParamID id, PB2Value &v)
{
	float toplen, botlen, sidewidth;

	pblock2->GetValue(sidewidthId, t, sidewidth, FOREVER);
	pblock2->GetValue(toplenId, t, toplen, FOREVER);
	pblock2->GetValue(botlenId, t, botlen, FOREVER);
	if (toplen == 0.0f && botlen == 0.0f && id == sidewidthId) // if width is set before the lengths
	{
		toplen = sidewidth;
		botlen = sidewidth;
		pblock2->SetValue(toplenId, t, toplen);
		pblock2->SetValue(botlenId, t, botlen);
	}
	// Upper limit of side width should be min(toplen, botlen)
	float fmax = (float)fabs(toplen) > (float)fabs(botlen) ? (float)fabs(botlen) : (float)fabs(toplen);
	fmax = fmax > MIN_WIDTH ? fmax : MIN_WIDTH;
	IParamMap2 * pParamMap = pblock2->GetMap(sidewidthId);
	if (pParamMap)
		pParamMap->SetRange(sidewidthId, MIN_WIDTH, fmax);
	if (sidewidth > fmax) {
		if (id == sidewidthId)
			v.f = fmax;
		else
			pblock2->SetValue(sidewidthId, t, fmax);
	}
}

void InitCExtTopBotWidth(IParamBlock2 *pblock2, TimeValue t, ParamID sidelenId, ParamID topwidthId, ParamID botwidthId)
{
	float sidelen, topwidth, botwidth;

	pblock2->GetValue(sidelenId, t, sidelen, FOREVER);
	pblock2->GetValue(topwidthId, t, topwidth, FOREVER);
	pblock2->GetValue(botwidthId, t, botwidth, FOREVER);
	// Sum of top and bot width should be lower than side length
	float ftopmax = (float)fabs(sidelen) - botwidth;
	float fbotmax = (float)fabs(sidelen) - topwidth;

	IParamMap2 * pParamMap = pblock2->GetMap(botwidthId);
	if (pParamMap)
		pParamMap->SetRange(botwidthId, MIN_WIDTH, fbotmax);
	pParamMap = pblock2->GetMap(topwidthId);
	if (pParamMap)
		pParamMap->SetRange(topwidthId, MIN_WIDTH, ftopmax);
}

void InitCExtSideWidth(IParamBlock2 *pblock2, TimeValue t, ParamID sidewidthId, ParamID toplenId, ParamID botlenId)
{
	float toplen, botlen, sidewidth;

	pblock2->GetValue(sidewidthId, t, sidewidth, FOREVER);
	pblock2->GetValue(toplenId, t, toplen, FOREVER);
	pblock2->GetValue(botlenId, t, botlen, FOREVER);
	// Upper limit of side width should be min(toplen, botlen)
	float fmax = (float)fabs(toplen) > (float)fabs(botlen) ? (float)fabs(botlen) : (float)fabs(toplen);
	IParamMap2 * pParamMap = pblock2->GetMap(sidewidthId);
	if (pParamMap)
		pParamMap->SetRange(sidewidthId, MIN_WIDTH, fmax);
}

class CExtTopBotClassAccessor : public PBAccessor
{
	bool m_disable; // for preventing re-entry
public:
	CExtTopBotClassAccessor() : m_disable(false) {}
	virtual void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t) override
	{
		if (m_disable)
			return;
		if (theHold.RestoreOrRedoing())
			return;
		IParamBlock2* pblock2 = CExtDesc.GetParamBlockDescByID(cext_type_in)->class_params;
		DbgAssert(pblock2);
		if (pblock2 == nullptr)
			return;
		m_disable = true;
		switch (id)
		{
		case cext_ti_sidelength:
		case cext_ti_botwidth:
		case cext_ti_topwidth:
			FixCExtTopBotWidth(pblock2, t, cext_ti_sidelength, cext_ti_topwidth, cext_ti_botwidth, id, v);
			FixCExtTopBotWidth(pblock2, t, cext_ti_sidelength, cext_ti_botwidth, cext_ti_topwidth, id, v);
			break;
		default:
			break;
		}
		m_disable = false;
	}
};
static CExtTopBotClassAccessor cExtTopBotClass_Accessor;


class CExtTopBotAccessor : public PBAccessor
{
	bool m_disable; // for preventing re-entry
public:
	CExtTopBotAccessor() : m_disable(false) {}
	virtual void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t) override
	{
		if (m_disable)
			return;
		if (theHold.RestoreOrRedoing())
			return;
		IParamBlock2* pblock2 = ((CExtObject*)owner)->pblock2;
		DbgAssert(pblock2);
		if (pblock2 == nullptr)
			return;
		m_disable = true;
		switch (id)
		{
		case cext_sidelength:
		case cext_botwidth:
		case cext_topwidth:
			FixCExtTopBotWidth(pblock2, t, cext_sidelength, cext_topwidth, cext_botwidth, id, v);
			FixCExtTopBotWidth(pblock2, t, cext_sidelength, cext_botwidth, cext_topwidth, id, v);
			break;
		default:
			break;
		}
		m_disable = false;
	}
};
static CExtTopBotAccessor cExtTopBot_Accessor;

class CExtSideClassAccessor : public PBAccessor
{
	bool m_disable; // for preventing re-entry
public:
	CExtSideClassAccessor() : m_disable(false) {}
	virtual void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t) override
	{
		if (m_disable)
			return;
		if (theHold.RestoreOrRedoing())
			return;
		IParamBlock2* pblock2 = CExtDesc.GetParamBlockDescByID(cext_type_in)->class_params;
		DbgAssert(pblock2);
		if (pblock2 == nullptr)
			return;
		m_disable = true;
		switch (id)
		{
		case cext_ti_sidewidth:
		case cext_ti_botlength:
		case cext_ti_toplength:
			FixCExtSideWidth(pblock2, t, cext_ti_sidewidth, cext_ti_toplength, cext_ti_botlength, id, v);
			break;
		default:
			break;
		}
		m_disable = false;
	}
};
static CExtSideClassAccessor cExtSideClass_Accessor;

class CExtSideAccessor : public PBAccessor
{
	bool m_disable; // for preventing re-entry
public:
	CExtSideAccessor() : m_disable(false) {}
	virtual void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t) override
	{
		if (m_disable)
			return;
		if (theHold.RestoreOrRedoing())
			return;
		IParamBlock2* pblock2 = ((CExtObject*)owner)->pblock2;
		DbgAssert(pblock2);
		if (pblock2 == nullptr)
			return;
		m_disable = true;
		switch (id)
		{
		case cext_sidewidth:
		case cext_botlength:
		case cext_toplength:
			FixCExtSideWidth(pblock2, t, cext_sidewidth, cext_toplength, cext_botlength, id, v);
			break;
		default:
			break;
		}
		m_disable = false;
	}
};
static CExtSideAccessor cExtSide_Accessor;

namespace
{
	class MapCoords_Accessor : public PBAccessor
	{
		void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
		{
			if (owner) ((CExtObject*)owner)->UpdateUI();
		}
	};
	static MapCoords_Accessor mapCoords_Accessor;
}

// class creation type block
static ParamBlockDesc2 cext_crtype_blk(cext_creation_type, _T("C_ExtCreationType"), 0, &CExtDesc, P_CLASS_PARAMS + P_AUTO_UI,
	//rollout
	IDD_UEXTRUSIONS1, IDS_RB_CREATE_DIALOG, BEGIN_EDIT_CREATE, 0, NULL,
	// params
	cext_create_meth, _T("typeinCreationMethod"), TYPE_INT, 0, IDS_RB_CREATE_DIALOG,
	p_default, 0,
	p_range, 0, 1,
	p_ui, TYPE_RADIO, 2, IDC_UEXTR_CORNER, IDC_UEXTR_CENTER,
	p_end,
	p_end
	);

// class type-in block
static ParamBlockDesc2 cext_typein_blk(cext_type_in, _T("C_ExtTypeIn"), 0, &CExtDesc, P_CLASS_PARAMS + P_AUTO_UI,
	//rollout
	IDD_C_EXTRUSION2, IDS_RB_KEYBOARDENTRY, BEGIN_EDIT_CREATE, APPENDROLL_CLOSED, NULL,
	// params
	cext_ti_pos, _T("typeInPos"), TYPE_POINT3, 0, IDS_RB_TYPEIN_POS,
	p_default, Point3(0, 0, 0),
	p_range, float(-1.0E30), float(1.0E30),
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CEXT_POSX, IDC_CEXT_POSXSPIN, IDC_CEXT_POSY, IDC_CEXT_POSYSPIN, IDC_CEXT_POSZ, IDC_CEXT_POSZSPIN, SPIN_AUTOSCALE,
	p_end,
	cext_ti_toplength, _T("typeInBackLength"), TYPE_FLOAT, 0, IDS_AP_BACKLENGTH,
	p_default, 0.0f,
	p_range, MIN_LENGTH, MAX_LENGTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CEXT_TOPLEN, IDC_CEXT_TOPLENSPIN, SPIN_AUTOSCALE,
	p_accessor, &cExtSideClass_Accessor,
	p_end,
	cext_ti_sidelength, _T("typeInSideLength"), TYPE_FLOAT, 0, IDS_RB_SIDELENGTH,
	p_default, 0.0f,
	p_range, MIN_LENGTH, MAX_LENGTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CEXT_SIDELEN, IDC_CEXT_SIDELENSPIN, SPIN_AUTOSCALE,
	p_accessor, &cExtTopBotClass_Accessor,
	p_end,
	cext_ti_botlength, _T("typeInFrontLength"), TYPE_FLOAT, 0, IDS_AP_FRONTLENGTH,
	p_default, 0.0f,
	p_range, MIN_LENGTH, MAX_LENGTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CEXT_BOTLEN, IDC_CEXT_BOTLENSPIN, SPIN_AUTOSCALE,
	p_accessor, &cExtSideClass_Accessor,
	p_end,
	cext_ti_topwidth, _T("typeInBackWidth"), TYPE_FLOAT, 0, IDS_AP_BACKWIDTH,
	p_default, 0.0f,
	p_range, MIN_WIDTH, MAX_WIDTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CEXT_TOPWID, IDC_CEXT_TOPWIDSPIN, SPIN_AUTOSCALE,
	p_accessor, &cExtTopBotClass_Accessor,
	p_end,
	cext_ti_sidewidth, _T("typeInSideWidth"), TYPE_FLOAT, 0, IDS_RB_SIDEWIDTH,
	p_default, 0.0f,
	p_range, MIN_WIDTH, MAX_WIDTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CEXT_SIDEWID, IDC_CEXT_SIDEWIDSPIN, SPIN_AUTOSCALE,
	p_accessor, &cExtSideClass_Accessor,
	p_end,
	cext_ti_botwidth, _T("typeInFrontWidth"), TYPE_FLOAT, 0, IDS_AP_FRONTWIDTH,
	p_default, 0.0f,
	p_range, MIN_WIDTH, MAX_WIDTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CEXT_BOTWID, IDC_CEXT_BOTWIDSPIN, SPIN_AUTOSCALE,
	p_accessor, &cExtTopBotClass_Accessor,
	p_end,
	cext_ti_height, _T("typeInHeight"), TYPE_FLOAT, 0, IDS_RB_HEIGHT,
	p_default, 0.0f,
	p_range, MIN_HEIGHT, MAX_HEIGHT,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CEXT_HEIGHT, IDC_CEXT_HEIGHTSPIN, SPIN_AUTOSCALE,
	p_end,
	p_end
	);

// per instance cext block
static ParamBlockDesc2 cext_param_blk(cext_params, _T("C_ExtParameters"), 0, &CExtDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_REF_NO,
	//rollout
	IDD_C_EXTRUSION3, IDS_AP_PARAMETERS, 0, 0, NULL,
	// params
	cext_toplength, _T("Back Length"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_AP_BACKLENGTH,
	p_default, 0.0f,
	p_range, MIN_LENGTH, MAX_LENGTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CEXT_TOPLEN, IDC_CEXT_TOPLENSPIN, SPIN_AUTOSCALE,
	p_accessor, &cExtSide_Accessor,
	p_end,
	cext_sidelength, _T("Side Length"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_SIDELENGTH,
	p_default, 0.0f,
	p_range, MIN_LENGTH, MAX_LENGTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CEXT_SIDELEN, IDC_CEXT_SIDELENSPIN, SPIN_AUTOSCALE,
	p_accessor, &cExtTopBot_Accessor,
	p_end,
	cext_botlength, _T("Front Length"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_AP_FRONTLENGTH,
	p_default, 0.0f,
	p_range, MIN_LENGTH, MAX_LENGTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CEXT_BOTLEN, IDC_CEXT_BOTLENSPIN, SPIN_AUTOSCALE,
	p_accessor, &cExtSide_Accessor,
	p_end,
	cext_topwidth, _T("Back Width"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_AP_BACKWIDTH,
	p_default, 0.0f,
	p_range, MIN_WIDTH, MAX_WIDTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CEXT_TOPWID, IDC_CEXT_TOPWIDSPIN, SPIN_AUTOSCALE,
	p_accessor, &cExtTopBot_Accessor,
	p_end,
	cext_sidewidth, _T("Side Width"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_SIDEWIDTH,
	p_default, 0.0f,
	p_range, MIN_WIDTH, MAX_WIDTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CEXT_SIDEWID, IDC_CEXT_SIDEWIDSPIN, SPIN_AUTOSCALE,
	p_accessor, &cExtSide_Accessor,
	p_end,
	cext_botwidth, _T("Front Width"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_AP_FRONTWIDTH,
	p_default, 0.0f,
	p_range, MIN_WIDTH, MAX_WIDTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CEXT_BOTWID, IDC_CEXT_BOTWIDSPIN, SPIN_AUTOSCALE,
	p_accessor, &cExtTopBot_Accessor,
	p_end,
	cext_height, _T("height"), TYPE_WORLD, P_ANIMATABLE, IDS_RB_HEIGHT,
	p_default, 0.0f,
	p_range, MIN_HEIGHT, MAX_HEIGHT,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CEXT_HEIGHT, IDC_CEXT_HEIGHTSPIN, SPIN_AUTOSCALE,
	p_end,
	cext_tsegs, _T("Back Segments"), TYPE_INT, P_ANIMATABLE, IDS_AP_BACKSEGS,
	p_default, DEF_SEGMENTS,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_CEXT_TSEGS, IDC_CEXT_TSEGSPIN, 0.1f,
	p_end,
	cext_ssegs, _T("Side Segments"), TYPE_INT, P_ANIMATABLE, IDS_RB_SSEGS,
	p_default, DEF_SEGMENTS,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_CEXT_SSEGS, IDC_CEXT_SSEGSPIN, 0.1f,
	p_end,
	cext_bsegs, _T("Front Segments"), TYPE_INT, P_ANIMATABLE, IDS_AP_FRONTSEGS,
	p_default, DEF_SEGMENTS,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_CEXT_BSEGS, IDC_CEXT_BSEGSPIN, 0.1f,
	p_end,
	cext_wsegs, _T("Width Segments"), TYPE_INT, P_ANIMATABLE, IDS_RB_WSEGS,
	p_default, DEF_SEGMENTS,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_CEXT_WSEGS, IDC_CEXT_WSEGSSPIN, 0.1f,
	p_end,
	cext_hsegs, _T("Height Segments"), TYPE_INT, P_ANIMATABLE, IDS_RB_HSEGS,
	p_default, DEF_SEGMENTS,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_CEXT_HSEGS, IDC_CEXT_HSEGSSPIN, 0.1f,
	p_end,
	cext_mapping, _T("mapCoords"), TYPE_BOOL, P_RESET_DEFAULT, IDS_MXS_GENUVS,
	p_default, TRUE,
	p_ui, TYPE_SINGLECHECKBOX, IDC_GENTEXTURE,
	p_accessor, &mapCoords_Accessor,
	p_end,
	cext_centercreate, _T("centerCreate"), TYPE_BOOL, P_RESET_DEFAULT, 0,
	p_default, FALSE,
	p_end,
	p_end
	);


// variable type, NULL, animatable, number
ParamBlockDescID CExtdescVer0[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0, cext_toplength },
	{ TYPE_FLOAT, NULL, TRUE, 1, cext_sidelength },
	{ TYPE_FLOAT, NULL, TRUE, 2, cext_botlength },
	{ TYPE_FLOAT, NULL, TRUE, 3, cext_topwidth },
	{ TYPE_FLOAT, NULL, TRUE, 4, cext_sidewidth },
	{ TYPE_FLOAT, NULL, TRUE, 5, cext_botwidth },
	{ TYPE_FLOAT, NULL, TRUE, 6, cext_height },
	{ TYPE_INT, NULL, TRUE, 7, cext_tsegs },
	{ TYPE_INT, NULL, TRUE, 8, cext_ssegs },
	{ TYPE_INT, NULL, TRUE, 9, cext_bsegs },
	{ TYPE_INT, NULL, TRUE, 10, cext_wsegs },
	{ TYPE_INT, NULL, TRUE, 11, cext_hsegs },
	{ TYPE_INT, NULL, FALSE, 12, cext_mapping },
};

// Array of old versions
static ParamVersionDesc versions[] = {
	ParamVersionDesc(CExtdescVer0,13,0),
};
#define NUM_OLDVERSIONS	1

// ParamBlock data for SaveToPrevious support
#define PBLOCK_LENGTH	13
#define CURRENT_VERSION	0

//--- TypeInDlgProc --------------------------------
class CExtWidthDlgProc : public ParamMap2UserDlgProc {
public:
	CExtObject *ob;
	HWND mhWnd;
	CExtWidthDlgProc(CExtObject *o) { ob = o; }
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	void DeleteThis() override { delete this; }
	void UpdateUI();
	BOOL GetRWSState();
};

BOOL CExtWidthDlgProc::GetRWSState()
{
	BOOL check = IsDlgButtonChecked(mhWnd, IDC_REAL_WORLD_MAP_SIZE);
	return check;
}

void CExtWidthDlgProc::UpdateUI()
{
	if (ob == NULL) return;
	BOOL usePhysUVs = ob->GetUsePhysicalScaleUVs();
	CheckDlgButton(mhWnd, IDC_REAL_WORLD_MAP_SIZE, usePhysUVs);
	EnableWindow(GetDlgItem(mhWnd, IDC_REAL_WORLD_MAP_SIZE), ob->HasUVW());
}

INT_PTR CExtWidthDlgProc::DlgProc(
	TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG: {
		mhWnd = hWnd;
		InitCExtTopBotWidth(map->GetParamBlock(), t, cext_sidelength, cext_topwidth, cext_botwidth);
		InitCExtSideWidth(map->GetParamBlock(), t, cext_sidewidth, cext_toplength, cext_botlength);
		UpdateUI();
		break;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_GENTEXTURE:
			UpdateUI();
			break;

		case IDC_REAL_WORLD_MAP_SIZE: {
			BOOL check = IsDlgButtonChecked(hWnd, IDC_REAL_WORLD_MAP_SIZE);
			theHold.Begin();
			ob->SetUsePhysicalScaleUVs(check);
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			ob->ip->RedrawViews(ob->ip->GetTime());
			break;
		}
		}
	}
	return FALSE;
}

class CExtTypeInDlgProc : public ParamMap2UserDlgProc {
public:
	CExtObject *ob;

	CExtTypeInDlgProc(CExtObject *o) { ob = o; }
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	void DeleteThis() override { delete this; }
};

INT_PTR CExtTypeInDlgProc::DlgProc(
	TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG: {
		InitCExtTopBotWidth(map->GetParamBlock(), t, cext_ti_sidelength, cext_ti_topwidth, cext_ti_botwidth);
		InitCExtSideWidth(map->GetParamBlock(), t, cext_ti_sidewidth, cext_ti_toplength, cext_ti_botlength);
		break;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CEXT_CREATE: {
			if (cext_typein_blk.GetFloat(cext_ti_height) == 0.0f) return TRUE;

			// We only want to set the value if the object is 
			// not in the scene.
			if (ob->TestAFlag(A_OBJ_CREATING)) {
				ob->pblock2->SetValue(cext_height, 0, cext_typein_blk.GetFloat(cext_ti_height));
				ob->pblock2->SetValue(cext_toplength, 0, cext_typein_blk.GetFloat(cext_ti_toplength));
				ob->pblock2->SetValue(cext_sidelength, 0, cext_typein_blk.GetFloat(cext_ti_sidelength));
				ob->pblock2->SetValue(cext_botlength, 0, cext_typein_blk.GetFloat(cext_ti_botlength));
				ob->pblock2->SetValue(cext_topwidth, 0, cext_typein_blk.GetFloat(cext_ti_topwidth));
				ob->pblock2->SetValue(cext_sidewidth, 0, cext_typein_blk.GetFloat(cext_ti_sidewidth));
				ob->pblock2->SetValue(cext_botwidth, 0, cext_typein_blk.GetFloat(cext_ti_botwidth));
			}
			else
				CExtObject::typeinCreate = true;

			Matrix3 tm(1);
			tm.SetTrans(cext_typein_blk.GetPoint3(cext_ti_pos));
			ob->suspendSnap = FALSE;
			ob->ip->NonMouseCreate(tm);
			// NOTE that calling NonMouseCreate will cause this
			// object to be deleted. DO NOT DO ANYTHING BUT RETURN.
			return TRUE;
		}
		}
		break;
	}
	return FALSE;
}


//--- CExt methods -------------------------------

CExtObject::CExtObject(BOOL loading)
{

	CExtDesc.MakeAutoParamBlocks(this);

	if (!loading && !GetPhysicalScaleUVsDisabled())
		SetUsePhysicalScaleUVs(true);
}

bool CExtObject::SpecifySaveReferences(ReferenceSaveManager& referenceSaveManager)
{
	// if saving to previous version that used pb1 instead of pb2...
	DWORD saveVersion = GetSavingVersion();
	if (saveVersion != 0 && saveVersion <= MAX_RELEASE_R19)
	{
		ProcessPB2ToPB1SaveToPrevious(this, pblock2, PBLOCK_REF_NO, CExtdescVer0, PBLOCK_LENGTH, CURRENT_VERSION);
	}
	return __super::SpecifySaveReferences(referenceSaveManager);
}

void CExtObject::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	__super::BeginEditParams(ip, flags, prev);
	this->ip = ip;
	// If this has been freshly created by type-in, set creation values:
	if (CExtObject::typeinCreate)
	{
		pblock2->SetValue(cext_height, 0, cext_typein_blk.GetFloat(cext_ti_height));
		pblock2->SetValue(cext_toplength, 0, cext_typein_blk.GetFloat(cext_ti_toplength));
		pblock2->SetValue(cext_sidelength, 0, cext_typein_blk.GetFloat(cext_ti_sidelength));
		pblock2->SetValue(cext_botlength, 0, cext_typein_blk.GetFloat(cext_ti_botlength));
		pblock2->SetValue(cext_topwidth, 0, cext_typein_blk.GetFloat(cext_ti_topwidth));
		pblock2->SetValue(cext_sidewidth, 0, cext_typein_blk.GetFloat(cext_ti_sidewidth));
		pblock2->SetValue(cext_botwidth, 0, cext_typein_blk.GetFloat(cext_ti_botwidth));
		pblock2->SetValue(cext_centercreate, 0, cext_crtype_blk.GetInt(cext_create_meth));
		CExtObject::typeinCreate = false;
	}

	// throw up all the appropriate auto-rollouts
	CExtDesc.BeginEditParams(ip, this, flags, prev);
	// if in Create Panel, install a callback for the type in.
	if (flags & BEGIN_EDIT_CREATE)
		cext_typein_blk.SetUserDlgProc(new CExtTypeInDlgProc(this));
	// install a callback for the params.
	cext_param_blk.SetUserDlgProc(new CExtWidthDlgProc(this));
}

void CExtObject::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	__super::EndEditParams(ip, flags, next);
	this->ip = NULL;
	CExtDesc.EndEditParams(ip, this, flags, next);
}

void BuildCExtMesh(Mesh &mesh,
	int hsegs, int tsegs, int ssegs, int bsegs, int wsegs,
	float height, float toplen, float sidelen, float botlen,
	float topwidth, float sidewidth, float botwidth,
	int genUVs, BOOL create, BOOL usePhysUVs)
{
	int nf = 0;
	int nfaces, ntverts = 0;
	BOOL minush = height < 0.0f;
	if (minush) height = -height;
	BOOL minusx = (toplen < 0.0f), minusy = (sidelen < 0.0f);
	toplen = (float)fabs(toplen);
	botlen = (float)fabs(botlen);
	sidelen = (float)fabs(sidelen);

	// sides + top/bot
	int VertexPerLevel = 2 * (wsegs + tsegs + ssegs + bsegs);
	int topverts = (wsegs + 1)*(1 + tsegs + ssegs + bsegs);
	int nverts = 2 * topverts + (hsegs + 1)*VertexPerLevel;
	nfaces = hsegs * 4 * (wsegs + bsegs + tsegs + ssegs) + 4 * wsegs*(tsegs + ssegs + bsegs);

	mesh.setNumVerts(nverts);
	mesh.setNumFaces(nfaces);
	if (genUVs)
	{
		ntverts = nverts + hsegs + 1;
		mesh.setNumTVerts(ntverts);
		mesh.setNumTVFaces(nfaces);
	}
	else
	{
		mesh.setNumTVerts(0);
		mesh.setNumTVFaces(0);
	}
	Point3 p;
	p.x = p.y = p.z = 0.0f;
	float minx = (botlen > toplen ? 0.0f : botlen - toplen), maxx = botlen;
	float xlen = (botlen > toplen ? botlen : toplen);
	float xincr, xpos = 0.0f, yincr;
	float uvdist = 2.0f*(botlen + toplen + sidelen) - 2.0f*sidewidth;
	float ystart, xstart;
	float xtv = 0.0f, ytv = 0.0f, ypos = 0.0f, dx = sidewidth / wsegs, tdy = topwidth / wsegs, bdy = botwidth / wsegs;
	int i, j, nv = 0, fc = 0, dlevel = bsegs + ssegs + tsegs + 1, botv = nverts - topverts;
	int tlast, tendcount = ntverts - hsegs - 1, tnv = 0, bottv = tendcount - topverts;
	float mirrorFix = (usePhysUVs && (minusx^minusy)) ? -1.0f : 1.0f;
	tlast = tendcount;
	for (j = 0; j <= wsegs; j++)
	{
		xstart = 0.0f; xincr = (botlen - j*dx) / bsegs;
		yincr = botwidth / wsegs; ystart = j*yincr;
		for (i = 0; i <= bsegs; i++)
		{
			mesh.setVert(nv, xpos = xstart + i*xincr, ystart, height);
			mesh.setVert(botv, xpos, ystart, 0.0f);
			if (genUVs)
			{
				xtv = (xpos - minx) / (usePhysUVs ? 1.0 : xlen);
				mesh.setTVert(tnv, mirrorFix*xtv, ytv = ystart / (usePhysUVs ? 1.0 : sidelen), 0.0f);
				mesh.setTVert(bottv, mirrorFix*xtv, (usePhysUVs ? sidelen : 1.0f) - ytv, 0.0f);
			}
			nv++; botv++; bottv++; tnv++;
		}
		yincr = (sidelen - j*(tdy + bdy)) / ssegs; xpos = mesh.verts[nv - 1].x;
		for (i = 1; i <= ssegs; i++)
		{
			mesh.setVert(nv, xpos, ypos = ystart + i*yincr, height);
			mesh.setVert(botv, xpos, ypos, 0.0f);
			if (genUVs)
			{
				mesh.setTVert(tnv, mirrorFix*xtv, ytv = ypos / (usePhysUVs ? 1.0 : sidelen), 0.0f);
				mesh.setTVert(bottv, mirrorFix*xtv, (usePhysUVs ? sidelen : 1.0f) - ytv, 0.0f);
			}
			nv++; botv++; bottv++; tnv++;
		}
		xstart = xpos; xincr = (toplen - j*dx) / tsegs;
		for (i = 1; i <= tsegs; i++)
		{
			mesh.setVert(nv, xpos = xstart - i*xincr, ypos, height);
			mesh.setVert(botv, xpos, ypos, 0.0f);
			if (genUVs)
			{
				xtv = (xpos - minx) / (usePhysUVs ? 1.0 : xlen);
				mesh.setTVert(tnv, mirrorFix*xtv, ytv, 0.0f);
				mesh.setTVert(bottv, mirrorFix*xtv, (usePhysUVs ? xlen : 1.0f) - ytv, 0.0f);
			}
			nv++; botv++; bottv++; tnv++;
		}
	}
	xstart = 0.0f; xpos = 0.0f; ypos = 0.0f;
	float uval = 0.0f;
	int refnv = nv;
	xincr = botlen / bsegs;
	float heightScale = usePhysUVs ? height : 1.0f;
	float uScale = (usePhysUVs ? 1.0 : uvdist);
	for (i = 0; i <= bsegs; i++)
	{
		mesh.setVert(nv, xpos = xstart + i*xincr, 0.0f, height);
		if (genUVs) {
			xtv = (uval = xpos) / uScale;
			mesh.setTVert(tnv, mirrorFix*xtv, heightScale, 0.0f);
		}
		nv++; tnv++;
	}
	yincr = sidelen / ssegs; xpos = mesh.verts[nv - 1].x;
	for (i = 1; i <= ssegs; i++)
	{
		mesh.setVert(nv, xpos, ypos += yincr, height);
		if (genUVs) {
			mesh.setTVert(tnv, mirrorFix*((uval += yincr) / uScale), heightScale, 0.0f);
		}
		nv++; tnv++;
	}
	xincr = toplen / tsegs;
	for (i = 1; i <= tsegs; i++)
	{
		mesh.setVert(nv, xpos -= xincr, ypos, height);
		if (genUVs) mesh.setTVert(tnv, mirrorFix*((uval += xincr) / uScale), heightScale, 0.0f);
		nv++;; tnv++;
	}
	yincr = topwidth / wsegs;
	for (i = 1; i <= wsegs; i++)
	{
		mesh.setVert(nv, xpos, ypos -= yincr, height);
		if (genUVs) mesh.setTVert(tnv, mirrorFix*((uval += yincr) / uScale), heightScale, 0.0f);
		nv++;; tnv++;
	}
	xincr = (toplen - sidewidth) / tsegs;
	for (i = 1; i <= tsegs; i++)
	{
		mesh.setVert(nv, xpos += xincr, ypos, height);
		if (genUVs) mesh.setTVert(tnv, mirrorFix*((uval += xincr) / uScale), heightScale, 0.0f);
		nv++;; tnv++;
	}
	yincr = (sidelen - topwidth - botwidth) / ssegs;
	for (i = 1; i <= ssegs; i++)
	{
		mesh.setVert(nv, xpos, ypos -= yincr, height);
		if (genUVs) mesh.setTVert(tnv, mirrorFix*((uval += yincr) / uScale), heightScale, 0.0f);
		nv++;; tnv++;
	}
	xincr = (botlen - sidewidth) / bsegs;
	for (i = 1; i <= bsegs; i++)
	{
		mesh.setVert(nv, xpos -= xincr, ypos, height);
		if (genUVs) mesh.setTVert(tnv, mirrorFix*((uval += xincr) / uScale), heightScale, 0.0f);
		nv++;; tnv++;
	}
	yincr = botwidth / wsegs;
	for (i = 1; i < wsegs; i++)
	{
		mesh.setVert(nv, xpos, ypos -= yincr, height);
		if (genUVs) mesh.setTVert(tnv, mirrorFix*((uval += yincr) / uScale), heightScale, 0.0f);
		nv++; tnv++;
	}
	if (genUVs) mesh.setTVert(tendcount++, mirrorFix*(usePhysUVs ? uvdist : 1.0f), heightScale, 0.0f);
	float zval, hincr = height / hsegs, zv;
	for (j = 0; j < VertexPerLevel; j++)
	{
		zval = height;
		for (i = 1; i <= hsegs; i++)
		{
			zval -= hincr;
			mesh.setVert(refnv + VertexPerLevel*i, mesh.verts[refnv].x, mesh.verts[refnv].y, zval);
			if (genUVs)
			{
				mesh.setTVert(refnv + VertexPerLevel*i, mesh.tVerts[refnv].x, zv = zval / (usePhysUVs ? 1.0 : height), 0.0f);
				if (j == VertexPerLevel - 1) mesh.setTVert(tendcount++, mirrorFix*(usePhysUVs ? uvdist : 1.0f), zv, 0.0f);
			}
		}
		refnv++;
	}
	int base = 0, top = dlevel, alevel = dlevel - 1;
	for (i = 0; i < wsegs; i++)
	{
		for (j = 0; j < alevel; j++)
		{
			if (genUVs)
			{
				mesh.tvFace[fc].setTVerts(top, base, base + 1);
				mesh.tvFace[fc + 1].setTVerts(top, base + 1, top + 1);
			}
			AddFace(&mesh.faces[fc++], top, base, base + 1, 0, 1);
			AddFace(&mesh.faces[fc++], top, base + 1, top + 1, 1, 1);
			top++; base++;
		} top++; base++;
	}
	base = top + VertexPerLevel;
	tendcount = tlast;
	int b1, smgroup = 2, s0 = bsegs + 1, s1 = s0 + ssegs, s2 = s1 + tsegs, s3 = s2 + wsegs;
	int s4 = s3 + tsegs, s5 = s4 + ssegs, s6 = s5 + bsegs;
	for (i = 0; i < hsegs; i++)
	{
		for (j = 1; j <= VertexPerLevel; j++)
		{
			if (genUVs)
			{
				b1 = (j < VertexPerLevel ? base + 1 : tendcount + 1);
				mesh.tvFace[fc].setTVerts(top, base, b1);
				mesh.tvFace[fc + 1].setTVerts(top, b1, (j < VertexPerLevel ? top + 1 : tendcount++));
			}
			b1 = (j < VertexPerLevel ? base + 1 : base - VertexPerLevel + 1);
			smgroup = (j < s0 ? 2 : (j < s1 ? 4 : (j < s2 ? 2 : (j < s3 ? 4 : (j < s4 ? 2 : (j < s5 ? 4 : (j < s6 ? 2 : 4)))))));
			AddFace(&mesh.faces[fc++], top, base, b1, 0, smgroup);
			AddFace(&mesh.faces[fc++], top, b1, (j < VertexPerLevel ? top + 1 : top - VertexPerLevel + 1), 1, smgroup);
			top++; base++;
		}
	}
	top = base; base = top + alevel;
	base = top + dlevel;
	// Just for Jack
	for (i = 0; i < wsegs; i++)
	{
		for (j = 0; j < alevel; j++)
		{
			if (genUVs)
			{
				mesh.tvFace[fc].setTVerts(top, base, base + 1);
				mesh.tvFace[fc + 1].setTVerts(top, base + 1, top + 1);
			}
			AddFace(&mesh.faces[fc++], top, base, base + 1, 0, 1);
			AddFace(&mesh.faces[fc++], top, base + 1, top + 1, 1, 1);
			top++; base++;
		} top++; base++;
	}
	if (minusx || minusy || minush)
	{
		float centerx = (create ? toplen : 0), centery = (create ? sidelen : 0);
		for (i = 0; i < nverts; i++)
		{
			if (minusx) mesh.verts[i].x = -mesh.verts[i].x + centerx;
			if (minusy) mesh.verts[i].y = -mesh.verts[i].y + centery;
			if (minush) mesh.verts[i].z -= height;
		}
		DWORD hold;
		int tedge;
		if (minusx != minusy)
			for (i = 0; i < nfaces; i++)
			{
				hold = mesh.faces[i].v[0]; mesh.faces[i].v[0] = mesh.faces[i].v[2]; mesh.faces[i].v[2] = hold;
				tedge = mesh.faces[i].getEdgeVis(0); mesh.faces[i].setEdgeVis(0, mesh.faces[i].getEdgeVis(1));
				mesh.faces[i].setEdgeVis(1, tedge);
				if (genUVs)
				{
					hold = mesh.tvFace[i].t[0]; mesh.tvFace[i].t[0] = mesh.tvFace[i].t[2]; mesh.tvFace[i].t[2] = hold;
				}
			}
	}
	assert(fc == mesh.numFaces);
	//	assert(nv==mesh.numVerts); */
	mesh.InvalidateTopologyCache();
}

BOOL CExtObject::HasUVW() {
	BOOL genUVs;
	Interval v;
	pblock2->GetValue(cext_mapping, 0, genUVs, v);
	return genUVs;
}

void CExtObject::SetGenUVW(BOOL sw) {
	if (sw == HasUVW()) return;
	pblock2->SetValue(cext_mapping, 0, sw);
	UpdateUI();
}

void CExtObject::BuildMesh(TimeValue t)
{
	int hsegs, tsegs, ssegs, bsegs, wsegs;
	float height, toplen, sidelen, botlen, topwidth, sidewidth, botwidth;
	int genUVs;

	// Start the validity interval at forever and widdle it down.
	ivalid = FOREVER;

	pblock2->GetValue(cext_hsegs, t, hsegs, ivalid);
	pblock2->GetValue(cext_tsegs, t, tsegs, ivalid);
	pblock2->GetValue(cext_ssegs, t, ssegs, ivalid);
	pblock2->GetValue(cext_bsegs, t, bsegs, ivalid);
	pblock2->GetValue(cext_wsegs, t, wsegs, ivalid);
	pblock2->GetValue(cext_toplength, t, toplen, ivalid);
	pblock2->GetValue(cext_sidelength, t, sidelen, ivalid);
	pblock2->GetValue(cext_botlength, t, botlen, ivalid);
	pblock2->GetValue(cext_topwidth, t, topwidth, ivalid);
	pblock2->GetValue(cext_sidewidth, t, sidewidth, ivalid);
	pblock2->GetValue(cext_botwidth, t, botwidth, ivalid);
	pblock2->GetValue(cext_height, t, height, ivalid);
	pblock2->GetValue(cext_mapping, t, genUVs, ivalid);
	LimitValue(height, MIN_HEIGHT, MAX_HEIGHT);
	LimitValue(toplen, MIN_LENGTH, MAX_LENGTH);
	LimitValue(sidelen, MIN_LENGTH, MAX_LENGTH);
	LimitValue(botlen, MIN_LENGTH, MAX_LENGTH);
	LimitValue(topwidth, MIN_WIDTH, MAX_WIDTH);
	LimitValue(sidewidth, MIN_WIDTH, MAX_WIDTH);
	LimitValue(botwidth, MIN_WIDTH, MAX_WIDTH);
	LimitValue(hsegs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(tsegs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(ssegs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(bsegs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(wsegs, MIN_SEGMENTS, MAX_SEGMENTS);
	int createmeth = pblock2->GetInt(cext_centercreate, 0, FOREVER);
	BuildCExtMesh(mesh, hsegs, tsegs, ssegs, bsegs, wsegs, height,
		toplen, sidelen, botlen, topwidth, sidewidth, botwidth, genUVs, createmeth, GetUsePhysicalScaleUVs());
}

inline Point3 operator+(const PatchVert &pv, const Point3 &p)
{
	return p + pv.p;
}


Object* CExtObject::ConvertToType(TimeValue t, Class_ID obtype)
{
	return __super::ConvertToType(t, obtype);
}

int CExtObject::CanConvertToType(Class_ID obtype)
{
	if (obtype == triObjectClassID) {
		return 1;
	}
	else {
		return __super::CanConvertToType(obtype);
	}
}

class CExtObjCreateCallBack : public CreateMouseCallBack {
	CExtObject *ob;
	Point3 p[2], d;
	IPoint2 sp0, sp1, sp2;
	float xwid, l, hd, slen;
public:
	int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat);
	void SetObj(CExtObject *obj) { ob = obj; }
};

int CExtObjCreateCallBack::proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat)
{

	if (!vpt || !vpt->IsAlive())
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	DWORD snapdim = SNAP_IN_3D;

	if (msg == MOUSE_FREEMOVE)
	{
		vpt->SnapPreview(m, m, NULL, snapdim);
	}

	if (msg == MOUSE_POINT || msg == MOUSE_MOVE) {
		switch (point) {
		case 0:	ob->suspendSnap = TRUE;
			sp0 = m;
			p[0] = vpt->SnapPoint(m, m, NULL, snapdim);
			mat.SetTrans(p[0]); // Set Node's transform				
			ob->pblock2->SetValue(cext_botlength, 0, 0.01f);
			ob->pblock2->SetValue(cext_toplength, 0, 0.01f);
			ob->pblock2->SetValue(cext_sidelength, 0, 0.01f);
			ob->pblock2->SetValue(cext_centercreate, 0, cext_crtype_blk.GetInt(cext_create_meth));
			break;
		case 1:
			mat.IdentityMatrix();
			sp1 = m;
			p[1] = vpt->SnapPoint(m, m, NULL, snapdim);
			d = p[1] - p[0];
			if (flags&MOUSE_CTRL) {
				// Constrain to square base
				float len;
				if (fabs(d.x) > fabs(d.y)) len = d.x;
				else len = d.y;
				d.x = d.y = len;
			}
			if (!cext_crtype_blk.GetInt(cext_create_meth))
			{
				mat.SetTrans(p[0]);
				if (flags&MOUSE_CTRL) d.x = (d.y *= 2.0f);
			}
			else
			{
				mat.SetTrans(p[0] - Point3(fabs(d.x), fabs(d.y), fabs(d.z)));
				d = 2.0f*d;
			}
			float tmp;
			xwid = (tmp = (float)fabs(d.x));
			slen = (float)fabs(d.y);
			tmp *= 0.2f;
			ob->pblock2->SetValue(cext_botlength, 0, d.x);
			ob->pblock2->SetValue(cext_toplength, 0, d.x);
			ob->pblock2->SetValue(cext_sidelength, 0, d.y);
			ob->pblock2->SetValue(cext_botwidth, 0, tmp);
			ob->pblock2->SetValue(cext_topwidth, 0, tmp);
			ob->pblock2->SetValue(cext_sidewidth, 0, 0.2f*slen);

			if (msg == MOUSE_POINT && (Length(sp1 - sp0) < 3 ))
			{
				return CREATE_ABORT;
			}
			break;
		case 2:
		{
			sp2 = m;
			float h = vpt->SnapLength(vpt->GetCPDisp(p[1], Point3(0, 0, 1), sp1, m, TRUE));
			ob->pblock2->SetValue(cext_height, 0, h);

			if (msg == MOUSE_POINT) { if (Length(m - sp0) < 3) { return CREATE_ABORT; } }
			break;
		}
		case 3:
			float f = vpt->SnapLength(vpt->GetCPDisp(p[1], Point3(0, 1, 0), sp2, m));
			if (f < 0.0f) f = 0.0f;
			float fmax = slen / 2.0f;
			if (f > fmax) f = fmax;
			ob->pblock2->SetValue(cext_topwidth, 0, f);
			ob->pblock2->SetValue(cext_sidewidth, 0, (f > xwid ? xwid : f));
			ob->pblock2->SetValue(cext_botwidth, 0, f);

			if (msg == MOUSE_POINT)
			{
				ob->suspendSnap = FALSE;
				return CREATE_STOP;
			}
			break;
		}
	}
	else {
		if (msg == MOUSE_ABORT)
		{
			return CREATE_ABORT;
		}
	}
	return 1;
}

static CExtObjCreateCallBack cylCreateCB;

CreateMouseCallBack* CExtObject::GetCreateMouseCallBack()
{
	cylCreateCB.SetObj(this);
	return(&cylCreateCB);
}

BOOL CExtObject::OKtoDisplay(TimeValue t)
{
	float radius;
	pblock2->GetValue(cext_botlength, t, radius, FOREVER);
	if (radius == 0.0f) return FALSE;
	else return TRUE;
}


void CExtObject::InvalidateUI()
{
	cext_param_blk.InvalidateUI(pblock2->LastNotifyParamID());
}

RefTargetHandle CExtObject::Clone(RemapDir& remap)
{
	CExtObject* newob = new CExtObject(FALSE);
	newob->ReplaceReference(0, remap.CloneRef(pblock2));
	newob->ivalid.SetEmpty();
	BaseClone(this, newob, remap);
	newob->SetUsePhysicalScaleUVs(GetUsePhysicalScaleUVs());
	return(newob);
}

void CExtObject::UpdateUI()
{
	if (ip == NULL)
		return;
	CExtWidthDlgProc* dlg = static_cast<CExtWidthDlgProc*>(cext_param_blk.GetUserDlgProc());
	dlg->UpdateUI();
}

BOOL CExtObject::GetUsePhysicalScaleUVs()
{
	return ::GetUsePhysicalScaleUVs(this);
}


void CExtObject::SetUsePhysicalScaleUVs(BOOL flag)
{
	BOOL curState = GetUsePhysicalScaleUVs();
	if (curState == flag)
		return;
	if (theHold.Holding())
		theHold.Put(new RealWorldScaleRecord<CExtObject>(this, curState));
	::SetUsePhysicalScaleUVs(this, flag);
	if (pblock2 != NULL)
		pblock2->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	UpdateUI();
	macroRec->SetProperty(this, _T("realWorldMapSize"), mr_bool, flag);
}

#define CYTPE_CHUNK			0x0100

//------------------------------------------------------------------------------
// PostLoadCallback
//------------------------------------------------------------------------------
class CExtObject::CExtPLC : public PostLoadCallback
{
	friend class CExtObject;
public:
	CExtPLC(CExtObject& aCext) : mCext(aCext) { }
	void proc(ILoad *iload)
	{
		mCext.pblock2->SetValue(cext_centercreate, 0, mCreateType);
		delete this;
	}
protected:
	int			mCreateType;
private:
	CExtObject& mCext;
};

IOResult CExtObject::Load(ILoad *iload)
{
	ParamBlock2PLCB* plcb = new ParamBlock2PLCB(versions, NUM_OLDVERSIONS, &cext_param_blk, this, PBLOCK_REF_NO);
	iload->RegisterPostLoadCallback(plcb);
	ULONG nb;
	IOResult res = IO_OK;
	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case CYTPE_CHUNK:
		{
			CExtPLC* plcb2 = new CExtPLC(*this);
			res = iload->Read(&plcb2->mCreateType, sizeof(int), &nb);
			iload->RegisterPostLoadCallback(plcb2);
		}
		break;
		}
		iload->CloseChunk();
		if (res != IO_OK)  return res;
	}

	return IO_OK;
}

IOResult CExtObject::Save(ISave* isave)
{
	DWORD savingVersion = isave->SavingVersion();
	if (savingVersion == 0)
		savingVersion = MAX_RELEASE;
	if (isave->SavingVersion() <= MAX_RELEASE_R19)
	{
		ULONG nb;
		isave->BeginChunk(CYTPE_CHUNK);
		int createmeth = pblock2->GetInt(cext_centercreate, 0, FOREVER);
		isave->Write(&createmeth, sizeof(createmeth), &nb);
		isave->EndChunk();
	}
	return IO_OK;
}



