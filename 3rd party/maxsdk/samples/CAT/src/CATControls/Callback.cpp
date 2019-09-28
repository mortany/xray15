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
#include "Callback.h"

#include <algorithm>

CATCallbackRegister::CATCallbackRegister()
	: listCallbacks()
{
}

CATCallbackRegister::~CATCallbackRegister()
{
	listCallbacks.erase(listCallbacks.begin(), listCallbacks.end());
}

// This calls NotifyCallback() for each callback class registered.
//
void CATCallbackRegister::NotifyCallbacks()
{
	for (CallbackList::iterator it = listCallbacks.begin(); it != listCallbacks.end(); it++) {
		NotifyCallback(*it);
	}
}

// Registers a callback.  If the callback is already registered,
// just do nothing.
void CATCallbackRegister::RegisterCallback(CATCallback *cb)
{
	if (std::find(listCallbacks.begin(), listCallbacks.end(), cb) == listCallbacks.end()) {
		listCallbacks.push_back(cb);
	}
}

// Unregisters a callback.  If the callback isn't registered,
// just do nothing.
void CATCallbackRegister::UnRegisterCallback(CATCallback *cb)
{
	//	CallbackList::iterator it = std::find(listCallbacks.begin(), listCallbacks.end(), cb);
	//	if (it != listCallbacks.end()) {
	listCallbacks.remove(cb);
	//	}
}

///////////////////////////////////////////////////////////////
// CATActiveLayerChangeCallbackRegister
//
CATActiveLayerChangeCallbackRegister::CATActiveLayerChangeCallbackRegister() : nActiveLayer(0) {}

void CATActiveLayerChangeCallbackRegister::NotifyCallback(CATCallback *cb)
{
	((CATActiveLayerChangeCallback*)cb)->ActiveLayerChanged(nActiveLayer);
}

void CATActiveLayerChangeCallbackRegister::NotifyActiveLayerChange(int nLayer)
{
	nActiveLayer = nLayer;
	NotifyCallbacks();
}
