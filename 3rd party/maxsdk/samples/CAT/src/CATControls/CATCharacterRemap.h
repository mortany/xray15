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

#include <map>

typedef std::map<ReferenceTarget *, ReferenceTarget *> ReferenceMapping;
class CATCharacterRemap : public ReferenceMapping
{
public:

	RefTargetHandle FindMapping(RefTargetHandle from_instance)
	{
		ReferenceMapping::iterator itr = find(from_instance);
		if (itr != end())
			return itr->second;
		return NULL;
	}

	void AddEntry(RefTargetHandle from_instance, RefTargetHandle to_instance)
	{
		DbgAssert(from_instance != NULL && to_instance != NULL);
		DbgAssert(FindMapping(from_instance) == NULL || FindMapping(from_instance) == to_instance);

		if (FindMapping(from_instance) == NULL)
		{
			insert(std::pair<ReferenceTarget *, ReferenceTarget *>(from_instance, to_instance));
		}
	}

	// Call this function to run a 4-part node matching algorithm.
	bool BuildMapping(Tab<INode*>& srcNodes, Tab<INode*> dstNodes);

private:

	void SetMappingBetweenNodes(Tab<INode*>& srcExtraNodes, int idxSource, Tab<INode*>& dstExtraNodes, int idxDest);
	bool BuildMappingForSubAnims(ReferenceMaker* srcref, ReferenceMaker* tgtref);
};
