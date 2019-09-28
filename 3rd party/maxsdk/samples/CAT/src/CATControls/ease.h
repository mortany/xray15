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

// Ease curve for character

#pragma once

#include <vector>
#include <Util/StaticAssert.h>

#define EASE_CLASS_ID	Class_ID(0x453f7278, 0x4b8e1dfd)

class stepkey {
	// Warning - instances of this class are saved as a binary blob to scene file
	// Adding/removing members will break file i/o
public:
	// Key value
	int time;	// time value
	float val;	// key value

	// Vect from key
	float xVect;	// x vect to nxt key
	float yVect;	// x vect to nxt key

	stepkey() { time = 0; val = xVect = yVect = 0.0f; };
	stepkey(TimeValue t, float v);
	void SetVect(TimeValue nxtTime, float nxtVal);
};

// compile-time validates size of stepkey class to ensure its size doesn't change since instances are
// saved as a binary blob to scene file. Changes in size will break file i/o
class stepkeySizeValidator {
	stepkeySizeValidator() {}  // not creatable
	MaxSDK::Util::StaticAssert< (sizeof(stepkey) == sizeof(int) + 3 * sizeof(float)) > validator;
};

inline stepkey::stepkey(TimeValue t, float v)
{
	time = t;
	val = v;
	xVect = yVect = 0;
}
inline void stepkey::SetVect(TimeValue nxtTime, float nxtVal)
{
	xVect = (float)(nxtTime - time);
	yVect = (float)(nxtVal - val);
}
// LinInterp controller, no CTRL_RELATIVE
class Ease : public Control
{
private:
	int numKeys;

	int avgKeyTime;
	float avgKeyValue;

	std::vector<stepkey> keys;
public:

	Ease();
	~Ease();

	Class_ID ClassID() { return EASE_CLASS_ID; };
	SClass_ID SuperClassID() { return CTRL_FLOAT_CLASS_ID; };
	void GetClassName(TSTR& s) { s = _T("Ease"); }
	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget,
		PartID& partID, RefMessage message, BOOL propagate);
	RefTargetHandle Clone(RemapDir &remap);

	void DeleteThis() { delete this; }
	void Copy(Control *from) { UNREFERENCED_PARAMETER(from); }
	void CommitValue(TimeValue) {}
	void RestoreValue(TimeValue) {}

	BOOL CanMakeUnique() { return FALSE; }

	void SetValue(TimeValue t, void *val, int commit = 1, GetSetMethod method = CTRL_ABSOLUTE);
	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method = CTRL_ABSOLUTE);

	// Our GetValue dont do anything (cept in DEBUG), these functions here
	// are the real deal, replace StepEase (GetValue) and flipped (GetTime),
	// solves the problem of not being able to get a time for a value. Relies on
	// the permanent  +ve gradient of graph.
	virtual float GetValue(TimeValue t);
	virtual TimeValue GetTime(float Value);

	int IsKeyable() { return FALSE; }

	BOOL BypassTreeView() { return FALSE; } // In release, you cant see this
	BOOL BypassTrackBar() { return TRUE; }	// You can never see this...

	// new functions
	virtual int NumKeys();
	virtual int GetKeyTime(int index);
	virtual int AppendKey(stepkey* k);
	virtual void SetNumKeys(int i);
	virtual int GetKeyTimes(Tab<TimeValue> &times, Interval range, DWORD flags);
	virtual stepkey *GetKey(int index);
	virtual void SetVect(int index, float xVal, float yVal);

	// Miscellaneous subanim stuff that we don't care about
	// This makes us Red
	virtual Interval GetExtents();
	Interval GetTimeRange(DWORD flags) { UNREFERENCED_PARAMETER(flags); return GetCOREInterface()->GetAnimRange(); };
	DWORD GetSubAnimCurveColor(int) { return PAINTCURVE_XCOLOR; };

	// Save/Load all information
	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	void NotifyChange() { NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE); }
};

class EaseClassDesc : public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }							// Show this in create branch?
	void *			Create(BOOL loading = FALSE) { UNREFERENCED_PARAMETER(loading); return new Ease(); }
	const TCHAR *	ClassName() { return _T("Ease"); }
	SClass_ID		SuperClassID() { return CTRL_FLOAT_CLASS_ID; }			// This determins the type of our controller
	Class_ID		ClassID() { return EASE_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("Ease"); }					// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }						// returns owning module handle
};

extern ClassDesc2* GetEaseDesc();
