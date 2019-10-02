#include "stdafx.h"
#pragma hdrstop

#include "exporter.h"
#include "..\..\..\editors\ECore\Editor\EditObject.h"
#include "MeshExpUtility.h"
//----------------------------------------------------------------------------------------------

BOOL CExporter::ExportSkin(INode* pNode, LPCTSTR fname)
{
	m_Style				= eExportSkin;
//	ScanBones			(pNode);

	int i				= U.ipanel->GetSelNodeCount();
	while (i--)			ScanMesh(U.ipanel->GetSelNode(i));

	if (!Capture())		return FALSE;
	if (m_bHasError)	return FALSE;
	CEditableObject* OBJECT = xr_new<CEditableObject>(TEXT("object"));
	OBJECT->SetVersionToCurrent(TRUE,TRUE);
	if (!OBJECT->ImportMAXSkeleton(this)){
		xr_delete(OBJECT);
		return FALSE;
	}
	OBJECT->Optimize	();
	OBJECT->SaveObject	(fname);
	xr_delete			(OBJECT);
	return TRUE;
};
