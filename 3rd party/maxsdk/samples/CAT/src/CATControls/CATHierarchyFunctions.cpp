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

// These are functions used by CATMotionHierarchy and CATClipMotionHierarchy
//
#include "CATPlugins.h"
#include "CATHierarchyFunctions.h"

#define CAT_ANYSTRING_CHUNK		0xD1E

// Saves a single string in its own chunk.  This allows you to store
// multiple strings in one chunk.
IOResult WriteStringChunk(ISave *isave, const TSTR &str)
{
	isave->BeginChunk(CAT_ANYSTRING_CHUNK);
	isave->WriteCString(str.data());
	isave->EndChunk();
	return IO_OK;
}

// Loads a single string from its own chunk.
IOResult ReadStringChunk(ILoad *iload, TSTR &str)
{
	IOResult ok = IO_OK;
	TCHAR *strBuf;

	if (IO_OK == (ok = iload->OpenChunk())) {
		if (iload->CurChunkID() == CAT_ANYSTRING_CHUNK) {
			ok = iload->ReadCStringChunk(&strBuf);
			if (ok == IO_OK) str = strBuf;
		}
		else {
			ok = IO_ERROR;
		}
		iload->CloseChunk();
	}
	return ok;
}
