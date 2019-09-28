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
	Functions to retrieve the standard path for various CAT preset
	files.  Each of these returns a temporary pointer to an
	internal buffer containing the full pathname requested, without
	the trailing backslash.  Successive calls to any of these
	functions will overwrite the buffer.
 **********************************************************************/
#pragma once

#define CAT_RIG_PRESET_EXT			_T("rig")
#define CAT3_RIG_PRESET_EXT			_T("rg3")
#define CAT_MOTION_PRESET_EXT		_T("cmp")
#define CAT_HANDPOSE_EXT			_T("chp")
#define CAT_CLIP_EXT				_T("clp")
#define CAT_POSE_EXT				_T("pse")
#define CAT_HTR_EXT					_T("htr")
#define CAT_BVH_EXT					_T("bvh")
#define CAT_BIP_EXT					_T("bip")
#define CAT_FBX_EXT					_T("fbx")

#define CAT_RIG_PRESET_FILTER			_T( "CAT3 Rig Preset(*.") CAT3_RIG_PRESET_EXT _T(")\0*.") CAT3_RIG_PRESET_EXT _T("\0")\
											_T("CAT Rig Preset(*.") CAT_RIG_PRESET_EXT _T(")\0*.") CAT_RIG_PRESET_EXT _T("\0\0")

#define CAT_RIG3_PRESET_FILTER			_T( "CAT3 Rig Preset(*.") CAT3_RIG_PRESET_EXT _T(")\0*.") CAT3_RIG_PRESET_EXT _T("\0\0")

#define CAT_MOTION_PRESET_FILTER		_T( "CAT Motion Preset(*.") CAT_MOTION_PRESET_EXT _T(")\0*.") CAT_MOTION_PRESET_EXT _T("\0")\
											_T("All Files\0*.*\0\0")

#define CAT_HANDPOSE_PRESET_FILTER		_T( "CAT Hand Pose Preset(*.") CAT_HANDPOSE_EXT _T(")\0*.") CAT_HANDPOSE_EXT _T("\0")\
											_T("All Files\0*.*\0\0")

#define CAT_CLIP_FILTER					_T( "CAT Clip(*.") CAT_CLIP_EXT _T(")\0*.") CAT_CLIP_EXT _T("\0")\
										    _T("All Files\0*.*\0\0")

#define CAT_POSE_FILTER					_T( "CAT Pose(*.") CAT_POSE_EXT _T(")\0*.") CAT_POSE_EXT _T("\0")\
										    _T("All Files\0*.*\0\0")

#define CAT_CLIPORPOSE_FILTER			_T( "CAT Pose(*.") CAT_POSE_EXT _T(")\0*.") CAT_POSE_EXT _T("\0")\
											_T("CAT Clip(*.") CAT_CLIP_EXT _T(")\0*.") CAT_CLIP_EXT _T("\0")\
										    _T("All Files\0*.*\0\0")

 // Mocap import is disabled for Max5
#ifndef VISUALSTUDIO6
#define CAT_FREEFORM_FILTER				_T( "CAT Pose(*.") CAT_POSE_EXT _T(")\0*.") CAT_POSE_EXT _T("\0")\
											_T("CAT Clip(*.") CAT_CLIP_EXT _T(")\0*.") CAT_CLIP_EXT _T("\0")\
											_T("BioVision BVH(*.") CAT_BVH_EXT _T(")\0*.") CAT_BVH_EXT _T("\0")\
											_T("MotionAnalysis HTR(*.") CAT_HTR_EXT _T(")\0*.") CAT_HTR_EXT _T("\0")\
											_T("Biped Bip(*.") CAT_BIP_EXT _T(")\0*.") CAT_BIP_EXT _T("\0")\
											_T("All Files\0*.*\0\0")
#else
#define CAT_FREEFORM_FILTER				_T( "CAT Pose(*.") CAT_POSE_EXT _T(")\0*.") CAT_POSE_EXT _T("\0")\
											_T("CAT Clip(*.") CAT_CLIP_EXT _T(")\0*.") CAT_CLIP_EXT _T("\0")\
											_T("All Files\0*.*\0\0")
#endif

extern TCHAR *GetRigPresetPath();
extern TCHAR *GetMotionPresetPath();
extern TCHAR *GetHandPosePresetPath();
extern TCHAR *GetClipPath();
extern TCHAR *GetPosePath();
extern TCHAR *GetPlugCFGPath(const TCHAR *filename = NULL);

BOOL GetSaveFileName(HINSTANCE hInstance, HWND hWnd, TCHAR* fileBuf, int bufSize, LPSTR filter, LPSTR defaultExtension, LPSTR fileTitle = NULL, LPSTR initialDir = NULL, LPSTR caption = NULL);
BOOL GetOpenFileName(HINSTANCE hInstance, HWND hWnd, TCHAR* fileBuf, int bufSize, LPSTR filter, LPSTR fileTitle = NULL, LPSTR initialDir = NULL, LPSTR caption = NULL);

BOOL DialogOpenRigPreset(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize);  // offers only rg3 file type
BOOL DialogOpenMotionPreset(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize, TCHAR *initialDir = NULL);
BOOL DialogOpenHandPosePreset(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize, TCHAR *initialDir = NULL);

BOOL DialogSaveRigPreset(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize, TCHAR *initialName = NULL);
BOOL DialogSaveMotionPreset(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize, TCHAR *initialName = NULL, TCHAR *initialDir = NULL);
BOOL DialogSaveHandPosePreset(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize, TCHAR *initialName = NULL, TCHAR *initialDir = NULL);

BOOL DialogOpenClip(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize, TCHAR *initialDir = NULL);
//BOOL DialogSaveClip(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize, TCHAR *initialName=NULL, TCHAR *initialDir=NULL);

BOOL DialogOpenPose(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize, TCHAR *initialDir = NULL);
//BOOL DialogSavePose(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize, TCHAR *initialName=NULL, TCHAR *initialDir=NULL);

BOOL DialogOpenClipOrPose(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize, TCHAR *initialDir = NULL);
BOOL DialogSaveClipOrPose(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize, BOOL bIsClip, TCHAR *initialName = NULL, TCHAR *initialDir = NULL);
//BOOL DialogSaveClipAndPose(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize, TCHAR *initialName=NULL, TCHAR *initialDir=NULL);

BOOL DialogOpenFreeform(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize, BOOL bIsClip, TCHAR *initialDir = NULL);
BOOL DialogSaveFreeform(HINSTANCE hInstance, HWND hWnd, TCHAR *fileBuf, int bufSize, TCHAR *initialName = NULL, TCHAR *initialDir = NULL);

void ResetLastFreeformFilterIndex();
