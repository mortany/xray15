/**********************************************************************
*<
FILE: sphere.cpp

DESCRIPTION:  Sphere object, Revised implementation

CREATED BY: Rolf Berteig

HISTORY: created 10/10/95

*>	Copyright (c) 1994, All Rights Reserved.
**********************************************************************/
#include "prim.h"

#include "iparamm2.h"
#include "Simpobj.h"
#include "surf_api.h"
#include "notify.h"
#include "macroRec.h"
#include "RealWorldMapUtils.h"
#include <ReferenceSaveManager.h>
#include <Util/StaticAssert.h>

class SphereObject : public GenSphere, public RealWorldMapSizeInterface   {
	friend class SphereParamDlgProc;
	friend class SphereObjCreateCallBack;
	friend class SphereTypeInDlgProc;

public:			
	// Class vars
	static IObjParam *ip;
	static bool typeinCreate;
	// mjm - 3.19.99 - ensure accurate matIDs and smoothing groups
	int lastSquash;
	BOOL lastNoHemi;

	SphereObject(BOOL loading);

	// From Object
	int CanConvertToType(Class_ID obtype) override;
	Object* ConvertToType(TimeValue t, Class_ID obtype) override;
	void GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist) override;

	CreateMouseCallBack* GetCreateMouseCallBack() override;
	void BeginEditParams( IObjParam  *ip, ULONG flags,Animatable *prev) override;
	void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next) override;
	RefTargetHandle Clone(RemapDir& remap) override;
	bool SpecifySaveReferences(ReferenceSaveManager& referenceSaveManager) override;
	const TCHAR *GetObjectName() override { return GetString(IDS_RB_SPHERE); }
	BOOL HasUVW() override;
	void SetGenUVW(BOOL sw) override;
	BOOL IsParamSurface() override {return TRUE;}
	Point3 GetSurfacePoint(TimeValue t, float u, float v,Interval &iv) override;

	// From GeomObject
	int IntersectRay(TimeValue t, Ray& ray, float& at, Point3& norm) override;

	// Animatable methods		
	void DeleteThis() override {delete this;}
	Class_ID ClassID() override { return Class_ID(SPHERE_CLASS_ID,0); }

	// From ReferenceTarget
	IOResult Load(ILoad *iload) override;
	IOResult Save(ISave *isave) override;

	// From SimpleObjectBase
	void BuildMesh(TimeValue t) override;
	BOOL OKtoDisplay(TimeValue t) override;
	void InvalidateUI() override;

	// From GenSphere
	void SetParams(float rad, int segs, BOOL smooth=TRUE, BOOL genUV=TRUE,
		float hemi=0.0f, BOOL squash=FALSE, BOOL recenter=FALSE) override;


	// Get/Set the UsePhyicalScaleUVs flag.
	BOOL GetUsePhysicalScaleUVs() override;
	void SetUsePhysicalScaleUVs(BOOL flag) override;
	void UpdateUI();

	//From FPMixinInterface
	BaseInterface* GetInterface(Interface_ID id) override
	{ 
		if (id == RWS_INTERFACE) 
			return (RealWorldMapSizeInterface*)this; 
		else {
			BaseInterface* intf = GenSphere::GetInterface(id);
			if (intf)
				return intf;
			else
				return FPMixinInterface::GetInterface(id);
		}
	} 
};


// Misc stuff
#define MAX_SEGMENTS	200
#define MIN_SEGMENTS	4

#define MIN_RADIUS		float(0)
#define MAX_RADIUS		float(1.0E30)

#define MIN_SMOOTH		0
#define MAX_SMOOTH		1

#define DEF_SEGMENTS	32	// 16
#define DEF_RADIUS		float(0.0)

#define SMOOTH_ON	1
#define SMOOTH_OFF	0

#define MIN_SLICE	float(-1.0E30)
#define MAX_SLICE	float( 1.0E30)


//--- ClassDescriptor and class vars ---------------------------------

static BOOL sInterfaceAdded = FALSE;

// The class descriptor for sphere
class SphereClassDesc:public ClassDesc2 {
public:
	// xavier robitaille | 03.02.15 | private boxes, spheres and cylinders 
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE)
	{
		if (!sInterfaceAdded) {
			AddInterface(&gRealWorldMapSizeDesc);
			sInterfaceAdded = TRUE;
		}
		return new SphereObject(loading);
	}
	const TCHAR *	ClassName() override { return GetString(IDS_RB_SPHERE_CLASS); }
	SClass_ID		SuperClassID() override { return GEOMOBJECT_CLASS_ID; }
	Class_ID		ClassID() override { return Class_ID(SPHERE_CLASS_ID,0); }
	const TCHAR* 	Category() override { return GetString(IDS_RB_PRIMITIVES); }
	const TCHAR* InternalName() override { return _T("Sphere"); } // returns fixed parsable name (scripter-visible name)
	HINSTANCE  HInstance() override { return hInstance; }   // returns owning module handle
};

static SphereClassDesc sphereDesc;
extern ClassDesc* GetSphereDesc() { return &sphereDesc; }

// in prim.cpp  - The dll instance handle
extern HINSTANCE hInstance;

IObjParam *SphereObject::ip = NULL;

bool SphereObject::typeinCreate = false;

#define PBLOCK_REF_NO  0

// ParamBlockDesc2 IDs
enum paramblockdesc2_ids { sphere_creation_type, sphere_type_in, sphere_params, };
enum sphere_creation_type_param_ids { sphere_create_meth, };
enum sphere_type_in_param_ids { sphere_ti_pos, sphere_ti_radius, };
enum sphere_param_param_ids {
	sphere_radius = SPHERE_RADIUS, sphere_segs = SPHERE_SEGS, sphere_smooth = SPHERE_SMOOTH,
	sphere_hemi = SPHERE_HEMI, sphere_squash = SPHERE_SQUASH, sphere_recenter = SPHERE_RECENTER,
	sphere_mapping = SPHERE_GENUVS, sphere_slice = SPHERE_SLICEON, sphere_pieslice1 = SPHERE_SLICEFROM,
	sphere_pieslice2 = SPHERE_SLICETO,
};

namespace
{
	MaxSDK::Util::StaticAssert< (sphere_params == SPHERE_PARAMBLOCK_ID) > validator;
}

namespace
{
	class MapCoords_Accessor : public PBAccessor
	{
		void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
		{
			if (owner) ((SphereObject*)owner)->UpdateUI();
		}
	};
	static MapCoords_Accessor mapCoords_Accessor;
}

// class creation type block
static ParamBlockDesc2 sphere_crtype_blk(sphere_creation_type, _T("SphereCreationType"), 0, &sphereDesc, P_CLASS_PARAMS + P_AUTO_UI,
	//rollout
	IDD_SPHEREPARAM1, IDS_RB_CREATIONMETHOD, BEGIN_EDIT_CREATE, 0, NULL,
	// params
	sphere_create_meth, _T("typeinCreationMethod"), TYPE_INT, 0, IDS_RB_CREATIONMETHOD,
	p_default, 1,
	p_range, 0, 1,
	p_ui, TYPE_RADIO, 2, IDC_CREATEDIAMETER, IDC_CREATERADIUS,
	p_end,
	p_end
	);

// class type-in block
static ParamBlockDesc2 sphere_typein_blk(sphere_type_in, _T("SphereTypeIn"), 0, &sphereDesc, P_CLASS_PARAMS + P_AUTO_UI,
	//rollout
	IDD_SPHEREPARAM3, IDS_RB_KEYBOARDENTRY, BEGIN_EDIT_CREATE, APPENDROLL_CLOSED, NULL,
	// params
	sphere_ti_pos, _T("typeInPos"), TYPE_POINT3, 0, IDS_RB_TYPEIN_POS,
	p_default, Point3(0, 0, 0),
	p_range, float(-1.0E30), float(1.0E30),
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_TI_POSX, IDC_TI_POSXSPIN, IDC_TI_POSY, IDC_TI_POSYSPIN, IDC_TI_POSZ, IDC_TI_POSZSPIN, SPIN_AUTOSCALE,
	p_end,
	sphere_ti_radius, _T("typeInRadius"), TYPE_FLOAT, 0, IDS_RB_RADIUS,
	p_default, DEF_RADIUS,
	p_range, MIN_RADIUS, MAX_RADIUS,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_RADIUS, IDC_RADSPINNER, SPIN_AUTOSCALE,
	p_end,
	p_end
	);

// per instance sphere block
static ParamBlockDesc2 sphere_param_blk(sphere_params, _T("SphereParameters"), 0, &sphereDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_REF_NO,
	//rollout
	IDD_SPHEREPARAM2, IDS_RB_PARAMETERS, 0, 0, NULL,
	// params
	sphere_radius, _T("radius"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_RADIUS,
	p_default, DEF_RADIUS,
	p_ms_default, 25.0,
	p_range, MIN_RADIUS, MAX_RADIUS,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_RADIUS, IDC_RADSPINNER, SPIN_AUTOSCALE,
	p_end,
	sphere_segs, _T("segs"), TYPE_INT, P_ANIMATABLE, IDS_RB_SEGS,
	p_default, DEF_SEGMENTS,
	p_ms_default, 16,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_SEGMENTS, IDC_SEGSPINNER, 0.1f,
	p_end,
	sphere_smooth, _T("smooth"), TYPE_BOOL, P_ANIMATABLE, IDS_RB_SMOOTH,
	p_default, TRUE,
	p_ui, TYPE_SINGLECHECKBOX, IDC_OBSMOOTH,
	p_end,
	sphere_hemi, _T("hemisphere"), TYPE_FLOAT, P_ANIMATABLE, IDS_RB_HEMISPHERE,
	p_default, 0.0,
	p_range, 0.0, 1.0,
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_HEMISPHERE, IDC_HEMISPHERESPINNER, 0.005f,
	p_end,
	sphere_squash, _T("chop"), TYPE_INT, 0, IDS_RB_SQUASH,
	p_default, 0,
	p_range, 0, 1,
	p_ui, TYPE_RADIO, 2, IDC_HEMI_CHOP, IDC_HEMI_SQUASH,
	p_vals, 0, 1,
	p_end,
	sphere_recenter, _T("recenter"), TYPE_BOOL, 0, IDS_RB_RECENTER,
	p_default, FALSE,
	p_ui, TYPE_SINGLECHECKBOX, IDC_HEMI_RECENTER,
	p_end,
	sphere_mapping, _T("mapCoords"), TYPE_BOOL, 0, IDS_RB_GENTEXCOORDS,
	p_default, TRUE,
	p_ms_default, FALSE,
	p_ui, TYPE_SINGLECHECKBOX, IDC_GENTEXTURE,
	p_accessor, &mapCoords_Accessor,
	p_end,
	sphere_slice, _T("slice"), TYPE_BOOL, P_RESET_DEFAULT, IDS_AP_SLICEON,
	p_default, FALSE,
	p_ui, TYPE_SINGLECHECKBOX, IDC_SC_SLICEON,
	p_enable_ctrls, 2, sphere_pieslice1, sphere_pieslice2,
	p_end,
	sphere_pieslice1, _T("sliceFrom"), TYPE_ANGLE, P_ANIMATABLE + P_RESET_DEFAULT, IDS_AP_SLICEFROM,
	p_default, 0.0,
	p_range, MIN_SLICE, MAX_SLICE,
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_SC_SLICE1, IDC_SC_SLICE1SPIN, 0.5f,
	p_end,
	sphere_pieslice2, _T("sliceTo"), TYPE_ANGLE, P_ANIMATABLE + P_RESET_DEFAULT, IDS_AP_SLICETO,
	p_default, 0.0,
	p_range, MIN_SLICE, MAX_SLICE,
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_SC_SLICE2, IDC_SC_SLICE2SPIN, 0.5f,
	p_end,
	p_end
	);

static ParamBlockDescID descVer0[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0 },
	{ TYPE_INT, NULL, TRUE, 1 },
	{ TYPE_INT, NULL, TRUE, 2 } };

static ParamBlockDescID descVer1[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0 },
	{ TYPE_INT, NULL, TRUE, 1 },
	{ TYPE_INT, NULL, TRUE, 2 },
	{ TYPE_FLOAT, NULL, TRUE, 3 },
	{ TYPE_INT, NULL, FALSE, 4 },
	{ TYPE_INT, NULL, FALSE, 5 } };

static ParamBlockDescID descVer2[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0 },
	{ TYPE_INT, NULL, TRUE, 1 },
	{ TYPE_BOOL, NULL, TRUE, 2 },
	{ TYPE_FLOAT, NULL, TRUE, 3 },
	{ TYPE_INT, NULL, FALSE, 4 },
	{ TYPE_INT, NULL, FALSE, 5 },
	{ TYPE_INT, NULL, FALSE, 6 } };

static ParamBlockDescID descVer3[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0 },
	{ TYPE_INT, NULL, TRUE, 1 },
	{ TYPE_BOOL, NULL, TRUE, 2 },
	{ TYPE_FLOAT, NULL, TRUE, 3 },
	{ TYPE_INT, NULL, FALSE, 4 },
	{ TYPE_INT, NULL, FALSE, 5 },
	{ TYPE_INT, NULL, FALSE, 6 },
	{ TYPE_INT, NULL, FALSE, 7 },
	{ TYPE_FLOAT, NULL, TRUE, 8 },
	{ TYPE_FLOAT, NULL, TRUE, 9 },
};


// Array of old versions
static ParamVersionDesc versions[] = {
	ParamVersionDesc(descVer0,3,0),
	ParamVersionDesc(descVer1,6,1),
	ParamVersionDesc(descVer2,7,2),
	ParamVersionDesc(descVer3,10,3)
};
#define NUM_OLDVERSIONS	4

// ParamBlock data for SaveToPrevious support
#define PBLOCK_LENGTH	10
#define CURRENT_VERSION	3


//--- TypeInDlgProc --------------------------------

class SphereTypeInDlgProc : public ParamMap2UserDlgProc {
public:
	SphereObject *so;

	SphereTypeInDlgProc(SphereObject *s) {so=s;}
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	void DeleteThis() override {delete this;}
};

INT_PTR SphereTypeInDlgProc::DlgProc(
	TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch (msg) {
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_TI_CREATE: {
			if (sphere_typein_blk.GetFloat(sphere_ti_radius) == 0.0) return TRUE;

			// We only want to set the value if the object is 
			// not in the scene.
			if (so->TestAFlag(A_OBJ_CREATING)) {
				so->pblock2->SetValue(sphere_radius, 0, sphere_typein_blk.GetFloat(sphere_ti_radius));
			}
			else
				SphereObject::typeinCreate = true;

			Matrix3 tm(1);
			tm.SetTrans(sphere_typein_blk.GetPoint3(sphere_ti_pos));
			so->suspendSnap = FALSE;
			so->ip->NonMouseCreate(tm);					
			// NOTE that calling NonMouseCreate will cause this
			// object to be deleted. DO NOT DO ANYTHING BUT RETURN.
			return TRUE;	
							}
		}
		break;	
	}
	return FALSE;
}


class SphereParamDlgProc : public ParamMap2UserDlgProc {
public:
	SphereObject *so;
	HWND thishWnd;

	SphereParamDlgProc(SphereObject *s) {so=s;thishWnd=NULL;}
	INT_PTR DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam) override;
	void UpdateUI();
	BOOL GetRWSState();
	void DeleteThis() override {delete this;}
};

BOOL SphereParamDlgProc::GetRWSState()
{
	BOOL check = IsDlgButtonChecked(thishWnd, IDC_REAL_WORLD_MAP_SIZE2);
	return check;
}

void SphereParamDlgProc::UpdateUI()
{
	if (!thishWnd) return;
	BOOL usePhysUVs = so->GetUsePhysicalScaleUVs();
	CheckDlgButton(thishWnd, IDC_REAL_WORLD_MAP_SIZE2, usePhysUVs);
	EnableWindow(GetDlgItem(thishWnd, IDC_REAL_WORLD_MAP_SIZE2), so->HasUVW());
}

INT_PTR SphereParamDlgProc::DlgProc(
	TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{ thishWnd=hWnd;
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

//--- Sphere methods -------------------------------


SphereObject::SphereObject(BOOL loading) : lastSquash(-1), lastNoHemi(FALSE)
{
	SetAFlag(A_PLUGIN1);
	sphereDesc.MakeAutoParamBlocks(this);

	if (!loading && !GetPhysicalScaleUVsDisabled())
		SetUsePhysicalScaleUVs(true);
}


#define NEWMAP_CHUNKID	0x0100

bool SphereObject::SpecifySaveReferences(ReferenceSaveManager& referenceSaveManager)
{
	// if saving to previous version that used pb1 instead of pb2...
	DWORD saveVersion = GetSavingVersion();
	if (saveVersion != 0 && saveVersion <= MAX_RELEASE_R19)
	{
		ProcessPB2ToPB1SaveToPrevious(this, pblock2, PBLOCK_REF_NO, descVer3, PBLOCK_LENGTH, CURRENT_VERSION);
	}
	return __super::SpecifySaveReferences(referenceSaveManager);
}

IOResult SphereObject::Load(ILoad *iload) 
{
	ClearAFlag(A_PLUGIN1);

	IOResult res;
	while (IO_OK==(res=iload->OpenChunk())) {
		switch (iload->CurChunkID()) {	
		case NEWMAP_CHUNKID:
			SetAFlag(A_PLUGIN1);
			break;
		}
		iload->CloseChunk();
		if (res!=IO_OK)  return res;
	}

	iload->RegisterPostLoadCallback(
		new ParamBlock2PLCB(versions,NUM_OLDVERSIONS,&sphere_param_blk,this,0));
	return IO_OK;
}

IOResult SphereObject::Save(ISave *isave)
{
	if (TestAFlag(A_PLUGIN1)) {
		isave->BeginChunk(NEWMAP_CHUNKID);
		isave->EndChunk();
	}
	return IO_OK;
}

void SphereObject::BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev)
{
	__super::BeginEditParams(ip,flags,prev);

	this->ip = ip;
	// If this has been freshly created by type-in, set creation values:
	if (SphereObject::typeinCreate)
	{
		pblock2->SetValue(sphere_radius, 0, sphere_typein_blk.GetFloat(sphere_ti_radius));
		SphereObject::typeinCreate = false;
	}

	// throw up all the appropriate auto-rollouts
	sphereDesc.BeginEditParams(ip, this, flags, prev);
	// if in Create Panel, install a callback for the type in.
	if (flags & BEGIN_EDIT_CREATE)
		sphere_typein_blk.SetUserDlgProc(new SphereTypeInDlgProc(this));
	// install a callback for the params.
	sphere_param_blk.SetUserDlgProc(new SphereParamDlgProc(this));
}

void SphereObject::EndEditParams( IObjParam *ip, ULONG flags,Animatable *next )
{		
	__super::EndEditParams(ip,flags,next);
	this->ip = NULL;
	sphereDesc.EndEditParams(ip, this, flags, next);
}

Point3 SphereObject::GetSurfacePoint(
	TimeValue t, float u, float v,Interval &iv)
{
	float rad;
	pblock2->GetValue(sphere_radius, t, rad, iv);
	Point3 pos;	
	v -= 0.5f;
	float ar = (float)cos(v*PI);
	pos.x = rad * float(cos(u*TWOPI)) * ar;
	pos.y = rad * float(sin(u*TWOPI)) * ar;
	pos.z = rad * float(sin(v*PI));
	return pos;
}

void SphereObject::SetParams(float rad, int segs, BOOL smooth, BOOL genUV,
							 float hemi, BOOL squash, BOOL recenter) {
								 pblock2->SetValue(sphere_radius,0, rad);
								 pblock2->SetValue(sphere_hemi,0, hemi);
								 pblock2->SetValue(sphere_segs,0, segs);
								 pblock2->SetValue(sphere_squash,0, squash);
								 pblock2->SetValue(sphere_smooth,0, smooth);
								 pblock2->SetValue(sphere_recenter,0, recenter);
								 pblock2->SetValue(sphere_mapping,0, genUV);
}			   

BOOL SphereObject::HasUVW() { 
	BOOL genUVs;
	Interval v;
	pblock2->GetValue(sphere_mapping, 0, genUVs, v);
	return genUVs; 
}

void SphereObject::SetGenUVW(BOOL sw) {  
	if (sw==HasUVW()) return;
	pblock2->SetValue(sphere_mapping,0, sw);
	UpdateUI();
}

void MakeMeshCapTexture(Mesh &mesh, Matrix3 &itm, int fstart, int fend, BOOL usePhysUVs)
{
	if(fstart == fend)
		return;
	// Find out which verts are used by the cap
	BitArray capFaces(mesh.numFaces);
	capFaces.ClearAll();
	for(int i = fstart; i < fend; ++i) {
		capFaces.Set(i);
	}
	MakeMeshCapTexture(mesh, itm, capFaces, usePhysUVs);
}

// Minmax the verts involved in X/Y axis and total them
void MakeMeshCapTexture(Mesh &mesh, Matrix3 &itm, BitArray& capFaces, BOOL usePhysUVs) {
	if (capFaces.NumberSet() == 0)
		return;

	// Find out which verts are used by the cap
	BitArray capVerts(mesh.numVerts);
	capVerts.ClearAll();
	for(int i = 0; i < capFaces.GetSize(); ++i) {
		if (capFaces[i]) {
			Face &f = mesh.faces[i];
			capVerts.Set(f.v[0]);
			capVerts.Set(f.v[1]);
			capVerts.Set(f.v[2]);
		}
	}

	// Minmax the verts involved in X/Y axis and total them
	Box3 bounds;
	int numCapVerts = 0;
	int numCapFaces = capFaces.NumberSet();
	IntTab capIndexes;
	capIndexes.SetCount(mesh.numVerts);
	int baseTVert = mesh.getNumTVerts();
	for(int i = 0; i < mesh.numVerts; ++i) {
		if(capVerts[i]) {
			capIndexes[i] = baseTVert + numCapVerts++;
			bounds += mesh.verts[i] * itm;
		}
	}
	mesh.setNumTVerts(baseTVert + numCapVerts, TRUE);
	Point3 s;
	if (usePhysUVs)
		s = Point3(1.0f, 1.0f, 0.0f);
	else
		s = Point3(1.0f / bounds.Width().x, 1.0f / bounds.Width().y, 0.0f);

	Point3 t(-bounds.Min().x, -bounds.Min().y, 0.0f);
	// Do the TVerts
	for(int i = 0; i < mesh.numVerts; ++i) {
		if(capVerts[i])
			mesh.setTVert(baseTVert++, ((mesh.verts[i] * itm) + t) * s);
	}
	// Do the TVFaces
	for(int i = 0; i < capFaces.GetSize(); ++i) {
		if (capFaces[i]) {
			Face &f = mesh.faces[i];
			mesh.tvFace[i] = TVFace(capIndexes[f.v[0]], capIndexes[f.v[1]], capIndexes[f.v[2]]);
		}
	}
}

float uval[3]={1.0f,0.0f,1.0f};
void SphereObject::BuildMesh(TimeValue t)
{
	Point3 p;	
	int na = 0;
	int nb = 0;
	int nc = 0;
	int nd = 0;
	int jx = 0;
	int nf = 0;
	int nv = 0;
	float delta = 0.0f;
	float delta2 = 0.0f;
	float a = 0.0f;
	float alt = 0.0f;
	float secrad = 0.0f;
	float secang = 0.0f;
	float b = 0.0f;
	float c = 0.0f;
	int segs = 0;
	int smooth = 0;
	float radius = 0.0f;
	float hemi = 0.0f;
	BOOL noHemi = FALSE;	
	int squash = 0;
	int recenter = 0;
	BOOL genUVs = TRUE;
	float startAng = 0.0f;
	float pie1 = 0.0f;
	float pie2 = 0.0f;
	int doPie = 0;
	if (TestAFlag(A_PLUGIN1)) startAng = HALFPI;

	// Start the validity interval at forever and widdle it down.
	ivalid = FOREVER;
	pblock2->GetValue(sphere_radius, t, radius, ivalid);
	pblock2->GetValue(sphere_segs, t, segs, ivalid);
	pblock2->GetValue(sphere_smooth, t, smooth, ivalid);
	pblock2->GetValue(sphere_hemi, t, hemi, ivalid);
	pblock2->GetValue(sphere_squash, t, squash, ivalid);
	pblock2->GetValue(sphere_recenter, t, recenter, ivalid);
	pblock2->GetValue(sphere_mapping, t, genUVs, ivalid);
	pblock2->GetValue(sphere_pieslice1,t,pie1,ivalid);
	pblock2->GetValue(sphere_pieslice2,t,pie2,ivalid);
	pblock2->GetValue(sphere_slice,t,doPie,ivalid);
	LimitValue(segs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(smooth, MIN_SMOOTH, MAX_SMOOTH);
	LimitValue(radius, MIN_RADIUS, MAX_RADIUS);
	LimitValue(hemi, 0.0f, 1.0f);

	float totalPie(0.0f);
	if (doPie) doPie = 1;
	else doPie = 0;
	if (doPie)
	{ pie2+=startAng;pie1+=startAng;
	while (pie1 < pie2) pie1 += TWOPI;
	while (pie1 > pie2+TWOPI) pie1 -= TWOPI;
	if (pie1==pie2) totalPie = TWOPI;
	else totalPie = pie1-pie2;	
	}

	if (hemi<0.00001f) noHemi = TRUE;
	if (hemi>=1.0f) hemi = 0.9999f;
	hemi = (1.0f-hemi) * PI;
	float basedelta=2.0f*PI/(float)segs;
	delta2=(doPie?totalPie/(float)segs:basedelta);
	if (!noHemi && squash) {
		delta  = 2.0f*hemi/float(segs-2);
	} else {
		delta  = basedelta;
	}

	int rows;
	if (noHemi || squash) {
		rows = (segs/2-1);
	} else {
		rows = int(hemi/delta) + 1;
	}
	int realsegs=(doPie?segs+2:segs);
	int nverts = rows * realsegs + 2;
	int nfaces = rows * realsegs * 2;
	if (doPie) 
	{ startAng=pie2;segs+=1;
	if (!noHemi) {nfaces-=2;nverts-=1;}
	}
	mesh.setNumVerts(nverts);
	mesh.setNumFaces(nfaces);
	mesh.setSmoothFlags(smooth != 0);
	int lastvert=nverts-1;

	// mjm - 3.19.99 - ensure accurate matIDs and smoothing groups
	if (lastSquash != squash || lastNoHemi != noHemi)
	{
		lastSquash = squash;
		lastNoHemi = noHemi;
		mesh.InvalidateStrips();
	}

	// Top vertex 
	mesh.setVert(nv, 0.0f, 0.0f, radius);
	nv++;

	// Middle vertices 
	alt=delta;
	for(int ix=1; ix<=rows; ix++) {
		if (!noHemi && ix==rows) alt = hemi;
		a = (float)cos(alt)*radius;		
		secrad = (float)sin(alt)*radius;
		secang = startAng; //0.0f
		for(int jx=0; jx<segs; ++jx) {
			b = (float)cos(secang)*secrad;
			c = (float)sin(secang)*secrad;
			mesh.setVert(nv++,b,c,a);
			secang+=delta2;
		}
		if (doPie &&(noHemi ||(ix<rows))) mesh.setVert(nv++,0.0f,0.0f,a);
		alt+=delta;		
	}

	/* Bottom vertex */
	if (noHemi) {
		mesh.setVert(nv++, 0.0f, 0.0f,-radius);
	}
	else {
		a = (float)cos(hemi)*radius;
		mesh.setVert(nv++, 0.0f, 0.0f, a);
	}

	BOOL issliceface;
	// Now make faces 
	if (doPie) segs++;

	BOOL usePhysUVs = GetUsePhysicalScaleUVs();
	BitArray startSliceFaces;
	BitArray endSliceFaces;

	if (usePhysUVs) {
		startSliceFaces.SetSize(mesh.numFaces);
		endSliceFaces.SetSize(mesh.numFaces);
	}
	// Make top conic cap
	for(int ix=1; ix<=segs; ++ix) {
		issliceface=(doPie && (ix>=segs-1));
		nc=(ix==segs)?1:ix+1;
		mesh.faces[nf].setEdgeVisFlags(1,1,1);
		if ((issliceface)&&(ix==segs-1))
		{	mesh.faces[nf].setSmGroup(smooth?4:0);
		mesh.faces[nf].setMatID(2);
		if (usePhysUVs)
			startSliceFaces.Set(nf);
		}
		else if ((issliceface)&&(ix==segs))
		{	mesh.faces[nf].setSmGroup(smooth?8:0);
		mesh.faces[nf].setMatID(3);
		if (usePhysUVs)
			endSliceFaces.Set(nf);
		}
		else
		{	mesh.faces[nf].setSmGroup(smooth?1:0);
		mesh.faces[nf].setMatID(1); // mjm - 5.5.99 - rollback change - should be fixed in later release
		//			mesh.faces[nf].setMatID(0); // mjm - 3.2.99 - was set to 1
		}
		mesh.faces[nf].setVerts(0, ix, nc);
		nf++;
	}

	/* Make midsection */
	int lastrow=rows-1,lastseg=segs-1,almostlast=lastseg-1;
	BOOL weirdpt=doPie && !noHemi,weirdmid=weirdpt && (rows==2);
	for(int ix=1; ix<rows; ++ix) {
		jx=(ix-1)*segs+1;
		for(int kx=0; kx<segs; ++kx) {
			issliceface=(doPie && (kx>=almostlast));

			na = jx+kx;
			nb = na+segs;
			nb = (weirdmid &&(kx==lastseg)? lastvert:na+segs);
			if ((weirdmid) &&(kx==almostlast)) nc=lastvert; else
				nc = (kx==lastseg)? jx+segs: nb+1;
			nd = (kx==lastseg)? jx : na+1;

			mesh.faces[nf].setEdgeVisFlags(1,1,0);


			if ((issliceface)&&((kx==almostlast-2)||(kx==almostlast)))
			{	mesh.faces[nf].setSmGroup(smooth?4:0);
			mesh.faces[nf].setMatID(2);
			if (usePhysUVs)
				startSliceFaces.Set(nf);
			}
			else if((issliceface)&&((kx==almostlast-1)||(kx==almostlast+1)))
			{	mesh.faces[nf].setSmGroup(smooth?8:0);
			mesh.faces[nf].setMatID(3);
			if (usePhysUVs)
				endSliceFaces.Set(nf);
			}
			else
			{	mesh.faces[nf].setSmGroup(smooth?1:0);
			mesh.faces[nf].setMatID(1); // mjm - 5.5.99 - rollback change - should be fixed in later release
			//				mesh.faces[nf].setMatID(0); // mjm - 3.2.99 - was set to 1
			}

			mesh.faces[nf].setVerts(na,nb,nc);
			nf++;

			mesh.faces[nf].setEdgeVisFlags(0,1,1);

			if ((issliceface)&&((kx==almostlast-2)||(kx==almostlast)))
			{	mesh.faces[nf].setSmGroup(smooth?4:0);
			mesh.faces[nf].setMatID(2);
			if (usePhysUVs)
				startSliceFaces.Set(nf);
			}
			else if((issliceface)&&((kx==almostlast-1)||(kx==almostlast+1)))
			{	mesh.faces[nf].setSmGroup(smooth?8:0);
			mesh.faces[nf].setMatID(3);
			if (usePhysUVs)
				endSliceFaces.Set(nf);
			}
			else
			{	mesh.faces[nf].setSmGroup(smooth?1:0);
			mesh.faces[nf].setMatID(1); // mjm - 5.5.99 - rollback change - should be fixed in later release
			//				mesh.faces[nf].setMatID(0); // mjm - 3.2.99 - was set to 1
			}

			mesh.faces[nf].setVerts(na,nc,nd);
			nf++;
		}
	}

	// Make bottom conic cap
	na = mesh.getNumVerts()-1;
	int botsegs=(weirdpt?segs-2:segs);
	jx = (rows-1)*segs+1;lastseg=botsegs-1;
	int fstart = nf;
	for(int ix=0; ix<botsegs; ++ix) {
		issliceface=(doPie && (ix>=botsegs-2));
		nc = ix + jx;
		nb = (!weirdpt && (ix==lastseg)?jx:nc+1);
		mesh.faces[nf].setEdgeVisFlags(1,1,1);

		if ((issliceface)&&(noHemi)&&(ix==botsegs-2))
		{	mesh.faces[nf].setSmGroup(smooth?4:0);
		mesh.faces[nf].setMatID(2);
		}
		else if ((issliceface)&&(noHemi)&&(ix==botsegs-1))
		{	mesh.faces[nf].setSmGroup(smooth?8:0);
		mesh.faces[nf].setMatID(3);
		}
		else if ((!issliceface)&&(noHemi))
		{	mesh.faces[nf].setSmGroup(smooth?1:0);
		mesh.faces[nf].setMatID(1); // mjm - 5.5.99 - rollback change - should be fixed in later release
		//			mesh.faces[nf].setMatID(0); // mjm - 3.2.99 - was set to 1
		}
		else if (!noHemi)
		{	mesh.faces[nf].setSmGroup(smooth?2:0);
		mesh.faces[nf].setMatID(0); // mjm - 5.5.99 - rollback change - should be fixed in later release
		//			mesh.faces[nf].setMatID(1); // mjm - 3.2.99 - was set to 0
		}
		//		else
		//		{	mesh.faces[nf].setSmGroup(0);
		//			mesh.faces[nf].setMatID(noHemi?1:0); // mjm - 5.5.99 - rollback change - should be fixed in later release
		//			mesh.faces[nf].setMatID(noHemi?0:1); // mjm - 3.2.99 - was commented out but set to 1:0
		//		}

		mesh.faces[nf].setVerts(na, nb, nc);

		nf++;
	}

	int fend = nf;
	// Put the flat part of the hemisphere at z=0
	if (recenter) {
		float shift = (float)cos(hemi) * radius;
		for (int ix=0; ix<mesh.getNumVerts(); ix++) {
			mesh.verts[ix].z -= shift;
		}
	}

	if (genUVs) {
		int tvsegs=segs;
		int tvpts=(doPie?segs+1:segs); 
		int ntverts = (rows+2)*(tvpts+1);
		//		if (doPie) {ntverts-=6; if (weirdpt) ntverts-3;}
		mesh.setNumTVerts(ntverts);
		mesh.setNumTVFaces(nfaces);
		nv = 0;
		delta  = basedelta;  // make the texture squash too
		alt = 0.0f; // = delta;
		float uScale = usePhysUVs ? ((float) 2.0f * PI * radius) : 1.0f;
		float vScale = usePhysUVs ? ((float) PI * radius) : 1.0f;
		int dsegs=(doPie?3:0),midsegs=tvpts-dsegs,m1=midsegs+1,t1=tvpts+1;
		for(int ix=0; ix < rows+2; ix++) {
			//	if (!noHemi && ix==rows) alt = hemi;		
			secang = 0.0f; //angle;
			float yang=1.0f-alt/PI;
			for(int jx=0; jx <= midsegs; ++jx) {
				mesh.setTVert(nv++, uScale*(secang/TWOPI), vScale*yang, 0.0f);
				secang += delta2;
			}
			for (int jx=0;jx<dsegs;jx++) mesh.setTVert(nv++,uScale*uval[jx],vScale*yang,0.0f);
			alt += delta;		
		}

		nf = 0;dsegs=(doPie?2:0),midsegs=segs-dsegs;
		// Make top conic cap
		for(int ix=0; ix<midsegs; ++ix) {
			mesh.tvFace[nf++].setTVerts(ix,ix+t1,ix+t1+1);
		} int ix=midsegs+1;int topv=ix+1;
		for (int jx=0;jx<dsegs;jx++)
		{ mesh.tvFace[nf++].setTVerts(topv,ix+t1,ix+t1+1);ix++;
		}
		int cpt;
		/* Make midsection */
		for(ix=1; ix<rows; ++ix) {
			cpt=ix*t1;
			for(int kx=0; kx<tvsegs; ++kx) {
				if (kx==midsegs) cpt++;
				na = cpt+kx;
				nb = na+t1;
				nc = nb+1;
				nd = na+1;
				assert(nc<ntverts);
				assert(nd<ntverts);
				mesh.tvFace[nf++].setTVerts(na,nb,nc);
				mesh.tvFace[nf++].setTVerts(na,nc,nd);
			}
		}
		// Make bottom conic cap
		if (noHemi || !usePhysUVs) {
			int lastv=rows*t1,jx=lastv+t1;
			if (weirdpt) dsegs=0;
			int j1;
			for (int j1=lastv; j1<lastv+midsegs; j1++) {
				mesh.tvFace[nf++].setTVerts(jx,j1+1,j1);jx++;
			}
			j1=lastv+midsegs+1;topv=j1+t1+1;
			for (int ix=0;ix<dsegs;ix++) {
				mesh.tvFace[nf++].setTVerts(topv,j1+1,j1);j1++;
			}
			assert(nf==nfaces);
		} else {
			Matrix3 m = TransMatrix(Point3(0.0f, 0.0f, -a));
			m.PreScale(Point3(1.0f, -1.0f, 1.0f));
			MakeMeshCapTexture(mesh, m, fstart, fend, usePhysUVs);
		}
		if (usePhysUVs) {
			Matrix3 tm = RotateZMatrix(-pie1) * RotateXMatrix(float(-PI/2.0));
			tm.Scale(Point3(-1.0f, 1.0f, 1.0f));
			MakeMeshCapTexture(mesh, tm, startSliceFaces, usePhysUVs);
			tm = RotateZMatrix(-pie2) * RotateXMatrix(float(-PI/2.0));
			MakeMeshCapTexture(mesh, tm, endSliceFaces, usePhysUVs);
		}
	}
	else {
		mesh.setNumTVerts(0);
		mesh.setNumTVFaces(0);
	}

	mesh.InvalidateTopologyCache();
}

// Triangular patch layout:
//
//   A---> ac ----- ca <---C
//   |                    / 
//   |                  /
//   v    i1    i3    /
//   ab            cb
//
//   |           /
//   |    i2   /
// 
//   ba     bc
//   ^     /
//   |   /
//   | /
//   B
//
// vertices ( a b c d ) are in counter clockwise order when viewed from 
// outside the surface

// Vector length for unit circle
#define CIRCLE_VECTOR_LENGTH 0.5517861843f

static void BuildSpherePatch(PatchMesh& amesh, float radius, int smooth, BOOL textured, BOOL usePhysUVs)
{
	Point3 p;	
	int np=0,nv=0;

	int nverts = 6;
	int nvecs = 48;
	int npatches = 8;
	amesh.setNumVerts(nverts);
	amesh.setNumTVerts(textured ? 13 : 0);
	amesh.setNumVecs(nvecs);
	amesh.setNumPatches(npatches);
	amesh.setNumTVPatches(textured ? npatches : 0);

	Point3 v0(0.0f, 0.0f, radius);		// Top
	Point3 v1(0.0f, 0.0f, -radius);		// Bottom
	Point3 v2(0.0f, -radius, 0.0f);		// Front
	Point3 v3(radius, 0.0f, 0.0f);		// Right
	Point3 v4(0.0f, radius, 0.0f);		// Back
	Point3 v5(-radius, 0.0f, 0.0f);		// Left

	// Create the vertices.
	amesh.verts[0].flags = PVERT_COPLANAR;
	amesh.verts[1].flags = PVERT_COPLANAR;
	amesh.verts[2].flags = PVERT_COPLANAR;
	amesh.verts[3].flags = PVERT_COPLANAR;
	amesh.verts[4].flags = PVERT_COPLANAR;
	amesh.verts[5].flags = PVERT_COPLANAR;
	amesh.setVert(0, v0);
	amesh.setVert(1, v1);
	amesh.setVert(2, v2);
	amesh.setVert(3, v3);
	amesh.setVert(4, v4);
	amesh.setVert(5, v5);

	if(textured) {
		float uScale = usePhysUVs ? ((float) 2.0f * PI * radius) : 1.0f;
		float vScale = usePhysUVs ? ((float) PI * radius) : 1.0f;

		amesh.setTVert(0, UVVert(uScale*0.125f,vScale*1.0f,0.0f));
		amesh.setTVert(1, UVVert(uScale*0.375f,vScale*1.0f,0.0f));
		amesh.setTVert(2, UVVert(uScale*0.625f,vScale*1.0f,0.0f));
		amesh.setTVert(3, UVVert(uScale*0.875f,vScale*1.0f,0.0f));
		amesh.setTVert(4, UVVert(uScale*0.0f,vScale*0.5f,0.0f));
		amesh.setTVert(5, UVVert(uScale*0.25f,vScale*0.5f,0.0f));
		amesh.setTVert(6, UVVert(uScale*0.5f,vScale*0.5f,0.0f));
		amesh.setTVert(7, UVVert(uScale*0.75f,vScale*0.5f,0.0f));
		amesh.setTVert(8, UVVert(uScale*1.0f,vScale*0.5f,0.0f));
		amesh.setTVert(9, UVVert(uScale*0.125f,vScale*0.0f,0.0f));
		amesh.setTVert(10, UVVert(uScale*0.375f,vScale*0.0f,0.0f));
		amesh.setTVert(11, UVVert(uScale*0.625f,vScale*0.0f,0.0f));
		amesh.setTVert(12, UVVert(uScale*0.875f,vScale*0.0f,0.0f));

		amesh.getTVPatch(0).setTVerts(3,7,8);
		amesh.getTVPatch(1).setTVerts(0,4,5);
		amesh.getTVPatch(2).setTVerts(1,5,6);
		amesh.getTVPatch(3).setTVerts(2,6,7);
		amesh.getTVPatch(4).setTVerts(12,8,7);
		amesh.getTVPatch(5).setTVerts(9,5,4);
		amesh.getTVPatch(6).setTVerts(10,6,5);
		amesh.getTVPatch(7).setTVerts(11,7,6);
	}

	// Create the edge vectors
	float vecLen = CIRCLE_VECTOR_LENGTH * radius;
	Point3 xVec(vecLen, 0.0f, 0.0f);
	Point3 yVec(0.0f, vecLen, 0.0f);
	Point3 zVec(0.0f, 0.0f, vecLen);
	amesh.setVec(0, v0 - yVec);
	amesh.setVec(2, v0 + xVec);
	amesh.setVec(4, v0 + yVec);
	amesh.setVec(6, v0 - xVec);
	amesh.setVec(8, v1 - yVec);
	amesh.setVec(10, v1 + xVec);
	amesh.setVec(12, v1 + yVec);
	amesh.setVec(14, v1 - xVec);
	amesh.setVec(9, v2 - zVec);
	amesh.setVec(16, v2 + xVec);
	amesh.setVec(1, v2 + zVec);
	amesh.setVec(23, v2 - xVec);
	amesh.setVec(11, v3 - zVec);
	amesh.setVec(18, v3 + yVec);
	amesh.setVec(3, v3 + zVec);
	amesh.setVec(17, v3 - yVec);
	amesh.setVec(13, v4 - zVec);
	amesh.setVec(20, v4 - xVec);
	amesh.setVec(5, v4 + zVec);
	amesh.setVec(19, v4 + xVec);
	amesh.setVec(15, v5 - zVec);
	amesh.setVec(22, v5 - yVec);
	amesh.setVec(7, v5 + zVec);
	amesh.setVec(21, v5 + yVec);

	// Create the patches
	amesh.MakeTriPatch(np++, 0, 0, 1, 2, 16, 17, 3, 3, 2, 24, 25, 26, smooth);
	amesh.MakeTriPatch(np++, 0, 2, 3, 3, 18, 19, 4, 5, 4, 27, 28, 29, smooth);
	amesh.MakeTriPatch(np++, 0, 4, 5, 4, 20, 21, 5, 7, 6, 30, 31, 32, smooth);
	amesh.MakeTriPatch(np++, 0, 6, 7, 5, 22, 23, 2, 1, 0, 33, 34, 35, smooth);
	amesh.MakeTriPatch(np++, 1, 10, 11, 3, 17, 16, 2, 9, 8, 36, 37, 38, smooth);
	amesh.MakeTriPatch(np++, 1, 12, 13, 4, 19, 18, 3, 11, 10, 39, 40, 41, smooth);
	amesh.MakeTriPatch(np++, 1, 14, 15, 5, 21, 20, 4, 13, 12, 42, 43, 44, smooth);
	amesh.MakeTriPatch(np++, 1, 8, 9, 2, 23, 22, 5, 15, 14, 45, 46, 47, smooth);

	// Create all the interior vertices and make them non-automatic
	float chi = 0.5893534f * radius;

	int interior = 24;
	amesh.setVec(interior++, Point3(chi, -chi, radius)); 
	amesh.setVec(interior++, Point3(chi, -radius, chi)); 
	amesh.setVec(interior++, Point3(radius, -chi, chi)); 

	amesh.setVec(interior++, Point3(chi, chi, radius)); 
	amesh.setVec(interior++, Point3(radius, chi, chi)); 
	amesh.setVec(interior++, Point3(chi, radius, chi)); 

	amesh.setVec(interior++, Point3(-chi, chi, radius)); 
	amesh.setVec(interior++, Point3(-chi, radius, chi)); 
	amesh.setVec(interior++, Point3(-radius, chi, chi)); 

	amesh.setVec(interior++, Point3(-chi, -chi, radius)); 
	amesh.setVec(interior++, Point3(-radius, -chi, chi)); 
	amesh.setVec(interior++, Point3(-chi, -radius, chi)); 

	amesh.setVec(interior++, Point3(chi, -chi, -radius)); 
	amesh.setVec(interior++, Point3(radius, -chi, -chi)); 
	amesh.setVec(interior++, Point3(chi, -radius, -chi)); 

	amesh.setVec(interior++, Point3(chi, chi, -radius)); 
	amesh.setVec(interior++, Point3(chi, radius, -chi)); 
	amesh.setVec(interior++, Point3(radius, chi, -chi)); 

	amesh.setVec(interior++, Point3(-chi, chi, -radius)); 
	amesh.setVec(interior++, Point3(-radius, chi, -chi)); 
	amesh.setVec(interior++, Point3(-chi, radius, -chi)); 

	amesh.setVec(interior++, Point3(-chi, -chi, -radius)); 
	amesh.setVec(interior++, Point3(-chi, -radius, -chi)); 
	amesh.setVec(interior++, Point3(-radius, -chi, -chi)); 

	for(int i = 0; i < 8; ++i)
		amesh.patches[i].SetAuto(FALSE);

	// Finish up patch internal linkages (and bail out if it fails!)
	if( !amesh.buildLinkages() )
	{
		DbgAssert(0);
	}

	// Calculate the interior bezier points on the PatchMesh's patches
	amesh.computeInteriors();
	amesh.InvalidateGeomCache();
}


Object *
	BuildNURBSSphere(float radius, float hemi, BOOL recenter, BOOL genUVs, BOOL doPie, float pie1, float pie2)
{
	NURBSSet nset;

	Point3 center(0,0,0);
	Point3 northAxis(0,0,1);
	Point3 refAxis(0,-1,0);

	if (recenter)
		center = Point3(0.0f, 0.0f, static_cast<float>(-cos((1.0f-hemi) * PI) * radius));

	NURBSCVSurface *surf = new NURBSCVSurface();
	nset.AppendObject(surf);
	surf->SetGenerateUVs(genUVs);

	surf->SetTextureUVs(0, 0, Point2(0.0f, hemi));
	surf->SetTextureUVs(0, 1, Point2(0.0f, 1.0f));
	surf->SetTextureUVs(0, 2, Point2(1.0f, hemi));
	surf->SetTextureUVs(0, 3, Point2(1.0f, 1.0f));

	surf->FlipNormals(TRUE);
	surf->Renderable(TRUE);
	TCHAR bname[80];
	TCHAR sname[80];
	_tcscpy(bname, GetString(IDS_RB_SPHERE));
	_stprintf(sname, _T("%s%s"), bname, GetString(IDS_CT_SURF));
	surf->SetName(sname);

	float startAngleU = 0.0f;
	float endAngleU = TWOPI;
	pie1 += HALFPI;
	pie2 += HALFPI;
	if (doPie && pie1 != pie2) {
		float sweep = TWOPI - (pie2-pie1);
		if (sweep > TWOPI) sweep -= TWOPI;
		refAxis = Point3(Point3(1,0,0) * RotateZMatrix(pie2));
		endAngleU = sweep;
		if (fabs(endAngleU) < 1e-5) endAngleU = TWOPI;
	}
	if (hemi == 0.0f && (!doPie || endAngleU == TWOPI)) {
		GenNURBSSphereSurface(radius, center, northAxis, Point3(0,-1,0),
			-PI, PI, -HALFPI, HALFPI,
			FALSE, *surf);
	} else if (hemi > 0.0f && (!doPie || endAngleU == TWOPI)) {
		GenNURBSSphereSurface(radius, center, northAxis, Point3(0,-1,0),
			-PI, PI, -HALFPI + (hemi * PI), HALFPI,
			FALSE, *surf);
		// now cap it
		NURBSCapSurface *cap0 = new NURBSCapSurface();
		nset.AppendObject(cap0);
		cap0->SetGenerateUVs(genUVs);
		cap0->SetParent(0);
		cap0->SetEdge(0);
		cap0->FlipNormals(TRUE);
		cap0->Renderable(TRUE);
		TCHAR sname[80];
		_stprintf(sname, _T("%s%s%02d"), bname, GetString(IDS_CT_CAP), 0);
		cap0->SetName(sname);
	} else {
		float startAngleV = -HALFPI + (hemi * PI);
		float endAngleV = HALFPI;
		GenNURBSSphereSurface(radius, center, northAxis, refAxis,
			startAngleU, endAngleU, startAngleV, endAngleV,
			TRUE, *surf);
#define F(s1, s2, s1r, s1c, s2r, s2c) \
	fuse.mSurf1 = (s1); \
	fuse.mSurf2 = (s2); \
	fuse.mRow1 = (s1r); \
	fuse.mCol1 = (s1c); \
	fuse.mRow2 = (s2r); \
	fuse.mCol2 = (s2c); \
	nset.mSurfFuse.Append(1, &fuse);

		NURBSFuseSurfaceCV fuse;

		// pole(s)
		for (int f = 1; f < surf->GetNumVCVs(); f++) {
			if (hemi <= 0.0f) {
				// south pole
				F(0, 0, 0, 0, 0, f);
			}
			//north pole
			F(0, 0, surf->GetNumUCVs()-1, 0, surf->GetNumUCVs()-1, f);
		}

		NURBSCVSurface *s0 = (NURBSCVSurface*)nset.GetNURBSObject(0);
		int numU, numV;
		s0->GetNumCVs(numU, numV);


		if (doPie && endAngleU > 0.0f && endAngleU < TWOPI) {
			// next the two pie slices
			for (int c = 0; c < 2; c++) {
				NURBSCVSurface *s = new NURBSCVSurface();
				nset.AppendObject(s);
				// we'll be cubic in on direction and match the sphere in the other
				s->SetUOrder(s0->GetUOrder());
				int numKnots = s0->GetNumUKnots();
				s->SetNumUKnots(numKnots);
				for (int i = 0; i < numKnots; i++)
					s->SetUKnot(i, s0->GetUKnot(i));

				s->SetVOrder(4);
				s->SetNumVKnots(8);
				for (int i = 0; i < 4; i++) {
					s->SetVKnot(i, 0.0);
					s->SetVKnot(i+4, 1.0);
				}

				s->SetNumCVs(numU, 4);
				for (int v = 0; v < 4; v++) {
					for (int u = 0; u < numU; u++) {
						if (v == 0) { // outside edge
							if (c == 0) {
								s->SetCV(u, v, *s0->GetCV(u, 0));
								F(0, 1, u, 0, u, v);
							} else {
								s->SetCV(u, v, *s0->GetCV(u, numV-1));
								F(0, 2, u, numV-1, u, v);
							}
						} else
							if (v == 3) { // center axis
								Point3 p(0.0f, 0.0f, s0->GetCV(u, 0)->GetPosition(0).z);
								NURBSControlVertex ncv;
								ncv.SetPosition(0, p);
								ncv.SetWeight(0, 1.0f);
								s->SetCV(u, v, ncv);
								F(1, c+1, u, 3, u, v);
							} else {
								Point3 center(0.0f, 0.0f, s0->GetCV(u, 0)->GetPosition(0).z);
								Point3 edge;
								if (c == 0)
									edge = Point3(s0->GetCV(u, 0)->GetPosition(0));
								else
									edge = Point3(s0->GetCV(u, numV-1)->GetPosition(0));
								NURBSControlVertex ncv;
								ncv.SetPosition(0, center + ((edge - center)*(float)v/3.0f));
								ncv.SetWeight(0, 1.0f);
								s->SetCV(u, v, ncv);
							}
					}
				}
				s->SetGenerateUVs(genUVs);

				s->SetTextureUVs(0, 0, Point2(0.0f, 0.0f));
				s->SetTextureUVs(0, 1, Point2(0.0f, 1.0f));
				s->SetTextureUVs(0, 2, Point2(1.0f, 0.0f));
				s->SetTextureUVs(0, 3, Point2(1.0f, 1.0f));

				if (c == 0)
					s->FlipNormals(FALSE);
				else
					s->FlipNormals(TRUE);
				s->Renderable(TRUE);
				_stprintf(sname, _T("%s%s%02d"), bname, GetString(IDS_CT_SLICE), c+1);
				s->SetName(sname);
			}
		}

		if (hemi > 0.0f) {
			// Cap -- we will always have slices since we
			// handle the non-slice cases with cap surfaces

			NURBSCVSurface *s = new NURBSCVSurface();
			s->SetGenerateUVs(genUVs);

			s->SetTextureUVs(0, 0, Point2(1.0f, 1.0f));
			s->SetTextureUVs(0, 1, Point2(0.0f, 1.0f));
			s->SetTextureUVs(0, 2, Point2(1.0f, 0.0f));
			s->SetTextureUVs(0, 3, Point2(0.0f, 0.0f));

			s->FlipNormals(TRUE);
			s->Renderable(TRUE);
			_stprintf(sname, _T("%s%s"), bname, GetString(IDS_CT_CAP));
			s->SetName(sname);
			int cap = nset.AppendObject(s);

			// we'll be cubic in on direction and match the sphere in the other
			s->SetUOrder(4);
			s->SetNumUKnots(8);
			for (int i = 0; i < 4; i++) {
				s->SetUKnot(i, 0.0);
				s->SetUKnot(i+4, 1.0);
			}

			s->SetVOrder(s0->GetVOrder());
			s->SetNumVKnots(s0->GetNumVKnots());
			for (int i = 0; i < s->GetNumVKnots(); i++)
				s->SetVKnot(i, s0->GetVKnot(i));

			s->SetNumCVs(4, numV);

			Point3 bot;
			if (recenter)
				bot = Point3(0,0,0);
			else
				bot = Point3(0.0, 0.0, cos((1.0-hemi) * PI)*radius);
			for (int v = 0; v < numV; v++) {
				Point3 edge = s0->GetCV(0, v)->GetPosition(0);
				double w = s0->GetCV(0, v)->GetWeight(0);
				for (int u = 0; u < 4; u++) {
					NURBSControlVertex ncv;
					ncv.SetPosition(0, bot + ((edge - bot)*((float)u/3.0f)));
					ncv.SetWeight(0, w);
					s->SetCV(u, v, ncv);
					if (u == 3) {
						// fuse the cap to the sphere
						F(cap, 0, 3, v, 0, v);
					}
					if (u == 1 || u == 2) {
						// fuse the ends to the slices
						if (v == 0) {
							F(cap, 1, u, v, 0, u);
						}
						if (v == numV-1) {
							F(cap, 2, u, v, 0, u);
						}
					}
				}

				if (v > 0) {
					// fuse the center degeneracy
					F(cap, cap, 0, 0, 0, v);
				}
			}
		}
	}

	Matrix3 mat;
	mat.IdentityMatrix();
	Object *ob = CreateNURBSObject(NULL, &nset, mat);
	return ob;
}


Object* SphereObject::ConvertToType(TimeValue t, Class_ID obtype)
{
	if (obtype == patchObjectClassID) {
		Interval valid = FOREVER;
		float radius;
		int smooth, genUVs;
		pblock2->GetValue(sphere_radius,t,radius,valid);
		pblock2->GetValue(sphere_smooth,t,smooth,valid);
		pblock2->GetValue(sphere_mapping,t,genUVs,valid);
		PatchObject *ob = new PatchObject();
		BuildSpherePatch(ob->patch,radius,smooth,genUVs,GetUsePhysicalScaleUVs());
		ob->SetChannelValidity(TOPO_CHAN_NUM,valid);
		ob->SetChannelValidity(GEOM_CHAN_NUM,valid);
		ob->UnlockObject();
		return ob;
	} 
	if (obtype == EDITABLE_SURF_CLASS_ID) {
		Interval valid = FOREVER;
		float radius, hemi;
		int recenter, genUVs;
		float pie1, pie2;
		BOOL doPie;
		pblock2->GetValue(sphere_radius,t,radius,valid);
		pblock2->GetValue(sphere_hemi,t,hemi,valid);
		pblock2->GetValue(sphere_recenter,t,recenter,valid);
		pblock2->GetValue(sphere_mapping,t,genUVs,valid);
		pblock2->GetValue(sphere_pieslice1,t,pie1,ivalid);
		pblock2->GetValue(sphere_pieslice2,t,pie2,ivalid);
		pblock2->GetValue(sphere_slice,t,doPie,ivalid);
		Object *ob = BuildNURBSSphere(radius, hemi, recenter,genUVs, doPie, pie1, pie2);
		ob->SetChannelValidity(TOPO_CHAN_NUM,valid);
		ob->SetChannelValidity(GEOM_CHAN_NUM,valid);
		ob->UnlockObject();
		return ob;

	}

	return __super::ConvertToType(t,obtype);

}

int SphereObject::CanConvertToType(Class_ID obtype)
{
	if(obtype==defObjectClassID ||	obtype==triObjectClassID) return 1;
	if(obtype == patchObjectClassID) return 1;
	if(obtype == EDITABLE_SURF_CLASS_ID) return 1;
	return __super::CanConvertToType(obtype);
}


void SphereObject::GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist)
{
	__super::GetCollapseTypes(clist, nlist);
	Class_ID id = EDITABLE_SURF_CLASS_ID;
	TSTR *name = new TSTR(GetString(IDS_SM_NURBS_SURFACE));
	clist.Append(1,&id);
	nlist.Append(1,&name);
}

class SphereObjCreateCallBack : public CreateMouseCallBack {
	IPoint2 sp0;
	SphereObject *ob;
	Point3 p0;
public:
	int proc( ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat) override;
	void SetObj(SphereObject *obj) {ob = obj;}
};

int SphereObjCreateCallBack::proc(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat ) {

	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}


	float r;
	Point3 p1,center;

	if (msg == MOUSE_FREEMOVE)
	{
		vpt->SnapPreview(m,m,NULL, SNAP_IN_3D);
	}

	if (msg==MOUSE_POINT||msg==MOUSE_MOVE) {
		switch(point) {
		case 0:  // only happens with MOUSE_POINT msg
			ob->pblock2->SetValue(sphere_radius,0,0.0f);
			ob->suspendSnap = TRUE;				
			sp0 = m;
			p0 = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);
			mat.SetTrans(p0);
			break;
		case 1:
			mat.IdentityMatrix();
			//mat.PreRotateZ(HALFPI);
			p1 = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);
			bool createByRadius = sphere_crtype_blk.GetInt(sphere_create_meth) == 1;
			if (createByRadius) {
				//radius
				r = Length(p1-p0);
				mat.SetTrans(p0);
			}
			else {
				//diameter
				center = (p0+p1)/float(2);
				mat.SetTrans(center);
				r = Length(center-p0);
			} 
			ob->pblock2->SetValue(sphere_radius,0,r);

			if (flags&MOUSE_CTRL) {
				float ang = (float)atan2(p1.y-p0.y,p1.x-p0.x);					
				mat.PreRotateZ(ob->ip->SnapAngle(ang));
			}

			if (msg==MOUSE_POINT) {
				ob->suspendSnap = FALSE;
				return (Length(m-sp0)<3 )?CREATE_ABORT:CREATE_STOP;
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

static SphereObjCreateCallBack sphereCreateCB;

CreateMouseCallBack* SphereObject::GetCreateMouseCallBack() 
{
	sphereCreateCB.SetObj(this);
	return(&sphereCreateCB);
}


BOOL SphereObject::OKtoDisplay(TimeValue t) 
{
	float radius;
	pblock2->GetValue(sphere_radius,t,radius,FOREVER);
	if (radius==0.0f) return FALSE;
	else return TRUE;
}

// From GeomObject
int SphereObject::IntersectRay(
	TimeValue t, Ray& ray, float& at, Point3& norm)
{
	int smooth, recenter;
	pblock2->GetValue(sphere_smooth,t,smooth,FOREVER);
	pblock2->GetValue(sphere_recenter,t,recenter,FOREVER);	
	float hemi;
	pblock2->GetValue(sphere_hemi,t,hemi,FOREVER);
	if (!smooth || hemi!=0.0f || recenter) {
		return __super::IntersectRay(t,ray,at,norm);
	}	

	float r;
	float a, b, c, ac4, b2, at1, at2;
	float root;
	BOOL neg1, neg2;

	pblock2->GetValue(sphere_radius,t,r,FOREVER);

	a = DotProd(ray.dir,ray.dir);
	b = DotProd(ray.dir,ray.p) * 2.0f;
	c = DotProd(ray.p,ray.p) - r*r;

	ac4 = 4.0f * a * c;
	b2 = b*b;

	if (ac4 > b2) return 0;

	// We want the smallest positive root
	root = float(sqrt(b2-ac4));
	at1 = (-b + root) / (2.0f * a);
	at2 = (-b - root) / (2.0f * a);
	neg1 = at1<0.0f;
	neg2 = at2<0.0f;
	if (neg1 && neg2) return 0;
	else
		if (neg1 && !neg2) at = at2;
		else 
			if (!neg1 && neg2) at = at1;
			else
				if (at1<at2) at = at1;
				else at = at2;

				norm = Normalize(ray.p + at*ray.dir);

				return 1;
}

void SphereObject::InvalidateUI() 
{
	sphere_param_blk.InvalidateUI(pblock2->LastNotifyParamID());
}

RefTargetHandle SphereObject::Clone(RemapDir& remap) 
{
	SphereObject* newob = new SphereObject(FALSE);	
	newob->ReplaceReference(0,remap.CloneRef(pblock2));
	newob->ivalid.SetEmpty();	
	BaseClone(this, newob, remap);
	newob->SetUsePhysicalScaleUVs(GetUsePhysicalScaleUVs());
	return(newob);
}

void SphereObject::UpdateUI()
{
	if (ip == NULL)
		return;
	SphereParamDlgProc* dlg = static_cast<SphereParamDlgProc*>(sphere_param_blk.GetUserDlgProc());
	dlg->UpdateUI();
}

BOOL SphereObject::GetUsePhysicalScaleUVs()
{
	return ::GetUsePhysicalScaleUVs(this);
}


void SphereObject::SetUsePhysicalScaleUVs(BOOL flag)
{
	BOOL curState = GetUsePhysicalScaleUVs();
	if (curState == flag)
		return;
	if (theHold.Holding())
		theHold.Put(new RealWorldScaleRecord<SphereObject>(this, curState));
	::SetUsePhysicalScaleUVs(this, flag);
	if (pblock2 != NULL)
		pblock2->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	UpdateUI();
	macroRec->SetProperty(this, _T("realWorldMapSize"), mr_bool, flag);
}

