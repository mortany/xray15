/**********************************************************************
 *<
	FILE: ffdmod.cpp

	DESCRIPTION: DllMain is in here

	CREATED BY: Ravi Karra

	HISTORY: created 1/11/99

 *>	Copyright (c) 1996 Rolf Berteig, All Rights Reserved.
 **********************************************************************/


#include "ffdmod.h"
#include "ffdui.h"
#include "istdplug.h"
#include <maxscript/maxscript.h>
#include <maxscript/foundation/3dmath.h>
#include <maxscript\macros\define_instantiation_functions.h>

// Maxscript stuff
def_visible_primitive			( conform,			"conformToShape" );
def_visible_primitive_debug_ok	( getDimensions,	"getDimensions" );
def_visible_primitive			( setDimensions,	"setDimensions" );
def_visible_primitive			( animateAll,		"animateAll" );
def_visible_primitive			( resetLattice,		"resetLattice" );

#define get_ffd_mod()																		\
	IFFDMod<Modifier>* ffd_osm = nullptr;													\
	IFFDMod<WSMObject>* ffd_wsm = nullptr;													\
	ReferenceTarget *ref = arg_list[0]->to_reftarg();										\
	SClass_ID sid = ref->SuperClassID();												\
	Class_ID cid = ref->ClassID();															\
	if (sid == WSM_OBJECT_CLASS_ID)															\
	{																						\
		if ( cid != FFDNMWSSQUARE_CLASS_ID && cid != FFDNMWSCYL_CLASS_ID  )					\
			throw RuntimeError(GetString(IDS_RK_NOT_FFD_ERROR), arg_list[0]);				\
		ffd_wsm = (IFFDMod<WSMObject>*)ref;													\
	}																						\
	else																					\
	{																						\
		Modifier *mod = arg_list[0]->to_modifier();											\
		if ( cid != FFDNMOSSQUARE_CLASS_ID && cid != FFDNMOSCYL_CLASS_ID &&					\
				cid != FFD44_CLASS_ID && cid != FFD33_CLASS_ID && cid !=FFD22_CLASS_ID )	\
			throw RuntimeError(GetString(IDS_RK_NOT_FFD_ERROR), arg_list[0]);				\
		ffd_osm = (IFFDMod<Modifier>*)mod;													\
	}


Value*
conform_cf(Value** arg_list, int count)
{
	check_arg_count(conform, 1, count);
	get_ffd_mod();
	ffd_osm ? ffd_osm->Conform() : ffd_wsm->Conform();
	needs_redraw_set();
	return &ok;
}

Value*
getDimensions_cf(Value** arg_list, int count)
{
	check_arg_count(setDimensions, 1, count);
	get_ffd_mod();
	IPoint3 p = ffd_osm ? ffd_osm->GetGridDim() : ffd_wsm->GetGridDim();
	return new Point3Value(Point3(p.x, p.y, p.z));	
}

Value*
setDimensions_cf(Value** arg_list, int count)
{
	check_arg_count(setDimensions, 2, count);
	get_ffd_mod();
	Point3 p = arg_list[1]->to_point3();
	IPoint3 ip3 (IPoint3((int)p.x, (int)p.y, (int)p.z));
	ffd_osm ? ffd_osm->SetGridDim(ip3) : ffd_wsm->SetGridDim(ip3);
	needs_redraw_set();
	return &ok;
}

Value*
animateAll_cf(Value** arg_list, int count)
{
	check_arg_count(animateAll, 1, count);
	get_ffd_mod();
	ffd_osm ? ffd_osm->AnimateAll() : ffd_wsm->AnimateAll();	
	needs_redraw_set();
	return &ok;
}

Value*
resetLattice_cf(Value** arg_list, int count)
{
	check_arg_count(resetLattice, 1, count);
	get_ffd_mod();
	ffd_osm ? ffd_osm->SetGridDim(ffd_osm->GetGridDim()) : ffd_wsm->SetGridDim(ffd_wsm->GetGridDim());
	ref->NotifyDependents(FOREVER,0,REFMSG_SUBANIM_STRUCTURE_CHANGED);
	needs_redraw_set();
	return &ok;
}

static ActionDescription spActions[] = {
	ID_SUBOBJ_TOP,
    IDS_SWITCH_TOP,
    IDS_SWITCH_TOP,
    IDS_RB_FFDGEN,

	ID_SUBOBJ_CP,
    IDS_SWITCH_CP,
    IDS_SWITCH_CP,
    IDS_RB_FFDGEN,

    ID_SUBOBJ_LATTICE,
    IDS_SWITCH_LATTICE,
    IDS_SWITCH_LATTICE,
    IDS_RB_FFDGEN,

    ID_SUBOBJ_SETVOLUME,
    IDS_SWITCH_SETVOLUME,			
    IDS_SWITCH_SETVOLUME,
    IDS_RB_FFDGEN,

	};

ActionTable* GetActions()
{
    TSTR name = GetString(IDS_RB_FFDGEN);
    HACCEL hAccel = LoadAccelerators(hInstance,
                                     MAKEINTRESOURCE(IDR_FFD_SHORTCUTS));
    int numOps = NumElements(spActions);
    ActionTable* pTab;
    pTab = new ActionTable(kFFDActions, kFFDContext, name, hAccel, numOps,
                             spActions, hInstance);        
    GetCOREInterface()->GetActionManager()->RegisterActionContext(kFFDContext, name.data());
	return pTab;
}


int sMyEnumProc::proc(ReferenceMaker *rmaker) 
	{ 
	if (rmaker->SuperClassID()==BASENODE_CLASS_ID)    
	{
		Nodes.Append(1, (INode **)&rmaker);  
		return DEP_ENUM_SKIP;
	}

	return DEP_ENUM_CONTINUE;
	}

