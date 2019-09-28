/**********************************************************************
 *<
	FILE: chcyl.cpp - builds filleted/chamfered cylinders
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

class ChCylinderObject : public SimpleObject2, public RealWorldMapSizeInterface {
	friend class ChCylTypeInDlgProc;
	friend class ChCylinderObjCreateCallBack;
	friend class ChCylFilletDlgProc;
	friend class ChCylFilletAccessor;
	public:
		// Class vars
		static IObjParam *ip;
		static bool typeinCreate;

		ChCylinderObject(BOOL loading);

		// From Object
		int CanConvertToType(Class_ID obtype) override;
		Object* ConvertToType(TimeValue t, Class_ID obtype) override;
				
		// From BaseObject
		CreateMouseCallBack* GetCreateMouseCallBack() override;
		void BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev) override;
		void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next) override;
		const TCHAR *GetObjectName() override { return GetString(IDS_RB_CHCYLINDER); }
		BOOL HasUVW() override;
		void SetGenUVW(BOOL sw) override;
				
		// Animatable methods		
		void DeleteThis() override { delete this; }
		Class_ID ClassID() override { return CHAMFERCYL_CLASS_ID; }
				
		// From ref
		RefTargetHandle Clone(RemapDir& remap) override;
		bool SpecifySaveReferences(ReferenceSaveManager& referenceSaveManager) override;
		IOResult Load(ILoad *iload) override;

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
	};

#define MIN_SEGMENTS	1
#define MAX_SEGMENTS	200

#define MIN_SIDES		3
#define MAX_SIDES		200

#define MIN_RADIUS		float(0)
#define MAX_RADIUS		float( 1.0E30)
#define MIN_HEIGHT		float(-1.0E30)
#define MAX_HEIGHT		float( 1.0E30)
#define MIN_SLICE	float(-1.0E30)
#define MAX_SLICE	float( 1.0E30)

#define DEF_SEGMENTS 	1
#define DEF_SIDES		12

#define DEF_RADIUS		float(0.0)
#define DEF_HEIGHT		float(0.01)
#define DEF_FILLET		float(0.01)

#define SMOOTH_ON		1
#define SMOOTH_OFF		0

// in solids.cpp  - The dll instance handle
extern HINSTANCE hInstance;

// class variables for Chamfer Box class.
IObjParam *ChCylinderObject::ip = NULL;
bool ChCylinderObject::typeinCreate = false;

#define PBLOCK_REF_NO  0

//--- ClassDescriptor and class vars ---------------------------------

static BOOL sInterfaceAdded = FALSE;

class ChCylClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE)
	{
		if (!sInterfaceAdded) {
			AddInterface(&gRealWorldMapSizeDesc);
			sInterfaceAdded = TRUE;
		}
		return new ChCylinderObject(loading);
	}
	const TCHAR *	ClassName() override { return GetString(IDS_AP_CHCYLINDER_CLASS); }
	SClass_ID		SuperClassID() override { return GEOMOBJECT_CLASS_ID; }
	Class_ID		ClassID() override { return CHAMFERCYL_CLASS_ID; }
	const TCHAR* 	Category() override { return GetString(IDS_RB_EXTENDED); }
	const TCHAR* InternalName() { return _T("ChamferCyl"); } // returns fixed parsable name (scripter-visible name)
	HINSTANCE  HInstance() { return hInstance; }   // returns owning module handle
};

static ChCylClassDesc chcylDesc;

ClassDesc* GetChCylinderDesc() { return &chcylDesc; }

#define MIN_LENGTH		float(0.0)
#define MAX_LENGTH		float(1.0E30)

// ParamBlockDesc2 IDs
enum paramblockdesc2_ids { chcyl_creation_type, chcyl_type_in, chcyl_params, };
enum chcyl_creation_type_param_ids { chcyl_create_meth, };
enum chcyl_type_in_param_ids { chcyl_ti_pos, chcyl_ti_radius, chcyl_ti_height, chcyl_ti_fillet, };
enum chcyl_param_param_ids {
	chcyl_radius = CHCYL_RADIUS, chcyl_height = CHCYL_HEIGHT, chcyl_fillet = CHCYL_FILLET,
	chcyl_hsegs = CHCYL_HSEGS, chcyl_fsegs = CHCYL_FSEGS, chcyl_sides = CHCYL_SIDES,
	chcyl_csegs = CHCYL_CSEGS, chcyl_smooth = CHCYL_SMOOTHON, chcyl_slice = CHCYL_SLICEON,
	chcyl_pieslice1 = CHCYL_SLICEFROM, chcyl_pieslice2 = CHCYL_SLICETO, chcyl_mapping = CHCYL_GENUVS,
};

namespace
{
	MaxSDK::Util::StaticAssert< (chcyl_params == CHCYL_PARAMBLOCK_ID) > validator;
}

void FixChCylFillet(IParamBlock2* pblock2, TimeValue t, ParamID radiusId, ParamID heightId, ParamID filletId, ParamID id, PB2Value &v)
{
	float radius = 0.f, height = 0.f, fillet = 0.f;
	pblock2->GetValue(radiusId, t, radius, FOREVER);
	pblock2->GetValue(heightId, t, height, FOREVER);
	pblock2->GetValue(filletId, t, fillet, FOREVER);
	// Fillet length upper limit is the minimum between radius and height/2
	float hh = 0.5f*(float)fabs(height), maxf = (hh > radius ? radius : hh);
	IParamMap2 * pParamMap = pblock2->GetMap(filletId);
	if (pParamMap)
		pParamMap->SetRange(filletId, 0.f, maxf);
	if (fillet > maxf)
	{
		if (id == filletId)
			v.f = maxf;
		else if (id == radiusId || id == heightId)
			pblock2->SetValue(filletId, t, maxf);
	}
}

void InitChCylFillet(IParamBlock2* pblock2, TimeValue t, ParamID radiusId, ParamID heightId, ParamID filletId)
{
	float radius = 0.f, height = 0.f, fillet = 0.f;
	pblock2->GetValue(radiusId, t, radius, FOREVER);
	pblock2->GetValue(heightId, t, height, FOREVER);
	pblock2->GetValue(filletId, t, fillet, FOREVER);
	// Fillet length upper limit is the minimum between radius and height/2
	float hh = 0.5f*(float)fabs(height), maxf = (hh > radius ? radius : hh);
	IParamMap2 * pParamMap = pblock2->GetMap(filletId);
	if (pParamMap)
		pParamMap->SetRange(filletId, 0.f, maxf);
}

class ChCylFilletClassAccessor : public PBAccessor
{
	bool m_disable; // for preventing re-entry
public:
	ChCylFilletClassAccessor() : m_disable(false) {}
	virtual void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t) override
	{
		if (m_disable)
			return;
		if (theHold.RestoreOrRedoing())
			return;
		IParamBlock2* pblock2 = chcylDesc.GetParamBlockDescByID(chcyl_type_in)->class_params;
		DbgAssert(pblock2);
		if (pblock2 == nullptr)
			return;
		m_disable = true;
		FixChCylFillet(pblock2, t, chcyl_ti_radius, chcyl_ti_height, chcyl_ti_fillet, id, v);
		m_disable = false;
	}
};
static ChCylFilletClassAccessor chCylFilletClass_Accessor;


class ChCylFilletAccessor : public PBAccessor
{
	bool m_disable; // for preventing re-entry
public:
	ChCylFilletAccessor() : m_disable(false) {}
	virtual void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t) override
	{
		if (m_disable)
			return;
		if (theHold.RestoreOrRedoing())
			return;
		IParamBlock2* pblock2 = ((ChCylinderObject*)owner)->pblock2;
		DbgAssert(pblock2);
		if (pblock2 == nullptr)
			return;
		m_disable = true;
		FixChCylFillet(pblock2, t, chcyl_radius, chcyl_height, chcyl_fillet, id, v);
		m_disable = false;
	}
};
static ChCylFilletAccessor chCylFillet_Accessor;

namespace
{
	class MapCoords_Accessor : public PBAccessor
	{
		void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
		{
			if (owner) ((ChCylinderObject*)owner)->UpdateUI();
		}
	};
	static MapCoords_Accessor mapCoords_Accessor;
}

// class creation type block
static ParamBlockDesc2 chcyl_crtype_blk(chcyl_creation_type, _T("ChamferCylCreationType"), 0, &chcylDesc, P_CLASS_PARAMS + P_AUTO_UI,
	//rollout
	IDD_UREVS1, IDS_RB_CREATE_DIALOG, BEGIN_EDIT_CREATE, 0, NULL,
	// params
	chcyl_create_meth, _T("typeinCreationMethod"), TYPE_INT, 0, IDS_RB_CREATE_DIALOG,
	p_default, 1,
	p_range, 0, 1,
	p_ui, TYPE_RADIO, 2, IDC_UCYLS_BYDIA, IDC_UCYLS_BYRAD, // Diameter/radius
	p_end,
	p_end
	);

// class type-in block
static ParamBlockDesc2 chcyl_typein_blk(chcyl_type_in, _T("ChamferCylTypeIn"), 0, &chcylDesc, P_CLASS_PARAMS + P_AUTO_UI,
	//rollout
	IDD_CHAMFERCYL2, IDS_RB_KEYBOARDENTRY, BEGIN_EDIT_CREATE, APPENDROLL_CLOSED, NULL,
	// params
	chcyl_ti_pos, _T("typeInPos"), TYPE_POINT3, 0, IDS_RB_TYPEIN_POS,
	p_default, Point3(0, 0, 0),
	p_range, float(-1.0E30), float(1.0E30),
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CCY_POSX, IDC_CCY_POSXSPIN, IDC_CCY_POSY, IDC_CCY_POSYSPIN, IDC_CCY_POSZ, IDC_CCY_POSZSPIN, SPIN_AUTOSCALE,
	p_end,
	chcyl_ti_radius, _T("typeInRadius"), TYPE_FLOAT, 0, IDS_RB_RADIUS,
	p_default, 0.0f,
	p_range, MIN_RADIUS, MAX_RADIUS,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CCY_RADIUS, IDC_CCY_RADIUSSPIN, SPIN_AUTOSCALE,
	p_accessor, &chCylFilletClass_Accessor,
	p_end,
	chcyl_ti_height, _T("typeInHeight"), TYPE_FLOAT, 0, IDS_RB_HEIGHT,
	p_default, 0.0f,
	p_range, MIN_HEIGHT, MAX_HEIGHT,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CCY_HEIGHT, IDC_CCY_HEIGHTSPIN, SPIN_AUTOSCALE,
	p_accessor, &chCylFilletClass_Accessor,
	p_end,
	chcyl_ti_fillet, _T("typeInFillet"), TYPE_FLOAT, 0, IDS_RB_FILLET,
	p_default, 0.0f,
	p_range, MIN_LENGTH, MAX_LENGTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CCY_FILLET, IDC_CCY_FILLETSPIN, SPIN_AUTOSCALE,
	p_accessor, &chCylFilletClass_Accessor,
	p_end,
	p_end
	);

// per instance Cylinder block
static ParamBlockDesc2 chcyl_param_blk(chcyl_params, _T("ChamferCylParameters"), 0, &chcylDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_REF_NO,
	//rollout
	IDD_CHAMFERCYL3, IDS_AP_PARAMETERS, 0, 0, NULL,
	// params
	chcyl_radius, _T("radius"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_RADIUS,
	p_default, DEF_RADIUS,
	p_range, MIN_RADIUS, MAX_RADIUS,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CCY_RADIUS, IDC_CCY_RADIUSSPIN, SPIN_AUTOSCALE,
	p_accessor, &chCylFillet_Accessor,
	p_end,
	chcyl_height, _T("height"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_HEIGHT,
	p_default, 0.0f,
	p_range, MIN_HEIGHT, MAX_HEIGHT,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CCY_HEIGHT, IDC_CCY_HEIGHTSPIN, SPIN_AUTOSCALE,
	p_accessor, &chCylFillet_Accessor,
	p_end,
	chcyl_fillet, _T("fillet"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_FILLET,
	p_default, 0.0f,
	p_range, MIN_LENGTH, MAX_LENGTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CCY_FILLET, IDC_CCY_FILLETSPIN, SPIN_AUTOSCALE,
	p_accessor, &chCylFillet_Accessor,
	p_end,
	chcyl_hsegs, _T("Height Segments"), TYPE_INT, P_ANIMATABLE, IDS_RB_HSEGS,
	p_default, DEF_SEGMENTS,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_CCY_HGTSEGS, IDC_CCY_HGTSEGSSPIN, 0.1f,
	p_end,
	chcyl_fsegs, _T("Fillet Segments"), TYPE_INT, P_ANIMATABLE, IDS_RB_FSEGS,
	p_default, DEF_SEGMENTS,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_CCY_FILLETSEGS, IDC_CCY_FILLETSEGSSPIN, 0.1f,
	p_end,
	chcyl_sides, _T("sides"), TYPE_INT, P_ANIMATABLE, IDS_RB_SIDES,
	p_default, DEF_SIDES,
	p_range, MIN_SIDES, MAX_SIDES,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_CCY_SIDES, IDC_CCY_SIDESSPIN, 0.1f,
	p_end,
	chcyl_csegs, _T("Cap Segments"), TYPE_INT, P_ANIMATABLE, IDS_RB_CAPSEGS,
	p_default, DEF_SEGMENTS,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_CCY_CAPSEGS, IDC_CCY_CAPSEGSSPIN, 0.1f,
	p_end,
	chcyl_smooth, _T("Smooth On"), TYPE_BOOL, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_SMOOTHON,
	p_default, TRUE,
	p_ui, TYPE_SINGLECHECKBOX, IDC_CCY_SMOOTHON,
	p_end,
	chcyl_slice, _T("Slice On"), TYPE_BOOL, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_SLICEON,
	p_default, FALSE,
	p_ui, TYPE_SINGLECHECKBOX, IDC_CCY_SLICEON,
	p_enable_ctrls, 2, chcyl_pieslice1, chcyl_pieslice2,
	p_end,
	chcyl_pieslice1, _T("Slice From"), TYPE_ANGLE, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_SLICEFROM,
	p_default, 0.0,
	p_range, MIN_SLICE, MAX_SLICE,
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_CCY_SLICE1, IDC_CCY_SLICE1SPIN, 0.5f,
	p_end,
	chcyl_pieslice2, _T("Slice To"), TYPE_ANGLE, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_SLICETO,
	p_default, 0.0,
	p_range, MIN_SLICE, MAX_SLICE,
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_CCY_SLICE2, IDC_CCY_SLICE2SPIN, 0.5f,
	p_end,
	chcyl_mapping, _T("mapCoords"), TYPE_BOOL, P_RESET_DEFAULT, IDS_MXS_GENUVS,
	p_default, TRUE,
	p_ui, TYPE_SINGLECHECKBOX, IDC_GENTEXTURE,
	p_accessor, &mapCoords_Accessor,
	p_end,
	p_end
	);

// variable type, NULL, animatable, number
ParamBlockDescID chcyldescVer0[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0, chcyl_radius },
	{ TYPE_FLOAT, NULL, TRUE, 1, chcyl_height },
	{ TYPE_FLOAT, NULL, TRUE, 2, chcyl_fillet },
	{ TYPE_INT, NULL, TRUE, 3, chcyl_hsegs }, 
	{ TYPE_INT, NULL, TRUE, 4, chcyl_fsegs }, 
	{ TYPE_INT, NULL, TRUE, 5, chcyl_sides }, 
	{ TYPE_INT, NULL, TRUE, 6, chcyl_csegs },
	{ TYPE_INT, NULL, TRUE, 7, chcyl_smooth }, 
	{ TYPE_INT, NULL, TRUE, 8, chcyl_slice },
	{ TYPE_FLOAT, NULL, TRUE, 9, chcyl_pieslice1 },
	{ TYPE_FLOAT, NULL, TRUE, 10, chcyl_pieslice2 },
	{ TYPE_INT, NULL, FALSE, 11, chcyl_mapping } 
	};

// Array of old versions
static ParamVersionDesc versions[] = {
	ParamVersionDesc(chcyldescVer0,12,0)
};


#define NUM_OLDVERSIONS	1
// ParamBlock data for SaveToPrevious support
#define CURRENT_VERSION	0
#define PBLOCK_LENGTH	12

//--- TypeInDlgProc --------------------------------

class ChCylTypeInDlgProc : public ParamMap2UserDlgProc {
	public:
		ChCylinderObject *ob;

		ChCylTypeInDlgProc(ChCylinderObject *o) {ob=o;}
		INT_PTR DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam) override;
		void DeleteThis() override {delete this;}
	};

INT_PTR ChCylTypeInDlgProc::DlgProc(
	TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG: {
		InitChCylFillet(map->GetParamBlock(), t, chcyl_ti_radius, chcyl_ti_height, chcyl_ti_fillet);
		break;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CCY_CREATE: {
			if (chcyl_typein_blk.GetFloat(chcyl_ti_radius) == 0.0) return TRUE;

			// We only want to set the value if the object is 
			// not in the scene.
			if (ob->TestAFlag(A_OBJ_CREATING)) {
				ob->pblock2->SetValue(chcyl_radius, 0, chcyl_typein_blk.GetFloat(chcyl_ti_radius));
				ob->pblock2->SetValue(chcyl_height, 0, chcyl_typein_blk.GetFloat(chcyl_ti_height));
				ob->pblock2->SetValue(chcyl_fillet, 0, chcyl_typein_blk.GetFloat(chcyl_ti_fillet));
			}
			else
				ChCylinderObject::typeinCreate = true;

			Matrix3 tm(1);
			tm.SetTrans(chcyl_typein_blk.GetPoint3(chcyl_ti_pos));
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


class ChCylFilletDlgProc : public ParamMap2UserDlgProc {
	public:
		ChCylinderObject *ob;
        HWND mhWnd;
		ChCylFilletDlgProc(ChCylinderObject *o) {ob=o;}
		INT_PTR DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam) override;
		void DeleteThis() override {delete this;}
        void UpdateUI();
        BOOL GetRWSState();
	};

BOOL ChCylFilletDlgProc::GetRWSState()
{
    BOOL check = IsDlgButtonChecked(mhWnd, IDC_REAL_WORLD_MAP_SIZE);
    return check;
}

void ChCylFilletDlgProc::UpdateUI()
{
    if (ob == NULL) return;
    BOOL usePhysUVs = ob->GetUsePhysicalScaleUVs();
    CheckDlgButton(mhWnd, IDC_REAL_WORLD_MAP_SIZE, usePhysUVs);
    EnableWindow(GetDlgItem(mhWnd, IDC_REAL_WORLD_MAP_SIZE), ob->HasUVW());
}

INT_PTR ChCylFilletDlgProc::DlgProc(
	TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG: {
		mhWnd = hWnd;
		InitChCylFillet(map->GetParamBlock(), t, chcyl_radius, chcyl_height, chcyl_fillet);
		UpdateUI();
		break;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
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
//--- ChCylinder methods -------------------------------

ChCylinderObject::ChCylinderObject(BOOL loading)
{
	chcylDesc.MakeAutoParamBlocks(this);

	if (!loading && !GetPhysicalScaleUVsDisabled())
		SetUsePhysicalScaleUVs(true);
}

bool ChCylinderObject::SpecifySaveReferences(ReferenceSaveManager& referenceSaveManager)
{
	// if saving to previous version that used pb1 instead of pb2...
	DWORD saveVersion = GetSavingVersion();
	if (saveVersion != 0 && saveVersion <= MAX_RELEASE_R19)
	{
		ProcessPB2ToPB1SaveToPrevious(this, pblock2, PBLOCK_REF_NO, chcyldescVer0, PBLOCK_LENGTH, CURRENT_VERSION);
	}
	return __super::SpecifySaveReferences(referenceSaveManager);
}

IOResult ChCylinderObject::Load(ILoad *iload)
{
	ParamBlock2PLCB* plcb = new ParamBlock2PLCB(versions, NUM_OLDVERSIONS, &chcyl_param_blk, this, PBLOCK_REF_NO);
	iload->RegisterPostLoadCallback(plcb);
	return IO_OK;
}


void ChCylinderObject::BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev)
{
	__super::BeginEditParams(ip,flags,prev);
	this->ip = ip;
	// If this has been freshly created by type-in, set creation values:
	if (ChCylinderObject::typeinCreate)
	{
		pblock2->SetValue(chcyl_radius, 0, chcyl_typein_blk.GetFloat(chcyl_ti_radius));
		pblock2->SetValue(chcyl_height, 0, chcyl_typein_blk.GetFloat(chcyl_ti_height));
		pblock2->SetValue(chcyl_fillet, 0, chcyl_typein_blk.GetFloat(chcyl_ti_fillet));
		ChCylinderObject::typeinCreate = false;
	}

	// throw up all the appropriate auto-rollouts
	chcylDesc.BeginEditParams(ip, this, flags, prev);
	// if in Create Panel, install a callback for the type in.
	if (flags & BEGIN_EDIT_CREATE)
		chcyl_typein_blk.SetUserDlgProc(new ChCylTypeInDlgProc(this));
	// install a callback for the params.
	chcyl_param_blk.SetUserDlgProc(new ChCylFilletDlgProc(this));
}

void ChCylinderObject::EndEditParams( IObjParam *ip, ULONG flags,Animatable *next )
{		
	__super::EndEditParams(ip,flags,next);
	this->ip = NULL;
	chcylDesc.EndEditParams(ip, this, flags, next);
}

void BuildChCylinderMesh(Mesh &mesh,
		int segs, int smooth, int llsegs, int capsegs, int csegs,int doPie,
		float radius1, float radius2, float height, float pie1, float pie2,
		int genUVs, BOOL usePhysUVs)
	{
	Point3 p;
	BOOL minush=height<0.0f;
	if (minush) height=-height;
	int ic = 1;
	int nf=0,nv=0, lsegs,VertexPerLevel;
	float delta,ang,hh=height*0.5f;	
	float totalPie, startAng = 0.0f;
	if (radius2>radius1) radius2=radius1;
	if (radius2>hh) radius2=hh;

	if (doPie) doPie = 1;
	else doPie = 0; 

	lsegs = llsegs-1 + 2*capsegs;

	// Make pie2 < pie1 and pie1-pie2 < TWOPI
	while (pie1 < pie2) pie1 += TWOPI;
	while (pie1 > pie2+TWOPI) pie1 -= TWOPI;
	if (pie1==pie2) totalPie = TWOPI;
	else totalPie = pie1-pie2;		
	int nfaces,ntverts,levels=csegs*2+(llsegs-1);
	int capv=segs,sideedge=capsegs+csegs,*edgelstr,*edgelstl,totlevels,incap;
    // capv=vertex in one cap layer
	totlevels=levels+capsegs*2+2;
	incap=capsegs+1;
	int	tvinslice=totlevels+totlevels-2;
	if (doPie) {
		delta    = totalPie/(float)(segs);
		startAng = pie2; capv++;
		VertexPerLevel=segs+2;
		nfaces=2*segs*(levels+1)+(sideedge+llsegs)*4;
		ntverts=tvinslice+2*(segs+1);
		// 2 faces between every 2 vertices, with 2 ends, except in central cap)
	} else {
		delta = (float)2.0*PI/(float)segs;
		VertexPerLevel=segs;
		nfaces=2*segs*(levels+1);
		ntverts=2*(segs+1)+llsegs-1;
	}
	if (height<0) {delta = -delta;}
	edgelstl=new int[totlevels];
	edgelstr=new int[totlevels];
	int lastlevel=totlevels-1,dcapv=capv-1,dvertper=VertexPerLevel-1;
	edgelstr[0]=0;edgelstl[0]=0;
	edgelstr[1]=1;
	edgelstl[1]=capv;
   int i;
	for (i=2;i<=sideedge;i++)
	{ edgelstr[i]=edgelstr[i-1]+capv;
	  edgelstl[i]=edgelstr[i]+dcapv;
	}
	while (i<=totlevels-sideedge)
	{ edgelstr[i]=edgelstr[i-1]+VertexPerLevel;
	  edgelstl[i]=edgelstr[i]+dcapv;
	  i++;
	}
	while (i<lastlevel)
	{ edgelstr[i]=edgelstr[i-1]+capv;
	  edgelstl[i]=edgelstr[i]+dcapv;
	  i++;
	}
	edgelstl[lastlevel]=(edgelstr[lastlevel]=edgelstl[i-1]+1);
	int nverts=edgelstl[lastlevel]+1;

	nfaces+=2*segs*(2*capsegs-1);

	mesh.setNumVerts(nverts);
	mesh.setNumFaces(nfaces);
	mesh.setSmoothFlags(smooth != 0);
	if (genUVs) 
	{ ntverts+=nverts;
	  mesh.setNumTVerts(ntverts);
	  mesh.setNumTVFaces(nfaces);
	} 
	else 
	{ mesh.setNumTVerts(0);
	  mesh.setNumTVFaces(0);
	}
	mesh.setSmoothFlags((smooth != 0) | ((doPie != 0) << 1));

	// bottom vertex 
	mesh.setVert(nv, Point3(0.0f,0.0f,height));
	mesh.setVert(nverts-1, Point3(0.0f,0.0f,0.0f));		
	float ru,cang,sang,rotz,botz,deltacsegs=PI/(2.0f*csegs),radius=radius1-radius2;
	int iy,msegs=segs,deltaend=nverts-capv-1;
	// Bottom cap vertices
	ang = startAng;	 
	if (!doPie) msegs--;
	for (int jx = 0; jx<=msegs; jx++) 
	{ cang=(float)cos(ang);
	  sang=(float)sin(ang);
	  iy=0;
	  for(int ix=1; ix<=sideedge; ix++)
	  { if (ix<=capsegs)
		{ botz=0.0f;
		  p.z = height;
		  ru=radius*float(ix)/float(capsegs);
	    }
		else
		{ iy++;
		  ru=radius+radius2*(float)sin(rotz=deltacsegs*float(iy));
		  if (jx==0)
		  {	p.z=(iy==csegs?height-radius2:height-radius2*(1.0f-(float)cos(rotz)));
		  } else p.z=mesh.verts[edgelstr[ix]].z;
		  botz=height-p.z;
		  if ((doPie)&&((jx==0)&&(ix==sideedge)))
		  {	mesh.setVert(edgelstl[ix]+1,Point3(0.0f,0.0f,p.z));
			mesh.setVert(edgelstl[lastlevel-ix]+1,Point3(0.0f,0.0f,botz));
		  }
		}
		p.x = cang*ru;
		p.y = sang*ru;	
		mesh.setVert(edgelstr[ix]+jx, p);
		mesh.setVert(edgelstr[lastlevel-ix]+jx,Point3(p.x,p.y,botz));
	  }
	  ang += delta;
	}
	//top layer done, now reflect sides down 
	int startv=edgelstr[sideedge],deltav;
	if (llsegs>1)
	{ float sideheight=height-2.0f*radius2,topd=height-radius2,sincr=sideheight/llsegs;
	  for (int sidevs=0;sidevs<VertexPerLevel;sidevs++)
	  { p=mesh.verts[startv];
	    deltav=VertexPerLevel;
	    for (int ic=1;ic<llsegs;ic++)
	    { p.z =topd-sincr*ic;
	 	  mesh.setVert(startv+deltav, p);
		  deltav+=VertexPerLevel;
	    }
	    startv++;
	  }
	}
	int lasttvl=0,lasttvr=0;
	if (genUVs)
	{ int tvcount=0,nexttv;
      float udenom= usePhysUVs ? 1.0f : 2.0f*radius1;
	  for (i=0;i<=sideedge;i++)
	  {	nexttv=edgelstr[i];
		while (nexttv<=edgelstl[i])
		{ mesh.setTVert(tvcount++,(radius1+mesh.verts[nexttv].x)/udenom,(radius1+mesh.verts[nexttv].y)/udenom,0.0f);
		  nexttv++;
	    }
	  }
	  int hcount=0;
	  float hlevel;
      float heightScale = usePhysUVs ? height-2.0f*radius2 : 1.0f;
      float radiusScale = usePhysUVs ? ((float)2.0f * PI * radius1) : 1.0f;
      if (doPie)
          radiusScale = radiusScale * ((float) fabs(totalPie) / (2.0f*PI));
	  for (i=sideedge;i<=lastlevel-sideedge;i++)
	  { hlevel=1.0f-hcount++/(float)llsegs;
		for (int iseg=0;iseg<=segs;iseg++)
		 mesh.setTVert(tvcount++,radiusScale*((float)iseg/segs),heightScale*hlevel,0.0f);
	  }
	  i--;
	  while (i<=lastlevel)
	  {	nexttv=edgelstr[i];
		while (nexttv<=edgelstl[i])
		{ mesh.setTVert(tvcount++,(radius1+mesh.verts[nexttv].x)/udenom,(radius1+mesh.verts[nexttv].y)/udenom,0.0f);
		  nexttv++;
	    }
		i++;
	  }
	  if (doPie)
	  { lasttvl=lasttvr=tvcount;
		float u,v;
		mesh.setTVert(tvcount++,0.0f,usePhysUVs ? height : 1.0f,0.0f);
		for (i=sideedge;i<=sideedge+llsegs;i++)
	    { mesh.setTVert(tvcount++,0.0f,mesh.verts[edgelstl[i]].z/(usePhysUVs?1.0f:(height-2.0f*radius2)),0.0f);
		}
		mesh.setTVert(tvcount++,0.0f,0.0f,0.0f);
		for (i=1;i<lastlevel;i++)
		{ u=(float)sqrt(mesh.verts[edgelstl[i]].x*mesh.verts[edgelstl[i]].x+mesh.verts[edgelstl[i]].y*mesh.verts[edgelstl[i]].y)/(usePhysUVs ? 1.0f : radius1);
		  v=mesh.verts[edgelstl[i]].z/(usePhysUVs? 1.0f : height);
		  mesh.setTVert(tvcount++,u,v,0.0f);
		  mesh.setTVert(tvcount++,u,v,0.0f);
		}
	  }
	}	
	int lvert=(doPie?segs+1:segs);
    int t0,t1,b0,b1,tvt0=0,tvt1=0,tvb0=1,tvb1=2,fc=0,smoothgr=(smooth?4:0),vseg=segs+1;
	int tvcount=0,lowerside=lastlevel-sideedge,onside=0;
	BOOL ok,wrap;
	// Now make faces ---
	for (int clevel=0;clevel<lastlevel-1;clevel++)
	{ t1=(t0=edgelstr[clevel])+1;
	  b1=(b0=edgelstr[clevel+1])+1;
	  ok=!doPie; wrap=FALSE;
	  if ((clevel>0)&&((doPie)||(onside==1))) {tvt0++;tvt1++;tvb0++,tvb1++;}
	  if (clevel==1) {tvt0=1;tvt1=2;}
	  if (clevel==sideedge)
	    {tvt1+=lvert;tvt0+=lvert;tvb0+=vseg;tvb1+=vseg;onside++;}
	  else if (clevel==lowerside)
	    {tvt1+=vseg;tvt0+=vseg;tvb0+=lvert;tvb1+=lvert;onside++;}
	  while ((b0<edgelstl[clevel+1])||ok)
	  { if (b1==edgelstr[clevel+2]) 
	    { b1=edgelstr[clevel+1]; 
	      t1=edgelstr[clevel];
		  ok=FALSE;wrap=(onside!=1);}
	  if (smooth)
	  { if (csegs>1) smoothgr=4;
	    else
	    {if ((clevel<capsegs)||(clevel>=lastlevel-capsegs))
	      smoothgr=4;
	    else if ((clevel<sideedge)||(clevel>=lowerside)) 
		  smoothgr=8;
		else smoothgr=16;
	    }
	  }
	  if (genUVs) mesh.tvFace[fc].setTVerts(tvt0,tvb0,(wrap?tvb1-segs:tvb1));
		AddFace(&mesh.faces[fc++],t0,b0,b1,0,smoothgr);
	    if (clevel>0)
		{ if (genUVs)
		  { if (wrap) mesh.tvFace[fc].setTVerts(tvt0++,tvb1-segs,tvt1-segs);
			else mesh.tvFace[fc].setTVerts(tvt0++,tvb1,tvt1);
			tvt1++;
		  }
		  AddFace(&mesh.faces[fc++],t0,b1,t1,1,smoothgr);
		  t0++;t1++;
		}
		b0++;b1++;tvb0++,tvb1++;
	  }
	}
	smoothgr=(smooth?4:0);
	t1=(t0=edgelstr[lastlevel-1])+1;b0=edgelstr[lastlevel];
	int lastpt=(doPie?lastlevel-1:lastlevel);
	if (doPie){tvt0++;tvt1++;tvb0++,tvb1++;}
	while (t0<edgelstl[lastpt])
	  { if ((!doPie)&&(t1==edgelstr[lastlevel]))
	    { t1=edgelstr[lastlevel-1];tvt1-=segs;}
		if (genUVs) mesh.tvFace[fc].setTVerts(tvt0++,tvb0,tvt1++);
		AddFace(&mesh.faces[fc++],t0,b0,t1,1,smoothgr);
		t0++;t1++;
	  }
	int chv=edgelstl[sideedge]+1,botcap=lastlevel-sideedge;
	int chb=edgelstl[botcap]+1,chm0,chm1,last=0,sg0=(smooth?2:0),sg1=(smooth?1:0);
	if (doPie)
	{int topctv=lasttvl+1,tvcount=topctv+llsegs+2;
	  for (int i=1;i<=lastlevel;i++)
	  { if (i<=sideedge)
		{ if (genUVs)
		  { mesh.tvFace[fc].setTVerts(tvcount,topctv,lasttvl);lasttvl=tvcount++;
		    mesh.tvFace[fc+1].setTVerts(lasttvr,topctv,tvcount);lasttvr=tvcount++;
		  }
		  AddFace(&mesh.faces[fc++],edgelstl[i],chv,edgelstl[last],(i==1?1:2),sg0);
		  AddFace(&mesh.faces[fc++],edgelstr[last],chv,edgelstr[i],(i==1?3:2),sg1);
		}
	    else if (i<=botcap)
		{ if (genUVs)
		  { topctv++;
			mesh.tvFace[fc].setTVerts(lasttvl,tvcount,topctv);
			mesh.tvFace[fc+1].setTVerts(lasttvl,topctv,topctv-1);lasttvl=tvcount++;
		    mesh.tvFace[fc+2].setTVerts(topctv-1,topctv,tvcount);
		    mesh.tvFace[fc+3].setTVerts(topctv-1,tvcount,lasttvr);lasttvr=tvcount++;
		  }
		  AddFace(&mesh.faces[fc++],edgelstl[last],edgelstl[i],chm1=(edgelstl[i]+1),0,sg0);
	      AddFace(&mesh.faces[fc++],edgelstl[last],chm1,chm0=(edgelstl[last]+1),1,sg0);
		  AddFace(&mesh.faces[fc++],chm0,chm1,edgelstr[i],0,sg1);
	      AddFace(&mesh.faces[fc++],chm0,edgelstr[i],edgelstr[last],1,sg1);
		}
		else
		{if (genUVs)
		  {	if (i==lastlevel) tvcount=topctv+1;
			mesh.tvFace[fc].setTVerts(tvcount,topctv,lasttvl);
			  if (i<lastlevel) lasttvl=tvcount++;
		    mesh.tvFace[fc+1].setTVerts(lasttvr,topctv,tvcount);lasttvr=tvcount++;
		  }
		  AddFace(&mesh.faces[fc++],edgelstl[i],chb,edgelstl[last],(i==lastlevel?3:2),sg0);
	      AddFace(&mesh.faces[fc++],edgelstr[last],chb,edgelstr[i],(i==lastlevel?1:2),sg1);
		}
		last++;
	  }
	}
	if (minush)
	for (int i=0;i<nverts;i++)
	   { mesh.verts[i].z-=height;	}
	if (edgelstr) delete []edgelstr;
	if (edgelstl) delete []edgelstl;
	assert(fc==mesh.numFaces);
//	assert(nv==mesh.numVerts);
	mesh.InvalidateTopologyCache();
	}

BOOL ChCylinderObject::HasUVW() { 
	BOOL genUVs;
	Interval v;
	pblock2->GetValue(chcyl_mapping, 0, genUVs, v);
	return genUVs; 
	}

void ChCylinderObject::SetGenUVW(BOOL sw) {  
	if (sw==HasUVW()) return;
	pblock2->SetValue(chcyl_mapping,0, sw);
	UpdateUI();
	}

void ChCylinderObject::BuildMesh(TimeValue t)
	{	
	int segs, smooth, hsegs, capsegs,fsegs;
	float radius,height,pie1, pie2,fillet;
	int doPie, genUVs;	

	// Start the validity interval at forever and widdle it down.
	ivalid = FOREVER;
	
	pblock2->GetValue(chcyl_fsegs,t,fsegs,ivalid);
	pblock2->GetValue(chcyl_sides,t,segs,ivalid);
	pblock2->GetValue(chcyl_hsegs,t,hsegs,ivalid);
	pblock2->GetValue(chcyl_csegs,t,capsegs,ivalid);
	pblock2->GetValue(chcyl_radius,t,radius,ivalid);
	pblock2->GetValue(chcyl_height,t,height,ivalid);
	pblock2->GetValue(chcyl_fillet,t,fillet,ivalid);
	pblock2->GetValue(chcyl_smooth,t,smooth,ivalid);
	pblock2->GetValue(chcyl_pieslice1,t,pie1,ivalid);
	pblock2->GetValue(chcyl_pieslice2,t,pie2,ivalid);
	pblock2->GetValue(chcyl_slice,t,doPie,ivalid);
	pblock2->GetValue(chcyl_mapping,t,genUVs,ivalid);
	LimitValue(radius, MIN_RADIUS, MAX_RADIUS);
	LimitValue(height, MIN_HEIGHT, MAX_HEIGHT);
	LimitValue(fsegs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(hsegs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(capsegs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(segs, MIN_SIDES, MAX_SIDES);
	LimitValue(smooth, 0, 1);	
	
	BuildChCylinderMesh(mesh,
		segs, smooth, hsegs, capsegs, fsegs,doPie,
		radius, fillet, height, pie1, pie2, genUVs, GetUsePhysicalScaleUVs());
	}

inline Point3 operator+(const PatchVert &pv,const Point3 &p)
	{
	return p+pv.p;
	}

#define CIRCLE_VECTOR_LENGTH 0.5517861843f


Object* ChCylinderObject::ConvertToType(TimeValue t, Class_ID obtype)
	{
		return __super::ConvertToType(t,obtype);
	}

int ChCylinderObject::CanConvertToType(Class_ID obtype)
	{
	if (obtype==triObjectClassID) {
		return 1;
	} else {
		return __super::CanConvertToType(obtype);
		}
	}

class ChCylinderObjCreateCallBack: public CreateMouseCallBack {
	ChCylinderObject *ob;	
	Point3 p[2];
	IPoint2 sp0,sp1,sp2;
	float h,r;
	public:
		int proc( ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat );
		void SetObj(ChCylinderObject *obj) { ob = obj; }
	};

int ChCylinderObjCreateCallBack::proc(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat ) {
	
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	float f;

	DWORD snapdim = SNAP_IN_3D;

	if (msg == MOUSE_FREEMOVE)
	{
		vpt->SnapPreview(m,m,NULL, snapdim);
	}
	bool createByRadius = chcyl_crtype_blk.GetInt(chcyl_create_meth) == 1;
	if (msg==MOUSE_POINT||msg==MOUSE_MOVE) {
		switch(point) {
			case 0:
				ob->suspendSnap = TRUE;				
				sp0 = m;				
				p[0] = vpt->SnapPoint(m,m,NULL,snapdim);
				mat.SetTrans(p[0]); // Set Node's transform				
				ob->pblock2->SetValue(chcyl_radius,0,0.01f);
				ob->pblock2->SetValue(chcyl_height,0,0.01f);
				ob->pblock2->SetValue(chcyl_fillet,0,0.01f);
				break;
			case 1: 
				mat.IdentityMatrix();
				//mat.PreRotateZ(HALFPI);
				sp1 = m;							   
				p[1] = vpt->SnapPoint(m,m,NULL,snapdim);
				if (createByRadius) {
					// radius	
					r = Length(p[1]-p[0]);
					mat.SetTrans(p[0]);
				} else {
					// diameter
					Point3 center = (p[0]+p[1])/float(2);
					r = Length(center-p[0]);
					mat.SetTrans(center);  // Modify Node's transform
					}
				
				ob->pblock2->SetValue(chcyl_radius,0,r);

				if (flags&MOUSE_CTRL) {
					float ang = (float)atan2(p[1].y-p[0].y,p[1].x-p[0].x);
					mat.PreRotateZ(ob->ip->SnapAngle(ang));
					}

				if (msg==MOUSE_POINT) {
					if (Length(m-sp0)<3 ) {
						return CREATE_ABORT;
						}
					}
				break;
			case 2:
				{ sp2=m;
				h = vpt->SnapLength(vpt->GetCPDisp(p[1],Point3(0,0,1),sp1,m,TRUE));
				ob->pblock2->SetValue(chcyl_height,0,h);

				if (msg==MOUSE_POINT) {					
					if (Length(m-sp0)<3) 
					{ 
						return CREATE_ABORT;}
					}
				}
				break;
			case 3:
				f =vpt->SnapLength(vpt->GetCPDisp(p[1],Point3(0,1,0),sp2,m));
				float hh=0.5f*float(fabs(h));if (f<0.0f) f=0.0f;
				if (f>r) f=r;
				if (f>hh) f=hh;
				ob->pblock2->SetValue(chcyl_fillet,0,f);

				if (msg==MOUSE_POINT) 
				{  ob->suspendSnap = FALSE;
				   return CREATE_STOP;
				}
				break;
			}
		}
	else
	if (msg == MOUSE_ABORT) {
		return CREATE_ABORT;
		}

	return TRUE;
	}

static ChCylinderObjCreateCallBack cylCreateCB;

CreateMouseCallBack* ChCylinderObject::GetCreateMouseCallBack() 
	{
	cylCreateCB.SetObj(this);
	return(&cylCreateCB);
	}

BOOL ChCylinderObject::OKtoDisplay(TimeValue t)
{
	float radius;
	pblock2->GetValue(chcyl_radius, t, radius, FOREVER);
	if (radius == 0.0f) return FALSE;
	else return TRUE;
}

void ChCylinderObject::InvalidateUI()
{
	chcyl_param_blk.InvalidateUI(pblock2->LastNotifyParamID());
}

RefTargetHandle ChCylinderObject::Clone(RemapDir& remap) 
	{
	ChCylinderObject* newob = new ChCylinderObject(FALSE);	
	newob->ReplaceReference(0,remap.CloneRef(pblock2));	
	newob->ivalid.SetEmpty();	
	BaseClone(this, newob, remap);
    newob->SetUsePhysicalScaleUVs(GetUsePhysicalScaleUVs());
	return(newob);
	}

void ChCylinderObject::UpdateUI()
{
	if (ip == NULL)
		return;
	ChCylFilletDlgProc* dlg = static_cast<ChCylFilletDlgProc*>(chcyl_param_blk.GetUserDlgProc());
	dlg->UpdateUI();
}

BOOL ChCylinderObject::GetUsePhysicalScaleUVs()
{
    return ::GetUsePhysicalScaleUVs(this);
}


void ChCylinderObject::SetUsePhysicalScaleUVs(BOOL flag)
{
    BOOL curState = GetUsePhysicalScaleUVs();
    if (curState == flag)
        return;
    if (theHold.Holding())
        theHold.Put(new RealWorldScaleRecord<ChCylinderObject>(this, curState));
    ::SetUsePhysicalScaleUVs(this, flag);
    if (pblock2 != NULL)
		pblock2->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	UpdateUI();
	macroRec->SetProperty(this, _T("realWorldMapSize"), mr_bool, flag);
}

