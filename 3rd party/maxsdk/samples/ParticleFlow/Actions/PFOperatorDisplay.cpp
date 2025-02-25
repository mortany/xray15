/**********************************************************************
 *<
	FILE: PFOperatorDisplay.h

	DESCRIPTION: Viewport/Render Operator implementation
				 The Operator is used to define appearance of the particles
				 in the current Event (or globally if it's a global
				 Operator) for viewports

	CREATED BY: Oleg Bayborodin

	HISTORY: created 11-06-02

 *>	Copyright (c) 2001, All Rights Reserved.
 **********************************************************************/

#include "resource.h"
#include "max.h"
#include "iparamm2.h"

#include "PFActions_SysUtil.h"
#include "PFActions_GlobalFunctions.h"
#include "PFActions_GlobalVariables.h"

#include "PFOperatorDisplay.h"

#include "PFOperatorDisplay_ParamBlock.h"
#include "IParticleChannels.h"
#include "IChannelContainer.h"
#include "IParticleGroup.h"
#include "PFMessages.h"
#include "IPViewManager.h"
#include "ShapeShareSplitter.h"

#include "Graphics/IDisplayManager.h"
#include "Graphics/VertexBufferHandle.h"
#include "Graphics/IMeshDisplay2.h"
#include "Graphics/SolidColorMaterialHandle.h"
#include "Graphics/GeometryRenderItemHandle.h"
#include "Graphics/CustomRenderItemHandle.h"
#include "PFBoxGeometry.h"
#include <vector>
#include <map>

namespace PFActions {

// Display operator creates a particle channel to store a successive order number
// when a particle appeares in the event. The number is used to determine if
// the particle is visible given the current visibility percent
#define PARTICLECHANNELVISIBLER_INTERFACE Interface_ID(0x2de61303, 0x1eb34500) 
#define PARTICLECHANNELVISIBLEW_INTERFACE Interface_ID(0x2de61303, 0x1eb34501) 
#define GetParticleChannelVisibleRInterface(obj) ((IParticleChannelIntR*)obj->GetInterface(PARTICLECHANNELVISIBLER_INTERFACE)) 
#define GetParticleChannelVisibleWInterface(obj) ((IParticleChannelIntW*)obj->GetInterface(PARTICLECHANNELVISIBLEW_INTERFACE)) 
#define COLOR_ARGB(a,r,g,b) ((DWORD)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))

class PFOperatorDisplayExtensionHelper : public IPFViewportExtension2
{
private:
	friend class PFOperatorDisplay;

	PFOperatorDisplayExtensionHelper(PFOperatorDisplay* pOwner):
		mpOwner(pOwner)
	{}
	virtual ~PFOperatorDisplayExtensionHelper() {}

	virtual bool UpdatePerNodeItems(
		const MaxSDK::Graphics::GenerateMeshRenderItemsContext& generateRenderItemsContext,
		const MaxSDK::Graphics::UpdateDisplayContext& updateDisplayContext,
		MaxSDK::Graphics::UpdateNodeContext& nodeContext,
		MaxSDK::Graphics::IRenderItemContainer& targetRenderItemContainer, 
		IObject*		pCont, 
		Object*			pSystem,
		INode*			psNode);

	virtual BaseInterface* GetInterface(Interface_ID id);

	virtual void DeleteInterface();

	PFOperatorDisplay* mpOwner;
};

void PFOperatorDisplayExtensionHelper::DeleteInterface()
{
	delete this;
}

BaseInterface* PFOperatorDisplayExtensionHelper::GetInterface(Interface_ID id)
{
	if (id == IPFVIEWPORT_EXTENSION2_INTERFACE_ID)
	{
		return this;
	}

	return mpOwner->GetInterface(id);
}
typedef std::vector<int> ParticleIndexArray;
struct ConsolidationData
{
	Matrix3* WorldMatrices;
	ParticleIndexArray ParticleIndices;
	size_t numInstances;
};
struct MeshInstanceData
{
	MaxSDK::Graphics::IMeshDisplay2* pMeshDisplay;
	MaxSDK::Graphics::VertexBufferHandle vertexBufferHandle;
	MeshInstanceData(MaxSDK::Graphics::IMeshDisplay2* pMeshDisplay2,
		MaxSDK::Graphics::VertexBufferHandle& vertexBuffer) :
		pMeshDisplay(pMeshDisplay2),
		vertexBufferHandle(vertexBuffer)
	{};
};
typedef std::map<Mesh*, ConsolidationData> ConsolidationMapping;
typedef std::vector<MeshInstanceData> InstanceDataArray;

bool CanUseInstance(ConsolidationData& instData)
{
	return instData.numInstances > 10;
}

void MergeMeshes(Mesh& outMesh, ConsolidationMapping& srcMeshes, Matrix3& nodeMatrix, const IParticleChannelIntR* chMtlIndex, bool bHasMap, int maxNumMaps)
{
	nodeMatrix.Invert();
	int numVertsAdd = 0;
	int numFacesAdd = 0;
	int numTVertsAdd[MAX_MESHMAPS] = { 0 };
	int numTFacesAdd[MAX_MESHMAPS] = { 0 };

	for (auto it : srcMeshes)
	{
		if (CanUseInstance(it.second))
		{
			continue;
		}

		Mesh* pMesh = it.first;
		ConsolidationData& instData = it.second;
		for (int i = 0; i < instData.numInstances; ++i)
		{
			Point3* destVerts = outMesh.verts + numVertsAdd;
			Matrix3 matWorld = instData.WorldMatrices[i] * nodeMatrix;
			matWorld.TransformPoints(pMesh->verts, destVerts, pMesh->numVerts);
			Face* destFaces = outMesh.faces + numFacesAdd;
			bool bOverrideMtlID = false;
			MtlID mtlID = 0;
			if (chMtlIndex && !chMtlIndex->IsGlobal())
			{
				mtlID = chMtlIndex->GetValue(instData.ParticleIndices[i]);
				bOverrideMtlID = true;
			}
			for (int j = 0; j< pMesh->numFaces; ++j)
			{
				Face& srcFace = pMesh->faces[j];
				destFaces[j].v[0] = srcFace.v[0] + numVertsAdd;
				destFaces[j].v[1] = srcFace.v[1] + numVertsAdd;
				destFaces[j].v[2] = srcFace.v[2] + numVertsAdd;
				destFaces[j].flags = srcFace.flags;
				destFaces[j].smGroup = srcFace.smGroup;
				if (bOverrideMtlID)
				{
					destFaces[j].setMatID(mtlID);
				}
			}
			numVertsAdd += pMesh->numVerts;
			numFacesAdd += pMesh->numFaces;
			if (bHasMap)
			{
				for (int imap = -NUM_HIDDENMAPS; imap < maxNumMaps; ++imap)
				{
					if (outMesh.mapFaces(imap))
					{
						TVFace* destMap = outMesh.mapFaces(imap) + numTFacesAdd[imap + NUM_HIDDENMAPS];
						if(pMesh->mapFaces(imap))
						{
							TVFace* srcMap = pMesh->mapFaces(imap);
							for (int j = 0, jend = pMesh->numFaces; j < jend; ++j)
							{
								destMap[j].t[0] = srcMap[j].t[0] + numTVertsAdd[imap + NUM_HIDDENMAPS];
								destMap[j].t[1] = srcMap[j].t[1] + numTVertsAdd[imap + NUM_HIDDENMAPS];
								destMap[j].t[2] = srcMap[j].t[2] + numTVertsAdd[imap + NUM_HIDDENMAPS];
							}
						}
						else
						{
							memset(destMap, 0, sizeof(TVFace) * pMesh->numFaces);
						}
						numTFacesAdd[imap + NUM_HIDDENMAPS] += pMesh->numFaces;
					}
					if (outMesh.mapVerts(imap) && pMesh->mapVerts(imap))
					{ 
						UVVert* destUVVerts = outMesh.mapVerts(imap) + numTVertsAdd[imap + NUM_HIDDENMAPS];
						memcpy(destUVVerts,
							pMesh->mapVerts(imap),
							pMesh->getNumMapVerts(imap) * sizeof(UVVert));
						numTVertsAdd[imap + NUM_HIDDENMAPS] += pMesh->getNumMapVerts(imap);
					}
				}
			}
		}
	}
}

void BuildMeshAndData(Mesh& outMesh,
	InstanceDataArray& outInstances,
	ConsolidationMapping& srcMeshes,
	Matrix3 nodeMatrix,
	ShapeShareSplitter& shapeSplitter, 
	const IParticleChannelIntR* chMtlIndex,
	const MaxSDK::Graphics::GenerateMeshRenderItemsContext& generateRenderItemsContext)
{
	int totalVerts = 0;
	int totalFaces = 0;
	int maxNumMaps = 0;
	int totalMapVerts[MAX_MESHMAPS] = { 0 };
	bool bHasMap = false;
	for (auto it : srcMeshes)
	{
		if (CanUseInstance(it.second))
		{
			MaxSDK::Graphics::VertexBufferHandle vertexBufferHandle;
			if (shapeSplitter.GetUVDataCount() == 0)
			{
				vertexBufferHandle = MaxSDK::Graphics::GenerateInstanceData(it.second.WorldMatrices, it.second.numInstances);
			}
			else
			{
				vertexBufferHandle = MaxSDK::Graphics::GenerateInstanceData(shapeSplitter.GetUVData(), it.second.WorldMatrices, it.second.numInstances);
			}
			MaxSDK::Graphics::IMeshDisplay2* pMeshDisplay2 = static_cast<MaxSDK::Graphics::IMeshDisplay2*>(it.first->GetInterface(IMesh_DISPLAY2_INTERFACE_ID));
			if (NULL != pMeshDisplay2)
			{
				pMeshDisplay2->PrepareDisplay(generateRenderItemsContext);
			}
			MeshInstanceData meshImp(pMeshDisplay2, vertexBufferHandle);
			outInstances.push_back(meshImp);
		}
		else
		{
			//Summarize mesh resources so that we can only allocate memory once
			const Mesh* pMesh = it.first;
			if (pMesh)
			{
				totalVerts += (int)(pMesh->numVerts * it.second.numInstances);
				totalFaces += (int)(pMesh->numFaces * it.second.numInstances);
				maxNumMaps = max(maxNumMaps, pMesh->getNumMaps());
				for (int i = -NUM_HIDDENMAPS; i < maxNumMaps; ++i)
				{
					if (!pMesh->mapSupport(i))
					{
						continue;
					}
					bHasMap = true;
					const int numMapVerts = pMesh->getNumMapVerts(i);
					totalMapVerts[i + NUM_HIDDENMAPS] += (int)(numMapVerts * it.second.numInstances);
				}
			}
		}
	}
	if (totalVerts == 0
		|| totalFaces == 0)
	{
		return;
	}
	outMesh.setNumVerts(totalVerts);
	outMesh.setNumFaces(totalFaces);
	for (int i = -NUM_HIDDENMAPS; i < maxNumMaps; ++i)
	{
		if (totalMapVerts[i + NUM_HIDDENMAPS] > 0)
		{
			outMesh.setMapSupport(i);
			outMesh.setNumMapVerts(i, totalMapVerts[i + NUM_HIDDENMAPS]);
		}
	}
	MergeMeshes(outMesh, srcMeshes, nodeMatrix, chMtlIndex, bHasMap, maxNumMaps);
}

bool PFOperatorDisplayExtensionHelper::UpdatePerNodeItems(
	const MaxSDK::Graphics::GenerateMeshRenderItemsContext& generateRenderItemsContext,
	const MaxSDK::Graphics::UpdateDisplayContext& updateDisplayContext,
	MaxSDK::Graphics::UpdateNodeContext& nodeContext,
	MaxSDK::Graphics::IRenderItemContainer& targetRenderItemContainer,
	IObject* pCont, 
	Object* pSystem,
	INode* psNode)
{
	using namespace MaxSDK::Graphics;

	int displayType = mpOwner->pblock()->GetInt(kDisplay_type);
	int selectedType = mpOwner->pblock()->GetInt(kDisplay_selectedType);
	int showNum = mpOwner->pblock()->GetInt(kDisplay_showNumbering);
	float visPercent = GetPFFloat(mpOwner->pblock(), kDisplay_visible, updateDisplayContext.GetDisplayTime());

	DbgAssert(pCont); DbgAssert(pSystem); 
	DbgAssert(psNode);
	if (psNode == nullptr) return 0;
	if (pCont == NULL) return 0;
	if (pSystem == NULL) return 0;
	INode* pgNode = nodeContext.GetRenderNode().GetMaxNode();
	IPFSystem* iSystem = PFSystemInterface(pSystem);
	if (iSystem == NULL) return 0; // invalid particle system

	// all particles are visible in 'select particles' mode
	bool subSelPartMode = (psNode->Selected() && (pSystem->GetSubselState() == 1));
	if (subSelPartMode) {
		if (displayType == kDisplay_type_none)
			displayType = kDisplay_type_dots;
		if (selectedType == kDisplay_type_none)
			selectedType = kDisplay_type_dots;
		visPercent = 1.0f;
	}
	if ((displayType == kDisplay_type_none) && (selectedType == kDisplay_type_none))
		return 0;

	// acquire particle channels
	IParticleChannelAmountR* chAmount = GetParticleChannelAmountRInterface(pCont);
	if (chAmount == NULL) return 0; // can't find number of particles in the container
	const int count = chAmount->Count();
	if (count == 0) return 0;		// there no particles to draw
	int visCount = int(visPercent * count);
	if (visCount <= 0) return 0;	// there no particles to draw
	IParticleChannelPoint3R* chPos = GetParticleChannelPositionRInterface(pCont);	
	if (chPos == NULL) return 0;	// can't find particle position in the container
	IParticleChannelPoint3R* chSpeed = NULL;
	if ((displayType == kDisplay_type_lines) || (selectedType == kDisplay_type_lines))
	{
		chSpeed = GetParticleChannelSpeedRInterface(pCont);
		// if no speed channel then change drawing type to dots
		if (chSpeed == NULL) {
			if (displayType == kDisplay_type_lines)
				displayType = kDisplay_type_dots;
			if (selectedType == kDisplay_type_lines)
				selectedType = kDisplay_type_dots;
		}
	}
	IParticleChannelIDR* chID = GetParticleChannelIDRInterface(pCont);
	// if no ID channel then it's not possible to show numbers & selections
	if (chID == NULL) showNum = 0;
	
	IParticleChannelQuatR* chOrient = NULL;
	IParticleChannelPoint3R* chScale = NULL;
	IParticleChannelMeshR* chShape = NULL;
	IParticleChannelIntR* chMtlIndex = NULL;
	IParticleChannelMeshMapR* chMapping = NULL;

	if ((displayType == kDisplay_type_boundingBoxes) ||
		(displayType == kDisplay_type_geometry) ||
		(selectedType == kDisplay_type_boundingBoxes) ||
		(selectedType == kDisplay_type_geometry))
	{
		chOrient = GetParticleChannelOrientationRInterface(pCont);
		chScale = GetParticleChannelScaleRInterface(pCont);
		chShape = GetParticleChannelShapeRInterface(pCont);
		// if no shape channel then change drawing type to X marks
		if (chShape == NULL) displayType = kDisplay_type_Xs;
		chMtlIndex = GetParticleChannelMtlIndexRInterface(pCont);
		chMapping = GetParticleChannelShapeTextureRInterface(pCont);
	}

	IChannelContainer* chCont = GetChannelContainerInterface(pCont);
	if (chCont == NULL) return false;
	IParticleChannelIntR* chVisibleR = (IParticleChannelIntR*)chCont->GetPrivateInterface(PARTICLECHANNELVISIBLER_INTERFACE, (Object*)this);
	IParticleChannelBoolR* chVisOpR = (IParticleChannelBoolR*)chCont->GetPrivateInterface(PARTICLECHANNELVISIBILITYOPR_INTERFACE, (Object*)this);
	IParticleChannelBoolR* chVisVpR = GetParticleChannelVisibilityVPRInterface(pCont);

	int i, index, numMtls=1;

	Tab<int> blockNon, blockSel;
	int selNum=0, nonNum=0;
	IParticleChannelBoolR* chSelect = GetParticleChannelSelectionRInterface(pCont);
	blockNon.SetCount(count); blockSel.SetCount(count);
	bool doVisibleFilter = ((visPercent < 1.0f) && (chVisibleR != NULL));
	bool checkVisibility = doVisibleFilter || (chVisOpR != NULL) || (chVisVpR != NULL);
	if (chSelect != NULL) 
	{
		if (!checkVisibility)
		{
			for(i=0; i<count; i++) 
			{
				if (chSelect->GetValue(i))
					blockSel[selNum++] = i;
				else
					blockNon[nonNum++] = i;
			}
		}
		else
		{
			for(i=0; i<count; i++) 
			{
				if (doVisibleFilter)
					if (mpOwner->isInvisible(chVisibleR->GetValue(i))) 
						continue;
				if (chVisOpR && chVisOpR->GetValue(i))
					continue;
				if (chVisVpR && chVisVpR->GetValue(i))
					continue;
				if (chSelect->GetValue(i))
					blockSel[selNum++] = i;
				else
					blockNon[nonNum++] = i;
			}
		}
	} else {
		if (!checkVisibility)
		{
			for(i=0; i<count; i++) 
				blockNon[nonNum++] = i;
		}
		else
		{
			for(i=0; i<count; i++) 
			{
				if (doVisibleFilter)
					if (mpOwner->isInvisible(chVisibleR->GetValue(i))) 
						continue;
				if (chVisOpR && chVisOpR->GetValue(i))
					continue;
				if (chVisVpR && chVisVpR->GetValue(i))
					continue;
				blockNon[nonNum++] = i;
			}
		}
	}
	blockNon.SetCount(nonNum);
	blockSel.SetCount(selNum);

	// define color for particles
	Color primColor, selColor;
	Color subselColor = GetColorManager()->GetColorAsPoint3(PFSubselectionColorId);

	if (psNode->Selected()
		|| pgNode->Selected()) {
		switch (pSystem->GetSubselState())
		{
		case 0:
			primColor = Color(GetSelColor());
			selColor = Color(GetSelColor());
			break;
		case 1:
			primColor = mpOwner->pblock()->GetColor(kDisplay_color);
			selColor = Color(subselColor);
			break;
		case 2: {
			primColor = mpOwner->pblock()->GetColor(kDisplay_color);
			selColor = mpOwner->pblock()->GetColor(kDisplay_color);
			IParticleGroup* iPGroup = ParticleGroupInterface(pgNode);
			if (iPGroup != NULL)
				if (iSystem->IsActionListSelected(iPGroup->GetActionList())) {
					primColor = Color(subselColor);
					selColor = Color(subselColor);
				}
		}
			break;
		}
	} else if (psNode->IsFrozen()) {
		primColor = Color(GetFreezeColor());
		selColor = Color(GetFreezeColor());
	} else {
		primColor = mpOwner->pblock()->GetColor(kDisplay_color);
		selColor = mpOwner->pblock()->GetColor(kDisplay_color);
	}

	//TODO: Nitrous
	Material* nodeMtl = pgNode->Mtls();
	
	Material vpMtl;
	Point3 pos[33], speed;

	enum { noneCat, markerCat, lineCat, geomCat };
	int ordCat, selCat;
	switch (displayType) {
	case kDisplay_type_boundingBoxes:
	case kDisplay_type_geometry:
		ordCat = geomCat;
		break;
	case kDisplay_type_lines:
		ordCat = lineCat;
		break;
	case kDisplay_type_none:
		ordCat = noneCat;
		break;
	default:
		ordCat = markerCat;
	}
	switch (selectedType) {
	case kDisplay_type_boundingBoxes:
	case kDisplay_type_geometry:
		selCat = geomCat;
		break;
	case kDisplay_type_lines:
		selCat = lineCat;
		break;
	case kDisplay_type_none:
		selCat = noneCat;
		break;
	default:
		selCat = markerCat;
	}

	if (pgNode->GetMtl() != nullptr
		&& pgNode->GetMtl()->IsMultiMtl()) {
		numMtls = pgNode->GetMtl()->NumSubMtls();
	}
	else {
		numMtls = 1;
	}

	InstanceDataArray instanceData;
	ShapeShareSplitter shapeSplitter(count);
	shapeSplitter.Init(chShape, chMtlIndex, chMapping, numMtls);
	Matrix3* matrixPool = (Matrix3*)malloc(sizeof(Matrix3)* (blockSel.Count() + blockNon.Count()));
	size_t usedPoolSlots = 0;
	if ((ordCat == geomCat) || (selCat == geomCat))
	{
		vpMtl = *(SysUtil::GetParticleMtl());
		for(int runType = 0; runType < 2; runType++)
		{
			Tab<int> *block = NULL;
			if (runType) {
				if (selNum == 0) continue;
				if (selCat != geomCat) continue;

				vpMtl.Kd = vpMtl.Ks = selColor;
				block = &blockSel;
			} else {
				if (nonNum == 0) continue;
				if (ordCat != geomCat) continue;
				block = &blockNon;
			}
			std::map<Mesh*, size_t> InstanceCounter;
			int blockCount = block->Count();
			for (i = 0; i < blockCount; ++i)
			{
				index = (*block)[i];
				Mesh* pMesh = shapeSplitter.GetShape(index);
				auto it = InstanceCounter.find(pMesh);
				if (it == InstanceCounter.end())
				{
					InstanceCounter[pMesh] = 1;
				}
				else
				{
					it->second++;
				}
			}
			ConsolidationMapping meshParts;
			for (auto it : InstanceCounter)
			{
				ConsolidationData& data = meshParts[it.first];
				data.WorldMatrices = matrixPool + usedPoolSlots;
				data.numInstances = 0;
				usedPoolSlots += it.second;
			}
			//We use mesh pointer to identify different meshes
			//Only the meshes with same source pointer can be
			//collected and rendered with instancing
			
			for(i=0; i<blockCount; ++i) 
			{
				index = (*block)[i];
				Mesh* pMesh = shapeSplitter.GetShape(index);
				if (pMesh) 
				{
					ConsolidationData& instData = meshParts[pMesh];
					instData.ParticleIndices.push_back(index);
					Matrix3& gwTM = *(instData.WorldMatrices + instData.numInstances);
					gwTM.IdentityMatrix();
					if (chOrient) 
						gwTM.SetRotate(chOrient->GetValue(index));
					if (chScale) 
						gwTM.PreScale(chScale->GetValue(index));
					gwTM.SetTrans(chPos->GetValue(index));
					instData.numInstances++;
				}
			}
			if (displayType == kDisplay_type_boundingBoxes) 
			{
				static MaxSDK::Graphics::SolidColorMaterialHandle boxMtl;
				if (!boxMtl.IsValid())
				{
					boxMtl.Initialize();
				}
				boxMtl.SetColor(AColor(primColor));
				for (auto it : meshParts)
				{
					if (nullptr == it.first) continue;
					MaxSDK::Graphics::VertexBufferHandle vertexBufferHandle;
					vertexBufferHandle = MaxSDK::Graphics::GenerateInstanceData(it.second.WorldMatrices, it.second.numInstances);
					Box3 meshBox = it.first->getBoundingBox();
					PFBoxGeometry* pBoxGeom = new PFBoxGeometry(meshBox);
					MaxSDK::Graphics::GeometryRenderItemHandle geoItem;
					geoItem.Initialize();
					geoItem.SetRenderGeometry(pBoxGeom);
					geoItem.SetVisibilityGroup(RenderItemVisible_Gizmo);
					geoItem.SetCustomMaterial(boxMtl);
					MaxSDK::Graphics::RenderItemHandle hInstanceItem;
					if (MaxSDK::Graphics::GenerateInstanceRenderItem(hInstanceItem, geoItem, vertexBufferHandle))
					{
						targetRenderItemContainer.AddRenderItem(hInstanceItem);
					}				
				}
			}
			else
			{
				Matrix3& matNode = pgNode->GetNodeTM(updateDisplayContext.GetDisplayTime());
				Mesh meshCombine;
				BuildMeshAndData(meshCombine, 
					instanceData, 
					meshParts, 
					matNode, 
					shapeSplitter, 
					chMtlIndex,
					generateRenderItemsContext);
				if (meshCombine.numVerts > 0)
				{
					IMeshDisplay2* pCombinedDisplay = static_cast<IMeshDisplay2*>(meshCombine.GetInterface(IMesh_DISPLAY2_INTERFACE_ID));
					if (NULL != pCombinedDisplay)
					{
						pCombinedDisplay->PrepareDisplay(generateRenderItemsContext);
						pCombinedDisplay->GetRenderItems(generateRenderItemsContext, nodeContext, targetRenderItemContainer);
					}
				}
			}
		}
	}
	
	for(auto iter : instanceData)
	{
		RenderItemHandleArray renderItems;
		MaxSDK::Graphics::GenerateMeshRenderItemsContext instanceContext(generateRenderItemsContext);
		instanceContext.SetInstanceData(iter.vertexBufferHandle);
		iter.pMeshDisplay->GetRenderItems(instanceContext,nodeContext,renderItems);

		targetRenderItemContainer.AddRenderItems(renderItems);
	}
	free(matrixPool);
	matrixPool = nullptr;

	return true;
}

DWORD GetNewObjectColor();

// static members
Object*				PFOperatorDisplay::m_editOb	 = NULL;
IObjParam*			PFOperatorDisplay::m_ip      = NULL;

// constructors/destructors
PFOperatorDisplay::PFOperatorDisplay()
{ 
	RegisterParticleFlowNotification();
	_pblock() = NULL;
	GetClassDesc()->MakeAutoParamBlocks(this); 
	_activeIcon() = _inactiveIcon() = NULL;
	if (pblock() != NULL) // set random initial color
		pblock()->SetValue(kDisplay_color, 0, Color(GetNewObjectColor()));

	mpPFViewportExtensionInterface = NULL ;

	SlateModelWrapper::DefineColorSwatchUI();
}

PFOperatorDisplay::~PFOperatorDisplay()
{
	int wasHolding = theHold.Holding();
	if (wasHolding) theHold.Suspend();
	DeleteAllRefsFromMe();
	if (wasHolding) theHold.Resume();

	if (mpPFViewportExtensionInterface != NULL)
	{
		delete mpPFViewportExtensionInterface ;
		mpPFViewportExtensionInterface = NULL;
	}

}

//+>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>+
//|							From InterfaceServer									 |
//+>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>+
BaseInterface* PFOperatorDisplay::GetInterface(Interface_ID id)
{
	if (id == PFACTION_INTERFACE) return (IPFAction*)this;
	if (id == PFOPERATOR_INTERFACE) return (IPFOperator*)this;
	if (id == PFVIEWPORT_INTERFACE) return (IPFViewport*)this;
	if (id == PVIEWITEM_INTERFACE) return (IPViewItem*)this;
	if (id == IPFVIEWPORT_EXTENSION2_INTERFACE_ID)
	{
		if (mpPFViewportExtensionInterface == NULL)
		{
			mpPFViewportExtensionInterface = new PFOperatorDisplayExtensionHelper(this) ;
		}
		return mpPFViewportExtensionInterface ;
	}
	// return the GetInterface() of its super class
	return HelperObject::GetInterface(id) ;
}

//+>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>+
//|							From FPMixinInterface							 |
//+>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>+
FPInterfaceDesc* PFOperatorDisplay::GetDescByID(Interface_ID id)
{
	if (id == PFACTION_INTERFACE) return &display_action_FPInterfaceDesc;
	if (id == PFOPERATOR_INTERFACE) return &display_operator_FPInterfaceDesc;
	if (id == PVIEWITEM_INTERFACE) return &display_PViewItem_FPInterfaceDesc;
	return NULL;
}

//+>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>+
//|							From Animatable									 |
//+>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>+
void PFOperatorDisplay::DeleteThis()
{
	delete this;
}

//+--------------------------------------------------------------------------+
//|							From Animatable									 |
//+--------------------------------------------------------------------------+
void PFOperatorDisplay::GetClassName(TSTR& s)
{
	s = GetString(IDS_OPERATOR_DISPLAY_CLASS_NAME);
}

//+--------------------------------------------------------------------------+
//|							From Animatable									 |
//+--------------------------------------------------------------------------+
Class_ID PFOperatorDisplay::ClassID()
{
	return PFOperatorDisplay_Class_ID;
}

//+--------------------------------------------------------------------------+
//|							From Animatable									 |
//+--------------------------------------------------------------------------+
void PFOperatorDisplay::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	_ip() = ip; _editOb() = this;
	GetClassDesc()->BeginEditParams(ip, this, flags, prev);
}

//+--------------------------------------------------------------------------+
//|							From Animatable									 |
//+--------------------------------------------------------------------------+
void PFOperatorDisplay::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	_ip() = NULL; _editOb() = NULL;
	GetClassDesc()->EndEditParams(ip, this, flags, next );
}

//+--------------------------------------------------------------------------+
//|							From Animatable									 |
//+--------------------------------------------------------------------------+
Animatable* PFOperatorDisplay::SubAnim(int i)
{
	switch(i) {
	case 0: return _pblock();
	}
	return NULL;
}

//+--------------------------------------------------------------------------+
//|							From Animatable									 |
//+--------------------------------------------------------------------------+
TSTR PFOperatorDisplay::SubAnimName(int i)
{
	return PFActions::GetString(IDS_PARAMETERS);
}

//+--------------------------------------------------------------------------+
//|							From Animatable									 |
//+--------------------------------------------------------------------------+
ParamDimension* PFOperatorDisplay::GetParamDimension(int i)
{
	return _pblock()->GetParamDimension(i);
}

//+--------------------------------------------------------------------------+
//|							From Animatable									 |
//+--------------------------------------------------------------------------+
IParamBlock2* PFOperatorDisplay::GetParamBlock(int i)
{
	if (i==0) return _pblock();
	return NULL;
}

//+--------------------------------------------------------------------------+
//|							From Animatable									 |
//+--------------------------------------------------------------------------+
IParamBlock2* PFOperatorDisplay::GetParamBlockByID(short id)
{
	if (id == 0) return _pblock();
	return NULL;
}

//+>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>+
//|							From ReferenceMaker								 |
//+>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>+
RefTargetHandle PFOperatorDisplay::GetReference(int i)
{
	if (i==0) return (RefTargetHandle)pblock();
	return NULL;
}

//+--------------------------------------------------------------------------+
//|							From ReferenceMaker								 |
//+--------------------------------------------------------------------------+
void PFOperatorDisplay::SetReference(int i, RefTargetHandle rtarg)
{
	if (i==0) _pblock() = (IParamBlock2*)rtarg;
}

//+--------------------------------------------------------------------------+
//|							From ReferenceMaker								 |
//+--------------------------------------------------------------------------+
RefResult PFOperatorDisplay::NotifyRefChanged(const Interval& changeInt,RefTargetHandle hTarget,PartID& partID, RefMessage message, BOOL propagate)
{
	Color newColor;
	DWORD theColor;

	switch (message) {
		case REFMSG_CHANGE:
			if (pblock() == hTarget) {
				ParamID lastUpdateID = pblock()->LastNotifyParamID();
				switch (lastUpdateID)
				{
				case kDisplay_type:
					NotifyDependents(FOREVER, 0, kPFMSG_DynamicNameChange, NOTIFY_ALL, TRUE);
					// the break is omitted on purpose (bayboro 11-18-02)
				case kDisplay_visible:
					if (lastUpdateID == kDisplay_visible) recalcVisibility();
					// the break is omitted on purpose (bayboro 11-18-02)
				case kDisplay_color: 
				case kDisplay_showNumbering:
				case kDisplay_selectedType:
					newColor = pblock()->GetColor(kDisplay_color, 0);
					theColor = newColor.toRGB();
					NotifyDependents( FOREVER, (PartID)theColor, kPFMSG_UpdateWireColor );

					if ( lastUpdateID == kDisplay_color )
						SlateModelWrapper::NotifyListenersDisplayColorChanged();

					UpdatePViewUI(lastUpdateID);
					return REF_STOP;
				default:
					UpdatePViewUI(lastUpdateID);
				}
			}
			break;
	}	
	return REF_SUCCEED;
}

//+--------------------------------------------------------------------------+
//|							From ReferenceMaker								 |
//+--------------------------------------------------------------------------+
RefTargetHandle PFOperatorDisplay::Clone(RemapDir &remap)
{
	PFOperatorDisplay* newOp = new PFOperatorDisplay();
	newOp->ReplaceReference(0, remap.CloneRef(pblock()));
	BaseClone(this, newOp, remap);
	return newOp;
}

//+--------------------------------------------------------------------------+
//|							From ReferenceMaker								 |
//+--------------------------------------------------------------------------+
int PFOperatorDisplay::RemapRefOnLoad(int iref)
{
	return iref;
}

//+--------------------------------------------------------------------------+
//|							From ReferenceMaker								 |
//+--------------------------------------------------------------------------+
IOResult PFOperatorDisplay::Save(ISave *isave)
{
	return IO_OK;
}

//+--------------------------------------------------------------------------+
//|							From ReferenceMaker								 |
//+--------------------------------------------------------------------------+
IOResult PFOperatorDisplay::Load(ILoad *iload)
{
	return IO_OK;
}

//+>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>+
//|							From BaseObject									 |
//+>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>+
const TCHAR* PFOperatorDisplay::GetObjectName()
{
	return GetString(IDS_OPERATOR_DISPLAY_OBJECT_NAME);
}

//+>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>+
//|							From IPFAction									 |
//+>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>+
bool PFOperatorDisplay::Init(IObject* pCont, Object* pSystem, INode* node, Tab<Object*>& actions, Tab<INode*>& actionNodes)
{
	_totalParticles(pCont) = 0;
	return true;
}

bool PFOperatorDisplay::Release(IObject* pCont)
{
	return true;
}

const ParticleChannelMask& PFOperatorDisplay::ChannelsUsed(const Interval& time) const
{
								//  read								&	write channels
	static ParticleChannelMask mask(PCU_Amount|PCU_ID|PCU_Position|PCU_Speed|PCU_Shape|PCU_ShapeTexture|PCU_MtlIndex, 0);
	return mask;
}

//+>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>+
//|							From IPFOperator								 |
//+>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>+
bool PFOperatorDisplay::Proceed(IObject* pCont, 
									 PreciseTimeValue timeStart, 
									 PreciseTimeValue& timeEnd,
									 Object* pSystem,
									 INode* pNode,
									 INode* actionNode,
									 IPFIntegrator* integrator)
{
	// acquire absolutely necessary particle channels
	IParticleChannelAmountR* chAmount = GetParticleChannelAmountRInterface(pCont);
	if (chAmount == NULL) return false; // can't find number of particles in the container
	int count = chAmount->Count();
	if (count <= 0) return true; // no particles to worry about
	IParticleChannelNewR* chNew = GetParticleChannelNewRInterface(pCont);
	if (chNew == NULL) return false; // can't find newly entered particles for duration calculation
	if (chNew->IsAllOld()) return true; // everything has been initialized
	
	IChannelContainer* chCont = GetChannelContainerInterface(pCont);
	if (chCont == NULL) return false;
	// first try to get the private channel.
	// If it's not possible then create a new one
	bool initVisible = false;
	IParticleChannelIntR* chVisibleR = (IParticleChannelIntR*)chCont->EnsureInterface(PARTICLECHANNELVISIBLER_INTERFACE,
													ParticleChannelInt_Class_ID,
													true,	PARTICLECHANNELVISIBLER_INTERFACE,
															PARTICLECHANNELVISIBLEW_INTERFACE,
													true, actionNode, (Object*)this, // no transfer & private
													&initVisible);
	IParticleChannelIntW*chVisibleW = (IParticleChannelIntW*)chCont->GetPrivateInterface(PARTICLECHANNELVISIBLEW_INTERFACE, (Object*)this);
	if ((chVisibleR == NULL) || (chVisibleW == NULL)) return false; // can't find/create Visible channel

	bool initVisOp = false;
	IParticleChannelBoolR* chVisR = (IParticleChannelBoolR*)chCont->GetPrivateInterface(PARTICLECHANNELVISIBILITYOPR_INTERFACE, (Object*)this);
	IParticleChannelBoolW* chVisW = NULL;
	if (chVisR != NULL)
	{
		chVisW = (IParticleChannelBoolW*)chCont->EnsureInterface(PARTICLECHANNELVISIBILITYOPW_INTERFACE,
															ParticleChannelBool_Class_ID,
															true, PARTICLECHANNELVISIBILITYOPR_INTERFACE,
															PARTICLECHANNELVISIBILITYOPW_INTERFACE, true,
															actionNode, (Object*)this, &initVisOp);
	}

	int i;
	int total = _totalParticles(pCont);
	for(i=0; i<count; i++) 
	{
		if (chNew->IsNew(i)) 
		{
			int visIndex = chVisibleR->GetValue(i);
			if (visIndex == 0) {
				total += 1;
				chVisibleW->SetValue(i, total);
			}
			if (initVisOp)
				chVisW->SetValue(i, true);
		}
	}
	_totalParticles(pCont) = total;
	return true;
}

//+>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>+
//|							From IPFViewport								 |
//+>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>+
int PFOperatorDisplay::Display(IObject* pCont,  TimeValue time, Object* pSystem, INode* psNode, INode* pgNode, ViewExp *vpt, int flags)
{
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	int displayType = pblock()->GetInt(kDisplay_type);
	int selectedType = pblock()->GetInt(kDisplay_selectedType);
	int showNum = pblock()->GetInt(kDisplay_showNumbering);
	float visPercent = GetPFFloat(pblock(), kDisplay_visible, time);

	DbgAssert(pCont); DbgAssert(pSystem); DbgAssert(psNode); DbgAssert(pgNode);
	if (pCont == NULL) return 0;
	if (pSystem == NULL) return 0;
	if (psNode == NULL) return 0;
	if (pgNode == NULL) return 0;
	IPFSystem* iSystem = PFSystemInterface(pSystem);
	if (iSystem == NULL) return 0; // invalid particle system

	// all particles are visible in 'select particles' mode
	bool subSelPartMode = (psNode->Selected() && (pSystem->GetSubselState() == 1));
	if (subSelPartMode) {
		if (displayType == kDisplay_type_none)
			displayType = kDisplay_type_dots;
		if (selectedType == kDisplay_type_none)
			selectedType = kDisplay_type_dots;
		visPercent = 1.0f;
	}
	if ((displayType == kDisplay_type_none) && (selectedType == kDisplay_type_none))
		return 0;

	// acquire particle channels
	IParticleChannelAmountR* chAmount = GetParticleChannelAmountRInterface(pCont);
	if (chAmount == NULL) return 0; // can't find number of particles in the container
	int count = chAmount->Count();
	if (count == 0) return 0;		// there no particles to draw
	int visCount = int(visPercent * count);
	if (visCount <= 0) return 0;	// there no particles to draw
	IParticleChannelPoint3R* chPos = GetParticleChannelPositionRInterface(pCont);	
	if (chPos == NULL) return 0;	// can't find particle position in the container
	IParticleChannelPoint3R* chSpeed = NULL;
	if ((displayType == kDisplay_type_lines) || (selectedType == kDisplay_type_lines))
	{
		chSpeed = GetParticleChannelSpeedRInterface(pCont);
		// if no speed channel then change drawing type to dots
		if (chSpeed == NULL) {
			if (displayType == kDisplay_type_lines)
				displayType = kDisplay_type_dots;
			if (selectedType == kDisplay_type_lines)
				selectedType = kDisplay_type_dots;
		}
	}
	IParticleChannelIDR* chID = GetParticleChannelIDRInterface(pCont);
	// if no ID channel then it's not possible to show numbers & selections
	if (chID == NULL) showNum = 0;
	
	IParticleChannelQuatR* chOrient = NULL;
	IParticleChannelPoint3R* chScale = NULL;
	IParticleChannelMeshR* chShape = NULL;
	IParticleChannelIntR* chMtlIndex = NULL;
	IParticleChannelMeshMapR* chMapping = NULL;

	if ((displayType == kDisplay_type_boundingBoxes) ||
		(displayType == kDisplay_type_geometry) ||
		(selectedType == kDisplay_type_boundingBoxes) ||
		(selectedType == kDisplay_type_geometry))
	{
		chOrient = GetParticleChannelOrientationRInterface(pCont);
		chScale = GetParticleChannelScaleRInterface(pCont);
		chShape = GetParticleChannelShapeRInterface(pCont);
		// if no shape channel then change drawing type to X marks
		if (chShape == NULL) displayType = kDisplay_type_Xs;
		chMtlIndex = GetParticleChannelMtlIndexRInterface(pCont);
		chMapping = GetParticleChannelShapeTextureRInterface(pCont);
	}

	IChannelContainer* chCont = GetChannelContainerInterface(pCont);
	if (chCont == NULL) return false;
	IParticleChannelIntR* chVisibleR = (IParticleChannelIntR*)chCont->GetPrivateInterface(PARTICLECHANNELVISIBLER_INTERFACE, (Object*)this);
	IParticleChannelBoolR* chVisOpR = (IParticleChannelBoolR*)chCont->GetPrivateInterface(PARTICLECHANNELVISIBILITYOPR_INTERFACE, (Object*)this);
	IParticleChannelBoolR* chVisVpR = GetParticleChannelVisibilityVPRInterface(pCont);

	int i, j, index, num, numMtls=1;
	float sizeFactor;
	Tab<int> blockNon, blockSel;
	int selNum=0, nonNum=0;
	IParticleChannelBoolR* chSelect = GetParticleChannelSelectionRInterface(pCont);
	blockNon.SetCount(count); blockSel.SetCount(count);
	bool doVisibleFilter = ((visPercent < 1.0f) && (chVisibleR != NULL));
	bool checkVisibility = doVisibleFilter || (chVisOpR != NULL) || (chVisVpR != NULL);
	if (chSelect != NULL) 
	{
		if (!checkVisibility)
		{
			for(i=0; i<count; i++) 
			{
				if (chSelect->GetValue(i))
					blockSel[selNum++] = i;
				else
					blockNon[nonNum++] = i;
			}
		}
		else
		{
			for(i=0; i<count; i++) 
			{
				if (doVisibleFilter)
					if (isInvisible(chVisibleR->GetValue(i))) 
						continue;
				if (chVisOpR && chVisOpR->GetValue(i))
					continue;
				if (chVisVpR && chVisVpR->GetValue(i))
					continue;
				if (chSelect->GetValue(i))
					blockSel[selNum++] = i;
				else
					blockNon[nonNum++] = i;
			}
		}
	} else {
		if (!checkVisibility)
		{
			for(i=0; i<count; i++) 
				blockNon[nonNum++] = i;
		}
		else
		{
			for(i=0; i<count; i++) 
			{
				if (doVisibleFilter)
					if (isInvisible(chVisibleR->GetValue(i))) 
						continue;
				if (chVisOpR && chVisOpR->GetValue(i))
					continue;
				if (chVisVpR && chVisVpR->GetValue(i))
					continue;
				blockNon[nonNum++] = i;
			}
		}
	}
	blockNon.SetCount(nonNum);
	blockSel.SetCount(selNum);

	GraphicsWindow* gw = vpt->getGW();
	DWORD oldRndLimits, newRndLimits;
	newRndLimits = oldRndLimits = gw->getRndLimits();

	// define color for particles
	bool doPrimColor = false;
	bool doSelColor = false;
	Color primColor, selColor;
	Color subselColor = GetColorManager()->GetColorAsPoint3(PFSubselectionColorId);
	if (psNode->Selected()) {
		switch (pSystem->GetSubselState())
		{
		case 0:
			doPrimColor = doSelColor = ((oldRndLimits & GW_WIREFRAME) != 0);
			primColor = Color(GetSelColor());
			selColor = Color(GetSelColor());
			break;
		case 1: 
			doSelColor = true;
			primColor = pblock()->GetColor(kDisplay_color);
			selColor = Color(subselColor);
			break;
		case 2: {
			primColor = pblock()->GetColor(kDisplay_color);
			selColor = pblock()->GetColor(kDisplay_color);
			IParticleGroup* iPGroup = ParticleGroupInterface(pgNode);
			if (iPGroup != NULL)
				if (iSystem->IsActionListSelected(iPGroup->GetActionList())) {
					doPrimColor = doSelColor = true;
					primColor = Color(subselColor);
					selColor = Color(subselColor);
				}
			}
			break;
		}
	} else if (psNode->IsFrozen()) {
		doPrimColor = doSelColor = true;
		primColor = Color(GetFreezeColor());
		selColor = Color(GetFreezeColor());
	} else {
		primColor = pblock()->GetColor(kDisplay_color);
		selColor = pblock()->GetColor(kDisplay_color);
	}

	Material* nodeMtl = pgNode->Mtls();
	Matrix3 gwTM;
	gwTM.IdentityMatrix();
	gw->setTransform(gwTM);
	Material* curMtl;
	Material vpMtl;
	Point3 pos[33], speed;
	int edgeVis[33];

	enum { noneCat, markerCat, lineCat, geomCat };
	int ordCat, selCat;
	switch (displayType) {
	case kDisplay_type_boundingBoxes:
	case kDisplay_type_geometry:
		ordCat = geomCat;
		break;
	case kDisplay_type_lines:
		ordCat = lineCat;
		break;
	case kDisplay_type_none:
		ordCat = noneCat;
		break;
	default:
		ordCat = markerCat;
	}
	switch (selectedType) {
	case kDisplay_type_boundingBoxes:
	case kDisplay_type_geometry:
		selCat = geomCat;
		break;
	case kDisplay_type_lines:
		selCat = lineCat;
		break;
	case kDisplay_type_none:
		selCat = noneCat;
		break;
	default:
		selCat = markerCat;
	}

	if ((ordCat == geomCat) || (selCat == geomCat))
	{
		DWORD prevLimits = newRndLimits;
		DWORD boxLimits = newRndLimits;
		if (!(boxLimits&GW_BOX_MODE)) boxLimits |= GW_BOX_MODE;

		vpMtl = *(SysUtil::GetParticleMtl());
		for(int runType = 0; runType < 2; runType++)
		{
			Tab<int> *block = NULL;
			if (runType) {
				if (selNum == 0) continue;
				if (selCat != geomCat) continue;
				if (selectedType == kDisplay_type_boundingBoxes)
					gw->setRndLimits(boxLimits);
				else
					gw->setRndLimits(prevLimits);
				vpMtl.Kd = vpMtl.Ks = selColor;
				gw->setColor(LINE_COLOR, selColor);
				if ((nodeMtl != NULL) && (!doSelColor)) {
					curMtl = nodeMtl;
					numMtls = pgNode->NumMtls();
				} else {
					curMtl = &vpMtl;
					if (doSelColor) curMtl->Kd = curMtl->Ks = selColor;
					else curMtl->Kd = curMtl->Ks = pblock()->GetColor(kDisplay_color);
					numMtls = 1;
				}
				block = &blockSel;
			} else {
				if (nonNum == 0) continue;
				if (ordCat != geomCat) continue;
				if (displayType == kDisplay_type_boundingBoxes)
					gw->setRndLimits(boxLimits);
				else
					gw->setRndLimits(prevLimits);
				gw->setColor(LINE_COLOR, primColor);
				if ((nodeMtl != NULL) && (!doPrimColor)) {
					curMtl = nodeMtl;
					numMtls = pgNode->NumMtls();
				} else {
					curMtl = &vpMtl;
					if (doPrimColor) curMtl->Kd = curMtl->Ks = primColor;
					else curMtl->Kd = curMtl->Ks = pblock()->GetColor(kDisplay_color);
					numMtls = 1;
				}
				block = &blockNon;
			}
			for(i=0; i<numMtls; i++) gw->setMaterial(curMtl[i], i);

			for(i=0; i<block->Count(); i++) {
				index = (*block)[i];
				Mesh* pMesh = NULL;
				if (chShape != NULL)
					pMesh = const_cast <Mesh*>(chShape->GetValue(index));
				if (pMesh) {
					//Nitrous retain mode has difficulty to display per-view bounding box
					//So under nitrous, we use legacy bounding box draw method if current
					//display type is geometry.
					//If current display type is bounding box, then all views display boxes.
					//At this time, we can use retained instanced bounding box geometry
					if(!MaxSDK::Graphics::IsRetainedModeEnabled()
						|| ((displayType == kDisplay_type_geometry) && (newRndLimits & GW_BOX_MODE)))
					{
						Mesh curMesh;
						gwTM.IdentityMatrix();
						if (chOrient) gwTM.SetRotate(chOrient->GetValue(index));
						if (chScale) gwTM.PreScale(chScale->GetValue(index));
						gwTM.SetTrans(chPos->GetValue(index));
						gw->setTransform(gwTM);
						// set vertex color
						switch (psNode->GetVertexColorType()) {
						case nvct_color:
							if (pMesh->curVCChan == 0) break;
							pMesh->setVCDisplayData (0);
							break;
						case nvct_illumination:
							if (pMesh->curVCChan == MAP_SHADING) break;
							pMesh->setVCDisplayData (MAP_SHADING);
							break;
						case nvct_alpha:
							if (pMesh->curVCChan == MAP_ALPHA) break;
							pMesh->setVCDisplayData (MAP_ALPHA);
							break;
						// CAL-06/15/03: add a new option to view map channel as vertex color. (FID #1926)
						case nvct_map_channel:
							if (pMesh->curVCChan == psNode->GetVertexColorMapChannel()) break;
							pMesh->setVCDisplayData (psNode->GetVertexColorMapChannel());
							break;
						}

						// seed SDK mesh->render() method remark for necessity of this call
						if (numMtls > 1) gw->setMaterial(curMtl[0], 0);

						if ((chMtlIndex != NULL) || (chMapping != NULL)) {
							curMesh = *pMesh;
							bool assignedMtlIndices = AssignMeshMaterialIndices(&curMesh, chMtlIndex, index);
							bool assignedMapping = AssignMeshMapping(&curMesh, chMapping, index);
							if (assignedMtlIndices || assignedMapping) 
								pMesh = &curMesh;
							if (assignedMtlIndices) {
								int mtlIndex = chMtlIndex->GetValue(index);
								if (mtlIndex < numMtls)
									gw->setMaterial(curMtl[mtlIndex], mtlIndex);
							}
							pMesh->InvalidateStrips();
						}
						pMesh->render(gw, curMtl, (flags&USE_DAMAGE_RECT) ? &vpt->GetDammageRect() : NULL, COMP_ALL, numMtls);
					}
				} else {
					pos[0] = chPos->GetValue(index);
					gw->marker(pos, X_MRKR);
				}
			}
		}

		gwTM.IdentityMatrix();
		gw->setTransform(gwTM);
		gw->setRndLimits(prevLimits);
	}

	if ((ordCat == lineCat) || (selCat == lineCat))
	{
		sizeFactor = GetTicksPerFrame();
		for(i=0; i<32; i++)
			edgeVis[i] = (i%2) ? GW_EDGE_SKIP : GW_EDGE_VIS;
		edgeVis[32] = GW_EDGE_SKIP;

		if (displayType == kDisplay_type_lines)
		{
			gw->setColor(LINE_COLOR, primColor);
			j = 0;
			num = blockNon.Count();
			for(index=0; index<num; index++)
			{
				i = blockNon[index];
				pos[j] = chPos->GetValue(i);
				pos[j+1] = pos[j] + sizeFactor*chSpeed->GetValue(i);
				j += 2;
				if (j == 30) {
					gw->polyline(j, pos, NULL, NULL, 0, edgeVis);
					j = 0;
				}
			}
			if (j) gw->polyline(j, pos, NULL, NULL, 0, edgeVis);
		}

		if (selectedType == kDisplay_type_lines)
		{
			gw->setColor(LINE_COLOR, selColor);
			j = 0;
			num = blockSel.Count();
			for(index=0; index<num; index++)
			{
				i = blockSel[index];
				pos[j] = chPos->GetValue(i);
				pos[j+1] = pos[j] + sizeFactor*chSpeed->GetValue(i);
				j += 2;
				if (j == 30) {
					gw->polyline(j, pos, NULL, NULL, 0, edgeVis);
					j = 0;
				}
			}
			if (j) gw->polyline(j, pos, NULL, NULL, 0, edgeVis);
		}
	}

	if ((ordCat == markerCat) || (selCat == markerCat))
	{
		MarkerType mType = POINT_MRKR;
		int num;
		if ((ordCat == markerCat) && (blockNon.Count()>0))
		{
			switch (displayType)
			{
				case kDisplay_type_dots: mType = POINT_MRKR; break;
				case kDisplay_type_ticks: mType = PLUS_SIGN_MRKR; break;
				case kDisplay_type_circles: mType = CIRCLE_MRKR; break;
				case kDisplay_type_diamonds: mType = DIAMOND_MRKR; break;
				case kDisplay_type_boxes: mType = HOLLOW_BOX_MRKR; break;
				case kDisplay_type_asterisks: mType = ASTERISK_MRKR; break;
				case kDisplay_type_triangles: mType = TRIANGLE_MRKR; break;
				case kDisplay_type_Xs: mType = X_MRKR; break;
			}
			num = blockNon.Count();
			VertexBuffer* pGWVB = gw->MarkerBufferLock();
			if (nullptr != pGWVB)
			{
				gw->MarkerBufferSetMarkerType(mType);
				int bufferSize = gw->MarkerBufferSize();
				int iMarkerCount = 0;
				DWORD dwColor = COLOR_ARGB((DWORD)(255), (DWORD)(primColor.r * 255), (DWORD)(primColor.g * 255), (DWORD)(primColor.b * 255));
				for (int i = 0; i < num; ++i)
				{
					pGWVB->position = chPos->GetValue(blockNon[i]);
					pGWVB->color = dwColor;
					++pGWVB;
					++iMarkerCount;
					if (iMarkerCount == bufferSize)
					{
						gw->MarkerBufferUnLock();
						gw->MarkerBufferDraw(bufferSize);
						pGWVB = gw->MarkerBufferLock();
						iMarkerCount = 0;
					}
				}
				gw->MarkerBufferUnLock();
				gw->MarkerBufferDraw(iMarkerCount);
			}
			else
			{
				gw->setColor(LINE_COLOR, primColor);
				gw->startMarkers();
				for (i = 0; i<num; i++) {
					pos[0] = chPos->GetValue(blockNon[i]);
					gw->marker(pos, mType);
				}
				gw->endMarkers();
			}							
		}
		if ((selCat == markerCat) && (blockSel.Count()>0))
		{
			switch (selectedType)
			{
				case kDisplay_type_dots: mType = POINT_MRKR; break;
				case kDisplay_type_ticks: mType = PLUS_SIGN_MRKR; break;
				case kDisplay_type_circles: mType = CIRCLE_MRKR; break;
				case kDisplay_type_diamonds: mType = DIAMOND_MRKR; break;
				case kDisplay_type_boxes: mType = HOLLOW_BOX_MRKR; break;
				case kDisplay_type_asterisks: mType = ASTERISK_MRKR; break;
				case kDisplay_type_triangles: mType = TRIANGLE_MRKR; break;
				case kDisplay_type_Xs: mType = X_MRKR; break;
			}
			num = blockSel.Count();
			VertexBuffer* pGWVB = gw->MarkerBufferLock();
			if (nullptr != pGWVB)
			{
				gw->MarkerBufferSetMarkerType(mType);
				int bufferSize = gw->MarkerBufferSize();
				int iMarkerCount = 0;
				DWORD dwColor = COLOR_ARGB((DWORD)(255), (DWORD)(selColor.r * 255), (DWORD)(selColor.g * 255), (DWORD)(selColor.b * 255));
				for (int i = 0; i < num; ++i)
				{
					pGWVB->position = chPos->GetValue(blockSel[i]);
					pGWVB->color = dwColor;
					++pGWVB;
					++iMarkerCount;
					if (iMarkerCount == bufferSize)
					{
						gw->MarkerBufferUnLock();
						gw->MarkerBufferDraw(bufferSize);
						pGWVB = gw->MarkerBufferLock();
						iMarkerCount = 0;
					}
				}
				gw->MarkerBufferUnLock();
				gw->MarkerBufferDraw(iMarkerCount);
			}
			else
			{
				gw->setColor(LINE_COLOR, selColor);
				gw->startMarkers();
				for (i = 0; i < num; i++) {
					pos[0] = chPos->GetValue(blockSel[i]);
					gw->marker(pos, mType);
				}
				gw->endMarkers();
			}
		}
	}

	if (showNum)
	{ // show born index along with each particle
		TCHAR numText[16];
		num = blockNon.Count();
		if (num > 0) {
			gw->setColor(TEXT_COLOR, primColor);
			for(i=0; i<num; i++) {
				index = blockNon[i];
				_stprintf_s(numText, 16, _T("%d"), chID->GetParticleBorn(index)+1); // indices shown are 1-based
				pos[0] = chPos->GetValue(index);
				gw->text(pos, numText);
			}
		}
		num = blockSel.Count();
		if (num > 0) {
			gw->setColor(TEXT_COLOR, selColor);
			for(i=0; i<num; i++) {
				index = blockSel[i];
				_stprintf_s(numText, 16, _T("%d"), chID->GetParticleBorn(index)+1); // indices shown are 1-based
				pos[0] = chPos->GetValue(index);
				gw->text(pos, numText);
			}
		}
	}

	return 0;
}

//+--------------------------------------------------------------------------+
//|							From IPFViewport								 |
//+--------------------------------------------------------------------------+
int PFOperatorDisplay::HitTest(IObject* pCont, TimeValue time, Object* pSystem, INode* psNode, INode* pgNode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt)
{
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	int displayType = pblock()->GetInt(kDisplay_type);
	int selectedType = pblock()->GetInt(kDisplay_selectedType);
	float visPercent = GetPFFloat(pblock(), kDisplay_visible, time);

	DbgAssert(pCont); DbgAssert(pSystem); DbgAssert(psNode); DbgAssert(pgNode);
	if (pCont == NULL) return 0;
	if (pSystem == NULL) return 0;
	if (psNode == NULL) return 0;
	if (pgNode == NULL) return 0;
	IPFSystem* iSystem = PFSystemInterface(pSystem);
	if (iSystem == NULL) return 0; // invalid particle system

	IParticleGroup* iPGroup = ParticleGroupInterface(pgNode);
	INode* theActionListNode = (iPGroup != NULL) ? iPGroup->GetActionList() : NULL;

	BOOL selOnly    = flags&SUBHIT_SELONLY		? TRUE : FALSE;
	BOOL unselOnly  = flags&SUBHIT_UNSELONLY	? TRUE : FALSE;
	BOOL abortOnHit = flags&SUBHIT_ABORTONHIT	? TRUE : FALSE;

	// can't process a situation when selOnly and unselOnly both are TRUE;
	DbgAssert((selOnly && unselOnly) == FALSE);
	if (selOnly && unselOnly) return 0;

	// all particles are visible in 'select particles' mode
	bool subSelPartMode = (psNode->Selected() && (pSystem->GetSubselState() == 1));
	if (subSelPartMode) {
		if (displayType == kDisplay_type_none)
			displayType = kDisplay_type_dots;
		if (selectedType == kDisplay_type_none)
			selectedType = kDisplay_type_dots;
		visPercent = 1.0f;
	}
	if ((displayType == kDisplay_type_none) && (selectedType == kDisplay_type_none))
		return 0;

	// acquire particle channels
	IParticleChannelAmountR* chAmount = GetParticleChannelAmountRInterface(pCont);
	if (chAmount == NULL) return 0; // can't find number of particles in the container
	int count = chAmount->Count();
	if (count == 0) return 0;		// there no particles to hit
	int visCount = int(visPercent * count);
	if (visCount <= 0) return 0;	// there no particles to hit
	IParticleChannelPoint3R* chPos = GetParticleChannelPositionRInterface(pCont);	
	if (chPos == NULL) return 0;	// can't find particle position in the container
	IParticleChannelPoint3R* chSpeed = NULL;
	if ((displayType == kDisplay_type_lines) || (selectedType == kDisplay_type_lines))
	{
		chSpeed = GetParticleChannelSpeedRInterface(pCont);
		// if no speed channel then change drawing type to dots
		if (chSpeed == NULL) {
			if (displayType == kDisplay_type_lines)
				displayType = kDisplay_type_dots;
			if (selectedType == kDisplay_type_lines)
				selectedType = kDisplay_type_dots;
		}
	}
	IParticleChannelIDR* chID = GetParticleChannelIDRInterface(pCont);
	// if no ID channel then it's not possible to process selected/unselected
	if (chID == NULL) {
		selOnly = FALSE; unselOnly = FALSE; 
	}
	
	IParticleChannelQuatR* chOrient = NULL;
	IParticleChannelPoint3R* chScale = NULL;
	IParticleChannelMeshR* chShape = NULL;
	if ((displayType == kDisplay_type_boundingBoxes) ||
		(displayType == kDisplay_type_geometry) ||
		(selectedType == kDisplay_type_boundingBoxes) ||
		(selectedType == kDisplay_type_geometry))
	{
		chOrient = GetParticleChannelOrientationRInterface(pCont);
		chScale = GetParticleChannelScaleRInterface(pCont);
		chShape = GetParticleChannelShapeRInterface(pCont);
		// if no shape channel then change drawing type to X marks
		if (chShape == NULL) displayType = kDisplay_type_Xs;
	}

	IChannelContainer* chCont = GetChannelContainerInterface(pCont);
	if (chCont == NULL) return false;
	IParticleChannelIntR* chVisibleR = (IParticleChannelIntR*)chCont->GetPrivateInterface(PARTICLECHANNELVISIBLER_INTERFACE, (Object*)this);

	int i, index, num, numMtls;
	float sizeFactor;
	Tab<int> blockNon, blockSel;
	int selNum=0, nonNum=0;
	IParticleChannelBoolR* chSelect = GetParticleChannelSelectionRInterface(pCont);
	blockNon.SetCount(count); blockSel.SetCount(count);
	bool doVisibleFilter = ((visPercent < 1.0f) && (chVisibleR != NULL));
	if (chSelect != NULL) {
		for(i=0; i<count; i++) {
			if (doVisibleFilter)
				if (isInvisible(chVisibleR->GetValue(i))) continue;
			if (chSelect->GetValue(i))
				blockSel[selNum++] = i;
			else
				blockNon[nonNum++] = i;
		}
	} else {
		for(i=0; i<count; i++) {
			if (doVisibleFilter)
				if (isInvisible(chVisibleR->GetValue(i))) continue;
			blockNon[nonNum++] = i;
		}
	}
	blockNon.SetCount(nonNum);
	blockSel.SetCount(selNum);

	if (psNode->Selected()) {

		switch (pSystem->GetSubselState())
		{
		case 0:
			selOnly = FALSE;
			unselOnly = FALSE;
			break;
		case 1:
			chSelect = GetParticleChannelSelectionRInterface(pCont);
			if (chSelect == NULL)
				selOnly = unselOnly = FALSE;
			break;
		case 2: {
			if (iPGroup != NULL) {
				if (iSystem->IsActionListSelected(theActionListNode)) {
					if (unselOnly) return 0; // all particles in the group are selected
					if (selOnly) {
						selOnly = FALSE; // work with all particles
					}
				} else {
					if (selOnly) return 0; // particles in the group are not selected
					if (unselOnly) {
						unselOnly = FALSE; // work with all particles
					}
				}
			} else {
				return 0; // invalid pgroup
			}
				}
				
			break;
		}
	} else {
		selOnly = FALSE;
		unselOnly = FALSE;
	}

	enum { noneCat, markerCat, lineCat, geomCat };
	int ordCat, selCat;
	switch (displayType) {
	case kDisplay_type_boundingBoxes:
	case kDisplay_type_geometry:
		ordCat = geomCat;
		break;
	case kDisplay_type_lines:
		ordCat = lineCat;
		break;
	case kDisplay_type_none:
		ordCat = noneCat;
		break;
	default:
		ordCat = markerCat;
	}
	switch (selectedType) {
	case kDisplay_type_boundingBoxes:
	case kDisplay_type_geometry:
		selCat = geomCat;
		break;
	case kDisplay_type_lines:
		selCat = lineCat;
		break;
	case kDisplay_type_none:
		selCat = noneCat;
		break;
	default:
		selCat = markerCat;
	}

	DWORD savedLimits;
	Matrix3 gwTM;
	Point3 pos[33], speed;
	int res=0;
	HitRegion hr;
	Material* curMtl;
	PFHitData* hitData = NULL;
	bool firstHit = true;
	
	GraphicsWindow* gw = vpt->getGW();
	gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK) & ~ GW_ILLUM);
	gwTM.IdentityMatrix();
	gw->setTransform(gwTM);
	MakeHitRegion(hr, type, crossing, 4, p);
	gw->setHitRegion(&hr);
	gw->clearHitCode();

	if ((ordCat == lineCat) || (selCat == lineCat))
	{
		sizeFactor = GetTicksPerFrame();
	
		num = blockNon.Count();
		if ((displayType == kDisplay_type_lines) && (!selOnly) && (num > 0))
		{
			for(i=0; i<num; i++)
			{
				int index = blockNon[i];
				pos[0] = chPos->GetValue(index);
				pos[1] = pos[0] + sizeFactor*chSpeed->GetValue(index);
				gw->polyline(2, pos, NULL, NULL, 1, NULL);
				if (gw->checkHitCode()) {
					res = TRUE;
					if (firstHit) hitData = new PFHitData(pgNode, theActionListNode);
					vpt->LogHit(psNode, (ModContext*)pSystem, gw->getHitDistance(), chID ? chID->GetParticleBorn(index) : 0, hitData);
					if (firstHit) {
						hitData = NULL;
						firstHit = false;
					}
					gw->clearHitCode();
					if (abortOnHit) goto hasHit;
				}
			}
		}

		num = blockSel.Count();
		if ((selectedType == kDisplay_type_lines) && (!unselOnly) && (num > 0))
		{
			for(i=0; i<num; i++)
			{
				int index = blockSel[i];
				pos[0] = chPos->GetValue(index);
				pos[1] = pos[0] + sizeFactor*chSpeed->GetValue(index);
				gw->polyline(2, pos, NULL, NULL, 1, NULL);
				if (gw->checkHitCode()) {
					res = TRUE;
					if (firstHit) hitData = new PFHitData(pgNode, theActionListNode);
					vpt->LogHit(psNode, (ModContext*)pSystem, gw->getHitDistance(), chID ? chID->GetParticleBorn(index) : 0, hitData);
					if (firstHit) {
						hitData = NULL;
						firstHit = false;
					}
					gw->clearHitCode();
					if (abortOnHit) goto hasHit;
				}
			}
		}
	}

	if ((ordCat == markerCat) || (selCat == markerCat))
	{
		MarkerType mType = POINT_MRKR;
		int num = blockNon.Count();
		if ((ordCat == markerCat) && (num>0) && (!selOnly))
		{
			switch (displayType)
			{
				case kDisplay_type_dots: mType = POINT_MRKR; break;
				case kDisplay_type_ticks: mType = PLUS_SIGN_MRKR; break;
				case kDisplay_type_circles: mType = CIRCLE_MRKR; break;
				case kDisplay_type_diamonds: mType = DIAMOND_MRKR; break;
				case kDisplay_type_boxes: mType = HOLLOW_BOX_MRKR; break;
				case kDisplay_type_asterisks: mType = ASTERISK_MRKR; break;
				case kDisplay_type_triangles: mType = TRIANGLE_MRKR; break;
				case kDisplay_type_Xs: mType = X_MRKR; break;
			}
			for(i=0; i<num; i++)
			{
				int index = blockNon[i];
				pos[0] = chPos->GetValue(index);
				gw->marker(pos, mType);
				if (gw->checkHitCode()) {
					res = TRUE;
					if (firstHit) hitData = new PFHitData(pgNode, theActionListNode);
					vpt->LogHit(psNode, (ModContext*)pSystem, gw->getHitDistance(), chID ? chID->GetParticleBorn(index) : 0, hitData);
					if (firstHit) {
						hitData = NULL;
						firstHit = false;
					}
					gw->clearHitCode();
					if (abortOnHit) goto hasHit;
				}
			}
		}

		num = blockSel.Count();
		if ((selCat == markerCat) && (num>0) && (!unselOnly))
		{
			switch (selectedType)
			{
				case kDisplay_type_dots: mType = POINT_MRKR; break;
				case kDisplay_type_ticks: mType = PLUS_SIGN_MRKR; break;
				case kDisplay_type_circles: mType = CIRCLE_MRKR; break;
				case kDisplay_type_diamonds: mType = DIAMOND_MRKR; break;
				case kDisplay_type_boxes: mType = HOLLOW_BOX_MRKR; break;
				case kDisplay_type_asterisks: mType = ASTERISK_MRKR; break;
				case kDisplay_type_triangles: mType = TRIANGLE_MRKR; break;
				case kDisplay_type_Xs: mType = X_MRKR; break;
			}
			for(i=0; i<num; i++)
			{
				int index = blockSel[i];
				pos[0] = chPos->GetValue(index);
				gw->marker(pos, mType);
				if (gw->checkHitCode()) {
					res = TRUE;
					if (firstHit) hitData = new PFHitData(pgNode, theActionListNode);
					vpt->LogHit(psNode, (ModContext*)pSystem, gw->getHitDistance(), chID ? chID->GetParticleBorn(index) : 0, hitData);
					if (firstHit) {
						hitData = NULL;
						firstHit = false;
					}
					gw->clearHitCode();
					if (abortOnHit) goto hasHit;
				}
			}
		}
	}

	if ((ordCat == geomCat) || (selCat == geomCat))
	{
		DWORD prevLimits = savedLimits;
		DWORD boxLimits = savedLimits;
		if (!(boxLimits&GW_BOX_MODE)) boxLimits |= GW_BOX_MODE;

		curMtl = SysUtil::GetParticleMtl();
		numMtls = 1;
		if (pgNode->Mtls() != NULL) {
			curMtl = pgNode->Mtls();
			numMtls = pgNode->NumMtls();
		}

		for(int runType = 0; runType < 2; runType++)
		{
			Tab<int> *block = NULL;
		
			if (runType) {
				if (unselOnly) continue;
				if (selNum == 0) continue;
				if (selCat != geomCat) continue;
				if (selectedType == kDisplay_type_boundingBoxes)
					gw->setRndLimits(boxLimits);
				else
					gw->setRndLimits(prevLimits);
				block = &blockSel;
			} else {
				if (selOnly) continue;
				if (nonNum == 0) continue;
				if (ordCat != geomCat) continue;
				if (displayType == kDisplay_type_boundingBoxes)
					gw->setRndLimits(boxLimits);
				else
					gw->setRndLimits(prevLimits);
				block = &blockNon;
			}

			for(i=0; i<block->Count(); i++) {
				index = (*block)[i];
				Mesh* pMesh = NULL;
				if (chShape != NULL)
					pMesh = const_cast <Mesh*>(chShape->GetValue(index));
				if (pMesh) {
					if (!MaxSDK::Graphics::IsHardwareHitTesting(vpt))
					{
						gwTM.IdentityMatrix();
						if (chOrient) gwTM.SetRotate(chOrient->GetValue(index));
						if (chScale) gwTM.PreScale(chScale->GetValue(index));
						gwTM.SetTrans(chPos->GetValue(index));
						gw->setTransform(gwTM);
						pMesh->select(gw, curMtl, &hr, TRUE, numMtls);
					}
				} else {
					pos[0] = chPos->GetValue(index);
					gw->marker(pos, X_MRKR);
				}

				if (gw->checkHitCode()) {
					res = TRUE;
					if (firstHit) hitData = new PFHitData(pgNode, theActionListNode); // the extended data information is attached to the hit of the first particle only to save on memory allocations
					vpt->LogHit(psNode, (ModContext*)pSystem, gw->getHitDistance(), chID ? chID->GetParticleBorn(index) : 0, hitData);
					if (firstHit) {
						hitData = NULL;
						firstHit = false;
					}
					gw->clearHitCode();
					if (abortOnHit) goto hasHit;
				}
			}
		}
		gwTM.IdentityMatrix();
		gw->setTransform(gwTM);
		gw->setRndLimits(prevLimits);
	}

hasHit:
	gw->setRndLimits(savedLimits);
	return res;
}

//+--------------------------------------------------------------------------+
//|							From IPFViewport								 |
//+--------------------------------------------------------------------------+
void PFOperatorDisplay::GetWorldBoundBox(IObject* pCont, TimeValue time, Object* pSystem, INode* inode, ViewExp* /*vp*/, Box3& box )
{

	int displayType = pblock()->GetInt(kDisplay_type);
	int selectedType = pblock()->GetInt(kDisplay_selectedType);

	// all particles are visible in 'select particles' mode
	if (pSystem == NULL) return;
	bool subSelPartMode = (pSystem->GetSubselState() == 1);
	if (subSelPartMode) {
		if (displayType == kDisplay_type_none)
			displayType = kDisplay_type_dots;
		if (selectedType == kDisplay_type_none)
			selectedType = kDisplay_type_dots;
	}

	if ((displayType == kDisplay_type_none) &&
		(selectedType == kDisplay_type_none)) return; // nothing to draw

	// acquire particle channels
	IParticleChannelAmountR* chAmount = GetParticleChannelAmountRInterface(pCont);
	if (chAmount == NULL) return; // can't find number of particles in the container
	int count = chAmount->Count();
	if (count == 0) return; // there no particles to draw
	IParticleChannelPoint3R* chPos = GetParticleChannelPositionRInterface(pCont);	
	if (chPos == NULL) return; // can't find particle position in the container

	box += chPos->GetBoundingBox();
	
	if ((displayType == kDisplay_type_lines) || (selectedType == kDisplay_type_lines))
	{
		IParticleChannelPoint3R* chSpeed = GetParticleChannelSpeedRInterface(pCont);
		if (chSpeed != NULL) 
			box.EnlargeBy(GetTicksPerFrame()*chSpeed->GetMaxLengthValue());
	}
	else if ((displayType == kDisplay_type_boundingBoxes) || 
		     (displayType == kDisplay_type_geometry) ||
			 (selectedType == kDisplay_type_boundingBoxes) || 
		     (selectedType == kDisplay_type_geometry))
	{
		IParticleChannelMeshR* chShape = GetParticleChannelShapeRInterface(pCont);
		if (chShape != NULL)	{
			Box3 meshBox = chShape->GetMaxBoundingBox();
			if (!meshBox.IsEmpty())	{
				Point3 pmin = meshBox.Min();
				Point3 pmax = meshBox.Max();
				Point3 h( max(fabs(pmin.x),fabs(pmax.x)), max(fabs(pmin.y),fabs(pmax.y)), max(fabs(pmin.z),fabs(pmax.z)) );
				float enlargeFactor = Length(h);
				IParticleChannelPoint3R* chScale = GetParticleChannelScaleRInterface(pCont);
				if (chScale)
					enlargeFactor *= chScale->GetMaxLengthValue();
				box.EnlargeBy(enlargeFactor);
			}
		}
	}
}

//+--------------------------------------------------------------------------+
//|							From IPFViewport								 |
//+--------------------------------------------------------------------------+
void PFOperatorDisplay::GetLocalBoundBox(IObject* pCont, TimeValue time, Object* pSystem, INode* inode, ViewExp* vp, Box3& box )
{
	if ( ! vp || ! vp->IsAlive() )
	{
		box.Init();
		return;
	}


	Box3 pbox;
	GetWorldBoundBox(pCont, time, pSystem, inode, vp, pbox);
	if (!pbox.IsEmpty())
		box += pbox*Inverse(inode->GetObjTMBeforeWSM(time));
}

//+--------------------------------------------------------------------------+
//|							From IPFViewport								 |
//+--------------------------------------------------------------------------+
DWORD PFOperatorDisplay::GetWireColor() const
{
	return (pblock()->GetColor(kDisplay_color, 0)).toRGB();
}

//+--------------------------------------------------------------------------+
//|							From IPFViewport								 |
//+--------------------------------------------------------------------------+
void PFOperatorDisplay::SetWireColor(DWORD color)
{
	pblock()->SetValue(kDisplay_color, 0, Color(color));
}

//+--------------------------------------------------------------------------+
//|							From IPFViewport								 |
//+--------------------------------------------------------------------------+
void PFOperatorDisplay::MaybeEnlargeViewportRect(IObject* pCont, TimeValue time, GraphicsWindow *gw, Rect &rect)
{
	// check out if writing particles IDs
	bool showNumbers = (pblock()->GetInt(kDisplay_showNumbering, time) != 0);
	if (!showNumbers) return;

	// acquire particle channels
	IParticleChannelAmountR* chAmount = GetParticleChannelAmountRInterface(pCont);
	if (chAmount == NULL) return; // can't find number of particles in the container
	int count = chAmount->Count();
	if (count == 0) return; // there no particles to draw
	IParticleChannelIDR* chID = GetParticleChannelIDRInterface(pCont);	
	if (chID == NULL) return; // can't find particle position in the container
	int maxNumber = 0;
	for(int i=0; i<count; i++) {
		int curNum = chID->GetParticleBorn(i);
		if (curNum > maxNumber) maxNumber = curNum;
	}
	TCHAR dummy[256];
	SIZE size;
	_stprintf_s(dummy, 256, _T("%d"), maxNumber);
	gw->getTextExtents(dummy, &size);
	rect.SetW(rect.w() + size.cx);
	rect.SetY(rect.y() - size.cy);
	rect.SetH(rect.h() + size.cy);
}

//+--------------------------------------------------------------------------+
//|							From PFOperatorDisplay							 |
//+--------------------------------------------------------------------------+
void PFOperatorDisplay::UpdatePViewUI(ParamID updateID) const
{
	for(int i=0; i<NumPViewParamMaps(); i++) {
		IParamMap2* map = GetPViewParamMap(i);
		map->Invalidate(updateID);
		Interval currentTimeInterval;
		currentTimeInterval.SetInstant(GetCOREInterface()->GetTime());
		map->Validity() = currentTimeInterval;
	}
}

//+--------------------------------------------------------------------------+
//|							From PFOperatorDisplay							 |
//+--------------------------------------------------------------------------+
bool PFOperatorDisplay::isInvisible(int index)
// index is 1-based
{
	if (index <= 0) return false;
	if (index > invisibleParticles().GetSize()) recalcVisibility(index+1023);	
	return (invisibleParticles()[index-1] != 0);
}

//+--------------------------------------------------------------------------+
//|							From PFOperatorDisplay							 |
//+--------------------------------------------------------------------------+
void PFOperatorDisplay::recalcVisibility(int amount)
{
	if (amount == 0) {
		_invisibleParticles().SetSize(0);
		return;
	}
	if (amount < invisibleParticles().GetSize()) {
		_invisibleParticles().SetSize(amount, 1);
		return;
	}
	float visPercent = GetPFFloat(pblock(), kDisplay_visible, 0);
	DbgAssert(visPercent < 1.0f);

	int oldNum = invisibleParticles().GetSize();
	int visible = oldNum - _invisibleParticles().NumberSet();
	_invisibleParticles().SetSize(amount, 1);
	if (oldNum == 0) {
		visible = 1;
		_invisibleParticles().Clear(0);
		oldNum = 1;
	}
	for(int i=oldNum; i<amount; i++) {
		if (float(visible)/i > visPercent) {
			_invisibleParticles().Set(i);
		} else {
			_invisibleParticles().Clear(i);
			visible++;
		}
	}
}

//+--------------------------------------------------------------------------+
//|							From IPViewItem									 |
//+--------------------------------------------------------------------------+
HBITMAP PFOperatorDisplay::GetActivePViewIcon()
{
	if (activeIcon() == NULL)
		_activeIcon() = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_DISPLAY_ACTIVEICON),IMAGE_BITMAP,
//									kActionImageWidth, kActionImageHeight, LR_LOADTRANSPARENT|LR_SHARED);
									kActionImageWidth, kActionImageHeight, LR_SHARED);
	return activeIcon();
}

//+--------------------------------------------------------------------------+
//|							From IPViewItem									 |
//+--------------------------------------------------------------------------+
HBITMAP PFOperatorDisplay::GetInactivePViewIcon()
{
	if (inactiveIcon() == NULL)
		_inactiveIcon() = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_DISPLAY_INACTIVEICON),IMAGE_BITMAP,
//									kActionImageWidth, kActionImageHeight, LR_LOADTRANSPARENT|LR_SHARED);
									kActionImageWidth, kActionImageHeight, LR_SHARED);

	return inactiveIcon();
}

//+--------------------------------------------------------------------------+
//|							From IPViewItem									 |
//+--------------------------------------------------------------------------+
bool PFOperatorDisplay::HasDynamicName(TSTR& nameSuffix)
{
	int type	= pblock()->GetInt(kDisplay_type, 0);
	int ids = 0;
	switch(type) {
	case kDisplay_type_none:			ids = IDS_DISPLAYTYPE_NONE;		break;
	case kDisplay_type_dots:			ids = IDS_DISPLAYTYPE_DOTS;		break;
	case kDisplay_type_ticks:			ids = IDS_DISPLAYTYPE_TICKS;	break;
	case kDisplay_type_circles:			ids = IDS_DISPLAYTYPE_CIRCLES;	break;
	case kDisplay_type_lines:			ids = IDS_DISPLAYTYPE_LINES;	break;
	case kDisplay_type_boundingBoxes:	ids = IDS_DISPLAYTYPE_BBOXES;	break;
	case kDisplay_type_geometry:		ids = IDS_DISPLAYTYPE_GEOM;		break;
	case kDisplay_type_diamonds:		ids = IDS_DISPLAYTYPE_DIAMONDS;	break;
	case kDisplay_type_boxes:			ids = IDS_DISPLAYTYPE_BOXES;	break;
	case kDisplay_type_asterisks:		ids = IDS_DISPLAYTYPE_ASTERISKS;break;
	case kDisplay_type_triangles:		ids = IDS_DISPLAYTYPE_TRIANGLES;break;
	}
	nameSuffix = GetString(ids);
	return true;
}

ClassDesc* PFOperatorDisplay::GetClassDesc() const
{
	return GetPFOperatorDisplayDesc();
}

// here are the 256 default object colors
struct ObjectColors { BYTE	r, g, b; }; 
static ObjectColors objectColors[] = {
	// basic colors
		0xFF,0xB9,0xEF, 0xEE,0xFF,0xB9, 0xB9,0xFF,0xFF, 0xFD,0xAA,0xAA,
		0xF9,0x60,0x60, 0xC4,0x1D,0x1D, 0x96,0x07,0x07, 0xFE,0xCD,0xAB,
		0xFA,0x9F,0x61, 0xC5,0x62,0x1E, 0x96,0x42,0x09, 0xFE,0xEE,0xAB,
		0xFA,0xDD,0x61, 0xC5,0xA5,0x1E, 0x96,0x7B,0x09, 0xEE,0xFE,0xAB,
		0xDD,0xFA,0x61, 0xA5,0xC5,0x1E, 0x7E,0x96,0x07, 0xCD,0xFE,0xAB,
		0x9F,0xFA,0x61, 0x62,0xC5,0x1E, 0x44,0x96,0x07, 0xAB,0xFE,0xAB,
		0x61,0xFA,0x61, 0x1E,0xC5,0x1E, 0x07,0x96,0x07, 0xAB,0xFE,0xCD,
		0x61,0xFA,0x9F, 0x1E,0xC5,0x62, 0x07,0x96,0x41, 0xAB,0xFE,0xEE,
		0x61,0xFA,0xDD, 0x1E,0xC5,0xA5, 0x07,0x96,0x7E, 0xAC,0xEF,0xFF,
		0x62,0xDE,0xFB, 0x20,0xA6,0xC5, 0x09,0x7B,0x96, 0xAC,0xCE,0xFF,
		0x62,0xA0,0xFB, 0x20,0x63,0xC5, 0x09,0x44,0x9A, 0xAC,0xAC,0xFF,
		0x62,0x62,0xFB, 0x20,0x20,0xC5, 0x09,0x09,0x98, 0xCD,0xAD,0xFF,
		0x9C,0x62,0xFB, 0x5F,0x20,0xC5, 0x40,0x09,0x98, 0xED,0xAC,0xFF,
		0xDA,0x62,0xFB, 0xA2,0x20,0xC5, 0x79,0x09,0x98, 0xFF,0xAC,0xEF,
		0xFB,0x62,0xDE, 0xC5,0x20,0xA6, 0x9A,0x09,0x7B, 0xFE,0xAB,0xCD,
		0xFA,0x61,0x9F, 0xC5,0x1E,0x62, 0x9D,0x08,0x41, 0x60,0x60,0x60
};

DWORD GetNewObjectColor()
{
	int index = rand()%64 + 1;
	return RGB(objectColors[index].r, objectColors[index].g, objectColors[index].b);
}



} // end of namespace PFActions
