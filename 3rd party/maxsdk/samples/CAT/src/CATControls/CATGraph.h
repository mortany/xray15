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

// We expect to have this later...
class CATHierarchyBranch;
class CATHierarchyBranch2;
class CATKey;
class CATMotionLimb;

/////////////////////////////////////////////////////
// CATGraph
// Superclass for all graph controller. Very handy for the ui

enum CATGraphDim {
	DEFAULT_DIM,
	ANGLE_DIM,
	TIME_DIM
};

#define I_CATGRAPH		0x1d954124

class CATGraph : public Control
{

protected:
	IParamBlock2	*pblock;	//ref 0
	CATMotionLimb	*catmotionlimb;

	int		dim;
	float	flipval;

public:
	enum EnumRefs {
		PBLOCK_REF,
		REF_CATMOTIONLIMB,
		NUMREFS
	};
	enum CATGraphparams {
		PB_LIMBDATA,
		PB_CATBRANCH
	};

	CATGraph();
	~CATGraph();

	SClass_ID SuperClassID() { return CTRL_FLOAT_CLASS_ID; }

	//
	// from class Control:
	//
#ifndef CAT3
	int NumSubs() { return 1; }
	TSTR SubAnimName(int i) { GetString(IDS_PBLOCK); }
	Animatable* SubAnim(int i) { return pblock; }
	int NumRefs() { return NUMREFS; }
#else
	int NumSubs() { return 1; }
	TSTR SubAnimName(int i);
	Animatable* SubAnim(int i);
	int NumRefs() { return NUMREFS; }
#endif
	RefTargetHandle GetReference(int i);// { return pblock; }
private:
	virtual void SetReference(int i, RefTargetHandle rtarg);// { pblock=(IParamBlock2*)rtarg; }
public:

	int	NumParamBlocks() { return 1; }							// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return pblock; }		// return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
	{
		UNREFERENCED_PARAMETER(t); UNREFERENCED_PARAMETER(val); UNREFERENCED_PARAMETER(commit); UNREFERENCED_PARAMETER(method);
	};
	int IsKeyable() { return FALSE; };
	BOOL IsLeaf() { return FALSE; }

	//		BOOL CanMakeUnique(){ return FALSE; };
	//		BOOL CanCopyTrack(){ return FALSE; };
	BOOL IsAnimated() { return TRUE; };

	Interval GetTimeRange(DWORD flags) { UNREFERENCED_PARAMETER(flags);  return GetCOREInterface()->GetAnimRange(); };

	//RefTargetHandle Clone( RemapDir &remap );
	RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL) { return REF_SUCCEED; };

	BOOL AssignController(Animatable *control, int subAnim);

	//////////////////////////////////////////////////////////////////////////
	//Class Animateable

	void* GetInterface(ULONG id) {
		if (id == I_CATGRAPH) 		return (void*)this;
		return						Control::GetInterface(id);
	}

	/*
		*	Class CATGraph
		*/
		//		CATMotionLimb *limb;

	COLORREF GetGraphColour();
	TSTR GetGraphName();

	virtual CATMotionLimb*	GetCATMotionLimb();
	virtual void	SetCATMotionLimb(Control* newLimb);

	int				GetUnits() { return dim; }
	void			SetUnits(int newdim) { dim = newdim; }

	//		LimbData2*		GetLimb();
	//		void			SetLimb(Control* newLimb);

	CATHierarchyBranch2* GetBranch();
	void				SetBranch(CATHierarchyBranch* newBranch);
	void RegisterLimb(int stringID, CATMotionLimb *limb, CATHierarchyBranch* branch);
	void UnRegisterLimb(CATMotionLimb* limbOld);

	void DrawGraph(TimeValue t,
		bool	isActive,
		int		nSelectedKey,
		HDC		hGraphDC,
		int		iGraphWidth,
		int		iGraphHeight,
		float	alpha,
		float	beta);
	void DrawKeys(const TimeValue	t,
		const int	iGraphWidth,
		const int	iGraphHeight,
		const float		alpha,
		const float		beta,
		HDC				&hGraphDC,
		const int		nSelectedKey);
	void DrawKeyKnob(Point2 center,
		HDC			&hGraphDC,
		const int iGraphWidth,
		const int		size,
		const bool hot);
	void DrawKey(const CATKey key1,
		const bool		isOutTan1,
		const Point2	outTan,
		const bool		key1Hot,
		const CATKey key2,
		const bool		isInTan2,
		const Point2	inTan,
		const bool		key2Hot,
		HDC		&hGraphDC,
		HPEN	&keyPen,
		HPEN	&hotKeyPen,
		const int iGraphWidth);
	void	DrawKeyLine(HDC			&hGraphDC,
		const Point2	start,
		const Point2	end,
		const int iGraphWidth,
		const bool hot);

	/*
		Virtual Methods
	*/

	// set up lots of pointers so that we don't need to get them during getvalues
	virtual void	InitControls() {};
	virtual int		GetNumGraphKeys() { return 0; };

	//	virtual CATKey	GetCATKey(int keynum) = 0;
	virtual void	GetCATKey(const int	i, const TimeValue t, CATKey& /*key*/, bool &isInTan, bool &isOutTan) const
	{
		UNREFERENCED_PARAMETER(i); UNREFERENCED_PARAMETER(t); UNREFERENCED_PARAMETER(isInTan); UNREFERENCED_PARAMETER(isOutTan);
	};

	virtual void GetGraphKey(
		int iKeyNum, CATHierarchyBranch* ctrlBranch,
		Control** ctrlTime, float &fTimeVal, float &minTime, float &maxTime,
		Control** ctrlValue, float &fValueVal, float &minVal, float &maxVal,
		Control** ctrlTangent, float &fTangentVal,
		Control** ctrlInTanLen, float &fInTanLenVal,
		Control** ctrlOutTanLen, float &fOutTanLenVal,
		Control**	ctrlSlider) = 0;

	virtual float GetYval(TimeValue t, int LoopT) = 0;
	virtual float GetGraphYval(TimeValue t, int LoopT) { return GetYval(t, LoopT); };

	void PasteGraph(CATHierarchyBranch2* branchCopy);

	void FlipValues() { flipval *= -1.0f; }
	void SetFlipVal(float val) { flipval = val; }

	// TODO, most of the CATGraph GetValues don't need to exist.
	// someone needs to go through, and weed them all out
	void GetValue(TimeValue t, void * val, Interval&valid, GetSetMethod method);

	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	void CloneCATGraph(CATGraph* catgraph, RemapDir &remap);

	// Total number of keys.
	virtual int GetNumKeys();

	// Sets the number of keys allocated.
	// May add blank keys or delete existing keys
	virtual void SetNumKeys(int) {};

	// Fill in 'key' with the ith key
	virtual void GetKey(int i, IKey *key);

	// Set the ith key
	virtual void SetKey(int, IKey *) {};

	// Append a new key onto the end. Note that the
	// key list will ultimately be sorted by time. Returns
	// the key's index.
	virtual int AppendKey(IKey *key);

	// If any changes are made that would require the keys to be sorted
	// this method should be called.
	virtual void SortKeys() {};

	///////////////////////////////////////////////////////////////////////
	// from class Animatable
	virtual int NumKeys() { return GetNumKeys(); }

	virtual TimeValue GetKeyTime(int index);

	virtual int GetKeyIndex(TimeValue t);

	virtual BOOL GetNextKeyTime(TimeValue t, DWORD flags, TimeValue &nt);

	virtual void CopyKeysFromTime(TimeValue src, TimeValue dst, DWORD flags);

	virtual void DeleteKeyAtTime(TimeValue t);

	virtual BOOL IsKeyAtTime(TimeValue t, DWORD flags);

	virtual int GetKeyTimes(Tab<TimeValue> &times, Interval range, DWORD flags);

	virtual int GetKeySelState(BitArray &sel, Interval range, DWORD flags);
};
