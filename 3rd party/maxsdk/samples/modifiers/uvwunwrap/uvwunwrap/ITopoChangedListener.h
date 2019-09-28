/*

Copyright 2015 Autodesk, Inc.  All rights reserved.

Use of this software is subject to the terms of the Autodesk license agreement provided at
the time of installation or download, or which otherwise accompanies this software in either
electronic or hard copy form.

*/

#pragma once

/**
 * An topo changed listener detect the topo changed event and handle the event.
 **/
class ITopoChangedListener
{
public:
	virtual void HandleTopoChanged() = 0;
};