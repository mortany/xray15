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

#include "CATObject Superclass.h"

extern ClassDesc2* GetCATHubDesc();

#define CATHUB_CLASS_ID	Class_ID(0x73dc4833, 0x65c93caa)

class HubObject : public CATObject {
	public:

		enum PARAMLIST { HUB_PBLOCK_REF, NUMPARAMS};		// our subanim list.

		enum { raifobjects_params };

		//TODO: Add enums for various parameters
		enum {
			PB_HUBTRANS,
			PB_CATUNITS,
			PB_LENGTH,
			PB_WIDTH,
			PB_HEIGHT,
			PB_PIVOTPOSY,
			PB_PIVOTPOSZ
		};

		// Parameter block handled by parent

		// From SimpleObject
		void BuildMesh(TimeValue t);
		BOOL OKtoDisplay(TimeValue t);
		void InvalidateUI();

		//From Animatable
		Class_ID ClassID() { return CATHUB_CLASS_ID; }
		void GetClassName(TSTR& s) {s = GetString(IDS_CL_HUBOBJECT);}

		RefTargetHandle Clone( RemapDir &remap );

		RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL);

		int	NumParamBlocks() { return 1; }					// return number of ParamBlocks in this instance
		IParamBlock2* GetParamBlock(int) {return pblock2; } // return i'th ParamBlock
		IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock2->ID() == id) ? pblock2 : NULL; } // return id'd ParamBlock

		RefTargetHandle GetReference(int i);

		void DeleteThis() { delete this; }
		//Constructor/Destructor

		HubObject();
		~HubObject();

		void SetParams(float width, float height, float length, float pivotY=0.0f, float pivotZ=0.0f);

		//////////////////////////////////////////////////////////////////////////
		BOOL SaveRig(CATRigWriter *save);
		BOOL LoadRig(CATRigReader *load);

		void	 SetTransformController(Control* ctrl){ pblock2->SetValue(PB_HUBTRANS, 0, (ReferenceTarget*)ctrl); };

		Object* AsObject(){ return this; };

		float GetX(){ return pblock2->GetFloat(PB_WIDTH); }
		float GetY(){ return pblock2->GetFloat(PB_LENGTH);	}
		float GetZ(){ return pblock2->GetFloat(PB_HEIGHT); }

		void SetX(float val){ pblock2->SetValue(PB_WIDTH, 0, val); };
		void SetY(float val){ pblock2->SetValue(PB_LENGTH, 0, val); };
		void SetZ(float val){ pblock2->SetValue(PB_HEIGHT, 0, val); };

		float GetCATUnits(){ return pblock2->GetFloat(PB_CATUNITS); }
		void SetCATUnits(float val){ pblock2->SetValue(PB_CATUNITS, 0, val); };

		void Update(){ ivalid = NEVER;	NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE); }

		virtual BOOL PasteRig(ICATObject* pasteobj, DWORD flags, float scalefactor){

			HubObject* pastehubobj = (HubObject*)pasteobj->AsObject();
			pblock2->SetValue(PB_PIVOTPOSZ, 0, pastehubobj->pblock2->GetFloat(PB_PIVOTPOSZ));
			pblock2->SetValue(PB_PIVOTPOSY, 0, pastehubobj->pblock2->GetFloat(PB_PIVOTPOSY));

			// this guy will paste the rest of the properties
			return CATObject::PasteRig(pasteobj, flags, scalefactor);
		}

};

extern ClassDesc2* GetCATHubDesc();
