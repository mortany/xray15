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

/**********************************************************************
	DESCRIPTION:

		Loads and saves CAT Rig preset scripts.  This file implements
		a rather useful class called Dictionary.  It's a fast-lookup
		string tree.  Can't remember what the data structure is called
		but it's one of those ones that store a pointer for each
		character in your word so you can find it in N comparisons
		where N is the number of characters in your word.  I know
		there's a name for it and it's frustrating me that I can't
		remember what it is.
 **********************************************************************/
#include "cat.h"
#include "CatPlugins.h"
#include "resource.h"
#include "decomp.h"

 // Need to test for presence of IArbBoneTrans in ResolveParentChilds
#include "ArbBoneTrans.h"
#include "IKTargController.h"
#include "maxtextfile.h"

/////////////////////////////////////
// Max SDK includes

// 2 different interfaces to script controllers
#include <maxscript/maxwrapper/scriptcontroller.h>
#include "icustattribcontainer.h"

#include <istdplug.h>
#include <sstream>

#include <maxscript/maxwrapper/scriptcontroller.h>

#include <maxscript/maxscript.h>
#include <maxscript/maxwrapper/mxsobjects.h>

#pragma warning(push)
#pragma warning(disable:4100) // unreferenced formal parameters in the samples/controllers .h files
#pragma warning(push)
#pragma warning(disable:4238) // necessary for _BV types
#include "../samples/controllers/reactor/reactapi.h"
#include "../samples/controllers/attach.h"
#pragma warning(pop)
#pragma warning(pop)
////////////////////////////////////////////////////////
// CAT
#include "CATRigPresets.h"
#include <CATAPI/CATClassID.h>

#include "CATHierarchyLeaf.h"
#include "Locale.h"

#include "ProgressWindow.h"
#include "CATClipRoot.h"
#include "CATHierarchyBranch2.h"
#include "ICATParent.h"

#include "CATNodeControl.h"

/*
 * Dictionary for all our preset file identifiers, including internal
 * identifiers used for tokenising.
 */

static DictionaryEntry dictEntries[] = {
	//  <NAME>					<IDENTIFIER ID>			<IDENTIFIER TYPE>		<FOLLOWING TYPE>
	//---------------------------------------------------------------------------------------------- Global stuff
		_T("Version"),				idCATVersion,			tokIdentifier,			tokInt,
		//---------------------------------------------------------------------------------------------- CATParentParams
			_T("ParentParams"),			idParentParams,			tokGroup,				tokOpenCurly,
			_T("CATParent"),			idCATParent,			tokGroup,				tokOpenCurly,
			_T("CharacterName"),		idCharacterName,		tokIdentifier,			tokString,
			_T("Units"),				idCatUnits,				tokIdentifier,			tokFloat,
			_T("LengthAxis"),			idLengthAxis,			tokIdentifier,			tokInt,
			//---------------------------------------------------------------------------------------------- CATObjectParams
				_T("ObjectParams"),			idObjectParams,			tokGroup,				tokOpenCurly,
				_T("BoneName"),				idBoneName,				tokIdentifier,			tokString,
				_T("BoneColour"),			idBoneColour,			tokIdentifier,			tokInt,
				_T("Width"),				idWidth,				tokIdentifier,			tokFloat,
				_T("Length"),				idLength,				tokIdentifier,			tokFloat,
				_T("Height"),				idHeight,				tokIdentifier,			tokFloat,
				_T("Flags"),				idFlags,				tokIdentifier,			tokULONG,
				//---------------------------------------------------------------------------------------------- CATControllerParams
					_T("SetupTM"),				idSetupTM,				tokIdentifier,			tokMatrix,
					_T("SetupController"),		idSetupCtrl,			tokGroup,				tokOpenCurly,
					_T("SetupPos"),				idSetupPos,				tokIdentifier,			tokPoint,
					_T("OffsetTM"),				idOffsetTM,				tokIdentifier,			tokMatrix,
					//---------------------------------------------------------------------------------------------- CATPelvisParams
						_T("NumLimbs"),				idNumLimbs,				tokIdentifier,			tokInt,
						//---------------------------------------------------------------------------------------------- HubParams
							_T("HubParams"),			idHubParams,			tokGroup,				tokOpenCurly,
							_T("Hub"),					idHub,					tokGroup,				tokOpenCurly,
							//---------------------------------------------------------------------------------------------- CATLimbParams
								_T("LimbParams"),			idLimbParams,			tokGroup,				tokOpenCurly,
								_T("Limb"),					idLimb,					tokGroup,				tokOpenCurly,
								_T("LimbFlags"),			idLimbFlags,			tokIdentifier,			tokULONG,
								_T("NumBones"),				idNumBones,				tokIdentifier,			tokInt,
								_T("NumBoneSegs"),			idNumBoneSegs,			tokIdentifier,			tokInt,
								_T("BoneSeg"),				idBoneSeg,				tokGroup,				tokOpenCurly,
								_T("RootPos"),				idRootPos,				tokIdentifier,			tokPoint,
								_T("LeftRight"),			idLeftRight,			tokIdentifier,			tokInt,
								_T("LimbBone"),				idLimbBone,				tokGroup,				tokOpenCurly, //---- CATLimbSegParams
								_T("AngleRatio"),			idAngleRatio,			tokIdentifier,			tokFloat,
								_T("BoneRatio"),			idBoneRatio,			tokIdentifier,			tokFloat,
								_T("SetupSwivel"),			idSetupSwivel,			tokIdentifier,			tokFloat,
								_T("Palm"),					idPalm,					tokGroup,				tokOpenCurly,
								_T("TargetAlign"),			idTargetAlign,			tokIdentifier,			tokFloat,
								_T("CollarBone"),			idCollarbone,			tokGroup,				tokOpenCurly,
								_T("Platform"),				idPlatform,				tokGroup,				tokOpenCurly,
								_T("Toe"),					idToe,					tokGroup,				tokOpenCurly,
								_T("ArbBones"),				idArbBones,				tokGroup,				tokOpenCurly,
								_T("ArbBone"),				idArbBone,				tokGroup,				tokOpenCurly,
								_T("ExtraControllers"),		idExtraControllers,		tokGroup,				tokOpenCurly,
								_T("ExtraController"),		idExtraController,		tokGroup,				tokOpenCurly,
								_T("CAs"),					idCAs,					tokGroup,				tokOpenCurly,
								_T("LMR"),					idLMR,					tokIdentifier,			tokInt,
								//---------------------------------------------------------------------------------------------- CATSpineParams
									_T("SpineParams"),			idSpineParams,			tokGroup,				tokOpenCurly,
									_T("Spine"),				idSpine,				tokGroup,				tokOpenCurly,
									_T("SpineBase"),			idSpineBase,			tokIdentifier,			tokMatrix,
									_T("SpineLink"),			idSpineLink,			tokGroup,				tokOpenCurly,
									_T("TailParams"),			idTailParams,			tokGroup,				tokOpenCurly,
									_T("Tail"),					idTail,					tokGroup,				tokOpenCurly,
									_T("TailBone"),				idTailBone,				tokGroup,				tokOpenCurly,
									_T("SpineType"),			idSpineType,			tokIdentifier,			tokInt,
									_T("NumLinks"),				idNumLinks,				tokIdentifier,			tokInt,
									_T("AbsRelSpine"),			idAbsRel,				tokIdentifier,			tokFloat,
									_T("AbsRelCtrl"),			idAbsRelCtrl,			tokGroup,				tokOpenCurly,
									_T("Taper"),				idTaper,				tokIdentifier,			tokFloat,
									_T("Frequency"),			idFrequency,			tokIdentifier,			tokFloat,
									_T("CATWeightParams"),		idCATWeightParams,		tokGroup,				tokOpenCurly,
									_T("TailStiffnessParams"),	idTailStiffnessParams,	tokGroup,				tokOpenCurly,
									_T("PhaseBiasParams"),		idPhaseBiasParams,		tokGroup,				tokOpenCurly,
									//---------------------------------------------------------------------------------------------- CATEverythingElse
										_T("DigitParams"),			idDigitParams,			tokGroup,				tokOpenCurly,
										_T("Digit"),				idDigit,				tokGroup,				tokOpenCurly,
										_T("DigitSegParams"),		idDigitSegParams,		tokGroup,				tokOpenCurly,
										_T("DigitSeg"),				idDigitSeg,				tokGroup,				tokOpenCurly,
										_T("PivotPosX"),			idPivotPosX,			tokIdentifier,			tokFloat,
										_T("PivotPosY"),			idPivotPosY,			tokIdentifier,			tokFloat,
										_T("PivotPosZ"),			idPivotPosZ,			tokIdentifier,			tokFloat,
										_T("WeightInVal"),			idWeightInVal,			tokIdentifier,			tokFloat,
										_T("WeightInTan"),			idWeightInTan,			tokIdentifier,			tokFloat,
										_T("WeightInLen"),			idWeightInLen,			tokIdentifier,			tokFloat,
										_T("WeightOutVal"),			idWeightOutVal,			tokIdentifier,			tokFloat,
										_T("WeightOutTan"),			idWeightOutTan,			tokIdentifier,			tokFloat,
										_T("WeightOutLen"),			idWeightOutLen,			tokIdentifier,			tokFloat,
										_T("CATMotionWeight"),		idCATMotionWeight,		tokIdentifier,			tokFloat,
										//---------------------------------------------------------------------------------------------- CATEverythingElse
											_T("ValLayerInfo"),			idValLayerInfo,			tokIdentifier,			tokLayerInfo,
											_T("LayerType"),			idLayerType,			tokIdentifier,			tokString,
											_T("BodyPartID"),			idBodyPartID,			tokIdentifier,			tokString,
											_T("GroupWeights"),			idGroupWeights,			tokGroup,				tokOpenCurly,

											_T("NLAInfo"),				idNLAInfo,				tokGroup,				tokOpenCurly,
											_T("Weights"),				idWeights,				tokGroup,				tokOpenCurly,
											_T("TimeWarp"),				idTimeWarp,				tokGroup,				tokOpenCurly,
											_T("RootBone"),				idRootBone,				tokGroup,				tokOpenCurly,

											// these groups are designed to replace the use of subnum in places like limbdata and palm
											_T("SwivelValues"),			idSwivelValues,			tokGroup,				tokOpenCurly,
											_T("BendAngleValues"),		idBendAngleValues,		tokGroup,				tokOpenCurly,
											_T("TargetAlignValues"),	idTargetAlignValues,	tokGroup,				tokOpenCurly,
											_T("IKFKValues"),			idIKFKValues,			tokGroup,				tokOpenCurly,
											_T("LimbIKPos"),			idLimbIKPos,			tokGroup,				tokOpenCurly,
											_T("Retargetting"),			idRetargetting,			tokGroup,				tokOpenCurly,
											_T("IKFKValue"),			idIKFKValue,			tokIdentifier,			tokFloat,
											_T("IKTargetValues"),		idIKTargetValues,		tokGroup,				tokOpenCurly,
											_T("UpVectorValues"),		idUpVectorValues,		tokGroup,				tokOpenCurly,
											_T("PivotPosValues"),		idPivotPosValues,		tokGroup,				tokOpenCurly,
											_T("PivotRotValues"),		idPivotRotValues,		tokGroup,				tokOpenCurly,
											_T("DangleRatio"),			idDangleRatio,			tokGroup,				tokOpenCurly,
											_T("DangleRatioVal"),		idDangleRatioVal,		tokIdentifier,			tokFloat,

											_T("Controller"),			idController,			tokGroup,				tokOpenCurly,
											_T("CATParams"),			idCATParams,			tokGroup,				tokOpenCurly,
											_T("ClipLength"),			idClipLength,			tokIdentifier,			tokInt,
											_T("SubNumber"),			idSubNum,				tokIdentifier,			tokInt,
											_T("ClassIDs"),				idValClassIDs,			tokIdentifier,			tokClassIDs,
											_T("LayerRange"),			idLayerRange,			tokIdentifier,			tokInterval,
											_T("TimeRange"),			idTimeRange,			tokIdentifier,			tokInterval,
											_T("Pose"),					idPose,					tokGroup,				tokOpenCurly,
											_T("Keys"),					idKeys,					tokGroup,				tokOpenCurly,
											_T("ORTIn"),				idORTIn,				tokIdentifier,			tokInt,
											_T("ORTOut"),				idORTOut,				tokIdentifier,			tokInt,
											_T("RangeStart"),			idRangeStart,			tokIdentifier,			tokInt,
											_T("RangeEnd"),				idRangeEnd,				tokIdentifier,			tokInt,
											_T("AxisOrder"),			idAxisOrder,			tokIdentifier,			tokInt,
											_T("ListItemName"),			idListItemName,			tokIdentifier,			tokString,
											_T("ListItemActive"),		idListItemActive,		tokIdentifier,			tokInt,

											_T("ValMatrix"),			idValMatrix3,			tokIdentifier,			tokMatrix,
											_T("ValPoint"),				idValPoint,				tokIdentifier,			tokPoint,
											_T("ValQuat"),				idValQuat,				tokIdentifier,			tokQuat,
											_T("ValAngAxis"),			idValAngAxis,			tokIdentifier,			tokAngAxis,
											_T("ValFloat"),				idValFloat,				tokIdentifier,			tokFloat,
											_T("ValAngle"),				idValAngle,				tokIdentifier,			tokFloat,
											_T("ValInt"),				idValInt,				tokIdentifier,			tokInt,
											_T("ValTimeValue"),			idValTimeValue,			tokIdentifier,			tokInt,
											_T("ValString"),			idValStr,				tokIdentifier,			tokString,
											_T("ValColor"),				idValColor,				tokIdentifier,			tokPoint,
											_T("ValType"),				idValType,				tokIdentifier,			tokInt,

											_T("ValKeyBezXYZ"),			idValKeyBezXYZ,			tokIdentifier,			tokKeyBezXYZ,
											_T("ValKeyBezRot"),			idValKeyBezRot,			tokIdentifier,			tokKeyBezRot,
											_T("ValKeyBezScale"),		idValKeyBezScale,		tokIdentifier,			tokKeyBezScale,
											_T("ValKeyBezFloat"),		idValKeyBezFloat,		tokIdentifier,			tokKeyBezFloat,

											_T("ValKeyTCBXYZ"),			idValKeyTCBXYZ,			tokIdentifier,			tokKeyTCBXYZ,
											_T("ValKeyTCBRot"),			idValKeyTCBRot,			tokIdentifier,			tokKeyTCBRot,
											_T("ValKeyTCBScale"),		idValKeyTCBScale,		tokIdentifier,			tokKeyTCBScale,
											_T("ValKeyTCBFloat"),		idValKeyTCBFloat,		tokIdentifier,			tokKeyTCBFloat,

											_T("ValKeyLinXYZ"),			idValKeyLinXYZ,			tokIdentifier,			tokKeyLinXYZ,
											_T("ValKeyLinRot"),			idValKeyLinRot,			tokIdentifier,			tokKeyLinRot,
											_T("ValKeyLinScale"),		idValKeyLinScale,		tokIdentifier,			tokKeyLinScale,
											_T("ValKeyLinFloat"),		idValKeyLinFloat,		tokIdentifier,			tokKeyLinFloat,

											// these are Pose values with timevalues attaches
											// like a simple keyframe with no interpolation settings
											 _T("SimpleKeys"),			idSimpleKeys,			tokGroup,				tokOpenCurly,
											 _T("ValKeyFloat"),			idValKeyFloat,			tokIdentifier,			tokKeyFloat,
											 _T("ValKeyPoint"),			idValKeyPoint,			tokIdentifier,			tokKeyPoint,
											 _T("ValKeyMatrix"),			idValKeyMatrix,			tokIdentifier,			tokKeyMatrix,

											 _T("PathNodeGuess"),		idPathNodeGuess,		tokIdentifier,			tokMatrix,

											 // CATMotion parameters
											 _T("CATMotionLayer"),		idCATMotionLayer,		tokGroup,				tokOpenCurly,
											 _T("Globals"),				idGlobals,				tokGroup,				tokOpenCurly,
											 _T("LimbPhases"),			idLimbPhases,			tokGroup,				tokOpenCurly,
											 _T("WeldedLeaves"),			idWeldedLeaves,			tokIdentifier,			tokBool,
											 _T("WalkMode"),				idWalkMode,				tokIdentifier,			tokInt,
											 _T("Footprints"),			idFootprints,			tokGroup,				tokOpenCurly,
											 _T("NumLayers"),			idNumLayers,			tokIdentifier,			tokInt,
											 _T("LayersName"),			idLayerName,			tokIdentifier,			tokString,
											 _T("LayerMethod"),			idLayerMethod,			tokIdentifier,			tokString,
											 _T("SelectedLayer"),		idSelectedLayer,		tokIdentifier,			tokInt,

											 // These tags are for saving out custom meshes
											 _T("MeshParams"),			idMeshParams,			tokGroup,				tokOpenCurly,
											 _T("NumVerticies"),			idNumVerticies,			tokIdentifier,			tokInt,
											 _T("NumFaces"),				idNumFaces,				tokIdentifier,			tokInt,
											 _T("Vert"),					idVertex,				tokIdentifier,			tokPoint,
											 _T("ValFace"),				idValFace,				tokIdentifier,			tokFace,

											 _T("ObjectOffsetPos"),		idObjectOffsetPos,		tokIdentifier,			tokPoint,
											 _T("ObjectOffsetRot"),		idObjectOffsetRot,		tokIdentifier,			tokQuat,
											 _T("ObjectOffsetScl"),		idObjectOffsetScl,		tokIdentifier,			tokPoint,

											 ///////////////////////////////////////////////////////
											 _T("Node"),					idNode,					tokIdentifier,			tokINode,
											 _T("ParentNode"),			idParentNode,			tokIdentifier,			tokINode,
											 _T("SceneRootNode"),		idSceneRootNode,		tokIdentifier,			tokINode,
											 _T("NodeName"),				idNodeName,				tokIdentifier,			tokString,

											 ///////////////////////////////////////////////////////
											 // Constraints
											 _T("Constraint"),			idConstraint,			tokGroup,				tokOpenCurly,
											 _T("NumTargets"),			idNumTargets,			tokIdentifier,			tokInt,
											 _T("Target"),				idTarget,				tokIdentifier,			tokINode,
											 _T("Weight"),				idWeight,				tokIdentifier,			tokFloat,

											 ///////////////////////////////////////////////////////
											 // ParameterBlocks
											 _T("ParamBlock"),			idParamBlock,			tokGroup,				tokOpenCurly,
											 _T("PBIndex"),				idPBIndex,				tokIdentifier,			tokInt,
											 _T("TabIndex"),				idTabIndex,				tokIdentifier,			tokInt,

											 ///////////////////////////////////////////////////////
											 // Script Controller
											 _T("ScriptControlParams"),	idScriptControlParams,	tokGroup,				tokOpenCurly,
											 _T("ScriptText"),			idScriptText,			tokGroup,				tokOpenCurly,
											 _T("DescriptionText"),		idDescriptionText,		tokGroup,				tokOpenCurly,
											 _T("Script"),				idScript,				tokIdentifier,			tokString,
											 _T("Name"),					idName,					tokIdentifier,			tokString,

											 ///////////////////////////////////////////////////////
											 // Reaction Controller
											 _T("Address"),				idAddress,				tokIdentifier,			tokString,

											 //--<END OF TABLE>------------------------------------------------------------------------------ END OF TABLE
												 NULL,					idNothing,				tokNothing,				tokNothing
};

// This builds a dictionary for our tokeniser / parser.
static Dictionary catDictionary(dictEntries);

//
// Given and ID retrieve its name
//
const TCHAR *IdentName(int id) {
	DictionaryEntry* entry = catDictionary.GetEntry(id);
	DbgAssert(entry != NULL);
	return entry->name;
}

//
// Given a string retrieve its ID
//
USHORT StringIdent(const TCHAR* s)
{
	DictionaryEntry* entry = catDictionary.Lookup(s);
	return entry->id;
}

/////////////////////////////////////////////////////////////////
// CATRigReader functions
/////////////////////////////////////////////////////////////////
void CATRigReader::Init()
{
	// GB 24-Oct-03: Save current numeric locale and set to standard C.
	// This cures our number translation problems in central europe.
	strOldNumericLocale = ReplaceLocale(LC_NUMERIC, _T("C"));

	curIdentifier = idNothing;
	curClause = rigNothing;
	curType = tokNothing;

	thisToken = &tokens[0];
	nextToken = &tokens[1];

	thisToken->SetDictionary(&catDictionary);
	nextToken->SetDictionary(&catDictionary);

	pProgressWindow = NULL;
	nNextProgressUpdateChars = 0;
	nProgressIncrementChars = 0;

	dwFileVersion = 0;
	nFileSize = 0;
	nCharsRead = 0;
	nLineNumber = 1;
	nGroupLevel = 0;
	groupstack.resize(16);

	// these values are used by the clip system
//	tmCurrPathNodeGuess = Matrix3(1);
	tmFilePathNodeGuess = Matrix3(1);
	tmTransform = Matrix3(1);
	dScale = 1.0f;

	layers = NULL;
}

CATRigReader::CATRigReader(const TCHAR *filename, ICATParentTrans* catparenttrans)
	: groupstack(), cliploader_flags(0)
{
	this->catparenttrans = catparenttrans;
	Init();

	MaxSDK::Util::TextFile::Reader infilestream;

	// Open file.  If it fails, set the current clause
	// to rigAbort and DbgAssert an error.  Otherwise,
	// Read the first token in the file, so we're all
	// ready for a call to ReadClause().
	 // Original file open mode is Text
	if (!infilestream.Open(filename))
	{
		curClause = rigAbort;
		GetCATMessages().Error(0, GetString(IDS_ERR_NOFILEWRITE), filename);
	}
	else {
		while (!infilestream.IsEndOfFile()) {
			TSTR str = infilestream.ReadLine();
			for (int i = 0; i <= str.Length(); i++) //including the '\0'?
				instream.put(str[i]);
		}

		infilestream.Close();

		CalcFileSize();
		GetNextToken();
	}
}

CATRigReader::CATRigReader(const TSTR& data, ICATParentTrans* catparenttrans)
	: groupstack(), cliploader_flags(0)
{
	this->catparenttrans = catparenttrans;
	Init();

	for (int i = 0; i < data.Length(); i++) {
		instream.put(data[i]);
	}
	instream.seekg(0, std::ios_base::beg);

	if (!instream.good()) {
		curClause = rigAbort;
		GetCATMessages().Error(0, GetString(IDS_ERR_NOREADERINIT));
	}
	else {
		CalcFileSize();
		GetNextToken();
	}
}

CATRigReader::~CATRigReader() {
	//	if (instream.is_open()) instream.close();
	RestoreLocale(LC_NUMERIC, strOldNumericLocale);
	if (pProgressWindow) pProgressWindow->ProgressEnd();
}

// Calculates the size of the file
void CATRigReader::CalcFileSize()
{
	std::stringstream::pos_type nInitialPos = instream.tellg();
	instream.seekg(0, std::ios_base::end);
	nFileSize = (ULONG)instream.tellg();
	instream.seekg(nInitialPos, std::ios_base::beg);

	// Init progress bar stuff.
	nProgressIncrementChars = nFileSize / 64;
	if (nProgressIncrementChars <= 0)
		nProgressIncrementChars = 1;	// would be a very strange case.

	nNextProgressUpdateChars = min(nProgressIncrementChars, nFileSize);
}

//
// Turns progress bar on.
//
void CATRigReader::ShowProgress(TCHAR *szProgressMessage)
{
	UNREFERENCED_PARAMETER(szProgressMessage);
	if (!pProgressWindow) {
		// Initialise the progress bar.
		pProgressWindow = GetProgressWindow();
		if (!pProgressWindow->ProgressBegin(GetString(IDS_PROGRESS), GetString(IDS_LOADING)))
			pProgressWindow = NULL;
	}
}

// This function is a bit confusing.  It reads the next
// token in the stream into nextToken.  That means, the
// result of the previous call to GetNextToken() will be
// in thisToken.  However, the function returns thisToken
// because it's usually the one we're interested in.
CATToken* CATRigReader::GetNextToken() {
	if (thisToken->type == tokEOL) nLineNumber++;
	std::swap(thisToken, nextToken);

	do {
		nextToken->ReadToken(instream);
		nCharsRead += nextToken->NumCharsRead();
	} while (nextToken->type == tokComment);

	return thisToken;
}

/////////////////////////////////////////////////////////////////
// This is the guts of our parser.  It reads useful clauses from
// the rig file and provides a GetValue() function to grab them.
// The possible clauses are listed below:
//
//   rigBeginGroup     looks like "GroupName {"
//   rigEndGroup       looks like "}"
//   rigAssignment     looks like "IdentName = Value"
//   rigEnd            means we're done
//   rigAbort          means something messed up big-time
//   rigArrayValue     looks like "Value"
//   rigArrayValue     looks like "Value"
//
// To read the next clause, call NextClause().  There is no
// return value, but the user can access any error messages
// by calling GetErrors().
//
// We find out what type of clause we're looking at by calling
// CurClauseID().  The two forms rigBeginGroup and rigAssignment
// have an associated identifier number, which can be found by
// calling CurIdentifier().
//
// Some type-checking is done, plus some very basic error
// recovery (skipping to the next line).  All-up, it's best not
// to try writing a file that will confuse this parser...  Let's
// just all try to be nice.
//
BOOL CATRigReader::ReadClause() {
	if (curClause == rigAbort || curClause == rigEnd) return FALSE;

	// Automatic group level increment from the last call
	// to this function, which was a BeginGroup clause.
	if (curClause == rigBeginGroup) nGroupLevel++;

	CatTokenType expect;
	bool done = false;
	BOOL errorOccurred = TRUE;

	while (!done) {
		GetNextToken();

		// First examine thisToken.  If we've a line separator,
		// just loop again and get the next token because we're
		// looking for a clause beginning at thisToken.  If it's
		// end-of-file, we exit the loop.
		//
		if (thisToken->isEOL()) continue;
		else if (thisToken->isEOF()) {
			if (nGroupLevel == 0) {
				errorOccurred = FALSE;
			}
			else {
				GetCATMessages().Error(nLineNumber, GetString(IDS_ERR_CURLYBRACE));
			}
			curClause = rigEnd;
			done = true;
			continue;
		}

		// Now we examine the next token to determine the clause type.
		// If it's an openCurly then we expect a group clause.
		// If it's an equals then we expect an assignment clause.
		// Whether the formation of these clauses is legal or not is
		// yet unknown.  We may have an unknown or wrong identifier
		// beginning a group clause, for example.  We employ as much
		// error recovery as we can in these situations.
		//
		switch (nextToken->type) {
			// If the next token is an open curly brace, then this
			// is a group clause, all going well...  If the token
			// before the curly is not a group identifier then we
			// have a problem.  There's two approaches.
			//   1. This is a group clause because of the curly,
			//      but is illegal because the identifier is not
			//      a group identifier.
			//   2. This is an assignment clause because of the
			//      identifier, but is illegal because there's a
			//      curly instead of an equals.
			// We attempt to resolve ambiguity by looking at the
			// token which follows the curly.  If this token is
			// a value type that matches the identifier's expected
			// type, then we have (2).  If the token is EOL, we
			// have (1).  If the token is a value type that does
			// not match the identifier's expected type, we have
			// (1) followed by an "expected group or assignment"
			// error.
			//
			// Other possible cases are if the preceding token is
			// something stupid.  In all of these cases we assume
			// this is still a group clause, and spit out an
			// "expected group identifier" message.
			//
			// In every case except for (2), we still enter a new
			// group level, but may skip the entire group.
		case tokOpenCurly:
			// Save some state information and skip past the
			// openCurly token.
			curIdentifier = thisToken->id;
			curClause = rigBeginGroup;
			curType = thisToken->type;
			expect = thisToken->expect;

			GetNextToken();

			if (curType == tokGroup) {
				// Success - this is the only case where an error
				// doesn't occur.
				errorOccurred = FALSE;
				done = true;

				// If beginning a new group, push the group id onto
				// the stack, but don't increment nGroupLevel.  It
				// gets incremented the next time ReadClause() is
				// called.
				if (nGroupLevel >= groupstack.size())
					groupstack.resize(groupstack.size() + 16);
				groupstack[nGroupLevel] = curIdentifier;
			}
			else if (curType == tokIdentifier && nextToken->isValue()) {
				// This looks like an assignment -- skip its value.
				GetCATMessages().Error(nLineNumber, GetString(IDS_ERR_NOTIDENT), IdentName(curIdentifier));
				SkipNextTokenOrValue();
			}
			else {
				// Push a fake group onto the stack, and skip to the
				// end of it.  The function SkipGroup() makes calls
				// to this function, so the auto-increment group
				// level thing still works and we don't do it here.
				GetCATMessages().Error(nLineNumber, GetString(IDS_ERR_GROUPIDENT));

				if (nGroupLevel >= groupstack.size())
					groupstack.resize(groupstack.size() + 16);
				groupstack[nGroupLevel] = idUnknown;

				SkipGroup();
			}
			continue;

			// The next token is an assignment operator (equals).
			// First we need to check that the preceding token is
			// an identifier.  If it is, we check the token that
			// follows the equals.  If this is the type that the
			// identifier expects, or is transmutable to that type,
			// then the clause is valid.  If the following type
			// does not match or cannot be transmuted to the
			// expected type, we DbgAssert a type error, skip the
			// clause and continue.
			//
			// If the preceding token is a group identifier, we
			// still assume it's an assignment.  It's illegal
			// because you can't assign to a group identifier.
			// We still look for a value type following the
			// equals.  If there is none, we give a "missing
			// value" error.  We choose the philosophy whereby
			// the user didn't realise the identifier was for
			// groups not assignments.
			//
			// If the preceding token is anything else and the
			// token following equals is a value type, we chuck
			// an "expected identifier" message and skip past the
			// value type.  If the following token is not a value
			// type we freak and skip to EOL while chucking a
			// "syntax error".
		case tokEquals:
			// Save some state information and skip past the
			// equals token, so that nextToken should point
			// to a value type.
			curIdentifier = thisToken->id;
			curClause = rigAssignment;
			curType = thisToken->type;
			expect = thisToken->expect;

			GetNextToken();

			if (curType == tokIdentifier) {
				if (nextToken->type == expect) {
					// Assignment is properly formed.  Break out
					// here so we don't call continue cos we need
					// to get to the assignment checking stuff at
					// the end of the loop.
					errorOccurred = FALSE;
					curType = expect;
					done = true;
					break;
				}
				else if (nextToken->TransmuteTo(expect)) {
					// The value can and has been transmuted to
					// our desired type.  Now, issue a warning
					// and break (to avoid continue -- see
					// previous comment).
					if (!(nextToken->transmutedFrom == tokInt && expect == tokFloat)) {
						// GB 05-Feb-2004: Only warn if the conversion is unreasonable.
						GetCATMessages().Warning(nLineNumber, GetString(IDS_CONVERSION), TokenName(nextToken->transmutedFrom), TokenName(expect));
					}
					errorOccurred = FALSE;
					curType = expect;
					done = true;
					break;
				}
				else if (nextToken->isValue()) {
					// Assignment has wrong type.  Give a type
					// error and call GetNextToken() to prepare
					// for the next clause (we skip this one).
					AssertTypeError(curIdentifier, expect, nextToken->type);
					SkipNextTokenOrValue();
				}
				else {
					// There is no value on right-hand side of
					// assignment.
					AssertTypeError(curIdentifier, expect, nextToken->type);
					SkipNextTokenOrValue();
				}
			}
			else if (curType == tokGroup) {
				if (nextToken->isValue()) {
					// The assignment is okay, except that it's to
					// a group identifier.
					GetCATMessages().Error(nLineNumber, GetString(IDS_ERR_NOASSIGNIDENT), IdentName(curIdentifier));
					SkipNextTokenOrValue();
				}
				else {
					// Complain about the equals sign, since there's
					// no value on the right.  The clause looks very
					// much like a group clause!
					AssertExpectedGot(expect, tokEquals);
				}
			}
			else {
				// This clause couldn't look less like an assignment
				// if it tried...  Freak out and skip to EOL.
				AssertSyntaxError();
				SkipToEOL();
			}
			continue;
		}

		// Detect clauses that use only the first token, or clauses
		// that are missing the second token.
		//
		if (!done) {
			switch (thisToken->type) {
				// If ending a group, decrement the group level
				// immediately.  If the group level is already at
				// ground-zero, we give an error bitching about how
				// there's too many close curlies.
			case tokCloseCurly:
				curClause = rigEndGroup;
				done = true;

				if (nGroupLevel > 0) {
					errorOccurred = FALSE;
					nGroupLevel--;
				}
				else {
					GetCATMessages().Error(nLineNumber, GetString(IDS_ERR_NOCURLY));
				}
				break;

				// If we get a group identifier here, it could be because the
				// group is missing an open curly.  But it could be a group that
				// expects a value to appear before the curly.  We can distinguish
				// by using the 'expect' field.
				//
				// If the group expects a curly, then we pretend there's one there
				// and continue, after issuing an error message.
				//
				// If the group expects a value, then the token following the value
				// must be a curly.
			case tokGroup:
				curIdentifier = thisToken->id;
				curClause = rigBeginGroup;
				curType = thisToken->type;
				expect = thisToken->expect;

				if (expect == tokOpenCurly) {
					GetCATMessages().Error(nLineNumber, GetString(IDS_ERR_MISSING), TokenName(tokOpenCurly));
					done = true;

					if (nGroupLevel >= groupstack.size())
						groupstack.resize(groupstack.size() + 16);
					groupstack[nGroupLevel] = curIdentifier;
				}
				else {
					// Arrange thisToken to point to the value, so it can be
					// retrieved by calling GetValue().  Do this by calling
					// GetNextToken().  The other reason for doing this of course
					// is to check that the following token is a curly.
					GetNextToken();

					if (nextToken->isOpenCurly()) {
						// Success - this is the only case where an error
						// doesn't occur.
						errorOccurred = FALSE;
						done = true;

						// If beginning a new group, push the group id onto
						// the stack, but don't increment nGroupLevel.  It
						// gets incremented the next time ReadClause() is
						// called.
						if (nGroupLevel >= groupstack.size())
							groupstack.resize(groupstack.size() + 16);
						groupstack[nGroupLevel] = curIdentifier;
					}
					else {
						// Just assume that the curly is missing and perform no
						// other error recovery.
						GetCATMessages().Error(nLineNumber, GetString(IDS_ERR_MISSING), TokenName(tokOpenCurly));
						done = true;

						if (nGroupLevel >= groupstack.size())
							groupstack.resize(groupstack.size() + 16);
						groupstack[nGroupLevel] = curIdentifier;
					}
				}
				break;

				// If we get an identifier here it's because it's
				// missing an equals.  Check there's still a value
				// sitting here.
			case tokIdentifier:
				GetCATMessages().Error(nLineNumber, GetString(IDS_ERR_MISSING), TokenName(tokEquals));

				curIdentifier = thisToken->id;
				curClause = rigAssignment;
				curType = thisToken->type;
				expect = thisToken->expect;

				if (nextToken->type == expect) {
					// Assignment is properly formed and nextToken
					// already points to the value.
					curType = expect;
					done = true;
				}
				else if (nextToken->TransmuteTo(expect)) {
					// The value can and has been transmuted to
					// our desired type.
					if (!(nextToken->transmutedFrom == tokInt && expect == tokFloat)) {
						// GB 05-Feb-2004: Only warn if the conversion is unreasonable.
						GetCATMessages().Warning(nLineNumber, GetString(IDS_CONVERSION), TokenName(nextToken->transmutedFrom), TokenName(expect));
					}
					curType = expect;
					done = true;
				}
				else if (nextToken->isValue()) {
					// Assignment has wrong type.  Give a type
					// error and call GetNextToken() to prepare
					// for the next clause (we skip this one).
					AssertTypeError(curIdentifier, expect, nextToken->type);
					SkipNextTokenOrValue();
				}
				else {
					// There is no value on right-hand side of
					// assignment.
					AssertTypeError(curIdentifier, expect, nextToken->type);
					SkipNextTokenOrValue();
				}
				break;

				// If we have a value sitting around on its own, we
				// assume some dimwit forgot to put in an identifier
				// and an equals sign.
			case tokBool:
			case tokInt:
			case tokULONG:
			case tokFloat:
			case tokString:
			case tokPoint:
			case tokQuat:
			case tokAngAxis:
			case tokMatrix:
				curIdentifier = thisToken->id;
				curType = thisToken->type;
				GetCATMessages().Error(nLineNumber, GetString(IDS_ERR_VALUETYPE), TokenName(curType));
				SkipNextTokenOrValue();
				break;

				// Up to this point everything legal or seemingly
				// legal has been handled.  If we are here, there's
				// a problem.  Issue a syntax error, skip the line,
				// and be done.
			default:
				AssertSyntaxError();
				SkipToEOL();
			}
		}

		// Finally, if we reckon we're done, and this is an assignment
		// clause, check what sort of value we're assigning.  If it's
		// a point, quat, or angaxis, there's one last terrifying
		// parse to make sure the type is formed properly.  If not,
		// we go back round the loop for more exciting fun.  No matter
		// what case, always call GetNextToken() to get rid of the
		// value.
		if (done && curClause == rigAssignment) {
			switch (curType) {
			case tokPoint:
				done = ReadSomeFloats(3);
				break;
			case tokQuat:
				done = ReadSomeFloats(4);
				break;
			case tokAngAxis:
				done = ReadSomeFloats(4);
				break;
			case tokMatrix:
				done = ReadSomeFloats(12);
				break;
			case tokKeyBezFloat:
				if (ReadSomeULONGS(1)) {
					if (ReadSomeInts(1))
						done = ReadSomeFloats(5);
				}
				break;
			case tokKeyBezXYZ:
				if (ReadSomeULONGS(1)) {
					if (ReadSomeInts(1))
						done = ReadSomeFloats(15);
				}
				break;
			case tokKeyBezScale:
				if (ReadSomeULONGS(1)) {
					if (ReadSomeInts(1))
						done = ReadSomeFloats(19);
				}
				break;
			case tokKeyBezRot:
				if (ReadSomeULONGS(1)) {
					if (ReadSomeInts(1))
						done = ReadSomeFloats(4);
				}
				break;
			case tokKeyTCBFloat:
				if (ReadSomeULONGS(1)) {
					if (ReadSomeInts(1))
						done = ReadSomeFloats(6);
				}
				break;
			case tokKeyTCBXYZ:
				if (ReadSomeULONGS(1)) {
					if (ReadSomeInts(1))
						done = ReadSomeFloats(8);
				}
				break;
			case tokKeyTCBScale:
				if (ReadSomeULONGS(1)) {
					if (ReadSomeInts(1))
						done = ReadSomeFloats(12);
				}
				break;
			case tokKeyTCBRot:
				if (ReadSomeULONGS(1)) {
					if (ReadSomeInts(1))
						done = ReadSomeFloats(9);
				}
				break;
			case tokKeyLinFloat:
				if (ReadSomeULONGS(1)) {
					if (ReadSomeInts(1))
						done = ReadSomeFloats(1);
				}
				break;
			case tokKeyLinXYZ:
				if (ReadSomeULONGS(1)) {
					if (ReadSomeInts(1))
						done = ReadSomeFloats(3);
				}
				break;
			case tokKeyLinScale:
				if (ReadSomeULONGS(1)) {
					if (ReadSomeInts(1))
						done = ReadSomeFloats(7);
				}
				break;
			case tokKeyLinRot:
				if (ReadSomeULONGS(1)) {
					if (ReadSomeInts(1))
						done = ReadSomeFloats(4);
				}
				break;
			case tokClassIDs:
				done = ReadSomeULONGS(3);
				if (dwFileVersion >= CAT_VERSION_1200) {
					// step ofer the current token so our string becomes the current token
					GetNextToken();
				}
				break;
			case tokInterval:
				done = ReadSomeInts(2);
				break;

				// new simple keyframe classes
			case tokKeyFloat:
				if (ReadSomeULONGS(0)) {
					if (ReadSomeInts(2))
						done = ReadSomeFloats(1);
				}
				break;
			case tokKeyPoint:
				if (ReadSomeULONGS(1)) {
					if (ReadSomeInts(1))
						done = ReadSomeFloats(3);
				}
				break;
			case tokKeyMatrix:
				if (ReadSomeULONGS(1)) {
					if (ReadSomeInts(1))
						done = ReadSomeFloats(12);
				}
				break;

			case tokLayerInfo:
				if (ReadSomeULONGS(2)) {
					// step ofer the current token so our string becomes the current token
					GetNextToken();
				}
				break;

			case tokFace:
				done = ReadSomeInts(5);
				break;

			case tokINode:
				GetNextToken();
				break;
			}
			GetNextToken();

			if (!done)
				errorOccurred = TRUE;
		}
	}

	// Update progress bar
	if (pProgressWindow && nCharsRead >= nNextProgressUpdateChars) {
		nNextProgressUpdateChars = min(nCharsRead + nProgressIncrementChars, nFileSize);
		pProgressWindow->SetPercent((100 * nCharsRead) / nFileSize);
	}

	return !errorOccurred && ok();
}

// There is a token stored in nextToken that we wish to skip.
// It may be a multi-token value, like a quat or point.  We
// don't skip groups or anything like that.  This is just real
// simple.  If nextToken is EOL or EOF we do nothing.
bool CATRigReader::SkipNextTokenOrValue()
{
	if (nextToken->isEOLorEOF()) return true;
	GetNextToken();
	switch (nextToken->type) {
	case tokPoint:		return ReadSomeFloats(3);
	case tokQuat:		return ReadSomeFloats(4);
	case tokAngAxis:	return ReadSomeFloats(4);
	case tokMatrix:		return ReadSomeFloats(12);
		// not too sure how to step over the string at the end
	case tokClassIDs:	return ReadSomeULONGS(3);
	case tokInterval:	return ReadSomeInts(2);
	}
	return true;
}

// Skips to EOL (or EOF), so that the token is stored in
// thisToken.
void CATRigReader::SkipToEOL()
{
	do {
		GetNextToken();
	} while (!thisToken->isEOLorEOF());
}

// This function reads 'n' floats tokens and stores them in
// the 'floatvalues' array.  The maximum number we can read
// is 4.  It returns true if successful and we should end
// the clause, otherwise false (if there were errors,
// indicating that we should continue and ignore the clause).
bool CATRigReader::ReadSomeFloats(int n) {
	DbgAssert(n <= MAX_TOKENCHARS);
	bool success = true;
	for (int i = 0; i < n; i++) {
		GetNextToken();
		if (!nextToken->isNumber()) {
			GetCATMessages().Error(nLineNumber, GetString(IDS_ERR_EXPECTED), TokenName(tokFloat), TokenName(nextToken->type));
			success = false;
			if (nextToken->isEOLorEOF()) break;
		}
		floatvalues[i] = nextToken->asFloat();
	}
	return success;
}

// This function reads 'n' floats tokens and stores them in
// the 'floatvalues' array.  The maximum number we can read
// is 4.  It returns true if successful and we should end
// the clause, otherwise false (if there were errors,
// indicating that we should continue and ignore the clause).
bool CATRigReader::ReadSomeInts(int n) {
	DbgAssert(n <= MAX_TOKENCHARS);
	bool success = true;
	for (int i = 0; i < n; i++) {
		GetNextToken();
		if (!nextToken->isNumber()) {
			GetCATMessages().Error(nLineNumber, GetString(IDS_ERR_EXPECTED), TokenName(tokInt), TokenName(nextToken->type));
			success = false;
			if (nextToken->isEOLorEOF()) break;
		}
		intvalues[i] = nextToken->asInt();
	}
	return success;
}

bool CATRigReader::ReadSomeULONGS(int n) {
	DbgAssert(n <= MAX_TOKENCHARS);
	bool success = true;
	for (int i = 0; i < n; i++) {
		GetNextToken();
		if (!nextToken->isNumber()) {
			GetCATMessages().Error(nLineNumber, GetString(IDS_ERR_EXPECTED), TokenName(tokULONG), TokenName(nextToken->type));
			success = false;
			if (nextToken->isEOLorEOF()) break;
		}
		const tstring& str = nextToken->asString();
		ulongvalues[i] = (ULONG)_tstof(&str[0]);
	}
	return success;
}

// This is a generic GetValue() called by the public GetValue()
// functions.  It's just here to reduce unnecessary work.
// Returns TRUE ordinarily, or FALSE if we've really messed up
// and called it incorrectly.
BOOL CATRigReader::GetValue(void* val, CatTokenType type) {
	//	if (curType == type) {
	switch (type) {
	case tokBool:		*(bool*)val = thisToken->asBool(); break;
	case tokInt:		*(int*)val = thisToken->asInt(); break;
	case tokULONG:		*(int*)val = thisToken->asULONG(); break;
	case tokFloat:		*(float*)val = thisToken->asFloat(); break;
	case tokString:		*(TSTR*)val = TSTR(thisToken->asString().c_str()); break;
	case tokPoint:		*(Point3*)val = Point3(floatvalues); break;
	case tokQuat:		*(Quat*)val = Quat(floatvalues); break;
	case tokAngAxis:	*(AngAxis*)val = AngAxis(Point3(floatvalues), floatvalues[3]); break;
	case tokMatrix:		*(Matrix3*)val = Matrix3(Point3(floatvalues), Point3(&floatvalues[3]), Point3(&floatvalues[6]), Point3(&floatvalues[9])); break;
	case tokKeyBezFloat: {
		(*(IBezFloatKey*)val).flags = (DWORD)ulongvalues[0];
		(*(IBezFloatKey*)val).time = (TimeValue)intvalues[0];
		(*(IBezFloatKey*)val).val = floatvalues[0];
		(*(IBezFloatKey*)val).intan = floatvalues[1];
		(*(IBezFloatKey*)val).outtan = floatvalues[2];
		(*(IBezFloatKey*)val).inLength = floatvalues[3];
		(*(IBezFloatKey*)val).outLength = floatvalues[4];
		break;
	}
	case tokKeyBezXYZ: {
		(*(IBezPoint3Key*)val).flags = (DWORD)ulongvalues[0];
		(*(IBezPoint3Key*)val).time = (TimeValue)intvalues[0];
		(*(IBezPoint3Key*)val).val = Point3(&floatvalues[0]);
		(*(IBezPoint3Key*)val).intan = Point3(&floatvalues[3]);
		(*(IBezPoint3Key*)val).outtan = Point3(&floatvalues[6]);
		(*(IBezPoint3Key*)val).inLength = Point3(&floatvalues[9]);
		(*(IBezPoint3Key*)val).outLength = Point3(&floatvalues[12]);
		break;
	}
	case tokKeyBezScale: {
		(*(IBezScaleKey*)val).flags = (DWORD)ulongvalues[0];
		(*(IBezScaleKey*)val).time = (TimeValue)intvalues[0];
		(*(IBezScaleKey*)val).val.s = Point3(&floatvalues[0]);
		(*(IBezScaleKey*)val).val.q = Quat(&floatvalues[3]);
		(*(IBezScaleKey*)val).intan = Point3(&floatvalues[7]);
		(*(IBezScaleKey*)val).outtan = Point3(&floatvalues[10]);
		(*(IBezScaleKey*)val).inLength = Point3(&floatvalues[13]);
		(*(IBezScaleKey*)val).outLength = Point3(&floatvalues[16]);
		break;
	}
	case tokKeyBezRot: {
		(*(IBezQuatKey*)val).flags = (DWORD)ulongvalues[0];
		(*(IBezQuatKey*)val).time = (TimeValue)intvalues[0];
		(*(IBezQuatKey*)val).val = Quat(floatvalues);
		break;
	}
	case tokKeyTCBFloat:
	case tokKeyTCBXYZ:
	case tokKeyTCBScale:
	case tokKeyTCBRot:
	{
		int currIndex = 0;
		(*(ITCBKey*)val).flags = (DWORD)ulongvalues[0];
		(*(ITCBKey*)val).time = (TimeValue)intvalues[0];
		switch (type)
		{
		case tokKeyTCBFloat:
			(*(ITCBFloatKey*)val).val = floatvalues[0];
			currIndex = 1;
			break;
		case tokKeyTCBXYZ:
			(*(ITCBPoint3Key*)val).val = Point3(&floatvalues[0]);
			currIndex = 3;
			break;
		case tokKeyTCBScale:
			(*(ITCBScaleKey*)val).val.s = Point3(&floatvalues[0]);
			(*(ITCBScaleKey*)val).val.q = Quat(&floatvalues[3]);
			currIndex = 7;
			break;
		case tokKeyTCBRot:
			(*(ITCBRotKey*)val).val.angle = floatvalues[0];
			(*(ITCBRotKey*)val).val.axis = Point3(&floatvalues[1]);
			currIndex = 4;
			break;
		}
		(*(ITCBKey*)val).tens = floatvalues[currIndex++];
		(*(ITCBKey*)val).cont = floatvalues[currIndex++];
		(*(ITCBKey*)val).bias = floatvalues[currIndex++];
		(*(ITCBKey*)val).easeIn = floatvalues[currIndex++];
		(*(ITCBKey*)val).easeOut = floatvalues[currIndex++];
		break;
	}
	case tokKeyLinFloat: {
		(*(ILinFloatKey*)val).flags = (DWORD)ulongvalues[0];
		(*(ILinFloatKey*)val).time = (TimeValue)intvalues[0];
		(*(ILinFloatKey*)val).val = floatvalues[0];
		break;
	}
	case tokKeyLinXYZ: {
		(*(ILinPoint3Key*)val).flags = (DWORD)ulongvalues[0];
		(*(ILinPoint3Key*)val).time = (TimeValue)intvalues[0];
		(*(ILinPoint3Key*)val).val = Point3(&floatvalues[0]);
		break;
	}
	case tokKeyLinScale: {
		(*(ILinScaleKey*)val).flags = (DWORD)ulongvalues[0];
		(*(ILinScaleKey*)val).time = (TimeValue)intvalues[0];
		(*(ILinScaleKey*)val).val.s = Point3(floatvalues);
		(*(ILinScaleKey*)val).val.q = Quat(&floatvalues[3]);
		break;
	}
	case tokKeyLinRot: {
		(*(ILinRotKey*)val).flags = (DWORD)intvalues[0];
		(*(ILinRotKey*)val).time = (TimeValue)intvalues[0];
		(*(ILinRotKey*)val).val = Quat(floatvalues);
		break;
	}
	case tokClassIDs:
		(*(TokClassIDs*)val).usSuperClassID = ulongvalues[0];
		(*(TokClassIDs*)val).usClassIDA = ulongvalues[1];
		(*(TokClassIDs*)val).usClassIDB = ulongvalues[2];
		if (dwFileVersion >= CAT_VERSION_1200) {
			(*(TokClassIDs*)val).classname = TSTR(thisToken->asString().c_str());
		}
		break;
	case tokInterval:
		(*(Interval*)val).SetStart(intvalues[0]);
		(*(Interval*)val).SetEnd(intvalues[1]);
		break;
		// new simple keyframe classes
	case tokKeyFloat: {
		(*(CATClipKeyFloat*)val).flags = (DWORD)ulongvalues[0];
		(*(CATClipKeyFloat*)val).time = (TimeValue)intvalues[0];
		(*(CATClipKeyFloat*)val).val = floatvalues[0];
		break;
	}
	case tokKeyPoint: {
		(*(CATClipKeyPoint*)val).flags = (DWORD)ulongvalues[0];
		(*(CATClipKeyPoint*)val).time = (TimeValue)intvalues[0];
		(*(CATClipKeyPoint*)val).val = Point3(floatvalues);
		break;
	}
	case tokKeyMatrix: {
		(*(CATClipKeyMatrix*)val).flags = (DWORD)ulongvalues[0];
		(*(CATClipKeyMatrix*)val).time = (TimeValue)intvalues[0];
		(*(CATClipKeyMatrix*)val).val = Matrix3(Point3(floatvalues), Point3(&floatvalues[3]), Point3(&floatvalues[6]), Point3(&floatvalues[9]));
		break;
	}

	case tokLayerInfo: {
		((LayerInfo*)val)->dwFlags = (DWORD)ulongvalues[0];
		((LayerInfo*)val)->dwLayerColour = (COLORREF)ulongvalues[1];
		((LayerInfo*)val)->strName = TSTR(thisToken->asString().c_str());
		break;
	}

	case tokFace: {
		((Face*)val)->setVerts((DWORD)intvalues[0], (DWORD)intvalues[1], (DWORD)intvalues[2]);
		((Face*)val)->setSmGroup((DWORD)intvalues[3]);
		((Face*)val)->flags = (DWORD)intvalues[4];
		break;
	}
	case tokINode: {
		TSTR str = TSTR(thisToken->asString().c_str());
		INode* node = catparenttrans->GetBoneByAddress(str);
		(*(INode**)val) = node;
		break;
	}
	}
	return TRUE;
}

// This skips an entire group, including its subgroups.  If
// the current identifier is a begingroup, we skip it and all its
// subgroups.  Otherwise we skip the rest of the group, including
// its subgroups.
void CATRigReader::SkipGroup()
{
	int nTargetLevel;

	if (curClause == rigBeginGroup) {
		// The group level hasn't yet been incremented and
		// won't be until the next call to ReadClause(),
		// so the current level is the target.
		nTargetLevel = nGroupLevel;
	}
	else {
		// The target level is one below the current level.
		// if the current level is zero, it's okay because
		// we'll finish when rigEnd is hit.
		nTargetLevel = nGroupLevel - 1;
	}

	// This part does the skipping.  Note that errors and
	// stuff in the clauses being skipped will still show up.
	while (curClause != rigEnd && curClause != rigAbort) {
		ReadClause();
		if (nGroupLevel == (ULONG)nTargetLevel) break;
	}
}

// Stores a value into a paramblock.  Some types cannot be
// stored in a paramblock, and if you try, this function
// returns FALSE.  Otherwise it's all good and returns TRUE.
//
BOOL CATRigReader::ToParamBlock(IParamBlock2* pblock, ParamID pid)
{
	switch (curType) {
	case tokBool:
	{
		// Max bools are floats
		bool b;
		float val;
		if (!GetValue(b)) return FALSE;
		val = b ? 1.0f : 0.0f;
		pblock->EnableNotifications(FALSE);
		pblock->SetValue(pid, 0, val);
	}
	break;

	case tokInt:
	{
		int val;
		if (!GetValue(val)) return FALSE;
		//		pblock->EnableNotifications(FALSE);
		pblock->SetValue(pid, 0, val);
	}
	break;
	case tokULONG:
	{
		ULONG val;
		if (!GetValue(val)) return FALSE;
		//		pblock->EnableNotifications(FALSE);
		pblock->SetValue(pid, 0, (int)val);
	}
	break;

	case tokFloat:
	{
		float val;
		if (!GetValue(val)) return FALSE;
		pblock->EnableNotifications(FALSE);
		pblock->SetValue(pid, 0, val);
	}
	break;

	case tokString:
	{
		TSTR val;
		if (!GetValue(val)) return FALSE;
		pblock->EnableNotifications(FALSE);
		pblock->SetValue(pid, 0, val.data());
	}
	break;

	case tokPoint:
	{
		Point3 val;
		if (!GetValue(val)) return FALSE;
		pblock->EnableNotifications(FALSE);
		pblock->SetValue(pid, 0, val);
	}
	break;

	case tokQuat:
	case tokAngAxis:
		return FALSE;
		break;

	case tokMatrix:
	{
		Matrix3 val;
		if (!GetValue(val)) return FALSE;
		pblock->EnableNotifications(FALSE);
		pblock->SetValue(pid, 0, val);
	}
	break;

	default:
		return FALSE;
	}
	pblock->EnableNotifications(TRUE);
	return TRUE;
}

// Stores a value into a controller.  All numeric/bool types
// get stored as floats.  Attempting this on a string results
// in a return value of FALSE (no such thing as a string
// controller).  Otherwise (except if there's an internal
// error, which there shouldn't be) returns TRUE.
//
BOOL CATRigReader::ToController(Control* ctrl)
{
	switch (curType) {
	case tokBool:
	{
		// Max bools are floats
		bool b;
		float val;
		if (!GetValue(b)) return FALSE;
		val = b ? 1.0f : 0.0f;
		ctrl->SetValue(0, (void*)&val);
	}
	break;

	case tokULONG:
	case tokInt:
	{
		// Only floats are stored in a controller.
		int i;
		float val;
		if (!GetValue(i)) return FALSE;
		val = (float)i;
		ctrl->SetValue(0, (void*)&val);
	}
	break;

	case tokFloat:
	{
		float val;
		if (!GetValue(val)) return FALSE;
		ctrl->SetValue(0, (void*)&val);
	}
	break;

	case tokString:
		// no such thing as a string controller
		return FALSE;
		break;

	case tokPoint:
	{
		Point3 val;
		if (!GetValue(val)) return FALSE;
		ctrl->SetValue(0, (void*)&val);
	}
	break;

	case tokQuat:
	{
		Quat val;
		if (!GetValue(val)) return FALSE;
		ctrl->SetValue(0, (void*)&val);
	}
	break;

	case tokAngAxis:
	{
		// CTRL_ABSOLUTE on a rotation controller
		// requires a Quat value.
		AngAxis aa;
		if (!GetValue(aa)) return FALSE;
		Quat val(aa);
		ctrl->SetValue(0, (void*)&val);
	}
	break;

	case tokMatrix:
	{
		Matrix3 tm;
		if (!GetValue(tm)) return FALSE;
		SetXFormPacket val(tm);
		ctrl->SetValue(0, (void*)&val, TRUE);
	}
	break;

	default:
		return FALSE;
	}
	return TRUE;
}

void CATRigReader::AssertTypeError(int idIdentifier, int tokExpected, int tokActual)
{
	GetCATMessages().Error(nLineNumber, GetString(IDS_ERR_EXPECTED1),
		IdentName(idIdentifier), TokenName(tokExpected), TokenName(tokActual));
}

void CATRigReader::AssertExpectedGot(CatTokenType tokExpected, CatTokenType tokGot)
{
	GetCATMessages().Error(nLineNumber, GetString(IDS_ERR_EXPECTED), TokenName(tokExpected), TokenName(tokGot));
}

void CATRigReader::AssertSyntaxError()
{
	GetCATMessages().Error(nLineNumber, GetString(IDS_ERR_SYNTAX));
}

void CATRigReader::AssertNoSubGroups()
{
	if (nGroupLevel == 0)
		GetCATMessages().Error(nLineNumber, GetString(IDS_ERR_NOSUBGROUPS));
	else
		GetCATMessages().Error(nLineNumber, GetString(IDS_ERR_NOSUBGROUPS1), IdentName(groupstack[nGroupLevel - 1]));
}

void CATRigReader::AssertOutOfPlace()
{
	if (nGroupLevel == 0)
		GetCATMessages().Error(nLineNumber, GetString(IDS_ERR_NOIDENTHERE), IdentName(curIdentifier));
	else
		GetCATMessages().Error(nLineNumber, GetString(IDS_ERR_NOIDENTGROUP), IdentName(curIdentifier), IdentName(groupstack[nGroupLevel - 1]));
}

void CATRigReader::AssertObjectCreationError()
{
	GetCATMessages().Error(nLineNumber, GetString(IDS_ERR_OBJCREATE));
}

/************************************************************************/
/* We have just read a new Controller { group. So we now figure out     */
/* what kind of subindex we are reading and if the current one is        */
/* different we set the flag for use simplekeys                         */
/************************************************************************/
BOOL CATRigReader::ReadSubAnim(Control *ctrl, int subindex, Interval timerange, const float dScale, int flags)
{
	if (CurClauseID() == rigBeginGroup)// && CurIdentifier() == idController || CurIdentifier() == idWeights || CurIdentifier() == idGroupWeights || CurIdentifier() == idController
	{
		if (ctrl == NULL)
		{
			SkipGroup();
			return TRUE;
		}

		// MAXX-7673 (Max 2013 + only)
		// in 2013, we stopped returning subanims directly from
		// the CATHierarchyLeaf, and returned the pblock directly.
		// Turns out this was a bad idea, because our clips rely
		// on the subanim structure.  We deal with ambiguity below
		CATHierarchyLeaf* pLeaf = dynamic_cast<CATHierarchyLeaf*>(ctrl);

		int nSubs = (pLeaf != NULL) ? pLeaf->GetNumLayers() : ctrl->NumSubs();
		if (subindex >= nSubs) {
			SkipGroup();
			return TRUE;
		}

		Animatable* sub = NULL;
		if (CurIdentifier() == idParamBlock)
		{
			BOOL res = FALSE;
			IParamBlock2 *pblock = dynamic_cast<IParamBlock2*>(ctrl->SubAnim(subindex));
			if (pblock != NULL)
				res = ReadParamBlock(pblock, timerange, dScale, flags);
			else
				SkipGroup();
			return res;
		}
		else if (pLeaf != NULL)
		{
			// MAXX-7673 (Max 2013 + only) part deux
			// If we were a CATHierarchyLeaf, and we weren't requesting
			// to load a PB2, then we must be loading and old-skool parameter,
			// which loaded directly onto the leaves (and skipped the PB2)
			sub = pLeaf->GetLayer(subindex);
		}
		else
			sub = ctrl->SubAnim(subindex);

		if (!GetControlInterface(sub) && !GetIListControlInterface(ctrl)) {
			return FALSE;
		}
		Control *newSubAnim = (Control*)sub;
		DbgAssert(newSubAnim);

		//process the lock flags. If our any of our p/r/s is locked then skip that branch in the loader
		if (((newSubAnim->SuperClassID() == CTRL_POSITION_CLASS_ID) && flags&CLIPFLAG_SKIPPOS) ||
			((newSubAnim->SuperClassID() == CTRL_ROTATION_CLASS_ID) && flags&CLIPFLAG_SKIPROT) ||
			((newSubAnim->SuperClassID() == CTRL_SCALE_CLASS_ID) && flags&CLIPFLAG_SKIPSCL))
		{
			SkipGroup();
			return TRUE;
		}

		//////////////////////////////////////////////////////////////////////////
		// ST - Scaling data requires that we be capable of scaling
		// floats, but only if the float is a subindex of a position.
		// here we clear our scale flag if the next controller is
		// not capable of containing position information.
		int newFlags = flags;
		if (!((ctrl->SuperClassID() == CTRL_POSITION_CLASS_ID) || (ctrl->SuperClassID() == CTRL_MATRIX3_CLASS_ID)))
			newFlags = newFlags&~CLIPFLAG_SCALE_DATA;

		//////////////////////////////////////////////////////////////////////////
		// If we are a XYZ position controller
		// we only flip the x axis
		if (ctrl->SuperClassID() == CTRL_POSITION_CLASS_ID) {
			if (ctrl->ClassID() == IPOS_CONTROL_CLASS_ID) {
				if (catparenttrans->GetLengthAxis() == Z) {
					if (subindex == 1 || subindex == 2)	newFlags = newFlags&~CLIPFLAG_MIRROR;
				}
				else {
					if (subindex == 0 || subindex == 1)	newFlags = newFlags&~CLIPFLAG_MIRROR;
				}
			}
		}

		// If we are a euler rotation controller
		// we don't flip the X axis, cause its pitch
		if (ctrl->SuperClassID() == CTRL_ROTATION_CLASS_ID) {
			if (ctrl->ClassID().PartA() == EULER_CONTROL_CLASS_ID) {
				if (catparenttrans->GetLengthAxis() == Z) {
					if (subindex == 0)	newFlags = newFlags&~CLIPFLAG_MIRROR;
				}
				else {
					if (subindex == 2)	newFlags = newFlags&~CLIPFLAG_MIRROR;
				}
			}
		}

		// If we are a euler rotation controller
		// we don't flip the X axis, cause its pitch
		if (ctrl->SuperClassID() == CTRL_SCALE_CLASS_ID) {
			newFlags = newFlags&~CLIPFLAG_MIRROR;
			newFlags = newFlags&~CLIPFLAG_SCALE_DATA;
		}

		if (ctrl->SuperClassID() == CTRL_POINT3_CLASS_ID) {
			if (ctrl->ClassID() == IPOINT3_CONTROL_CLASS_ID) {
				if (flags&CLIPFLAG_POS && (subindex == 1 || subindex == 2))
					newFlags = newFlags&~CLIPFLAG_MIRROR;
				if (flags&CLIPFLAG_ROT && (subindex == 0))
					newFlags = newFlags&~CLIPFLAG_MIRROR;
			}
		}
		// If we are loading into a foot pivot controller and we are about to laod the 2nd subanim
		// This controller is not in world space.
		if (ctrl->ClassID() == Class_ID(0x6e341cba, 0x6f9c52ca)) {//HD_PIVOTTRANS_CLASS_ID){
			if (subindex == 1) {
				newFlags &= ~CLIPFLAG_WORLDSPACE;
				newFlags &= ~CLIPFLAG_APPLYTRANSFORMS;
			}
			if (subindex == 2) {
				SkipGroup();
				return TRUE;
			}
		}

		// The next line can be 1 of 2 things. Either a Pose value for the current controller
		// or a classid which tells us what knid if subanim we have comming up
		NextClause();

		// we may have simply a pose saved out for this subindex
		switch (CurIdentifier()) {
		case idValFloat:
		case idValPoint:
		case idValQuat:
		case idValMatrix3:
			return ReadPoseGroup(newSubAnim, timerange.Start(), dScale, newFlags);
		}

		if (CurIdentifier() != idValClassIDs) return FALSE;
		TokClassIDs classid;
		GetValue(classid);
		Class_ID newClassID(classid.usClassIDA, classid.usClassIDB);

		if (newSubAnim->ClassID() != newClassID) {
			if (catparenttrans->GetCATLayerRoot()->GetSelectedLayerMethod() == LAYER_CATMOTION) {
				// We can't assign new controllers to CATMotion layers in the middle of loading.
				SkipGroup();
				return TRUE;
			}
			// create the new replacement subindex
			Control *newCtrl = (Control*)CreateInstance(classid.usSuperClassID, newClassID);

			if (newCtrl) {
				DbgAssert(newCtrl->SuperClassID() == classid.usSuperClassID);
				newCtrl->Copy(newSubAnim);
			}
			newSubAnim = newCtrl;
			ctrl->AssignController(newSubAnim, subindex);
		}

		ReadController(newSubAnim, timerange, dScale, newFlags);
	}
	else
	{
		SkipGroup();
		return TRUE;
	}
	return TRUE;
}
/************************************************************************/
/* This is a recursive function that traverses a controller hierarchy	*/
/* untill it finds a leaf, then calls the appropriate save function on  */
/* that leaf controller                                                 */
/************************************************************************/
BOOL CATRigReader::ReadController(Control *ctrl, Interval timerange, const float dScale, int flags)
{
	if (!ctrl) return FALSE;

	BOOL done = FALSE;
	int subanimindex = -1, tabNum = 0;
	Interval range;
	BOOL range_start_set = FALSE, range_end_set = FALSE;
	TSTR script = _T("");
	TSTR name = _T("");
	DWORD inheritflags = 0;

	// Create the Controls subanims, and set up the controller itself
	while (!done && ok())
	{
		if (!NextClause()) {
			//  Here everything goes funky...
			return FALSE;
		}
		switch (CurClauseID())
		{
		case rigBeginGroup:
			switch (CurIdentifier())
			{
				// Note, this group type is depreciated. We only have this group reader for legacy reasons
			case idPose:
				ReadPoseGroup(ctrl, timerange.Start(), dScale, flags);
				break;
			case idSimpleKeys:
				// we can use simple keys in places where we want to keep the original controller
				// but it is different to the current one. Or where we need to transfom/mirror
				// keyframes that re in world space.
				if (ctrl->SuperClassID() == CTRL_MATRIX3_CLASS_ID && flags&CLIPFLAG_WORLDSPACE && (flags&CLIPFLAG_MIRROR || (flags&CLIPFLAG_APPLYTRANSFORMS && !tmTransform.IsIdentity())))
					//	if(flags&CLIPFLAG_MIRROR || (flags&CLIPFLAG_APPLYTRANSFORMS && !tmTransform.IsIdentity()))
				{
					ReadSimpleKeys(ctrl, timerange, dScale, flags);
					// once we have read simplekeys, we really don't want to read anymore of this group
					// this skipgroup will skip the rest of the current controllers group
					SkipGroup();
					done = TRUE;
					break;
				}
				else// this will skip the SimpleKeys Group
					SkipGroup();
				break;
			case idKeys:
				ReadKeys(ctrl, timerange, dScale, flags);
				break;
			case idConstraint:
				ReadConstraint(ctrl, timerange, dScale, flags);
				break;
			case idScriptControlParams:
				ReadScriptController(ctrl, timerange, dScale, flags);
				break;
			case idScriptText:
				TSTR scriptstring(_T(""));
				if (ReadString(scriptstring) &&
					ctrl->GetInterface(IID_SCRIPT_CONTROL))
				{
					IScriptCtrl		*scriptctrl = (IScriptCtrl*)ctrl->GetInterface(IID_SCRIPT_CONTROL);
					scriptctrl->SetExpression(scriptstring);
				}
				break;
			}
			break;
		case rigAssignment:
		{
			switch (CurIdentifier()) {
			case idFlags:
				GetValue(inheritflags);
				// for some retarded reason, the flags are inverted when SETTING the inheritance flags
				// when 'Getting' a clear bit means it DOES inherit,
				// while 'Setting' a clear bit means it DOESN'T.... logical aye...
				inheritflags = ~inheritflags;
				break;
			case idOffsetTM: {
				Matrix3 tm(1);
				GetValue(tm);
				ctrl->SetInheritanceFlags(inheritflags, TRUE);
				Interval iv = FOREVER;
				ctrl->GetValue(timerange.Start(), (void*)&tm, iv, CTRL_RELATIVE);
				break;
			}
			case idORTIn: {
				int ort;
				DisableRefMsgs();
				if (GetValue(ort))
					ctrl->SetORT(ort, ORT_BEFORE);
				EnableRefMsgs();
				break;
			}
			case idORTOut: {
				int ort;
				DisableRefMsgs();
				if (GetValue(ort))
					ctrl->SetORT(ort, ORT_AFTER);
				EnableRefMsgs();
				break;
			}
			case idRangeStart: {
				int rangestart;
				if (GetValue(rangestart)) {
					range.SetStart(timerange.Start() + rangestart);
					range_start_set = TRUE;
				}
				break;
			}
			case idRangeEnd: {
				int rangeend;
				if (GetValue(rangeend)) {
					range.SetEnd(timerange.Start() + rangeend);
					range_end_set = TRUE;
				}
				break;
			}
			case idScript: {
				TSTR scriptline;
				if (GetValue(script)) {
					script = script + scriptline;
				}
				break;
			}
			case idAxisOrder: {
				int order;
				DisableRefMsgs();
				if ((ctrl->ClassID().PartA() == EULER_CONTROL_CLASS_ID) && GetValue(order))
				{
					IEulerControl *eulerctrl = (IEulerControl*)ctrl->GetInterface(I_EULERCTRL);
					if (eulerctrl) eulerctrl->SetOrder(order);
				}
				EnableRefMsgs();
				break;
			}
			case idListItemName:
				if (GetIListControlInterface(ctrl) && GetValue(name)) {
					IListControl *listctrl = GetIListControlInterface(ctrl);
					listctrl->SetName(tabNum, name);
				}
				tabNum++;
				break;
			case idListItemActive:
				if (GetIListControlInterface(ctrl)) {
					IListControl *listctrl = GetIListControlInterface(ctrl);
					int activelistitem;
					if (GetValue(activelistitem)) {
						listctrl->SetActive(activelistitem);
					}
				}
				break;
			case idWeldedLeaves:
				// New, CAT3.X clips store the welded state with the leaf,
				// to support welding on offset graphs (MAXX-5972)
				if (ctrl->ClassID() == CATHIERARCHYLEAF_CLASS_ID)
				{
					CATHierarchyLeaf* pLeaf = static_cast<CATHierarchyLeaf*>(ctrl);
					CATHierarchyBranch2* pBranch = pLeaf->GetLeafParent();
					DbgAssert(pBranch != NULL);
					if (pBranch != NULL)
					{
						BOOL welded;
						GetValue(welded);
						if (welded)
							pBranch->WeldLeaves();
						else pBranch->UnWeldLeaves();
					}
				}
				// Older clips can store the loaded state with the branch
				else if (ctrl->GetInterface(I_CATGRAPH)) {
					CATHierarchyBranch2* branch = ((CATGraph*)ctrl)->GetBranch();
					BOOL welded;
					GetValue(welded);
					if (welded)
						branch->WeldLeaves();
					else branch->UnWeldLeaves();
				}
				break;
			case idValFloat:
			case idValPoint:
			case idValQuat:
			case idValMatrix3:
				ReadPoseIntoController(ctrl, timerange.Start(), dScale, flags);
				break;

				// If we have any subanims, this
				// will set them up to be be the right type
			case idSubNum:
			{
				if (ctrl->IsLeaf()) {
					AssertOutOfPlace();
					SkipGroup();
					break;
				}
				int newSubNum;
				GetValue(newSubNum);

				// For table management, if we are
				// no longer assigning to the same parameter index
				// then reset the number back to the start of the
				// tab (this apply's for non tab members
				if (newSubNum != subanimindex)
					tabNum = 0;
				subanimindex = newSubNum;

				// If we have a subNumber, it means we
				// must have a controller next. Because
				// there is a set order of steps, we dont loop
				// and write out step by step
				NextClause();

				// this method figures out what kind of sub anim
				// we have and then calls ReadController
				if (!ReadSubAnim(ctrl, subanimindex, timerange, dScale, flags))
				{
					AssertOutOfPlace();
					SkipGroup();
				}
			}
			}
		}
		break;
		case rigEndGroup:
			done = TRUE;
			// we may have read in a script and so if have we
			// have put it into the script controller.
			if (script.Length() > 1 &&
				ctrl->GetInterface(IID_SCRIPT_CONTROL))
			{
				IScriptCtrl		*scriptctrl = (IScriptCtrl*)ctrl->GetInterface(IID_SCRIPT_CONTROL);
				scriptctrl->SetExpression(script);
			}
			break;
		default:
			AssertOutOfPlace();
			SkipGroup();
			return ok();
		}
	}
	// there is no way to detect whether manual ranges are on.
	// So we save out ranges regardless and laod them in.
	// Here we compare loaded ranges with current ones and if they are
	// different, then we can De-Couple the ranges and set them manually
	// CAT2.1
	// I have removed this because. Sometimes, you may be loading animation into
	// an existing layer, like when loading limb animation.
	// TODO:We need a flag to tell us that this is a new layer,
	// so that we can configure these ranges
//	if(range_start_set && range_end_set){
//		Interval currrange = ctrl->GetTimeRange(TIMERANGE_ALL);
//		if(currrange.Start() != range.Start() ||
//			currrange.End() != range.End())
//			ctrl->EditTimeRange(range, 0);
//	}

	return TRUE;
}

/************************************************************************/
/* A pose is often saved and is the only itme in the group
/************************************************************************/
BOOL CATRigReader::ReadPoseGroup(Control *ctrl, TimeValue t, const float dScale, int flags)
{
	// Here we read a value which
	// is a base 'pose', the start of
	// this clip. While this
	// is not an animation we put it in
	// a loop so it will keep reading
	// till the group its in finishes
	// (otherwise, its endClause will end
	// this ReadController call)
	BOOL done = FALSE;
	do
	{
		switch (CurClauseID()) {
		case rigAssignment:
			if (CurIdentifier() == idValFloat ||
				CurIdentifier() == idValPoint ||
				CurIdentifier() == idValQuat ||
				CurIdentifier() == idValMatrix3) {
				ReadPoseIntoController(ctrl, t, dScale, flags);
			}
			break;
		case rigEndGroup:
			done = TRUE;
			break;
		}
	} while (!done && NextClause());
	return TRUE;
}

// this method is used by the CATRigReader to laod the current tag into the current controller
// ideally this method would not need to exist
BOOL CATRigReader::ReadPoseIntoController(Control *ctrl, TimeValue t, const float dScale, int flags)
{
	UNREFERENCED_PARAMETER(dScale);
	BOOL createkeys = FALSE;
	if (flags&CLIPFLAG_CLIP) {
		// Only add a key if we already have keys and we are loading a clip value
		// When loading onto an empty track, we place a key at the start. This is for TCB controllers
		// That need to be initialised witha pose value before the 1st key.
		createkeys = TRUE;//ctrl->IsAnimated();
		// We want to avoid putting a timewarp keyframe at the start of a timewarp curve
		if (createkeys && !(flags&CLIPFLAG_TIMEWARP_KEYFRAMES)) {

			// make sure keys are created
			DisableRefMsgs();

			// If this is the only value, or if this is simply a pose for a controller, then
			// We don't want keyframes getting created on time 0
			// If we create the keyframe first, then we will only get one keyframe
			int createkeyflags = 0;
			if (!(flags&CLIPFLAG_SKIPPOS))	createkeyflags |= COPYKEY_POS;
			if (!(flags&CLIPFLAG_SKIPROT))	createkeyflags |= COPYKEY_ROT;
			if (!(flags&CLIPFLAG_SKIPSCL))	createkeyflags |= COPYKEY_SCALE;
			ctrl->CopyKeysFromTime(t, t, createkeyflags);

			SuspendAnimate();
			AnimateOn();
		}
	}

	BOOL ok = TRUE;

	switch (CurIdentifier()) {
	case idValFloat: {
		float val;
		if (GetValuePose(flags, ctrl->SuperClassID(), (void*)&val))
			ctrl->SetValue(t, (void*)&val);
		else ok = FALSE;
		break;
	}
	case idValPoint: {
		Point3 val;
		if (GetValuePose(flags, ctrl->SuperClassID(), (void*)&val)) {
			if (ctrl->SuperClassID() == CTRL_POINT3_CLASS_ID || (ctrl->SuperClassID() == CTRL_POSITION_CLASS_ID && !(flags&CLIPFLAG_SKIPPOS)))
				ctrl->SetValue(t, (void*)&val);
			if (ctrl->SuperClassID() == CTRL_SCALE_CLASS_ID && !(flags&CLIPFLAG_SKIPSCL)) {
				ScaleValue scaleVal(val, Quat(1));
				ctrl->SetValue(t, (void*)&scaleVal);
			}
		}
		else ok = FALSE;
		break;
	}
	case idValQuat: {
		Quat val;
		if (!(flags&CLIPFLAG_SKIPROT) && GetValuePose(flags, ctrl->SuperClassID(), (void*)&val))
		{
			// are we loading an old file, onto a PRS, that used to be a rotation.
			// instead load the pose into the Matrix controller using the correct setvalue
			if (GetVersion() < CAT_VERSION_1700 &&
				ctrl->SuperClassID() == CTRL_MATRIX3_CLASS_ID)
			{
				if (ctrl->GetRotationController())
					ctrl->GetRotationController()->SetValue(t, (void*)&val);
			}
			else ctrl->SetValue(t, (void*)&val);
		}
		else ok = FALSE;
		break;
	}
	case idValMatrix3: {
		Matrix3 val;
		if (GetValuePose(flags, ctrl->SuperClassID(), (void*)&val)) {
			Matrix3 tm(1);
			Interval iv = FOREVER;
			ctrl->GetValue(t, (void*)&tm, iv, CTRL_RELATIVE);
			// we may be loading a pose into a bone that would
			// like to keep some its current channels
			if (flags&CLIPFLAG_SKIPPOS)
				val.SetTrans(tm.GetTrans());
			if (flags&CLIPFLAG_SKIPROT)
				val.SetRotate(Quat(tm));
			if (flags&CLIPFLAG_SKIPSCL) {
				tm.SetRotate(Quat(val)); tm.SetTrans(val.GetTrans()); val = tm;
			}

			if (ctrl->SuperClassID() == CTRL_ROTATION_CLASS_ID) {
				AffineParts parts;
				decomp_affine(val, &parts);
				ctrl->SetValue(t, (void*)&parts.q, 1, CTRL_ABSOLUTE);
			}
			else {
				SetXFormPacket xformset(val);
				ctrl->SetValue(t, (void*)&xformset);
			}
		}
		else ok = FALSE;
		break;
	}
	}

	if (createkeys && !(flags&CLIPFLAG_TIMEWARP_KEYFRAMES)) {
		// make sure keys are created
		AnimateOff();
		ResumeAnimate();
		EnableRefMsgs();
	}

	return ok;
}

void CATRigReader::TransformMatrix(Matrix3 &tm, const float dScale, int flags)
{
	// Mirror first then apply transforms
	if (flags&CLIPFLAG_MIRROR) {

		if (flags&CLIPFLAG_WORLDSPACE)
		{
			if (!(flags&CLIPFLAG_MIRROR_WORLD_X) && !(flags&CLIPFLAG_MIRROR_WORLD_Y))
				tm = tm * Inverse(tmFilePathNodeGuess);

			if (flags&CLIPFLAG_MIRROR_WORLD_X)
			{
				// World Y Axis Plane
				MirrorMatrix(tm, kYAxis);
				// For Z axis, flip X & Z
				if (catparenttrans->GetLengthAxis() == Z)
					tm.PreScale(Point3(-1, -1, 1));
			}
			else if (flags&CLIPFLAG_MIRROR_WORLD_Y)
			{
				// World X Axis Plane
				MirrorMatrix(tm, kXAxis);
				if (catparenttrans->GetLengthAxis() == X)
					tm.PreScale(Point3(-1, 1, -1));
			}

			else if (catparenttrans->GetLengthAxis() == Z)//Character symmetry plane
				MirrorMatrix(tm, kXAxis);
			else MirrorMatrix(tm, kZAxis);

			// put it back where you found it
			if (!(flags&CLIPFLAG_MIRROR_WORLD_X) && !(flags&CLIPFLAG_MIRROR_WORLD_Y))
				tm = tm * tmFilePathNodeGuess;

		}
		else {
			// Non-World space bones are always mirrored according to the bones mirror axis
			if (catparenttrans->GetLengthAxis() == Z)
				MirrorMatrix(tm, kXAxis);
			else MirrorMatrix(tm, kZAxis);
		}
	}

	// Scale data first.
	if (flags&CLIPFLAG_SCALE_DATA) (tm).SetTrans((tm).GetTrans() * dScale);

	// apply the offset, it has already been scaled in CATParent::LoadClip
	if (flags&CLIPFLAG_APPLYTRANSFORMS) {
		if (flags&CLIPFLAG_WORLDSPACE)
			tm = tm * tmTransform;
		//	else{
		//		if(layers){
		//			CATNodeControl* catnodecontrol = layers->GetCATNodeControl();
		//			if(catnodecontrol){
		//				Matrix3 tmParent = catnodecontrol->GettmBoneParent();
		//				tm = ((tm * tmParent) * tmTransform) * Inverse(tmParent);
		//			}
		//		}
		//	}

	}
}

void CATRigReader::TransformPoint(Point3 &p, const float dScale, int flags)
{
	/*	if(flags&CLIPFLAG_MIRROR && superClassID == CTRL_POINT3_CLASS_ID && flags&CLIPFLAG_ROT){
			if(catparenttrans->GetLengthAxis()==Z)
				 MirrorPoint( p, X, TRUE);
			else MirrorPoint( p, Z, TRUE);
		}
		else{
	*/
	Matrix3 tm(1);
	tm.SetTrans(p);
	TransformMatrix(tm, dScale, flags);
	p = tm.GetTrans();
	//	}
}

// this method now reads the value for the current pose and returns it
// this means that our CATRig can call this method and set the pose value directly into the NodeTM
// one day all the pose values will be managed this way and
//BOOL CATRigReader::GetValuePose(Control *ctrl, TimeValue t, int flags)//, const float dScale, superClassID)
BOOL CATRigReader::GetValuePose(int flags, SClass_ID superClassID, void* val)
{
	switch (CurIdentifier())
	{
	case idValFloat:
	{
		if (superClassID != CTRL_FLOAT_CLASS_ID) return FALSE;
		GetValue(val, tokFloat);

		if (flags&CLIPFLAG_MIRROR)
			*(float*)val *= -1.0f;

		if ((flags&CLIPFLAG_CLIP) && (flags&CLIPFLAG_SCALE_DATA))
			*(float*)val *= dScale;
		break;
	}
	case idValPoint:
	{
		GetValue(val, tokPoint);

		// Scale values need no processing
		if (superClassID == CTRL_SCALE_CLASS_ID) break;
		if (superClassID != CTRL_POINT3_CLASS_ID && superClassID != CTRL_POSITION_CLASS_ID) return FALSE;

		TransformPoint(*(Point3*)val, dScale, flags);

		/*			// Mirror first then apply transforms
					if(flags&CLIPFLAG_MIRROR){

						if(flags&CLIPFLAG_WORLDSPACE){

							if(!(flags&CLIPFLAG_MIRROR_WORLD_X) && !(flags&CLIPFLAG_MIRROR_WORLD_Y))
								*(Point3*)val = *(Point3*)val * Inverse(tmFilePathNodeGuess);

						//	if(flags&CLIPFLAG_MIRROR_WORLD_X)
						//		 (*(Point3*)val) = (*(Point3*)val) * Point3(1.0f, -1.0f ,1.0f);
						//	else (*(Point3*)val) = (*(Point3*)val) * Point3(-1.0f, 1.0f ,1.0f);

							if(flags&CLIPFLAG_MIRROR_WORLD_X)
								 MirrorPoint(*(Point3*)val, Y, FALSE);
							else if(flags&CLIPFLAG_MIRROR_WORLD_Y)
								 MirrorPoint(*(Point3*)val, X, FALSE);
							else if(catparenttrans->GetLengthAxis()==Z)//Character symmetry plane
								 MirrorPoint(*(Point3*)val, X, FALSE);
							else MirrorPoint(*(Point3*)val, Z, FALSE);

							if(flags&CLIPFLAG_SCALE_DATA)
								*(Point3*)val *= dScale;

							// put it back where you found it
							if(!(flags&CLIPFLAG_MIRROR_WORLD_X) && !(flags&CLIPFLAG_MIRROR_WORLD_Y))
								*(Point3*)val = *(Point3*)val * tmFilePathNodeGuess;
						}
						// if we are not a world space matrix, then we need to mirror always using the X Axis
						// this is because we are mirrioring due to our parent space which is always X=sideways
						else{
							if((superClassID == CTRL_POINT3_CLASS_ID) && flags&CLIPFLAG_ROT){
								if(catparenttrans->GetLengthAxis()==Z)//Character symmetry plane
									MirrorPoint(*(Point3*)val, X, TRUE);
								else MirrorPoint(*(Point3*)val, Z, TRUE);
							}else{
								if(catparenttrans->GetLengthAxis()==Z)//Character symmetry plane
									MirrorPoint(*(Point3*)val, X, FALSE);
								else MirrorPoint(*(Point3*)val, Z, FALSE);
							}
						}
					}else{
						if(flags&CLIPFLAG_SCALE_DATA)
							*(Point3*)val *= dScale;
					}
		*/			break;
	}
	case idValQuat:
	{
		GetValue(val, tokQuat);
		Matrix3 tm(1);
		(*(Quat*)val).MakeMatrix(tm);

		TransformMatrix(tm, dScale, flags);
		*(Quat*)val = Quat(tm);
		break;
	}
	// We no longer store these matricies in clip files in 1.7 onwards
	// We now always split a pose into P R and S components so that we
	// can load/(not load) them separately
	case idValMatrix3:
	{
		if (superClassID != CTRL_MATRIX3_CLASS_ID) return FALSE;
		GetValue(val, tokMatrix);
		TransformMatrix(*(Matrix3*)val, dScale, flags);
		break;
	}
	}
	return TRUE;
}

//**********************************************************************//
//	We are loding keys without interpolation data						//
//	Really only the matrix version is used, and we only use them		//
//	so that we can Mirror/Transform the clip in space					//
//	One day I'd like to use the ChangeParents system as a better way	//
//	Transfroming data													//
//**********************************************************************//
BOOL CATRigReader::ReadSimpleKeys(Control *ctrl, Interval timerange, const float dScale, int flags)
{
	//////////////////////////////////////////////////////////////////////////
	// Keys store offset time
	// from start of clip section.
	// Add on new start time
	TimeValue t = timerange.Start();

	//////////////////////////////////////////////////////////////////////////
	// Delete keys in the time being written too...
	Tab <TimeValue> oldKeyTimes;
	//oldKeyTimes.Resize(ctrl->NumKeys()); This will hopefully not be needed
	ctrl->GetKeyTimes(oldKeyTimes, timerange, 0);
	for (int i = 0; i < oldKeyTimes.Count(); i++)
		ctrl->DeleteKeyAtTime(oldKeyTimes[i]);
	//////////////////////////////////////////////////////////////////////////

	// make sure keys are created
	DisableRefMsgs();
	SuspendAnimate();
	AnimateOn();
	// The CopyKeysFromTime method is a nice generic way of keying an entire hierarchy
	// AddNewKey, only works on Leaf controllers.
	int createkeyflags = COPYKEY_POS | COPYKEY_ROT | COPYKEY_SCALE;

	BOOL done = FALSE;
	while (!done && ok())
	{
		if (!NextClause()) {
			done = TRUE;
			break;
		}

		if (CurClauseID() == rigAssignment) {

			switch (CurIdentifier()) {
				//////////////////////////////////////////////////////////////////////////
				// CATClipKey keys
				//
			case idValKeyFloat: {
				if (ctrl->SuperClassID() != CTRL_FLOAT_CLASS_ID) {
					AssertOutOfPlace();
					SkipGroup();
					break;
				}
				CATClipKeyFloat	key;
				GetValue((void*)&key, tokKeyFloat);
				key.time += t;
				if (flags&CLIPFLAG_TIMEWARP_KEYFRAMES)	key.val += t;
				if (flags&CLIPFLAG_SCALE_DATA)			key.val *= dScale;
				ctrl->CopyKeysFromTime(key.time, key.time, createkeyflags);
				ctrl->SetValue(key.time, (void*)&(key.val));
				break;
			}
			case idValKeyPoint: {
				if ((ctrl->SuperClassID() != CTRL_POSITION_CLASS_ID) ||
					(ctrl->SuperClassID() != CTRL_POINT3_CLASS_ID)) {
					AssertOutOfPlace();
					SkipGroup();
					break;
				}
				CATClipKeyPoint	key;
				GetValue((void*)&key, tokKeyPoint);
				key.time += t;

				TransformPoint(key.val, dScale, flags);

				/*	if(flags&CLIPFLAG_SCALE_DATA) key.val *= dScale;

					if(flags&CLIPFLAG_MIRROR){
						Point3 flipval;;
						if((ctrl->SuperClassID() == CTRL_POINT3_CLASS_ID) && flags&CLIPFLAG_ROT){
							if(catparenttrans->GetLengthAxis()==Z)
								 flipval = Point3(1.0f, -1.0f, -1.0f);
							else flipval = Point3(-1.0f, -1.0f, 1.0f);
						}else{
							if(catparenttrans->GetLengthAxis()==Z)
								 flipval = Point3(-1.0f, 1.0f, 1.0f);
							else flipval = Point3(1.0f, 1.0f, -1.0f);
						}
						key.val *= flipval;
					}
				*/	ctrl->CopyKeysFromTime(key.time, key.time, createkeyflags);
				ctrl->SetValue(key.time, (void*)&(key.val));
				break;
			}
									  // this is the only key that can use transforms
			case idValKeyMatrix: {
				CATClipKeyMatrix	key;
				GetValue((void*)&key, tokKeyMatrix);

				// Apply all the appropriate transfomr to the Matrix to prepare it for the controller
				TransformMatrix(key.val, dScale, flags);
				key.time += t;

				ctrl->CopyKeysFromTime(key.time, key.time, createkeyflags);

				if (ctrl->SuperClassID() == CTRL_POSITION_CLASS_ID) {
					ctrl->SetValue(key.time, (void*)&(key.val.GetTrans()));
				}
				else if (ctrl->SuperClassID() == CTRL_ROTATION_CLASS_ID) {
					Quat quatval(key.val);
					ctrl->SetValue(key.time, (void*)&quatval);
				}
				else if (ctrl->SuperClassID() == CTRL_MATRIX3_CLASS_ID) {
					SetXFormPacket xformsetval(key.val);
					ctrl->SetValue(key.time, (void*)&xformsetval);
				}

				else {
					AssertOutOfPlace();
					SkipGroup();
				}
				break;
			}
			default:
				AssertOutOfPlace();
				break;
			}
		}
		else if (CurClauseID() == rigEndGroup)
		{
			done = TRUE;
			break;
		}
	}
	AnimateOff();
	ResumeAnimate();
	EnableRefMsgs();
	return TRUE;
}

/************************************************************************/
/*    we have just begun a keys group so we are expecting key           */
/*    Identifiers coming up                                             */
/************************************************************************/
void AddKey(Control *, IKey *) {

}

BOOL CATRigReader::ReadKeys(Control *ctrl, Interval timerange, const float dScale, int flags)
{
	IKeyControl *ctrlInterface = GetKeyControlInterface(ctrl);
	if (!ctrlInterface) {
		// Post error message
		SkipGroup();
		return TRUE;
	}

	//////////////////////////////////////////////////////////////////////////
	// Keys store offset time
	// from start of clip section.
	// Add on new start time
	TimeValue t = timerange.Start();
	//////////////////////////////////////////////////////////////////////////

	DisableRefMsgs();

	//////////////////////////////////////////////////////////////////////////
	// Delete keys in the time being written too...
	Tab <TimeValue> oldKeyTimes;
	//oldKeyTimes.Resize(ctrl->NumKeys()); This will hopefully not be needed
	ctrl->GetKeyTimes(oldKeyTimes, timerange, 0);
	for (int i = 0; i < oldKeyTimes.Count(); i++)
		ctrl->DeleteKeyAtTime(oldKeyTimes[i]);
	//////////////////////////////////////////////////////////////////////////

	BOOL done = FALSE;
	while (!done && ok())
	{
		if (!NextClause()) {
			EnableRefMsgs();
			return FALSE;
		}

		if (CurClauseID() == rigAssignment) {
			// Out of Range types. We really need to be able to save these now because things
			// like the timewarp curves really need to have the correct ort setting....
			switch (CurIdentifier()) {

			case idValQuat: {
				// We want to start any TCB keyframe sequence with a pose value.
				// TCB keys are always relative to the previous pose, so this is ios importatnt.
				ReadPoseIntoController(ctrl, timerange.Start(), dScale, flags);
				break;
			}

			//////////////////////////////////////////////////////////////////////////
			// Float keys
			//
			// This maybe confusing BUT...
			// this float could be X position
			// on a controller. If so, it must
			// be scaled. So we trust in our
			// parent controllers to clear the CLIPFLAG_SCALE_DATA
			// flag if we arent actually a position

			case idValKeyBezFloat: {
				if (HYBRIDINTERP_FLOAT_CLASS_ID != ctrl->ClassID().PartA()) {
					AssertOutOfPlace();
					SkipGroup();
					break;
				}
				IBezFloatKey	key;
				GetValue((void*)&key, tokKeyBezFloat);
				key.time += t;
				if (flags&CLIPFLAG_TIMEWARP_KEYFRAMES)	key.val += t;

				if (flags&CLIPFLAG_SCALE_DATA) {
					key.val *= dScale;
					key.intan *= dScale;
					key.outtan *= dScale;
				}
				if (flags&CLIPFLAG_MIRROR) {
					key.val *= -1.0f;
					key.intan *= -1.0f;
					key.outtan *= -1.0f;
				}
				ctrlInterface->AppendKey((IKey*)&key);
				break;
			}
			case idValKeyTCBFloat: {
				if (TCBINTERP_FLOAT_CLASS_ID != ctrl->ClassID().PartA()) {
					AssertOutOfPlace();
					SkipGroup();
					break;
				}
				ITCBFloatKey	key;
				GetValue((void*)&key, tokKeyTCBFloat);
				key.time += t;
				if (flags&CLIPFLAG_TIMEWARP_KEYFRAMES)	key.val += t;
				if (flags&CLIPFLAG_SCALE_DATA)			key.val *= dScale;
				if (flags&CLIPFLAG_MIRROR)				key.val *= -1.0f;

				ctrlInterface->AppendKey((IKey*)&key);
				break;
			}
			case idValKeyLinFloat: {
				if (LININTERP_FLOAT_CLASS_ID != ctrl->ClassID().PartA()) {
					AssertOutOfPlace();
					SkipGroup();
					break;
				}
				ILinFloatKey key;
				GetValue((void*)&key, tokKeyLinFloat);
				key.time += t;
				if (flags&CLIPFLAG_TIMEWARP_KEYFRAMES)	key.val += t;
				if (flags&CLIPFLAG_SCALE_DATA)			key.val *= dScale;
				if (flags&CLIPFLAG_MIRROR)				key.val *= -1.0f;

				ctrlInterface->AppendKey((IKey*)&key);
				break;
			}
			//////////////////////////////////////////////////////////////////////////
			// Position Keys
			//
			case idValKeyBezXYZ: {
				if ((HYBRIDINTERP_POSITION_CLASS_ID != ctrl->ClassID().PartA()) &&
					(HYBRIDINTERP_POINT3_CLASS_ID != ctrl->ClassID().PartA())) {
					AssertOutOfPlace();
					SkipGroup();
					break;
				}

				IBezPoint3Key	key;
				GetValue((void*)&key, tokKeyBezXYZ);
				key.time += t;

				TransformPoint(key.val, dScale, flags);
				TransformPoint(key.intan, dScale, flags);
				TransformPoint(key.outtan, dScale, flags);

				ctrlInterface->AppendKey((IKey*)&key);
			}
			break;
			case idValKeyTCBXYZ: {
				if ((TCBINTERP_POSITION_CLASS_ID != ctrl->ClassID().PartA()) &&
					(TCBINTERP_POINT3_CLASS_ID != ctrl->ClassID().PartA())) {
					AssertOutOfPlace();
					SkipGroup();
					break;
				}
				ITCBPoint3Key	key;
				GetValue((void*)&key, tokKeyTCBXYZ);
				key.time += t;

				TransformPoint(key.val, dScale, flags);
				ctrlInterface->AppendKey((IKey*)&key);
				break;
			}
			case idValKeyLinXYZ: {
				if ((LININTERP_POSITION_CLASS_ID != ctrl->ClassID().PartA())) {
					AssertOutOfPlace();
					SkipGroup();
					break;
				}
				ILinPoint3Key	key;
				GetValue((void*)&key, tokKeyLinXYZ);
				key.time += t;

				TransformPoint(key.val, dScale, flags);
				ctrlInterface->AppendKey((IKey*)&key);
				break;
			}
			//////////////////////////////////////////////////////////////////////////
			// Rotation Keys
			//
			case idValKeyBezRot: {
				if (HYBRIDINTERP_ROTATION_CLASS_ID != ctrl->ClassID().PartA()) {
					AssertOutOfPlace();
					SkipGroup();
					break;
				}
				IBezQuatKey key;
				GetValue((void*)&key, tokKeyBezRot);
				key.time += t;
				if (flags&CLIPFLAG_MIRROR) {
					if (catparenttrans->GetLengthAxis() == Z)
						MirrorQuat(key.val, X);
					else MirrorQuat(key.val, Z);
				}
				ctrlInterface->AppendKey((IKey*)&key);
				break;
			}
			case idValKeyTCBRot: {
				if (TCBINTERP_ROTATION_CLASS_ID != ctrl->ClassID().PartA()) {
					AssertOutOfPlace();
					SkipGroup();
					break;
				}
				ITCBRotKey	key;
				GetValue((void*)&key, tokKeyTCBRot);
				key.time += t;
				if (flags&CLIPFLAG_MIRROR) {
					if (catparenttrans->GetLengthAxis() == Z)
						MirrorAngAxis(key.val, X);
					else MirrorAngAxis(key.val, Z);
				}
				ctrlInterface->AppendKey((IKey*)&key);
				break;
			}
			case idValKeyLinRot: {
				if (LININTERP_ROTATION_CLASS_ID != ctrl->ClassID().PartA()) {
					AssertOutOfPlace();
					SkipGroup();
					break;
				}
				ILinRotKey	key;
				GetValue((void*)&key, tokKeyLinRot);
				key.time += t;
				if (flags&CLIPFLAG_MIRROR) {
					if (catparenttrans->GetLengthAxis() == Z)
						MirrorQuat(key.val, X);
					else MirrorQuat(key.val, Z);
				}
				ctrlInterface->AppendKey((IKey*)&key);
				break;
			}
			//////////////////////////////////////////////////////////////////////////
			// Scale Keys
			//
			// We don't scale, mirror, or transfomr scale keys because it makes a mess
			case idValKeyBezScale: {
				if (HYBRIDINTERP_SCALE_CLASS_ID != ctrl->ClassID().PartA()) {
					AssertOutOfPlace();
					SkipGroup();
				}
				IBezScaleKey key;
				GetValue((void*)&key, tokKeyBezScale);
				key.time += t;
				ctrlInterface->AppendKey((IKey*)&key);
				break;
			}
			case idValKeyTCBScale: {
				if (TCBINTERP_SCALE_CLASS_ID != ctrl->ClassID().PartA()) {
					AssertOutOfPlace();
					SkipGroup();
				}
				ITCBScaleKey	key;
				GetValue((void*)&key, tokKeyTCBScale);
				key.time += t;
				ctrlInterface->AppendKey((IKey*)&key);
				break;
			}
			case idValKeyLinScale: {
				if (LININTERP_SCALE_CLASS_ID != ctrl->ClassID().PartA()) {
					AssertOutOfPlace();
					SkipGroup();
				}
				ILinScaleKey	key;
				GetValue((void*)&key, tokKeyLinScale);
				key.time += t;
				ctrlInterface->AppendKey((IKey*)&key);
				break;
			}
			default:
				AssertOutOfPlace();
				break;
			}
		}
		else if (CurClauseID() == rigEndGroup)
		{
			done = TRUE;
			break;
		}
	}

	ctrlInterface->SortKeys();
	EnableRefMsgs();

	// disabled 13-09-07 to try and speed up clip loading.
	// We are getting some massive load times and I am not sure this is required
	// re-enabled 15-10-07 If a controller believes it has no keyframes then it simply
	// returns a cached value.
	ctrl->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	return TRUE;
}

bool CanReparentOldType(Class_ID clid)
{
	return (clid == HUB_CLASS_ID ||
		clid == ARBBONETRANS_CLASS_ID ||
		clid == FOOTTRANS2_CLASS_ID ||
		clid == IKTARGTRANS_CLASS_ID);
	// Only the classes listed above had any effect from being re-parented in CAT v < 3500.
}

void CATRigReader::ResolveParentChilds()
{
	// Earlier versions of CAT basically ignored standard Max
	// inheritance.  Because of this, files saved with earlier versions
	// may have strange parenting.  This parenting has no effect in earlier
	// versions of CAT, but now its definitely creating a problem for me!
	// Easiest fix, disable re-parenting on older versions of CAT rig files.
	bool bPreventReparenting = GetVersion() < CAT3_VERSION_3500;

	for (int i = 0; i < tabParentChild.Count(); i++)
	{
		INode* pChild = tabParentChild[i]->childnode;
		if (pChild == NULL)
			continue;

		if (bPreventReparenting)
		{
			// If our controller is not one of the 'acceptable' types
			// then prevent it from being re-parented.
			Control * pChildCtrl = pChild->GetTMController();
			if (!CanReparentOldType(pChildCtrl->ClassID()))
			{
				// All others will be re-parented.
				continue; // <-- all others.
			}
		}

		INode *parentNode = catparenttrans->GetBoneByAddress(tabParentChild[i]->parent_address);
		if (parentNode != NULL)
		{
			// if we are not already the parent, then do the re-parenting.
			if (parentNode != pChild->GetParentNode())
			{
				parentNode->AttachChild(pChild, FALSE);
			}
		}
	}
};

/////////////////////////////////////////////////////////////////
// CATRigWriter functions
/////////////////////////////////////////////////////////////////

CATRigWriter::CATRigWriter()
	: strIndent(_T(""))
{
	this->catparenttrans = NULL;
	Init();
}

void CATRigWriter::Init()
{
	nIndentLevel = 0;

	// GB 24-Oct-03: Save current numeric locale and set to standard C.
	// This cures our number translation problems in central europe.
	strOldNumericLocale = ReplaceLocale(LC_NUMERIC, _T("C"));
}

CATRigWriter::CATRigWriter(const TCHAR *filename, ICATParentTrans* catparenttrans)
	: strIndent(_T(""))
{
	Init();
	this->catparenttrans = catparenttrans;

	//	Open(filename);
	this->filename = filename;
}

BOOL CATRigWriter::GetStreamBuffer(TSTR &str)
{
	outstream.seekg(0, std::ios_base::end);
	ULONG nFileSize = (ULONG)outstream.tellg();
	outstream.seekg(0, std::ios_base::beg);

	TCHAR c;
	int nCharsRead = 0;
	bool done = false;
	str.Resize(nFileSize);
	while (!done) {
		if (outstream.eof() || outstream.fail()) {
			done = true;
			continue;
		}
		outstream.get(c);
		//	str << c;
		str.dataForWrite()[nCharsRead] = c;
		++nCharsRead;
	}
	return TRUE;
}

CATRigWriter::~CATRigWriter() {
	//	if (outstream.is_open()) outstream.close();
	Close();
	RestoreLocale(LC_NUMERIC, strOldNumericLocale);
}

BOOL CATRigWriter::Open(const TCHAR *filename)
{
	UNREFERENCED_PARAMETER(filename);
	//	outstream.open(filename);
	//	return outstream.good();

	return TRUE;
}

void CATRigWriter::Close()
{
	if (filename.Length() <= 2) return;

	MaxSDK::Util::TextFile::Writer outfileWriter;
	Interface14 *iface = GetCOREInterface14();
	unsigned int encoding = 0;
	if (iface->LegacyFilesCanBeStoredUsingUTF8())
		encoding = CP_UTF8 | MaxSDK::Util::TextFile::Writer::WRITE_BOM;
	else
	{
		LANGID langID = iface->LanguageToUseForFileIO();
		encoding = iface->CodePageForLanguage(langID);
	}
	if (!outfileWriter.Open(filename.data(), false, encoding)) {
		GetCATMessages().Error(0, GetString(IDS_ERR_NOFILEWRITE), filename.data());
	}
	else {

		outstream.seekg(0, std::ios_base::beg);
		TCHAR c;
		bool done = false;
		while (!done) {
			if (outstream.eof() || outstream.fail()) {
				done = true;
				continue;
			}
			outstream.get(c);
			outfileWriter.WriteChar(c);
		}

		outfileWriter.Close();
	}
}

BOOL CATRigWriter::BeginGroup(USHORT id, void* val/*=NULL*/)
{
	DictionaryEntry* entry = catDictionary.GetEntry(id);
	DbgAssert(entry->type == tokGroup);

	if (!val || entry->expect == tokNothing)
		outstream << strIndent << entry->name << _T(" {") << std::endl;
	else {
		outstream << strIndent << entry->name << _T(' ');
		WriteVal(entry->expect, val);
		outstream << _T(" {") << std::endl;
	}

	Indent();

	return outstream.good();
}

BOOL CATRigWriter::EndGroup()
{
	Outdent();
	outstream << strIndent << _T("}") << std::endl;
	return outstream.good();
}

BOOL CATRigWriter::Comment(const TCHAR *msg, ...)
{
	TCHAR scratch[200] = { 0 };
	va_list args;
	va_start(args, msg);
	_stprintf(scratch, _T("# "));
	_vstprintf(&scratch[_tcslen(scratch)], msg, args);
	va_end(args);
	if (outstream.good()) outstream << strIndent << tstring(scratch) << std::endl;
	return outstream.good();
}

BOOL CATRigWriter::WritePoint3(Point3 p3)
{
	BOOL isGood = outstream.good();
	int i = 0;
	while (isGood && i < 3)
	{
		outstream << _T(' ') << p3[i];
		i++;
		isGood = outstream.good();
	}
	return isGood;
}

BOOL CATRigWriter::Write(USHORT id, void *data)
{
	DictionaryEntry* entry = catDictionary.GetEntry(id);
	DbgAssert(entry->type == tokIdentifier);

	outstream << strIndent << IdentName(id) << _T(" = ") << std::flush;

	DbgAssert(outstream.good());
	DbgAssert(!outstream.fail());

	BOOL ok = WriteVal(entry->expect, data);

	if (ok) outstream << std::endl;

	return ok;
}

BOOL CATRigWriter::WriteVal(CatTokenType toVal, void *data)
{
	switch (toVal) {
		//	switch(entry->expect) {
	case tokBool:	outstream << *(bool*)data ? IdentName(idTrue) : IdentName(idFalse); break;
	case tokInt:
		outstream << *(int*)data; break;
		break;
	case tokULONG:
		outstream << *(int*)data; break;
		break;
	case tokFloat:
		outstream << *(float*)data; break;
		break;
	case tokString:
		outstream << _T('"') << (TCHAR*)data << _T('"');
		break;
	case tokPoint: {
		outstream << IdentName(idPoint);
		WritePoint3(*(Point3*)data);
		break;
	}
	case tokQuat: {
		Quat q = *(Quat*)data;
		outstream << IdentName(idQuat) << _T(" ") << q.x << _T(" ") << q.y << _T(" ") << q.z << _T(" ") << q.w;
		break;
	}
	case tokAngAxis: {
		AngAxis a = *(AngAxis*)data;
		outstream << IdentName(idAngAxis) << _T(" ") << a.axis.x << _T(" ") << a.axis.y << _T(" ") << a.axis.z << _T(" ") << a.angle;
		break;
	}
	case tokMatrix: {
		Matrix3 m = *(Matrix3*)data;
		outstream << IdentName(idMatrix);
		for (int i = 0; i < 4; i++) {
			WritePoint3(m.GetRow(i));
		}
		break;
	}
	// Controller Keys
	case tokKeyBezXYZ: {
		IBezPoint3Key m = *(IBezPoint3Key*)data;
		outstream << IdentName(idKeyBezXYZ);
		outstream << _T(' ') << m.flags;
		outstream << _T(' ') << m.time;
		WritePoint3(m.val);
		WritePoint3(m.intan);
		WritePoint3(m.outtan);
		WritePoint3(m.inLength);
		WritePoint3(m.outLength);
		break;
	}
	case tokKeyBezRot: {
		IBezQuatKey m = *(IBezQuatKey*)data;
		outstream << IdentName(idKeyBezRot);
		outstream << _T(' ') << m.flags;
		outstream << _T(' ') << m.time;
		Quat q = m.val;
		Point3 axis(q.x, q.y, q.z);
		outstream << _T(' ') << q.w;
		WritePoint3(axis);
		break;
	}
	case tokKeyBezScale: {
		IBezScaleKey m = *(IBezScaleKey*)data;
		outstream << IdentName(idKeyBezScale);
		outstream << _T(' ') << m.flags;
		outstream << _T(' ') << m.time;
		WritePoint3(m.val.s);
		Quat q = m.val.q;
		Point3 axis(q.x, q.y, q.z);
		outstream << _T(' ') << q.w;
		WritePoint3(axis);

		WritePoint3(m.intan);
		WritePoint3(m.outtan);
		WritePoint3(m.inLength);
		WritePoint3(m.outLength);
		break;
	}

	case tokKeyBezFloat: {
		IBezFloatKey m = *(IBezFloatKey*)data;
		outstream << IdentName(idKeyBezFloat);
		outstream << _T(' ') << m.flags;
		outstream << _T(' ') << m.time;
		outstream << _T(' ') << m.val;
		outstream << _T(' ') << m.intan;
		outstream << _T(' ') << m.outtan;
		outstream << _T(' ') << m.inLength;
		outstream << _T(' ') << m.outLength;
		break;
	}
	// TCB
	case tokKeyTCBXYZ: {
		ITCBPoint3Key m = *(ITCBPoint3Key*)data;
		outstream << IdentName(idKeyTCBXYZ);
		outstream << _T(' ') << m.flags;
		outstream << _T(' ') << m.time;
		WritePoint3(m.val);
		outstream << _T(' ') << m.tens;
		outstream << _T(' ') << m.cont;
		outstream << _T(' ') << m.bias;
		outstream << _T(' ') << m.easeIn;
		outstream << _T(' ') << m.easeOut;
		break;
	}
	case tokKeyTCBRot: {
		ITCBRotKey m = *(ITCBRotKey*)data;
		AngAxis ax = m.val;
		outstream << IdentName(idKeyTCBRot);
		outstream << _T(' ') << m.flags;
		outstream << _T(' ') << m.time;

		outstream << _T(' ') << ax.angle;
		WritePoint3(ax.axis);

		outstream << _T(' ') << m.tens;
		outstream << _T(' ') << m.cont;
		outstream << _T(' ') << m.bias;
		outstream << _T(' ') << m.easeIn;
		outstream << _T(' ') << m.easeOut;
		break;
	}
	case tokKeyTCBScale: {
		ITCBScaleKey m = *(ITCBScaleKey*)data;
		outstream << IdentName(idKeyTCBScale);
		outstream << _T(' ') << m.flags;
		outstream << _T(' ') << m.time;

		WritePoint3(m.val.s);
		Quat q = m.val.q;
		Point3 axis(q.x, q.y, q.z);
		outstream << _T(' ') << q.w;
		WritePoint3(axis);

		outstream << _T(' ') << m.tens;
		outstream << _T(' ') << m.cont;
		outstream << _T(' ') << m.bias;
		outstream << _T(' ') << m.easeIn;
		outstream << _T(' ') << m.easeOut;
		break;
	}
	case tokKeyTCBFloat: {
		ITCBFloatKey m = *(ITCBFloatKey*)data;
		outstream << IdentName(idKeyTCBFloat);
		outstream << _T(' ') << m.flags;
		outstream << _T(' ') << m.time;
		outstream << _T(' ') << m.val;
		outstream << _T(' ') << m.tens;
		outstream << _T(' ') << m.cont;
		outstream << _T(' ') << m.bias;
		outstream << _T(' ') << m.easeIn;
		outstream << _T(' ') << m.easeOut;
		break;
	}

	case tokKeyLinXYZ: {
		ILinPoint3Key m = *(ILinPoint3Key*)data;
		outstream << IdentName(idKeyLinXYZ);
		outstream << _T(' ') << m.flags;
		outstream << _T(' ') << m.time;
		WritePoint3(m.val);
		break;
	}
	case tokKeyLinRot: {
		ILinRotKey m = *(ILinRotKey*)data;
		outstream << IdentName(idKeyLinRot);
		outstream << _T(' ') << m.flags;
		outstream << _T(' ') << m.time;
		Quat q = m.val;
		Point3 axis(q.x, q.y, q.z);
		outstream << _T(' ') << q.w;
		WritePoint3(axis);
		break;
	}
	case tokKeyLinFloat: {
		ILinFloatKey m = *(ILinFloatKey*)data;
		outstream << IdentName(idKeyLinFloat);
		outstream << _T(' ') << m.flags;
		outstream << _T(' ') << m.time;
		outstream << _T(' ') << m.val;
		break;
	}

	case tokClassIDs: {
		TokClassIDs m = *(TokClassIDs*)data;
		outstream << IdentName(idClassIDs);
		outstream << _T(' ') << m.usSuperClassID;
		outstream << _T(' ') << m.usClassIDA;
		outstream << _T(' ') << m.usClassIDB << _T(' ');

		// PT 16/02/04 Added this so we don't need extra tags
		// really can't get away for the subindex tags
		//	outstream << _T(' ') << m.subindex << _T(' ');
		WriteVal(tokString, (void*)m.classname.data());
		break;
	}

	case tokInterval: {
		Interval m = *(Interval*)data;
		outstream << IdentName(idInterval);
		outstream << _T(' ') << m.Start() << _T(' ') << m.End();
		break;
	}

	// these are Keyframes with timevalues attached
	// a simple keyframe with no interpollation settings
	case tokKeyFloat: {
		CATClipKeyFloat m = *(CATClipKeyFloat*)data;
		outstream << IdentName(idKeyFloat);
		outstream << _T(' ') << m.flags;
		outstream << _T(' ') << m.time;
		outstream << _T(' ') << m.val;
		break;
	}
	case tokKeyPoint: {
		CATClipKeyPoint m = *(CATClipKeyPoint*)data;
		outstream << IdentName(idKeyPoint);
		outstream << _T(' ') << m.flags;
		outstream << _T(' ') << m.time;
		WritePoint3(m.val);
		break;
	}
	case tokKeyMatrix: {
		CATClipKeyMatrix m = *(CATClipKeyMatrix*)data;
		outstream << IdentName(idKeyMatrix);
		outstream << _T(' ') << m.flags;
		outstream << _T(' ') << m.time;
		for (int i = 0; i < 4; i++)
			WritePoint3(m.val.GetRow(i));
		break;
	}

	case tokLayerInfo: {
		LayerInfo layer = *(LayerInfo*)data;
		outstream << IdentName(idLayerInfo);
		outstream << _T(' ') << layer.dwFlags;
		outstream << _T(' ') << layer.dwLayerColour << _T(' ');
		WriteVal(tokString, (void*)layer.strName.data());
		break;
	}

	case tokFace: {
		Face fc = *(Face*)data;
		outstream << IdentName(idFace);
		outstream << _T(' ') << fc.v[0] << _T(' ') << fc.v[1] << _T(' ') << fc.v[2];
		outstream << _T(' ') << fc.smGroup;
		outstream << _T(' ') << fc.flags;
		break;
	}
	case tokINode: {
		INode* node = (INode*)data;
		if (!node || node->IsRootNode()) {
			outstream << IdentName(idINode);
			TSTR address = IdentName(idSceneRootNode); //catparenttrans->GetBoneAddress();
			outstream << _T(' ');
			WriteVal(tokString, (void*)address.data());
			break;
		}
		if (node == catparenttrans->GetNode()) {
			outstream << IdentName(idINode);
			TSTR address = catparenttrans->GetBoneAddress();
			outstream << _T(' ');
			WriteVal(tokString, (void*)address.data());
		}
		CATNodeControl* catnodecontrol = (CATNodeControl*)node->GetTMController()->GetInterface(I_CATNODECONTROL);
		if (catnodecontrol && catnodecontrol->GetCATParentTrans() == catparenttrans) {
			outstream << IdentName(idINode);
			TSTR address = catnodecontrol->GetBoneAddress();
			outstream << _T(' ');
			WriteVal(tokString, (void*)address.data());
		}
		break;
	}

	default:
		// What is this???
		DbgAssert(FALSE);
	}

	return outstream.good();
}

BOOL CATRigWriter::WriteNode(INode* node) {
	CATNodeControl* catnodecontrol = (CATNodeControl*)node->GetTMController()->GetInterface(I_CATNODECONTROL);
	if (catnodecontrol || node == catparenttrans->GetNode())
		return Write(idNode, node);
	else {
		TSTR nodename(node->GetName());
		return Write(idNodeName, nodename);
	}
}

INode* CATRigReader::GetINode() {
	INode* node = NULL;
	switch (CurIdentifier()) {
	case idNode:
		GetValue((void*)&node, tokINode);
		break;
	case idNodeName: {
		TSTR nodename;
		GetValue(nodename);
		node = GetCOREInterface()->GetINodeByName(nodename);
		break;
	}
	}
	return node;
}

BOOL CATRigWriter::FromParamBlock(IParamBlock2* pblock, USHORT id, ParamID paramID)
{
	DictionaryEntry* entry = catDictionary.GetEntry(id);
	DbgAssert(entry->type == tokIdentifier);

	Interval valid = FOREVER;

	switch (entry->expect) {
	case tokBool: {
		bool val;
		float fval;
		pblock->GetValue(paramID, 0, fval, valid);
		val = fval != 0.0f ? true : false;
		Write(id, (void*)&val);
		break;
	}
	case tokInt:
	case tokULONG: {
		int val;
		pblock->GetValue(paramID, 0, val, valid);
		Write(id, (void*)&val);
		break;
	}
	case tokFloat: {
		float val;
		pblock->GetValue(paramID, 0, val, valid);
		Write(id, (void*)&val);
		break;
	}
	case tokString: {
		const MCHAR* val = NULL;
		pblock->GetValue(paramID, 0, val, valid);
		Write(id, (void*)val);
		break;
	}
	case tokPoint: {
		Point3 val;
		pblock->GetValue(paramID, 0, val, valid);
		Write(id, (void*)&val);
		break;
	}

	case tokQuat:
	case tokAngAxis:
		// Param blocks don't store Quat or AngAxis values.
		DbgAssert(FALSE);
		break;

	case tokMatrix: {
		Matrix3 val;
		pblock->GetValue(paramID, 0, val, valid);
		Write(id, (void*)&val);
		break;
	}

	default:
		// What is this???
		DbgAssert(FALSE);
	}

	return outstream.good();
}

BOOL CATRigWriter::FromController(Control* ctrl, USHORT id)
{
	DictionaryEntry* entry = catDictionary.GetEntry(id);
	DbgAssert(entry->type == tokIdentifier);

	Interval valid = FOREVER;

	switch (entry->expect) {
	case tokBool: {
		bool val;
		float fval;
		ctrl->GetValue(0, (void*)&fval, valid, CTRL_ABSOLUTE);
		val = fval != 0.0f ? true : false;
		Write(id, (void*)&val);
		break;
	}
	case tokULONG:
	case tokInt: {
		int val;
		float fval;
		ctrl->GetValue(0, (void*)&fval, valid, CTRL_ABSOLUTE);
		val = (int)fval;
		Write(id, (void*)&val);
		break;
	}
	case tokFloat: {
		float val;
		ctrl->GetValue(0, (void*)&val, valid, CTRL_ABSOLUTE);
		Write(id, (void*)&val);
		break;
	}
	case tokPoint: {
		float val;
		ctrl->GetValue(0, (void*)&val, valid, CTRL_ABSOLUTE);
		Write(id, (void*)&val);
		break;
	}
	case tokQuat: {
		Quat val;
		ctrl->GetValue(0, (void*)&val, valid, CTRL_ABSOLUTE);
		Write(id, (void*)&val);
		break;
	}
	case tokAngAxis: {
		Quat qval;
		AngAxis val;
		ctrl->GetValue(0, (void*)&qval, valid, CTRL_ABSOLUTE);
		val = AngAxis(qval);
		Write(id, (void*)&val);
		break;
	}
	case tokMatrix: {
		Matrix3 val(1);
		ctrl->GetValue(0, (void*)&val, valid, CTRL_RELATIVE);
		Write(id, (void*)&val);
		break;
	}
	default:
		DbgAssert(FALSE);
		break;
	}

	return outstream.good();
}

/************************************************************************/
/* This is a recursive function that traverses a controller hierarchy	*/
/* until it finds a leaf, then calls the appropriate save function on   */
/* that leaf controller                                                 */
/************************************************************************/
BOOL CATRigWriter::WriteController(Control* ctrl, int flags, Interval timerange)
{
	// Set the TopLevel Flag.
	flags |= CLIPFLAG_TOPLEVEL;
	return WriteControllerHierarchy(ctrl, flags, timerange);
}

/************************************************************************/
/* This is a recursive function that traverses a controller hierarchy	*/
/* until it finds a leaf, then calls the appropriate save function on   */
/* that leaf controller                                                 */
/************************************************************************/
BOOL CATRigWriter::WriteControllerHierarchy(Control* ctrl, int flags, Interval timerange)
{
	// First, check to see that this is a controller
	if (!ctrl || !GetControlInterface(ctrl)) return FALSE;
	if (ctrl->GetInterface(I_IKCONTROL)) {
		return FALSE;
	}

	BOOL ok = TRUE;
	BOOL isAnimated = ctrl->IsAnimated();

	// Spit out a description of this controller
	// including the class IDs so that we regenerate
	// the controller when loading the file back in
	TSTR strClassName;
	ULONG superclassID = ctrl->SuperClassID();
	ctrl->GetClassName(strClassName);

	// if we are going to start saving out keyframes, we need to save out the type of controller too.
	TokClassIDs classid(superclassID, ctrl->ClassID().PartA(), ctrl->ClassID().PartB(), strClassName);
	Write(idValClassIDs, (void*)&classid);

	// If we are a CATMotion hierarchy, ensure we save the welded state.
	CATHierarchyLeaf* ctrlAsLeaf = dynamic_cast<CATHierarchyLeaf*>(ctrl);
	if (ctrlAsLeaf != NULL)
	{
		CATHierarchyLeaf* pLeaf = static_cast<CATHierarchyLeaf*>(ctrl);
		BOOL welded = pLeaf->GetLeafParent()->GetisWelded();
		Write(idWeldedLeaves, (void*)&welded);
	}

	// If we are a leaf control, and we aren't animated, then save
	// our value and return.
	if (!(flags&CLIPFLAG_SKIP_KEYFRAMES))
	{
		if (ctrl->IsLeaf() && !isAnimated)
		{
			WritePose(timerange.Start(), ctrl);
			return ok;
		}
	}

	if (ctrl->SuperClassID() == CTRL_MATRIX3_CLASS_ID)
	{
		if (ctrl->ClassID() == Class_ID(PRS_CONTROL_CLASS_ID, 0)) {
			// Save out the inheritance flags information on the PRS controllers
			DWORD inheritflags = ctrl->GetInheritanceFlags();
			Write(idFlags, (void*)&inheritflags);

			if (inheritflags&INHERIT_ALL) {
				// Internally in the PRS controller is a matrix called inheritOffsetTM.
				// we cannot see this matrix, but if the inhetritance flags have been customised,
				// them this matrix effects all the motion generated by the PRS controller.
				// Here we are trying to guess the effect of the inheritOffsetTM by removing the effect of each
				// Channel(P, R, & S).
				DisableRefMsgs();
				Control *tempprs = (Control*)CloneRefHierarchy(ctrl);
				// Clear pos
				if (tempprs->GetPositionController()) {
					tempprs->GetPositionController()->DeleteKeys(TRACK_DOALL);
					Point3 p(0.0f, 0.0f, 0.0f);
					tempprs->GetPositionController()->SetValue(timerange.Start(), (void*)&p);
				}
				// Clear Rot
				if (tempprs->GetRotationController()) {
					tempprs->GetRotationController()->DeleteKeys(TRACK_DOALL);
					Quat q; q.Identity();
					tempprs->GetRotationController()->SetValue(timerange.Start(), (void*)&q);
				}
				// clear Scl
				if (tempprs->GetScaleController()) {
					tempprs->GetScaleController()->DeleteKeys(TRACK_DOALL);
					Point3 s(1.0f, 1.0f, 1.0f);
					tempprs->GetScaleController()->SetValue(timerange.Start(), (void*)&s);
				}

				// all that should be left in this controler is the inheritOffsetTM
				Matrix3 tmOffset(1);
				Interval iv = FOREVER;
				tempprs->GetValue(timerange.Start(), (void*)&tmOffset, iv, CTRL_RELATIVE);
				Write(idOffsetTM, (void*)&tmOffset);

				tempprs->DeleteThis();
				EnableRefMsgs();
			}
		}

		// if we have just entered
		if (flags&CLIPFLAG_TOPLEVEL && !(flags&CLIPFLAG_SKIP_KEYFRAMES)) {
			// before we start diving further into the hierarchy lest save out simple keys.
			// these controllers
			WriteSimpleKeys(timerange, ctrl);
			// now clear the flag so we don't do this again
			flags &= ~CLIPFLAG_TOPLEVEL;
		}

		if (ctrl->GetInterface(LINK_CONSTRAINT_INTERFACE)) {/// && !(flags&CLIPFLAG_SKIP_KEYFRAMES)){
			WriteLinkConst((ILinkCtrl*)ctrl->GetInterface(LINK_CONSTRAINT_INTERFACE), flags, timerange);
		}
	}
	//////////////////////////////////////////////////////////////////////////
	if (ctrl->SuperClassID() == CTRL_ROTATION_CLASS_ID)
	{
		if (ctrl->ClassID().PartA() == EULER_CONTROL_CLASS_ID)
		{
			IEulerControl *eulerctrl = (IEulerControl*)ctrl->GetInterface(I_EULERCTRL);
			if (eulerctrl)
			{
				int order = eulerctrl->GetOrder();
				Write(idAxisOrder, (void*)&order);
			}
		}
	}
	if (GetILookAtConstInterface(ctrl) || GetIOrientConstInterface(ctrl) || GetIPosConstInterface(ctrl) || GetIPathConstInterface(ctrl) ||
		ctrl->ClassID().PartA() == SURF_CONTROL_CLASSID.PartA() ||
		ctrl->ClassID().PartA() == ATTACH_CONTROL_CLASS_ID.PartA()) {
		return WriteConstraint(ctrl, flags, timerange);
	}
	//////////////////////////////////////////////////////////////////////////
	if (!ctrl->IsLeaf())
	{
		// MAXX-
		// We have changed the sub-anim structure of our controllers.  In order
		// to save clips that can be loaded in previous versions of CAT, we
		// continue to save out our subanims from CATHierarchyLeaves in
		// the old sub order.
		int numSubs = ctrl->NumSubs();
		if (ctrlAsLeaf != NULL)
			numSubs = ctrlAsLeaf->GetNumLayers();

		Animatable* sub;
		for (int i = 0; (i < numSubs) && ok; i++)
		{
			sub = NULL;
			if (ctrlAsLeaf != NULL)
				sub = ctrlAsLeaf->GetLayer(i);
			else
				sub = ctrl->SubAnim(i);

			// make sure we have a sub and it is a controller
			if (sub && ok)
			{
				// Saving out the subanims index,
				// so the loader doesnt have to
				// guess as to which subindex it was
				Write(idSubNum, i);
				if (GetControlInterface(sub)) {
					BeginGroup(idController);
					ok = WriteControllerHierarchy((Control*)sub, flags, timerange);
					EndGroup();// the 'Controller' group
				}
				else if (sub->ClassID().PartA() == 130) {// TODO Get the real IParamBlock2 class ID
					BeginGroup(idParamBlock);
					ok = WriteParamBlock((IParamBlock2*)sub, flags, timerange);
					EndGroup();// the 'ParamBlock' group
				}
				//////////////////////////////////////////////////////////////////////////
			}
		}

		if (GetIListControlInterface(ctrl)) {
			// We write these values out AFTER the subanims are written out so that we read them in after.
			// the names of the list items need to be read in after the list items are created
			IListControl *listctrl = GetIListControlInterface(ctrl);
			int numlistitems = listctrl->GetListCount();
			for (int i = 0; (i < numlistitems) && ok; i++)
			{
				TSTR listitemname = listctrl->GetName(i);
				Write(idListItemName, listitemname);
			}
			Write(idListItemActive, listctrl->GetActive());
		}

		return ok;
	}
	// we are a leaf controller.
	// maybe we have keyframes//(numKeys != NOT_KEYFRAMEABLE && numKeys > 0)
	else WriteLeafController(timerange, flags, ctrl);

	return TRUE;
}

BOOL CATRigWriter::WriteRefs(Interval timerange, int flags, Control* ctrl)
{
	BOOL ok = TRUE;
	for (int i = 0; (i < ctrl->NumRefs()) && ok; i++)
	{
		ReferenceTarget* ref = ctrl->GetReference(i);
		// make sure we have a sub and it is a controller
		if (ref && ok)
		{
			// Saving out the subanims index,
			// so the loader doesnt have to
			// guess as to which subindex it was
			Write(idSubNum, i);
			if (GetControlInterface(ref)) {
				BeginGroup(idController);
				ok = WriteControllerHierarchy((Control*)ref, flags, timerange);
				EndGroup();// the 'Controller' group
			}
			else if (ref->ClassID().PartA() == 130) {// TODO Get the real IParamBlock2 class ID
				BeginGroup(idParamBlock);
				ok = WriteParamBlock((IParamBlock2*)ref, flags, timerange);
				EndGroup();// the 'ParamBlock' group
			}
			//////////////////////////////////////////////////////////////////////////
		}
	}
	return ok;
};

BOOL CATRigWriter::WriteCAs(ReferenceTarget*)
{
	BOOL ok = TRUE;
	return ok;
};

BOOL WriteParamTab(CATRigWriter* save, IParamBlock2* pblock, ParamID pid, int flags, Interval timerange)
{
	ParamType2 type = pblock->GetParameterType(pid);
	int count = pblock->Count(pid);
	int index = pblock->IDtoIndex(pid);
	for (int i = 0; i < count; i++)
	{
		// Write out the pid in the table of the current item
		save->Write(idTabIndex, i);
		switch (type)
		{
		case TYPE_FLOAT_TAB:
			if (pblock->GetControllerByIndex(index, i)) {
				save->BeginGroup(idController);
				save->WriteControllerHierarchy(pblock->GetControllerByIndex(index, i), flags, timerange);
				save->EndGroup();
			}
			else {
				float val = pblock->GetFloat(pid, 0, i);
				save->Write(idValFloat, val);
			}
			break;
		case TYPE_MATRIX3_TAB:
			save->Write(idValMatrix3, pblock->GetMatrix3(pid, 0, i));
			break;
		case TYPE_INODE_TAB:
			if (pblock->GetINode(pid, 0, i)) {
				INode *node = pblock->GetINode(pid, 0, i);
				save->WriteNode(node);
			}
			break;
		case TYPE_STRING_TAB:
			const MCHAR* temp_string = pblock->GetStr(pid, 0, i);
			save->Write(idValStr, TSTR(temp_string));
			break;
		}
	}
	return save->ok();
}

#pragma warning(push)
#pragma warning(disable:4063) // can't put ParamType2's in switch statement
BOOL CATRigWriter::WriteParamBlock(IParamBlock2* pblock, int flags, Interval timerange)
{
	int numParams = pblock->NumParams();
	for (int i = 0; i < numParams; i++)
	{
		ParamID pid = pblock->IndextoID(i);
		ParamType2 type = pblock->GetParameterType(pid);

		// This code crashed on the new list controllers in Max 9
		switch (type)
		{
		case TYPE_FLOAT:
		case TYPE_ANGLE:
		case TYPE_PCNT_FRAC: {
			Write(idPBIndex, i);
			if (pblock->GetControllerByIndex(i)) {
				BeginGroup(idController);
				WriteControllerHierarchy(pblock->GetControllerByIndex(i), flags, timerange);
				EndGroup();
			}
			else {
				float val = pblock->GetFloat(pid);
				Write(idValFloat, val);
			}
			break;
		}
		case TYPE_INT:
		case TYPE_BOOL:
		case TYPE_TIMEVALUE: {
			Write(idPBIndex, i);
			if (pblock->GetControllerByIndex(i)) {
				BeginGroup(idController);
				WriteControllerHierarchy(pblock->GetControllerByIndex(i), flags, timerange);
				EndGroup();
			}
			else {
				int val = pblock->GetInt(pid);
				Write(idValInt, val);
			}
			break;
		}
		case TYPE_POINT3: {
			Write(idPBIndex, i);
			if (pblock->GetControllerByIndex(i)) {
				BeginGroup(idController);
				WriteControllerHierarchy(pblock->GetControllerByIndex(i), flags, timerange);
				EndGroup();
			}
			else {
				Point3 val = pblock->GetPoint3(pid);
				Write(idValPoint, val);
			}
			break;
		}
		case TYPE_MATRIX3: {
			Write(idPBIndex, i);
			Matrix3 val = pblock->GetMatrix3(pid);
			Write(idValMatrix3, (void*)&val);
			break;
		}
		case TYPE_INODE:
			Write(idPBIndex, (void*)&i);
			if (pblock->GetINode(pid)) {
				WriteNode(pblock->GetINode(pid));
			}
			break;
		case TYPE_INODE_TAB:
			// We should not try and save out these params as the footprint system in
			// CATMotion will cause huge tables of data to be saved out needlessly
			if (flags&CLIPFLAG_SKIP_NODE_TABLES) break;
		case TYPE_FLOAT_TAB:
//		case TYPE_MATRIX3_TAB:
		case TYPE_STRING_TAB:
			Write(idPBIndex, i);
			WriteParamTab(this, pblock, pid, flags, timerange);
			break;
		case TYPE_STRING:
			Write(idPBIndex, i);
			TSTR valstring(pblock->GetStr(pid));
			Write(idValStr, valstring);
			break;
		}
	}
	return ok();
}

BOOL CATRigReader::ReadParamBlock(IParamBlock2* pblock, Interval timerange, const float dScale, int flags)
{
	if (!pblock)
		return FALSE;
	BOOL done = FALSE;
	int curr_param_index = -1;
	ParamID curr_param_id = -1; // This gets assigned in the rigAssignment clause.
	int curr_tab_index = -1;
	ParamType2 type;

	while (!done && ok())
	{
		if (!NextClause()) {
			//  Here everything goes funky...
			return FALSE;
		}
		switch (CurClauseID())
		{
		case rigBeginGroup:
		{
			switch (CurIdentifier())
			{
			case idController:
				DbgAssert(curr_param_index >= 0);
				Control* ctrl = NULL;
				if (curr_tab_index < 0) {
					ctrl = pblock->GetControllerByID(curr_param_id);
				}
				// If there wasn't a controller already in this slot, then there should be
				if (!ctrl) {
					ParamType2 type = pblock->GetParameterType(curr_param_id);
					switch (type) {
					case TYPE_FLOAT_TAB:
					case TYPE_FLOAT:
					case TYPE_ANGLE_TAB:
					case TYPE_ANGLE:
					case TYPE_BOOL:
					case TYPE_PCNT_FRAC:
						ctrl = NewDefaultFloatController();
						break;
					case TYPE_POINT3:
						ctrl = NewDefaultPoint3Controller();
						break;
					default:
						GetCATMessages().Error(nLineNumber, GetString(IDS_ERR_WRONGTYPE));
						DbgAssert(FALSE);
						SkipGroup();
						break;
					}
					if (curr_tab_index >= 0) {
						if (curr_tab_index >= pblock->Count(curr_param_id))
							pblock->SetCount(curr_param_id, curr_tab_index + 1);
						pblock->SetControllerByID(curr_param_id, curr_tab_index, ctrl, FALSE);
					}
					else
						pblock->SetControllerByID(curr_param_id, 0, ctrl);
				}

				NextClause();
				// we may have simply a pose saved out for this subindex
				if (CurIdentifier() == idValFloat ||
					CurIdentifier() == idValPoint ||
					CurIdentifier() == idValQuat ||
					CurIdentifier() == idValMatrix3) {
					ReadPoseGroup(ctrl, timerange.Start(), dScale, flags);
					break;
				}
				if (CurIdentifier() != idValClassIDs) {
					SkipGroup();
					break;
				}
				TokClassIDs classid;
				GetValue(classid);
				Class_ID newClassID(classid.usClassIDA, classid.usClassIDB);

				if (ctrl->ClassID() != newClassID) {
					// create the new replacement subindex
					Control *newCtrl = (Control*)CreateInstance(classid.usSuperClassID, newClassID);

					if (newCtrl) {
						DbgAssert(newCtrl->SuperClassID() == classid.usSuperClassID);
						newCtrl->Copy(ctrl);
					}
					ctrl = newCtrl;
					if (curr_tab_index >= 0) {
						if (curr_tab_index >= pblock->Count(curr_param_id))
							pblock->SetCount(curr_param_id, curr_tab_index + 1);
						pblock->SetControllerByIndex(curr_param_index, curr_tab_index, ctrl, FALSE);
					}
					else
						pblock->SetControllerByIndex(curr_param_index, 0, ctrl);
				}
				ReadController(ctrl, timerange, dScale, flags);
				break;
			}
		}
		break;
		case rigAssignment:
		{
			curr_param_id = pblock->IndextoID(curr_param_index);
			switch (CurIdentifier()) {
			case idPBIndex: {
				int temp_index = -1;
				GetValue(temp_index);
				// This is the place where the ID gets assigned
				curr_param_id = pblock->IndextoID(temp_index);
				// Stephen, should I assign the curr_param_index here?
				curr_param_index = temp_index;
				// make sure we are starting a new tab
				curr_tab_index = -1;
				DbgAssert(curr_param_index < pblock->NumParams());
				type = pblock->GetParameterType(curr_param_id);
				break;
			}
			case idTabIndex:
				GetValue(curr_tab_index);
				break;
			case idValInt: {
				int val;
				GetValue(val);
				pblock->SetValue(curr_param_id, 0, val);
				break;
			}
			case idValFloat: {
				float val;
				GetValue(val);
				if (curr_tab_index >= 0) {
					if (curr_tab_index >= pblock->Count(curr_param_id))
						pblock->SetCount(curr_param_id, curr_tab_index + 1);
					pblock->SetValue(curr_param_id, 0, val, curr_tab_index);
				}
				else
					pblock->SetValue(curr_param_id, 0, val);
				break;
			}
			case idValPoint: {
				Point3 val;
				GetValue(val);
				if (curr_tab_index >= 0) {
					if (curr_tab_index >= pblock->Count(curr_param_id))
						pblock->SetCount(curr_param_id, curr_tab_index + 1);
					pblock->SetValue(curr_param_id, 0, val, curr_tab_index);
				}
				else
					pblock->SetValue(curr_param_id, 0, val);
				break;
			}
			case idValMatrix3: {
				Matrix3 val;
				GetValue(val);
				if (curr_tab_index >= 0) {
					if (curr_tab_index >= pblock->Count(curr_param_id))
						pblock->SetCount(curr_param_id, curr_tab_index + 1);
					pblock->SetValue(curr_param_id, 0, val, curr_tab_index);
				}
				else
					pblock->SetValue(curr_param_id, 0, val);
				break;
			}
			case idNode:
			case idNodeName: {
				INode *node = GetINode();
				if (!node) break;
				if (curr_tab_index >= 0) {
					if (flags&CLIPFLAG_SKIP_NODE_TABLES) break;
					if (curr_tab_index >= pblock->Count(curr_param_id))
						pblock->SetCount(curr_param_id, curr_tab_index + 1);
					pblock->SetValue(curr_param_id, 0, node, curr_tab_index);
				}
				else
					pblock->SetValue(curr_param_id, 0, node);
				break;
			}
			case idValStr: {
				TSTR str;
				GetValue((void*)&str, tokString);
				if (curr_tab_index >= 0) {
					if (curr_tab_index >= pblock->Count(curr_param_id))
						pblock->SetCount(curr_param_id, curr_tab_index + 1);
					pblock->SetValue(curr_param_id, 0, str, curr_tab_index);
				}
				else
					pblock->SetValue(curr_param_id, 0, str);
				break;
			}
			}
			break;
		}
		case rigAbort:
		case rigEnd:
		case rigEndGroup:
			done = TRUE;
			break;
		}
	}
	return ok();
}

// we walk our way down through the hierarchy looking for the subanim that is a child of our current Animatable
BOOL WalkSubs(Animatable *curr, ReferenceTarget* tgt, TSTR &address) {
	for (int i = 0; i < curr->NumSubs(); i++) {
		if (!curr->SubAnim(i)) continue;
		if (curr->SubAnim(i) == tgt) {
			address = curr->SubAnimName(i);
			return TRUE;
		}
		else if (WalkSubs(curr->SubAnim(i), tgt, address)) {
			if (!curr->BypassPropertyLevel()) {
				address = curr->SubAnimName(i) + _T(".") + address;
			}
			return TRUE;
		}
	}
	return FALSE;
}

Animatable* FindSub(Animatable *curr, TSTR address)
{
	// Find the 1st part of the address
	int i = 0;
	while (address[i] != _T('\n') && address[i] != _T('.') && i < address.Length()) i++;
	TSTR address_chunk;
	if (address[i] == _T('\n'))		address_chunk = address.Substr(0, i - 1);
	else if (address[i] == _T('.')) { address_chunk = address.Substr(0, i); }
	else { address_chunk = address;				i--; }

	address = address.remove(0, i + 1);

	if (address_chunk.Length() > 1) {
		for (int i = 0; i < curr->NumSubs(); i++) {
			if (curr->SubAnimName(i) == address_chunk) {
				if (address.Length() > 0)
					return FindSub(curr->SubAnim(i), address);
				else return curr->SubAnim(i);
			}
		}
	}
	return NULL;
}

TSTR GenerateSubAnimAddress(Animatable *curr, ReferenceTarget *ref)
{
	// We only keep references to controllers on our own nodes

	TSTR address(_T(""));
	if (WalkSubs(curr, ref, address) && address.Length() > 1) return address;
	return _T("");
}

BOOL CATRigWriter::WriteScriptController(Control* ctrl, int flags, Interval timerange)
{
	UNREFERENCED_PARAMETER(flags);
	BOOL ok = TRUE;

	BeginGroup(idScriptControlParams);

	IScriptCtrl		*scriptctrl = (IScriptCtrl*)ctrl->GetInterface(IID_SCRIPT_CONTROL);

	// Write out the description text field.
	// This is simply a note written by scripters to help
	// people understand whats going on in thier script
	if (scriptctrl->GetDescription()) {
		BeginGroup(idDescriptionText);
		ok = WriteStringSequence(TSTR(scriptctrl->GetDescription()));
		EndGroup();
	}

	// I havn't figured out how to construct a 'Value' class so I can't use the FPInterface
	IBaseScriptControl8		*scriptctrl8 = (IBaseScriptControl8*)ctrl;
	for (int i = 0; i < scriptctrl8->getVarCount(); i++) {
		TSTR varname = scriptctrl->GetVarName(i);
		// Skip all the constants that come with the script controller
		if (varname == _T("T") || varname == _T("S") || varname == _T("F") || varname == _T("NT")) continue;
		FPValue &val = scriptctrl8->getVarValue(i, timerange.Start());
		switch (val.type) {
		case TYPE_BOOL:
		case TYPE_INT:			Write(idName, varname);	Write(idValType, val.type);	Write(idValInt, val.i);		break;
		case TYPE_TIMEVALUE:	Write(idName, varname);	Write(idValType, val.type);	Write(idValTimeValue, val.t);		break;
		case TYPE_FLOAT:		Write(idName, varname);	Write(idValType, val.type);	Write(idValFloat, val.f);		break;
		case TYPE_ANGLE:		Write(idName, varname);	Write(idValType, val.type);	Write(idValAngle, val.f);		break;
		case TYPE_PCNT_FRAC:	Write(idName, varname);	Write(idValType, val.type);	Write(idValFloat, val.f);		break;
		case TYPE_COLOR:
		case TYPE_COLOR_BV:		Write(idName, varname);	Write(idValType, val.type);	Write(idValColor, val.clr);	break;
		case TYPE_RGBA:
		case TYPE_POINT3_BV:
		case TYPE_POINT3:		Write(idName, varname);	Write(idValType, val.type);	Write(idValPoint, val.p);		break;
		case TYPE_REFTARG:
		{
			INode *node = FindReferencingClass<INode>(val.r);
			if (!node) break;

			TSTR addr = GenerateSubAnimAddress(node, val.r);
			Write(idName, varname);
			Write(idValType, val.type);
			Write(idValStr, addr);
			WriteNode(node);
			break;
		}
		case TYPE_CONTROL:
		{
			INode *node = FindReferencingClass<INode>(val.ctrl);
			if (!node) break;

			TSTR addr = GenerateSubAnimAddress(node, val.ctrl);
			Write(idName, varname);
			Write(idValType, val.type);
			Write(idValStr, addr);
			WriteNode(node);
			break;
		}
		case TYPE_MATRIX3:
		case TYPE_MATRIX3_BV:	Write(idName, varname);	Write(idValType, val.type);	Write(idValMatrix3, val.m);		break;
		case TYPE_INODE:		Write(idName, varname);	Write(idValType, val.type);	WriteNode(val.n);					break;
		case TYPE_STRING:		Write(idName, varname);	Write(idValType, val.type);	Write(idValStr, val.tstr);	break;
		}
	}

	TSTR	script(scriptctrl->GetExpression());
	scriptctrl->SetExpression(script);

	BeginGroup(idScriptText);
	ok = WriteStringSequence(script);
	EndGroup();

	EndGroup(); // idScriptControlParams
	return ok;
}
#pragma warning(pop)

BOOL CATRigReader::ReadScriptController(Control* mxsctrl, Interval timerange, const float dScale, int flags)
{
	UNREFERENCED_PARAMETER(flags); UNREFERENCED_PARAMETER(dScale); UNREFERENCED_PARAMETER(timerange);
	if (!mxsctrl) return FALSE;

	BOOL done = FALSE;
	TSTR varname;
	TSTR varaddress(_T(""));
	INode* node = NULL;
	int valtype = 0;
	FPValue fpval;

	IScriptCtrl		*scriptctrl = (IScriptCtrl*)mxsctrl->GetInterface(IID_SCRIPT_CONTROL);
	if (!scriptctrl) {
		SkipGroup();
		return TRUE;
	}

	while (!done && ok())
	{
		if (!NextClause()) {
			//  Here everything goes funky...
			return FALSE;
		}
		switch (CurClauseID())
		{
		case rigBeginGroup:
			switch (CurIdentifier()) {
			case idDescriptionText: {
				TSTR string(_T(""));
				if (ReadString(string)) {
					scriptctrl->SetDescription(string);
				}
				break;
			}
			case idScriptText: {
				TSTR string(_T(""));
				if (ReadString(string)) {
					scriptctrl->SetExpression(string);
				}
				break;
			}
			}
			break;

		case rigAssignment:
		{
			switch (CurIdentifier()) {
			case idName:		GetValue(varname);		break;
			case idValType:		GetValue(valtype);		break;
			case idValTimeValue:
			case idValInt: {
				int int_val;
				GetValue(int_val);
				fpval.Load(valtype, int_val);
				scriptctrl->AddConstant(varname, fpval);
				break;
			}
			case idValAngle:
			case idValFloat: {
				int int_val;
				GetValue(int_val);
				fpval.Load(valtype, int_val);
				scriptctrl->AddConstant(varname, fpval);
				break;
			}
								  break;
			case idValColor: {
				Point3 p3_val;
				GetValue(p3_val);
				fpval.Load(valtype, Color(p3_val));
				scriptctrl->AddConstant(varname, fpval);
				break;
			}
			case idValPoint: {
				Point3 p3_val;
				GetValue(p3_val);
				fpval.Load(valtype, p3_val);
				scriptctrl->AddConstant(varname, fpval);
				break;
			}
			case idNode:
			case idNodeName:
				node = GetINode();
				if (!node)continue;
				if (valtype == TYPE_INODE) {
					scriptctrl->AddNode(varname, node);
				}
				else {
					Control* anim = (Control*)FindSub(node, varaddress);
					if (!anim)continue;
					Value* val = MAXClass::make_wrapper_for(anim);
					scriptctrl->AddTarget(varname, val, 0);
				}
				break;
			case idValStr:
				GetValue(varaddress);
				break;
			}
		}
		break;
		case rigEndGroup:
			done = TRUE;
			break;
		default:
			AssertOutOfPlace();
			SkipGroup();
			return ok();
		}
	}
	return TRUE;
}

BOOL CATRigWriter::WriteStringSequence(TSTR str)
{
	while (str && str.Length() > 0) {
		int i = 0;
		while (str[i] != _T('\n') && i < str.Length()) i++;
		TSTR strline;
		if (str[i] == _T('\n'))	strline = str.Substr(0, i - 1);
		else { strline = str;	i--; }
		if (strline.Length() > 1)
			Write(idValStr, strline);
		str = str.remove(0, i + 1);
		if (str.Length() <= 1) return ok();
	}
	return ok();
}

BOOL CATRigWriter::WriteReactionController(Control* ctrl, int flags, Interval timerange)
{
	UNREFERENCED_PARAMETER(ctrl); UNREFERENCED_PARAMETER(flags); UNREFERENCED_PARAMETER(timerange);
	BOOL ok = TRUE;
	return ok;
}

/**************************************************************************/
/* This is a function that saves out basic pose values for the controller */
/**************************************************************************/
BOOL CATRigWriter::WritePose(TimeValue t, Control* ctrl)
{
	ULONG superclassID = ctrl->SuperClassID();
	Interval iv = FOREVER;

	// to reduce the size of clip files I have made poses
	// just a value that comes after the controller class ids
	// instead of a full group
	switch (superclassID) {
	case CTRL_MATRIX3_CLASS_ID: {
		Matrix3 val(1);
		ctrl->GetValue(t, (void*)&val, iv, CTRL_RELATIVE);
		Write(idValMatrix3, val);
		break;
	}
	case CTRL_POSITION_CLASS_ID:
	case CTRL_POINT3_CLASS_ID: {
		Point3 val;
		ctrl->GetValue(t, (void*)&val, iv);
		Write(idValPoint, val);
		break;
	}
	case CTRL_ROTATION_CLASS_ID: {
		Quat val;
		ctrl->GetValue(t, (void*)&val, iv);
		Write(idValQuat, val);
		break;
	}
	case CTRL_SCALE_CLASS_ID: {
		ScaleValue val;
		ctrl->GetValue(t, (void*)&val, iv);
		Write(idValPoint, val.s);
		break;
	}
	case CTRL_FLOAT_CLASS_ID: {
		float val;
		ctrl->GetValue(t, (void*)&val, iv);
		Write(idValFloat, val);
		break;
	}
	}
	return TRUE;
}

/**************************************************************************/
/* This is a function that saves out basic key values for the controller  */
/* currently this is only used on PRS controllers						  */
/**************************************************************************/
BOOL CATRigWriter::WriteSimpleKeys(Interval timerange, Control* ctrl)
{
	TimeValue nextkeyt, t = timerange.Start();

	BOOL ok = TRUE;
	BOOL done = FALSE;
	int flags = NEXTKEY_RIGHT | NEXTKEY_POS | NEXTKEY_ROT | NEXTKEY_SCALE;

	ULONG superclassID = ctrl->SuperClassID();
	ULONG keyflags = 0;
	BeginGroup(idSimpleKeys);
	nextkeyt = t;

	while (t < timerange.End() && !done)
	{
		// On our 1st pass through we write a key on the start time
		if (nextkeyt == timerange.Start() && timerange.Start() >= FOREVER.Start())
		{
			t = timerange.Start() + 1;
			nextkeyt++;
		}
		else
		{
			ok = ctrl->GetNextKeyTime(t, flags, nextkeyt);
			// no more nextkeys,
			if (!ok || t >= nextkeyt || nextkeyt > timerange.End()) {
				done = TRUE;
				if (ok && timerange.End() < FOREVER.End()) {
					// Write one more key to cap off the end of the sequence, and then quit.
					t = timerange.End() - 1;
				}
				else {
					continue;
				}
			}
			else {
				t = nextkeyt;
			}
		}

		Interval iv = FOREVER;
		switch (superclassID) {
		case CTRL_MATRIX3_CLASS_ID:
		case CTRL_POSITION_CLASS_ID:
		case CTRL_ROTATION_CLASS_ID: {
			Matrix3 val(1);
			ctrl->GetValue(t, (void*)&val, iv, CTRL_RELATIVE);
			CATClipKeyMatrix catclipkey(keyflags, t, val);
			Write(idValKeyMatrix, (void*)&catclipkey);
			break;
		}
		case CTRL_POINT3_CLASS_ID: {
			Point3 val(0.0f, 0.0f, 0.0f);
			ctrl->GetValue(t, (void*)&val, iv);
			CATClipKeyPoint catclipkey(keyflags, t, val);
			Write(idValKeyPoint, (void*)&catclipkey);
			break;
		}
		case CTRL_FLOAT_CLASS_ID: {
			float val;
			ctrl->GetValue(t, (void*)&val, iv);
			CATClipKeyFloat catclipkey(keyflags, t, val);
			Write(idValKeyFloat, (void*)&catclipkey);
			break;
		}
		}

	}

	EndGroup(); // the 'Keys' Group
	return TRUE;
}

/**************************************************************************/
/* This is a function that decides what kind of keyframes this controller */
/* and then calls WriteKeys with the correct arguments                    */
/**************************************************************************/
BOOL CATRigWriter::WriteLeafController(Interval timerange, int flags, Control* ctrl)
{
	int ortin = ctrl->GetORT(ORT_BEFORE);
	Write(idORTIn, ortin);

	int ortout = ctrl->GetORT(ORT_AFTER);
	Write(idORTOut, ortout);

	if (!(flags&CLIPFLAG_SKIP_KEYFRAMES)) {
		Interval range = ctrl->GetTimeRange(TIMERANGE_ALL);
		int start = range.Start();
		int end = range.End();
		Write(idRangeStart, start);
		Write(idRangeEnd, end);
	}

	// Find out if we are a script controller...
	if (ctrl->GetInterface(IID_SCRIPT_CONTROL)) {
		return WriteScriptController(ctrl, flags, timerange);
	}

	if (ctrl->GetInterface(REACTOR_INTERFACE)) {
		//	return WriteReactionController(ctrl, flags, timerange);
	}

	if (!GetKeyControlInterface(ctrl))	return ok();

	// We are saving a clip file of the controller settings, but not the keyframes
	if (flags&CLIPFLAG_SKIP_KEYFRAMES) 	return ok();

	switch (ctrl->ClassID().PartA())
	{
		//////////////////////////////////////////////////////////////////////////
		// Position
		//
	case HYBRIDINTERP_POSITION_CLASS_ID: {
		IBezPoint3Key	key;
		WriteKeys(timerange, flags, ctrl, idValKeyBezXYZ, (IKey*)&key);
		break;
	}
	case TCBINTERP_POSITION_CLASS_ID: {
		ITCBPoint3Key	key;
		WriteKeys(timerange, flags, ctrl, idValKeyTCBXYZ, (IKey*)&key);
		break;
	}
	case LININTERP_POSITION_CLASS_ID: {
		ILinPoint3Key	key;
		WriteKeys(timerange, flags, ctrl, idValKeyLinXYZ, (IKey*)&key);
		break;
	}

	//////////////////////////////////////////////////////////////////////////
	// rotation
	case HYBRIDINTERP_ROTATION_CLASS_ID: {
		IBezQuatKey	key;
		WriteKeys(timerange, flags, ctrl, idValKeyBezRot, (IKey*)&key);
		break;
	}
	case TCBINTERP_ROTATION_CLASS_ID: {
		ITCBRotKey	key;
		WriteKeys(timerange, flags, ctrl, idValKeyTCBRot, (IKey*)&key);
		break;
	}
	case LININTERP_ROTATION_CLASS_ID: {
		ILinRotKey	key;
		WriteKeys(timerange, flags, ctrl, idValKeyLinRot, (IKey*)&key);
		break;
	}
	//////////////////////////////////////////////////////////////////////////
	// scale
	case HYBRIDINTERP_SCALE_CLASS_ID: {
		IBezScaleKey	key;
		WriteKeys(timerange, flags, ctrl, idValKeyBezScale, (IKey*)&key);
		break;
	}
	case TCBINTERP_SCALE_CLASS_ID: {
		ITCBScaleKey	key;
		WriteKeys(timerange, flags, ctrl, idValKeyTCBScale, (IKey*)&key);
		break;
	}
	case LININTERP_SCALE_CLASS_ID: {
		ILinScaleKey	key;
		WriteKeys(timerange, flags, ctrl, idValKeyLinScale, (IKey*)&key);
		break;
	}
	//////////////////////////////////////////////////////////////////////////
	// Point3
	case HYBRIDINTERP_POINT3_CLASS_ID: {
		IBezPoint3Key	key;
		WriteKeys(timerange, flags, ctrl, idValKeyBezXYZ, (IKey*)&key);
		break;
	}
	case TCBINTERP_POINT3_CLASS_ID: {
		ITCBPoint3Key	key;
		WriteKeys(timerange, flags, ctrl, idValKeyTCBXYZ, (IKey*)&key);
		break;
	}
	//////////////////////////////////////////////////////////////////////////
	// float
	case HYBRIDINTERP_FLOAT_CLASS_ID: {
		IBezFloatKey	key;
		WriteKeys(timerange, flags, ctrl, idValKeyBezFloat, (IKey*)&key);
		break;
	}
	case TCBINTERP_FLOAT_CLASS_ID: {
		ITCBFloatKey	key;
		WriteKeys(timerange, flags, ctrl, idValKeyTCBFloat, (IKey*)&key);
		break;
	}
	case LININTERP_FLOAT_CLASS_ID: {
		ILinFloatKey	key;
		WriteKeys(timerange, flags, ctrl, idValKeyLinFloat, (IKey*)&key);
		break;
	}
	//////////////////////////////////////////////////////////////////////////

	default: {
		//			mputs("Error: Skipping unkown controller type");
		break;
	}
	}
	return TRUE;
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
BOOL CATRigWriter::WriteKeys(Interval timerange, int flags, Control* ctrl, USHORT idKey, IKey *key)
{
	DbgAssert(outstream.good());
	DbgAssert(!outstream.fail());

	IKeyControl *ikeys = GetKeyControlInterface(ctrl);
	if (!ikeys) return FALSE;
	int numKeys = ikeys->GetNumKeys();

	if (numKeys == 0) return TRUE;

	BeginGroup(idKeys);

	// Why confuse things by changing the keyframe time values?
	// Because, suppose we are saving a sequence from the middle of an animation, like a punch action
	// then when we reload it, we should be able to reload it at a different time. We add on the start time
	// when reloading the animation.
	// If we are saving an entire sequence, then we still need to subtract off the scene start time,
	// otherwise our keys will be offset by scene start time when we reload.  This is necessary
	// to ensure that key timings are kept consistent when saving/loading to scenes with a
	// initial time that is not 0 (MAXX-5972)
	int iKeyTimeOffset = timerange.Start();
	if (iKeyTimeOffset == TIME_NegInfinity)
		iKeyTimeOffset = GetCOREInterface()->GetAnimRange().Start();

	int writtenkeys = 0;
	for (int i = 0; i < numKeys; i++)
	{
		if (timerange.InInterval(ctrl->GetKeyTime(i)))
		{
			// We must start our TCB keyframe sequence with a pose value
			// We should skip the 1st key in a TCB key sequence, because the Pose should be the value loaded
			if (idKey == idValKeyTCBRot && writtenkeys == 0) {
				ikeys->GetKey(i, key);
				WritePose(key->time, ctrl);
				writtenkeys++;
				continue;
			}
			ikeys->GetKey(i, key);
			key->time = key->time - iKeyTimeOffset;

			// If we are a timewarp controller, then our
			// key value (out time) is relative to the
			// key time (in time).  So needs to be
			// offset as well!
			if (flags&CLIPFLAG_TIMEWARP_KEYFRAMES)
			{
				switch (idKey)
				{
				case idValKeyTCBFloat:
				{
					ITCBFloatKey* floatKey = static_cast<ITCBFloatKey*>(key);
					floatKey->val -= iKeyTimeOffset;
					break;
				}
				case idValKeyLinFloat:
				{
					ILinFloatKey* floatKey = static_cast<ILinFloatKey*>(key);
					floatKey->val -= iKeyTimeOffset;
					break;
				}
				case idValKeyBezFloat:
				{
					IBezFloatKey* floatKey = static_cast<IBezFloatKey*>(key);
					floatKey->val -= iKeyTimeOffset;
					break;
				}
				default:
					DbgAssert("EPIC FAIL HERE");
					break;
				}
			}
			Write(idKey, (void*)key);
			writtenkeys++;
		}
	}
	EndGroup();
	return TRUE;
}

// In c++ we have no access to things like the weights controller
// this kind of issue pops up again and again.
// For this I truely believe that we will have to rewrite the whole
// clip system in script.

BOOL CATRigReader::ReadConstraint(Control* ctrl, Interval timerange, const float dScale, int flags)
{
	if (!ctrl) return FALSE;

	BOOL done = FALSE;
	int numtargets = 0;
	int loadedtargets = 0;
	INode* node = NULL;
	float weight = 0.0f;
	int id = -1;
	//	int val;
	Class_ID classid = ctrl->ClassID();

	while (!done && ok())
	{
		if (!NextClause()) {
			//  Here everything goes funky...
			return FALSE;
		}
		switch (CurClauseID())
		{
		case rigBeginGroup:
			break;
		case rigAssignment:
		{
			switch (CurIdentifier()) {
			case idNumTargets:
				GetValue(numtargets);
				break;
			case idNode:
			case idNodeName: {
				node = GetINode();
				if (ctrl->GetInterface(I_ATTACHCTRL)) {
					IAttachCtrl *attach = (IAttachCtrl*)ctrl->GetInterface(I_ATTACHCTRL);
					attach->SetObject(node);
				}
				else {
					loadedtargets++;
				}
				break;
			}
			case idValInt:

				if (ILinkCtrl* linctrl = (ILinkCtrl*)ctrl->GetInterface(LINK_CONSTRAINT_INTERFACE)) {
					TimeValue t;
					GetValue(t);
					// If null is passed to the link constraint as a target, it is a world constraint
					linctrl->AddNewLink(node, t);
					Matrix3 tm(1);
					Interval iv = FOREVER;
					ctrl->GetValue(t + 1, (void*)&tm, iv, CTRL_RELATIVE);
					node = NULL;
				}
				else if (ctrl->GetInterface(I_ATTACHCTRL)) {
					GetValue(id);
				}
				break;
			case idValPoint:
				if (ctrl->GetInterface(I_ATTACHCTRL) && id >= 0) {
					Point3 pos;
					GetValue(pos);
					IAttachCtrl *attach = (IAttachCtrl*)ctrl->GetInterface(I_ATTACHCTRL);
					attach->SetKeyPos(0, id, pos);
				}
				break;
			case idWeight:
				GetValue(weight);
				if (node && GetILookAtConstInterface(ctrl)) {
					((ILookAtConstRotation*)GetILookAtConstInterface(ctrl))->AppendTarget(node, weight);
					node = NULL;
					weight = 0.0f;
				}
				break;
			case idSubNum:
			{
				int subanimindex;
				GetValue(subanimindex);

				// Step onto the next clause that tells us what kind on sub this is.
				NextClause();

				// this method figures out what kind of sub anim
				// we have and then calls ReadController
				if (!ReadSubAnim(ctrl, subanimindex, timerange, dScale, flags))
				{
					AssertOutOfPlace();
					SkipGroup();
					done = TRUE;
				}
			}
			break;
			case idFlags:
				if (ctrl->GetInterface(I_ATTACHCTRL)) {
					int align;
					GetValue(align);
					((IAttachCtrl*)ctrl->GetInterface(I_ATTACHCTRL))->SetAlign((BOOL)align);
				}
				break;
			}
		}
		break;
		case rigEndGroup:
			done = TRUE;
			break;
		default:
			AssertOutOfPlace();
			SkipGroup();
			return ok();
		}
	}
	return TRUE;
}

BOOL CATRigReader::ReadString(TSTR &string)
{
	BOOL done = FALSE;
	while (!done && ok())
	{
		if (!NextClause()) {
			//  Here everything goes funky...
			return FALSE;
		}
		switch (CurClauseID())
		{
		case rigBeginGroup:
			break;
		case rigAssignment:
			switch (CurIdentifier()) {
			case idValStr:
			case idScript: {
				TSTR line;
				if (GetValue(line)) {
					string.printf(_T("%s \n %s"), string.data(), line.data());
				}
				break;
			}
			}
			break;
		case rigEndGroup:
			done = TRUE;
			break;
		default:
			AssertOutOfPlace();
			SkipGroup();
			return ok();
		}
	}
	return TRUE;
}

BOOL CATRigWriter::WriteLinkConst(ILinkCtrl* constr, int flags, Interval timerange)
{
	UNREFERENCED_PARAMETER(flags); UNREFERENCED_PARAMETER(timerange);
	BeginGroup(idConstraint);

	int numtargets = constr->GetNumTargets();
	Write(idNumTargets, numtargets);
	for (int i = 0; i < numtargets; i++) {
		INode* node = constr->GetNode(i);
		if (!node) {
			// we have a world link.
			TSTR worldlink(_T(""));
			Write(idNodeName, worldlink);
		}
		else {
			WriteNode(node);
		}
		TimeValue t = constr->GetLinkTime(i);
		Write(idValInt, t);
	}

	EndGroup();
	return TRUE;
};

BOOL CATRigWriter::WriteConstraint(Control* constr, int flags, Interval timerange)
{
	BeginGroup(idConstraint);

	int numSubs = constr->NumSubs();
	Animatable* sub;
	for (int i = 0; (i < numSubs) && ok(); i++)
	{
		sub = constr->SubAnim(i);
		// make sure we have a sub and it is a controller
		if (sub && ok())
		{
			if (GetControlInterface(sub)) {
				// Saving out the subanims index,
				// so the loader doesnt have to
				// guess as to which subindex it was
				Write(idSubNum, i);
				BeginGroup(idController);
				WriteControllerHierarchy((Control*)sub, flags, timerange);
				EndGroup();// the 'Controller' group
			}
			else if (sub->ClassID().PartA() == 130) {// TODO Get the real IParamBlock2 class ID
				Write(idSubNum, i);
				BeginGroup(idParamBlock);
				WriteParamBlock((IParamBlock2*)sub, flags, timerange);
				EndGroup();// the 'ParamBlock' group
			}
			//////////////////////////////////////////////////////////////////////////
		}
	}

	if (constr->GetInterface(I_ATTACHCTRL)) {
		IAttachCtrl *attach = (IAttachCtrl*)constr->GetInterface(I_ATTACHCTRL);

		INode* node = attach->GetObject();
		if (node) {
			WriteNode(node);
		}

		ULONG alignbool = attach->GetAlign();
		Write(idFlags, alignbool);

		/////////////////////////////////////
		AKey* akey = attach->GetKey(0);
		if (akey) {
			Point3 pos(akey->u0, akey->u1, 0.0f);
			Write(idValInt, akey->face);
			Write(idValPoint, pos);
		}
	}
	EndGroup();
	return TRUE;
};

BOOL CATRigWriter::SaveMesh(Mesh &mesh)
{
	if (mesh.numFaces > 0)
	{
		int i;
		BeginGroup(idMeshParams);

		int numVerts = mesh.getNumVerts();
		Write(idNumVerticies, numVerts);
		for (i = 0; i < numVerts; i++) {
			Point3 vert = mesh.getVert(i);
			Write(idVertex, vert);
		}

		int numFaces = mesh.getNumFaces();
		Write(idNumFaces, numFaces);
		for (i = 0; i < numFaces; i++) {
			Face face = mesh.faces[i];
			Write(idValFace, face);
		}

		// mesh
		EndGroup();
	}
	return TRUE;
}

BOOL CATRigReader::ReadMesh(Mesh &mesh)
{
	BOOL done = FALSE;

	mesh = Mesh();
	Point3 vert;
	Face face;
	int numVerts = 0;
	int numFaces = 0;
	int currVert = 0;
	int currFace = 0;

	while (!done && ok())
	{
		if (!NextClause())
		{
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if (CurGroupID() != idMeshParams) return FALSE;
		}
		// Now check the clause ID and act accordingly.
		switch (CurClauseID()) {
		case rigAssignment:
			switch (CurIdentifier())
			{
			case idNumVerticies:
				if (GetValue(numVerts))	mesh.setNumVerts(numVerts);
				break;
			case idNumFaces:
				if (GetValue(numFaces))	mesh.setNumFaces(numFaces);
				break;
			case idVertex:
				if (GetValue(vert)) {
					mesh.setVert(currVert, vert);
					currVert++;
				}
				break;
			case idValFace:
				if (GetValue(face)) {
					mesh.faces[currFace] = face;
					currFace++;
				}
				break;
			default:
				AssertOutOfPlace();
			}
			break;
		case rigEndGroup:

			// Start using the modified geometry
			mesh.InvalidateTopologyCache();

			done = TRUE;
			break;
		case rigAbort:
		case rigEnd:
			return FALSE;
		}
	}
	return ok();
}
