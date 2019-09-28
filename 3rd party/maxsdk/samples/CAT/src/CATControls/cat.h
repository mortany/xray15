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

#include "Max.h"
#include "istdplug.h"
#include "iparamb2.h"

#include "version.h"
#include "CATMessages.h"
#include "math.h"
#include "notify.h"
#include "macrorec.h"

#include "../CATObjects/CatDotIni.h"

#include <dllutilities.h>

// TODO!  This disables the signed/unsigned mismatch warning
// REMOVE IT, but then fix up the rest of Max where people
// have screwed up PartID vs ChannelMask usage.
#pragma warning(disable : 4245)

extern bool g_bOutOfDateWarningDisplayed;
extern bool g_bOldFileWarningDisplayed;
extern bool g_bSavingCAT3Rig;
extern bool g_bLoadingCAT3Rig;

// A small, exception safe utility class to temporarily prevent CAT evaluations
// Pass in a parent if you want to trigger an Update on destruction.
// This class should never be created on the stack.
class CATParentTrans;
class CATEvaluationLock {
private:

	static int s_iStopEvaluating;
	CATParentTrans* mpLockingParent;

public:
	CATEvaluationLock(CATParentTrans* pParent = NULL);
	~CATEvaluationLock();

	static bool IsEvaluationLocked();
};

// A small class for wrapping the A_LOCK_TARGET flag
// on animatable.  Because we are never sure if the
// flag is being set by someone else already, we need to
// verify that the flag is not already set before setting it
// This class creates the equivalent functionality of a
// stack, you can simply add as may AutoDeleteLock's as you
// want.
class MaxAutoDeleteLock {
private:
	ReferenceTarget* mTarget;
	bool mWasLocked;
	bool mAutoDeleteOnRelease;

public:
	MaxAutoDeleteLock(ReferenceTarget* pTarget, bool bDoAutoDelOnRelease = true);
	~MaxAutoDeleteLock();
};

// A small, exception safe utility class to temporarily stop 3ds Max
// reference messages from propagating.
class MaxReferenceMsgLock {
public:
	MaxReferenceMsgLock();
	~MaxReferenceMsgLock();
};

//////////////////////////////////////////////////////////////////////////

extern HIMAGELIST hIcons; //image list of button icons

#define asRGB(col) (col).toRGB()

#define SAFE_DELETE(x) { if (x != NULL) { delete x; x = NULL; } }
#define SAFE_RELEASE_BTN(x) { if (x != NULL) { ReleaseICustButton(x); x = NULL; } }
#define SAFE_RELEASE_EDIT(x) { if (x != NULL) { ReleaseICustEdit(x); x = NULL; } }
#define SAFE_RELEASE_SPIN(x) { if (x != NULL) { ReleaseISpinner(x); x = NULL; } }
#define SAFE_RELEASE_SLIDER(x) { if (x != NULL) { ReleaseISlider(x); x = NULL; } }
#define SAFE_RELEASE_COLORSWATCH(x) { if (x != NULL) { ReleaseIColorSwatch(x); x = NULL; } }

// define a try/catch block that is only active in release mode
#ifdef NDEBUG
#define __TRY		try
#define __CATCH(x)	catch(x)
#else // in DEBUG, __TRY executes, and __CATCH never does
static int __CATsINeverChangeIntVar = 1;
#define __TRY		if(__CATsINeverChangeIntVar != 0)	// hInstance _never == NULL
#define __CATCH(x)	else if(assert1( __LINE__, __FILE__, __FUNCTION__, "ERROR: This expression should NOT BE EVALUATED IN DEBUG") && __CATsINeverChangeIntVar == 0)
#endif

// Enable this define to disable all authorization on all builds
#define CAT_NO_AUTH

//////////////////////////////////////////////////////////////////////
// These values tefine key time values for the CATMotion system.

#define STEPRATIO17		(17)		// 1/6 of a foottime
#define STEPRATIO25		(25)		// 1/4 of a foottime
#define STEPRATIO33		(33)		// 2/6 of a foottime
#define STEPRATIO50		(50)		// 1/2 of a foottime
#define STEPRATIO66		(66)		// 4/6 of a foottime
#define STEPRATIO75		(75)		// 3/4 of a foottime
#define STEPRATIO83		(83)		// 5/6 of a foottime
#define STEPRATIO87		(87)		// 5/6 of a foottime
#define STEPRATIO100	(100)		// 1 foottime, 1 footstep

#define BASE	192			// 1 frame at 25 fps

#define STEPTIME12		(BASE * 12)	// 1/8 of a foottime
#define STEPTIME17		(BASE * 17)	// 1/6 of a foottime
#define STEPTIME25		(BASE * 25)	// 1/4 of a foottime
#define STEPTIME33		(BASE * 33)	// 2/6 of a foottime
#define STEPTIME50		(BASE * 50)	// 1/2 of a foottime
#define STEPTIME66		(BASE * 66)	// 4/6 of a foottime
#define STEPTIME75		(BASE * 75)	// 3/4 of a foottime
#define STEPTIME83		(BASE * 83)	// 5/6 of a foottime
#define STEPTIME87		(BASE * 87)	// 7/8 of a foottime
#define STEPTIME100		(BASE * 100)	// 1 foottime, 1 footstep

//////////////////////////////////////////////////////////////////////
//

// The following few defines make our UIs a bit tidier
#define IS_CHECKED(hWnd, hBtn)			(SendMessage(GetDlgItem(hWnd, hBtn), BM_GETCHECK, 0, 0) == BST_CHECKED)
#define SET_CHECKED(hWnd, hBtn, on)		(SendMessage(GetDlgItem(hWnd, hBtn), BM_SETCHECK, (WPARAM)(on ? BST_CHECKED : BST_UNCHECKED), 0))
#define SET_TEXT(hWnd, item, txt)		(SendMessage(GetDlgItem(hWnd, item), WM_SETTEXT, 0, (LPARAM)txt))

// These are special notify messages for layers.  They
// are specifically for the function CATMessage().
enum {
	CAT_CATUNITS_CHANGED,
	CAT_CATMODE_CHANGED,
	CAT_COLOUR_CHANGED,

	CAT_SHOWHIDE,
	// when we recieve this message we will
	// update 3 things Show/Hide, visibility, and Renderability
	CAT_VISIBILITY,
	CAT_NAMECHANGE,
	CAT_TRANSCHANGE,
	CAT_UPDATE,
	CAT_KEYFREEFORM,
	CAT_KEYLIMBBONES,
	CAT_UPDATE_NODEPROPERTIES,
	CAT_SET_LENGTH_AXIS,
	CAT_REFRESH_OBJECTS_LENGTHAXIS,
	CAT_LOCK_LIMB_FK,
	CAT_UPDATE_USER_PROPS,

	CAT_INVALIDATE_TM,
	CAT_PRE_RIG_SAVE,
	CAT_POST_RIG_SAVE,
	CAT_POST_RIG_LOAD,

	CAT_EVALUATE_BONES,
	CAT_TDM_CHANGED,

	CLIP_LAYER_APPEND,
	CLIP_LAYER_INSERT,
	CLIP_LAYER_REMOVE,
	CLIP_LAYER_SELECT,
	CLIP_LAYER_CLONE,
	CLIP_LAYER_MOVEUP,
	CLIP_LAYER_MOVEDOWN,
	CLIP_WEIGHTS_CHANGED,
	CLIP_BRANCH_ADDED,
	CLIP_BRANCH_REMOVED,
	CLIP_WEIGHTVIEW_RENUMBER,
	// needed for the undo system
	CLIP_LAYER_SOLOED,
	CLIP_LAYER_PASTED,
	CLIP_LAYER_DUMP_SETUP_POSE_TO_LAYER,
	CLIP_LAYER_SET_INHERITANCE_FLAGS,
	CLIP_LAYER_CHECK_NUM_LAYERS,
	CLIP_LAYER_SETPOS_CLASS,
	CLIP_LAYER_SETROT_CLASS,
	CLIP_LAYER_SETSCL_CLASS,
	CLIP_LAYER_CALL_PLACB,
	CLIP_ARB_LAYER_DELETED		// This message is called to notify a charater when an CLIP_FLAG_ARB_CONTROLLER has been deleted (because no-one owns a reference to it).
};

#define PASTELAYERFLAG_INSTANCE				(1<<0)

#define PASTERIGFLAG_MIRROR					(1<<0)
#define PASTERIGFLAG_DONT_PASTE_CHILDREN	(1<<1)
#define PASTERIGFLAG_DONT_PASTE_ROOTPOS		(1<<2)
#define PASTERIGFLAG_DONT_PASTE_FLAGS		(1<<3)
#define PASTERIGFLAG_DONT_PASTE_CONTROLLER	(1<<4)
#define PASTERIGFLAG_DONT_CHECK_FOR_LOOP	(1<<5)
#define PASTERIGFLAG_DONT_PASTE_MESHES		(1<<6)
#define PASTERIGFLAG_FIRST_PASTED_BONE		(1<<8)

// Find a class of type type that references the target.
template <class T> extern
T* FindReferencingClass(ReferenceTarget* pTarget, int maxDepth = -1);

// 3ds Max's version of create Instance can mess up our class pointers.
// Because it returns a void*, there is no way to properly cast our pointers.
// To fix this, we replace the 3ds Max version with one that
// returns a pointer we can cast from.  NOTE - this will also force a compiler error
// on Interface::CreateInstance, which is fine because we never want to use Max's version.
inline ReferenceTarget* DoCreateInstance(SClass_ID superClass, Class_ID classID) { return static_cast<ReferenceTarget*>(CreateInstance(superClass, classID)); }
#ifdef CreateInstance
#undef CreateInstance
#endif //CreateInstance
#define CreateInstance DoCreateInstance

class MaskedBitmap {
private:
	int nWidth, nHeight;
	HBITMAP hbmImage;
	HBITMAP hbmMask;

public:
	MaskedBitmap(HDC hdc, int width, int height, const BYTE *lpBitmap);
	~MaskedBitmap();
	void MaskBlit(HDC hdc, int x, int y, HBRUSH hBrush);
	const int Width() const { return nWidth; }
	const int Height() const { return nHeight; }
};

extern BOOL CopyMeshFromNode(TimeValue t, INode *oldnode, INode* newnode, Mesh &catmesh, Point3 dim);

class LayerCloneCallback {
public:
	LPSTR name;				// the identifier's name
	Animatable		*orrigal_anim;
	IParamBlock2	*pblock;	// expected type to follow this identifier
	int id, tabindex;						// pblcokid
	Animatable		**new_anim;

	LayerCloneCallback(IParamBlock2	*copiedpblock, Animatable	*anim, int id, int tabindex = 0) {
		orrigal_anim = anim;
		pblock = copiedpblock;
		this->id = id;
		this->tabindex = tabindex;
		new_anim = NULL;
	}
};

IParamBlock2*	CloneParamBlock(IParamBlock2 *pblock, RemapDir& remap);
int GetFileModifedTime(const TSTR& file, TSTR &out_time);

extern void MakeFace(Face &face, int a, int b, int c, int ab, int bc, int ca, DWORD sg, MtlID id);

inline void ModVec(Point3& v, int lengthaxis) { if (lengthaxis == 0) { float tmp = v.x; v.x = v.z; v.z = tmp; } };
inline Point3 FloatToVec(float val, int lengthaxis) { if (lengthaxis == 0) return Point3(val, 0.0f, 0.0f);		return Point3(0.0f, 0.0f, val); };

extern void InitCATIcons();
extern void ReleaseCATIcons();
