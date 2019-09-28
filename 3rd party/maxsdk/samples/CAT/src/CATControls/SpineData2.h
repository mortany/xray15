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

#pragma once

#include "FnPub/IBoneGroupManagerFP.h"
#include "FnPub/ISpineFP.h"
#include "CATControl.h"
 //
 //	Static IDs and Strings for the SpineData2 class.
 //
#define REFMSG_USER_UPDATESPINE				 0x113b3265

#define SPINEFLAG_PIN_TIP_HUB		(1<<1)//2
#define SPINEFLAG_RELATIVESPINE		(1<<2)//2
#define SPINEFLAG_FKSPINE			(1<<3)//2

#define SPINEDATA2_CLASS_ID	Class_ID(0x1d473a0e, 0x3b151c9d)

class Hub;
class SpineTrans2;
class CATClipValue;
class CATClipFloat;
class CatAPI::INodeControl;

class SpineData2 : public CATControl, public ISpineFP, public IBoneGroupManagerFP
{
private:
	friend class SpineDataPLCB;
	friend class SpineMotionDlgCallBack;
	friend class SpineSetupDlgCallBack;

	DWORD m_flags;

	CATClipFloat *layerAbsRel;

	BOOL bRelativeSpine;
	BOOL bNumSegmentsChanging;
	bool mHoldAnimStretchy;

	IHub* pre_clone_base_hub;

	/*
	 *	this matrix is passed into the Tip Hub as its parent matrix.
	 *	we cache is to that we can update the spine
	 */
	Matrix3 mtmHubTipParent;

	// Our only actual reference
	IParamBlock2 *pblock;

public:
	enum { PBLOCK_REF, LAYERABS_REL, NUMREFS };

	// Maybe we don't even need the weight params
	enum {
		PB_CATPARENT_DEPRECATED = 0,// CATMode_control,
		PB_HUBBASE,
		PB_HUBTIP,
		PB_SPINEID_DEPRECATED,

		PB_NAME,
		PB_SIZE,
		PB_LENGTH,
		PB_NUMLINKS,
		PB_LINKLENGTH_DEPRECATED,
		PB_ROOTPOS_DEPRECATED,

		PB_SPINETRANS_TAB,
		PB_SPINENODE_TAB_DEPRECATED,
		PB_SPINEWEIGHT,
		PB_SETUPABS_REL_DEPRECATED,
		PB_BASE_TRANSFORM
	};

	enum { PBLOCK, SPINEBASEPOS, BASEOB, TIPOB, NUMPARAMS };	// our subanim list.

public:

	//
	// constructors.
	//
	SpineData2(BOOL loading = FALSE);
	~SpineData2();

	//
	// from class Animatable:
	//
	Class_ID ClassID() { return SPINEDATA2_CLASS_ID; }
	SClass_ID SuperClassID();
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_SPINEDATA2); }

	int NumSubs() { return NUMREFS; }
	TSTR SubAnimName(int i);
	Animatable* SubAnim(int i);

	int NumRefs() { return NUMREFS; }
	RefTargetHandle GetReference(int i);
private:
	virtual void SetReference(int i, RefTargetHandle rtarg);
public:
	RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL);

	int	NumParamBlocks() { return 1; }							// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return pblock; }		// return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL);

	int	 GetKeyTimes(Tab<TimeValue> &times, Interval range, DWORD flags);
	BOOL IsKeyAtTime(TimeValue t, DWORD flags);
	BOOL GetNextKeyTime(TimeValue t, DWORD flags, TimeValue &nt);

	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);
	//
	// from class Control:
	//
	int IsKeyable() { return FALSE; };
	RefTargetHandle Clone(RemapDir& remap);
	void PostCloneManager();

	// Sub Object Gizmo methods
	static MoveCtrlApparatusCMode *moveMode;
	static RotateCtrlApparatusCMode *rotateMode;
	Box3 bbox;
	DWORD selLevel;

	// Our own method for drawing the gizmo to the screen
	void DrawGizmo(TimeValue t, GraphicsWindow *gw, const Matrix3& tmParent);

	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
	void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp *vpt, Box3& box);
	int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);
	void ActivateSubobjSel(int level, XFormModes& modes);
	void SelectSubComponent(CtrlHitRecord *hitRec, BOOL selected, BOOL all);
	void ClearSelection(int selLevel);
	int SubObjectIndex(CtrlHitRecord *hitRec);
	void SelectSubComponent(CtrlHitRecord *hitRec, BOOL selected, BOOL all, BOOL invert = FALSE);

	//int NumSubObjects(TimeValue t,INode *node);
	//void GetSubObjectTM(TimeValue t,INode *node,int subIndex,Matrix3& tm);
	//Point3 GetSubObjectCenter(TimeValue t,INode *node,int subIndex,int type);
	void GetSubObjectCenters(SubObjAxisCallback *cb, TimeValue t, INode *node);
	void GetSubObjectTMs(SubObjAxisCallback *cb, TimeValue t, INode *node);

	void SubMove(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin = FALSE);
	void SubRotate(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Quat& val, BOOL localOrigin = FALSE);

	BaseInterface* GetInterface(Interface_ID id);

	//////////////////////////////////////////////////////////////////////////
	// Flags
	void SetFlag(DWORD f);
	void ClearFlag(DWORD f);
	BOOL TestFlag(DWORD f) const { return (m_flags & f) == f; };

	//////////////////////////////////////////////////////////////////////////
	// CATControl
	USHORT GetRigID() { return idSpine; }

	CATGroup* GetGroup();

	virtual TSTR GetName();
	virtual void SetName(TSTR newname, BOOL quiet = FALSE);

	void CATMessage(TimeValue t, UINT msg, int data = -1);

	int NumLayerControllers() { return 1; };
	CATClipValue* GetLayerController(int i);

	int		NumChildCATControls();
	CATControl*	GetChildCATControl(int i);
	CATControl*	GetParentCATControl();

	BOOL LoadClip(CATRigReader *load, Interval range, int layerindex, float dScale, int flags);
	BOOL SaveClip(CATRigWriter *save, int flags, Interval timerange, int layerindex);

	BOOL LoadRig(CATRigReader *load);
	BOOL SaveRig(CATRigWriter *save);

	BOOL PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor);
	BOOL BuildMapping(CATControl* pastectrl, CATCharacterRemap &remap, BOOL includeERNnodes);
	BOOL PasteLayer(CATControl* pastectrl, int fromindex, int toindex, DWORD flags, RemapDir &remap);

	TSTR GetBoneAddress();
	void SetLengthAxis(int axis);

	virtual void UpdateUI();

	////////////////////////////////////////////////////////////////////////////
	// Spine
	// This will only be called by RibcageTrans when there is a head
	// it needs to take the newTipTrans and add a TipOffset from
	// the head Trans and store the result in the head and pass it back
	// we need one of these so that we can have interactive updating of the Ribcage
	// the head will have to think of a new way of doing this
	void Update();
	void Initialise(Hub* hub, int nSpineID, BOOL loading, int numbones);

	int		GetSpineID() { return GetBoneID(); }
	void	SetSpineID(int id) { SetBoneID(id); }

	Hub*	GetBaseHub();
	void	SetBaseHub(Hub* hub);
	Hub*	GetTipHub();
	void	SetTipHub(Hub* hub);
	CatAPI::IHub*	GetBaseIHub();
	CatAPI::IHub*	GetTipIHub();

	SpineTrans2*		GetSpineBone(int id);
	void CreateLinks(BOOL loading = FALSE);

	//////////////////////////////////////////////////////////////////////////
	// IBoneGroupManager
	void SetNumBones(int newNumLinks);
	int  GetNumBones() const;
	CatAPI::INodeControl* GetBoneINodeControl(int id);

	void UpdateSpineColours(TimeValue t);
	void RenameSpine();
	void SetSpineWeights();
	float GetRotWeight(float index);
	void UpdateSpineCurvatureWeights();

	void UpdateSpineCATUnits();

	// Get the base of the spine (including setup etc)
	//const Matrix3& GetSpineBaseTM();
	//void SetSpineBaseTM(const Matrix3& tmParent, const Point3& p3ParentScale);

	// Get the parent matrix for any tip hubs
	const Matrix3& GetSpineTipTM() const;

	// Calculate the desired base & tip of the spine (the 2 points we are 'anchored' too)
	void	CalcSpineBaseAnchor(const Matrix3& tmBaseNode, const Point3& p3BaseScale,
		Matrix3& tmSpineBase, Matrix3& tmSpineTipParent);
	void	CalcSpineAnchors(TimeValue t, const Matrix3& tmBaseNode, const Point3& p3BaseScale,
		Matrix3& tmSpineBase, Matrix3& tmSpineTip, Point3& p3TipScale, Interval& iValid);

	/** Evaluate all spine bones for the current time.  A spine bone cannot be evaluated in isolation, as they are evaluated with
		respect to the base and tip
		\param t - The time to evaluate the spine at
		\param tmSpineBase - The base of the chain.  This is the base hubs NodeTM (no scale)
		\param p3BaseScale - The base hubs scale
		\param[out] valid - the validity of the spine as a whole */
	void EvalSpine(TimeValue t, const Matrix3& tmParent, const Point3& p3ParentScale, Interval &valid);

	// Evaluate the affect of the rotation between the 2 hubs on each spine link.
	Matrix3 EvalSpineRotation(const Matrix3& tmSpineBase, const Matrix3& tmSpineTip, const Point3& p3BaseScale, const Point3& p3TipScale);

	// Evaluate the affect of the position between the 2 hubs on each spine link.
	Matrix3 EvalSpinePosition(float absrel, const Point3& p3CurrentPos, const Matrix3& tmSpineBase, const Point3& p3BaseScale, const Matrix3& tmSpineTip, const Point3& p3TipScale);

	// Calculate the value required by the tip hub to generate the desired value.
	Matrix3 CalcTipHubSetValue(TimeValue t, const Matrix3& tmDesiredValue, Matrix3& tmParent);

	// Called by SetValueHubTip to calculate a new HubPos so the spine reaches the approriate world position
	// Returns TRUE if the last guess succeeded, or FALSE if we need to continue with the calculated p3HubPos
	bool CalculateGuestimateHubPos(const Point3& vToCurrResult, const Point3& vToDesiredResult, Point3& p3HubPos, float& dError);

	// Calculate the value required by the tip hub to generate the desired value.
	void SetValueHubBase(TimeValue t, SetXFormPacket *ptr, int commit, GetSetMethod method);
	void SetValueHubTip(TimeValue t, SetXFormPacket *ptr, int commit, GetSetMethod &method);
	void HoldAnimStretchy(bool hold) { mHoldAnimStretchy = hold; }

	// Called when the tip hub is setting values on an FK spine.  Basically calculates the
	// the procedural values for the spine and sets them on the controllers.
	void SetValueTipHubFK(TimeValue t, SetXFormPacket *ptr, const Matrix3& tmSpineBase, int commit, GetSetMethod &method);

	// Returns the absolute/relative ratio (whether or not the tip hub inherits rotations from the base)
	float	GetAbsRel(TimeValue t, Interval &valid);
	void	SetAbsRel(TimeValue t, float val);

	// TODO: Get rid of these functions
	/** Get the transform the Spine starts from.  This includes the
		spines offset from the base hub */
	void	SetBaseTransform(Matrix3 tmSpineBase) { pblock->SetValue(PB_BASE_TRANSFORM, 0, tmSpineBase); };
	Matrix3	GetBaseTransform() const { return pblock->GetMatrix3(PB_BASE_TRANSFORM); };

	void	SetSpineFK(BOOL tf);
	BOOL	GetSpineFK() const { return TestFlag(SPINEFLAG_FKSPINE); }

	float	GetSpineLength();
	void	SetSpineLength(float val);

	float	GetSpineWidth();
	void	SetSpineWidth(float val);

	bool	CanScaleSpine();
	void	ScaleSpineLength(TimeValue t, float scale);

	float	GetSpineDepth();
	void	SetSpineDepth(float val);
};
