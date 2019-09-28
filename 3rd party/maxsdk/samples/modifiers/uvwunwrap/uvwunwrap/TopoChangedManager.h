//
// Copyright 2015 Autodesk, Inc.  All rights reserved. 
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc. and
// are protected under applicable copyright and trade secret law.  They may
// not be disclosed to, copied or used by any third party without the prior
// written consent of Autodesk, Inc.
//

#pragma once
#include <Noncopyable.h>
#include "ITopoChangedListener.h"
#include <set>

/**
 * A topo changed manager can handle the topo changed event in UV space and
 * notify the listener who detects the topo changed event.
 **/
class TopoChangedManager
{
public:
	TopoChangedManager()
	{
		mTopoChangedListeners.clear();
	}
	virtual ~TopoChangedManager()
	{
		mTopoChangedListeners.clear();
	}
	virtual void RegisterTopoChangedListener(ITopoChangedListener* pTopoChangedListener);
	virtual void UnRegisterTopoChangedListener(ITopoChangedListener* pTopoChangedListener);
	virtual void NotifyTopoChanged();

private:
	typedef std::set<ITopoChangedListener*> TopoChangedListenerSet;
	TopoChangedListenerSet mTopoChangedListeners; 
};

inline void TopoChangedManager::RegisterTopoChangedListener(ITopoChangedListener* pTopoChangedListener)
{
	if(NULL == pTopoChangedListener)
	{
		return;
	}

	mTopoChangedListeners.insert(TopoChangedListenerSet::value_type(pTopoChangedListener));
}

inline void TopoChangedManager::UnRegisterTopoChangedListener(ITopoChangedListener* pTopoChangedListener)
{
	if(NULL == pTopoChangedListener)
	{
		return;
	}

	mTopoChangedListeners.erase(TopoChangedListenerSet::value_type(pTopoChangedListener));
}

inline void TopoChangedManager::NotifyTopoChanged()
{
	if(mTopoChangedListeners.empty())
	{
		return;
	}

	TopoChangedListenerSet::iterator it = mTopoChangedListeners.begin();
	for(; it != mTopoChangedListeners.end(); it++)
	{
		(*it)->HandleTopoChanged();
	}
}