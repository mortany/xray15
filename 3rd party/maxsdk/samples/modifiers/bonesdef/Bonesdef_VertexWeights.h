

/*****************************************************************

  This is just a header that contains all our vertex attribute and weight
  classes

******************************************************************/
#pragma once

#include "point3.h"
#include "bitarray.h"
#include "tab.h"
#include <assert1.h>
#include <vector>
#include <map>
#include <memory>

// this contains our vertex weight info
// basically it contains a index into the bones list, the weight and bunch of optional spline parameters
class VertexInfluenceListClass
{
public:
	int Bones; // this is an index into the bones list,which bones belongs to the weight
	float Influences; // this is the unnormalized weight for the Bones
	float normalizedInfluences; // this is the normalized weight for Bones

	// extra data to hold spline stuff
	// this contains info on the closest point on the spline to the vertex
	// the data is based on the initial spline position
	int SubCurveIds; // this which curve the point is closest to NOTE NOT USED YET
	int SubSegIds; // this is the closest seg
	float u; // this is the U value along the segment which is the closest point to the vertex
	Point3 Tangents; // this is the tangent
	Point3 OPoints; // this is the point in spline space

	VertexInfluenceListClass()
	{
		Bones = 0;
		Influences = 0.0f;
		normalizedInfluences = 0.0f;
		// extra data to hold spline stuff
		SubCurveIds = -1;
		SubSegIds = -1;
		u = 0.0f;
		Tangents = Point3(1.0f, 0.0f, 0.0f);
		OPoints = Point3(0.0f, 0.0f, 0.0f);
	}
};

class VertexListClass;
//
// a class to hold all the VertexInluenceListClass instances in a Tab<> data member.
// it allows having one place to hold linearily all the data and allows updates in case of deletion/insertion.
// an instance of this class is referenced by each VertexListClass instance of the pVertexData member.
//
struct BoneVertexDataManager
{
	BoneVertexDataManager();
	~BoneVertexDataManager() = default;

	void SetupData();
	void SetVertexDataPtr(Tab<VertexListClass*>* pVData);
	void DeleteItem(int id, int index);
	bool AppendItem(int id, int index, VertexInfluenceListClass& w);
	int GetDataStart(int id) const;
	VertexInfluenceListClass& GetItemAtIndex(int id, int index);
	void UpdateDataAlignment(int newMaxSize);
	bool IsValidIndex(int id, int index);
	int ReplaceData(int id, Tab<VertexInfluenceListClass>& w);
	int PastWeights(int id, Tab<VertexInfluenceListClass>& w);


	const static auto INCREMENT_SIZE{10};
	int maxSize{ INCREMENT_SIZE };
	//std::map<int, int> mapId2DataStart;
	Tab<VertexListClass*>* pVertexData;
	Tab<VertexInfluenceListClass> allInfluenceList;
};
using BoneVertexMgr = std::shared_ptr<BoneVertexDataManager>;

// this is the per vertex data class
// it contains all the attributes of the vertex and a list of weights for this vertex
class VertexListClass
{
public:
	VertexListClass();
	VertexListClass(VertexListClass& from);
	VertexListClass& operator=(VertexListClass & rhs);
	// These are get/sets for our flag properties
	// the properties are
	// Modified		whether the vertex has been hand weighted
	// Unormalized  whether the vertex is normalized
	// Rigid		whether the vertex is rigid,if it is rigid only one bone will be affect the vertex
	// Rigid Handle only applies to patches, when set if it is a handle it will use the weights of the knot that owns
	// the handle TempSelected	used internally to hold temporary selections for cut and paste
	BOOL IsSelectedTemp() const;
	BOOL IsModified() const;
	BOOL IsUnNormalized() const;
	BOOL IsRigid() const;
	BOOL IsRigidHandle() const;
	BOOL IsHidden() const;
	// gets/sets whether the vertex is writing to the skin weight or the DQ blend weight
	BOOL IsDQOverride() const;
	void DQOverride(BOOL override);
	void SelectedTemp(BOOL sel);
	void Modified(BOOL modify);
	void UnNormalized(BOOL unnormalized);
	void Rigid(BOOL rigid);
	void RigidHandle(BOOL rigidHandle);
	void Hide(BOOL hide);
	VertexInfluenceListClass& GetItemAtIndex(int index);
	// Toggles bits in a bitarray for all bones affecting this vertex,
	// which have normalized influence greater than a given threshold.
	// Does NOT zero any bits previously toggled in the bitarray
	int GetAllAffectedBones(BitArray& bones, float threshold);
	// this returns the bone that most affects this vertex
	int GetMostAffectedBone();
	// this returns the ith index of the bone that most affects this vertex
	int GetMostAffectedBoneID();
	// this loops though the unnormalized weights
	// and stuffs the normalized values in the normalizedInfluences variable
	void NormalizeWeights();
	int WeightCount();
	void SetWeightCount(int ct);
	void ZeroWeights();
	int GetBoneIndex(int index);
	int FindWeightIndex(int boneIndex);
	void SetBoneIndex(int index, int boneIndex);
	float GetNormalizedWeight(int index);
	float GetWeight(int index);
	float GetWeightByBone(int boneIndex, bool raw, bool normalized);
	void SetWeight(int index, float w);
	void SetNormalizedWeight(int index, float w);
	void SetWeightByBone(int boneIndex, float w, bool raw, bool normalize);
	void SetWeightInfo(int index, int boneIndex, float w, float nw);
	float GetCurveU(int index);
	int GetCurveID(int index);
	int GetSegID(int index);
	Point3 GetOPoint(int index);
	Point3 GetTangent(int index);
	void SetCurveU(int index, float u);
	void SetCurveID(int index, int id);
	void SetSegID(int index, int id);
	void SetOPoint(int index, const Point3& p);
	void SetTangent(int index, const Point3& p);
	void SetWeightSplineInfo(int index, float u, int curve, int seg, const Point3& p, const Point3& t);
	void DeleteWeight(int index);
	void AppendWeight(VertexInfluenceListClass& w);
	VertexInfluenceListClass CopySingleWeight(int index);
	void PasteSingleWeight(int index, VertexInfluenceListClass& td);
	Tab<VertexInfluenceListClass> CopyWeights();
	void PasteWeights(Tab<VertexInfluenceListClass>& w);
	int* GetBoneIndexAddr(int index);
	float* GetNormalizedWeightAddr(int index);
	float* GetCurveUAddr(int index);
	int* GetCurveIDAddr(int index);
	int* GetSegIDAddr(int index);
	Point3* GetOPointAddr(int index);
	Point3* GetTangentAddr(int index);
	int GetClosestBone() const;
	void SetClosestBone(int bid);
	int GetClosestBoneCache() const;
	void SetClosestBoneCache(int bid);
	float GetDQBlendWeight() const;
	void SetDQBlendWeight(float weight);
	void Sort();
	void SetVertexDataManager(int id, BoneVertexMgr mgr);
	BoneVertexMgr GetVertexDataManager() const;
	int GetTotalItems() const;
	void RemoveWeightsBelowThreshold(float value);

	DWORD flags{ 0 };
	Point3 LocalPos{ Point3(0.0f, 0.0f, 0.0f) }; // this is local position of the vertex before any skin deformation
	Point3 LocalPosPostDeform{ Point3(0.0f, 0.0f, 0.0f) }; // this is local position of the vertex before after skin deformation

private:
	bool onLoad{ false };
	bool needUpdate{ false };
	// holds all the VertexInluenceListClass instances plus update hanlder for items delete
	BoneVertexMgr mgrVertexData;

	int dataId{ -1 };
	int totalItems{ 0 };
	int closestBone{-1};
	int closestBoneCache{-1};
	float closetWeight{1.0f};
	float mDQBlendWeight{0.0f}; // the Dual Quaternion blend weight for this vertex, this is how much the DQ solver will
						  // controiute to the deformation for this vertex
public:
	Tab<VertexInfluenceListClass> tabInfluenceList; // this is used for tempoarary data in case a copy constructor is used.
};

// this class is used to cache our distances
// every time a bone is selected all the distances are computed from this bone
// and stored in a table of this class
class VertexDistanceClass
{
public:
	float dist;
	float u;
	int SubCurveIds;
	int SubSegIds;
	Point3 Tangents;
	Point3 OPoints;
};

// this is a legacy class to load older files
// THIS SHOULD NOT BE CHANGED
class VertexListClassOld
{
public:
	BOOL selected;
	BOOL modified;
	Point3 LocalPos;
	// table of misc data
	Tab<VertexInfluenceListClass> d;
	int GetMostAffectedBone()
	{
		if (d.Count() == 0)
			return -1;
		int largestID = d[0].Bones;
		float largestVal = d[0].Influences;
		for (int i = 1; i < (d.Count()); i++)
		{
			for (int j = i; j < d.Count(); j++)
			{
				if (d[j].Influences > largestVal)
				{
					largestVal = d[j].Influences;
					largestID = d[j].Bones;
				}
			}
		}
		return largestID;
	}
	int GetMostAffectedBoneID()
	{
		if (d.Count() == 0)
			return -1;
		int largestID = 0;
		float largestVal = d[0].Influences;
		for (int i = 1; i < (d.Count()); i++)
		{
			for (int j = i; j < d.Count(); j++)
			{
				if (d[j].Influences > largestVal)
				{
					largestVal = d[j].Influences;
					largestID = j;
				}
			}
		}
		return largestID;
	}
};

class INode;
class VertexWeightCopyBuffer
{
public:
	Tab<INode*> bones;
	Tab<VertexListClass*> copyBuffer;

	VertexWeightCopyBuffer() = default;
	~VertexWeightCopyBuffer()
	{
		FreeCopyBuffer();
	}
	void FreeCopyBuffer()
	{
		for (int i = 0; i < copyBuffer.Count(); i++)
			delete copyBuffer[i];
	}
};
