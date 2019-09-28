/**********************************************************************
 *<
	FILE: cone.cpp

	DESCRIPTION:  Cone object

 *>	Copyright (c) 1994, All Rights Reserved.c
 **********************************************************************/

#include "prim.h"

#include "polyobj.h"
#include "iparamm2.h"
#include "Simpobj.h"
#include "surf_api.h"
#include "macroRec.h"
#include "RealWorldMapUtils.h"
#include <ReferenceSaveManager.h>
#include <Util/StaticAssert.h>

enum ConeVersion {
	kConeOriginalVersion = 0,	// Buggy version using custom poly generator
	kConePolyFromMeshVersion,	// Fixed version, builds poly version from trimesh
};

class ConeObject : public SimpleObject2, public RealWorldMapSizeInterface {
	friend class ConeTypeInDlgProc;
	friend class ConeObjCreateCallBack;
public:
	// Class vars
	static IObjParam *ip;
	static bool typeinCreate;

	// Versioning
	int version;

	ConeObject(BOOL loading);

	// From Object
	int CanConvertToType(Class_ID obtype) override;
	Object* ConvertToType(TimeValue t, Class_ID obtype) override;
	void GetCollapseTypes(Tab<Class_ID> &clist, Tab<TSTR*> &nlist) override;
	BOOL IsParamSurface() override { return TRUE; }
	Point3 GetSurfacePoint(TimeValue t, float u, float v, Interval &iv) override;

	// From BaseObject
	CreateMouseCallBack* GetCreateMouseCallBack() override;
	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev) override;
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next) override;
	const TCHAR *GetObjectName() override { return GetString(IDS_RB_CONE); }
	BOOL HasUVW() override;
	void SetGenUVW(BOOL sw) override;

	// Animatable methods		
	void DeleteThis() override { delete this; }
	Class_ID ClassID() override { return Class_ID(CONE_CLASS_ID, 0); }

	// From ref
	RefTargetHandle Clone(RemapDir& remap) override;
	bool SpecifySaveReferences(ReferenceSaveManager& referenceSaveManager) override;
	IOResult Load(ILoad *iload) override;
	IOResult Save(ISave *isave) override;

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
			return (RealWorldMapSizeInterface*)this;
		BaseInterface* intf = SimpleObject2::GetInterface(id);
		if (intf)
			return intf;
		return FPMixinInterface::GetInterface(id);
	}

	// local
	Object* BuildPoly(TimeValue t);
};

// class variables for Cone class.
IObjParam *ConeObject::ip = NULL;
bool ConeObject::typeinCreate = false;

#define PBLOCK_REF_NO	 0

#define MIN_SEGMENTS		1
#define MAX_SEGMENTS		200

#define MIN_SIDES			3
#define MAX_SIDES			200

#define MIN_RADIUS		float(0)
#define MAX_RADIUS		float( 1.0E30)
#define MIN_HEIGHT		float(-1.0E30)
#define MAX_HEIGHT		float( 1.0E30)
#define MIN_PIESLICE		float(-1.0E30)
#define MAX_PIESLICE		float( 1.0E30)

#define DEF_SEGMENTS 	5
#define DEF_CAPSEGMENTS 1
#define DEF_SIDES			24

#define DEF_RADIUS		float(0.0)
#define DEF_HEIGHT		float(0.0)

// in prim.cpp  - The dll instance handle
extern HINSTANCE hInstance;

//--- ClassDescriptor and class vars ---------------------------------

static BOOL sInterfaceAdded = FALSE;

class ConeClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() override { return 1; }
	void *		Create(BOOL loading = FALSE) override
	{
		if (!sInterfaceAdded) {
			AddInterface(&gRealWorldMapSizeDesc);
			sInterfaceAdded = TRUE;
		}
		return new ConeObject(loading);
	}
	const TCHAR *	ClassName() override { return GetString(IDS_RB_CONE_CLASS); }
	SClass_ID		SuperClassID() override { return GEOMOBJECT_CLASS_ID; }
	Class_ID			ClassID() override { return Class_ID(CONE_CLASS_ID, 0); }
	const TCHAR* 	Category() override { return GetString(IDS_RB_PRIMITIVES); }
	const TCHAR*	InternalName() override { return _T("Cone"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() override { return hInstance; }			// returns owning module handle
};

static ConeClassDesc coneDesc;

ClassDesc* GetConeDesc() { return &coneDesc; }

// ParamBlockDesc2 IDs
enum paramblockdesc2_ids { cone_creation_type, cone_type_in, cone_params, };
enum cone_creation_type_param_ids { cone_create_meth, };
enum cone_type_in_param_ids { cone_ti_pos, cone_ti_radius1, cone_ti_radius2, cone_ti_height, };
enum cone_param_param_ids {
	cone_radius1 = CONE_RADIUS1, cone_radius2 = CONE_RADIUS2, cone_height = CONE_HEIGHT,
	cone_segs = CONE_SEGMENTS, cone_capsegs = CONE_CAPSEGMENTS, cone_sides = CONE_SIDES,
	cone_smooth = CONE_SMOOTH, cone_slice = CONE_SLICEON, cone_pieslice1 = CONE_PIESLICE1,
	cone_pieslice2 = CONE_PIESLICE2, cone_mapping = CONE_GENUVS,
};

namespace
{
	MaxSDK::Util::StaticAssert< (cone_params == CONE_PARAMBLOCK_ID) > validator;
}

namespace
{
	class MapCoords_Accessor : public PBAccessor
	{
		void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
		{
			if (owner) ((ConeObject*)owner)->UpdateUI();
		}
	};
	static MapCoords_Accessor mapCoords_Accessor;
}

// class creation type block
static ParamBlockDesc2 cone_crtype_blk(cone_creation_type, _T("ConeCreationType"), 0, &coneDesc, P_CLASS_PARAMS + P_AUTO_UI,
	//rollout
	IDD_CONEPARAM1, IDS_RB_CREATIONMETHOD, BEGIN_EDIT_CREATE, 0, NULL,
	// params
	cone_create_meth, _T("typeinCreationMethod"), TYPE_INT, 0, IDS_RB_CREATIONMETHOD,
		p_default, 1,
		p_range, 0, 1,
		p_ui, TYPE_RADIO, 2, IDC_CREATEDIAMETER, IDC_CREATERADIUS,
		p_end,
	p_end
	);

// class type-in block
static ParamBlockDesc2 cone_typein_blk(cone_type_in, _T("ConeTypeIn"), 0, &coneDesc, P_CLASS_PARAMS + P_AUTO_UI,
	//rollout
	IDD_CONEPARAM3, IDS_RB_KEYBOARDENTRY, BEGIN_EDIT_CREATE, APPENDROLL_CLOSED, NULL,
	// params
	cone_ti_pos, _T("typeInPos"), TYPE_POINT3, 0, IDS_RB_TYPEIN_POS,
		p_default, Point3(0, 0, 0),
		p_range, float(-1.0E30), float(1.0E30),
		p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_TI_POSX, IDC_TI_POSXSPIN, IDC_TI_POSY, IDC_TI_POSYSPIN, IDC_TI_POSZ, IDC_TI_POSZSPIN, SPIN_AUTOSCALE,
		p_end,
	cone_ti_radius1, _T("typeInRadius1"), TYPE_FLOAT, 0, IDS_RB_RADIUS1,
		p_default, DEF_RADIUS,
		p_range, MIN_RADIUS, MAX_RADIUS,
		p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_RADIUS1, IDC_RADSPINNER1, SPIN_AUTOSCALE,
		p_end,
	cone_ti_radius2, _T("typeInRadius2"), TYPE_FLOAT, 0, IDS_RB_RADIUS2,
		p_default, DEF_RADIUS,
		p_range, MIN_RADIUS, MAX_RADIUS,
		p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_RADIUS2, IDC_RADSPINNER2, SPIN_AUTOSCALE,
		p_end,
	cone_ti_height, _T("typeInHeight"), TYPE_FLOAT, 0, IDS_RB_HEIGHT,
		p_default, DEF_HEIGHT,
		p_range, MIN_HEIGHT, MAX_HEIGHT,
		p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_LENGTH, IDC_LENSPINNER, SPIN_AUTOSCALE,
		p_end,
	p_end
	);

// per instance cone block
static ParamBlockDesc2 cone_param_blk(cone_params, _T("ConeParameters"), 0, &coneDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_REF_NO,
	//rollout
	IDD_CONEPARAM2, IDS_RB_PARAMETERS, 0, 0, NULL,
	// params
	cone_radius1, _T("radius1"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_RADIUS1,
		p_default, DEF_RADIUS,
		p_ms_default, 15.0,
		p_range, MIN_RADIUS, MAX_RADIUS,
		p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_RADIUS1, IDC_RADSPINNER1, SPIN_AUTOSCALE,
		p_end,
	cone_radius2, _T("radius2"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_RADIUS2,
		p_default, DEF_RADIUS,
		p_ms_default, 0.0,
		p_range, MIN_RADIUS, MAX_RADIUS,
		p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_RADIUS2, IDC_RADSPINNER2, SPIN_AUTOSCALE,
		p_end,
	cone_height, _T("height"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_HEIGHT,
		p_default, DEF_HEIGHT,
		p_ms_default, 25.0,
		p_range, MIN_HEIGHT, MAX_HEIGHT,
		p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_LENGTH, IDC_LENSPINNER, SPIN_AUTOSCALE,
		p_end,
	cone_segs, _T("heightsegs"), TYPE_INT, P_ANIMATABLE, IDS_RB_HEIGHTSEGS,
		p_default, DEF_SEGMENTS,
		p_range, MIN_SEGMENTS, MAX_SEGMENTS,
		p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_SEGMENTS, IDC_SEGSPINNER, 0.1f,
		p_end,
	cone_capsegs, _T("capsegs"), TYPE_INT, P_ANIMATABLE, IDS_RB_CAPSEGMENTS,
		p_default, DEF_CAPSEGMENTS,
		p_range, MIN_SEGMENTS, MAX_SEGMENTS,
		p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_CAPSEGMENTS, IDC_CAPSEGSPINNER, 0.1f,
		p_end,
	cone_sides, _T("sides"), TYPE_INT, P_ANIMATABLE, IDS_RB_SIDES,
		p_default, DEF_SIDES,
		p_range, MIN_SIDES, MAX_SIDES,
		p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_SIDES, IDC_SIDESPINNER, 0.1f,
		p_end,
	cone_smooth, _T("smooth"), TYPE_BOOL, P_ANIMATABLE, IDS_RB_SMOOTH,
		p_default, TRUE,
		p_ui, TYPE_SINGLECHECKBOX, IDC_OBSMOOTH,
		p_end,
	cone_slice, _T("slice"), TYPE_BOOL, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_SLICEON,
		p_default, FALSE,
		p_ui, TYPE_SINGLECHECKBOX, IDC_SLICEON,
		p_enable_ctrls, 2, cone_pieslice1, cone_pieslice2,
		p_end,
	cone_pieslice1, _T("sliceFrom"), TYPE_ANGLE, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_SLICEFROM,
		p_default, 0.0,
		p_range, MIN_PIESLICE, MAX_PIESLICE,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_PIESLICE1, IDC_PIESLICESPIN1, 0.5f,
		p_end,
	cone_pieslice2, _T("sliceTo"), TYPE_ANGLE, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_SLICETO,
		p_default, 0.0,
		p_range, MIN_PIESLICE, MAX_PIESLICE,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_PIESLICE2, IDC_PIESLICESPIN2, 0.5f,
		p_end,
	cone_mapping, _T("mapCoords"), TYPE_BOOL, 0, IDS_RB_GENTEXCOORDS,
		p_default, TRUE,
		p_ms_default, FALSE,
		p_ui, TYPE_SINGLECHECKBOX, IDC_GENTEXTURE,
		p_accessor, &mapCoords_Accessor,
		p_end,
	p_end
	);

//--- Parameter block descriptors -------------------------------
static ParamBlockDescID descVer0[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0, cone_radius1 },
	{ TYPE_FLOAT, NULL, TRUE, 1, cone_radius2 },
	{ TYPE_FLOAT, NULL, TRUE, 2, cone_height },
	{ TYPE_INT, NULL, TRUE, 3, cone_segs },
	{ TYPE_INT, NULL, TRUE, 4, cone_sides },
	{ TYPE_INT, NULL, TRUE, 5, cone_smooth },
	{ TYPE_INT, NULL, TRUE, 6, cone_slice },
	{ TYPE_FLOAT, NULL, TRUE, 7, cone_pieslice1 },
	{ TYPE_FLOAT, NULL, TRUE, 8, cone_pieslice2 } };

static ParamBlockDescID descVer1[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0, cone_radius1 },
	{ TYPE_FLOAT, NULL, TRUE, 1, cone_radius2 },
	{ TYPE_FLOAT, NULL, TRUE, 2, cone_height },
	{ TYPE_INT, NULL, TRUE, 3, cone_segs },
	{ TYPE_INT, NULL, TRUE, 9, cone_capsegs },
	{ TYPE_INT, NULL, TRUE, 4, cone_sides },
	{ TYPE_INT, NULL, TRUE, 5, cone_smooth },
	{ TYPE_INT, NULL, TRUE, 6, cone_slice },
	{ TYPE_FLOAT, NULL, TRUE, 7, cone_pieslice1 },
	{ TYPE_FLOAT, NULL, TRUE, 8, cone_pieslice2 } };

static ParamBlockDescID descVer2[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0, cone_radius1 },
	{ TYPE_FLOAT, NULL, TRUE, 1, cone_radius2 },
	{ TYPE_FLOAT, NULL, TRUE, 2, cone_height },
	{ TYPE_INT, NULL, TRUE, 3, cone_segs },
	{ TYPE_INT, NULL, TRUE, 9, cone_capsegs },
	{ TYPE_INT, NULL, TRUE, 4, cone_sides },
	{ TYPE_INT, NULL, TRUE, 5, cone_smooth },
	{ TYPE_INT, NULL, TRUE, 6, cone_slice },
	{ TYPE_FLOAT, NULL, TRUE, 7, cone_pieslice1 },
	{ TYPE_FLOAT, NULL, TRUE, 8, cone_pieslice2 },
	{ TYPE_INT, NULL, FALSE, 10, cone_mapping } };

// Array of old versions
static ParamVersionDesc versions[] = {
	ParamVersionDesc(descVer0,9,0),
	ParamVersionDesc(descVer1,10,0),
	ParamVersionDesc(descVer2,11,2),
};
#define NUM_OLDVERSIONS	3

// ParamBlock data for SaveToPrevious support
#define PBLOCK_LENGTH	11
#define CURRENT_VERSION	2

//--- TypeInDlgProc --------------------------------
class ConeTypeInDlgProc : public ParamMap2UserDlgProc {
public:
	ConeObject *ob;

	ConeTypeInDlgProc(ConeObject *o) { ob = o; }
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	void DeleteThis() override { delete this; }
};

INT_PTR ConeTypeInDlgProc::DlgProc(
	TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_TI_CREATE: {
			if (cone_typein_blk.GetFloat(cone_ti_radius1) == 0.0) return TRUE;

			// We only want to set the value if the object is not in the scene.
			if (ob->TestAFlag(A_OBJ_CREATING)) {
				ob->pblock2->SetValue(cone_radius1, 0, cone_typein_blk.GetFloat(cone_ti_radius1));
				ob->pblock2->SetValue(cone_radius2, 0, cone_typein_blk.GetFloat(cone_ti_radius2));
				ob->pblock2->SetValue(cone_height, 0, cone_typein_blk.GetFloat(cone_ti_height));
			}
			else
				ConeObject::typeinCreate = true;

			Matrix3 tm(1);
			tm.SetTrans(cone_typein_blk.GetPoint3(cone_ti_pos));
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

//--- ParamDlgProc --------------------------------
class ConeParamDlgProc : public ParamMap2UserDlgProc {
public:
	ConeObject *so;
	HWND thishWnd;

	ConeParamDlgProc(ConeObject *s) { so = s; thishWnd = NULL; }
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	void DeleteThis() override { delete this; }
	void UpdateUI();
	BOOL GetRWSState();
};

BOOL ConeParamDlgProc::GetRWSState()
{
	BOOL check = IsDlgButtonChecked(thishWnd, IDC_REAL_WORLD_MAP_SIZE2);
	return check;
}

void ConeParamDlgProc::UpdateUI()
{
	if (!thishWnd) return;
	BOOL usePhysUVs = so->GetUsePhysicalScaleUVs();
	CheckDlgButton(thishWnd, IDC_REAL_WORLD_MAP_SIZE2, usePhysUVs);
	EnableWindow(GetDlgItem(thishWnd, IDC_REAL_WORLD_MAP_SIZE2), so->HasUVW());
}

INT_PTR ConeParamDlgProc::DlgProc(
	TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	thishWnd = hWnd;
	switch (msg) {
	case WM_INITDIALOG: {
		UpdateUI();
		break;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_REAL_WORLD_MAP_SIZE2: {
			BOOL check = IsDlgButtonChecked(hWnd, IDC_REAL_WORLD_MAP_SIZE2);
			theHold.Begin();
			so->SetUsePhysicalScaleUVs(check);
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			so->ip->RedrawViews(so->ip->GetTime());
			break;
		}
		}
		break;
	}
	return FALSE;
}

//--- Cone methods -------------------------------

ConeObject::ConeObject(BOOL loading) : version(kConePolyFromMeshVersion)
{
	coneDesc.MakeAutoParamBlocks(this);

	if (!loading && !GetPhysicalScaleUVsDisabled())
		SetUsePhysicalScaleUVs(true);
}

bool ConeObject::SpecifySaveReferences(ReferenceSaveManager& referenceSaveManager)
{
	// if saving to previous version that used pb1 instead of pb2...
	DWORD saveVersion = GetSavingVersion();
	if (saveVersion != 0 && saveVersion <= MAX_RELEASE_R19)
	{
		ProcessPB2ToPB1SaveToPrevious(this, pblock2, PBLOCK_REF_NO, descVer2, PBLOCK_LENGTH, CURRENT_VERSION);
	}
	return __super::SpecifySaveReferences(referenceSaveManager);
}

#define VERSION_CHUNK 0x100

IOResult ConeObject::Load(ILoad *iload)
{
	ULONG nb;
	IOResult res;
	ParamBlock2PLCB* plcb = new ParamBlock2PLCB(versions, NUM_OLDVERSIONS, &cone_param_blk, this, PBLOCK_REF_NO);
	iload->RegisterPostLoadCallback(plcb);
	// Set to original version when loading; new version files will contain a version chunk
	version = kConeOriginalVersion;
	while (IO_OK == (res = iload->OpenChunk()))
	{
		switch (iload->CurChunkID())
		{
		case VERSION_CHUNK:
			res = iload->Read(&version, sizeof(int), &nb);
			if (res != IO_OK)
				return res;
			break;
		}
		iload->CloseChunk();
	}
	return IO_OK;
}

IOResult ConeObject::Save(ISave *isave)
{
	IOResult result = IO_OK;
	ULONG nb;

	isave->BeginChunk(VERSION_CHUNK);
	result = isave->Write(&version, sizeof(int), &nb);
	isave->EndChunk();

	return result;
}

void ConeObject::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	__super::BeginEditParams(ip, flags, prev);
	this->ip = ip;
	// If this has been freshly created by type-in, set creation values:
	if (ConeObject::typeinCreate)
	{
		pblock2->SetValue(cone_radius1, 0, cone_typein_blk.GetFloat(cone_ti_radius1));
		pblock2->SetValue(cone_radius2, 0, cone_typein_blk.GetFloat(cone_ti_radius2));
		pblock2->SetValue(cone_height, 0, cone_typein_blk.GetFloat(cone_ti_height));
		ConeObject::typeinCreate = false;
	}

	// throw up all the appropriate auto-rollouts
	coneDesc.BeginEditParams(ip, this, flags, prev);
	// if in Create Panel, install a callback for the type in.
	if (flags & BEGIN_EDIT_CREATE)
		cone_typein_blk.SetUserDlgProc(new ConeTypeInDlgProc(this));
	// install a callback for the params.
	cone_param_blk.SetUserDlgProc(new ConeParamDlgProc(this));
}

void ConeObject::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	__super::EndEditParams(ip, flags, next);
	this->ip = NULL;
	coneDesc.EndEditParams(ip, this, flags, next);
}

// In cyl.cpp
extern void BuildCylinderMesh(Mesh &mesh,
	int segs, int smooth, int llsegs, int capsegs, int doPie,
	float radius1, float radius2, float height, float pie1, float pie2,
	int genUVs, int usePhysUVs);

extern void BuildCylinderPoly(MNMesh & mesh,
	int segs, int smooth, int lsegs, int capsegs, int doPie,
	float radius1, float radius2, float height, float pie1, float pie2,
	int genUVs, BOOL usePhysUVs);

BOOL ConeObject::HasUVW() {
	BOOL genUVs;
	Interval v;
	pblock2->GetValue(cone_mapping, 0, genUVs, v);
	return genUVs;
}

void ConeObject::SetGenUVW(BOOL sw) {
	if (sw == HasUVW()) return;
	pblock2->SetValue(cone_mapping, 0, sw);
}

Point3 ConeObject::GetSurfacePoint(
	TimeValue t, float u, float v, Interval &iv)
{
	float radius1, radius2, height;
	pblock2->GetValue(cone_radius1, t, radius1, iv);
	pblock2->GetValue(cone_radius2, t, radius2, iv);
	pblock2->GetValue(cone_height, t, height, iv);
	Point3 p;
	float sn = -(float)cos(u*TWOPI);
	float cs = (float)sin(u*TWOPI);
	p.x = (1.0f - v)*radius1*cs + v*radius2*cs;
	p.y = (1.0f - v)*radius1*sn + v*radius2*sn;
	p.z = height * v;
	return p;
}

Object *ConeObject::BuildPoly(TimeValue t) {
	int segs, smooth, llsegs, capsegs;
	float radius1, radius2, height, pie1, pie2;
	int doPie, genUVs;

	// Start the validity interval at forever and widdle it down.
	Interval tvalid = FOREVER;

	pblock2->GetValue(cone_sides, t, segs, tvalid);
	pblock2->GetValue(cone_segs, t, llsegs, tvalid);
	pblock2->GetValue(cone_capsegs, t, capsegs, tvalid);
	pblock2->GetValue(cone_smooth, t, smooth, tvalid);
	pblock2->GetValue(cone_slice, t, doPie, tvalid);
	pblock2->GetValue(cone_mapping, t, genUVs, tvalid);
	Interval gvalid = tvalid;
	pblock2->GetValue(cone_radius1, t, radius1, gvalid);
	pblock2->GetValue(cone_radius2, t, radius2, gvalid);
	pblock2->GetValue(cone_height, t, height, gvalid);
	pblock2->GetValue(cone_pieslice1, t, pie1, gvalid);
	pblock2->GetValue(cone_pieslice2, t, pie2, gvalid);
	LimitValue(radius1, MIN_RADIUS, MAX_RADIUS);
	LimitValue(radius2, MIN_RADIUS, MAX_RADIUS);
	LimitValue(height, MIN_HEIGHT, MAX_HEIGHT);
	LimitValue(llsegs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(segs, MIN_SIDES, MAX_SIDES);
	LimitValue(capsegs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(smooth, 0, 1);

	PolyObject *pobj = new PolyObject();
	// MAXX-29604: Poly creation code is buggy.  If new version, just create the trimesh version as usual and then convert that to MNMesh
	if (version != kConeOriginalVersion)
	{
		BuildCylinderMesh(mesh,
			segs, smooth, llsegs, capsegs, doPie,
			radius1, radius2, height, pie1, pie2, genUVs, GetUsePhysicalScaleUVs());
		pobj->GetMesh().SetFromTri(mesh);
		pobj->GetMesh().MakePolyMesh(0, FALSE);
	}
	else
	{
		MNMesh & mesh = pobj->GetMesh();
		BuildCylinderPoly(mesh, segs, smooth, llsegs, capsegs, doPie,
			radius1, radius2, height, pie1, pie2, genUVs, GetUsePhysicalScaleUVs());
	}
	pobj->SetChannelValidity(TOPO_CHAN_NUM, tvalid);
	pobj->SetChannelValidity(GEOM_CHAN_NUM, gvalid);
	return pobj;
}

void ConeObject::BuildMesh(TimeValue t)
{
	int segs, llsegs, smooth, capsegs;
	float radius1, radius2, height, pie1, pie2;
	int doPie, genUVs;

	// Start the validity interval at forever and widdle it down.
	ivalid = FOREVER;

	pblock2->GetValue(cone_sides, t, segs, ivalid);
	pblock2->GetValue(cone_segs, t, llsegs, ivalid);
	pblock2->GetValue(cone_capsegs, t, capsegs, ivalid);
	pblock2->GetValue(cone_radius1, t, radius1, ivalid);
	pblock2->GetValue(cone_radius2, t, radius2, ivalid);
	pblock2->GetValue(cone_height, t, height, ivalid);
	pblock2->GetValue(cone_smooth, t, smooth, ivalid);
	pblock2->GetValue(cone_pieslice1, t, pie1, ivalid);
	pblock2->GetValue(cone_pieslice2, t, pie2, ivalid);
	pblock2->GetValue(cone_slice, t, doPie, ivalid);
	pblock2->GetValue(cone_mapping, t, genUVs, ivalid);
	LimitValue(radius1, MIN_RADIUS, MAX_RADIUS);
	LimitValue(radius2, MIN_RADIUS, MAX_RADIUS);
	LimitValue(height, MIN_HEIGHT, MAX_HEIGHT);
	LimitValue(llsegs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(segs, MIN_SIDES, MAX_SIDES);
	LimitValue(capsegs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(smooth, 0, 1);
	BOOL usePhysUVs = GetUsePhysicalScaleUVs();

	BuildCylinderMesh(mesh,
		segs, smooth, llsegs, capsegs, doPie,
		radius1, radius2, height, pie1, pie2, genUVs, usePhysUVs);
}

// In cyl,cpp
extern void BuildCylinderPatch(
	TimeValue t, PatchMesh &patch,
	float radius1, float radius2, float height, int genUVs, BOOL usePhysUVs);

Object *
BuildNURBSCone(float radius1, float radius2, float height, BOOL sliceon, float pie1, float pie2, BOOL genUVs)
{
	BOOL flip = FALSE;

	if (radius1 == 0.0f)
		radius1 = 0.001f;
	if (radius2 == 0.0f)
		radius2 = 0.001f;

	if (height < 0.0f)
		flip = !flip;

	NURBSSet nset;

	Point3 origin(0, 0, 0);
	Point3 symAxis(0, 0, 1);
	Point3 refAxis(0, 1, 0);

	float startAngle = 0.0f;
	float endAngle = TWOPI;
	if (sliceon && pie1 != pie2) {
		float sweep = TWOPI - (pie2 - pie1);
		if (sweep > TWOPI) sweep -= TWOPI;
		refAxis = Point3(Point3(1, 0, 0) * RotateZMatrix(pie2));
		endAngle = sweep;
	}


	// first the main surface
	NURBSCVSurface *surf = new NURBSCVSurface();
	nset.AppendObject(surf);
	surf->SetGenerateUVs(genUVs);

	surf->SetTextureUVs(0, 0, Point2(0.0f, 0.0f));
	surf->SetTextureUVs(0, 1, Point2(0.0f, 1.0f));
	surf->SetTextureUVs(0, 2, Point2(1.0f, 0.0f));
	surf->SetTextureUVs(0, 3, Point2(1.0f, 1.0f));

	surf->FlipNormals(!flip);
	surf->Renderable(TRUE);
	TCHAR bname[80];
	TCHAR sname[80];
	_tcscpy(bname, GetString(IDS_RB_CONE));
	_stprintf(sname, _T("%s%s"), bname, GetString(IDS_CT_SURF));

	if (sliceon && pie1 != pie2) {
		surf->SetName(sname);
		GenNURBSConeSurface(radius1, radius2, height, origin, symAxis, refAxis,
			startAngle, endAngle, TRUE, *surf);

#define F(s1, s2, s1r, s1c, s2r, s2c) \
		fuse.mSurf1 = (s1); \
		fuse.mSurf2 = (s2); \
		fuse.mRow1 = (s1r); \
		fuse.mCol1 = (s1c); \
		fuse.mRow2 = (s2r); \
		fuse.mCol2 = (s2c); \
		nset.mSurfFuse.Append(1, &fuse);

		NURBSFuseSurfaceCV fuse;

		NURBSCVSurface *s0 = (NURBSCVSurface*)nset.GetNURBSObject(0);

		Point3 cen;

		// next the two caps
		for (int c = 0; c < 2; c++) {
			if (c == 0) {
				cen = Point3(0, 0, 0);
			}
			else {
				cen = Point3(0.0f, 0.0f, height);
			}
			NURBSCVSurface *s = new NURBSCVSurface();
			nset.AppendObject(s);
			// we'll be cubic in on direction and match the sphere in the other
			s->SetUOrder(4);
			s->SetNumUKnots(8);
			for (int i = 0; i < 4; i++) {
				s->SetUKnot(i, 0.0);
				s->SetUKnot(i + 4, 1.0);
			}

			s->SetVOrder(s0->GetVOrder());
			s->SetNumVKnots(s0->GetNumVKnots());
			for (int i = 0; i < s->GetNumVKnots(); i++)
				s->SetVKnot(i, s0->GetVKnot(i));

			int numU, numV;
			s0->GetNumCVs(numU, numV);
			s->SetNumCVs(4, numV);

			for (int v = 0; v < numV; v++) {
				Point3 edge;
				if (c == 0)
					edge = s0->GetCV(0, v)->GetPosition(0);
				else
					edge = s0->GetCV(numU - 1, v)->GetPosition(0);
				double w = s0->GetCV(0, v)->GetWeight(0);
				for (int u = 0; u < 4; u++) {
					NURBSControlVertex ncv;
					ncv.SetPosition(0, cen + ((edge - cen)*((float)u / 3.0f)));
					ncv.SetWeight(0, w);
					s->SetCV(u, v, ncv);
				}
			}
			s->SetGenerateUVs(genUVs);

			s->SetTextureUVs(0, 0, Point2(1.0f, 1.0f));
			s->SetTextureUVs(0, 1, Point2(0.0f, 1.0f));
			s->SetTextureUVs(0, 2, Point2(1.0f, 0.0f));
			s->SetTextureUVs(0, 3, Point2(0.0f, 0.0f));

			if (c == 0)
				s->FlipNormals(!flip);
			else
				s->FlipNormals(flip);
			s->Renderable(TRUE);
			_stprintf(sname, _T("%s%s%02d"), bname, GetString(IDS_CT_CAP), c + 1);
			s->SetName(sname);
		}

		NURBSCVSurface *s1 = (NURBSCVSurface *)nset.GetNURBSObject(1);
		NURBSCVSurface *s2 = (NURBSCVSurface *)nset.GetNURBSObject(2);

		// next the two pie slices
		for (int c = 0; c < 2; c++) {
			if (c == 0)
				cen = Point3(0, 0, 0);
			else
				cen = Point3(0.0f, 0.0f, height);
			NURBSCVSurface *s = new NURBSCVSurface();
			nset.AppendObject(s);
			// we'll match the cylinder in on dimention and the caps in the other.
			s->SetUOrder(s0->GetUOrder());
			int numKnots = s0->GetNumUKnots();
			s->SetNumUKnots(numKnots);
			for (int i = 0; i < numKnots; i++)
				s->SetUKnot(i, s0->GetUKnot(i));

			s->SetVOrder(s1->GetUOrder());
			numKnots = s1->GetNumUKnots();
			s->SetNumVKnots(numKnots);
			for (int i = 0; i < numKnots; i++)
				s->SetVKnot(i, s1->GetUKnot(i));

			int s0u, s0v, s1u, s1v, s2u, s2v;
			s0->GetNumCVs(s0u, s0v);
			s1->GetNumCVs(s1u, s1v);
			s2->GetNumCVs(s2u, s2v);
			int uNum = s0u, vNum = s1u;
			s->SetNumCVs(uNum, vNum);
			for (int v = 0; v < vNum; v++) {
				for (int u = 0; u < uNum; u++) {
					// we get get the ends from the caps and the edge from the main sheet
					if (u == 0) {  // bottom
						if (c == 0) {
							s->SetCV(u, v, *s1->GetCV(v, 0));
							F(1, 3, v, 0, u, v);
						}
						else {
							s->SetCV(u, v, *s1->GetCV(v, s1v - 1));
							F(1, 4, v, s1v - 1, u, v);
						}
					}
					else if (u == uNum - 1) { // top
						if (c == 0) {
							s->SetCV(u, v, *s2->GetCV(v, 0));
							F(2, 3, v, 0, u, v);
						}
						else {
							s->SetCV(u, v, *s2->GetCV(v, s2v - 1));
							F(2, 4, v, s2v - 1, u, v);
						}
					}
					else { // middle
						if (v == vNum - 1) { // outer edge
							if (c == 0) {
								s->SetCV(u, v, *s0->GetCV(u, 0));
								F(0, 3, u, 0, u, v);
							}
							else {
								s->SetCV(u, v, *s0->GetCV(u, s0v - 1));
								F(0, 4, u, s0v - 1, u, v);
							}
						}
						else { // inside
							float angle;
							if (c == 0) angle = pie2;
							else angle = pie1;
							float hrad1 = radius1 * (float)v / (float)(vNum - 1);
							float hrad2 = radius2 * (float)v / (float)(vNum - 1);
							float rad = hrad1 + ((hrad2 - hrad1) * (float)u / (float)(uNum - 1));
							NURBSControlVertex ncv;
							ncv.SetPosition(0, Point3(rad, 0.0f, height * (float)u / (float)(uNum - 1)) * RotateZMatrix(angle));
							ncv.SetWeight(0, 1.0f);
							s->SetCV(u, v, ncv);
						}
					}
				}
			}
			s->SetGenerateUVs(genUVs);

			s->SetTextureUVs(0, 0, Point2(0.0f, 0.0f));
			s->SetTextureUVs(0, 1, Point2(0.0f, 1.0f));
			s->SetTextureUVs(0, 2, Point2(1.0f, 0.0f));
			s->SetTextureUVs(0, 3, Point2(1.0f, 1.0f));

			if (c == 0)
				s->FlipNormals(!flip);
			else
				s->FlipNormals(flip);
			s->Renderable(TRUE);
			_stprintf(sname, _T("%s%s%02d"), bname, GetString(IDS_CT_SLICE), c + 1);
			s->SetName(sname);
		}

		NURBSCVSurface *s3 = (NURBSCVSurface *)nset.GetNURBSObject(3);
		NURBSCVSurface *s4 = (NURBSCVSurface *)nset.GetNURBSObject(4);

		// now fuse up the rest
		// Fuse the edges
		for (int v = 0; v < s0->GetNumVCVs(); v++) {
			F(0, 1, 0, v, s1->GetNumUCVs() - 1, v);
			F(0, 2, s0->GetNumUCVs() - 1, v, s2->GetNumUCVs() - 1, v);
		}

		// Fuse the cap centers
		for (int v = 1; v < s1->GetNumVCVs(); v++) {
			F(1, 1, 0, 0, 0, v);
			F(2, 2, 0, 0, 0, v);
		}

		// Fuse the core
		for (int u = 0; u < s3->GetNumUCVs(); u++) {
			F(3, 4, u, 0, u, 0);
		}
	}
	else {
		GenNURBSConeSurface(radius1, radius2, height, origin, symAxis, refAxis,
			startAngle, endAngle, FALSE, *surf);

		// now create caps on the ends
		if (radius1 != 0.0) {
			NURBSCapSurface *cap0 = new NURBSCapSurface();
			nset.AppendObject(cap0);
			cap0->SetGenerateUVs(genUVs);
			cap0->SetParent(0);
			cap0->SetEdge(0);
			cap0->FlipNormals(!flip);
			cap0->Renderable(TRUE);
			TCHAR sname[80];
			_stprintf(sname, _T("%s%s%02d"), bname, GetString(IDS_CT_CAP), 0);
			cap0->SetName(sname);
		}

		if (radius2 != 0.0) {
			NURBSCapSurface *cap1 = new NURBSCapSurface();
			nset.AppendObject(cap1);
			cap1->SetGenerateUVs(genUVs);
			cap1->SetParent(0);
			cap1->SetEdge(1);
			cap1->FlipNormals(flip);
			cap1->Renderable(TRUE);
			_stprintf(sname, _T("%s%s%02d"), bname, GetString(IDS_CT_CAP), 1);
			cap1->SetName(sname);
		}
	}

	Matrix3 mat;
	mat.IdentityMatrix();
	Object *ob = CreateNURBSObject(NULL, &nset, mat);
	return ob;
}

Object* ConeObject::ConvertToType(TimeValue t, Class_ID obtype)
{
	if (obtype == patchObjectClassID) {
		Interval valid = FOREVER;
		float radius1, radius2, height;
		int genUVs;
		pblock2->GetValue(cone_radius1, t, radius1, valid);
		pblock2->GetValue(cone_radius2, t, radius2, valid);
		pblock2->GetValue(cone_height, t, height, valid);
		pblock2->GetValue(cone_mapping, t, genUVs, valid);
		if (radius1 < 0.0f) radius1 = 0.0f;
		if (radius2 < 0.0f) radius2 = 0.0f;
		PatchObject *ob = new PatchObject();
		BuildCylinderPatch(t, ob->patch, radius1, radius2, height, genUVs, GetUsePhysicalScaleUVs());
		ob->SetChannelValidity(TOPO_CHAN_NUM, valid);
		ob->SetChannelValidity(GEOM_CHAN_NUM, valid);
		ob->UnlockObject();
		return ob;
	}

	if (obtype == polyObjectClassID) {
		Object *ob = BuildPoly(t);
		ob->UnlockObject();
		return ob;
	}

	if (obtype == EDITABLE_SURF_CLASS_ID) {
		Interval valid = FOREVER;
		float radius1, radius2, height, pie1, pie2;
		int sliceon, genUVs;
		pblock2->GetValue(cone_radius1, t, radius1, valid);
		pblock2->GetValue(cone_radius2, t, radius2, valid);
		pblock2->GetValue(cone_height, t, height, valid);
		pblock2->GetValue(cone_pieslice1, t, pie1, valid);
		pblock2->GetValue(cone_pieslice2, t, pie2, valid);
		pblock2->GetValue(cone_slice, t, sliceon, valid);
		pblock2->GetValue(cone_mapping, t, genUVs, valid);
		if (radius1 < 0.0f) radius1 = 0.0f;
		if (radius2 < 0.0f) radius2 = 0.0f;
		Object *ob = BuildNURBSCone(radius1, radius2, height, sliceon, pie1, pie2, genUVs);
		ob->SetChannelValidity(TOPO_CHAN_NUM, valid);
		ob->SetChannelValidity(GEOM_CHAN_NUM, valid);
		ob->UnlockObject();
		return ob;
	}
	return __super::ConvertToType(t, obtype);
}

int ConeObject::CanConvertToType(Class_ID obtype)
{
	if (obtype == defObjectClassID || obtype == mapObjectClassID || obtype == triObjectClassID)
		return 1;

	if (obtype == patchObjectClassID) return 1;
	if (obtype == EDITABLE_SURF_CLASS_ID) return 1;
	if (obtype == polyObjectClassID) return 1;

	return __super::CanConvertToType(obtype);
}

void ConeObject::GetCollapseTypes(Tab<Class_ID> &clist, Tab<TSTR*> &nlist)
{
	__super::GetCollapseTypes(clist, nlist);
	Class_ID id = EDITABLE_SURF_CLASS_ID;
	TSTR *name = new TSTR(GetString(IDS_SM_NURBS_SURFACE));
	clist.Append(1, &id);
	nlist.Append(1, &name);
}

class ConeObjCreateCallBack : public CreateMouseCallBack {
	ConeObject *ob;
	Point3 p[4];
	IPoint2 sp0, sp1, sp2, sp3;
	float r1;
public:
	int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat) override;
	void SetObj(ConeObject *obj) { ob = obj; }
};

int ConeObjCreateCallBack::proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat) {

	if (!vpt || !vpt->IsAlive())
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	float r;

	if (msg == MOUSE_FREEMOVE)
	{
		vpt->SnapPreview(m, m, NULL, SNAP_IN_3D);
	}

	if (msg == MOUSE_POINT || msg == MOUSE_MOVE) {
		switch (point) {
		case 0:
			ob->suspendSnap = TRUE;
			sp0 = m;
			p[0] = vpt->SnapPoint(m, m, NULL, SNAP_IN_3D);
			mat.SetTrans(p[0]); // Set Node's transform				
			ob->pblock2->SetValue(cone_radius1, 0, 0.01f);
			ob->pblock2->SetValue(cone_radius2, 0, 0.01f);
			ob->pblock2->SetValue(cone_height, 0, 0.01f);
			break;
		case 1:
		{
			mat.IdentityMatrix();
			//mat.PreRotateZ(HALFPI);
			sp1 = m;
			p[1] = vpt->SnapPoint(m, m, NULL, SNAP_IN_3D);
			bool createByRadius = cone_crtype_blk.GetInt(cone_create_meth) == 1;
			if (createByRadius) {
				// radius	
				r = Length(p[1] - p[0]);
				mat.SetTrans(p[0]);
			}
			else {
				// diameter
				Point3 center = (p[0] + p[1]) / float(2);
				r = Length(center - p[0]);
				mat.SetTrans(center);  // Modify Node's transform
			}

			ob->pblock2->SetValue(cone_radius1, 0, r);
			ob->pblock2->SetValue(cone_radius2, 0, r + 1.0f);
			r1 = r;

			if (flags&MOUSE_CTRL) {
				float ang = (float)atan2(p[1].y - p[0].y, p[1].x - p[0].x);
				mat.PreRotateZ(ob->ip->SnapAngle(ang));
			}

			if (msg == MOUSE_POINT) {
				if (Length(m - sp0) < 3 ) {
					return CREATE_ABORT;
				}
			}
			break;
		}
		case 2:
		{
			sp2 = m;
			float h = vpt->SnapLength(vpt->GetCPDisp(p[1], Point3(0, 0, 1), sp1, m, TRUE));
			ob->pblock2->SetValue(cone_height, 0, h);
		}
		break;

		case 3:
			r = vpt->SnapLength(vpt->GetCPDisp(p[1], Point3(0, 0, 1), sp2, m))
				+ r1;
			ob->pblock2->SetValue(cone_radius2, 0, r);

			if (msg == MOUSE_POINT) {
				ob->suspendSnap = FALSE;
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

static ConeObjCreateCallBack coneCreateCB;

CreateMouseCallBack* ConeObject::GetCreateMouseCallBack()
{
	coneCreateCB.SetObj(this);
	return(&coneCreateCB);
}

BOOL ConeObject::OKtoDisplay(TimeValue t)
{
	float radius1=0.f, radius2=0.f;
	pblock2->GetValue(cone_radius1, t, radius1, FOREVER);
	pblock2->GetValue(cone_radius2, t, radius2, FOREVER);
	if (radius1 == 0.0f && radius2 == 0.0f) return FALSE;
	else return TRUE;
}

void ConeObject::InvalidateUI()
{
	cone_param_blk.InvalidateUI(pblock2->LastNotifyParamID());
}

RefTargetHandle ConeObject::Clone(RemapDir& remap)
{
	ConeObject* newob = new ConeObject(FALSE);
	newob->ReplaceReference(0, remap.CloneRef(pblock2));
	newob->ivalid.SetEmpty();
	BaseClone(this, newob, remap);
	newob->SetUsePhysicalScaleUVs(GetUsePhysicalScaleUVs());
	newob->version = version;
	return(newob);
}

void ConeObject::UpdateUI()
{
	if (ip == nullptr)
		return;
	ConeParamDlgProc* dlg = static_cast<ConeParamDlgProc*>(cone_param_blk.GetUserDlgProc());
	dlg->UpdateUI();
}

BOOL ConeObject::GetUsePhysicalScaleUVs()
{
	return ::GetUsePhysicalScaleUVs(this);
}

void ConeObject::SetUsePhysicalScaleUVs(BOOL flag)
{
	BOOL curState = GetUsePhysicalScaleUVs();
	if (curState == flag)
		return;
	if (theHold.Holding())
		theHold.Put(new RealWorldScaleRecord<ConeObject>(this, curState));
	::SetUsePhysicalScaleUVs(this, flag);
	if (pblock2 != NULL)
		pblock2->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	UpdateUI();
	macroRec->SetProperty(this, _T("realWorldMapSize"), mr_bool, flag);
}

