#pragma once

#include "geombind.h"
#include "matrix3.h"
#include <bitarray.h>

class BonesDefMod;
class BoneModData;
class ObjectState;

#define WM_SOLVE_DONE WM_USER+1

//when executing some commands we want to split the command into 2 parts, the collection of the parameters and then
//the actual command execution which happens on the next evaluation of the.   This interface lets you do this, just assignt 
//bonesdefmodifiers local mod command to a pointer to this and it will be executed the next eval.  The reason for this setup is
//GetObjectTM is problematic inside of an eval.
class IBMD_Command
{
public:
	IBMD_Command() {};
	virtual ~IBMD_Command() {};
	virtual void Execute(BonesDefMod * mod, BoneModData *bmd) = 0;
	// Called by the system when command is invoked.
	// Advanced version, calls into simpler version by default.  Implement if INode and ObjectState are needed.
	virtual void Execute(BonesDefMod * mod, TimeValue t, BoneModData *bmd, ObjectState *os, INode *node)
	{ Execute(mod,bmd); }
};


//this is a command to builds the skin weights based on a voxel tree or heat map.
class BMD_Command_VoxelWeight : public IBMD_Command
{
public:
	BMD_Command_VoxelWeight();
	virtual ~BMD_Command_VoxelWeight();
	virtual void Execute(BonesDefMod * mod, BoneModData *bmd);

	//this is the skinned node matrix, we compute this on the ui side since doing an getobjectTM in the eval can be bad and lead to endless loops
	Matrix3 mObjTm;
	float mFalloff;
	//max number of bones to influence a vertex
	int mMaxInfluence;
	//this is the size of the voxel tree, the larger the value the more accurate but more memory and time to compute
	int mVoxelSize;
	//this produces better results but cost more computation time
	bool mUseWinding;
	// the type of weight computation 0= voxel,1 = heat map
	int mType;
	//this determines whether end bones in skin will have anub automatically added.   This is used in cases like the end of fingers
	//if you do not manually  add a nub to the end of the bone it wont compute as much since the the voxel engine uses joint to joint
	//to do the computation, if autonub is on these nub bones will automatically be added
	bool mAutoNubs;

	//these are the bone node matrices, we compute this on the ui side since doing an getobjectTM in the eval can be bad and lead to endless loops
	Tab<Matrix3> mBoneObjTm;

protected:
	//heat naps have special requirements for the mesh which must be a manifold mesh
	//also meshes that pass a subselection need to be processed since they require a remmapping
	//local mod data containing the mesh to process
	//objtm transform the poitns into world space
	//flipMesh whether to flip the mesh faces based on parity
	//mwah the mesh returned to be used for the heat map
	//returns a mapping of indices to the original mesh
	Tab<int> ProcessHeatMapMesh(BoneModData *bmd, MaxSDK::GeomBind::IMesh *mesh,Matrix3 objTM,  bool flipMesh);
	void ToOpenGLMatrix(Matrix3& mat, double* m);

	bool Solve(BonesDefMod * mod, BoneModData *bmd, const BitArray& vsel);
};


