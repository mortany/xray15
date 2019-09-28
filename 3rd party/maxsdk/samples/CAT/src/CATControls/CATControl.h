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

#include "ICATParent.h"
#include "../CATObjects/ICATObject.h"
#include "FnPub/ICATControlFP.h"
#include <CATAPI/CATClassID.h>
#include "CATGroup.h"

class CATRigReader;
class CATRigWriter;

class ECATParent;
class ICATObject;
class CATControl;
class CATNodeControl;
class ArbBoneTrans;
class CATCharacterRemap;
class CATClipRoot;

// our global instance of our classdesc class.
extern ClassDesc2* GetArbBoneTransDesc();
extern ClassDesc2* GetLimbData2Desc();
extern ClassDesc2* GetSpineData2Desc();
extern ClassDesc2* GetSpineTrans2Desc();
extern ClassDesc2* GetDigitSegTransDesc();
extern ClassDesc2* GetDigitDataDesc();
extern ClassDesc2* GetBoneDataDesc();
extern ClassDesc2* GetCollarboneTransDesc();
extern ClassDesc2* GetPalmTrans2Desc();
extern ClassDesc2* GetBoneSegTransDesc();
extern ClassDesc2* GetHubDesc();

extern ClassDesc2* GetTailTransDesc();
extern ClassDesc2* GetTailData2Desc();

extern USHORT GetNextRigID(TSTR &address, int &boneid);

/**********************************************************************
 * ICATNodeControlFP: Published functions for CATParent
 */
#define I_CATCONTROL		0xd83dc1

 /////////////////////////////////////////////////////
 // CATControl
 // Superclass for all controllers in CAT
#define CCFLAG_FB_IK_LOCKED					(1<<1)//2
#define CCFLAG_FB_IK_BYPASS					(1<<2)//4
#define CCFLAG_EFFECT_HIERARCHY				(1<<4)//4

// we save the INode lock flags here with the
// CATControl flags.
#define CNCFLAG_LOCK_LOCAL_POS				(1<<5)
#define CNCFLAG_LOCK_LOCAL_ROT				(1<<6)
#define CNCFLAG_LOCK_LOCAL_SCL				(1<<7)
#define CCFLAG_SETUP_STRETCHY						(1<<3)//4

#define CNCFLAG_LOCK_SETUPMODE_LOCAL_POS	(1<<8)
#define CNCFLAG_LOCK_SETUPMODE_LOCAL_ROT	(1<<9)
#define CNCFLAG_LOCK_SETUPMODE_LOCAL_SCL	(1<<10)
#define CCFLAG_ANIM_STRETCHY				(1<<12)

// we set this flag when we want all systems
// to bail immediately from all getvalues
#define CNCFLAG_LOCK_STOP_EVALUATING		(1<<11)
#define CNCFLAG_RETURN_EXTRA_KEYS			(1<<13)
#define CNCFLAG_EVALUATING					(1<<14)

#define CNCFLAG_DISPLAY_ONION_SKINS			(1<<20)

#define CNCFLAG_IMPOSE_POS_LIMITS			(1<<21)
#define CNCFLAG_IMPOSE_ROT_LIMITS			(1<<22)
#define CNCFLAG_IMPOSE_SCL_LIMITS			(1<<23)

#define CNCFLAG_INHERIT_ANIM_POS			(1<<25)
#define CNCFLAG_INHERIT_ANIM_ROT			(1<<26)
#define CNCFLAG_INHERIT_ANIM_SCL			(1<<27)
#define CNCFLAG_INHERIT_ANIM_ALL			(CNCFLAG_INHERIT_ANIM_POS|CNCFLAG_INHERIT_ANIM_ROT|CNCFLAG_INHERIT_ANIM_SCL)

#define CNCFLAG_KEEP_ROLLOUTS				(1<<28)

#define KEY_POSITION		(1<<0)
#define KEY_ROTATION		(1<<1)
#define KEY_SCALE			(1<<2)
#define KEY_ALL				(KEY_POSITION|KEY_ROTATION|KEY_SCALE)

class CATControl : public Control, public ICATControlFP
{
	friend class SetCCFlagRestore;
	friend class CATControlPLCB;
	friend class SuppressLinkInfoUpdate;

private:
	ICATParentTrans*	mpCATParentTrans;

	// This BoneID identifies
	// the index of each CAT rig element
	int					miBoneID;

protected:

	DWORD				ccflags;
	TSTR				name;
	DWORD				dwFileSaveVersion;

	// keeps track of the panel our rollout is being displayed on
	static int flagsbegin;
	static bool suppressLinkInfoUpdate;
	// Class Interface pointer
	static IObjParam *ipbegin;

public:

#pragma region A few helper classes for RAII management of flags

	class SuppressLinkInfoUpdate {
	public:
		SuppressLinkInfoUpdate();
		~SuppressLinkInfoUpdate();
	};

	// This little helper class turns on the CLIP KeyFreeform
	// flag for a given scope
	class KeyFreeformMode {
		CATClipValue* m_pKeyingVal;

		// While keying, prevent message propagation
		// (technically, there should be no change anyway)
		MaxReferenceMsgLock lockThis;
	public:
		KeyFreeformMode(CATClipValue* pKeyingVal);
		~KeyFreeformMode();
	};

	// This little helper class turns on the CLIP KeyFreeform
	// flag for a given scope
	class BlockEvaluation {
	private:

		// The class we are locking.
		CATControl* mBlockedClass;

		// Block the default constructor.
		BlockEvaluation();

	public:
		BlockEvaluation(CATControl* pBlockedClass);
		~BlockEvaluation();
	};

#pragma endregion

	CATControl();
	~CATControl();

	// This implementation should be good for all derivative classes.
	void DeleteThis() { delete this; }

	void SetCCFlag(ULONG f, BOOL on = TRUE);
	void ClearCCFlag(ULONG f) { SetCCFlag(f, FALSE); }
	bool TestCCFlag(ULONG f) const { return (ccflags & f) == f; };

	// Every cat controller has an ID
	int		GetBoneID() const { return miBoneID; }
	void	SetBoneID(int id);

	//////////////////////////////////////////////////////////////////////////
	// CATHierarchy Operations
	virtual CATGroup* GetGroup() = 0;
	CATClipWeights* GetClipWeights();

	// returns the CATParentTrans
	// if bSearch is true, it is guaranteed that a valid pointer is returned.
	// if not, then the cached pointer is returned directly.
	ICATParentTrans*	GetCATParentTrans(bool bSearch = true);
	const ICATParentTrans*	GetCATParentTrans() const { return const_cast<CATControl*>(this)->GetCATParentTrans(); }

	// Search up the CAT tree to find a parent.  Returns the first pointer found within the rig.
	virtual ICATParentTrans*	FindCATParentTrans();

	// Set the CATParent pointer.  This is primarily called to set the CATParent on the
	// root hub, and children of the hub find the CATParent from there.  It is also
	// used to NULL the CATParent pointer on deletion
	void SetCATParentTrans(ICATParentTrans* pParent) { mpCATParentTrans = pParent; }

	DWORD GetFileSaveVersion();

	inline float	GetCATUnits() const { return GetCATParentTrans() ? GetCATParentTrans()->GetCATUnits() : 0.3f; }
	inline int		GetLengthAxis() const { return GetCATParentTrans() ? const_cast<ICATParentTrans*>(GetCATParentTrans())->GetLengthAxis() : X; }
	CATMode			GetCATMode() const;

	virtual TSTR GetName() { return  name; }
	virtual void SetName(TSTR newname, BOOL quiet = FALSE);

	virtual TSTR	GetBoneAddress();
	virtual INode*	GetBoneByAddress(TSTR address);

	virtual void CATMessage(TimeValue t, UINT msg, int data = -1);
	virtual void KeyFreeform(TimeValue t, ULONG flags = KEY_ALL);
	virtual void SetLengthAxis(int axis);

	virtual void DisplayLayer(TimeValue t, ViewExp *vpt, int flags, Box3 &bbox);

	BOOL IsEvaluationBlocked() { return TestCCFlag(CNCFLAG_LOCK_STOP_EVALUATING); }

	//////////////////////////////////////////////////////////////////////////
	// Rig Creation/Maintenance ect.
	virtual USHORT	GetRigID() = 0;

	// These functions can be used to iterate the CATCharacter.
	// Note there is no "Set", these pointers are hard-coded,
	// but there is a function "Clear" down below that can be
	// used to remove these pointers on class destruction.
	void				GetAllChildCATControls(Tab <CATControl*> &dAllChildren);
	virtual int			NumChildCATControls() { return 0; };
	virtual CATControl*	GetChildCATControl(int /*i*/) { return NULL; };
	virtual CATControl*	GetParentCATControl() = 0;

	virtual int NumLayerControllers() = 0;
	virtual CATClipValue* GetLayerController(int i) = 0;

	virtual void UpdateUI() {};
	virtual void Update() { CATMessage(GetCOREInterface()->GetTime(), CAT_UPDATE); };

	virtual BOOL SaveRig(CATRigWriter *save) = 0;
	virtual BOOL LoadRig(CATRigReader *load) = 0;

	virtual BOOL SaveClip(CATRigWriter* save, int flags, Interval timerange, int layerindex) = 0;
	virtual BOOL LoadClip(CATRigReader *load, Interval range, int layerindex, float dScale, int flags) = 0;

	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	void CloneCATControl(CATControl* clonedctrl, RemapDir& remap);

	//////////////////////////////////////////////////////////////////////////
	// Layer Ranges operations
	// We override these methods to do special things with
	// key ranges.  This allows us to use the track view as
	// a non-linear animation editor.  Isn't that cool?!
	Interval GetLayerTimeRange(int index, DWORD flags);
	void EditLayerTimeRange(int index, Interval range, DWORD flags);
	void MapLayerKeys(int index, TimeMap *map, DWORD flags);
	int	 GetKeyTimes(Tab<TimeValue> &times, Interval range, DWORD flags);
	BOOL IsKeyAtTime(TimeValue t, DWORD flags);
	BOOL GetNextKeyTime(TimeValue t, DWORD flags, TimeValue &nt);

	void SetLayerORT(int index, int ort, int type);
	void EnableLayerORTs(int index, BOOL enable);

	//////////////////////////////////////////////////////////////////////////
	virtual BOOL PasteLayer(CATControl* pastectrl, int fromindex, int toindex, DWORD flags, RemapDir &remap);
	virtual BOOL PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor);

	virtual BOOL PasteERNNodes(CATControl* pastectrl, CATCharacterRemap &remap);
	virtual BOOL BuildMapping(CATControl* pastectrl, CATCharacterRemap &remap, BOOL includeERNnodes);

	virtual	void AddSystemNodes(INodeTab &nodes, SysNodeContext ctxt);
	virtual	void AddLayerControllers(Tab <Control*>	 &layerctrls);
	virtual void DeleteBoneHierarchy();

	virtual void ApplyForce(Point3 &force, Point3 &force_origin, AngAxis &rotation, Point3 &rotation_origin, CATNodeControl *source)
	{
		UNREFERENCED_PARAMETER(force); UNREFERENCED_PARAMETER(force_origin); UNREFERENCED_PARAMETER(rotation);
		UNREFERENCED_PARAMETER(rotation_origin); UNREFERENCED_PARAMETER(source);
	};

	///////////////////////////////////////////////////////////////////////
	// From Class Control
	// These methods must be implemented by all controllers, so we do here.
	virtual BOOL IsLeaf() { return FALSE; }
	virtual BOOL CanCopyAnim() { return FALSE; }				// can we be copied and pasted?
	virtual void GetValue(TimeValue, void *, Interval &, GetSetMethod) {};
	virtual void SetValue(TimeValue, void *, int, GetSetMethod) {};
	// if we are being assigned over the top of a controller, we could try and copy data of it. But we aren't public so it doesn't make sense
	virtual void Copy(Control *) {};
	BOOL IsReplaceable() { return FALSE; };

	void CopyKeysFromTime(TimeValue src, TimeValue dst, DWORD flags);
	void AddNewKey(TimeValue t, DWORD flags);

	//////////////////////////////////////////////////////////////////////////
	// This is the interfaces system we use to access other controllers is C++
	void* GetInterface(ULONG id)
	{
		if (id == I_MASTER)
		{
			CATGroup* pGroup = GetGroup();
			return (pGroup != NULL) ? pGroup->AsCATControl() : NULL;
		}
		if (id == I_CATCONTROL)		return (CATControl*)this;
		return Control::GetInterface(id);
	}

	virtual	void GetSystemNodes(INodeTab &nodes, SysNodeContext ctxt);

	//////////////////////////////////////////////////////////////////////////
	// Function Publishing

	// These special functions are the ones exposed to script.
	virtual void PasteLayer(Control* ctrl, int fromindex, int toindex, BOOL instance);
	virtual BOOL PasteFromCtrl(ReferenceTarget* ctrl, BOOL bMirrorData);

	// Loading / Saving
	// You can only save clips 1 layer at a time
	// This is because it makes no sense to add a layer when loading a clip for a bodypart only
	virtual BOOL SaveClip(TSTR filename, TimeValue start_t, TimeValue end_t);
	virtual INode* LoadClip(TSTR filename, TimeValue t, BOOL bMirrorData);

	virtual BOOL SavePose(TSTR filename);
	virtual INode* LoadPose(TSTR filename, TimeValue t, BOOL bMirrorData);

	void CollapsePoseToCurrLayer(TimeValue t);
	void CollapseTimeRangeToCurrLayer(TimeValue start_t, TimeValue end_t, TimeValue freq);
	void ResetTransforms(TimeValue t);

	virtual BaseInterface* GetInterface(Interface_ID id);
	FPInterfaceDesc* GetDescByID(Interface_ID id);

	INode* GetCATParent() { return GetCATParentTrans() ? GetCATParentTrans()->GetNode() : nullptr; }

	// Return true if it is possible to paste to this CATControl
	BOOL CanPasteControl();
	static CATControl* GetPasteControl();
	static void SetPasteControl(CATControl* pasteCATControl);

	// The job of this function is to remove this class from the CATControl
	// hierarchy.  It simply NULL's the pointer of this class to its children,
	// and it's children's pointer to this, and it's parent's pointer to this,
	// and this's pointer to its parent.  This should be called on
	// deletion to ensure that no matter what happens no invalid
	// pointers are left in the parameter blocks. (RMPG-109)
	void DestructCATControlHierarchy();

	// Called on destruction.  This function finds all CATMotion
	// layers on this bone and triggers cleanup on the
	// CATMotionControllers and CATHierarchyBranch/Leaf hierarchies.
	virtual void DestructAllCATMotionLayers();

	// On deletion, clean up any pointers to us.
	RefResult AutoDelete();

	// Added to allow an automatic destruction of CAT pointers
	// This is to ensure that we never have hanging pointers
	// between CAT items.  This is called in
	// DestructCATControlHierarchy only.
	virtual void ClearParentCATControl();
	virtual void ClearChildCATControl(CATControl* pDestructingClass);

private:

	// Do not allow direct access to this function
	// Instead, use BlockEvaluations helper member class
	void BlockAllEvaluation(BOOL tf) { HoldSuspend hs; SetCCFlag(CNCFLAG_LOCK_STOP_EVALUATING, tf); };

	static CATControl* mpPasteCtrl;
};

//* Do not merge forward
extern bool NullPointerInParamBlock(IParamBlock2* pblock, Control* pSomePtr);
// This class is provided to be the base ClassDesc
// for classes that derive from CATControl
class CATControlClassDesc : public ClassDesc2
{
public:
	CATControlClassDesc();
	~CATControlClassDesc();

	// Default values
	SClass_ID		SuperClassID();				// By default, we are REF_TARG_CLASS_ID
	int 			IsPublic();					// Never show this in create branch
	const TCHAR* 	Category();
	HINSTANCE		HInstance();

	// Instead of the usual Create Fn's, CATControls
	// should implement this Create Fn to do their
	// Creation.  This ensures appropriate down-casting
	virtual CATControl*		DoCreate(BOOL loading) = 0;

private:
	// Implement the base class Create fn.  If we used
	// this fn directly, we run the risk of returning
	// the wrong pointer C++ doesn't cast the returned
	// pointer to the type Max expects (RefTarget)
	void* Create(BOOL loading) { return DoCreate(loading); }
};
