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

#include "../CATControls/CatPlugins.h"
#include "modstack.h"
#include "CATObject Superclass.h"
#include "../CATControls/CATNodeControl.h"

//////////////////////////////////////////////////////////////////////////

// From Object
void CATObject::Init()
{
	flags = 0;
	catmesh = Mesh();
	ip = NULL;
	dwFileSaveVersion = 0;
	node = NULL;
}

Object* CATObject::ConvertToType(TimeValue t, Class_ID obtype)
{
	//TODO: If the plugin can convert to a nurbs surface then check to see
	//		whether obtype == EDITABLE_SURF_CLASS_ID and convert the object
	//		to nurbs surface and return the object

	return SimpleObject2::ConvertToType(t, obtype);
}

int CATObject::CanConvertToType(Class_ID obtype)
{
	//TODO: Check for any additional types the plugin supports
	if (obtype == defObjectClassID ||
		obtype == triObjectClassID) {
		return 1;
	}
	else {
		return SimpleObject2::CanConvertToType(obtype);
	}
}

// From Object
int CATObject::IntersectRay(TimeValue, Ray& ray, float& at, Point3& norm)
{
	return mesh.IntersectRay(ray, at, norm);
}

//From BaseObject
CreateMouseCallBack* CATObject::GetCreateMouseCallBack()
{
	return NULL;
}

void CATObject::SetLengthAxis(int axis) {
	if (axis == X) {
		flags |= CATOBJECTFLAG_LENGTHAXIS_X;
	}
	else {
		flags &= ~CATOBJECTFLAG_LENGTHAXIS_X;
	}
}

void CATObject::BuildMesh(TimeValue)
{
	// Get necessary params
	float catunits = GetCATUnits();
	// Everything is scaled by CATUnits.
	float x = GetX() * (catunits / 2.0f);
	float y = GetY() * (catunits / 2.0f);
	float z = GetZ() * catunits;

	if (flags&CATOBJECTFLAG_LENGTHAXIS_X) {
		float temp = x;
		x = z;
		z = temp;
	}

	if (flags&CATOBJECTFLAG_USECUSTOMMESH) {

		mesh = catmesh;

		Point3 rescale(x, y, z);
		// Scale everything up by CATUnits
		for (int i = 0; i < mesh.getNumVerts(); i++) {
			Point3 p = mesh.getVert(i);
			if (flags&CATOBJECTFLAG_LENGTHAXIS_X) {
				float temp = p.x;
				p.x = p.z;
				p.z = -temp;
			}
			mesh.setVert(i, p * rescale);
		}

		mesh.InvalidateTopologyCache();
	}
	else {
		// Initialise the mesh...
		mesh.setNumVerts(8);
		mesh.setNumFaces(12);
		mesh.InvalidateTopologyCache();

		// Vertices
		mesh.setVert(0, -x, -y, -z);
		mesh.setVert(1, x, -y, -z);
		mesh.setVert(2, -x, y, -z);
		mesh.setVert(3, x, y, -z);
		mesh.setVert(4, -x, -y, z);
		mesh.setVert(5, x, -y, z);
		mesh.setVert(6, -x, y, z);
		mesh.setVert(7, x, y, z);

		// Faces
		MakeFace(mesh.faces[0], 0, 2, 3, 1, 2, 0, 1, 1);
		MakeFace(mesh.faces[1], 3, 1, 0, 1, 2, 0, 1, 1);
		MakeFace(mesh.faces[2], 4, 5, 7, 1, 2, 0, 2, 0);
		MakeFace(mesh.faces[3], 7, 6, 4, 1, 2, 0, 2, 0);
		MakeFace(mesh.faces[4], 0, 1, 5, 1, 2, 0, 3, 4);
		MakeFace(mesh.faces[5], 5, 4, 0, 1, 2, 0, 3, 4);
		MakeFace(mesh.faces[6], 1, 3, 7, 1, 2, 0, 4, 3);
		MakeFace(mesh.faces[7], 7, 5, 1, 1, 2, 0, 4, 3);
		MakeFace(mesh.faces[8], 3, 2, 6, 1, 2, 0, 5, 5);
		MakeFace(mesh.faces[9], 6, 7, 3, 1, 2, 0, 5, 5);
		MakeFace(mesh.faces[10], 2, 0, 4, 1, 2, 0, 6, 2);
		MakeFace(mesh.faces[11], 4, 6, 2, 1, 2, 0, 6, 2);

	}

	mesh.InvalidateTopologyCache();

	ivalid.SetInfinite();
}

void CATObject::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	this->ip = ip;

	Control* ctrl = GetTransformController();
	if (ctrl) ctrl->BeginEditParams(ip, flags, prev);
}

void CATObject::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	Control* ctrl = GetTransformController();
	if (ctrl) ctrl->EndEditParams(ip, flags, next);

	this->ip = NULL;
}

//////////////////////////////////////////////////////////////////////////
#define CATOBJECT_NUMVERTICIES_CHUNK		1<<1
#define CATOBJECT_NUMFACES_CHUNK			1<<2
#define CATOBJECT_VERTICIES_CHUNK			1<<3
#define CATOBJECT_FACES_CHUNK				1<<4
#define CATOBJECT_FLAGS						1<<5
#define CATOBJECT_VERSION					1<<6

IOResult CATObject::Save(ISave *isave)
{
	DWORD nb;
	int i;

	// Stores the current version.
	isave->BeginChunk(CATOBJECT_VERSION);
	isave->Write(&dwFileSaveVersion, sizeof(DWORD), &nb);
	isave->EndChunk();

	// Stores the number of verticies.
	isave->BeginChunk(CATOBJECT_NUMVERTICIES_CHUNK);
	isave->Write(&catmesh.numVerts, sizeof(int), &nb);
	isave->EndChunk();

	// Stores the number of faces.
	isave->BeginChunk(CATOBJECT_NUMFACES_CHUNK);
	isave->Write(&catmesh.numFaces, sizeof(int), &nb);
	isave->EndChunk();

	// Stores the verticies.
	isave->BeginChunk(CATOBJECT_VERTICIES_CHUNK);
	for (i = 0; i < catmesh.getNumVerts(); i++)
		isave->Write(&catmesh.getVert(i), sizeof(Point3), &nb);
	isave->EndChunk();

	// Stores the faces.
	isave->BeginChunk(CATOBJECT_FACES_CHUNK);
	for (i = 0; i < catmesh.getNumFaces(); i++)
		isave->Write(&catmesh.faces[i], sizeof(Face), &nb);
	isave->EndChunk();

	// Stores the number of faces.
	isave->BeginChunk(CATOBJECT_FLAGS);
	isave->Write(&flags, sizeof(DWORD), &nb);
	isave->EndChunk();

	return IO_OK;
}

class CATObjectPostLoadCallback : public PostLoadCallback {
public:
	CATObject *pobj;

	CATObjectPostLoadCallback(CATObject *p) { pobj = p; }
	void proc(ILoad *) {
		pobj->dwFileSaveVersion = CAT_VERSION_CURRENT;
	}
};

IOResult CATObject::Load(ILoad *iload)
{
	IOResult res = IO_OK;
	DWORD nb;

	catmesh = Mesh();
	int nNumFaces = 0;
	int nNumVerticies = 0;

	// Default file save version to the version just prior to
	// when it was implemented.

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case CATOBJECT_NUMVERTICIES_CHUNK:
			res = iload->Read(&nNumVerticies, sizeof DWORD, &nb);
			catmesh.setNumVerts(nNumVerticies);
			break;
		case CATOBJECT_NUMFACES_CHUNK:
			res = iload->Read(&nNumFaces, sizeof DWORD, &nb);
			catmesh.setNumFaces(nNumFaces);
			break;
		case CATOBJECT_VERTICIES_CHUNK:
			for (int i = 0; i < catmesh.getNumVerts(); i++)
				res = iload->Read(&catmesh.verts[i], sizeof Point3, &nb);
			break;
		case CATOBJECT_FACES_CHUNK:
			for (int i = 0; i < catmesh.getNumFaces(); i++)
				res = iload->Read(&catmesh.faces[i], sizeof Face, &nb);
			break;
		case CATOBJECT_FLAGS:
			res = iload->Read(&flags, sizeof DWORD, &nb);
			break;
		case CATOBJECT_VERSION:
			res = iload->Read(&dwFileSaveVersion, sizeof DWORD, &nb);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}
	iload->RegisterPostLoadCallback(new CATObjectPostLoadCallback(this));
	return IO_OK;
}

BOOL CATObject::SaveMesh(CATRigWriter *save)
{
	if (catmesh.numFaces > 0 && UsingCustomMesh())
	{
		int i;
		save->BeginGroup(idMeshParams);

		//		save->BeginGroup(idVerticies);
		int numVerts = catmesh.getNumVerts();
		save->Write(idNumVerticies, numVerts);
		for (i = 0; i < numVerts; i++) {
			Point3 vert = catmesh.getVert(i);
			save->Write(idVertex, vert);
		}
		// verticies
//		save->EndGroup();

//		save->BeginGroup(idMeshFaces);
		int numFaces = catmesh.getNumFaces();
		save->Write(idNumFaces, numFaces);
		for (i = 0; i < numFaces; i++) {
			Face face = catmesh.faces[i];
			save->Write(idValFace, face);
		}
		// faces
//		save->EndGroup();

		// mesh
		save->EndGroup();
	}
	return TRUE;
}

BOOL CATObject::LoadMesh(CATRigReader *load)
{
	BOOL done = FALSE;
	BOOL ok = TRUE;

	catmesh = Mesh();
	Point3 vert;
	Face face;
	int numVerts = 0;
	int numFaces = 0;
	int currVert = 0;
	int currFace = 0;

	while (load->ok() && !done && ok)
	{
		if (!load->NextClause())
		{
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if (load->CurGroupID() != idMeshParams) return FALSE;
		}
		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID()) {
		case rigAssignment:
			switch (load->CurIdentifier())
			{
			case idNumVerticies:
				if (load->GetValue(numVerts))
					catmesh.setNumVerts(numVerts);
				break;
			case idNumFaces:
				if (load->GetValue(numFaces))
					catmesh.setNumFaces(numFaces);
				break;
			case idVertex:
				if (load->GetValue(vert)) {
					catmesh.setVert(currVert, vert);
					currVert++;
				}
				break;
			case idValFace:
				if (load->GetValue(face)) {
					catmesh.faces[currFace] = face;
					currFace++;
				}
				break;
			default:
				load->AssertOutOfPlace();
			}
			break;
		case rigEndGroup:

			// Start using the modified geometry
			flags = flags | CATOBJECTFLAG_CUSTOMMESHAVAIL;
			flags = flags | CATOBJECTFLAG_USECUSTOMMESH;
			mesh.InvalidateTopologyCache();
			NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
			done = TRUE;
			break;
		case rigAbort:
		case rigEnd:
			return FALSE;
		}
	}
	return ok && load->ok();
}

BOOL CATObject::SaveRig(CATRigWriter *save)
{
	save->BeginGroup(idObjectParams);

	save->Write(idFlags, flags);
	float x = GetX();	save->Write(idWidth, x);
	float y = GetY();	save->Write(idHeight, y);
	float z = GetZ();	save->Write(idLength, z);

	if (GetTransformController() && node) {
		Point3 objOffsetPos = node->GetObjOffsetPos();
		save->Write(idObjectOffsetPos, objOffsetPos);
		Quat objOffsetRot = node->GetObjOffsetRot();
		save->Write(idObjectOffsetRot, objOffsetRot);
		Point3 objOffsetScl = node->GetObjOffsetScale().s;
		save->Write(idObjectOffsetScl, objOffsetScl);
	}

	save->SaveMesh(catmesh);

	//	save->WriteCAs(this);

	// Object
	save->EndGroup();
	return TRUE;
}
BOOL CATObject::LoadRig(CATRigReader *load)
{
	BOOL done = FALSE;
	BOOL ok = TRUE;
	float val;
	if (!node)GetTransformController();
	DbgAssert(node);
	Point3	objOffsetPos;
	Quat	objOffsetRot;
	Point3	objOffsetScl;

	while (load->ok() && !done && ok)
	{
		if (!load->NextClause())
		{
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if (load->CurGroupID() != idObjectParams) return FALSE;
		}

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID())
		{
		case rigBeginGroup:
			switch (load->CurIdentifier()) {
			case idMeshParams:
				load->ReadMesh(catmesh);
				Update();
				break;
			default:
				load->AssertOutOfPlace();
			}
			break;

		case rigAssignment:
			switch (load->CurIdentifier())
			{
			case idFlags:			load->GetValue(flags);						break;
			case idWidth:			load->GetValue(val);			SetX(val);	break;
			case idHeight:			load->GetValue(val);			SetY(val);	break;
			case idLength:			load->GetValue(val);			SetZ(val);	break;
			case idObjectOffsetPos:	load->GetValue(objOffsetPos);	node->SetObjOffsetPos(objOffsetPos);				break;
			case idObjectOffsetRot:	load->GetValue(objOffsetRot);	node->SetObjOffsetRot(objOffsetRot);				break;
			case idObjectOffsetScl:	load->GetValue(objOffsetScl);	node->SetObjOffsetScale(ScaleValue(objOffsetScl));	break;
			default:				load->AssertOutOfPlace();
			}
			break;
		case rigAbort:
		case rigEnd:
			ok = FALSE;
		case rigEndGroup:
			done = TRUE;
			break;
		}
	}
	return ok && load->ok();
}

class CATObjectRestore : public RestoreObj {
public:
	CATObject		*catobject;
	Mesh			catmesh;
	DWORD			flags;

	CATObjectRestore(CATObject *o) {
		catobject = o;
		catmesh = o->catmesh;
		flags = o->flags;
	}

	void Restore(int isUndo) {
		if (isUndo) {
			Mesh tempmesh = catobject->catmesh;
			DWORD tempflags = catobject->flags;
			catobject->catmesh = catmesh;
			catobject->flags = flags;
			catmesh = tempmesh;
			flags = tempflags;
			catobject->Update();
		}
	}

	void Redo() {
		Mesh tempmesh = catobject->catmesh;
		DWORD tempflags = catobject->flags;
		catobject->catmesh = catmesh;
		catobject->flags = flags;
		catmesh = tempmesh;
		flags = tempflags;
		catobject->Update();
	}

	int Size() { return sizeof(catmesh) + 2; }
	void EndHold() { catobject->ClearAFlag(A_HELD); }
};

BOOL CATObject::CopyMeshFromNode(INode* node)
{
	//////////////////////////////////////////////////////////////////////////
	// Get the top of the current modifier stack
	if (!node) return FALSE;

	Object *wsobj = node->EvalWorldState(0).obj;
	if (wsobj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0))) {

		if (theHold.Holding())	theHold.Put(new CATObjectRestore(this));

		HoldSuspend hs;

		TriObject *triObject = (TriObject *)wsobj->ConvertToType(0, Class_ID(TRIOBJ_CLASS_ID, 0));
		catmesh = Mesh(triObject->GetMesh());
		triObject->DeleteThis();

		if (GetTransformController() && GetTransformController()->GetInterface(I_CATNODECONTROL)) {
			CATNodeControl *ctrl = (CATNodeControl*)GetTransformController()->GetInterface(I_CATNODECONTROL);
			INode* catnode = ctrl->GetNode();
			TimeValue t = GetCOREInterface()->GetTime();
			Matrix3 tmOffset = node->GetObjectTM(t) * Inverse(catnode->GetObjectTM(t));
			if (ctrl->GetCATParentTrans()->GetLengthAxis() == X)
				tmOffset.RotateY((float)-M_PI_2);
			// Transform the mesh
			for (int i = 0; i < catmesh.getNumVerts(); i++)
				catmesh.verts[i] = catmesh.verts[i] * tmOffset;
		}

		// Get necessary params
		float catunits = GetCATUnits();
		// Everything is scaled by CATUnits.
		float x = GetX() * (catunits / 2.0f);
		float y = GetY() * (catunits / 2.0f);
		float z = GetZ() * catunits;

		Point3 rescale(x, y, z);

		// Scale everything up by CATUnits
		for (int i = 0; i < catmesh.getNumVerts(); i++)
			catmesh.verts[i] = (catmesh.verts[i] / rescale);

		// Start using the modified geometry
		flags |= CATOBJECTFLAG_CUSTOMMESHAVAIL;
		flags |= CATOBJECTFLAG_USECUSTOMMESH;
		Update();
		node->EvalWorldState(0);
	}
	return TRUE;
}

BOOL CATObject::PasteRig(ICATObject* pasteobj, DWORD flags, float scalefactor)
{
	SetX(pasteobj->GetX() * scalefactor);
	SetY(pasteobj->GetY() * scalefactor);
	SetZ(pasteobj->GetZ() * scalefactor);
	CATObject* pastecatobj = (CATObject*)pasteobj->AsObject();

	int i;
	if (pastecatobj->flags&CATOBJECTFLAG_USECUSTOMMESH && ~(flags&PASTERIGFLAG_DONT_PASTE_MESHES))
	{
		catmesh = pastecatobj->catmesh;
		this->flags |= CATOBJECTFLAG_CUSTOMMESHAVAIL;
		this->flags |= CATOBJECTFLAG_USECUSTOMMESH;

		if (flags&PASTERIGFLAG_MIRROR) {
			for (i = 0; i < catmesh.numVerts; i++)
				catmesh.verts[i].x *= -1.0f;

			// now we are insideout so flip all the normals
			for (i = 0; i < catmesh.numFaces; i++)
				catmesh.FlipNormal(i);
		}
	}
	// Now paste the actual flags
	flags = pastecatobj->flags;

	Update();
	return TRUE;
}

Control* CATObject::GetTransformController()
{
	if (!node) {
		// Find the node using the handlemessage system
		// [ST 04-30-08] We can't assume that this will work,
		// it is possible to be called before our node is created.
		node = FindReferencingClass<INode>(this);
	}
	if (node)	return node->GetTMController();
	else		return NULL;
}

// If someone has just tried to collapse the stack on our object, we stop them here
// instead of dieing peacefully, we take control, grab the mesh of the node that is
//  being collapsed, and put ourselves back in as the object.
void CATObject::NotifyPostCollapse(INode *node, Object *obj, IDerivedObject *derObj, int index)
{
	UNREFERENCED_PARAMETER(obj); UNREFERENCED_PARAMETER(derObj); UNREFERENCED_PARAMETER(index);
	DbgAssert(node);
	// grab the mesh off the node;
	CopyMeshFromNode(node);
	// make ourselves the object again
	node->SetObjectRef(this);
	Update();
	node->EvalWorldState(0);
}

void CATObject::DisplayObject(TimeValue t, INode* inode, ViewExp *vpt, int flags, Color color, Box3 &bbox) {
	UNREFERENCED_PARAMETER(flags);
	GraphicsWindow *gw = vpt->getGW();
	Material *mtl = gw->getMaterial();
	Matrix3 m = inode->GetObjectTM(t);
	gw->setTransform(m);
	DWORD rlim = gw->getRndLimits();
	gw->setRndLimits(GW_WIREFRAME | GW_EDGES_ONLY | GW_BACKCULL);

	gw->setColor(LINE_COLOR, color);

	mesh.render(gw, mtl, NULL, COMP_ALL);
	gw->setRndLimits(rlim);

	// Now get the World Bounding Box
	Box3 w_bbox;
	GetWorldBoundBox(t, inode, vpt, w_bbox);
	bbox += w_bbox;
}

/**********************************************************************
 * Function publishing interface descriptor...
 */
static FPInterfaceDesc catobject_FPinterface(
	I_CATOBJECT, _T("CATObjectFPInterface"), 0, NULL, FP_MIXIN,

	CATObject::fnCopyMeshFromNode, _T("CopyMeshFromNode"), 0, TYPE_VOID, 0, 1,
		_T("node"), 0, TYPE_INODE,
	CATObject::fnPasteRig, _T("PasteRig"), 0, TYPE_VOID, 0, 2,
		_T("pasteobject"), 0, TYPE_OBJECT,
		_T("mirrordata"), 0, TYPE_BOOL,

	properties,

	CATObject::propGetTMController, FP_NO_FUNCTION, _T("TMController"), 0, TYPE_CONTROL,
	CATObject::propGetUseCustomMesh, CATObject::propSetUseCustomMesh, _T("UseCustomMesh"), 0, TYPE_BOOL,

	p_end
);

FPInterfaceDesc* CATObject::GetDescByID(Interface_ID id) {
	if (id == I_CATOBJECT) return &catobject_FPinterface;
	return FPMixinInterface::GetDescByID(id);
}
