/**********************************************************************
*<
FILE: BlobMesh.cpp

DESCRIPTION:   Appwizard generated plugin

CREATED BY: 

HISTORY: 

add filter to pick object to prevent dupes and circular dependancies

add filters to the pf evernt picker

*>   Copyright (c) 2000, All Rights Reserved.
**********************************************************************/

#include "BlobMesh.h"
#include <IParticleObjectExt.h>
#include <3dsmaxport.h>
#include <vector>
#include <polyshp.h>

int MyEnumProc::proc(ReferenceMaker *rmaker) 
{ 
	if (rmaker->SuperClassID()==BASENODE_CLASS_ID)    
	{
		Nodes.Append(1, (INode **)&rmaker);  
		if( mbDoHalt )		
			return DEP_ENUM_HALT;
	}

	return DEP_ENUM_CONTINUE;
}


static BlobMeshClassDesc BlobMeshDesc;
ClassDesc2* GetBlobMeshDesc() { return &BlobMeshDesc; }




static FPInterfaceDesc blobmesh_interface(
	BLOBMESH_INTERFACE, _T("blobMeshOps"), 0, &BlobMeshDesc , FP_MIXIN,
	blobmesh_addnode, _T("addBlob"), 0, TYPE_VOID, 0, 1,
	_T("node"), 0, TYPE_INODE,
	blobmesh_removenode, _T("removeBlob"), 0, TYPE_VOID, 0, 1,
	_T("node"), 0, TYPE_INODE,

	blobmesh_addpfnode, _T("addPFBlob"), 0, TYPE_VOID, 0, 1,
	_T("node"), 0, TYPE_INODE,
	blobmesh_removepfnode, _T("removePFBlob"), 0, TYPE_VOID, 0, 1,
	_T("node"), 0, TYPE_INODE,

	blobmesh_pickmode, _T("pick"), 0, TYPE_VOID, 0, 0,
	blobmesh_addmode,_T("add"), 0, TYPE_VOID, 0, 0,
	blobmesh_addpfmode, _T("addPF"), 0, TYPE_VOID, 0, 0,

	p_end
	);


void *BlobMeshClassDesc::Create(BOOL loading)
{
	AddInterface(&blobmesh_interface);
	return new BlobMesh;
}

FPInterfaceDesc* IBlobMesh::GetDesc()
{
	return &blobmesh_interface;
}






static ParamBlockDesc2 blobmesh_param_blk ( blobmesh_params, _T("params"),  0, &BlobMeshDesc, 
	P_AUTO_CONSTRUCT + P_AUTO_UI+P_MULTIMAP, PBLOCK_REF, 
	//rollout
	2,
	blobmesh_baseparams, IDD_PANEL, IDS_PARAMS, 0, 0, NULL,
	blobmesh_pfparams, IDD_PFPANEL, IDS_PFPARAM, 0, 0, NULL,

	// params
	pb_size,          _T("size"),       TYPE_WORLD,    P_ANIMATABLE,  IDS_SIZE, 
	p_default,     20.f, 
	p_range,       0.0001f,1000.0f, 
	p_ui,          blobmesh_baseparams, TYPE_SPINNER,     EDITTYPE_UNIVERSE, IDC_SIZE2, IDC_SIZE2SPIN, 0.1f, 
	p_end,

	pb_tension,       _T("tension"),       TYPE_FLOAT,    P_ANIMATABLE,  IDS_TENSION, 
	p_default,     1.f, 
	p_range,       0.01f,1.0f, 
	p_ui,          blobmesh_baseparams, TYPE_SPINNER,     EDITTYPE_FLOAT, IDC_TENSION,  IDC_TENSIONSPIN, 0.1f, 
	p_end,

	pb_render,     _T("renderCoarseness"),    TYPE_WORLD,    P_ANIMATABLE,  IDS_RENDER, 
	p_default,     3.f, 
	p_range,       0.0001f,1000.0f, 
	p_ui,          blobmesh_baseparams, TYPE_SPINNER,     EDITTYPE_FLOAT, IDC_RENDER,   IDC_RENDERSPIN, 0.1f, 
	p_end,

	pb_viewport,      _T("viewportCoarseness"),  TYPE_WORLD,    P_ANIMATABLE,  IDS_VIEWPORT, 
	p_default,     6.f, 
	p_range,       0.0001f,1000.0f, 
	p_ui,          blobmesh_baseparams, TYPE_SPINNER,     EDITTYPE_FLOAT, IDC_VIEWPORT, IDC_VIEWPORTSPIN, 0.1f, 
	p_end,

	pb_relativecoarseness,  _T("relativeCoarseness"),  TYPE_BOOL,     0,          IDS_COARSENESS,
	p_default,    FALSE,
	p_ui,          blobmesh_baseparams,TYPE_SINGLECHECKBOX,   IDC_AUTOCOARSE, 
	p_end, 

	pb_nodelist,    _T("nodeList"),  TYPE_INODE_TAB,      0, P_AUTO_UI|P_VARIABLE_SIZE, IDS_NODELIST,
	p_ui,       blobmesh_baseparams, TYPE_NODELISTBOX, IDC_LIST1,0,0,IDC_REMOVE,
	p_end,

	pb_usesoftsel,    _T("useSoftSelection"),    TYPE_BOOL,     0,          IDS_USESOFTSEL,
	p_default,    FALSE,
	p_ui,          blobmesh_baseparams, TYPE_SINGLECHECKBOX,  IDC_USESOFTSEL, 
	p_end, 

	pb_minsize,          _T("minSize"),       TYPE_WORLD,    P_ANIMATABLE,  IDS_MINSIZE, 
	p_default,     10.f, 
	p_range,       0.0001f,1000.0f, 
	p_ui,          blobmesh_baseparams, TYPE_SPINNER,     EDITTYPE_UNIVERSE, IDC_MINSIZE,  IDC_MINSIZESPIN, 0.1f, 
	p_end,

	pb_oldmetaballmethod,   _T("largeDataSetOptimization"),  TYPE_BOOL,     0,          IDS_OLDMETHOD,
	p_default,    FALSE,
	p_ui,          blobmesh_baseparams, TYPE_SINGLECHECKBOX,  IDC_OLDMETHOD, 
	p_end, 

	pb_useallpf,   _T("useAllPFEvents"),   TYPE_BOOL,     0,          IDS_USEALLPFEVENTS,
	p_default,    TRUE,
	p_ui,          blobmesh_pfparams, TYPE_SINGLECHECKBOX,    IDC_USEALLPF, 
	p_end, 
	pb_pfeventlist,    _T("pfEventList"),  TYPE_INODE_TAB,      0, P_AUTO_UI|P_VARIABLE_SIZE, IDS_PFEVENTLIST,
	p_ui,       blobmesh_pfparams, TYPE_NODELISTBOX, IDC_PFLIST,0,0,IDC_PFREMOVE,
	p_end,


	pb_offinview,  _T("offInView"),  TYPE_BOOL,     0,          IDS_OFFINVIEW,
	p_default,    FALSE,
	p_ui,          blobmesh_baseparams, TYPE_SINGLECHECKBOX,  IDC_OFFINVIEW, 
	p_end, 


	p_end
	);




IObjParam *BlobMesh::ip       = NULL;


BOOL BlobMeshValidatorClass::Validate(PB2Value &v)
{
	INode *node = (INode*) v.r;

	for (int i = 0; i < mod->pblock2->Count(pb_nodelist); i++)
	{
		if (node == mod->pblock2->GetINode(pb_nodelist, 0, i))
			return FALSE;
	}

	node->BeginDependencyTest();
	mod->NotifyDependents(FOREVER,0,REFMSG_TEST_DEPENDENCY);
	if (node->EndDependencyTest()) 
	{     
		return FALSE;
	} 

	return TRUE;
}




//--- BlobMesh -------------------------------------------------------

BlobMesh::BlobMesh()
{
	inPickMode = FALSE;
	selfNode = NULL;
	inRender = FALSE;
	BlobMeshDesc.MakeAutoParamBlocks(this);
	validator.mod = this; 
}

BlobMesh::~BlobMesh()
{
}

IOResult BlobMesh::Load(ILoad *iload)
{
	//TODO: Add code to allow plugin to load its data

	return IO_OK;
}

IOResult BlobMesh::Save(ISave *isave)
{
	//TODO: Add code to allow plugin to save its data

	return IO_OK;
}

/*
void BlobMesh::RescaleWorldUnits(float f) {
if (TestAFlag(A_WORK1))
return;
SetAFlag(A_WORK1);
pblock2->RescaleParam(pb_size,f);   
pblock2->RescaleParam(pb_minsize,f);   
}
*/

void BlobMesh::BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev)
{
	this->ip = ip;

	SimpleObject2::BeginEditParams(ip,flags,prev);
	BlobMeshDesc.BeginEditParams(ip, this, flags, prev);

	blobmesh_param_blk.SetUserDlgProc(blobmesh_pfparams,new BlobMeshDlgProc(this));
	blobmesh_param_blk.ParamOption(pb_nodelist,p_validator,&validator);

	blobmesh_param_blk.SetUserDlgProc(blobmesh_params,new BlobMeshParamsDlgProc(this));

}

void BlobMesh::EndEditParams( IObjParam *ip, ULONG flags,Animatable *next )
{
	//TODO: Save plugin parameter values into class variables, if they are not hosted in ParamBlocks. 


	SimpleObject2::EndEditParams(ip,flags,next);
	BlobMeshDesc.EndEditParams(ip, this, flags, next);

	ip->ClearPickMode();

	this->ip = NULL;
}

//From Object
BOOL BlobMesh::HasUVW() 
{ 
	//TODO: Return whether the object has UVW coordinates or not
	return TRUE; 
}

void BlobMesh::SetGenUVW(BOOL sw) 
{  
	if (sw==HasUVW()) return;
	//TODO: Set the plugin's internal value to sw            
}

//Class for interactive creation of the object using the mouse
class BlobMeshCreateCallBack : public CreateMouseCallBack {
	IPoint2 sp0;      //First point in screen coordinates
	BlobMesh *ob;     //Pointer to the object 
	Point3 p0;        //First point in world coordinates
public:  
	int proc( ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat);
	void SetObj(BlobMesh *obj) {ob = obj;}
};

int BlobMeshCreateCallBack::proc(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat )
{
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	//TODO: Implement the mouse creation code here
	if (msg==MOUSE_POINT||msg==MOUSE_MOVE) {
		switch(point) {
		case 0: // only happens with MOUSE_POINT msg
			ob->suspendSnap = TRUE;
			sp0 = m;
			p0 = vpt->SnapPoint(m,m,NULL,SNAP_IN_PLANE);
			mat.SetTrans(p0);
			return CREATE_STOP;
			break;
			//TODO: Add for the rest of points
		}
	} else {
		if (msg == MOUSE_ABORT) return CREATE_ABORT;
	}

	return TRUE;
}

static BlobMeshCreateCallBack BlobMeshCreateCB;

//From BaseObject
CreateMouseCallBack* BlobMesh::GetCreateMouseCallBack() 
{
	BlobMeshCreateCB.SetObj(this);
	return(&BlobMeshCreateCB);
}

int BlobMesh::RenderBegin(TimeValue t, ULONG flags)
{
	SetAFlag(A_RENDER);
	inRender = TRUE;
	ivalid.SetEmpty();
	NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
	return 0;
}

int BlobMesh::RenderEnd(TimeValue t)
{
	ClearAFlag(A_RENDER);
	inRender = FALSE;
	ivalid.SetEmpty();
	NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
	return 0;

}


//From SimpleObject
const TCHAR* BlobMesh::GetObjectName()
{
	return GetString(IDS_CLASS_NAME_IMP);
}

void BlobMesh::BuildMesh(TimeValue t)
{

	//TODO: Implement the code to build the mesh representation of the object
	//    using its parameter settings at the time passed. The plug-in should 
	//    use the data member mesh to store the built mesh.
	int numberOfNodes = pblock2->Count(pb_nodelist);

	float size, tension, renderCoarseness, viewCoarseness,coarseness;

	// Interval valid;
	pblock2->GetValue(pb_size,t,size,ivalid);
	pblock2->GetValue(pb_tension,t,tension,ivalid);
	if (tension < 0.011f) tension = 0.011f;
	if (tension > 1.0f) tension = 1.0f;

	pblock2->GetValue(pb_render,t,renderCoarseness,ivalid);
	pblock2->GetValue(pb_viewport,t,viewCoarseness,ivalid);

	BOOL autoCoarseness;
	pblock2->GetValue(pb_relativecoarseness,t,autoCoarseness,ivalid);

	BOOL largeDataSetOpt;
	pblock2->GetValue(pb_oldmetaballmethod,t,largeDataSetOpt,ivalid);


	BOOL useSoftSel;
	float minSize;
	pblock2->GetValue(pb_usesoftsel,t,useSoftSel,ivalid);
	pblock2->GetValue(pb_minsize,t,minSize,ivalid);

	if (inRender)
		coarseness = renderCoarseness;
	else coarseness = viewCoarseness;

	if (autoCoarseness)
		coarseness = size/coarseness;

	Tab<INode *> pfList;

	BOOL useAllPFEvents;
	pblock2->GetValue(pb_useallpf,0,useAllPFEvents,FOREVER);



	int pfCt = pblock2->Count(pb_pfeventlist);

	BOOL offInView;
	pblock2->GetValue(pb_offinview,0,offInView,FOREVER);



	for (int i = 0; i < pfCt; i++)
	{
		INode* node = NULL;
		pblock2->GetValue(pb_pfeventlist,0,node,FOREVER,i);
		if (node)
			pfList.Append(1,&node);
	}

	float thres = 0.6f;

	//need to get our tm
	if (selfNode == NULL)
	{
		MyEnumProc dep(true);
		DoEnumDependents(&dep);
		if (dep.Nodes.Count() > 0)
			selfNode = dep.Nodes[0];
	}

	Matrix3 baseTM(1);

	if (selfNode != NULL)
		baseTM = Inverse(selfNode->GetObjectTM(t));
	//loop throght the nodes

	if (!inRender)
	{
		if (offInView)
			numberOfNodes = 0;

	}

	std::vector<SphereData> data;
	data.reserve(500);

	for (int i = 0; i < numberOfNodes; i++)
	{
		INode* node = NULL;
		pblock2->GetValue(pb_nodelist,t,node,ivalid,i);

		if (node)
		{
			//get the nodes tm
			Matrix3 objectTM = node->GetObjectTM(t);
			Matrix3 toLocalSpace = objectTM*baseTM;

			ObjectState tos =  node->EvalWorldState(t,TRUE);

			if (tos.obj->IsParticleSystem())
			{
				SimpleParticle* pobj      = NULL;
				IParticleObjectExt* epobj = NULL;
				pobj = static_cast<SimpleParticle*>( tos.obj->GetInterface(I_SIMPLEPARTICLEOBJ) );
				if (pobj)
				{
					pobj->UpdateParticles(t, node);

					int count = pobj->parts.Count();
					data.reserve(data.size() + count);
					float closest = 999999999.9f;
					SphereData d;
					for (int pid = 0; pid < count; pid++)
					{
						TimeValue age  = pobj->ParticleAge(t,pid);
						TimeValue life = pobj->ParticleLife(t,pid);
						if (age != -1)
						{
							float psize = pobj->ParticleSize(t,pid);
							Point3 curval = pobj->parts.points[pid];
							d.center = curval * baseTM;
							d.radius = psize;
							d.oradius = psize;
							d.rsquare = psize * psize;
							d.tover4 = tension * d.rsquare *d.rsquare ;
							data.push_back(d);
						}
					}
				}
				else
				{
					epobj = (IParticleObjectExt*) tos.obj->GetInterface(PARTICLEOBJECTEXT_INTERFACE);
					if (epobj) 
					{
						
						epobj->UpdateParticles(node, t);

						int count = epobj->NumParticles();
						data.reserve(data.size() + count);
						for (int pid = 0; pid < count; pid++)
						{
							TimeValue age  = epobj->GetParticleAgeByIndex(pid);
							if (age!=-1)
							{
								INode *node = epobj->GetParticleGroup(pid);

								Point3 *curval = epobj->GetParticlePositionByIndex(pid);

								BOOL useParticle = TRUE;

								if (!useAllPFEvents)
								{
									useParticle = FALSE;
									for (int k = 0; k < pfList.Count(); k++)
									{
										if (node == pfList[k])
										{
											useParticle = TRUE;
											k = pfList.Count();
										}
									}
								}

								if ((curval) && (useParticle))
								{
									float scale = epobj->GetParticleScaleByIndex(pid) ;
									float psize = scale;

									SphereData d;
									d.center = *curval*baseTM;
									d.radius = psize;
									d.oradius = psize;
									d.rsquare = psize * psize;
									d.tover4 = tension * d.rsquare *d.rsquare ;
									data.push_back(d);
								}
							}
						}
					}

				}
			}
			else if (tos.obj->IsShapeObject()) 
			{
				PolyShape shape;

				ShapeObject *pathOb = (ShapeObject*)tos.obj;
				pathOb->MakePolyShape(t, shape);
				
				// first find out how many points there are:
				size_t num_points = 0;
				for (int i = 0; i < shape.numLines; i++)
				{
					PolyLine& line = shape.lines[i];
					num_points += line.numPts;
				}
				data.reserve(data.size() + num_points);

				for (int i = 0; i < shape.numLines; i++)
				{
					PolyLine line = shape.lines[i];
					for (int j = 0; j < line.numPts; j++)
					{
						SphereData d;
						float tsize = size;
						d.center = line.pts[j].p*toLocalSpace;
						d.radius = tsize;
						d.oradius = tsize;
						d.rsquare = tsize * tsize;
						d.tover4 = tension * d.rsquare *d.rsquare ;
						data.push_back(d);
					}
				}
			}
			else if (tos.obj->SuperClassID()==GEOMOBJECT_CLASS_ID)
			{
				SphereData d;
				BOOL converted = FALSE;

				TriObject *triObj = NULL;


				if(tos.obj->IsSubClassOf(triObjectClassID)) 
				{
					triObj = (TriObject *)tos.obj;
				}

				// If it can convert to a TriObject, do it
				else if(tos.obj->CanConvertToType(triObjectClassID)) 
				{
					triObj = (TriObject *)tos.obj->ConvertToType(t, triObjectClassID);
					converted = TRUE;
				}

				if (triObj != NULL)
				{
					Mesh* mesh = &triObj->GetMesh();
					if (mesh)
					{
						int vcount = mesh->getNumVerts();
						float *vsw = mesh->getVSelectionWeights ();
						BitArray vsel =  mesh->VertSel();
						data.reserve(data.size() + vcount);
						for (int j = 0; j < vcount; j++)
						{
							float tsize = size;
							if (useSoftSel)
							{
								tsize = 0.0f;
								if (vsw)
								{
									float v = 0.0f;
									if (vsel[j]) 
										v = 1.0f;
									else 
									{
										if (vsw)
											v = vsw[j];
									}
									if (v == 0.0f)
										tsize = 0.0f;
									else
									{
										tsize = minSize + (size -minSize)*v;
									}

								}
								else
								{
									float v = 0.0f;
									if (vsel[j]) 
										v = 1.0f;
									tsize = minSize + (size -minSize)*v;
								}
							}
							if (tsize != 0.0f)
							{
								d.center = mesh->getVert(j)*toLocalSpace;
								d.radius = tsize;
								d.oradius = tsize;
								d.rsquare = tsize * tsize;
								d.tover4 = tension * d.rsquare *d.rsquare ;
								data.push_back(d);
							}
						}
					}

					if (converted) triObj->DeleteThis();
				}
			}
			else
			{
				SphereData d;
				d.center = Point3(0.0f,0.0f,0.0f)*toLocalSpace;
				d.radius = size;
				d.oradius = size;
				d.rsquare = size * size;
				d.tover4 = tension * d.rsquare *d.rsquare ;
				data.push_back(d);
			}
		}
	}


	if ((data.size() == 0) && (numberOfNodes==0))
	{
		data.resize(1);
		data[0].center = Point3(0.0f,0.0f,0.0f);
		data[0].radius = size;
		data[0].oradius = size;
		data[0].rsquare = size * size;
		data[0].tover4 = tension * data[0].rsquare *data[0].rsquare ;

	}

	if (data.size() > 0)
	{
		int iRes = 1;
		if (!largeDataSetOpt)
		{
			MetaParticle oldBlob;
			iRes = oldBlob.CreatePodMetas(&data[0],(int)data.size(),&mesh,thres,coarseness);
		}
		else
		{
			MetaParticleFast blob;
			iRes = blob.CreatePodMetas(&data[0],(int)data.size(),&mesh,thres,coarseness);
		}

		// An out of memory error is the only reason iRes would be zero in either case
		if( (iRes == 0) && GetCOREInterface() )
			GetCOREInterface()->DisplayTempPrompt(GetString(IDS_NOT_ENOUGH_MEM), 5000 );
	}
	else
	{
		mesh.setNumFaces(0);
		mesh.setNumVerts(0);
	}

	mesh.InvalidateTopologyCache();

	ivalid.Set(t,t);
}

BOOL BlobMesh::OKtoDisplay(TimeValue t) 
{
	//TODO: Check whether all the parameters have valid values and return the state
	return TRUE;
}

void BlobMesh::InvalidateUI() 
{
	// Hey! Update the param blocks
	blobmesh_param_blk.InvalidateUI();
}

Object* BlobMesh::ConvertToType(TimeValue t, Class_ID obtype)
{
	//TODO: If the plugin can convert to a nurbs surface then check to see 
	//    whether obtype == EDITABLE_SURF_CLASS_ID and convert the object
	//    to nurbs surface and return the object

	return SimpleObject2::ConvertToType(t,obtype);
}

int BlobMesh::CanConvertToType(Class_ID obtype)
{
	//TODO: Check for any additional types the plugin supports
	if (obtype==defObjectClassID ||
		obtype==triObjectClassID) {
			return 1;
	} else {    
		return SimpleObject2::CanConvertToType(obtype);
	}
}

// From Object
int BlobMesh::IntersectRay(
	TimeValue t, Ray& ray, float& at, Point3& norm)
{
	//TODO: Return TRUE after you implement this method
	return FALSE;
}

void BlobMesh::GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist)
{
	Object::GetCollapseTypes(clist, nlist);
	//TODO: Append any any other collapse type the plugin supports
}

// From ReferenceTarget
RefTargetHandle BlobMesh::Clone(RemapDir& remap) 
{
	BlobMesh* newob = new BlobMesh();   
	// Make a copy all the data and also clone all the references
	newob->ReplaceReference(0,remap.CloneRef(pblock2));
	newob->ivalid.SetEmpty();
	BaseClone(this, newob, remap);
	return(newob);
}




static INT_PTR CALLBACK AddDlgProc(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BlobMesh *mod = DLGetWindowLongPtr<BlobMesh*>(hWnd);

	switch (msg) {
	case WM_INITDIALOG:
		{
			mod = (BlobMesh*)lParam;
			DLSetWindowLongPtr(hWnd, lParam);

			for (int i=0; i < mod->pfNodes.Count(); i++)
			{
				const TCHAR *name = mod->pfNodes[i]->GetName();

				if (name)
				{
					TCHAR title[200];
					_tcscpy(title,name);
					SendMessage(GetDlgItem(hWnd,IDC_EVENTLIST),
						LB_ADDSTRING,0,(LPARAM)(TCHAR*)title);
				}
			}
			break;
		}

	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{
		case IDOK:
			{

				int listCt = SendMessage(GetDlgItem(hWnd,IDC_EVENTLIST),
					LB_GETCOUNT,0,0);
				int selCt =  SendMessage(GetDlgItem(hWnd,IDC_EVENTLIST),
					LB_GETSELCOUNT ,0,0);
				int *selList;
				selList = new int[selCt];

				SendMessage(GetDlgItem(hWnd,IDC_EVENTLIST),
					LB_GETSELITEMS  ,(WPARAM) selCt,(LPARAM) selList);
				mod->addPFNodes.SetCount(selCt);
				for (int i=0; i < selCt; i++)
				{
					mod->addPFNodes[i] = selList[i];
				}

				delete [] selList;

				EndDialog(hWnd,1);
				break;
			}
		case IDCANCEL:
			mod->pfNodes.ZeroCount();
			EndDialog(hWnd,0);
			break;
		}
		break;


	case WM_CLOSE:
		mod->pfNodes.ZeroCount();
		EndDialog(hWnd, 0);
		break;


	default:
		return FALSE;
	}
	return TRUE;
}

void
	BlobMesh::PickPFEvents(HWND hWnd)
{
	Tab<INode *> pfEvents;
	int numberOfNodes = pblock2->Count(pb_nodelist);

	TimeValue t = GetCOREInterface()->GetTime();


	pfNodes.ZeroCount();
	addPFNodes.ZeroCount();


	for (int i = 0; i < numberOfNodes; i++)
	{
		INode *node;
		pblock2->GetValue(pb_nodelist,t,node,ivalid,i);

		if (node)
		{
			ObjectState tos =  node->EvalWorldState(t,TRUE);

			if (tos.obj->IsParticleSystem())
			{
				IParticleObjectExt* epobj;
				epobj = (IParticleObjectExt*) tos.obj->GetInterface(PARTICLEOBJECTEXT_INTERFACE);

				if (epobj) 
				{
					MyEnumProc dep(false);              
					tos.obj->DoEnumDependents(&dep);
					for (int i = 0; i < dep.Nodes.Count(); i++)
					{
						Interval valid;
						INode *node = dep.Nodes[i];

						Object *obj = node->GetObjectRef();


						if ((obj) && (obj->GetInterface(PARTICLEGROUP_INTERFACE)) )
						{
							pfNodes.Append(1,&node);
						}
					}

				}

			}

		}
	}
	if (pfNodes.Count() > 0)
	{
		int iret = DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_ADD_DIALOG),
			hWnd,AddDlgProc,(LPARAM)this);
		if ((iret) && (addPFNodes.Count() > 0))
		{
			theHold.Begin();
			for (int i = 0; i < addPFNodes.Count(); i++)
			{
				int index = addPFNodes[i];
				INode *node = pfNodes[index];
				pblock2->Append(pb_pfeventlist,1,&node);
			}
			theHold.Accept(GetString(IDS_ADDEVENTS));

		}
	}  

}

