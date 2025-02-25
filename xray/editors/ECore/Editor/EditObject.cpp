//----------------------------------------------------
// file: EditObject.cpp
//----------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#if defined (_MAX_EXPORT)
	#include "..\..\..\xrEngine\fmesh.h"
#else
	#include "..\..\..\xrEngine\fmesh.h"
#endif

#include "EditObject.h"
#include "EditMesh.h"

#ifdef _EDITOR
	#include "motion.h"
	#include "bone.h"
	#include "ImageManager.h"
#endif

// mimimal bounding box size
float g_MinBoxSize 	= 0.05f;


CEditableObject::CEditableObject(LPCTSTR name)
{
	m_LibName		= name;

	m_objectFlags.zero	();
    m_ObjectVersion	= 0;

#ifdef _EDITOR
    vs_SkeletonGeom	= 0;
#endif
	m_Box.invalidate();

    m_LoadState.zero();

    m_ActiveSMotion = 0;

	t_vPosition.set	(0.f,0.f,0.f);
    t_vScale.set   	(1.f,1.f,1.f);
    t_vRotate.set  	(0.f,0.f,0.f);

	a_vPosition.set	(0.f,0.f,0.f);
    a_vRotate.set  	(0.f,0.f,0.f);

    bOnModified		= false;

    m_RefCount		= 0;

    m_LODShader		= 0;
    
    m_CreateName	= TEXT("unknown");
    m_CreateTime	= 0;
	m_ModifName		= TEXT("unknown");
    m_ModifTime		= 0;
}

CEditableObject::~CEditableObject()
{
    ClearGeometry();
}
//----------------------------------------------------

void CEditableObject::VerifyMeshNames()
{
	int idx=0;
	string1024 	nm,pref; 
    for(EditMeshIt m_def=m_Meshes.begin();m_def!=m_Meshes.end();m_def++){
		wcscpy_s	(pref,(*m_def)->m_Name.size()?(*m_def)->m_Name.c_str():TEXT("mesh"));
        _Trim	(pref);
		wcscpy	(nm,pref);
		while (FindMeshByName(nm,*m_def))
			wsprintf(nm,TEXT("%s%2d"),pref,idx++);
        (*m_def)->SetName(nm);
    }
}

bool CEditableObject::ContainsMesh(const CEditableMesh* m)
{
    VERIFY(m);
    for(EditMeshIt m_def=m_Meshes.begin();m_def!=m_Meshes.end();m_def++)
        if (m==(*m_def)) return true;
    return false;
}

CEditableMesh* CEditableObject::FindMeshByName	(LPCTSTR name, CEditableMesh* Ignore)
{
    for(EditMeshIt m=m_Meshes.begin();m!=m_Meshes.end();m++)
        if ((Ignore!=(*m))&&(_wcsicmp((*m)->Name().c_str(),name)==0)) return (*m);
    return 0;
}

void CEditableObject::ClearGeometry ()
{
#ifdef _EDITOR
    OnDeviceDestroy();
#endif
    if (!m_Meshes.empty())
        for(EditMeshIt 	m=m_Meshes.begin(); m!=m_Meshes.end();m++)xr_delete(*m);
    if (!m_Surfaces.empty())
        for(SurfaceIt 	s_it=m_Surfaces.begin(); s_it!=m_Surfaces.end(); s_it++)
            xr_delete(*s_it);
    m_Meshes.clear();
    m_Surfaces.clear();
#ifdef _EDITOR
    // bones
    for(BoneIt b_it=m_Bones.begin(); b_it!=m_Bones.end();b_it++)xr_delete(*b_it);
    m_Bones.clear();
    // skeletal motions
    for(SMotionIt s_it=m_SMotions.begin(); s_it!=m_SMotions.end();s_it++) xr_delete(*s_it);
    m_SMotions.clear();
#endif
    m_ActiveSMotion = 0;
}

int CEditableObject::GetFaceCount(){
	int cnt=0;
    for(EditMeshIt m = m_Meshes.begin();m!=m_Meshes.end();m++)
        cnt+=(*m)->GetFaceCount();
	return cnt;
}

int CEditableObject::GetSurfFaceCount(LPCTSTR surf_name){
	int cnt=0;
    CSurface* surf = FindSurfaceByName(surf_name);
    for(EditMeshIt m = m_Meshes.begin();m!=m_Meshes.end();m++)
        cnt+=(*m)->GetSurfFaceCount(surf);
	return cnt;
}

int CEditableObject::GetVertexCount(){
	int cnt=0;
    for(EditMeshIt m = m_Meshes.begin();m!=m_Meshes.end();m++)
        cnt+=(*m)->GetVertexCount();
	return cnt;
}

void CEditableObject::UpdateBox(){
	VERIFY(!m_Meshes.empty());
    EditMeshIt m = m_Meshes.begin();
    m_Box.invalidate();
    for(;m!=m_Meshes.end();m++){
        Fbox meshbox;
        (*m)->GetBox(meshbox);
        for(int i=0; i<8; i++){
            Fvector pt;
            meshbox.getpoint(i, pt);
            m_Box.modify(pt);
        }
    }
}
//----------------------------------------------------
void CEditableObject::RemoveMesh(CEditableMesh* mesh){
	EditMeshIt m_it = std::find(m_Meshes.begin(),m_Meshes.end(),mesh);
    VERIFY(m_it!=m_Meshes.end());
	m_Meshes.erase(m_it);
    xr_delete(mesh);
}

void CEditableObject::TranslateToWorld(const Fmatrix& parent)
{
	EditMeshIt m = m_Meshes.begin();
	for(;m!=m_Meshes.end();m++) (*m)->Transform( parent );
#ifdef _EDITOR
	OnDeviceDestroy();
#endif
	UpdateBox();
}

CSurface*	CEditableObject::FindSurfaceByName(LPCTSTR surf_name, int* s_id){
	for(SurfaceIt s_it=m_Surfaces.begin(); s_it!=m_Surfaces.end(); s_it++)
    	if (_wcsicmp((*s_it)->_Name(),surf_name)==0){ if (s_id) *s_id=s_it-m_Surfaces.begin(); return *s_it;}
    return 0;
}

LPCTSTR CEditableObject::GenerateSurfaceName(LPCTSTR base_name)
{
	static string1024 nm;
	wcscpy_s(nm, base_name);
	if (FindSurfaceByName(nm)){
		DWORD idx=0;
		do{
			wsprintf(nm,TEXT("%s_%d"),base_name,idx);
			idx++;
		}while(FindSurfaceByName(nm));
	}
	return nm;
}

bool CEditableObject::VerifyBoneParts()
{
	U8Vec b_use(BoneCount(),0);
    for (BPIt bp_it=m_BoneParts.begin(); bp_it!=m_BoneParts.end(); bp_it++)
        for (int i=0; i<int(bp_it->bones.size()); i++){
        	int idx = FindBoneByNameIdx(bp_it->bones[i].c_str());
            if (idx==-1){
            	bp_it->bones.erase(bp_it->bones.begin()+i);
            	i--;
            }else{
	        	b_use[idx]++;
            }
        }

    for (U8It u_it=b_use.begin(); u_it!=b_use.end(); u_it++)
    	if (*u_it!=1) return false;
    return true;
}

void CEditableObject::PrepareOGFDesc(ogf_desc& desc)
{
	string512			tmp;
	desc.source_file	= m_LibName.c_str();
    desc.create_name	= m_CreateName.c_str();
    desc.create_time	= m_CreateTime;
    desc.modif_name		= m_ModifName.c_str();
    desc.modif_time		= m_ModifTime;
    desc.build_name		= strconcat(sizeof(tmp),tmp,TEXT("\\\\"),Core.CompName,TEXT("\\"),Core.UserName);
    ctime				(&desc.build_time);
}

void CEditableObject::SetVersionToCurrent(BOOL bCreate, BOOL bModif)
{
	string512			tmp;
	if (bCreate){
		m_CreateName	= strconcat(sizeof(tmp),tmp, TEXT("\\\\"),Core.CompName, TEXT("\\"),Core.UserName);
		m_CreateTime	= time(NULL);
	}
	if (bModif){
		m_ModifName		= strconcat(sizeof(tmp),tmp, TEXT("\\\\"),Core.CompName, TEXT("\\"),Core.UserName);
		m_ModifTime		= time(NULL);
	}
}

void CEditableObject::GetFaceWorld(const Fmatrix& parent, CEditableMesh* M, int idx, Fvector* verts)
{
	const Fvector* PT[3];
	M->GetFacePT(idx, PT);
	parent.transform_tiny(verts[0],*PT[0]);
    parent.transform_tiny(verts[1],*PT[1]);
	parent.transform_tiny(verts[2],*PT[2]);
}

void CEditableObject::Optimize()
{
    for(EditMeshIt m_def=m_Meshes.begin();m_def!=m_Meshes.end();m_def++){
    	(*m_def)->OptimizeMesh	(false);
        (*m_def)->RebuildVMaps	();
    }
}

bool CEditableObject::Validate()
{
	bool bRes = true;
    for(SurfaceIt 	s_it=m_Surfaces.begin(); s_it!=m_Surfaces.end(); s_it++)
        if (false==(*s_it)->Validate()){ 
        	Msg(TEXT("!Invalid surface found: Object [%s], Surface [%s]."),GetName(),(*s_it)->_Name());
        	bRes=false;
        }
    for(EditMeshIt m_def=m_Meshes.begin();m_def!=m_Meshes.end();m_def++)
        if (false==(*m_def)->Validate()){ 
        	Msg(TEXT("!Invalid mesh found: Object [%s], Mesh [%s]."),m_LibName.c_str(),(*m_def)->Name().c_str());
        	bRes=false;
        }
    return bRes;
}
//----------------------------------------------------------------------------

