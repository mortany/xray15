/*****************************************************************************
 *<
	FILE: chbox.cpp

	DESCRIPTION: Chbox - builds filleted/chamfered boxes

	CREATED BY:  Audrey Peterson
	Copyright (c) 1996 All Rights Reserved
 *>
 *****************************************************************************/
// Resource include file.
#include "solids.h"
#include "iparamm2.h"
#include "simpobj.h"
#include "macroRec.h"
#include "RealWorldMapUtils.h"
#include <ReferenceSaveManager.h>
#include <Util/StaticAssert.h>

#define topsquare 0
#define messyedge 1
#define middle 2
#define bottomedge 3
#define bottomsquare 4

typedef struct{
  int surface,deltavert;
} chinfo;

class ChBoxObject : public SimpleObject2, public RealWorldMapSizeInterface {
	friend class ChBoxTypeInDlgProc;
	friend class ChBoxObjCreateCallBack;
	friend class ChBoxFilletAccessor;
	public:
		// Class vars	
		static IObjParam *ip;
		static bool typeinCreate;

		ChBoxObject(BOOL loading);
		
		// From Object
		int CanConvertToType(Class_ID obtype) override;
		Object* ConvertToType(TimeValue t, Class_ID obtype) override;

		// From BaseObject
		CreateMouseCallBack* GetCreateMouseCallBack() override;
		void BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev) override;
		void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next) override;
		const TCHAR *GetObjectName() override { return GetString(IDS_RB_CHNAME); }
		BOOL HasUVW() override;
		void SetGenUVW(BOOL sw) override;

		// Animatable methods
		void DeleteThis() override { delete this; }
		Class_ID ClassID() override { return CHAMFERBOX_CLASS_ID; }
		
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
			if (NULL != pInterface)
			{
				return pInterface;
			}

			return SimpleObject2::GetInterface(id);
		} 
	};				

// in solids.cpp  - The dll instance handle
extern HINSTANCE hInstance;

// class variables for Chamfer Box class.
IObjParam *ChBoxObject::ip = NULL;
bool ChBoxObject::typeinCreate = false;

#define PBLOCK_REF_NO  0

static BOOL sInterfaceAdded = FALSE;

class ChBoxObjClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() override { return 1; }
	void *			Create(BOOL loading = FALSE) override
	{
		if (!sInterfaceAdded) {
			AddInterface(&gRealWorldMapSizeDesc);
			sInterfaceAdded = TRUE;
		}
		return new ChBoxObject(loading);
	}
	const TCHAR *	ClassName() override { return GetString(IDS_AP_CHBOX_CLASS); }
	SClass_ID		SuperClassID() override { return GEOMOBJECT_CLASS_ID; }
	Class_ID		ClassID() override { return CHAMFERBOX_CLASS_ID; }
	const TCHAR* 	Category() override { return GetString(IDS_RB_EXTENDED); }
	const TCHAR* InternalName() { return _T("ChamferBox"); } // returns fixed parsable name (scripter-visible name)
	HINSTANCE  HInstance() { return hInstance; }   // returns owning module handle
};

static ChBoxObjClassDesc chboxObjDesc;
ClassDesc* GetChBoxobjDesc() { return &chboxObjDesc; }



// Misc stuff
#define CMIN_RADIUS		float(0.0)
#define BMIN_LENGTH		float(0.1)
#define BMAX_LENGTH		float(1.0E30)
#define BMIN_WIDTH		float(0.1)
#define BMAX_WIDTH		float(1.0E30)
#define BMIN_HEIGHT		float(-1.0E30)
#define BMAX_HEIGHT		float(1.0E30)

#define BDEF_DIM		float(0)
#define BDEF_SEGS		1
#define CDEF_SEGS		3

#define MIN_SEGMENTS	1
#define MAX_SEGMENTS	200

// ParamBlockDesc2 IDs
enum paramblockdesc2_ids { chbox_creation_type, chbox_type_in, chbox_params, };
enum chbox_creation_type_param_ids { chbox_create_meth, };
enum chbox_type_in_param_ids { chbox_ti_pos, chbox_ti_length, chbox_ti_width, chbox_ti_height, chbox_ti_radius, };
enum chbox_param_param_ids {
	chbox_length = CHBOX_LENGTH, chbox_width = CHBOX_WIDTH, chbox_height = CHBOX_HEIGHT,
	chbox_radius = CHBOX_RADIUS, chbox_lsegs = CHBOX_LSEGS, chbox_wsegs = CHBOX_WSEGS,
	chbox_hsegs = CHBOX_HSEGS, chbox_csegs = CHBOX_CSEGS, chbox_mapping = CHBOX_GENUVS,
	chbox_smooth = CHBOX_SMOOTH,
};

namespace
{
	MaxSDK::Util::StaticAssert< (chbox_params == CHBOX_PARAMBLOCK_ID) > validator;
}


void FixChBoxFillet(IParamBlock2* pblock2, TimeValue t, ParamID lengthId, ParamID widthId, ParamID filletId, ParamID id, PB2Value &v)
{
	float length = 0.f, width = 0.f, fillet = 0.f;
	pblock2->GetValue(lengthId, t, length, FOREVER);
	pblock2->GetValue(widthId, t, width, FOREVER);
	pblock2->GetValue(filletId, t, fillet, FOREVER);
	// Fillet length upper limit is the minimum between length/2 and width/2
	float maxf = (length > width ? 0.5f*(float)fabs(width) : 0.5f*(float)fabs(length));
	IParamMap2 * pParamMap = pblock2->GetMap(filletId);
	if (pParamMap)
		pParamMap->SetRange(filletId, 0.f, maxf);
	if (fillet > maxf)
	{
		if (id == filletId)
			v.f = maxf;
		else if (id == lengthId || id == widthId)
			pblock2->SetValue(filletId, t, maxf);
	}
}

void InitChBoxFillet(IParamBlock2* pblock2, TimeValue t, ParamID lengthId, ParamID widthId, ParamID filletId)
{
	float length = 0.f, width = 0.f, fillet = 0.f;
	pblock2->GetValue(lengthId, t, length, FOREVER);
	pblock2->GetValue(widthId, t, width, FOREVER);
	pblock2->GetValue(filletId, t, fillet, FOREVER);
	// Fillet length upper limit is the minimum between length/2 and width/2
	float maxf = (length > width ? 0.5f*(float)fabs(width) : 0.5f*(float)fabs(length));
	IParamMap2 * pParamMap = pblock2->GetMap(filletId);
	if (pParamMap)
		pParamMap->SetRange(filletId, 0.f, maxf);
}


class ChBoxFilletClassAccessor : public PBAccessor
{
	bool m_disable; // for preventing re-entry
public:
	ChBoxFilletClassAccessor() : m_disable(false) {}
	virtual void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t) override
	{
		if (m_disable)
			return;
		if (theHold.RestoreOrRedoing())
			return;
		IParamBlock2* pblock2 = chboxObjDesc.GetParamBlockDescByID(chbox_type_in)->class_params;
		DbgAssert(pblock2);
		if (pblock2 == nullptr)
			return;
		m_disable = true;
		FixChBoxFillet(pblock2, t, chbox_ti_length, chbox_ti_width, chbox_ti_radius, id, v);
		m_disable = false;
	}
};
static ChBoxFilletClassAccessor chBoxFilletClass_Accessor;


class ChBoxFilletAccessor : public PBAccessor
{
	bool m_disable; // for preventing re-entry
public:
	ChBoxFilletAccessor() : m_disable(false) {}
	virtual void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t) override
	{
		if (m_disable)
			return;
		if (theHold.RestoreOrRedoing())
			return;
		IParamBlock2* pblock2 = ((ChBoxObject*)owner)->pblock2;
		DbgAssert(pblock2);
		if (pblock2 == nullptr)
			return;
		m_disable = true;
		FixChBoxFillet(pblock2, t, chbox_length, chbox_width, chbox_radius, id, v);
		m_disable = false;
	}
};
static ChBoxFilletAccessor chBoxFillet_Accessor;

namespace
{
	class MapCoords_Accessor : public PBAccessor
	{
		void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
		{
			if (owner) ((ChBoxObject*)owner)->UpdateUI();
		}
	};
	static MapCoords_Accessor mapCoords_Accessor;
}

// class creation type block
static ParamBlockDesc2 chbox_crtype_blk(chbox_creation_type, _T("ChamferBoxCreationType"), 0, &chboxObjDesc, P_CLASS_PARAMS + P_AUTO_UI,
	//rollout
	IDD_CHAMFERCUBE1, IDS_RB_CREATE_DIALOG, BEGIN_EDIT_CREATE, 0, NULL,
	// params
	chbox_create_meth, _T("typeinCreationMethod"), TYPE_INT, 0, IDS_RB_CREATE_DIALOG,
	p_default, 0,
	p_range, 0, 1,
	p_ui, TYPE_RADIO, 2, IDC_CC_CREATEBOX, IDC_CC_CREATECUBE,
	p_end,
	p_end
	);

// class type-in block
static ParamBlockDesc2 chbox_typein_blk(chbox_type_in, _T("ChamferBoxTypeIn"), 0, &chboxObjDesc, P_CLASS_PARAMS + P_AUTO_UI,
	//rollout
	IDD_CHAMFERCUBE2, IDS_RB_KEYBOARDENTRY, BEGIN_EDIT_CREATE, APPENDROLL_CLOSED, NULL,
	// params
	chbox_ti_pos, _T("typeInPos"), TYPE_POINT3, 0, IDS_RB_TYPEIN_POS,
	p_default, Point3(0, 0, 0),
	p_range, float(-1.0E30), float(1.0E30),
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CC_POSX, IDC_CC_POSXSPIN, IDC_CC_POSY, IDC_CC_POSYSPIN, IDC_CC_POSZ, IDC_CC_POSZSPIN, SPIN_AUTOSCALE,
	p_end,
	chbox_ti_length, _T("typeInLength"), TYPE_FLOAT, 0, IDS_RB_LENGTH,
	p_default, 0.1f,
	p_range, BMIN_LENGTH, BMAX_LENGTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CC_LENGTH, IDC_CC_LENGTHSPIN, SPIN_AUTOSCALE,
	p_accessor, &chBoxFilletClass_Accessor,
	p_end,
	chbox_ti_width, _T("typeInWidth"), TYPE_FLOAT, 0, IDS_RB_WIDTH,
	p_default, 0.1f,
	p_range, BMIN_WIDTH, BMAX_WIDTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CC_WIDTH, IDC_CC_WIDTHSPIN, SPIN_AUTOSCALE,
	p_accessor, &chBoxFilletClass_Accessor,
	p_end,
	chbox_ti_height, _T("typeInHeight"), TYPE_FLOAT, 0, IDS_RB_HEIGHT,
	p_default, 0.1f,
	p_range, BMIN_HEIGHT, BMAX_HEIGHT,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CC_HEIGHT, IDC_CC_HEIGHTSPIN, SPIN_AUTOSCALE,
	p_end,
	chbox_ti_radius, _T("typeInFillet"), TYPE_FLOAT, 0, IDS_RB_FILLET,
	p_default, 0.01f,
	p_range, CMIN_RADIUS, BMAX_HEIGHT,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CC_RADIUS, IDC_CC_RADIUSSPIN, SPIN_AUTOSCALE,
	p_accessor, &chBoxFilletClass_Accessor,
	p_end,
	p_end
	);

// per instance box block
static ParamBlockDesc2 chbox_param_blk(chbox_params, _T("ChamferBoxParameters"), 0, &chboxObjDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_REF_NO,
	//rollout
	IDD_CHAMFERCUBE3, IDS_AP_PARAMETERS, 0, 0, NULL,
	// params
	chbox_length, _T("length"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_LENGTH,
	p_default, 0.1f,
	p_range, BMIN_LENGTH, BMAX_LENGTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CC_LENGTH, IDC_CC_LENGTHSPIN, SPIN_AUTOSCALE,
	p_accessor, &chBoxFillet_Accessor,
	p_end,
	chbox_width, _T("width"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_WIDTH,
	p_default, 0.1f,
	p_range, BMIN_WIDTH, BMAX_WIDTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CC_WIDTH, IDC_CC_WIDTHSPIN, SPIN_AUTOSCALE,
	p_accessor, &chBoxFillet_Accessor,
	p_end,
	chbox_height, _T("height"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_HEIGHT,
	p_default, 0.1f,
	p_range, BMIN_HEIGHT, BMAX_HEIGHT,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CC_HEIGHT, IDC_CC_HEIGHTSPIN, SPIN_AUTOSCALE,
	p_end,
	chbox_radius, _T("fillet"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_FILLET,
	p_default, 0.01f,
	p_range, CMIN_RADIUS, BMAX_HEIGHT,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CC_RADIUS, IDC_CC_RADIUSSPIN, SPIN_AUTOSCALE,
	p_accessor, &chBoxFillet_Accessor,
	p_end,
	chbox_lsegs, _T("Length Segments"), TYPE_INT, P_ANIMATABLE, IDS_RB_LSEGS,
	p_default, BDEF_SEGS,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_CC_LENSEGS, IDC_CC_LENSEGSSPIN, 0.1f,
	p_end,
	chbox_wsegs, _T("Width Segments"), TYPE_INT, P_ANIMATABLE, IDS_RB_WSEGS,
	p_default, BDEF_SEGS,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_CC_WIDSEGS, IDC_CC_WIDSEGSSPIN, 0.1f,
	p_end,
	chbox_hsegs, _T("Height Segments"), TYPE_INT, P_ANIMATABLE, IDS_RB_HSEGS,
	p_default, BDEF_SEGS,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_CC_HGTSEGS, IDC_CC_HGTSEGSSPIN, 0.1f,
	p_end,
	chbox_csegs, _T("Fillet Segments"), TYPE_INT, P_ANIMATABLE, IDS_RB_CSEGS,
	p_default, 3,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_CC_RADSEGS, IDC_CC_RADSEGSSPIN, 0.1f,
	p_end,
	chbox_mapping, _T("mapCoords"), TYPE_BOOL, P_RESET_DEFAULT, IDS_MXS_GENUVS,
	p_default, TRUE,
	p_ui, TYPE_SINGLECHECKBOX, IDC_GENTEXTURE,
	p_accessor, &mapCoords_Accessor,
	p_end,
	chbox_smooth, _T("smooth"), TYPE_BOOL, P_RESET_DEFAULT, IDS_MXS_SMOOTH,
	p_default, TRUE,
	p_ui, TYPE_SINGLECHECKBOX, IDC_OT_SMOOTH,
	p_end,
	p_end
	);

ParamBlockDescID descVer0[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0 },
	{ TYPE_FLOAT, NULL, TRUE, 1 },
	{ TYPE_FLOAT, NULL, TRUE, 2 },
	{ TYPE_FLOAT, NULL, TRUE, 3 }, 
	{ TYPE_INT, NULL, TRUE, 4 }, 
	{ TYPE_INT, NULL, TRUE, 5 }, 
	{ TYPE_INT, NULL, TRUE, 6 }, 
	{ TYPE_INT, NULL, TRUE, 7 }, 
	{ TYPE_INT, NULL, FALSE, 8 }, 
	};

ParamBlockDescID descVer1[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0 },
	{ TYPE_FLOAT, NULL, TRUE, 1 },
	{ TYPE_FLOAT, NULL, TRUE, 2 },
	{ TYPE_FLOAT, NULL, TRUE, 3 }, 
	{ TYPE_INT, NULL, TRUE, 4 }, 
	{ TYPE_INT, NULL, TRUE, 5 }, 
	{ TYPE_INT, NULL, TRUE, 6 }, 
	{ TYPE_INT, NULL, TRUE, 7 }, 
	{ TYPE_INT, NULL, FALSE, 8 }, 
	{ TYPE_INT, NULL, FALSE, 9 }, 
	};

// Array of old versions
static ParamVersionDesc versions[] = {
	ParamVersionDesc(descVer0,9,0),
	ParamVersionDesc(descVer1,10,1),
	};
#define NUM_OLDVERSIONS	2

// ParamBlock data for SaveToPrevious support
#define PBLOCK_LENGTH	10
#define CURRENT_VERSION 1


//--- TypeInDlgProc --------------------------------

class ChBoxTypeInDlgProc : public ParamMap2UserDlgProc {
	public:
		ChBoxObject *ob;

		ChBoxTypeInDlgProc(ChBoxObject *o) {ob=o;}
		INT_PTR DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam) override;
		void DeleteThis() override {delete this;}
	};

INT_PTR ChBoxTypeInDlgProc::DlgProc(
		TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
	{
	switch (msg) {
		case WM_INITDIALOG: {
			InitChBoxFillet(map->GetParamBlock(), t, chbox_ti_length, chbox_ti_width, chbox_ti_radius);
			break;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_CC_CREATE: {					
					// We only want to set the value if the object is 
					// not in the scene.
					if (ob->TestAFlag(A_OBJ_CREATING)) {
						ob->pblock2->SetValue(chbox_length, 0, chbox_typein_blk.GetFloat(chbox_ti_length));
						ob->pblock2->SetValue(chbox_width, 0, chbox_typein_blk.GetFloat(chbox_ti_width));
						ob->pblock2->SetValue(chbox_height, 0, chbox_typein_blk.GetFloat(chbox_ti_height));
						ob->pblock2->SetValue(chbox_radius, 0, chbox_typein_blk.GetFloat(chbox_ti_radius));
					}
					else
						ChBoxObject::typeinCreate = true;

					Matrix3 tm(1);
					tm.SetTrans(chbox_typein_blk.GetPoint3(chbox_ti_pos));
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


class ChBoxParamDlgProc : public ParamMap2UserDlgProc {
	public:
		ChBoxObject *ob;
        HWND mhWnd;
		ChBoxParamDlgProc(ChBoxObject *o) {ob=o;}
		INT_PTR DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam) override;
		void DeleteThis() override {delete this;}
        void UpdateUI();
        BOOL GetRWSState();
	};

BOOL ChBoxParamDlgProc::GetRWSState()
{
    BOOL check = IsDlgButtonChecked(mhWnd, IDC_REAL_WORLD_MAP_SIZE);
    return check;
}

void ChBoxParamDlgProc::UpdateUI()
{
    if (ob == NULL) return;
    BOOL usePhysUVs = ob->GetUsePhysicalScaleUVs();
    CheckDlgButton(mhWnd, IDC_REAL_WORLD_MAP_SIZE, usePhysUVs);
    EnableWindow(GetDlgItem(mhWnd, IDC_REAL_WORLD_MAP_SIZE), ob->HasUVW());
}

INT_PTR ChBoxParamDlgProc::DlgProc(
		TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
	{
	switch (msg) {
    case WM_INITDIALOG: {
        mhWnd = hWnd;
		InitChBoxFillet(map->GetParamBlock(), t, chbox_length, chbox_width, chbox_radius);
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


//--- Box methods -------------------------------

// Constructor
ChBoxObject::ChBoxObject(BOOL loading)
{
	chboxObjDesc.MakeAutoParamBlocks(this);

	if (!loading && !GetPhysicalScaleUVsDisabled())
		SetUsePhysicalScaleUVs(true);
}

bool ChBoxObject::SpecifySaveReferences(ReferenceSaveManager& referenceSaveManager)
{
	// if saving to previous version that used pb1 instead of pb2...
	DWORD saveVersion = GetSavingVersion();
	if (saveVersion != 0 && saveVersion <= MAX_RELEASE_R19)
	{
		ProcessPB2ToPB1SaveToPrevious(this, pblock2, PBLOCK_REF_NO, descVer1, PBLOCK_LENGTH, CURRENT_VERSION);
	}
	return __super::SpecifySaveReferences(referenceSaveManager);
}

IOResult ChBoxObject::Load(ILoad *iload)
{
	ParamBlock2PLCB* plcb = new ParamBlock2PLCB(versions, NUM_OLDVERSIONS, &chbox_param_blk, this, PBLOCK_REF_NO);
	iload->RegisterPostLoadCallback(plcb);
	return IO_OK;
}

// This method is called by the system when the user needs 
// to edit the objects parameters in the command panel.  
void ChBoxObject::BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev)
{
	__super::BeginEditParams(ip,flags,prev);

	// Save the interface pointer.
	this->ip = ip;

	// If this has been freshly created by type-in, set creation values:
	if (ChBoxObject::typeinCreate)
	{
		bool createCube = chbox_crtype_blk.GetInt(chbox_create_meth) == 1;
		if (createCube)
		{
			float val = chbox_typein_blk.GetFloat(chbox_ti_length);
			pblock2->SetValue(chbox_length, 0, val);
			pblock2->SetValue(chbox_width, 0, val);
			pblock2->SetValue(chbox_height, 0, val);
		}
		else
		{
			pblock2->SetValue(chbox_length, 0, chbox_typein_blk.GetFloat(chbox_ti_length));
			pblock2->SetValue(chbox_width, 0, chbox_typein_blk.GetFloat(chbox_ti_width));
			pblock2->SetValue(chbox_height, 0, chbox_typein_blk.GetFloat(chbox_ti_height));
		}
		typeinCreate = false;
	}

	// throw up all the appropriate auto-rollouts
	chboxObjDesc.BeginEditParams(ip, this, flags, prev);
	// if in Create Panel, install a callback for the type in.
	if (flags & BEGIN_EDIT_CREATE)
	{
		chbox_typein_blk.SetUserDlgProc(new ChBoxTypeInDlgProc(this));
	}
	// install a callback for the params.
	chbox_param_blk.SetUserDlgProc(new ChBoxParamDlgProc(this));

}

// This is called by the system to terminate the editing of the
// parameters in the command panel.  
void ChBoxObject::EndEditParams( IObjParam *ip, ULONG flags,Animatable *next )
{
	__super::EndEditParams(ip,flags,next);
	this->ip = NULL;
	chboxObjDesc.EndEditParams(ip, this, flags, next);
}

#define POSX 0	// right
#define POSY 1	// back
#define POSZ 2	// top
#define NEGX 3	// left
#define NEGY 4	// front
#define NEGZ 5	// bottom

int direction(Point3 *v) {
	Point3 a = v[0]-v[2];
	Point3 b = v[1]-v[0];
	Point3 n = CrossProd(a,b);
	switch(MaxComponent(n)) {
		case 0: return (n.x<0)?NEGX:POSX;
		case 1: return (n.y<0)?NEGY:POSY;
		case 2: return (n.z<0)?NEGZ:POSZ;
		}
	return 0;
	}

// Remap the sub-object material numbers so that the top face is the first one
// The order now is:
// Top / Bottom /  Left/ Right / Front / Back
static int mapDir[6] ={ 3, 5, 0, 2, 4, 1 };
static int hsegs,wsegs,csegs,fcount,scount,curvertex;
static float wincr,hincr,cincr;
static Point3 NewPt,Toppt,CornerPt;
static int boxpos,wsegcount,hseg;

void CalculateHeightDivisions(int plus,int *PtRotation)
{float deltay;

  curvertex++;
  if (hseg==hsegs)
    { (*PtRotation)++;
      hseg=0;
      NewPt.y=(plus?Toppt.y:-CornerPt.y);}
  else
   { deltay=hincr*hseg;
     NewPt.y=(plus?-CornerPt.y+deltay:Toppt.y-deltay);
     hseg++;
   }
}
void CalculateWidthDivisions(int plus,int *PtRotation)
{float deltax;

  curvertex++;
   if (wsegcount==wsegs)
       {NewPt.x=(plus?CornerPt.x:-CornerPt.x);
       wsegcount=0;(*PtRotation)++;}
   else
    { deltax=wincr*wsegcount;
      NewPt.x=(plus?-CornerPt.x+deltax:CornerPt.x-deltax);
      wsegcount++;
    }
}

void FillinSquare(int *PtRotation,int *cornervert,float CurRadius)
{ if (hseg>0) CalculateHeightDivisions(((*PtRotation)>1),PtRotation);
  else if (wsegcount>0) CalculateWidthDivisions((*PtRotation)<3,PtRotation);
  else
   { switch (*PtRotation){
       case 0: NewPt.x=-CornerPt.x-CurRadius;
               NewPt.y=CornerPt.y;
               hseg++;
               break;
       case 1: NewPt.x=-CornerPt.x;
               NewPt.y=-CornerPt.y-CurRadius;
               wsegcount++;
               break;
       case 2: NewPt.x=CornerPt.x+CurRadius;
               NewPt.y=-CornerPt.y;
               hseg++;
               break;
       case 3: NewPt.x=CornerPt.x;
               NewPt.y=CornerPt.y+CurRadius;
               wsegcount++;
               break;
       default:;
      }
      curvertex+=csegs;
      if ((*PtRotation)==2) cornervert[*PtRotation]=curvertex-csegs;
      else cornervert[*PtRotation]=curvertex;
    }
}

void CalculateNewPt(float dx,float dy,int *PtRotation,int *cornervert,int deltapt)
{
    (*PtRotation)++;
    switch (*PtRotation){
      case 1: NewPt.y=-CornerPt.y-dy;
              NewPt.x=-CornerPt.x-dx;
              curvertex=cornervert[1]-deltapt;
              break;
      case 2: NewPt.x=CornerPt.x+dx;
              NewPt.y=-CornerPt.y-dy;
              curvertex=cornervert[2]+deltapt;
              break;
      case 3: NewPt.y=CornerPt.y+dy;
              curvertex=cornervert[3]-deltapt;
              *PtRotation=6;
              break;
   }
}

static int sidenum,endpt,face,topchamferfaces,chamferstart;
static int SidesPerSlice,topnum,tstartpt,maxfaces;
static int circleseg,firstface,cstartpt;
static chinfo chamferinfo[4];

//int ChBoxObject::getnextfirstvertex()
int getnextfirstvertex()
{ int c;

  if (boxpos==bottomedge)
  { c=curvertex -=chamferinfo[sidenum].deltavert;
    if (curvertex==endpt)
    { circleseg=1;firstface=0;
      if (endpt!=cstartpt) sidenum++;
       endpt-=chamferinfo[sidenum].deltavert*(chamferinfo[sidenum].surface==1?hsegs:wsegs);
     }
  }
  else
  { c=++curvertex;
    if (boxpos==messyedge)
     { c=(face==topchamferfaces+chamferstart-2?cstartpt:curvertex);
       if ((circleseg>0)&&(circleseg++>csegs)) circleseg=0;
     }
    else  if (boxpos==middle)
     c=(++fcount==SidesPerSlice?fcount=0,curvertex-SidesPerSlice:curvertex);

  }
  return(c);
}

int getnextsecondvertex()
{ int c;

  if (boxpos==messyedge)
  { c=topnum +=chamferinfo[sidenum].deltavert;
    if (topnum==endpt)
    { circleseg=1;firstface=1;
      if (endpt!=tstartpt) sidenum++;
      else {topnum=cstartpt;boxpos=middle;circleseg=0;firstface=0;}
       endpt+=chamferinfo[sidenum].deltavert*(chamferinfo[sidenum].surface==1?hsegs:wsegs);
     }
  }
  else
  { c=++topnum;
   if (boxpos==bottomedge)
    { c=(face==maxfaces-chamferstart-1?tstartpt:topnum);
      if ((circleseg>0)&&(circleseg++>csegs)) circleseg=0;
    }
    else if (boxpos==middle)
       c=(++scount==SidesPerSlice?scount=0,topnum-SidesPerSlice:topnum);
    else
       {if (++scount==wsegs) {scount=0;c=topnum++;}
       }
  }
  return(c);
}

void AddFace(Face *f,int smooth_group,int tdelta,TVFace *tvface,int genUVs)
{ int a,b,c;
  f[0].setSmGroup(smooth_group);
  f[0].setMatID((MtlID)0); 	 /*default */
  a=topnum; 
  if (firstface)
  { b=curvertex;
	f[0].setVerts(a, b, c=getnextfirstvertex());
    f[0].setEdgeVisFlags(1,1,0);
  }
  else
  { if (((boxpos==topsquare)||(boxpos==bottomsquare))&&(++fcount==wsegs))
    {fcount=0;b=curvertex++;}
    else if (boxpos==messyedge?face==topchamferfaces+chamferstart-1:scount==SidesPerSlice-1)
       b=(boxpos==middle?curvertex-SidesPerSlice:cstartpt);
	else b=curvertex;
	f[0].setVerts(a, b, c=getnextsecondvertex());
    f[0].setEdgeVisFlags(0,1,1);
  }
  if (genUVs)
  { a+=tdelta;b+=tdelta;c+=tdelta;
	tvface[0].setTVerts(a,b,c);
  }
  face++;
}

BOOL ChBoxObject::HasUVW() { 
	BOOL genUVs;
	Interval v;
	pblock2->GetValue(chbox_mapping, 0, genUVs, v);
	return genUVs; 
	}

void ChBoxObject::SetGenUVW(BOOL sw) {  
	if (sw==HasUVW()) return;
	pblock2->SetValue(chbox_mapping,0, sw);
	}

void ChBoxObject::BuildMesh(TimeValue t)
	{
	int smooth,dsegs,vertices;
	int WLines,HLines,DLines,CLines,VertexPerSlice;
	int VertexPerFace,FacesPerSlice,chamferend;
	float usedw,usedd,usedh,cradius,zdelta,CurRadius;
	Point3 va,vb,p;
	float depth, width, height;
	int genUVs = 1,sqvertex,CircleLayers;
	BOOL bias = 0,minusd;

	// Start the validity interval at forever and widdle it down.
	ivalid = FOREVER;	
	pblock2->GetValue(chbox_length,t,height,ivalid);
	pblock2->GetValue(chbox_width,t,width,ivalid);
	pblock2->GetValue(chbox_height,t,depth,ivalid);
	minusd=depth<0.0f;
	depth=(float)fabs(depth);
	pblock2->GetValue(chbox_radius,t,cradius,ivalid);
	pblock2->GetValue(chbox_lsegs,t,hsegs,ivalid);
	pblock2->GetValue(chbox_wsegs,t,wsegs,ivalid);
	pblock2->GetValue(chbox_hsegs,t,dsegs,ivalid);
	pblock2->GetValue(chbox_csegs,t,csegs,ivalid);
	pblock2->GetValue(chbox_mapping,t,genUVs,ivalid);
	pblock2->GetValue(chbox_smooth,t,smooth,ivalid);
	
	LimitValue(csegs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(dsegs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(wsegs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(hsegs, MIN_SEGMENTS, MAX_SEGMENTS);

	smooth=(smooth>0?1:0);
	mesh.setSmoothFlags(smooth);
	float twocrad,usedm,mindim=(height>width?(width>depth?depth:width):(height>depth?depth:height));
	usedm=mindim-2*cradius;
	if (usedm<0.01f) cradius=(mindim-0.01f)/2.0f;
	twocrad=2.0f*cradius;
    usedh=height-twocrad;
    usedw=width-twocrad;
    usedd=depth-twocrad;
	float cangincr=PI/(2.0f*csegs),cudelta,udist;
    CircleLayers=csegs;
	cudelta=cradius*(float)sqrt(2.0f*(1.0f-(float)cos(cangincr)));
	udist=4.0f*csegs*cudelta+2.0f*width+2.0f*height-4.0f*cradius;
	chamferinfo[0].surface=1;chamferinfo[0].deltavert=1;
	chamferinfo[1].surface=2;chamferinfo[1].deltavert=1;
	chamferinfo[2].surface=1;chamferinfo[2].deltavert=-1;
	chamferinfo[3].surface=2;chamferinfo[3].deltavert=-1;
    WLines=wsegs-1;
    HLines=hsegs-1;
    DLines=dsegs-1;
    CLines=csegs+1;
    VertexPerSlice=2*(WLines+HLines)+4*CLines;
/* WLines*HLines on middle, 2*Clines*(WLines+HLines) on sides, 4*CLines*csegs+4 for circles */
    VertexPerFace=WLines*HLines+2*CLines*(WLines+HLines+2*csegs)+4;
    vertices=VertexPerFace*2+VertexPerSlice*DLines;
    sqvertex=(wsegs+1)*(hsegs+1);
/* 4 vertices, 2 faces/cseg + 2 each hseg & wseg sides, each seg w/ 2 faces*/
    SidesPerSlice=2*(2*csegs+hsegs+wsegs);
    FacesPerSlice=SidesPerSlice*2;
/* this one only has 1 face/ cseg */
    topchamferfaces=4*(csegs+hsegs+wsegs);
/*top chamfer + top face(2 faces/seg)(*2 for bottom) plus any depth faces*/
    maxfaces=2*(topchamferfaces+2*hsegs*wsegs)+(2*(CircleLayers-1)+dsegs)*FacesPerSlice;
    chamferstart=2*hsegs*wsegs;
    chamferend=chamferstart+topchamferfaces+(CircleLayers-1)*FacesPerSlice;
    chamferinfo[0].deltavert +=wsegs;
    chamferinfo[2].deltavert -=wsegs;
	int bottomvertex,vertexnum,tverts;
	int twomapped,endvert=vertices+(twomapped=2*VertexPerSlice);
	float xmax,ymax;
	mesh.setNumVerts(vertices);
	mesh.setNumFaces(maxfaces);
	tverts=endvert+DLines+2;
	if (genUVs)
	{ mesh.setNumTVerts(tverts);
	  mesh.setNumTVFaces(maxfaces);
	}
	else
	{ mesh.setNumTVerts(0);
	  mesh.setNumTVFaces(0);
	}
    zdelta=depth/2;
    wsegcount=0;vertexnum=0;
    bottomvertex=vertices-1;
    CornerPt.z=zdelta;
    CornerPt.x=(xmax=width/2)-cradius;
    CornerPt.y=(ymax=height/2)-cradius;
    NewPt.x=Toppt.x=-CornerPt.x;
    NewPt.y=Toppt.y=CornerPt.y;
    NewPt.z=Toppt.z=zdelta;
      /* Do top and bottom faces */
	hincr=usedh/hsegs;		//yincr
	wincr=usedw/wsegs;		//xincr
    BOOL usePhysUVs = GetUsePhysicalScaleUVs();

	int segcount,topvertex,tvcounter=0,tvbottom=endvert-1;
	float udiv=usePhysUVs ? 1.0f : 2.0f*xmax,vdiv=usePhysUVs ? 1.0f :2.0f*ymax, u = 0.0f, v = 0.0f;
	for (hseg=0;hseg<=hsegs;hseg++)
	{ if (hseg>0) 
	  {NewPt.y=(hseg==hsegs?-CornerPt.y:Toppt.y-hseg*hincr); NewPt.x=Toppt.x; }
	  for (segcount=0;segcount<=wsegs;segcount++)
	  { /* make top point */
	   NewPt.z=Toppt.z;
       NewPt.x=(segcount==wsegs?CornerPt.x:Toppt.x+segcount*wincr);
	   if (genUVs) 
		 mesh.setTVert(vertexnum,u=(xmax+NewPt.x)/udiv,v=(ymax+NewPt.y)/vdiv,0.0f);
	   mesh.setVert(vertexnum++,NewPt);
		/* make bottom pt */
       NewPt.z=-zdelta;
	   if (genUVs) 
	     mesh.setTVert(tvbottom--,u,(usePhysUVs ? 2.0f*ymax : 1.0f)-v,0.0f);
	   mesh.setVert(bottomvertex--,NewPt);
	  }
	}
    /* start on the chamfer */
	int layer,vert;
    layer=0;
    hseg=0;
	tvcounter=vertexnum;
    bottomvertex-=(VertexPerSlice-1);
    topvertex=vertexnum;
	BOOL done = FALSE,atedge = FALSE;
	float dincr=usedd/dsegs,cincr=2.0f*CircleLayers,RotationAngle;
	float dx,dy;
	int cornervert[4],PtRotation;
     for (layer=1;layer<=CircleLayers;layer++)	   /* add chamfer layer */
	 { if (layer==CircleLayers)	{zdelta=cradius;CurRadius=cradius;}
	   else
	   { RotationAngle=(PI*layer)/cincr;
	 	 zdelta=cradius-(cradius*(float)cos(RotationAngle));
		 CurRadius=cradius*(float)sin(RotationAngle);
	   }
	   zdelta=CornerPt.z-zdelta;
       atedge=(layer==CircleLayers);
	   int vfromedge=0,oldside=0,vfromstart=0;
	   sidenum=0;
	   float u1 = 0.0f, v1 = 0.0f;
	   BOOL atstart=TRUE;
       while (vertexnum<topvertex+csegs)	/* add vertex loop */
	   { PtRotation=hseg=wsegcount=0;done=FALSE;
         RotationAngle=(vertexnum-topvertex)*cangincr;
         curvertex=vert=vertexnum;
		 NewPt.x=Toppt.x-(dx=CurRadius*(float)sin(RotationAngle));
         NewPt.y=Toppt.y+(dy=CurRadius*(float)cos(RotationAngle));
         NewPt.z=zdelta;
		 while (!done)
		 { mesh.setVert(vert,NewPt);
		   if (genUVs) 
		    mesh.setTVert(vert,u1=(xmax+NewPt.x)/udiv,v1=(ymax+NewPt.y)/vdiv,0.0f);
           /* reflected vertex to second face */
           vert=bottomvertex+curvertex-topvertex;
           NewPt.z=-zdelta;
		   mesh.setVert(vert,NewPt);
		   if (genUVs)
		     mesh.setTVert(vert+twomapped,u1,(usePhysUVs ? 2.0f*ymax : 1.0f)-v1,0.0f);
           if ((atedge)&&(DLines>0))	 /* add non-corner points */
		    for (segcount=1;segcount<=DLines;segcount++)
		    { NewPt.z=zdelta-segcount*dincr;
		      mesh.setVert(vert=curvertex+VertexPerSlice*segcount,NewPt);
		    }
		   /* Rotate Pt */
		   done=PtRotation>5;
		   if (done == FALSE)
		   { if (vertexnum==topvertex) 
		     { FillinSquare(&PtRotation,cornervert,CurRadius);
		       if (curvertex==topvertex+VertexPerSlice-1) (PtRotation)=6;
		     }
			 else
				CalculateNewPt(dx,dy,&PtRotation,cornervert,vertexnum-topvertex);
		     vert=curvertex;
			 NewPt.z=zdelta;
		   }
		 }
	     vertexnum++;	   /* done rotation */
	   }
       vertexnum=topvertex +=VertexPerSlice;
       bottomvertex -=VertexPerSlice;  
	}
	float dfromedge=0.0f;
	int tvnum,j,i,chsegs=csegs+1,cwsegs=wsegs;
	if (genUVs)
	{ u=0.0f;
	  dfromedge=-cudelta;
	  tvnum=vertexnum;
	  vertexnum=topvertex-VertexPerSlice;
      float uDenom = usePhysUVs ? 1.0f : udist;
      float vScale = usePhysUVs ? usedd : 1.0f;
      float maxU = 0.0f;
	  for (j=0;j<2;j++)
	  {
     int gverts;
	  for (gverts=0;gverts<chsegs;gverts++)
	  {	dfromedge+=cudelta;
		mesh.setTVert(tvnum,u=dfromedge/uDenom,vScale,0.0f);
        if (u > maxU) maxU = u;
	    for (i=1;i<=dsegs;i++)
	     mesh.setTVert(tvnum+VertexPerSlice*i,u,vScale*(1.0f-(float)i/dsegs),0.0f);
		vertexnum++;
		tvnum++;
	  }
	  chsegs=csegs;
	  for (gverts=0;gverts<hsegs;gverts++)
	  { dfromedge+=(float)fabs(mesh.verts[vertexnum].y-mesh.verts[vertexnum-1].y);
		mesh.setTVert(tvnum,u=dfromedge/uDenom,vScale,0.0f);
        if (u > maxU) maxU = u;
	    for (i=1;i<=dsegs;i++)
	     mesh.setTVert(tvnum+VertexPerSlice*i,u,vScale*(1.0f-(float)i/dsegs),0.0f);
		vertexnum++;
		tvnum++;
	  }
	  for (gverts=0;gverts<csegs;gverts++)
	  {	dfromedge+=cudelta;
		mesh.setTVert(tvnum,u=dfromedge/uDenom,vScale,0.0f);
        if (u > maxU) maxU = u;
	    for (i=1;i<=dsegs;i++)
	     mesh.setTVert(tvnum+VertexPerSlice*i,u,vScale*(1.0f-(float)i/dsegs),0.0f);
		vertexnum++;
		tvnum++;
	  }
	  if (j==1) cwsegs--;
	  for (gverts=0;gverts<cwsegs;gverts++)
	  { dfromedge+=(float)fabs(mesh.verts[vertexnum].x-mesh.verts[vertexnum-1].x);
		mesh.setTVert(tvnum,u=dfromedge/uDenom,vScale,0.0f);
        if (u > maxU) maxU = u;
	    for (i=1;i<=dsegs;i++)
	     mesh.setTVert(tvnum+VertexPerSlice*i,u,vScale*(1.0f-(float)i/dsegs),0.0f);
		vertexnum++;
		tvnum++;
	  }
	  }
	  int lastvert=endvert;
      float uScale = usePhysUVs ? udist-4*cradius : 1.0f;
	  mesh.setTVert(lastvert++,uScale, vScale,0.0f);
	  for (j=1;j<dsegs;j++)
	    mesh.setTVert(lastvert++,uScale, vScale*(1.0f-(float)j/dsegs),0.0f);
	  mesh.setTVert(lastvert,uScale, 0.0f,0.0f);
	}
    /* all vertices calculated - Now specify faces*/
	 int tvdelta=0;
    sidenum=topnum=face=fcount=scount=circleseg=0;
    curvertex=wsegs+1;
    firstface=layer=1;
//	smooth=(csegs>1?1:0);
    tstartpt=cstartpt=endpt=0;
    boxpos=topsquare;cstartpt=chamferinfo[0].deltavert;
	AddFace(&mesh.faces[face],smooth,tvdelta,&mesh.tvFace[face],genUVs);
      while (face<chamferstart)   /* Do Square Layer */
	  { firstface=!firstface;
		AddFace(&mesh.faces[face],smooth,tvdelta,&mesh.tvFace[face],genUVs);
      }  
      boxpos=messyedge;firstface=1;
      topnum=tstartpt=0;
      cstartpt=curvertex=topnum+sqvertex;circleseg=1;
      endpt=hsegs*(wsegs+1);
      /* Do Chamfer */
	  while (face<chamferend)
      { AddFace(&mesh.faces[face],smooth,tvdelta,&mesh.tvFace[face],genUVs);
		if (circleseg==0) firstface=!firstface;
	  }
      fcount=scount=0;
      boxpos=middle;tvdelta+=VertexPerSlice;
     /*Do box sides */
	  int tpt,lastv=tverts-1;
	  BOOL inside=TRUE;
      while (face<maxfaces-chamferstart-topchamferfaces)
	  { tpt=face;
		AddFace(&mesh.faces[face],smooth,tvdelta,&mesh.tvFace[face],genUVs);
		if (genUVs && inside)
		{ if ((firstface)&&(mesh.tvFace[tpt].t[2]<mesh.tvFace[tpt].t[1]))
			mesh.tvFace[tpt].t[2]=endvert+1;
		  else if (mesh.tvFace[tpt].t[2]<mesh.tvFace[tpt].t[0])
		  { mesh.tvFace[tpt].t[1]=endvert+1;
			mesh.tvFace[tpt].t[2]=endvert;
			endvert++;
			inside=endvert<lastv;
			if (inside==FALSE) 
			  tvdelta+=VertexPerSlice;
		  }
		}	
		firstface=!firstface;
	  }
      /* back in chamfer */
      circleseg=2;firstface=0;
      boxpos=bottomedge;
      sidenum=0;tstartpt=topnum;
      cstartpt=curvertex=vertices-1;
      endpt=cstartpt-hsegs*chamferinfo[0].deltavert;
	  while (face<maxfaces-chamferstart)  /* Do Second Chamfer */
	  { AddFace(&mesh.faces[face],smooth,tvdelta,&mesh.tvFace[face],genUVs);
		if (circleseg==0) firstface=!firstface;
	  }
      boxpos=bottomsquare;
      curvertex=topnum;
      topnum=curvertex+chamferinfo[0].deltavert;
      firstface=1;fcount=0;
	  while (face<maxfaces)
	  { AddFace(&mesh.faces[face],smooth,tvdelta,&mesh.tvFace[face],genUVs);
        firstface=!firstface;
	  }
    float deltaz=(minusd?-depth/2.0f:depth/2.0f);
	for (i=0;i<vertices;i++)
	{ mesh.verts[i].z+=deltaz;
	}
	mesh.InvalidateTopologyCache();
	}

Object* ChBoxObject::ConvertToType(TimeValue t, Class_ID obtype)
	{
		return __super::ConvertToType(t,obtype);
	}

int ChBoxObject::CanConvertToType(Class_ID obtype)
	{
	if (obtype==triObjectClassID) 
	{
		return 1;
	} else {
		return __super::CanConvertToType(obtype);
		}
	}


class ChBoxObjCreateCallBack: public CreateMouseCallBack {
	ChBoxObject *ob;
	Point3 p0,p1;
	IPoint2 sp0, sp1,sp2;
	BOOL square;
	public:
		int proc( ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat );
		void SetObj(ChBoxObject *obj) { ob = obj; }
	};

int ChBoxObjCreateCallBack::proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat)
{
	if (!vpt || !vpt->IsAlive())
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	Point3 d;
	DWORD snapdim = SNAP_IN_3D;
	bool createCube = chbox_crtype_blk.GetInt(chbox_create_meth) == 1;
	if (msg == MOUSE_FREEMOVE)
	{
		vpt->SnapPreview(m, m, NULL, snapdim);
	}

	if (msg == MOUSE_POINT || msg == MOUSE_MOVE)
	{
		switch (point)
		{
		case 0:
			sp0 = m;
			ob->pblock2->SetValue(chbox_width, 0, 0.0f);
			ob->pblock2->SetValue(chbox_length, 0, 0.0f);
			ob->pblock2->SetValue(chbox_height, 0, 0.0f);
			ob->suspendSnap = TRUE;
			p0 = vpt->SnapPoint(m, m, NULL, snapdim);
			p1 = p0 + Point3(.01, .01, .01);
			mat.SetTrans(float(.5)*(p0 + p1));
#ifdef BOTTOMPIV
			{
				Point3 xyz = mat.GetTrans();
				xyz.z = p0.z;
				mat.SetTrans(xyz);
			}
#endif
			break;
		case 1:
			sp1 = m;
			p1 = vpt->SnapPoint(m, m, NULL, snapdim);
			p1.z = p0.z + (float).01;
			if (createCube || (flags&MOUSE_CTRL))
			{
				mat.SetTrans(p0);
			}
			else
			{
				mat.SetTrans(float(.5)*(p0 + p1));
#ifdef BOTTOMPIV 					
				Point3 xyz = mat.GetTrans();
				xyz.z = p0.z;
				mat.SetTrans(xyz);
#endif
			}
			d = p1 - p0;
			square = FALSE;
			if (createCube)
			{
				// Constrain to cube
				d.x = d.y = d.z = Length(d)*2.0f;
			}
			else
				if (flags&MOUSE_CTRL)
				{
					// Constrain to square base
					float len;
					if (fabs(d.x) > fabs(d.y)) len = d.x;
					else len = d.y;
					d.x = d.y = 2.0f * len;
					square = TRUE;
				}
			ob->pblock2->SetValue(chbox_width, 0, float(fabs(d.x)));
			ob->pblock2->SetValue(chbox_length, 0, float(fabs(d.y)));
			ob->pblock2->SetValue(chbox_height, 0, d.z);

			if (msg == MOUSE_POINT && createCube)
			{
				if (Length(sp1 - sp0) < 3) CREATE_ABORT;
			}
			else if (msg == MOUSE_POINT &&
				(Length(sp1 - sp0) < 3 ))
			{
				return CREATE_ABORT;
			}
			break;
		case 2:
			sp2 = m;
			if (!createCube)
			{
				p1.z = p0.z + vpt->SnapLength(vpt->GetCPDisp(p0, Point3(0, 0, 1), sp1, m, TRUE));
				d = p1 - p0;
				if (square)
				{ // Constrain to square base
					float len;
					if (fabs(d.x) > fabs(d.y)) len = d.x;
					else len = d.y;
					d.x = d.y = 2.0f * len;
				}
				ob->pblock2->SetValue(chbox_width, 0, float(fabs(d.x)));
				ob->pblock2->SetValue(chbox_length, 0, float(fabs(d.y)));
				ob->pblock2->SetValue(chbox_height, 0, d.z);
			}
			else
			{
				d.x = vpt->SnapLength(vpt->GetCPDisp(p1, Point3(0, 1, 0), sp1, m));
				if (d.x < 0.0f) d.x = 0.0f;
				ob->pblock2->SetValue(chbox_radius, 0, d.x);
				if (msg == MOUSE_POINT && createCube)
				{
					ob->suspendSnap = FALSE;
					return CREATE_STOP;
				}
			}
			break;
		case 3:
			d.x = vpt->SnapLength(vpt->GetCPDisp(p1, Point3(0, 1, 0), sp2, m));
			if (d.x < 0.0f) d.x = 0.0f;
			ob->pblock2->SetValue(chbox_radius, 0, d.x);
			if (msg == MOUSE_POINT)
			{
				ob->suspendSnap = FALSE;
				return CREATE_STOP;
			}
			break;
		}
	}
	else
		if (msg == MOUSE_ABORT)
		{
			return CREATE_ABORT;
		}

	return TRUE;
}

static ChBoxObjCreateCallBack chboxCreateCB;

CreateMouseCallBack* ChBoxObject::GetCreateMouseCallBack() {
	chboxCreateCB.SetObj(this);
	return(&chboxCreateCB);
}


BOOL ChBoxObject::OKtoDisplay(TimeValue t)
{
	/*
	float l, w, h;
	pblock->GetValue(PB_LENGTH,t,l,FOREVER);
	pblock->GetValue(PB_WIDTH,t,w,FOREVER);
	pblock->GetValue(PB_HEIGHT,t,h,FOREVER);
	if (l==0.0f || w==0.0f || h==0.0f) return FALSE;
	else return TRUE;
	*/
	return TRUE;
}

void ChBoxObject::InvalidateUI()
{
	chbox_param_blk.InvalidateUI(pblock2->LastNotifyParamID());
}

RefTargetHandle ChBoxObject::Clone(RemapDir& remap)
{
	ChBoxObject* newob = new ChBoxObject(FALSE);
	newob->ReplaceReference(0, remap.CloneRef(pblock2));
	newob->ivalid.SetEmpty();
	BaseClone(this, newob, remap);
	newob->SetUsePhysicalScaleUVs(GetUsePhysicalScaleUVs());
	return(newob);
}


void ChBoxObject::UpdateUI()
{
	if (ip == NULL)
		return;
	ChBoxParamDlgProc* dlg = static_cast<ChBoxParamDlgProc*>(chbox_param_blk.GetUserDlgProc());
	dlg->UpdateUI();
}

BOOL ChBoxObject::GetUsePhysicalScaleUVs()
{
    return ::GetUsePhysicalScaleUVs(this);
}


void ChBoxObject::SetUsePhysicalScaleUVs(BOOL flag)
{
    BOOL curState = GetUsePhysicalScaleUVs();
    if (curState == flag)
        return;
    if (theHold.Holding())
        theHold.Put(new RealWorldScaleRecord<ChBoxObject>(this, curState));
    ::SetUsePhysicalScaleUVs(this, flag);
    if (pblock2 != NULL)
		pblock2->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	UpdateUI();
	macroRec->SetProperty(this, _T("realWorldMapSize"), mr_bool, flag);
}

