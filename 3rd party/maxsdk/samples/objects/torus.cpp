/**********************************************************************
 *<
	FILE: torus.cpp

	DESCRIPTION:  Defines a Test Object Class

	CREATED BY: Dan Silva
	MODIFIED BY: Rolf Berteig

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

class TorusObject : public SimpleObject2, public RealWorldMapSizeInterface {
	friend class TorusTypeInDlgProc;
	friend class TorusObjCreateCallBack;
	friend class TorusParamDlgProc;
	public:
		// Class vars
		static IObjParam *ip;
		static bool typeinCreate;
	
		TorusObject(BOOL loading);
		
		// From Object
		int CanConvertToType(Class_ID obtype) override;
		Object* ConvertToType(TimeValue t, Class_ID obtype) override;
		void GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist) override;
		BOOL HasUVW() override;
		void SetGenUVW(BOOL sw) override;
		BOOL IsParamSurface() override {return TRUE;}
		Point3 GetSurfacePoint(TimeValue t, float u, float v,Interval &iv) override;

		// From BaseObject
		CreateMouseCallBack* GetCreateMouseCallBack() override;
		void BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev) override;
		void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next) override;
		const TCHAR *GetObjectName() override { return GetString(IDS_RB_TORUS); }

		// Animatable methods		
		void DeleteThis() override { delete this; }
		Class_ID ClassID() override { return Class_ID( TORUS_CLASS_ID,0); }
		
		// From ref
		RefTargetHandle Clone(RemapDir& remap) override;
		bool SpecifySaveReferences(ReferenceSaveManager& referenceSaveManager) override;
		IOResult Load(ILoad *iload) override;
		IOResult Save(ISave *isave) override;

		// From SimpleObject2
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

#define MIN_SEGMENTS	3
#define MAX_SEGMENTS	200

#define MIN_SIDES		3
#define MAX_SIDES		200

#define MIN_RADIUS		float(0)
#define MAX_RADIUS		float( 1.0E30)
#define MIN_PIESLICE	float(-1.0E30)
#define MAX_PIESLICE	float( 1.0E30)

#define DEF_SEGMENTS 	24
#define DEF_SIDES		12

#define DEF_RADIUS		(0.1f)
#define DEF_RADIUS2   	(10.0f)

#define SMOOTH_STRIPES	3
#define SMOOTH_ON		2
#define SMOOTH_SIDES	1
#define SMOOTH_OFF		0


//--- ClassDescriptor and class vars ---------------------------------

static BOOL sInterfaceAdded = FALSE;

class TorusClassDesc:public ClassDesc2 {
	public:
	int 			IsPublic() override { return 1; }
	void *			Create(BOOL loading = FALSE) override
    {
		if (!sInterfaceAdded) {
			AddInterface(&gRealWorldMapSizeDesc);
			sInterfaceAdded = TRUE;
		}
        return new TorusObject(loading);
    }
	const TCHAR *	ClassName() override { return GetString(IDS_RB_TORUS_CLASS); }
	SClass_ID		SuperClassID() override { return GEOMOBJECT_CLASS_ID; }
	Class_ID		ClassID() override { return Class_ID(TORUS_CLASS_ID,0); }
	const TCHAR* 	Category() override { return GetString(IDS_RB_PRIMITIVES); }
	const TCHAR*	InternalName() override { return _T("Torus"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() override { return hInstance; }			// returns owning module handle
};

static TorusClassDesc torusDesc;

ClassDesc* GetTorusDesc() { return &torusDesc; }

// in prim.cpp  - The dll instance handle
extern HINSTANCE hInstance;

// class variable for sphere class.
IObjParam *TorusObject::ip         = NULL;
bool TorusObject::typeinCreate = false;

#define PBLOCK_REF_NO	 0

enum paramblockdesc2_ids { torus_creation_type, torus_type_in, torus_params, };
enum torus_creation_type_param_ids { torus_create_meth, };
enum torus_type_in_param_ids { torus_ti_pos, torus_ti_radius1, torus_ti_radius2 };
enum torus_param_param_ids {
	torus_radius1 = TORUS_RADIUS, torus_radius2 = TORUS_RADIUS2, torus_rotation = TORUS_ROTATION,
	torus_twist = TORUS_TWIST, torus_segs = TORUS_SEGMENTS, torus_sides = TORUS_SIDES,
	torus_smooth = TORUS_SMOOTH, torus_slice = TORUS_SLICEON, torus_pieslice1 = TORUS_PIESLICE1,
	torus_pieslice2 = TORUS_PIESLICE2, torus_mapping = TORUS_GENUVS,
};

namespace
{
	MaxSDK::Util::StaticAssert< (torus_params == TORUS_PARAMBLOCK_ID) > validator;
}

namespace
{
	class MapCoords_Accessor : public PBAccessor
	{
		void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
		{
			if (owner) ((TorusObject*)owner)->UpdateUI();
		}
	};
	static MapCoords_Accessor mapCoords_Accessor;
}

// class creation type block
static ParamBlockDesc2 torus_crtype_blk(torus_creation_type, _T("TorusCreationType"), 0, &torusDesc, P_CLASS_PARAMS + P_AUTO_UI,
	//rollout
	IDD_TORUSPARAM1, IDS_RB_CREATIONMETHOD, BEGIN_EDIT_CREATE, 0, NULL,
	// params
	torus_create_meth, _T("typeinCreationMethod"), TYPE_INT, 0, IDS_RB_CREATIONMETHOD,
	p_default, 1,
	p_range, 0, 1,
	p_ui, TYPE_RADIO, 2, IDC_CREATEDIAMETER, IDC_CREATERADIUS,
	p_end,
	p_end
	);

// class type-in block
static ParamBlockDesc2 torus_typein_blk(torus_type_in, _T("TorusTypeIn"), 0, &torusDesc, P_CLASS_PARAMS + P_AUTO_UI,
	//rollout
	IDD_TORUSPARAM3, IDS_RB_KEYBOARDENTRY, BEGIN_EDIT_CREATE, APPENDROLL_CLOSED, NULL,
	// params
	torus_ti_pos, _T("typeInPos"), TYPE_POINT3, 0, IDS_RB_TYPEIN_POS,
	p_default, Point3(0, 0, 0),
	p_range, float(-1.0E30), float(1.0E30),
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_TI_POSX, IDC_TI_POSXSPIN, IDC_TI_POSY, IDC_TI_POSYSPIN, IDC_TI_POSZ, IDC_TI_POSZSPIN, SPIN_AUTOSCALE,
	p_end,
	torus_ti_radius1, _T("typeInRadius1"), TYPE_FLOAT, 0, IDS_RB_RADIUS1,
	p_default, 0.0,
	p_range, MIN_RADIUS, MAX_RADIUS,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_RADIUS1, IDC_RADSPINNER1, SPIN_AUTOSCALE,
	p_end,
	torus_ti_radius2, _T("typeInRadius2"), TYPE_FLOAT, 0, IDS_RB_RADIUS2,
	p_default, DEF_RADIUS2,
	p_range, MIN_RADIUS, MAX_RADIUS,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_RADIUS2, IDC_RAD2SPINNER, SPIN_AUTOSCALE,
	p_end,
	p_end
	);

static int smoothIDs[] = { IDC_SMOOTH_NONE,IDC_SMOOTH_SIDES,IDC_SMOOTH_ALL,IDC_SMOOTH_STRIPES };
// per instance torus block
static ParamBlockDesc2 torus_param_blk(torus_params, _T("TorusParameters"), 0, &torusDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_REF_NO,
	//rollout
	IDD_TORUSPARAM2, IDS_RB_PARAMETERS, 0, 0, NULL,
	// params
	torus_radius1, _T("radius1"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_RADIUS1,
	p_default, 0.0,
	p_ms_default, 25.0,
	p_range, MIN_RADIUS, MAX_RADIUS,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_RADIUS1, IDC_RADSPINNER1, SPIN_AUTOSCALE,
	p_end,
	torus_radius2, _T("radius2"), TYPE_WORLD, P_ANIMATABLE, IDS_RB_RADIUS2,
	p_default, DEF_RADIUS2,
	p_ms_default, 10.0,
	p_range, MIN_RADIUS, MAX_RADIUS,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_RADIUS2, IDC_RAD2SPINNER, SPIN_AUTOSCALE,
	p_end,
	torus_rotation, _T("tubeRotation"), TYPE_ANGLE, P_ANIMATABLE, IDS_RB_ROTATION2,
	p_default, 0.0,
	p_range, MIN_PIESLICE, MAX_PIESLICE,
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_TORUS_ROT, IDC_TORUS_ROTSPIN, 0.5f,
	p_end,
	torus_twist, _T("tubeTwist"), TYPE_ANGLE, P_ANIMATABLE, IDS_RB_TWIST,
	p_default, 0.0,
	p_range, MIN_PIESLICE, MAX_PIESLICE,
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_TORUS_TWIST, IDC_TORUS_TWISTSPIN, 0.5f,
	p_end,
	torus_segs, _T("segs"), TYPE_INT, P_ANIMATABLE, IDS_RB_SEGS,
	p_default, DEF_SEGMENTS,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_SEGMENTS, IDC_SEGSPINNER, 0.1f,
	p_end,
	torus_sides, _T("sides"), TYPE_INT, P_ANIMATABLE, IDS_RB_SIDES,
	p_default, DEF_SIDES,
	p_range, MIN_SIDES, MAX_SIDES,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_SIDES, IDC_SIDESPINNER, 0.1f,
	p_end,
	torus_smooth, _T("smooth"), TYPE_INT, P_ANIMATABLE, IDS_RB_SMOOTH,
	p_default, SMOOTH_ON,
	p_ms_default, 0,
	p_range, 0, 3,
	p_ui, TYPE_RADIO, 4, IDC_SMOOTH_NONE, IDC_SMOOTH_SIDES, IDC_SMOOTH_ALL, IDC_SMOOTH_STRIPES,
	p_vals, 0, 1, 2, 3,
	p_end,
	torus_slice, _T("slice"), TYPE_BOOL, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_SLICEON,
	p_default, FALSE,
	p_ui, TYPE_SINGLECHECKBOX, IDC_SLICEON,
	p_enable_ctrls, 2, torus_pieslice1, torus_pieslice2,
	p_end,
	torus_pieslice1, _T("sliceTo"), TYPE_ANGLE, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_SLICETO,
	p_default, 0.0,
	p_range, MIN_PIESLICE, MAX_PIESLICE,
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_PIESLICE1, IDC_PIESLICESPIN1, 0.5f,
	p_end,
	torus_pieslice2, _T("sliceFrom"), TYPE_ANGLE, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_SLICEFROM,
	p_default, 0.0,
	p_range, MIN_PIESLICE, MAX_PIESLICE,
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_PIESLICE2, IDC_PIESLICESPIN2, 0.5f,
	p_end,
	torus_mapping, _T("mapCoords"), TYPE_BOOL, 0, IDS_RB_GENTEXCOORDS,
	p_default, TRUE,
	p_ms_default, FALSE,
	p_ui, TYPE_SINGLECHECKBOX, IDC_GENTEXTURE,
	p_accessor, &mapCoords_Accessor,
	p_end,
	p_end
	);


static ParamBlockDescID descVer0[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0, torus_radius1 },
	{ TYPE_FLOAT, NULL, TRUE, 1, torus_radius2 },
	{ TYPE_INT, NULL, TRUE, 2, torus_segs },
	{ TYPE_INT, NULL, TRUE, 3, torus_sides },
	{ TYPE_INT, NULL, TRUE, 4, torus_smooth } };

static ParamBlockDescID descVer1[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0, torus_radius1 },
	{ TYPE_FLOAT, NULL, TRUE, 1, torus_radius2 },
	{ TYPE_FLOAT, NULL, TRUE, 8, torus_rotation },
	{ TYPE_INT, NULL, TRUE, 2, torus_segs },
	{ TYPE_INT, NULL, TRUE, 3, torus_sides },
	{ TYPE_INT, NULL, TRUE, 4, torus_smooth },
	{ TYPE_INT, NULL, TRUE, 5, torus_slice },
	{ TYPE_FLOAT, NULL, TRUE, 6, torus_pieslice1 },
	{ TYPE_FLOAT, NULL, TRUE, 7, torus_pieslice2 } };

static ParamBlockDescID descVer2[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0, torus_radius1 },
	{ TYPE_FLOAT, NULL, TRUE, 1, torus_radius2 },
	{ TYPE_FLOAT, NULL, TRUE, 8, torus_rotation },
	{ TYPE_FLOAT, NULL, TRUE, 9, torus_twist },
	{ TYPE_INT, NULL, TRUE, 2, torus_segs },
	{ TYPE_INT, NULL, TRUE, 3, torus_sides },
	{ TYPE_INT, NULL, TRUE, 4, torus_smooth },
	{ TYPE_INT, NULL, TRUE, 5, torus_slice },
	{ TYPE_FLOAT, NULL, TRUE, 6, torus_pieslice1 },
	{ TYPE_FLOAT, NULL, TRUE, 7, torus_pieslice2 } };

static ParamBlockDescID descVer3[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0, torus_radius1 },
	{ TYPE_FLOAT, NULL, TRUE, 1, torus_radius2 },
	{ TYPE_FLOAT, NULL, TRUE, 8, torus_rotation },
	{ TYPE_FLOAT, NULL, TRUE, 9, torus_twist },
	{ TYPE_INT, NULL, TRUE, 2, torus_segs },
	{ TYPE_INT, NULL, TRUE, 3, torus_sides },
	{ TYPE_INT, NULL, TRUE, 4, torus_smooth },
	{ TYPE_INT, NULL, TRUE, 5, torus_slice },
	{ TYPE_FLOAT, NULL, TRUE, 6, torus_pieslice1 },
	{ TYPE_FLOAT, NULL, TRUE, 7, torus_pieslice2 },
	{ TYPE_INT, NULL, FALSE, 10, torus_mapping } };

// Array of old versions
static ParamVersionDesc versions[] = {
	ParamVersionDesc(descVer0,5,0),
	ParamVersionDesc(descVer1,9,1),
	ParamVersionDesc(descVer2,10,2),
	ParamVersionDesc(descVer3,11,3)
	};
#define NUM_OLDVERSIONS	4

// ParamBlock data for SaveToPrevious support
#define PBLOCK_LENGTH	11
#define CURRENT_VERSION	3


//--- TypeInDlgProc --------------------------------

class TorusTypeInDlgProc : public ParamMap2UserDlgProc {
	public:
		TorusObject *ob;

		TorusTypeInDlgProc(TorusObject *o) {ob=o;}
		INT_PTR DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam) override;
		void DeleteThis() override {delete this;}
	};

INT_PTR TorusTypeInDlgProc::DlgProc(
		TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
	{
	switch (msg) {
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_TI_CREATE: {
					if (torus_typein_blk.GetFloat(torus_ti_radius1)==0.0) return TRUE;
					
					// We only want to set the value if the object is 
					// not in the scene.
					if (ob->TestAFlag(A_OBJ_CREATING)) {
						ob->pblock2->SetValue(torus_radius1, 0, torus_typein_blk.GetFloat(torus_ti_radius1));
						ob->pblock2->SetValue(torus_radius2, 0, torus_typein_blk.GetFloat(torus_ti_radius2));
					}
					else
						TorusObject::typeinCreate = true;

					Matrix3 tm(1);
					tm.SetTrans(torus_typein_blk.GetPoint3(torus_ti_pos));
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




class TorusParamDlgProc : public ParamMap2UserDlgProc {
	public:
		TorusObject *so;
		HWND thishWnd;

		TorusParamDlgProc(TorusObject *s) {so=s;thishWnd=NULL;}
		INT_PTR DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam) override;
        BOOL GetRWSState();
		void DeleteThis() override {delete this;}
		void UpdateUI();

};

BOOL TorusParamDlgProc::GetRWSState()
{
    BOOL check = IsDlgButtonChecked(thishWnd, IDC_REAL_WORLD_MAP_SIZE2);
    return check;
}

void TorusParamDlgProc::UpdateUI()
{ 
	if (!thishWnd) return;
	BOOL usePhysUVs = so->GetUsePhysicalScaleUVs();
	CheckDlgButton(thishWnd, IDC_REAL_WORLD_MAP_SIZE2, usePhysUVs);
	EnableWindow(GetDlgItem(thishWnd, IDC_REAL_WORLD_MAP_SIZE2), so->HasUVW());
}

INT_PTR TorusParamDlgProc::DlgProc(
		TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    thishWnd=hWnd;
	switch (msg) {
		case WM_INITDIALOG:
			UpdateUI();
			break;

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


//--- Torus methods -------------------------------


TorusObject::TorusObject(BOOL loading)
{
	SetAFlag(A_PLUGIN1);
	torusDesc.MakeAutoParamBlocks(this);

	if (!loading && !GetPhysicalScaleUVsDisabled())
		SetUsePhysicalScaleUVs(true);
}

#define NEWMAP_CHUNKID	0x0100

bool TorusObject::SpecifySaveReferences(ReferenceSaveManager& referenceSaveManager)
{
	// if saving to previous version that used pb1 instead of pb2...
	DWORD saveVersion = GetSavingVersion();
	if (saveVersion != 0 && saveVersion <= MAX_RELEASE_R19)
	{
		ProcessPB2ToPB1SaveToPrevious(this, pblock2, PBLOCK_REF_NO, descVer3, PBLOCK_LENGTH, CURRENT_VERSION);
	}
	return __super::SpecifySaveReferences(referenceSaveManager);
}

IOResult TorusObject::Load(ILoad *iload) 
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
		new ParamBlock2PLCB(versions,NUM_OLDVERSIONS,&torus_param_blk,this,0));
	return IO_OK;
}

IOResult TorusObject::Save(ISave *isave)
{
	if (TestAFlag(A_PLUGIN1)) {
		isave->BeginChunk(NEWMAP_CHUNKID);
		isave->EndChunk();
		}
 	return IO_OK;
}

void TorusObject::BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev )
{
	__super::BeginEditParams(ip,flags,prev);

	this->ip = ip;
	// If this has been freshly created by type-in, set creation values:
	if (TorusObject::typeinCreate)
	{
		pblock2->SetValue(torus_radius1, 0, torus_typein_blk.GetFloat(torus_ti_radius1));
		pblock2->SetValue(torus_radius2, 0, torus_typein_blk.GetFloat(torus_ti_radius2));
		TorusObject::typeinCreate = false;
	}

	// throw up all the appropriate auto-rollouts
	torusDesc.BeginEditParams(ip, this, flags, prev);
	// if in Create Panel, install a callback for the type in.
	if (flags & BEGIN_EDIT_CREATE)
		torus_typein_blk.SetUserDlgProc(new TorusTypeInDlgProc(this));
	// install a callback for the params.
	torus_param_blk.SetUserDlgProc(new TorusParamDlgProc(this));
}
		
void TorusObject::EndEditParams( IObjParam *ip, ULONG flags,Animatable *next )
{
	__super::EndEditParams(ip,flags,next);
	this->ip = NULL;
	torusDesc.EndEditParams(ip, this, flags, next);
}


Point3 TorusObject::GetSurfacePoint(
		TimeValue t, float u, float v,Interval &iv)
{
	float radius,radius2;
	pblock2->GetValue(torus_radius1,t,radius,iv);
	pblock2->GetValue(torus_radius2,t,radius2,iv);	
	float ang = (1.0f-u)*TWOPI, ang2 = v*TWOPI;
	float sinang  = (float)sin(ang);
	float cosang  = (float)cos(ang);		
	float sinang2 = (float)sin(ang2);
	float cosang2 = (float)cos(ang2);
	float rt = radius+radius2*cosang2;
	Point3 p;
	p.x = rt*cosang;
	p.y = -rt*sinang;
	p.z = radius2*sinang2;
	return p;
}

BOOL TorusObject::HasUVW() { 
	BOOL genUVs;
	Interval v;
	pblock2->GetValue(torus_mapping, 0, genUVs, v);
	return genUVs; 
}

void TorusObject::SetGenUVW(BOOL sw) {  
	if (sw==HasUVW()) return;
	pblock2->SetValue(torus_mapping, 0, sw);
}


void TorusObject::BuildMesh(TimeValue t)
{
	Point3 p;
	int ix,na,nb,nc,nd,jx,kx;
	int nf=0,nv=0;
	float delta,ang;
	float delta2,ang2;
	int sides,segs,smooth;
	float radius,radius2, rotation;
	float sinang,cosang, sinang2,cosang2,rt;
	float twist, pie1, pie2, totalPie, startAng = 0.0f;
	int doPie  = TRUE;	
	int genUVs = TRUE;	

	// Start the validity interval at forever and widdle it down.
	ivalid = FOREVER;	
	pblock2->GetValue(torus_radius1,t,radius,ivalid);
	pblock2->GetValue(torus_radius2,t,radius2,ivalid);
	pblock2->GetValue(torus_rotation,t,rotation,ivalid);
	pblock2->GetValue(torus_twist,t,twist,ivalid);
	pblock2->GetValue(torus_segs,t,segs,ivalid);
	pblock2->GetValue(torus_sides,t,sides,ivalid);
	pblock2->GetValue(torus_smooth,t,smooth,ivalid);
	pblock2->GetValue(torus_pieslice1,t,pie1,ivalid);
	pblock2->GetValue(torus_pieslice2,t,pie2,ivalid);
	pblock2->GetValue(torus_slice,t,doPie,ivalid);
	pblock2->GetValue(torus_mapping,t,genUVs,ivalid);
	LimitValue(radius, MIN_RADIUS, MAX_RADIUS );
	LimitValue(radius2, MIN_RADIUS, MAX_RADIUS );
	LimitValue(segs, MIN_SEGMENTS, MAX_SEGMENTS );
	LimitValue(sides, MIN_SIDES, MAX_SIDES );	

    // Convert doPie to a 0 or 1 value since it is used in arithmetic below
    // Controllers can give it non- 0 or 1 values
    doPie = doPie ? 1 : 0;

	// We do the torus backwards from the cylinder
	pie1 = -pie1;
	pie2 = -pie2;

	// Make pie2 < pie1 and pie1-pie2 < TWOPI
	while (pie1 < pie2) pie1 += TWOPI;
	while (pie1 > pie2+TWOPI) pie1 -= TWOPI;
	if (pie1==pie2) totalPie = TWOPI;
	else totalPie = pie1-pie2;	
	
	if (doPie) {
		segs++; //*** O.Z. fix for bug 240436 
		delta    = totalPie/(float)(segs-1);
		startAng = pie2;
	} else {
		delta = (float)2.0*PI/(float)segs;
		}
	
	delta2 = (float)2.0*PI/(float)sides;
	
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
	mesh.setSmoothFlags(smooth);
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
		ang2 = rotation + twist * float(ix+1)/float(segs);
		for (jx = 0; jx<sides; jx++) {
			sinang2 = (float)sin(ang2);
			cosang2 = (float)cos(ang2);
			rt = radius+radius2*cosang2;
			p.x = rt*cosang;
			p.y = -rt*sinang;
			p.z = radius2*sinang2;	
			mesh.setVert(nv++, p);
			ang2 += delta2;
			}	
		ang += delta;
		}
	
	if (doPie) {
		p.x = radius * (float)cos(startAng);
		p.y = -radius * (float)sin(startAng);
		p.z = 0.0f;
		mesh.setVert(nv++, p);

		ang -= delta;
		p.x = radius * (float)cos(ang);
		p.y = -radius * (float)sin(ang);
		p.z = 0.0f;
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

			DWORD grp = 0;
			if (smooth==SMOOTH_SIDES) {
				if (kx==sides-1 && (sides&1)) {
					grp = (1<<2);
				} else {
					grp = (kx&1) ? (1<<0) : (1<<1);
					}
			} else 
			if (smooth==SMOOTH_STRIPES) {
				if (ix==segs-1 && (segs&1)) {
					grp = (1<<2);
				} else {
					grp = (ix&1) ? (1<<0) : (1<<1);
					}
			} else 
			if (smooth > 0) {
				grp = 1;
				}

			mesh.faces[nf].setEdgeVisFlags(0,1,1);
			mesh.faces[nf].setSmGroup(grp);
			mesh.faces[nf].setMatID(0);
			mesh.faces[nf++].setVerts( na,nc,nb);

			mesh.faces[nf].setEdgeVisFlags(1,1,0);
			mesh.faces[nf].setSmGroup(grp);
			mesh.faces[nf].setMatID(0);
			mesh.faces[nf++].setVerts(na,nd,nc);
			}
	 	}

	if (doPie) {		
		na = nv -2;
		for(ix=0; ix<sides; ++ix) {
			nb = ix;
			nc = (ix==(sides-1))?0:ix+1;
			mesh.faces[nf].setEdgeVisFlags(0,1,0);
			mesh.faces[nf].setSmGroup((1<<3));
			mesh.faces[nf].setMatID(1);
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
			mesh.faces[nf].setSmGroup((1<<3));
			mesh.faces[nf].setMatID(2);
            if (usePhysUVs)
                endSliceFaces.Set(nf);
			mesh.faces[nf++].setVerts(na,nb,nc);
			}
		}

	
	// UVWs -------------------
	
	if (genUVs) {
        float uScale = usePhysUVs ? ((float) 2.0f * PI * radius) : 1.0f;
        float vScale = usePhysUVs ? ((float) 2.0f * PI * radius2) : 1.0f;
        if (doPie) {
            float pieScale = float(totalPie/(2.0*PI));
            uScale *= float(pieScale);
        }

		nv=0;
		for(ix=0; ix<=segs-doPie; ix++) {
			for (jx=0; jx<=sides; jx++) {
                if (usePhysUVs)
                    mesh.setTVert(nv++,uScale*(1.0f - float(ix)/float(segs-doPie)),vScale*float(jx)/float(sides),0.0f);
                else
				mesh.setTVert(nv++,float(jx)/float(sides),float(ix)/float(segs),0.0f);
				}
			}
		int pie1 = 0;
		int pie2 = 0;
		if (doPie) {
			pie1 = nv;
            if (usePhysUVs)
                mesh.setTVert(nv++,0.0f,vScale*0.5f,0.0f);
            else
			mesh.setTVert(nv++,0.5f,1.0f,0.0f);
			pie2 = nv;
            if (usePhysUVs)
                mesh.setTVert(nv++,uScale*0.5f,vScale*0.0f,0.0f);
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

#define Tang(vv,ii) ((vv)*4+(ii))

inline Point3 operator+(const PatchVert &pv,const Point3 &p)
{
	return p+pv.p;
}

#define CIRCLE_FACT8	0.265202f
#define CIRCLE_FACT4	0.5517861843f

void BuildTorusPatch(
		TimeValue t, PatchMesh &patch, 
		float radius1, float radius2, int genUVs, BOOL usePhysUVs)
{
	int segs = 8, sides = 4;
	int nverts = segs * sides;
	int nvecs = segs*sides*8;
	int npatches = segs * sides;
	patch.setNumVerts(nverts);
	patch.setNumTVerts(genUVs ? (segs + 1) * (sides + 1) : 0);
	patch.setNumVecs(nvecs);
	patch.setNumPatches(npatches);	
	patch.setNumTVPatches(genUVs ? npatches : 0);
	int ix=0, jx=0, kx=sides*segs*4, i, j;
	float ang1 = 0.0f, delta1 = TWOPI/float(segs);
	float ang2 = 0.0f, delta2 = TWOPI/float(sides);	
	float circleLenIn = CIRCLE_FACT8*(radius1-radius2);
	float circleLenOut = CIRCLE_FACT8*(radius1+radius2);
	float circleLenMid = CIRCLE_FACT4*radius2;
	float circleLen;
	float sinang1, cosang1, sinang2, cosang2, rt, u;
	Point3 p, v;
	DWORD a, b, c, d;

	for (i=0; i<segs; i++) {
		sinang1 = (float)sin(ang1);
		cosang1 = (float)cos(ang1);
		ang2 = 0.0f;
		for (j=0; j<sides; j++) {			
			sinang2 = (float)sin(ang2);
			cosang2 = (float)cos(ang2);
			rt = radius1+radius2*cosang2;
			
			// Vertex
			p.x = rt*cosang1;
			p.y = rt*sinang1;
			p.z = radius2*sinang2;	
			patch.setVert(ix, p);
			
			// Tangents			
			u = (cosang2+1.0f)/2.0f;
			circleLen = u*circleLenOut + (1.0f-u)*circleLenIn;

			v.x = -sinang1*circleLen;
			v.y = cosang1*circleLen;
			v.z = 0.0f;
			patch.setVec(jx++,patch.verts[ix] + v);
			
			v.x = sinang1*circleLen;
			v.y = -cosang1*circleLen;
			v.z = 0.0f;
			patch.setVec(jx++,patch.verts[ix] + v);
			
			v.x = -sinang2*cosang1*circleLenMid;
			v.y = -sinang2*sinang1*circleLenMid;
			v.z = cosang2*circleLenMid;
			patch.setVec(jx++,patch.verts[ix] + v);
			
			v.x = sinang2*cosang1*circleLenMid;
			v.y = sinang2*sinang1*circleLenMid;
			v.z = -cosang2*circleLenMid;
			patch.setVec(jx++,patch.verts[ix] + v);			

			// Build the patch
			a = ((i+1)%segs)*sides + (j+1)%sides;
			b = i*sides + (j+1)%sides;
			c = i*sides + j;			
			d = ((i+1)%segs)*sides + j;
			
			patch.patches[ix].SetType(PATCH_QUAD);
			patch.patches[ix].setVerts(a, b, c, d);
			patch.patches[ix].setVecs(
				Tang(a,1),Tang(b,0),Tang(b,3),Tang(c,2),
				Tang(c,0),Tang(d,1),Tang(d,2),Tang(a,3));
			patch.patches[ix].setInteriors(kx, kx+1, kx+2, kx+3);
			patch.patches[ix].smGroup = 1;

			kx += 4;
			ix++;
			ang2 += delta2;
			}		
		ang1 += delta1;
		}	

	if(genUVs) {
		int tv = 0;
		int tvp = 0;
		float fsegs = (float)segs;
		float fsides = (float)sides;
        float uScale = usePhysUVs ? ((float) 2.0f * PI * radius1) : 1.0f;
        float vScale = usePhysUVs ? ((float) 2.0f * PI * radius2) : 1.0f;
		for (i=0; i<=segs; i++) {
			float u = (float)i / (fsegs-1);
			for (j=0; j<=sides; j++,++tv) {
				float v = (float)j / (fsides-1);
                if (usePhysUVs)
                    patch.setTVert(tv, UVVert(vScale*v, uScale*u, 0.0f));
                else
                    patch.setTVert(tv, UVVert(uScale*(1.0f-u), vScale*v, 0.0f));
				if(j < sides && i < segs)
					patch.getTVPatch(tvp++).setTVerts(tv, tv+1, tv+sides+2, tv+sides+1);
				}		
			}	
		}
			
	if( !patch.buildLinkages() )
   {
      assert(0);
   }
	patch.computeInteriors();
	patch.InvalidateGeomCache();
}



Object *BuildNURBSTorus(float radius, float radius2, BOOL sliceon, float pie1, float pie2, BOOL genUVs)
{
	NURBSSet nset;

	Point3 origin(0,0,0);
	Point3 symAxis(0,0,1);
	Point3 refAxis(0,1,0);

	float startAngle = 0.0f;
	float endAngle = TWOPI;
	if (sliceon && pie1 != pie2) {
		float sweep = pie2-pie1;
		if (sweep <= 0.0f) sweep += TWOPI;
		refAxis = Point3(Point3(0,1,0) * RotateZMatrix(pie1));
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

	surf->FlipNormals(TRUE);
	surf->Renderable(TRUE);
	TCHAR bname[80];
	TCHAR sname[80];
	_tcscpy(bname, GetString(IDS_RB_TORUS));
	_stprintf(sname, _T("%s%s"), bname, GetString(IDS_CT_SURF));
	surf->SetName(sname);

	if (sliceon && pie1 != pie2) {
		GenNURBSTorusSurface(radius, radius2, origin, symAxis, refAxis,
						startAngle, endAngle, -PI, PI, TRUE, *surf);
		// now create caps on the ends
		NURBSCapSurface *cap0 = new NURBSCapSurface();
		nset.AppendObject(cap0);
		cap0->SetGenerateUVs(genUVs);
		cap0->SetParent(0);
		cap0->SetEdge(2);
		cap0->FlipNormals(FALSE);
		cap0->Renderable(TRUE);
		TCHAR sname[80];
		_stprintf(sname, _T("%s%s%02d"), bname, GetString(IDS_CT_CAP), 0);
		cap0->SetName(sname);

		NURBSCapSurface *cap1 = new NURBSCapSurface();
		nset.AppendObject(cap1);
		cap1->SetGenerateUVs(genUVs);
		cap1->SetParent(0);
		cap1->SetEdge(3);
		cap1->FlipNormals(TRUE);
		cap1->Renderable(TRUE);
		_stprintf(sname, _T("%s%s%02d"), bname, GetString(IDS_CT_CAP), 1);
		cap1->SetName(sname);
	} else {
		GenNURBSTorusSurface(radius, radius2, origin, symAxis, refAxis,
						startAngle, endAngle, -PI, PI, FALSE, *surf);
    }


	Matrix3 mat;
	mat.IdentityMatrix();
	Object *ob = CreateNURBSObject(NULL, &nset, mat);
	return ob;
}


Object* TorusObject::ConvertToType(TimeValue t, Class_ID obtype)
{
	if (obtype == patchObjectClassID) {
		Interval valid = FOREVER;
		float radius1, radius2;
		int genUVs;
		pblock2->GetValue(torus_radius1,t,radius1,valid);
		pblock2->GetValue(torus_radius2,t,radius2,valid);
		pblock2->GetValue(torus_mapping,t,genUVs,valid);
		PatchObject *ob = new PatchObject();
		BuildTorusPatch(t,ob->patch,radius1,radius2,genUVs, GetUsePhysicalScaleUVs());
		ob->SetChannelValidity(TOPO_CHAN_NUM,valid);
		ob->SetChannelValidity(GEOM_CHAN_NUM,valid);
		ob->UnlockObject();
		return ob;
	} 
    if (obtype == EDITABLE_SURF_CLASS_ID) {
		Interval valid = FOREVER;
		float radius, radius2, pie1, pie2;
		int sliceon, genUVs;
		pblock2->GetValue(torus_radius1,t,radius,valid);
		pblock2->GetValue(torus_radius2,t,radius2,valid);
		pblock2->GetValue(torus_pieslice1,t,pie1,valid);
		pblock2->GetValue(torus_pieslice2,t,pie2,valid);
		pblock2->GetValue(torus_slice,t,sliceon,valid);
		pblock2->GetValue(torus_mapping,t,genUVs,valid);
		Object *ob = BuildNURBSTorus(radius, radius2, sliceon, pie1, pie2, genUVs);
		ob->SetChannelValidity(TOPO_CHAN_NUM,valid);
		ob->SetChannelValidity(GEOM_CHAN_NUM,valid);
		ob->UnlockObject();
		return ob;
		
	}

	return __super::ConvertToType(t,obtype);
}

int TorusObject::CanConvertToType(Class_ID obtype)
{
	if(obtype==defObjectClassID || obtype==triObjectClassID) return 1;
    if(obtype == patchObjectClassID) return 1;
    if(obtype==EDITABLE_SURF_CLASS_ID) return 1;
    return __super::CanConvertToType(obtype);
}

void TorusObject::GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist)
{
    __super::GetCollapseTypes(clist, nlist);
    Class_ID id = EDITABLE_SURF_CLASS_ID;
    TSTR *name = new TSTR(GetString(IDS_SM_NURBS_SURFACE));
    clist.Append(1,&id);
    nlist.Append(1,&name);
}


class TorusObjCreateCallBack: public CreateMouseCallBack {
	TorusObject *ob;	
	Point3 p0, p1, p2;
	IPoint2 sp0,sp1,sp2;	
	float oldRad2;
public:
	int proc( ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat ) override;
	void SetObj(TorusObject *obj) { ob = obj; }
};



int TorusObjCreateCallBack::proc(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat ) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	
	float r, r2;
	Point3 center;

	if (msg == MOUSE_FREEMOVE)
	{
			vpt->SnapPreview(m,m,NULL, SNAP_IN_3D);
	}
	Interval ivalid = FOREVER;
	bool createByRadius;
	float radius2;
	if (msg==MOUSE_POINT||msg==MOUSE_MOVE) {
		switch(point) {
			case 0:  // only happens with MOUSE_POINT msg
				ob->suspendSnap = TRUE;				
				sp0 = m;
					p0 = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);
				mat.SetTrans(p0);
				break;
			case 1:
				mat.IdentityMatrix();
				//mat.PreRotateZ(HALFPI);
				sp1 = m;							   
					p1 = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);
				createByRadius = torus_crtype_blk.GetInt(torus_create_meth) == 1;
				radius2 = ob->pblock2->GetValue(torus_radius2, 0, radius2, ivalid);
				if (createByRadius) {
					// radius
					r = Length(p1 - p0) - radius2;
					mat.SetTrans(p0);
				} else {
					// diameter
					Point3 center = (p0+p1)/float(2);
					r = Length(center-p0) - radius2;
					mat.SetTrans(center);  // Modify Node's transform
				}

				if (msg==MOUSE_POINT) {
					ob->suspendSnap = FALSE;
					if (Length(m-sp0)<3 )
						return CREATE_ABORT;
				}
				
				ob->pblock2->SetValue(torus_radius1,0,r);
				
				if (flags&MOUSE_CTRL) {
					float ang = (float)atan2(p1.y-p0.y,p1.x-p0.x);
					mat.PreRotateZ(ob->ip->SnapAngle(ang));
				}				
				break;
			
			case 2:					
				center = mat.GetTrans();
				mat.IdentityMatrix();
				//mat.PreRotateZ(HALFPI);
				mat.SetTrans(center);

					p2 = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);

				r   = Length(p1-p0);
				r2  = Length(p2-p0);
//O.Z. *** begin fix for bug #189695
				createByRadius = torus_crtype_blk.GetInt(torus_create_meth) == 1;
				if (createByRadius) {
					ob->pblock2->SetValue(torus_radius1,0,(r2+r)/2.0f);
					ob->pblock2->SetValue(torus_radius2,0,(float)fabs(r - r2) / 2.0f);
				}
				else {
					Point3 center = (p2+p0)/float(2);
					r = Length(center-p0);
					mat.SetTrans(center);  // Modify Node's transform
					ob->pblock2->SetValue(torus_radius2,0,r);
				}
//O.Z. *** end fix for bug #189695
				
				if (flags&MOUSE_CTRL) {
					float ang = (float)atan2(p2.y-p0.y,p2.x-p0.x);					
					mat.PreRotateZ(ob->ip->SnapAngle(ang));
				}

				if (msg==MOUSE_POINT) {
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


static TorusObjCreateCallBack torusCreateCB;

CreateMouseCallBack* TorusObject::GetCreateMouseCallBack() {
	torusCreateCB.SetObj(this);
	return(&torusCreateCB);
}

BOOL TorusObject::OKtoDisplay(TimeValue t) 
{
	return TRUE;
}


void TorusObject::InvalidateUI() 
{
	torus_param_blk.InvalidateUI(pblock2->LastNotifyParamID());
}

RefTargetHandle TorusObject::Clone(RemapDir& remap) 
{
	TorusObject* newob = new TorusObject(FALSE);	
	newob->ReplaceReference(0,remap.CloneRef(pblock2));	
	newob->ivalid.SetEmpty();	
	BaseClone(this, newob, remap);
    newob->SetUsePhysicalScaleUVs(GetUsePhysicalScaleUVs());
	return(newob);
}

void TorusObject::UpdateUI()
{
	if (ip == NULL)
		return;
	TorusParamDlgProc* dlg = static_cast<TorusParamDlgProc*>(torus_param_blk.GetUserDlgProc());
	dlg->UpdateUI();
}

BOOL TorusObject::GetUsePhysicalScaleUVs()
{
    return ::GetUsePhysicalScaleUVs(this);
}


void TorusObject::SetUsePhysicalScaleUVs(BOOL flag)
{
    BOOL curState = GetUsePhysicalScaleUVs();
    if (curState == flag)
        return;
    if (theHold.Holding())
        theHold.Put(new RealWorldScaleRecord<TorusObject>(this, curState));
    ::SetUsePhysicalScaleUVs(this, flag);
    if (pblock2 != NULL)
		pblock2->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	UpdateUI();
	macroRec->SetProperty(this, _T("realWorldMapSize"), mr_bool, flag);
}

