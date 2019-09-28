// Legs weight on character movement

#pragma once

#include "CATGraph.h"
#include <CATAPI/CATClassID.h>

class LegWeight : public CATGraph {
public:

	enum { PBLOCK_REF };
	//TODO: Add enums for various parameters
	enum {
		PB_LIMBDATA,
		PB_CATBRANCH,

		PB_LIFTSTARTTIME,
		PB_LIFTENDTIME,
		PB_PLANTSTARTTIME,
		PB_PLANTENDTIME,
	};

	//From Animatable
	Class_ID ClassID() { return LEGWEIGHT_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_LEGWEIGHT); }

	RefTargetHandle Clone(RemapDir &remap);

	void DeleteThis() { delete this; }
	//Constructor/Destructor

	LegWeight();
	~LegWeight();

	//
	// from class Control:
	//
	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);
	void Copy(Control *from);

	//
	// from class CATGraph:
	//
	int GetNumGraphKeys() { return 4; };
	void GetCATKey(const int i,
		const TimeValue	t,
		CATKey &key,
		bool &isInTan, bool &isOutTan) const;

	void GetGraphKey(
		int iKeyNum, CATHierarchyBranch* ctrlBranch,
		Control** ctrlTime, float &fTimeVal, float &fPrevKeyTime, float &fNextKeyTime,
		Control** ctrlValue, float &fValueVal, float &minVal, float &maxVal,
		Control** ctrlTangent, float &fTangentVal,
		Control** ctrlInTanLen, float &fInTanLenVal,
		Control** ctrlOutTanLen, float &fOutTanLenVal,
		Control** ctrlSlider);

	COLORREF GetGraphColour();
	TSTR GetGraphName();

	float GetGraphYval(TimeValue t, int LoopT);
	float GetYval(TimeValue t, int LoopT);

};

class LegWeightClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE) { UNREFERENCED_PARAMETER(loading); return new LegWeight(); }
	const TCHAR *	ClassName() { return GetString(IDS_CL_LEGWEIGHT); }
	SClass_ID		SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	Class_ID		ClassID() { return LEGWEIGHT_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("LegWeight"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle

};

extern ClassDesc2* GetLegWeightDesc();
