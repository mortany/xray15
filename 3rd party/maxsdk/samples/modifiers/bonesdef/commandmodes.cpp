/**********************************************************************

FILE: CommandModes.cpp

DESCRIPTION:  Bones def varius command modes
Plus scripter access

CREATED BY: Peter Watje

HISTORY: 8/5/98

*>	Copyright (c) 1998, All Rights Reserved.
**********************************************************************/

#include "mods.h"
#include "iparamm.h"
#include "shape.h"
#include "spline3d.h"
#include "splshape.h"
#include "linshape.h"
#include "iiksys.h"
#include "modstack.h"

// This uses the linked-list class templates
#include "linklist.h"
#include "bonesdef.h"
#include "macrorec.h"
#include "geombind.h"

#include <maxscript/maxscript.h>
#include <maxscript/foundation/strings.h>
#include <maxscript/foundation/arrays.h>
#include <maxscript/foundation/3dmath.h>
#include <maxscript/foundation/numbers.h>
#include <maxscript/maxwrapper/pathname.h>

#include "BMD_Command.h"
#include "MouseCursors.h"
#include "3dsmaxport.h"
#include "ObjectWrapper.h"

// STL headers
#include <set>

#define SAFE_DELETE(x) if( (x)!=NULL ) { delete (x); (x)=NULL; }

/* --------------------  local generics and names   --------------- */

#include <maxscript\macros\local_class_instantiations.h>

def_name ( threshhold );

/* --------------------  local function   ------------------------- */

#include <maxscript\macros\define_instantiation_functions.h>

def_struct_primitive( bakeSelectedVerts,skinOps, "bakeSelectedVerts" );

def_struct_primitive_debug_ok ( isRigidHandle,skinOps, "isRigidHandle" );
def_struct_primitive( rigidHandle,skinOps, "rigidHandle" );

def_struct_primitive( Invalidate,skinOps, "Invalidate" );

def_struct_primitive_debug_ok ( isRigidVertex,skinOps, "isRigidVertex" );
def_struct_primitive( rigidVertex,skinOps, "rigidVertex" );

def_struct_primitive_debug_ok ( isUnNormalizeVertex,skinOps, "isUnNormalizeVertex" );
def_struct_primitive( unNormalizeVertex,skinOps, "unNormalizeVertex" );

//reset the current gizmos rotation plane
//skinops.GizmoResetRotationPlane $.modifiers[#Skin]
def_struct_primitive( gizmoResetRotationPlane,skinOps, "gizmoResetRotationPlane" );

//add gizmo
//skinops.buttonAddGizmo $.modifiers[#Skin]
def_struct_primitive( buttonAddGizmo,skinOps, "buttonAddGizmo" );
//remove gizmo
//skinops.buttonRemoveGizmo $.modifiers[#Skin]
def_struct_primitive( buttonRemoveGizmo,skinOps, "buttonRemoveGizmo" );
//CopyGizmo
//skinops.buttonCopyGizmo $.modifiers[#Skin]
def_struct_primitive( buttonCopyGizmo,skinOps, "buttonCopyGizmo" );
//PasteGizmo
//skinops.buttonPasteGizmo $.modifiers[#Skin]
def_struct_primitive( buttonPasteGizmo,skinOps, "buttonPasteGizmo" );

//setCurrentSelectGizmo
//skinops.selectGizmo $.modifiers[#Skin] selectedGizmo
def_struct_primitive( selectGizmo,skinOps, "selectGizmo" );
//getSelectedGizmo
//skinops.getSelectedGizmo $.modifiers[#Skin]
def_struct_primitive_debug_ok ( getSelectedGizmo,skinOps, "getSelectedGizmo" );
//getNumberOfGizmos
//skinops.getNumberOfGizmos $.modifiers[#Skin]
def_struct_primitive_debug_ok ( getNumberOfGizmos,skinOps, "getNumberOfGizmos" );

//enableGizmo
//skinops.enableGizmo $.modifiers[#Skin] gizmoid enable
def_struct_primitive( enableGizmo,skinOps, "enableGizmo" );

//setCurrentSelectGizmoType
//skinops.selectGizmoType $.modifiers[#Skin] selectedGizmoType
def_struct_primitive( selectGizmoType,skinOps, "selectGizmoType" );
//getCurrentSelectGizmoType
//skinops.getCurrentSelectGizmoType $.modifiers[#Skin] getSelectedGizmoType
def_struct_primitive_debug_ok ( getSelectedGizmoType,skinOps, "getSelectedGizmoType" );
//getNumberOfGizmoTypes
//skinops.getNumberOfGizmoTypes $.modifiers[#Skin]
def_struct_primitive_debug_ok ( getNumberOfGizmoTypes,skinOps, "getNumberOfGizmoTypes" );

// Maxscript stuff
// need to doc

//skinops.buttonExclude $.modifiers[#Skin]
def_struct_primitive( saveEnvelope,skinOps, "saveEnvelope" );
def_struct_primitive( SaveEnvelopeAsASCII,skinOps, "SaveEnvelopeAsASCII" );

//skinops.buttonExclude $.modifiers[#Skin]
def_struct_primitive( loadEnvelope,skinOps, "loadEnvelope" );
def_struct_primitive( loadEnvelopeAsASCII,skinOps, "loadEnvelopeAsASCII" );

//skinops.buttonExclude $.modifiers[#Skin]
def_struct_primitive( buttonExclude,skinOps, "buttonExclude" );
//skinops.buttonInclude $.modifiers[#Skin]
def_struct_primitive( buttonInclude,skinOps, "buttonInclude" );
//skinops.buttonSelectExclude $.modifiers[#Skin]
def_struct_primitive( buttonSelectExcluded,skinOps, "buttonSelectExcluded" );

//skinops.buttonPaint $.modifiers[#Skin]
def_struct_primitive( buttonPaint,skinOps, "buttonPaint" );

//skinops.buttonAdd $.modifiers[#Skin]
def_struct_primitive( buttonAdd,skinOps, "buttonAdd" );
//skinops.buttonRemove $.modifiers[#Skin]
def_struct_primitive( buttonRemove,skinOps, "buttonRemove" );

//skinops.buttonAddCrossSection $.modifiers[#Skin]
def_struct_primitive( buttonAddCrossSection,skinOps, "buttonAddCrossSection" );
//skinops.buttonRemoveCrossSection $.modifiers[#Skin]
def_struct_primitive( buttonRemoveCrossSection,skinOps, "buttonRemoveCrossSection" );

//skinops.selectEndPoint $.modifiers[#Skin]
def_struct_primitive( selectEndPoint,skinOps, "selectEndPoint" );
//skinops.selectStartPoint $.modifiers[#Skin]
def_struct_primitive( selectStartPoint,skinOps, "selectStartPoint" );

//skinops.selectCrossSection $.modifiers[#Skin] crossid inner
def_struct_primitive( selectCrossSection,skinOps, "selectCrossSection" );

//skinops.copySelectedBone $.modifiers[#Skin]
def_struct_primitive( copySelectedBone,skinOps, "copySelectedBone" );
//skinops.pasteToSelectedBone $.modifiers[#Skin]
def_struct_primitive( pasteToSelectedBone,skinOps, "pasteToSelectedBone" );
//skinops.pasteToAllBones $.modifiers[#Skin]
def_struct_primitive( pasteToAllBones,skinOps, "pasteToAllBones" );
//skinops.pasteToBone $.modifiers[#Skin] boneID
def_struct_primitive( pasteToBone,skinOps, "pasteToBone" );

//skinops.setSelectedBonePropRelative $.modifiers[#Skin] Relative
def_struct_primitive( setSelectedBonePropRelative,skinOps, "setSelectedBonePropRelative" );
//skinops.getSelectedBonePropRelative $.modifiers[#Skin]
def_struct_primitive_debug_ok ( getSelectedBonePropRelative,skinOps, "getSelectedBonePropRelative" );

//skinops.setSelectedBonePropEnvelopeVisible $.modifiers[#Skin] visible
def_struct_primitive( setSelectedBonePropEnvelopeVisible,skinOps, "setSelectedBonePropEnvelopeVisible" );
//skinops.getSelectedBonePropEnvelopeVisible $.modifiers[#Skin]
def_struct_primitive_debug_ok ( getSelectedBonePropEnvelopeVisible,skinOps, "getSelectedBonePropEnvelopeVisible" );

//skinops.setSelectedBonePropFalloff $.modifiers[#Skin] falloff
def_struct_primitive( setSelectedBonePropFalloff,skinOps, "setSelectedBonePropFalloff" );
//skinops.getSelectedBonePropEnvelopeVisible $.modifiers[#Skin]
def_struct_primitive_debug_ok ( getSelectedBonePropFalloff,skinOps, "getSelectedBonePropFalloff" );

//skinops.setBonePropRelative $.modifiers[#Skin] BoneID Relative
def_struct_primitive( setBonePropRelative,skinOps, "setBonePropRelative" );
//skinops.getBonePropRelative $.modifiers[#Skin] BoneID
def_struct_primitive_debug_ok ( getBonePropRelative,skinOps, "getBonePropRelative" );

//skinops.setBonePropEnvelopeVisible $.modifiers[#Skin] BoneID visible
def_struct_primitive( setBonePropEnvelopeVisible,skinOps, "setBonePropEnvelopeVisible" );
//skinops.getBonePropEnvelopeVisible $.modifiers[#Skin] BoneID
def_struct_primitive_debug_ok ( getBonePropEnvelopeVisible,skinOps, "getBonePropEnvelopeVisible" );

//skinops.setBonePropFalloff $.modifiers[#Skin] BoneID falloff
def_struct_primitive( setBonePropFalloff,skinOps, "setBonePropFalloff" );
//skinops.getBonePropEnvelopeVisible $.modifiers[#Skin] BoneID
def_struct_primitive_debug_ok ( getBonePropFalloff,skinOps, "getBonePropFalloff" );

//skinops.resetSelectedVerts $.modifiers[#Skin]
def_struct_primitive( resetSelectedVerts,skinOps, "resetSelectedVerts" );

//skinops.resetSelectedBone $.modifiers[#Skin]
def_struct_primitive( resetSelectedBone,skinOps, "resetSelectedBone" );
//skinops.resetAllBone $.modifiers[#Skin]
def_struct_primitive( resetAllBones,skinOps, "resetAllBones" );

//skinops.isBoneSelected $.modifiers[#Skin] index
def_struct_primitive_debug_ok ( isBoneSelected,skinOps, "isBoneSelected" );

//skinops.removebone $.modifiers[#Skin]
//skinops.removebone $.modifiers[#Skin] index
def_struct_primitive( removeBone,skinOps, "RemoveBone" );

//skinops.removeUnusedBones $.modifiers[#Skin] threshhold:<arg>
def_struct_primitive( removeUnusedBones,skinOps, "RemoveUnusedBones" );

//skinops.addbone $.modifiers[#Skin] Node Update
def_struct_primitive( addBone,skinOps, "AddBone" );

//skinops.replacebone $.modifiers[#Skin] boneID node vertices:<arg>
def_struct_primitive( replaceBone,skinOps, "ReplaceBone" );

//skinops.addCrossSection $.modifiers[#Skin] BoneID U InnerRadius OuterRadius
//skinops.addCrossSection $.modifiers[#Skin] U InnerRadius OuterRadius
//skinops.addCrossSection $.modifiers[#Skin] U
def_struct_primitive( addCrossSection,skinOps, "AddCrossSection" );

//skinops.RemoveCrossSection $.modifiers[#Skin]
//skinops.RemoveCrossSection $.modifiers[#Skin] BoneID CrossSectionID
def_struct_primitive( removeCrossSection,skinOps, "RemoveCrossSection" );

//skinops.GetCrossSectionU $.modifiers[#Skin] BoneID CrossSectionID
def_struct_primitive_debug_ok ( getCrossSectionU,skinOps, "GetCrossSectionU" );
//skinops.SetCrossSectionU $.modifiers[#Skin] BoneID CrossSectionID UValue
def_struct_primitive( setCrossSectionU,skinOps, "SetCrossSectionU" );

//skinops.GetEndPoint $.modifiers[#Skin] BoneID
def_struct_primitive_debug_ok ( getEndPoint,skinOps, "GetEndPoint" );
//skinops.SetEndPoint $.modifiers[#Skin] BoneID [float,float,float]
def_struct_primitive( setEndPoint,skinOps, "SetEndPoint" );
def_struct_primitive_debug_ok ( getStartPoint,skinOps, "GetStartPoint" );
def_struct_primitive( setStartPoint,skinOps, "SetStartPoint" );

//just returns the number of bones in the system
def_struct_primitive_debug_ok ( getNumberBones,skinOps, "GetNumberBones" );

//just returns the number of vertice in the system
def_struct_primitive_debug_ok ( getNumberVertices,skinOps,  "GetNumberVertices" );
//skinops.GetVertexWeightCount vertexid
//returns the number of bones influencing that vertex
def_struct_primitive_debug_ok ( getVertexWeightCount, skinOps, "GetVertexWeightCount" );
//skinops.GetBoneNodes
//returns an array of bone nodes in the same order they exist in the skin modifier
def_struct_primitive_debug_ok ( getBoneNodes, skinOps,       "GetBoneNodes" );
//skinops.GetBoneNode boneID
//just returns the node of the bone
def_struct_primitive_debug_ok ( getBoneNode, skinOps,       "GetBoneNode" );
//skinops.GetBoneName boneID
//just returns the name of the bone
def_struct_primitive_debug_ok ( getBoneName, skinOps,       "GetBoneName" );

//skinops.GetBoneIDByListID listID
// returns the ID of the bone corresponding to the list box id
def_struct_primitive_debug_ok( getBoneIDByListID, skinOps, "GetBoneIDByListID");

//skinops.GetListIDByBoneID boneID
// returns the list box id corresponding to the ID of the bone
def_struct_primitive_debug_ok( getListIDByBoneID, skinOps, "GetListIDByBoneID");

//skinops.GetBoneWeight boneID nthvertex 
//returns the influence of the nth vertex affected by that bone
def_struct_primitive_debug_ok ( getBoneWeight,skinOps,    "GetBoneWeight" );
//skinops.GetBoneWeights boneID
//returns an array of weights indicating influence of that bone for all vertices
def_struct_primitive_debug_ok ( getBoneWeights,skinOps, "GetBoneWeights" );
//skinops.SetBoneWeights boneID weights
//assigns all vertex influence weights for a BoneID
//erase all previous weight info for the bone
//BoneID and Weights can be a number and array of floats,
//or array of numbers and array-of-arrays of floats, one float entry per vertex
def_struct_primitive_debug_ok ( setBoneWeights,skinOps, "SetBoneWeights" );

//skinops.Hammer number/array/bitarray
//perform vertex weight "hammer" operation on the vertices specified
//this resets the bone vertex weightings to an average of all adjacent vertices
def_struct_primitive_debug_ok ( hammer,skinOps, "Hammer" );

//skinops.GetVertexWeight vertexid nthbone
//returns the inlfuence of the nth bone affecting that vertex
def_struct_primitive_debug_ok ( getVertexWeight,skinOps,    "GetVertexWeight" );
//skinops.GetVertexWeightBoneID vertexid nthbone
//returns the bone id of the nth bone affecting that vertex
def_struct_primitive_debug_ok ( getVertexWeightBoneID,skinOps, "GetVertexWeightBoneID" );

//skinops.SelectVertices number/array/bitarray
//selects the vertices specified
def_struct_primitive (selectVertices, skinOps,			"SelectVertices" );
//GetSelectedVertices
//get the current selected vertices
def_struct_primitive_debug_ok ( getSelectedVertices, skinOps,     "GetSelectedVertices" );

//skinSetVertexWeights VertexID BoneID Weights
//assigns vertex to BoneID with Weight n
//it does not erase any previous weight info
//BoneID and Weights can be arrays or just numbers but if they are arrays they need to be the same length
def_struct_primitive (setVertexWeights,skinOps,				"SetVertexWeights" );
//skinReplaceVertexWeights VertexID BoneID Weights
//assigns vertex to BoneID with Weight n
//it erases any previous bone weight info that vertex before assignment
//BoneID and Weights can be arrays or just numbers but if they are arrays they need to be the same length
def_struct_primitive (replaceVertexWeights,skinOps,			"ReplaceVertexWeights" );

//skinIsVertexModified vertID
//just returns if the vertex has been modified
def_struct_primitive_debug_ok (isVertexModified, skinOps,         "IsVertexModified" );

//skinSelectBone BoneID
//selects that bone
def_struct_primitive (selectBone, skinOps,			"SelectBone" );
//GetSelectedBone
//get the current selected bone
def_struct_primitive_debug_ok ( getSelectedBone, skinOps,         "GetSelectedBone" );

//getNumberCrossSections boneID
//returns the number of cross sections for that bone
def_struct_primitive_debug_ok ( getNumberCrossSections, skinOps,        "GetNumberCrossSections" );

//getinnerradius boneid crossSectionID
//returns the inner crossscetion radius
def_struct_primitive_debug_ok ( getInnerRadius, skinOps,       "GetInnerRadius" );
//getOuterRadius boneid crossSectionID
//returns the inner crossscetion radius
def_struct_primitive_debug_ok ( getOuterRadius, skinOps,       "GetOuterRadius" );

//setinnerradius boneid crossSectionID radius
//sets the inner radius of a cross section
def_struct_primitive (setInnerRadius, skinOps,			"SetInnerRadius" );
//setOuterRadius boneid crossSectionID radius
//sets the outer radius of a cross section
def_struct_primitive (setOuterRadius, skinOps,			"SetOuterRadius" );

//IsVertexSelected vertID
//just returns if the vertex has been selected
def_struct_primitive_debug_ok ( isVertexSelected, skinOps,     "IsVertexSelected" );
//takes a bool which determines if all viewports are zoomed
def_struct_primitive (zoomToBone, skinOps,		"ZoomToBone" );
def_struct_primitive (zoomToGizmo, skinOps,		"ZoomToGizmo" );

def_struct_primitive (selectNextBone, skinOps,		"selectNextBone" );
def_struct_primitive (selectPreviousBone, skinOps,		"selectPreviousBone" );

def_struct_primitive (addBoneFromViewStart, skinOps,		"addBoneFromViewStart" );
def_struct_primitive (addBoneFromViewEnd, skinOps,		"addBoneFromViewEnd" );

def_struct_primitive (multiRemove, skinOps,		"multiRemove" );

def_struct_primitive (paintWeights, skinOps,		"paintWeightsButton" );
def_struct_primitive (paintOptions, skinOps,		"paintOptionsButton" );

//MIRROR
def_struct_primitive( mirrorPaste,skinOps, "mirrorPaste" );
def_struct_primitive( mirrorPasteBone,skinOps, "mirrorPasteBone" );
def_struct_primitive( selectMirrorBones,skinOps, "selectMirrorBones" );
def_struct_primitive( setMirrorTM,skinOps, "setMirrorTM" );
def_struct_primitive( updateMirror,skinOps, "updateMirror" );

def_struct_primitive( pasteAllBones,skinOps, "pasteAllBones" );
def_struct_primitive( pasteAllVerts,skinOps, "pasteAllVerts" );

def_struct_primitive( blurSelected,skinOps, "blendSelected" );

def_struct_primitive( setSelectedCrossSection,skinOps, "SetSelectedCrossSection" );
def_struct_primitive_debug_ok ( getSelectedCrossSectionIndex, skinOps,  "GetSelectedCrossSectionIndex" );
def_struct_primitive_debug_ok ( getSelectedCrossSectionIsInner, skinOps,   "GetSelectedCrossSectionIsInner" );

def_struct_primitive (removeZeroWeights, skinOps,	"RemoveZeroWeights" );

def_struct_primitive (selectChild, skinOps,	"selectChild" );
def_struct_primitive (selectParent, skinOps,	"selectParent" );

def_struct_primitive (selectNextSibling, skinOps,	"selectNextSibling" );
def_struct_primitive (selectPreviousSibling, skinOps,	"selectPreviousSibling" );
def_struct_primitive (weightTool, skinOps,	"weightTool" );

def_struct_primitive (setWeight, skinOps,	"setWeight" );
def_struct_primitive (addWeight, skinOps,	"addWeight" );
def_struct_primitive (scaleWeight, skinOps,	"scaleWeight" );
def_struct_primitive (copyWeight, skinOps,	"copyWeights" );
def_struct_primitive (pasteWeight, skinOps,	"pasteWeights" );
def_struct_primitive (pasteWeightByPos, skinOps,	"pasteWeightsByPos" );

def_struct_primitive (selectBoneByNode, skinOps,	"selectBoneByNode" );
def_struct_primitive_debug_ok ( isWeightToolOpen, skinOps,  "isWeightToolOpen" );
def_struct_primitive (closeWeightTool, skinOps,	"closeWeightTool" );

def_struct_primitive (weightTable, skinOps,	"WeightTable" );
def_struct_primitive_debug_ok ( isWeightTableOpen, skinOps, "isWeightTableOpen" );
def_struct_primitive (closeWeightTable, skinOps,	"closeWeightTable" );

def_struct_primitive       ( growSelection, skinOps,  "growSelection" );
def_struct_primitive       ( shrinkSelection, skinOps,   "shrinkSelection" );
def_struct_primitive       ( loopSelection, skinOps,  "loopSelection" );
def_struct_primitive       ( ringSelection, skinOps,  "ringSelection" );
def_struct_primitive       ( selectVerticesByBone, skinOps, "selectVerticesByBone" );

//DQ maxscript exposure
def_struct_primitive       ( setVertexDQWeight, skinOps, "setVertexDQWeight" );
def_struct_primitive       ( getVertexDQWeight, skinOps, "getVertexDQWeight" );

def_struct_primitive       ( enableDQOverrideWeighting, skinOps, "enableDQOverrideWeighting" );

def_struct_primitive       ( voxelWeighting, skinOps, "voxelWeighting" );


static BonesDefMod* get_bonedef_mod(Value* arg)
{
	Modifier *mod = arg->to_modifier();
	SClass_ID sid = mod->SuperClassID();
	Class_ID id = mod->ClassID();
	if ( sid != OSM_CLASS_ID || id != Class_ID(9815843,87654) )
		throw RuntimeError(GetString(IDS_PW_NOT_BONESDEF_ERROR), arg);
	BonesDefMod *bmod = (BonesDefMod*)mod;
	return bmod;
}


// Guard and setup helper, for skinOps maxscript functions
// Ensures the modifier ip pointer is valid, even if not editing in the UI
// Optionally initializes a ModContextList and INodeTab of modified objects,
// with handling for node specified as maxscript optional argument
class BonesDefModGuard
{
	public:

		BonesDefModGuard( Value* modVal, Value* nodeVal, ModContextList* mcList, INodeTab* nodes )
		{
			this->bmod = get_bonedef_mod( modVal );
			this->mcList = mcList;
			this->nodes = nodes;
			
			ipOld = bmod->ip;
			isEditing = (ipOld==NULL?  FALSE:TRUE);
			count = 0;

			if( (mcList!=NULL) || (nodes!=NULL) )
			{
				if( !bmod->TestAFlag(A_IS_DELETED) )
				{
					BOOL useSel =	(isEditing?  TRUE  : FALSE);  // use selection if editing, scene otherwise
					BOOL useScene =	(isEditing?  FALSE : TRUE);

					// TODO: Add handling in case nodeVal is a MaxScript array of nodes

					// throw error message if value defined, but not a node
					INode* node = ((nodeVal==NULL)||(nodeVal==&undefined)? NULL : nodeVal->to_node());

					if( node!=NULL )
						count = GetModContextListNode( bmod, node, mcList, nodes );
					if( (count==0) && useSel && isEditing )
						count = GetModContextListSel( bmod, mcList, nodes );
					if( (count==0) && useScene )
						count = GetModContextListScene( bmod, mcList, nodes );
				}

				if( count==0 ) // modifier apparently is not applied on node
					throw RuntimeError(GetString(IDS_PW_SKIN_NOT_SELECTED), modVal);
			}

			if( !isEditing )
				bmod->ip = (IObjParam*)(GetCOREInterface());
		}
		~BonesDefModGuard()
		{
			if( !isEditing )
				bmod->ip = NULL;
		}

		BonesDefMod* GetMod()						{return bmod;}

	protected:
		BonesDefMod* bmod;
		IObjParam* ipOld;
		BOOL isEditing;
		ModContextList* mcList;
		INodeTab* nodes;
		int count; // number of objects modifier is applied to and included in current operation
};


// maxscript value + mod data -> bitarray of affected vertices, one bit per vert
// input is a number, array or numbers, or bitarray
static BOOL GetVertBitarray(Value* arg, BoneModData* bmd, BitArray& verts)
{
	verts.SetSize( bmd->VertexData.Count() );
	if (is_array(arg)) // array of indexes
	{
		Array* vval = (Array*)arg;

		for (int i = 0; i < vval->size; i++)
		{
			int vindex = vval->data[i]->to_int()-1;
			if ( (vindex < 0 ) || (vindex >= bmd->VertexData.Count()) )
				throw RuntimeError(GetString(IDS_PW_EXCEEDED_VERTEX_COUNT), arg);

			verts.Set( vindex );
		}
		return TRUE;
	}
	else if (is_BitArrayValue(arg))   // array of indexes
	{
		BitArrayValue *list = (BitArrayValue *) arg;
		for (int vindex = 0; vindex < list->bits.GetSize(); vindex++)
		{
			if (list->bits[vindex])
			{
				if ( (vindex < 0 ) || (vindex >= bmd->VertexData.Count()) )
					throw RuntimeError(GetString(IDS_PW_EXCEEDED_VERTEX_COUNT), arg);

				verts.Set( vindex );
			}
		}
		return TRUE;
	}
	else  // single index
	{
		int vindex = arg->to_int()-1;
		if ( (vindex < 0 ) || (vindex >= bmd->VertexData.Count()) )
			throw RuntimeError(GetString(IDS_PW_EXCEEDED_VERTEX_COUNT), arg);

		verts.Set( vindex );
		return TRUE;
	}
	return FALSE;
}

bool IsDerivedObject( Object* obj )
{
	return ( (obj!=NULL) && (obj->SuperClassID() == GEN_DERIVOB_CLASS_ID) );
}

bool IsXrefObject( Object* obj )
{
	return ( (obj!=NULL) && (obj->SuperClassID()==SYSTEM_CLASS_ID) &&
		(obj->ClassID()==XREFOBJ_CLASS_ID));
}

// modifier + node -> derivedObject + optional index of modifier within derivedObject
IDerivedObject* GetDerivedObject( INode* node, Modifier* mod, int& modIndex_out, int recurseXrefs )
{
	if( node==NULL )
		return NULL;

	Object* obj = node->GetObjectRef();

	if (!obj)
		return NULL;

	//ObjectState os = node->EvalWorldState(0);
	//if (os.obj && os.obj->SuperClassID() != GEOMOBJECT_CLASS_ID) {
	//	return NULL;
	//}

	// For all derived objects (can be > 1)
	while ( IsDerivedObject(obj) || (recurseXrefs && IsXrefObject(obj)) ) {
		// If an xref, recurse into object it holds
		if( recurseXrefs && IsXrefObject(obj) )
		{
			obj = (Object*)(obj->GetReference(0));
			continue;
		}

		IDerivedObject* dobj = (IDerivedObject*)obj;
		int numMods = dobj->NumModifiers();
		// Step through all modififers and verify the class id
		for (int m=0; m<numMods; m++) {
			Modifier* checkMod = dobj->GetModifier(m);
			if (mod == checkMod) {
				modIndex_out = m;
				return dobj;
			}
		}
		obj = dobj->GetObjRef();
	}

	return NULL;
}


// modifier + node -> modcontext
ModContext* GetModContext(INode* node, Modifier* mod, BOOL validate)
{
	int modIndex;

	IDerivedObject* dobj = GetDerivedObject( node, mod, modIndex, 1 );
	if (!dobj)
		return NULL;

	ModContext *mc = dobj->GetModContext(modIndex);
	if( validate && (mc->localData==NULL) )
	{
		node->EvalWorldState(0); // force an update to set the ModContext localData
		return GetModContext(node,mod,FALSE);
	}
	return mc;
}


// Enumerates all nodes with specified modifier applied, collects corresponding ModContexts
class ModContextEnum : public DependentEnumProc
{
public:
	Modifier* mod;
	ModContextList* mcList;
	INodeTab* nodes;
	int count;
	ModContextEnum( Modifier* mod, ModContextList* mcList_out, INodeTab* nodes_out )
		: mod(mod), mcList(mcList_out), nodes(nodes_out), count(0) {}
	~ModContextEnum() {}
	int Count() { return count; }
	virtual int proc(ReferenceMaker *rm)
	{
		if( rm==NULL )
		{
			return REF_ENUM_SKIP; // unexpected but whatever
		}
		if( rm->SuperClassID()==OSM_CLASS_ID )
		{	// Step zero, still looking at the modifier itself, continue
			return REF_ENUM_CONTINUE;
		}
		if( rm->SuperClassID()==MODAPP_CLASS_ID )
		{	// First step up the reference graph toward the node, continue
			return REF_ENUM_CONTINUE;
		}
		if( rm->SuperClassID()==GEN_DERIVOB_CLASS_ID )
		{	// Second step up the reference graph toward the node, continue
			return REF_ENUM_CONTINUE;
		}
		if( rm->SuperClassID()==BASENODE_CLASS_ID )
		{	// Found the node, record an entry
			count++;
			INode* node = (INode*)rm;
			if( nodes!=NULL )
			{
				nodes->Append( 1, &node );
			}
			if( mcList!=NULL )
			{
				ModContext* mc = GetModContext( node, mod );
				mcList->Append( 1, &mc );
			}
			// don't travel further down this branch of the dependency tree 
			return REF_ENUM_SKIP;
		}
		return REF_ENUM_SKIP;
	}
};


// Scene nodes: modifier -> modcontext + node list .. for ALL nodes with modifier applied 
int GetModContextListScene( Modifier* mod, ModContextList* mcList_out, INodeTab* nodes_out  )
{
	ModContextEnum mcEnum( mod, mcList_out, nodes_out );
	mod->DoEnumDependents( &mcEnum );
	int count = mcEnum.Count();

	return count;
}


// Selected nodes: modifier -> modcontext list + nodes list ... for ONLY selected nodes
int GetModContextListSel( Modifier* mod, ModContextList* mcList_out, INodeTab* nodes_out )
{
	DbgAssert( (mcList_out!=NULL) || (nodes_out!=NULL ) );
	if( (mcList_out==NULL) && (nodes_out==NULL ) ) return 0;
	int count = 0;

	// Ensure modifier is being edited,
	// otherwise modcontexts would be fetched for the wrong modifier
	if( mod->TestAFlag(A_MOD_BEING_EDITED) )
	{
		ModContextList mcList_temp;
		INodeTab nodes_temp;
		if( mcList_out==NULL )	mcList_out = &mcList_temp;
		if( nodes_out==NULL )	nodes_out = &nodes_temp;
		GetCOREInterface()->GetModContexts(*mcList_out,*nodes_out);
		count = nodes_out->Count();
	}

	return count;
}

// Multiple nodes or selected: modifier + node list -> modcontext list + nodes list ... for ONLY selected nodes
int GetModContextListNodesOrSel( Modifier* mod, INodeTab* nodes, ModContextList* mcList_out, INodeTab* nodes_out )
{
	if( (nodes!=NULL) && (nodes->Count()!=0) )
		return GetModContextListNodes( mod, nodes, mcList_out, nodes_out );
	return GetModContextListSel( mod, mcList_out, nodes_out );
}


// Multiple nodes: modifier + node list -> modcontext list + nodes list
int GetModContextListNodes( Modifier* mod, INodeTab* nodes, ModContextList* mcList_out, INodeTab* nodes_out )
{
	DbgAssert( nodes!=NULL );
	if( nodes==NULL ) return 0;
	DbgAssert( (mcList_out!=NULL) || (nodes_out!=NULL ) );
	if( (mcList_out==NULL) && (nodes_out==NULL ) ) return 0;

	int count = 0;
	for( int i=0; i<nodes->Count(); i++ )
	{
		if( mcList_out==NULL )
		{
			count++;
			nodes_out->Append( 1, nodes->Addr(i) );
		}
		else
		{
			INode* node = (*nodes)[i];
			ModContext* mc = GetModContext( node, mod );
			if ( mc!=NULL ) // modifier apparently is not applied on node
			{
				count++;
				mcList_out->Append( 1, &mc );
				if( nodes_out!=NULL )
					nodes_out->Append( 1, &node );
			}
		}
	}

	return count;
}


// One node: modifier + node -> modcontext list + nodes list
int GetModContextListNode( Modifier* mod, INode* node, ModContextList* mcList_out, INodeTab* nodes_out )
{
	DbgAssert( node!=NULL );
	if( node==NULL ) return 0;
	DbgAssert( (mcList_out!=NULL) || (nodes_out!=NULL ) );
	if( (mcList_out==NULL) && (nodes_out==NULL ) ) return 0;

	int count = 0;
	if( mcList_out==NULL )
	{
		count++;
		nodes_out->Append( 1, &node );
	}
	else
	{
		ModContext* mc = GetModContext( node, mod );
		if ( mc!=NULL ) // modifier apparently is not applied on node
		{
			count++;
			mcList_out->Append( 1, &mc );
			if( nodes_out!=NULL )
				nodes_out->Append( 1, &node );
		}
	}

	return 1;
}


// Many or selected nodes or scene nodes with modifier applied;
// modifier + node list -> modcontext + node list for the given node, or for all selected, or all applied
int GetModContextList( Modifier* bmod, INodeTab* nodes, ModContextList* mcList_out, INodeTab* nodes_out,
	BOOL useSel, BOOL useScene  )
{
	// if nodeVal and nodes_out array are both specified, the node will be placed as a solo entry in the array
	DbgAssert( bmod!=NULL );
	if( bmod==NULL )
		return 0;
	DbgAssert( (mcList_out!=NULL) || (nodes_out!=NULL) );
	if( (mcList_out==NULL) && (nodes_out==NULL) )
		return 0;

	int count = 0;
	if( nodes!=NULL )
		count = GetModContextListNodes( bmod, nodes, mcList_out, nodes_out );
	if( (count==0) && useSel )
		count = GetModContextListSel( bmod, mcList_out, nodes_out );
	if( (count==0) && useScene )
		count = GetModContextListScene( bmod, mcList_out, nodes_out );

	return count;
}


// modifier + maxscript value -> boneID
// input is a number, none, or string name of a node
int FindBoneID( BonesDefMod* bmod, Value* boneValue)
{
	int boneID=-1;
	if( is_number(boneValue) )
	{	// Assume value is already in ID, convert to 0-based
		boneID = boneValue->to_int()-1;
	}
	if( is_node(boneValue) )
	{	// Value is a bone node pointer
		boneID = bmod->FindBoneID(boneValue->to_node());
	}
	else if( is_string(boneValue) )
	{	// Value is a bone node name
		boneID = bmod->FindBoneID(boneValue->to_string());
	}
	if (boneID == -1)
		throw RuntimeError(GetString(IDS_PW_BONE_NOT_FOUND_ERROR), boneValue);
	return boneID;
}


// modifier + maxscript value -> boneID
// input is a number, none, or string name of a node
// if input is a node, and not already used as a bone in the skin modifier, it is added as a bone
int FindOrAddBoneID( BonesDefMod* bmod, Value* boneValue, INodeTab* modNodes )
{
	int boneID = -1;
	if( is_number(boneValue) )
	{	// Assume value is already in ID, convert to 0-based
		boneID = boneValue->to_int()-1;
	}
	if( is_node(boneValue) )
	{	// Value is a bone node pointer
		INode* boneNode = boneValue->to_node();
		boneID = bmod->FindBoneID(boneNode);
		if( boneID<0 )
		{
			bmod->AddBone( boneNode, 0, modNodes );
			boneID = bmod->FindBoneID(boneNode);
			DbgAssert( boneID>=0 );
		}
	}
	else if( is_string(boneValue) )
	{	// Value is a bone node name
		boneID = bmod->FindBoneID(boneValue->to_string());
	}
	if (boneID == -1)
		throw RuntimeError(GetString(IDS_PW_BONE_NOT_FOUND_ERROR), boneValue); // TODO: Localize this
	return boneID;
}


// name string -> node pointer, for any node in the scene
INode* FindNode( const TCHAR* nodeName )
{
	SceneNodeByNameCache& nodeByNameCache = SceneNodeByNameCache::GetInst();
	return nodeByNameCache.GetSceneNodeByName( nodeName, true, false );
	//INode* node = nodeByNameCache.GetSceneNodeByName( nodeName, true, false );
	//return (INode*)(Animatable::GetHandleByAnim( node )); // returns -1 if null
}

static int ConvertBoneIDToBoneIndex(BonesDefMod* bmod, int boneID)
{
	int boneIndex = bmod->ConvertBoneIDToBoneIndex(boneID);
	if (boneIndex == -1)
		throw RuntimeError(_T("Bone ID out of range: "), Integer::intern(boneID+1)); // TODO: Localize this
	return boneIndex;
}

static int ConvertBoneIDToListID(BonesDefMod* bmod, int boneID)
{
	int boneIndex = bmod->ConvertBoneIDToBoneIndex(boneID);
	if (boneIndex == -1)
		throw RuntimeError(_T("Bone ID out of range: "), Integer::intern(boneID+1)); // TODO: Localize this
	int listID = bmod->ConvertBoneIndexToListID(boneIndex);
	if (listID == -1)
		throw RuntimeError(_T("Invalid Bone ID in the scene: "), Integer::intern(boneID+1)); // TODO: Localize this
	return listID;
}




//descr modifier int:type float:falloff int:maxinfluence int:voxelsize bool:usewindingnumber bool:autonub
Value* voxelWeighting_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(voxelWeighting, 7, count);

	INodeTab nodes;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, &nodes );
	BonesDefMod* bmod = guard.GetMod();

	int engineType = arg_list[1]->to_int();
	float falloff = arg_list[2]->to_float();
	int maxInfluence = arg_list[3]->to_int();
	int voxelSize = arg_list[4]->to_int();
	bool useWinding = false;
	bool autoNub = false;
	if (arg_list[5]->to_bool()) useWinding = true;
	if (arg_list[6]->to_bool()) autoNub = true;

	
	if (engineType == 0)
		bmod->BMD_VoxelWeighting_Command(voxelSize,falloff,maxInfluence,useWinding,autoNub, &nodes);
	else
		bmod->BMD_HeatMapWeighting_Command(falloff,maxInfluence,autoNub, &nodes);

	return &ok;
}


Value* enableDQOverrideWeighting_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(setVertexDQWeight, 2, count);

	INodeTab nodes;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, &nodes );
	BonesDefMod* bmod = guard.GetMod();

	BOOL enable = arg_list[1]->to_bool();

	if (enable)
		bmod->EnableDQMaskEditing(true, &nodes);
	else
		bmod->EnableDQMaskEditing(false, &nodes);

	return &ok;
}


Value* setVertexDQWeight_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(setVertexDQWeight, 3, count);

	ModContextList mcList;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int index = arg_list[1]->to_int()-1;
	float weight = arg_list[2]->to_float();

	int objects = mcList.Count();

	if (objects > 0)
	{
		BoneModData *bmd = (BoneModData*)mcList[0]->localData;
		bmd->SetDQBlendWeight(index,weight);
	}
	return &ok;
}

Value* getVertexDQWeight_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(setVertexDQWeight, 2, count);

	ModContextList mcList;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int index = arg_list[1]->to_int()-1;
	float weight = 0.0f;

	int objects = mcList.Count();

	if (objects > 0)
	{
		BoneModData *bmd = (BoneModData*)mcList[0]->localData;
		weight = bmd->GetDQBlendWeight(index);
	}
	return_value (Float::intern(weight));
}


Value* selectVerticesByBone_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(selectVerticesByBone, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	bmod->SelectVerticesByBone(bmod->ModeBoneIndex);

	return &ok;
}

Value* growSelection_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(growSelection, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	bmod->GrowVertSel(GROW_SEL);

	return &ok;
}

Value* ringSelection_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(growSelection, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	bmod->EdgeSel(RING_SEL);

	return &ok;
}

Value* loopSelection_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(growSelection, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	bmod->EdgeSel(LOOP_SEL);

	return &ok;
}

Value* shrinkSelection_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(growSelection, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	bmod->GrowVertSel(SHRINK_SEL);

	return &ok;
}

Value* closeWeightTable_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(closeWeightTable, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	if (bmod->hWeightTable)
		bmod->fnWeightTable();

	return &ok;
}

Value* isWeightTableOpen_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(isWeightTableOpen, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	if (bmod->hWeightTable)
		return_value (Integer::intern(TRUE));
	else return_value (Integer::intern(FALSE));
}

Value* closeWeightTool_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(closeWeightTool, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	if (bmod->weightToolHWND)
		bmod->BringUpWeightTool();

	return &ok;
}

Value* isWeightToolOpen_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(isWeightToolOpen, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	if (bmod->weightToolHWND)
		return_value (Integer::intern(TRUE));
	else return_value (Integer::intern(FALSE));
}

Value* weightTable_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(weightTable, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	bmod->fnWeightTable();

	return &ok;
}

Value* selectBoneByNode_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(selectBoneByNode, 2, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	INode *node = arg_list[1]->to_node();
	for (int i = 0; i < bmod->BoneData.Count(); i++)
	{
		if (node == bmod->BoneData[i].Node)
		{
			bmod->SelectBone(i);
			break;
		}
	}

	return &ok;
}

Value* pasteWeight_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(pasteWeight, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	bmod->PasteWeights(FALSE,0.1f);

	return &ok;
}

Value* pasteWeightByPos_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(pasteWeightByPos, 2, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	float v = arg_list[1]->to_float();
	bmod->PasteWeights(TRUE,v);

	return &ok;
}

Value* copyWeight_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(copyWeight, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	bmod->CopyWeights();

	return &ok;
}

Value* scaleWeight_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(scaleWeight, 2, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	float v = arg_list[1]->to_float();
	bmod->ScaleWeight(v);

	return &ok;
}

Value* addWeight_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(addWeight, 2, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	float v = arg_list[1]->to_float();
	bmod->AddWeight(v);

	return &ok;
}

Value* setWeight_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(setWeight, 2, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	float v = arg_list[1]->to_float();
	bmod->SetWeight(v);

	return &ok;
}

Value* 	weightTool_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(weightTool, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	bmod->BringUpWeightTool();

	return &ok;
}

Value* selectPreviousSibling_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(selectPreviousSibling, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	bmod->SelectSibling(FALSE);

	return &ok;
}

Value* selectNextSibling_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(selectNextSibling, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	bmod->SelectSibling(TRUE);

	return &ok;
}

Value* selectChild_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(selectChild, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	bmod->SelectChild();

	return &ok;
}

Value* selectParent_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(selectChild, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	bmod->SelectParent();

	return &ok;
}

Value* removeZeroWeights_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(removeZeroWeights, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	bmod->RemoveZeroWeights();

	return &ok;
}

Value* getSelectedCrossSectionIndex_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(getSelectedCrossSectionIndex, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int ct = bmod->ModeBoneEnvelopeIndex+1;

	return_value (Integer::intern(ct));
}

Value* getSelectedCrossSectionIsInner_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(getSelectedCrossSectionIsInner, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int ct = 0;
	if (bmod->ModeBoneEnvelopeSubType < 4)
		ct = 1;

	return_value (Integer::intern(ct));
}

Value* setSelectedCrossSection_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(setSelectedCrossSection, 2, count);

	ModContextList mcList;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, NULL );
	BonesDefMod* bmod = guard.GetMod();

	float radius = arg_list[1]->to_float();

	int objects = mcList.Count();

	if (objects > 0)
	{
		BoneModData *bmd = (BoneModData*)mcList[0]->localData;

		BOOL animate = FALSE;
		bmod->pblock_advance->GetValue(skin_advance_animatable_envelopes,0,animate,FOREVER);
		if (!animate)
		{
			SuspendAnimate();
			AnimateOff();
		}

		if (bmod->IsValidBoneIndex(bmod->ModeBoneIndex))
		{
			BoneDataClass &boneData = bmod->BoneData[bmod->ModeBoneIndex];
			if ((bmod->ModeBoneEnvelopeIndex>=0) && (bmod->ModeBoneEnvelopeIndex < boneData.CrossSectionList.Count()))
			{
				if (bmod->ModeBoneEnvelopeSubType<4)
					boneData.CrossSectionList[bmod->ModeBoneEnvelopeIndex].InnerControl->SetValue(bmod->currentTime,&radius,TRUE,CTRL_ABSOLUTE);
				else
					boneData.CrossSectionList[bmod->ModeBoneEnvelopeIndex].OuterControl->SetValue(bmod->currentTime,&radius,TRUE,CTRL_ABSOLUTE);
			}
		}
		if (!animate)
			ResumeAnimate();

		bmod->Reevaluate(TRUE);
		bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
		bmod->ip->RedrawViews(bmod->ip->GetTime());
	}

	return &ok;
}

Value* blurSelected_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(blurSelected, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	bmod->BlurSelected();

	return &ok;
}

static PickControlNode thePickMode;

void BonesDefMod::AddFromViewStart()
{
	if (inAddBoneMode)
	{
		inAddBoneMode = FALSE;
		ip->ClearPickMode();
	}

	if (ip && (!inAddBoneMode))
	{
		inAddBoneMode = TRUE;
		thePickMode.mod  = this;
		ip->SetPickMode(&thePickMode);
	}
}

void BonesDefMod::AddFromViewEnd()
{
	if (ip)
	{
		inAddBoneMode = FALSE;
		ip->ClearPickMode();
	}
}

static INT_PTR CALLBACK DeleteDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void BonesDefMod::MultiDelete()
{
	int iret = DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_REMOVE_DIALOG), hParam,DeleteDlgProc,(LPARAM)this);

	if (iret)
	{
		int ctl = removeList.Count(); // removeList contains boneID values
		for (int m =(ctl-1); m >= 0 ; m --)
		{
			int k = ConvertBoneIDToBoneIndex(removeList[m]);
			//transform end points back
			if ((k != -1) && (BoneData[k].Node))
			{
				//Delete all old cross sections
				RemoveBone(k);
			}
		}
		int ct = NumberNonNullBones();
		if (ct ==0)
			EnableWindow(GetDlgItem(hParam,IDC_REMOVE),FALSE);
		else EnableWindow(GetDlgItem(hParam,IDC_REMOVE),TRUE);

		int fsel = SendMessage(GetDlgItem(hParam,IDC_LIST1), LB_GETCURSEL,0,0);
		ModeBoneIndex = ConvertListIDToBoneIndex(fsel);

		cacheValid = TRUE;

		UpdatePropInterface();
		NotifyDependents(FOREVER,PART_DISPLAY,REFMSG_CHANGE);
		ip->RedrawViews(ip->GetTime());
	}
}

void BonesDefMod::SelectNextBone()
{
	int totalCount = NumberNonNullBones();
	if (totalCount > 1)
	{
		int listID = SendMessage(GetDlgItem(hParam,IDC_LIST1),LB_GETCURSEL ,0,0);
		listID++;
		int listCount = SendMessage(GetDlgItem(hParam,IDC_LIST1),LB_GETCOUNT ,0,0);

		if (listID >= listCount)
			listID = 0;

		int boneIndex = ConvertListIDToBoneIndex(listID);
		SelectBoneByBoneIndex(boneIndex);
		UpdatePropInterface();
		Reevaluate(TRUE);
		NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);

		ip->RedrawViews(ip->GetTime());
	}
}
void BonesDefMod::SelectPreviousBone()
{
	int totalCount = NumberNonNullBones();
	if (totalCount > 1)
	{
		int listID = SendMessage(GetDlgItem(hParam,IDC_LIST1),LB_GETCURSEL ,0,0);
		listID--;
		int listCount = SendMessage(GetDlgItem(hParam,IDC_LIST1),LB_GETCOUNT ,0,0);

		if (listID < 0)
			listID = listCount-1;

		int boneIndex = ConvertListIDToBoneIndex(listID);
		SelectBoneByBoneIndex(boneIndex);
		UpdatePropInterface();
		Reevaluate(TRUE);
		NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);

		ip->RedrawViews(ip->GetTime());
	}
}

//MIRROR
Value* mirrorPaste_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(mirrorPaste, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	if (bmod->mirrorData.Enabled())
		bmod->mirrorData.Paste();

	return &ok;
}

Value* pasteAllBones_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(pasteAllBones, 2, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	BOOL BtoG =  arg_list[1]->to_bool();
	if (bmod->mirrorData.Enabled())
		bmod->mirrorData.PasteAllBones(BtoG);

	return &ok;
}

Value* pasteAllVerts_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(pasteAllVerts, 2, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	BOOL BtoG =  arg_list[1]->to_bool();
	if (bmod->mirrorData.Enabled())
		bmod->mirrorData.PasteAllVertices(BtoG);

	return &ok;
}

Value* mirrorPasteBone_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(mirrorPasteBone, 3, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int sourceBoneID = arg_list[1]->to_int()-1;
	int destBoneID = arg_list[2]->to_int()-1;

	int sourceBoneIndex = ConvertBoneIDToBoneIndex(bmod, sourceBoneID);
	int destBoneIndex = ConvertBoneIDToBoneIndex(bmod, destBoneID);

	if (bmod->mirrorData.Enabled())
	{
		//hold all our bones
		if (theHold.Holding())
			theHold.Put(new PasteToAllRestore(bmod));

		Matrix3 mtm;
		int mirrorPlaneDir = 0;
		TimeValue t = bmod->ip->GetTime();
		bmod->mirrorData.GetMirrorTM(mtm, mirrorPlaneDir);
		bmod->mirrorData.PasteBones(t,mtm,mirrorPlaneDir,sourceBoneIndex,destBoneIndex);
	}

	return &ok;
}

Value* setMirrorTM_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(setMirrorTM, 2, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	Matrix3 tm = arg_list[1]->to_matrix3();
	if (bmod->mirrorData.Enabled())
	{
		bmod->mirrorData.SetInitialTM(tm);
	}

	return &ok;
}

Value* updateMirror_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(setMirrorTM, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	if (bmod->mirrorData.Enabled())
	{
		bmod->mirrorData.BuildBonesMirrorData();
		bmod->mirrorData.BuildVertexMirrorData();
	}

	return &ok;
}

Value* selectMirrorBones_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(selectMirrorBones, 2, count);

	ModContextList mcList;
	INodeTab nodes;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, &nodes );
	BonesDefMod* bmod = guard.GetMod();

	int objects = mcList.Count();
	Value* ival = arg_list[1];

	if (objects > 0)
	{
		bmod->mirrorData.ClearBoneSelection();

		if (!is_array(ival) && !is_BitArrayValue(ival))   // single index
		{
			int boneID = FindBoneID( bmod, ival );
			int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);
			bmod->mirrorData.SelectBone(boneIndex, TRUE);
		}
		else if (is_array(ival))   // array of indexes
		{
			Array* aval = (Array*)ival;
			for (int i = 0; i < aval->size; i++)
			{
				ival = aval->data[i];
				int boneID = FindBoneID( bmod, ival );
				if( boneID>=0 )
				{
					int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);
					bmod->mirrorData.SelectBone(boneIndex, TRUE);
				}
			}
		}
		else if (is_BitArrayValue(ival))   // array of indexes
		{
			BitArrayValue *list = (BitArrayValue *) ival;
			BitArray bitsIndex(bmod->BoneData.Count());
			for (int i = 0; i < list->bits.GetSize(); i++)
			{
				if (list->bits[i])
				{
					int boneIndex = ConvertBoneIDToBoneIndex(bmod, i);
					bitsIndex.Set(boneIndex);
				}
			}
			bmod->mirrorData.SelectBones(bitsIndex);
		}

		TimeValue t = bmod->ip->GetTime();
		INode* node = nodes[0];
		if (node)
		{
			node->InvalidateRect(t);
		}
		bmod->ip->RedrawViews(t);
	}
	return &ok;
}

Value* paintWeights_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(paintWeights, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	//Defect 724761 - The user is not supposed to be painting
	//on a spline object or on a Nurbs Curve with the Skin modifier
	//Paint Weights script command.
	BOOL objIsSplineOrNubrsCurve = FALSE;

	MyEnumProc dep;
	bmod->DoEnumDependents(&dep);
	//this puts back the original state of the node vc mods and shade state

	for (int  i = 0; i < dep.Nodes.Count(); i++)
	{
		INode *node = dep.Nodes[i];

		//get object state
		ObjectState os;
		os = node->EvalWorldState(bmod->RefFrame);

		//If the sub class of the Object is a Nurbs
		//we need to check if is't a nurbs curve or a nurbs surface.
		if (os.obj->IsSubClassOf(EDITABLE_SURF_CLASS_ID))
		{
			NURBSSet NurbsGroup;
			BOOL objectIsANurbs = GetNURBSSet(os.obj, GetCOREInterface()->GetTime(), NurbsGroup, TRUE);
			if (objectIsANurbs)
			{
				for(int j = 0; j < NurbsGroup.GetNumObjects(); j++)
				{
					//check if the type of the Nurbs object is a curve (point curve or CV curve)
					if (NurbsGroup.GetNURBSObject(j)->GetType() == kNPointCurve ||
						NurbsGroup.GetNURBSObject(j)->GetType() == kNCVCurve)
					{
						objIsSplineOrNubrsCurve = TRUE;
						break;
					}
				}
			}

			break;
		}

		//check if the sub class of the Object is a spline
		if(os.obj->IsSubClassOf(splineShapeClassID))
		{
			objIsSplineOrNubrsCurve = TRUE;
			break;
		}
	}
	//If the sub class of the Object isn't a spline or nurbs curve
	//the paint mode start normally
	if(!objIsSplineOrNubrsCurve)
		bmod->StartPaintMode();

	return &ok;
}

Value* paintOptions_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(paintOptions, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	bmod->PaintOptions();

	return &ok;
}

Value* bakeSelectedVerts_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(bakeSelectedVerts, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	bmod->BakeSelectedVertices();
	bmod->PaintAttribList();

	return &ok;
}

Value* isRigidHandle_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(isRigidHandle, 2, count);

	ModContextList mcList;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int vertID = arg_list[1]->to_int()-1;

	int objects = mcList.Count();

	BOOL rigid = FALSE;

	if ( objects != 0)
	{
		BoneModData *bmd = (BoneModData*)mcList[0]->localData;
		if ((vertID >= 0) && (vertID < bmd->VertexData.Count()))
			rigid = bmd->VertexData[vertID]->IsRigidHandle();
	}

	return_value (Integer::intern(rigid));
}

Value* rigidHandle_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(rigidHandle, 3, count);

	ModContextList mcList;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int vertID = arg_list[1]->to_int()-1;
	int rigid = arg_list[2]->to_bool();

	int objects = mcList.Count();

	if ( objects != 0)
	{
		BoneModData *bmd = (BoneModData*)mcList[0]->localData;
		if ((vertID >= 0) && (vertID < bmd->VertexData.Count()))
		{
			bmd->VertexData[vertID]->RigidHandle(rigid);
			bmd->validVerts.Set(vertID,FALSE);
		}
	}
	return &ok;
}

Value* Invalidate_cf(Value** arg_list, int count)
{
	// TODO: Not working with the node:<arg> key argument

	int countNoKeys = _count_with_keys(arg_list,count); // number of args not including key args
	if (countNoKeys == 1)
	{
		check_arg_count_with_keys(Invalidate, 1, count);
	}
	else
	{
		check_arg_count_with_keys(Invalidate, 2, count);
	}

	ModContextList mcList;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int objects = mcList.Count();

	BOOL rigid = TRUE;

	if ( objects != 0)
	{
		BoneModData *bmd = (BoneModData*)mcList[0]->localData;
		bmd->validVerts.ClearAll();
		bmod->Reevaluate(TRUE);
		bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);

		bmod->ip->RedrawViews(bmod->ip->GetTime());
	}

	return_value (Integer::intern(rigid));
}

Value* isRigidVertex_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(isRigidVertex, 2, count);

	ModContextList mcList;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int vertID = arg_list[1]->to_int()-1;

	int objects = mcList.Count();

	BOOL rigid = FALSE;

	if ( objects != 0)
	{
		BoneModData *bmd = (BoneModData*)mcList[0]->localData;
		if ((vertID >= 0) && (vertID < bmd->VertexData.Count()))
			rigid = bmd->VertexData[vertID]->IsRigid();
	}

	return_value (Integer::intern(rigid));
}

Value* rigidVertex_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(rigidVertex, 3, count);

	ModContextList mcList;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int vertID = arg_list[1]->to_int()-1;
	int rigid = arg_list[2]->to_bool();

	int objects = mcList.Count();

	if ( objects != 0)
	{
		BoneModData *bmd = (BoneModData*)mcList[0]->localData;
		if ((vertID >= 0) && (vertID < bmd->VertexData.Count()))
		{
			bmd->VertexData[vertID]->Rigid(rigid);
			bmd->validVerts.Set(vertID,FALSE);
		}
	}
	return &ok;
}

Value* isUnNormalizeVertex_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(isUnNormalizeVertex, 2, count);

	ModContextList mcList;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int vertID = arg_list[1]->to_int()-1;

	int objects = mcList.Count();

	BOOL unNormalize = FALSE;

	if ( objects != 0)
	{
		BoneModData *bmd = (BoneModData*)mcList[0]->localData;
		if ((vertID >= 0) && (vertID < bmd->VertexData.Count()))
			unNormalize = bmd->VertexData[vertID]->IsUnNormalized();
	}

	return_value (Integer::intern(unNormalize));
}

Value* unNormalizeVertex_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(unNormalizeVertex, 3, count);

	ModContextList mcList;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int vertID = arg_list[1]->to_int()-1;
	int unNormalize = arg_list[2]->to_bool();

	int objects = mcList.Count();

	if ( objects != 0)
	{
		BoneModData *bmd = (BoneModData*)mcList[0]->localData;
		if ((vertID >= 0) && (vertID < bmd->VertexData.Count()))
			bmod->NormalizeWeight(bmd,vertID, unNormalize);
	}
	return &ok;
}

Value* multiRemove_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(multiRemove, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	if ( bmod->ModeBoneIndex != -1 )
	{
		bmod->MultiDelete();
	}

	return &ok;
}

Value* addBoneFromViewStart_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(addBoneFromViewStart, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	bmod->AddFromViewStart();

	return &ok;
}

Value* addBoneFromViewEnd_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(addBoneFromViewEnd, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	bmod->AddFromViewEnd();

	return &ok;
}

Value* selectNextBone_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(selectNextBone, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	bmod->SelectNextBone();

	return &ok;
}

Value* selectPreviousBone_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(selectPreviousBone, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	bmod->SelectPreviousBone();

	return &ok;
}

Value* zoomToBone_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(zoomToBone, 2, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int all = arg_list[1]->to_bool();
	bmod->ZoomToBone(all);

	return &ok;
}

Value* zoomToGizmo_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(zoomToGizmo, 2, count);

	ModContextList mcList;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int all = arg_list[1]->to_bool();

	int objects = mcList.Count();

	if (objects != 0)
	{
		BoneModData *bmd = (BoneModData*)mcList[0]->localData;
		bmod->ZoomToGizmo(bmd,all);
	}

	return &ok;
}

Value* selectGizmo_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(selectGizmo, 2, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int giz = arg_list[1]->to_int()-1;
	bmod->SelectGizmo(giz);

	return &ok;
}

Value* getSelectedGizmo_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(getSelectedGizmo, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	return_value (Integer::intern(bmod->currentSelectedGizmo+1));
}

Value* getNumberOfGizmos_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(getNumberOfGizmos, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	return_value (Integer::intern(bmod->pblock_gizmos->Count(skin_gizmos_list)));
}

//enableGizmo
//skinops.enableGizmo $.modifiers[#Skin] gizmoID enable
Value* enableGizmo_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(enableGizmo, 3, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int gizID = arg_list[1]->to_int()-1;
	int enable = arg_list[2]->to_bool();;

	if ((gizID >= 0) && (gizID <bmod->pblock_gizmos->Count(skin_gizmos_list)))
	{
		ReferenceTarget *ref = bmod->pblock_gizmos->GetReferenceTarget(skin_gizmos_list,0,gizID);
		GizmoClass *gizmo = (GizmoClass *)ref;
		if (gizmo)
			gizmo->Enable(enable);
	}

	bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	bmod->ip->RedrawViews(bmod->ip->GetTime());

	return &ok;
}

Value* selectGizmoType_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(selectGizmoType, 2, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int giz = arg_list[1]->to_int()-1;
	bmod->SelectGizmoType(giz);

	return &ok;
}

Value* getSelectedGizmoType_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(getSelectedGizmoType, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int did = SendMessage(GetDlgItem(bmod->hParamGizmos,IDC_DEFORMERS), CB_GETCURSEL,0,0)+1;

	return_value (Integer::intern(did));
}

Value* getNumberOfGizmoTypes_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(getNumberOfGizmoTypes, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	return_value (Integer::intern(bmod->gizmoIDList.Count()));
}

Value* buttonCopyGizmo_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(buttonCopyGizmo, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	SendMessage(bmod->hParamGizmos,WM_COMMAND,IDC_COPY,NULL);

	return &ok;
}

Value* buttonPasteGizmo_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(buttonPasteGizmo, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	SendMessage(bmod->hParamGizmos,WM_COMMAND,IDC_PASTE,NULL);

	return &ok;
}

Value* gizmoResetRotationPlane_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(gizmoResetRotationPlane, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	if ((bmod->currentSelectedGizmo >= 0) && (bmod->currentSelectedGizmo<bmod->pblock_gizmos->Count(skin_gizmos_list)))
	{
		ReferenceTarget *ref;
		ref = bmod->pblock_gizmos->GetReferenceTarget(skin_gizmos_list,0,bmod->currentSelectedGizmo);
		IGizmoClass3 *giz3 = (IGizmoClass3 *) ref->GetInterface(I_GIZMO3);
		if (giz3)
		{
			giz3->ResetRotationPlane();
			bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
			bmod->ip->RedrawViews(bmod->ip->GetTime());
		}
	}

	return &ok;
}

Value* buttonAddGizmo_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(buttonAddGizmo, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	SendMessage(bmod->hParamGizmos,WM_COMMAND,IDC_ADD,NULL);

	return &ok;
}

Value* buttonRemoveGizmo_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(buttonRemoveGizmo, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	SendMessage(bmod->hParamGizmos,WM_COMMAND,IDC_REMOVE,NULL);

	return &ok;
}

Value* SaveEnvelopeAsASCII_cf(Value** arg_list, int count)
{
	TSTR fname;

	int countNoKeys = _count_with_keys(arg_list,count); // number of args not including key args
	if (countNoKeys == 1)
	{
	}
	else if (countNoKeys == 2)
	{
		fname = arg_list[1]->to_filename();
	}
	else check_arg_count_with_keys(SaveEnvelopeAsASCII, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	if ( countNoKeys == 1)
	{
		bmod->SaveEnvelopeDialog(FALSE);
	}
	else
	{
		DbgAssert(!fname.isNull());
		bmod->SaveEnvelope(fname,TRUE);
	}

	return &ok;
}

Value* saveEnvelope_cf(Value** arg_list, int count)
{
	TSTR fname;
	int countNoKeys = _count_with_keys(arg_list,count); // number of args not including key args
	if (countNoKeys == 1)
	{
	}
	else if (countNoKeys == 2)
	{
		fname = arg_list[1]->to_filename();
	}
	else check_arg_count_with_keys(saveEnvelope, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	if ( countNoKeys == 1)
	{
		bmod->SaveEnvelopeDialog();
	}
	else
	{
		DbgAssert(!fname.isNull());
		bmod->SaveEnvelope(fname);
	}

	return &ok;
}

Value* loadEnvelope_cf(Value** arg_list, int count)
{
	TSTR fname;
	int countNoKeys = _count_with_keys(arg_list,count); // number of args not including key args
	if (countNoKeys == 1)
	{
	}
	else if (countNoKeys == 2)
	{
		fname = arg_list[1]->to_filename();
	}
	else check_arg_count_with_keys(loadEnvelope, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	if ( countNoKeys == 1)
	{
		bmod->LoadEnvelopeDialog();
	}
	else
	{
		DbgAssert(!fname.isNull());
		bmod->LoadEnvelope(fname);
	}
	return &ok;
}

Value* loadEnvelopeAsASCII_cf(Value** arg_list, int count)
{
	TSTR fname;
	int countNoKeys = _count_with_keys(arg_list,count); // number of args not including key args
	if (countNoKeys == 1)
	{
	}
	else if (countNoKeys == 2)
	{
		fname = arg_list[1]->to_filename();
	}
	else check_arg_count_with_keys(loadEnvelopeAsASCII, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	if ( countNoKeys == 1)
	{
		bmod->LoadEnvelopeDialog(FALSE);
	}
	else
	{
		DbgAssert(!fname.isNull());
		bmod->LoadEnvelope(fname,TRUE);
	}
	return &ok;
}

Value* buttonInclude_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(buttonInclude, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	SendMessage(bmod->hParam,WM_COMMAND,IDC_INCLUDE,NULL);

	return &ok;
}

Value* buttonExclude_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(buttonExclude, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	SendMessage(bmod->hParam,WM_COMMAND,IDC_EXCLUDE,NULL);

	return &ok;
}

Value* buttonSelectExcluded_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(buttonSelectExcluded, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	SendMessage(bmod->hParam,WM_COMMAND,IDC_SELECT_EXCLUDED,NULL);

	return &ok;
}

Value* buttonPaint_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(buttonPaint, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	SendMessage(bmod->hParam,WM_COMMAND,IDC_PAINT,NULL);

	return &ok;
}

Value* buttonAddCrossSection_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(buttonAddCrossSection, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	if ((bmod->iCrossSectionButton) && (bmod->iCrossSectionButton->IsEnabled()))
	{
		SendMessage(bmod->hParam,WM_COMMAND,IDC_CREATE_CROSS_SECTION,NULL);
	}

	return &ok;
}

Value* buttonRemoveCrossSection_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(buttonRemoveCrossSection, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	BOOL enabled = IsWindowEnabled(GetDlgItem(bmod->hParam,IDC_CREATE_REMOVE_SECTION));
	if ( enabled )
	{
		SendMessage(bmod->hParam,WM_COMMAND,IDC_CREATE_REMOVE_SECTION,NULL);
	}

	return &ok;
}

Value* buttonAdd_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(buttonAdd, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	SendMessage(bmod->hParam,WM_COMMAND,IDC_ADD,NULL);

	return &ok;
}

Value* buttonRemove_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(buttonRemove, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	SendMessage(bmod->hParam,WM_COMMAND,IDC_REMOVE,NULL);

	return &ok;
}

Value* selectCrossSection_cf(Value** arg_list, int count)
{
	//skinops.selectCrossSection $.modifiers[#Skin] CrossSectionID Inner_Outer
	check_arg_count_with_keys(selectCrossSection, 3, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int crossID = arg_list[1]->to_int()-1;
	int inner = arg_list[2]->to_int();

	if (bmod->IsValidBoneIndex(bmod->ModeBoneIndex))
	{
		BoneDataClass &boneData = bmod->BoneData[bmod->ModeBoneIndex];
		if ( (crossID < 0) || (crossID >= boneData.CrossSectionList.Count()) )
			throw RuntimeError(GetString(IDS_PW_EXCEEDED_CROSS_COUNT), arg_list[0]);
		boneData.end1Selected = FALSE;
		boneData.end2Selected = FALSE;
		bmod->ModeBoneEndPoint = -1;
		bmod->ModeBoneEnvelopeIndex = crossID;
		bmod->ModeBoneEnvelopeSubType = inner*4;

		bmod->updateP = TRUE;
		bmod->UpdatePropInterface();
		bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
		bmod->ip->RedrawViews(bmod->ip->GetTime());
	}

	return &ok;
}

Value* selectStartPoint_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(selectStartPoint, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	if (bmod->IsValidBoneIndex(bmod->ModeBoneIndex))
	{
		BoneDataClass &boneData = bmod->BoneData[bmod->ModeBoneIndex];
		bmod->ModeBoneEndPoint = 0;
		boneData.end1Selected = TRUE;
		boneData.end2Selected = FALSE;

		bmod->UpdatePropInterface();
		bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
		bmod->ip->RedrawViews(bmod->ip->GetTime());
	}

	return &ok;
}

Value* selectEndPoint_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(selectEndPoint, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	if (bmod->IsValidBoneIndex(bmod->ModeBoneIndex))
	{
		BoneDataClass &boneData = bmod->BoneData[bmod->ModeBoneIndex];
		bmod->ModeBoneEndPoint = 1;
		boneData.end2Selected = TRUE;
		boneData.end1Selected = FALSE;

		bmod->UpdatePropInterface();
		bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
		bmod->ip->RedrawViews(bmod->ip->GetTime());
	}

	return &ok;
}

Value* copySelectedBone_cf(Value** arg_list, int count)
{
	//skinops.copySelectedBone $.modifiers[#Skin]
	check_arg_count_with_keys(copySelectedBone, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	if (bmod->IsValidBoneIndex(bmod->ModeBoneIndex))
	{
		bmod->CopyBone();
		if (bmod->iPaste)
			bmod->iPaste->Enable(TRUE);
	}

	return &ok;
}

Value* pasteToSelectedBone_cf(Value** arg_list, int count)
{
	//skinops.pasteToSelectedBone $.modifiers[#Skin]
	check_arg_count_with_keys(pasteToSelectedBone, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	if (bmod->IsValidBoneIndex(bmod->ModeBoneIndex))
	{
		bmod->PasteBone();
		bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
		bmod->ip->RedrawViews(bmod->ip->GetTime());
	}

	return &ok;
}

Value* pasteToAllBones_cf(Value** arg_list, int count)
{
	//skinops.pasteToAllBones $.modifiers[#Skin]
	check_arg_count_with_keys(pasteToAllBones, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	if (bmod->IsValidBoneIndex(bmod->ModeBoneIndex))
	{
		bmod->PasteToAllBones();
		bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
		bmod->ip->RedrawViews(bmod->ip->GetTime());
	}

	return &ok;
}

Value* pasteToBone_cf(Value** arg_list, int count)
{
	//skinops.pasteToBone $.modifiers[#Skin] boneID
	check_arg_count_with_keys(pasteToBone, 2, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneID = FindBoneID( bmod, arg_list[1] );
	int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID); // boneIndex not used, but want range checking done in ConvertBoneIDToBoneIndex

	if (bmod->IsValidBoneIndex(bmod->ModeBoneIndex))
	{
		bmod->pasteList.SetCount(1);
		bmod->pasteList[0] = boneID;
		bmod->PasteToSomeBones();
		bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
		bmod->ip->RedrawViews(bmod->ip->GetTime());
	}

	return &ok;
}

Value* setSelectedBonePropRelative_cf(Value** arg_list, int count)
{
	//skinops.setSelectedBonePropRelative $.modifiers[#Skin] relative
	check_arg_count_with_keys(setSelectedBonePropRelative, 2, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int relative = arg_list[1]->to_int();

	if (bmod->IsValidBoneIndex(bmod->ModeBoneIndex))
	{
		if (!relative)
			bmod->BoneData[bmod->ModeBoneIndex].flags |= BONE_ABSOLUTE_FLAG;
		else bmod->BoneData[bmod->ModeBoneIndex].flags &= ~BONE_ABSOLUTE_FLAG;
		bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
		bmod->UpdatePropInterface();
		bmod->ip->RedrawViews(bmod->ip->GetTime());
	}
	return &ok;
}

Value* getSelectedBonePropRelative_cf(Value** arg_list, int count)
{
	//skinops.getSelectedBonePropRelative $.modifiers[#Skin]
	check_arg_count_with_keys(getSelectedBonePropRelative, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	if (bmod->IsValidBoneIndex(bmod->ModeBoneIndex))
	{
		if (bmod->BoneData[bmod->ModeBoneIndex].flags & BONE_ABSOLUTE_FLAG)
			return_value (Integer::intern(0));
		else return_value (Integer::intern(1));
	}
	return &ok;
}

Value* setSelectedBonePropFalloff_cf(Value** arg_list, int count)
{
	//skinops.setSelectedBonePropFalloff $.modifiers[#Skin] relative
	check_arg_count_with_keys(setSelectedBonePropFalloff, 2, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int foff = arg_list[1]->to_int();

	if (bmod->IsValidBoneIndex(bmod->ModeBoneIndex))
	{
		if (foff == 1)
			bmod->BoneData[bmod->ModeBoneIndex].FalloffType = BONE_FALLOFF_X_FLAG;
		else if (foff == 2)
			bmod->BoneData[bmod->ModeBoneIndex].FalloffType = BONE_FALLOFF_SINE_FLAG;
		else if (foff == 3)
			bmod->BoneData[bmod->ModeBoneIndex].FalloffType = BONE_FALLOFF_3X_FLAG;
		else if (foff == 4)
			bmod->BoneData[bmod->ModeBoneIndex].FalloffType = BONE_FALLOFF_X3_FLAG;
		bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
		bmod->UpdatePropInterface();
		bmod->ip->RedrawViews(bmod->ip->GetTime());
	}

	return &ok;
}

Value* getSelectedBonePropFalloff_cf(Value** arg_list, int count)
{
	//skinops.getSelectedBonePropFalloff $.modifiers[#Skin]
	check_arg_count_with_keys(getSelectedBonePropFalloff, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	if (bmod->IsValidBoneIndex(bmod->ModeBoneIndex))
	{
		if (bmod->BoneData[bmod->ModeBoneIndex].FalloffType == BONE_FALLOFF_X_FLAG)
			return_value (Integer::intern(1));
		else if (bmod->BoneData[bmod->ModeBoneIndex].FalloffType == BONE_FALLOFF_SINE_FLAG)
			return_value (Integer::intern(2));
		else if (bmod->BoneData[bmod->ModeBoneIndex].FalloffType == BONE_FALLOFF_3X_FLAG)
			return_value (Integer::intern(3));
		else if (bmod->BoneData[bmod->ModeBoneIndex].FalloffType == BONE_FALLOFF_X3_FLAG)
			return_value (Integer::intern(4));
	}

	return &ok;
}

Value* setSelectedBonePropEnvelopeVisible_cf(Value** arg_list, int count)
{
	//skinops.setSelectedBonePropEnvelopeVisible $.modifiers[#Skin] relative
	check_arg_count_with_keys(setSelectedBonePropEnvelopeVisible, 2, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int vis = arg_list[1]->to_int();

	if (bmod->IsValidBoneIndex(bmod->ModeBoneIndex))
	{
		if (vis == 0)
			bmod->BoneData[bmod->ModeBoneIndex].flags &= ~BONE_DRAW_ENVELOPE_FLAG;
		else
			bmod->BoneData[bmod->ModeBoneIndex].flags |= BONE_DRAW_ENVELOPE_FLAG;
		bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
		bmod->UpdatePropInterface();
		bmod->ip->RedrawViews(bmod->ip->GetTime());
	}

	return &ok;
}

Value* getSelectedBonePropEnvelopeVisible_cf(Value** arg_list, int count)
{
	//skinops.getSelectedBonePropEnvelopeVisible $.modifiers[#Skin]
	check_arg_count_with_keys(getSelectedBonePropEnvelopeVisible, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	if (bmod->IsValidBoneIndex(bmod->ModeBoneIndex))
	{
		if (bmod->BoneData[bmod->ModeBoneIndex].flags & BONE_DRAW_ENVELOPE_FLAG)
			return_value (Integer::intern(1));
		else return_value (Integer::intern(0));
	}

	return &ok;
}

Value* setBonePropRelative_cf(Value** arg_list, int count)
{
	//skinops.setBonePropRelative $.modifiers[#Skin] boneID relative
	check_arg_count_with_keys(setBonePropRelative, 3, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneID = FindBoneID( bmod, arg_list[1] );
	int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);
	int relative = arg_list[2]->to_int();

	if (!relative)
		bmod->BoneData[boneIndex].flags |= BONE_ABSOLUTE_FLAG;
	else bmod->BoneData[boneIndex].flags &= ~BONE_ABSOLUTE_FLAG;
	bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	bmod->UpdatePropInterface();
	bmod->ip->RedrawViews(bmod->ip->GetTime());

	return &ok;
}

Value* getBonePropRelative_cf(Value** arg_list, int count)
{
	//skinops.getBonePropRelative $.modifiers[#Skin] boneID
	check_arg_count_with_keys(getBonePropRelative, 2, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneID = FindBoneID( bmod, arg_list[1] );
	int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);

	if (bmod->BoneData[boneIndex].flags & BONE_ABSOLUTE_FLAG)
		return_value (Integer::intern(0));
	else return_value (Integer::intern(1));

	return &ok;
}

Value* setBonePropFalloff_cf(Value** arg_list, int count)
{
	//skinops.setBonePropFalloff $.modifiers[#Skin] boneID relative
	check_arg_count_with_keys(setBonePropFalloff, 3, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneID = FindBoneID( bmod, arg_list[1] );
	int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);
	int foff = arg_list[2]->to_int();

	if (foff == 1)
		bmod->BoneData[boneIndex].FalloffType = BONE_FALLOFF_X_FLAG;
	else if (foff == 2)
		bmod->BoneData[boneIndex].FalloffType = BONE_FALLOFF_SINE_FLAG;
	else if (foff == 3)
		bmod->BoneData[boneIndex].FalloffType = BONE_FALLOFF_3X_FLAG;
	else if (foff == 4)
		bmod->BoneData[boneIndex].FalloffType = BONE_FALLOFF_X3_FLAG;
	bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	bmod->UpdatePropInterface();
	bmod->ip->RedrawViews(bmod->ip->GetTime());

	return &ok;
}

Value* getBonePropFalloff_cf(Value** arg_list, int count)
{
	//skinops.getBonePropFalloff $.modifiers[#Skin] boneID
	check_arg_count_with_keys(getBonePropFalloff, 2, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneID = FindBoneID( bmod, arg_list[1] );
	int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);

	if (bmod->BoneData[boneIndex].FalloffType == BONE_FALLOFF_X_FLAG)
		return_value (Integer::intern(1));
	else if (bmod->BoneData[boneIndex].FalloffType == BONE_FALLOFF_SINE_FLAG)
		return_value (Integer::intern(2));
	else if (bmod->BoneData[boneIndex].FalloffType == BONE_FALLOFF_3X_FLAG)
		return_value (Integer::intern(3));
	else if (bmod->BoneData[boneIndex].FalloffType == BONE_FALLOFF_X3_FLAG)
		return_value (Integer::intern(4));

	return &ok;
}

Value* setBonePropEnvelopeVisible_cf(Value** arg_list, int count)
{
	//skinops.setBonePropEnvelopeVisible $.modifiers[#Skin] boneID vis
	check_arg_count_with_keys(setBonePropEnvelopeVisible, 3, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneID = FindBoneID( bmod, arg_list[1] );
	int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);
	int vis = arg_list[2]->to_int();

	if (vis == 0)
		bmod->BoneData[boneIndex].flags &= ~BONE_DRAW_ENVELOPE_FLAG;
	else
		bmod->BoneData[boneIndex].flags |= BONE_DRAW_ENVELOPE_FLAG;
	bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	bmod->UpdatePropInterface();
	bmod->ip->RedrawViews(bmod->ip->GetTime());

	return &ok;
}

Value* getBonePropEnvelopeVisible_cf(Value** arg_list, int count)
{
	//skinops.getBonePropEnvelopeVisible $.modifiers[#Skin] boneID
	check_arg_count_with_keys(getBonePropEnvelopeVisible, 2, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneID = FindBoneID( bmod, arg_list[1] );
	int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);

	if (bmod->BoneData[boneIndex].flags & BONE_DRAW_ENVELOPE_FLAG)
		return_value (Integer::intern(1));
	else return_value (Integer::intern(0));

	return &ok;
}

Value* resetSelectedVerts_cf(Value** arg_list, int count)
{
	//skinops.resetSelectedVerts $.modifiers[#Skin]
	check_arg_count_with_keys(resetSelectedVerts, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	bmod->UnlockVerts();
	bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	bmod->ip->RedrawViews(bmod->ip->GetTime());

	return &ok;
}

Value* resetSelectedBone_cf(Value** arg_list, int count)
{
	//skinops.resetSelectedBone $.modifiers[#Skin]
	check_arg_count_with_keys(resetSelectedBone, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	bmod->unlockBone = TRUE;
	bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	bmod->ip->RedrawViews(bmod->ip->GetTime());

	return &ok;
}

Value* resetAllBones_cf(Value** arg_list, int count)
{
	//skinops.resetAllBones $.modifiers[#Skin]
	check_arg_count_with_keys(resetAllBones, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	bmod->unlockAllBones = TRUE;
	bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	bmod->ip->RedrawViews(bmod->ip->GetTime());

	return &ok;
}

Value* isBoneSelected_cf(Value** arg_list, int count)
{
	//skinops.isBoneSelected $.modifiers[#Skin] boneID
	check_arg_count_with_keys(isBoneSelected, 2, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneID = FindBoneID( bmod, arg_list[1] );
	int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);

	if (bmod->ModeBoneIndex == boneIndex)
		return_value (Integer::intern(1));
	else return_value (Integer::intern(0));
}

Value* removeBone_cf(Value** arg_list, int count)
{
	//skinops.removeBone $.modifiers[#Skin] boneID
	if( count<1 )
		check_arg_count_with_keys(removeBone, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneIndex = 0;
	int countNoKeys = _count_with_keys(arg_list,count); // number of args not including key args
	if (countNoKeys == 1)
	{
		boneIndex = bmod->ModeBoneIndex;
	}
	else if (countNoKeys == 2)
	{
		int boneID = FindBoneID( bmod, arg_list[1] );
		boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);
	}
	else check_arg_count_with_keys(removeBone, 2, count);

	if (bmod->IsValidBoneIndex(bmod->ModeBoneIndex))
	{
		bmod->RemoveBone(boneIndex);
		bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
		bmod->ip->RedrawViews(bmod->ip->GetTime());
	}

	return &ok;
}


Value* removeUnusedBones_cf(Value** arg_list, int count)
{
	//skinops.removeUnusedBones $.modifiers[#Skin] threshhold:<arg>
	check_arg_count_with_keys(removeUnusedBones, 1, count);

	ModContextList mcList;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int objects = mcList.Count();
	int numBones = bmod->GetNumBones();
	Value* threshholdVal = key_arg_or_default(threshhold, &unsupplied );
	float threshhold = (threshholdVal==&unsupplied?  0.0f : threshholdVal->to_float() );

	BitArray bones(numBones);
	if (objects > 0)
	{
		BoneModData *bmd = (BoneModData*)mcList[0]->localData;
		if (bmd != NULL)
		{
			int i, vertCount = bmd->VertexData.Count();
			for (i = 0; i < vertCount; i++)
			{
				bmd->VertexData[i]->GetAllAffectedBones(bones, threshhold);
			}

			bool foundAny = false;
			for (i = (numBones - 1); i >= 0; i--)
			{
				if (!bones[i])
				{
					bmod->RemoveBone(i);
					foundAny = true;
				}
			}
			if (foundAny)
			{
				bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
				bmod->ip->RedrawViews(bmod->ip->GetTime());
			}
		}
	}

	return &ok;
}


Value* addBone_cf(Value** arg_list, int count)
{
	//skinops.addbone $.modifiers[#Skin] bone_node update
	check_arg_count_with_keys(addBone, 3, count);

	INodeTab nodes;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, &nodes );
	BonesDefMod* bmod = guard.GetMod();

	INode *node = arg_list[1]->to_node();
	int update = arg_list[2]->to_int();

	bmod->AddBone(node,update, &nodes);

	return &ok;
}


Value* replaceBone_cf( Value** arg_list, int count )
{
	//skinops.replaceBone $.modifiers[#Skin] boneID node vertices:<arg>
	check_arg_count_with_keys(replaceBone, 3, count);

	ModContextList mcList;
	INodeTab nodes;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, &nodes );
	BonesDefMod* bmod = guard.GetMod();

	// Iterate through all relevant nodes using the modifier
	int oldBoneID = FindBoneID( bmod, arg_list[1] );
	int oldBoneIndex = ConvertBoneIDToBoneIndex(bmod, oldBoneID);

	// (1) Add new bone if necessary

	Value* newBoneValue = arg_list[2];
	int newBoneID = FindOrAddBoneID( bmod, arg_list[2], &nodes );
	int newBoneIndex = ConvertBoneIDToBoneIndex(bmod, newBoneID);

	// (2) Replace weight values for old bone with new bone on selected verts

	bool isOldUsed = false; // whether any verts still use the old bone

	int objects = mcList.Count();

	if (objects > 0)
	{
		for( int j=0; j<objects; j++ )
		{
			BoneModData *bmd = static_cast<BoneModData*>(mcList[j]->localData);
			if (bmd != NULL)
			{
				int i, vertCount = bmd->VertexData.Count();

				// (2a) Collect list of vertices for replacement
				//      Input may be a single number, bitarray, or array of integers

				// Assign bitarray of affected verts
				Value* ival = key_arg_or_default(vertices, NULL);
				BitArray vertsel;
				if (ival != NULL)
					GetVertBitarray(ival, bmd, vertsel);
				else
				{	// No optional arg specifid, toggle all verts as affected
					vertsel.SetSize(vertCount);
					vertsel.SetAll();
				}

				// (2b) Loop through vertices to perform replacement

				for (i = 0; i < vertCount; i++)
				{
					VertexListClass* vc = bmd->VertexData[i];
					if (vertsel[i])
					{	// Perform replacement on this vert
						int oldWeightIndex = vc->FindWeightIndex(oldBoneIndex);
						if (oldWeightIndex >= 0)
							vc->SetBoneIndex(oldWeightIndex, newBoneIndex);
					}
					else if (!isOldUsed)
					{	// Check if old bone still using this vert
						int oldWeightIndex = vc->FindWeightIndex(oldBoneIndex);
						if (oldWeightIndex >= 0)
							isOldUsed = true;
					}
				}
			}
		}

		// (3) Delete old bone if not used anywhere

		if( !isOldUsed )
			bmod->RemoveBone( oldBoneIndex );
		bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
		//NotifyDependents(FOREVER, 0, REFMSG_SUBANIM_STRUCTURE_CHANGED); // TODO: Need this?
		bmod->ip->RedrawViews(bmod->ip->GetTime());
		//RefillListBox(); // TODO: Need this?
	}

	return &ok;
}


Value* addCrossSection_cf(Value** arg_list, int count)
{
	//skinops.addCrossSection $.modifiers[#Skin] BoneID U InnerRadius OuterRadius
	//skinops.addCrossSection $.modifiers[#Skin] U InnerRadius OuterRadius
	//skinops.addCrossSection $.modifiers[#Skin] U
	if( count<1 )
		check_arg_count(addCrossSection, 5, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneIndex = 0;
	float u = 0.0f;
	float inner = 0.0f, outer = 0.0f;

	int countNoKeys = _count_with_keys(arg_list,count); // number of args not including key args
	if (countNoKeys == 5)
	{
		int boneID = FindBoneID( bmod, arg_list[1] );
		boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);

		u = arg_list[2]->to_float();
		inner = arg_list[3]->to_float();
		outer = arg_list[4]->to_float();
	}
	else if (countNoKeys ==4)
	{
		boneIndex = bmod->ModeBoneIndex;
		u = arg_list[1]->to_float();
		inner = arg_list[2]->to_float();
		outer = arg_list[3]->to_float();
	}
	else if (countNoKeys ==2)
	{
		u = arg_list[1]->to_float();
	}
	else check_arg_count_with_keys(addCrossSection, 5, count);

	if (bmod->IsValidBoneIndex(bmod->ModeBoneIndex))
	{
		if ((countNoKeys == 4) || (countNoKeys ==5)	)
			bmod->AddCrossSection(boneIndex, u, inner, outer);
		else bmod->AddCrossSection(u);

		if (bmod->ModeBoneIndex != boneIndex)
			bmod->Reevaluate(TRUE);
		bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
		bmod->ip->RedrawViews(bmod->ip->GetTime());
	}

	return &ok;
}

Value* removeCrossSection_cf(Value** arg_list, int count)
{
	//skinops.removeCrossSection $.modifiers[#Skin]
	//skinops.removeCrossSection $.modifiers[#Skin] BoneID CrossSectionID
	if( count<1 )
		check_arg_count(removeCrossSection, 3, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneIndex = 0;
	int crossID = 0;
	int countNoKeys = _count_with_keys(arg_list,count); // number of args not including key args
	if (countNoKeys == 1)
	{
		boneIndex = bmod->ModeBoneIndex;
		crossID = bmod->ModeBoneEnvelopeIndex;
	}
	else if (countNoKeys == 3)
	{
		int boneID = FindBoneID( bmod, arg_list[1] );
		boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);
		crossID = arg_list[2]->to_int()-1;
	}
	else check_arg_count_with_keys(removeCrossSection, 3, count);

	if (bmod->IsValidBoneIndex(bmod->ModeBoneIndex))
	{
		if ( (crossID < 0) || (crossID >= bmod->BoneData[boneIndex].CrossSectionList.Count()) )
			throw RuntimeError(GetString(IDS_PW_EXCEEDED_CROSS_COUNT), arg_list[0]);
		if (bmod->BoneData[boneIndex].CrossSectionList.Count() > 2)
			bmod->RemoveCrossSection(boneIndex, crossID);
	}

	if (bmod->ModeBoneIndex != boneIndex)
		bmod->Reevaluate(TRUE);
	bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	bmod->ip->RedrawViews(bmod->ip->GetTime());

	return &ok;
}

Value* getCrossSectionU_cf(Value** arg_list, int count)
{
	//skinops.GetCrossSectionU $.modifiers[#Skin] BoneID CrossSectionID
	check_arg_count_with_keys(getCrossSectionU, 3, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneID = FindBoneID( bmod, arg_list[1] );
	int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);
	int crossID = arg_list[2]->to_int()-1;

	BoneDataClass &boneData = bmod->BoneData[boneIndex];
	if ( (crossID < 0) || (crossID >= boneData.CrossSectionList.Count()) )
		throw RuntimeError(GetString(IDS_PW_EXCEEDED_CROSS_COUNT), arg_list[0]);

	float u = boneData.CrossSectionList[crossID].u;

	return_value (Float::intern(u));
}

Value* setCrossSectionU_cf(Value** arg_list, int count)
{
	//skinops.setCrossSectionU $.modifiers[#Skin] BoneID CrossSectionID U
	check_arg_count_with_keys(setCrossSectionU, 4, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneID = FindBoneID( bmod, arg_list[1] );
	int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);
	int crossID = arg_list[2]->to_int()-1;
	float u = arg_list[3]->to_float();

	BoneDataClass &boneData = bmod->BoneData[boneIndex];
	if ( (crossID < 0) || (crossID >= boneData.CrossSectionList.Count()) )
		throw RuntimeError(GetString(IDS_PW_EXCEEDED_CROSS_COUNT), arg_list[0]);

	boneData.CrossSectionList[crossID].u = u;

	if (bmod->ModeBoneIndex != boneIndex)
		bmod->Reevaluate(TRUE);
	bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	bmod->ip->RedrawViews(bmod->ip->GetTime());

	return &ok;
}

Value* getEndPoint_cf(Value** arg_list, int count)
{
	//skinops.GetEndPoint $.modifiers[#Skin] BoneID
	check_arg_count_with_keys(getEndPoint, 2, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneID = FindBoneID( bmod, arg_list[1] );
	int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);

	Point3 w(0.0f,0.0f,0.0f);
	Interval v;
	bmod->BoneData[boneIndex].EndPoint2Control->GetValue(bmod->currentTime,&w,v);

	return new Point3Value(w);
}

Value* setEndPoint_cf(Value** arg_list, int count)
{
	//skinops.setEndPoint $.modifiers[#Skin] BoneID EndPointPos
	check_arg_count_with_keys(setEndPoint, 3, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneID = FindBoneID( bmod, arg_list[1] );
	int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);
	Point3 l2 = arg_list[2]->to_point3();

	BOOL animate = FALSE;
	bmod->pblock_advance->GetValue(skin_advance_animatable_envelopes,0,animate,FOREVER);
	if (!animate)
	{
		SuspendAnimate();
		AnimateOff();
	}

	bmod->BoneData[boneIndex].EndPoint2Control->SetValue(bmod->currentTime,&l2);

	if (!animate)
		ResumeAnimate();

	if (bmod->ModeBoneIndex != boneIndex)
		bmod->Reevaluate(TRUE);
	bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	bmod->ip->RedrawViews(bmod->ip->GetTime());

	return &ok;
}

Value* getStartPoint_cf(Value** arg_list, int count)
{
	//skinops.getStartPoint $.modifiers[#Skin] BoneID
	check_arg_count_with_keys(getStartPoint, 2, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneID = FindBoneID( bmod, arg_list[1] );
	int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);

	Point3 w(0.0f,0.0f,0.0f);
	Interval v;
	bmod->BoneData[boneIndex].EndPoint1Control->GetValue(bmod->currentTime,&w,v);

	return new Point3Value(w);
}

Value* setStartPoint_cf(Value** arg_list, int count)
{
	//skinops.setStartPoint $.modifiers[#Skin] BoneID StartPointPos
	check_arg_count_with_keys(setStartPoint, 3, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneID = FindBoneID( bmod, arg_list[1] );
	int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);
	Point3 l2 = arg_list[2]->to_point3();

	BOOL animate = FALSE;
	bmod->pblock_advance->GetValue(skin_advance_animatable_envelopes,0,animate,FOREVER);
	if (!animate)
	{
		SuspendAnimate();
		AnimateOff();
	}

	bmod->BoneData[boneIndex].EndPoint1Control->SetValue(bmod->currentTime,&l2);

	if (!animate)
		ResumeAnimate();

	if (bmod->ModeBoneIndex != boneID)
		bmod->Reevaluate(TRUE);

	bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	bmod->ip->RedrawViews(bmod->ip->GetTime());

	return &ok;
}

Value* getNumberBones_cf(Value** arg_list, int count)
{
	//skinops.getNumberBones $.modifiers[#Skin]
	check_arg_count_with_keys(getNumberBones, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int ct = bmod->NumberNonNullBones();

	return_value (Integer::intern(ct));
}

Value* getNumberVertices_cf(Value** arg_list, int count)
{
	//skinops.getNumberVertices $.modifiers[#Skin]
	check_arg_count_with_keys(getNumberVertices, 1, count);

	ModContextList mcList;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int objects = mcList.Count();

	int ct = 0;
	if (objects != 0)
	{
		BoneModData *bmd = (BoneModData*)mcList[0]->localData;
		ct = bmd->VertexData.Count();
	}

	return_value (Integer::intern(ct));
}

Value* getVertexWeightCount_cf(Value** arg_list, int count)
{
	//skinops.getVertexWeightCount $.modifiers[#Skin] VertexID
	check_arg_count_with_keys(getVertexWeightCount, 2, count);

	ModContextList mcList;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int vindex = arg_list[1]->to_int()-1;

	int objects = mcList.Count();

	int ct = 0;
	if (objects != 0)
	{
		BoneModData *bmd = (BoneModData*)mcList[0]->localData;
		if (bmd != NULL)
		{
			if ((vindex < 0) || (vindex >= bmd->VertexData.Count()))
				throw RuntimeError(GetString(IDS_PW_EXCEEDED_VERTEX_COUNT), arg_list[0]);
			ct = bmd->VertexData[vindex]->WeightCount();
		}
	}

	return_value (Integer::intern(ct));
}

static Value* GetBoneNodeFromIndex(BonesDefMod* bmod, int boneIndex)
{
	if(NULL == bmod || NULL == bmod->ip) return &undefined;
	if (!bmod->IsValidBoneIndex(boneIndex)) return &undefined;
	BoneDataClass &boneData = bmod->BoneData[boneIndex];
	return new MAXNode(boneData.Node);
}

static Value* GetBoneNameFromIndex(BonesDefMod* bmod, int boneIndex, int listName)
{
	if(NULL == bmod || NULL == bmod->ip) return &undefined;
	if (!bmod->IsValidBoneIndex(boneIndex)) return &undefined;
	BoneDataClass &boneData = bmod->BoneData[boneIndex];
	if (listName)
	{
		Class_ID bid(BONE_CLASS_ID,0);
		ObjectState os = boneData.Node->EvalWorldState(bmod->RefFrame);
		if (( os.obj->ClassID() == bid) && (!boneData.name.isNull()) )
		{
			return new String(boneData.name);
		}
		else return new String(boneData.Node->GetName());
	}
	else return new String(boneData.Node->GetName());
}

Value* getBoneIDByListID_cf(Value** arg_list, int count)
{
	//skinops.getBoneIDByListID $.modifiers[#Skin] ListID
	check_arg_count_with_keys(getBoneIDByListID, 2, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int listID = arg_list[1]->to_int();
	int boneIndex = bmod->ConvertListIDToBoneIndex(listID-1);
	if (boneIndex == -1)
		throw RuntimeError(_T("List index out of range: "), Integer::intern(listID));
	int boneID = bmod->ConvertBoneIndexToBoneID(boneIndex);
	if (boneIndex == -1)
		throw RuntimeError(_T("Invalid Bone index in the scene: "), Integer::intern(boneIndex+1));

	return_value (Integer::intern(boneID+1));
}

Value* getListIDByBoneID_cf(Value** arg_list, int count)
{
	//skinops.getListIDByBoneID $.modifiers[#Skin] boneID
	check_arg_count_with_keys(getListIDByBoneID, 2, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneID = FindBoneID( bmod, arg_list[1] );
	int listID = ConvertBoneIDToListID(bmod, boneID);
	if (listID == -1)
		throw RuntimeError(_T("Invalid Bone ID in the scene: "), Integer::intern(boneID+1));

	return_value (Integer::intern(listID+1));
}

Value* getBoneNodes_cf(Value** arg_list, int count)
{
	//skinops.getBoneNodes $.modifiers[#Skin]
	check_arg_count_with_keys(getBoneNode, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int numBones = bmod->GetNumBones();
	one_typed_value_local(Array* result);
	vl.result = new Array (numBones);
	for( int i=0; i<numBones; i++ )
	{
		vl.result->append( new MAXNode(bmod->GetBone(i)) );
	}
	return_value(vl.result);
}

Value* getBoneNode_cf(Value** arg_list, int count)
{
	//skinops.getBoneNode $.modifiers[#Skin] boneID
	check_arg_count_with_keys(getBoneNode, 2, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneID = FindBoneID( bmod, arg_list[1] );
	int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);

	return GetBoneNodeFromIndex(bmod, boneIndex);
}

Value* getBoneName_cf(Value** arg_list, int count)
{
	//skinops.getBoneName $.modifiers[#Skin] boneID listname
	check_arg_count_with_keys(getBoneName, 3, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneID = FindBoneID( bmod, arg_list[1] );
	int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);
	int listName = arg_list[2]->to_int();

	return GetBoneNameFromIndex(bmod, boneIndex, listName);
}


// TODO: Maybe create method BonesDefMod::GetBoneWeights() and move this elsewhere

// Helper for getBoneWeights()
// Given a bone index, iterates through all vertices using the bone,
// and allows the bone weight to be read or set for each vertex
class BoneWeightIterator
{
	public:
		Tab<VertexListClass*>& vertexData;
		int boneIndex;
		int vertexIndex, vertexWeightIndex;
		float weightCache;
		BoneWeightIterator( BoneModData* modData, int boneIndex )
		: boneIndex(boneIndex), vertexData(modData->VertexData)
		{
			vertexIndex = vertexWeightIndex = 0;
			weightCache = 0.0f;
			if( !End() )
				weightCache = vertexData[vertexIndex]->GetWeightByBone(boneIndex, false, true);
		}
		bool End()			{ return (vertexIndex >= vertexData.Count()); }
		float GetWeight()	{ return weightCache; }
		void SetWeight( float w )
		{
			if( !End() )
			{
				// Set weight for this vertex and bone; raw value=true, normalize=true 
				vertexData[vertexIndex]->SetWeightByBone(boneIndex, w, true, true);
				weightCache = w;
			}
		}
		bool Next()
		{
			vertexIndex++;
			vertexWeightIndex=0;
			if( !End() )
			{
				// Get weight for this vertex and bone; raw value=true, normalized=false 
				weightCache = vertexData[vertexIndex]->GetWeightByBone(boneIndex, false, true);
				return true;
			}
			//else
			weightCache = 0.0f;
			return false;
		}
		bool operator++(int _unused)	{ return Next(); } // postfix
		bool operator++()				{ return Next(); } // prefix
};


Value* getBoneWeight_cf(Value** arg_list, int count)
{
	//skinops.getBoneWeight $.modifiers[#Skin] boneID vertexIndex
	check_arg_count_with_keys(getBoneWeight, 3, count);

	ModContextList mcList;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneID = FindBoneID( bmod, arg_list[1] );
	int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);
	int vindex = arg_list[2]->to_int()-1;

	int objects = mcList.Count();

	if ( objects!=1 )
		throw RuntimeError(GetString(IDS_PW_ONE_NODE_COUNT), Integer::intern(objects) );

	float ct = 0.0f;
	if (objects != 0)
	{
		BoneModData *bmd = (BoneModData*)mcList[0]->localData;
		if (bmd != NULL)
		{
			if ((vindex < 0) || (vindex >= bmd->VertexData.Count()))
				throw RuntimeError(GetString(IDS_PW_EXCEEDED_VERTEX_COUNT), arg_list[0]);
			// Get weight for this vertex and bone; raw value=false, normalized=true 
			ct = bmd->VertexData[vindex]->GetWeightByBone(boneIndex, false, true);
		}
	}

	return Float::intern(ct);
}

Value* getBoneWeights_cf(Value** arg_list, int count)
{
	//skinops.getBoneWeights $.modifiers[#Skin] boneID
	check_arg_count_with_keys(getBoneWeight, 2, count);

	ModContextList mcList;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneID = FindBoneID( bmod, arg_list[1] );
	int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);

	int objects = mcList.Count();

	if ( objects!=1 )
		throw RuntimeError(GetString(IDS_PW_ONE_NODE_COUNT), Integer::intern(objects) );

	BoneModData *bmd = (BoneModData*)mcList[0]->localData;
	if (bmd != NULL)
	{
		int numVertices = bmd->VertexData.Count();

		one_typed_value_local(Array* result);
		vl.result = new Array(numVertices);
		for (BoneWeightIterator iter(bmd, boneIndex); !iter.End(); iter.Next())
		{
			vl.result->append(Float::intern(iter.GetWeight()));
		}
		return_value(vl.result);
	}
	else
	{
		return NULL;
	}
}

Value* setBoneWeights_cf(Value** arg_list, int count)
{
	//skinops.setBoneWeights $.modifiers[#Skin] <boneID_integer> <vertex_index_integer> <weight_float>
	//skinops.setBoneWeights $.modifiers[#Skin] <boneID_integer> <vertex_index_array> <weight_array>
	check_arg_count_with_keys(setBoneWeight, 4, count);

	ModContextList mcList;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int objects = mcList.Count();

	if ( objects!=1 )
		throw RuntimeError(GetString(IDS_PW_ONE_NODE_COUNT), Integer::intern(objects) );

	int retval = 0;
	int boneID = FindBoneID( bmod, arg_list[1] );
	int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);
	Value* vertsval = arg_list[2];
	Value* weightsval = arg_list[3];

	BoneModData *bmd = (BoneModData*)mcList[0]->localData;
	if (bmd == NULL)
	{
		return NULL;
	}

	if (!is_array(weightsval) && !is_array(vertsval))  // single index
	{
		int vindex = vertsval->to_int()-1;
		if ( (vindex < 0 ) || (vindex >= bmd->VertexData.Count()) )
			throw RuntimeError(GetString(IDS_PW_EXCEEDED_VERTEX_COUNT), arg_list[0]);

		float weight = weightsval->to_float();
		if (!bmd->VertexData[vindex]->IsUnNormalized())
		{
			if (weight < 0.0f ) weight = 0.0f;
			if (weight > 1.0f ) weight = 1.0f;
		}
		bmod->SetVertexWeight(bmd, vindex, boneIndex, weight);
		retval = 1;
	}
	else // array of indexes
	{
		type_check(weightsval, Array, _T("setBoneWeights"));
		type_check(vertsval, Array, _T("setBoneWeights"));
		Array* wval = (Array*)weightsval;
		Array* vval = (Array*)vertsval;
		if (wval->size != vval->size) throw RuntimeError(GetString(IDS_PW_WEIGHT_VERTEX_COUNT), arg_list[0]);

		Tab<int> v;
		Tab<float> w;
		for (int i = 0; i < wval->size; i++)
		{
			int vindex = vval->data[i]->to_int()-1;
			if ( (vindex < 0 ) || (vindex >= bmd->VertexData.Count()) )
				throw RuntimeError(GetString(IDS_PW_EXCEEDED_VERTEX_COUNT), arg_list[0]);

			float weight = wval->data[i]->to_float();
			if (!bmd->VertexData[vindex]->IsUnNormalized())
			{
				if (weight < 0.0f ) weight = 0.0f;
				if (weight > 1.0f ) weight = 1.0f;
			}
			v.Append(1,&vindex,1);
			w.Append(1,&weight,1);
		}
		bmod->SetBoneWeights(bmd,boneIndex, v, w);
		retval = wval->size;
	}

	bmod->Reevaluate(TRUE);
	bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	bmod->ip->RedrawViews(bmod->ip->GetTime());
	return Integer::intern(retval);
}


// Command operation: Hammer Vertex class
class SkinOpsHammerCommand : public IBMD_Command
{
	public:
		~SkinOpsHammerCommand() {}
		void Execute( BonesDefMod * mod, BoneModData *bmd ) {}
		void Execute( BonesDefMod * mod, TimeValue t, BoneModData *bmd, ObjectState* os, INode* node );
		BitArray& GetVertSel() { return vertsel; } // get selection for read-write
	protected:
		BitArray vertsel;
};


// Command operation: Hammer Vertex method
// Requires ObjectState, as input to the modifier, for access to the mesh topology
void SkinOpsHammerCommand::Execute( BonesDefMod * mod, TimeValue t, BoneModData *bmd, ObjectState* os, INode* node )
{
	if( vertsel.AnyBitSet() )
	{
		// Use ObjectWrapper
		// Simplifies collection of topology and adjacency info,
		// Unifies interface for object types, to avoid special code for each (trimesh, polymesh, patch)
		ObjectWrapper obj;
		obj.Init( t, *os );

		// (1) For each affected vert, create a lookup of adjacent verts

		typedef std::set<int> IntSet;
		typedef std::map<int,double> IntToDoubleMap;

		Tab<IntSet*> adjacentSetList; // for each vert index, list of adjacent vert indices
		Tab<IntToDoubleMap*> weightAvgList; // for each vertex, lookup of [bone,weight] pairs

		int i, vertCount = bmd->VertexData.Count();
		adjacentSetList.SetCount(vertCount);
		weightAvgList.SetCount(vertCount);

		// (1a) Initialize the adjacency sets and weight averages, one per vertex
		for( i=0; i<vertCount; i++ )
		{
			if( vertsel[i] )
			{
				 adjacentSetList[i] = new IntSet();
				 weightAvgList[i] = new IntToDoubleMap();
			}
			else
			{
				adjacentSetList[i] = NULL;
				weightAvgList[i] = NULL;
			}
		}

		// (1b) Iterate edges of mesh to populate adjacency sets
		int edgeCount = obj.NumEdges();
		for( i=0; i<edgeCount; i++ )
		{
			GenEdge e = obj.GetEdge(i); // ObjectWrapper does its magic
			int a = e.v[0], b = e.v[1];

			DbgAssert( (a<vertCount) && (b<vertCount) );
			if( (a<vertCount) && (b<vertCount) )
			{
				if( vertsel[a] ) // if we need adjacency info for vert A, update it with B
					adjacentSetList[a]->insert( b );
				if( vertsel[b] ) // if we need adjacency info for vert B, update it with A
					adjacentSetList[b]->insert( a );
			}
		}

		// (2) For each affected vert, collect adjacent vert info, to produce average

		for( i=0; i<vertCount; i++ )
		{
			if( (adjacentSetList[i]!=NULL) && (weightAvgList[i]!=NULL) )
			{
				IntToDoubleMap& weightAvg = *(weightAvgList[i]);
				IntSet& adjacentSet = *(adjacentSetList[i]);

				size_t adjacentCount = adjacentSet.size();

				// (2a) For each vert adjacent to the current vert of interest,
				//      find all its bone weights, and add to a running total list of weights
				IntSet::iterator adjacentVert;
				for( adjacentVert = adjacentSet.begin(); adjacentVert != adjacentSet.end(); ++adjacentVert )
				{
					// Vertes Data of the current adjacent vert to the current vert of interest
					VertexListClass* vertexDataAdj = bmd->VertexData[ *adjacentVert ];
					DbgAssert( vertexDataAdj!=NULL );
					if( vertexDataAdj!=NULL )
					{
						for( int k=0; k<vertexDataAdj->WeightCount(); k++ )
						{
							int boneIndex = vertexDataAdj->GetBoneIndex(k);
							float boneWeight = vertexDataAdj->GetNormalizedWeight(k);
							IntToDoubleMap::iterator iter = weightAvg.find( boneIndex );
							// if weight for this bone exists already; add to the sum
							// else if no weight for this bone exists yet; set initial value
							if( iter!=weightAvg.end() )
								iter->second += boneWeight;
							else 
								weightAvg[boneIndex] = boneWeight;
						}
					}
				}

				// (2b) Compute averages. For each bone weight in the running total list of weights,
				//      normalize by dividing by the number of verts who contribute to the total
				IntToDoubleMap::iterator iter;
				for( iter = weightAvg.begin(); iter != weightAvg.end(); ++iter )
				{
					iter->second /= adjacentCount;
				}

			}
		}

		// (3) Finally, update original data with the new averages
		for( i=0; i<vertCount; i++ )
		{
			if( (weightAvgList[i]!=NULL) )
			{
				IntToDoubleMap& weightAvg = *(weightAvgList[i]);

				VertexListClass* vc = bmd->VertexData[ i ];
				vc->ZeroWeights();
				IntToDoubleMap::iterator iter;
				if( weightAvg.size()==1 )
				{
					iter = weightAvg.begin();
					vc->SetClosestBone( iter->first );
					vc->SetClosestBoneCache( iter->first );
					vc->SetWeightByBone( iter->first, iter->second, true, false );
				}
				else
				{
					vc->SetClosestBone(-1);
					for( iter = weightAvg.begin(); iter != weightAvg.end(); ++iter )
					{
						VertexInfluenceListClass vd;
						vd.Bones = iter->first;
						vd.normalizedInfluences = iter->second;
						vd.Influences = iter->second;
						vc->AppendWeight(vd);
					}
				}
				vc->Modified(TRUE);
			}
		}

		// (4) Delete the lists
		for( i=adjacentSetList.Count()-1; i>=0; i-- ) {
			SAFE_DELETE( adjacentSetList[i] );
		}
		for( i=weightAvgList.Count()-1; i>=0; i-- ) {
			SAFE_DELETE( weightAvgList[i] )
		}
	}

}

Value* hammer_cf(Value** arg_list, int count)
{
	//skinops.hammer $.modifiers[#Skin] <vertex_index_integer>
	//skinops.hammer $.modifiers[#Skin] <vertex_index_array>
	check_arg_count_with_keys(hammer, 2, count);
	int retval = 0;

	ModContextList mcList;
	INodeTab nodes;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, &nodes );
	BonesDefMod* bmod = guard.GetMod();

	int objects = mcList.Count();

	if ( objects!=1 )
		throw RuntimeError(GetString(IDS_PW_ONE_NODE_COUNT), Integer::intern(objects) );

	TimeValue t = GetCOREInterface()->GetTime();
	BoneModData *bmd = (BoneModData*)mcList[0]->localData;
	INode* node = nodes[0];

	if (bmd == NULL)
	{
		return NULL;
	}

	// (1) Create a command to execute on the affected verts
	SkinOpsHammerCommand* cmd = new SkinOpsHammerCommand(); // deleted by Execute()
	BitArray& vertsel = cmd->GetVertSel();

	// (2) Assign bitarray of affected verts
	BOOL ok = GetVertBitarray( arg_list[1], bmd, vertsel );
	if( !ok )
		integer_type_check(arg_list[1], _T("hammer"));
	retval = vertsel.NumberSet();

	// (3) Execute the command
	bmod->Execute( cmd, t, bmd, node ); // execute hammer command, deletes the command object

	return Integer::intern(retval);
}


Value* getVertexWeight_cf(Value** arg_list, int count)
{
	//skinops.getVertexWeight $.modifiers[#Skin] vertexID vertexWeightIndex
	check_arg_count_with_keys(getVertexWeight, 3, count);

	ModContextList mcList;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int vindex = arg_list[1]->to_int()-1;
	int subindex = arg_list[2]->to_int()-1;

	int objects = mcList.Count();

	float ct = 0.0f;
	if (objects != 0)
	{
		BoneModData *bmd = (BoneModData*)mcList[0]->localData;
		if ( (vindex < 0 ) || (vindex >= bmd->VertexData.Count()) )
			throw RuntimeError(GetString(IDS_PW_EXCEEDED_VERTEX_COUNT), arg_list[0]);
		if ( (subindex < 0 ) || (subindex >= bmd->VertexData[vindex]->WeightCount()) )
			throw RuntimeError(GetString(IDS_PW_EXCEEDED_SUBVERTEX_COUNT), arg_list[0]);
		ct = bmod->RetrieveNormalizedWeight(bmd,vindex,subindex);
	}

	return_value (Float::intern(ct));
}

Value* getVertexWeightBoneID_cf(Value** arg_list, int count)
{
	//skinops.getVertexWeightBoneID $.modifiers[#Skin] vertexID vertexWeightIndex
	check_arg_count_with_keys(getVertexWeightBoneID, 3, count);

	ModContextList mcList;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int vindex = arg_list[1]->to_int()-1;
	int subindex = arg_list[2]->to_int()-1;

	int objects = mcList.Count();

	int ct = 0;
	if (objects != 0)
	{
		BoneModData *bmd = (BoneModData*)mcList[0]->localData;
		if ( (vindex < 0 ) || (vindex >= bmd->VertexData.Count()) )
			throw RuntimeError(GetString(IDS_PW_EXCEEDED_VERTEX_COUNT), arg_list[0]);
		if ( (subindex < 0 ) || (subindex >= bmd->VertexData[vindex]->WeightCount()) )
			throw RuntimeError(GetString(IDS_PW_EXCEEDED_SUBVERTEX_COUNT), arg_list[0]);

		int boneIndex = bmd->VertexData[vindex]->GetBoneIndex(subindex);
		ct = bmod->ConvertBoneIndexToBoneID(boneIndex)+1;
	}
	return_value (Integer::intern(ct));
}

Value* selectVertices_cf(Value** arg_list, int count)
{
	//skinops.SelectVertices $.modifiers[#Skin] ( <vertex_integer> | <vertex_array > | <<vertex_bitarray> )
	check_arg_count_with_keys(SelectVertices, 2, count);

	ModContextList mcList;
	INodeTab nodes;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, &nodes );
	BonesDefMod* bmod = guard.GetMod();

	int objects = mcList.Count();

	int ct = 0;
	Value* ival = arg_list[1];
	if (objects > 0)
	{
		BoneModData *bmd = (BoneModData*)mcList[0]->localData;

		if (bmd == NULL)
		{
			return NULL;
		}

		// Unselect all verts
		bmd->selected.ClearAll();

		// Assign bitarray of affected verts
		BitArray vertsel;
		GetVertBitarray(ival,bmd,vertsel);

		// Select verts according to bitarray
		for( int vindex = 0; vindex < vertsel.GetSize(); vindex++ )
		{
			if (vertsel[vindex])
				bmd->selected.Set(vindex,TRUE);
		}

		TimeValue t = bmod->ip->GetTime();
		INode* node = nodes[0];
		if (node)
		{
			node->InvalidateRect(t);
		}

		bmod->ip->RedrawViews(t);
	}
	return &ok;
}

Value* getSelectedVertices_cf(Value** arg_list, int count)
{
	//skinops.getSelectedVertices $.modifiers[#Skin]
	check_arg_count_with_keys(getSelectedVertices, 1, count);

	ModContextList mcList;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int objects = mcList.Count();

	if ( objects!=1 )
		throw RuntimeError(GetString(IDS_PW_ONE_NODE_COUNT), Integer::intern(objects) );

	BoneModData *bmd = (BoneModData*)mcList[0]->localData;

	if (bmd == NULL)
	{
		return NULL;
	}

	int numVertices = bmd->selected.GetSize();
	one_typed_value_local(Array* result);
	vl.result = new Array (numVertices);
	for( int i=0; i<numVertices; i++ )
	{
		if( bmd->selected[i] )
			vl.result->append( Integer::intern(i+1) ); // 0-based to 1-based
	}
	return_value(vl.result);
}

// Same as replaceVertexWeights_cf(), except existing weights for the vert are not cleared to zero,
// so they will contribute afterwards, in addition to weights explicitly set by this call
Value* setVertexWeights_cf(Value** arg_list, int count)
{
	//skinops.setVertexWeights $.modifiers[#Skin] <vertex_integer> <vertex_bone_integer> <weight_float>
	//skinops.setVertexWeights $.modifiers[#Skin] <vertex_integer> <vertex_bone_array> <weight_array>
	check_arg_count_with_keys(setVertexWeights, 4, count);

	ModContextList mcList;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int objects = mcList.Count();

	int vertID = arg_list[1]->to_int()-1;
	Value* bonesval = arg_list[2];
	Value* weightsval = arg_list[3];

	if (objects != 0)
	{
		BoneModData *bmd = (BoneModData*)mcList[0]->localData;
		if ( (vertID < 0 ) || (vertID >= bmd->VertexData.Count()) )
			throw RuntimeError(GetString(IDS_PW_EXCEEDED_VERTEX_COUNT), arg_list[0]);

		if (!is_array(weightsval) && !is_array(bonesval))  // single index
		{
			int boneID = FindBoneID( bmod, bonesval );
			int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);

			float weight = weightsval->to_float();
			if (!bmd->VertexData[vertID]->IsUnNormalized())
			{
				if (weight < 0.0f ) weight = 0.0f;
				if (weight > 1.0f ) weight = 1.0f;
			}
			bmod->SetVertex(bmd, vertID, boneIndex, weight);
		}
		else // array of indexes
		{
			type_check(weightsval, Array, _T("setVertexWeights"));
			type_check(bonesval, Array, _T("setVertexWeights"));
			Array* wval = (Array*)weightsval;
			Array* bval = (Array*)bonesval;
			if (wval->size != bval->size) throw RuntimeError(GetString(IDS_PW_WEIGHT_BONE_COUNT), arg_list[0]);

			Tab<int> b;
			Tab<float> v;
			for (int i = 0; i < wval->size; i++)
			{
				int boneID = FindBoneID( bmod, bval->data[i] );
				int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);
				float weight = wval->data[i]->to_float();
				if (!bmd->VertexData[vertID]->IsUnNormalized())
				{
					if (weight < 0.0f ) weight = 0.0f;
					if (weight > 1.0f ) weight = 1.0f;
				}
				b.Append(1,&boneIndex,1);
				v.Append(1,&weight,1);
			}
			bmod->SetVertexWeights(bmd,vertID, b, v);
		}
	}
	bmod->Reevaluate(TRUE);
	bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	bmod->ip->RedrawViews(bmod->ip->GetTime());

	return &ok;
}

// Same as setVertexWeights_cf(), except all existing weights for the vert are cleared to zero first,
// so only weights explicitly set by this call will apply afterwards
Value* replaceVertexWeights_cf(Value** arg_list, int count)
{
	//skinops.replaceVertexWeights $.modifiers[#Skin] <vertex_integer> <vertex_bone_integer> <weight_float>
	//skinops.replaceVertexWeights $.modifiers[#Skin] <vertex_integer> <vertex_bone_array> <weight_array>
	check_arg_count_with_keys(replaceVertexWeights, 4, count);

	ModContextList mcList;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int objects = mcList.Count();

	int vertID = arg_list[1]->to_int()-1;
	Value* bonesval = arg_list[2];
	Value* weightsval = arg_list[3];

	if (objects != 0)
	{
		BoneModData *bmd = (BoneModData*)mcList[0]->localData;
		if ( (vertID < 0 ) || (vertID >= bmd->VertexData.Count()) )
			throw RuntimeError(GetString(IDS_PW_EXCEEDED_VERTEX_COUNT), arg_list[0]);

		if (!is_array(weightsval) && !is_array(bonesval))  // single index
		{
			int boneID = FindBoneID( bmod, bonesval );
			int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);

			float weight = weightsval->to_float();
			if (!bmd->VertexData[vertID]->IsUnNormalized())
			{
				if (weight < 0.0f ) weight = 0.0f;
				if (weight > 1.0f ) weight = 1.0f;
			}
			bmd->VertexData[vertID]->ZeroWeights();
			bmod->SetVertex(bmd, vertID, boneIndex, weight);
		}
		else // array of indexes
		{
			type_check(weightsval, Array, _T("setVertexWeights"));
			type_check(bonesval, Array, _T("setVertexWeights"));
			Array* wval = (Array*)weightsval;
			Array* bval = (Array*)bonesval;
			if (wval->size != bval->size) throw RuntimeError(GetString(IDS_PW_WEIGHT_BONE_COUNT), arg_list[0]);

			Tab<int> b;
			Tab<float> v;
			for (int i = 0; i < wval->size; i++)
			{
				int boneID = FindBoneID( bmod, bval->data[i] );
				int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);
				float weight = wval->data[i]->to_float();
				if (!bmd->VertexData[vertID]->IsUnNormalized())
				{
					if (weight < 0.0f ) weight = 0.0f;
					if (weight > 1.0f ) weight = 1.0f;
				}
				b.Append(1,&boneIndex,1);
				v.Append(1,&weight,1);
			}
			bmd->VertexData[vertID]->ZeroWeights();
			bmod->SetVertexWeights(bmd,vertID, b, v);
		}
	}
	bmod->Reevaluate(TRUE);
	bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	bmod->ip->RedrawViews(bmod->ip->GetTime());

	return &ok;
}

Value* isVertexModified_cf(Value** arg_list, int count)
{
	//skinops.isVertexModified $.modifiers[#Skin] <vertex_integer>
	check_arg_count_with_keys(isVertexModified, 2, count);

	ModContextList mcList;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int vertID = arg_list[1]->to_int()-1;

	int objects = mcList.Count();

	int ct = 0;
	if (objects != 0)
	{
		BoneModData *bmd = (BoneModData*)mcList[0]->localData;
		if ( (vertID < 0 ) || (vertID >= bmd->VertexData.Count()) )
			throw RuntimeError(GetString(IDS_PW_EXCEEDED_VERTEX_COUNT), arg_list[0]);
		ct = bmd->VertexData[vertID]->IsModified();
	}

	return_value (Integer::intern(ct));
}

Value* isVertexSelected_cf(Value** arg_list, int count)
{
	//skinops.isVertexSelected $.modifiers[#Skin] <vertex_integer>
	check_arg_count_with_keys(isVertexSelected, 2, count);

	ModContextList mcList;
	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), &mcList, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int vertID = arg_list[1]->to_int()-1;

	int objects = mcList.Count();

	int ct = 0;
	if (objects != 0)
	{
		BoneModData *bmd = (BoneModData*)mcList[0]->localData;
		if ( (vertID < 0 ) || (vertID >= bmd->VertexData.Count()) )
			throw RuntimeError(GetString(IDS_PW_EXCEEDED_VERTEX_COUNT), arg_list[0]);
		ct = bmd->selected[vertID];
	}

	return_value (Integer::intern(ct));
}

Value* selectBone_cf(Value** arg_list, int count)
{
	//skinops.selectBone $.modifiers[#Skin] BoneID
	check_arg_count_with_keys(selectBone, 2, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneID = FindBoneID( bmod, arg_list[1] );
	int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);

	bmod->SelectBoneByBoneIndex(boneIndex);
	bmod->UpdatePropInterface();
	if (bmod->ModeBoneIndex != boneIndex)
		bmod->Reevaluate(TRUE);
	bmod->ModeBoneIndex = boneIndex;
	bmod->updateP = TRUE;

	bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	bmod->ip->RedrawViews(bmod->ip->GetTime());

	return &ok;
}

Value* getSelectedBone_cf(Value** arg_list, int count)
{
	//skinops.getSelectedBone $.modifiers[#Skin]
	check_arg_count_with_keys(getSelectedBone, 1, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneID = bmod->ConvertBoneIndexToBoneID(bmod->ModeBoneIndex)+1;

	return_value (Integer::intern(boneID));
}

Value* getNumberCrossSections_cf(Value** arg_list, int count)
{
	//skinops.getNumberCrossSections $.modifiers[#Skin] boneID
	check_arg_count_with_keys(getNumberCrossSections, 2, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneID = FindBoneID( bmod, arg_list[1] );
	int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);

	int ct = bmod->BoneData[boneIndex].CrossSectionList.Count();

	return_value (Integer::intern(ct));
}

Value* getInnerRadius_cf(Value** arg_list, int count)
{
	//skinops.getInnerRadius $.modifiers[#Skin] BoneID CrossSectionID
	check_arg_count_with_keys(getInnerRadius, 3, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneID = FindBoneID( bmod, arg_list[1] );
	int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);
	int crossID = arg_list[2]->to_int()-1;

	BoneDataClass &boneData = bmod->BoneData[boneIndex];
	if ( (crossID < 0) || (crossID >= boneData.CrossSectionList.Count()) )
		throw RuntimeError(GetString(IDS_PW_EXCEEDED_CROSS_COUNT), arg_list[0]);

	Interval v;
	float radius = 0;
	boneData.CrossSectionList[crossID].InnerControl->GetValue(bmod->currentTime,&radius,v);

	return_value (Float::intern(radius));
}

Value* getOuterRadius_cf(Value** arg_list, int count)
{
	//skinops.getOuterRadius $.modifiers[#Skin] BoneID CrossSectionID
	check_arg_count_with_keys(getOuterRadius, 3, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneID = FindBoneID( bmod, arg_list[1] );
	int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);
	int crossID = arg_list[2]->to_int()-1;

	BoneDataClass &boneData = bmod->BoneData[boneIndex];
	if ( (crossID < 0) || (crossID >= boneData.CrossSectionList.Count()) )
		throw RuntimeError(GetString(IDS_PW_EXCEEDED_CROSS_COUNT), arg_list[0]);

	Interval v;
	float radius = 0;
	boneData.CrossSectionList[crossID].OuterControl->GetValue(bmod->currentTime,&radius,v);

	return_value (Float::intern(radius));
}

Value* setInnerRadius_cf(Value** arg_list, int count)
{
	//skinops.setInnerRadius $.modifiers[#Skin] BoneID CrossSectionID radius
	check_arg_count_with_keys(setInnerRadius, 4, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneID = FindBoneID( bmod, arg_list[1] );
	int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);
	int crossID = arg_list[2]->to_int()-1;
	float radius = arg_list[3]->to_float();

	BoneDataClass &boneData = bmod->BoneData[boneIndex];
	if ( (crossID < 0) || (crossID >= boneData.CrossSectionList.Count()) )
		throw RuntimeError(GetString(IDS_PW_EXCEEDED_CROSS_COUNT), arg_list[0]);

	BOOL animate = FALSE;
	bmod->pblock_advance->GetValue(skin_advance_animatable_envelopes,0,animate,FOREVER);
	if (!animate)
	{
		SuspendAnimate();
		AnimateOff();
	}

	boneData.CrossSectionList[crossID].InnerControl->SetValue(bmod->currentTime,&radius);

	if (!animate)
		ResumeAnimate();

	if (bmod->ModeBoneIndex != boneIndex)
		bmod->Reevaluate(TRUE);
	bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	bmod->ip->RedrawViews(bmod->ip->GetTime());

	return &ok;
}

Value* setOuterRadius_cf(Value** arg_list, int count)
{
	//skinops.setOuterRadius $.modifiers[#Skin] BoneID CrossSectionID radius
	check_arg_count_with_keys(setOuterRadius, 4, count);

	BonesDefModGuard guard( arg_list[0], key_arg_or_default(node, NULL), NULL, NULL );
	BonesDefMod* bmod = guard.GetMod();

	int boneID = FindBoneID( bmod, arg_list[1] );
	int boneIndex = ConvertBoneIDToBoneIndex(bmod, boneID);
	int crossID = arg_list[2]->to_int()-1;
	float radius = arg_list[3]->to_float();

	BoneDataClass &boneData = bmod->BoneData[boneIndex];
	if ( (crossID < 0) || (crossID >= boneData.CrossSectionList.Count()) )
		throw RuntimeError(GetString(IDS_PW_EXCEEDED_CROSS_COUNT), arg_list[0]);

	BOOL animate = FALSE;
	bmod->pblock_advance->GetValue(skin_advance_animatable_envelopes,0,animate,FOREVER);
	if (!animate)
	{
		SuspendAnimate();
		AnimateOff();
	}

	boneData.CrossSectionList[crossID].OuterControl->SetValue(bmod->currentTime,&radius);

	if (!animate)
		ResumeAnimate();

	if (bmod->ModeBoneIndex != boneIndex)
		bmod->Reevaluate(TRUE);
	bmod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	bmod->ip->RedrawViews(bmod->ip->GetTime());

	return &ok;
}

/*-------------------------------------------------------------------*/
/*																	*/
/*				Create Cross Section Command Mode					*/
/*																	*/
/*-------------------------------------------------------------------*/

HCURSOR CreateCrossSectionMouseProc::GetTransformCursor()
{
	static HCURSOR hCur = NULL;

	if ( !hCur ) {
		hCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::SegmentRefine);
	}

	return hCur;
}

BOOL CreateCrossSectionMouseProc::HitTest( ViewExp *vpt, IPoint2 *p, int type, int flags )
{
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	//do a poly hit test
	int savedLimits, res = 0;
	GraphicsWindow *gw = vpt->getGW();

	HitRegion hr;
	MakeHitRegion(hr,type, 1,8,p);
	gw->setHitRegion(&hr);
	gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK) & ~GW_ILLUM);
	gw->setTransform(Matrix3(1));
	gw->clearHitCode();

	gw->setColor(LINE_COLOR, 1.0f,1.0f,1.0f);

	if (!mod->IsValidBoneIndex(mod->ModeBoneIndex ))
		return FALSE;
	BoneDataClass &boneData = mod->BoneData[mod->ModeBoneIndex];

	if (boneData.flags & BONE_SPLINE_FLAG)
	{
		ShapeObject *pathOb = NULL;
		ObjectState os = boneData.Node->EvalWorldState(mod->ip->GetTime());
		pathOb = (ShapeObject*)os.obj;
		if (pathOb->NumberOfCurves(mod->ip->GetTime()) != 0)
		{
			Matrix3 tm = boneData.Node->GetObjectTM(mod->ip->GetTime());
			Point3 plist[2];
			SplineU = -1.0f;
			float u = 0.0f;
			for (int spid = 0; spid < 100; spid++)
			{
				plist[0] = pathOb->InterpCurve3D(mod->ip->GetTime(), 0,u) * tm;
				plist[1] = pathOb->InterpCurve3D(mod->ip->GetTime(), 0,u+0.01f) * tm;
				u += 0.01f;
				gw->polyline(2, plist, NULL, NULL, 0);
				if (gw->checkHitCode())
				{
					res = TRUE;
					gw->clearHitCode();
					a = plist[0];
					b = plist[1];
					SplineU = u;
					spid = 100;
				}
				gw->clearHitCode();
			}
		}
	}
	else
	{
		Point3 plist[2];
		plist[0] = mod->Worldl1;
		plist[1] = mod->Worldl2;

		gw->polyline(2, plist, NULL, NULL, 0);
		if (gw->checkHitCode()) {
			res = TRUE;
			gw->clearHitCode();
		}
		gw->clearHitCode();
	}

	gw->setRndLimits(savedLimits);

	return res;
}

int CreateCrossSectionMouseProc::proc(
	HWND hwnd,
	int msg,
	int point,
	int flags,
	IPoint2 m )
{
	ViewExp &vpt = iObjParams->GetViewExp(hwnd);
	if ( ! vpt.IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	int res = TRUE;
	if ( !mod->ip ) return FALSE;

	switch ( msg ) {
	case MOUSE_PROPCLICK:
		iObjParams->SetStdCommandMode(CID_OBJMOVE);
		break;

	case MOUSE_POINT:
		if(HitTest(vpt.ToPointer(),&m,HITTYPE_POINT,0) ) {
			theHold.Begin();
			theHold.Put(new PasteRestore(mod));
			theHold.Accept(GetString(IDS_PW_ADDCROSSSECTION));

			//transform mouse point to world
			float u;
			if (mod->BoneData[mod->ModeBoneIndex].flags & BONE_SPLINE_FLAG)
				GetHit(u);
			else u = mod->GetU(vpt.ToPointer(),mod->Worldl1,mod->Worldl2, m);

			if (u <= 0.0f) u = 0.0001f;
			if (u >= 1.0f) u = 0.9999f;
			mod->AddCrossSection(u);
			macroRecorder->FunctionCall(_T("skinOps.addCrossSection"), 2,0, mr_reftarg, mod, mr_float,u);
			BOOL s = FALSE;

			mod->Reevaluate(TRUE);
			mod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
			mod->ip->RedrawViews(mod->ip->GetTime());
		}
		res = FALSE;
		break;

	case MOUSE_FREEMOVE:
		if ( HitTest(vpt.ToPointer(),&m,HITTYPE_POINT,HIT_ABORTONHIT) ) {
			SetCursor(LoadCursor(NULL,IDC_CROSS ));
		}
		else {
			SetCursor(LoadCursor(NULL,IDC_ARROW));
		}
		break;
	}

	return res;
}

/*-------------------------------------------------------------------*/

void CreateCrossSectionMode::EnterMode()
{
	mod->iCrossSectionButton->SetCheck(TRUE);
}

void CreateCrossSectionMode::ExitMode()
{
	mod->iCrossSectionButton->SetCheck(FALSE);
}

void BonesDefMod::StartCrossSectionMode(int type)
{
	if ( !ip ) return;

	if (ip->GetCommandMode() == CrossSectionMode) {
		ip->SetStdCommandMode(CID_OBJMOVE);
		return;
	}

	CrossSectionMode->SetType(type);
	ip->SetCommandMode(CrossSectionMode);
}

/*-------------------------------------------------------------------*/

static void BoneXORDottedLine( HWND hwnd, IPoint2 p0, IPoint2 p1 )
{
	HDC hdc;
	hdc = GetDC( hwnd );
	SetROP2( hdc, R2_XORPEN );
	SetBkMode( hdc, TRANSPARENT );
	SelectObject( hdc, CreatePen( PS_DOT, 0, ComputeViewportXORDrawColor() ) );
	MoveToEx( hdc, p0.x, p0.y, NULL );
	LineTo( hdc, p1.x, p1.y );
	DeleteObject( SelectObject( hdc, GetStockObject( BLACK_PEN ) ) );
	ReleaseDC( hwnd, hdc );
}

static void BoneXORDottedCircle( HWND hwnd, IPoint2 p0, float Radius )
{
	HDC hdc;
	hdc = GetDC( hwnd );
	SetROP2( hdc, R2_XORPEN );
	SetBkMode( hdc, TRANSPARENT );
	SelectObject( hdc, CreatePen( PS_DOT, 0, ComputeViewportXORDrawColor() ) );
	MoveToEx( hdc, p0.x +(int)Radius, p0.y, NULL );
	float angle = 0.0f;
	float inc = 2.0f*PI/20.f;
	IPoint2 p1;
	for (int i = 0; i < 20; i++)
	{
		angle += inc;
		p1.x = (int)(Radius * sin(angle) + Radius * cos(angle));
		p1.y = (int)(Radius * sin(angle) - Radius * cos(angle));
		LineTo( hdc, p0.x + p1.x, p0.y+p1.y );
	}

	DeleteObject( SelectObject( hdc, GetStockObject( BLACK_PEN ) ) );
	ReleaseDC( hwnd, hdc );
}

class CacheModEnumProc : public ModContextEnumProc {
public:
	BonesDefMod *lm;
	CacheModEnumProc(BonesDefMod *l)
	{
		lm = l;
	}
private:
	BOOL proc (ModContext *mc);
};

BOOL CacheModEnumProc::proc (ModContext *mc) {
	if (mc->localData == NULL) return TRUE;

	BoneModData *bmd = (BoneModData *) mc->localData;
	bmd->CurrentCachePiece = -1;
	return TRUE;
}

/*------------------------------------------------------------------*/
/*																	*/
/*				Select Bone Dialog Mode								*/
/*																	*/
/*------------------------------------------------------------------*/

#define PERCENT_LENGTH	0.3f
#define PERCENT_LENGTH_CLOSED	0.1f

void BonesDefMod::AddBone(INode *node, BOOL update, INodeTab* modNodes)
{
	if (ip == nullptr)
		return;

	RefreshNamePicker();

	ModContextList mcList;
	INodeTab nodes;
	// get list of modcontexts, using the passed node list or using the scene selection, as appropriate
	GetModContextListNodesOrSel( this, modNodes, &mcList, &nodes );

	assert(nodes.Count());
	Matrix3 ourTM;
	ourTM = nodes[0]->GetObjectTM(RefFrame);

	Class_ID bid(BONE_CLASS_ID,0);

	BOOL staticEnvelope = FALSE;
	float staticInnerPercent = 0.;
	float staticOuterPercent = 0.;
	float staticOuter = 0.;
	float staticInner = 0.;
	pblock_param->GetValue(skin_initial_staticenvelope,0,staticEnvelope,FOREVER);
	pblock_param->GetValue(skin_initial_envelope_innerpercent,0,staticInnerPercent,FOREVER);
	pblock_param->GetValue(skin_initial_envelope_outerpercent,0,staticOuterPercent,FOREVER);
	pblock_param->GetValue(skin_initial_envelope_inner,0,staticInner,FOREVER);
	pblock_param->GetValue(skin_initial_envelope_outer,0,staticOuter,FOREVER);

	BOOL mirror = FALSE;
	pblock_mirror->GetValue(skin_mirrorenabled,0,mirror,FOREVER);
	if (mirror)
		pblock_mirror->SetValue(skin_mirrorenabled,0,FALSE);

	//need to add subcount for shapes also
	int subcount = 1;
	ObjectState os = node->EvalWorldState(RefFrame);

	for (int j = 0; j < subcount; j++)
	{
		BoneDataClass t;
		t.Node = node;
		TCHAR title[200];
		_tcscpy(title,node->GetName());

		int current=-1;
		BOOL found = FALSE;
		for (int bct = 0; bct < BoneData.Count();bct++)
		{
			if (BoneData[bct].Node == NULL)
			{
				current = bct;
				found = TRUE;
				break;
			}
		}
		if (!found)
			current = BoneData.Count();
		int BoneRefID = GetOpenID();
		int End1RefID = GetOpenID();
		int End2RefID = GetOpenID();

		if (current != -1) {
			//append a new bone
			BoneDataClass t;
			if (!found)
				BoneData.Append(t);
			BoneDataClass &boneData = BoneData[current];
			boneData.Node = NULL;
			boneData.EndPoint1Control = NULL;
			boneData.EndPoint2Control = NULL;

			BOOL isBoneObject = (os.obj->ClassID() == bid?  TRUE:FALSE);
			// Helper InitTM() sets boneData.tm, .InitObjectTM, .InitStretchTM, and .InitNodeTM
			boneData.InitTM( node, RefFrame, hasStretchTM, isBoneObject );

			for(int k = 0 ; k < mcList.Count() ; k++)
			{
				BoneModData *bmd = (BoneModData*)mcList[k]->localData;
				if (bmd->pSE)
					bmd->pSE->SetNumBones(BoneData.Count());
			}

			boneData.CrossSectionList.ZeroCount();

			Point3 l1(0.0f,0.0f,0.0f),l2(0.0f,0.0f,0.0f);

			//object is bone use its first child as the axis
			boneData.flags = 0;
			if (isBoneObject)
			{
				l1.x = 0.0f;
				l1.y = 0.0f;
				l1.z = 0.0f;
				l2.x = 0.0f;
				l2.y = 0.0f;
				l2.z = 0.0f;
				//get child node
				INode* parent = node->GetParentNode();
				Matrix3 otm = parent->GetObjectTM(RefFrame);

				for(int k = 0 ; k < mcList.Count() ; k++)
				{
					BoneModData *bmd = (BoneModData*)mcList[k]->localData;
					bmd->pSE->SetNumBones(BoneData.Count());
				}

				Matrix3 ChildTM = node->GetObjectTM(RefFrame);

				_tcscpy(title,node->GetName());

				l2 = l2 * ChildTM;
				l2 = l2 * Inverse(otm);
				Point3 Vec = (l2-l1);
				l1 += Vec * 0.1f;
				l2 -= Vec * 0.1f;
				float el1 = 9999999999999.0f,el2 = 999999999.0f;

				int objects = mcList.Count();
				Point3 ll1 = l1 * Inverse(boneData.tm);
				Point3 ll2 = l2 * Inverse(boneData.tm);

				for (int nc = 0; nc < nodes.Count(); nc++)
				{
					ObjectState base_os = nodes[nc]->EvalWorldState(ip->GetTime());
					BuildEnvelopes(nodes[nc], base_os.obj, ll1, ll2, el1, el2);
				}

				float e_inner, e_outer;
				e_inner = el1*staticInnerPercent ;
				e_outer = e_inner *staticOuterPercent;
				if (staticEnvelope)
				{
					e_inner=staticInner;
					e_outer=staticOuter;
				}

				AddCrossSection(current, 0.0f, e_inner,e_outer);
				e_inner = el2*staticInnerPercent ;
				e_outer = e_inner *staticOuterPercent;
				if (staticEnvelope)
				{
					e_inner=staticInner;
					e_outer=staticOuter;
				}

				AddCrossSection(current, 1.0f, e_inner,e_outer);
				boneData.flags = BONE_BONE_FLAG;
			}
			//object is bone use its first child as the axis
			else if (os.obj->SuperClassID()==SHAPE_CLASS_ID)
			{
				//build distance based on spline
				boneData.flags = boneData.flags|BONE_SPLINE_FLAG;
				ShapeObject *pathOb = NULL;
				ObjectState os = node->EvalWorldState(RefFrame);

				BezierShape bShape;
				ShapeObject *shape = (ShapeObject *)os.obj;
				if(shape->CanMakeBezier())
					shape->MakeBezier(RefFrame, bShape);
				else {
					PolyShape pShape;
					shape->MakePolyShape(RefFrame, pShape);
					bShape = pShape;	// UGH -- Convert it from a PolyShape -- not good!
				}

				pathOb = (ShapeObject*)os.obj;

				if (bShape.splines[0]->Closed() )
					boneData.flags = boneData.flags|BONE_SPLINECLOSED_FLAG;

				l1 = pathOb->InterpCurve3D(RefFrame, 0, 0.0f, SPLINE_INTERP_SIMPLE);
				l2 = pathOb->InterpCurve3D(RefFrame, 0, 1.0f, SPLINE_INTERP_SIMPLE);

				float el1 = 0.0f,el2 = 0.0f;
				float s1 = bShape.splines[0]-> SplineLength();

				Matrix3 tempTM = Inverse(boneData.tm)*Inverse(ourTM);
				BuildMajorAxis(node,l1,l2,el1,&tempTM);
				if (el1< 0.1f) el1 = staticInner;
				el1 += 10.0f;
				el1 = el1 * 0.5f;
				el2= el1;

				float e_inner, e_outer;
				e_inner = el1 *staticInnerPercent;
				e_outer = e_inner *staticOuterPercent;
				if (staticEnvelope)
				{
					e_inner=staticInner;
					e_outer=staticOuter;
				}

				AddCrossSection(current, 0.0f, e_inner,e_outer);
				e_inner = el1 *staticInnerPercent;
				e_outer = e_inner *staticOuterPercent;
				if (staticEnvelope)
				{
					e_inner=staticInner;
					e_outer=staticOuter;
				}

				AddCrossSection(current, 1.0f, e_inner,e_outer);

				//copy initial reference spline into our spline
				boneData.referenceSpline = *bShape.splines[0];
			}

			else
			{
				float el1 = 99999999.0f,el2 = 99999999.0f;
				Matrix3 tempTM;
				tempTM = Inverse(boneData.tm)*Inverse(ourTM);
				BuildMajorAxis(node,l1,l2,el1,&tempTM);

				if (os.obj->ClassID() == Class_ID(POINTHELP_CLASS_ID,0))
				{
					ViewExp& vpt = GetCOREInterface()->GetActiveViewExp();
					if ( ! vpt.IsAlive() )
					{
						// why are we here
						DbgAssert(!_T("Invalid viewport!"));
						return;
					}
					Box3 bounds;
					bounds.Init();
					os.obj->GetLocalBoundBox(0,node,vpt.ToPointer() , bounds ) ;
					el1 = Length(bounds.Max() - bounds.Min())*0.25f;
				}

				el1 = el1 * 0.5f;
				el2= el1;

				int objects = mcList.Count();
				Point3 ll1 = l1 * Inverse(boneData.tm);
				Point3 ll2 = l2 * Inverse(boneData.tm);

				float e_inner, e_outer;
				e_inner = el1 *staticInnerPercent;
				e_outer = e_inner *staticOuterPercent;
				if (staticEnvelope)
				{
					e_inner=staticInner;
					e_outer=staticOuter;
				}

				if (e_inner <= 0.001f) e_inner = 5.0f;
				if (e_outer <= 0.001f) e_outer = 10.f;

				if (Length(l1-l2) < 0.05f)
				{
					l1.x *= 5.0f;
					l2.x *= 5.0f;
				}
				// CHANGE BY MICHAELSON BRITT
				// Optimization to prevent AddCrossSection() from notifying dependents.
				// This is dangerous.  Last operation in a sequence must have update
				// set to true, to avoid dependency errors and bugs.
				AddCrossSection(current, 0.0f, e_inner,e_outer,update);
				e_inner = el2 *staticInnerPercent;
				e_outer = e_inner *staticOuterPercent;
				if (staticEnvelope)
				{
					e_inner=staticInner;
					e_outer=staticOuter;
				}

				if (e_inner <= 0.001f) e_inner = 5.0f;
				if (e_outer <= 0.001f) e_outer = 10.f;
				// CHANGE BY MICHAELSON BRITT
				// Optimization to prevent AddCrossSection() from notifying dependents.
				// This is dangerous.  Last operation in a sequence must have update
				// set to true, to avoid dependency errors and bugs.
				AddCrossSection(current, 1.0f, e_inner,e_outer,update);
			}

			boneData.flags = boneData.flags|BONE_ABSOLUTE_FLAG;
			boneData.FalloffType = 0;

			boneData.BoneRefID = BoneRefID;
			boneData.RefEndPt1ID = End1RefID;
			boneData.RefEndPt2ID = End2RefID;

			boneData.end1Selected = FALSE;
			boneData.end2Selected = FALSE;

			if (os.obj->ClassID() == bid)
			{
				//get child node
				INode* parent = node->GetParentNode();
				ReplaceReference(BoneRefID,parent,FALSE);
				boneData.name = title;
			}
			else ReplaceReference(BoneRefID,node,FALSE);

			INode *n = boneData.Node;
			if (n)
			{
				float squash = GetSquash(ip->GetTime(), n);
				if (current>=pblock_param->Count(skin_local_squash))
				{
					float f = 1.0f;
					pblock_param->Append(skin_local_squash,1,&f);
				}
				else
				{
					pblock_param->SetValue(skin_local_squash,ip->GetTime(),1.0f,current);
				}

				if (current>=pblock_param->Count(skin_initial_squash))
				{
					pblock_param->Append(skin_initial_squash,1,&squash);
				}
				else
				{
					pblock_param->SetValue(skin_initial_squash,ip->GetTime(),squash,current);
				}

				ReplaceReference(End1RefID,NewDefaultPoint3Controller());
				ReplaceReference(End2RefID,NewDefaultPoint3Controller());

				BOOL animate = FALSE;
				pblock_advance->GetValue(skin_advance_animatable_envelopes,0,animate,FOREVER);
				if (!animate)
				{
					SuspendAnimate();
					AnimateOff();
				}

				boneData.EndPoint1Control->SetValue(currentTime,&l1,TRUE,CTRL_ABSOLUTE);
				boneData.EndPoint2Control->SetValue(currentTime,&l2,TRUE,CTRL_ABSOLUTE);

				if (!animate)
					ResumeAnimate();

				int insertIndex = FindListInsertPos(title);
				insertIndex = SendMessage(GetDlgItem(hParam,IDC_LIST1), LB_INSERTSTRING, (WPARAM)insertIndex, (LPARAM)(TCHAR*)title);
				SendMessage(GetDlgItem(hParam,IDC_LIST1), LB_SETITEMDATA, insertIndex, current);
			}

			nodes.DisposeTemporary();
		}

		ModeBoneEndPoint = -1;
		ModeBoneEnvelopeIndex = -1;
		ModeBoneEnvelopeSubType = -1;
		SelectBoneByBoneIndex(current);

		Reevaluate(TRUE);
	}

	if (update)
	{
		// Update UI only if displayed currently
		if( editing )
		{
			if ( (BoneData.Count() >0) && (ip && ip->GetSubObjectLevel() == 1) )
			{
				EnableButtons();
			}

			BoneDataClass &boneData = BoneData[ModeBoneIndex];
			if (boneData.flags & BONE_ABSOLUTE_FLAG)
			{
				iAbsolute->SetCheck(FALSE);
			}
			else
			{
				iAbsolute->SetCheck(TRUE);
			}

			if (boneData.flags & BONE_DRAW_ENVELOPE_FLAG)
			{
				iEnvelope->SetCheck(TRUE);
			}
			else
			{
				iEnvelope->SetCheck(FALSE);
			}

			if (boneData.FalloffType == BONE_FALLOFF_X_FLAG)
				iFalloff->SetCurFlyOff(0,FALSE);
			else if (boneData.FalloffType == BONE_FALLOFF_SINE_FLAG)
				iFalloff->SetCurFlyOff(1,FALSE);
			else if (boneData.FalloffType == BONE_FALLOFF_X3_FLAG)
				iFalloff->SetCurFlyOff(3,FALSE);
			else if (boneData.FalloffType == BONE_FALLOFF_3X_FLAG)
				iFalloff->SetCurFlyOff(2,FALSE);
		}

		NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
		cacheValid = FALSE;
	}
	//WEIGHTTABLE
	weightTableWindow.RecomputeBones();
	NotifyDependents(FOREVER, 0, REFMSG_SUBANIM_STRUCTURE_CHANGED);

	BOOL shorten = FALSE;
	pblock_advance->GetValue(skin_advance_shortennames,0,shorten,FOREVER);
	if (!shorten)
	{
		HDC hdc = GetDC(GetDlgItem(hParam,IDC_LIST1));
		HFONT hOldFont = (HFONT)SelectObject(hdc, GetCOREInterface()->GetAppHFont());
		int textWidth = 0;
		for (int i=0;i<BoneData.Count();i++)
		{
			BoneDataClass &boneData = BoneData[i];
			if (boneData.Node != NULL)
			{
				TCHAR title[500];
				_tcscpy(title,boneData.Node->GetName());
				SIZE size;

				int titleLen = static_cast<int>(_tcslen(title));
				TSTR tempstr;
				tempstr.printf(_T("%s"),title);
				GetTextExtentPoint32(hdc,  (LPCTSTR)tempstr, tempstr.Length(),&size);

				if (size.cx > textWidth) textWidth = size.cx;
			}
		}
		SendDlgItemMessage(hParam, IDC_LIST1, LB_SETHORIZONTALEXTENT, (textWidth+8), 0);
		SelectObject(hdc, hOldFont);
		ReleaseDC(GetDlgItem(hParam,IDC_LIST1),hdc);
	}

	if (update)
	{
		TimeValue t = GetCOREInterface()->GetTime();
		for (int i = 0; i < nodes.Count(); i++)
		{
			if (nodes[i])
				nodes[i]->EvalWorldState(t);
		}
	}

}

BOOL BonesDefMod::AddBoneEx(INode *node, BOOL update)
{
	//make sure the node is valid
	node->BeginDependencyTest();
	NotifyDependents(FOREVER,0,REFMSG_TEST_DEPENDENCY);
	//check to make sure it is not circular
	if (node->EndDependencyTest()) return FALSE;

	//check to make sure it is not a nurbs curve
	ObjectState os = node->EvalWorldState(RefFrame);

	if (os.obj == NULL) return FALSE;

	if (os.obj->SuperClassID()==SHAPE_CLASS_ID)
	{
		if ( (os.obj->ClassID()==EDITABLE_SURF_CLASS_ID) )
			return FALSE;
		ShapeObject *pathOb = (ShapeObject*)os.obj;
		if (pathOb->NumberOfCurves(RefFrame) == 0) return FALSE;
	}

	Tab<INode*> nodes;

	MyEnumProc dep;
	DoEnumDependents(&dep);
	nodes = dep.Nodes;

	if (nodes.Count() == 0) return FALSE;

	Matrix3 ourTM;
	ourTM = nodes[0]->GetObjectTM(RefFrame);

	Class_ID bid(BONE_CLASS_ID,0);

	BOOL staticEnvelope = FALSE;
	float staticInnerPercent = 0.;
	float staticOuterPercent = 0.;
	float staticOuter = 0.;
	float staticInner = 0.;
	pblock_param->GetValue(skin_initial_staticenvelope,0,staticEnvelope,FOREVER);
	pblock_param->GetValue(skin_initial_envelope_innerpercent,0,staticInnerPercent,FOREVER);
	pblock_param->GetValue(skin_initial_envelope_outerpercent,0,staticOuterPercent,FOREVER);
	pblock_param->GetValue(skin_initial_envelope_inner,0,staticInner,FOREVER);
	pblock_param->GetValue(skin_initial_envelope_outer,0,staticOuter,FOREVER);

	BOOL mirror = FALSE;
	pblock_mirror->GetValue(skin_mirrorenabled,0,mirror,FOREVER);
	if (mirror)
		pblock_mirror->SetValue(skin_mirrorenabled,0,FALSE);

	//need to add subcount for shapes also
	int subcount = 1;
	for (int j = 0; j < subcount; j++)
	{
		BoneDataClass t;
		t.Node = node;
		TCHAR title[200];
		_tcscpy(title,node->GetName());

		int current=-1;
		BOOL found = FALSE;
		for (int bct = 0; bct < BoneData.Count();bct++)
		{
			if (BoneData[bct].Node == NULL)
			{
				current = bct;
				found = TRUE;
				break;
			}
		}
		if (!found)
			current = BoneData.Count();
		int BoneRefID = GetOpenID();
		int End1RefID = GetOpenID();
		int End2RefID = GetOpenID();

		if (current != -1) {
			Matrix3 otm = t.Node->GetObjectTM(RefFrame);
			Matrix3 stretchTM = t.Node->GetStretchTM(RefFrame);
			Matrix3 ntm = t.Node->GetNodeTM(RefFrame);

			//append a new bone
			BoneDataClass t;
			if (!found)
				BoneData.Append(t);
			BoneDataClass &boneData = BoneData[current];
			boneData.Node = NULL;
			boneData.EndPoint1Control = NULL;
			boneData.EndPoint2Control = NULL;

			BOOL isBoneObject = (os.obj->ClassID() == bid?  TRUE:FALSE);
			// Helper InitTM() sets boneData.tm, .InitObjectTM, .InitStretchTM, and .InitNodeTM
			boneData.InitTM( node, RefFrame, hasStretchTM, isBoneObject );

			boneData.CrossSectionList.ZeroCount();

			Point3 l1(0.0f,0.0f,0.0f),l2(0.0f,0.0f,0.0f);

			//object is bone use its first child as the axis
			boneData.flags = 0;
			if (isBoneObject)
			{
				l1.x = 0.0f;
				l1.y = 0.0f;
				l1.z = 0.0f;
				l2.x = 0.0f;
				l2.y = 0.0f;
				l2.z = 0.0f;
				//get child node
				INode* parent = node->GetParentNode();
				otm = parent->GetObjectTM(RefFrame);
				stretchTM = parent->GetStretchTM(RefFrame);
				ntm = parent->GetNodeTM(RefFrame);

				// Helper InitTM() sets boneData.tm, .InitObjectTM, .InitStretchTM, and .InitNodeTM
				boneData.InitTM( node, RefFrame, hasStretchTM, isBoneObject );

				Matrix3 ChildTM = node->GetObjectTM(RefFrame);

				_tcscpy(title,node->GetName());

				l2 = l2 * ChildTM;
				l2 = l2 * Inverse(otm);
				Point3 Vec = (l2-l1);
				l1 += Vec * 0.1f;
				l2 -= Vec * 0.1f;
				float el1 = 9999999999999.0f,el2 = 999999999.0f;

				Point3 ll1 = l1 * Inverse(boneData.tm);
				Point3 ll2 = l2 * Inverse(boneData.tm);

				for (int nc = 0; nc < nodes.Count(); nc++)
				{
					ObjectState base_os = nodes[nc]->EvalWorldState(GetCOREInterface()->GetTime());

					BuildEnvelopes(nodes[nc], base_os.obj, ll1, ll2, el1, el2);
				}

				float e_inner, e_outer;
				e_inner = el1*staticInnerPercent ;
				e_outer = e_inner *staticOuterPercent;
				if (staticEnvelope)
				{
					e_inner=staticInner;
					e_outer=staticOuter;
				}

				AddCrossSection(current, 0.0f, e_inner,e_outer);
				e_inner = el2*staticInnerPercent ;
				e_outer = e_inner *staticOuterPercent;
				if (staticEnvelope)
				{
					e_inner=staticInner;
					e_outer=staticOuter;
				}

				AddCrossSection(current, 1.0f, e_inner,e_outer);
				boneData.flags = BONE_BONE_FLAG;
			}
			//object is bone use its first child as the axis
			else if (os.obj->SuperClassID()==SHAPE_CLASS_ID)
			{
				//build distance based on spline
				boneData.flags = boneData.flags|BONE_SPLINE_FLAG;
				ShapeObject *pathOb = NULL;
				ObjectState os = node->EvalWorldState(RefFrame);

				BezierShape bShape;
				ShapeObject *shape = (ShapeObject *)os.obj;
				if(shape->CanMakeBezier())
					shape->MakeBezier(RefFrame, bShape);
				else {
					PolyShape pShape;
					shape->MakePolyShape(RefFrame, pShape);
					bShape = pShape;	// UGH -- Convert it from a PolyShape -- not good!
				}

				pathOb = (ShapeObject*)os.obj;

				if (bShape.splines[0]->Closed() )
					boneData.flags = boneData.flags|BONE_SPLINECLOSED_FLAG;

				l1 = pathOb->InterpCurve3D(RefFrame, 0, 0.0f, SPLINE_INTERP_SIMPLE);
				l2 = pathOb->InterpCurve3D(RefFrame, 0, 1.0f, SPLINE_INTERP_SIMPLE);

				float el1 = 0.0f,el2 = 0.0f;
				float s1 = bShape.splines[0]-> SplineLength();

				Matrix3 tempTM = Inverse(boneData.tm)*Inverse(ourTM);
				BuildMajorAxis(node,l1,l2,el1,&tempTM);
				if (el1< 0.1f) el1 = staticInner;
				el1 += 10.0f;
				el1 = el1 * 0.5f;
				el2= el1;

				float e_inner, e_outer;
				e_inner = el1 *staticInnerPercent;
				e_outer = e_inner *staticOuterPercent;
				if (staticEnvelope)
				{
					e_inner=staticInner;
					e_outer=staticOuter;
				}

				AddCrossSection(current, 0.0f, e_inner,e_outer);
				e_inner = el1 *staticInnerPercent;
				e_outer = e_inner *staticOuterPercent;
				if (staticEnvelope)
				{
					e_inner=staticInner;
					e_outer=staticOuter;
				}

				AddCrossSection(current, 1.0f, e_inner,e_outer);

				//copy initial reference spline into our spline
				boneData.referenceSpline = *bShape.splines[0];
			}
			else
			{
				float el1 = 99999999.0f,el2 = 99999999.0f;
				Matrix3 tempTM;
				tempTM = Inverse(boneData.tm)*Inverse(ourTM);
				BuildMajorAxis(node,l1,l2,el1,&tempTM);

				if (os.obj->ClassID() == Class_ID(POINTHELP_CLASS_ID,0))
				{
					ViewExp& vpt = GetCOREInterface()->GetActiveViewExp();
					if ( ! vpt.IsAlive() )
					{
						// why are we here
						DbgAssert(!_T("Invalid viewport!"));
						return FALSE;
					}
					Box3 bounds;
					bounds.Init();
					os.obj->GetLocalBoundBox(0,node,vpt.ToPointer() , bounds ) ;
					el1 = Length(bounds.Max() - bounds.Min()) * 0.25f;
				}

				el1 = el1 * 0.5f;
				el2= el1;

				Point3 ll1 = l1 * Inverse(boneData.tm);
				Point3 ll2 = l2 * Inverse(boneData.tm);

				float e_inner = el1 *staticInnerPercent;
				float e_outer = e_inner *staticOuterPercent;

				if (e_inner <= 0.001f) e_inner = 5.0f;
				if (e_outer <= 0.001f) e_outer = 10.0f;

				if (Length(l1-l2) < 0.05f)
				{
					l1.x *= 5.0f;
					l2.x *= 5.0f;
				}

				if (staticEnvelope)
				{
					e_inner=staticInner;
					e_outer=staticOuter;
				}
				AddCrossSection(current, 0.0f, e_inner,e_outer);
				e_inner = el2 *staticInnerPercent;
				e_outer = e_inner *staticOuterPercent;
				if (staticEnvelope)
				{
					e_inner=staticInner;
					e_outer=staticOuter;
				}

				if (e_inner <= 0.001f) e_inner = 5.0f;
				if (e_outer <= 0.001f) e_outer = 10.0f;

				AddCrossSection(current, 1.0f, e_inner,e_outer);
			}

			boneData.flags = boneData.flags|BONE_ABSOLUTE_FLAG;
			boneData.FalloffType = 0;

			boneData.BoneRefID = BoneRefID;
			boneData.RefEndPt1ID = End1RefID;
			boneData.RefEndPt2ID = End2RefID;

			boneData.end1Selected = FALSE;
			boneData.end2Selected = FALSE;

			if (os.obj->ClassID() == bid)
			{
				//get child node
				INode* parent = node->GetParentNode();
				ReplaceReference(BoneRefID,parent,FALSE);
				boneData.name = title;
			}
			else ReplaceReference(BoneRefID,node,FALSE);

			INode *n = boneData.Node;
			if (n)
			{
				float squash = GetSquash(GetCOREInterface()->GetTime(), n);
				if (current>=pblock_param->Count(skin_local_squash))
				{
					float f = 1.0f;
					pblock_param->Append(skin_local_squash,1,&f);
				}
				else
				{
					pblock_param->SetValue(skin_local_squash,GetCOREInterface()->GetTime(),1.0f,current);
				}

				if (current>=pblock_param->Count(skin_initial_squash))
				{
					pblock_param->Append(skin_initial_squash,1,&squash);
				}
				else
				{
					pblock_param->SetValue(skin_initial_squash,GetCOREInterface()->GetTime(),squash,current);
				}

				ReplaceReference(End1RefID,NewDefaultPoint3Controller());
				ReplaceReference(End2RefID,NewDefaultPoint3Controller());

				BOOL animate = FALSE;
				pblock_advance->GetValue(skin_advance_animatable_envelopes,0,animate,FOREVER);
				if (!animate)
				{
					SuspendAnimate();
					AnimateOff();
				}

				boneData.EndPoint1Control->SetValue(currentTime,&l1,TRUE,CTRL_ABSOLUTE);
				boneData.EndPoint2Control->SetValue(currentTime,&l2,TRUE,CTRL_ABSOLUTE);
				if (!animate)
					ResumeAnimate();

				int insertIndex = FindListInsertPos(title);
				insertIndex = SendMessage(GetDlgItem(hParam,IDC_LIST1), LB_INSERTSTRING, insertIndex, (LPARAM)title);

				SendMessage(GetDlgItem(hParam,IDC_LIST1), LB_SETITEMDATA, insertIndex, current);
			}
		}

		ModeBoneEndPoint = -1;
		ModeBoneEnvelopeIndex = -1;
		ModeBoneEnvelopeSubType = -1;
		SelectBoneByBoneIndex(current);

		Reevaluate(TRUE);
	}

	if ((ip) && (update))
	{
		if ( (BoneData.Count() >0) && (ip->GetSubObjectLevel() == 1) )
		{
			EnableButtons();
		}

		BoneDataClass &boneData = BoneData[ModeBoneIndex];
		if (boneData.flags & BONE_ABSOLUTE_FLAG)
		{
			iAbsolute->SetCheck(FALSE);
		}
		else
		{
			iAbsolute->SetCheck(TRUE);
		}

		if (boneData.flags & BONE_DRAW_ENVELOPE_FLAG)
		{
			iEnvelope->SetCheck(TRUE);
		}
		else
		{
			iEnvelope->SetCheck(FALSE);
		}

		if (boneData.FalloffType == BONE_FALLOFF_X_FLAG)
			iFalloff->SetCurFlyOff(0,FALSE);
		else if (boneData.FalloffType == BONE_FALLOFF_SINE_FLAG)
			iFalloff->SetCurFlyOff(1,FALSE);
		else if (boneData.FalloffType == BONE_FALLOFF_X3_FLAG)
			iFalloff->SetCurFlyOff(3,FALSE);
		else if (boneData.FalloffType == BONE_FALLOFF_3X_FLAG)
			iFalloff->SetCurFlyOff(2,FALSE);

		NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
		cacheValid = FALSE;
	}

	//WEIGHTTABLE
	weightTableWindow.RecomputeBones();

	NotifyDependents(FOREVER, 0, REFMSG_SUBANIM_STRUCTURE_CHANGED);

	BOOL shorten = FALSE;
	pblock_advance->GetValue(skin_advance_shortennames,0,shorten,FOREVER);
	if (!shorten)
	{
		HDC hdc = GetDC(GetDlgItem(hParam,IDC_LIST1));
		HFONT hOldFont = (HFONT)SelectObject(hdc, GetCOREInterface()->GetAppHFont());
		int textWidth = 0;
		for (int i=0;i<BoneData.Count();i++)
		{
			BoneDataClass &boneData = BoneData[i];
			if (boneData.Node != NULL)
			{
				TCHAR title[500];
				_tcscpy(title,boneData.Node->GetName());
				SIZE size;

				int titleLen = static_cast<int>(_tcslen(title));
				TSTR tempstr;
				tempstr.printf(_T("%s"),title);
				GetTextExtentPoint32(hdc,  (LPCTSTR)tempstr, tempstr.Length(),&size);

				if (size.cx > textWidth) textWidth = size.cx;
			}
		}
		SendDlgItemMessage(hParam, IDC_LIST1, LB_SETHORIZONTALEXTENT, (textWidth+8), 0);
		SelectObject(hdc, hOldFont);
		ReleaseDC(GetDlgItem(hParam,IDC_LIST1),hdc);
	}

	if (update)
	{
		TimeValue t = GetCOREInterface()->GetTime();
		for (int i = 0; i < nodes.Count(); i++)
		{
			if (nodes[i])
				nodes[i]->EvalWorldState(t);
		}
	}

	return TRUE;
}

void BonesDefMod::ReplaceBone(int boneIndex, INode *node)
{
	if( node!=NULL )
	{
		//check to make sure it is not a nurbs curve
		ObjectState os = node->EvalWorldState(RefFrame);

		Class_ID bid(BONE_CLASS_ID,0);
		BOOL isBoneObject = (os.obj->ClassID() == bid?  TRUE:FALSE);

		BoneData[boneIndex].Node = node;
		BoneData[boneIndex].InitTM( node, RefFrame, hasStretchTM, isBoneObject );
		// TODO: Any need to replace reference?
		//int refID = GetBoneRefID( boneIndex );
		//if( refID>=0 )
		//	ReplaceReference( refID, node );
	}
}


BOOL BonesDefMod::SetSkinTm(INode *skinNode, Matrix3 objectTm, Matrix3 nodeTm)
{
	//if no node bail
	if (skinNode == NULL) return FALSE;
	//get the bmd
	BoneModData *bmd = GetBMD(skinNode);
	if (bmd == NULL) return FALSE;
	bmd->BaseTM = objectTm;
	bmd->BaseNodeTM = nodeTm;
	bmd->InverseBaseTM = Inverse(bmd->BaseTM);

	return TRUE;
}
BOOL BonesDefMod::SetBoneTm(INode *boneNode, Matrix3 objectTm, Matrix3 nodeTm)
{
	if (boneNode == NULL) return FALSE;

	BOOL hit =  FALSE;
	for (int i=0; i < BoneData.Count(); i++)
	{
		BoneDataClass &boneData = BoneData[i];
		if (boneNode == boneData.Node)
		{
			boneData.InitObjectTM = objectTm;
			boneData.InitNodeTM = nodeTm;
			boneData.tm    = Inverse(objectTm);
			boneData.InitStretchTM.IdentityMatrix();
			return TRUE;
		}
	}

	return FALSE;
}

BOOL BonesDefMod::AddWeights(INode *node, int vertexID, Tab<INode*> &nodeList, Tab<float> &weights)
{
	//if no node bail
	if (node == NULL) return FALSE;
	//get the local mod data if null return false
	//check to make sure teh weight ad node list are the same size
	if (nodeList.Count() != weights.Count()) return FALSE;
	//get the bmd
	BoneModData *bmd = GetBMD(node);
	if (bmd == NULL) return FALSE;
	bmd->rebuildWeights = TRUE;
	//get bone indices from the node, if any do not exist bail
	Tab<int> boneIDList;
	boneIDList.SetCount(nodeList.Count());
	for (int i =0; i < nodeList.Count(); i++)
	{
		if (nodeList[i] == NULL) return FALSE;
		BOOL hit =  FALSE;
		for (int j=0; j < BoneData.Count(); j++)
		{
			if (nodeList[i] == BoneData[j].Node)
			{
				boneIDList[i] = j;
				hit = TRUE;
				break;
			}
		}
		if (!hit) return FALSE;
	}

	//if the vertex data tab is to small expand it
	if (vertexID >= bmd->VertexData.Count())
	{
		BoneVertexMgr boneVertexMgr;
		if (bmd->VertexData.Count() > 0)
			boneVertexMgr = bmd->VertexData[0]->GetVertexDataManager();
		else {
			boneVertexMgr = std::make_shared<BoneVertexDataManager>();
		}

		auto oldSize = bmd->VertexData.Count();
		//small performance if we adding one at a time allocate 1000 extra so we are not grinding memory
		if (oldSize == vertexID)
		{
			auto* vc = new VertexListClass();
			vc->SetVertexDataManager(vertexID, boneVertexMgr);
			vc->Modified(FALSE);
			vc->ZeroWeights();
			bmd->VertexData.Append(1,&vc,1000);
			boneVertexMgr->SetVertexDataPtr(&(bmd->VertexData));
		}
		else
		{
			bmd->VertexData.SetCount(vertexID+1);
			boneVertexMgr->SetVertexDataPtr(&(bmd->VertexData));
			for (int i = oldSize; i < bmd->VertexData.Count(); i++)
			{
				auto *vc = new VertexListClass;				
				vc->SetVertexDataManager(i, boneVertexMgr);
				vc->Modified (FALSE);
				vc->ZeroWeights();
				bmd->VertexData[i] = vc;
			}
		}
	}

	//set the vertex data to modified
	if ((vertexID >=0) && (vertexID < bmd->VertexData.Count()))
	{
		bmd->reevaluate = TRUE;
		bmd->VertexData[vertexID]->Modified(TRUE);
		bmd->VertexData[vertexID]->SetWeightCount(boneIDList.Count());
		for (int i = 0; i < boneIDList.Count(); i++)
		{
			//set the weight list
			bmd->VertexData[vertexID]->SetWeightInfo(i,boneIDList[i],weights[i],weights[i]);

			// note that we always want to set all member data so it doesn't contain whatever happened to be in memory
			// due to the SetWeightCount call above. The tab being set is a Tab<VertexInfluenceListClass> and is 
			// written to the scene file. See MAXX-46135
			VertexInfluenceListClass td;
			if (BoneData[boneIDList[i]].flags & BONE_SPLINE_FLAG)
			{
				Interval valid;
				Matrix3 ntm = BoneData[boneIDList[i]].Node->GetObjTMBeforeWSM(RefFrame,&valid);
				ntm = bmd->BaseTM * Inverse(ntm);

				float garbage = SplineToPoint(bmd->VertexData[vertexID]->LocalPos,
					&BoneData[boneIDList[i]].referenceSpline,
					td.u,
					td.OPoints,td.Tangents,
					td.SubCurveIds,td.SubSegIds,
					ntm);
			}
			bmd->VertexData[vertexID]->SetCurveID(i,td.SubCurveIds);
			bmd->VertexData[vertexID]->SetSegID(i,td.SubSegIds);
			bmd->VertexData[vertexID]->SetCurveU(i,td.u);

			bmd->VertexData[vertexID]->SetOPoint(i,td.OPoints);
			bmd->VertexData[vertexID]->SetTangent(i,td.Tangents);
		}
	}
	else return FALSE;

	return TRUE;
}

BOOL BonesDefMod::SetBoneStretchTm(INode *boneNode, Matrix3 stretchTm)
{
	if (boneNode == NULL) return FALSE;

	BOOL hit =  FALSE;
	for (int i=0; i < BoneData.Count(); i++)
	{
		BoneDataClass &boneData = BoneData[i];
		if (boneNode == boneData.Node)
		{
			boneData.InitStretchTM = stretchTm;
			return TRUE;
		}
	}

	return FALSE;
}

Matrix3 BonesDefMod::GetBoneStretchTm(INode *boneNode)
{
	if (boneNode == NULL) return FALSE;

	Matrix3 tm(1);
	BOOL hit =  FALSE;
	for (int i=0; i < BoneData.Count(); i++)
	{
		BoneDataClass &boneData = BoneData[i];
		if (boneNode == boneData.Node)
		{
			return boneData.InitStretchTM;
		}
	}

	return tm;
}

//Hit Dialog
void DumpHitDialog::proc(INodeTab &nodeTab)
{
	int nodeCount = nodeTab.Count();

	if (nodeCount == 0) return;

	for (int i=0;i<nodeTab.Count();i++)
	{
		eo->AddBone(nodeTab[i], FALSE);
		macroRecorder->FunctionCall(_T("skinOps.addBone"), 3,0, mr_reftarg, eo,
			mr_reftarg, nodeTab[i],
			mr_int, 1
			);
		macroRecorder->EmitScript();
	}

	if ( (eo->BoneData.Count() >0) && (eo->ip && eo->ip->GetSubObjectLevel() == 1) )
	{
		eo->EnableButtons();
	}

	if (eo->BoneData[eo->ModeBoneIndex].flags & BONE_ABSOLUTE_FLAG)
	{
		eo->iAbsolute->SetCheck(FALSE);
	}
	else
	{
		eo->iAbsolute->SetCheck(TRUE);
	}

	if (eo->BoneData[eo->ModeBoneIndex].flags & BONE_DRAW_ENVELOPE_FLAG)
	{
		eo->iEnvelope->SetCheck(TRUE);
	}
	else
	{
		eo->iEnvelope->SetCheck(FALSE);
	}

	if (eo->BoneData[eo->ModeBoneIndex].FalloffType == BONE_FALLOFF_X_FLAG)
		eo->iFalloff->SetCurFlyOff(0,FALSE);
	else if (eo->BoneData[eo->ModeBoneIndex].FalloffType == BONE_FALLOFF_SINE_FLAG)
		eo->iFalloff->SetCurFlyOff(1,FALSE);
	else if (eo->BoneData[eo->ModeBoneIndex].FalloffType == BONE_FALLOFF_X3_FLAG)
		eo->iFalloff->SetCurFlyOff(3,FALSE);
	else if (eo->BoneData[eo->ModeBoneIndex].FalloffType == BONE_FALLOFF_3X_FLAG)
		eo->iFalloff->SetCurFlyOff(2,FALSE);

	eo->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	CacheModEnumProc lmdproc(eo);
	eo->EnumModContexts(&lmdproc);

	eo->cacheValid = FALSE;
}

int DumpHitDialog::filter(INode *node)
{
	TCHAR name1[200];
	_tcscpy(name1,node->GetName());

	node->BeginDependencyTest();
	eo->NotifyDependents(FOREVER,0,REFMSG_TEST_DEPENDENCY);
	if (node->EndDependencyTest())
	{
		return FALSE;
	}
	else if (node->GetTMController()->ClassID() == IKCHAINCONTROL_CLASS_ID)
	{
		return FALSE;
	}
	else
	{
		ObjectState os = node->GetObjectRef()->Eval(0);

		Class_ID bid(BONE_CLASS_ID,0);
		for (int i = 0;i < eo->BoneData.Count(); i++)
		{
			BoneDataClass &boneData = eo->BoneData[i];
			if (boneData.Node)
			{
				ObjectState bos = boneData.Node->EvalWorldState(0);

				if ( (node == boneData.Node) &&
					(os.obj->ClassID() != bid)  )
					return FALSE;
			}
		}

		if (os.obj == NULL)   return FALSE;

		if (os.obj->ClassID() == bid)
		{
			int found = SendMessage(GetDlgItem(eo->hParam,IDC_LIST1), LB_FINDSTRING,(WPARAM) 0,(LPARAM)(TCHAR*)name1);
			if (found != LB_ERR ) return FALSE;
		}

		//check for end nodes
		if (os.obj->ClassID() == bid)
		{
			//get parent if
			INode* parent = node->GetParentNode();
			if (parent == NULL) return FALSE;
			if (parent->IsRootNode()) return FALSE;

			ObjectState pos = parent->EvalWorldState(0);
			if (pos.obj->ClassID() != bid)  return FALSE;
		}
		if (os.obj->SuperClassID()==SHAPE_CLASS_ID)
		{
			if ( (os.obj->ClassID()==EDITABLE_SURF_CLASS_ID)
				)
				return FALSE;
			ShapeObject *pathOb = (ShapeObject*)os.obj;
			if (pathOb->NumberOfCurves(GetCOREInterface()->GetTime()) == 0) return FALSE;
		}
	}

	return TRUE;
}

static INT_PTR CALLBACK DeleteDlgProc(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BonesDefMod *mod = DLGetWindowLongPtr<BonesDefMod*>(hWnd);

	switch (msg) {
	case WM_INITDIALOG:
		{
			mod = (BonesDefMod*)lParam;
			DLSetWindowLongPtr(hWnd, lParam);

			for (int i=0; i < mod->BoneData.Count(); i++)
			{
				const TCHAR *temp = mod->GetBoneName(i);
				if (temp)
				{
					SendMessage(GetDlgItem(hWnd,IDC_LIST1), LB_ADDSTRING,0,(LPARAM)temp);
				}
			}
			break;
		}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			{
				// removeList contains boneID values. IDC_LIST1 contains unsorted list, so boneID values
				int listCt = SendMessage(GetDlgItem(hWnd,IDC_LIST1), LB_GETCOUNT,0,0);
				int selCt =  SendMessage(GetDlgItem(hWnd,IDC_LIST1), LB_GETSELCOUNT ,0,0);
				int *selList = new int[selCt];

				SendMessage(GetDlgItem(hWnd,IDC_LIST1), LB_GETSELITEMS  ,(WPARAM) selCt,(LPARAM) selList);
				mod->removeList.SetCount(selCt);
				for (int i=0; i < selCt; i++)
				{
					mod->removeList[i] = selList[i];
				}
				delete [] selList;

				EndDialog(hWnd,1);
				break;
			}
		case IDCANCEL:
			mod->removeList.ZeroCount();
			EndDialog(hWnd,0);
			break;
		}
		break;

	case WM_CLOSE:
		mod->removeList.ZeroCount();
		EndDialog(hWnd, 0);
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

BOOL PickControlNode::Filter(INode *node)
{
	node->BeginDependencyTest();
	mod->NotifyDependents(FOREVER,0,REFMSG_TEST_DEPENDENCY);
	if (node->EndDependencyTest())
	{
		return FALSE;
	}
	else if (node->GetTMController()->ClassID() == IKCHAINCONTROL_CLASS_ID)
	{
		return FALSE;
	}
	else
	{
		for (int i =0; i < mod->BoneData.Count(); i++)
		{
			if (mod->BoneData[i].Node == node)
				return FALSE;
		}
		ObjectState os = node->EvalWorldState(0);
		if (os.obj->SuperClassID()==SHAPE_CLASS_ID)
		{
			if ((os.obj->ClassID()==EDITABLE_SURF_CLASS_ID))
				return FALSE;

			ShapeObject *pathOb = (ShapeObject*)os.obj;
			if (pathOb->NumberOfCurves(GetCOREInterface()->GetTime()) == 0) return FALSE;
		}
		return TRUE;
	}
	return TRUE;
}

BOOL PickControlNode::HitTest(
	IObjParam *ip,HWND hWnd,ViewExp * /*vpt*/,IPoint2 m,int flags)
{
	if (ip->PickNode(hWnd,m,this))
		return TRUE;
	return FALSE;
}

BOOL PickControlNode::Pick(IObjParam *ip,ViewExp *vpt)
{
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	INode *node = vpt->GetClosestHit();
	if (node)
	{
		theHold.Begin();

		theHold.Put(new AddBoneRestore(mod));

		BMDModEnumProc lmdproc(mod);
		mod->EnumModContexts(&lmdproc);

		for ( int i = 0; i < lmdproc.bmdList.Count(); i++ )
		{
			BoneModData *bmd = (BoneModData*)lmdproc.bmdList[i];
			theHold.Put(new WeightRestore(mod,bmd));
		}

		mod->AddBone(node,TRUE);

		theHold.Accept(GetString(IDS_PW_ADDBONE));

		if (ip)
		{
			if ( (mod->BoneData.Count() >0) && (mod->ip && mod->ip->GetSubObjectLevel() == 1) )
			{
				mod->EnableButtons();
			}

			if (mod->IsValidBoneIndex(mod->ModeBoneIndex))
				EnableWindow(GetDlgItem(mod->hParam,IDC_REMOVE),TRUE);

			BoneDataClass &boneData = mod->BoneData[mod->ModeBoneIndex];
			if (boneData.flags & BONE_ABSOLUTE_FLAG)
			{
				mod->iAbsolute->SetCheck(FALSE);
			}
			else
			{
				mod->iAbsolute->SetCheck(TRUE);
			}

			if (boneData.flags & BONE_DRAW_ENVELOPE_FLAG)
			{
				mod->iEnvelope->SetCheck(TRUE);
			}
			else
			{
				mod->iEnvelope->SetCheck(FALSE);
			}

			if (boneData.FalloffType == BONE_FALLOFF_X_FLAG)
				mod->iFalloff->SetCurFlyOff(0,FALSE);
			else if (boneData.FalloffType == BONE_FALLOFF_SINE_FLAG)
				mod->iFalloff->SetCurFlyOff(1,FALSE);
			else if (boneData.FalloffType == BONE_FALLOFF_X3_FLAG)
				mod->iFalloff->SetCurFlyOff(3,FALSE);
			else if (boneData.FalloffType == BONE_FALLOFF_3X_FLAG)
				mod->iFalloff->SetCurFlyOff(2,FALSE);

			mod->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
			CacheModEnumProc lmdproc(mod);
			mod->EnumModContexts(&lmdproc);

			mod->cacheValid = FALSE;
		}

		if (mod->ip)
			mod->ip->RedrawViews(mod->ip->GetTime());
	}
	return FALSE;
}

void PickControlNode::EnterMode(IObjParam *ip)
{
}

HCURSOR PickControlNode::GetDefCursor(IObjParam *ip)
{
	static HCURSOR hCur = NULL;

	if ( !hCur )
	{
		hCur =  UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::HitCursor);
	}
	return hCur;
}

HCURSOR PickControlNode::GetHitCursor(IObjParam *ip)
{
	static HCURSOR hCur = NULL;

	if ( !hCur )
	{
		hCur = UI::MouseCursors::LoadMouseCursor(UI::MouseCursors::HitCursor);
	}
	return hCur;
}

void PickControlNode::ExitMode(IObjParam *ip)
{
	mod->inAddBoneMode = FALSE;
	mod->ResetSelection();
}

