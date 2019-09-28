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

/**********************************************************************
	This controller carries the Facing vector allong underneath the character
	The CATParent references us, So it is the parent/CATPath's responsibility to
	set CATUnits on us.
 **********************************************************************/
#pragma once

#include "ICATParent.h"
#include "ExtraRigNodes.h"

class CATCharacterRemap;
class CATControl;
class Hub;

//
//	Static IDs and Strings for the CATParentTrans class.
//
#define CATPARENTTRANS_CLASS_ID			Class_ID(0x22d5030, 0x4344191c)

class CATParentTrans : public ICATParentTrans, public ExtraRigNodes {
private:

	friend class CATParentPLCB;
	friend class CATParentTransPLCB;
	friend class CATParentTransChangeRestore;
	friend class CATParent;

	// pointers to our controls.
	Control *prs;
	int flagsBegin;
	// Class Interface pointer
	IObjParam *ipbegin;

	LONG rootnodehdl;

public:
	enum PARAMLIST { PBLOCK_REF, PRS, LAYERROOT, NUMPARAMS };		// our subanim list.
	enum { PB_CATPARENT_DEPRECATED, PB_CATUNITS = 5 };

	IParamBlock2	*pblock;	//ref 0

public:
	//
	// constructors.
	//
	void Init(BOOL loading);
	CATParentTrans(BOOL loading = FALSE);
	CATParentTrans(ECATParent *catparent, INode* node);
	~CATParentTrans();

	//
	// from class Animatable:
	//
	Class_ID ClassID() { return CATPARENTTRANS_CLASS_ID; }
	SClass_ID SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_CATPARENTTRANS); };
	void DeleteThis() { delete this; }
	int NumSubs() { return NUMPARAMS; }
	Animatable* SubAnim(int i);
	TSTR SubAnimName(int i);
	DWORD GetSubAnimCurveColor(int) { return PAINTCURVE_GENCOLOR; }
	ParamDimension* GetParamDimension(int) { return defaultDim; }
	BOOL AssignController(Animatable *control, int subAnim);
	BOOL IsReplaceable() { return FALSE; };

	//
	// from class ReferenceMaker
	//
	int NumRefs() { return NUMPARAMS; };
	RefTargetHandle GetReference(int i);
private:
	virtual void SetReference(int i, RefTargetHandle rtarg);
public:

	// CATParentTrans overrides NotifyDependents to ensure
	// that dependency tests properly test loops.  Although there is
	// no reference link, ever CATNode in our tree depends on the CATParent.
	// In setup mode, they derive their transform from the CATParent, among
	// other things.  To ensure no dependency loops we override
	// NotifyDependents to call through to all CATNodeControls to prevent
	// any link being accidentally created.
	virtual RefResult NotifyDependents(const Interval& changeInt, PartID partID, RefMessage message, SClass_ID sclass = NOTIFY_ALL, BOOL propagate = TRUE, RefTargetHandle hTarg = NULL, NotifyDependentsOption notifyDependentsOption = REFNOTIFY_ALLOW_OPTIMIZATIONS);

	RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL);

	int	NumParamBlocks() { return 1; }							// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return pblock; }		// return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	//
	// from class Control:
	//
	int IsKeyable() { return FALSE; };
	BOOL CanInstanceController() { return FALSE; }
	void Copy(Control *from);
	RefTargetHandle Clone(RemapDir& remap);
	BOOL IsLeaf() { return FALSE; }
	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);
	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method);
	//	BOOL BypassTreeView() { return TRUE; }
	Control* GetPositionController() { return prs->GetPositionController(); }
	Control* GetRotationController() { return prs->GetRotationController(); }
	Control* GetScaleController() { return prs->GetScaleController(); }

	BOOL ChangeParents(TimeValue t, const Matrix3& oldP, const Matrix3& newP, const Matrix3& tm) { return prs ? prs->ChangeParents(t, oldP, newP, tm) : FALSE; }

	//
	// These 2 methods (from animatable) deal with copy/paste.
	//
	virtual BOOL CanCopyAnim() { return TRUE; }				// can we be copied and pasted?
	// this is a translation tool that lets max go through our
	// subanims and get their appropriate ref number.  Most of the time it will just
	// translate direcly to our subNum but it may not.  (if we reference an object that ain't a subanim.)
	int SubNumToRefNum(int subNum) { return subNum; };

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next);

	void RefDeleted();
	////////////////////////////////////////////////////////////////////////////////////

	void	GetSystemNodes(INodeTab &nodes, SysNodeContext ctxt);

	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	void* GetInterface(ULONG id) {
		if (id == I_MASTER) 				return (void *)this;
		else							return ICATParentTrans::GetInterface(id);
	}

	Box3 bbox;
	void	DrawGizmo(TimeValue t, GraphicsWindow *gw);
	int		Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);

	////////////////////////////////////////////////////////////////////////////////////
	// ICATParentTrans
private:
	DWORD dwFileSaveVersion;
	CATClipRoot *layerroot;

	Hub* mRootHub;
	INode*	node;
	INode*	rootnode;
	float	catunits;
	CATMode	catmode;
	int		lengthaxis;
	int		colourmode;
	TSTR	catname;

	BOOL bRigCreating;
	Interval ivalid;
	Interval tmCATValid;
	Point3 p3CATParentScale;
	Matrix3 tmCATParent;

	DWORD flags;

	// GB 27-Jan-2004: To prevent multiple colour updates at one point in time.
	TimeValue tvLastColourUpdateTime;

	// accessed by the callbacks
	BOOL isResetting;
public:

	int pre_export_selected_layer;

	//////////////////////////////////////////////////////////////////////////
	// ICATParentTrans interface

	ICATParentTrans* GetCATParentTrans() { return (ICATParentTrans*)this; }

	virtual int		GetVersion() { return GetFileSaveVersion(); };
	virtual int		GetBuildNumber() { return CAT_VERSION_CURRENT; };

	///////////////////////////////////////////////////
	virtual BOOL	SaveRig(const MSTR& filename);
	virtual INode*	LoadRig(const MSTR& filename);

	// this is the function that is called by script and the rollout
	virtual BOOL	SaveClip(TSTR filename, TimeValue start_t, TimeValue end_t, int from_layer, int to_layer);
	virtual INode*	LoadClip(TSTR filename, TimeValue t, BOOL scale_data, BOOL transformData, BOOL mirrorData, BOOL mirrorWorldX, BOOL mirrorWorldY);
	virtual BOOL	SavePose(TSTR filename);
	virtual INode*	LoadPose(TSTR filename, TimeValue t, BOOL scale_data, BOOL transformData, BOOL mirrorData, BOOL mirrorWorldX, BOOL mirrorWorldY);

	virtual void	CollapsePoseToCurLayer(TimeValue t);
	virtual BOOL	CollapseTimeRangeToLayer(TimeValue start_t, TimeValue end_t, TimeValue iKeyFreq, BOOL regularplot, int numpasses, float posdelta, float rotdelta);

	virtual void	PasteLayer(INode* from, int fromindex, int toindex, BOOL instance);
	virtual INode*	CreatePasteLayerTransformNode();

	TSTR			GetFileTagValue(TSTR file, TSTR tag);

	virtual void	AddRootNode();
	INode*			GetRootNode();
	virtual void	RemoveRootNode();

	virtual bool FillHIKDefinition(HIKDefinition& definition);

	////////////////////////////////////////////////////////////////////
	// Script Traversial
	virtual INodeTab		GetCATRigNodes();
	virtual Tab <Control*>	GetCATRigLayerCtrls();

	//////////////////////////////////////////////////////////////////////////
	// Local functions

	void SetFlag(const DWORD f) { flags |= f; }
	void ClearFlag(const DWORD f) { flags &= ~f; }
	BOOL TestFlag(const DWORD f) { return (flags&f) == f; }

	void SetMaskFlag(const DWORD mask, const DWORD f) { flags = (flags&~mask) | (f&mask); }
	DWORD GetMaskFlag(const DWORD mask) const { return flags&mask; }

	///////////////////////////////////////////////
	virtual CATColourMode GetColourMode();
	virtual CATColourMode GetEffectiveColourMode();
	virtual void SetColourMode(CATColourMode mode) { SetColourMode(mode, TRUE); }
	virtual void UpdateColours(BOOL bRedraw = TRUE, BOOL bRecalculate = TRUE);

	void SetColourMode(CATColourMode colmode, BOOL bRedraw);

	////////////////////////////////////////////////////////////////////
	// Gets the file save version.  This is the CAT version
	// that saved the file.
	DWORD GetFileSaveVersion() { return (((dwFileSaveVersion < CAT_VERSION_1700) && GetCATParent()) ? GetCATParent()->GetFileSaveVersion() : dwFileSaveVersion); }

	//Access to our node
	INode*	GetNode();
	void	SetNode(INode* n) { node = n; };

	// In CAT3, we don't store any pointers to objects.
	ECATParent*		GetCATParent();

	// XRefed Rigs do not need to change thier names.
	MSTR	GetCATName() { return catname; };
	void	SetCATName(const MSTR& strCATName);

	Control*	GetRootIHub();
	CATNodeControl* GetRootHub();
	void	SetRootHub(Hub* hub);;

	virtual ILayerRoot* GetLayerRoot();
	CATClipRoot* GetCATLayerRoot() { return layerroot; }

	//////////////////////////////////////////////////////////////////////////
	// RIG functions

	virtual BOOL AddHub();
	BOOL LoadRig(class CATRigReader* load);
	BOOL SaveRig(class CATRigWriter* save);

	INode* LoadRig(const MSTR& filename, Matrix3* tm);
	INode* LoadCATAsciiRig(const MSTR& file);
	INode* LoadCATBinaryRig(const MSTR& file, MSTR& name, Matrix3 * tm);

	void UnRegisterCallbacks();

	////////////////////////////////////////////////////////////////////
	// Layer Functions

	virtual INode* LoadClip(TSTR filename, int layerindex, int flags, TimeValue startT, CATControl* bodypart);
	virtual BOOL   SaveClip(TSTR filename, int flags, Interval timerange, Interval layerrange, CATControl* bodypart);

	virtual void CATMessage(TimeValue t, UINT msg, int data = -1);

	BOOL LoadClip(TSTR filename, int layerindex, int flags, TimeValue startT);
	BOOL SaveClip(TSTR filename, int flags, Interval timerange, Interval layerrange);

	// TODO move the UI for TDM to the motion panel
	virtual int  GetTrackDisplayMethod();
	virtual void SetTrackDisplayMethod(int n);

	////////////////////////////////////////////////////////////////////
	// CATBone Length Axis system (CAT 1.72)
	// making the lengthasis optional, now we need to have a few functions for conversions
	// of vecotrs and index's

	virtual int		GetLengthAxis() { return lengthaxis; }
	virtual void	SetLengthAxis(int axis);

	////////////////////////////////////////////////////////////////////////////////////
	// ICATParentFP

	virtual float	GetCATUnits() const;
	virtual void	SetCATUnits(float val);

	virtual CATMode	GetCATMode() const { return catmode; };
	virtual void	SetCATMode(CATMode mode);

	virtual void	UpdateCharacter();
	virtual Matrix3 ApproxCharacterTransform(TimeValue t);

	virtual INode*	GetBoneByAddress(const MSTR& address);
	virtual TSTR	GetBoneAddress();

	virtual Matrix3 GettmCATParent(TimeValue t);
	virtual Point3	GetCATParentScale(TimeValue t);

	////////////////////////////////////////////////////////////////////
	// CATRuntime User Properties
	virtual void	UpdateUserProps();

	////////////////////////////////////////////////////////////////////
	// FBIK
	virtual void	EvaluateCharacter(TimeValue t);

	void	AddSystemNodes(INodeTab &nodes, SysNodeContext ctxt);
	BOOL	BuildMapping(ICATParentTrans* pasteicatparenttrans, CATCharacterRemap &remap, BOOL includeERNnodes);

	Point3 GetCOM(TimeValue t);

	////////////////////////////////////////////////////////////////////////////////////
	// Function Publishing
	BaseInterface* GetInterface(Interface_ID id);
	FPInterfaceDesc* GetDescByID(Interface_ID id);
};
