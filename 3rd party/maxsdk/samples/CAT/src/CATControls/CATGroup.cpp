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

#include "CatPlugins.h"
#include "macrorec.h"
#include "CATMessages.h"
#include "CATClipValue.h"

#include "ICATParent.h"
#include "CATGroup.h"

#define		CATGROUPCHUNK_CATPARENT		1
#define		CATGROUPCHUNK_COLOUR		2

IOResult CATGroup::Save(ISave *)
{/*
	DWORD nb, refID;

	isave->BeginChunk(CATGROUPCHUNK_CATPARENT);
	refID = isave->GetRefID((void*)catparent);
	isave->Write( &refID, sizeof DWORD, &nb);
	isave->EndChunk();

	isave->BeginChunk(CATGROUPCHUNK_COLOUR);
	isave->Write( &dwColour, sizeof(COLORREF), &nb);
	isave->EndChunk();
	*/
	return IO_OK;
}

IOResult CATGroup::Load(ILoad *)
{
	/*	IOResult res = IO_OK;
		DWORD nb, refID;

		while (IO_OK == (res = iload->OpenChunk())) {
			switch (iload->CurChunkID()) {
			case CATGROUPCHUNK_CATPARENT:
				res = iload->Read(&refID, sizeof DWORD, &nb);
				if (res == IO_OK && refID != (DWORD)-1)
					iload->RecordBackpatch(refID, (void**)&catparent);
				break;
			case CATGROUPCHUNK_COLOUR:
				res = iload->Read(&dwColour, sizeof(COLORREF), &nb);
				if (res != IO_OK) break;
				break;

			default: break;
			}
			iload->CloseChunk();
			if (res != IO_OK) return res;
		}
	*/	return IO_OK;
}
