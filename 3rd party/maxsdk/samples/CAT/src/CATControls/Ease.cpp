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

// Ease curve for character.
// Simple linear interpolation controller, dont do much, just stores and retreives values

#include "CatPlugins.h"
#include "Ease.h"

 // our global instance of our classdesc class.
static EaseClassDesc EaseDesc;
ClassDesc2* GetEaseDesc() { return &EaseDesc; }

Ease::Ease()
	: keys()	// initialise keys vector
{
	numKeys = 0;
	avgKeyTime = 0;
	avgKeyValue = 0;
}

Ease::~Ease()
{
	DeleteAllRefs();
}

RefTargetHandle Ease::Clone(RemapDir& remap)
{
	// make a new FootBend object to be the clone
	Ease *newEase = new Ease();

	for (int i = 0; i < numKeys; i++)
	{
		stepkey key = keys[i];
		newEase->AppendKey(&key);
	}

	BaseClone(this, newEase, remap);
	return newEase;
}

// We can assume that no key will ever be set that
// will be less than the current max time
void Ease::SetValue(TimeValue t, void *val, int, GetSetMethod)
{
	float newVal = *(float*)val;
	if (!numKeys || ((newVal >= keys[numKeys - 1].val) && (t > keys[numKeys - 1].time)))
	{
		if (numKeys != (int)keys.size())
			numKeys = (int)keys.size();
		stepkey aKey(t, newVal);
		if (numKeys)
		{
			keys[numKeys - 1].SetVect(t, *(float*)val);
			aKey.xVect = keys[numKeys - 1].xVect;
			aKey.yVect = keys[numKeys - 1].yVect;
			avgKeyTime = (t - keys[0].time) / numKeys;
			avgKeyValue = (*(float*)val - keys[0].val) / numKeys;	// Increment numKeys after averaging value's....
		}														// Would love to come up with something faster than
		keys.push_back(aKey);									// this, its fine for short animations, but 10000 keys+? dont know
		numKeys++;
	}
}

void Ease::GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod)
{
	*(float*)val = GetValue(t);
	valid.SetInstant(t);
}

// Given time, get value. Exactly same as GetValue above.
float Ease::GetValue(TimeValue t)
{
	if (!numKeys)
		return 0.0f;

	// Linearly interpolate from end of graph, two
	// brackets following handle out-of-range getvalues
	if (t <= keys[0].time)
	{
		if (numKeys > 1)
		{
			float tRatio = ((float)(t - keys[0].time)) / keys[0].xVect;
			return /*(int)*/(keys[0].yVect * tRatio) + keys[0].val;
		}
		else
			return keys[0].val;
	}
	if (t >= keys[numKeys - 1].time)
	{
		if (numKeys > 1)
		{
			float tRatio = ((float)(t - keys[numKeys - 1].time)) / keys[numKeys - 2].xVect;
			return /*(int)*/(keys[numKeys - 2].yVect * tRatio) + keys[numKeys - 1].val;
		}
		else
			return keys[0].val;
	}

	// find starting keyIndex based on the average key time
	int KeyIndex = 0;
	if (avgKeyTime)
		KeyIndex = (t - keys[0].time) / avgKeyTime;
	// Ensure it is in range
	if (KeyIndex >= numKeys) KeyIndex = numKeys - 1;
	else if (KeyIndex < 0) KeyIndex = 0;

	// if starting key is too low
	if (keys[KeyIndex].time < t)
	{
		while (KeyIndex < numKeys - 1)
		{
			if (keys[++KeyIndex].time >= t)
			{
				KeyIndex--;
				break;
			}
		}
		// t is between this KeyIndex and the next key
		float tRatio = ((float)(t - keys[KeyIndex].time)) / keys[KeyIndex].xVect;
		return /*(int)*/(keys[KeyIndex].yVect * tRatio) + keys[KeyIndex].val;
	}

	// if starting key is too high
	if (keys[KeyIndex].time > t)
	{
		while (KeyIndex > 0)
		{
			if (keys[--KeyIndex].time <= t)
				break;
		}
		float tRatio = ((float)(t - keys[KeyIndex].time)) / keys[KeyIndex].xVect;
		return /*(int)*/(keys[KeyIndex].yVect * tRatio) + keys[KeyIndex].val;
	}

	// starting key is on t
	return keys[KeyIndex].val;
};

// Second version of above function, finds time given value
TimeValue Ease::GetTime(float value)
{
	if (!numKeys)
		return 0;

	// Linearly interpolate from end of graph, two
	// brackets following handle out-of-range getvalues
	if (value < keys[0].val)
	{
		if (numKeys > 1)
		{
			float tRatio = (value - keys[0].val) / keys[0].yVect;
			return (int)(keys[0].xVect * tRatio) + keys[0].time;
		}
		else
			return keys[0].time;
	}
	if (value > keys[numKeys - 1].val)
	{
		if (numKeys > 1)
		{
			if (keys[numKeys - 2].yVect > 0.0f) {
				float tRatio = (value - keys[numKeys - 1].val) / keys[numKeys - 2].yVect;
				return (int)(keys[numKeys - 2].xVect * tRatio) + keys[numKeys - 1].time;
			}
			else {
				//Here we are tyring to query at what time the graph reaches a particular value
				// when in fact it never reaches this value.
				return 999999;
			}
		}
		else
			return (int)keys[0].val;
	}

	// find starting keyIndex based on the average key time
	int KeyIndex = 0;
	if (avgKeyValue)
		KeyIndex = (int)(value / avgKeyValue);
	// Ensure it is in range
	if (KeyIndex >= numKeys) KeyIndex = numKeys - 1;
	else if (KeyIndex < 0) KeyIndex = 0;

	// if starting key is too low
	if (keys[KeyIndex].val < value)
	{
		while (KeyIndex < numKeys - 1)
		{
			if (keys[++KeyIndex].val > value)
			{
				KeyIndex--;
				break;
			}
		}
		// value is between this KeyIndex and the next key
		float tRatio = (value - keys[KeyIndex].val) / keys[KeyIndex].yVect;
		return (int)(keys[KeyIndex].xVect * tRatio) + keys[KeyIndex].time;
	}

	// if starting key is too high
	if (keys[KeyIndex].val > value)
	{
		while (KeyIndex > 0)
		{
			if (keys[--KeyIndex].val < value)
				break;
		}
		float tRatio = ((float)(value - keys[KeyIndex].val)) / keys[KeyIndex].yVect;
		return (int)(keys[KeyIndex].xVect * tRatio) + keys[KeyIndex].time;
	}

	// starting key is on t
	return keys[KeyIndex].time;
}

int Ease::NumKeys()
{
	return numKeys;
}

int Ease::GetKeyTime(int index)
{
	index = max(min(index, (numKeys - 1)), 0);
	return keys[index].time;
}

int Ease::AppendKey(stepkey* k)
{
	if (numKeys)
	{
		DbgAssert(k->time >= keys[numKeys - 1].time && k->val >= keys[numKeys - 1].val);

		if (k->time == keys[numKeys - 1].time)
		{
			keys[numKeys - 1].val = k->val;
			//
			// Setting the vector of the last key ensures that the
			// graph will extrapolate linearly, normally this is the
			// same tangent as the last key, but if we are the first key
			// then the tangent is simply our own values
			//
			// Woops, the first key is always (t,0). My bad
/*			if(numKeys >= 2)
			{
				keys[numKeys-2].SetVect(k->time, k->val);
				keys[numKeys-1].xVect = keys[numKeys-2].xVect;
				keys[numKeys-1].yVect = keys[numKeys-2].yVect;
			}
			else
			{
				keys[numKeys-1].SetVect(k->time * 2, k->val * 2);
			}
*/
//			keys[numKeys-1].SetVect(k->time * 2, k->val * 2);

			// There are two or more keys, therefore there is a gradient
			if (numKeys > 1)
				keys[numKeys - 2].SetVect(k->time, k->val);
			// else we are replacing the first key, and the gradient is 0
			else
				keys[0].SetVect(STEPTIME100, 0); // Why? because

			return numKeys;
		}

		// this file has been built on the assumption that the keys
		// will be appended in a sorted order, and will never decrease
		// in value
//		DbgAssert(k->time > keys[numKeys - 1].time);

		keys[numKeys - 1].SetVect(k->time, k->val);
		avgKeyTime = (k->time - keys[0].time) / numKeys;
	}
	else
	{
		k->SetVect(STEPTIME100, 0); // And again, cause nothing else appeals
	}
	keys.push_back(*k);

	return numKeys++;	// Increment after return so as to
}						// return the index of the inserted key

void Ease::SetNumKeys(int i)
{
	if (!i)	// i == 0
		keys.clear();
	else
	{
		keys.resize(i);
		for (int s = 0; s < i - numKeys; s++);
	}
	numKeys = i;
}

stepkey* Ease::GetKey(int index)
{
	if (index < 0)
		return NULL;
	if (index < numKeys)
		return &keys[index];
	return NULL;
}

int Ease::GetKeyTimes(Tab<TimeValue> &times, Interval range, DWORD flags)
{
	UNREFERENCED_PARAMETER(flags);
	// find starting keyIndex based on the average key time
	int KeyIndex = 0;
	if (avgKeyTime)
		KeyIndex = range.Start() / avgKeyTime;
	// Ensure it is in range
	KeyIndex = max(min(KeyIndex, (numKeys - 1)), 0);

	// if starting key is too low
	if (keys[KeyIndex].time < range.Start())
	{
		while (KeyIndex < numKeys - 1)
		{
			if (keys[++KeyIndex].time > range.Start())
				break;
		}
	}

	// if starting key is too high
	if (keys[KeyIndex].time > range.Start())
	{
		while (KeyIndex > 0)
		{
			if (keys[--KeyIndex].time < range.Start())
			{
				KeyIndex++;
				break;
			}
		}
	}

	times.Resize(numKeys - KeyIndex);
	times.SetCount(numKeys - KeyIndex);
	int i;
	for (i = 0; i < numKeys - KeyIndex; i++)
	{
		if (keys[KeyIndex + i].time > range.End())
			break;
		times[i] = keys[KeyIndex + i].time;
	}
	if (i + KeyIndex != numKeys)
	{
		times.Resize(i);
		times.SetCount(i);
	}
	return KeyIndex;
}

Interval Ease::GetExtents()
{
	Interval extents = NEVER;
	if (numKeys)
		extents.Set(keys[0].time, keys[numKeys - 1].time);
	return extents;
}

// Set the value on the (index) key to look at the next
// key (Xval, YVal);
/*void Ease::SetVect(int index, int xVal, int yVal)
{
	keys[index].SetVect(xVal, yVal);
}*/
void Ease::SetVect(int index, float xVal, float yVal)
{
	keys[index].SetVect((TimeValue)xVal, yVal);
}

RefResult Ease::NotifyRefChanged(const Interval&, RefTargetHandle,
	PartID&, RefMessage, BOOL)
{
	return REF_SUCCEED;
};

// Load/Save all data

#define ALLKEYS_CHUNK		0x00001
#define NUMKEY_CHUNK		0x00010
#define AVGKEY_CHUNK		0x00100

IOResult Ease::Save(ISave *isave)
{
	ULONG nb;
	isave->BeginChunk(NUMKEY_CHUNK);
	int NK = numKeys;
	isave->Write(&NK, sizeof(int), &nb);
	isave->EndChunk();

	stepkey thisKey;
	isave->BeginChunk(ALLKEYS_CHUNK);
	for (int i = 0; i < numKeys; i++)
	{
		thisKey = keys[i];
		isave->WriteVoid(&thisKey, sizeof(stepkey), &nb); // no TCHAR data
	}
	isave->EndChunk();

	isave->BeginChunk(AVGKEY_CHUNK);
	int AKT = avgKeyTime;
	isave->Write(&AKT, sizeof(int), &nb);
	isave->EndChunk();

	return IO_OK;
}

IOResult Ease::Load(ILoad *iload)
{
	ULONG nb;	// Number of bytes read... we dont really need to worry (we just use IOResult)
	IOResult res = IO_OK;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID())
		{
		case NUMKEY_CHUNK:	// This simply has to read first. Assume it will, cause I can't force it
		{
			int NK = 0;
			res = iload->Read(&NK, sizeof(int), &nb);
			numKeys = NK;
			iload->CloseChunk();
			if (res != IO_OK)  return res;
			break;
		}
		case ALLKEYS_CHUNK:
		{
			stepkey thisKey;
			keys.resize(numKeys);
			for (int i = 0; i < numKeys; i++)
			{
				res = iload->ReadVoid(&thisKey, sizeof(stepkey), &nb);
				if (res != IO_OK) return res;
				keys[i] = thisKey;
			}

			// ST 24/03/04. We dont seem to be maintaining this value.
			// I dont know if it makes much of a difference, but
			// lets keep it valid.
			avgKeyValue = 0.0f;
			if (numKeys > 0) avgKeyValue = keys[numKeys - 1].val - keys[0].val;

			iload->CloseChunk();
			break;
		}
		case AVGKEY_CHUNK:
		{
			int AKT = 0;
			res = iload->Read(&AKT, sizeof(int), &nb);
			avgKeyTime = AKT;
			iload->CloseChunk();
			if (res != IO_OK)  return res;
			break;
		}
		}
	}
	return IO_OK;
}