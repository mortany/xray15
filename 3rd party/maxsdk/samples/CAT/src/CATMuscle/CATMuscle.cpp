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

#include "CATMuscle.h"

// Max Stuff
#include "notify.h"
#include "MaxIcon.h"

Muscle*		muscleCopy = NULL;
BOOL		g_bIsResetting = FALSE;
BOOL		g_bKeepRollouts = FALSE;

INode* FindParent(INode* node)
{
	BOOL cond = TRUE;
	while (cond)
	{
		if (node->GetParentNode()->IsRootNode()) {
			return NULL;
		}
		if (node->GetParentNode()->GetTMController()->GetInterface(I_MASTER) != node->GetTMController()->GetInterface(I_MASTER)) {
			return node->GetParentNode();
		}
		node = node->GetParentNode();
	}
	return NULL;
};

// returns true if the node was selected
BOOL DeSelectAndDelete(INode* node)
{
	if (node == NULL)
		return FALSE;

	BOOL res = FALSE;
	if (node->Selected()) {
		GetCOREInterface()->DeSelectNode(node);
		res = TRUE;
	}
	node->Delete(GetCOREInterface()->GetTime(), FALSE);
	return res;
}

