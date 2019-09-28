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

#include "cat.h"
#include "CATParentTrans.h"
#include "CatPlugins.h"
#include "CATNodeControl.h"
#include "../CATMuscle/MuscleStrand.h"
#include "../CATMuscle/Muscle.h"
#include "MaxIcon.h"

bool g_bOutOfDateWarningDisplayed = false;
bool g_bOldFileWarningDisplayed = false;
HIMAGELIST hIcons = NULL;

bool g_bSavingCAT3Rig = false;
bool g_bLoadingCAT3Rig = false;

//////////////////////////////////////////////////////////////////////////
// Class CATEvaluationLock
// Auto-locks CAT evaluation, and optionally updates a single CATCharacter
// on destruction.

// If non-0, this will prevent ALL CATNodeControls from calling GetValue.
int CATEvaluationLock::s_iStopEvaluating = 0;

CATEvaluationLock::CATEvaluationLock(CATParentTrans* pParent)
	: mpLockingParent(pParent)
{
	s_iStopEvaluating++;
}

CATEvaluationLock::~CATEvaluationLock()
{
	s_iStopEvaluating--;
	DbgAssert(s_iStopEvaluating >= 0);
	if (mpLockingParent != NULL)
		mpLockingParent->UpdateCharacter();
}

bool CATEvaluationLock::IsEvaluationLocked()
{
	return s_iStopEvaluating != 0;
}

//////////////////////////////////////////////////////////////////////////
// Class MaxAutoDeleteLock
// This class locks the auto-delete of a passed class while it is alive.

MaxAutoDeleteLock::MaxAutoDeleteLock(ReferenceTarget* pTarget, bool bDoAutoDelOnRelease)
	: mTarget(pTarget)
	, mAutoDeleteOnRelease(bDoAutoDelOnRelease)
	, mWasLocked(false)
{
	if (mTarget != NULL)
	{
		mWasLocked = mTarget->TestAFlag(A_LOCK_TARGET) == TRUE;
		if (!mWasLocked)
			mTarget->SetAFlag(A_LOCK_TARGET);
	}
}

MaxAutoDeleteLock::~MaxAutoDeleteLock()
{
	if (mTarget != NULL)
	{
		if (!mWasLocked)
		{
			mTarget->ClearAFlag(A_LOCK_TARGET);
			if (mAutoDeleteOnRelease)
				mTarget->MaybeAutoDelete();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Class CATEvaluationLock
// Disables reference messages on creation, and enables them on destruction

MaxReferenceMsgLock::MaxReferenceMsgLock()
{
	DisableRefMsgs();
}
MaxReferenceMsgLock::~MaxReferenceMsgLock()
{
	EnableRefMsgs();
}

//////////////////////////////////////////////////////////////////////////
// Find a class of type type that references the target.

template <class T>
T* IterateDependents(ReferenceTarget *pTarget, int maxDepth)
{
	// Iterate dependents, Depth-first on
	// the assumption that the class we are
	// looking for was one of the first to reference this class.
	DependentIterator di(pTarget);
	ReferenceMaker* pMaker = NULL;
	maxDepth--;
	while ((pMaker = di.Next()) != NULL)
	{
		T *pRefOwner = dynamic_cast<T *>(pMaker);
		if (NULL != pRefOwner)
		{
			return pRefOwner;
		}
		if (maxDepth > 0 && pMaker->IsRefTarget())
		{
			T* val = IterateDependents<T>(static_cast<ReferenceTarget*>(pMaker), maxDepth);
			if (val != NULL)
				return val;
		}
	}
	return NULL;
}

template <class T>
T* FindReferencingClass(ReferenceTarget* pTarget, int maxDepth/*=-1*/)
{
	return IterateDependents<T>(pTarget, maxDepth);
}

// Provide explicit implementations for the above function to allow linking
template INode* FindReferencingClass<INode>(ReferenceTarget*, int);
// Provide explicit implementations for the above function to allow linking
template MuscleStrand* FindReferencingClass<MuscleStrand>(ReferenceTarget*, int);
// Provide explicit implementations for the above function to allow linking
template Muscle* FindReferencingClass<Muscle>(ReferenceTarget*, int);
// Provide explicit implementations for the above function to allow linking
template CATNodeControl* FindReferencingClass<CATNodeControl>(ReferenceTarget*, int);

//////////////////////////////////////////////////////////////////////
// class MaskedBitmap
//
// This class provides a method to paint a bitmap icon to a DC using
// any requested colour.  The bitmap is automatically masked.
//

MaskedBitmap::MaskedBitmap(HDC hdc, int width, int height, const BYTE *lpBitmap) {
	nWidth = MaxSDK::UIScaled(width);
	nHeight = MaxSDK::UIScaled(height);

	int nWidthAligned = width;
	if (width % 16 > 0)
		nWidthAligned = width - (width % 16) + 16;

	int nTotalSize = (nWidthAligned >> 3) * height;
	BYTE *lpMask = new BYTE[nTotalSize];
	for (int i = 0; i < nTotalSize; i++)
		lpMask[i] = ~lpBitmap[i];

	HBITMAP hbmMonoImage = CreateBitmap(width, height, 1, 1, lpBitmap);
	HBITMAP hbmMonoMask = CreateBitmap(width, height, 1, 1, lpMask);

	HBITMAP scaledMonoImage = MaxSDK::GetUIScaledBitmap(hbmMonoImage);
	if (scaledMonoImage)
	{
		DeleteObject(hbmMonoImage);
		hbmMonoImage = scaledMonoImage;
	}

	HBITMAP scaledMonoMask = MaxSDK::GetUIScaledBitmap(hbmMonoMask);
	if (scaledMonoMask)
	{
		DeleteObject(hbmMonoMask);
		hbmMonoMask = scaledMonoMask;
	}

	hbmImage = CreateCompatibleBitmap(hdc, nWidth, nHeight);
	hbmMask = CreateCompatibleBitmap(hdc, nWidth, nHeight);

	HDC hdcBitmap = CreateCompatibleDC(hdc);
	HDC hdcMonoBitmap = CreateCompatibleDC(hdc);

	SelectObject(hdcBitmap, hbmImage);
	SelectObject(hdcMonoBitmap, hbmMonoImage);
	BitBlt(hdcBitmap, 0, 0, nWidth, nHeight, hdcMonoBitmap, 0, 0, SRCCOPY);

	SelectObject(hdcBitmap, hbmMask);
	SelectObject(hdcMonoBitmap, hbmMonoMask);
	BitBlt(hdcBitmap, 0, 0, nWidth, nHeight, hdcMonoBitmap, 0, 0, SRCCOPY);

	SelectObject(hdcBitmap, NULL);
	SelectObject(hdcMonoBitmap, NULL);
	DeleteDC(hdcBitmap);
	DeleteDC(hdcMonoBitmap);
	DeleteObject(hbmMonoImage);
	DeleteObject(hbmMonoMask);

	delete[] lpMask;
}

MaskedBitmap::~MaskedBitmap() {
	DeleteObject(hbmImage);
	DeleteObject(hbmMask);
}

void MaskedBitmap::MaskBlit(HDC hdc, int x, int y, HBRUSH hBrush) {
	HDC hdcBitmap = CreateCompatibleDC(hdc);
	HDC hdcColourImage = CreateCompatibleDC(hdc);
	HBITMAP hbmColourImage = CreateCompatibleBitmap(hdc, nWidth, nHeight);

	// Copy the bitmap image into hbmColourImage by merging
	// it with the given brush colour.
	SelectObject(hdcColourImage, hbmColourImage);
	SelectObject(hdcBitmap, hbmImage);
	HBRUSH hOldBrush = SelectBrush(hdcColourImage, hBrush);
	BitBlt(hdcColourImage, 0, 0, nWidth, nHeight, hdcBitmap, 0, 0, MERGECOPY);
	SelectObject(hdcColourImage, hOldBrush);

	// Apply the mask to the destination DC.
	SelectObject(hdcBitmap, hbmMask);
	BitBlt(hdc, x, y, nWidth, nHeight, hdcBitmap, 0, 0, SRCAND);

	// Copy the colour image to the destination DC.
	BitBlt(hdc, x, y, nWidth, nHeight, hdcColourImage, 0, 0, SRCPAINT);

	// Clean up
	SelectObject(hdcBitmap, NULL);
	SelectObject(hdcColourImage, NULL);
	DeleteObject(hbmColourImage);
	DeleteDC(hdcBitmap);
	DeleteDC(hdcColourImage);
}

void PrintMesh(Mesh &mesh) {

	DebugPrint(_T("mesh.setNumVerts( %i );\n"), mesh.getNumVerts());
	DebugPrint(_T("mesh.setNumFaces( %i );\n"), mesh.getNumFaces());
	int i;
	for (i = 0; i < mesh.getNumVerts(); i++)
		DebugPrint(_T("mesh.setVert(%i,    %f * x,    %f * y,    %f * z);\n"), i, mesh.verts[i].x, mesh.verts[i].y, mesh.verts[i].z);
	for (i = 0; i < mesh.getNumFaces(); i++) {
		int sm = 1 >> mesh.faces[i].getSmGroup();
		Face &face = mesh.faces[i];
		DebugPrint(_T("MakeFace(mesh.faces[ %i],  %i,  %i, %i, %i, %i, %i,  %i, %i);\n"),
			i,
			face.getVert(0),
			face.getVert(1),
			face.getVert(2),
			face.getEdgeVis(0),
			face.getEdgeVis(1),
			face.getEdgeVis(2),
			sm,
			face.getMatID());
	}

}

BOOL CopyMeshFromNode(TimeValue t, INode *oldnode, INode* newnode, Mesh &catmesh, Point3 dim)
{
	//////////////////////////////////////////////////////////////////////////
	// Get the top of the current modifier stack
	DbgAssert(oldnode && newnode);

	Object *wsobj = newnode->EvalWorldState(0).obj;
	if (wsobj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0))) {

		TriObject *triObject = (TriObject *)wsobj->ConvertToType(0, Class_ID(TRIOBJ_CLASS_ID, 0));
		int i;
		catmesh = Mesh(triObject->GetMesh());

		{
			Matrix3 tmOffset = newnode->GetObjectTM(t) * Inverse(oldnode->GetObjectTM(t));
			// Transform the mesh
			for (i = 0; i < catmesh.getNumVerts(); i++)
				catmesh.verts[i] = catmesh.verts[i] * tmOffset;
		}
		// Scale everything up by CATUnits
		for (i = 0; i < catmesh.getNumVerts(); i++)
			catmesh.verts[i] = (catmesh.verts[i] / dim);

		PrintMesh(catmesh);
	}
	return TRUE;
}

class PostPatchParamBlock : public PostPatchProc
{
public:
	IParamBlock2* mpOrigBlock;

	PostPatchParamBlock(IParamBlock2* pblock)
		: mpOrigBlock(pblock)
	{
	}

	int Proc(RemapDir& remap)
	{
		if (mpOrigBlock == NULL)
			return TRUE;

		IParamBlock2* pClonedBlock = static_cast<IParamBlock2*>(remap.FindMapping(mpOrigBlock));
		if (pClonedBlock == NULL)
			return TRUE;

		// ParamBlocks do not seem to want to clone their reftarg pointers so we force it to happen
		for (int j = 0; j < mpOrigBlock->NumParams(); j++) {
			ParamID id = mpOrigBlock->IndextoID(j);
			ParamType2 type = mpOrigBlock->GetParameterType(id);
			if (type == TYPE_REFTARG_TAB) {
				for (int k = 0; k < mpOrigBlock->Count(id); k++) {
					ReferenceTarget *ref = mpOrigBlock->GetReferenceTarget(id, 0, k);
					ReferenceTarget *clonedref = pClonedBlock->GetReferenceTarget(id, 0, k);
					if (ref) {
						RefTargetHandle newTarg = remap.FindMapping(ref);
						DbgAssert(newTarg != NULL);
						if (newTarg && (newTarg != clonedref)) {
							pClonedBlock->SetValue(id, 0, newTarg, k);
						}
					}
				}
			}
			else if (type == TYPE_REFTARG) {
				ReferenceTarget *ref = mpOrigBlock->GetReferenceTarget(id);
				ReferenceTarget *clonedref = pClonedBlock->GetReferenceTarget(id);
				if (ref) {
					RefTargetHandle newTarg = remap.FindMapping(ref);
					DbgAssert(newTarg != NULL);
					if (newTarg && (newTarg != clonedref)) {
						pClonedBlock->SetValue(id, 0, newTarg);
					}
				}
			}
		}

		return TRUE;
	}
};

IParamBlock2*	CloneParamBlock(IParamBlock2 *pblock, RemapDir& remap)
{
	IParamBlock2* clonedpblock = (IParamBlock2*)pblock->Clone(remap);

	remap.AddPostPatchProc(new PostPatchParamBlock(pblock), true);
	return clonedpblock;
}

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

// GetLastWriteTime - Retrieves the last-write time and converts
//                    the time to a string
//
// Return value - TRUE if successful, FALSE otherwise
// hFile      - Valid file handle
// lpszString - Pointer to buffer to receive string

BOOL GetLastWriteTime(HANDLE hFile, LPTSTR lpszString, DWORD dwSize)
{
	FILETIME ftCreate, ftAccess, ftWrite;
	SYSTEMTIME stUTC, stLocal;
	DWORD dwRet;

	// Retrieve the file times for the file.
	if (!GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite))
		return FALSE;

	// Convert the last-write time to local time.
	FileTimeToSystemTime(&ftWrite, &stUTC);
	SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

	// Build a string showing the date and time.
	dwRet = StringCchPrintf(lpszString, dwSize,
		TEXT("%02d/%02d/%d  %02d:%02d"),
		stLocal.wMonth, stLocal.wDay, stLocal.wYear,
		stLocal.wHour, stLocal.wMinute);

	if (S_OK == dwRet)
		return TRUE;
	else return FALSE;
}

int GetFileModifedTime(const TSTR& file, TSTR &out_time)
{
	HANDLE hFile;
	TCHAR szBuf[MAX_PATH];

	hFile = CreateFile(file.data(), GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, 0, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		return 0;
	}
	if (GetLastWriteTime(hFile, szBuf, MAX_PATH)) {
		out_time = szBuf;
		CloseHandle(hFile);
		return 1;
	}
	CloseHandle(hFile);
	return 0;
}

// BuildMesh() exported from 3dsmax
void MakeFace(Face &face, int a, int b, int c, int ab, int bc, int ca, DWORD sg, MtlID id) {
	int sm = 1 << sg;
	face.setVerts(a, b, c);
	face.setSmGroup(sm);
	face.setEdgeVisFlags(ab, bc, ca);
	face.setMatID(id);
}

void InitCATIcons()
{
	hIcons = ImageList_Create(24, 24, ILC_COLOR24 | ILC_MASK, 24, 0);
	LoadMAXFileIcon(_T("CAT"), hIcons, kBackground, FALSE);
}

void ReleaseCATIcons()
{
	if (hIcons != NULL)
	{
		ImageList_Destroy(hIcons);
		hIcons = NULL;
	}
}
