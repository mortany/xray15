//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2009 Autodesk, Inc.  All rights reserved.
//  Copyright 2003 Character Animation Technologies.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////

// Parent object

#pragma once

#include "..\CATControls\ICATParent.h"
#include "..\CATAPI.h"

extern ClassDesc2* GetCATParentDesc();
extern void InitCATParentGlobalCallbacks();
extern void ReleaseCATParentGlobalCallbacks();

#define PARENTFLAG_USECUSTOMMESH			(1<<4)
#define PARENTFLAG_CREATED					(1<<6)
#define PARENTFLAG_SAVING_RIG				(1<<7)
#define PARENTFLAG_SAVING_ANIMATION			(1<<8)
#define PARENTFLAG_RELOAD_RIG_ON_SCENE_LOAD	(1<<10)
#define PARENTFLAG_LOADED_IN_CAT3			(1<<11)

class CATParentPLCB;
class CATParentTrans;
class CATParentCreateCallBack;
class CATClipRoot;
class Hub;

class CATParent : public ECATParent
{
private:
	// Allow the callback to directly modify mode.  It knows that
	// it must send a CAT_REFMSG_CATMODE to our dependents.
	friend class CATParentParamDlgCallBack;
	friend class CATParentPLCB;
	friend class CATParentCreateCallBack;
	friend class CATParentCreateCallBack;
	friend class CATClipRootPLCB;
	friend class CATParentChangeRestore;
	friend class CATParentTrans;

	////////////////////////////////////////////////////////
	// Memebers
	Hub*	roothub_deprecated;
	INode*	node_deprecated;

	int		lengthaxis;
	DWORD	flags;
	CATMode	catmode;
	TSTR	rigpresetfile;
	TSTR	rigpresetfile_modifiedtime;

public:
	enum CATStuffParams {
		PB_PARENT_NODE,
		PB_PARENT_CTRL,
		PB_ROOTHUB,

		PB_STEP_EASE,
		PB_DISTCOVERED,
	};
	enum CATParentParams {
		PB_CATNAME,
		PB_CATMODE,
		PB_CATUNITS,
		PB_CATVISIBILITY,
		PB_CATRENDERABLE,
		PB_CATFROZEN,
		PB_CATHIERARCHY,
		PB_CLIPHIERARCHY,

	};

	enum CATParentParamIDs {

		ID_CATPARENT_PARAMS,
		//	ID_CATRIG_PARAMS,
		ID_CATSTUFF_PARAMS
	};

	enum CATParentRefs {
		REF_CATPARENT_PARAMS,
		//	REF_CATRIG_PARAMS,
		REF_CATSTUFF_PARAMS,
	};

	// These are references that get saved/loaded automatically
	IParamBlock2*	catparentparams;
	IParamBlock2*	catstuffparams;

	Mesh catmesh;

public:

	//Class vars
	IObjParam *ipbegin;			// Access to the interface
	ULONG flagsBegin;		// Stores flags passed to BeginEditParams()... for CATParentParamDlgCallBack
	// From BaseObject
	CreateMouseCallBack* GetCreateMouseCallBack();

	// From Object
	BOOL HasUVW();
	void SetGenUVW(BOOL sw);

	int CanConvertToType(Class_ID obtype);
	Object* ConvertToType(TimeValue t, Class_ID obtype);
	void GetCollapseTypes(Tab<Class_ID> &clist, Tab<TSTR*> &nlist) { UNREFERENCED_PARAMETER(clist); UNREFERENCED_PARAMETER(nlist); };

	int IntersectRay(TimeValue t, Ray& ray, float& at, Point3& norm);
	void InitNodeName(TSTR& s) { s = GetString(IDS_CHARACTER); }

	// Object state and validity functions
	ObjectState Eval(TimeValue) { return ObjectState(this); }
	Interval ObjectValidity(TimeValue) { return FOREVER; }

	int	NumParamBlocks() { return 2; }		// catparentparams, catstuffparams
	int NumRefs() { return 2; }				// catparentparams, catstuffparams
	int SubNumToRefNum(int subNum) { return subNum; }
	int NumSubs() { return 1; }				// only visible subanim is the LayerRoot

	IParamBlock2* GetParamBlock(int i);
	IParamBlock2* GetParamBlockByID(BlockID id);

	RefTargetHandle GetReference(int i);
private:
	virtual void SetReference(int i, RefTargetHandle rtarg);
public:
	Animatable* SubAnim(int i);
	TSTR SubAnimName(int i);
	RefResult NotifyRefChanged(const Interval& iv, RefTargetHandle hTarg, PartID& partID, RefMessage msg, BOOL propagate);

	// From Animatable
	void BeginEditParams(IObjParam  *ip, ULONG flags, Animatable *prev);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next);

	// From SimpleObject
	void BuildMesh(TimeValue t);
	BOOL OKtoDisplay(TimeValue t);
	void InvalidateUI();

	// Loading/Saving
	IOResult Load(ILoad *iload);
	IOResult Save(ISave *isave);

	// from Base Object
	const TCHAR* GetObjectName();

	// From Animatable
	Class_ID ClassID() { return CATPARENT_CLASS_ID; }
	SClass_ID SuperClassID() { return HELPER_CLASS_ID; }
	void GetClassName(TSTR& s);

	// As a world space object, we can't be instanced (we can't do that)
	BOOL IsWorldSpaceObject() { return TRUE; };
	RefTargetHandle Clone(RemapDir &remap);
	void DeleteThis() { delete this; }

	//Constructor/Destructor
	void Init();
	CATParent(BOOL loading);
	~CATParent();

	// Stop our objects beong collapsed
	virtual void NotifyPostCollapse(INode *node, Object *obj, IDerivedObject *derObj, int index);

	void RefAdded(RefMakerHandle rm);

	BOOL SaveMesh(class CATRigWriter* save);
	BOOL LoadMesh(class CATRigReader* load);

private:
	DWORD dwFileSaveVersion;

	CATParentTrans* catparenttrans;

	BOOL bRigCreating;
public:

	////////////////////////////////////////////////////////////////////
	// Rig Management

	ICATParentTrans* GetCATParentTrans();
	const ICATParentTrans* GetCATParentTrans() const;

	////////////////////////////////////////////////////////////////////
	// Callbacks
	static void CATParentNodeCreateNotify(void *param, NotifyInfo *info);
	void RegisterCallbacks();
	void UnRegisterCallbacks();

	////////////////////////////////////////////////////////////////////
	// Gets the file save version.  This is the CAT version
	// that saved the file.
	virtual DWORD GetFileSaveVersion() { return dwFileSaveVersion; }

	////////////////////////////////////////////////////////////////////
	// Layers
	void CATMessage(TimeValue t, UINT msg, int data);

	////////////////////////////////////////////////////////////////////
	// Flags
	void SetFlag(const DWORD f, BOOL tf = TRUE) { if (tf) flags |= f; else flags &= ~f; }
	void ClearFlag(const DWORD f) { flags &= ~f; }
	BOOL TestFlag(const DWORD f) { return (flags&f) == f; }

	void SetMaskFlag(const DWORD mask, const DWORD f) { flags = (flags&~mask) | (f&mask); }
	DWORD GetMaskFlag(const DWORD mask) const { return flags&mask; }

	// CATRigParams accessors (NOT in ECatParent)
	void	SetRigFilename(TSTR name) { rigpresetfile = name; }
	TSTR	GetRigFilename() { return rigpresetfile; }
	virtual BOOL	GetReloadRigOnSceneLoad() { return TestFlag(PARENTFLAG_RELOAD_RIG_ON_SCENE_LOAD); };
	virtual void	SetReloadRigOnSceneLoad(BOOL tf) { SetFlag(PARENTFLAG_RELOAD_RIG_ON_SCENE_LOAD, tf); };

	void	SetRigFileModifiedTime(TSTR str) { rigpresetfile_modifiedtime = str; }
	TSTR	GetRigFileModifiedTime() { return rigpresetfile_modifiedtime; }

	// Tells this catparent to take ownership of the UI.
	void SetOwnsUI();
	void RefreshUI();

	////////////////////////////////////////////////////////////////////
	// Function Publishing

	BaseInterface*		GetInterface(Interface_ID id);

	FPInterfaceDesc*	GetDescByID(Interface_ID id);
};

/**********************************************************************
* Object creation callback class, for interactive creation of the
* object using the mouse.
*/
class CATParentCreateCallBack : public CreateMouseCallBack {
private:
	IPoint2 sp0, sp1;	// First point in screen coordinates
	CATParent *ob;		// Pointer to the object
	Point3 p0, p1;		// First point in world coordinates
	float defaultCATUnits;
	BOOL iscreating;
public:
	int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat);
	void SetObj(CATParent *obj) { ob = obj; }
	void SetDefaultCATUnits(float catunits) { defaultCATUnits = catunits; }
	BOOL IsCreating() { return iscreating; }
	void SetNotCreating() { iscreating = FALSE; ob = NULL; }
};
