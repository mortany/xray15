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

class IProgressWindow : public MaxHeapOperators
{
public:
	virtual ~IProgressWindow() {};
	virtual BOOL ProgressBegin(const TCHAR *szCaption, const TCHAR *szMessage) = 0;
	virtual void ProgressEnd() = 0;

	virtual void SetCaption(const TCHAR *szCaption) = 0;
	virtual void SetMessage(const TCHAR *szMessage) = 0;
	virtual void SetPercent(int nPercent) = 0;
};

extern IProgressWindow *GetProgressWindow();
