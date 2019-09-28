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

#include <list>

 ////////////////////////////////////////////////////////////////
 // CATCallback
 //
class CATCallback : public MaxHeapOperators
{
public:
};

////////////////////////////////////////////////////////////////
// CATCallbackRegister
//
// This maintains a collection of registered callbacks.
// Callbacks are called in the order in which they were
// registered.
//
class CATCallbackRegister {
private:
	typedef std::list<CATCallback*> CallbackList;
	CallbackList listCallbacks;

protected:
	// This is called for each element of the list
	virtual void NotifyCallback(CATCallback *cb) = 0;

	void NotifyCallbacks();

public:
	CATCallbackRegister();
	virtual ~CATCallbackRegister();

	void RegisterCallback(CATCallback *cb);
	void UnRegisterCallback(CATCallback *cb);
};

////////////////////////////////////////////////////////////////
// CATActiveLayerChangeCallback
//
// A class inherits this if it wishes to be notified by the
// clip hierarchy when the active layer changes.  It must
// call CATClipRoot::RegisterActiveLayerChangeCallback().
//
class CATActiveLayerChangeCallback : public CATCallback {
public:
	virtual void ActiveLayerChanged(int nLayer) = 0;
};

////////////////////////////////////////////////////////////////
// CATActiveLayerChangeCallbackRegister
//
//
//
class CATActiveLayerChangeCallbackRegister : public CATCallbackRegister {
protected:
	int nActiveLayer;

	void NotifyCallback(CATCallback *cb);

public:
	CATActiveLayerChangeCallbackRegister();
	void NotifyActiveLayerChange(int nLayer);
};

