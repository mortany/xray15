/**********************************************************************
 *<
	FILE:pyramid.cpp
	CREATED BY:  Audrey Peterson

 *>	Copyright (c) 1996, All Rights Reserved.
 **********************************************************************/

#include "solids.h"

#include "iparamm2.h"
#include "Simpobj.h"
#include "surf_api.h"
#include "macroRec.h"
#include "RealWorldMapUtils.h"
#include <ReferenceSaveManager.h>
#include <Util/StaticAssert.h>


static Class_ID PYRMID_CLASS_ID(0x76bf318a, 0x4bf37b10);
class PyramidObject : public SimpleObject2, public RealWorldMapSizeInterface {
	friend class PyramidTypeInDlgProc;
	friend class PyramidObjCreateCallBack;
	public:
		// Class vars
		static IObjParam *ip;
		static bool typeinCreate;
		
		PyramidObject(BOOL loading);		

		// From Object
		int CanConvertToType(Class_ID obtype) override;
		Object* ConvertToType(TimeValue t, Class_ID obtype) override;
		void GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist) override;
				
		// From BaseObject
		CreateMouseCallBack* GetCreateMouseCallBack() override;
		void BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev) override;
		void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next) override;
		const TCHAR *GetObjectName() override { return GetString(IDS_RB_PYRAMID); }
		BOOL HasUVW() override;
		void SetGenUVW(BOOL sw) override;
				
		// Animatable methods		
		void DeleteThis() override { delete this; }
		Class_ID ClassID() override { return PYRMID_CLASS_ID; }
				
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

#define DEF_SEGMENTS 	1
#define DEF_SIDES		1

#define DEF_RADIUS		float(0.0)
#define DEF_HEIGHT		float(0.01)
#define DEF_FILLET		float(0.01)


//--- ClassDescriptor and class vars ---------------------------------

static BOOL sInterfaceAdded = FALSE;

// in solids.cpp  - The dll instance handle
extern HINSTANCE hInstance;

// class variables for Pyramid class.
IObjParam *PyramidObject::ip = NULL;
bool PyramidObject::typeinCreate = false;

class PyramidClassDesc:public ClassDesc2 {
	public:
	int 			IsPublic() override { return 1; }
	void *			Create(BOOL loading = FALSE) override
    {
		if (!sInterfaceAdded) {
			AddInterface(&gRealWorldMapSizeDesc);
			sInterfaceAdded = TRUE;
		}
        return new PyramidObject(loading);
    }
	const TCHAR *	ClassName() override { return GetString(IDS_AP_PYRAMID_CLASS); }
	SClass_ID		SuperClassID() override { return GEOMOBJECT_CLASS_ID; }
	Class_ID		ClassID() override { return PYRAMID_CLASS_ID; }
	const TCHAR* 	Category() override { return GetString(IDS_AP_PRIMITIVES);  }
	const TCHAR*	InternalName() override { return _T("Pyramid"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() override { return hInstance; }			// returns owning module handle
};

static PyramidClassDesc PyramidDesc;

ClassDesc* GetPyramidDesc() { return &PyramidDesc; }

#define PBLOCK_REF_NO	 0

#define BMIN_HEIGHT		float(-1.0E30)
#define BMAX_HEIGHT		float(1.0E30)
#define BMIN_LENGTH		float(0.1)
#define BMAX_LENGTH		float(1.0E30)
#define BMIN_WIDTH		float(0)
#define BMAX_WIDTH		float(1.0E30)

// ParamBlockDesc2 IDs
enum paramblockdesc2_ids { pyramid_creation_type, pyramid_type_in, pyramid_params, };
enum pyramid_creation_type_param_ids { pyramid_create_meth, };
enum pyramid_type_in_param_ids { pyramid_ti_pos, pyramid_ti_width, pyramid_ti_depth, pyramid_ti_height, };
enum pyramid_param_param_ids {
	pyramid_width = PYR_WIDTH, pyramid_depth = PYR_DEPTH, pyramid_height = PYR_HEIGHT,
	pyramid_wsegs = PYR_WSEGS, pyramid_dsegs = PYR_DSEGS, pyramid_hsegs = PYR_HSEGS,
	pyramid_mapping = PYR_GENUVS,
};

namespace
{
	MaxSDK::Util::StaticAssert< (pyramid_params == PYR_PARAMBLOCK_ID) > validator;
}

namespace
{
	class MapCoords_Accessor : public PBAccessor
	{
		void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
		{
			if (owner) ((PyramidObject*)owner)->UpdateUI();
		}
	};
	static MapCoords_Accessor mapCoords_Accessor;
}

// class creation type block
static ParamBlockDesc2 pyramid_crtype_blk(pyramid_creation_type, _T("PyramidCreationType"), 0, &PyramidDesc, P_CLASS_PARAMS + P_AUTO_UI,
	//rollout
	IDD_PYRAMID1, IDS_RB_CREATE_DIALOG, BEGIN_EDIT_CREATE, 0, NULL,
	// params
	pyramid_create_meth, _T("typeinCreationMethod"), TYPE_INT, 0, IDS_RB_CREATE_DIALOG,
	p_default, 1,
	p_range, 0, 1,
	p_ui, TYPE_RADIO, 2, IDC_PYR_CREATEBASE, IDC_PYR_CREATECENTER,
	p_end,
	p_end
	);

// class type-in block
static ParamBlockDesc2 pyramid_typein_blk(pyramid_type_in, _T("PyramidTypeIn"), 0, &PyramidDesc, P_CLASS_PARAMS + P_AUTO_UI,
	//rollout
	IDD_PYRAMID2, IDS_RB_KEYBOARDENTRY, BEGIN_EDIT_CREATE, APPENDROLL_CLOSED, NULL,
	// params
	pyramid_ti_pos, _T("typeInPos"), TYPE_POINT3, 0, IDS_RB_TYPEIN_POS,
	p_default, Point3(0, 0, 0),
	p_range, float(-1.0E30), float(1.0E30),
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_PYR_POSX, IDC_PYR_POSXSPIN, IDC_PYR_POSY, IDC_PYR_POSYSPIN, IDC_PYR_POSZ, IDC_PYR_POSZSPIN, SPIN_AUTOSCALE,
	p_end,
	pyramid_ti_width, _T("typeInWidth"), TYPE_FLOAT, 0, IDS_RB_WIDTH,
	p_default, 0.0,
	p_range, BMIN_WIDTH, BMAX_WIDTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_PYR_WIDTH, IDC_PYR_WIDTHSPIN, SPIN_AUTOSCALE,
	p_end,
	pyramid_ti_depth, _T("typeInDepth"), TYPE_FLOAT, 0, IDS_RB_DEPTH,
	p_default, 0.0,
	p_range, BMIN_LENGTH, BMAX_LENGTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_PYR_DEPTH, IDC_PYR_DEPTHSPIN, SPIN_AUTOSCALE,
	p_end,
	pyramid_ti_height, _T("typeInHeight"), TYPE_FLOAT, 0, IDS_RB_HEIGHT,
	p_default, 0.0,
	p_range, BMIN_HEIGHT, BMAX_HEIGHT,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_PYR_HEIGHT, IDC_PYR_HEIGHTSPIN, SPIN_AUTOSCALE,
	p_end,
	p_end
	);

// per instance pyramid block
static ParamBlockDesc2 pyramid_param_blk(pyramid_params, _T("PyramidParameters"), 0, &PyramidDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_REF_NO,
	//rollout
	IDD_PYRAMID3, IDS_AP_PARAMETERS, 0, 0, NULL,
	// params
	pyramid_width, _T("width"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_WIDTH,
	p_default, 0.0,
	p_ms_default, 25.0,
	p_range, BMIN_WIDTH, BMAX_WIDTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_PYR_WIDTH, IDC_PYR_WIDTHSPIN, SPIN_AUTOSCALE,
	p_end,
	pyramid_depth, _T("depth"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_DEPTH,
	p_default, 0.1,
	p_ms_default, 25.0,
	p_range, BMIN_LENGTH, BMAX_LENGTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_PYR_DEPTH, IDC_PYR_DEPTHSPIN, SPIN_AUTOSCALE,
	p_end,
	pyramid_height, _T("height"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_HEIGHT,
	p_default, 0.0,
	p_ms_default, 25.0,
	p_range, BMIN_HEIGHT, BMAX_HEIGHT,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_PYR_HEIGHT, IDC_PYR_HEIGHTSPIN, SPIN_AUTOSCALE,
	p_end,
	pyramid_wsegs, _T("widthSegs"), TYPE_INT, P_ANIMATABLE, IDS_RB_WSEGS,
	p_default, DEF_SEGMENTS,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_PYR_WIDSEGS, IDC_PYR_WIDSEGSSPIN, 0.1f,
	p_end,
	pyramid_dsegs, _T("depthSegs"), TYPE_INT, P_ANIMATABLE, IDS_RB_DSEGS,
	p_default, DEF_SIDES,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_PYR_DEPSEGS, IDC_PYR_DEPSEGSSPIN, 0.1f,
	p_end,
	pyramid_hsegs, _T("heightSegs"), TYPE_INT, P_ANIMATABLE, IDS_RB_HSEGS,
	p_default, DEF_SIDES,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_PYR_HGTSEGS, IDC_PYR_HGTSEGSSPIN, 0.1f,
	p_end,
	pyramid_mapping, _T("mapCoords"), TYPE_BOOL, 0, IDS_MXS_GENUVS,
	p_default, TRUE,
	p_ms_default, FALSE,
	p_ui, TYPE_SINGLECHECKBOX, IDC_GENTEXTURE,
	p_accessor, &mapCoords_Accessor,
	p_end,
	p_end
	);

// variable type, NULL, animatable, number
ParamBlockDescID PyramiddescVer0[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0, pyramid_width },
	{ TYPE_FLOAT, NULL, TRUE, 1, pyramid_depth },
	{ TYPE_FLOAT, NULL, TRUE, 2, pyramid_height },
	{ TYPE_INT, NULL, TRUE, 3, pyramid_wsegs },
	{ TYPE_INT, NULL, TRUE, 4, pyramid_dsegs },
	{ TYPE_INT, NULL, TRUE, 5, pyramid_hsegs },
	{ TYPE_INT, NULL, FALSE, 6, pyramid_mapping },
	};

#define PBLOCK_LENGTH	7
// Array of old versions
static ParamVersionDesc versions[] = {
	ParamVersionDesc(PyramiddescVer0,7,0)
};

// ParamBlock data for SaveToPrevious support
#define NUM_OLDVERSIONS	1
#define CURRENT_VERSION	0

//--- TypeInDlgProc --------------------------------

class PyramidTypeInDlgProc : public ParamMap2UserDlgProc {
	public:
		PyramidObject *ob;

		PyramidTypeInDlgProc(PyramidObject *o) {ob=o;}
		INT_PTR DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam) override;
		void DeleteThis() override {delete this;}
	};

INT_PTR PyramidTypeInDlgProc::DlgProc(
		TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
	{
	switch (msg) {
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_PR_CREATE: {
					if (pyramid_typein_blk.GetFloat(pyramid_ti_height)==0.0f) return TRUE;
					
					// We only want to set the value if the object is 
					// not in the scene.
					if (ob->TestAFlag(A_OBJ_CREATING)) {
						ob->pblock2->SetValue(pyramid_height, 0, pyramid_typein_blk.GetFloat(pyramid_ti_height));
						ob->pblock2->SetValue(pyramid_width, 0, pyramid_typein_blk.GetFloat(pyramid_ti_width));
						ob->pblock2->SetValue(pyramid_depth, 0, pyramid_typein_blk.GetFloat(pyramid_ti_depth));
					}
					else
						PyramidObject::typeinCreate = true;

					Matrix3 tm(1);
					tm.SetTrans(pyramid_typein_blk.GetPoint3(pyramid_ti_pos));
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


class PyramidParamDlgProc : public ParamMap2UserDlgProc {
	public:
		PyramidObject *mpPyramidObj;
        HWND mhWnd;
		PyramidParamDlgProc(PyramidObject *o) {mpPyramidObj=o;}
		INT_PTR DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam) override;
		void DeleteThis() override {delete this;}
        void UpdateUI();
        BOOL GetRWSState();
	};

BOOL PyramidParamDlgProc::GetRWSState()
{
    BOOL check = IsDlgButtonChecked(mhWnd, IDC_REAL_WORLD_MAP_SIZE);
    return check;
}

void PyramidParamDlgProc::UpdateUI()
{
    if (mpPyramidObj == NULL) return;
    BOOL usePhysUVs = mpPyramidObj->GetUsePhysicalScaleUVs();
    CheckDlgButton(mhWnd, IDC_REAL_WORLD_MAP_SIZE, usePhysUVs);
    EnableWindow(GetDlgItem(mhWnd, IDC_REAL_WORLD_MAP_SIZE), mpPyramidObj->HasUVW());
}

INT_PTR PyramidParamDlgProc::DlgProc(
		TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch (msg) {
    case WM_INITDIALOG: {
        mhWnd = hWnd;
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
            mpPyramidObj->SetUsePhysicalScaleUVs(check);
            theHold.Accept(GetString(IDS_DS_PARAMCHG));
            mpPyramidObj->ip->RedrawViews(mpPyramidObj->ip->GetTime());
            break;
        }
            
        }
        break;
        
    }
	return FALSE;
}


//--- Pyramid methods -------------------------------

PyramidObject::PyramidObject(BOOL loading) 
{
	PyramidDesc.MakeAutoParamBlocks(this);
	if (!loading && !GetPhysicalScaleUVsDisabled())
		SetUsePhysicalScaleUVs(true);
}

bool PyramidObject::SpecifySaveReferences(ReferenceSaveManager& referenceSaveManager)
{
	// if saving to previous version that used pb1 instead of pb2...
	DWORD saveVersion = GetSavingVersion();
	if (saveVersion != 0 && saveVersion <= MAX_RELEASE_R19)
	{
		ProcessPB2ToPB1SaveToPrevious(this, pblock2, PBLOCK_REF_NO, PyramiddescVer0, PBLOCK_LENGTH, CURRENT_VERSION);
	}
	return __super::SpecifySaveReferences(referenceSaveManager);
}

IOResult PyramidObject::Load(ILoad *iload) 
	{
	ParamBlock2PLCB* plcb = new ParamBlock2PLCB(versions, NUM_OLDVERSIONS, &pyramid_param_blk, this, PBLOCK_REF_NO);
	iload->RegisterPostLoadCallback(plcb);
	return IO_OK;
	}


void PyramidObject::BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev)
{
	__super::BeginEditParams(ip,flags,prev);
	this->ip = ip;
	// If this has been freshly created by type-in, set creation values:
	if (PyramidObject::typeinCreate)
	{
		pblock2->SetValue(pyramid_width, 0, pyramid_typein_blk.GetFloat(pyramid_ti_width));
		pblock2->SetValue(pyramid_depth, 0, pyramid_typein_blk.GetFloat(pyramid_ti_depth));
		pblock2->SetValue(pyramid_height, 0, pyramid_typein_blk.GetFloat(pyramid_ti_height));
		PyramidObject::typeinCreate = false;
	}

	// throw up all the appropriate auto-rollouts
	PyramidDesc.BeginEditParams(ip, this, flags, prev);
	// if in Create Panel, install a callback for the type in.
	if (flags & BEGIN_EDIT_CREATE)
		pyramid_typein_blk.SetUserDlgProc(new PyramidTypeInDlgProc(this));
	// install a callback for the params.
	pyramid_param_blk.SetUserDlgProc(new PyramidParamDlgProc(this));
}
		
void PyramidObject::EndEditParams( IObjParam *ip, ULONG flags,Animatable *next )
{		
	__super::EndEditParams(ip,flags,next);
	this->ip = NULL;
	PyramidDesc.EndEditParams(ip, this, flags, next);
}

void BuildPyramidMesh(Mesh &mesh,
		int hsegs, int wsegs, int dsegs, float height, 
		float width, float depth, int genUVs, int usePhysUVs)
{	int nf=0,ulevel=wsegs+dsegs+2,totalsegs=2*(wsegs+dsegs);
	int nfaces = 0;
	int ntverts = 0;
	nfaces = 2*totalsegs*hsegs;
	int nverts = totalsegs*hsegs+2;

	mesh.setNumVerts(nverts);
	mesh.setNumFaces(nfaces);
	if (genUVs) 
	{	ntverts = ulevel*hsegs+3+totalsegs;
		mesh.setNumTVerts(ntverts);
		mesh.setNumTVFaces(nfaces);
	} 
	else 
	{	mesh.setNumTVerts(0);
		mesh.setNumTVFaces(0);
	}
	
	Point3 CenterPt=Point3(width/2.0f,depth/2.0f,height);
	mesh.setVert(0,0.0f,0.0f,height);
	mesh.setVert(nverts-1,0.0f,0.0f,0.0f);
    float heightScale = usePhysUVs ? height : 1.0f;
    float depthScale = usePhysUVs ? depth : 1.0f;
    float widthScale = usePhysUVs ? width : 1.0f;
	if (genUVs) 
	{	mesh.setTVert(0,widthScale*0.5f, heightScale*1.0f,0.0f);
		mesh.setTVert(ntverts-2,widthScale*0.5f,heightScale*0.5f,0.0f);
		mesh.setTVert(ntverts-1,depthScale*0.5f, heightScale*1.0f,0.0f);
	}
	int i,lpt,nv=1,tnv=1,fc=0,t0,tot0=totalsegs+1,tt0;
	float hwid=width/2.0f,hdep=depth/2.0f;
	float lwidth=width/hsegs,wincr,dincr,ldepth=depth/hsegs,hpos;
	float dstart,wstart,wpos,dpos,v,hdelta=height/hsegs;
	t0=tt0=0;int visflag;
	for (i=1;i<=hsegs;i++)
	{	v = (hpos=height-hdelta*i)/height;
		wincr=(wstart=lwidth*i)/wsegs;
		wstart=wstart/-2.0f;
		dincr=(dstart=ldepth*i)/dsegs;
		dstart=dstart/-2.0f;
		mesh.setVert(nv++,wstart,dstart,hpos);
		wpos=wstart;
		visflag=(i==1?ALLF:0);
		if (genUVs) 
			mesh.setTVert(tnv++,widthScale*(1.0f-(hwid-wstart)/width),heightScale*v,0.0f);
		for (lpt=1;lpt<=wsegs;lpt++)
		{	mesh.setVert(nv,wpos+=wincr,dstart,hpos);
			if (genUVs) 
			{	tt0 = tnv-ulevel-1;
				mesh.tvFace[fc].setTVerts((i<2?0:tt0),tnv-1,tnv);
				if (i>1)		
					mesh.tvFace[fc+1].setTVerts(tt0,tnv,tt0+1);
				mesh.setTVert(tnv++,widthScale*(1.0f-(hwid-wpos)/width),heightScale*v,0.0f);
			}
			AddFace(&mesh.faces[fc++],(i>1?t0:0),nv-1,nv,visflag,4);
			if (i>1) 
			{	AddFace(&mesh.faces[fc++],t0,nv,t0+1,1,4);
				t0++;
			}
			nv++;
		}
		if (genUVs) 
			mesh.setTVert(tnv++,depthScale*(1.0f-(hwid-dstart)/depth),heightScale*v,0.0f);
		dpos=dstart;
		for (lpt=1;lpt<=dsegs;lpt++)
		{	mesh.setVert(nv,wpos,dpos+=dincr,hpos);
			if (genUVs) 
			{	tt0=tnv-ulevel-1;
				mesh.tvFace[fc].setTVerts((i<2?ntverts-1:tt0),tnv-1,tnv);
				if (i>1)		
					mesh.tvFace[fc+1].setTVerts(tt0,tnv,tt0+1);
				mesh.setTVert(tnv++,depthScale*(1.0f-(hwid-dpos)/depth),heightScale*v,0.0f);
			}
			AddFace(&mesh.faces[fc++],(i>1?t0:0),nv-1,nv,visflag,2);
			if (i>1) 
			{	AddFace(&mesh.faces[fc++],t0,nv,t0+1,1,2);
				t0++;
			}
			nv++;
		}
		if (genUVs) 
		{	tnv++; 
			tt0=(tnv-=ulevel)-ulevel-1;
		}
		for (lpt=1;lpt<=wsegs;lpt++)
		{	mesh.setVert(nv,wpos-=wincr,dpos,hpos);
			if (genUVs) 
			{	mesh.tvFace[fc].setTVerts((i<2?0:tt0),tnv-1,tnv);
				if (i>1)		
					mesh.tvFace[fc+1].setTVerts(tt0,tnv,tt0+1);
				tnv++;
				tt0++;
			}
			AddFace(&mesh.faces[fc++],(i<2?0:t0),nv-1,nv,visflag,16);
			if (i>1) 
			{	AddFace(&mesh.faces[fc++],t0,nv,t0+1,1,16);
				t0++;
			}
			nv++;
		}
		if (genUVs)
		{	tnv++;
			tt0=tnv-ulevel-1;
		}
		for (lpt=1;lpt<=dsegs;lpt++)
		{	if (lpt<dsegs) 
				mesh.setVert(nv,wpos,dpos-=dincr,hpos);
			if (genUVs) 
			{	mesh.tvFace[fc].setTVerts((i<2?ntverts-1:tt0),tnv-1,tnv);
				if (i>1)		
					mesh.tvFace[fc+1].setTVerts(tt0,tnv,tt0+1);
				tnv++;tt0++;
			}
			if (lpt==dsegs)
			{	AddFace(&mesh.faces[fc++],t0,nv-1,t0+1,visflag,32);
				if (i>1) 
				{	AddFace(&mesh.faces[fc++],t0,t0+1,t0-totalsegs+1,1,32);
					t0++;
				}
			}
			else
			{	AddFace(&mesh.faces[fc++],(i<2?0:t0),nv-1,nv,visflag,32);
				if (i>1) 
				{	AddFace(&mesh.faces[fc++],t0,nv,t0+1,1,32);
					t0++;
				}
			}
			if (lpt<dsegs) 
				nv++;
		}
		if (i==1) 
			t0=1;
	}
	// Now make faces ---
	int b0 = nverts-1,
		bt = ntverts-2;
	if (genUVs) 
	{	mesh.setTVert(tnv++,widthScale*(1.0f-(hwid-mesh.verts[t0].x)/(float)width),depthScale*(hdep-mesh.verts[t0].y)/(float)depth,0.0f);
	}
	t0++;
	for (i=1;i<=totalsegs;i++)
	{	if (genUVs) 
		{	mesh.tvFace[fc].setTVerts(tnv-1,bt,(i<totalsegs?tnv:tnv-totalsegs));
			mesh.setTVert(tnv++,widthScale*(1.0f-(hwid-mesh.verts[t0].x)/(float)width),depthScale*(hdep-mesh.verts[t0].y)/(float)depth,0.0f);
		}
		AddFace(&mesh.faces[fc++],t0-1,b0,(i<totalsegs?t0:t0-totalsegs),ALLF,8);
		t0++;
	}
	if (height<0)
	{	DWORD hold;
		int tedge0,tedge1,tedge2;
		for (i=0;i<fc;i++)
		{	tedge0=(mesh.faces[i].getEdgeVis(0)?EDGE_VIS:EDGE_INVIS);
			tedge1=(mesh.faces[i].getEdgeVis(1)?EDGE_VIS:EDGE_INVIS);
			tedge2=(mesh.faces[i].getEdgeVis(2)?EDGE_VIS:EDGE_INVIS);
			hold=mesh.faces[i].v[0];mesh.faces[i].v[0]=mesh.faces[i].v[2];mesh.faces[i].v[2]=hold;
			mesh.faces[i].setEdgeVisFlags(tedge1,tedge0,tedge2);
 			if (genUVs)
			{	hold=mesh.tvFace[i].t[0];mesh.tvFace[i].t[0]=mesh.tvFace[i].t[2];mesh.tvFace[i].t[2]=hold;
			}
		}
	}

	assert(fc==mesh.numFaces);
//	assert(nv==mesh.numVerts); 
	mesh.InvalidateTopologyCache();
}

BOOL PyramidObject::HasUVW() { 
	BOOL genUVs;
	Interval v;
	pblock2->GetValue(pyramid_mapping, 0, genUVs, v);
	return genUVs; 
	}

void PyramidObject::SetGenUVW(BOOL sw) {  
	if (sw==HasUVW()) return;
	pblock2->SetValue(pyramid_mapping,0, sw);
	UpdateUI();
	}

void PyramidObject::BuildMesh(TimeValue t)
	{	
	int hsegs,wsegs,dsegs;
	float height,width,depth;
	int genUVs;	

	// Start the validity interval at forever and widdle it down.
	ivalid = FOREVER;
	
	pblock2->GetValue(pyramid_hsegs,t,hsegs,ivalid);
	pblock2->GetValue(pyramid_wsegs,t,wsegs,ivalid);
	pblock2->GetValue(pyramid_dsegs,t,dsegs,ivalid);
	pblock2->GetValue(pyramid_height,t,height,ivalid);
	pblock2->GetValue(pyramid_width,t,width,ivalid);
	pblock2->GetValue(pyramid_depth,t,depth,ivalid);
	pblock2->GetValue(pyramid_mapping,t,genUVs,ivalid);
	LimitValue(height, MIN_HEIGHT, MAX_HEIGHT);
	LimitValue(width, MIN_HEIGHT, MAX_HEIGHT);
	LimitValue(depth, MIN_HEIGHT, MAX_HEIGHT);
	LimitValue(hsegs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(wsegs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(dsegs, MIN_SEGMENTS, MAX_SEGMENTS);
	
    BOOL usePhysUVs = GetUsePhysicalScaleUVs();
	BuildPyramidMesh(mesh, hsegs, wsegs, dsegs, height, 
		width, depth, genUVs, usePhysUVs);
	}

inline Point3 operator+(const PatchVert &pv,const Point3 &p)
	{
	return p+pv.p;
	}



Object*
BuildNURBSPyramid(float width, float depth, float height, int genUVs)
{
	int pyramid_faces[5][4] = { {0, 1, 2, 3}, // bottom
							{2, 3, 4, 4}, // back
							{1, 0, 4, 4}, // front
							{3, 1, 4, 4}, // left
							{0, 2, 4, 4}};// right
	Point3 pyramid_verts[5] = { Point3(-0.5, -0.5, 0.0),
							Point3( 0.5, -0.5, 0.0),
							Point3(-0.5,  0.5, 0.0),
							Point3( 0.5,  0.5, 0.0),
							Point3( 0.0,  0.0, 1.0)};

	NURBSSet nset;

	for (int face = 0; face < 5; face++) {
		Point3 bl = pyramid_verts[pyramid_faces[face][0]];
		Point3 br = pyramid_verts[pyramid_faces[face][1]];
		Point3 tl = pyramid_verts[pyramid_faces[face][2]];
		Point3 tr = pyramid_verts[pyramid_faces[face][3]];

		Matrix3 size;
		size.IdentityMatrix();
		Point3 lwh(width, depth, height);
		size.Scale(lwh);

		bl = bl * size;
		br = br * size;
		tl = tl * size;
		tr = tr * size;

		NURBSCVSurface *surf = new NURBSCVSurface();
		nset.AppendObject(surf);
		surf->SetUOrder(4);
		surf->SetVOrder(4);
		surf->SetNumCVs(4, 4);
		surf->SetNumUKnots(8);
		surf->SetNumVKnots(8);

		Point3 top, bot;
		for (int r = 0; r < 4; r++) {
			top = tl + (((float)r/3.0f) * (tr - tl));
			bot = bl + (((float)r/3.0f) * (br - bl));
			for (int c = 0; c < 4; c++) {
				NURBSControlVertex ncv;
				ncv.SetPosition(0, bot + (((float)c/3.0f) * (top - bot)));
				ncv.SetWeight(0, 1.0f);
				surf->SetCV(r, c, ncv);
			}
		}

		for (int k = 0; k < 4; k++) {
			surf->SetUKnot(k, 0.0);
			surf->SetVKnot(k, 0.0);
			surf->SetUKnot(k + 4, 1.0);
			surf->SetVKnot(k + 4, 1.0);
		}

		surf->Renderable(TRUE);
		surf->SetGenerateUVs(genUVs);
		if (height > 0.0f)
			surf->FlipNormals(TRUE);
		else
			surf->FlipNormals(FALSE);

		switch(face) {
		case 0: // bottom
			surf->SetTextureUVs(0, 0, Point2(1.0f, 0.0f));
			surf->SetTextureUVs(0, 1, Point2(0.0f, 0.0f));
			surf->SetTextureUVs(0, 2, Point2(1.0f, 1.0f));
			surf->SetTextureUVs(0, 3, Point2(0.0f, 1.0f));
			break;
		default: // sides
			surf->SetTextureUVs(0, 0, Point2(0.5f, 1.0f));
			surf->SetTextureUVs(0, 1, Point2(0.5f, 1.0f));
			surf->SetTextureUVs(0, 2, Point2(0.0f, 0.0f));
			surf->SetTextureUVs(0, 3, Point2(1.0f, 0.0f));
			break;
		}

		TCHAR bname[80];
		_stprintf(bname, _T("%s%02d"), GetString(IDS_CT_SURF), face);
		surf->SetName(bname);
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
	// Fuse the degenerate peaks
	for (int i = 1; i < 5; i++) {
		for (int j = 1; j < 4; j++) {
			F(i, i, 0, 3, j, 3);
		}
	}

	// Fuse the peaks together
	F(1, 2, 0, 3, 0, 3);
	F(1, 3, 0, 3, 0, 3);
	F(1, 4, 0, 3, 0, 3);

	// Bottom(0) to Back (1)
	F(0, 1, 3, 3, 3, 0);
	F(0, 1, 2, 3, 2, 0);
	F(0, 1, 1, 3, 1, 0);
	F(0, 1, 0, 3, 0, 0);

	// Bottom(0) to Front (2)
	F(0, 2, 0, 0, 3, 0);
	F(0, 2, 1, 0, 2, 0);
	F(0, 2, 2, 0, 1, 0);
	F(0, 2, 3, 0, 0, 0);

	// Bottom(0) to Left (3)
	F(0, 3, 3, 0, 3, 0);
	F(0, 3, 3, 1, 2, 0);
	F(0, 3, 3, 2, 1, 0);
	F(0, 3, 3, 3, 0, 0);

	// Bottom(0) to Right (4)
	F(0, 4, 0, 0, 0, 0);
	F(0, 4, 0, 1, 1, 0);
	F(0, 4, 0, 2, 2, 0);
	F(0, 4, 0, 3, 3, 0);

	// Front (2)  to Right (4)
	F(2, 4, 3, 1, 0, 1);
	F(2, 4, 3, 2, 0, 2);

	// Right (4) to Back (1)
	F(4, 1, 3, 1, 0, 1);
	F(4, 1, 3, 2, 0, 2);

	// Back (1) to Left (3)
	F(1, 3, 3, 1, 0, 1);
	F(1, 3, 3, 2, 0, 2);

	// Left (3) to Front (2)
	F(3, 2, 3, 1, 0, 1);
	F(3, 2, 3, 2, 0, 2);

	Matrix3 mat;
	mat.IdentityMatrix();
	Object *obj = CreateNURBSObject(NULL, &nset, mat);
	return obj;
}

Object* PyramidObject::ConvertToType(TimeValue t, Class_ID obtype)
	{
	if (obtype == EDITABLE_SURF_CLASS_ID) {
		Interval valid = FOREVER;
		float depth, width, height;
		int genUVs;
		pblock2->GetValue(pyramid_width,t,width,valid);
		pblock2->GetValue(pyramid_depth,t,depth,valid);
		pblock2->GetValue(pyramid_height,t,height,valid);
		pblock2->GetValue(pyramid_mapping,t,genUVs,valid);
		Object *ob = BuildNURBSPyramid(width, depth, height, genUVs);
		ob->SetChannelValidity(TOPO_CHAN_NUM,valid);
		ob->SetChannelValidity(GEOM_CHAN_NUM,valid);
		ob->UnlockObject();
		return ob;
	} 
    return __super::ConvertToType(t,obtype);
	}

int PyramidObject::CanConvertToType(Class_ID obtype)
	{
	if (obtype == EDITABLE_SURF_CLASS_ID)
        return 1;
	if (obtype == triObjectClassID)
		return 1;

    return __super::CanConvertToType(obtype);
	}


void PyramidObject::GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist)
{
    __super::GetCollapseTypes(clist, nlist);
    Class_ID id = EDITABLE_SURF_CLASS_ID;
    TSTR *name = new TSTR(GetString(IDS_SM_NURBS_SURFACE));
    clist.Append(1,&id);
    nlist.Append(1,&name);
}


class PyramidObjCreateCallBack: public CreateMouseCallBack {
	PyramidObject *ob;	
	Point3 p[2],d;
	IPoint2 sp0,sp1;
	BOOL square;
	public:
		int proc( ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat ) override;
		void SetObj(PyramidObject *obj) { ob = obj; }
	};

int PyramidObjCreateCallBack::proc(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat ) 
{
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}


	DWORD snapdim = SNAP_IN_3D;

	if (msg == MOUSE_FREEMOVE)
	{
		vpt->SnapPreview(m,m,NULL, snapdim);
	}
	bool createFromCenter = pyramid_crtype_blk.GetInt(pyramid_create_meth) == 1;
	if (msg==MOUSE_POINT||msg==MOUSE_MOVE) {
		switch(point) {
		case 0:
				ob->suspendSnap = TRUE;				
				sp0 = m;				
				p[0] = vpt->SnapPoint(m,m,NULL,snapdim);
				p[1] = p[0] + Point3(.01,.01,.01);
				mat.SetTrans(float(.5)*(p[0]+p[1]));				
				ob->pblock2->SetValue(pyramid_width,0,0.01f);
				ob->pblock2->SetValue(pyramid_depth,0,0.01f);
				ob->pblock2->SetValue(pyramid_height,0,0.01f);
				break;
			case 1: 
				sp1 = m;
				p[1] = vpt->SnapPoint(m,m,NULL,snapdim);
				p[1].z = p[0].z +(float).01;
				if (createFromCenter || (flags&MOUSE_CTRL))
				{ mat.SetTrans(p[0]);	} 
				else mat.SetTrans(float(.5)*(p[0]+p[1]));
				d = p[1]-p[0];
				square = FALSE;
				if (flags&MOUSE_CTRL) {
					// Constrain to square base
					float len;
					if (fabs(d.x) > fabs(d.y)) len = d.x;
					else len = d.y;
					d.x = d.y = 2.0f * len;
					square = TRUE;
					}
				ob->pblock2->SetValue(pyramid_width,0,(float)fabs(d.x));
				ob->pblock2->SetValue(pyramid_depth,0,(float)fabs(d.y));

				if (msg==MOUSE_POINT && (Length(sp1-sp0)<3 )) 
				{ return CREATE_ABORT;	}
				break;
			case 2:
				p[1].z = vpt->SnapLength(vpt->GetCPDisp(p[0],Point3(0,0,1),sp1,m,TRUE));
//				mat.SetTrans(2,(p[1].z>0?p[0].z:p[1].z));			
				ob->pblock2->SetValue(pyramid_width,0,(float)fabs(d.x));
				ob->pblock2->SetValue(pyramid_depth,0,(float)fabs(d.y));
				ob->pblock2->SetValue(pyramid_height,0,p[1].z);

				if (msg==MOUSE_POINT) {					
					ob->suspendSnap = FALSE;
					return (Length(m-sp0)<3)?CREATE_ABORT:CREATE_STOP;
					}
				break;

			}
	} else {
		if (msg == MOUSE_ABORT)
			return CREATE_ABORT;
		}
	return 1;
	}

static PyramidObjCreateCallBack cylCreateCB;

CreateMouseCallBack* PyramidObject::GetCreateMouseCallBack() 
	{
	cylCreateCB.SetObj(this);
	return(&cylCreateCB);
	}

BOOL PyramidObject::OKtoDisplay(TimeValue t) 
	{
	float radius;
	pblock2->GetValue(pyramid_width,t,radius,FOREVER);
	if (radius==0.0f) return FALSE;
	else return TRUE;
	}

void PyramidObject::InvalidateUI() 
	{
	pyramid_param_blk.InvalidateUI(pblock2->LastNotifyParamID());
	}

RefTargetHandle PyramidObject::Clone(RemapDir& remap) 
	{
	PyramidObject* newob = new PyramidObject(FALSE);	
	newob->ReplaceReference(0,remap.CloneRef(pblock2));	
	newob->ivalid.SetEmpty();	
	BaseClone(this, newob, remap);
    newob->SetUsePhysicalScaleUVs(GetUsePhysicalScaleUVs());
	return(newob);
	}

void PyramidObject::UpdateUI()
{
	if (ip == NULL)
		return;
	PyramidParamDlgProc* dlg = static_cast<PyramidParamDlgProc*>(pyramid_param_blk.GetUserDlgProc());
	dlg->UpdateUI();
}

BOOL PyramidObject::GetUsePhysicalScaleUVs()
{
    return ::GetUsePhysicalScaleUVs(this);
}


void PyramidObject::SetUsePhysicalScaleUVs(BOOL flag)
{
    BOOL curState = GetUsePhysicalScaleUVs();
    if (curState == flag)
        return;
    if (theHold.Holding())
        theHold.Put(new RealWorldScaleRecord<PyramidObject>(this, curState));
    ::SetUsePhysicalScaleUVs(this, flag);
    if (pblock2 != NULL)
		pblock2->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	UpdateUI();
	macroRec->SetProperty(this, _T("realWorldMapSize"), mr_bool, flag);
}

