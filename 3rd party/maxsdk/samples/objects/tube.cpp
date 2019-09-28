/**********************************************************************
 *<
	FILE: tube.cpp

	DESCRIPTION:  A tube object

	CREATED BY: Rolf Berteig

	HISTORY: created 13 September 1994

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#include "prim.h"

#include "iparamm2.h"
#include "Simpobj.h"
#include "surf_api.h"
#include "macroRec.h"
#include "RealWorldMapUtils.h"
#include <ReferenceSaveManager.h>
#include <Util/StaticAssert.h>

class TubeObject : public SimpleObject2, public RealWorldMapSizeInterface {
	friend class TubeTypeInDlgProc;
	friend class TubeObjCreateCallBack;
	friend class TubeParamDlgProc;
	public:
		// Class vars
		static IObjParam *ip;
		static bool typeinCreate;

		TubeObject(BOOL loading);
		
		// From Object
		int CanConvertToType(Class_ID obtype) override;
		Object* ConvertToType(TimeValue t, Class_ID obtype) override;
		void GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist) override;
		BOOL HasUVW() override;
		void SetGenUVW(BOOL sw) override;
			
		// From BaseObject
		CreateMouseCallBack* GetCreateMouseCallBack() override;
		void BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev) override;
		void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next) override;
		const TCHAR *GetObjectName() override { return GetString(IDS_RB_TUBE);}

		// Animatable methods		
		void DeleteThis() override {delete this;}
		Class_ID ClassID() override {return Class_ID(TUBE_CLASS_ID,0);}
		
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
			else {
				BaseInterface* intf = SimpleObject2::GetInterface(id);
				if (intf)
					return intf;
				else
					return FPMixinInterface::GetInterface(id);
			}
		}
	};

// segments  = sides
// lsegments = segments

#define MIN_SEGMENTS	1
#define MAX_SEGMENTS	200

#define MIN_SIDES		1
#define MAX_SIDES		200

#define MIN_RADIUS		float(0)
#define MAX_RADIUS		float( 1.0E30)
#define MIN_PIESLICE	float(-1.0E30)
#define MAX_PIESLICE	float( 1.0E30)

#define DEF_SEGMENTS 	18	// 24
#define DEF_SIDES		5	// 1
#define DEF_CAPSEGMENTS	1

#define DEF_RADIUS		(0.1f)
#define DEF_RADIUS2   	(10.0f)

#define SMOOTH_ON		1
#define SMOOTH_SIDES	1
#define SMOOTH_OFF		0

//--- ClassDescriptor and class vars ---------------------------------

static BOOL sInterfaceAdded = FALSE;

class TubeClassDesc:public ClassDesc2 {
	public:
	int 			IsPublic() override { return 1; }
	void *			Create(BOOL loading = FALSE) override
    {
		if (!sInterfaceAdded) {
			AddInterface(&gRealWorldMapSizeDesc);
			sInterfaceAdded = TRUE;
		}
        return new TubeObject(loading);
    }
	const TCHAR *	ClassName() override { return GetString(IDS_RB_TUBE_CLASS);}
	SClass_ID		SuperClassID() override { return GEOMOBJECT_CLASS_ID; }
	Class_ID		ClassID() override { return Class_ID(TUBE_CLASS_ID,0); }
	const TCHAR* 	Category() override { return GetString(IDS_RB_PRIMITIVES); }
	const TCHAR*	InternalName() override { return _T("Tube"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() override { return hInstance; }			// returns owning module handle

	};

static TubeClassDesc tubeDesc;

ClassDesc* GetTubeDesc() { return &tubeDesc; }

// in prim.cpp  - The dll instance handle
extern HINSTANCE hInstance;

// class variable for tube class.
IObjParam *TubeObject::ip         = NULL;
bool TubeObject::typeinCreate = false;

#define PBLOCK_REF_NO	 0

// ParamBlockDesc2 IDs
enum paramblockdesc2_ids { tube_creation_type, tube_type_in, tube_params, };
enum tube_creation_type_param_ids { tube_create_meth, };
enum tube_type_in_param_ids { tube_ti_pos, tube_ti_radius1, tube_ti_radius2, tube_ti_height, };
enum tube_param_param_ids {
	tube_radius1 = TUBE_RADIUS, tube_radius2 = TUBE_RADIUS2, tube_height = TUBE_HEIGHT,
	tube_segs = TUBE_SEGMENTS, tube_capsegs = TUBE_CAPSEGMENTS, tube_sides = TUBE_SIDES,
	tube_smooth = TUBE_SMOOTH, tube_slice = TUBE_SLICEON, tube_pieslice1 = TUBE_PIESLICE1,
	tube_pieslice2 = TUBE_PIESLICE2, tube_mapping = TUBE_GENUVS,
};

namespace
{
	MaxSDK::Util::StaticAssert< (tube_params == TUBE_PARAMBLOCK_ID) > validator;
}

namespace
{
	class MapCoords_Accessor : public PBAccessor
	{
		void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
		{
			if (owner) ((TubeObject*)owner)->UpdateUI();
		}
	};
	static MapCoords_Accessor mapCoords_Accessor;
}

// class creation type block
static ParamBlockDesc2 tube_crtype_blk(tube_creation_type, _T("TubeCreationType"), 0, &tubeDesc, P_CLASS_PARAMS + P_AUTO_UI,
	//rollout
	IDD_TUBEPARAM1, IDS_RB_CREATIONMETHOD, BEGIN_EDIT_CREATE, 0, NULL,
	// params
	tube_create_meth, _T("typeinCreationMethod"), TYPE_INT, 0, IDS_RB_CREATIONMETHOD,
	p_default, 1,
	p_range, 0, 1,
	p_ui, TYPE_RADIO, 2, IDC_CREATEDIAMETER, IDC_CREATERADIUS,
	p_end,
	p_end
	);

// class type-in block
static ParamBlockDesc2 tube_typein_blk(tube_type_in, _T("TubeTypeIn"), 0, &tubeDesc, P_CLASS_PARAMS + P_AUTO_UI,
	//rollout
	IDD_TUBEPARAM3, IDS_RB_KEYBOARDENTRY, BEGIN_EDIT_CREATE, APPENDROLL_CLOSED, NULL,
	// params
	tube_ti_pos, _T("typeInPos"), TYPE_POINT3, 0, IDS_RB_TYPEIN_POS,
	p_default, Point3(0, 0, 0),
	p_range, float(-1.0E30), float(1.0E30),
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_TI_POSX, IDC_TI_POSXSPIN, IDC_TI_POSY, IDC_TI_POSYSPIN, IDC_TI_POSZ, IDC_TI_POSZSPIN, SPIN_AUTOSCALE,
	p_end,
	tube_ti_radius1, _T("typeInRadius1"), TYPE_FLOAT, 0, IDS_RB_RADIUS1,
	p_default, 0.0f,
	p_range, MIN_RADIUS, MAX_RADIUS,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_RADIUS1, IDC_RADSPINNER1, SPIN_AUTOSCALE,
	p_end,
	tube_ti_radius2, _T("typeInRadius2"), TYPE_FLOAT, 0, IDS_RB_RADIUS2,
	p_default, 0.0f,
	p_range, MIN_RADIUS, MAX_RADIUS,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_RADIUS2, IDC_RAD2SPINNER, SPIN_AUTOSCALE,
	p_end,
	tube_ti_height, _T("typeInHeight"), TYPE_FLOAT, 0, IDS_RB_HEIGHT,
	p_default, 0.0f,
	p_range, float(-1.0E30), float(1.0E30),
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_LENGTH, IDC_LENSPINNER, SPIN_AUTOSCALE,
	p_end,
	p_end
	);

// per instance tube block
static ParamBlockDesc2 tube_param_blk(tube_params, _T("TubeParameters"), 0, &tubeDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_REF_NO,
	//rollout
	IDD_TUBEPARAM2, IDS_RB_PARAMETERS, 0, 0, NULL,
	// params
	tube_radius1, _T("radius1"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_RADIUS1,
	p_default, DEF_RADIUS,
	p_ms_default, 25.0,
	p_range, MIN_RADIUS, MAX_RADIUS,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_RADIUS1, IDC_RADSPINNER1, SPIN_AUTOSCALE,
	p_end,
	tube_radius2, _T("radius2"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_RADIUS2,
	p_default, DEF_RADIUS,
	p_ms_default, 20.0,
	p_range, MIN_RADIUS, MAX_RADIUS,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_RADIUS2, IDC_RAD2SPINNER, SPIN_AUTOSCALE,
	p_end,
	tube_height, _T("height"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_HEIGHT,
	p_default, 0.0,
	p_ms_default, 50.0,
	p_range, float(-1.0E30), float(1.0E30),
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_LENGTH, IDC_LENSPINNER, SPIN_AUTOSCALE,
	p_end,
	tube_segs, _T("sides"), TYPE_INT, P_ANIMATABLE, IDS_RB_SIDES,
	p_default, DEF_SEGMENTS,
	p_ms_default, 24,
	p_range, 3, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_SEGMENTS, IDC_SEGSPINNER, 0.1f,
	p_end,
	tube_capsegs, _T("capsegs"), TYPE_INT, P_ANIMATABLE, IDS_RB_CAPSEGMENTS,
	p_default, DEF_CAPSEGMENTS,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_CAPSEGMENTS, IDC_CAPSEGSPINNER, 0.1f,
	p_end,
	tube_sides, _T("heightsegs"), TYPE_INT, P_ANIMATABLE, IDS_RB_HEIGHTSEGS,
	p_default, DEF_SIDES,
	p_ms_default, 1,
	p_range, MIN_SIDES, MAX_SIDES,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_SIDES, IDC_SIDESPINNER, 0.1f,
	p_end,
	tube_smooth, _T("smooth"), TYPE_BOOL, P_ANIMATABLE, IDS_RB_SMOOTH,
	p_default, TRUE,
	p_ui, TYPE_SINGLECHECKBOX, IDC_OBSMOOTH,
	p_end,
	tube_slice, _T("slice"), TYPE_BOOL, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_SLICEON,
	p_default, FALSE,
	p_ui, TYPE_SINGLECHECKBOX, IDC_SLICEON,
	p_enable_ctrls, 2, tube_pieslice1, tube_pieslice2,
	p_end,
	tube_pieslice1, _T("sliceFrom"), TYPE_ANGLE, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_SLICEFROM,
	p_default, 0.0,
	p_range, MIN_PIESLICE, MAX_PIESLICE,
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_PIESLICE1, IDC_PIESLICESPIN1, 0.5f,
	p_end,
	tube_pieslice2, _T("sliceTo"), TYPE_ANGLE, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_SLICETO,
	p_default, 0.0,
	p_range, MIN_PIESLICE, MAX_PIESLICE,
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_PIESLICE2, IDC_PIESLICESPIN2, 0.5f,
	p_end,
	tube_mapping, _T("mapCoords"), TYPE_BOOL, 0, IDS_RB_GENTEXCOORDS,
	p_default, TRUE,
	p_ms_default, FALSE,
	p_ui, TYPE_SINGLECHECKBOX, IDC_GENTEXTURE,
	p_accessor, &mapCoords_Accessor,
	p_end,
	p_end
	);

static ParamBlockDescID descVer0[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0 },
	{ TYPE_FLOAT, NULL, TRUE, 1 },
	{ TYPE_FLOAT, NULL, TRUE, 2 },
	{ TYPE_INT, NULL, TRUE, 3 },
	{ TYPE_INT, NULL, TRUE, 4 },
	{ TYPE_INT, NULL, TRUE, 5 },
	{ TYPE_INT, NULL, TRUE, 6 },
	{ TYPE_INT, NULL, TRUE, 7 },
	{ TYPE_FLOAT, NULL, TRUE, 8 },
	{ TYPE_FLOAT, NULL, TRUE, 9 } };

static ParamBlockDescID descVer1[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0 },
	{ TYPE_FLOAT, NULL, TRUE, 1 },
	{ TYPE_FLOAT, NULL, TRUE, 2 },
	{ TYPE_INT, NULL, TRUE, 3 },
	{ TYPE_INT, NULL, TRUE, 4 },
	{ TYPE_INT, NULL, TRUE, 5 },
	{ TYPE_BOOL, NULL, TRUE, 6 },
	{ TYPE_INT, NULL, TRUE, 7 },
	{ TYPE_FLOAT, NULL, TRUE, 8 },
	{ TYPE_FLOAT, NULL, TRUE, 9 },
	{ TYPE_INT, NULL, FALSE, 10 } };


// Array of old versions
static ParamVersionDesc versions[] = {
	ParamVersionDesc(descVer0,10,0),
	ParamVersionDesc(descVer1,11,1)	
	};
#define NUM_OLDVERSIONS	2

// ParamBlock data for SaveToPrevious support
#define PBLOCK_LENGTH	11
#define CURRENT_VERSION	1


//--- TypeInDlgProc --------------------------------

class TubeTypeInDlgProc : public ParamMap2UserDlgProc {
	public:
		TubeObject *ob;

		TubeTypeInDlgProc(TubeObject *o) {ob=o;}
		INT_PTR DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam) override;
		void DeleteThis() override {delete this;}
	};

INT_PTR TubeTypeInDlgProc::DlgProc(
		TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
	{
	switch (msg) {
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_TI_CREATE: {
					if (tube_typein_blk.GetFloat(tube_ti_radius1) ==0.0) return TRUE;
					
					// We only want to set the value if the object is 
					// not in the scene.
					if (ob->TestAFlag(A_OBJ_CREATING)) {
						ob->pblock2->SetValue(tube_radius1, 0, tube_typein_blk.GetFloat(tube_ti_radius1));
						ob->pblock2->SetValue(tube_radius2, 0, tube_typein_blk.GetFloat(tube_ti_radius2));
						ob->pblock2->SetValue(tube_height, 0, tube_typein_blk.GetFloat(tube_ti_height));
					}
					else
						TubeObject::typeinCreate = true;

					Matrix3 tm(1);
					tm.SetTrans(tube_typein_blk.GetPoint3(tube_ti_pos));
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





class TubeParamDlgProc : public ParamMap2UserDlgProc {
	public:
		TubeObject *so;
		HWND thishWnd;

		TubeParamDlgProc(TubeObject *s) {so=s;thishWnd=NULL;}
		INT_PTR DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam) override;
		void UpdateUI();
		void DeleteThis() override {delete this;}
        BOOL GetRWSState();
	};

BOOL TubeParamDlgProc::GetRWSState()
{
    BOOL check = IsDlgButtonChecked(thishWnd, IDC_REAL_WORLD_MAP_SIZE2);
    return check;
}

void TubeParamDlgProc::UpdateUI()
{
	if (!thishWnd) return;
	BOOL usePhysUVs = so->GetUsePhysicalScaleUVs();
	CheckDlgButton(thishWnd, IDC_REAL_WORLD_MAP_SIZE2, usePhysUVs);
	EnableWindow(GetDlgItem(thishWnd, IDC_REAL_WORLD_MAP_SIZE2), so->HasUVW());
}

INT_PTR TubeParamDlgProc::DlgProc(
	TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	thishWnd = hWnd;
	switch (msg) {
	case WM_INITDIALOG:
		UpdateUI();
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_GENTEXTURE:
			UpdateUI();
			break;
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


//--- Tube methods -------------------------------


TubeObject::TubeObject(BOOL loading)
	{	
	SetAFlag(A_PLUGIN1);
	tubeDesc.MakeAutoParamBlocks(this);
	if (!loading && !GetPhysicalScaleUVsDisabled())
		SetUsePhysicalScaleUVs(true);
	}

bool TubeObject::SpecifySaveReferences(ReferenceSaveManager& referenceSaveManager)
{
	// if saving to previous version that used pb1 instead of pb2...
	DWORD saveVersion = GetSavingVersion();
	if (saveVersion != 0 && saveVersion <= MAX_RELEASE_R19)
	{
		ProcessPB2ToPB1SaveToPrevious(this, pblock2, PBLOCK_REF_NO, descVer1, PBLOCK_LENGTH, CURRENT_VERSION);
	}
	return __super::SpecifySaveReferences(referenceSaveManager);
}


#define NEWMAP_CHUNKID	0x0100

IOResult TubeObject::Load(ILoad *iload) 
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
		new ParamBlock2PLCB(versions, NUM_OLDVERSIONS, &tube_param_blk, this, 0));
	return IO_OK;
	}

IOResult TubeObject::Save(ISave *isave)
	{
	if (TestAFlag(A_PLUGIN1)) {
		isave->BeginChunk(NEWMAP_CHUNKID);
		isave->EndChunk();
		}
 	return IO_OK;
	}

void TubeObject::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	__super::BeginEditParams(ip, flags, prev);
	this->ip = ip;
	// If this has been freshly created by type-in, set creation values:
	if (TubeObject::typeinCreate)
	{
		pblock2->SetValue(tube_radius1, 0, tube_typein_blk.GetFloat(tube_ti_radius1));
		pblock2->SetValue(tube_radius2, 0, tube_typein_blk.GetFloat(tube_ti_radius2));
		pblock2->SetValue(tube_height, 0, tube_typein_blk.GetFloat(tube_ti_height));
		TubeObject::typeinCreate = false;
	}

	// throw up all the appropriate auto-rollouts
	tubeDesc.BeginEditParams(ip, this, flags, prev);
	// if in Create Panel, install a callback for the type in.
	if (flags & BEGIN_EDIT_CREATE)
		tube_typein_blk.SetUserDlgProc(new TubeTypeInDlgProc(this));
	// install a callback for the params.
	tube_param_blk.SetUserDlgProc(new TubeParamDlgProc(this));
}
		
void TubeObject::EndEditParams( IObjParam *ip, ULONG flags,Animatable *next )
{
	__super::EndEditParams(ip, flags, next);
	this->ip = NULL;
	tubeDesc.EndEditParams(ip, this, flags, next);
}

BOOL TubeObject::HasUVW() { 
	BOOL genUVs;
	Interval v;
	pblock2->GetValue(tube_mapping, 0, genUVs, v);
	return genUVs; 
	}

void TubeObject::SetGenUVW(BOOL sw) {  
	if (sw==HasUVW()) return;
	pblock2->SetValue(tube_mapping,0, sw);
	}

void TubeObject::BuildMesh(TimeValue t)
	{
	Point3 p;
	int ix,na,nb,nc,nd,jx,kx;
	int nf=0,nv=0;
	float delta,ang;	
	int sides,segs,smooth,capsegs, ssides;
	float radius,radius2, temp;
	float sinang,cosang, height;
	float pie1, pie2, totalPie, startAng = 0.0f;
	int doPie = TRUE;	
	int genUVs = TRUE;

	// Start the validity interval at forever and widdle it down.
	ivalid = FOREVER;	
	pblock2->GetValue(tube_radius1,t,radius,ivalid);
	pblock2->GetValue(tube_radius2,t,radius2,ivalid);
	pblock2->GetValue(tube_height,t,height,ivalid);
	pblock2->GetValue(tube_segs,t,segs,ivalid);
	pblock2->GetValue(tube_capsegs,t,capsegs,ivalid);
	pblock2->GetValue(tube_sides,t,ssides,ivalid);
	pblock2->GetValue(tube_smooth,t,smooth,ivalid);
	pblock2->GetValue(tube_pieslice1,t,pie1,ivalid);
	pblock2->GetValue(tube_pieslice2,t,pie2,ivalid);
	pblock2->GetValue(tube_slice,t,doPie,ivalid);
	pblock2->GetValue(tube_mapping,t,genUVs,ivalid);
	LimitValue( radius, MIN_RADIUS, MAX_RADIUS );
	LimitValue( radius2, MIN_RADIUS, MAX_RADIUS );
	LimitValue( segs, MIN_SEGMENTS, MAX_SEGMENTS );
	LimitValue( capsegs, MIN_SEGMENTS, MAX_SEGMENTS );
	LimitValue( ssides, MIN_SIDES, MAX_SIDES );	
	doPie = doPie ? 1 : 0;

	// We do the torus backwards from the cylinder
	temp = -pie1;
	pie1 = -pie2;
	pie2 = temp;	

	// Flip parity when radi are swaped or height is negative
	if ((radius<radius2 || height<0) && !(radius<radius2&&height<0)) {
		temp    = radius;
		radius  = radius2;
		radius2 = temp;
		}	

	// Make pie2 < pie1 and pie1-pie2 < TWOPI
	while (pie1 < pie2) pie1 += TWOPI;
	while (pie1 > pie2+TWOPI) pie1 -= TWOPI;
	if (pie1==pie2) totalPie = TWOPI;
	else totalPie = pie1-pie2;	
	
	if (doPie) {
		segs++;	 // *** O.Z. fix for 240436
		delta    = totalPie/(float)(segs-1);
		startAng = pie2;
	} else {
		delta = (float)2.0*PI/(float)segs;
		}	

	sides = 2*(ssides+capsegs);

	if (TestAFlag(A_PLUGIN1)) startAng -= HALFPI;

	int nverts;
	int nfaces;
	if (doPie) {
		nverts = sides*segs + 2;
		nfaces = 2*sides*segs;
	} else {
		nverts = sides*segs;
		nfaces = 2*sides*segs;
		}
	mesh.setNumVerts(nverts);
	mesh.setNumFaces(nfaces);
	mesh.setSmoothFlags(smooth != 0);
	if (genUVs) {
		if (doPie) {
			mesh.setNumTVerts((sides+1)*segs+2);
			mesh.setNumTVFaces(2*sides*segs);
		} else {
			mesh.setNumTVerts((sides+1)*(segs+1));
			mesh.setNumTVFaces(2*sides*segs);
			}
	} else {
		mesh.setNumTVerts(0);
		mesh.setNumTVFaces(0);
		}

	ang = startAng;

	// make verts
	for(ix=0; ix<segs; ix++) {
		sinang = (float)sin(ang);
		cosang = (float)cos(ang);				
		
		// Radius 1
		for (jx = 0; jx<ssides; jx++) {
			p.x = radius*cosang;
			p.y = -radius*sinang;
			p.z = float(jx)/float(ssides) * height;
			mesh.setVert(nv++, p);
			}
		
		// Top		
		for (jx = 0; jx<capsegs; jx++) {
			float u = float(jx)/float(capsegs);
			p.x = (u*radius2 + (1.0f-u)*radius) * cosang;
			p.y = -(u*radius2 + (1.0f-u)*radius) * sinang;
			p.z = height;
			mesh.setVert(nv++, p);
			}
		
		// Radius 2
		for (jx = 0; jx<ssides; jx++) {
			p.x = radius2*cosang;
			p.y = -radius2*sinang;
			p.z = (1.0f-float(jx)/float(ssides)) * height;
			mesh.setVert(nv++, p);
			}
		
		// Bottom
		for (jx = 0; jx<capsegs; jx++) {
			float u = float(jx)/float(capsegs);
			p.x = (u*radius + (1.0f-u)*radius2) * cosang;
			p.y = -(u*radius + (1.0f-u)*radius2) * sinang;
			p.z = 0.0f;
			mesh.setVert(nv++, p);
			}
		
		ang += delta;
		}
	
	if (doPie) {
		float averag = (radius + radius2) / 2.0f;
		p.x = averag * (float)cos(startAng);
		p.y = -averag * (float)sin(startAng);
		p.z = height/2.0f;
		mesh.setVert(nv++, p);

		ang -= delta;
		p.x = averag * (float)cos(ang);
		p.y = -averag * (float)sin(ang);
		p.z = height/2.0f;
		mesh.setVert(nv++, p);
		}
	
	// Make faces

    BOOL usePhysUVs = GetUsePhysicalScaleUVs();
    BitArray startSliceFaces;
    BitArray endSliceFaces;

    if (usePhysUVs) {
        startSliceFaces.SetSize(mesh.numFaces);
        endSliceFaces.SetSize(mesh.numFaces);
    }

	/* Make midsection */
	for(ix=0; ix<segs-doPie; ++ix) {
		jx=ix*sides;
		for (kx=0; kx<sides; ++kx) {
			na = jx+kx;
			nb = (ix==(segs-1))?kx:na+sides;
			nd = (kx==(sides-1))? jx : na+1;
			nc = nb+nd-na;
			
			DWORD grp =  0;
			int mtlid = 0;
			if  (kx<ssides) {
				mtlid = 2;
				grp = (1<<1);
				}
			else if (kx<ssides+capsegs) {
				mtlid = 0;
				grp = (1<<2);
				}
			else if (kx<2*ssides+capsegs) {
				mtlid = 3;
				grp = (1<<1);
				}
			else {
				mtlid = 1;
				grp = (1<<2);
				}

			if (!smooth) grp = 0;

			mesh.faces[nf].setEdgeVisFlags(0,1,1);
			mesh.faces[nf].setSmGroup(grp);
			mesh.faces[nf].setMatID(mtlid);
			mesh.faces[nf++].setVerts( na,nc,nb);

			mesh.faces[nf].setEdgeVisFlags(1,1,0);
			mesh.faces[nf].setSmGroup(grp);
			mesh.faces[nf].setMatID(mtlid);
			mesh.faces[nf++].setVerts(na,nd,nc);
			}
	 	}

	if (doPie) {		
		na = nv -2;
		for(ix=0; ix<sides; ++ix) {
			nb = ix;
			nc = (ix==(sides-1))?0:ix+1;
			mesh.faces[nf].setEdgeVisFlags(0,1,0);
			mesh.faces[nf].setSmGroup((1<<0));
			mesh.faces[nf].setMatID(4);
            if (usePhysUVs)
                startSliceFaces.Set(nf);
			mesh.faces[nf++].setVerts(na,nc,nb);
			}
		
		na = nv -1;
		jx = sides*(segs-1);
		for(ix=0; ix<sides; ++ix) {
			nb = jx+ix;
			nc = (ix==(sides-1))?jx:nb+1;
			mesh.faces[nf].setEdgeVisFlags(0,1,0);
			mesh.faces[nf].setSmGroup((1<<0));
			mesh.faces[nf].setMatID(5);
            if (usePhysUVs)
                endSliceFaces.Set(nf);
			mesh.faces[nf++].setVerts(na,nb,nc);
			}
		}

	// UVWs -------------------
	
	if (genUVs) {
		nv=0;
        float rd = radius > radius2 ? radius : radius2;
        float uScale = usePhysUVs ? ((float) 2.0f * PI * rd) : 1.0f;
        if (doPie) {
            float pieScale = float(totalPie/(2.0*PI));
            uScale *= float(pieScale);
        }


        if (usePhysUVs) {
            for(ix=0; ix<=segs-doPie; ix++) {
                float v = 0.0f;
                float v0 = 0.0f;;
                float vScale = height;
                float u = uScale*(1.0f - float(ix)/float(segs-doPie));
                for (jx=0; jx <= ssides; jx++) {
                    v = v0 + vScale*float(jx)/float(ssides);
                    mesh.setTVert(nv++, u, v, 0.0f);
                }
                v0 = v;
                vScale = float(fabs(radius - radius2));
                for (jx=1; jx <= capsegs; jx++) {
                    v = v0 + vScale*float(jx)/float(capsegs);
                    mesh.setTVert(nv++, u, v, 0.0f);
                }
                v0 = v;
                vScale = height;
                for (jx=1; jx <= ssides; jx++) {
                    v = v0 + vScale*float(jx)/float(ssides);
                    mesh.setTVert(nv++, u, v, 0.0f);
                }
                v0 = v;
                vScale = float(fabs(radius - radius2));
                for (jx=1; jx <= capsegs; jx++) {
                    v = v0 + vScale*float(jx)/float(capsegs);
                    mesh.setTVert(nv++, u, v, 0.0f);
                }
            }
        } else {
		for(ix=0; ix<=segs-doPie; ix++) {
			for (jx=0; jx<=sides; jx++) {
                    mesh.setTVert(nv++,
                                  float(jx)/float(sides),
                                  float(ix)/float(segs),
                                  0.0f);
                }
				}
			}

		int pie1 = 0;
		int pie2 = 0;
		if (doPie) {
			pie1 = nv;
            if (usePhysUVs)
                mesh.setTVert(nv++,0.0f,radius2*0.5f,0.0f);
            else
			mesh.setTVert(nv++,0.5f,1.0f,0.0f);
			pie2 = nv;
            if (usePhysUVs)
                mesh.setTVert(nv++,uScale*0.5f,0.0f,0.0f);
            else
                mesh.setTVert(nv++,1.0f,0.5f,0.0f);
			}				
		
		nf=0;
		for(ix=0; ix<segs-doPie; ix++) {
			na = ix*(sides+1);
			nb = (ix+1)*(sides+1);
			for (jx=0; jx<sides; jx++) {
				mesh.tvFace[nf++].setTVerts(na,nb+1,nb);
				mesh.tvFace[nf++].setTVerts(na,na+1,nb+1);
				na++;
				nb++;
				}
			}
		if (doPie) {						
            if (usePhysUVs) {
                Matrix3 tm = RotateZMatrix(startAng) * RotateXMatrix(float(-PI/2.0));
                tm.Scale(Point3(-1.0f, 1.0f, 1.0f));
                MakeMeshCapTexture(mesh, tm, startSliceFaces, usePhysUVs);
                tm = RotateZMatrix(ang) * RotateXMatrix(float(-PI/2.0));
                MakeMeshCapTexture(mesh, tm, endSliceFaces, usePhysUVs);
            } else {
			for (jx=0; jx<sides; jx++) {
				mesh.tvFace[nf++].setTVerts(pie1,jx+1,jx);				
				}			
			nb = (sides+1)*(segs-1);
			for (jx=0; jx<sides; jx++) {
				mesh.tvFace[nf++].setTVerts(pie2,nb,nb+1);
				nb++;
				}
			}
		}
    }

	mesh.InvalidateTopologyCache();
	}

#define CIRCLE_FACT	0.5517861843f
#define Tang(vv,ii) ((vv)*4+(ii))

inline Point3 operator+(const PatchVert &pv,const Point3 &p)
	{
	return p+pv.p;
	}

void BuildTubePatch(
		PatchMesh &patch, 
		float radius1, float radius2, float height, int genUVs, BOOL usePhysUVs)
	{
	if (radius1<radius2) {
		float temp = radius1;
		radius1 = radius2;
		radius2 = temp;
		}
	int nverts = 16;
	int nvecs = 128;
	int npatches = 16;
	patch.setNumVerts(nverts);	
	patch.setNumTVerts(genUVs ? 50 : 0);
	patch.setNumVecs(nvecs);
	patch.setNumPatches(npatches);
	patch.setNumTVPatches(genUVs ? npatches : 0);
	float ocircleLen = radius1*CIRCLE_FACT;
	float icircleLen = radius2*CIRCLE_FACT;
	float radLen = (radius1-radius2)/3.0f;
	float heightLen = height/3.0f;
	int i;
	DWORD a, b, c, d;

	// Base
	patch.setVert(0, radius1, 0.0f, 0.0f);
	patch.setVert(1, radius2, 0.0f, 0.0f);
	patch.setVert(2, 0.0f, radius1, 0.0f);
	patch.setVert(3, 0.0f, radius2, 0.0f);
	patch.setVert(4, -radius1, 0.0f, 0.0f);
	patch.setVert(5, -radius2, 0.0f, 0.0f);
	patch.setVert(6, 0.0f, -radius1, 0.0f);	
	patch.setVert(7, 0.0f, -radius2, 0.0f);

	// Top
	patch.setVert(8, radius1, 0.0f, height);
	patch.setVert(9, radius2, 0.0f, height);
	patch.setVert(10, 0.0f, radius1, height);
	patch.setVert(11, 0.0f, radius2, height);
	patch.setVert(12, -radius1, 0.0f, height);
	patch.setVert(13, -radius2, 0.0f, height);
	patch.setVert(14, 0.0f, -radius1, height);	
	patch.setVert(15, 0.0f, -radius2, height);

	Point3 ovecs[] = {
		Point3(0.0f, ocircleLen, 0.0f),		
		Point3(-ocircleLen, 0.0f, 0.0f),
		Point3(0.0f, -ocircleLen, 0.0f),
		Point3(ocircleLen, 0.0f, 0.0f),
		};
	Point3 rovecs[] = {
		Point3(0.0f, radLen, 0.0f),		
		Point3(-radLen, 0.0f, 0.0f),
		Point3(0.0f, -radLen, 0.0f),
		Point3(radLen, 0.0f, 0.0f),
		};

	Point3 ivecs[] = {
		Point3(0.0f, icircleLen, 0.0f),
		Point3(icircleLen, 0.0f, 0.0f),
		Point3(0.0f, -icircleLen, 0.0f),
		Point3(-icircleLen, 0.0f, 0.0f),
		};
	Point3 rivecs[] = {
		Point3(0.0f, radLen, 0.0f),
		Point3(radLen, 0.0f, 0.0f),
		Point3(0.0f, -radLen, 0.0f),
		Point3(-radLen, 0.0f, 0.0f),
		};

	// Tangents
	int ix=0;
	for (i=0; i<4; i++) {		
		patch.setVec(ix++,patch.verts[i*2] + ovecs[i]);
		patch.setVec(ix++,patch.verts[i*2] + rovecs[(i+1)%4]);
		patch.setVec(ix++,patch.verts[i*2] + ovecs[(i+2)%4]);
		patch.setVec(ix++,patch.verts[i*2] + Point3(0.0f,0.0f,heightLen));

		patch.setVec(ix++,patch.verts[i*2+1] + ivecs[(4-i)%4]);
		patch.setVec(ix++,patch.verts[i*2+1] + rivecs[((4-i)%4+1)%4]);
		patch.setVec(ix++,patch.verts[i*2+1] + ivecs[((4-i)%4+2)%4]);
		patch.setVec(ix++,patch.verts[i*2+1] + Point3(0.0f,0.0f,heightLen));
		}
	for (i=0; i<4; i++) {		
		patch.setVec(ix++,patch.verts[i*2+8] + ovecs[i]);
		patch.setVec(ix++,patch.verts[i*2+8] + rovecs[(i+1)%4]);
		patch.setVec(ix++,patch.verts[i*2+8] + ovecs[(i+2)%4]);
		patch.setVec(ix++,patch.verts[i*2+8] + Point3(0.0f,0.0f,-heightLen));
															
		patch.setVec(ix++,patch.verts[i*2+9] + ivecs[(4-i)%4]);
		patch.setVec(ix++,patch.verts[i*2+9] + rivecs[((4-i)%4+1)%4]);
		patch.setVec(ix++,patch.verts[i*2+9] + ivecs[((4-i)%4+2)%4]);
		patch.setVec(ix++,patch.verts[i*2+9] + Point3(0.0f,0.0f,-heightLen));
		}	
	
	// Patches
	int px = 0;
	for (i=0; i<4; i++) {
		a = i*2+8;
		b = i*2;
		c = (i*2+2)%8;
		d = (i*2+2)%8+8;
		patch.patches[px].SetType(PATCH_QUAD);
		patch.patches[px].setVerts(a, b, c, d);
		patch.patches[px].setVecs(
			Tang(a,3),Tang(b,3),Tang(b,0),Tang(c,2),
			Tang(c,3),Tang(d,3),Tang(d,2),Tang(a,0));
		patch.patches[px].setInteriors(ix, ix+1, ix+2, ix+3);
		patch.patches[px].smGroup = 1;
//watje 3-17-99 to support patch matids
		patch.patches[px].setMatID(2);

		ix+=4;
		px++;
		}
	for (i=0; i<4; i++) {
		a = (i*2+1+2)%8+8;
		b = (i*2+1+2)%8;
		c = i*2+1;
		d = i*2+1+8;				
		patch.patches[px].SetType(PATCH_QUAD);
		patch.patches[px].setVerts(a, b, c, d);
		patch.patches[px].setVecs(
			Tang(a,3),Tang(b,3),Tang(b,2),Tang(c,0),
			Tang(c,3),Tang(d,3),Tang(d,0),Tang(a,2));
		patch.patches[px].setInteriors(ix, ix+1, ix+2, ix+3);
		patch.patches[px].smGroup = 1;
//watje 3-17-99 to support patch matids
		patch.patches[px].setMatID(3);
		ix+=4;
		px++;
		}
	
	for (i=0; i<4; i++) {
		a = i*2+1;
		b = (i*2+3)%8;
		c = (i*2+2)%8;
		d = (i*2);
		patch.patches[px].SetType(PATCH_QUAD);
		patch.patches[px].setVerts(a, b, c, d);
		patch.patches[px].setVecs(
			Tang(a,0),Tang(b,2),Tang(b,1),Tang(c,1),
			Tang(c,2),Tang(d,0),Tang(d,1),Tang(a,1));
		patch.patches[px].setInteriors(ix, ix+1, ix+2, ix+3);
		patch.patches[px].smGroup = 2;
//watje 3-17-99 to support patch matids
		patch.patches[px].setMatID(1);

		ix+=4;
		px++;
		}
	for (i=0; i<4; i++) {
		a = (i*2+3)%8 + 8;
		b = i*2+1 + 8;
		c = (i*2) + 8;
		d = (i*2+2)%8 + 8;		
		patch.patches[px].SetType(PATCH_QUAD);
		patch.patches[px].setVerts(a, b, c, d);
		patch.patches[px].setVecs(
			Tang(a,2),Tang(b,0),Tang(b,1),Tang(c,1),
			Tang(c,0),Tang(d,2),Tang(d,1),Tang(a,1));
		patch.patches[px].setInteriors(ix, ix+1, ix+2, ix+3);
		patch.patches[px].smGroup = 2;
//watje 3-17-99 to support patch matids
		patch.patches[px].setMatID(0);

		ix+=4;
		px++;
		}
	
	if(genUVs) {
        float rd = radius1 > radius2 ? radius1 : radius2;
        float uScale = usePhysUVs ? ((float) 2.0f * PI * rd) : 1.0f;
        float vScale = usePhysUVs ? height*4.0f : 1.0f;

		patch.setTVert(0, UVVert(uScale*0.0f, vScale*0.5f, 0.0f));
		patch.setTVert(1, UVVert(uScale*0.0f, vScale*0.25f, 0.0f));
		patch.setTVert(2, UVVert(uScale*0.25f, vScale*0.5f, 0.0f));
		patch.setTVert(3, UVVert(uScale*0.25f, vScale*0.25f, 0.0f));
		patch.setTVert(4, UVVert(uScale*0.5f, vScale*0.5f, 0.0f));
		patch.setTVert(5, UVVert(uScale*0.5f, vScale*0.25f, 0.0f));
		patch.setTVert(6, UVVert(uScale*0.75f, vScale*0.5f, 0.0f));
		patch.setTVert(7, UVVert(uScale*0.75f, vScale*0.25f, 0.0f));
		patch.setTVert(8, UVVert(uScale*1.0f, vScale*0.5f, 0.0f));
		patch.setTVert(9, UVVert(uScale*1.0f, vScale*0.25f, 0.0f));

		patch.setTVert(10, UVVert(uScale*0.0f, vScale*0.75f, 0.0f));
		patch.setTVert(11, UVVert(uScale*0.0f, vScale*1.0f, 0.0f));
		patch.setTVert(12, UVVert(uScale*0.25f, vScale*0.75f, 0.0f));
		patch.setTVert(13, UVVert(uScale*0.25f, vScale*1.0f, 0.0f));
		patch.setTVert(14, UVVert(uScale*0.5f, vScale*0.75f, 0.0f));
		patch.setTVert(15, UVVert(uScale*0.5f, vScale*1.0f, 0.0f));
		patch.setTVert(16, UVVert(uScale*0.75f, vScale*0.75f, 0.0f));
		patch.setTVert(17, UVVert(uScale*0.75f, vScale*1.0f, 0.0f));
		patch.setTVert(18, UVVert(uScale*1.0f, vScale*0.75f, 0.0f));
		patch.setTVert(19, UVVert(uScale*1.0f, vScale*1.0f, 0.0f));

		patch.setTVert(20, UVVert(uScale*0.0f, vScale*0.0f, 0.0f));
		patch.setTVert(21, UVVert(uScale*0.25f, vScale*0.0f, 0.0f));
		patch.setTVert(22, UVVert(uScale*0.5f, vScale*0.0f, 0.0f));
		patch.setTVert(23, UVVert(uScale*0.75f, vScale*0.0f, 0.0f));
		patch.setTVert(24, UVVert(uScale*1.0f, vScale*0.0f, 0.0f));

		patch.getTVPatch(0).setTVerts(10,0,2,12);
		patch.getTVPatch(1).setTVerts(12,2,4,14);
		patch.getTVPatch(2).setTVerts(14,4,6,16);
		patch.getTVPatch(3).setTVerts(16,6,8,18);
		patch.getTVPatch(4).setTVerts(21,3,1,20);
		patch.getTVPatch(5).setTVerts(22,5,3,21);
		patch.getTVPatch(6).setTVerts(23,7,5,22);
		patch.getTVPatch(7).setTVerts(24,9,7,23);

        float vScale2 = usePhysUVs ? float(fabs(radius1 - radius2))*4.0f : 1.0f;

		patch.setTVert(0+25, UVVert(uScale*0.0f, vScale2*0.5f, 0.0f));
		patch.setTVert(1+25, UVVert(uScale*0.0f, vScale2*0.25f, 0.0f));
		patch.setTVert(2+25, UVVert(uScale*0.25f, vScale2*0.5f, 0.0f));
		patch.setTVert(3+25, UVVert(uScale*0.25f, vScale2*0.25f, 0.0f));
		patch.setTVert(4+25, UVVert(uScale*0.5f, vScale2*0.5f, 0.0f));
		patch.setTVert(5+25, UVVert(uScale*0.5f, vScale2*0.25f, 0.0f));
		patch.setTVert(6+25, UVVert(uScale*0.75f, vScale2*0.5f, 0.0f));
		patch.setTVert(7+25, UVVert(uScale*0.75f, vScale2*0.25f, 0.0f));
		patch.setTVert(8+25, UVVert(uScale*1.0f, vScale2*0.5f, 0.0f));
		patch.setTVert(9+25, UVVert(uScale*1.0f, vScale2*0.25f, 0.0f));

		patch.setTVert(10+25, UVVert(uScale*0.0f, vScale2*0.75f, 0.0f));
		patch.setTVert(11+25, UVVert(uScale*0.0f, vScale2*1.0f, 0.0f));
		patch.setTVert(12+25, UVVert(uScale*0.25f, vScale2*0.75f, 0.0f));
		patch.setTVert(13+25, UVVert(uScale*0.25f, vScale2*1.0f, 0.0f));
		patch.setTVert(14+25, UVVert(uScale*0.5f, vScale2*0.75f, 0.0f));
		patch.setTVert(15+25, UVVert(uScale*0.5f, vScale2*1.0f, 0.0f));
		patch.setTVert(16+25, UVVert(uScale*0.75f, vScale2*0.75f, 0.0f));
		patch.setTVert(17+25, UVVert(uScale*0.75f, vScale2*1.0f, 0.0f));
		patch.setTVert(18+25, UVVert(uScale*1.0f, vScale2*0.75f, 0.0f));
		patch.setTVert(19+25, UVVert(uScale*1.0f, vScale2*1.0f, 0.0f));

		patch.setTVert(20+25, UVVert(uScale*0.0f, vScale2*0.0f, 0.0f));
		patch.setTVert(21+25, UVVert(uScale*0.25f, vScale2*0.0f, 0.0f));
		patch.setTVert(22+25, UVVert(uScale*0.5f, vScale2*0.0f, 0.0f));
		patch.setTVert(23+25, UVVert(uScale*0.75f, vScale2*0.0f, 0.0f));
		patch.setTVert(24+25, UVVert(uScale*1.0f, vScale2*0.0f, 0.0f));

		patch.getTVPatch(8).setTVerts(25+1,25+3,25+2,25+0);
		patch.getTVPatch(9).setTVerts(25+3,25+5,25+4,25+2);
		patch.getTVPatch(10).setTVerts(25+5,25+7,25+6,25+4);
		patch.getTVPatch(11).setTVerts(25+7,25+9,25+8,25+6);
		patch.getTVPatch(12).setTVerts(25+13,25+11,25+10,25+12);
		patch.getTVPatch(13).setTVerts(25+15,25+13,25+12,25+14);
		patch.getTVPatch(14).setTVerts(25+17,25+15,25+14,25+16);
		patch.getTVPatch(15).setTVerts(25+19,25+17,25+16,25+18);
		}
			
	if( !patch.buildLinkages() )
   {
      assert(0);
   }
	patch.computeInteriors();
	patch.InvalidateGeomCache();
	}


Object *
BuildNURBSTube(float radius1, float radius2, float height,
				BOOL sliceon, float pie1, float pie2, BOOL genUVs)
{
	BOOL flip = FALSE;

	if (radius1 < radius2)
		flip = !flip;

	if (height < 0.0f)
		flip = !flip;

	NURBSSet nset;

	Point3 origin(0,0,0);
	Point3 symAxis(0,0,1);
	Point3 refAxis(0,1,0);
	float startAngle = 0.0f;
	float endAngle = TWOPI;
	pie1 += HALFPI;
	pie2 += HALFPI;
	if (sliceon && pie1 != pie2) {
		float sweep = TWOPI - (pie2-pie1);
		if (sweep > TWOPI) sweep -= TWOPI;
		refAxis = Point3(Point3(1,0,0) * RotateZMatrix(pie2));
		endAngle = sweep;
	}


	// first the main surfaces
	NURBSCVSurface *s0 = new NURBSCVSurface();
	nset.AppendObject(s0);
	NURBSCVSurface *s1 = new NURBSCVSurface();
	nset.AppendObject(s1);

	s0->SetGenerateUVs(genUVs);
	s1->SetGenerateUVs(genUVs);

	s0->SetTextureUVs(0, 0, Point2(0.0f, 0.0f));
	s0->SetTextureUVs(0, 1, Point2(0.0f, 1.0f));
	s0->SetTextureUVs(0, 2, Point2(1.0f, 0.0f));
	s0->SetTextureUVs(0, 3, Point2(1.0f, 1.0f));

	s1->SetTextureUVs(0, 0, Point2(0.0f, 0.0f));
	s1->SetTextureUVs(0, 1, Point2(0.0f, 1.0f));
	s1->SetTextureUVs(0, 2, Point2(1.0f, 0.0f));
	s1->SetTextureUVs(0, 3, Point2(1.0f, 1.0f));

	s0->FlipNormals(!flip);
	s1->FlipNormals(flip);

	s0->Renderable(TRUE);
	s1->Renderable(TRUE);

	TCHAR bname[80];
	TCHAR sname[80];
	_tcscpy(bname, GetString(IDS_RB_TUBE));
	_stprintf(sname, _T("%s%s01"), bname, GetString(IDS_CT_SURF));
	s0->SetName(sname);

	_stprintf(sname, _T("%s%s02"), bname, GetString(IDS_CT_SURF));
	s1->SetName(sname);

	if (sliceon && pie1 != pie2) {
		// since GenNURBSCylinderSurface() returns a surface with more CVs
		// if it's larger we need to generate a single surface and make the
		// other one based on it but scaling the CVs based on the radius
		// ratio.
		float radius = (radius1 > radius2) ? radius1 : radius2;

		GenNURBSCylinderSurface(radius, height, origin, symAxis, refAxis,
						startAngle, endAngle, TRUE, *s0);
		GenNURBSCylinderSurface(radius, height, origin, symAxis, refAxis,
						startAngle, endAngle, TRUE, *s1);

		if (radius1 > radius2) {
			s0->MatID(3);
			s1->MatID(4);
		} else {
			s0->MatID(4);
			s1->MatID(3);
		}

		if (radius1 > radius2) {
			double scale = radius2/radius1;
			Matrix3 mat = ScaleMatrix(Point3(scale, scale, 1.0));
			int numU, numV;
			s1->GetNumCVs(numU, numV);
			for (int u = 0; u < numU; u++) {
				for (int v = 0; v < numV; v++) {
					Point3 pos = s1->GetCV(u, v)->GetPosition(0);
					Point3 npos = pos * mat;
					s1->GetCV(u, v)->SetPosition(0, npos);
				}
			}
		} else {
			double scale = radius1/radius2;
			Matrix3 mat = ScaleMatrix(Point3(scale, scale, 1.0));
			int numU, numV;
			s0->GetNumCVs(numU, numV);
			for (int u = 0; u < numU; u++) {
				for (int v = 0; v < numV; v++) {
					Point3 pos = s0->GetCV(u, v)->GetPosition(0);
					Point3 npos = pos * mat;
					s0->GetCV(u, v)->SetPosition(0, npos);
				}
			}
		}


#define F(s1, s2, s1r, s1c, s2r, s2c) \
		fuse.mSurf1 = (s1); \
		fuse.mSurf2 = (s2); \
		fuse.mRow1 = (s1r); \
		fuse.mCol1 = (s1c); \
		fuse.mRow2 = (s2r); \
		fuse.mCol2 = (s2c); \
		nset.mSurfFuse.Append(1, &fuse);

		NURBSFuseSurfaceCV fuse;

		// next the two caps
		for (int c = 0; c < 2; c++) {
			Point3 cen;
			if (c == 0)
				cen = Point3(0,0,0);
			else
				cen = Point3(0.0f, 0.0f, height);
			NURBSCVSurface *s = new NURBSCVSurface();
			nset.AppendObject(s);
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

			int numU, numV;
			s0->GetNumCVs(numU, numV);
			s->SetNumCVs(4, numV);

			for (int v = 0; v < numV; v++) {
				Point3 in_edge, out_edge;
				if (c == 0) {
					in_edge = s0->GetCV(0, v)->GetPosition(0);
					out_edge = s1->GetCV(0, v)->GetPosition(0);
				} else {
					in_edge = s0->GetCV(s0->GetNumUCVs()-1, v)->GetPosition(0);
					out_edge = s1->GetCV(s1->GetNumUCVs()-1, v)->GetPosition(0);
				}
				NURBSControlVertex ncv;
				ncv.SetWeight(0, s0->GetCV(0, v)->GetWeight(0));
				for (int u = 0; u < 4; u++) {
					ncv.SetPosition(0, in_edge + ((out_edge - in_edge)*((float)u/3.0f)));
					s->SetCV(u, v, ncv);
				}
			}
			s->SetGenerateUVs(genUVs);

			s->SetTextureUVs(0, 0, Point2(0.0f, 1.0f));
			s->SetTextureUVs(0, 1, Point2(1.0f, 1.0f));
			s->SetTextureUVs(0, 2, Point2(0.0f, 0.0f));
			s->SetTextureUVs(0, 3, Point2(1.0f, 0.0f));

			if (c == 0) {
				s->FlipNormals(flip);
				s->MatID(2);
			} else {
				s->FlipNormals(!flip);
				s->MatID(1);
			}
			s->Renderable(TRUE);
			_stprintf(sname, _T("%s%s%02d"), bname, GetString(IDS_CT_CAP), c+1);
			s->SetName(sname);
		}

		NURBSCVSurface *s2 = (NURBSCVSurface *)nset.GetNURBSObject(2);
		NURBSCVSurface *s3 = (NURBSCVSurface *)nset.GetNURBSObject(3);

		// next the two pie slices
		for (int c = 0; c < 2; c++) {
			NURBSCVSurface *s = new NURBSCVSurface();
			nset.AppendObject(s);
			// we'll match the tube in one dimension and the caps in the other.
			s->SetUOrder(s0->GetUOrder());
			int numKnots = s0->GetNumUKnots();
			s->SetNumUKnots(numKnots);
			for (int i = 0; i < numKnots; i++)
				s->SetUKnot(i, s0->GetUKnot(i));

			s->SetVOrder(s2->GetUOrder());
			numKnots = s2->GetNumUKnots();
			s->SetNumVKnots(numKnots);
			for (int i = 0; i < numKnots; i++)
				s->SetVKnot(i, s2->GetUKnot(i));

			int s0u, s0v, s1u, s1v, s2u, s2v, s3u, s3v;
			s0->GetNumCVs(s0u, s0v);
			s1->GetNumCVs(s1u, s1v);
			s2->GetNumCVs(s2u, s2v);
			s3->GetNumCVs(s3u, s3v);
			int uNum = s0u, vNum = s2u;
			s->SetNumCVs(uNum, vNum);

			for (int v = 0; v < vNum; v++) {
				for (int u = 0; u < uNum; u++) {
					// we get get the ends from the caps and the edge from the main sheet
					if (u == 0) {  // bottom
						if (c == 0) {
							s->SetCV(u, v, *s2->GetCV(v, 0));
						} else {
							s->SetCV(u, v, *s2->GetCV(v, s2v-1));
						}
					} else if (u == uNum-1) { // top
						if (c == 0) {
							s->SetCV(u, v, *s3->GetCV(v, 0));
						} else {
							s->SetCV(u, v, *s3->GetCV(v, s3v-1));
						}
					} else { // middle
						// get x and y from a cap and z from the main sheet.
						Point3 p;
						if (c == 0)
							p = Point3(s2->GetCV(v, 0)->GetPosition(0).x,
										s2->GetCV(v, 0)->GetPosition(0).y,
										s0->GetCV(u, s0v-1)->GetPosition(0).z);
						else
							p = Point3(s2->GetCV(v, s2v-1)->GetPosition(0).x,
										s2->GetCV(v, s2v-1)->GetPosition(0).y,
										s0->GetCV(u, s0v-1)->GetPosition(0).z);
						NURBSControlVertex ncv;
						ncv.SetPosition(0, p);
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

			if (c == 0) {
				s->FlipNormals(flip);
				s->MatID(6);
			} else {
				s->FlipNormals(!flip);
				s->MatID(5);
			}
			s->Renderable(TRUE);
			_stprintf(sname, _T("%s%s%02d"), bname, GetString(IDS_CT_SLICE), c+1);
			s->SetName(sname);
		}

		NURBSCVSurface *s4 = (NURBSCVSurface *)nset.GetNURBSObject(4);
		NURBSCVSurface *s5 = (NURBSCVSurface *)nset.GetNURBSObject(5);

		// Fuse the edges
		for (int v = 0; v < s0->GetNumVCVs(); v++) {
			F(0, 2, 0, v, 0, v);
			F(0, 3, s0->GetNumUCVs()-1, v, 0, v);
			F(1, 2, 0, v, s2->GetNumUCVs()-1, v);
			F(1, 3, s1->GetNumUCVs()-1, v, s3->GetNumUCVs()-1, v);
		}
		// Now the caps
		for (int u = 0; u < s4->GetNumUCVs(); u++) {
			F(4, 0, u, 0, u, 0);
			F(4, 1, u, s4->GetNumVCVs()-1, u, 0);
			F(5, 0, u, 0, u, s0->GetNumVCVs()-1);
			F(5, 1, u, s4->GetNumVCVs()-1, u, s0->GetNumVCVs()-1);
		}
		for (int v = 1; v < s4->GetNumVCVs()-1; v++) {
			F(4, 2, 0, v, v, 0);
			F(4, 3, s4->GetNumUCVs()-1, v, v, 0);
			F(5, 2, 0, v, v, s2->GetNumVCVs()-1);
			F(5, 3, s5->GetNumUCVs()-1, v, v, s3->GetNumVCVs()-1);
		}
	} else {
		GenNURBSCylinderSurface(radius1, height, origin, symAxis, refAxis,
						startAngle, endAngle, FALSE, *s0);
		GenNURBSCylinderSurface(radius2, height, origin, symAxis, refAxis,
						startAngle, endAngle, FALSE, *s1);

		if (radius1 > radius2) {
			s0->MatID(3);
			s1->MatID(4);
		} else {
			s0->MatID(4);
			s1->MatID(3);
		}

		TCHAR sname[80];
		// now 4 iso curves 
		NURBSIsoCurve *iso_0_0 = new NURBSIsoCurve();
		nset.AppendObject(iso_0_0);
		iso_0_0->SetParent(0);
		iso_0_0->SetDirection(FALSE);
		iso_0_0->SetTrim(FALSE);
		iso_0_0->SetParam(0, 0.0);
		_stprintf(sname, _T("%s%s%02d"), bname, GetString(IDS_CT_CAP), 0);
		iso_0_0->SetName(sname);

		NURBSIsoCurve *iso_0_1 = new NURBSIsoCurve();
		nset.AppendObject(iso_0_1);
		iso_0_1->SetParent(0);
		iso_0_1->SetDirection(FALSE);
		iso_0_1->SetTrim(FALSE);
		iso_0_1->SetParam(0, 1.0);
		_stprintf(sname, _T("%s%s%02d"), bname, GetString(IDS_CT_CAP), 1);
		iso_0_1->SetName(sname);

		NURBSIsoCurve *iso_1_0 = new NURBSIsoCurve();
		nset.AppendObject(iso_1_0);
		iso_1_0->SetParent(1);
		iso_1_0->SetDirection(FALSE);
		iso_1_0->SetTrim(FALSE);
		iso_1_0->SetParam(0, 0.0);
		_stprintf(sname, _T("%s%s%02d"), bname, GetString(IDS_CT_CAP), 2);
		iso_1_0->SetName(sname);

		NURBSIsoCurve *iso_1_1 = new NURBSIsoCurve();
		nset.AppendObject(iso_1_1);
		iso_1_1->SetParent(1);
		iso_1_1->SetDirection(FALSE);
		iso_1_1->SetTrim(FALSE);
		iso_1_1->SetParam(0, 1.0);
		_stprintf(sname, _T("%s%s%02d"), bname, GetString(IDS_CT_CAP), 3);
		iso_1_1->SetName(sname);

		// now 2 ruled surfaces
		NURBSRuledSurface *cap0 = new NURBSRuledSurface();
		nset.AppendObject(cap0);
		cap0->SetGenerateUVs(genUVs);
		cap0->SetParent(0, 2);
		cap0->SetParent(1, 4);
		cap0->FlipNormals(!flip);
		cap0->Renderable(TRUE);
		cap0->MatID(2);
		_stprintf(sname, _T("%s%s%02d"), bname, GetString(IDS_CT_CAP), 0);
		cap0->SetName(sname);

		NURBSRuledSurface *cap1 = new NURBSRuledSurface();
		nset.AppendObject(cap1);
		cap1->SetGenerateUVs(genUVs);
		cap1->SetParent(0, 3);
		cap1->SetParent(1, 5);
		cap1->FlipNormals(flip);
		cap1->Renderable(TRUE);
		cap1->MatID(1);
		_stprintf(sname, _T("%s%s%02d"), bname, GetString(IDS_CT_CAP), 1);
		cap1->SetName(sname);
	}

	Matrix3 mat;
	mat.IdentityMatrix();
	Object *ob = CreateNURBSObject(NULL, &nset, mat);
	return ob;
}

extern Object *BuildNURBSCylinder(float radius, float height,
				BOOL sliceon, float pie1, float pie2, BOOL genUVs);

Object* TubeObject::ConvertToType(TimeValue t, Class_ID obtype)
	{
	if (obtype == patchObjectClassID) {
		Interval valid = FOREVER;
		float radius1, radius2, height;
		int genUVs;
		pblock2->GetValue(tube_radius1,t,radius1,valid);
		pblock2->GetValue(tube_radius2,t,radius2,valid);
		pblock2->GetValue(tube_height,t,height,valid);
		pblock2->GetValue(tube_mapping,t,genUVs,valid);
		PatchObject *ob = new PatchObject();
		BuildTubePatch(ob->patch,radius1,radius2,height,genUVs, GetUsePhysicalScaleUVs());
		ob->SetChannelValidity(TOPO_CHAN_NUM,valid);
		ob->SetChannelValidity(GEOM_CHAN_NUM,valid);
		ob->UnlockObject();
		return ob;
	}
    if (obtype == EDITABLE_SURF_CLASS_ID) {
		Interval valid = FOREVER;
		float radius1, radius2, height, pie1, pie2;
		int sliceon, genUVs;
		pblock2->GetValue(tube_radius1,t,radius1,valid);
		pblock2->GetValue(tube_radius2,t,radius2,valid);
		pblock2->GetValue(tube_height,t,height,valid);
		pblock2->GetValue(tube_pieslice1,t,pie1,valid);
		pblock2->GetValue(tube_pieslice2,t,pie2,valid);
		pblock2->GetValue(tube_slice,t,sliceon,valid);
		pblock2->GetValue(tube_mapping,t,genUVs,valid);
		if (radius1 < 0.0f) radius1 = 0.0f;
		if (radius2 < 0.0f) radius2 = 0.0f;
		Object *ob;
		if (radius1 == 0.0f || radius2 == 0.0f) {
			float radius;
			if (radius1 == 0.0f) radius = radius2;
			else radius = radius1;
			ob = BuildNURBSCylinder(radius, height,
				sliceon, pie1 - HALFPI, pie2 - HALFPI, genUVs);
		} else
			ob = BuildNURBSTube(radius1, radius2, height, sliceon, pie1, pie2, genUVs);
		ob->SetChannelValidity(TOPO_CHAN_NUM,valid);
		ob->SetChannelValidity(GEOM_CHAN_NUM,valid);
		ob->UnlockObject();
		return ob;
	}

	return __super::ConvertToType(t,obtype);

	}

int TubeObject::CanConvertToType(Class_ID obtype)
	{
    if(obtype == patchObjectClassID) return 1;
    if(obtype == EDITABLE_SURF_CLASS_ID) return 1;
	if(obtype==defObjectClassID || obtype==triObjectClassID) return 1;
    return __super::CanConvertToType(obtype);
	}
	

void TubeObject::GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist)
{
    __super::GetCollapseTypes(clist, nlist);
    Class_ID id = EDITABLE_SURF_CLASS_ID;
    TSTR *name = new TSTR(GetString(IDS_SM_NURBS_SURFACE));
    clist.Append(1,&id);
    nlist.Append(1,&name);
}


class TubeObjCreateCallBack: public CreateMouseCallBack {
	TubeObject *ob;	
	Point3 p0, p1, p2;
	IPoint2 sp0,sp1,sp2;	
	public:
		int proc( ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat );
		void SetObj(TubeObject *obj) { ob = obj; }
	};



int TubeObjCreateCallBack::proc(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat ) {
	
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}


	float r;
	Point3 center;

	if (msg == MOUSE_FREEMOVE)
	{
			vpt->SnapPreview(m,m,NULL, SNAP_IN_3D);
	}

	if (msg==MOUSE_POINT||msg==MOUSE_MOVE) {
		bool createByRadius = tube_crtype_blk.GetInt(tube_create_meth) == 1;
		switch(point) {
			case 0:  // only happens with MOUSE_POINT msg
				ob->pblock2->SetValue(tube_radius1,0,0.0f);
				ob->pblock2->SetValue(tube_radius2,0,0.0f);
				ob->pblock2->SetValue(tube_height,0,0.1f);
				ob->suspendSnap = TRUE;				
				sp0 = m;
					p0 = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);
				mat.SetTrans(p0);
				break;
			case 1:
				mat.IdentityMatrix();
				sp1 = m;							   
					p1 = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);
				if (createByRadius) {
					// radius	
					r = Length(p1-p0);
					mat.SetTrans(p0);
				} else {
					// diameter
					Point3 center = (p0+p1)/float(2);
					r = Length(center-p0);
					mat.SetTrans(center);  // Modify Node's transform
					}

				if (msg==MOUSE_POINT) {
					ob->suspendSnap = FALSE;
					if (Length(m-sp0)<3 )
						return CREATE_ABORT;
					}
				
				ob->pblock2->SetValue(tube_radius1,0,r);
				ob->pblock2->SetValue(tube_radius2,0,r+0.1f);

				if (flags&MOUSE_CTRL) {
					float ang = (float)atan2(p1.y-p0.y,p1.x-p0.x);					
					mat.PreRotateZ(ob->ip->SnapAngle(ang));
					}				
				break;
			
			case 2:									
				mat.IdentityMatrix();
				//mat.PreRotateZ(HALFPI);
				sp2 = m;							   
					p2 = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);

				if (createByRadius) {
					// radius	
					r = Length(p2-p0);
					mat.SetTrans(p0);
				} else {
					// diameter
					Point3 center = (p2+p0)/float(2);
					r = Length(center-p0);
					mat.SetTrans(center);  // Modify Node's transform
					}
				
				ob->pblock2->SetValue(tube_radius2,0,r);
				
				if (flags&MOUSE_CTRL) {
					float ang = (float)atan2(p2.y-p0.y,p2.x-p0.x);					
					mat.PreRotateZ(ob->ip->SnapAngle(ang));
					}
				
				break;					   
			
			case 3:
				{
				float h = vpt->SnapLength(vpt->GetCPDisp(p2,Point3(0,0,1),sp2,m));
				ob->pblock2->SetValue(tube_height,0,h);

				if (msg==MOUSE_POINT) {
					ob->suspendSnap = FALSE;
					return CREATE_STOP;
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


static TubeObjCreateCallBack tubeCreateCB;

CreateMouseCallBack* TubeObject::GetCreateMouseCallBack() {
	tubeCreateCB.SetObj(this);
	return(&tubeCreateCB);
	}

BOOL TubeObject::OKtoDisplay(TimeValue t) 
	{
	return TRUE;
	}


void TubeObject::InvalidateUI() 
	{
		tube_param_blk.InvalidateUI(pblock2->LastNotifyParamID());
	}

RefTargetHandle TubeObject::Clone(RemapDir& remap) 
	{
	TubeObject* newob = new TubeObject(FALSE);	
	newob->ReplaceReference(0,remap.CloneRef(pblock2));	
	newob->ivalid.SetEmpty();	
	BaseClone(this, newob, remap);
    newob->SetUsePhysicalScaleUVs(GetUsePhysicalScaleUVs());
	return(newob);
	}

void TubeObject::UpdateUI()
{
	if (ip == NULL)
		return;
	TubeParamDlgProc* dlg = static_cast<TubeParamDlgProc*>(tube_param_blk.GetUserDlgProc());
	dlg->UpdateUI();
}

BOOL TubeObject::GetUsePhysicalScaleUVs()
{
    return ::GetUsePhysicalScaleUVs(this);
}


void TubeObject::SetUsePhysicalScaleUVs(BOOL flag)
{
    BOOL curState = GetUsePhysicalScaleUVs();
    if (curState == flag)
        return;
    if (theHold.Holding())
        theHold.Put(new RealWorldScaleRecord<TubeObject>(this, curState));
    ::SetUsePhysicalScaleUVs(this, flag);
    if (pblock2 != NULL)
		pblock2->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	UpdateUI();
	macroRec->SetProperty(this, _T("realWorldMapSize"), mr_bool, flag);
}

