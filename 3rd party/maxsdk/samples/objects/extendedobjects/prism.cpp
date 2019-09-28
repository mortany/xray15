/**********************************************************************
 *<
	FILE:prism.cpp
	CREATED BY:  Audrey Peterson

 *>	Copyright (c) 1996, All Rights Reserved.
 **********************************************************************/

#include "solids.h"

#include "iparamm2.h"
#include "Simpobj.h"
#include "surf_api.h"
#include <ReferenceSaveManager.h>
#include <Util/StaticAssert.h>

class PrismObject : public SimpleObject2 {
	friend class PrismTypeInDlgProc;
	friend class PrismObjCreateCallBack;
	friend class PrismSideLenAccessor;
public:
	// Class vars	
	static IObjParam *ip;
	static bool typeinCreate;

	PrismObject();

	// From Object
	int CanConvertToType(Class_ID obtype) override;
	Object* ConvertToType(TimeValue t, Class_ID obtype) override;
	void GetCollapseTypes(Tab<Class_ID> &clist, Tab<TSTR*> &nlist) override;

	// From BaseObject
	CreateMouseCallBack* GetCreateMouseCallBack() override;
	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev) override;
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next) override;
	const TCHAR *GetObjectName() { return GetString(IDS_RB_PRISM); }
	BOOL HasUVW() override;
	void SetGenUVW(BOOL sw) override;

	// Animatable methods		
	void DeleteThis() override { delete this; }
	Class_ID ClassID() override { return PRISM_CLASS_ID; }

	// From ref
	RefTargetHandle Clone(RemapDir& remap) override;
	bool SpecifySaveReferences(ReferenceSaveManager& referenceSaveManager) override;
	IOResult Load(ILoad *iload) override;
	
	// From SimpleObjectBase
	void BuildMesh(TimeValue t) override;
	BOOL OKtoDisplay(TimeValue t) override;
	void InvalidateUI() override;
};

// class variables for Prism class.
IObjParam *PrismObject::ip = NULL;
bool PrismObject::typeinCreate = false;

#define PBLOCK_REF_NO	 0

#define MIN_SEGMENTS	1
#define MAX_SEGMENTS	200

#define DEF_SEGMENTS 	1
#define DEF_SIDES		1

#define DEF_RADIUS		float(0.0)
#define DEF_HEIGHT		float(0.01)
#define DEF_FILLET		float(0.01)

#define BMIN_HEIGHT		float(-1.0E30)
#define BMAX_HEIGHT		float(1.0E30)
#define BMIN_LENGTH		float(0.1)
#define BMAX_LENGTH		float(1.0E30)

//--- ClassDescriptor and class vars ---------------------------------

class PrismClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() override { return 1; }
	void *			Create(BOOL loading = FALSE) override { return new PrismObject; }
	const TCHAR *	ClassName() override { return GetString(IDS_AP_PRISM_CLASS); }
	SClass_ID		SuperClassID() override { return GEOMOBJECT_CLASS_ID; }
	Class_ID		ClassID() override { return PRISM_CLASS_ID; }
	const TCHAR* 	Category() override { return GetString(IDS_RB_EXTENDED); }
	const TCHAR*	InternalName() override { return _T("Prism"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() override { return hInstance; }			// returns owning module handle
};

static PrismClassDesc PrismDesc;

ClassDesc* GetPrismDesc() { return &PrismDesc; }

// in solids.cpp  - The dll instance handle
extern HINSTANCE hInstance;

// ParamBlockDesc2 IDs
enum paramblockdesc2_ids { prism_creation_type, prism_type_in, prism_params, };
enum prism_creation_type_param_ids { prism_create_meth, };
enum prism_type_in_param_ids { prism_ti_pos, prism_ti_side1, prism_ti_side2, prism_ti_side3, prism_ti_height, };
enum prism_param_param_ids {
	prism_side1 = PRISM_SIDE1, prism_side2 = PRISM_SIDE2, prism_side3 = PRISM_SIDE3,
	prism_height = PRISM_HEIGHT, prism_s1segs = PRISM_S1SEGS, prism_s2segs = PRISM_S2SEGS,
	prism_s3segs = PRISM_S3SEGS, prism_hsegs = PRISM_HSEGS, prism_mapping = PRISM_GENUVS,
};

namespace
{
	MaxSDK::Util::StaticAssert< (prism_params == PRISM_PARAMBLOCK_ID) > validator;
}

void FixSideLengths(IParamBlock2* pblock2, TimeValue t, ParamID side1, ParamID side2, ParamID side3, ParamID side_changing)
{
	float s1len = 0.f, s2len = 0.f, s3len = 0.f;
	float a, b, c, ll, ul;
	pblock2->GetValue(side1, t, s1len, FOREVER);
	pblock2->GetValue(side2, t, s2len, FOREVER);
	pblock2->GetValue(side3, t, s3len, FOREVER);
	// Given only the length of two sides of a triangle, the length of the third side is not fixed.
	// Let a and b represent the lengths of the two known sides such that a >= b.
	// Let c represent the length of the unknown side, the length of c must fall within
	// a−b <= c <= a+b
	// Here we are limiting upper to 0.95*(a+b) (variable ul here)
	ParamID idToChange;
	if (side_changing == side1)
	{
		a = s1len;
		b = s3len;
		c = s2len;
		idToChange = side2;
	}
	else if (side_changing == side2)
	{
		a = s2len;
		b = s1len;
		c = s3len;
		idToChange = side3;
	}
	else if (side_changing == side3)
	{
		a = s3len;
		b = s2len;
		c = s1len;
		idToChange = side1;
	}
	else
	{
		DbgAssert(false);
		return;
	}
	ll = fabs(a - b);
	ul = 0.95f * (a + b);
	if (c < ll)
	{
		pblock2->SetValue(idToChange, t, ll);
	}
	else if (c > ul)
	{
		pblock2->SetValue(idToChange, t, ul);
	}
}

class PrismSideLenClassAccessor : public PBAccessor
{
	bool m_disable; // for preventing re-entry
public:
	PrismSideLenClassAccessor() : m_disable(false) {}
	virtual void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t) override
	{
		if (m_disable)
			return;
		if (theHold.RestoreOrRedoing())
			return;
		IParamBlock2* pblock2 = PrismDesc.GetParamBlockDescByID(prism_type_in)->class_params;
		DbgAssert(pblock2);
		if (pblock2 == nullptr)
			return;
		m_disable = true;
		FixSideLengths(pblock2, t, prism_ti_side1, prism_ti_side2, prism_ti_side3, id);
		m_disable = false;
	}
};
static PrismSideLenClassAccessor prismSideLenClass_Accessor;


class PrismSideLenAccessor : public PBAccessor
{
	bool m_disable; // for preventing re-entry
public:
	PrismSideLenAccessor() : m_disable(false) {}
	virtual void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t) override
	{
		if (m_disable)
			return;
		if (theHold.RestoreOrRedoing())
			return;
		IParamBlock2* pblock2 = ((PrismObject*)owner)->pblock2;
		DbgAssert(pblock2);
		if (pblock2 == nullptr)
			return;
		m_disable = true;
		FixSideLengths(pblock2, t, prism_side1, prism_side2, prism_side3, id);
		m_disable = false;
	}
};
static PrismSideLenAccessor prismSideLen_Accessor;

// class creation type block
static ParamBlockDesc2 prism_crtype_blk(prism_creation_type, _T("PrismCreationType"), 0, &PrismDesc, P_CLASS_PARAMS + P_AUTO_UI,
	//rollout
	IDD_PRISM1, IDS_RB_CREATE_DIALOG, BEGIN_EDIT_CREATE, 0, NULL,
	// params
	prism_create_meth, _T("typeinCreationMethod"), TYPE_INT, 0, IDS_RB_CREATE_DIALOG,
	p_default, 1,
	p_range, 0, 1,
	p_ui, TYPE_RADIO, 2, IDC_PR_CREATEBASE, IDC_PR_CREATEVERTICES,
	p_end,
	p_end
);

// class type-in block
static ParamBlockDesc2 prism_typein_blk(prism_type_in, _T("PrismTypeIn"), 0, &PrismDesc, P_CLASS_PARAMS + P_AUTO_UI,
	//rollout
	IDD_PRISM2, IDS_RB_KEYBOARDENTRY, BEGIN_EDIT_CREATE, APPENDROLL_CLOSED, NULL,
	// params
	prism_ti_pos, _T("typeInPos"), TYPE_POINT3, 0, IDS_RB_TYPEIN_POS,
	p_default, Point3(0, 0, 0),
	p_range, float(-1.0E30), float(1.0E30),
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_PR_POSX, IDC_PR_POSXSPIN, IDC_PR_POSY, IDC_PR_POSYSPIN, IDC_PR_POSZ, IDC_PR_POSZSPIN, SPIN_AUTOSCALE,
	p_end,
	prism_ti_side1, _T("typeInSide1Length"), TYPE_WORLD, 0, IDS_RB_SIDE1,
	p_default, 0.0,
	p_accessor, &prismSideLenClass_Accessor,
	p_range, BMIN_LENGTH, BMAX_LENGTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_PR_SIDE1LEN, IDC_PR_SIDE1LENSPIN, SPIN_AUTOSCALE,
	p_end,
	prism_ti_side2, _T("typeInSide2Length"), TYPE_WORLD, 0, IDS_RB_SIDE2,
	p_default, 0.0,
	p_accessor, &prismSideLenClass_Accessor,
	p_range, BMIN_LENGTH, BMAX_LENGTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_PR_SIDE2LEN, IDC_PR_SIDE2LENSPIN, SPIN_AUTOSCALE,
	p_end,
	prism_ti_side3, _T("typeInSide3Length"), TYPE_WORLD, 0, IDS_RB_SIDE3,
	p_default, 0.0,
	p_accessor, &prismSideLenClass_Accessor,
	p_range, BMIN_LENGTH, BMAX_LENGTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_PR_SIDE3LEN, IDC_PR_SIDE3LENSPIN, SPIN_AUTOSCALE,
	p_end,
	prism_ti_height, _T("typeInHeight"), TYPE_WORLD, 0, IDS_RB_HEIGHT,
	p_default, 0.0,
	p_range, BMIN_HEIGHT, BMAX_HEIGHT,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_PR_HEIGHT, IDC_PR_HEIGHTSPIN, SPIN_AUTOSCALE,
	p_end,
	p_end
);

// per instance pyramid block
static ParamBlockDesc2 prism_param_blk(prism_params, _T("PrismParameters"), 0, &PrismDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_REF_NO,
	//rollout
	IDD_PRISM3, IDS_AP_PARAMETERS, 0, 0, NULL,
	// params
	prism_side1, _T("side1Length"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_SIDE1,
	p_default, 0.0,
	p_ms_default, 25.0,
	p_accessor, &prismSideLen_Accessor,
	p_range, BMIN_LENGTH, BMAX_LENGTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_PR_SIDE1LEN, IDC_PR_SIDE1LENSPIN, SPIN_AUTOSCALE,
	p_end,
	prism_side2, _T("side2Length"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_SIDE2,
	p_default, 0.0,
	p_ms_default, 25.0,
	p_accessor, &prismSideLen_Accessor,
	p_range, BMIN_LENGTH, BMAX_LENGTH,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_PR_SIDE2LEN, IDC_PR_SIDE2LENSPIN, SPIN_AUTOSCALE,
	p_end,
	prism_side3, _T("side3Length"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_SIDE3,
	p_default, 0.0,
	p_ms_default, 25.0,
	p_range, BMIN_LENGTH, BMAX_LENGTH,
	p_accessor, &prismSideLen_Accessor,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_PR_SIDE3LEN, IDC_PR_SIDE3LENSPIN, SPIN_AUTOSCALE,
	p_end,
	prism_height, _T("height"), TYPE_WORLD, P_ANIMATABLE + P_RESET_DEFAULT, IDS_RB_HEIGHT,
	p_default, 0.0,
	p_ms_default, 10.0,
	p_range, BMIN_HEIGHT, BMAX_HEIGHT,
	p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_PR_HEIGHT, IDC_PR_HEIGHTSPIN, SPIN_AUTOSCALE,
	p_end,
	prism_s1segs, _T("side1Segs"), TYPE_INT, P_ANIMATABLE, IDS_RB_S1SEGS,
	p_default, DEF_SIDES,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_PR_SIDE1SEGS, IDC_PR_SIDE1SEGSSPIN, 0.1f,
	p_end,
	prism_s2segs, _T("side2Segs"), TYPE_INT, P_ANIMATABLE, IDS_RB_S2SEGS,
	p_default, DEF_SIDES,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_PR_SIDE2SEGS, IDC_PR_SIDE2SEGSSPIN, 0.1f,
	p_end,
	prism_s3segs, _T("side3Segs"), TYPE_INT, P_ANIMATABLE, IDS_RB_S3SEGS,
	p_default, DEF_SIDES,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_PR_SIDE3SEGS, IDC_PR_SIDE3SEGSSPIN, 0.1f,
	p_end,
	prism_hsegs, _T("heightSegs"), TYPE_INT, P_ANIMATABLE, IDS_RB_HSEGS,
	p_default, DEF_SEGMENTS,
	p_range, MIN_SEGMENTS, MAX_SEGMENTS,
	p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_PR_HGTSEGS, IDC_PR_HGTSPIN, 0.1f,
	p_end,
	prism_mapping, _T("mapCoords"), TYPE_BOOL, 0, IDS_MXS_GENUVS,
	p_default, TRUE,
	p_ms_default, FALSE,
	p_ui, TYPE_SINGLECHECKBOX, IDC_GENTEXTURE,
	p_end,
	p_end
);

// variable type, NULL, animatable, number
ParamBlockDescID PrismdescVer0[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0, prism_side1 },
	{ TYPE_FLOAT, NULL, TRUE, 1, prism_side2 },
	{ TYPE_FLOAT, NULL, TRUE, 2, prism_side3 },
	{ TYPE_FLOAT, NULL, TRUE, 3, prism_height },
	{ TYPE_INT, NULL, TRUE, 4, prism_s1segs },
	{ TYPE_INT, NULL, TRUE, 5, prism_s2segs },
	{ TYPE_INT, NULL, TRUE, 6, prism_s3segs },
	{ TYPE_INT, NULL, TRUE, 7, prism_hsegs },
	{ TYPE_INT, NULL, FALSE, 8, prism_mapping },
};

// Array of old versions
static ParamVersionDesc versions[] = {
	ParamVersionDesc(PrismdescVer0,9,0),
};

#define PBLOCK_LENGTH	9

// ParamBlock data for SaveToPrevious support
#define NUM_OLDVERSIONS	1
#define CURRENT_VERSION	0

//--- TypeInDlgProc --------------------------------

class PrismTypeInDlgProc : public ParamMap2UserDlgProc {
public:
	PrismObject *ob;

	PrismTypeInDlgProc(PrismObject *o) { ob = o; }
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	void DeleteThis() override { delete this; }
};

INT_PTR PrismTypeInDlgProc::DlgProc(
	TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_PR_CREATE: {
			if (prism_typein_blk.GetFloat(prism_ti_height) == 0.0f) return TRUE;

			// We only want to set the value if the object is 
			// not in the scene.
			if (ob->TestAFlag(A_OBJ_CREATING)) {
				ob->pblock2->SetValue(prism_height, 0, prism_typein_blk.GetFloat(prism_ti_height));
				ob->pblock2->SetValue(prism_side1, 0, prism_typein_blk.GetFloat(prism_ti_side1));
				ob->pblock2->SetValue(prism_side2, 0, prism_typein_blk.GetFloat(prism_ti_side2));
				ob->pblock2->SetValue(prism_side3, 0, prism_typein_blk.GetFloat(prism_ti_side3));
			}
			else
				PrismObject::typeinCreate = true;

			Matrix3 tm(1);
			tm.SetTrans(prism_typein_blk.GetPoint3(prism_ti_pos));
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

//--- Prism methods -------------------------------

PrismObject::PrismObject()
{
	PrismDesc.MakeAutoParamBlocks(this);
}

bool PrismObject::SpecifySaveReferences(ReferenceSaveManager& referenceSaveManager)
{
	// if saving to previous version that used pb1 instead of pb2...
	DWORD saveVersion = GetSavingVersion();
	if (saveVersion != 0 && saveVersion <= MAX_RELEASE_R19)
	{
		ProcessPB2ToPB1SaveToPrevious(this, pblock2, PBLOCK_REF_NO, PrismdescVer0, PBLOCK_LENGTH, CURRENT_VERSION);
	}
	return __super::SpecifySaveReferences(referenceSaveManager);
}

IOResult PrismObject::Load(ILoad *iload)
{
	ParamBlock2PLCB* plcb = new ParamBlock2PLCB(versions, NUM_OLDVERSIONS, &prism_param_blk, this, PBLOCK_REF_NO);
	iload->RegisterPostLoadCallback(plcb);
	return IO_OK;
}

void PrismObject::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	__super::BeginEditParams(ip, flags, prev);
	this->ip = ip;
	// If this has been freshly created by type-in, set creation values:
	if (PrismObject::typeinCreate)
	{
		pblock2->SetValue(prism_side1, 0, prism_typein_blk.GetFloat(prism_ti_side1));
		pblock2->SetValue(prism_side2, 0, prism_typein_blk.GetFloat(prism_ti_side2));
		pblock2->SetValue(prism_side3, 0, prism_typein_blk.GetFloat(prism_ti_side3));
		pblock2->SetValue(prism_height, 0, prism_typein_blk.GetFloat(prism_ti_height));
		PrismObject::typeinCreate = false;
	}

	// throw up all the appropriate auto-rollouts
	PrismDesc.BeginEditParams(ip, this, flags, prev);
	// if in Create Panel, install a callback for the type in.
	if (flags & BEGIN_EDIT_CREATE)
		prism_typein_blk.SetUserDlgProc(new PrismTypeInDlgProc(this));
}

void PrismObject::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	__super::EndEditParams(ip, flags, next);
	this->ip = NULL;
	PrismDesc.EndEditParams(ip, this, flags, next);
}

void BuildPrismMesh(Mesh &mesh,
	int s1segs, int s2segs, int s3segs, int llsegs,
	float s1len, float s2len, float s3len, float height,
	int genUVs)
{
	BOOL minush = (height < 0.0f);
	if (minush) height = -height;
	int nf = 0, totalsegs = s1segs + s2segs + s3segs;
	float s13len = s1len*s3len;
	if ((s1len <= 0.0f) || (s2len <= 0.0f) || (s3len <= 0.0f))
	{
		mesh.setNumVerts(0);
		mesh.setNumFaces(0);
		mesh.setNumTVerts(0);
		mesh.setNumTVFaces(0);
	}
	else
	{
		float acvalue = (s2len*s2len - s1len*s1len - s3len*s3len) / (-2.0f*s13len);
		acvalue = (acvalue < -1.0f ? -1.0f : (acvalue > 1.0f ? acvalue = 1.0f : acvalue));
		float theta = (float)acos(acvalue);
		int nfaces = 0;
		int ntverts = 0;
		nfaces = 2 * totalsegs*(llsegs + 1);
		int nverts = totalsegs*(llsegs + 1) + 2;

		mesh.setNumVerts(nverts);
		mesh.setNumFaces(nfaces);
		if (genUVs)
		{
			ntverts = nverts + 2 * totalsegs + llsegs + 1;
			mesh.setNumTVerts(ntverts);
			mesh.setNumTVFaces(nfaces);
		}
		else
		{
			ntverts = 0;
			mesh.setNumTVerts(0);
			mesh.setNumTVFaces(0);
		}

		Point3 Pt0 = Point3(0.0f, 0.0f, height), Pt1 = Point3(s1len, 0.0f, height);
		Point3 Pt2 = Point3(s3len*(float)cos(theta), s3len*(float)sin(theta), height);
		Point3 CenterPt = (Pt0 + Pt1 + Pt2) / 3.0f, Mins;
		float maxx;
		if (s1len < Pt2.x) { Mins.x = s1len; maxx = Pt2.x; }
		else { Mins.x = Pt2.x; maxx = s1len; }
		if (maxx < 0.0f) maxx = 0.0f;
		if (Mins.x > 0.0f) Mins.x = 0.0f;
		Mins.y = 0.0f;
		float xdist = maxx - Mins.x, ydist = Pt2.y;
		if (xdist == 0.0f) xdist = 0.001f; if (ydist == 0.0f) ydist = 0.001f;
		mesh.setVert(0, CenterPt);
		mesh.setVert(nverts - 1, Point3(CenterPt.x, CenterPt.y, 0.0f));
		mesh.setVert(1, Pt0);
		int botstart = ntverts - totalsegs - 1, tnv = totalsegs + 1;
		float u = 0.0f, yval = Pt2.y, bu, tu, tb;
		if (genUVs)
		{
			mesh.setTVert(0, tu = (CenterPt.x - Mins.x) / xdist, tb = (CenterPt.y - Mins.y) / ydist, 0.0f);
			mesh.setTVert(1, bu = (-Mins.x / xdist), 0.0f, 0.0f);
			mesh.setTVert(tnv++, 0.0f, 1.0f, 0.0f);
			mesh.setTVert(botstart++, 1.0f - bu, 1.0f, 0.0f);
			mesh.setTVert(ntverts - 1, 1.0f - tu, 1.0f - tb, 0.0f);
		}
		int i, nv = 2;
		float sincr = s1len / s1segs, tdist = s1len + s2len + s3len, udiv = sincr / tdist, pos;
		if (tdist == 0.0f) tdist = 0.0001f;
		for (i = 1; i < s1segs; i++)
		{
			mesh.setVert(nv, Point3(pos = sincr*i, 0.0f, height));
			if (genUVs)
			{
				mesh.setTVert(nv, bu = (pos - Mins.x) / xdist, 0.0f, 0.0f);
				mesh.setTVert(tnv++, u += udiv, 1.0f, 0.0f);
				mesh.setTVert(botstart++, 1.0f - bu, 1.0f, 0.0f);
			}
			nv++;
		}
		mesh.setVert(nv, Pt1);
		if (genUVs)
		{
			mesh.setTVert(nv, bu = (Pt1.x - Mins.x) / xdist, 0.0f, 0.0f);
			mesh.setTVert(tnv++, u += udiv, 1.0f, 0.0f);
			mesh.setTVert(botstart++, 1.0f - bu, 1.0f, 0.0f);
		}
		Point3 slope = (Pt2 - Pt1) / (float)s2segs;
		float ypos, bv;
		nv++; udiv = (s2len / s2segs) / tdist;
		for (i = 1; i < s2segs; i++)
		{
			mesh.setVert(nv, Point3(pos = (Pt1.x + slope.x*i), ypos = (Pt1.y + slope.y*i), height));
			if (genUVs)
			{
				mesh.setTVert(nv, bu = (pos - Mins.x) / xdist, bv = (ypos - Mins.y) / ydist, 0.0f);
				mesh.setTVert(tnv++, u += udiv, 1.0f, 0.0f);
				mesh.setTVert(botstart++, 1.0f - bu, 1.0f - bv, 0.0f);
			}
			nv++;
		}
		mesh.setVert(nv, Pt2);
		if (genUVs)
		{
			mesh.setTVert(nv, bu = (Pt2.x - Mins.x) / xdist, 1.0f, 0.0f);
			mesh.setTVert(tnv++, u += udiv, 1.0f, 0.0f);
			mesh.setTVert(botstart++, 1.0f - bu, 0.0f, 0.0f);
		}
		nv++; slope = (Pt0 - Pt2) / (float)s3segs; udiv = (s2len / s2segs) / tdist;
		for (i = 1; i < s3segs; i++)
		{
			mesh.setVert(nv, Point3(pos = (Pt2.x + slope.x*i), ypos = (Pt2.y + slope.y*i), height));
			if (genUVs)
			{
				mesh.setTVert(nv, bu = (pos - Mins.x) / xdist, bv = (ypos - Mins.y) / ydist, 0.0f);
				mesh.setTVert(tnv++, u += udiv, 1.0f, 0.0f);
				mesh.setTVert(botstart++, 1.0f - bu, 1.0f - bv, 0.0f);
			}
			nv++;
		}
		if (genUVs)	mesh.setTVert(tnv++, 1.0f, 1.0f, 0.0f);
		//top layer done, now reflect sides down 
		int sidevs, startv = 1, deltav, ic;
		startv = 1;
		sincr = height / llsegs;
		Point3 p;
		for (sidevs = 0; sidevs < totalsegs; sidevs++)
		{
			p = mesh.verts[startv];
			deltav = totalsegs;
			for (ic = 1; ic <= llsegs; ic++)
			{
				p.z = height - sincr*ic;
				mesh.setVert(startv + deltav, p);
				deltav += totalsegs;
			}
			startv++;
		}
		if (genUVs)
		{
			startv = totalsegs + 1;
			int tvseg = totalsegs + 1;
			for (sidevs = 0; sidevs <= totalsegs; sidevs++)
			{
				p = mesh.tVerts[startv];
				deltav = tvseg;
				for (ic = 1; ic <= llsegs; ic++)
				{
					p.y = 1.0f - ic / (float)llsegs;
					mesh.setTVert(startv + deltav, p);
					deltav += tvseg;
				}
				startv++;
			}
		}
		int fc = 0, sidesm = 2;
		int last = totalsegs - 1;
		// Now make faces ---
		int j, b0 = 1, b1 = 2, tb0, tb1, tt0, tt1, t0, t1, ecount = 0, s2end = s1segs + s2segs;
		for (i = 0; i < totalsegs; i++)
		{
			if (genUVs) mesh.tvFace[fc].setTVerts(0, b0, (i < last ? b1 : 1));
			AddFace(&mesh.faces[fc++], 0, b0++, (i < last ? b1++ : 1), 0, 1);
		}
		tt1 = (tt0 = i + 1) + 1; t0 = 1; t1 = 2; b1 = (b0 = t0 + totalsegs) + 1;
		tb1 = (tb0 = tt1 + totalsegs) + 1;
		for (i = 1; i <= llsegs; i++)
		{
			for (j = 0; j < totalsegs; j++)
			{
				if (genUVs)
				{
					mesh.tvFace[fc].setTVerts(tt0, tb0++, tb1);
					mesh.tvFace[fc + 1].setTVerts(tt0++, tb1++, tt1++);
				}
				if (j < s1segs) sidesm = 2;
				else if (j < s2end) sidesm = 4;
				else sidesm = 8;
				AddFace(&mesh.faces[fc++], t0, b0++, (j == last ? t1 : b1), 0, sidesm);
				if (j < last)
					AddFace(&mesh.faces[fc++], t0++, b1, t1, 1, sidesm);
				else
					AddFace(&mesh.faces[fc++], t0++, b1 - totalsegs, t1 - totalsegs, 1, sidesm);
				t1++; b1++;
			}
			tt0++; tt1++; tb0++; tb1++;
		}
		if (genUVs) { tt0 = (tt1 = ntverts - totalsegs) - 1; tb0 = ntverts - 1; }
		for (i = 0; i < totalsegs; i++)
		{
			if (genUVs)
			{
				mesh.tvFace[fc].setTVerts(tt0++, tb0, (i == last ? tt1 - totalsegs : tt1));
				tt1++;
			}
			AddFace(&mesh.faces[fc++], t0++, b0, (i == last ? t1 - totalsegs : t1), 1, 1);
			t1++;
		}
		if (minush)
			for (i = 0; i < nverts; i++) mesh.verts[i].z -= height;
		assert(fc == mesh.numFaces);
		//	assert(nv==mesh.numVerts); */
	}
	mesh.InvalidateTopologyCache();
}
BOOL PrismObject::HasUVW() {
	BOOL genUVs;
	Interval v;
	pblock2->GetValue(prism_mapping, 0, genUVs, v);
	return genUVs;
}

void PrismObject::SetGenUVW(BOOL sw) {
	if (sw == HasUVW()) return;
	pblock2->SetValue(prism_mapping, 0, sw);
}

void PrismObject::BuildMesh(TimeValue t)
{
	int hsegs, s1segs, s2segs, s3segs;
	float height, s1len, s2len, s3len;
	int genUVs;

	// Start the validity interval at forever and widdle it down.
	ivalid = FOREVER;

	pblock2->GetValue(prism_hsegs, t, hsegs, ivalid);
	pblock2->GetValue(prism_s1segs, t, s1segs, ivalid);
	pblock2->GetValue(prism_s2segs, t, s2segs, ivalid);
	pblock2->GetValue(prism_s3segs, t, s3segs, ivalid);
	pblock2->GetValue(prism_height, t, height, ivalid);
	pblock2->GetValue(prism_side1, t, s1len, ivalid);
	pblock2->GetValue(prism_side2, t, s2len, ivalid);
	pblock2->GetValue(prism_side3, t, s3len, ivalid);
	pblock2->GetValue(prism_mapping, t, genUVs, ivalid);
	LimitValue(height, BMIN_HEIGHT, BMAX_HEIGHT);
	LimitValue(s1len, BMIN_LENGTH, BMAX_LENGTH);
	LimitValue(s2len, BMIN_LENGTH, BMAX_LENGTH);
	LimitValue(s3len, BMIN_LENGTH, BMAX_LENGTH);
	LimitValue(hsegs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(s1segs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(s2segs, MIN_SEGMENTS, MAX_SEGMENTS);
	LimitValue(s3segs, MIN_SEGMENTS, MAX_SEGMENTS);

	BuildPrismMesh(mesh, s1segs, s2segs, s3segs, hsegs,
		s1len, s2len, s3len, height, genUVs);
}

inline Point3 operator+(const PatchVert &pv, const Point3 &p)
{
	return p + pv.p;
}

Object*
BuildNURBSPrism(float side1, float side2, float side3, float height, int genUVs)
{
	float s13len = side1*side3;
	float theta = (float)acos((side2*side2 - side1*side1 - side3*side3) / (-2.0f*s13len));

	int prism_faces[5][4] = { {0, 1, 2, 2}, // bottom
							{1, 0, 4, 3}, // front
							{2, 1, 5, 4}, // left
							{0, 2, 3, 5}, // right
							{4, 3, 5, 5} };// top
	Point3 prism_verts[6] = { Point3(0.0f, 0.0f, 0.0f),
		Point3(side1,  0.0f, 0.0f),
		Point3(side3*(float)cos(theta), side3*(float)sin(theta), 0.0f),
		Point3(0.0f, 0.0f, height),
		Point3(side1,  0.0f, height),
							Point3(side3*(float)cos(theta), side3*(float)sin(theta), height) };

	NURBSSet nset;

	for (int face = 0; face < 5; face++) {
		Point3 bl = prism_verts[prism_faces[face][0]];
		Point3 br = prism_verts[prism_faces[face][1]];
		Point3 tl = prism_verts[prism_faces[face][2]];
		Point3 tr = prism_verts[prism_faces[face][3]];

		NURBSCVSurface *surf = new NURBSCVSurface();
		nset.AppendObject(surf);
		surf->SetUOrder(4);
		surf->SetVOrder(4);
		surf->SetNumCVs(4, 4);
		surf->SetNumUKnots(8);
		surf->SetNumVKnots(8);

		Point3 top, bot;
		for (int r = 0; r < 4; r++) {
			top = tl + (((float)r / 3.0f) * (tr - tl));
			bot = bl + (((float)r / 3.0f) * (br - bl));
			for (int c = 0; c < 4; c++) {
				NURBSControlVertex ncv;
				ncv.SetPosition(0, bot + (((float)c / 3.0f) * (top - bot)));
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

		float sum = side1 + side2 + side3;
		float s1 = side1 / sum;
		float s3 = 1.0f - side3 / sum;
		switch (face) {
		case 0:
			surf->SetTextureUVs(0, 0, Point2(0.5f, 0.0f));
			surf->SetTextureUVs(0, 1, Point2(0.5f, 0.0f));
			surf->SetTextureUVs(0, 2, Point2(1.0f, 1.0f));
			surf->SetTextureUVs(0, 3, Point2(0.0f, 1.0f));
			break;
		case 1:
			surf->SetTextureUVs(0, 0, Point2(s1, 1.0f));
			surf->SetTextureUVs(0, 1, Point2(0.0f, 1.0f));
			surf->SetTextureUVs(0, 2, Point2(s1, 0.0f));
			surf->SetTextureUVs(0, 3, Point2(0.0f, 0.0f));
			break;
		case 2:
			surf->SetTextureUVs(0, 0, Point2(s3, 1.0f));
			surf->SetTextureUVs(0, 1, Point2(s1, 1.0f));
			surf->SetTextureUVs(0, 2, Point2(s3, 0.0f));
			surf->SetTextureUVs(0, 3, Point2(s1, 0.0f));
			break;
		case 3:
			surf->SetTextureUVs(0, 0, Point2(1.0f, 1.0f));
			surf->SetTextureUVs(0, 1, Point2(s3, 1.0f));
			surf->SetTextureUVs(0, 2, Point2(1.0f, 0.0f));
			surf->SetTextureUVs(0, 3, Point2(s3, 0.0f));
			break;
		case 4:
			surf->SetTextureUVs(0, 0, Point2(0.5f, 1.0f));
			surf->SetTextureUVs(0, 1, Point2(0.5f, 1.0f));
			surf->SetTextureUVs(0, 2, Point2(1.0f, 0.0f));
			surf->SetTextureUVs(0, 3, Point2(0.0f, 0.0f));
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
	// Bottom(0) to Front (1)
	F(0, 1, 3, 0, 0, 0);
	F(0, 1, 2, 0, 1, 0);
	F(0, 1, 1, 0, 2, 0);
	F(0, 1, 0, 0, 3, 0);

	// Bottom(0) to Left (2)
	F(0, 2, 3, 0, 3, 0);
	F(0, 2, 3, 1, 2, 0);
	F(0, 2, 3, 2, 1, 0);
	F(0, 2, 3, 3, 0, 0);

	// Bottom(0) to Right (3)
	F(0, 3, 0, 0, 0, 0);
	F(0, 3, 0, 1, 1, 0);
	F(0, 3, 0, 2, 2, 0);
	F(0, 3, 0, 3, 3, 0);

	// Top(4) to Front (1)
	F(4, 1, 3, 0, 3, 3);
	F(4, 1, 2, 0, 2, 3);
	F(4, 1, 1, 0, 1, 3);
	F(4, 1, 0, 0, 0, 3);

	// Top(4) to Left (2)
	F(4, 2, 0, 0, 3, 3);
	F(4, 2, 0, 1, 2, 3);
	F(4, 2, 0, 2, 1, 3);
	F(4, 2, 0, 3, 0, 3);

	// Top(4) to Right (3)
	F(4, 3, 3, 0, 0, 3);
	F(4, 3, 3, 1, 1, 3);
	F(4, 3, 3, 2, 2, 3);
	F(4, 3, 3, 3, 3, 3);

	// Front(1) to Left (2)
	F(1, 2, 0, 1, 3, 1);
	F(1, 2, 0, 2, 3, 2);

	// Left(2) to Right (3)
	F(2, 3, 0, 1, 3, 1);
	F(2, 3, 0, 2, 3, 2);

	// Right(3) to Front (1)
	F(3, 1, 0, 1, 3, 1);
	F(3, 1, 0, 2, 3, 2);

	// Fuse the triangles together
	for (int i = 1; i < 4; i++) {
		F(0, 0, 0, 3, i, 3);
		F(4, 4, 0, 3, i, 3);
	}

	Matrix3 mat;
	mat.IdentityMatrix();
	Object *obj = CreateNURBSObject(NULL, &nset, mat);
	return obj;
}

Object* PrismObject::ConvertToType(TimeValue t, Class_ID obtype)
{
	if (obtype == EDITABLE_SURF_CLASS_ID) {
		Interval valid = FOREVER;
		float side1, side2, side3, height;
		int genUVs;
		pblock2->GetValue(prism_side1, t, side1, valid);
		pblock2->GetValue(prism_side2, t, side2, valid);
		pblock2->GetValue(prism_side3, t, side3, valid);
		pblock2->GetValue(prism_height, t, height, valid);
		pblock2->GetValue(prism_mapping, t, genUVs, valid);
		Object *ob = BuildNURBSPrism(side1, side2, side3, height, genUVs);
		ob->SetChannelValidity(TOPO_CHAN_NUM, valid);
		ob->SetChannelValidity(GEOM_CHAN_NUM, valid);
		ob->UnlockObject();
		return ob;
	}

	return __super::ConvertToType(t, obtype);
}

int PrismObject::CanConvertToType(Class_ID obtype)
{
	if (obtype == EDITABLE_SURF_CLASS_ID)
		return 1;
	if (obtype == triObjectClassID)
		return 1;

	return __super::CanConvertToType(obtype);
}

void PrismObject::GetCollapseTypes(Tab<Class_ID> &clist, Tab<TSTR*> &nlist)
{
	__super::GetCollapseTypes(clist, nlist);
	Class_ID id = EDITABLE_SURF_CLASS_ID;
	TSTR *name = new TSTR(GetString(IDS_SM_NURBS_SURFACE));
	clist.Append(1, &id);
	nlist.Append(1, &name);
}

class PrismObjCreateCallBack : public CreateMouseCallBack {
	PrismObject *ob;
	Point3 p0, p1, p2, tmp, d;
	IPoint2 sp0, sp1, sp2;
	float l, hd;
public:
	int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat);
	void SetObj(PrismObject *obj) { ob = obj; }
};

int PrismObjCreateCallBack::proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat)
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
		{
			ob->suspendSnap = TRUE;
			sp0 = m;
			p0 = vpt->SnapPoint(m, m, NULL, snapdim);
			mat.SetTrans(p0); // Set Node's transform				
			ob->pblock2->SetValue(prism_side1, 0, 0.01f);
			ob->pblock2->SetValue(prism_side2, 0, 0.01f);
			ob->pblock2->SetValue(prism_side3, 0, 0.01f);
			ob->pblock2->SetValue(prism_height, 0, 0.01f);
			break;
		}
		case 1:
		{
			mat.IdentityMatrix();
			sp1 = m;
			p1 = vpt->SnapPoint(m, m, NULL, snapdim);
			mat.SetTrans(p0);
			d = p1 - p0;
			if (flags&MOUSE_CTRL) {
				// Constrain to square base
				float len;
				if (fabs(d.x) > fabs(d.y)) len = d.x;
				else len = d.y;
				d.x = d.y = 2.0f * len;
			}
			hd = d.x, l = (float)sqrt(hd*hd + d.y*d.y);
			float dl = 0.95f*(l + l);
			hd = (float)fabs(d.x);
			if (hd > dl) hd = dl;
			dl = 0.95f*(hd + l);
			if (l > dl) l = dl;
			ob->pblock2->SetValue(prism_side1, 0, hd);
			ob->pblock2->SetValue(prism_side2, 0, l);
			ob->pblock2->SetValue(prism_side3, 0, l);
			if (msg == MOUSE_POINT)
			{
				if (Length(sp1 - sp0) < 3 )
				{
					return CREATE_ABORT;
				}
				else { tmp = (p2 = p0 + Point3(hd / 2.0f, l, 0.0f)); }
			}
			break;
		}
		case 2:
			if (!prism_crtype_blk.GetInt(prism_create_meth))
			{
				float h = vpt->SnapLength(vpt->GetCPDisp(p1, Point3(0, 0, 1), sp1, m, TRUE));
				ob->pblock2->SetValue(prism_height, 0, h);

				if (msg == MOUSE_POINT)
				{
					ob->suspendSnap = FALSE;
					return (Length(m - sp0) < 3) ? CREATE_ABORT : CREATE_STOP;
				}
			}
			else
			{
				sp2 = m;
				Point3 newpt = vpt->SnapPoint(m, m, NULL, snapdim);
				p2 = tmp + (newpt - p1);
				d = p2 - p0;
				float l2, dl, l3, l1;
				l1 = hd;
				l3 = (float)sqrt(d.x*d.x + d.y*d.y);
				Point3 midpt = p0 + Point3(hd, 0.0f, 0.0f);
				d = p2 - midpt; l2 = (float)sqrt(d.x*d.x + d.y*d.y);
				dl = 0.95f*(l2 + l3);
				if (l1 > dl)
				{
					l1 = dl;
					ob->pblock2->SetValue(prism_side1, 0, (hd = l1));
				}
				dl = 0.95f*(l1 + l3);
				if (l2 > dl) l2 = dl;
				ob->pblock2->SetValue(prism_side2, 0, l2);
				dl = 0.95f*(l1 + l2);
				if (l3 > dl) l3 = dl;
				ob->pblock2->SetValue(prism_side3, 0, l3);
				if (msg == MOUSE_POINT)
				{
					if (Length(m - sp0) < 3) CREATE_ABORT;
				}
			}
			break;

		case 3:
			float h = vpt->SnapLength(vpt->GetCPDisp(p1, Point3(0, 0, 1), sp2, m, TRUE));
			ob->pblock2->SetValue(prism_height, 0, h);
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
			return CREATE_ABORT;
	}
	return 1;
}

static PrismObjCreateCallBack cylCreateCB;

CreateMouseCallBack* PrismObject::GetCreateMouseCallBack()
{
	cylCreateCB.SetObj(this);
	return(&cylCreateCB);
}

BOOL PrismObject::OKtoDisplay(TimeValue t)
{
	float side1;
	pblock2->GetValue(prism_side1, t, side1, FOREVER);
	if (side1 == 0.0f) return FALSE;
	else return TRUE;
}

void PrismObject::InvalidateUI()
{
	prism_param_blk.InvalidateUI(pblock2->LastNotifyParamID());
}

RefTargetHandle PrismObject::Clone(RemapDir& remap)
{
	PrismObject* newob = new PrismObject();
	newob->ReplaceReference(0, remap.CloneRef(pblock2));
	newob->ivalid.SetEmpty();
	BaseClone(this, newob, remap);
	return(newob);
}


