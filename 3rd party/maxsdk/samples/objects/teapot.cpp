/*****************************************************************************
*<
FILE: teapot.cpp

DESCRIPTION:  Teapot object, Revised implementation

CREATED BY: Charles Thaeler

BASED ON : Sphere_C

HISTORY: created 12/4/95

*>	Copyright (c) 1994, All Rights Reserved.
*****************************************************************************/
#include "prim.h"
#include "iparamm2.h"
#include "simpobj.h"
#include "surf_api.h"
#include "tea_util.h"
#include "macroRec.h"
#include "RealWorldMapUtils.h"
#include <ReferenceSaveManager.h>
#include <Util/StaticAssert.h>


// The teapot object class definition.  It is derived from SimpleObject2.
// SimpleObject2 is the class to derive objects from which have 
// geometry are renderable, and represent themselves using a mesh and are pb2 based.
class TeapotObject : public SimpleObject2, public RealWorldMapSizeInterface  {
	friend class TeapotTypeInDlgProc;
	friend class TeapotObjCreateCallBack;
public:			
	// This is the Interface pointer into MAX.  It is used to call
	// functions implemented in MAX itself.  
	static IObjParam *ip;
	static bool typeinCreate;

	TeapotObject(BOOL loading);

	// From Object
	int CanConvertToType(Class_ID obtype) override;
	Object* ConvertToType(TimeValue t, Class_ID obtype) override;
	void GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist) override;
	BOOL HasUVW() override;
	void SetGenUVW(BOOL sw) override;

	// This method allows the plug-in to provide MAX with a procedure
	// to manage user input from the mouse during creation of the 
	// Teapot object.
	CreateMouseCallBack* GetCreateMouseCallBack() override;
	// This method is called when the user may edit the Teapots
	// parameters.
	void BeginEditParams( IObjParam  *ip, ULONG flags,Animatable *prev) override;
	// Called when the user is done editing the Teapots parameters.
	void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next) override;
	RefTargetHandle Clone(RemapDir& remap) override;
	// This is the name that appears in the history list (modifier stack).
	const TCHAR *GetObjectName() override { return GetString(IDS_RB_TEAPOT); }

	// From GeomObject 
	int IntersectRay(TimeValue t, Ray& ray, float& at, Point3& norm) override;

	// Animatable methods
	// Deletes the instance from memory.		
	void DeleteThis() override {delete this;}
	// This returns the unique ClassID of the Teapot procedural object.
	Class_ID ClassID() override {return Class_ID(TEAPOT_CLASS_ID1, TEAPOT_CLASS_ID2); }

	// From ReferenceTarget
	// Called by MAX when the teapot is being loaded from disk.
	IOResult Load(ILoad *iload) override;
	//Support of Save to Previous.
	bool SpecifySaveReferences(ReferenceSaveManager& referenceSaveManager) override;

	// From SimpleObjectBase
	// This method builds the mesh representation at the specified time.
	void BuildMesh(TimeValue t) override;
	// This method returns a flag to indicate if it is OK to 
	// display the object at the time passed.
	BOOL OKtoDisplay(TimeValue t) override;
	// This method informs the system that the user interface
	// controls need to be updated to reflect the current time.
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
private:
	class TeapartPLC;
};


// Misc stuff
#define MAX_SEGMENTS	64
#define MIN_SEGMENTS	1

#define MIN_RADIUS		float(0)
#define MAX_RADIUS		float(1.0E30)

#define MIN_SMOOTH		0
#define MAX_SMOOTH		2

#define DEF_SEGMENTS	8
#define DEF_RADIUS		float(0.0)

#define SMOOTH_ON		1
#define SMOOTH_OFF		0

#define POT_BODY		1
#define POT_HANDLE		1
#define POT_SPOUT		1
#define POT_LID			1
#define POT_UVS			1
#define OLD_POT_BOTH    0
#define OLD_POT_BODY    1
#define OLD_POT_LID     2
#define FIXED_OLD_VER   -1


// See the Advanced Topics section on DLL Functions and Class Descriptors
// for more information.
/*===========================================================================*\
| The Class Descriptor
\*===========================================================================*/

static BOOL sInterfaceAdded = FALSE;

class TeapotClassDesc :public ClassDesc2 {
public:
	// The IsPublic() method should return TRUE if the plug-in can be picked
	// and assigned by the user. Some plug-ins may be used privately by other
	// plug-ins implemented in the same DLL and should not appear in lists for
	// user to choose from, so these plug-ins would return FALSE.
	int 			IsPublic() { return 1; }
	// This is the method that actually creates a new instance of
	// a plug-in class.  By having the system call this method,
	// the plug-in may use any memory manager it wishes to 
	// allocate its objects.  The system calls the correspoding 
	// DeleteThis() method of the plug-in to free the memory.  Our 
	// implementations use 'new' and 'delete'.
	void *			Create(BOOL loading = FALSE)
	{
		if (!sInterfaceAdded) {
			AddInterface(&gRealWorldMapSizeDesc);
			sInterfaceAdded = TRUE;
		}
		return new TeapotObject(loading);
	}
	// This is used for debugging purposes to give the class a 
	// displayable name.  It is also the name that appears on the button
	// in the MAX user interface.
	const TCHAR *	ClassName() override { return GetString(IDS_RB_TEAPOT_CLASS); }
	// The system calls this method at startup to determine the type of object
	// this is.  In our case, we're a geometric object so we return 
	// GEOMOBJECT_CLASS_ID.  The possible options are defined in PLUGAPI.H
	SClass_ID		SuperClassID() override { return GEOMOBJECT_CLASS_ID; }
	// The system calls this method to retrieve the unique
	// class id for this object.
	Class_ID		ClassID() override { return Class_ID(TEAPOT_CLASS_ID1, TEAPOT_CLASS_ID2); }
	// The category is selected
	// in the bottom most drop down list in the create branch.
	// If this is set to be an exiting category (i.e. "Primitives", ...) then
	// the plug-in will appear in that category. If the category doesn't
	// yet exists then it is created.  We use the new How To category for
	// all the example plug-ins in the How To sections.
	const TCHAR* 	Category() override { return GetString(IDS_RB_PRIMITIVES); }
	// Returns fixed parsable name (scripter-visible name)
	const TCHAR* InternalName() { return _T("Teapot"); } 
	// Returns owning module handle
	HINSTANCE  HInstance() { return hInstance; }
};

// Declare a static instance of the class descriptor.
static TeapotClassDesc teapotDesc;
// This function returns the address of the descriptor.  We call it from 
// the LibClassDesc() function, which is called by the system when loading
// the DLLs at startup.
ClassDesc* GetTeapotDesc() { return &teapotDesc; }

// in prim.cpp  - The dll instance handle
extern HINSTANCE hInstance;

IObjParam *TeapotObject::ip = NULL;
bool TeapotObject::typeinCreate = false;

#define PBLOCK_REF_NO  0

// ParamBlockDesc2 IDs
enum paramblockdesc2_ids { teapot_creation_type, teapot_type_in, teapot_params, };
enum teapot_creation_type_param_ids { teapot_create_meth, };
enum teapot_type_in_param_ids { teapot_ti_pos, teapot_ti_radius, };
enum teapot_param_param_ids {
	teapot_radius = TEAPOT_RADIUS, teapot_segs = TEAPOT_SEGS, teapot_smooth = TEAPOT_SMOOTH,
	teapot_teapart = TEAPOT_TEAPART,  // Obsolete but needed for versioning
	teapot_body = TEAPOT_BODY, teapot_handle = TEAPOT_HANDLE, 
	teapot_spout = TEAPOT_SPOUT, teapot_lid = TEAPOT_LID, teapot_mapping = TEAPOT_GENUVS,
};

namespace
{
	MaxSDK::Util::StaticAssert< (teapot_params == TEAPOT_PARAMBLOCK_ID) > validator;
}

namespace
{
	class MapCoords_Accessor : public PBAccessor
	{
		void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
		{
			if (owner) ((TeapotObject*)owner)->UpdateUI();
		}
	};
	static MapCoords_Accessor mapCoords_Accessor;
}

// class creation type block
static ParamBlockDesc2 teapot_crtype_blk(teapot_creation_type, _T("TeapotCreationType"), 0, &teapotDesc, P_CLASS_PARAMS + P_AUTO_UI,
	//rollout
	IDD_TEAPOTPARAM1, IDS_RB_CREATIONMETHOD, BEGIN_EDIT_CREATE, 0, NULL,
	// params
	teapot_create_meth, _T("typeinCreationMethod"), TYPE_INT, 0, IDS_RB_CREATIONMETHOD,
	p_default, 1,
	p_range, 0, 1,
	p_ui, TYPE_RADIO, 2, IDC_CREATEDIAMETER, IDC_CREATERADIUS,
	p_end,
	p_end
	);

// class type-in block
static ParamBlockDesc2 teapot_typein_blk(teapot_type_in, _T("TeapotTypeIn"), 0, &teapotDesc, P_CLASS_PARAMS + P_AUTO_UI,
	//rollout
	IDD_TEAPOTPARAM3, IDS_RB_KEYBOARDENTRY, BEGIN_EDIT_CREATE, APPENDROLL_CLOSED, NULL,
	// params
	teapot_ti_pos, _T("typeInPos"), TYPE_POINT3, 0, IDS_RB_TYPEIN_POS,
	p_default, Point3(0, 0, 0),
	p_range, float(-1.0E30), float(1.0E30),
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_TI_POSX, IDC_TI_POSXSPIN, IDC_TI_POSY, IDC_TI_POSYSPIN, IDC_TI_POSZ, IDC_TI_POSZSPIN, SPIN_AUTOSCALE,
	p_end,
	teapot_ti_radius, _T("typeInRadius"), TYPE_FLOAT, 0, IDS_RB_RADIUS,
	p_default, DEF_RADIUS,
	p_range, MIN_RADIUS, MAX_RADIUS,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_RADIUS, IDC_RADSPINNER, SPIN_AUTOSCALE,
	p_end,
	p_end
	);

// per instance teapot block
static ParamBlockDesc2 teapot_param_blk(teapot_params, _T("TeapotParameters"), 0, &teapotDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_REF_NO,
	//rollout
	IDD_TEAPOTPARAM2, IDS_RB_PARAMETERS, 0, 0, NULL,
	// params
	teapot_radius, _T("radius"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_RADIUS,
	p_default, DEF_RADIUS,
	p_ms_default, 25.0,
	p_range, MIN_RADIUS, MAX_RADIUS,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_RADIUS, IDC_RADSPINNER, SPIN_AUTOSCALE,
	p_end,
	teapot_segs, _T("segs"), TYPE_INT, P_ANIMATABLE, IDS_RB_SEGS,
	p_default, DEF_SEGMENTS,
	p_ms_default, 4,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_SEGMENTS, IDC_SEGSPINNER, 0.1f,
	p_end,
	teapot_smooth, _T("smooth"), TYPE_BOOL, P_ANIMATABLE, IDS_RB_SMOOTH,
	p_default, TEAPOT_SMOOTH,
	p_ui, TYPE_SINGLECHECKBOX, IDC_OBSMOOTH,
	p_end,
	teapot_teapart, _T(""), TYPE_INT, 0, 0,
	p_default, FIXED_OLD_VER,
	p_end,
	teapot_body, _T("body"), TYPE_BOOL, P_ANIMATABLE, IDS_RB_BODY,
	p_default, POT_BODY,
	p_ui, TYPE_SINGLECHECKBOX, IDC_TEA_BODY,
	p_end,
	teapot_handle, _T("handle"), TYPE_BOOL, P_ANIMATABLE, IDS_RB_HANDLE,
	p_default, POT_HANDLE,
	p_ui, TYPE_SINGLECHECKBOX, IDC_TEA_HANDLE,
	p_end,
	teapot_spout, _T("spout"), TYPE_BOOL, P_ANIMATABLE, IDS_RB_SPOUT,
	p_default, POT_SPOUT,
	p_ui, TYPE_SINGLECHECKBOX, IDC_TEA_SPOUT,
	p_end,
	teapot_lid, _T("lid"), TYPE_BOOL, P_ANIMATABLE, IDS_RB_LID,
	p_default, POT_LID,
	p_ui, TYPE_SINGLECHECKBOX, IDC_TEA_LID,
	p_end,
	teapot_mapping, _T("mapCoords"), TYPE_BOOL, 0, IDS_RB_GENTEXCOORDS,
	p_default, TRUE,
	p_ms_default, FALSE,
	p_ui, TYPE_SINGLECHECKBOX, IDC_GENTEXTURE,
	p_accessor, &mapCoords_Accessor,
	p_end,
	p_end
	);

// The parameter block descriptor defines the type of value represented
// by the parameter (int, float, Color...) and if it is animated or not.

// This class requires these values to be initialized:
// { - ParameterType, 
//   - Not Used, must be set to NULL, 
//   - Flag which indicates if the parameter is animatable,
//   - ID of the parameter used to match a corresponding ID in the 
//     other version of the parameter
//  }

// This one is the oldest version.
static ParamBlockDescID descVer0[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0, teapot_radius },
	{ TYPE_INT, NULL, TRUE, 1, teapot_segs },
	{ TYPE_INT, NULL, TRUE, 2, teapot_smooth } };

// This is the older version.
static ParamBlockDescID descVer1[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0, teapot_radius },
	{ TYPE_INT, NULL, TRUE, 1, teapot_segs },
	{ TYPE_INT, NULL, TRUE, 2, teapot_smooth },
	{ TYPE_INT, NULL, TRUE, 3, teapot_teapart } };

// This is the current version.
static ParamBlockDescID descVer2[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0, teapot_radius },
	{ TYPE_INT, NULL, TRUE, 1, teapot_segs },
	{ TYPE_INT, NULL, TRUE, 2, teapot_smooth },
	{ TYPE_INT, NULL, FALSE, 3, teapot_teapart },
	{ TYPE_BOOL, NULL, TRUE, 4, teapot_body },
	{ TYPE_BOOL, NULL, TRUE, 5, teapot_handle },
	{ TYPE_BOOL, NULL, TRUE, 6, teapot_spout },
	{ TYPE_BOOL, NULL, TRUE, 7, teapot_lid },
	{ TYPE_BOOL, NULL, FALSE, 8, teapot_mapping } };

// Array of old versions
static ParamVersionDesc versions[] = {
	ParamVersionDesc(descVer0,3,1),
	ParamVersionDesc(descVer1,4,2),
	ParamVersionDesc(descVer2,9,3)
};
#define NUM_OLDVERSIONS	3

// ParamBlock data for SaveToPrevious support
#define PBLOCK_LENGTH	9
#define CURRENT_VERSION	3

//--- TypeInDlgProc --------------------------------

class TeapotTypeInDlgProc : public ParamMap2UserDlgProc {
public:
	TeapotObject *so;

	TeapotTypeInDlgProc(TeapotObject *s) {so=s;}
	INT_PTR DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,
		WPARAM wParam,LPARAM lParam) override;
	void DeleteThis() override {delete this;}
};

// This is the method called when the user clicks on the Create button
// in the Keyboard Entry rollup.  It was registered as the dialog proc
// for this button by the SetUserDlgProc() method called from 
// BeginEditParams().
INT_PTR TeapotTypeInDlgProc::DlgProc(
	TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg,
	WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_TI_CREATE: {
			if (teapot_typein_blk.GetFloat(teapot_ti_radius) == 0.0) return TRUE;

			// We only want to set the value if the object is 
			// not in the scene.
			if (so->TestAFlag(A_OBJ_CREATING)) {
				so->pblock2->SetValue(teapot_radius, 0, teapot_typein_blk.GetFloat(teapot_ti_radius));
			}
			else
				TeapotObject::typeinCreate = true;

			Matrix3 tm(1);
			tm.SetTrans(teapot_typein_blk.GetPoint3(teapot_ti_pos));
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


class TeapotParamDlgProc : public ParamMap2UserDlgProc {
public:
	TeapotObject *mpTeapotObj;
	HWND mhWnd;
	TeapotParamDlgProc(TeapotObject *o) {mpTeapotObj=o;}
	INT_PTR DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam) override;
	void DeleteThis() override {delete this;}
	void UpdateUI();
	BOOL GetRWSState();
};

BOOL TeapotParamDlgProc::GetRWSState()
{
	BOOL check = IsDlgButtonChecked(mhWnd, IDC_REAL_WORLD_MAP_SIZE);
	return check;
}

void TeapotParamDlgProc::UpdateUI()
{
	BOOL usePhysUVs = mpTeapotObj->GetUsePhysicalScaleUVs();
	CheckDlgButton(mhWnd, IDC_REAL_WORLD_MAP_SIZE, usePhysUVs);

	EnableWindow(GetDlgItem(mhWnd, IDC_REAL_WORLD_MAP_SIZE), mpTeapotObj->HasUVW());
}

INT_PTR TeapotParamDlgProc::DlgProc(
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
			mpTeapotObj->SetUsePhysicalScaleUVs(check);
			theHold.Accept(GetString(IDS_DS_PARAMCHG));
			mpTeapotObj->ip->RedrawViews(mpTeapotObj->ip->GetTime());
			break;
									  }

		}
		break;

	}
	return FALSE;
}

//--- Teapot methods -------------------------------

// Constructor
TeapotObject::TeapotObject(BOOL loading)
{
	// Create the parameter block and make a reference to it.
	teapotDesc.MakeAutoParamBlocks(this);
	if (!loading && !GetPhysicalScaleUVsDisabled())
		SetUsePhysicalScaleUVs(true);
}

bool TeapotObject::SpecifySaveReferences(ReferenceSaveManager& referenceSaveManager)
{
	// if saving to previous version that used pb1 instead of pb2...
	DWORD saveVersion = GetSavingVersion();
	if (saveVersion != 0 && saveVersion <= MAX_RELEASE_R19)
	{
		ProcessPB2ToPB1SaveToPrevious(this, pblock2, PBLOCK_REF_NO, descVer2, PBLOCK_LENGTH, CURRENT_VERSION);
	}
	return __super::SpecifySaveReferences(referenceSaveManager);
}

//------------------------------------------------------------------------------
// PostLoadCallback
//------------------------------------------------------------------------------
class TeapotObject::TeapartPLC : public PostLoadCallback
{
public:
	TeapartPLC(TeapotObject& aTeapot) : mTeapot(aTeapot) { }
	void proc(ILoad *iload)
	{
		int body, handle, spout, lid, oldpart;
		mTeapot.pblock2->GetValue(teapot_teapart, 0, oldpart, FOREVER);
		switch (oldpart) {
		case OLD_POT_BOTH:
			body = handle = spout = lid = 1;
			mTeapot.pblock2->SetValue(teapot_teapart, 0, FIXED_OLD_VER);
			mTeapot.pblock2->SetValue(teapot_body, 0, body);
			mTeapot.pblock2->SetValue(teapot_handle, 0, handle);
			mTeapot.pblock2->SetValue(teapot_spout, 0, spout);
			mTeapot.pblock2->SetValue(teapot_lid, 0, lid);
			break;
		case OLD_POT_BODY:
			body = handle = spout = 1;
			lid = 0;
			mTeapot.pblock2->SetValue(teapot_teapart, 0, FIXED_OLD_VER);
			mTeapot.pblock2->SetValue(teapot_body, 0, body);
			mTeapot.pblock2->SetValue(teapot_handle, 0, handle);
			mTeapot.pblock2->SetValue(teapot_spout, 0, spout);
			mTeapot.pblock2->SetValue(teapot_lid, 0, lid);
			break;
		case OLD_POT_LID:
			body = handle = spout = 0;
			lid = 1;
			mTeapot.pblock2->SetValue(teapot_teapart, 0, FIXED_OLD_VER);
			mTeapot.pblock2->SetValue(teapot_body, 0, body);
			mTeapot.pblock2->SetValue(teapot_handle, 0, handle);
			mTeapot.pblock2->SetValue(teapot_spout, 0, spout);
			mTeapot.pblock2->SetValue(teapot_lid, 0, lid);
			break;
		case FIXED_OLD_VER:
			break;
		}
		delete this;
	}
private:
	TeapotObject& mTeapot;
};

// Called by MAX when the Teapot object is loaded from disk.
IOResult TeapotObject::Load(ILoad *iload) 
{	
	// This is the callback that corrects for any older versions
	// of the parameter block structure found in the MAX file 
	// being loaded.
	ParamBlock2PLCB* plcb = new ParamBlock2PLCB(versions, NUM_OLDVERSIONS, &teapot_param_blk, this, PBLOCK_REF_NO);
	iload->RegisterPostLoadCallback(plcb);
	TeapartPLC* plcb2 = new TeapartPLC(*this);
	iload->RegisterPostLoadCallback(plcb2);
	return IO_OK;
}

// This method is called by the system when the user needs 
// to edit the objects parameters in the command panel.  
void TeapotObject::BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev)
{
	// We subclass off SimpleObject2 so we must call its
	// BeginEditParams() method first.
	__super::BeginEditParams(ip,flags,prev);
	// Save the interface pointer.
	this->ip = ip;
	// If this has been freshly created by type-in, set creation values:
	if (TeapotObject::typeinCreate)
	{
		pblock2->SetValue(teapot_radius, 0, teapot_typein_blk.GetFloat(teapot_ti_radius));
		TeapotObject::typeinCreate = false;
	}

	// throw up all the appropriate auto-rollouts
	teapotDesc.BeginEditParams(ip, this, flags, prev);
	// if in Create Panel, install a callback for the type in.
	if (flags & BEGIN_EDIT_CREATE)
		teapot_typein_blk.SetUserDlgProc(new TeapotTypeInDlgProc(this));
	// install a callback for the params.
	teapot_param_blk.SetUserDlgProc(new TeapotParamDlgProc(this));
}

// This is called by the system to terminate the editing of the
// parameters in the command panel.  
void TeapotObject::EndEditParams( IObjParam *ip, ULONG flags,Animatable *next )
{		
	__super::EndEditParams(ip,flags,next);
	this->ip = NULL;
	teapotDesc.EndEditParams(ip, this, flags, next);
}


static void blend_vector(Teapoint d0,Teapoint d1,Teapoint d2,Teapoint d3,
						 float t, Teapoint *result);

static int
	GetShare(TeaShare shares[], int patch, int edge, int vert)
{
	int *verts = NULL;

	switch (edge) {
	case 0:
		verts = shares[patch].left;
		break;
	case 1:
		verts = shares[patch].top;
		break;
	case 2:
		verts = shares[patch].right;
		break;
	case 3:
		verts = shares[patch].bottom;
		break;
	}
	return verts[vert];
}

static void
	display_curve(int * patch_array, TeaShare shares[],
	Teapoint d0, Teapoint d1, Teapoint d2, Teapoint d3, int steps,
	Mesh& amesh, int *nvert, int patch, int row)
{
	float    t,		/* t varies on 0.0 -> 1.0 */
		step;
	int      i;
	Teapoint temp;
	TeaEdges *edge = &edges[patch];

	step = (float)1.0 / steps;
	t = step;

	if (row == 0) {
		switch (edge->first) {
		case GenAll:				// Generate all
		case GenSingularityBegin:	// in row 0 all these are the same
			/* the first point IS d0 so we don't need to calculate it */
			amesh.setVert(*nvert, d0.x, d0.y, d0.z);
			patch_array[row * (steps + 1)] = *nvert;
			(*nvert)++;

			/* no interior vertices are shared */
			for (i = 1; i < steps; i++) {
				blend_vector(d0, d1, d2, d3, t, &temp);
				amesh.setVert(*nvert, temp.x, temp.y, temp.z);
				patch_array[row * (steps + 1) + i] = *nvert;
				(*nvert)++;
				t += step;
			}

			/* the last vertex IS d3 so we don't need to calculate it */
			amesh.setVert(*nvert, d3.x, d3.y, d3.z);
			patch_array[row * (steps + 1) + steps] = *nvert;
			(*nvert)++;
			break;
		case ShareBegin:
			patch_array[row * (steps + 1)] = GetShare(shares, edge->patch0, edge->edge0, steps);

			/* no interior vertices are shared */
			for (i = 1; i < steps; i++) {
				blend_vector(d0, d1, d2, d3, t, &temp);
				amesh.setVert(*nvert, temp.x, temp.y, temp.z);
				patch_array[row * (steps + 1) + i] = *nvert;
				(*nvert)++;
				t += step;
			}

			/* the last vertex IS d3 so we don't need to calculate it */
			amesh.setVert(*nvert, d3.x, d3.y, d3.z);
			patch_array[row * (steps + 1) + steps] = *nvert;
			(*nvert)++;
			break;
		case ShareAll:  // This should never happen with the current data
			for (i = 0; i <= steps; i++)
				patch_array[row * (steps + 1) + i] = GetShare(shares, edge->patch0, edge->edge0, i);
			break;
		default:
			assert(0);
			break;
		}
	} else {
		EdgeType et;
		if (row < steps)
			et = edge->center;
		else
			et = edge->last;
		switch (et) {
		case GenAll:				// Generate all
			/* the first point IS d0 so we don't need to calculate it */
			amesh.setVert(*nvert, d0.x, d0.y, d0.z);
			patch_array[row * (steps + 1)] = *nvert;
			(*nvert)++;

			/* no interior vertices are shared */
			for (i = 1; i < steps; i++) {
				blend_vector(d0, d1, d2, d3, t, &temp);
				amesh.setVert(*nvert, temp.x, temp.y, temp.z);
				patch_array[row * (steps + 1) + i] = *nvert;
				(*nvert)++;
				t += step;
			}

			/* the last vertex IS d3 so we don't need to calculate it */
			amesh.setVert(*nvert, d3.x, d3.y, d3.z);
			patch_array[row * (steps + 1) + steps] = *nvert;
			(*nvert)++;
			break;

		case GenSingularityBegin:	// generate all vertices except the first which
			// is a singularity from the first row
			patch_array[row * (steps + 1)] = patch_array[0];

			/* no interior vertices are shared */
			for (i = 1; i < steps; i++) {
				blend_vector(d0, d1, d2, d3, t, &temp);
				amesh.setVert(*nvert, temp.x, temp.y, temp.z);
				patch_array[row * (steps + 1) + i] = *nvert;
				(*nvert)++;
				t += step;
			}

			/* the last vertex IS d3 so we don't need to calculate it */
			amesh.setVert(*nvert, d3.x, d3.y, d3.z);
			patch_array[row * (steps + 1) + steps] = *nvert;
			(*nvert)++;
			break;
		case ShareAll:			/* this should only happen as the last row */
			for (i = 0; i <= steps; i++)
				patch_array[row * (steps + 1) + i] = GetShare(shares, edge->patch2, edge->edge2, i);
			break;
		case ShareBegin:
			patch_array[row * (steps + 1)] = GetShare(shares, edge->patch3, edge->edge3, row);

			/* no interior vertices are shared */
			for (i = 1; i < steps; i++) {
				blend_vector(d0, d1, d2, d3, t, &temp);
				amesh.setVert(*nvert, temp.x, temp.y, temp.z);
				patch_array[row * (steps + 1) + i] = *nvert;
				(*nvert)++;
				t += step;
			}

			/* the last vertex IS d3 so we don't need to calculate it */
			amesh.setVert(*nvert, d3.x, d3.y, d3.z);
			patch_array[row * (steps + 1) + steps] = *nvert;
			(*nvert)++;
			break;
		case ShareBeginSingularityEnd:
			patch_array[row * (steps + 1)] = GetShare(shares, edge->patch3, edge->edge3, row);

			/* no interior vertices are shared */
			for (i = 1; i < steps; i++) {
				blend_vector(d0, d1, d2, d3, t, &temp);
				amesh.setVert(*nvert, temp.x, temp.y, temp.z);
				patch_array[row * (steps + 1) + i] = *nvert;
				(*nvert)++;
				t += step;
			}

			patch_array[row * (steps + 1) + steps] = patch_array[steps];
			break;
		default:
			assert(0);
			break;
		}
	}
}


static void
	blend_vector(Teapoint d0,Teapoint d1,Teapoint d2,Teapoint d3, float t,
	Teapoint *result)
{
	result->x = d0.x * (1.0f - t)*(1.0f - t)*(1.0f - t) +
		d1.x * 3.0f * t * (1.0f - t)*(1.0f - t) +
		d2.x * 3.0f * t*t          *(1.0f - t) +
		d3.x *        t*t*t;
	result->y = d0.y * (1.0f - t)*(1.0f - t)*(1.0f - t) +
		d1.y * 3.0f * t * (1.0f - t)*(1.0f - t) +
		d2.y * 3.0f * t*t           *(1.0f - t) +
		d3.y *        t*t*t;
	result->z = d0.z * (1.0f - t)*(1.0f - t)*(1.0f - t) +
		d1.z * 3.0f * t * (1.0f - t)*(1.0f - t) +
		d2.z * 3.0f * t*t           *(1.0f - t) +
		d3.z *        t*t*t;
}


static void
	display_patch(TeaShare shares[], int patch, int steps, Mesh& amesh, int *nvert, int *nface,
	int smooth, int genUVs, int *uv_vert, int *uv_face, float uScale, float vScale)
{
	float    t,step;
	Teapoint    d0,d1,d2,d3;
	int start_vert, *patch_array;

	patch_array = (int *)malloc(sizeof(int) * (steps + 1) * (steps + 1));

	step = 1.0f / steps;
	t = 0.0f;
	start_vert = *nvert;

	for (int i = 0; i <= steps; i++) {
		blend_vector(verts[patches[patch][0][0]],
			verts[patches[patch][0][1]],
			verts[patches[patch][0][2]],
			verts[patches[patch][0][3]],t,&d0);    
		blend_vector(verts[patches[patch][1][0]],
			verts[patches[patch][1][1]],
			verts[patches[patch][1][2]],
			verts[patches[patch][1][3]],t,&d1);    
		blend_vector(verts[patches[patch][2][0]],
			verts[patches[patch][2][1]],
			verts[patches[patch][2][2]],
			verts[patches[patch][2][3]],t,&d2);    
		blend_vector(verts[patches[patch][3][0]],
			verts[patches[patch][3][1]],
			verts[patches[patch][3][2]],
			verts[patches[patch][3][3]],t,&d3);    
		display_curve(patch_array, shares, d0,d1,d2,d3,steps, amesh, nvert, patch, i);

		t += step;
	}

	/* now that we have generated all the vertices we can save the edges */
	for (int i = 0; i <= steps; i++) {
		shares[patch].left[i] = patch_array[i];					/* "left" edge */
		shares[patch].top[i] = patch_array[i*(steps+1)+steps];	/* "top" edge */
		shares[patch].right[i] = patch_array[steps*(steps+1)+i];	/* "right" edge */
		shares[patch].bottom[i] = patch_array[i*(steps+1)];		/* "bottom" edge */
	}

	/* now it's time to add the faces */
	for (int x = 0; x < steps; x++) {
		for (int y = 0; y < steps; y++) {
			int va, vb, vc, vd;

			va = patch_array[x * (steps + 1) + y];
			vb = patch_array[(x+1) * (steps + 1) + y];

			vd = patch_array[x * (steps + 1) + (y+1)];
			vc = patch_array[(x+1) * (steps + 1) + (y+1)];

#ifdef NO_DEGENERATE_POLYS
			if (va != vb && va != vc && vc != vb) {
#endif                
				amesh.faces[*nface].setEdgeVisFlags(1, 1, 0);
				amesh.faces[*nface].setVerts(va, vb, vc);
				amesh.faces[*nface].setSmGroup(smooth);
				(*nface)++;
#ifdef NO_DEGENERATE_POLYS
			}
#endif

#ifdef NO_DEGENERATE_POLYS
			if (vc != vd && vc != va && va != vd) {
#endif
				amesh.faces[*nface].setEdgeVisFlags(1, 1, 0);
				amesh.faces[*nface].setVerts(vc, vd, va);
				amesh.faces[*nface].setSmGroup(smooth);
				(*nface)++;
#ifdef NO_DEGENERATE_POLYS
			}
#endif
		}
	}


	// Now the UVs
	if (genUVs) {
		int base_vert = *uv_vert;
		float u, v,
			bU = edges[patch].bU, eU = edges[patch].eU,
			bV = edges[patch].bV, eV = edges[patch].eV,
			dU = (eU - bU)/float(steps),
			dV = (eV - bV)/float(steps);
		u = bU;
		for (int x = 0; x <= steps; x++) {
			v = bV;
			for (int y = 0; y <= steps; y++) {
				amesh.setTVert((*uv_vert)++, uScale*u, vScale*v, 0.0f);
				v += dV;
			}
			u += dU;
		}
		int na, nb;
		for(int ix=0; ix<steps; ix++) {
			na = base_vert + (ix * (steps + 1));
			nb = base_vert + (ix + 1) * (steps + 1);

			for (int jx=0; jx<steps; jx++) {
				int vva = patch_array[ix * (steps + 1) + jx];
				int vvb = patch_array[(ix+1) * (steps + 1) + jx];

				int vvd = patch_array[ix * (steps + 1) + (jx+1)];
				int vvc = patch_array[(ix+1) * (steps + 1) + (jx+1)];

#ifdef NO_DEGENERATE_POLYS
				if (vva != vvb && vva != vvc && vvc != vvb)
#endif
					amesh.tvFace[(*uv_face)++].setTVerts(na,nb,nb+1);
#ifdef NO_DEGENERATE_POLYS
				if (vvc != vvd && vvc != vva && vva != vvd)
#endif
					amesh.tvFace[(*uv_face)++].setTVerts(nb+1,na+1,na);
				na++;
				nb++;
			}
		}
	}

	free(patch_array);
}


BOOL TeapotObject::HasUVW() { 
	BOOL genUVs;
	Interval v;
	pblock2->GetValue(teapot_mapping, 0, genUVs, v);
	return genUVs; 
}

void TeapotObject::SetGenUVW(BOOL sw) {  
	if (sw==HasUVW()) return;
	pblock2->SetValue(teapot_mapping,0, sw);
	UpdateUI();
}

// Builds the mesh representation for the Teapot based on the
// state of it's parameters at the time requested.
void TeapotObject::BuildMesh(TimeValue t)
{

	int segs, smooth,
		body, handle, spout, lid, genUVs,
		cverts = 0, nvert = 0,
		cfaces = 0, nface = 0,
		uv_verts = 0, nuv_vert = 0,
		uv_faces = 0, nuv_face = 0;
	float radius;

	// Start the validity interval at forever and whittle it down.
	ivalid = FOREVER;
	pblock2->GetValue(teapot_radius,	t, radius,	ivalid);
	pblock2->GetValue(teapot_segs,		t, segs,	ivalid);
	pblock2->GetValue(teapot_smooth,	t, smooth,	ivalid);
	pblock2->GetValue(teapot_body,		t, body,	ivalid);
	pblock2->GetValue(teapot_handle,	t, handle,	ivalid);
	pblock2->GetValue(teapot_spout,		t, spout,	ivalid);
	pblock2->GetValue(teapot_lid,		t, lid,		ivalid);
	pblock2->GetValue(teapot_mapping,	t, genUVs,	ivalid);

	LimitValue(segs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(smooth, MIN_SMOOTH, MAX_SMOOTH);
	LimitValue(radius, MIN_RADIUS, MAX_RADIUS);
	LimitValue(body, 0, 1);
	LimitValue(handle, 0, 1);
	LimitValue(spout, 0, 1);
	LimitValue(lid, 0, 1);
	LimitValue(genUVs, 0, 1);

	//	int *patch_array = (int *)malloc(sizeof(int) * (segs + 1) * (segs + 1));

	TeaShare shares[PATCH_COUNT]; /* make an array to hold sharing data */
	for (int index = 0; index < PATCH_COUNT; index++) {
		shares[index].left =   (int *)malloc(sizeof(int) * (segs + 1));
		shares[index].top =    (int *)malloc(sizeof(int) * (segs + 1));
		shares[index].right =  (int *)malloc(sizeof(int) * (segs + 1));
		shares[index].bottom = (int *)malloc(sizeof(int) * (segs + 1));
	}

	if (body) {
		cverts += (segs - 1) * (segs - 1) * 16 +
			segs * 16 + (segs - 1) * 16 + 1;
#ifdef NO_DEGENERATE_POLYS
		cfaces += segs * segs * BODY_PATCHES * 2 - 4 * segs;
#else
		cfaces += segs * segs * BODY_PATCHES * 2;
#endif
		if (genUVs) {
			uv_verts += (segs + 1) * (segs + 1) * BODY_PATCHES;
#ifdef NO_DEGENERATE_POLYS
			uv_faces += segs * segs * BODY_PATCHES * 2 - 4 * segs;
#else
			uv_faces += segs * segs * BODY_PATCHES * 2;
#endif
		}
	}
	if (handle) {
		cverts += (segs + 1) * segs * 2 + segs * segs * 2;
		cfaces += segs * segs * HANDLE_PATCHES * 2;
		if (genUVs) {
			uv_verts += (segs + 1) * (segs + 1) * HANDLE_PATCHES;
			uv_faces += segs * segs * HANDLE_PATCHES * 2;
		}
	}
	if (spout) {
		cverts += (segs + 1) * segs * 2 + segs * segs * 2;
		cfaces += segs * segs * SPOUT_PATCHES * 2;
		if (genUVs) {
			uv_verts += (segs + 1) * (segs + 1) * SPOUT_PATCHES;
			uv_faces += segs * segs * SPOUT_PATCHES * 2;
		}
	}
	if (lid) {
		cverts += (segs - 1) * (segs - 1) * 8 +
			segs * 8 + (segs - 1) * 8 + 1;
#ifdef NO_DEGENERATE_POLYS
		cfaces += segs * segs * LID_PATCHES * 2 - 4 * segs;
#else
		cfaces += segs * segs * LID_PATCHES * 2;
#endif
		if (genUVs) {
			uv_verts += (segs + 1) * (segs + 1) * LID_PATCHES;
#ifdef NO_DEGENERATE_POLYS
			uv_faces += segs * segs * LID_PATCHES * 2 - 4 * segs;
#else
			uv_faces += segs * segs * LID_PATCHES * 2;
#endif
		}
	}

	mesh.setNumVerts(cverts);
	mesh.setNumFaces(cfaces);
	mesh.setSmoothFlags(smooth != 0);
	mesh.setNumTVerts(uv_verts);
	mesh.setNumTVFaces(uv_faces);

	BOOL usePhysUVs = GetUsePhysicalScaleUVs();
	float uScale = usePhysUVs ? ((float) PI * radius/2.0f) : 1.0f;
	float vScale = usePhysUVs ? ((float) PI * radius/2.0f) : 1.0f;

	if (body) {
		for (int index = FIRST_BODY_PATCH, i = 0; i < BODY_PATCHES; i++, index++)
			display_patch(shares, index, segs, mesh, &nvert, &nface, smooth, genUVs, &nuv_vert, &nuv_face, uScale, vScale);
	}
	if (handle) {
		for (int index = FIRST_HANDLE_PATCH, i = 0; i < HANDLE_PATCHES; i++, index++)
			display_patch(shares, index, segs, mesh, &nvert, &nface, smooth, genUVs, &nuv_vert, &nuv_face, uScale, vScale);
	}
	if (spout) {
		for (int index = FIRST_SPOUT_PATCH, i = 0; i < SPOUT_PATCHES; i++, index++)
			display_patch(shares, index, segs, mesh, &nvert, &nface, smooth, genUVs, &nuv_vert, &nuv_face, uScale, vScale);
	}
	if (lid) {
		for (int index = FIRST_LID_PATCH, i = 0; i < LID_PATCHES; i++, index++)
			display_patch(shares, index, segs, mesh, &nvert, &nface, smooth, genUVs, &nuv_vert, &nuv_face, uScale, vScale);
	}

	/* scale about 0,0,0 by radius / 2 (the teapot is generated 2x) */
	for (int c = 0; c < mesh.getNumVerts(); c++) {
#ifdef CENTER_ZERO
		mesh.verts[c].z -= 1.2;
#endif
		mesh.verts[c].x *= (radius / (float)2.0);
		mesh.verts[c].y *= (radius / (float)2.0);
		mesh.verts[c].z *= (radius / (float)2.0);
	}

	/* Clean up */
	for (int index = 0; index < PATCH_COUNT; index++) {
		free(shares[index].left);
		free(shares[index].top);
		free(shares[index].right);
		free(shares[index].bottom);
	}

	mesh.InvalidateTopologyCache();
}

// Declare a class derived from CreateMouseCallBack to handle
// the user input during the creation phase of the Teapot.
class TeapotObjCreateCallBack : public CreateMouseCallBack {
	IPoint2 sp0;
	TeapotObject *ob;
	Point3 p0;
public:
	int proc( ViewExp *vpt,int msg, int point, int flags, IPoint2 m, 
		Matrix3& mat);
	void SetObj(TeapotObject *obj) {ob = obj;}
};

// This is the method that actually handles the user input
// during the Teapot creation.
int TeapotObjCreateCallBack::proc(ViewExp *vpt,int msg, int point, 

								  int flags, IPoint2 m, Matrix3& mat ) {

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
											  sp0 = m;
											  p0 = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);
											  mat.SetTrans(p0);
											  break;
										  case 1:
											  mat.IdentityMatrix();
											  p1 = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);
											  if (teapot_crtype_blk.GetInt(teapot_create_meth)) {
												  r = Length(p1-p0);
												  mat.SetTrans(p0);
											  }
											  else {
												  center = (p0+p1)/float(2);
												  mat.SetTrans(center);
												  r = Length(center-p0);
											  } 
											  ob->pblock2->SetValue(teapot_radius,0,r);

											  if (flags&MOUSE_CTRL) {
												  float ang = (float)atan2(p1.y-p0.y,p1.x-p0.x);					
												  mat.PreRotateZ(ob->ip->SnapAngle(ang));
											  }

											  if (msg==MOUSE_POINT) {										
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

// A single instance of the callback object.
static TeapotObjCreateCallBack teapotCreateCB;

// This method allows MAX to access and call our proc method to 
// handle the user input.
CreateMouseCallBack* TeapotObject::GetCreateMouseCallBack() 
{
	teapotCreateCB.SetObj(this);
	return(&teapotCreateCB);
}


// Return TRUE if it is OK to display the mesh at the time requested,
// return FALSE otherwise.
BOOL TeapotObject::OKtoDisplay(TimeValue t) 
{
	float radius;
	pblock2->GetValue(teapot_radius,t,radius,FOREVER);
	if (radius==0.0f) return FALSE;
	else return TRUE;
}


// From GeomObject
int TeapotObject::IntersectRay(
	TimeValue t, Ray& ray, float& at, Point3& norm)
{
	int smooth;
	pblock2->GetValue(teapot_smooth,t,smooth,FOREVER);
	if (!smooth) {
		return __super::IntersectRay(t,ray,at,norm);
	}	

	BuildMesh(t);

	return mesh.IntersectRay(ray, at, norm);
}

// This method is called when the user interface controls need to be
// updated to reflect new values because of the user moving the time
// slider.  Here we simply call a method of the parameter map to 
// handle this for us.
void TeapotObject::InvalidateUI() 
{
	teapot_param_blk.InvalidateUI(pblock2->LastNotifyParamID());
}

// Make a copy of the Teapot object parameter block.
RefTargetHandle TeapotObject::Clone(RemapDir& remap) 
{
	TeapotObject* newob = new TeapotObject(FALSE);	
	newob->ReplaceReference(0, remap.CloneRef(pblock2));
	newob->ivalid.SetEmpty();	
	BaseClone(this, newob, remap);
	newob->SetUsePhysicalScaleUVs(GetUsePhysicalScaleUVs());
	return(newob);
}


static int
	isCorner(int x, int y)
{
	if ((x == 0 && y == 0) ||
		(x == 3 && y == 3) ||
		(x == 0 && y == 3) ||
		(x == 3 && y == 0))
		return TRUE;
	return FALSE;
}

static void
	setVV(PatchMesh &patch, int patch_array[4][4], int p, int x, int y, float radius,
	int *ix, int *jx)
{
	Point3 pnt;

	pnt.x = verts[patches[p][x][y]].x * radius;
	pnt.y = verts[patches[p][x][y]].y * radius;
	pnt.z = verts[patches[p][x][y]].z * radius;
	if (isCorner(x, y)) {
		patch_array[x][y] = *ix; // save it
		patch.setVert((*ix)++, pnt);
	} else {
		patch_array[x][y] = *jx; // save it
		patch.setVec((*jx)++, pnt);
	}
}

static void
	BuildTeapotPart(int base_patch, int num_patchs, PatchMesh &patch, int *in_ix, int *in_jx,
	int *in_patch, float radius, TeaShare shares[], int genUVs, BOOL usePhysUVs)
{
	int ix = *in_ix,
		jx = *in_jx,
		bpatch = *in_patch;

	for (int i = 0; i < num_patchs; i++) {
		int p = base_patch + i;
		int bvert = ix, bvec = jx;
		int patch_array[4][4];
		if (genUVs) {
			float uScale = usePhysUVs ? ((float) PI * radius) : 1.0f;
			float vScale = usePhysUVs ? ((float) PI * radius) : 1.0f;
			int tv = (bpatch+i) * 4;

			patch.setTVert(tv+0, UVVert(uScale*edges[p].eU, vScale*edges[p].bV, 0.0f));
			patch.setTVert(tv+1, UVVert(uScale*edges[p].eU, vScale*edges[p].eV, 0.0f));
			patch.setTVert(tv+2, UVVert(uScale*edges[p].bU, vScale*edges[p].eV, 0.0f));
			patch.setTVert(tv+3, UVVert(uScale*edges[p].bU, vScale*edges[p].bV, 0.0f));
			patch.getTVPatch(bpatch+i).setTVerts(tv+3,tv+0,tv+1,tv+2);
		}

		for (int x = 0; x < 4; x++) {
			for (int y = 0; y < 4; y++) {
				switch((y == 0 ? edges[p].first : (y == 3 ? edges[p].last : edges[p].center))) {
				case GenAll:
					setVV(patch, patch_array, p, x, y, radius, &ix, &jx);
					break;
				case GenSingularityBegin:
					if (x == 0 && y >= 1) { // the first point is common (must gen. 1st tangent)
						if (y == 3) { // corner
							patch_array[x][y] = patch_array[0][0];
						} else { // tangent
							if (y == 1) {
#if 0
								if (edges[p].first == ShareAll)
									patch_array[x][y] = GetShare(shares, p-1, 3, y);
								else
#endif
									setVV(patch, patch_array, p, x, y, radius, &ix, &jx);
							} else
								patch_array[x][y] = patch_array[0][1];
						}
					} else {
						setVV(patch, patch_array, p, x, y, radius, &ix, &jx);
					}
					break;
				case ShareBeginSingularityEnd:
					if (x == 0) {
						patch_array[x][y] = GetShare(shares, edges[p].patch3, edges[p].edge3, y);
					} else if (x == 3 && y > 1) {
						if (y == 3)
							patch_array[x][y] = patch_array[3][0]; // corner
						else
							patch_array[x][y] = patch_array[3][1]; // tangent
					} else {
						setVV(patch, patch_array, p, x, y, radius, &ix, &jx);
					}
					break;
				case ShareBegin:
					if (x == 0) {
						patch_array[x][y] = GetShare(shares, edges[p].patch3, edges[p].edge3, y);
					} else {
						setVV(patch, patch_array, p, x, y, radius, &ix, &jx);
					}
					break;
				case ShareAll: // this seems to work look CAREFULLY before changing
					if (y == 0)
						patch_array[x][y] = GetShare(shares, edges[p].patch0, edges[p].edge0, x);
					else
						patch_array[x][y] = GetShare(shares, edges[p].patch2, edges[p].edge2, x);
					break;
				default:
					assert(0); // we should never get here
				}
			}
		}

		/* now that we have generated all the vertices we can save the edges */
		for (int j = 0; j <  4; j++) {
			shares[p].left[j] =   patch_array[j][0];
			shares[p].top[j] =    patch_array[3][j];
			shares[p].right[j] =  patch_array[j][3];
			shares[p].bottom[j] = patch_array[0][j];
		}

		// Build the patches
		patch.patches[bpatch + i].SetType(PATCH_QUAD);
		patch.patches[bpatch + i].setVerts(patch_array[0][0],
			patch_array[0][3],
			patch_array[3][3],
			patch_array[3][0]);
		patch.patches[bpatch + i].setVecs(patch_array[0][1], patch_array[0][2],
			patch_array[1][3], patch_array[2][3],
			patch_array[3][2], patch_array[3][1],
			patch_array[2][0], patch_array[1][0]);
		patch.patches[bpatch + i].setInteriors(patch_array[1][1],
			patch_array[1][2],
			patch_array[2][2],
			patch_array[2][1]);
		patch.patches[bpatch + i].smGroup = 1;
		patch.patches[bpatch + i].SetAuto(FALSE);
	}

	*in_ix = ix;
	*in_jx = jx;
	*in_patch += num_patchs;
}

void BuildTeapotPatch(PatchMesh &patch, float radius,
					  int body, int handle, int spout, int lid, int genUVs, BOOL usePhysUVs)
{
	int nverts = 0;
	int nvecs = 0;
	int npatches = 0;
	int ntvpatches = 0;
	int ntvverts = 0;
	int bpatch = 0;
	int ix = 0;
	int jx = 0;

	if (body) {
		nverts += 17;
		nvecs += 132;
		ntvverts += 64;
		npatches += BODY_PATCHES;
		ntvpatches += BODY_PATCHES;
	}
	if (handle) {
		nverts += 6;
		nvecs += 36;
		ntvverts += 16;
		npatches += HANDLE_PATCHES;
		ntvpatches += HANDLE_PATCHES;
	}
	if (spout) {
		nverts += 6;
		nvecs += 36;
		ntvverts += 16;
		npatches += SPOUT_PATCHES;
		ntvpatches += SPOUT_PATCHES;
	}
	if (lid) {
		nverts += 9;
		nvecs += 68;
		ntvverts += 32;
		npatches += LID_PATCHES;
		ntvpatches += LID_PATCHES;
	}

	patch.setNumVerts(nverts);
	patch.setNumVecs(nvecs);
	patch.setNumPatches(npatches);
	patch.setNumTVerts(genUVs ? ntvverts : 0);
	patch.setNumTVPatches(genUVs ? ntvpatches : 0);

	TeaShare shares[PATCH_COUNT]; /* make an array to hold sharing data */
	for (int c = 0; c < PATCH_COUNT; c++) {
		shares[c].left =   (int *)malloc(4 * sizeof(int));
		shares[c].top =    (int *)malloc(4 * sizeof(int));
		shares[c].right =  (int *)malloc(4 * sizeof(int));
		shares[c].bottom = (int *)malloc(4 * sizeof(int));
	}

	if (body)
		BuildTeapotPart(FIRST_BODY_PATCH, BODY_PATCHES, patch, &ix, &jx,
		&bpatch, radius/2.0f, shares, genUVs, usePhysUVs);
	if (handle)
		BuildTeapotPart(FIRST_HANDLE_PATCH, HANDLE_PATCHES, patch, &ix, &jx,
		&bpatch, radius/2.0f, shares, genUVs, usePhysUVs);
	if (spout)
		BuildTeapotPart(FIRST_SPOUT_PATCH, SPOUT_PATCHES, patch, &ix, &jx,
		&bpatch, radius/2.0f, shares, genUVs, usePhysUVs);
	if (lid)
		BuildTeapotPart(FIRST_LID_PATCH, LID_PATCHES, patch, &ix, &jx,
		&bpatch, radius/2.0f, shares, genUVs, usePhysUVs);

	for (int c = 0; c < PATCH_COUNT; c++) {
		free(shares[c].left);
		free(shares[c].top);
		free(shares[c].right);
		free(shares[c].bottom);
	}

	if( !patch.buildLinkages() )
	{
		assert(0);
	}
	patch.InvalidateGeomCache();
}


Point3 Tbody[] = {	Point3(0.0,    0.0, 0.0),
	Point3(0.4000, 0.0, 0.0),
	Point3(0.6600, 0.0, 0.0155),
	Point3(0.7620, 0.0, 0.0544),
	Point3(0.7445, 0.0, 0.0832),
	Point3(0.8067, 0.0, 0.1225),
	Point3(1.0186, 0.0, 0.3000),
	Point3(1.0153, 0.0, 0.6665),
	Point3(0.7986, 0.0, 1.120),
	Point3(0.7564, 0.0, 1.210),
	Point3(0.7253, 0.0, 1.265),
	Point3(0.6788, 0.0, 1.264),
	Point3(0.7056, 0.0, 1.200)};

Point3 Tlid[] = {	Point3(0.0,    0.0, 1.575),
	Point3(0.1882, 0.0, 1.575),
	Point3(0.1907, 0.0, 1.506),
	Point3(0.1443, 0.0, 1.473),
	Point3(0.0756, 0.0, 1.390),
	Point3(0.0723, 0.0, 1.336),
	Point3(0.3287, 0.0, 1.283),
	Point3(0.6531, 0.0, 1.243),
	Point3(0.6500, 0.0, 1.200)};

void
	BuildNURBSBody(NURBSSet& nset, float radius, int genUVs)
{
	Point3 scale(radius, radius, radius);

	double knots[] = {	0.0, 0.0, 0.0, 0.0,
		0.1, 0.2, 0.3, 0.4,
		0.5, 0.6, 0.7, 0.8, 0.9,
		1.0, 1.0, 1.0, 1.0};

	NURBSCVCurve *c = new NURBSCVCurve();
	c->SetOrder(4);
	c->SetNumKnots(17);
	c->SetNumCVs(13);
	for (int k = 0; k < 17; k++)
		c->SetKnot(k, knots[k]);
	for (int i = 0; i < 13; i++) {
		NURBSControlVertex ncv;
		ncv.SetPosition(0, Tbody[i] * ScaleMatrix(scale));
		ncv.SetWeight(0, 1.0f);
		c->SetCV(i, ncv);
	}
	c->SetNSet(&nset);
	int p1 = nset.AppendObject(c);

	NURBSLatheSurface *s = new NURBSLatheSurface();

	s->SetParent(p1);
	Matrix3 mat = TransMatrix(Point3(0, 0, 0));
	s->SetAxis(0, mat);
	s->SetName(GetString(IDS_RB_TEAPOT));
	s->FlipNormals(TRUE);
	s->SetGenerateUVs(genUVs);
	s->Renderable(TRUE);
	s->SetTextureUVs(0, 0, Point2(0.0, 0.0));
	s->SetTextureUVs(0, 1, Point2(0.0, 2.0));
	s->SetTextureUVs(0, 2, Point2(4.0, 0.0));
	s->SetTextureUVs(0, 3, Point2(4.0, 2.0));
	s->SetNSet(&nset);
	nset.AppendObject(s);
}


void
	BuildNURBSLid(NURBSSet& nset, float radius, int genUVs)
{
	Point3 scale(radius, radius, radius);
	double knots[] = {	0.0, 0.0, 0.0, 0.0,
		1.0/6.0, 1.0/3.0, 0.5,
		2.0/3.0, 5.0/6.0,
		1.0, 1.0, 1.0, 1.0};
	NURBSCVCurve *c = new NURBSCVCurve();
	c->SetOrder(4);
	c->SetNumKnots(13);
	c->SetNumCVs(9);
	for (int k = 0; k < 13; k++)
		c->SetKnot(k, knots[k]);
	for (int i = 0; i < 9; i++) {
		NURBSControlVertex ncv;
		ncv.SetPosition(0, Tlid[i] * ScaleMatrix(scale));
		ncv.SetWeight(0, 1.0f);
		c->SetCV(i, ncv);
	}
	c->SetNSet(&nset);
	int p1 = nset.AppendObject(c);

	NURBSLatheSurface *s = new NURBSLatheSurface();
	Matrix3 mat = TransMatrix(Point3(0, 0, 0));
	s->SetAxis(0, mat);
	s->SetParent(p1);
	s->SetName(GetString(IDS_RB_TEAPOT));
	s->FlipNormals(FALSE);
	s->SetGenerateUVs(genUVs);
	s->Renderable(TRUE);
	s->SetTextureUVs(0, 0, Point2(0.0, 2.0));
	s->SetTextureUVs(0, 1, Point2(0.0, 0.0));
	s->SetTextureUVs(0, 2, Point2(2.0, 2.0));
	s->SetTextureUVs(0, 3, Point2(2.0, 0.0));
	nset.AppendObject(s);
}

#define F(s1, s2, s1r, s1c, s2r, s2c) \
	fuse.mSurf1 = (s1); \
	fuse.mSurf2 = (s2); \
	fuse.mRow1 = (s1r); \
	fuse.mCol1 = (s1c); \
	fuse.mRow2 = (s2r); \
	fuse.mCol2 = (s2c); \
	nset.mSurfFuse.Append(1, &fuse);

void
	BuildNURBSPart(NURBSSet& nset, int firstPatch, int numPatches, int *bpatch, float radius, int genUVs)
{
	Point3 scale(radius, radius, radius);

	for (int face = 0; face < numPatches; face++) {
		NURBSCVSurface *s = new NURBSCVSurface();
		nset.AppendObject(s);
		int bp = *bpatch + face;

		s->SetUOrder(4);
		s->SetVOrder(4);
		s->SetNumUKnots(8);
		s->SetNumVKnots(8);
		for (int i = 0; i < 4; i++) {
			s->SetUKnot(i, 0.0);
			s->SetUKnot(i+4, 1.0);
			s->SetVKnot(i, 0.0);
			s->SetVKnot(i+4, 1.0);
		}
		s->SetNumCVs(4, 4);
		s->FlipNormals(TRUE);

		int pnum = firstPatch + face;
		for (int r = 0; r < 4; r++) {
			for (int c = 0; c < 4; c++) {
				Teapoint *tp = &verts[patches[pnum][r][c]];
				NURBSControlVertex ncv;
				ncv.SetPosition(0, Point3(tp->x, tp->y, tp->z) * ScaleMatrix(scale));
				ncv.SetWeight(0, 1.0f);
				s->SetCV(r, c, ncv);
			}
		}

		s->SetGenerateUVs(genUVs);

		s->SetTextureUVs(0, 0, Point2(edges[pnum].bU, edges[pnum].bV));
		s->SetTextureUVs(0, 1, Point2(edges[pnum].bU, edges[pnum].eV));
		s->SetTextureUVs(0, 2, Point2(edges[pnum].eU, edges[pnum].bV));
		s->SetTextureUVs(0, 3, Point2(edges[pnum].eU, edges[pnum].eV));
	}

	// Now for fusing
	NURBSFuseSurfaceCV fuse;
	int s = *bpatch;
	// fuse the seams
	for (int u = 0; u < ((NURBSCVSurface*)nset.GetNURBSObject(s))->GetNumUCVs(); u++) {
		F(s,   s+1, u, 0, u, ((NURBSCVSurface*)nset.GetNURBSObject(s+1))->GetNumVCVs()-1);
		F(s+1, s,   u, 0, u, ((NURBSCVSurface*)nset.GetNURBSObject(s  ))->GetNumVCVs()-1);
		F(s+2, s+3, u, 0, u, ((NURBSCVSurface*)nset.GetNURBSObject(s+3))->GetNumVCVs()-1);
		F(s+3, s+2, u, 0, u, ((NURBSCVSurface*)nset.GetNURBSObject(s+2))->GetNumVCVs()-1);
	}

	// fuse the middles
	for (int v = 1; v < ((NURBSCVSurface*)nset.GetNURBSObject(s))->GetNumVCVs(); v++) {
		F(s,   s+2, ((NURBSCVSurface*)nset.GetNURBSObject(s  ))->GetNumUCVs()-1, v, 0, v);
		F(s+1, s+3, ((NURBSCVSurface*)nset.GetNURBSObject(s+1))->GetNumUCVs()-1, v, 0, v);
	}


	*bpatch += numPatches;
}

Object* BuildNURBSTeapot(float radius, int body, int handle, int spout, int lid, int genUVs)
{
	int numpat = 0;

	if (body)
		numpat += 1;
	if (handle)
		numpat += HANDLE_PATCHES;
	if (spout)
		numpat += SPOUT_PATCHES;
	if (lid)
		numpat += 1;

	NURBSSet nset;

	int bpatch = 0;

	if (handle)
		BuildNURBSPart(nset, FIRST_HANDLE_PATCH, HANDLE_PATCHES, &bpatch, radius/2.0f, genUVs);
	if (spout)
		BuildNURBSPart(nset, FIRST_SPOUT_PATCH, SPOUT_PATCHES, &bpatch, radius/2.0f, genUVs);
	if (body)
		BuildNURBSBody(nset, radius, genUVs);
	if (lid)
		BuildNURBSLid(nset, radius, genUVs);

	Matrix3 mat;
	mat.IdentityMatrix();
	Object *ob = CreateNURBSObject(NULL, &nset, mat);
	return ob;
}

Object* TeapotObject::ConvertToType(TimeValue t, Class_ID obtype)
{
	if (obtype == patchObjectClassID) {
		Interval valid = FOREVER;
		float radius;
		int body, handle, spout, lid, genUVs;
		pblock2->GetValue(teapot_radius,t,radius,valid);
		pblock2->GetValue(teapot_body,t,body,valid);	
		pblock2->GetValue(teapot_handle,t,handle,valid);	
		pblock2->GetValue(teapot_spout,t,spout,valid);	
		pblock2->GetValue(teapot_lid,t,lid,valid);	
		pblock2->GetValue(teapot_mapping,t,genUVs,valid);	
		PatchObject *ob = new PatchObject();
		BuildTeapotPatch(ob->patch,radius, body, handle, spout, lid, genUVs, GetUsePhysicalScaleUVs());
		ob->SetChannelValidity(TOPO_CHAN_NUM,valid);
		ob->SetChannelValidity(GEOM_CHAN_NUM,valid);
		ob->SetChannelValidity(TEXMAP_CHAN_NUM,valid);
		ob->UnlockObject();
		return ob;
	} 
	if (obtype == EDITABLE_SURF_CLASS_ID) {
		Interval valid = FOREVER;
		float radius;
		int body, handle, spout, lid, genUVs;
		pblock2->GetValue(teapot_radius, t, radius, valid);
		pblock2->GetValue(teapot_body, t, body, valid);
		pblock2->GetValue(teapot_handle, t, handle, valid);
		pblock2->GetValue(teapot_spout, t, spout, valid);
		pblock2->GetValue(teapot_lid, t, lid, valid);
		pblock2->GetValue(teapot_mapping, t, genUVs, valid);
		Object *ob = BuildNURBSTeapot(radius, body, handle, spout, lid, genUVs);
		ob->SetChannelValidity(TOPO_CHAN_NUM,valid);
		ob->SetChannelValidity(GEOM_CHAN_NUM,valid);
		ob->SetChannelValidity(TEXMAP_CHAN_NUM,valid);
		ob->UnlockObject();
		return ob;
	}
	return __super::ConvertToType(t,obtype);
}

int TeapotObject::CanConvertToType(Class_ID obtype)
{
	if(obtype == patchObjectClassID) return 1;
	if(obtype == EDITABLE_SURF_CLASS_ID) return 1;
	return __super::CanConvertToType(obtype);
}

void TeapotObject::GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist)
{
	Object::GetCollapseTypes(clist, nlist);
	Class_ID id = EDITABLE_SURF_CLASS_ID;
	TSTR *name = new TSTR(GetString(IDS_SM_NURBS_SURFACE));
	clist.Append(1,&id);
	nlist.Append(1,&name);
}

void TeapotObject::UpdateUI()
{
	if (ip == NULL)
		return;
	TeapotParamDlgProc* dlg = static_cast<TeapotParamDlgProc*>(teapot_param_blk.GetUserDlgProc());
	dlg->UpdateUI();
}

BOOL TeapotObject::GetUsePhysicalScaleUVs()
{
	return ::GetUsePhysicalScaleUVs(this);
}


void TeapotObject::SetUsePhysicalScaleUVs(BOOL flag)
{
	BOOL curState = GetUsePhysicalScaleUVs();
	if (curState == flag)
		return;
	if (theHold.Holding())
		theHold.Put(new RealWorldScaleRecord<TeapotObject>(this, curState));
	::SetUsePhysicalScaleUVs(this, flag);
	if (pblock2 != NULL)
		pblock2->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	UpdateUI();
	macroRec->SetProperty(this, _T("realWorldMapSize"), mr_bool, flag);
}

