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
	CATClipHierarchy is a repository for controllers that
	affect motion characteristics.
 **********************************************************************/

#pragma once

#include "ICATClipHierarchy.h"

 //GB 24-Jun-03: These strange #defines are for testing
 //snippets that I've sneakily added in....
 //
#define MUTANT_HOMICIDAL_CHICKEN	// Adds Clip Hierarchy to CAT (gulp!)

//#define DISPLAY_LAYER_ROLLOUTS_TOGGLE

//GB 24-Jun-03 end

//
// The following are used in the 'flags' member of CATClipThing and
// CATClipValue.  The low-order 8 bits are reserved for special use
// by specific branches or values.  The hi-order 8 bits have a set
// of flags common to all values or all branches.
//

// These are flags common to both branches and values

// The following are persistent flags, and will be set on creation.
// The value of these flags cannot be modified, as they are loaded/saved.

// The following are only set only on the CATClipRoot
#define CLIP_FLAG_SHOW_CONTRIBUTING_LAYERS				(1<<4)
#define CLIP_FLAG_SHOW_ALL_LAYERS						(1<<5)
#define CLIP_FLAG_SHOW_LAYER_ROLLOUTS					(1<<6)

// Define behaviour of CATClipValue instances
#define CLIP_FLAG_ARB_CONTROLLER			(1<<8)	// This an arb controller, created via MaxScript CreateLayerFloat function
#define CLIP_FLAG_HAS_TRANSFORM				(1<<9)	// Gets multiplied by the layer transform
#define CLIP_FLAG_ANGLE_DIM					(1<<15) // Holds layers that are angles

// Set on CATClipMatrix3 to define SetupMode behaviour.
#define CLIP_FLAG_SETUP_INHERITS_POS_FROM_CATPARENT		(1<<16)	// Inherit position from the CATParent (ex - pelvis and footplatforms)
#define CLIP_FLAG_SETUP_INHERITS_ROT_FROM_CATPARENT		(1<<17)	// Inherit rotation from the CATParent (ex - pelvis and footplatforms)
#define CLIP_FLAG_SETUP_FLIPROT_FROM_CATPARENT			(1<<18)
#define CLIP_FLAG_SETUP_INHERITS_POS_FROM_PARENT		(1<<19)

// Just like Max's inheritance flags.  Set on CATClipMatrix3.
#define CLIP_FLAG_INHERIT_POS							(1<<20)
#define CLIP_FLAG_INHERIT_ROT							(1<<21)
#define CLIP_FLAG_INHERIT_SCL							(1<<22)

// ST - 2011, I believe the below are obsolete, replaced by CATControl flags
#define CLIP_FLAG_LOCK_POS								(1<<23)
#define CLIP_FLAG_LOCK_ROT								(1<<24)
#define CLIP_FLAG_LOCK_SCL								(1<<25)

#define CLIP_FLAG_RELATIVE_TO_SETUPPOSE					(1<<27)	// Apply animation data relative to setup pose.
#define CLIP_FLAG_SETUPPOSE_CONTROLLER					(1<<28)	// Has animated controller for Setup val (eg - spring), rather than static value.

// These are branch flags

// The following flags represent temporary states.  These
// are not saved/loaded, and can be messed with as desired.
// TODO: Do a post-load to consolidate persistent flags
// and give us a few free indices to play with
#define CLIP_FLAG_COLLAPSINGLAYERS			(1<<0)	// Set only on CATClipRoot
#define CLIP_FLAG_DONT_INVALIDATE_CACHE		(1<<29)	// Not sure if this is used?
#define CLIP_FLAG_KEYFREEFORM				(1<<30)	// Set on CATClipValue to prevent it from reading the selected layer
#define CLIP_FLAG_DISABLE_LAYERS			(1<<31)	// Set on CATClipValue to prevent evaluation

// These are flags for what gets notified by CATMessages().
#define CLIP_NOTIFY_VALUES				(1<<0)
#define CLIP_NOTIFY_WEIGHTS				(1<<1)
#define CLIP_NOTIFY_BRANCHES			(1<<2)
#define CLIP_NOTIFY_RANGES				(1<<3)
#define CLIP_NOTIFY_TRANSFORMS			(1<<4)
#define CLIP_NOTIFY_WEIGHTVIEW			(1<<5)
#define CLIP_NOTIFY_LAYERUNITS_CHANGE	(1<<6)
#define CLIP_NOTIFY_ALL					(0x7f)

/*
 * These are special value list controls that store important
 * layer information.  Namely the weights and the transforms.
 * The weights list is stored at every branch level.  Weight
 * values are cached for the most recent time instant at which
 * they were evaluated.  The transforms list is stored at the
 * root only.  It is responsible for creating, managing and
 * deleting ghosts.
 */
#define CATCLIPWEIGHTS_CLASS_ID			Class_ID(0x72ed43dc, 0x3ec96a0a)
extern ClassDesc2* GetCATClipWeightsDesc();

/*
 * These are the different types of value list controls we
 * can have in the hierarchy.  Currently each type has only
 * a single associated controller (e.g. rotation lists use
 * Euler_XYZ controllers), but this will change in future,
 * probably by setting the flags to say what type to use.
 */
#define CATCLIPFLOAT_CLASS_ID			Class_ID(0x40585238, 0x2503240d)
#define CATCLIPROTATION_CLASS_ID		Class_ID(0x13246cd2, 0x7a1b1c73)
#define CATCLIPPOSITION_CLASS_ID		Class_ID(0x7fc51d47, 0x50ef7d9f)
#define CATCLIPPOINT3_CLASS_ID			Class_ID(0x20bc3406, 0x34dc5a2a)
#define CATCLIPSCALE_CLASS_ID			Class_ID(0x1fec31e8, 0x6c6a7532)
#define CATCLIPMATRIX3_CLASS_ID			Class_ID(0x18d026a1, 0x347d1cf3)

extern ClassDesc2* GetCATClipFloatDesc();
extern ClassDesc2* GetCATClipRotationDesc();
extern ClassDesc2* GetCATClipPositionDesc();
extern ClassDesc2* GetCATClipPoint3Desc();
extern ClassDesc2* GetCATClipScaleDesc();
extern ClassDesc2* GetCATClipMatrix3Desc();

/*
 * This section defines the branches that can appear in the
 * CAT Clip Hierarchy.  We define a base class called Thing
 * and then derive all branch controllers from it.
 *
 * All this stuff is implemented in CATClipBranch.cpp
 */

 // GB 12-Jun: We no longer store references to presets (leaves) inside
 // a hierarchy branch.  This is to stop messages going haywire in the
 // system.  It does mean that whoever creates a new preset branch MUST
 // reference it straight away, otherwise it will be lost during saving
 // and loading and we'll end up with invalid handles.
 // GB 07-Jul: Whoah there horsie...  Let's just wait and see whether
 // that's entirely necessary.  It sounds really dangerous.  That's
 // probably why I never actually implemented the above changes.
 //#define NO_LEAF_REFS

 // These are hierarchy branch types
#define CLIP_BRANCH_UNKNOWN					((USHORT)-1)
#define CLIP_BRANCH_ROOT					0
#define CLIP_BRANCH_BRANCH					1
#define CLIP_BRANCH_PELVIS					2			// Tells us the type of the hub group
#define CLIP_BRANCH_RIBCAGE					3			// Tells us the type of the hub group
#define CLIP_BRANCH_LEG						4
#define CLIP_BRANCH_ARM						5
#define CLIP_BRANCH_HEAD					6
//#define CLIP_BRANCH_LOWERBODY				7
//#define CLIP_BRANCH_UPPERBODY				8
#define CLIP_BRANCH_TAIL					9
#define CLIP_BRANCH_PALM					10
#define CLIP_BRANCH_HUB						11
#define CLIP_BRANCH_HUBGROUP				12
