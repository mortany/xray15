/**********************************************************************
 *<
	FILE: patchgrd.cpp

	DESCRIPTION:  A Quad Patch Grid object implementation

	CREATED BY: Tom Hudson

	HISTORY: created 22 June 1995

 *>	Copyright (c) 1995, All Rights Reserved.
 **********************************************************************/

#include "prim.h"
#include "iparamm.h"
#include "surf_api.h"
#include "tessint.h"
#include "MeshDelta.h"
#include "Graphics/IDisplayManager.h"
#include "Graphics/IMeshDisplay2.h"

// Parameter block indices
#define PB_LENGTH	0
#define PB_WIDTH	1
#define PB_WSEGS	2
#define PB_LSEGS	3
#define PB_TEXTURE	4

// Non-parameter block indices
#define PB_TI_POS			0
#define PB_TI_LENGTH		1
#define PB_TI_WIDTH			2

class QuadPatchCreateCallBack;

#define BMIN_LENGTH		float(0)
#define BMAX_LENGTH		float(1.0E30)
#define BMIN_WIDTH		float(0)
#define BMAX_WIDTH		float(1.0E30)

#define BDEF_DIM		float(0)
#define BDEF_SEGS		1
#define BMIN_SEGS		1
#define BMAX_SEGS		100

class QuadPatchObject : public GeomObject, public IParamArray {			   
	friend class QuadPatchCreateCallBack;
	friend INT_PTR CALLBACK QuadPatchParamDialogProc( HWND hDlg, UINT message, 
		WPARAM wParam, LPARAM lParam );
	
	public:
		// Object parameters		
		IParamBlock *pblock;
		Interval ivalid;
		int creating;

		// Class vars
		static IParamMap *pmapTypeIn;
		static IParamMap *pmapParam;
		static IObjParam *ip;
		static int dlgLSegs;
		static int dlgWSegs;
		static BOOL dlgTexture;
		static Point3 crtPos;		
		static float crtWidth, crtLength;
		static QuadPatchObject *editOb;

		// Caches
		PatchMesh patch;

		//  inherited virtual methods for Reference-management
		RefResult NotifyRefChanged( const Interval& changeInt, RefTargetHandle hTarget, 
		   PartID& partID, RefMessage message, BOOL propagate );
		void BuildPatch(TimeValue t,PatchMesh& amesh);
		void GetBBox(TimeValue t, Matrix3 &tm, Box3& box);

		QuadPatchObject();
		~QuadPatchObject();

		void InvalidateUI();
		void PatchMeshInvalid() { ivalid.SetEmpty(); }

		//  inherited virtual methods:		

		// From BaseObject
		int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);
		void Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt);
		int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
		Mesh* GetRenderMesh(TimeValue t, INode *inode, View& view, BOOL& needDelete);
		void PrepareMesh(TimeValue t);
		CreateMouseCallBack* GetCreateMouseCallBack();
		void BeginEditParams( IObjParam *ip, ULONG flags, Animatable *prev );
		void EndEditParams( IObjParam *ip, ULONG flags, Animatable *next );
		const TCHAR *GetObjectName() { return GetString(IDS_TH_QUADPATCH); }

		// From Object
		ObjectState Eval(TimeValue time);
		void InitNodeName(TSTR& s) { s = GetString(IDS_TH_QUADPATCH); }		
		Interval ObjectValidity(TimeValue t);
		int CanConvertToType(Class_ID obtype);
		Object* ConvertToType(TimeValue t, Class_ID obtype);
		
		// From GeomObject
		int IntersectRay(TimeValue t, Ray& r, float& at, Point3& norm);
		void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box );
		void GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vxt, Box3& box );
		void GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm=NULL,BOOL useSel=FALSE );

		PatchMesh& GetPatchMesh(TimeValue t);
		void UpdatePatchMesh(TimeValue t);

		// Animatable methods
		void DeleteThis() { delete this; }
		void FreeCaches(); 
		Class_ID ClassID() { return Class_ID( PATCHGRID_CLASS_ID, 0); }  
		void GetClassName(TSTR& s) { s = GetString(IDS_TH_QUADPATCHOBJECT_CLASS); }		
		
		int NumSubs() { return 1; }  
		Animatable* SubAnim(int i) { return pblock; }
		TSTR SubAnimName(int i) { return GetString(IDS_TH_PARAMETERS);}		
		int IsKeyable() { return 1;}
		BOOL BypassTreeView() { return FALSE; }

		// From ref
		RefTargetHandle Clone(RemapDir& remap);
		int NumRefs() {return 1;}
		RefTargetHandle GetReference(int i) {return pblock;}
private:
		virtual void SetReference(int i, RefTargetHandle rtarg) {pblock=(IParamBlock*)rtarg;}
public:

		// IO
		IOResult Load(ILoad *iload);

		// From IParamArray
		BOOL SetValue(int i, TimeValue t, int v);
		BOOL SetValue(int i, TimeValue t, float v);
		BOOL SetValue(int i, TimeValue t, Point3 &v);
		BOOL GetValue(int i, TimeValue t, int &v, Interval &ivalid);
		BOOL GetValue(int i, TimeValue t, float &v, Interval &ivalid);
		BOOL GetValue(int i, TimeValue t, Point3 &v, Interval &ivalid);

		ParamDimension *GetParameterDim(int pbIndex);
		TSTR GetParameterName(int pbIndex);

		// Automatic texture support
		BOOL HasUVW();
		void SetGenUVW(BOOL sw);
		
		//from IObjectDisplay2
		unsigned long GetObjectDisplayRequirement() const ;
		bool PrepareDisplay(
			const MaxSDK::Graphics::UpdateDisplayContext& prepareDisplayContext);
		bool UpdatePerNodeItems(
			const MaxSDK::Graphics::UpdateDisplayContext& updateDisplayContext,
			MaxSDK::Graphics::UpdateNodeContext& nodeContext,
			MaxSDK::Graphics::IRenderItemContainer& targetRenderItemContainer);
	};				

//------------------------------------------------------

class QuadPatchClassDesc:public ClassDesc {
	public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new QuadPatchObject; }
	const TCHAR *	ClassName() { return GetString(IDS_TH_QUAD_PATCH_CLASS); }
	SClass_ID		SuperClassID() { return GEOMOBJECT_CLASS_ID; }
	Class_ID		ClassID() { return Class_ID(PATCHGRID_CLASS_ID,0); }
	const TCHAR* 	Category() { return GetString(IDS_TH_PATCH_GRIDS);  }
	void			ResetClassParams(BOOL fileReset);
	};

static QuadPatchClassDesc quadPatchDesc;

ClassDesc* GetQuadPatchDesc() { return &quadPatchDesc; }

// in prim.cpp  - The dll instance handle
extern HINSTANCE hInstance;

// class variable for sphere class.
IParamMap *QuadPatchObject::pmapParam  = NULL;
IParamMap *QuadPatchObject::pmapTypeIn = NULL;
IObjParam *QuadPatchObject::ip;
int QuadPatchObject::dlgLSegs = BDEF_SEGS;
int QuadPatchObject::dlgWSegs = BDEF_SEGS;
BOOL QuadPatchObject::dlgTexture = TRUE;
Point3 QuadPatchObject::crtPos         = Point3(0,0,0);		
float QuadPatchObject::crtWidth        = 0.0f; 
float QuadPatchObject::crtLength       = 0.0f;
QuadPatchObject *QuadPatchObject::editOb = NULL;

void QuadPatchClassDesc::ResetClassParams(BOOL fileReset)
	{
	QuadPatchObject::dlgLSegs = BDEF_SEGS;
	QuadPatchObject::dlgWSegs = BDEF_SEGS;
	QuadPatchObject::dlgTexture = FALSE;
	QuadPatchObject::crtWidth   = 0.0f; 
	QuadPatchObject::crtLength  = 0.0f;
	QuadPatchObject::crtPos     = Point3(0,0,0);
	}

//
//
// Type in

static ParamUIDesc descTypeIn[] = {
	
	// Position
	ParamUIDesc(
		PB_TI_POS,
		EDITTYPE_UNIVERSE,
		IDC_TI_POSX,IDC_TI_POSXSPIN,
		IDC_TI_POSY,IDC_TI_POSYSPIN,
		IDC_TI_POSZ,IDC_TI_POSZSPIN,
		-99999999.0f,99999999.0f,
		SPIN_AUTOSCALE),
	
	// Length
	ParamUIDesc(
		PB_TI_LENGTH,
		EDITTYPE_UNIVERSE,
		IDC_LENGTHEDIT,IDC_LENSPINNER,
		BMIN_LENGTH,BMAX_LENGTH,
		SPIN_AUTOSCALE),	

	// Width
	ParamUIDesc(
		PB_TI_WIDTH,
		EDITTYPE_UNIVERSE,
		IDC_WIDTHEDIT,IDC_WIDTHSPINNER,
		BMIN_WIDTH,BMAX_WIDTH,
		SPIN_AUTOSCALE),
			
	};
#define TYPEINDESC_LENGTH 3

//
//
// Parameters

static ParamUIDesc descParam[] = {
	// Length
	ParamUIDesc(
		PB_LENGTH,
		EDITTYPE_UNIVERSE,
		IDC_LENGTHEDIT,IDC_LENSPINNER,
		BMIN_LENGTH,BMAX_LENGTH,
		SPIN_AUTOSCALE),	
	
	// Width
	ParamUIDesc(
		PB_WIDTH,
		EDITTYPE_UNIVERSE,
		IDC_WIDTHEDIT,IDC_WIDTHSPINNER,
		BMIN_WIDTH,BMAX_WIDTH,
		SPIN_AUTOSCALE),	
	
	// Length Segments
	ParamUIDesc(
		PB_LSEGS,
		EDITTYPE_INT,
		IDC_LSEGS,IDC_LSEGSPIN,
		(float)BMIN_SEGS,(float)BMAX_SEGS,
		0.1f),
	
	// Width Segments
	ParamUIDesc(
		PB_WSEGS,
		EDITTYPE_INT,
		IDC_WSEGS,IDC_WSEGSPIN,
		(float)BMIN_SEGS,(float)BMAX_SEGS,
		0.1f),
	
	// Gen UVs
	ParamUIDesc(PB_TEXTURE,TYPE_SINGLECHECKBOX,IDC_GENTEXTURE),			
	};
#define PARAMDESC_LENGTH 5


static ParamBlockDescID descVer0[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0 },		
	{ TYPE_FLOAT, NULL, TRUE, 1 },
	{ TYPE_INT, NULL, TRUE, 2 },
	{ TYPE_INT, NULL, TRUE, 3 },
 };
static ParamBlockDescID descVer1[] = {
	{ TYPE_FLOAT, NULL, TRUE, 0 },		
	{ TYPE_FLOAT, NULL, TRUE, 1 },
	{ TYPE_INT, NULL, TRUE, 2 },
	{ TYPE_INT, NULL, TRUE, 3 },
	{ TYPE_INT, NULL, FALSE, 4 },
 };
#define PBLOCK_LENGTH	5

// Array of old versions
static ParamVersionDesc versions[] = {
	ParamVersionDesc(descVer0,4,0)			
	};
#define NUM_OLDVERSIONS	1	

// Current version
#define CURRENT_VERSION	1
static ParamVersionDesc curVersion(descVer1,PBLOCK_LENGTH,CURRENT_VERSION);

//--- TypeInDlgProc --------------------------------

class QuadPatchTypeInDlgProc : public ParamMapUserDlgProc {
	public:
		QuadPatchObject *ob;

		QuadPatchTypeInDlgProc(QuadPatchObject *o) {ob=o;}
		INT_PTR DlgProc(TimeValue t,IParamMap *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);
		void DeleteThis() {delete this;}
	};

INT_PTR QuadPatchTypeInDlgProc::DlgProc(
		TimeValue t,IParamMap *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
	{
	switch (msg) {
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_TI_CREATE: {					
					if (ob->crtLength==0.0) return TRUE;
					if (ob->crtWidth==0.0) return TRUE;

					// Return focus to the top spinner
					SetFocus(GetDlgItem(hWnd, IDC_TI_POSX));

					// We only want to set the value if the object is 
					// not in the scene.
					if (ob->TestAFlag(A_OBJ_CREATING)) {
						ob->pblock->SetValue(PB_LENGTH,0,ob->crtLength);
						ob->pblock->SetValue(PB_WIDTH,0,ob->crtWidth);
						}

					Matrix3 tm(1);
					tm.SetTrans(ob->crtPos);					
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

void QuadPatchObject::BeginEditParams( IObjParam *ip, ULONG flags, Animatable *prev )
	{
	editOb = this;
	this->ip = ip;
	
	if (pmapTypeIn && pmapParam) {
		
		// Left over from last shape ceated
		pmapTypeIn->SetParamBlock(this);
		pmapParam->SetParamBlock(pblock);
	} else {
		
		// Gotta make a new one.
		if (flags & BEGIN_EDIT_CREATE) {
			pmapTypeIn = CreateCPParamMap(
				descTypeIn,TYPEINDESC_LENGTH,
				this,
				ip,
				hInstance,
				MAKEINTRESOURCE(IDD_PATCHGRIDPARAM2),
				GetString(IDS_TH_KEYBOARD_ENTRY),
				APPENDROLL_CLOSED);
			}

		pmapParam = CreateCPParamMap(
			descParam,PARAMDESC_LENGTH,
			pblock,
			ip,
			hInstance,
			MAKEINTRESOURCE(IDD_PATCHGRIDPARAM),
			GetString(IDS_TH_PARAMETERS),
			0);
		}

	if(pmapTypeIn) {
		// A callback for the type in.
		pmapTypeIn->SetUserDlgProc(new QuadPatchTypeInDlgProc(this));
		}
	}

void QuadPatchObject::EndEditParams( IObjParam *ip, ULONG flags, Animatable *next )
{
	editOb = NULL;
	this->ip = NULL;

	if (flags&END_EDIT_REMOVEUI ) {
		if (pmapTypeIn) DestroyCPParamMap(pmapTypeIn);
		DestroyCPParamMap(pmapParam);
		pmapParam  = NULL;
		pmapTypeIn = NULL;
	}
	else
	{
		pmapTypeIn->SetUserDlgProc(nullptr);
		pmapTypeIn->SetParamBlock(nullptr);
		pmapParam->SetParamBlock(nullptr);
	}

	// Save these values in class variables so the next object created will inherit them.
	pblock->GetValue(PB_LSEGS,ip->GetTime(),dlgLSegs,FOREVER);
	pblock->GetValue(PB_WSEGS,ip->GetTime(),dlgWSegs,FOREVER);	
	pblock->GetValue(PB_TEXTURE,ip->GetTime(),dlgTexture,FOREVER);	
}

PatchMesh &QuadPatchObject::GetPatchMesh(TimeValue t) {
	UpdatePatchMesh(t);
	return patch;
	}

void QuadPatchObject::UpdatePatchMesh(TimeValue t) {
	if ( ivalid.InInterval(t) )
		return;
	BuildPatch(t,patch);
	}

void QuadPatchObject::FreeCaches() {
	ivalid.SetEmpty();
	patch.FreeAll();
	}

// Quad patch layout
//
//   A---> ad ----- da <---D
//   |                     |
//   |                     |
//   v                     v
//   ab    i1       i4     dc
//
//   |                     |
//   |                     |
// 
//   ba    i2       i3     cd
//   ^					   ^
//   |                     |
//   |                     |
//   B---> bc ----- cb <---C
//
// vertices ( a b c d ) are in counter clockwise order when viewed from 
// outside the surface

void QuadPatchObject::BuildPatch(TimeValue t,PatchMesh& amesh)
	{
	int ix,iy,np,kv;
	int wsegs,lsegs,nv;
	Point3 v,p;
	float l, w;
	int tex;
	
	// Start the validity interval at forever and widdle it down.
	ivalid = FOREVER;
	pblock->GetValue( PB_LENGTH, t, l, ivalid );
	pblock->GetValue( PB_WIDTH, t, w, ivalid );
	pblock->GetValue( PB_LSEGS, t, lsegs, ivalid );
	pblock->GetValue( PB_WSEGS, t, wsegs, ivalid );
	pblock->GetValue( PB_TEXTURE, t, tex, ivalid );

	LimitValue(lsegs, BMIN_SEGS, BMAX_SEGS);
	LimitValue(wsegs, BMIN_SEGS, BMAX_SEGS);

	int lv = lsegs + 1;
	int wv = wsegs + 1;

	int nverts = lv * wv;
	int npatches = lsegs * wsegs;
	int nexteriors = npatches * 4 + lsegs * 2 + wsegs * 2;
	int ninteriors = npatches * 4;
	int nvecs = ninteriors + nexteriors;

	amesh.setNumVerts(nverts);
	amesh.setNumTVerts(tex ? nverts : 0);
	amesh.setNumVecs(nvecs);
	amesh.setNumPatches(npatches);
	amesh.setNumTVPatches(tex ? npatches : 0);

	v =  Point3(-w, -l, 0.0f) / 2.0f;   

	float dx = w/wsegs;
	float dy = l/lsegs;
	float fws = (float)wsegs;
	float fls = (float)lsegs;

	// Create the vertices.
	nv = 0;
	p.z = v.z;
	p.y = v.y;
	for(iy=0; iy<=lsegs; iy++) {
		p.x = v.x;
		for (ix=0; ix<=wsegs; ix++) {
			if(tex)
				amesh.setTVert(nv, UVVert((float)ix / fws, (float)iy / fls, 0.0f));
			amesh.verts[nv].flags = PVERT_COPLANAR;
			amesh.setVert(nv++, p);
			p.x += dx;
			}
		p.y += dy;
		}

	// Create patches.
	np = 0;
	int interior = nexteriors;
	int vecRowInc = lsegs * 2;
	int vecColInc = wsegs * 2;
	for(iy=0; iy<lsegs; iy++) {
		kv = iy*(wsegs+1);
		int rv = iy * vecColInc;	// Row vector start
		int cv = vecColInc * lv + iy * 2;	// column vector start
		for (ix=0; ix<wsegs; ix++,++np) {
			Patch &p = amesh.patches[np];
			int a = kv, b = kv+1, c = kv+wsegs+2, d = kv + wsegs + 1;
			int ab = rv, ba = rv+1;
			int bc = cv+vecRowInc, cb = cv + vecRowInc + 1;
			int cd = rv+vecColInc+1, dc = rv+vecColInc;
			int da = cv + 1, ad = cv;
			amesh.MakeQuadPatch(np, a, ab, ba, b, bc, cb, c, cd, dc, d, da, ad, interior, interior+1, interior+2, interior+3, 1);
			if(tex)
				amesh.getTVPatch(np).setTVerts(a,b,c,d);
			// Create the default vectors
			Point3 pa = amesh.getVert(a).p;
			Point3 pb = amesh.getVert(b).p;
			Point3 pc = amesh.getVert(c).p;
			Point3 pd = amesh.getVert(d).p;
			amesh.setVec(ab, pa + (pb - pa) / 3.0f);
			amesh.setVec(ba, pb - (pb - pa) / 3.0f);
			amesh.setVec(bc, pb + (pc - pb) / 3.0f);
			amesh.setVec(cb, pc - (pc - pb) / 3.0f);
			amesh.setVec(cd, pc + (pd - pc) / 3.0f);
			amesh.setVec(dc, pd - (pd - pc) / 3.0f);
			amesh.setVec(da, pd + (pa - pd) / 3.0f);
			amesh.setVec(ad, pa - (pa - pd) / 3.0f);
			kv++;
			cv += vecRowInc;
			rv += 2;
			interior += 4;
			}
		}
	// Verify that we have the right number of parts!
	assert(np==npatches);
	assert(nv==nverts);
	// Finish up patch internal linkages (and bail out if it fails!)
	if(!amesh.buildLinkages())
   {
      assert(0);
   }
	// Calculate the interior bezier points on the PatchMesh's patches
	amesh.computeInteriors();
	amesh.InvalidateGeomCache();
	// Tell the PatchMesh it just got changed
	amesh.InvalidateMesh();
	}

QuadPatchObject::QuadPatchObject() 
	{
	pblock = NULL;
	ReplaceReference(0, CreateParameterBlock(descVer1, PBLOCK_LENGTH, CURRENT_VERSION));

	pblock->SetValue(PB_LSEGS,0,dlgLSegs);
	pblock->SetValue(PB_WSEGS,0,dlgWSegs);
	pblock->SetValue(PB_LENGTH,0,crtLength);
	pblock->SetValue(PB_WIDTH,0,crtWidth);
	pblock->SetValue(PB_TEXTURE,0,dlgTexture);

	ivalid.SetEmpty();
	creating = 0;	
	}

QuadPatchObject::~QuadPatchObject()
	{
	DeleteAllRefsFromMe();
	pblock = NULL;
	}

class QuadPatchCreateCallBack: public CreateMouseCallBack {
	QuadPatchObject *ob;
	Point3 p0,p1;
	IPoint2 sp0;
	public:
		int proc( ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat );
		void SetObj(QuadPatchObject *obj) { ob = obj; }
	};

int QuadPatchCreateCallBack::proc(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat ) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	
	Point3 d;

	if (msg == MOUSE_FREEMOVE)
	{
			vpt->SnapPreview(m,m,NULL, SNAP_IN_3D);
	}

	if (msg==MOUSE_POINT||msg==MOUSE_MOVE) {
		switch(point) {
			case 0:
				sp0 = m;
				ob->creating = 1;	// tell object we're building it so we can disable snapping to itself
					p0 = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);
				p1 = p0 + Point3(0.01f,0.01f,0.0f);
				mat.SetTrans(float(.5)*(p0+p1));				
				ob->pblock->SetValue(PB_WIDTH,0,0.0f);
				ob->pblock->SetValue(PB_LENGTH,0,0.0f);
				ob->pmapParam->Invalidate();
				break;
			case 1:
					p1 = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);
				mat.SetTrans(float(.5)*(p0+p1));
				d = p1-p0;
				ob->pblock->SetValue(PB_WIDTH,0,float(fabs(d.x)));
				ob->pblock->SetValue(PB_LENGTH,0,float(fabs(d.y)));
				ob->pmapParam->Invalidate();										
				if (msg==MOUSE_POINT) {
					ob->creating = 0;
					return (Length(m-sp0)<3 ) ? CREATE_ABORT: CREATE_STOP;
					}
				break;
			}
		}
	else
	if (msg == MOUSE_ABORT) {
		ob->creating = 0;
		return CREATE_ABORT;
		}

	return TRUE;
	}

static QuadPatchCreateCallBack patchCreateCB;

CreateMouseCallBack* QuadPatchObject::GetCreateMouseCallBack() {
	patchCreateCB.SetObj(this);
	return(&patchCreateCB);
	}

// From BaseObject
int QuadPatchObject::HitTest(TimeValue t, INode *inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt) {	
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	
	HitRegion hitRegion;
	GraphicsWindow *gw = vpt->getGW();	
	Material *mtl = gw->getMaterial();
   	
	UpdatePatchMesh(t);
	gw->setTransform(inode->GetObjectTM(t));

	MakeHitRegion(hitRegion, type, crossing, 4, p);
	return patch.select( gw, mtl, &hitRegion, flags & HIT_ABORTONHIT );
	}

void QuadPatchObject::Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return;
	}
	
	if(creating)	// If creating this one, don't try to snap to it!
		return;

	Matrix3 tm = inode->GetObjectTM(t);	
	GraphicsWindow *gw = vpt->getGW();	
   	
	UpdatePatchMesh(t);
	gw->setTransform(tm);

	patch.snap( gw, snap, p, tm );
	}

int QuadPatchObject::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) {
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	
	Matrix3 tm;
	GraphicsWindow *gw = vpt->getGW();
	gw->setTransform(inode->GetObjectTM(t));
	UpdatePatchMesh(t);

	if(!MaxSDK::Graphics::IsRetainedModeEnabled())
	{
		if(!(gw->getRndMode() & GW_BOX_MODE)) {
			PrepareMesh(t);
			Mesh& mesh = patch.GetMesh();
			if(mesh.getNumVerts()) {
				mesh.render( gw, inode->Mtls(),
					(flags&USE_DAMAGE_RECT) ? &vpt->GetDammageRect() : NULL, 
					COMP_ALL | (inode->Selected()?COMP_OBJSELECTED:0), inode->NumMtls());	
				}
			}
	}

	patch.render( gw, inode->Mtls(),
		(flags&USE_DAMAGE_RECT) ? &vpt->GetDammageRect() : NULL, 
		COMP_ALL | (inode->Selected()?COMP_OBJSELECTED:0), inode->NumMtls());	
	return(0);
	}

// from IDisplay
unsigned long QuadPatchObject::GetObjectDisplayRequirement() const 
{
	return MaxSDK::Graphics::ObjectDisplayRequireLegacyDisplayMode;
}
bool QuadPatchObject::PrepareDisplay(
	const MaxSDK::Graphics::UpdateDisplayContext& prepareDisplayContext)
{
	PrepareMesh(prepareDisplayContext.GetDisplayTime());
	Mesh& mesh = patch.GetMesh();
	if(mesh.getNumVerts()>0)
	{
		using namespace MaxSDK::Graphics;

		mRenderItemHandles.ClearAllRenderItems();		
		IMeshDisplay2* pMeshDisplay = static_cast<IMeshDisplay2*>(mesh.GetInterface(IMesh_DISPLAY2_INTERFACE_ID));
		if (NULL == pMeshDisplay)
		{
			return false;
		}

		GenerateMeshRenderItemsContext generateMeshRenderItemsContext;
		generateMeshRenderItemsContext.GenerateDefaultContext(prepareDisplayContext);
		pMeshDisplay->PrepareDisplay(generateMeshRenderItemsContext);

		return true ;
	}

	return false;
}
bool QuadPatchObject::UpdatePerNodeItems(
	const MaxSDK::Graphics::UpdateDisplayContext& updateDisplayContext,
	MaxSDK::Graphics::UpdateNodeContext& nodeContext,
	MaxSDK::Graphics::IRenderItemContainer& targetRenderItemContainer)
{
	Mesh& mesh = patch.GetMesh();
	if(mesh.getNumVerts()>0)
	{
		using namespace MaxSDK::Graphics;

		GenerateMeshRenderItemsContext generateRenderItemsContext;
		generateRenderItemsContext.GenerateDefaultContext(updateDisplayContext);
		generateRenderItemsContext.RemoveInvisibleMeshElementDescriptions(nodeContext.GetRenderNode());

		IMeshDisplay2* pMeshDisplay = static_cast<IMeshDisplay2*>(mesh.GetInterface(IMesh_DISPLAY2_INTERFACE_ID));
		if (NULL == pMeshDisplay)
		{
			return false;
		}

		pMeshDisplay->GetRenderItems(generateRenderItemsContext,nodeContext,targetRenderItemContainer);

		return true ;
	}

	return false;
}
//////////////////////////////////  MESH WELDER ////////////////////
static void
WeldMesh(Mesh *mesh, float thresh)
{
	if (thresh == 0.0f)
		thresh = (float)1e-30; // find only the coincident ones	BitArray vset, eset;
	BitArray vset;
	vset.SetSize(mesh->numVerts);
	vset.SetAll();
	MeshDelta md;
	md.WeldByThreshold(*mesh, vset, thresh);
	md.Apply(*mesh);
}


typedef int (* GTess)(void *obj, SurfaceType type, Matrix3 *otm, Mesh *mesh,
							TessApprox *tess, TessApprox *disp, View *view,
							Mtl* mtl, BOOL dumpMiFile, BOOL splitMesh);
static GTess psGTessFunc = NULL;

// This function get the function to do GAP Tessellation from
// tessint.dll.  This is required because of the link order between
// core.dll and tessint.dll and gmi.dll.  -- Charlie Thaeler
static void
GetGTessFunction()
{
    if (psGTessFunc)
        return;
    // Get the library handle for tessint.dll
    HINSTANCE hInst = NULL;
	hInst = LoadLibraryEx(_T("tessint.dll"), NULL, 0);
    assert(hInst);

    psGTessFunc = (GTess)GetProcAddress(hInst, "GapTessellate");
	assert(psGTessFunc);
}

Mesh* QuadPatchObject::GetRenderMesh(TimeValue t, INode *inode, View& view, BOOL& needDelete) {
	UpdatePatchMesh(t);
	TessApprox tess = patch.GetProdTess();
	if (tess.type == TESS_SET) {
		needDelete = FALSE;
		patch.InvalidateMesh(); // force this...
		// temporarlily set the view tess to prod tess
		TessApprox tempTess = patch.GetViewTess();
		patch.SetViewTess(tess);
		PrepareMesh(t);
		patch.SetViewTess(tempTess);
		return &patch.GetMesh();
	} else {
		Mesh *nmesh = new Mesh/*(mesh)*/;
		Matrix3 otm = inode->GetObjectTM(t);

		Box3 bbox;
		GetDeformBBox(t, bbox);
		tess.merge *= Length(bbox.Width())/1000.0f;
		TessApprox disp = patch.GetDispTess();
		disp.merge *= Length(bbox.Width())/1000.0f;

		GetGTessFunction();
		(*psGTessFunc)(&patch, BEZIER_PATCH, &otm, nmesh, &tess, &disp, &view, inode->GetMtl(), FALSE, FALSE);
		if (tess.merge > 0.0f && patch.GetProdTessWeld())
			WeldMesh(nmesh, tess.merge);
		needDelete = TRUE;
		return nmesh;
	}
}

void QuadPatchObject::PrepareMesh(TimeValue t) {
	UpdatePatchMesh(t);
	patch.PrepareMesh();
	}

// From GeomObject
int QuadPatchObject::IntersectRay(TimeValue t, Ray& r, float& at, Point3& norm) {
	PrepareMesh(t);	// Turn it into a mesh
	return patch.IntersectRay(r, at, norm);
	}

void QuadPatchObject::GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm, BOOL useSel )
	{
	UpdatePatchMesh(t);
	patch.GetDeformBBox(box, tm, useSel);
	}

void QuadPatchObject::GetLocalBoundBox(TimeValue t, INode *inode,ViewExp* /*vpt*/,  Box3& box ) {
	GetDeformBBox(t,box);
	}

void QuadPatchObject::GetWorldBoundBox(TimeValue t, INode *inode, ViewExp* vpt, Box3& box )
	{

	if ( ! vpt || ! vpt->IsAlive() )
	{
		box.Init();
		return;
	}

	Box3	patchBox;

	Matrix3 mat = inode->GetObjectTM(t);
	
	GetLocalBoundBox(t,inode,vpt,patchBox);
	box.Init();
	for(int i = 0; i < 8; i++)
		box += mat * patchBox[i];
	}

//
// Reference Managment:
//

ParamDimension *QuadPatchObject::GetParameterDim(int pbIndex) 
	{
	switch (pbIndex) {
		case PB_LENGTH:return stdWorldDim;
		case PB_WIDTH: return stdWorldDim;
		case PB_WSEGS: return stdSegmentsDim;
		case PB_LSEGS: return stdSegmentsDim;
		default: return defaultDim;
		}
	}

TSTR QuadPatchObject::GetParameterName(int pbIndex) 
	{
	switch (pbIndex) {
		case PB_LENGTH: return GetString(IDS_RB_LENGTH);
		case PB_WIDTH:  return GetString(IDS_RB_WIDTH);
		case PB_WSEGS:  return GetString(IDS_RB_WSEGS);
		case PB_LSEGS:  return GetString(IDS_RB_LSEGS);
		default: return _T("");
		}
	}

// From ParamArray
BOOL QuadPatchObject::SetValue(int i, TimeValue t, int v) 
	{
	return TRUE;
	}

BOOL QuadPatchObject::SetValue(int i, TimeValue t, float v)
	{
	switch (i) {				
		case PB_TI_LENGTH: crtLength = v; break;
		case PB_TI_WIDTH:  crtWidth = v; break;
		}	
	return TRUE;
	}

BOOL QuadPatchObject::SetValue(int i, TimeValue t, Point3 &v) 
	{
	switch (i) {
		case PB_TI_POS: crtPos = v; break;
		}		
	return TRUE;
	}

BOOL QuadPatchObject::GetValue(int i, TimeValue t, int &v, Interval &ivalid) 
	{
	return TRUE;
	}

BOOL QuadPatchObject::GetValue(int i, TimeValue t, float &v, Interval &ivalid) 
	{	
	switch (i) {				
		case PB_TI_LENGTH: v = crtLength; break;
		case PB_TI_WIDTH:  v = crtWidth; break;
		}
	return TRUE;
	}

BOOL QuadPatchObject::GetValue(int i, TimeValue t, Point3 &v, Interval &ivalid) 
	{	
	switch (i) {		
		case PB_TI_POS: v = crtPos; break;		
		}
	return TRUE;
	}

void QuadPatchObject::InvalidateUI() 
	{
	if (pmapParam) pmapParam->Invalidate();
	}

RefResult QuadPatchObject::NotifyRefChanged(
		const Interval& changeInt, 
		RefTargetHandle hTarget, 
   		PartID& partID, 
		RefMessage message, 
		BOOL propagate ) 
   	{
	switch (message) {
		case REFMSG_CHANGE:
			PatchMeshInvalid();
			if (editOb==this) InvalidateUI();
			break;

		case REFMSG_GET_PARAM_DIM: {
			GetParamDim *gpd = (GetParamDim*)partID;
			gpd->dim = GetParameterDim(gpd->index);			
			return REF_HALT; 
			}

		case REFMSG_GET_PARAM_NAME: {
			GetParamName *gpn = (GetParamName*)partID;
			gpn->name = GetParameterName(gpn->index);			
			return REF_HALT; 
			}
		}
	return(REF_SUCCEED);
	}

ObjectState QuadPatchObject::Eval(TimeValue time){
	return ObjectState(this);
	}

Interval QuadPatchObject::ObjectValidity(TimeValue t) {
	UpdatePatchMesh(t);
	return ivalid;	
	}

int QuadPatchObject::CanConvertToType(Class_ID obtype) {
	if (obtype==patchObjectClassID || obtype==defObjectClassID ||
		obtype==mapObjectClassID || obtype==triObjectClassID) {
		return 1;
    }
	if ( obtype == EDITABLE_SURF_CLASS_ID ) {
		return 1;
    }
	if (Object::CanConvertToType (obtype)) return 1;
	if (CanConvertPatchObject (obtype)) return 1;
	return 0;
	}

Object* QuadPatchObject::ConvertToType(TimeValue t, Class_ID obtype) {
	if(obtype == patchObjectClassID || obtype == defObjectClassID ||
			obtype == mapObjectClassID) {
		PatchObject *ob;
		UpdatePatchMesh(t);
		ob = new PatchObject();	
		ob->patch = patch;
		ob->SetChannelValidity(TOPO_CHAN_NUM,ObjectValidity(t));
		ob->SetChannelValidity(GEOM_CHAN_NUM,ObjectValidity(t));
		return ob;
		}

	if(obtype == triObjectClassID) {
		TriObject *ob = CreateNewTriObject();
		PrepareMesh(t);
		ob->GetMesh() = patch.GetMesh();
		ob->SetChannelValidity(TOPO_CHAN_NUM,ObjectValidity(t));
		ob->SetChannelValidity(GEOM_CHAN_NUM,ObjectValidity(t));
		return ob;
		}
	if (obtype==EDITABLE_SURF_CLASS_ID) {
		PatchObject *pob;
		UpdatePatchMesh(t);
		pob = new PatchObject();	
		pob->patch = patch;
		Object *ob = BuildEMObjectFromPatchObject(pob);
		delete pob;
		ob->SetChannelValidity(TOPO_CHAN_NUM, ObjectValidity(t));
		ob->SetChannelValidity(GEOM_CHAN_NUM, ObjectValidity(t));
		return ob;
		}

	if (Object::CanConvertToType (obtype)) {
		return Object::ConvertToType (t, obtype);
	}

	if (CanConvertPatchObject (obtype)) {
		PatchObject *ob;
		UpdatePatchMesh(t);
		ob = new PatchObject();	
		ob->patch = patch;
		ob->SetChannelValidity(TOPO_CHAN_NUM,ObjectValidity(t));
		ob->SetChannelValidity(GEOM_CHAN_NUM,ObjectValidity(t));
		Object *ret = ob->ConvertToType (t, obtype);
		ob->DeleteThis ();
		return ret;
	}

	return NULL;
	}

RefTargetHandle QuadPatchObject::Clone(RemapDir& remap) {
	QuadPatchObject* newob = new QuadPatchObject();
	newob->ReplaceReference(0,remap.CloneRef(pblock));
	newob->ivalid.SetEmpty();	
	BaseClone(this, newob, remap);
	return(newob);
	}

#define GRID_MAPPING_CHUNK 0x1000	// Obsolete chunk (MAX r1.x)

class QuadPatchPostLoadCallback : public PostLoadCallback {
public:
	BOOL tex;
	ParamBlockPLCB *cb;
	QuadPatchPostLoadCallback(ParamBlockPLCB *c) {cb=c; tex=FALSE;}
	void proc(ILoad *iload) {
		if (cb == NULL || !cb->IsValid())
		{
			delete this;
			return;
		}
		ReferenceTarget* targ = cb->GetTarget();
		cb->proc(iload);
		cb = NULL;
		if (tex) {
			// The call to IsValid above verified that the pblock is safe
			((QuadPatchObject*)targ)->pblock->SetValue(PB_TEXTURE,0,1);
		}
		delete this;
	}
	virtual int Priority() { return 0; }
};

// IO
IOResult  QuadPatchObject::Load(ILoad *iload) {
	QuadPatchPostLoadCallback *plcb = new QuadPatchPostLoadCallback(new ParamBlockPLCB(versions,NUM_OLDVERSIONS,&curVersion,this,0));
	iload->RegisterPostLoadCallback(plcb);
	IOResult res;
	while (IO_OK==(res=iload->OpenChunk())) {
		switch(iload->CurChunkID())  {
			case GRID_MAPPING_CHUNK:
				plcb->tex = TRUE;	// Deal with this old switch after loading
				break;
			}
		iload->CloseChunk();
		if (res!=IO_OK) 
			return res;
		}
	return IO_OK;
	}

BOOL QuadPatchObject::HasUVW() { 
	BOOL genUVs;
	Interval v;
	pblock->GetValue(PB_TEXTURE, 0, genUVs, v);
	return genUVs; 
	}

void QuadPatchObject::SetGenUVW(BOOL sw) {  
	if (sw==HasUVW()) return;
	pblock->SetValue(PB_TEXTURE,0, sw);				
	InvalidateUI();
	}
