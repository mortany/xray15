#include "Bonesdef_VertexWeights.h"
#include "BonesDef_Constants.h"

namespace
{
	void TabToVector(const Tab<int>& tab, std::vector<int>& v)
	{
		for (auto i = 0; i < tab.Count(); ++i)
		{
			v.push_back(tab[i]);
		}
	}

	static int InfluSort(const void* elem1, const void* elem2)
	{
		auto* a = (VertexInfluenceListClass*)elem1;
		auto* b = (VertexInfluenceListClass*)elem2;
		if (a->Influences == b->Influences)
			return 0;
		if (a->Influences > b->Influences)
			return -1;
		else
			return 1;
	}
};

// BoneVertexDataManager functions
BoneVertexDataManager::BoneVertexDataManager()
	: pVertexData(nullptr)
{
}

void BoneVertexDataManager::SetupData()
{
	if (pVertexData && (pVertexData->Count() > 0))
	{
		// do I need to do something here before resetting the count?
		allInfluenceList.SetCount(pVertexData->Count()*maxSize);
	}
}

void BoneVertexDataManager::SetVertexDataPtr(Tab<VertexListClass*>* pVData)
{
	pVertexData = pVData;
	SetupData();
}

void BoneVertexDataManager::DeleteItem(int id, int index)
{
	const auto start = GetDataStart(id);
	if (start >= 0)
	{
		for (auto i = index; i < maxSize - 1; ++i)
		{
			allInfluenceList[start + index] = allInfluenceList[start + index + 1];
		}
	}
}

bool BoneVertexDataManager::AppendItem(int id, int index, VertexInfluenceListClass& w)
{
	if ((index) >= maxSize) {
		return false;
	}

	const auto start = GetDataStart(id);
	if (start >= 0)
	{
		allInfluenceList[start + index] = w;
		return true;
	}
	return false;
}

int BoneVertexDataManager::GetDataStart(int id) const
{
	return id * maxSize;
}

VertexInfluenceListClass& BoneVertexDataManager::GetItemAtIndex(int id, int index)
{
	const auto start = GetDataStart(id);
	return allInfluenceList[start + index];
}

void BoneVertexDataManager::UpdateDataAlignment(int newMaxSize)
{
	if (!pVertexData)
		return;

	if (newMaxSize > maxSize)
	{
		const auto count = pVertexData->Count();
		const auto totalSegs = allInfluenceList.Count() / maxSize;
		allInfluenceList.SetCount(count * newMaxSize);
		for (auto i = (totalSegs - 1); i >= 0; --i)
		{
			for (auto j = 0; j < maxSize; ++j)
			{
				allInfluenceList[i * newMaxSize + j] = allInfluenceList[i * maxSize + j];
			}
		}
		maxSize = newMaxSize;
	}
}

bool BoneVertexDataManager::IsValidIndex(int id, int index)
{
	const auto start = GetDataStart(id);
	return (index < maxSize) && ((start + index) < allInfluenceList.Count());
}

int BoneVertexDataManager::ReplaceData(int id, Tab<VertexInfluenceListClass>& w)
{
	if (w.Count() > maxSize)
	{
		UpdateDataAlignment(w.Count() + INCREMENT_SIZE);
	}

	const auto start = GetDataStart(id);
	if (start >= 0)
	{
		for (auto i = start; i < (start + w.Count()); ++i)
		{
			allInfluenceList[i] = w[i - start];
		}
		return w.Count();
	}
	return -1;
}

int BoneVertexDataManager::PastWeights(int id, Tab<VertexInfluenceListClass>& w)
{
	return ReplaceData(id, w);
}

// VertexListClass functions
VertexListClass::VertexListClass()
{
	Modified(FALSE);
}

VertexListClass::VertexListClass(VertexListClass& from)
{
	flags = from.flags;
	LocalPos = from.LocalPos;
	LocalPosPostDeform = from.LocalPosPostDeform;
	closestBone = from.closestBone;
	closestBoneCache = from.closestBoneCache;
	closetWeight = from.closetWeight;
	mDQBlendWeight = from.mDQBlendWeight;
	totalItems = 0;
	// with the copy ctor, we don't have a chance to set the data manager, so we hold on the data,
	// and when the data manager is set, it will check tabInfluenceList and use it to update the state 
	// of the instance
	tabInfluenceList.SetCount(from.totalItems);
	for (auto i = 0; i < from.totalItems; ++i)
	{
		auto &item(from.GetItemAtIndex(i));
		tabInfluenceList[i] = item;
	}
}

VertexListClass& VertexListClass::operator=(VertexListClass & rhs)
{
	if (this != &rhs)
	{
		flags = rhs.flags;
		LocalPos = rhs.LocalPos;
		LocalPosPostDeform = rhs.LocalPosPostDeform;
		closestBone = rhs.closestBone;
		closestBoneCache = rhs.closestBoneCache;
		closetWeight = rhs.closetWeight;
		mDQBlendWeight = rhs.mDQBlendWeight;
		if (mgrVertexData)
		{
			totalItems = rhs.totalItems;
			Tab<VertexInfluenceListClass> temp;
			temp.SetCount(rhs.totalItems);
			for (auto i = 0; i < rhs.totalItems; ++i)
			{
				temp[i] = rhs.GetItemAtIndex(i);
			}
			totalItems = mgrVertexData->ReplaceData(dataId, temp);
		}
	}
	return *this;
}

BOOL VertexListClass::IsSelectedTemp() const
{
	if (flags & VERTEXFLAG_TEMPSELECTED)
		return TRUE;
	else
		return FALSE;
}

BOOL VertexListClass::IsModified() const
{
	if (flags & VERTEXFLAG_DQWEIGHT_OVERRIDE) // if we are in the DQ blend override all dq blend weights are
											  // considered modified
		return TRUE;
	if (flags & VERTEXFLAG_MODIFIED)
		return TRUE;
	else
		return FALSE;
}

BOOL VertexListClass::IsUnNormalized() const
{
	if (flags & VERTEXFLAG_DQWEIGHT_OVERRIDE) // only have one weight so this has no meaning for the DQ override
											  // mode
		return TRUE;

	if (flags & VERTEXFLAG_UNNORMALIZED)
		return TRUE;
	else
		return FALSE;
}

BOOL VertexListClass::IsRigid() const
{
	if (flags & VERTEXFLAG_RIGID)
		return TRUE;
	else
		return FALSE;
}

BOOL VertexListClass::IsRigidHandle() const
{
	if (flags & VERTEXFLAG_RIGIDHANDLE)
		return TRUE;
	else
		return FALSE;
}

BOOL VertexListClass::IsHidden() const
{
	if (flags & VERTEXFLAG_HIDDEN)
		return TRUE;
	else
		return FALSE;
}

// gets/sets whether the vertex is writing to the skin weight or the DQ blend weight
BOOL VertexListClass::IsDQOverride() const
{
	if (flags & VERTEXFLAG_DQWEIGHT_OVERRIDE)
		return TRUE;
	else
		return FALSE;
}

void VertexListClass::DQOverride(BOOL override)
{
	if (override)
		flags |= VERTEXFLAG_DQWEIGHT_OVERRIDE;
	else
		flags &= ~VERTEXFLAG_DQWEIGHT_OVERRIDE;
}

void VertexListClass::SelectedTemp(BOOL sel)
{
	if (sel)
		flags |= VERTEXFLAG_TEMPSELECTED;
	else
		flags &= ~VERTEXFLAG_TEMPSELECTED;
}

void VertexListClass::Modified(BOOL modify)
{
	if (IsDQOverride()) // bail if we are in DQ overried mode
		return;

	if (modify)
	{
		if (!IsModified())
		{
			if (IsUnNormalized())
			{
				for (auto i = 0; i < totalItems; i++) {
					auto &item(GetItemAtIndex(i));
					item.Influences = item.normalizedInfluences;
				}
			}
			else
			{
				NormalizeWeights();
			}
		}
		flags |= VERTEXFLAG_MODIFIED;
		if (!onLoad)
			needUpdate = true;
	}
	else
		flags &= ~VERTEXFLAG_MODIFIED;
}

void VertexListClass::UnNormalized(BOOL unnormalized)
{
	if (IsDQOverride()) // bail if we are in DQ overried mode
		return;

	if (unnormalized)
	{
		flags |= VERTEXFLAG_UNNORMALIZED;
		if (!IsModified())
		{
			Modified(TRUE);
		}
	}
	else
	{
		const BOOL wasNormal = !(flags | VERTEXFLAG_UNNORMALIZED);
		flags &= ~VERTEXFLAG_UNNORMALIZED;
		if (wasNormal)
			Modified(TRUE);
	}
}

void VertexListClass::Rigid(BOOL rigid)
{
	if (IsDQOverride()) // bail if we are in DQ overried mode
		return;

	if (rigid)
		flags |= VERTEXFLAG_RIGID;
	else
		flags &= ~VERTEXFLAG_RIGID;
}

void VertexListClass::RigidHandle(BOOL rigidHandle)
{
	if (IsDQOverride()) // bail if we are in DQ overried mode
		return;

	if (rigidHandle)
		flags |= VERTEXFLAG_RIGIDHANDLE;
	else
		flags &= ~VERTEXFLAG_RIGIDHANDLE;
}

void VertexListClass::Hide(BOOL hide)
{
	if (hide)
		flags |= VERTEXFLAG_HIDDEN;
	else
		flags &= ~VERTEXFLAG_HIDDEN;
}

VertexInfluenceListClass& VertexListClass::GetItemAtIndex(int index)
{
	return mgrVertexData->GetItemAtIndex(dataId, index);
}

// Toggles bits in a bitarray for all bones affecting this vertex,
// which have normalized influence greater than a given threshold.
// Does NOT zero any bits previously toggled in the bitarray
int VertexListClass::GetAllAffectedBones(BitArray& bones, float threshold)
{
	if ((totalItems == 0) && (closestBone != -1))
	{
		bones.Set(closestBone);
		return 1;
	}

	if (totalItems == 0)
		return 0;

	auto count = 0;
	for (auto i = 0; i < totalItems; i++)
	{
		const VertexInfluenceListClass &item(GetItemAtIndex(i));
		const auto boneVal = item.normalizedInfluences;
		if (boneVal >= threshold)
		{
			const auto boneID = GetItemAtIndex(0).Bones;
			bones.Set(boneID);
			count++;
		}
	}
	return count;
}

// this returns the bone that most affects this vertex
int VertexListClass::GetMostAffectedBone()
{
	if ((totalItems == 0) && (closestBone != -1))
		return closestBone;

	if (totalItems == 0)
		return -1;

	auto largestID = GetItemAtIndex(0).Bones;
	auto largestVal = GetItemAtIndex(0).Influences;
	for (auto i = 1; i < totalItems; i++)
	{
		for (auto j = i; j < totalItems; j++)
		{
			VertexInfluenceListClass &item(GetItemAtIndex(j));
			if (item.Influences > largestVal)
			{
				largestVal = item.Influences;
				largestID = item.Bones;
			}
		}
	}
	return largestID;
}

// this returns the ith index of the bone that most affects this vertex
int VertexListClass::GetMostAffectedBoneID()
{
	if ((totalItems == 0) && (closestBone != -1))
		return 0;

	if (totalItems == 0)
		return -1;

	auto largestID = 0;
	auto largestVal = GetItemAtIndex(0).Influences;
	for (auto i = 1; i < totalItems; i++)
	{
		for (auto j = i; j < totalItems; j++)
		{
			VertexInfluenceListClass &item(GetItemAtIndex(j));
			if (item.Influences > largestVal)
			{
				largestVal = item.Influences;
				largestID = j;
			}
		}
	}
	return largestID;
}

// this loops though the unnormalized weights
// and stuffs the normalized values in the normalizedInfluences variable
void VertexListClass::NormalizeWeights()
{
	if (IsDQOverride()) // bail if we are in DQ override mode
		return;

	auto sum = 0.0f;
	for (auto i = 0; i < totalItems; i++) {
		sum += GetItemAtIndex(i).Influences;
	}

	for (auto i = 0; i < totalItems; i++)
	{
		VertexInfluenceListClass &item(GetItemAtIndex(i));
		if (sum == 0.0f)
			item.Influences = 0.0f;
		else {
			item.Influences = item.Influences / sum;
		}
		item.normalizedInfluences = item.Influences;
	}
}

int VertexListClass::WeightCount()
{
	if (IsDQOverride()) // only have one 1 weight per vertex in DQ blending
		return 1;

	if ((totalItems == 0) && (closestBone != -1))
		return 1; // using the single-bone optimization, no list

	return totalItems;
}

void VertexListClass::SetWeightCount(int ct)
{
	if (IsDQOverride())
		return;

	mgrVertexData->UpdateDataAlignment(ct);
	totalItems = ct;
}

void VertexListClass::ZeroWeights()
{
	if (IsDQOverride())
	{
		mDQBlendWeight = 0.0f;
		return;
	}

	totalItems = 0;
}

int VertexListClass::GetBoneIndex(int index)
{
	if (IsDQOverride()) // there is no bone associated with the DQ blend weight
	{
		return -1;
	}

	if ((totalItems == 0) && (closestBone != -1))
		return closestBone; // using the single-bone optimization, no list

	return GetItemAtIndex(index).Bones;
}

int VertexListClass::FindWeightIndex(int boneIndex) // index into vertex weight array for given bone, or -1
{
	if ((totalItems == 0) && (closestBone != -1))
	{ // using the single-bone optimization, no list
		// return first index if param matches the single-bone value, else fail
		return (closestBone == boneIndex ? 0 : -1);
	}
	for (auto i = 0; i < totalItems; i++)
		if (GetItemAtIndex(i).Bones == boneIndex)
			return i;

	return -1;
}

void VertexListClass::SetBoneIndex(int index, int boneIndex)
{
	if (IsDQOverride())
	{
		return;
	}

	// support for single-bone optimization, no list
	if ((totalItems == 0) && (closestBone != -1))
	{
		if (index == 0)
		{
			closestBone = boneIndex;
			closestBoneCache = boneIndex;
		}
	}
	if (index < totalItems) {
		GetItemAtIndex(index).Bones = boneIndex;
	}
}

float VertexListClass::GetNormalizedWeight(int index)
{
	if (IsDQOverride()) // return our blend weight when in override mode
	{
		return mDQBlendWeight;
	}

	if ((totalItems == 0) && (closestBone != -1))
		return 1.0f; // using the single-bone optimization, no list

	return GetItemAtIndex(index).normalizedInfluences;
}

float VertexListClass::GetWeight(int index)
{
	if (IsDQOverride())
	{
		return mDQBlendWeight; // return our blend weight when in override mode
	}

	if ((totalItems == 0) && (closestBone != -1))
		return 1.0f; // using the single-bone optimization, no list

	return GetItemAtIndex(index).Influences;
}

float VertexListClass::GetWeightByBone(int boneIndex, bool raw, bool normalized)
{
	if ((totalItems == 0) && (closestBone != -1))
	{ // using the single-bone optimization, no list
		if (closestBone == boneIndex)
			return 1.0f;
		return 0.0f;
	}
	// else search whole list
	for (auto i = 0; i < totalItems; i++)
	{
		VertexInfluenceListClass &item(GetItemAtIndex(i));
		if (item.Bones == boneIndex)
		{
			if (raw)
				return item.Influences;
			else if (normalized)
				return GetNormalizedWeight(i);
			else
				return GetWeight(i);
		}
	}
	return 0.0f;
}

void VertexListClass::SetWeight(int index, float w)
{
	if (IsDQOverride())
	{
		mDQBlendWeight = w; // Set our blend weight when in override mode
		return;
	}

	if (index < totalItems)
		GetItemAtIndex(index).Influences = w;
	else if (closestBone != -1)
	{
		VertexInfluenceListClass vd;
		vd.Bones = closestBone;
		vd.normalizedInfluences = w;
		vd.Influences = w;
		AppendWeight(vd);
	}
}

void VertexListClass::SetNormalizedWeight(int index, float w)
{
	if (IsDQOverride())
	{
		mDQBlendWeight = w; // Set our blend weight when in override mode
		return;
	}

	if (index < totalItems) {
		GetItemAtIndex(index).normalizedInfluences = w;
	}
}

void VertexListClass::SetWeightByBone(int boneIndex, float w, bool raw, bool normalize)
{
	if ((totalItems == 0) && (closestBone != -1))
	{ // using the single-bone optimization, no list
		if (closestBone == boneIndex)
			closetWeight = w;
	}
	else
	{ // set in list, not using single-bone optimization
		for (int i = 0; i < totalItems; i++)
		{
			VertexInfluenceListClass &item(GetItemAtIndex(i));
			if (item.Bones == boneIndex)
			{
				if (raw)
					item.Influences = w;
				else
					SetWeight(i, w);
				return;
			}
		}
		// Not found in list, append
		VertexInfluenceListClass vd;
		vd.Bones = boneIndex;
		vd.normalizedInfluences = w;
		vd.Influences = w;
		AppendWeight(vd);
	}
	if (normalize)
		NormalizeWeights();
}

void VertexListClass::SetWeightInfo(int index, int boneIndex, float w, float nw)
{
	if (IsDQOverride())
	{
		mDQBlendWeight = nw; // Set our blend weight when in override mode
		return;
	}

	if (index < totalItems)
	{
		VertexInfluenceListClass &item(GetItemAtIndex(index));
		item.Bones = boneIndex;
		item.Influences = w;
		item.normalizedInfluences = nw;
	}
}

float VertexListClass::GetCurveU(int index)
{
	if (index >= totalItems)
		return 0.0f;

	return GetItemAtIndex(index).u;
}

int VertexListClass::GetCurveID(int index)
{
	if (index >= totalItems)
		return 0;

	return GetItemAtIndex(index).SubCurveIds;
}

int VertexListClass::GetSegID(int index)
{
	if (index >= totalItems)
		return 0;

	return GetItemAtIndex(index).SubSegIds;
}

Point3 VertexListClass::GetOPoint(int index)
{
	if (index >= totalItems)
		return Point3(0.0f, 0.0f, 0.0f);

	return GetItemAtIndex(index).OPoints;
}

Point3 VertexListClass::GetTangent(int index)
{
	if (index >= totalItems)
		return Point3(0.0f, 0.0f, 0.0f);

	return GetItemAtIndex(index).Tangents;
}

void VertexListClass::SetCurveU(int index, float u)
{
	if (index < totalItems)
	{
		GetItemAtIndex(index).u = u;
	}
}

void VertexListClass::SetCurveID(int index, int id)
{
	if (index < totalItems)
	{
		GetItemAtIndex(index).SubCurveIds = id;
	}
}

void VertexListClass::SetSegID(int index, int id)
{
	if (index < totalItems)
	{
		GetItemAtIndex(index).SubSegIds = id;
	}
}

void VertexListClass::SetOPoint(int index, const Point3& p)
{
	if (index < totalItems)
	{
		GetItemAtIndex(index).OPoints = p;
	}
}

void VertexListClass::SetTangent(int index, const Point3& p)
{
	if (index < totalItems)
	{
		GetItemAtIndex(index).Tangents = p;
	}
}

void VertexListClass::SetWeightSplineInfo(int index, float u, int curve, int seg, const Point3& p, const Point3& t)
{
	if (index < totalItems)
	{
		VertexInfluenceListClass &item(GetItemAtIndex(index));
		item.u = u;
		item.SubCurveIds = curve;
		item.SubSegIds = seg;
		item.OPoints = p;
		item.Tangents = t;
	}
}

void VertexListClass::DeleteWeight(int index)
{
	if (IsDQOverride())
	{
		return;
	}

	if (index < totalItems) {
		mgrVertexData->DeleteItem(dataId, index);
		--totalItems;
	}
}

void VertexListClass::AppendWeight(VertexInfluenceListClass& w)
{
	if (IsDQOverride())
	{
		mDQBlendWeight = w.Influences;
		return;
	}

	mgrVertexData->UpdateDataAlignment(totalItems);
	if (mgrVertexData->AppendItem(dataId, totalItems, w))
	{
		++totalItems;
	}
}

VertexInfluenceListClass VertexListClass::CopySingleWeight(int index)
{
	if (IsDQOverride())
	{
		VertexInfluenceListClass td;
		td.Influences = mDQBlendWeight;
		td.normalizedInfluences = mDQBlendWeight;
		td.Bones = closestBone;
		return td;
	}

	if (index < totalItems)
		return GetItemAtIndex(index);
	else
	{
		VertexInfluenceListClass td;
		td.Influences = 1.0f;
		td.normalizedInfluences = 1.0f;
		td.Bones = closestBone;
		return td;
	}
}

void VertexListClass::PasteSingleWeight(int index, VertexInfluenceListClass& td)
{
	if (IsDQOverride())
	{
		mDQBlendWeight = td.Influences;
		return;
	}

	if (index < totalItems)
		GetItemAtIndex(index) = td;
	else if (totalItems == 0)
	{
		if (mgrVertexData->AppendItem(dataId, totalItems, td))
		{
			++totalItems;
		}
	}
}

Tab<VertexInfluenceListClass> VertexListClass::CopyWeights()
{
	Tab<VertexInfluenceListClass> temp;
	for (auto i = 0; i < totalItems; ++i) {
		auto td = GetItemAtIndex(i);
		temp.Append(1, &td);
	}

	if (IsDQOverride())
	{
		if (temp.Count())
			temp[0].Influences = mDQBlendWeight;
		return temp;
	}
	return temp;
}

void VertexListClass::PasteWeights(Tab<VertexInfluenceListClass>& w)
{
	if (IsDQOverride())
	{
		if (w.Count())
		{
			mDQBlendWeight = w[0].Influences;
		}
		return;
	}
	const auto val = mgrVertexData->PastWeights(dataId, w);
	if (val >= 0)
		totalItems = val;
}

int* VertexListClass::GetBoneIndexAddr(int index)
{
	if (mgrVertexData)
	{
		return &(GetItemAtIndex(index).Bones);
	}
	return nullptr;
}

float* VertexListClass::GetNormalizedWeightAddr(int index)
{
	if ((totalItems == 0) && (closestBone != -1))
		return &closetWeight;

	if (mgrVertexData) {
		return &(GetItemAtIndex(index).normalizedInfluences);
	}
	return nullptr;
}

float* VertexListClass::GetCurveUAddr(int index)
{
	if (mgrVertexData)
	{
		return &(GetItemAtIndex(index).u);
	}
	return nullptr;
}

int* VertexListClass::GetCurveIDAddr(int index)
{
	if (mgrVertexData)
	{
		return &(GetItemAtIndex(index).SubCurveIds);
	}
	return nullptr;
}

int* VertexListClass::GetSegIDAddr(int index)
{
	if (mgrVertexData)
	{
		return &(GetItemAtIndex(index).SubSegIds);
	}
	return nullptr;
}

Point3* VertexListClass::GetOPointAddr(int index)
{
	if (mgrVertexData) {
		return &(GetItemAtIndex(index).OPoints);
	}
	return nullptr;
}

Point3* VertexListClass::GetTangentAddr(int index)
{
	if (mgrVertexData)
	{
		return &(GetItemAtIndex(index).Tangents);
	}
	return nullptr;
}

int VertexListClass::GetClosestBone() const
{
	return closestBone;
}

void VertexListClass::SetClosestBone(int bid)
{
	closestBone = bid;
}

int VertexListClass::GetClosestBoneCache() const
{
	return closestBoneCache;
}

void VertexListClass::SetClosestBoneCache(int bid)
{
	closestBoneCache = bid;
}

float VertexListClass::GetDQBlendWeight() const
{
	return mDQBlendWeight;
}

void VertexListClass::SetDQBlendWeight(float weight)
{
	mDQBlendWeight = weight;
}

void VertexListClass::Sort()
{
	tabInfluenceList.Sort(InfluSort);
	Tab<VertexInfluenceListClass> temp;
	for (auto i = 0; i < totalItems; ++i) {
		auto td = GetItemAtIndex(i);
		temp.Append(1, &td);
	}
	temp.Sort(InfluSort);
	mgrVertexData->ReplaceData(dataId, temp);
}

void VertexListClass::SetVertexDataManager(int id, BoneVertexMgr mgr)
{
	if (mgr) {
		dataId = id;
		mgrVertexData = mgr;
		if (tabInfluenceList.Count() > 0)
		{
			totalItems = mgrVertexData->ReplaceData(dataId, tabInfluenceList);
			tabInfluenceList.ZeroCount();
		}
	}
}

BoneVertexMgr VertexListClass::GetVertexDataManager() const
{
	return mgrVertexData;
}

int VertexListClass::GetTotalItems() const
{
	return totalItems;
}

void VertexListClass::RemoveWeightsBelowThreshold(float threshold)
{
	Tab<VertexInfluenceListClass> temp;
	for (auto k = 0; k < totalItems; ++k) {
		const auto w = GetNormalizedWeight(k);
		if (w > threshold) {
			auto td = GetItemAtIndex(k);
			temp.Append(1, &td);
		}
	}
	totalItems = mgrVertexData->ReplaceData(dataId, temp);
}