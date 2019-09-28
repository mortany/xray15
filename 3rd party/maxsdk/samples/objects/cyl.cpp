/**********************************************************************
 *<
	FILE: cyl.cpp

	DESCRIPTION:  Cylinder object, Revised implementation

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#include "prim.h"

#include "iparamm2.h"
#include "Simpobj.h"
#include "surf_api.h"
#include "PolyObj.h"
#include "macroRec.h"
#include "RealWorldMapUtils.h"
#include <ReferenceSaveManager.h>
#include <Util/StaticAssert.h>

enum CylinderVersion {
	kCylinderOriginalVersion = 0,	// Buggy version using custom poly generator
	kCylinderPolyFromMeshVersion,	// Fixed version, builds poly version from trimesh
};

class CylinderObject : public GenCylinder, public RealWorldMapSizeInterface {
	friend class CylTypeInDlgProc;
	friend class CylinderObjCreateCallBack;
public:
	// Class vars
	static IObjParam *ip;
	static bool typeinCreate;

	// Versioning
	int version;

	CylinderObject(BOOL loading);

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
	const TCHAR *GetObjectName() override { return GetString(IDS_RB_CYLINDER); }
	BOOL HasUVW() override;
	void SetGenUVW(BOOL sw) override;

	// Animatable methods		
	void DeleteThis() override { delete this; }
	Class_ID ClassID() override { return Class_ID(CYLINDER_CLASS_ID, 0); }

	// From ref
	RefTargetHandle Clone(RemapDir& remap) override;
	bool SpecifySaveReferences(ReferenceSaveManager& referenceSaveManager) override;
	IOResult Load(ILoad *iload) override;
	IOResult Save(ISave *isave);

	// From SimpleObjectBase
	void BuildMesh(TimeValue t) override;
	BOOL OKtoDisplay(TimeValue t) override;
	void InvalidateUI() override;

	// From GenCylinder
	void SetParams(float rad, float height, int segs, int sides, int capsegs = 1, BOOL smooth = TRUE,
		BOOL genUV = TRUE, BOOL sliceOn = FALSE, float slice1 = 0.0f, float slice2 = 0.0f) override;

	// Get/Set the UsePhyicalScaleUVs flag.
	BOOL GetUsePhysicalScaleUVs() override;
	void SetUsePhysicalScaleUVs(BOOL flag) override;
	void UpdateUI();

	//From FPMixinInterface
	BaseInterface* GetInterface(Interface_ID id) override;

	// local
	Object *BuildPoly(TimeValue t);

};

// class variables for Cylinder class.
IObjParam *CylinderObject::ip = NULL;
bool CylinderObject::typeinCreate = false;

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
#define DEF_SIDES			18

#define DEF_RADIUS		float(0.0)
#define DEF_HEIGHT		float(0.01)

// in prim.cpp  - The dll instance handle
extern HINSTANCE hInstance;

//--- ClassDescriptor and class vars ---------------------------------
static BOOL sInterfaceAdded = FALSE;

class CylClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() override { return 1; }
	void *		Create(BOOL loading = FALSE) override
	{
		if (!sInterfaceAdded) {
			AddInterface(&gRealWorldMapSizeDesc);
			sInterfaceAdded = TRUE;
		}
		return new CylinderObject(loading);
	}
	const TCHAR *	ClassName() override { return GetString(IDS_RB_CYLINDER_CLASS); }
	SClass_ID		SuperClassID() override { return GEOMOBJECT_CLASS_ID; }
	Class_ID			ClassID() override { return Class_ID(CYLINDER_CLASS_ID, 0); }
	const TCHAR* 	Category() override { return GetString(IDS_RB_PRIMITIVES); }
	const TCHAR*	InternalName() override { return _T("Cylinder"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() override { return hInstance; }			// returns owning module handle
};

static CylClassDesc cylDesc;

ClassDesc* GetCylinderDesc() { return &cylDesc; }

// ParamBlockDesc2 IDs
enum paramblockdesc2_ids { cyl_creation_type, cyl_type_in, cyl_params, };
enum cyl_creation_type_param_ids { cyl_create_meth, };
enum cyl_type_in_param_ids { cyl_ti_pos, cyl_ti_radius, cyl_ti_height, };
enum cyl_param_param_ids {
	cyl_radius = CYLINDER_RADIUS, cyl_height = CYLINDER_HEIGHT,
	cyl_segs = CYLINDER_SEGMENTS, cyl_capsegs = CYLINDER_CAPSEGMENTS, cyl_sides = CYLINDER_SIDES,
	cyl_smooth = CYLINDER_SMOOTH, cyl_slice = CYLINDER_SLICEON, cyl_pieslice1 = CYLINDER_PIESLICE1,
	cyl_pieslice2 = CYLINDER_PIESLICE2, cyl_mapping = CYLINDER_GENUVS,
};

namespace
{
	MaxSDK::Util::StaticAssert< (cyl_params == CYLINDER_PARAMBLOCK_ID) > validator;
}

namespace
{
	class MapCoords_Accessor : public PBAccessor
	{
		void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
		{
			if (owner) ((CylinderObject*)owner)->UpdateUI();
		}
	};
	static MapCoords_Accessor mapCoords_Accessor;
}

// class creation type block
static ParamBlockDesc2 cyl_crtype_blk(cyl_creation_type, _T("CylinderCreationType"), 0, &cylDesc, P_CLASS_PARAMS + P_AUTO_UI,
	//rollout
	IDD_CYLINDERPARAM1, IDS_RB_CREATIONMETHOD, BEGIN_EDIT_CREATE, 0, NULL,
	// params
	cyl_create_meth, _T("typeinCreationMethod"), TYPE_INT, 0, IDS_RB_CREATIONMETHOD,
		p_default, 1,
		p_range, 0, 1,
		p_ui, TYPE_RADIO, 2, IDC_CREATEDIAMETER, IDC_CREATERADIUS,
		p_end,
	p_end
	);

// class type-in block
static ParamBlockDesc2 cyl_typein_blk(cyl_type_in, _T("CylinderTypeIn"), 0, &cylDesc, P_CLASS_PARAMS + P_AUTO_UI,
	//rollout
	IDD_CYLINDERPARAM3, IDS_RB_KEYBOARDENTRY, BEGIN_EDIT_CREATE, APPENDROLL_CLOSED, NULL,
	// params
	cyl_ti_pos, _T("typeInPos"), TYPE_POINT3, 0, IDS_RB_TYPEIN_POS,
		p_default, Point3(0, 0, 0),
		p_range, float(-1.0E30), float(1.0E30),
		p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_TI_POSX, IDC_TI_POSXSPIN, IDC_TI_POSY, IDC_TI_POSYSPIN, IDC_TI_POSZ, IDC_TI_POSZSPIN, SPIN_AUTOSCALE,
		p_end,
	cyl_ti_radius, _T("typeInRadius"), TYPE_FLOAT, 0, IDS_RB_RADIUS,
		p_default, DEF_RADIUS,
		p_range, MIN_RADIUS, MAX_RADIUS,
		p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_RADIUS, IDC_RADSPINNER, SPIN_AUTOSCALE,
		p_end,
	cyl_ti_height, _T("typeInHeight"), TYPE_FLOAT, 0, IDS_RB_HEIGHT,
		p_default, DEF_HEIGHT,
		p_range, MIN_HEIGHT, MAX_HEIGHT,
		p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_LENGTH, IDC_LENSPINNER, SPIN_AUTOSCALE,
		p_end,
	p_end
	);

// per instance Cylinder block
static ParamBlockDesc2 cyl_param_blk(cyl_params, _T("CylinderParameters"), 0, &cylDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_REF_NO,
	//rollout
	IDD_CYLINDERPARAM2, IDS_RB_PARAMETERS, 0, 0, NULL,
	// params
	cyl_radius, _T("radius"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_RADIUS,
		p_default, DEF_RADIUS,
		p_ms_default, 15.0,
		p_range, MIN_RADIUS, MAX_RADIUS,
		p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_RADIUS, IDC_RADSPINNER, SPIN_AUTOSCALE,
		p_end,
	cyl_height, _T("height"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_HEIGHT,
		p_default, DEF_HEIGHT,
		p_ms_default, 25.0,
		p_range, MIN_HEIGHT, MAX_HEIGHT,
		p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_LENGTH, IDC_LENSPINNER, SPIN_AUTOSCALE,
		p_end,
	cyl_segs, _T("heightsegs"), TYPE_INT, P_ANIMATABLE, IDS_RB_CIRCLESEGMENTS,
		p_default, DEF_SEGMENTS,
		p_ms_default, 1,
		p_range, MIN_SEGMENTS, MAX_SEGMENTS,
		p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_SEGMENTS, IDC_SEGSPINNER, 0.1f,
		p_end,
	cyl_capsegs, _T("capsegs"), TYPE_INT, P_ANIMATABLE, IDS_RB_CAPSEGMENTS,
		p_default, DEF_CAPSEGMENTS, 
		p_range, MIN_SEGMENTS, MAX_SEGMENTS,
		p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_CAPSEGMENTS, IDC_CAPSEGSPIN, 0.1f,
		p_end,
	cyl_sides, _T("sides"), TYPE_INT, P_ANIMATABLE, IDS_RB_SIDES,
		p_default, DEF_SIDES,
		p_ms_default, 24,
		p_range, MIN_SIDES, MAX_SIDES,
		p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_SIDES, IDC_SIDESPINNER, 0.1f,
		p_end,
	cyl_smooth, _T("smooth"), TYPE_BOOL, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_SMOOTH,
		p_default, TRUE,
		p_ui, TYPE_SINGLECHECKBOX, IDC_OBSMOOTH,
		p_end,
	cyl_slice, _T("slice"), TYPE_BOOL, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_SLICEON,
		p_default, FALSE,
		p_ui, TYPE_SINGLECHECKBOX, IDC_SLICEON,
		p_enable_ctrls, 2, cyl_pieslice1, cyl_pieslice2,
		p_end,
	cyl_pieslice1, _T("sliceFrom"), TYPE_ANGLE, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_SLICEFROM,
		p_default, 0.0,
		p_range, MIN_PIESLICE, MAX_PIESLICE,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_PIESLICE1, IDC_PIESLICESPIN1, 0.5f,
		p_end,
	cyl_pieslice2, _T("sliceTo"), TYPE_ANGLE, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_SLICETO,
		p_default, 0.0,
		p_range, MIN_PIESLICE, MAX_PIESLICE,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_PIESLICE2, IDC_PIESLICESPIN2, 0.5f,
		p_end,
	cyl_mapping, _T("mapCoords"), TYPE_BOOL, 0, IDS_RB_GENTEXCOORDS,
		p_default, TRUE,
		p_ms_default, FALSE,
		p_ui, TYPE_SINGLECHECKBOX, IDC_GENTEXTURE,
		p_accessor, &mapCoords_Accessor,
		p_end,
	p_end
	);

//--- Parameter block descriptors -------------------------------
static ParamBlockDescID descVer0[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0, cyl_radius },
	{ TYPE_FLOAT, NULL, TRUE, 1, cyl_height },
	{ TYPE_INT, NULL, TRUE, 2, cyl_segs },
	{ TYPE_INT, NULL, TRUE, 3, cyl_sides },
	{ TYPE_INT, NULL, TRUE, 4, cyl_smooth } };

static ParamBlockDescID descVer1[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0, cyl_radius },
	{ TYPE_FLOAT, NULL, TRUE, 1, cyl_height },
	{ TYPE_INT, NULL, TRUE, 2, cyl_segs },
	{ TYPE_INT, NULL, TRUE, 3, cyl_sides },
	{ TYPE_INT, NULL, TRUE, 4, cyl_smooth },
	{ TYPE_INT, NULL, TRUE, 5, cyl_slice },
	{ TYPE_FLOAT, NULL, TRUE, 6, cyl_pieslice1 },
	{ TYPE_FLOAT, NULL, TRUE, 7, cyl_pieslice2 } };

static ParamBlockDescID descVer2[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0, cyl_radius },
	{ TYPE_FLOAT, NULL, TRUE, 1, cyl_height },
	{ TYPE_INT, NULL, TRUE, 2, cyl_segs },
	{ TYPE_INT, NULL, TRUE, 8, cyl_capsegs },
	{ TYPE_INT, NULL, TRUE, 3, cyl_sides },
	{ TYPE_INT, NULL, TRUE, 4, cyl_smooth },
	{ TYPE_INT, NULL, TRUE, 5, cyl_slice },
	{ TYPE_FLOAT, NULL, TRUE, 6, cyl_pieslice1 },
	{ TYPE_FLOAT, NULL, TRUE, 7, cyl_pieslice2 } };

static ParamBlockDescID descVer3[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0, cyl_radius },
	{ TYPE_FLOAT, NULL, TRUE, 1, cyl_height },
	{ TYPE_INT, NULL, TRUE, 2, cyl_segs },
	{ TYPE_INT, NULL, TRUE, 8, cyl_capsegs },
	{ TYPE_INT, NULL, TRUE, 3, cyl_sides },
	{ TYPE_BOOL, NULL, TRUE, 4, cyl_smooth },
	{ TYPE_INT, NULL, TRUE, 5, cyl_slice },
	{ TYPE_FLOAT, NULL, TRUE, 6, cyl_pieslice1 },
	{ TYPE_FLOAT, NULL, TRUE, 7, cyl_pieslice2 },
	{ TYPE_INT, NULL, FALSE, 9, cyl_mapping } };

// Array of old versions
static ParamVersionDesc versions[] = {
	ParamVersionDesc(descVer0,5,0),
	ParamVersionDesc(descVer1,8,1),
	ParamVersionDesc(descVer2,9,2),
	ParamVersionDesc(descVer3,10,3)
};
#define NUM_OLDVERSIONS	4

// ParamBlock data for SaveToPrevious support
#define PBLOCK_LENGTH	10
#define CURRENT_VERSION	3

//--- TypeInDlgProc --------------------------------
class CylTypeInDlgProc : public ParamMap2UserDlgProc {
public:
	CylinderObject *ob;

	CylTypeInDlgProc(CylinderObject *o) { ob = o; }
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	void DeleteThis() override { delete this; }
};

INT_PTR CylTypeInDlgProc::DlgProc(
	TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_TI_CREATE: {
			if (cyl_typein_blk.GetFloat(cyl_ti_radius) == 0.0) return TRUE;

			// We only want to set the value if the object is 
			// not in the scene.
			if (ob->TestAFlag(A_OBJ_CREATING)) {
				ob->pblock2->SetValue(cyl_radius, 0, cyl_typein_blk.GetFloat(cyl_ti_radius));
				ob->pblock2->SetValue(cyl_height, 0, cyl_typein_blk.GetFloat(cyl_ti_height));
			}
			else
				CylinderObject::typeinCreate = true;

			Matrix3 tm(1);
			tm.SetTrans(cyl_typein_blk.GetPoint3(cyl_ti_pos));
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

class CylinderParamDlgProc : public ParamMap2UserDlgProc
{
private:
	CylinderObject* mCylinder;
	HWND thishWnd;
public:
	CylinderParamDlgProc(CylinderObject* s);
	INT_PTR DlgProc(TimeValue t, IParamMap2* map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	void DeleteThis() override;
	void UpdateUI();
	BOOL GetRWSState();
};

CylinderParamDlgProc::CylinderParamDlgProc(CylinderObject *s)
{
	mCylinder = s;
	thishWnd = NULL;
}

void CylinderParamDlgProc::DeleteThis()
{
	delete this;
}

BOOL CylinderParamDlgProc::GetRWSState()
{
	BOOL check = IsDlgButtonChecked(thishWnd, IDC_REAL_WORLD_MAP_SIZE2);
	return check;
}

void CylinderParamDlgProc::UpdateUI()
{
	if (!thishWnd)
		return;
	BOOL usePhysUVs = mCylinder->GetUsePhysicalScaleUVs();
	CheckDlgButton(thishWnd, IDC_REAL_WORLD_MAP_SIZE2, usePhysUVs);
	EnableWindow(GetDlgItem(thishWnd, IDC_REAL_WORLD_MAP_SIZE2), mCylinder->HasUVW());
}

INT_PTR CylinderParamDlgProc::DlgProc(
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
			mCylinder->SetUsePhysicalScaleUVs(check);
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			mCylinder->ip->RedrawViews(mCylinder->ip->GetTime());
			break;
		}
		}
		break;
	}
	return FALSE;
}

//--- Cylinder methods -------------------------------

CylinderObject::CylinderObject(BOOL loading) : version(kCylinderPolyFromMeshVersion)
{
	cylDesc.MakeAutoParamBlocks(this);

	if (!loading && !GetPhysicalScaleUVsDisabled())
		SetUsePhysicalScaleUVs(true);
}

bool CylinderObject::SpecifySaveReferences(ReferenceSaveManager& referenceSaveManager)
{
	// if saving to previous version that used pb1 instead of pb2...
	DWORD saveVersion = GetSavingVersion();
	if (saveVersion != 0 && saveVersion <= MAX_RELEASE_R19)
	{
		ProcessPB2ToPB1SaveToPrevious(this, pblock2, PBLOCK_REF_NO, descVer3, PBLOCK_LENGTH, CURRENT_VERSION);
	}
	return __super::SpecifySaveReferences(referenceSaveManager);
}

#define VERSION_CHUNK 0x100

IOResult CylinderObject::Load(ILoad *iload)
{
	ULONG nb;
	IOResult res;
	ParamBlock2PLCB* plcb = new ParamBlock2PLCB(versions, NUM_OLDVERSIONS, &cyl_param_blk, this, PBLOCK_REF_NO);
	iload->RegisterPostLoadCallback(plcb);
	// Set to original version when loading; new version files will contain a version chunk
	version = kCylinderOriginalVersion;
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

IOResult CylinderObject::Save(ISave *isave)
{
	IOResult result = IO_OK;
	ULONG nb;

	isave->BeginChunk(VERSION_CHUNK);
	result = isave->Write(&version, sizeof(int), &nb);
	isave->EndChunk();

	return result;
}

void CylinderObject::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	__super::BeginEditParams(ip, flags, prev);
	this->ip = ip;
	// If this has been freshly created by type-in, set creation values:
	if (CylinderObject::typeinCreate)
	{
		pblock2->SetValue(cyl_radius, 0, cyl_typein_blk.GetFloat(cyl_ti_radius));
		pblock2->SetValue(cyl_height, 0, cyl_typein_blk.GetFloat(cyl_ti_height));
		CylinderObject::typeinCreate = false;
	}

	// throw up all the appropriate auto-rollouts
	cylDesc.BeginEditParams(ip, this, flags, prev);
	// if in Create Panel, install a callback for the type in.
	if (flags & BEGIN_EDIT_CREATE)
		cyl_typein_blk.SetUserDlgProc(new CylTypeInDlgProc(this));
	// install a callback for the params.
	cyl_param_blk.SetUserDlgProc(new CylinderParamDlgProc(this));
}

void CylinderObject::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	__super::EndEditParams(ip, flags, next);
	this->ip = NULL;
	cylDesc.EndEditParams(ip, this, flags, next);
}

void CylinderObject::SetParams(float rad, float height, int segs, int sides, int capsegs, BOOL smooth,
	BOOL genUV, BOOL sliceOn, float slice1, float slice2) {
	pblock2->SetValue(cyl_radius, 0, rad);
	pblock2->SetValue(cyl_height, 0, height);
	pblock2->SetValue(cyl_segs, 0, segs);
	pblock2->SetValue(cyl_sides, 0, sides);
	pblock2->SetValue(cyl_slice, 0, sliceOn);
	pblock2->SetValue(cyl_pieslice1, 0, slice1);
	pblock2->SetValue(cyl_pieslice2, 0, slice2);
	pblock2->SetValue(cyl_mapping, 0, genUV);
}

Point3 CylinderObject::GetSurfacePoint(
	TimeValue t, float u, float v, Interval &iv)
{
	float radius, height;
	pblock2->GetValue(cyl_radius, t, radius, iv);
	pblock2->GetValue(cyl_height, t, height, iv);
	Point3 p;
	p.x = (float)cos(u*TWOPI)*radius;
	p.y = (float)sin(u*TWOPI)*radius;
	p.z = height * v;
	return p;
}

// Cone also uses this build function
void BuildCylinderPoly(MNMesh & mesh, int segs, int smooth, int lsegs,
	int capsegs, int doPie, float radius1, float radius2,
	float height, float pie1, float pie2, int genUVs,
	BOOL usePhysUVs) {
	Point3 p;
	int ix, jx;
	int nf = 0, nv = 0;//, lsegs;
	float delta, ang, u;
	float totalPie, startAng = 0.0f;

	if (doPie) doPie = 1;
	else doPie = 0;

	//lsegs = llsegs-1 + 2*capsegs;

	// Make pie2 < pie1 and pie1-pie2 < TWOPI
	while (pie1 < pie2) pie1 += TWOPI;
	while (pie1 > pie2 + TWOPI) pie1 -= TWOPI;
	if (pie1 == pie2) totalPie = TWOPI;
	else totalPie = pie1 - pie2;

	if (doPie) {
		delta = totalPie / (float)(segs - 1);
		startAng = (height < 0) ? pie1 : pie2;	// mjm - 2.16.99
	}
	else {
		delta = (float)2.0*PI / (float)segs;
	}

	if (height < 0) delta = -delta;

	int nverts;
	int nfaces;
	if (doPie) {
		// Number of vertices is height segments times (segs "perimeter" verts + 1 "central" vert)
		nverts = (segs + 1)*(lsegs + 1);
		// Faces to fill these rows...
		nfaces = (segs + 1)*lsegs;
		// Plus faces for the caps:
		if (capsegs > 1) {
			nverts += 2 * (capsegs - 1)*segs;
			nfaces += 2 * capsegs*(segs - 1);
		}
		else {
			nfaces += 2;
		}
	}
	else {
		// Number of vertices is the segments times the height segments:
		nverts = segs*(lsegs + 1);
		// Faces to fill these rows...
		nfaces = segs*lsegs;
		// Plus faces for the caps:
		if (capsegs > 1) {
			nverts += 2 * (capsegs - 1)*segs + 2;	// two pole vertices.
			nfaces += 2 * capsegs*segs;
		}
		else {
			nfaces += 2;
		}
	}
	mesh.setNumVerts(nverts);
	mesh.setNumFaces(nfaces);

	// Do sides first.
	nv = 0;
	nf = 0;
	int vv[4];
	int segss = segs + doPie;
	for (ix = 0; ix < lsegs + 1; ix++) {
		float   u = float(ix) / float(lsegs);
		float rad = (radius1*(1.0f - u) + radius2*u);
		p.z = height*((float)ix / float(lsegs));
		ang = startAng;
		for (jx = 0; jx < segss; jx++) {
			if (jx == segs) {
				p.x = p.y = 0.0f;
			}
			else {
				p.x = cosf(ang)*rad;
				p.y = sinf(ang)*rad;
			}
			mesh.v[nv].p = p;
			nv++;
			ang += delta;
		}
		if (ix) {
			for (jx = 0; jx < segss; jx++) {
				vv[0] = (ix - 1)*segss + jx;
				vv[1] = (ix - 1)*segss + (jx + 1) % segss;
				vv[2] = vv[1] + segss;
				vv[3] = vv[0] + segss;
				mesh.f[nf].MakePoly(4, vv);
				if ((jx == segs - 1) && doPie) {
					mesh.f[nf].material = 4;
					mesh.f[nf].smGroup = (1 << 2);
				}
				else {
					if (jx == segs) {
						mesh.f[nf].material = 3;
						mesh.f[nf].smGroup = (1 << 1);
					}
					else {
						mesh.f[nf].material = 2;
						mesh.f[nf].smGroup = (1 << 3);
					}
				}
				nf++;
			}
		}
	}

	// Caps:
	float rad;
	if (capsegs == 1) {
		mesh.f[nf].SetAlloc(segss);
		mesh.f[nf].deg = segss;
		for (jx = 0; jx < segss; jx++) mesh.f[nf].vtx[jx] = segss - 1 - jx;
		mesh.f[nf].smGroup = 1;
		mesh.f[nf].material = 1;
		if (doPie) mesh.RetriangulateFace(nf);
		else mesh.BestConvexDiagonals(nf);
		nf++;
		mesh.f[nf].SetAlloc(segss);
		mesh.f[nf].deg = segss;
		for (jx = 0, ix = segss*lsegs; jx < segss; jx++, ix++) mesh.f[nf].vtx[jx] = ix;
		mesh.f[nf].smGroup = 1;
		mesh.f[nf].material = 0;
		if (doPie) mesh.RetriangulateFace(nf);
		else mesh.BestConvexDiagonals(nf);
		nf++;
	}
	else {
		// Do Bottom Cap:
		int startv = nv;
		for (ix = 1; ix < capsegs; ix++) {
			p.z = 0.0f;
			u = float(capsegs - ix) / float(capsegs);
			ang = startAng;
			rad = radius1*u;
			for (jx = 0; jx < segs; jx++) {
				p.x = (float)cos(ang)*rad;
				p.y = (float)sin(ang)*rad;
				mesh.v[nv].p = p;
				nv++;
				ang += delta;
			}
			if (ix == 1) {
				// First row of faces:
				for (jx = 0; jx < segs - doPie; jx++) {
					vv[0] = (jx + 1) % segs;
					vv[1] = jx;
					vv[2] = startv + jx;
					vv[3] = startv + (jx + 1) % segs;
					mesh.f[nf].MakePoly(4, vv);
					mesh.f[nf].smGroup = 1;
					mesh.f[nf].material = 1;
					nf++;
				}
			}
			else {
				for (jx = 0; jx < segs - doPie; jx++) {
					vv[0] = startv + segs*(ix - 2) + (jx + 1) % segs;
					vv[1] = startv + segs*(ix - 2) + jx;
					vv[2] = vv[1] + segs;
					vv[3] = vv[0] + segs;
					mesh.f[nf].MakePoly(4, vv);
					mesh.f[nf].smGroup = 1;
					mesh.f[nf].material = 1;
					nf++;
				}
			}
		}
		if (!doPie) {
			mesh.v[nv].p = Point3(0, 0, 0);
			nv++;
		}
		// Last row of faces:
		for (jx = 0; jx < segs - doPie; jx++) {
			vv[0] = startv + segs*(capsegs - 2) + (jx + 1) % segs;
			vv[1] = startv + segs*(capsegs - 2) + jx;
			vv[2] = doPie ? segs : nv - 1;
			mesh.f[nf].MakePoly(3, vv);
			mesh.f[nf].smGroup = 1;
			mesh.f[nf].material = 1;
			nf++;
		}

		// Do top cap:
		startv = nv;
		for (ix = 1; ix < capsegs; ix++) {
			p.z = height;
			u = float(capsegs - ix) / float(capsegs);
			ang = startAng;
			rad = radius2*u;
			for (jx = 0; jx < segs; jx++) {
				p.x = (float)cos(ang)*rad;
				p.y = (float)sin(ang)*rad;
				mesh.v[nv].p = p;
				nv++;
				ang += delta;
			}
			if (ix == 1) {
				// First row of faces:
				for (jx = 0; jx < segs - doPie; jx++) {
					vv[1] = segss*lsegs + (jx + 1) % segs;
					vv[0] = segss*lsegs + jx;
					vv[3] = startv + jx;
					vv[2] = startv + (jx + 1) % segs;
					mesh.f[nf].MakePoly(4, vv);
					mesh.f[nf].smGroup = 1;
					mesh.f[nf].material = 0;
					nf++;
				}
			}
			else {
				for (jx = 0; jx < segs - doPie; jx++) {
					vv[1] = startv + segs*(ix - 2) + (jx + 1) % segs;
					vv[0] = startv + segs*(ix - 2) + jx;
					vv[2] = vv[1] + segs;
					vv[3] = vv[0] + segs;
					mesh.f[nf].MakePoly(4, vv);
					mesh.f[nf].smGroup = 1;
					mesh.f[nf].material = 0;
					nf++;
				}
			}
		}
		if (!doPie) {
			mesh.v[nv].p = Point3(0.0f, 0.0f, height);
			nv++;
		}
		// Last row of faces:
		for (jx = 0; jx < segs - doPie; jx++) {
			vv[1] = startv + segs*(capsegs - 2) + (jx + 1) % segs;
			vv[0] = startv + segs*(capsegs - 2) + jx;
			vv[2] = doPie ? segss*(lsegs + 1) - 1 : nv - 1;
			mesh.f[nf].MakePoly(3, vv);
			mesh.f[nf].smGroup = 1;
			mesh.f[nf].material = 0;
			nf++;
		}
	}

	if (genUVs) {
		UVWMapper mapper;
		mapper.cap = !usePhysUVs;
		mapper.type = MAP_CYLINDRICAL;
		mapper.uflip = 0;
		mapper.vflip = 0;
		mapper.wflip = 0;
		float rd = radius1 > radius2 ? radius1 : radius2;
		float utile = usePhysUVs ? ((float)2.0f*PI*rd) : 1.0f;
		float vtile = usePhysUVs ? height : 1.0f;

		mapper.utile = utile;
		mapper.vtile = vtile;
		mapper.wtile = 1.0f;
		mapper.tm.IdentityMatrix();
		Matrix3 tm(1);
		float r = fabsf(radius1) > fabsf(radius2) ? fabsf(radius1) : fabsf(radius2);
		float h = height;
		if (r == 0.0f) r = 1.0f;
		else r = 1.0f / r;
		if (h == 0.0f) h = 1.0f;
		else h = 1.0f / h;
		mapper.tm.Scale(Point3(r, r, h));
		mapper.tm.RotateZ(HALFPI);
		mapper.tm.SetTrans(Point3(0.0f, 0.0f, -0.5f));
		mesh.ApplyMapper(mapper, 1);
	}

	DbgAssert(nf == mesh.numf);
	DbgAssert(nv == mesh.numv);
	//Make sure the MNMesh caches (both geometry and topology) are clean before returning
	mesh.InvalidateGeomCache();
	mesh.InvalidateTopoCache();
	mesh.FillInMesh();
}

void BuildCylinderMesh(Mesh &mesh,
	int segs, int smooth, int llsegs, int capsegs, int doPie,
	float radius1, float radius2, float height, float pie1, float pie2,
	int genUVs, int usePhysUVs)
{
	Point3 p;
	int ix = 0;
	int na = 0;
	int nb = 0;
	int nc = 0;
	int nd = 0;
	int jx = 0;
	int kx = 0;
	int ic = 1;
	int nf = 0;
	int nv = 0;
	int lsegs = 0;
	float delta = 0.0f;
	float ang = 0.0f;
	float u = 0.0f;
	float totalPie = 0.0f;
	float startAng = 0.0f;

	if (doPie) doPie = 1;
	else doPie = 0;

	lsegs = llsegs - 1 + 2 * capsegs;

	// Make pie2 < pie1 and pie1-pie2 < TWOPI
	while (pie1 < pie2) pie1 += TWOPI;
	while (pie1 > pie2 + TWOPI) pie1 -= TWOPI;
	if (pie1 == pie2) totalPie = TWOPI;
	else totalPie = pie1 - pie2;

	if (doPie) {
		segs++; //*** O.Z. fix for bug 240436 
		delta = totalPie / (float)(segs - 1);
		startAng = (height < 0) ? pie1 : pie2;	// mjm - 2.16.99
//		startAng = pie2;						// mjm - 2.16.99
	}
	else {
		delta = (float)2.0*PI / (float)segs;
	}

	if (height < 0) delta = -delta;

	int nverts;
	int nfaces;
	if (doPie) {
		nverts = (segs + 1)*(lsegs);
		nfaces = 2 * (segs + 1)*(lsegs - 1) + 2 * (segs - 1);
	}
	else {
		nverts = 2 + segs*(lsegs);
		nfaces = 2 * segs*(lsegs);
	}
	mesh.setNumVerts(nverts);
	mesh.setNumFaces(nfaces);
	mesh.setSmoothFlags((smooth != 0) | ((doPie != 0) << 1));
	if (0/*genUVs*/) {
		mesh.setNumTVerts(nverts);
		mesh.setNumTVFaces(nfaces);
	}
	else {
		mesh.setNumTVerts(0);
		mesh.setNumTVFaces(0);
	}

	// bottom vertex 
	mesh.setVert(nv, Point3(0.0, 0.0, 0.0));
	//if (genUVs) mesh.setTVert(nv,0.5f,1.0f,0.0f);
	nv++;

	// Bottom cap vertices	
	for (ix = 0; ix < capsegs; ix++) {

		// Put center vertices all the way up
		if (ix && doPie) {
			p.z = height*((float)ic / float(lsegs - 1));
			p.x = p.y = 0.0f;
			mesh.setVert(nv, p);
			//if (genUVs) mesh.setTVert(nv,0.5f, (float)ic/float(lsegs-1), 0.0f);
			nv++;
			ic++;
		}

		p.z = 0.0f;
		u = float(ix + 1) / float(capsegs);
		ang = startAng;
		for (jx = 0; jx < segs; jx++) {
			p.x = (float)cos(ang)*radius1*u;
			p.y = (float)sin(ang)*radius1*u;
			mesh.setVert(nv, p);
			//if (genUVs) mesh.setTVert(nv,float(jx)/float(segs),1.0f-u,0.0f);
			nv++;
			ang += delta;
		}
	}

	// Middle vertices 
	for (ix = 1; ix < llsegs; ix++) {

		// Put center vertices all the way up
		if (doPie) {
			p.z = height*((float)ic / float(lsegs - 1));
			p.x = p.y = 0.0f;
			mesh.setVert(nv, p);
			//if (genUVs) mesh.setTVert(nv,0.5f, (float)ic/float(lsegs-1), 0.0f);
			nv++;
			ic++;
		}

		float   u = float(ix) / float(llsegs);
		float rad = (radius1*(1.0f - u) + radius2*u);
		p.z = height*((float)ix / float(llsegs));
		ang = startAng;
		for (jx = 0; jx < segs; jx++) {
			p.x = (float)cos(ang)*rad;
			p.y = (float)sin(ang)*rad;
			mesh.setVert(nv, p);
			//if (genUVs) mesh.setTVert(nv,float(jx)/float(segs),(float)ix/float(llsegs),0.0f);
			nv++;
			ang += delta;
		}
	}

	// Top cap vertices	
	for (ix = 0; ix < capsegs; ix++) {

		// Put center vertices all the way up
		if (doPie) {
			p.z = height*((float)ic / float(lsegs - 1));
			p.x = p.y = 0.0f;
			mesh.setVert(nv, p);
			//if (genUVs) mesh.setTVert(nv,0.5f, (float)ic/float(lsegs-1), 0.0f);
			ic++;
			nv++;
		}

		p.z = height;
		u = 1.0f - float(ix) / float(capsegs);
		ang = startAng;
		for (jx = 0; jx < segs; jx++) {
			p.x = (float)cos(ang)*radius2*u;
			p.y = (float)sin(ang)*radius2*u;
			mesh.setVert(nv, p);
			//if (genUVs) mesh.setTVert(nv,float(jx)/float(segs),u,0.0f);
			nv++;
			ang += delta;
		}
	}

	/* top vertex */
	if (!doPie) {
		mesh.setVert(nv, (float)0.0, (float)0.0, height);
		//if (genUVs) mesh.setTVert(nv,0.5f,0.0f,0.0f);
		nv++;
	}

	// Now make faces ---

	BitArray startSliceFaces;
	BitArray endSliceFaces;
	BitArray topCapFaces;
	BitArray botCapFaces;

	if (usePhysUVs) {
		startSliceFaces.SetSize(mesh.numFaces);
		endSliceFaces.SetSize(mesh.numFaces);
		topCapFaces.SetSize(mesh.numFaces);
		botCapFaces.SetSize(mesh.numFaces);
	}

	// Make bottom cap		

	for (ix = 1; ix <= segs - doPie; ++ix) {
		nc = (ix == segs) ? 1 : ix + 1;
		if (doPie && ix == 1)
			mesh.faces[nf].setEdgeVisFlags(capsegs > 1, 1, 1);
		else if (doPie && ix == segs - doPie)
			mesh.faces[nf].setEdgeVisFlags(1, 1, 0);
		else mesh.faces[nf].setEdgeVisFlags(capsegs > 1, 1, capsegs > 1);
		mesh.faces[nf].setSmGroup(1);
		mesh.faces[nf].setVerts(0, nc, ix);
		mesh.faces[nf].setMatID(1);
		if (usePhysUVs) {
			botCapFaces.Set(nf);
		}
		nf++;
	}

	int topCapStartFace = 0;

	/* Make midsection */
	for (ix = 0; ix < lsegs - 1; ++ix) {
		if (doPie) {
			jx = ix*(segs + 1);
		}
		else {
			jx = ix*segs + 1;
		}

		for (kx = 0; kx < segs + doPie; ++kx) {
			DWORD grp = 0;
			int mtlid;
			BOOL inSlice = FALSE;

			if (kx == 0 && doPie) {
				mtlid = 3;
				grp = (1 << 1);
				inSlice = TRUE;
				if (usePhysUVs) {
					startSliceFaces.Set(nf);
					startSliceFaces.Set(nf + 1);
				}
			}
			else
				if (kx == segs) {
					mtlid = 4;
					grp = (1 << 2);
					inSlice = TRUE;
					if (usePhysUVs) {
						endSliceFaces.Set(nf);
						endSliceFaces.Set(nf + 1);
					}
				}
				else
					if (ix < capsegs - 1 || ix >= capsegs + llsegs - 1) {
						grp = 1;
						mtlid = (ix < capsegs - 1) ? 0 : 1;
						if (usePhysUVs) {
							if (mtlid == 0) {
								botCapFaces.Set(nf);
								botCapFaces.Set(nf + 1);
							}
							else {
								topCapFaces.Set(nf);
								topCapFaces.Set(nf + 1);
							}
						}
					}
					else {
						mtlid = 2;
						if (smooth) {
							grp = (1 << 3);
						}
					}

					na = jx + kx;
					nb = na + segs + doPie;
					nc = (kx == (segs + doPie - 1)) ? jx + segs + doPie : nb + 1;
					nd = (kx == (segs + doPie - 1)) ? jx : na + 1;

					mesh.faces[nf].setEdgeVisFlags(0, !inSlice, 1);
					mesh.faces[nf].setSmGroup(grp);
					mesh.faces[nf].setVerts(na, nc, nb);
					mesh.faces[nf].setMatID(mtlid);
					nf++;

					mesh.faces[nf].setEdgeVisFlags(!inSlice, 1, 0);
					mesh.faces[nf].setSmGroup(grp);
					mesh.faces[nf].setVerts(na, nd, nc);
					mesh.faces[nf].setMatID(mtlid);
					nf++;

		}
	}

	//Make Top cap 			
	if (doPie) {
		na = (lsegs - 1)*(segs + 1);
		jx = na + 1;
	}
	else {
		na = mesh.getNumVerts() - 1;
		jx = (lsegs - 1)*segs + 1;
	}
	for (ix = 0; ix < segs - doPie; ++ix) {
		nb = jx + ix;
		nc = (ix == segs - 1) ? jx : nb + 1;
		if (doPie && ix == 0)
			mesh.faces[nf].setEdgeVisFlags(1, 1, 0);
		else if (doPie && ix == segs - doPie - 1)
			mesh.faces[nf].setEdgeVisFlags(capsegs>1, 1, 1);
		else mesh.faces[nf].setEdgeVisFlags(capsegs > 1, 1, capsegs > 1);
		mesh.faces[nf].setSmGroup(1);
		mesh.faces[nf].setVerts(na, nb, nc);
		mesh.faces[nf].setMatID(0);
		if (usePhysUVs) {
			topCapFaces.Set(nf);
		}
		nf++;
	}

	if (genUVs) {
		Matrix3 tm(1);
		float r = fabs(radius1) > fabs(radius2) ? (float)fabs(radius1) : (float)fabs(radius2);
		float h = height;
		if (r == 0.0f) r = 1.0f;
		else r = 1.0f / r;
		if (h == 0.0f) h = 1.0f;
		else h = 1.0f / h;
		tm.Scale(Point3(r, r, h));
		tm.RotateZ(HALFPI);
		tm.SetTrans(Point3(0.0f, 0.0f, -0.5f));
		float rd = radius1 > radius2 ? radius1 : radius2;
		float utile = usePhysUVs ? ((float)2.0f*PI*rd) : 1.0f;
		float vtile = usePhysUVs ? height : 1.0f;

		mesh.ApplyUVWMap(MAP_CYLINDRICAL,
			utile, vtile, 1.0f,
			0, 0, 0, FALSE,
			tm);

		if (usePhysUVs) {

			MakeMeshCapTexture(mesh, ScaleMatrix(Point3(1.0f, -1.0f, 1.0f)),
				botCapFaces, usePhysUVs);

			MakeMeshCapTexture(mesh, TransMatrix(Point3(0.0f, 0.0f, -height)),
				topCapFaces, usePhysUVs);
			if (doPie) {
				Matrix3 tm = RotateZMatrix(-startAng) * RotateXMatrix(float(-PI / 2.0));
				MakeMeshCapTexture(mesh, tm, startSliceFaces, usePhysUVs);
				tm = RotateZMatrix(-ang) * RotateXMatrix(float(-PI / 2.0));
				tm.Scale(Point3(-1.0f, 1.0f, 1.0f));
				MakeMeshCapTexture(mesh, tm, endSliceFaces, usePhysUVs);
			}
		}
	}

	assert(nf == mesh.numFaces);
	assert(nv == mesh.numVerts);
	mesh.InvalidateTopologyCache();
}

BOOL CylinderObject::HasUVW() {
	BOOL genUVs;
	Interval v;
	pblock2->GetValue(cyl_mapping, 0, genUVs, v);
	return genUVs;
}

void CylinderObject::SetGenUVW(BOOL sw) {
	if (sw == HasUVW()) return;
	pblock2->SetValue(cyl_mapping, 0, sw);
}

Object *CylinderObject::BuildPoly(TimeValue t) {
	int segs, smooth, llsegs, capsegs;
	float radius, height, pie1, pie2;
	int doPie, genUVs;

	// Start the validity interval at forever and widdle it down.
	Interval tvalid = FOREVER;

	pblock2->GetValue(cyl_sides, t, segs, tvalid);
	pblock2->GetValue(cyl_segs, t, llsegs, tvalid);
	pblock2->GetValue(cyl_capsegs, t, capsegs, tvalid);
	pblock2->GetValue(cyl_smooth, t, smooth, tvalid);
	pblock2->GetValue(cyl_slice, t, doPie, tvalid);
	pblock2->GetValue(cyl_mapping, t, genUVs, tvalid);
	Interval gvalid = tvalid;
	pblock2->GetValue(cyl_radius, t, radius, gvalid);
	pblock2->GetValue(cyl_height, t, height, gvalid);
	pblock2->GetValue(cyl_pieslice1, t, pie1, gvalid);
	pblock2->GetValue(cyl_pieslice2, t, pie2, gvalid);
	LimitValue(radius, MIN_RADIUS, MAX_RADIUS);
	LimitValue(height, MIN_HEIGHT, MAX_HEIGHT);
	LimitValue(llsegs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(capsegs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(segs, MIN_SIDES, MAX_SIDES);
	LimitValue(smooth, 0, 1);

	PolyObject *pobj = new PolyObject();
	// MAXX-29604: Poly creation code is buggy.  If new version, just create the trimesh version as usual and then convert that to MNMesh
	if (version != kCylinderOriginalVersion)
	{
		BuildCylinderMesh(mesh,
			segs, smooth, llsegs, capsegs, doPie,
			radius, radius, height, pie1, pie2, genUVs, GetUsePhysicalScaleUVs());
		pobj->GetMesh().SetFromTri(mesh);
		pobj->GetMesh().MakePolyMesh(0, FALSE);
	}
	else
	{
		MNMesh & mesh = pobj->GetMesh();
		BuildCylinderPoly(mesh, segs, smooth, llsegs, capsegs, doPie,
			radius, radius, height, pie1, pie2, genUVs, GetUsePhysicalScaleUVs());
	}
	pobj->SetChannelValidity(TOPO_CHAN_NUM, tvalid);
	pobj->SetChannelValidity(GEOM_CHAN_NUM, gvalid);
	return pobj;
}

void CylinderObject::BuildMesh(TimeValue t)
{
	int segs, smooth, llsegs, capsegs;
	float radius, height, pie1, pie2;
	int doPie, genUVs;

	// Start the validity interval at forever and widdle it down.
	ivalid = FOREVER;

	pblock2->GetValue(cyl_sides, t, segs, ivalid);
	pblock2->GetValue(cyl_segs, t, llsegs, ivalid);
	pblock2->GetValue(cyl_capsegs, t, capsegs, ivalid);
	pblock2->GetValue(cyl_radius, t, radius, ivalid);
	pblock2->GetValue(cyl_height, t, height, ivalid);
	pblock2->GetValue(cyl_smooth, t, smooth, ivalid);
	pblock2->GetValue(cyl_pieslice1, t, pie1, ivalid);
	pblock2->GetValue(cyl_pieslice2, t, pie2, ivalid);
	pblock2->GetValue(cyl_slice, t, doPie, ivalid);
	pblock2->GetValue(cyl_mapping, t, genUVs, ivalid);
	LimitValue(radius, MIN_RADIUS, MAX_RADIUS);
	LimitValue(height, MIN_HEIGHT, MAX_HEIGHT);
	LimitValue(llsegs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(capsegs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(segs, MIN_SIDES, MAX_SIDES);
	LimitValue(smooth, 0, 1);
	BOOL usePhysUVs = GetUsePhysicalScaleUVs();

	BuildCylinderMesh(mesh,
		segs, smooth, llsegs, capsegs, doPie,
		radius, radius, height, pie1, pie2, genUVs, usePhysUVs);
}

inline Point3 operator+(const PatchVert &pv, const Point3 &p)
{
	return p + pv.p;
}

#define CIRCLE_VECTOR_LENGTH 0.5517861843f

void BuildCylinderPatch(
	TimeValue t, PatchMesh &patch,
	float radius1, float radius2, float height, int genUVs, BOOL usePhysUVs)
{
	int nverts = 10;
	int nvecs = 80;
	int npatches = 12;
	patch.setNumVerts(nverts);
	patch.setNumTVerts(genUVs ? 12 : 0);
	patch.setNumVecs(nvecs);
	patch.setNumPatches(npatches);
	patch.setNumTVPatches(genUVs ? 12 : 0);

	// Center of base
	patch.setVert(0, 0.0f, 0.0f, 0.0f);

	// Base
	patch.setVert(1, radius1, 0.0f, 0.0f);
	patch.setVert(2, 0.0f, radius1, 0.0f);
	patch.setVert(3, -radius1, 0.0f, 0.0f);
	patch.setVert(4, 0.0f, -radius1, 0.0f);

	// Center of top
	patch.setVert(5, 0.0f, 0.0f, height);

	// Top
	patch.setVert(6, radius2, 0.0f, height);
	patch.setVert(7, 0.0f, radius2, height);
	patch.setVert(8, -radius2, 0.0f, height);
	patch.setVert(9, 0.0f, -radius2, height);

	// Tangents	
	Point3 vecs[] = {
		Point3(0.0f, CIRCLE_VECTOR_LENGTH,  0.0f),
		Point3(-CIRCLE_VECTOR_LENGTH, 0.0f, 0.0f),
		Point3(0.0f, -CIRCLE_VECTOR_LENGTH,  0.0f),
		Point3(CIRCLE_VECTOR_LENGTH, 0.0f, 0.0f) };

	float len = 1.0f / 3.0f;
	Point3 vecs2[] = {
		Point3(0.0f, len,  0.0f),
		Point3(-len, 0.0f, 0.0f),
		Point3(0.0f, -len,  0.0f),
		Point3(len, 0.0f, 0.0f) };

	float rr = (radius1 - radius2) / 3.0f;
	float hh = height / 3.0f;
	Point3 hvecs1[] = {
		Point3(-rr, 0.0f, hh),
		Point3(0.0f, -rr, hh),
		Point3(rr, 0.0f, hh),
		Point3(0.0f, rr, hh) };
	Point3 hvecs2[] = {
		Point3(rr, 0.0f, -hh),
		Point3(0.0f, rr, -hh),
		Point3(-rr, 0.0f, -hh),
		Point3(0.0f, -rr, -hh) };

	int ix = 0;
	for (int j = 0; j < 4; j++) {
		patch.setVec(ix++, patch.verts[0] + vecs2[j] * radius1);
	}
	for (int i = 0; i < 4; i++) {
		patch.setVec(ix++, patch.verts[i + 1] + vecs[(i) % 4] * radius1);
		patch.setVec(ix++, patch.verts[i + 1] + vecs2[(1 + i) % 4] * radius1);
		patch.setVec(ix++, patch.verts[i + 1] + vecs[(2 + i) % 4] * radius1);
		patch.setVec(ix++, patch.verts[i + 1] + hvecs1[i]);
	}
	for (int j = 0; j < 4; j++) {
		patch.setVec(ix++, patch.verts[5] + vecs2[j] * radius2);
	}
	for (int i = 0; i < 4; i++) {
		patch.setVec(ix++, patch.verts[i + 6] + vecs[(i) % 4] * radius2);
		patch.setVec(ix++, patch.verts[i + 6] + vecs2[(1 + i) % 4] * radius2);
		patch.setVec(ix++, patch.verts[i + 6] + vecs[(2 + i) % 4] * radius2);
		patch.setVec(ix++, patch.verts[i + 6] + hvecs2[i]);
	}

#define Tang(vv,ii) ((vv)*4+(ii))

	// Build the patches
	int interior = 40;
	for (int i = 0; i < 4; i++) {
		patch.patches[i].SetType(PATCH_QUAD);
		patch.patches[i].setVerts(i + 6, i + 1, (i + 1) % 4 + 1, (i + 1) % 4 + 6);
		patch.patches[i].setVecs(
			Tang(i + 6, 3), Tang(i + 1, 3),
			Tang(i + 1, 0), Tang((i + 1) % 4 + 1, 2),
			Tang((i + 1) % 4 + 1, 3), Tang((i + 1) % 4 + 6, 3),
			Tang((i + 1) % 4 + 6, 2), Tang(i + 6, 0));
		patch.patches[i].setInteriors(interior, interior + 1, interior + 2, interior + 3);
		patch.patches[i].smGroup = 1;
		//watje 3-17-99 to support patch matids
		patch.patches[i].setMatID(2);

		interior += 4;
	}

	for (int i = 0; i < 4; i++) {
		patch.patches[i + 4].SetType(PATCH_TRI);
		patch.patches[i + 4].setVerts(0, (i + 1) % 4 + 1, i + 1);
		patch.patches[i + 4].setVecs(
			Tang(0, i), Tang((i + 1) % 4 + 1, 1),
			Tang((i + 1) % 4 + 1, 2), Tang(i + 1, 0),
			Tang(i + 1, 1), Tang(0, (i + 3) % 4));
		patch.patches[i + 4].setInteriors(interior, interior + 1, interior + 2);
		patch.patches[i].smGroup = 2;
		//watje 3-17-99 to support patch matids
		patch.patches[i + 4].setMatID(1);

		interior += 3;
	}

	for (int i = 0; i < 4; i++) {
		patch.patches[i + 8].SetType(PATCH_TRI);
		patch.patches[i + 8].setVerts(5, i + 6, (i + 1) % 4 + 6);
		patch.patches[i + 8].setVecs(
			Tang(5, (i + 3) % 4), Tang(i + 6, 1),
			Tang(i + 6, 0), Tang((i + 1) % 4 + 6, 2),
			Tang((i + 1) % 4 + 6, 1), Tang(5, i));
		patch.patches[i + 8].setInteriors(interior, interior + 1, interior + 2);
		patch.patches[i].smGroup = 2;
		//watje 3-17-99 to support patch matids
		patch.patches[i + 8].setMatID(0);

		interior += 3;
	}
	//232207
	//needed to move this befoe the mapper since the mapper now uses the edge list
	if (!patch.buildLinkages())
	{
		assert(0);
	}
	if (genUVs) {
		Matrix3 tm(1);
		float r = fabs(radius1) > fabs(radius2) ? (float)fabs(radius1) : (float)fabs(radius2);
		float h = height;
		if (r == 0.0f) r = 1.0f;
		else r = 1.0f / r;
		if (h == 0.0f) h = 1.0f;
		else h = 1.0f / h;
		tm.Scale(Point3(r, r, h));
		tm.RotateZ(HALFPI);
		tm.SetTrans(Point3(0.0f, 0.0f, -0.5f));
		float rd = radius1 > radius2 ? radius1 : radius2;
		float utile = usePhysUVs ? ((float)2.0f*PI*rd) : 1.0f;
		float vtile = usePhysUVs ? height : 1.0f;
		patch.ApplyUVWMap(MAP_CYLINDRICAL,
			utile, vtile, 1.0f,
			0, 0, 0, 0,
			tm);
	}

	patch.computeInteriors();
	patch.InvalidateGeomCache();
}

Object *
BuildNURBSCylinder(float radius, float height, BOOL sliceon, float pie1, float pie2, BOOL genUVs)
{
	BOOL flip = FALSE;

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
	_tcscpy(bname, GetString(IDS_RB_CYLINDER));
	_stprintf(sname, _T("%s%s"), bname, GetString(IDS_CT_SURF));
	surf->SetName(sname);

	if (sliceon && pie1 != pie2) {
		GenNURBSCylinderSurface(radius, height, origin, symAxis, refAxis,
			startAngle, endAngle, TRUE, *surf);

		NURBSCVSurface *s0 = (NURBSCVSurface*)nset.GetNURBSObject(0);

		Point3 cen;
		// next the two caps
		for (int c = 0; c < 2; c++) {
			if (c == 0)
				cen = Point3(0, 0, 0);
			else
				cen = Point3(0.0f, 0.0f, height);
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

		// now the pie slices
#define F(s1, s2, s1r, s1c, s2r, s2c) \
		fuse.mSurf1 = (s1); \
		fuse.mSurf2 = (s2); \
		fuse.mRow1 = (s1r); \
		fuse.mCol1 = (s1c); \
		fuse.mRow2 = (s2r); \
		fuse.mCol2 = (s2c); \
		nset.mSurfFuse.Append(1, &fuse);

		NURBSFuseSurfaceCV fuse;

		NURBSCVSurface *s1 = (NURBSCVSurface*)nset.GetNURBSObject(1);
		NURBSCVSurface *s2 = (NURBSCVSurface*)nset.GetNURBSObject(2);

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
						// get x and y from a cap and z from the main sheet.
							Point3 p;
							if (c == 0)
								p = Point3(s1->GetCV(v, 0)->GetPosition(0).x,
									s1->GetCV(v, 0)->GetPosition(0).y,
									s0->GetCV(u, v)->GetPosition(0).z);
							else
								p = Point3(s1->GetCV(v, s1v - 1)->GetPosition(0).x,
									s1->GetCV(v, s1v - 1)->GetPosition(0).y,
									s0->GetCV(u, v)->GetPosition(0).z);
							NURBSControlVertex ncv;
							ncv.SetPosition(0, p);
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

		NURBSCVSurface *s3 = (NURBSCVSurface*)nset.GetNURBSObject(3);
		NURBSCVSurface *s4 = (NURBSCVSurface*)nset.GetNURBSObject(4);

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
		GenNURBSCylinderSurface(radius, height, origin, symAxis, refAxis,
			startAngle, endAngle, FALSE, *surf);

		// now create caps on the ends
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

	Matrix3 mat;
	mat.IdentityMatrix();
	Object *ob = CreateNURBSObject(NULL, &nset, mat);
	return ob;
}

Object* CylinderObject::ConvertToType(TimeValue t, Class_ID obtype)
{
	if (obtype == patchObjectClassID) {
		Interval valid = FOREVER;
		float radius, height;
		int genUVs;
		pblock2->GetValue(cyl_radius, t, radius, valid);
		pblock2->GetValue(cyl_height, t, height, valid);
		pblock2->GetValue(cyl_mapping, t, genUVs, valid);
		PatchObject *ob = new PatchObject();
		BuildCylinderPatch(t, ob->patch, radius, radius, height, genUVs, GetUsePhysicalScaleUVs());
		ob->SetChannelValidity(TOPO_CHAN_NUM, valid);
		ob->SetChannelValidity(GEOM_CHAN_NUM, valid);
		ob->UnlockObject();
		return ob;
	}
	if (obtype == EDITABLE_SURF_CLASS_ID) {
		Interval valid = FOREVER;
		float radius, height, pie1, pie2;
		int sliceon, genUVs;
		pblock2->GetValue(cyl_radius, t, radius, valid);
		pblock2->GetValue(cyl_height, t, height, valid);
		pblock2->GetValue(cyl_pieslice1, t, pie1, valid);
		pblock2->GetValue(cyl_pieslice2, t, pie2, valid);
		pblock2->GetValue(cyl_slice, t, sliceon, valid);
		pblock2->GetValue(cyl_mapping, t, genUVs, valid);
		Object *ob = BuildNURBSCylinder(radius, height, sliceon, pie1, pie2, genUVs);
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
	return __super::ConvertToType(t, obtype);
}

int CylinderObject::CanConvertToType(Class_ID obtype)
{
	if (obtype == defObjectClassID || obtype == triObjectClassID) return 1;
	if (obtype == patchObjectClassID) return 1;
	if (obtype == EDITABLE_SURF_CLASS_ID) return 1;
	if (obtype == polyObjectClassID) return 1;

	return __super::CanConvertToType(obtype);
}

void CylinderObject::GetCollapseTypes(Tab<Class_ID> &clist, Tab<TSTR*> &nlist)
{
	__super::GetCollapseTypes(clist, nlist);
	Class_ID id = EDITABLE_SURF_CLASS_ID;
	TSTR *name = new TSTR(GetString(IDS_SM_NURBS_SURFACE));
	clist.Append(1, &id);
	nlist.Append(1, &name);
}

class CylinderObjCreateCallBack : public CreateMouseCallBack {
	CylinderObject *ob;
	Point3 p[2];
	IPoint2 sp0, sp1;
public:
	int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat) override;
	void SetObj(CylinderObject *obj) { ob = obj; }
};

int CylinderObjCreateCallBack::proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat) {
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
			ob->pblock2->SetValue(cyl_radius, 0, 0.01f);
			ob->pblock2->SetValue(cyl_height, 0, 0.01f);
			break;
		case 1:
		{
			mat.IdentityMatrix();
			//mat.PreRotateZ(HALFPI);
			sp1 = m;
			p[1] = vpt->SnapPoint(m, m, NULL, SNAP_IN_3D);
			bool createByRadius = cyl_crtype_blk.GetInt(cyl_create_meth) == 1;
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

			ob->pblock2->SetValue(cyl_radius, 0, r);

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
			float h = vpt->SnapLength(vpt->GetCPDisp(p[1], Point3(0, 0, 1), sp1, m, TRUE));
			ob->pblock2->SetValue(cyl_height, 0, h);
			if (msg == MOUSE_POINT) {
				ob->suspendSnap = FALSE;
				return (Length(m - sp0) < 3) ? CREATE_ABORT : CREATE_STOP;
			}
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

static CylinderObjCreateCallBack cylCreateCB;

CreateMouseCallBack* CylinderObject::GetCreateMouseCallBack()
{
	cylCreateCB.SetObj(this);
	return(&cylCreateCB);
}

BOOL CylinderObject::OKtoDisplay(TimeValue t)
{
	float radius = 0.f;
	pblock2->GetValue(cyl_radius, t, radius, FOREVER);
	if (radius == 0.0f) return FALSE;
	else return TRUE;
}

void CylinderObject::InvalidateUI()
{
	cyl_param_blk.InvalidateUI(pblock2->LastNotifyParamID());
}

RefTargetHandle CylinderObject::Clone(RemapDir& remap)
{
	CylinderObject* newob = new CylinderObject(FALSE);
	newob->ReplaceReference(0, remap.CloneRef(pblock2));
	newob->ivalid.SetEmpty();
	BaseClone(this, newob, remap);
	newob->SetUsePhysicalScaleUVs(GetUsePhysicalScaleUVs());
	newob->version = version;
	return(newob);
}

void CylinderObject::UpdateUI()
{
	if (ip == NULL)
		return;
	CylinderParamDlgProc* dlg = static_cast<CylinderParamDlgProc*>(cyl_param_blk.GetUserDlgProc());
	dlg->UpdateUI();
}

BOOL CylinderObject::GetUsePhysicalScaleUVs()
{
	return ::GetUsePhysicalScaleUVs(this);
}

void CylinderObject::SetUsePhysicalScaleUVs(BOOL flag)
{
	BOOL curState = GetUsePhysicalScaleUVs();
	if (curState == flag)
		return;
	if (theHold.Holding())
		theHold.Put(new RealWorldScaleRecord<CylinderObject>(this, curState));
	::SetUsePhysicalScaleUVs(this, flag);
	if (pblock2 != NULL)
		pblock2->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	UpdateUI();
	macroRec->SetProperty(this, _T("realWorldMapSize"), mr_bool, flag);
}

BaseInterface* CylinderObject::GetInterface(Interface_ID id)
{
	if (id == RWS_INTERFACE)
		return (RealWorldMapSizeInterface*)this;
	else {
		BaseInterface* intf = GenCylinder::GetInterface(id);
		if (intf)
			return intf;
		else
			return FPMixinInterface::GetInterface(id);
	}
}
