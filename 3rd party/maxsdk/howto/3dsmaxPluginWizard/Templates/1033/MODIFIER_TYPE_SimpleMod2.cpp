[!output TEMPLATESTRING_COPYRIGHT]

#include "[!output PROJECT_NAME].h"
[!if QT_UI != 0]
#include "QtPluginRollup.h"
[!endif]

#define [!output CLASS_NAME]_CLASS_ID Class_ID([!output CLASSID1], [!output CLASSID2])

[!if EXTENSION != 0]
#define XTC[!output CLASS_NAME]_CLASS_ID Class_ID([!output EXTCLASSID1], [!output EXTCLASSID2])
[!endif]

#define PBLOCK_REF SIMPMOD_PBLOCKREF

[!if EXTENSION != 0]
class XTC[!output CLASS_NAME] : public XTCObject
{
public:
	XTC[!output CLASS_NAME]();
	~XTC[!output CLASS_NAME]();

	Class_ID    ExtensionID() override { return XTC[!output CLASS_NAME]_CLASS_ID; }

	XTCObject * Clone() override;

	void        DeleteThis() override { delete this; }
	int         Display(TimeValue t, INode* inode, ViewExp *vpt, int flags, Object *pObj) override;

	ChannelMask DependsOn() override       { return GEOM_CHANNEL|TOPO_CHANNEL; }
	ChannelMask ChannelsChanged() override { return GEOM_CHANNEL; }

	void        PreChanChangedNotify(TimeValue t, ModContext &mc, ObjectState* os, INode *node,Modifier *mod);
	void        PostChanChangedNotify(TimeValue t, ModContext &mc, ObjectState* os, INode *node,Modifier *mod);
	
	BOOL        SuspendObjectDisplay() override;
};
[!endif]

class [!output CLASS_NAME] : public [!output SUPER_CLASS_NAME]
{
public:
	// Constructor/Destructor
	[!output CLASS_NAME]();
	~[!output CLASS_NAME]();

	// Parameter block handled by parent

	// From BaseObject
	virtual const TCHAR*    GetObjectName() override { return GetString(IDS_CLASS_NAME); }

	// From Modifier

	virtual void            BeginEditParams(IObjParam *ip, ULONG flags,Animatable *prev) override;
	virtual void            EndEditParams(IObjParam *ip, ULONG flags,Animatable *next) override;

	// From SimpleModBase
	virtual Deformer&       GetDeformer(TimeValue t,ModContext &mc,Matrix3& mat,Matrix3& invmat) override;

	virtual Interval        GetValidity(TimeValue t) override;

	// Loading/Saving
	virtual IOResult        Load(ILoad *iload) override;
	virtual IOResult        Save(ISave *isave) override;

	// From Animatable
	virtual Class_ID        ClassID() override             { return [!output CLASS_NAME]_CLASS_ID; }
	virtual SClass_ID       SuperClassID() override        { return OSM_CLASS_ID; }
	virtual void            GetClassName(TSTR& s) override { s = GetString(IDS_CLASS_NAME); }

	virtual RefTargetHandle Clone(RemapDir &remap) override;

	virtual void            DeleteThis() override { delete this; }
};

// This is the callback object used by modifiers to deform "Deformable" objects.
class [!output CLASS_NAME]Deformer : public Deformer
{
public:
	[!output CLASS_NAME]Deformer();
	// TODO: Add other plug-in specific constructors and member functions
	Point3 Map(int i, Point3 p) override;
};

[!output TEMPLATESTRING_CLASSDESC]

[!if PARAM_MAPS != 0]
[!output TEMPLATESTRING_PARAMBLOCKDESC]
[!endif]

[!if EXTENSION != 0]

// TODO: Perform any Class Setup here.
XTC[!output CLASS_NAME]::XTC[!output CLASS_NAME]()
{

}

XTC[!output CLASS_NAME]::~XTC[!output CLASS_NAME]()
{

}

XTCObject * XTC[!output CLASS_NAME]::Clone()
{
	// TODO: Perform any class initialization ready for a clone.
	return new XTC[!output CLASS_NAME]();
}

int XTC[!output CLASS_NAME]::Display(TimeValue /*t*/, INode* /*inode*/, ViewExp * /*vpt*/, int /*flags*/, Object * /*pObj*/)
{
	// TODO: Add Extension Objects drawing routine here
	return 0;
}

/******************************************************************************************************************
*
	This method will be called before a modifier in the pipleine changes any channels that this XTCObject depends on
	The channels the XTCObect will react on are determine by the call to DependsOn() 
*
\******************************************************************************************************************/

void XTC[!output CLASS_NAME]::PreChanChangedNotify(TimeValue /*t*/, ModContext & /*mc*/, ObjectState* /*os*/, INode * /*node*/, Modifier * /*mod*/)
{

}

/******************************************************************************************************************
*
	This method will be called after a modifier in the pipleine changes any channels that this XTCObject depends on
	The channels the XTCObect will react on are determine by the call to DependsOn() 
*
\******************************************************************************************************************/

void XTC[!output CLASS_NAME]::PostChanChangedNotify(TimeValue /*t*/, ModContext & /*mc*/, ObjectState* /*os*/, INode * /*node*/, Modifier * /*mod*/)
{

}

BOOL XTC[!output CLASS_NAME]::SuspendObjectDisplay()
{
	// TODO: Tell the system to display the Object or not
	return FALSE;
}

[!endif]

[!output CLASS_NAME]Deformer::[!output CLASS_NAME]Deformer() 
{

}

/*************************************************************************************************
*
 	Map is called for every deformable point in the object
*
\*************************************************************************************************/

Point3 [!output CLASS_NAME]Deformer::Map(int /*i*/, Point3 p)
{
	// TODO: Add code to deform or alter a single point
	return p;
}

//--- [!output CLASS_NAME] -------------------------------------------------------
[!output CLASS_NAME]::[!output CLASS_NAME]()
{
	pblock2 = nullptr;
	Get[!output CLASS_NAME]Desc()->MakeAutoParamBlocks(this);
}

[!output CLASS_NAME]::~[!output CLASS_NAME]()
{

}

void [!output CLASS_NAME]::BeginEditParams(IObjParam *ip, ULONG flags,Animatable *prev)
{
	TimeValue t = ip->GetTime();
	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_BEGIN_EDIT);
	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_MOD_DISPLAY_ON);
	SetAFlag(A_MOD_BEING_EDITED);

	__super::BeginEditParams(ip,flags,prev);
	Get[!output CLASS_NAME]Desc()->BeginEditParams(ip, this, flags, prev);
}

void [!output CLASS_NAME]::EndEditParams(IObjParam *ip, ULONG flags,Animatable *next)
{
	__super::EndEditParams(ip,flags,next);
	Get[!output CLASS_NAME]Desc()->EndEditParams(ip, this, flags, next);

	TimeValue t = ip->GetTime();
	ClearAFlag(A_MOD_BEING_EDITED);
	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_END_EDIT);
	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_MOD_DISPLAY_OFF);
}

Interval [!output CLASS_NAME]::GetValidity(TimeValue /*t*/)
{
	Interval valid = FOREVER;
	// TODO: Return the validity interval of the modifier
	return valid;
}

Deformer& [!output CLASS_NAME]::GetDeformer(TimeValue /*t*/, ModContext& /*mc*/, Matrix3& /*mat*/, Matrix3& /*invmat*/)
{
	static [!output CLASS_NAME]Deformer deformer;
	// TODO: Add code to modify the deformer object
	return deformer;
}

RefTargetHandle [!output CLASS_NAME]::Clone(RemapDir& remap)
{
	[!output CLASS_NAME]* newmod = new [!output CLASS_NAME]();
	// TODO: Add the cloning code here
	newmod->SimpleMod2Clone(this, remap);
	BaseClone(this, newmod, remap);
	return(newmod);
}

IOResult [!output CLASS_NAME]::Load(ILoad* /*iload*/)
{
	// TODO: Add code to allow plugin to load its data
	return IO_OK;
}

IOResult [!output CLASS_NAME]::Save(ISave* /*isave*/)
{
	// TODO: Add code to allow plugin to save its data
	return IO_OK;
}

