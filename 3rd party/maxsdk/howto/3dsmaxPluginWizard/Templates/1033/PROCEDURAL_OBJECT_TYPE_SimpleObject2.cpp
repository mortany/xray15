[!output TEMPLATESTRING_COPYRIGHT]

#include "[!output PROJECT_NAME].h"
[!if QT_UI != 0]
#include "QtPluginRollup.h"
[!endif]

#define [!output CLASS_NAME]_CLASS_ID Class_ID([!output CLASSID1], [!output CLASSID2])

#define PBLOCK_REF 0

class [!output CLASS_NAME] : public [!output SUPER_CLASS_NAME]
{
public:
	// Constructor/Destructor
	[!output CLASS_NAME]();
	virtual ~[!output CLASS_NAME]();

	// Parameter block handled by parent

	// From BaseObject
	virtual CreateMouseCallBack* GetCreateMouseCallBack() override;

	// From Object
	virtual BOOL                 HasUVW() override;
	virtual void                 SetGenUVW(BOOL sw) override;
	virtual int                  CanConvertToType(Class_ID obtype) override;
	virtual Object*              ConvertToType(TimeValue t, Class_ID obtype) override;
	virtual void                 GetCollapseTypes(Tab<Class_ID>& clist,Tab<TSTR*>& nlist) override;
	virtual int                  IntersectRay(TimeValue t, Ray& ray, float& at, Point3& norm) override;

	// From Animatable
	virtual void                 BeginEditParams(IObjParam  *ip, ULONG flags,Animatable *prev) override;
	virtual void                 EndEditParams(IObjParam *ip, ULONG flags,Animatable *next) override;

	// From SimpleObject
	virtual void                 BuildMesh(TimeValue t) override;
	virtual void                 InvalidateUI() override;

	// From Animatable
	virtual Class_ID             ClassID() override             { return [!output CLASS_NAME]_CLASS_ID; }
	virtual SClass_ID            SuperClassID() override        { return GEOMOBJECT_CLASS_ID; }
	virtual void                 GetClassName(TSTR& s) override { s = GetString(IDS_CLASS_NAME); }

	virtual RefTargetHandle      Clone(RemapDir& remap) override;

	virtual int                  NumParamBlocks() override              { return 1; } // Return number of ParamBlocks in this instance
	virtual IParamBlock2*        GetParamBlock(int /*i*/) override      { return pblock2; } // Return i'th ParamBlock
	virtual IParamBlock2*        GetParamBlockByID(BlockID id) override { return (pblock2->ID() == id) ? pblock2 : NULL; } // Return id'd ParamBlock

	virtual void                 DeleteThis() override { delete this; }
};


[!output TEMPLATESTRING_CLASSDESC]


[!if PARAM_MAPS != 0]
[!output TEMPLATESTRING_PARAMBLOCKDESC]
[!endif]


//--- [!output CLASS_NAME] -------------------------------------------------------

[!output CLASS_NAME]::[!output CLASS_NAME]()
{
	Get[!output CLASS_NAME]Desc()->MakeAutoParamBlocks(this);
}

[!output CLASS_NAME]::~[!output CLASS_NAME]()
{
}

void [!output CLASS_NAME]::BeginEditParams(IObjParam* ip, ULONG flags, Animatable* prev)
{
	SimpleObject2::BeginEditParams(ip,flags,prev);
	Get[!output CLASS_NAME]Desc()->BeginEditParams(ip, this, flags, prev);
}

void [!output CLASS_NAME]::EndEditParams(IObjParam* ip, ULONG flags, Animatable* next)
{
	// TODO: Save plugin parameter values into class variables, if they are not hosted in ParamBlocks.
	SimpleObject2::EndEditParams(ip,flags,next);
	Get[!output CLASS_NAME]Desc()->EndEditParams(ip, this, flags, next);
}

// From Object
BOOL [!output CLASS_NAME]::HasUVW() 
{ 
	// TODO: Return whether the object has UVW coordinates or not
	return TRUE; 
}

void [!output CLASS_NAME]::SetGenUVW(BOOL sw) 
{
	if (sw==HasUVW()) 
	return;
	// TODO: Set the plugin's internal value to sw
}

// Class for interactive creation of the object using the mouse
class [!output CLASS_NAME]CreateCallBack : public CreateMouseCallBack {
	IPoint2 sp0; // First point in screen coordinates
	[!output CLASS_NAME]* ob; // Pointer to the object 
	Point3 p0; // First point in world coordinates
public:
	int  proc(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat);
	void SetObj([!output CLASS_NAME] *obj) { ob = obj; }
};

int [!output CLASS_NAME]CreateCallBack::proc(ViewExp *vpt,int msg, int point, int /*flags*/, IPoint2 m, Matrix3& mat)
{
	// TODO: Implement the mouse creation code here
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// Why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	if (msg == MOUSE_POINT || msg == MOUSE_MOVE) {
		switch(point) {
		case 0: // Only happens with MOUSE_POINT msg
			ob->suspendSnap = TRUE;
			sp0 = m;
			p0 = vpt->SnapPoint(m,m,NULL,SNAP_IN_PLANE);
			mat.SetTrans(p0);
			break;
		// TODO: Add for the rest of points
		}
	} else {
		if (msg == MOUSE_ABORT) return CREATE_ABORT;
	}

	return TRUE;
}

static [!output CLASS_NAME]CreateCallBack [!output CLASS_NAME]CreateCB;

// From BaseObject
CreateMouseCallBack* [!output CLASS_NAME]::GetCreateMouseCallBack() 
{
	[!output CLASS_NAME]CreateCB.SetObj(this);
	return(&[!output CLASS_NAME]CreateCB);
}

// From SimpleObject
void [!output CLASS_NAME]::BuildMesh(TimeValue /*t*/)
{
	// TODO: Implement the code to build the mesh representation of the object
	//       using its parameter settings at the time passed. The plug-in should 
	//       use the data member mesh to store the built mesh.
	//       SimpleObject ivalid member should be updated. This can be done while
	//       retrieving values from pblock2.
	ivalid = FOREVER;
}

void [!output CLASS_NAME]::InvalidateUI() 
{
	// Hey! Update the param blocks
	pblock2->GetDesc()->InvalidateUI();
}

Object* [!output CLASS_NAME]::ConvertToType(TimeValue t, Class_ID obtype)
{
	// TODO: If the plugin can convert to a nurbs surface then check to see 
	//       whether obtype == EDITABLE_SURF_CLASS_ID and convert the object
	//       to nurbs surface and return the object
	//       If no special conversion is needed remove this implementation.
	
	return __super::ConvertToType(t,obtype);
}

int [!output CLASS_NAME]::CanConvertToType(Class_ID obtype)
{
	// TODO: Check for any additional types the plugin supports
	//       If no special conversion is needed remove this implementation.
	return __super::CanConvertToType(obtype);
}

// From Object
int [!output CLASS_NAME]::IntersectRay(TimeValue /*t*/, Ray& /*ray*/, float& /*at*/, Point3& /*norm*/)
{
	// TODO: Return TRUE after you implement this method
	return FALSE;
}

void [!output CLASS_NAME]::GetCollapseTypes(Tab<Class_ID>& clist,Tab<TSTR*>& nlist)
{
	Object::GetCollapseTypes(clist, nlist);
	// TODO: Append any any other collapse type the plugin supports
}

// From ReferenceTarget
RefTargetHandle [!output CLASS_NAME]::Clone(RemapDir& remap) 
{
	[!output CLASS_NAME]* newob = new [!output CLASS_NAME]();
	// TODO: Make a copy all the data and also clone all the references
	newob->ReplaceReference(0,remap.CloneRef(pblock2));
	newob->ivalid.SetEmpty();
	BaseClone(this, newob, remap);
	return(newob);
}
