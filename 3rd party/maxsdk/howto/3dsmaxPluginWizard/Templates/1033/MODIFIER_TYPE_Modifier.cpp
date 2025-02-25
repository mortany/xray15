[!output TEMPLATESTRING_COPYRIGHT]

#include "[!output PROJECT_NAME].h"
[!if QT_UI != 0]
#include "QtPluginRollup.h"
[!endif]

#define [!output CLASS_NAME]_CLASS_ID Class_ID([!output CLASSID1], [!output CLASSID2])

[!if EXTENSION != 0]
#define XTC[!output CLASS_NAME_UPPER]_CLASS_ID Class_ID([!output EXTCLASSID1], [!output EXTCLASSID2])
[!endif]

#define PBLOCK_REF 0

[!if EXTENSION != 0]
class XTC[!output CLASS_NAME] : public XTCObject
{
public:
	XTC[!output CLASS_NAME]() {};
	~XTC[!output CLASS_NAME]() {};

	Class_ID    ExtensionID() override { return XTC[!output CLASS_NAME_UPPER]_CLASS_ID; }

	XTCObject * Clone() override { return new XTC[!output CLASS_NAME](); };

	void        DeleteThis() override { delete this; }
	int         Display(TimeValue /*t*/, INode* /*inode*/, ViewExp * /*vpt*/, int /*flags*/, Object * /*pObj*/) override { return 1; }

	ChannelMask DependsOn() override       { return GEOM_CHANNEL|TOPO_CHANNEL; }
	ChannelMask ChannelsChanged() override { return GEOM_CHANNEL; }

	void        PreChanChangedNotify(TimeValue /*t*/, ModContext & /*mc*/, ObjectState* /*os*/, INode * /*node*/, Modifier * /*mod*/, bool /*bEndOfPipeline*/) override {};
	void        PostChanChangedNotify(TimeValue /*t*/, ModContext & /*mc*/, ObjectState* /*os*/, INode * /*node*/, Modifier * /*mod*/, bool /*bEndOfPipeline*/) override {};

	BOOL        SuspendObjectDisplay() override { return FALSE; }
};
[!endif]

class [!output CLASS_NAME] : public [!output SUPER_CLASS_NAME] 
{
public:
	// Constructor/Destructor
	[!output CLASS_NAME]();
	virtual ~[!output CLASS_NAME]();

	virtual void                 DeleteThis() override { delete this; }

	// From Animatable
	virtual const TCHAR *        GetObjectName() override { return GetString(IDS_CLASS_NAME); }

[!if EXTENSION != 0]
	#pragma message(TODO("Add the channels that the modifier needs to perform its modification"))
	virtual ChannelMask          ChannelsUsed() override { return EXTENSION_CHANNEL; }

	// We have to include the channels, that the extension object changes, so the 
	// PostChanChangedNotify will be called after the modifier added the extension objects
	// to the object flowing up the stack.

	virtual ChannelMask          ChannelsChanged() override { return EXTENSION_CHANNEL|GEOM_CHANNEL; }

[!else]
	virtual ChannelMask          ChannelsUsed() override    { return GEOM_CHANNEL|TOPO_CHANNEL; }
	#pragma message(TODO("Add the channels that the modifier actually modifies"))
	virtual ChannelMask          ChannelsChanged() override { return GEOM_CHANNEL; }
[!endif]

	#pragma message(TODO("Return the ClassID of the object that the modifier can modify"))
	Class_ID                     InputType() override { return defObjectClassID; }

	virtual void                 ModifyObject(TimeValue t, ModContext &mc, ObjectState *os, INode *node) override;
	virtual void                 NotifyInputChanged(const Interval& changeInt, PartID partID, RefMessage message, ModContext *mc) override;

	virtual void                 NotifyPreCollapse(INode *node, IDerivedObject *derObj, int index) override;
	virtual void                 NotifyPostCollapse(INode *node,Object *obj, IDerivedObject *derObj, int index) override;

	virtual Interval             LocalValidity(TimeValue t) override;

	// From BaseObject
	#pragma message(TODO("Return true if the modifier changes topology"))
	virtual BOOL                 ChangeTopology() override { return FALSE; }
	
	virtual CreateMouseCallBack* GetCreateMouseCallBack() override { return NULL; }

	virtual BOOL                 HasUVW() override;
	virtual void                 SetGenUVW(BOOL sw) override;

	virtual void                 BeginEditParams(IObjParam *ip, ULONG flags,Animatable *prev) override;
	virtual void                 EndEditParams(IObjParam *ip, ULONG flags,Animatable *next) override;

	virtual Interval             GetValidity(TimeValue t);

	// Automatic texture support
	
	// Loading/Saving
	virtual IOResult             Load(ILoad *iload) override;
	virtual IOResult             Save(ISave *isave) override;

	// From Animatable
	virtual Class_ID             ClassID() override             { return [!output CLASS_NAME]_CLASS_ID; }
	virtual SClass_ID            SuperClassID() override        { return OSM_CLASS_ID; }
	virtual void                 GetClassName(TSTR& s) override { s = GetString(IDS_CLASS_NAME); }

	virtual RefTargetHandle      Clone(RemapDir &remap) override;
	virtual RefResult            NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate) override;

	virtual int                  NumSubs() override              { return 1; }
	virtual TSTR                 SubAnimName(int /*i*/) override { return GetString(IDS_PARAMS); }
	virtual Animatable*          SubAnim(int /*i*/) override     { return pblock; }

	// TODO: Maintain the number or references here
	virtual int                  NumRefs() override { return 1; }
	virtual RefTargetHandle      GetReference(int i) override;

	virtual int                  NumParamBlocks() override              { return 1; } // Return number of ParamBlocks in this instance
	virtual IParamBlock2*        GetParamBlock(int /*i*/) override      { return pblock; } // Return i'th ParamBlock
	virtual IParamBlock2*        GetParamBlockByID(BlockID id) override { return (pblock->ID() == id) ? pblock : NULL; } // Return id'd ParamBlock

protected:
	virtual void                 SetReference(int , RefTargetHandle rtarg) override;

private:
	// Parameter block
	IParamBlock2 *pblock; // Ref 0
};

void [!output CLASS_NAME]::SetReference(int i, RefTargetHandle rtarg)
{
	if (i == PBLOCK_REF)
	{
		pblock=(IParamBlock2*)rtarg;
	}
}

RefTargetHandle [!output CLASS_NAME]::GetReference(int i)
{
	if (i == PBLOCK_REF)
	{
		return pblock;
	}

	return nullptr;
}

[!output TEMPLATESTRING_CLASSDESC]

[!if PARAM_MAPS != 0]
[!output TEMPLATESTRING_PARAMBLOCKDESC]
[!endif]

//--- [!output CLASS_NAME] -------------------------------------------------------
[!output CLASS_NAME]::[!output CLASS_NAME]()
	: pblock(nullptr)
{
	Get[!output CLASS_NAME]Desc()->MakeAutoParamBlocks(this);
}

[!output CLASS_NAME]::~[!output CLASS_NAME]()
{

}

/*===========================================================================*\
 |  The validity of the parameters. First a test for editing is performed
 |  then Start at FOREVER, and intersect with the validity of each item
\*===========================================================================*/
Interval [!output CLASS_NAME]::LocalValidity(TimeValue /*t*/)
{
	// If being edited, return NEVER forces a cache to be built 
	// after previous modifier.
	if (TestAFlag(A_MOD_BEING_EDITED))
	{
		return NEVER;
	}

	#pragma message(TODO("Return the validity interval of the modifier"))
	return NEVER;
}

/*************************************************************************************************
*
	Between NotifyPreCollapse and NotifyPostCollapse, Modify is
	called by the system. NotifyPreCollapse can be used to save any plugin dependant data e.g.
	LocalModData
*
\*************************************************************************************************/

void [!output CLASS_NAME]::NotifyPreCollapse(INode* /*node*/, IDerivedObject* /*derObj*/, int /*index*/)
{
	#pragma message(TODO("Perform any Pre Stack Collapse methods here"))
}

/*************************************************************************************************
*
	NotifyPostCollapse can be used to apply the modifier back onto to the stack, copying over the
	stored data from the temporary storage. To reapply the modifier the following code can be 
	used

	Object *bo = node->GetObjectRef();
	IDerivedObject *derob = NULL;
	if(bo->SuperClassID() != GEN_DERIVOB_CLASS_ID)
	{
		derob = CreateDerivedObject(obj);
		node->SetObjectRef(derob);
	}
	else
	{
		derob = (IDerivedObject*) bo;
	}

	// Add ourselves to the top of the stack
	derob->AddModifier(this,NULL,derob->NumModifiers());

*
\*************************************************************************************************/

void [!output CLASS_NAME]::NotifyPostCollapse(INode* /*node*/,Object* /*obj*/, IDerivedObject* /*derObj*/, int /*index*/)
{
	#pragma message(TODO("Perform any Post Stack collapse methods here."))
}

/*************************************************************************************************
*
    ModifyObject will do all the work in a full modifier
    This includes casting objects to their correct form, doing modifications
    changing their parameters, etc
*
\************************************************************************************************/

void [!output CLASS_NAME]::ModifyObject(TimeValue /*t*/, ModContext& /*mc*/, ObjectState* [!if EXTENSION != 0]os[!else]/*os*/[!endif], INode* /*node*/)
{
	#pragma message(TODO("Add the code for actually modifying the object"))
[!if EXTENSION != 0]
	XTC[!output CLASS_NAME] *pObj = NULL;
	pObj = new  XTC[!output CLASS_NAME]();
	os->obj->AddXTCObject(pObj);
	os->obj->SetChannelValidity(EXTENSION_CHAN_NUM, FOREVER);
[!endif]
}

void [!output CLASS_NAME]::BeginEditParams(IObjParam* ip, ULONG flags, Animatable* prev)
{
	TimeValue t = ip->GetTime();
	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_BEGIN_EDIT);
	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_MOD_DISPLAY_ON);
	SetAFlag(A_MOD_BEING_EDITED);

	Get[!output CLASS_NAME]Desc()->BeginEditParams(ip, this, flags, prev);
}

void [!output CLASS_NAME]::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	Get[!output CLASS_NAME]Desc()->EndEditParams(ip, this, flags, next);

	TimeValue t = ip->GetTime();
	ClearAFlag(A_MOD_BEING_EDITED);
	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_END_EDIT);
	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_MOD_DISPLAY_OFF);
}

Interval [!output CLASS_NAME]::GetValidity(TimeValue /*t*/)
{
	Interval valid = FOREVER;
	#pragma message(TODO("Return the validity interval of the modifier"))
	return valid;
}

RefTargetHandle [!output CLASS_NAME]::Clone(RemapDir& remap)
{
	[!output CLASS_NAME]* newmod = new [!output CLASS_NAME]();
	#pragma message(TODO("Add the cloning code here"))
	newmod->ReplaceReference(PBLOCK_REF,remap.CloneRef(pblock));
	BaseClone(this, newmod, remap);
	return(newmod);
}

//From ReferenceMaker 
RefResult [!output CLASS_NAME]::NotifyRefChanged(
		const Interval& /*changeInt*/, RefTargetHandle hTarget,
		PartID& /*partID*/,  RefMessage message, BOOL /*propagate*/)
{
	#pragma message(TODO("Add code to handle the various reference changed messages"))
	switch (message)
	{
	case REFMSG_TARGET_DELETED:
		{
			if (hTarget == pblock)
			{
				pblock = nullptr;
			}
		}
		break;
	}
	return REF_SUCCEED;
}

/****************************************************************************************
*
 	NotifyInputChanged is called each time the input object is changed in some way
 	We can find out how it was changed by checking partID and message
*
\****************************************************************************************/

void [!output CLASS_NAME]::NotifyInputChanged(const Interval &/*changeInt*/, PartID /*partID*/, RefMessage /*message*/, ModContext* /*mc*/)
{

}

// From Object
BOOL [!output CLASS_NAME]::HasUVW() 
{
	#pragma message(TODO("Return whether the object has UVW coordinates or not"))
	return TRUE;
}

void [!output CLASS_NAME]::SetGenUVW(BOOL sw)
{
	if (sw==HasUVW())
	{
		return;
	}

	#pragma message(TODO("Set the plugin internal value to sw"))
}

IOResult [!output CLASS_NAME]::Load(ILoad* /*iload*/)
{
	#pragma message(TODO("Add code to allow plugin to load its data"))

	return IO_OK;
}

IOResult [!output CLASS_NAME]::Save(ISave* /*isave*/)
{
	#pragma message(TODO("Add code to allow plugin to save its data"))

	return IO_OK;
}

