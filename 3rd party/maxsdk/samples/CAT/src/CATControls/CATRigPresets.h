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

// Loads and saves CAT Rig preset scripts.

#pragma once

#include <vector>
#include <fstream>
#include <string>
#include <ios>
#include <iostream>
 //#include <strstreambuf>

#include "CATToken.h"
#include "ProgressWindow.h"

extern const TCHAR *IdentName(int id);
extern USHORT StringIdent(const TCHAR* s);

// GB 03-Jun: This is a sorta temporary thing.
#define CAT_RIG_PRESET_DIR "\\CAT\\CATRigs"

#define MAX_TOKENCHARS			20

//
// These are the types of clause that come out of CATRigReader
//
typedef enum tagCatRigClauseType CatRigClauseType;
enum tagCatRigClauseType {
	rigNothing,
	rigBeginGroup,
	rigEndGroup,
	rigAssignment,
	rigEnd,
	rigAbort
};

//
// These are the types of identifiers you can load and save.
//
enum {
	//------------------------- PresetParams
	idCATVersion = idStartYourIdentifiersHere,
	//------------------------- CATParentParams
	idParentParams,
	idCATParent,
	idCharacterName,
	idCatUnits,
	idLengthAxis,
	/*	idCatMode,
		idModelessMode,
		idSetupMode,
		idRelaxedMode,
		idMotionMode,
		idMocapMode,
		idFreeformMode,
		idPelvis,
	*/	//------------------------- CATObjectParams
	idObjectParams,
	idBoneName,
	idBoneColour,
	idWidth,
	idLength,
	idHeight,
	idFlags,
	//------------------------- CATControllerParams
	idSetupTM,
	idSetupCtrl,
	idSetupPos,
	//	idSetupRot,
	idOffsetTM,
	//	idOffsetPos,
	//	idOffsetRot,
		//------------------------- CATPelvisParams
	//	idPelvisParams,
	idNumLimbs,
	//------------------------- HubParams
	idHubParams,
	idHub,
	//------------------------- CATLimbParams
	idLimbParams,
	idLimb,
	idLimbFlags,
	idNumBones,
	idNumBoneSegs,
	idBoneSeg,
	idRootPos,
	idLeftRight,
	idLimbBone,
	idAngleRatio,
	idBoneRatio,
	idSetupSwivel,
	idPalm,
	idTargetAlign,
	idCollarbone,
	idPlatform,
	idToe,
	idArbBones,
	idArbBone,
	idExtraControllers,
	idExtraController,
	idCAs,
	idLMR,
	//------------------------- CATSpineParams
	idSpineParams,
	idSpine,
	idSpineBase,
	idSpineLink,
	idTailParams,
	idTail,
	idTailBone,
	//	idNeckParams,
	idSpineType,
	idNumLinks,
	idAbsRel,
	idAbsRelCtrl,
	idTaper,
	idFrequency,
	idCATWeightParams,
	idTailStiffnessParams,
	idPhaseBiasParams,
	//------------------------- EverythingElse...
//	idRibcageParams,
//	idHeadParams,
idDigitParams,
idDigit,
idDigitSegParams,
idDigitSeg,
idPivotPosX,
idPivotPosY,
idPivotPosZ,
idWeightInVal,
idWeightInTan,
idWeightInLen,
idWeightOutVal,
idWeightOutTan,
idWeightOutLen,
idCATMotionWeight,
//------------------------- Clips...
idValLayerInfo,
idLayerType,
idBodyPartID,

idGroupWeights,
idNLAInfo,
idWeights,
idTimeWarp,
idRootBone,

// groups for holding controllers
idSwivelValues,
idBendAngleValues,
idTargetAlignValues,
idIKFKValues,
idLimbIKPos,
idRetargetting,
idIKFKValue,
idIKTargetValues,
idUpVectorValues,
idPivotPosValues,
idPivotRotValues,
idDangleRatio,
idDangleRatioVal,

idController,
idCATParams,
idSubNum,
idClipLength,
idValClassIDs,
idLayerRange,
idTimeRange,
idPose,
idKeys,
idORTIn,
idORTOut,
idRangeStart,
idRangeEnd,
idAxisOrder,
idListItemName,
idListItemActive,

idValMatrix3,
idValPoint,
idValQuat,
idValAngAxis,
idValFloat,
idValAngle,
idValInt,
idValTimeValue,
idValStr,
idValColor,
idValType,

/*	idCtrlPRS,
	//
	idCtrlBezPosition,
	idCtrlLinPosition,
	idCtrlTCBPosition,
	idCtrlXYZPosition,
	//
	idCtrlBezRotation,
	idCtrlLinRotation,
	idCtrlTCBRotation,
	idCtrlXYZRotation,
	//
	idCtrlBezScale,
	idCtrlLinScale,
	idCtrlTCBScale,
	idCtrlXYZScale,
	//
	idCtrlBezPoint,
	idCtrlLinPoint,
	idCtrlTCBPoint,
	idCtrlXYZPoint,
	//
	idCtrlBezFloat,
	idCtrlLinFloat,
	idCtrlTCBFloat,
*/	//
idValKeyBezXYZ,
idValKeyBezRot,
idValKeyBezScale,
idValKeyBezFloat,

idValKeyTCBXYZ,
idValKeyTCBRot,
idValKeyTCBScale,
idValKeyTCBFloat,

idValKeyLinXYZ,
idValKeyLinRot,
idValKeyLinScale,
idValKeyLinFloat,

idSimpleKeys,
// these are Keyframes with timevalues attached
// a simple keyframe with no interpollation settings
idValKeyFloat,
idValKeyPoint,
idValKeyMatrix,

// this is used to help mirror/transform clips
idPathNodeGuess,

//------------------------- CATMotion Layers...
idCATMotionLayer,
idGlobals,
idLimbPhases,
idWalkMode,
idWeldedLeaves,
idFootprints,
//	idMaxStepDist,
//	idMaxStepTime,
	//------------------------- Layers...

	idNumLayers,
	idLayerName,
	idLayerMethod,
	idSelectedLayer,

	//------------------------- Meshs...
	idMeshParams,
	//	idVerticies,
	//	idFaces,
	idNumVerticies,
	idNumFaces,
	idVertex,
	idValFace,

	idObjectOffsetPos,
	idObjectOffsetRot,
	idObjectOffsetScl,
	//------------------------- Nodes...
	idNode,
	idParentNode,
	idSceneRootNode,
	idNodeName,

	//------------------------- Constraints...
	idConstraint,
	idNumTargets,
	idTarget,
	idWeight,

	//------------------------- ParamBlocks...
	idParamBlock,
	idPBIndex,
	idTabIndex,

	//------------------------- Script Controllers...
	idScriptControlParams,
	idScriptText,
	idDescriptionText,
	idScript,
	idName,
	//------------------------- Reaction Controllers...
	idAddress,
};

#define CLIPFLAG_CLIP						(1<<1)
#define CLIPFLAG_DONTREASSIGN				(1<<2)
#define CLIPFLAG_LOADPOS					(1<<3)
#define CLIPFLAG_ONLYWEIGHTED				(1<<4)
#define CLIPFLAG_LOADFEET					(1<<5)
#define CLIPFLAG_LOADPELVIS					(1<<6)
#define CLIPFLAG_SCALE_DATA					(1<<7)
#define CLIPFLAG_MIRROR						(1<<8)
#define CLIPFLAG_MIRROR_DATA_SWAPPED		(1<<26)

// I am seriously doubting the need for this idea now
// that we have ghost transforms for each layer.
#define CLIPFLAG_APPLYTRANSFORMS			(1<<9)
#define CLIPFLAG_LOADWEIGHTTRACKS			(1<<10)
// this flag gets temporarily set and then
// cleared as we traverse down the hierarchy
// It is for
#define CLIPFLAG_TOPLEVEL					(1<<11)
// if the current controller is not what the file specified &&
// we are not replacing controllers
#define CLIPFLAG_USE_SIMPLE_KEYS			(1<<12)

// so p3 controllers can tell how to mirror
// themselves
#define CLIPFLAG_POS						(1<<14)
#define CLIPFLAG_ROT						(1<<15)
// not all that is Matrix is world
#define CLIPFLAG_WORLDSPACE					(1<<13)
#define CLIPFLAG_CREATENEWLAYER				(1<<14)

#define CLIPFLAG_MIRROR_WORLD_X				(1<<16)
#define CLIPFLAG_MIRROR_WORLD_Y				(1<<17)

#define CLIPFLAG_SKIPPOS					(1<<18)
#define CLIPFLAG_SKIPROT					(1<<19)
#define CLIPFLAG_SKIPSCL					(1<<20)

#define CLIPFLAG_SKIP_SPINES				(1<<22)
#define CLIPFLAG_SKIP_KEYFRAMES				(1<<23)
#define CLIPFLAG_TIMEWARP_KEYFRAMES			(1<<24)
#define CLIPFLAG_SKIP_NODE_TABLES			(1<<25)

class ParentChild : public MaxHeapOperators
{
public:
	INode *childnode;
	// no flags yet but we will see
	TSTR parent_address;

	ParentChild(INode *childnode, TSTR parent_address) {
		this->childnode = childnode;
		this->parent_address = parent_address;
	}
};

//////////////////////////////////////////////////////////////////////////
// Keyframe classes
// PT 28 - 01 -04
// these are some ideas for platform independant keyframe classes
class LayerInfo : public MaxHeapOperators
{
public:
	DWORD		dwFlags;			// Flags describing the properties of this layer
	COLORREF	dwLayerColour;		// The colour of the layer
	TSTR		strName;			// The name of the layer
	LayerInfo() : dwFlags(0L), dwLayerColour(0L), strName(NULL) {}
	LayerInfo(DWORD flg, COLORREF clr, TSTR str) {
		dwFlags = flg;
		dwLayerColour = clr;
		strName = str;
	}
};
//////////////////////////////////////////////////////////////////////////
// Keyframe classes
// PT 28 - 01 -04
// these are some ideas for platform independant keyframe classes
class CATClipKey : public MaxHeapOperators
{
public:
	TimeValue time;
	// no flags yet but we will see
	DWORD flags;

	//	virtual IKey MakeMaxKey() = 0;
	//	virtual MayaKey MakeMayaKey() = 0;
};
//////////////////////////////////////////////////////////////////////////
// float keys
class CATClipKeyFloat : public CATClipKey {
public:
	float val;

	CATClipKeyFloat() : val(0.0f) {}
	CATClipKeyFloat(DWORD flg, TimeValue t, float fl) : val(0.0f) {
		flags = flg;
		time = t;
		val = fl;
	}
};
/*
class CATClipKeyFloatBez : public CATClipKeyFloat {
	public:
		float inX;
		float inY;
		float outX;
		float outY;
		IBezFloatKey maxkey;
		IKey* MakeMaxKey(){};
};
class CATClipKeyFloatTCB : public CATClipKeyFloat {
	public:
		float tens;
		float cont;
		float bias;
};
*/
//////////////////////////////////////////////////////////////////////////
// point
class CATClipKeyPoint : public CATClipKey {
public:
	Point3 val;

	CATClipKeyPoint() {}
	CATClipKeyPoint(DWORD flg, TimeValue t, Point3 p3) {
		flags = flg;
		time = t;
		val = p3;
	}
};

/*
class CATClipKeyPointBez : public CATClipKeyPoint {
	public:
		Point3 inX;
		Point3 inY;
		Point3 outX;
		Point3 outY;
};
class CATClipKeyPointTCB : public CATClipKeyPoint {
	public:
		Point3 tens;
		Point3 cont;
		Point3 bias;
};
*/
//////////////////////////////////////////////////////////////////////////
// quat
class CATClipKeyQuat : public CATClipKey {
public:
	Quat val;

	CATClipKeyQuat() {}
	CATClipKeyQuat(DWORD flg, TimeValue t, Quat qt) {
		flags = flg;
		time = t;
		val = qt;
	}
};
/*
class CATClipKeyQuatTCB : public CATClipKeyQuat {
	public:
		float tens;
		float cont;
		float bias;
};
*/

//////////////////////////////////////////////////////////////////////////
// Matrix
class CATClipKeyMatrix : public CATClipKey {
public:
	Matrix3 val;

	CATClipKeyMatrix() {}
	CATClipKeyMatrix(DWORD flg, TimeValue t, Matrix3 tm) {
		flags = flg;
		time = t;
		val = tm;
	}
};

//class ECATParent;
class ICATParentTrans;

/////////////////////////////////////////////////////////////////
// class CATRigReader
//
class CATRigReader : public MaxHeapOperators
{
private:
	ICATParentTrans	*catparenttrans;
	class CATClipValue	*layers;

	tstringstream instream;

	ULONG nLineNumber;
	ULONG nGroupLevel;

	ULONG nFileSize;
	ULONG nCharsRead;
	IProgressWindow *pProgressWindow;
	ULONG nNextProgressUpdateChars;
	ULONG nProgressIncrementChars;

	std::vector<int> groupstack;

	DWORD				dwFileVersion;
	USHORT				curIdentifier;
	CatRigClauseType	curClause;
	CatTokenType		curType;

	CATToken*	thisToken;
	CATToken*	nextToken;
	CATToken	tokens[2];

	float		floatvalues[MAX_TOKENCHARS];		// these are filled by ReadSomeFloats()
	LONG		intvalues[MAX_TOKENCHARS];			// these are filled by ReadSomeInts()
	ULONG		ulongvalues[MAX_TOKENCHARS];

	TCHAR* strOldNumericLocale;

	USHORT cliploader_flags;

	// we are passing a lot of parameters around with the rig reader
	// I would like the rig reader to become more integrated with
//	Matrix3 tmCurrPathNodeGuess;
	Matrix3 tmFilePathNodeGuess;
	Matrix3 tmTransform;
	float dScale;

	Tab <ParentChild*> tabParentChild;

	CATToken* GetNextToken();
	void AssertTypeError(int idIdentifier, int tokExpected, int tokActual);
	void AssertExpectedGot(CatTokenType tokExpected, CatTokenType tokGot);
	void AssertSyntaxError();

	BOOL ReadClause();
	bool ReadSomeFloats(int n);
	bool ReadSomeInts(int n);
	bool ReadSomeULONGS(int n);
	BOOL GetValue(void* val, CatTokenType type);

	void SkipToEOL();
	void CalcFileSize();

public:

	void Init();
	CATRigReader(const TCHAR *filename, ICATParentTrans* catparent);
	CATRigReader(const TSTR& data, ICATParentTrans* catparent);

	~CATRigReader();

	BOOL ok() { return (instream.good() && curClause != rigAbort); }
	ULONG FileSize() const { return nFileSize; }
	ULONG NumCharsRead() const { return nCharsRead; }

	void ShowProgress(TCHAR *szProgressMessage);

	DWORD GetVersion() const { return dwFileVersion; }
	void SetVersion(DWORD ver) { dwFileVersion = ver; }
	const CATMessages* GetErrors() const { return &GetCATMessages(); }

	BOOL NextClause() { return ReadClause(); }
	USHORT CurIdentifier() { return curIdentifier; }
	CatRigClauseType CurClauseID() { return curClause; }
	int CurGroupLevel() { return nGroupLevel; }
	int CurGroupID() { return nGroupLevel > 0 ? groupstack[nGroupLevel - 1] : -1; }

	void SkipGroup();
	bool SkipNextTokenOrValue();

	BOOL GetValue(bool& val) { return GetValue((void*)&val, tokBool); }
	BOOL GetValue(int& val) { return GetValue((void*)&val, tokInt); }
	BOOL GetValue(ULONG& val) { return GetValue((void*)&val, tokULONG); }
	BOOL GetValue(float& val) { return GetValue((void*)&val, tokFloat); }
	BOOL GetValue(TSTR& val) { return GetValue((void*)&val, tokString); }
	BOOL GetValue(Point3& val) { return GetValue((void*)&val, tokPoint); }
	BOOL GetValue(Quat& val) { return GetValue((void*)&val, tokQuat); }
	BOOL GetValue(AngAxis& val) { return GetValue((void*)&val, tokAngAxis); }
	BOOL GetValue(Matrix3& val) { return GetValue((void*)&val, tokMatrix); }
	BOOL GetValue(TokClassIDs& val) { return GetValue((void*)&val, tokClassIDs); }
	BOOL GetValue(LayerInfo& val) { return GetValue((void*)&val, tokLayerInfo); }
	BOOL GetValue(Face& val) { return GetValue((void*)&val, tokFace); }
	BOOL GetValue(INode** val) { return GetValue((void*)&val, tokINode); }
	INode* GetINode();//{ INode* node=NULL; return (GetValue((void*)&node, tokINode) ? node : NULL); };
	BOOL GetValue(Interval& val) { return GetValue((void*)&val, tokInterval); }

	BOOL ToParamBlock(IParamBlock2* pblock, ParamID pid);
	BOOL ToController(Control* ctrl);

	// If a group does not allow subgroups it should call
	// this function when it encounters a subgroup, and
	// then skip the group by calling SkipGroup().
	void AssertNoSubGroups();

	// This is called when the function processing a group
	// encounters a clause that is not handled.  It creates
	// an error message containing the current identifier
	// and group, plus a little bitch-whinge about how they
	// are not allowed.
	void AssertOutOfPlace();

	// If whoever is processing clauses tries to create a
	// new Max scene object as a result of the current
	// clause, they should call this function to report the
	// error, then cancel the rest of the import.
	void AssertObjectCreationError();

	BOOL ReadController(Control *ctrl, Interval timerange, const float dScale, int flags);
	BOOL ReadParamBlock(IParamBlock2* pblock, Interval timerange, const float dScale, int flags);

	BOOL ReadSubAnim(Control *ctrl, int subanim, Interval timerange, const float dScale, int flags);

	BOOL ReadPoseGroup(Control *ctrl, TimeValue t, const float dScale, int flags);

	void TransformMatrix(Matrix3 &tm, const float dScale, int flags);
	void TransformPoint(Point3 &p, const float dScale, int flags);

	BOOL ReadPoseIntoController(Control *ctrl, TimeValue t, const float dScale, int flags);

	BOOL GetValuePose(int flags, SClass_ID superClassID, void* val);
	BOOL ReadSimpleKeys(Control *ctrl, Interval timerange, const float dScale, int flags);
	BOOL ReadKeys(Control *ctrl, Interval timerange, const float dScale, int flags);

	BOOL ReadConstraint(Control* constr, Interval timerange, const float dScale, int flags);
	BOOL ReadScriptController(Control* mxsctrl, Interval timerange, const float dScale, int flags);
	BOOL ReadString(TSTR &string);

	// put in the new transform matricies
	void SetClipTransforms(/*Matrix3 tmCurr,*/ Matrix3 tmFile, Matrix3 tm, float sc) {/* tmCurrPathNodeGuess = tmCurr;*/ tmFilePathNodeGuess = tmFile; tmTransform = tm; dScale = sc; }
	//	Matrix3 GettmCurrPathNodeGuess(){ return tmCurrPathNodeGuess; }
	Matrix3 GettmFilePathNodeGuess() { return tmFilePathNodeGuess; }
	Matrix3 GettmTransform() { return tmTransform; }
	float	GetScale() { return dScale; }

	void AddParent(INode *childnode, TSTR parent_address) {
		ParentChild* newPC = new ParentChild(childnode, parent_address);
		tabParentChild.Append(1, &newPC);
	};
	void ResolveParentChilds();

	BOOL ReadMesh(Mesh &mesh);

	CATClipValue *GetLayerController() { return layers; }
	void SetLayerController(CATClipValue *ctrl) { layers = ctrl; }
};

/////////////////////////////////////////////////////////////////
// class CATRigWriter
//
// Z
//
class CATRigWriter : public MaxHeapOperators
{
private:
	ICATParentTrans	*catparenttrans;

	//	std::ofstream outstream;
	tstringstream outstream;
	TSTR filename;

	int nIndentLevel;
	tstring strIndent;

	TCHAR* strOldNumericLocale;

	void Indent() { nIndentLevel++; strIndent.assign(nIndentLevel * 2, _T(' ')); }
	void Outdent() {
		if (--nIndentLevel < 0)
			nIndentLevel = 0;
		strIndent.assign(nIndentLevel * 2, _T(' '));
	}

public:
	void Init();
	CATRigWriter();
	CATRigWriter(const TCHAR *filename, ICATParentTrans* catparent);

	~CATRigWriter();

	BOOL Open(const TCHAR *filename);
	void Close();
	BOOL GetStreamBuffer(TSTR &str);

	BOOL ok() const { return outstream.good(); }

	BOOL BeginGroup(USHORT id, void *val = NULL);
	BOOL EndGroup();

	BOOL Comment(const TCHAR *msg, ...);
	BOOL WritePoint3(Point3 p3);
private:
	BOOL Write(USHORT id, void *data);
public:
	BOOL Write(USHORT id, const bool& val) { return Write(id, (void*)&val); }
	BOOL Write(USHORT id, const int& val) { return Write(id, (void*)&val); }
	BOOL Write(USHORT id, const ULONG& val) { return Write(id, (void*)&val); }
	BOOL Write(USHORT id, const float& val) { return Write(id, (void*)&val); }
	BOOL Write(USHORT id, const Point3& val) { return Write(id, (void*)&val); }
	BOOL Write(USHORT id, const Quat& val) { return Write(id, (void*)&val); }
	BOOL Write(USHORT id, const AngAxis& val) { return Write(id, (void*)&val); }
	BOOL Write(USHORT id, const Matrix3& val) { return Write(id, (void*)&val); }
	BOOL Write(USHORT id, const TokClassIDs& val) { return Write(id, (void*)&val); }
	BOOL Write(USHORT id, const LayerInfo& val) { return Write(id, (void*)&val); }
	BOOL Write(USHORT id, const Face& val) { return Write(id, (void*)&val); }
	BOOL Write(USHORT id, const Interval& val) { return Write(id, (void*)&val); }
	BOOL Write(USHORT id, const INode* val) { return Write(id, (void*)val); }
	BOOL Write(USHORT id, const TSTR& val) { return Write(id, (void*)val.data()); }

	BOOL WriteVal(CatTokenType toVal, void *data);
	BOOL FromParamBlock(IParamBlock2* pblock, USHORT id, ParamID paramID);
	BOOL FromController(Control* ctrl, USHORT id);

	// starts out the process
	BOOL WriteController(Control* ctrl, int flags, Interval timerange);
	BOOL WriteParamBlock(IParamBlock2* pblock, int flags, Interval timerange);
	BOOL WriteScriptController(Control* ctrl, int flags, Interval timerange);
	BOOL WriteStringSequence(TSTR str);

	BOOL WriteReactionController(Control* ctrl, int flags, Interval timerange);

	// recursive function that rips through a hierarchy
	// writing out classids till it gets to the bottom
	BOOL WriteControllerHierarchy(Control* ctrl, int flags, Interval timerange);
	// This function
	BOOL WriteRefs(Interval timerange, int flags, Control* ctrl);
	BOOL WriteCAs(ReferenceTarget* ref);

	// Save out the value for the controller
	BOOL WritePose(TimeValue t, Control* ctrl);

	// writes out ORT and range values, and if needed,
	// decides what kind of keys this controller uses
	BOOL WriteLeafController(Interval timerange, int flags, Control* ctrl);

	// saves out the keyf for the controller given the correct keytype
	BOOL WriteKeys(Interval timerange, int flags, Control* ctrl, USHORT idKey, IKey *key);

	// writes out simple keys with no
	BOOL WriteSimpleKeys(Interval timerange, Control* ctrl);

	BOOL WriteNode(INode* node);
	//	BOOL WriteOrientConst(IOrientConstRotation* constr);
	BOOL WriteConstraint(Control* ctrl, int flags, Interval timerange);
	//	BOOL WritePosConst(IPosConstPosition* constr);
	BOOL WriteLinkConst(ILinkCtrl* constr, int flags, Interval timerange);

	BOOL SaveMesh(Mesh &mesh);
};
