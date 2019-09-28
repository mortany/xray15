/**********************************************************************
 *<
	FILE: TreeViewUtil.h

	DESCRIPTION:	Includes for Plugins

	CREATED BY:

	HISTORY:

 *>	Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/

#pragma once

#include "..\modsres.h"

#include <Max.h>

#include <istdplug.h>
#include <iparamb2.h>
#include <iparamm2.h>
#include <notify.h>
#include <modstack.h>
#include <macrorec.h>
#include <utilapi.h>
#include <iFnPub.h>


extern TCHAR *GetString(int id);
extern HINSTANCE hInstance;


#define MAPIOMODIFIER_CLASS_ID	Class_ID(0x31fbb666, 0x3b4123a)

#define MAPIOMODIFIER_INTERFACE Interface_ID(0x5333409b, 0x122faab8)

/*

$.modifiers[#mapio].save "C:\\temp\\sphere.msh"
$.modifiers[#mapio].load "C:\\temp\\sphere.msh"

*/

class IMapIOModifier : public Modifier, public FPMixinInterface
{
public:
	enum
	{
		script_save,
		script_load,
		mapio_getMapChannel,
		mapio_setMapChannel,
		mapio_isMapApplied,
		mapio_setMapApplied,
		mapio_isMapValid
	};
	//Function Publishing System
	//Function Map For Mixin Interface
	//*************************************************
	BEGIN_FUNCTION_MAP

	VFN_1(script_save, fnSave, TYPE_TSTR_BR);
	VFN_1(script_load, fnLoad, TYPE_TSTR_BR);
	FN_0(mapio_getMapChannel, TYPE_INT, fnGetMapChannel);
	FN_1(mapio_setMapChannel, TYPE_BOOL, fnSetMapChannel, TYPE_INT);
	FN_0(mapio_isMapApplied, TYPE_BOOL, fnIsMapApplied);
	FN_1(mapio_setMapApplied, TYPE_INT, fnSetMapApplied, TYPE_BOOL);
	FN_0(mapio_isMapValid, TYPE_BOOL, fnIsMapValid);

	END_FUNCTION_MAP

	FPInterfaceDesc* GetDesc();    // <-- must implement 

	virtual void	fnSave(TSTR& fileName) = 0;
	virtual void	fnLoad(TSTR& fileName) = 0;
	virtual int		fnGetMapChannel() = 0;
	virtual BOOL	fnSetMapChannel(int channel) = 0;
	virtual BOOL	fnIsMapApplied() = 0;
	virtual int		fnSetMapApplied(BOOL applied) = 0;
	virtual BOOL	fnIsMapValid() = 0;

};


#define PBLOCK_REF 0


class MapIOModifier : public IMapIOModifier
{
	public:


		// Parameter block
		IParamBlock2	*pblock;	//ref 0


		static IObjParam *ip;			//Access to the interface
		
		// From Animatable
		const TCHAR *GetObjectName() { return GetString(IDS_MAPIOMODIFIER); }

		//From Modifier
		ChannelMask ChannelsUsed()  { return GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL|VERTCOLOR_CHANNEL|TEXMAP_CHANNEL; }
		//TODO: Add the channels that the modifier actually modifies
		ChannelMask ChannelsChanged() { return TEXMAP_CHANNEL|VERTCOLOR_CHANNEL; }
		//TODO: Return the ClassID of the object that the modifier can modify
		Class_ID InputType() {return defObjectClassID;}

		void ModifyObject(TimeValue t, ModContext &mc, ObjectState *os, INode *node);
		void NotifyInputChanged(const Interval& changeInt, PartID partID, RefMessage message, ModContext *mc);

		void NotifyPreCollapse(INode *node, IDerivedObject *derObj, int index);
		void NotifyPostCollapse(INode *node,Object *obj, IDerivedObject *derObj, int index);


		Interval LocalValidity(TimeValue t);

		// From BaseObject
		//TODO: Return true if the modifier changes topology
		BOOL ChangeTopology() {return TRUE;}		
		
		CreateMouseCallBack* GetCreateMouseCallBack() {return NULL;}

		BOOL HasUVW();
		void SetGenUVW(BOOL sw);


		void BeginEditParams(IObjParam *ip, ULONG flags,Animatable *prev);
		void EndEditParams(IObjParam *ip, ULONG flags,Animatable *next);

		Interval GetValidity(TimeValue t);

		// Automatic texture support
		
		// Loading/Saving
		IOResult Load(ILoad *iload);
		IOResult Save(ISave *isave);

		//From Animatable
		Class_ID ClassID() {return MAPIOMODIFIER_CLASS_ID;}		
		SClass_ID SuperClassID() { return OSM_CLASS_ID; }
		void GetClassName(TSTR& s) {s = GetString(IDS_MAPIOMODIFIER);}

		RefTargetHandle Clone( RemapDir &remap );
		RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, 
			PartID& partID, RefMessage message, BOOL propagate);


		int NumSubs() { return 1; }
		TSTR SubAnimName(int i) { return GetString(IDS_PARAMS); }				
		Animatable* SubAnim(int i) { return pblock; }

		// TODO: Maintain the number or references here
		int NumRefs() { return 1; }
		RefTargetHandle GetReference(int i) { return pblock; }

		BaseInterface* GetInterface(Interface_ID id)
		{
			if (id == MAPIOMODIFIER_INTERFACE)
				return (IMapIOModifier*)this;
			else
				return Modifier::GetInterface(id);
		}



		virtual void	fnSave(TSTR& fileName) override;
		virtual void	fnLoad(TSTR& fileName)  override;

private:
		virtual void SetReference(int i, RefTargetHandle rtarg) { pblock=(IParamBlock2*)rtarg; }
public:




		int	NumParamBlocks() { return 1; }					// return number of ParamBlocks in this instance
		IParamBlock2* GetParamBlock(int i) { return pblock; } // return i'th ParamBlock
		IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

		void DeleteThis() { delete this; }		
		//Constructor/Destructor

		MapIOModifier();
		~MapIOModifier();	

		int	fnGetMapChannel();
		BOOL fnSetMapChannel(int id);
		BOOL fnIsMapApplied();
		int fnSetMapApplied(BOOL applied);
		int SetUseMapChannel(BOOL use);
		BOOL fnIsMapValid();
		

protected:

		BOOL isPoly;
		MNMesh mMNMesh;
		Mesh mMesh;

		BOOL mUVWStale;
	private:

};



