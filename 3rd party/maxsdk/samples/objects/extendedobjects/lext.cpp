/**********************************************************************
 *<
	FILE: lext.cpp	   - builds l-extrusions
	CREATED BY:  Audrey Peterson

 *>	Copyright (c) 1996, All Rights Reserved.
 **********************************************************************/

#include "solids.h"
#include "iparamm2.h"
#include "Simpobj.h"
#include "macroRec.h"
#include "RealWorldMapUtils.h"
#include <ReferenceSaveManager.h>
#include <Util/StaticAssert.h>

class LextObject : public SimpleObject2, public RealWorldMapSizeInterface {
	friend class LextWidthDlgProc;
	friend class LextObjCreateCallBack;
	friend class LextTypeInDlgProc;
	friend class LExtWidthAccessor;
public:
	// Class vars	
	static IObjParam *ip;
	static bool typeinCreate;

	LextObject(BOOL loading);

	// From Object
	int CanConvertToType(Class_ID obtype) override;
	Object* ConvertToType(TimeValue t, Class_ID obtype) override;

	// From BaseObject
	CreateMouseCallBack* GetCreateMouseCallBack() override;
	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev) override;
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next) override;
	const TCHAR *GetObjectName() override { return GetString(IDS_RB_LEXT); }
	BOOL HasUVW() override;
	void SetGenUVW(BOOL sw) override;

	// Animatable methods		
	void DeleteThis() override { delete this; }
	Class_ID ClassID() override { return LEXT_CLASS_ID; }

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
	class LExtPLC;
};

#define MIN_SEGMENTS	1
#define MAX_SEGMENTS	200

#define MIN_LENGTH		float(-1.0E30)
#define MAX_LENGTH		float( 1.0E30)
#define MIN_HEIGHT		float(-1.0E30)
#define MAX_HEIGHT		float( 1.0E30)
#define MIN_WIDTH		float(0.1)
#define MAX_WIDTH		float(1.0E30)

#define DEF_SEGMENTS 	1

//--- ClassDescriptor and class vars ---------------------------------
// in solids.cpp  - The dll instance handle
extern HINSTANCE hInstance;

// class variables for L-Ext class.
IObjParam *LextObject::ip = NULL;
bool LextObject::typeinCreate = false;

#define PBLOCK_REF_NO  0

static BOOL sInterfaceAdded = FALSE;

class LextClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() override { return 1; }
	void *			Create(BOOL loading = FALSE) override
	{
		if (!sInterfaceAdded) {
			AddInterface(&gRealWorldMapSizeDesc);
			sInterfaceAdded = TRUE;
		}
		return new LextObject(loading);
	}
	const TCHAR *	ClassName() override { return GetString(IDS_AP_LEXT_CLASS); }
	SClass_ID		SuperClassID() override { return GEOMOBJECT_CLASS_ID; }
	Class_ID		ClassID() override { return LEXT_CLASS_ID; }
	const TCHAR* 	Category() override { return GetString(IDS_RB_EXTENDED); }
	const TCHAR* InternalName() { return _T("L_Ext"); } // returns fixed parsable name (scripter-visible name)
	HINSTANCE  HInstance() { return hInstance; }   // returns owning module handle
};

static LextClassDesc LextDesc;

ClassDesc* GetLextDesc() { return &LextDesc; }

// ParamBlockDesc2 IDs
enum paramblockdesc2_ids { lext_creation_type, lext_type_in, lext_params, };
enum lext_creation_type_param_ids { lext_create_meth, };
enum lext_type_in_param_ids { lext_ti_pos, lext_ti_slength, lext_ti_blength, lext_ti_swidth, lext_ti_bwidth, lext_ti_height, };
enum lext_param_param_ids {
	lext_slength = LEXT_SIDELENGTH, lext_blength = LEXT_BOTLENGTH, lext_swidth = LEXT_SIDEWIDTH,
	lext_bwidth = LEXT_BOTWIDTH, lext_height = LEXT_HEIGHT, lext_ssegs = LEXT_SSEGS,
	lext_bsegs = LEXT_BSEGS, lext_wsegs = LEXT_WSEGS, lext_hsegs = LEXT_HSEGS,
	lext_mapping = LEXT_GENUVS, lext_centercreate = LEXT_CENTERCREATE,
};

namespace
{
	MaxSDK::Util::StaticAssert< (lext_params == LEXT_PARAMBLOCK_ID) > validator;
}

void FixLExtWidth(IParamBlock2 *pblock2, TimeValue t, ParamID lengthId, ParamID widthId, ParamID id, PB2Value &v)
{
	float length, width;

	pblock2->GetValue(widthId, t, width, FOREVER);
	pblock2->GetValue(lengthId, t, length, FOREVER);

	if (length == 0.0f && id == widthId) // if width is set before length
	{
		length = width;
		pblock2->SetValue(lengthId, t, length);
	}

	// Width upper limit is the adjencent the length
	float fmax = (float)fabs(length) > MIN_WIDTH ? (float)fabs(length) : MIN_WIDTH;
	IParamMap2 * pParamMap = pblock2->GetMap(widthId);
	if (pParamMap)
		pParamMap->SetRange(widthId, MIN_WIDTH, fmax);
	if (width > fmax)
	{
		if (id == widthId)
			v.f = fmax;
		else if (id == lengthId)
			pblock2->SetValue(widthId, t, fmax);
	}
}

void InitLExtWidth(IParamBlock2 *pblock2, TimeValue t, ParamID lengthId, ParamID widthId)
{
	float length, width;

	pblock2->GetValue(widthId, t, width, FOREVER);
	pblock2->GetValue(lengthId, t, length, FOREVER);
	// Width upper limit is the adjencent the length
	float fmax = (float)fabs(length) > MIN_WIDTH ? (float)fabs(length) : MIN_WIDTH;
	IParamMap2 * pParamMap = pblock2->GetMap(widthId);
	if (pParamMap)
		pParamMap->SetRange(widthId, MIN_WIDTH, fmax);
}

class LExtWidthClassAccessor : public PBAccessor
{
	bool m_disable; // for preventing re-entry
public:
	LExtWidthClassAccessor() : m_disable(false) {}
	virtual void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t) override
	{
		if (m_disable)
			return;
		if (theHold.RestoreOrRedoing())
			return;
		IParamBlock2* pblock2 = LextDesc.GetParamBlockDescByID(lext_type_in)->class_params;
		DbgAssert(pblock2);
		if (pblock2 == nullptr)
			return;
		m_disable = true;
		switch (id)
		{
		case lext_ti_slength:
		case lext_ti_bwidth:
			FixLExtWidth(pblock2, t, lext_ti_slength, lext_ti_bwidth, id, v);
			break;
		case lext_ti_blength:
		case lext_ti_swidth:
			FixLExtWidth(pblock2, t, lext_ti_blength, lext_ti_swidth, id, v);
			break;
		default:
			break;
		}
		m_disable = false;
	}
};
static LExtWidthClassAccessor lExtWidthClass_Accessor;


class LExtWidthAccessor : public PBAccessor
{
	bool m_disable; // for preventing re-entry
public:
	LExtWidthAccessor() : m_disable(false) {}
	virtual void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t) override
	{
		if (m_disable)
			return;
		if (theHold.RestoreOrRedoing())
			return;
		IParamBlock2* pblock2 = ((LextObject*)owner)->pblock2;
		DbgAssert(pblock2);
		if (pblock2 == nullptr)
			return;
		m_disable = true;
		switch (id)
		{
		case lext_slength:
		case lext_bwidth:
			FixLExtWidth(pblock2, t, lext_slength, lext_bwidth, id, v);
			break;
		case lext_blength:
		case lext_swidth:
			FixLExtWidth(pblock2, t, lext_blength, lext_swidth, id, v);
			break;
		default:
			break;
		}
		m_disable = false;
	}
};
static LExtWidthAccessor lExtWidth_Accessor;

namespace
{
	class MapCoords_Accessor : public PBAccessor
	{
		void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
		{
			if (owner) ((LextObject*)owner)->UpdateUI();
		}
	};
	static MapCoords_Accessor mapCoords_Accessor;
}

// class creation type block
static ParamBlockDesc2 lext_crtype_blk(lext_creation_type, _T("L_ExtCreationType"), 0, &LextDesc, P_CLASS_PARAMS + P_AUTO_UI,
	//rollout
	IDD_UEXTRUSIONS1, IDS_RB_CREATE_DIALOG, BEGIN_EDIT_CREATE, 0, NULL,
	// params
	lext_create_meth, _T("typeinCreationMethod"), TYPE_INT, 0, IDS_RB_CREATE_DIALOG,
	p_default, 0,
	p_range, 0, 1,
	p_ui, TYPE_RADIO, 2, IDC_UEXTR_CORNER, IDC_UEXTR_CENTER,
	p_end,
	p_end
);

// class type-in block
static ParamBlockDesc2 lext_typein_blk(lext_type_in, _T("L_ExtTypeIn"), 0, &LextDesc, P_CLASS_PARAMS + P_AUTO_UI,
	//rollout
	IDD_L_EXTRUSION2, IDS_RB_KEYBOARDENTRY, BEGIN_EDIT_CREATE, APPENDROLL_CLOSED, NULL,
	// params
	lext_ti_pos, _T("typeInPos"), TYPE_POINT3, 0, IDS_RB_TYPEIN_POS,
	p_default, Point3(0, 0, 0),
	p_range, float(-1.0E30), float(1.0E30),
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_LEXT_POSX, IDC_LEXT_POSXSPIN, IDC_LEXT_POSY, IDC_LEXT_POSYSPIN, IDC_LEXT_POSZ, IDC_LEXT_POSZSPIN, SPIN_AUTOSCALE,
	p_end,
	lext_ti_slength, _T("typeInSideLength"), TYPE_FLOAT, 0, IDS_RB_SIDELENGTH,
	p_default, 0.0f,
	p_range, MIN_LENGTH, MAX_LENGTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_LEXT_SIDELEN, IDC_LEXT_SIDELENSPIN, SPIN_AUTOSCALE,
	p_accessor, &lExtWidthClass_Accessor,
	p_end,
	lext_ti_blength, _T("typeInFrontLength"), TYPE_FLOAT, 0, IDS_AP_FRONTLENGTH,
	p_default, 0.0f,
	p_range, MIN_LENGTH, MAX_LENGTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_LEXT_BOTLEN, IDC_LEXT_BOTLENSPIN, SPIN_AUTOSCALE,
	p_accessor, &lExtWidthClass_Accessor,
	p_end,
	lext_ti_swidth, _T("typeInSideWidth"), TYPE_FLOAT, 0, IDS_RB_SIDEWIDTH,
	p_default, 0.0f,
	p_range, MIN_WIDTH, MAX_WIDTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_LEXT_SIDEWID, IDC_LEXT_SIDEWIDSPIN, SPIN_AUTOSCALE,
	p_accessor, &lExtWidthClass_Accessor,
	p_end,
	lext_ti_bwidth, _T("typeInFrontWidth"), TYPE_FLOAT, 0, IDS_AP_FRONTWIDTH,
	p_default, 0.0f,
	p_range, MIN_WIDTH, MAX_WIDTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_LEXT_BOTWID, IDC_LEXT_BOTWIDSPIN, SPIN_AUTOSCALE,
	p_accessor, &lExtWidthClass_Accessor,
	p_end,
	lext_ti_height, _T("typeInHeight"), TYPE_FLOAT, 0, IDS_RB_HEIGHT,
	p_default, 0.01f,
	p_range, MIN_HEIGHT, MAX_HEIGHT,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_LEXT_HEIGHT, IDC_LEXT_HEIGHTSPIN, SPIN_AUTOSCALE,
	p_end,
	p_end
);

// per instance lext block
static ParamBlockDesc2 lext_param_blk(lext_params, _T("L_ExtParameters"), 0, &LextDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_REF_NO,
	//rollout
	IDD_L_EXTRUSION3, IDS_AP_PARAMETERS, 0, 0, NULL,
	// params
	lext_slength, _T("Side Length"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_SIDELENGTH,
	p_default, 0.0f,
	p_range, MIN_LENGTH, MAX_LENGTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_LEXT_SIDELEN, IDC_LEXT_SIDELENSPIN, SPIN_AUTOSCALE,
	p_accessor, &lExtWidth_Accessor,
	p_end,
	lext_blength, _T("Front Length"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_AP_FRONTLENGTH,
	p_default, 0.0f,
	p_range, MIN_LENGTH, MAX_LENGTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_LEXT_BOTLEN, IDC_LEXT_BOTLENSPIN, SPIN_AUTOSCALE,
	p_accessor, &lExtWidth_Accessor,
	p_end,
	lext_swidth, _T("Side Width"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_SIDEWIDTH,
	p_default, 0.0f,
	p_range, MIN_WIDTH, MAX_WIDTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_LEXT_SIDEWID, IDC_LEXT_SIDEWIDSPIN, SPIN_AUTOSCALE,
	p_accessor, &lExtWidth_Accessor,
	p_end,
	lext_bwidth, _T("Front Width"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_AP_FRONTWIDTH,
	p_default, 0.0f,
	p_range, MIN_WIDTH, MAX_WIDTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_LEXT_BOTWID, IDC_LEXT_BOTWIDSPIN, SPIN_AUTOSCALE,
	p_accessor, &lExtWidth_Accessor,
	p_end,
	lext_height, _T("height"), TYPE_WORLD, P_ANIMATABLE, IDS_RB_HEIGHT,
	p_default, 0.0f,
	p_range, MIN_HEIGHT, MAX_HEIGHT,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_LEXT_HEIGHT, IDC_LEXT_HEIGHTSPIN, SPIN_AUTOSCALE,
	p_end,
	lext_ssegs, _T("Side Segments"), TYPE_INT, P_ANIMATABLE, IDS_RB_SSEGS,
	p_default, DEF_SEGMENTS,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_LEXT_SIDESEGS, IDC_LEXT_SIDESEGSSPIN, 0.1f,
	p_end,
	lext_bsegs, _T("Front Segments"), TYPE_INT, P_ANIMATABLE, IDS_AP_FRONTSEGS,
	p_default, DEF_SEGMENTS,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_LEXT_BOTSEGS, IDC_LEXT_BOTSEGSSPIN, 0.1f,
	p_end,
	lext_wsegs, _T("Width Segments"), TYPE_INT, P_ANIMATABLE, IDS_RB_WSEGS,
	p_default, DEF_SEGMENTS,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_LEXT_WIDTHSEGS, IDC_LEXT_WIDTHSEGSSPIN, 0.1f,
	p_end,
	lext_hsegs, _T("Height Segments"), TYPE_INT, P_ANIMATABLE, IDS_RB_HSEGS,
	p_default, DEF_SEGMENTS,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_LEXT_HGTSEGS, IDC_LEXT_HGTSEGSSPIN, 0.1f,
	p_end,
	lext_mapping, _T("mapCoords"), TYPE_BOOL, P_RESET_DEFAULT, IDS_MXS_GENUVS,
	p_default, TRUE,
	p_ui, TYPE_SINGLECHECKBOX, IDC_GENTEXTURE,
	p_accessor, &mapCoords_Accessor,
	p_end,
	lext_centercreate, _T("centerCreate"), TYPE_BOOL, P_RESET_DEFAULT, 0,
	p_default, FALSE,
	p_end,
	p_end
);


// variable type, NULL, animatable, number
ParamBlockDescID LextdescVer0[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0, lext_slength },
	{ TYPE_FLOAT, NULL, TRUE, 1, lext_blength },
	{ TYPE_FLOAT, NULL, TRUE, 2, lext_swidth },
	{ TYPE_FLOAT, NULL, TRUE, 3, lext_bwidth },
	{ TYPE_FLOAT, NULL, TRUE, 4, lext_height },
	{ TYPE_INT, NULL, TRUE, 5, lext_ssegs },
	{ TYPE_INT, NULL, TRUE, 6, lext_bsegs },
	{ TYPE_INT, NULL, TRUE, 7, lext_wsegs },
	{ TYPE_INT, NULL, TRUE, 8, lext_hsegs },
	{ TYPE_INT, NULL, FALSE, 9, lext_mapping },
};

// Array of old versions
static ParamVersionDesc versions[] = {
	ParamVersionDesc(LextdescVer0,10,0),
};
#define NUM_OLDVERSIONS	1

// ParamBlock data for SaveToPrevious support
#define PBLOCK_LENGTH	10
#define CURRENT_VERSION	0

class LextWidthDlgProc : public ParamMap2UserDlgProc {
public:
	LextObject *ob;
	HWND mhWnd;
	LextWidthDlgProc(LextObject *o) { ob = o; }
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	void DeleteThis() override { delete this; }
	void UpdateUI();
	BOOL GetRWSState();
};

BOOL LextWidthDlgProc::GetRWSState()
{
	BOOL check = IsDlgButtonChecked(mhWnd, IDC_REAL_WORLD_MAP_SIZE);
	return check;
}

void LextWidthDlgProc::UpdateUI()
{
	if (ob == NULL) return;
	BOOL usePhysUVs = ob->GetUsePhysicalScaleUVs();
	CheckDlgButton(mhWnd, IDC_REAL_WORLD_MAP_SIZE, usePhysUVs);
	EnableWindow(GetDlgItem(mhWnd, IDC_REAL_WORLD_MAP_SIZE), ob->HasUVW());
}

INT_PTR LextWidthDlgProc::DlgProc(
	TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG: {
		mhWnd = hWnd;
		InitLExtWidth(map->GetParamBlock(), t, lext_slength, lext_bwidth);
		InitLExtWidth(map->GetParamBlock(), t, lext_blength, lext_swidth);
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

//--- TypeInDlgProc --------------------------------
class LextTypeInDlgProc : public ParamMap2UserDlgProc {
public:
	LextObject *ob;

	LextTypeInDlgProc(LextObject *o) { ob = o; }
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	void DeleteThis() override { delete this; }
};

INT_PTR LextTypeInDlgProc::DlgProc(
	TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG: {
		InitLExtWidth(map->GetParamBlock(), t, lext_ti_slength, lext_ti_bwidth);
		InitLExtWidth(map->GetParamBlock(), t, lext_ti_blength, lext_ti_swidth);
		break;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_LEXT_CREATE: {
			if (lext_typein_blk.GetFloat(lext_ti_height) == 0.0) return TRUE;

			// We only want to set the value if the object is 
			// not in the scene.
			if (ob->TestAFlag(A_OBJ_CREATING)) {
				ob->pblock2->SetValue(lext_height, 0, lext_typein_blk.GetFloat(lext_ti_height));
				ob->pblock2->SetValue(lext_slength, 0, lext_typein_blk.GetFloat(lext_ti_slength));
				ob->pblock2->SetValue(lext_blength, 0, lext_typein_blk.GetFloat(lext_ti_blength));
				ob->pblock2->SetValue(lext_swidth, 0, lext_typein_blk.GetFloat(lext_ti_swidth));
				ob->pblock2->SetValue(lext_bwidth, 0, lext_typein_blk.GetFloat(lext_ti_bwidth));
			}
			else
				LextObject::typeinCreate = true;

			Matrix3 tm(1);
			tm.SetTrans(lext_typein_blk.GetPoint3(lext_ti_pos));
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

//--- Lext methods -------------------------------

LextObject::LextObject(BOOL loading)
{
	LextDesc.MakeAutoParamBlocks(this);
	if (!loading && !GetPhysicalScaleUVsDisabled())
		SetUsePhysicalScaleUVs(true);
}


bool LextObject::SpecifySaveReferences(ReferenceSaveManager& referenceSaveManager)
{
	// if saving to previous version that used pb1 instead of pb2...
	DWORD saveVersion = GetSavingVersion();
	if (saveVersion != 0 && saveVersion <= MAX_RELEASE_R19)
	{
		ProcessPB2ToPB1SaveToPrevious(this, pblock2, PBLOCK_REF_NO, LextdescVer0, PBLOCK_LENGTH, CURRENT_VERSION);
	}
	return __super::SpecifySaveReferences(referenceSaveManager);
}

#define CYTPE_CHUNK			0x0100
//------------------------------------------------------------------------------
// PostLoadCallback
//------------------------------------------------------------------------------
class LextObject::LExtPLC : public PostLoadCallback
{
	friend class LextObject;
public:
	LExtPLC(LextObject& aLext) : mLext(aLext) { }
	void proc(ILoad *iload)
	{
		mLext.pblock2->SetValue(lext_centercreate, 0, mCreateType);
		delete this;
	}
protected:
	int			mCreateType;
private:
	LextObject& mLext;
};

IOResult LextObject::Load(ILoad *iload)
{
	ParamBlock2PLCB* plcb = new ParamBlock2PLCB(versions, NUM_OLDVERSIONS, &lext_param_blk, this, PBLOCK_REF_NO);
	iload->RegisterPostLoadCallback(plcb);
	ULONG nb;
	IOResult res = IO_OK;
	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID())
		{
		case CYTPE_CHUNK:
		{
			LExtPLC* plcb2 = new LExtPLC(*this);
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

IOResult LextObject::Save(ISave* isave)
{
	DWORD savingVersion = isave->SavingVersion();
	if (savingVersion == 0)
		savingVersion = MAX_RELEASE;
	if (isave->SavingVersion() <= MAX_RELEASE_R19)
	{
		ULONG nb;
		isave->BeginChunk(CYTPE_CHUNK);
		int createmeth = pblock2->GetInt(lext_centercreate, 0, FOREVER);
		isave->Write(&createmeth, sizeof(createmeth), &nb);
		isave->EndChunk();
	}
	return IO_OK;
}


void LextObject::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	__super::BeginEditParams(ip, flags, prev);
	this->ip = ip;
	// If this has been freshly created by type-in, set creation values:
	if (LextObject::typeinCreate)
	{
		pblock2->SetValue(lext_slength, 0, lext_typein_blk.GetFloat(lext_ti_slength));
		pblock2->SetValue(lext_blength, 0, lext_typein_blk.GetFloat(lext_ti_blength));
		pblock2->SetValue(lext_swidth, 0, lext_typein_blk.GetFloat(lext_ti_swidth));
		pblock2->SetValue(lext_bwidth, 0, lext_typein_blk.GetFloat(lext_ti_bwidth));
		pblock2->SetValue(lext_height, 0, lext_typein_blk.GetFloat(lext_ti_height));
		pblock2->SetValue(lext_centercreate, 0, lext_crtype_blk.GetInt(lext_create_meth));
		LextObject::typeinCreate = false;
	}

	// throw up all the appropriate auto-rollouts
	LextDesc.BeginEditParams(ip, this, flags, prev);
	// if in Create Panel, install a callback for the type in.
	if (flags & BEGIN_EDIT_CREATE)
		lext_typein_blk.SetUserDlgProc(new LextTypeInDlgProc(this));
	// install a callback for the params.
	lext_param_blk.SetUserDlgProc(new LextWidthDlgProc(this));
}

void LextObject::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	__super::EndEditParams(ip, flags, next);
	this->ip = NULL;
	LextDesc.EndEditParams(ip, this, flags, next);
}

void BuildLextMesh(Mesh &mesh,
	int hsegs, int ssegs, int bsegs, int wsegs,
	float height, float sidelen, float botlen,
	float sidewidth, float botwidth, int genUVs, BOOL create, BOOL usePhysUVs)
{
	int nf = 0;
	int nfaces, ntverts = 0;
	// sides + top/bot
	BOOL minusx = (botlen < 0.0f), minusy = (sidelen < 0.0f), minush = (height < 0.0f);
	botlen = (float)fabs(botlen);
	sidelen = (float)fabs(sidelen);
	if (minush) height = -height;
	int VertexPerLevel = 2 * (wsegs + ssegs + bsegs);
	int topverts = (wsegs + 1)*(1 + ssegs + bsegs);
	int nverts = 2 * topverts + (hsegs + 1)*VertexPerLevel;
	nfaces = hsegs * 4 * (wsegs + bsegs + ssegs) + 4 * wsegs*(ssegs + bsegs);

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
	float minx = 0.0f, maxx = botlen;
	float xlen = botlen;
	float xincr, xpos = 0.0f, yincr;
	float uvdist = 2.0f*(botlen + sidelen);
	float ystart, xstart;
	float xtv = 0.0f, ytv, ypos, dx = sidewidth / wsegs, bdy = botwidth / wsegs;
	int i, j, nv = 0, fc = 0, dlevel = bsegs + ssegs + 1, botv = nverts - topverts;
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
		yincr = (sidelen - j*bdy) / ssegs; xpos = mesh.verts[nv - 1].x;
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
	}
	xstart = 0.0f; xpos = 0.0f; ypos = 0.0f;
	float uval = 0.0f;
	int refnv = nv;
	xincr = botlen / bsegs;
	float heightScale = usePhysUVs ? height : 1.0f;
	for (i = 0; i <= bsegs; i++)
	{
		mesh.setVert(nv, xpos = xstart + i*xincr, 0.0f, height);
		if (genUVs) {
			xtv = (uval = xpos) / (usePhysUVs ? 1.0 : uvdist);
			mesh.setTVert(tnv, mirrorFix*xtv, heightScale, 0.0f);
		}
		nv++; tnv++;
	}
	yincr = sidelen / ssegs; xpos = mesh.verts[nv - 1].x;
	for (i = 1; i <= ssegs; i++)
	{
		mesh.setVert(nv, xpos, ypos += yincr, height);
		if (genUVs) mesh.setTVert(tnv, mirrorFix*((uval += yincr) / (usePhysUVs ? 1.0 : uvdist)), heightScale, 0.0f);
		nv++; tnv++;
	}
	xincr = sidewidth / wsegs;
	for (i = 1; i <= wsegs; i++)
	{
		mesh.setVert(nv, xpos -= xincr, ypos, height);
		if (genUVs) mesh.setTVert(tnv, mirrorFix*((uval += xincr) / (usePhysUVs ? 1.0 : uvdist)), heightScale, 0.0f);
		nv++;; tnv++;
	}
	yincr = (sidelen - botwidth) / ssegs;
	for (i = 1; i <= ssegs; i++)
	{
		mesh.setVert(nv, xpos, ypos -= yincr, height);
		if (genUVs) mesh.setTVert(tnv, mirrorFix*((uval += yincr) / (usePhysUVs ? 1.0 : uvdist)), heightScale, 0.0f);
		nv++;; tnv++;
	}
	xincr = (botlen - sidewidth) / bsegs;
	for (i = 1; i <= bsegs; i++)
	{
		mesh.setVert(nv, xpos -= xincr, ypos, height);
		if (genUVs) mesh.setTVert(tnv, mirrorFix*((uval += xincr) / (usePhysUVs ? 1.0 : uvdist)), heightScale, 0.0f);
		nv++;; tnv++;
	}
	yincr = botwidth / wsegs;
	for (i = 1; i < wsegs; i++)
	{
		mesh.setVert(nv, xpos, ypos -= yincr, height);
		if (genUVs) mesh.setTVert(tnv, mirrorFix*((uval += yincr) / (usePhysUVs ? 1.0 : uvdist)), heightScale, 0.0f);
		nv++; tnv++;
	}
	if (genUVs) mesh.setTVert(tendcount++, mirrorFix*((usePhysUVs ? uvdist : 1.0f)), heightScale, 0.0f);
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
	int b1, smgroup = 2, s0 = bsegs + 1, s1 = s0 + ssegs, s2 = s1 + wsegs, s3 = s2 + ssegs, s4 = s3 + bsegs;
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
			smgroup = (j < s0 ? 2 : (j < s1 ? 4 : (j < s2 ? 2 : (j < s3 ? 4 : (j < s4 ? 2 : 4)))));
			AddFace(&mesh.faces[fc++], top, base, b1, 0, smgroup);
			AddFace(&mesh.faces[fc++], top, b1, (j < VertexPerLevel ? top + 1 : top - VertexPerLevel + 1), 1, smgroup);
			top++; base++;
		}
	}
	top = base; base = top + alevel;
	base = top + dlevel;
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
		float centerx = (create ? botlen : 0), centery = (create ? sidelen : 0);
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

BOOL LextObject::HasUVW() {
	BOOL genUVs;
	Interval v;
	pblock2->GetValue(lext_mapping, 0, genUVs, v);
	return genUVs;
}

void LextObject::SetGenUVW(BOOL sw) {
	if (sw == HasUVW()) return;
	pblock2->SetValue(lext_mapping, 0, sw);
	UpdateUI();
}

void LextObject::BuildMesh(TimeValue t)
{
	int hsegs, ssegs, bsegs, wsegs;
	float height, sidelen, botlen, sidewidth, botwidth;
	int genUVs;

	// Start the validity interval at forever and widdle it down.
	ivalid = FOREVER;

	pblock2->GetValue(lext_hsegs, t, hsegs, ivalid);
	pblock2->GetValue(lext_ssegs, t, ssegs, ivalid);
	pblock2->GetValue(lext_bsegs, t, bsegs, ivalid);
	pblock2->GetValue(lext_wsegs, t, wsegs, ivalid);
	pblock2->GetValue(lext_slength, t, sidelen, ivalid);
	pblock2->GetValue(lext_blength, t, botlen, ivalid);
	pblock2->GetValue(lext_swidth, t, sidewidth, ivalid);
	pblock2->GetValue(lext_bwidth, t, botwidth, ivalid);
	pblock2->GetValue(lext_height, t, height, ivalid);
	pblock2->GetValue(lext_mapping, t, genUVs, ivalid);
	LimitValue(height, MIN_HEIGHT, MAX_HEIGHT);
	LimitValue(sidelen, MIN_LENGTH, MAX_LENGTH);
	LimitValue(botlen, MIN_LENGTH, MAX_LENGTH);
	LimitValue(sidewidth, MIN_WIDTH, MAX_WIDTH);
	LimitValue(botwidth, MIN_WIDTH, MAX_WIDTH);
	LimitValue(hsegs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(ssegs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(bsegs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(wsegs, MIN_SEGMENTS, MAX_SEGMENTS);

	BOOL usePhysUVs = GetUsePhysicalScaleUVs();
	int createmeth = pblock2->GetInt(lext_centercreate, 0, FOREVER);
	BuildLextMesh(mesh, hsegs, ssegs, bsegs, wsegs, height,
		sidelen, botlen, sidewidth, botwidth, genUVs, createmeth, usePhysUVs);
}

inline Point3 operator+(const PatchVert &pv, const Point3 &p)
{
	return p + pv.p;
}

Object* LextObject::ConvertToType(TimeValue t, Class_ID obtype)
{
	return __super::ConvertToType(t, obtype);
}

int LextObject::CanConvertToType(Class_ID obtype)
{
	if (obtype == triObjectClassID) {
		return 1;
	}
	else {
		return __super::CanConvertToType(obtype);
	}
}

class LextObjCreateCallBack : public CreateMouseCallBack {
	LextObject *ob;
	Point3 p[2], d;
	IPoint2 sp0, sp1, sp2;
	float l, hd, xwid, slen;
public:
	int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat);
	void SetObj(LextObject *obj) { ob = obj; }
};

int LextObjCreateCallBack::proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat)
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
		case 0:
			ob->suspendSnap = TRUE;
			sp0 = m;
			p[0] = vpt->SnapPoint(m, m, NULL, snapdim);
			mat.SetTrans(p[0]); // Set Node's transform				
			ob->pblock2->SetValue(lext_slength, 0, 0.01f);
			ob->pblock2->SetValue(lext_blength, 0, 0.01f);
			ob->pblock2->SetValue(lext_centercreate, 0, lext_crtype_blk.GetInt(lext_create_meth));
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
			if (!lext_crtype_blk.GetInt(lext_create_meth))
			{
				mat.SetTrans(p[0]);
				if (flags&MOUSE_CTRL) d.x = (d.y *= 2.0f);
			}
			else
			{
				mat.SetTrans(p[0] - Point3(fabs(d.x), fabs(d.y), fabs(d.z)));
				d = 2.0f*d;
			}
			slen = (float)fabs(d.y);
			xwid = (float)fabs(d.x);
			ob->pblock2->SetValue(lext_blength, 0, d.x);
			ob->pblock2->SetValue(lext_bwidth, 0, 0.2f*(float)fabs(d.x));
			ob->pblock2->SetValue(lext_slength, 0, d.y);
			ob->pblock2->SetValue(lext_swidth, 0, 0.2f*(float)fabs(d.y));

			if (msg == MOUSE_POINT && (Length(sp1 - sp0) < 3 ))
			{
				return CREATE_ABORT;
			}
			break;
		case 2:
		{
			sp2 = m;
			float h = vpt->SnapLength(vpt->GetCPDisp(p[1], Point3(0, 0, 1), sp1, m, TRUE));
			ob->pblock2->SetValue(lext_height, 0, h);

			if (msg == MOUSE_POINT) {
				if (Length(m - sp0) < 3)
				{
					return CREATE_ABORT;
				}
			}
			break;
		}
		case 3:
			float f = vpt->SnapLength(vpt->GetCPDisp(p[1], Point3(0, 1, 0), sp2, m));
			if (f < 0.0f) f = 0.0f;
			if (f > slen) f = slen;
			ob->pblock2->SetValue(lext_swidth, 0, (f > xwid ? xwid : f));
			ob->pblock2->SetValue(lext_bwidth, 0, f);
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

static LextObjCreateCallBack lextCreateCB;

CreateMouseCallBack* LextObject::GetCreateMouseCallBack()
{
	lextCreateCB.SetObj(this);
	return(&lextCreateCB);
}

BOOL LextObject::OKtoDisplay(TimeValue t)
{
	float length;
	pblock2->GetValue(lext_blength, t, length, FOREVER);
	if (length == 0.0f) return FALSE;
	else return TRUE;
}

void LextObject::InvalidateUI()
{
	lext_param_blk.InvalidateUI(pblock2->LastNotifyParamID());
}

RefTargetHandle LextObject::Clone(RemapDir& remap)
{
	LextObject* newob = new LextObject(FALSE);
	newob->ReplaceReference(0, remap.CloneRef(pblock2));
	newob->ivalid.SetEmpty();
	BaseClone(this, newob, remap);
	newob->SetUsePhysicalScaleUVs(GetUsePhysicalScaleUVs());
	return(newob);
}

void LextObject::UpdateUI()
{
	if (ip == NULL)
		return;
	LextWidthDlgProc* dlg = static_cast<LextWidthDlgProc*>(lext_param_blk.GetUserDlgProc());
	dlg->UpdateUI();
}

BOOL LextObject::GetUsePhysicalScaleUVs()
{
	return ::GetUsePhysicalScaleUVs(this);
}


void LextObject::SetUsePhysicalScaleUVs(BOOL flag)
{
	BOOL curState = GetUsePhysicalScaleUVs();
	if (curState == flag)
		return;
	if (theHold.Holding())
		theHold.Put(new RealWorldScaleRecord<LextObject>(this, curState));
	::SetUsePhysicalScaleUVs(this, flag);
	if (pblock2 != NULL)
		pblock2->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	UpdateUI();
	macroRec->SetProperty(this, _T("realWorldMapSize"), mr_bool, flag);
}
