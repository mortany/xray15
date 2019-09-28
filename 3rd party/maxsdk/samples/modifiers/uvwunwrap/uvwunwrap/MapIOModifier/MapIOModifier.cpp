#include "MapIOModifier.h"
#include "MapIO.h"


class MapIOClassDesc:public ClassDesc2 {
	public:
	int 			IsPublic() { return TRUE; }
	void *			Create(BOOL loading = FALSE);
	const TCHAR *	ClassName() { return GetString(IDS_MAPIOMODIFIER); }
	SClass_ID		SuperClassID() { return OSM_CLASS_ID; }
	Class_ID		ClassID() { return MAPIOMODIFIER_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_RB_UNWRAPMOD); }

	const TCHAR*	InternalName() { return _T("MapIO"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }				// returns owning module handle

};



static MapIOClassDesc mapIOClassDesc;
ClassDesc* GetMapIODesc() { return &mapIOClassDesc; }


enum { mapio_params };


//TODO: Add enums for various parameters
enum { 
	pb_mapid,
	pb_usemap,
	pb_ismapapplied
};

// Blocks of UI
enum {
	unwrap_params
};

#define DEFAULT_CHANNEL 1

static ParamBlockDesc2 mapchannelpaste_param_blk (mapio_params, _T("params"),  0, &mapIOClassDesc,
	P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_REF,
	//rollout
	IDD_MAPIO_PARAMS, IDS_RB_PARAMETERS, 0, 0, NULL,

	// params
	pb_mapid, 			_T("mapID"), 		TYPE_INT, 	0, 	0, 
		p_default, DEFAULT_CHANNEL,
		p_range, 		1,99,
		p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_MAPIO_CHANNEL, IDC_MAPIO_CHAN_SPIN, SPIN_AUTOSCALE,
		p_end,

	pb_ismapapplied, _T("mapApplied"), TYPE_BOOL, P_RESET_DEFAULT, 0,
		p_default, FALSE,
		p_end,

	pb_usemap, 	_T("useMap"),		TYPE_BOOL, 		P_RESET_DEFAULT,				0,
		p_default, 		FALSE, 
		p_end, 

	p_end
	);


static FPInterfaceDesc mapio_interface(
	MAPIOMODIFIER_INTERFACE, _T("MapIO"), 0, &mapIOClassDesc, FP_MIXIN,

	IMapIOModifier::script_save, _T("Save"), 0, TYPE_VOID, 0, 1,
	_T("name"), 0, TYPE_TSTR_BR,

	IMapIOModifier::script_load, _T("Load"), 0, TYPE_VOID, 0, 1,
	_T("name"), 0, TYPE_TSTR_BR,

	IMapIOModifier::mapio_getMapChannel, _T("getMapChannel"), 0, TYPE_INT, 0, 0,
	IMapIOModifier::mapio_setMapChannel, _T("setMapChannel"), 0, TYPE_VOID, 0, 1,
	_T("mapChannel"), 0, TYPE_INT,

	IMapIOModifier::mapio_isMapApplied, _T("isMapApplied"), 0, TYPE_BOOL, 0, 0,
	IMapIOModifier::mapio_setMapApplied, _T("setMapApplied"), 0, TYPE_VOID, 0, 1,
	_T("mapApplied"), 0, TYPE_BOOL,


	IMapIOModifier::mapio_isMapValid, _T("isMapValid"), 0, TYPE_BOOL, 0, 0,

	p_end
);

IObjParam *MapIOModifier::ip			= NULL;

FPInterfaceDesc* IMapIOModifier::GetDesc()
{
	return &mapio_interface;
}


void *MapIOClassDesc::Create(BOOL loading)
{

	AddInterface(&mapio_interface);
	return new MapIOModifier();
}



//--- MapChannelPaste -------------------------------------------------------
MapIOModifier::MapIOModifier()
{
	isPoly = FALSE;
	mUVWStale = TRUE;
	pblock = NULL;
	mapIOClassDesc.MakeAutoParamBlocks(this);
}

MapIOModifier::~MapIOModifier()
{
}

/*===========================================================================*\
 |	The validity of the parameters.  First a test for editing is performed
 |  then Start at FOREVER, and intersect with the validity of each item
\*===========================================================================*/
Interval MapIOModifier::LocalValidity(TimeValue t)
{
	// if being edited, return NEVER forces a cache to be built 
	// after previous modifier.
	if (TestAFlag(A_MOD_BEING_EDITED))
		return NEVER;  
	//TODO: Return the validity interval of the modifier
	return NEVER;
}


/*************************************************************************************************
*
	Between NotifyPreCollapse and NotifyPostCollapse, Modify is
	called by the system.  NotifyPreCollapse can be used to save any plugin dependant data e.g.
	LocalModData
*
\*************************************************************************************************/

void MapIOModifier::NotifyPreCollapse(INode *node, IDerivedObject *derObj, int index)
{
	//TODO:  Perform any Pre Stack Collapse methods here
}



/*************************************************************************************************
*
	NotifyPostCollapse can be used to apply the modifier back onto to the stack, copying over the
	stored data from the temporary storage.  To reapply the modifier the following code can be 
	used

	Object *bo = node->GetObjectRef();
	IDerivedObject *derob = NULL;
	if(bo->SuperClassID() != GEN_DERIVOB_CLASS_ID)
	{
		derob = CreateDerivedObject(obj);
		node->SetObjectRef(derob);
	}
	else
		derob = (IDerivedObject*) bo;

	// Add ourselves to the top of the stack
	derob->AddModifier(this,NULL,derob->NumModifiers());

*
\*************************************************************************************************/

void MapIOModifier::NotifyPostCollapse(INode *node,Object *obj, IDerivedObject *derObj, int index)
{
	//TODO: Perform any Post Stack collapse methods here.

}


/*************************************************************************************************
*
	ModifyObject will do all the work in a full modifier
    This includes casting objects to their correct form, doing modifications
	changing their parameters, etc
*
\************************************************************************************************/


void MapIOModifier::ModifyObject(TimeValue t, ModContext &mc, ObjectState * os, INode *node) 
{
	//TODO: Add the code for actually modifying the object

	//get th map id
	int mapID = fnGetMapChannel();

		//get the mesh
	Mesh *mesh = NULL;
	MNMesh *mnmesh = NULL;
	PatchMesh *pmesh = NULL;
		
//	TriObject *collapsedtobj = NULL;
	if (os->obj->IsSubClassOf(triObjectClassID))
	{
		isPoly = FALSE;
		TriObject *tobj = (TriObject*)os->obj;
		mesh = &tobj->GetMesh();
	}
	else if (os->obj->IsSubClassOf(polyObjectClassID))
	{
		isPoly = TRUE;
		PolyObject *pobj = (PolyObject*)os->obj;
		mnmesh = &pobj->GetMesh();
	}
#ifndef NO_PATCHES
	else if (os->obj->IsSubClassOf(patchObjectClassID))
	{
		PatchObject *pobj = (PatchObject*)os->obj;
		pmesh = &pobj->patch;
	}
#endif // NO_PATCHES
	
	if (pmesh)
	{
	}
	else if (mnmesh)
	{
		//cheesy way to to determine if topo change
		if (mnmesh->numf != mMNMesh.numf) //reset the cache
		{
			mMNMesh = *mnmesh;
			for (int i = -NUM_HIDDENMAPS; i < mMNMesh.numm; i++)
			{
				mMNMesh.ClearMap(i);				
			}
			mUVWStale = TRUE;
		}

		//copy mmesh cache to uvw map

		if (mnmesh->M(mapID) == nullptr || mnmesh->M(mapID)->f == NULL)
		{
			mnmesh->SetMapNum(mapID + 1);
			mnmesh->InitMap(mapID);
		}

		MNMap* m = mnmesh->M(mapID);
		m->setNumVerts(mMNMesh.numv);

		for (int i = 0; i < mMNMesh.numv; i++)
		{
			m->v[i] = mMNMesh.v[i].p;
		}

		for (int i = 0; i < mnmesh->numf; i++)
		{
			if (mnmesh->f[i].GetFlag(MN_DEAD)) continue;
			int degree = mnmesh->f[i].deg;
			for (int j = 0; j < degree; j++)
			{
				m->f[i].tv[j] = mMNMesh.f[i].vtx[j];
			}
		}

	}
	else if (mesh)
	{
		//cheesy way to to determine if topo change
		if (mesh->numFaces != mMesh.numFaces) //reset the cache
		{
			mMesh = *mesh;
			for (int i = -NUM_HIDDENMAPS; i < mMesh.getNumMaps(); i++)
			{
				mMesh.setMapSupport(i, FALSE);				
			}
			mUVWStale = TRUE;
		}

		//copy mmesh cache to uvw map

		if (!mesh->mapSupport(mapID) || mesh->Map(mapID).tf == NULL)
		{
			mesh->setMapSupport(mapID);			
		}

		MeshMap& m = mesh->Map(mapID);
		m.setNumVerts(mMesh.numVerts);
		m.setNumFaces(mMesh.numFaces);

		for (int i = 0; i < mMesh.numVerts; i++)
		{
			m.tv[i] = mMesh.verts[i];
		}

		for (int i = 0; i < mesh->numFaces; i++)
		{
			int degree = 3;
			for (int j = 0; j < degree; j++)
			{
				m.tf[i].t[j] = mMesh.faces[i].v[j];
			}
		}
	}

	Interval iv;
	iv = FOREVER;

	//os->obj->PointsWereChanged();

	iv &= os->obj->ChannelValidity (t, VERT_COLOR_CHAN_NUM);
	iv &= os->obj->ChannelValidity (t, TEXMAP_CHAN_NUM);

	os->obj->UpdateValidity (VERT_COLOR_CHAN_NUM, iv);
	os->obj->UpdateValidity(TEXMAP_CHAN_NUM,iv);
}


void MapIOModifier::BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev )
{
	this->ip = ip;
	TimeValue t = ip->GetTime();
	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_BEGIN_EDIT);
	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_MOD_DISPLAY_ON);
	SetAFlag(A_MOD_BEING_EDITED);	

	mapIOClassDesc.BeginEditParams(ip, this, flags, prev);

}

void MapIOModifier::EndEditParams( IObjParam *ip, ULONG flags,Animatable *next)
{
	mapIOClassDesc.EndEditParams(ip, this, flags, next);

	TimeValue t = ip->GetTime();
	ClearAFlag(A_MOD_BEING_EDITED);
	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_END_EDIT);
	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_MOD_DISPLAY_OFF);
	this->ip = NULL;


}



Interval MapIOModifier::GetValidity(TimeValue t)
{
	Interval valid = FOREVER;
	//TODO: Return the validity interval of the modifier
	return valid;
}




RefTargetHandle MapIOModifier::Clone(RemapDir& remap)
{
	MapIOModifier* newmod = new MapIOModifier();	
	newmod->ReplaceReference(PBLOCK_REF,remap.CloneRef(pblock));
	BaseClone(this, newmod, remap);
	return(newmod);
}


//From ReferenceMaker 
RefResult MapIOModifier::NotifyRefChanged(
		const Interval& changeInt, RefTargetHandle hTarget,
		PartID& partID, RefMessage message, BOOL propagate) 
{
	//TODO: Add code to handle the various reference changed messages
	return REF_SUCCEED;
}

/****************************************************************************************
*
 	NotifyInputChanged is called each time the input object is changed in some way
 	We can find out how it was changed by checking partID and message
*
\****************************************************************************************/

void MapIOModifier::NotifyInputChanged(const Interval& changeInt, PartID partID, RefMessage message, ModContext *mc)
{

}



//From Object
BOOL MapIOModifier::HasUVW() 
{ 
	//TODO: Return whether the object has UVW coordinates or not
	return TRUE; 
}

void MapIOModifier::SetGenUVW(BOOL sw) 
{  
	if (sw==HasUVW()) return;
	//TODO: Set the plugin internal value to sw				
}

#define MMESH_CHUNK 7556
#define MNMESH_CHUNK 7557
#define IS_POLY_CHUNK 7558
#define UVW_STALE_CHUNK 7559

IOResult MapIOModifier::Load(ILoad *iload)
{
	ULONG nb;
	IOResult res;
	while (IO_OK == (res = iload->OpenChunk()))
	{
		switch (iload->CurChunkID())
		{
		case MMESH_CHUNK:
			res = mMesh.Load(iload);
			break;

		case MNMESH_CHUNK:
			res = mMNMesh.Load(iload);
			break;
			
		case IS_POLY_CHUNK:
			res = iload->Read(&isPoly, sizeof(BOOL), &nb);
			break;

		case UVW_STALE_CHUNK:
			res = iload->Read(&mUVWStale, sizeof(BOOL), &nb);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}
	
	return IO_OK;
}

IOResult MapIOModifier::Save(ISave *isave)
{
	Modifier::Save(isave);
	ULONG nb;

	isave->BeginChunk(MMESH_CHUNK);
	mMesh.Save(isave);
	isave->EndChunk();
	
	isave->BeginChunk(MNMESH_CHUNK);
	mMNMesh.Save(isave);
	isave->EndChunk();
	
	isave->BeginChunk(IS_POLY_CHUNK);
	isave->Write(&isPoly, sizeof(BOOL), &nb);
	isave->EndChunk();

	isave->BeginChunk(UVW_STALE_CHUNK);
	isave->Write(&mUVWStale, sizeof(BOOL), &nb);
	isave->EndChunk();

	return IO_OK;
}


int	MapIOModifier::fnGetMapChannel()
{
	int mapID = DEFAULT_CHANNEL;
	if (pblock)
	{
		TimeValue t = GetCOREInterface()->GetTime();
		pblock->GetValue(pb_mapid, t, mapID, FOREVER);
	}
	return mapID;
}

BOOL MapIOModifier::fnSetMapChannel(int mapID)
{
	if (pblock)
	{
		TimeValue t = GetCOREInterface()->GetTime();
		return pblock->SetValue(pb_mapid, t, mapID);
	}
	return FALSE;
}

BOOL MapIOModifier::fnIsMapApplied()
{
	BOOL applied = FALSE;
	if (pblock)
	{
		TimeValue t = GetCOREInterface()->GetTime();
		pblock->GetValue(pb_ismapapplied, t, applied, FOREVER);
	}
	return applied;
}

int MapIOModifier::fnSetMapApplied(BOOL applied)
{
	if (pblock)
	{
		TimeValue t = GetCOREInterface()->GetTime();
		return pblock->SetValue(pb_ismapapplied, t, applied);
	}
	return 0;
}

BOOL MapIOModifier::fnIsMapValid()
{
	if (mUVWStale) {
		return FALSE;
	}
	BOOL applied = FALSE;
	if (pblock)
	{
		TimeValue t = GetCOREInterface()->GetTime();
		pblock->GetValue(pb_ismapapplied, t, applied, FOREVER);


	}
	return applied;
}

int MapIOModifier::SetUseMapChannel(BOOL use)
{
	if (pblock)
	{
		TimeValue t = GetCOREInterface()->GetTime();
		pblock->SetValue(pb_usemap, t, use);
	}
	
	return 1;

}

void	MapIOModifier::fnSave(TSTR& fileName)
{
	MapIO* mapIO = nullptr;
	if (isPoly)
		mapIO = new MapIO(&mMNMesh);
	else {
		if (mMesh.numFaces == 0) {
			throw "Empty mesh";
		}
		mapIO = new MapIO(&mMesh);
	}

	mapIO->Write(fileName);

	delete mapIO;
}

void	MapIOModifier::fnLoad(TSTR& fileName)
{
	MapIO* mapIO = nullptr;
	if (isPoly)
		mapIO = new MapIO(&mMNMesh);
	else
		mapIO = new MapIO(&mMesh);

	mapIO->Read(fileName);

	mUVWStale = FALSE;

	delete mapIO;

	fnSetMapApplied(true);

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	if (GetCOREInterface())
		GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
}
