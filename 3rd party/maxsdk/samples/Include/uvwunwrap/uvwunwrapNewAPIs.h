/*

Copyright 2015 Autodesk, Inc.  All rights reserved.

Use of this software is subject to the terms of the Autodesk license agreement provided at
the time of installation or download, or which otherwise accompanies this software in either
electronic or hard copy form.

*/

#pragma once

#include "uvwunwrapExport.h"

// forward declarations
class ClusterNewPackData;
class ABFPeelData;
class IUnwrapInternal;
class ILSCMClusterData;
class IMeshTopoData;

/*! \remarks This method is used internally. It creates an internal data structure used by the new unwrap pack algorithm.
This was exposed so the SDK sample -- uvwunwrap modifier could take advantage of the new pack algorithm and is not needed
by 3rd party plug-in developers. */
UNWRAP_EXPORT ClusterNewPackData* CreateClusterNewPackData();

/*! \remarks This method is used internally. It destroys the internal data structure created by CreateClusterNewPackData().
This was exposed so the SDK sample -- uvwunwrap modifier could take advantage of the new pack algorithm and is not needed
by 3rd party plug-in developers. */
UNWRAP_EXPORT void DestroyClusterNewPackData(ClusterNewPackData* pData);

/*! \remarks This method is used internally. It packs the uvw clusters based on the new pack algorithm.
This was exposed so the SDK sample -- uvwunwrap modifier could take advantage of the new pack algorithm and is not needed
by 3rd party plug-in developers. */
UNWRAP_EXPORT BOOL LayoutClustersXY(float spacing, BOOL rotateClusters, BOOL combineClusters, IUnwrapInternal* pInternalUnwrap);


/*! \remarks This method is used internally. It creates an internal data structure used by the new peel algorithm.
This was exposed so the SDK sample -- uvwunwrap modifier could take advantage of the new peel algorithm and is not needed
by 3rd party plug-in developers. */
UNWRAP_EXPORT ABFPeelData* CreateABFPeelData();

/*! \remarks This method is used internally. It destroys the internal data structure created by CreateABFPeelData().
This was exposed so the SDK sample -- uvwunwrap modifier could take advantage of the new peel algorithm and is not needed
by 3rd party plug-in developers. */
UNWRAP_EXPORT void DestroyABFPeelData(ABFPeelData* pData);

/*! \remarks This method is used internally. It uses Linear Angle Based Flattenning algorithm to calculate the face angles.
This was exposed so the SDK sample -- uvwunwrap modifier could take advantage of the LABF algorithm and is not needed
by 3rd party plug-in developers. */
UNWRAP_EXPORT bool ComputeLABFFaceAngles(ILSCMClusterData* pLSCMCluster, IMeshTopoData *md, float abfErrBound, int maxIterationNumber);

/*! \remarks This method is used internally. It uses ABF++ algorithm to calculate the face angles.
This was exposed so the SDK sample -- uvwunwrap modifier could take advantage of the ABF++ algorithm and is not needed
by 3rd party plug-in developers.*/
UNWRAP_EXPORT bool ComputeABFPlusPlusFaceAngles(ILSCMClusterData* pLSCMCluster, IMeshTopoData *md, bool useSimplifyModel, int maxIterationNumber);

/*! \remarks This method prompts file overwrite warning in RenderUVTemplate dialog. 
When a UV tile file has existed, the dialog popups for user to choose overwrite it or not. */
UNWRAP_EXPORT bool RenderUV_ShowFileOverwriteDlg(const TCHAR *name, HWND hParent);

