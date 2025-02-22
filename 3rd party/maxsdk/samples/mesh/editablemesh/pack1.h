/**********************************************************************
 *<
	FILE: pack1.h

	DESCRIPTION:

	CREATED BY: Rolf Berteig

	HISTORY:

 *>	Copyright (c) 1996, All Rights Reserved.
 **********************************************************************/

#ifndef __PACK1__H
#define __PACK1__H

#include "Max.h"
#include "resource.h"
#include "resourceOverride.h"

TCHAR *GetString(int id);

extern ClassDesc* GetEditTriObjectDesc();

class StaticObject {
public:
	Object *ob;
	StaticObject () { ob = NULL; }
	~StaticObject () { Shutdown(); }
	Object *GetEditTriOb ();

   void Shutdown()
   {
      if (ob) { ob->DeleteThis(); ob = NULL; }
   }
};

extern StaticObject statob;
extern HINSTANCE hInstance;
extern int enabled;

#endif
