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

//////////////////////////////////////////////////////////////////////////
// CATMotion
#include "CATMotionLayer.h"

#include "Ease.h"
#include "LiftPlantMod.h"
#include "WeightShift.h"
#include "LiftOffset.h"
#include "MonoGraph.h"
#include "LegWeight.h"
#include "FootBend.h"
#include "StepShape.h"
#include "FootLift.h"
#include "KneeAngle.h"
#include "PivotPosData.h"

#include "CATMotionRot.h"
#include "CATMotionHub2.h"
#include "CATMotionLimb.h"
#include "CATMotionPlatform.h"
#include "CATMotionTail.h"
#include "CATMotionTailRot.h"
#include "CATMotionDigitRot.h"
#include "LiftOffset.h"
#include "CATp3.h"

#include "CATHierarchyRoot.h"
#include "CATHierarchyBranch2.h"
#include "CATHierarchyLeaf.h"

#include "PivotRot.h"

//////////////////////////////////////////////////////////////////////////
// Layers
#include "CATClipRoot.h"
#include "CATClipHierarchy.h"
#include "NLAInfo.h"
#include "LayerTransform.h"
#include "CATTransformOffset.h"

#include "ProxyTransform.h"
#include "CATUnitsPosition.h"
#include "RootNodeController.h"

//////////////////////////////////////////////////////////////////////////
// Rig Structure
#include "BoneSegTrans.h"
#include "BoneData.h"
#include "LimbData2.h"
#include "FootTrans2.h"
#include "CollarboneTrans.h"
#include "SpineData2.h"
#include "SpineTrans2.h"
#include "Hub.h"

#include "DigitData.h"
#include "DigitSegTrans.h"
#include "PalmTrans2.h"

#include "TailData2.h"
#include "TailTrans.h"

#include "IKTargController.h"
#include "ArbBoneTrans.h"

//////////////////////////////////////////////////////////////////////////
// Misc
#include "CATWeight.h"
#include "CATParentTrans.h"
#include "HDPivotTrans.h"

///////////////////////////
// Objects
#include "../CATObjects/CATParent.h"
#include "../CATObjects/Bone.h"
#include "../CATObjects/IKTargetObject.h"
#include "../CATObjects/HubObject.h"
