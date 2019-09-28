

/*

Copyright [2010] Autodesk, Inc.  All rights reserved.

Use of this software is subject to the terms of the Autodesk license agreement 
provided at the time of installation or download, or which otherwise accompanies 
this software in either electronic or hard copy form.   

*/

#include "ToolRegularMap.h"
#include "unwrap.h"
#include "modsres.h"

RegularMapRestore::RegularMapRestore(RegularMap *m, bool recomputeLocalData, MeshTopoData *md) : mRMd(NULL)
{
	map = m;
	map->GetData(mUBorderEdges,mUProcessedFaces);
	mUGeomEdge = map->GetLocalData()->GetGeomEdgeSel();
	mUTVEdge = map->GetLocalData()->GetTVEdgeSel();
	map->GetCurrentCluster(mUCurrentCluster);
	mRecomputeLocalData = recomputeLocalData;
	mUMd = md;
}
	
void RegularMapRestore::Restore(int isUndo)
{
	if (isUndo)
	{
		map->GetData(mRBorderEdges,mRProcessedFaces);
		mRGeomEdge = map->GetLocalData()->GetGeomEdgeSel();
		mRTVEdge = map->GetLocalData()->GetTVEdgeSel();
		map->GetCurrentCluster(mRCurrentCluster);
		mRMd = map->GetLocalData();
	}

	if (mRecomputeLocalData)
		map->Init(map->GetMod(), mUMd);

	map->SetData(mUBorderEdges,mUProcessedFaces);

	map->GetLocalData()->SetTVEdgeSel(mUTVEdge);
	map->GetLocalData()->SetGeomEdgeSel(mUGeomEdge);
	map->SetCurrentCluster(mUCurrentCluster);

	map->GetMod()->NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
	if (map->GetMod()->hView) map->GetMod()->InvalidateView();

}
void RegularMapRestore::Redo()
{

	if (mRecomputeLocalData)
		map->Init(map->GetMod(), mRMd);

	map->SetData(mRBorderEdges,mRProcessedFaces);

	map->GetLocalData()->SetTVEdgeSel(mRTVEdge);
	map->GetLocalData()->SetGeomEdgeSel(mRGeomEdge);

	map->SetCurrentCluster(mRCurrentCluster);

	map->GetMod()->NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
	if (map->GetMod()->hView) map->GetMod()->InvalidateView();


}

TSTR RegularMapRestore::Description()
{
	return TSTR(GetString(IDS_UNFOLDMAP));
}