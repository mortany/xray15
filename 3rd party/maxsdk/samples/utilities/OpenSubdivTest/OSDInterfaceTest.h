/**********************************************************************
 *<
	FILE: OSDInterfaceTest.h

	DESCRIPTION:	Sample OpenSubdiv Interface Access Test

	CREATED BY:	Tom Hudson

	HISTORY:

 *>	Copyright (c) 2014, All Rights Reserved.
 **********************************************************************/

#pragma once

#include "Max.h"
#include "resource.h"
#include "utilapi.h"
#include "istdplug.h"
#include <OSDParamInterface.h>

extern ClassDesc* GetOSDInterfaceTestDesc();

extern HINSTANCE hInstance;

class PickTarget;

class OSDInterfaceTest : public UtilityObj {
	public:
		IUtil *iu;
		Interface *ip;
		HWND hPanel;
		static PickTarget pickCB;

		OSDInterfaceTest();
		~OSDInterfaceTest();

		void BeginEditParams(Interface *ip,IUtil *iu);
		void EndEditParams(Interface *ip,IUtil *iu);
		void DeleteThis() {}

		void Init(HWND hWnd);
		void Destroy(HWND hWnd);
		
		void ClearInfoStrings();
		void CheckForOSDInterface(INode *node);
};

class PickTarget : 
      public PickModeCallback,
      public PickNodeCallback {
   public:     
      OSDInterfaceTest *it;           

      PickTarget() : it(NULL) {}

      BOOL HitTest(IObjParam *ip,HWND hWnd,ViewExp *vpt,IPoint2 m,int flags);
      BOOL Pick(IObjParam *ip,ViewExp *vpt);

      void EnterMode(IObjParam *ip);
      void ExitMode(IObjParam *ip);

      BOOL RightClick(IObjParam *ip,ViewExp *vpt)  {return TRUE;}

      BOOL Filter(INode *node);
      
      PickNodeCallback *GetFilter() {return this;}
      BOOL AllowMultiSelect() {return FALSE;}
   };
