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

//////////////////////////////////////////////////////////////////////////
// Class CATNodeControl - the base class for all controllers assigned
// directly to rig nodes.

#pragma once

#include "CATControl.h"
#include "ExtraRigNodes.h"
#include "CATClipValues.h"

#include "FnPub/INodeControlFP.h"
#include "../DataRestoreObj.h"

class ICATNodeData;

#define I_CATNODECONTROL		0x1d956991

class CATNodeControl :
	public CATControl,
	public INodeControlFP,
	public ExtraRigNodes,
	IDataRestoreOwner<DWORD> // Added for UIUpdate on undos to ccflags
{
private:
	friend class SetMirrorBoneRestore;
	friend class SetLimitRestore;

	INode*	preRigSaveParent;
	DWORD	preRigSaveLayerTransFlags;

	// My node;
	INode* mpNode;

	// The parent CATNodeControl.  This is not necessarily
	// the controller applied to mpNode's parent.
	CATNodeControl	*mpParentCATCtrl;

	// this is the final tm of the bone in world space
	// after we have applied scales to everything
	Matrix3 tmBoneWorld;

	// We cache our scale out seperately.  Apply this
	// scale to tmBoneWorld to get the actual world transform
	Point3 mp3ScaleWorld;

	// Local transform.  Probably not all that useful to cache, actually,
	// as many procedural controllers depend on the input.
	// TODO - Figure out an effective caching mechanism...
	Matrix3 tmBoneLocal;

	// Caching.  CAT is slow.
	// The problem with caching local transforms is that
	// procedural controllers can change their transform depending
	// on their parent matrix.  However, if our parent has not
	// changed, then we can safely return the last calculated matrix.
	Matrix3 tmLastEvalParent;

	// The validity of the local transform (tmBoneLocal)
	Interval mLocalValid;

protected:

	// The validity of the world transform (tmBoneWorld)
	// This is protected (not private), because BoneSegTrans
	// needs to set it directly, it does not call EvaluateWorldTrans
	Interval mWorldValid;

	// This holds the transform matrix for stuff
	CATClipMatrix3*		mLayerTrans;

	// these is the scaled dimensions of the bone;
	// it has the same validity interval as the local scale
	Point3	obj_dim;

	// All our Arbitrary Bones
	Tab <CATNodeControl*>	tabArbBones;

	// All our Arbitrary Bones
	Tab <CATClipValue*>	tabArbControllers;

	CATNodeControl	*mpMirrorBone;
	Point3	pos_limits_pos, pos_limits_neg, rot_limits_pos, rot_limits_neg, scl_limits_min, scl_limits_max;

	CATNodeControl();
	~CATNodeControl();

	// Create a node for this object, and assign this controller
	// as the transform controller.  Returns the new node.
	INode*	CreateNode(Object* pCATObject);

	// Get the transform of CATParent (scale will be removed).
	// Return true if succeed, false otherwise.
	bool GetCATParentTM(TimeValue t, Matrix3& tmCATParent);

public:

	INode*	GetNode() const;
	virtual void	SetNode(INode* n);
	INode*	GetParentNode() { return GetNode() ? GetNode()->GetParentNode() : NULL; };
	void	LinkParentChildNodes();

	// Some handy accessor functions for getting our world transform
	// If we have no node, will return
	Matrix3			GetNodeTM(TimeValue t);
	void			SetNodeTM(TimeValue t, Matrix3& tmCurrent);
	Matrix3			GetParentTM(TimeValue t) const;
	Point3			GetLocalScale(TimeValue t);
	const Point3&	GetWorldScale(TimeValue t) const { DbgAssert(mWorldValid.InInterval(t)); return mp3ScaleWorld; }
	Matrix3			GetCurrentTM(TimeValue t) { return mLocalValid.InInterval(t) ? tmBoneWorld : GetNodeTM(t); }
	const Matrix3&	GettmBoneWorld(TimeValue t) const { DbgAssert(mWorldValid.InInterval(t)); return tmBoneWorld; }

	// Returns if it is possible for this node to stretch
	// (simply - whether the stretchy flags are set)
	bool CanStretch(CATMode iCATMode);
	// Returns whether or not this node will stretch
	// given its current selected state etc.
	virtual bool IsStretchy(CATMode iCATMode);

	// Derived from CATControl
	virtual int				NumChildCATControls();
	virtual CATControl*		GetChildCATControl(int i);
	virtual void			ClearChildCATControl(CATControl* pDestructingClass);

	virtual int				NumLayerControllers();
	virtual CATClipValue*	GetLayerController(int i);

	virtual TSTR			GetBoneAddress();
	virtual INode*			GetBoneByAddress(TSTR address);

	// Set new parent node, and link the nodes
	virtual CATNodeControl*	FindParentCATNodeControl() = 0;
	void					SetParentCATNodeControl(CATNodeControl* pNewParent);
	CATNodeControl*			GetParentCATNodeControl(bool bGetCATParent = FALSE);

	virtual int				NumChildCATNodeControls() { return 0; };
	virtual CATNodeControl*	GetChildCATNodeControl(int /*i*/) { return NULL; };
	void					GetAllChildCATNodeControls(Tab<CATNodeControl*>& dAllChilds);

	CatAPI::IBoneGroupManager*	GetManager();

	// TODO: Make these functions private!
	BOOL			IsXReffed();
	Object*			GetObject() { if (INode* node = GetNode()) { return node->GetObjectRef()->FindBaseObject(); } return NULL; };
	ICATObject*		GetICATObject();

	virtual void	SetObjX(float val) { if (IsXReffed()) return; if (ICATObject *iobj = GetICATObject()) { iobj->SetX(val);	UpdateObjDim(); } };
	virtual void	SetObjY(float val) { if (IsXReffed()) return; if (ICATObject *iobj = GetICATObject()) { iobj->SetY(val);	UpdateObjDim(); } };
	virtual void	SetObjZ(float val) { if (IsXReffed()) return; if (ICATObject *iobj = GetICATObject()) { iobj->SetZ(val);	UpdateObjDim(); } };

	virtual float	GetObjX() { if (ICATObject *iobj = GetICATObject()) { return iobj->GetX(); } return 0; };
	virtual float	GetObjY() { if (ICATObject *iobj = GetICATObject()) { return iobj->GetY(); } return 0; };
	virtual float	GetObjZ() { if (ICATObject *iobj = GetICATObject()) { return iobj->GetZ(); } return 0; };

	virtual void	DisplayLayer(TimeValue t, ViewExp *vpt, int flags, Box3 &bbox);
	// this is the normal Max display
	Box3 bbox;
	int				Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);

	Point3 IGetMaxPositionLimits() { return pos_limits_pos; }
	void ISetMaxPositionLimits(Point3 p);
	Point3 IGetMinPositionLimits() { return pos_limits_neg; }
	void ISetMinPositionLimits(Point3 p);

	Point3 IGetMaxRotationLimits() { return rot_limits_pos; }
	void ISetMaxRotationLimits(Point3 p);
	Point3 IGetMinRotationLimits() { return rot_limits_neg; }
	void ISetMinRotationLimits(Point3 p);

	Point3 IGetMinScaleLimits() { return scl_limits_min; }
	void ISetMinScaleLimits(Point3 p);
	Point3 IGetMaxScaleLimits() { return scl_limits_max; }
	void ISetMaxScaleLimits(Point3 p);

	void DisplayPosLimits(TimeValue t, GraphicsWindow *gw);
	void DisplayRotLimits(TimeValue t, GraphicsWindow *gw);
	void DisplaySclLimits(TimeValue t, GraphicsWindow *gw);

	// TODO: Get Rid of this function!
	virtual Point3	GetBoneDimensions() { return GetBoneDimensions(GetCOREInterface()->GetTime()); }
	virtual Point3	GetBoneDimensions(TimeValue t);
	virtual void	SetBoneDimensions(TimeValue t, Point3 dim);

	/** Get the bones object's length (with CATUnits applied)
		This ignores scale.  To get the bones length including scale,
		call GetBoneDimensions */
	float	GetBoneLength();
	/** Set the bones object's length (with CATUnits applied)
		This ignores scale, but modifies the length of the input by
		CATUnits.  For example, if CATUnits = 2, and this function
		is called with 5, the actual length set on the bone would be
		2.5, which would be scaled by CATUnits to become 5 in the scene */
	void	SetBoneLength(float val);

	void	UpdateObject() { if (GetICATObject()) GetICATObject()->Update(); };

	virtual void	UpdateUserProps();

	virtual CATClipMatrix3* GetLayerTrans() const { return mLayerTrans; };

	// Invalidates all caches, and sends a change notification.
	void		InvalidateTransform();

	//////////////////////////////////////////////////////////////////////////
	// Rig Structure functions

	virtual Matrix3 GetSetupMatrix();
	virtual void SetSetupMatrix(Matrix3 tmSetup);
	void CreateSetupController(BOOL tf);
	BOOL IsUsingSetupController() const;

	// Offset the passed matrix such that any
	// it can be used as the parent matrix for this
	// bones child.  For example, limbbones/tails/spines
	// offset their children the length of their bone
	// Spines reset the parent matrix of their tip
	// hubs by their base matrix.
	// Hubs themselves do not offset their children.
	virtual void ApplyChildOffset(TimeValue t, Matrix3& tmChild) const;

	// Apply the Setup value.  Children can override this
	// if they want to do things differently.
	// TODO: Document this!
	virtual void ApplySetupOffset(TimeValue t, Matrix3 tmOrigParent, Matrix3& tmWorldTransform, Point3 &p3BoneLocalScale, Interval& ivValid);

	// Allows the CATNode to modify the incoming parent
	// matrix.  The default implementations resets the parent
	// matrix in SetupMode - this allows bones to return to the
	// original hierarchy in SetupMode and ignore user-modified
	// parenting.
	virtual void CalcParentTransform(TimeValue t, Matrix3& tmParent, Point3& p3ParentScale);

	// Calculate the values that our transforms
	// will be evaluated relative to.  This function
	// handles inheritance, and will remove scale from the
	// the transform.  :
	// \param tmParent [in|out] - the (max-defined) parent matrix
	//		passed to our GetValues and SetValues.  This matrix
	//		is modified, and should be used for as the parent of the
	//		current node.
	virtual void CalcInheritance(TimeValue t, Matrix3& tmParent, Point3& p3ParentScale);

	// given the input parent matrix, calculate the world matrix for this
	// bone.  tmWorld is passed as the parent matrix - add the local transform to this.
	// Do not set the scale directly on the matrix, instead return it seperately in
	// the p3LocalScale variable
	// \param tmParent - The original parent matrix.  Currently only needed by CATClipMatrix3 to calculate setup position offsets.
	// \param tmWorld [in|out] - Passed in as the parent matrix to work from.  Should be returned as the
	//		world matrix for this bone, sans scale
	// \param p3LocalScale [out] - If the bone has local scale, return it in this parameter
	// \param ivLocalValid [in|out] - Return the validity interval for the bones local transform (ignoring parent)
	virtual void CalcWorldTransform(TimeValue t, const Matrix3& tmOrigParent, const Point3& p3ParentScale, Matrix3& tmWorld, Point3& p3LocalScale, Interval& ivLocalValid);

	// Set the world transform
	virtual void SetWorldTransform(TimeValue t, const Matrix3& tmOrigParent, const Point3& p3ParentScale, SetXFormPacket* packet, int commit, GetSetMethod method);

#ifdef FBIK
	virtual void FBIKInit();
	virtual void FBIKPullBones(TimeValue t, CATNodeControl *src, Ray force);
	virtual void FBIKUpdateGlobalTM(TimeValue t);
#endif

	void ImposeLimits(TimeValue t, const Matrix3 &tmBoneParent, Matrix3 &tmBoneWorld, Point3& p3ScaleLocal, Interval& ivValid);

	/** Scale the bone of this CATNodeControl by the given scale in its local space. */
	void ScaleObjectSize(Point3 p3LocalScale);

	// RigSaving/Loading ID
	BOOL SaveRigArbBones(CATRigWriter *save);
	BOOL LoadRigArbBones(CATRigReader *load);

	BOOL SaveRigCAs(CATRigWriter *save);
	BOOL LoadRigCAs(CATRigReader *load);

	virtual BOOL SaveRig(CATRigWriter *save);
	virtual BOOL LoadRig(CATRigReader *load);

	// These implementations of SaveClip are common across all nodes and should not be overridden
	BOOL SaveClip(CATRigWriter* save, int flags, Interval timerange, int layerindex);
	BOOL LoadClip(CATRigReader *load, Interval range, int layerindex, float dScale, int flags);

	// Implement these functions to save and load custom data for a bone
	virtual bool SaveClipOthers(CATRigWriter* /*save*/, int /*flags*/, Interval /*timerange*/, int /*layerindex*/) { return true; };
	// Implement these functions to load custom data for a bone.  The function will be called for
	// every tag loaded, giving the bone the option to override default loading behaviour and/or
	// implement loading for custom tags.  Return true to indicate that the item has been loaded, or false
	// to indicate it has not been loaded.
	virtual bool LoadClipOthers(CATRigReader* /*load*/, Interval /*range*/, int /*layerindex*/, float /*dScale*/, int /*flags*/, int& /*data*/) { return false; };

	// Save clip info for arb bones and extra controllers
	BOOL SaveClipArbBones(CATRigWriter* save, int flags, Interval timerange, int layerindex);
	BOOL LoadClipArbBones(CATRigReader *load, Interval range, int layerindex, float dScale, int flags);
	BOOL SaveClipExtraControllers(CATRigWriter* save, int flags, Interval timerange, int layerindex);
	BOOL LoadClipExtraControllers(CATRigReader *load, Interval range, int layerindex, float dScale, int flags);

	virtual CatAPI::INodeControl*	GetArbBoneINodeControl(int i);
	virtual int						GetNumArbBones();
	virtual CatAPI::INodeControl*	AddArbBone(BOOL bAsNewGroup = FALSE);
	CATNodeControl*	AddArbBone(BOOL loading, BOOL bAsNewGroup = FALSE);

	CATNodeControl*	GetArbBone(int i);
	void			RemoveArbBone(CATNodeControl* pArbBone, BOOL deletenode = TRUE);
	void			InsertArbBone(int id, CATNodeControl *arbbone);
	void			CleanUpArbBones();

	// Create an ArbBone, but don't add the node.  Useful for assigning
	// to non-CAT objects.
	Control*			CreateArbBoneController(BOOL bAsNewGroup = FALSE);

	///////////////////////////////////////////////////////////////
	// Arbirary controllers to be used on CAs
	CATClipValue*	CreateLayerFloat();
	int				FindLayerFloat(Control *ctrl);
	void			RemoveLayerFloat(int id);
	void			InsertLayerFloat(int id, CATClipValue *arb_controller);
	CATClipValue*	GetLayerFloat(int i);
	int				NumLayerFloats() { return tabArbControllers.Count(); };

	void CloneCATNodeControl(CATNodeControl* clonedctrl, RemapDir& remap);

	virtual void		UpdateVisibility();
	virtual void		UpdateCATUnits();
	virtual void		UpdateObjDim();
	Point3				GetObjDim() { return obj_dim; }

	virtual void		CATMessage(TimeValue t, UINT msg, int data = -1);
	virtual void		KeyFreeform(TimeValue t, ULONG flags = KEY_ALL);

	virtual Color		GetCurrentColour(TimeValue t);
	virtual void		UpdateColour(TimeValue t);
	virtual Color		GetBonesRigColour();;
	virtual void		SetBonesRigColour(Color clr);;

	virtual TSTR		GetRigName();
	virtual void		UpdateName();

	BOOL PasteLayer(CATControl* pastectrl, int fromindex, int toindex, DWORD flags, RemapDir &remap);
	BOOL PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor);

	virtual BOOL PasteERNNodes(CATControl* pastectrl, CATCharacterRemap &remap);
	virtual BOOL BuildMapping(CATControl* pastectrl, CATCharacterRemap &remap, BOOL includeERNnodes);

	// here this method is for filling out the
	virtual void AddSystemNodes(INodeTab &nodes, SysNodeContext ctxt);

	//////////////////////////////////////////////////////////////////////////
	// The beginnings of full-body IK
	virtual void ApplyForce(Point3 &force, Point3 &force_origin, AngAxis &rotation, Point3 &rotation_origin, CATNodeControl *source);

	//////////////////////////////////////////////////////////////////////////
	virtual void SetLengthAxis(int axis);

	//////////////////////////////////////////////////////////////////////////
	//
	//
public:

	SClass_ID SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }

	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	// Interfacing between Max classes
	void* GetInterface(ULONG id) {
		if (id == I_CATNODECONTROL) 	return (void*)this;
		if (id == I_EXTRARIGNODES)	return ExtraRigNodes::GetInterface(id);
		return CATControl::GetInterface(id);
	}

	//////////////////////////////////////////////////////////////////////////
	// from class Control:
	//

	// Called whenever another class begins to reference us.
	// We use this callback to trap our Node pointer.
	void RefAdded(RefMakerHandle rm);

	// Called whenever a reference to us is deleted.  The ONLY
	// classes that should reference us is either the Node or the
	// owning class (Limb/Bone/Spine/Tail)Data etc.
	void RefDeleted();

	// Called when we are being deleted.  This is necessary, because
	// RefDeleted will NOT be called when theHold is not active.
	RefResult AutoDelete();

	// Unlink parents/children pointers, basically pre-delete.
	void UnlinkCATHierarchy();
	// Unlink the manager (if it exists, and has no bones left)
	void UnlinkBoneManager();

	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);
	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method);

	// Modify incoming SetValue to enforce locks.
	bool ApplySetValueLocks(TimeValue t, SetXFormPacket * ptr, int iCATMode);

	// Apply a rotation and/or stretch to accomodate a move.
	void SetValueMoveStretch(TimeValue t, SetXFormPacket * ptr, bool bIStretch, bool bParStretch, const Matrix3 &tmOrigParent, CATNodeControl* pParent);

	// SetValue implementation functions
	// p3Pivot is not const & reference, to prevent accidental changes
	// when it is a reference to tmCurrent.GetTrans() (See BoneSegTrans::CalcPositionStretch)
	virtual void CalcPositionStretch(TimeValue t, Matrix3& tmCurrent, Point3 p3Pivot, const Point3& p3Initial, const Point3& p3Final);

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL);

	virtual void UpdateUI();

	bool GetLocalTMComponents(TimeValue t, TMComponentsArg& cmpts, Matrix3Indirect& parentMatrix);;

	// It is not legal to instance a CAT transform controller
	// We assume the node pointer we use is the only one.
	BOOL CanInstanceController() { return FALSE; }

	void PostCloneNode();

	int GetKeySelState(BitArray &sel, Interval range, DWORD flags) { if (GetLayerTrans()) return GetLayerTrans()->GetKeySelState(sel, range, flags); return 0; }

	// Sub-Controllers
	Control *GetPositionController();
	Control *GetRotationController();
	Control *GetScaleController() { return GetLayerTrans() ? GetLayerTrans()->GetScaleController() : NULL; };
	BOOL SetPositionController(Control *c) { return GetLayerTrans() ? GetLayerTrans()->SetPositionController(c) : FALSE; };
	BOOL SetRotationController(Control *c) { return GetLayerTrans() ? GetLayerTrans()->SetRotationController(c) : FALSE; };
	BOOL SetScaleController(Control *c) { return GetLayerTrans() ? GetLayerTrans()->SetScaleController(c) : FALSE; };

	void CommitValue(TimeValue t) { if (GetLayerTrans()) GetLayerTrans()->CommitValue(t); };
	void RestoreValue(TimeValue t) { if (GetLayerTrans()) GetLayerTrans()->RestoreValue(t); };

	DWORD GetInheritanceFlags() { return GetLayerTrans() ? GetLayerTrans()->GetInheritanceFlags() : INHERIT_ALL; }
	BOOL  SetInheritanceFlags(DWORD f, BOOL keepPos) { return GetLayerTrans() ? GetLayerTrans()->SetInheritanceFlags(f, keepPos) : FALSE; }

	// From control -- for apparatus manipulation
	void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp *vpt, Box3& box);//{							if(GetLayerTrans())		 GetLayerTrans()->GetWorldBoundBox(t,inode, vpt, box);	};
	int  HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt) { return GetLayerTrans() ? GetLayerTrans()->HitTest(t, inode, type, crossing, flags, p, vpt) : 0; };
	void ActivateSubobjSel(int level, XFormModes& modes) { if (GetLayerTrans())		 GetLayerTrans()->ActivateSubobjSel(level, modes); };
	void SelectSubComponent(CtrlHitRecord *hitRec, BOOL selected, BOOL all) { if (GetLayerTrans())		 GetLayerTrans()->SelectSubComponent(hitRec, selected, all); };
	void ClearSelection(int selLevel) { if (GetLayerTrans())		 GetLayerTrans()->ClearSelection(selLevel); };
	int  SubObjectIndex(CtrlHitRecord *hitRec) { return GetLayerTrans() ? GetLayerTrans()->SubObjectIndex(hitRec) : 0; };
	void SelectSubComponent(CtrlHitRecord *hitRec, BOOL selected, BOOL all, BOOL invert) { if (GetLayerTrans())		 GetLayerTrans()->SelectSubComponent(hitRec, selected, all, invert); };

	void GetSubObjectCenters(SubObjAxisCallback *cb, TimeValue t, INode *node) { if (GetLayerTrans())		 GetLayerTrans()->GetSubObjectCenters(cb, t, node); };
	void GetSubObjectTMs(SubObjAxisCallback *cb, TimeValue t, INode *node) { if (GetLayerTrans())		 GetLayerTrans()->GetSubObjectTMs(cb, t, node); };
	void SubMove(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin = FALSE) { if (GetLayerTrans())		 GetLayerTrans()->SubMove(t, partm, tmAxis, val, localOrigin); };;
	void SubRotate(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Quat& val, BOOL localOrigin = FALSE) { if (GetLayerTrans())		 GetLayerTrans()->SubRotate(t, partm, tmAxis, val, localOrigin); };;
	void SubScale(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin = FALSE) { if (GetLayerTrans())		 GetLayerTrans()->SubScale(t, partm, tmAxis, val, localOrigin); };;

	// Transform Controller methods
	BOOL InheritsParentTransform() { return TRUE; }

	// When being re-linked, try to determine if any parent in our tree
	// belongs to this CATRig.  We need to know this so we can set
	// the HasTransform flag, which tells us what matrix to
	// inherit from in SetupMode, and how to apply the
	// LayerTransformGizmo in animation mode.
	void CalculateHasTransformFlag();

	BOOL ChangeParents(TimeValue t, const Matrix3& oldP, const Matrix3& newP, const Matrix3& tm);

	// IsLeaf / IsKeyable / IsColor
	BOOL IsLeaf() { return FALSE; }
	int IsKeyable() { return GetLayerTrans() ? GetLayerTrans()->IsKeyable() : FALSE; };
	// New Controller Assignment
	BOOL AssignController(Animatable *control, int subAnim) { UNREFERENCED_PARAMETER(control); UNREFERENCED_PARAMETER(subAnim); return FALSE; };
	DWORD GetSubAnimCurveColor(int) { return PAINTCURVE_GENCOLOR; }
	ParamDimension* GetParamDimension(int) { return defaultDim; }

	//////////////////////////////////////////////////////////////////////////
	// Function publishing

	// all the flags we see in the hierarchy panel
	virtual BOOL GetSetupStretchy() const { return TestCCFlag(CCFLAG_SETUP_STRETCHY); };
	virtual void SetSetupStretchy(BOOL b) { SetCCFlag(CCFLAG_SETUP_STRETCHY, b); };
	virtual BOOL GetAnimationStretchy() const { return TestCCFlag(CCFLAG_ANIM_STRETCHY); };
	virtual void SetAnimationStretchy(BOOL b) { SetCCFlag(CCFLAG_ANIM_STRETCHY, b); };
	virtual BOOL GetEffectHierarchy() { return TestCCFlag(CCFLAG_EFFECT_HIERARCHY); };
	virtual void SetEffectHierarchy(BOOL b) { SetCCFlag(CCFLAG_EFFECT_HIERARCHY, b); };
	virtual BOOL GetUseSetupController() const { return IsUsingSetupController() == TRUE; }
	virtual void SetUseSetupController(BOOL b) { CreateSetupController(b); }

	virtual BOOL		GetRelativeToSetupMode();
	virtual void		SetRelativeToSetupMode(BOOL isRelative);

	virtual BitArray*	GetSetupModeLocks();
	virtual void		SetSetupModeLocks(BitArray *locks);
	virtual BitArray*	GetAnimationLocks();
	virtual void		SetAnimationLocks(BitArray *locks);

	virtual BitArray*	GetSetupModeInheritance();
	virtual void		SetSetupModeInheritance(BitArray* v);
	virtual BitArray*	GetAnimModeInheritance();
	virtual void		SetAnimModeInheritance(BitArray* v);

	virtual INode*		GetMirrorBone();
	virtual void		SetMirrorBone(INode* node);

	virtual BaseInterface* GetInterface(Interface_ID id);

	FPInterfaceDesc* GetDescByID(Interface_ID id);

	virtual void OnRestoreDataChanged(DWORD val);

};

// This class desc is provided for classes that
// derive from CATNodeControl
class CATNodeControlClassDesc : public CATControlClassDesc
{
public:
	CATNodeControlClassDesc();

	// Default values
	SClass_ID		SuperClassID();
};
