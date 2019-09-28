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

#include "cat.h"
#include "CATCharacterRemap.h"
#include "CATClipValue.h"

bool CATCharacterRemap::BuildMappingForSubAnims(ReferenceMaker* srcref, ReferenceMaker* tgtref)
{
	if (srcref == NULL || tgtref == NULL)
		return false;

	// Do not propagate out to node classes, if a node wants
	// to be mapped, it will be mapped by the BuildMapping function
	if (srcref->SuperClassID() == BASENODE_CLASS_ID)
		return false;

	// Do not map CATClipValue subanims.  Layers
	// are never remapped.
	if (dynamic_cast<CATClipValue*>(srcref) != NULL)
		return false;

	int numrefs = min(srcref->NumRefs(), tgtref->NumRefs());
	for (int i = 0; i < numrefs; i++)
	{
		ReferenceMaker* srcrefmaker = srcref->GetReference(i);
		ReferenceMaker* tgtrefmaker = tgtref->GetReference(i);
		if (srcrefmaker && tgtrefmaker && tgtrefmaker->ClassID() == srcrefmaker->ClassID() && srcrefmaker->IsRefMaker())
		{

			// Check to see if this controller has already been added to the list.
			if (FindMapping((RefTargetHandle)srcrefmaker)) continue;

#ifdef CAT_DEBUG
			// This is just for debugging to look at the ClassName
			TSTR srcname, tgtname;
			srcrefmaker->GetClassName(srcname);
			tgtrefmaker->GetClassName(tgtname);
#endif

			AddEntry((RefTargetHandle)srcrefmaker, (RefTargetHandle)tgtrefmaker);
			BuildMappingForSubAnims(srcrefmaker, tgtrefmaker);
		}
	}
	return true;
}

// Find the mapped parent of pSrcNode
// Returns: True if the legal parent was found, else false
bool GetLegalParent(INode* pSrcNode, INode*& pOutDstParent, CATCharacterRemap* remap)
{
	pOutDstParent = NULL;
	INode* pSrcParent = pSrcNode->GetParentNode();
	if (!pSrcParent->IsRootNode())
	{
		pOutDstParent = static_cast<INode*>(remap->FindMapping(pSrcParent));
		return pOutDstParent != NULL;
	}
	return true; // NULL is legal for root node mapping.
}

// Object ClassID the same
bool MatchesClassID(const Class_ID& srcClass, INode* pDstNode)
{
	return (srcClass == pDstNode->GetObjectRef()->ClassID()) != FALSE;
}

// Parents are the same
bool MatchesParents(INode* pLegalDstParent, INode* pDstNode)
{
	if (pLegalDstParent != NULL)
		return pDstNode->GetParentNode() == pLegalDstParent;

	return (pDstNode->GetParentNode()->IsRootNode() != FALSE);
}

bool MatchesName(const MSTR& srcName, INode* pDstNode)
{
	return _tcscmp(srcName, pDstNode->GetName()) == 0;
}

bool IsEquivalent(const Matrix3& tm1, const Matrix3& tm2)
{
	const Point3* tm1Addr = reinterpret_cast<const Point3*>(tm1.GetAddr());
	const Point3* tm2Addr = reinterpret_cast<const Point3*>(tm2.GetAddr());
	for (int i = 0; i < 3; i++)
	{
		// Essentially, this does a projection, and normalizes it.  If its more than 0.01 (1% difference
		float diff = DotProd(tm1Addr[i], tm2Addr[i]) / tm1Addr[i].LengthSquared();
		if (diff > 1.01 || diff < 0.99)
			return false;
	}
	for (int i = 0; i < 3; i++)
	{
		// Measure position difference by straight comparisons.
		float diff = (tm1Addr[3][i] - tm2Addr[3][i]) / tm1Addr[3][i];
		if (diff > 0.00001 || diff < -0.00001)
			return false;
	}
	return true;
}

bool MatchesLocalTransform(const Matrix3& tmSrcLocal, INode* pDstNode)
{
	// We compare the entire matrix on this pass rather than just position,
	// because if the name has been changed we cannot afford as
	// much lee-way on the transform.  There might be multiple
	// entities at the same position, we need to be careful
	Interval ivDontCare;
	Control* ctrlDest = pDstNode->GetTMController();
	Matrix3 tmDestLocal(1);
	ctrlDest->GetValue(0, &tmDestLocal, ivDontCare, CTRL_RELATIVE);

	return IsEquivalent(tmSrcLocal, tmDestLocal);
}

//  Matches the standard expectation - nothing has changed.
bool MatchesExactly(const Class_ID& srcClass, const MSTR& srcName, INode* pLegalDstParent, const Matrix3& tmSrcLocal, INode* pDstNode)
{
	// Sanity check
	if (pDstNode == NULL)
		return false;

	// First check - we should only match similar objects.
	if (!MatchesClassID(srcClass, pDstNode))
		return false;

	// 2nd check - are parents the same?
	if (!MatchesParents(pLegalDstParent, pDstNode))
		return false;

	// Matches Names
	if (!MatchesName(srcName, pDstNode))
		return false;

	if (!MatchesLocalTransform(tmSrcLocal, pDstNode))
		return false;

	// Success!
	return true;
}

// Slightly less common - This is designed to match items which have changed their object identity (eg, added modifiers).
bool MatchesByTransformParentAndName(const Matrix3& tmSrcLocal, INode* pLegalDstParent, const MSTR& srcName, INode* pDstNode)
{
	if (pDstNode == NULL)
		return false;

	// Easiest check - are parents the same?
	if (!MatchesParents(pLegalDstParent, pDstNode))
		return false;

	// Next fastest - Name the same
	if (!MatchesName(srcName, pDstNode))
		return false;

	// Final check, is the transform the same?
	if (!MatchesLocalTransform(tmSrcLocal, pDstNode))
		return false;

	return true;
}

// Slightly less common again, this matches items that have changed their name only.
bool MatchesByClassIDParentAndTransform(const Class_ID& srcClass, const Matrix3& tmSrcLocal, INode* pLegalDstParent, INode* pDstNode)
{
	if (pDstNode == NULL)
		return false;

	// First check - we should only match similar objects.
	if (!MatchesClassID(srcClass, pDstNode))
		return false;

	// Easiest check - are parents the same?
	if (!MatchesParents(pLegalDstParent, pDstNode))
		return false;

	// Final check, is the transform the same?
	if (!MatchesLocalTransform(tmSrcLocal, pDstNode))
		return false;

	return true;
}

// the only safe bets remaining are similarly named children.  This implies
// transform and object have changed.
bool MatchesByNameAndParent(const MSTR& srcName, INode* pLegalDstParent, INode* pDstNode)
{
	if (pDstNode == NULL)
		return false;

	// Easiest check - are parents the same?
	if (!MatchesParents(pLegalDstParent, pDstNode))
		return false;

	// Next fastest - Name the same
	if (!MatchesName(srcName, pDstNode))
		return false;

	return true;
}

// Finally - the only safe bets remaining are similarly named children.  This implies
// transform object, and parent have changed.  This is probably a little lax...
bool MatchesByName(const MSTR& srcName, INode* pDstNode)
{
	if (pDstNode == NULL)
		return false;

	// Next fastest - Name the same
	if (!MatchesName(srcName, pDstNode))
		return false;

	return true;
}

void CATCharacterRemap::SetMappingBetweenNodes(Tab<INode*>& srcExtraNodes, int idxSource, Tab<INode*>& dstExtraNodes, int idxDest)
{
	INode* pSrcNode = srcExtraNodes[idxSource];
	INode* pDstNode = dstExtraNodes[idxDest];

	DbgAssert(pSrcNode != NULL);
	DbgAssert(pDstNode != NULL);

	// NOTE: It is possible that these nodes are already mapped.
	// For example, if an earlier node is constrained to a
	// later node, it will trigger the remapping and our node
	// will already be mapped. This is not an error.
	DbgAssert(FindMapping(pSrcNode) == NULL || FindMapping(pSrcNode) == pDstNode);

	if (pSrcNode == NULL || pDstNode == NULL)
		return;

	AddEntry(pSrcNode, pDstNode);
	AddEntry(pSrcNode->GetObjectRef(), pDstNode->GetObjectRef());
	AddEntry(pSrcNode->GetTMController(), pDstNode->GetTMController());

	BuildMappingForSubAnims(pSrcNode->GetTMController(), pDstNode->GetTMController());
	srcExtraNodes[idxSource] = NULL;
	dstExtraNodes[idxDest] = NULL;
}

bool CATCharacterRemap::BuildMapping(Tab<INode*>& srcNodes, Tab<INode*> dstNodes)
{
	Interval ivDontCare;

	// Match up extra nodes.  This is 4 part process, where
	// we first attempt to match by the most rigorous method,
	// then in decreasing safety. We do this so we get the
	// most certain matches, then add in the riskier matches after.

	// First, we attempt to match by node name, obj ClassID, and parent.
	//  This is the default, nothing has really changed (except transform) method.
	//  Any node that has changed it's class ID, name, or parent will be skipped here.
	//  This risk's matching multiple children with the same name incorrectly, but is acceptable.
	// Then we attempt to match by name, parent and local transform.
	//  This handles matching objects where the object only has changed (eg modifier added)
	// 3rd, we attempt to match by parent, ClassID and local position.
	//  This handles where the name of the entity has been changed only.  It risks
	//	matching similar objects in the same place
	// Finally, we match by name.
	//	This handles structure changes, object changes, etc.
	//  It risks matching similar named entities.
	// If the name, position, and type of the object have all changed, we are pooched!

	// First pass, match by Object and name.  This should catch 90% of cases.
	// We need to loop this pass so if one ERN is parented to another ERN node,
	// we don't fail to match because the parent is matched after the child.
	bool bHasMatchedSomething = false;
	do
	{
		bHasMatchedSomething = false;
		for (int i = 0; i < srcNodes.Count(); i++)
		{
			INode *pSrcNode = srcNodes[i];
			if (pSrcNode == NULL)
				continue;

			// Find the parent
			INode* pLegalDstParent = NULL;
			if (!GetLegalParent(pSrcNode, pLegalDstParent, this))
				continue;

			// Try to match by src name
			MSTR srcName = pSrcNode->GetName();

			// Class ID
			Class_ID srcClass = pSrcNode->GetObjectRef()->ClassID();

			// Match by local position
			Interval ivDontCare = FOREVER;
			Control* ctrlSrc = pSrcNode->GetTMController();
			Matrix3 tmSrcLocal(1);
			ctrlSrc->GetValue(0, &tmSrcLocal, ivDontCare, CTRL_RELATIVE);

			// Search through all available nodes to find a match
			for (int j = 0; j < dstNodes.Count(); j++)
			{
				if (MatchesExactly(srcClass, srcName, pLegalDstParent, tmSrcLocal, dstNodes[j]))
				{
					// Success - mark this node for matching
					SetMappingBetweenNodes(srcNodes, i, dstNodes, j);
					bHasMatchedSomething = true;
					break;
				}
			}
		}
	} while (bHasMatchedSomething);

	// Next, match by name, transform and parent.  This handles cases
	// where the object has been changed, but the name/position
	// was left the same - for example a modifier was added.
	do
	{
		bHasMatchedSomething = false;
		for (int i = 0; i < srcNodes.Count(); i++)
		{
			INode *pSrcNode = srcNodes[i];
			if (pSrcNode == NULL)
				continue;

			// Cache the data we will be testing against.

			// Find the parent
			INode* pLegalDstParent = NULL;
			if (!GetLegalParent(pSrcNode, pLegalDstParent, this))
				continue;

			// Try to match by src name
			MSTR srcName = pSrcNode->GetName();

			// Match by local position
			Interval ivDontCare = FOREVER;
			Control* ctrlSrc = pSrcNode->GetTMController();
			Matrix3 tmSrcLocal(1);
			ctrlSrc->GetValue(0, &tmSrcLocal, ivDontCare, CTRL_RELATIVE);

			// Search all dst nodes for a similar match.
			for (int j = 0; j < dstNodes.Count(); j++)
			{
				if (MatchesByTransformParentAndName(tmSrcLocal, pLegalDstParent, srcName, dstNodes[j]))
				{
					// Success - mark this node for matching
					SetMappingBetweenNodes(srcNodes, i, dstNodes, j);
					bHasMatchedSomething = true;
					break;
				}
			}
		}
	} while (bHasMatchedSomething);

	// Next, match by ClassID, parent node, and local transform .
	// This handles the case where the name was changed.
	do
	{
		bHasMatchedSomething = false;
		for (int i = 0; i < srcNodes.Count(); i++)
		{
			INode *pSrcNode = srcNodes[i];
			if (pSrcNode == NULL)
				continue;

			// Find the parent
			INode* pLegalDstParent = NULL;
			if (!GetLegalParent(pSrcNode, pLegalDstParent, this))
				continue;

			// Try to match by class ID
			Class_ID srcClass = pSrcNode->GetObjectRef()->ClassID();
			// Match by local position
			Interval ivDontCare = FOREVER;
			Control* ctrlSrc = pSrcNode->GetTMController();
			Matrix3 tmSrcLocal(1);
			ctrlSrc->GetValue(0, &tmSrcLocal, ivDontCare, CTRL_RELATIVE);

			// Search through all the nodes, if the extra node parent
			// is the same, the object is the same, and our local transform is the same, it's a match.
			for (int j = 0; j < dstNodes.Count(); j++)
			{
				if (MatchesByClassIDParentAndTransform(srcClass, tmSrcLocal, pLegalDstParent, dstNodes[j]))
				{
					// Success - mark this node for matching
					SetMappingBetweenNodes(srcNodes, i, dstNodes, j);
					bHasMatchedSomething = true;
					break;
				}
			}
		}
	} while (bHasMatchedSomething);

	// Next, match by name and parent.  This allows
	// children to change their transform and class ID
	do
	{
		bHasMatchedSomething = false;
		for (int i = 0; i < srcNodes.Count(); i++)
		{
			INode *pSrcNode = srcNodes[i];
			if (pSrcNode == NULL)
				continue;

			// Find the parent
			INode* pLegalDstParent = NULL;
			if (!GetLegalParent(pSrcNode, pLegalDstParent, this))
				continue;

			// Try to match by src name
			MSTR srcName = pSrcNode->GetName();

			for (int j = 0; j < dstNodes.Count(); j++)
			{
				if (MatchesByNameAndParent(srcName, pLegalDstParent, dstNodes[j]))
				{
					// Success - mark this node for matching
					SetMappingBetweenNodes(srcNodes, i, dstNodes, j);
					bHasMatchedSomething = true;
					break;
				}
			}
		}
	} while (bHasMatchedSomething);

	// For a final test, simply match by name
	for (int i = 0; i < srcNodes.Count(); i++)
	{
		INode *pSrcNode = srcNodes[i];
		if (pSrcNode == NULL)
			continue;

		// Try to match by src name
		MSTR srcName = pSrcNode->GetName();

		for (int j = 0; j < dstNodes.Count(); j++)
		{
			if (MatchesByName(srcName, dstNodes[j]))
			{
				// Success - mark this node for matching
				SetMappingBetweenNodes(srcNodes, i, dstNodes, j);
				break;
			}
		}
	}
	return TRUE;
}
