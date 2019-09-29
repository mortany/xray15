#ifndef EditObjectH
#define EditObjectH

#include "Bone.h"
#include "Motion.h"
//----------------------------------------------------
struct 	SRayPickInfo;
class 	CEditableMesh;
class 	CFrustum;
class 	CCustomMotion;
class	CBone;
class	Shader;
class	Mtl;
class	CExporter;
class	CMayaTranslator;
struct	st_ObjectDB;
struct	SXRShaderData;
struct  ogf_desc;
class	CCustomObject;

#ifndef _EDITOR
	class PropValue;
	#define ref_shader LPVOID
#endif

#ifdef _LW_IMPORT
	#include <lwobjimp.h>
	#include <lwsurf.h>
#endif

#define LOD_SHADER_NAME 		"details\\lod"
#define LOD_SAMPLE_COUNT 		8
#define LOD_IMAGE_SIZE 			64
#define RENDER_SKELETON_LINKS	4

// refs
class XRayMtl;
class SSimpleImage;

class ECORE_API CSurface
{
    u32				m_GameMtlID;
    ref_shader		m_Shader;
	enum ERTFlags 
	{ 
        rtValidShader	= (1<<0),
	};
public:
	enum EFlags{
    	sf2Sided		= (1<<0),
    };
    shared_str			m_Name;
    shared_str			m_Texture;	//
    shared_str			m_VMap;		//
    shared_str			m_ShaderName;
    shared_str			m_ShaderXRLCName;
    shared_str			m_GameMtlName;
    Flags32			m_Flags;
    u32				m_dwFVF;
#ifdef _MAX_EXPORT
	u32				mid;
	Mtl*			mtl;
#endif
#ifdef _LW_IMPORT
	LWSurfaceID		surf_id;
#endif
    Flags32			m_RTFlags;
	u32				tag;
    SSimpleImage*	m_ImageData;
public:
	CSurface		()
	{
    	m_GameMtlName="default";
        m_ImageData	= 0;
		m_Shader	= 0;
        m_RTFlags.zero	();
		m_Flags.zero	();
		m_dwFVF		= 0;
#ifdef _MAX_EXPORT
		mtl			= 0;
		mid			= 0;
#endif
#ifdef _LW_IMPORT
		surf_id		= 0;
#endif
		tag			= 0;
	}
    IC bool			Validate		()
    {
    	return (0!=xr_strlen(m_Texture))&&(0!=xr_strlen(m_ShaderName));
    }
    IC LPCTSTR		_Name			()const {return *m_Name;}
    IC LPCTSTR		_ShaderName		()const {return *m_ShaderName;}
    IC LPCTSTR		_GameMtlName	()const {return *m_GameMtlName;}
    IC LPCTSTR		_ShaderXRLCName	()const {return *m_ShaderXRLCName;}
    IC LPCTSTR		_Texture		()const {return *m_Texture;}
    IC LPCTSTR		_VMap			()const {return *m_VMap;}
    IC u32			_FVF			()const {return m_dwFVF;}
    IC void			SetName			(LPCTSTR name){m_Name=name;}
	IC void			SetShader		(LPCTSTR name)
	{
		R_ASSERT2(name&&name[0],"Empty shader name."); 
		m_ShaderName=name; 
	}
    IC void 		SetShaderXRLC	(LPCTSTR name){m_ShaderXRLCName=name;}
    IC void			SetGameMtl		(LPCTSTR name){m_GameMtlName=name;}
    IC void			SetFVF			(u32 fvf){m_dwFVF=fvf;}
    IC void			SetTexture		(LPCTSTR name){string512 buf; strcpy_s(buf,name); if(strext(buf)) *strext(buf)=0; m_Texture=buf;}
    IC void			SetVMap			(LPCTSTR name){m_VMap=name;}
};

DEFINE_VECTOR	(CSurface*,SurfaceVec,SurfaceIt);
DEFINE_VECTOR	(CEditableMesh*,EditMeshVec,EditMeshIt);
DEFINE_VECTOR	(COMotion*,OMotionVec,OMotionIt);
DEFINE_VECTOR	(CSMotion*,SMotionVec,SMotionIt);

struct ECORE_API SBonePart{
	shared_str 		alias;
    RStringVec 		bones;
};
DEFINE_VECTOR(SBonePart,BPVec,BPIt);

const u32 FVF_SV	= D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_NORMAL;

class ECORE_API CEditableObject{
	friend class CSceneObject;
	friend class CEditableMesh;
    friend class TfrmPropertiesEObject;
    friend class CSector;
    friend class TUI_ControlSectorAdd;
	friend class ELibrary;
	friend class TfrmEditLibrary;
	friend class MeshExpUtility;

// desc
	shared_str 		m_CreateName;
    time_t			m_CreateTime;
	shared_str 		m_ModifName;
    time_t			m_ModifTime;
    
// general
	xr_string		m_ClassScript;

	SurfaceVec		m_Surfaces;
	EditMeshVec		m_Meshes;

    ref_shader		m_LODShader;

	// skeleton
	BoneVec			m_Bones;
	SMotionVec		m_SMotions;
    BPVec			m_BoneParts;
    CSMotion*		m_ActiveSMotion;
public:
    SAnimParams				m_SMParam;
    xr_vector<shared_str>	m_SMotionRefs;
    shared_str				m_LODs;
public:
	// options
	Flags32			m_objectFlags;
	enum{
		eoDynamic 	 	= (1<<0),			
		eoProgressive 	= (1<<1),			
        eoUsingLOD		= (1<<2),			
        eoHOM			= (1<<3),			
        eoMultipleUsage	= (1<<4),			
        eoSoundOccluder	= (1<<5),           
		eoFORCE32		= u32(-1)           
    };
    IC BOOL			IsDynamic				(){return m_objectFlags.is(eoDynamic);}
    IC BOOL			IsStatic				(){return !m_objectFlags.is(eoSoundOccluder)&&!m_objectFlags.is(eoDynamic)&&!m_objectFlags.is(eoHOM)&&!m_objectFlags.is(eoMultipleUsage);}
    IC BOOL			IsMUStatic				(){return !m_objectFlags.is(eoSoundOccluder)&&!m_objectFlags.is(eoDynamic)&&!m_objectFlags.is(eoHOM)&&m_objectFlags.is(eoMultipleUsage);}
private:
	// bounding volume
	Fbox 			m_Box;
public:
    // temp variable for actor
	Fvector 		a_vPosition;
    Fvector			a_vRotate;

    // temp variables for transformation
	Fvector 		t_vPosition;
    Fvector			t_vScale;
    Fvector			t_vRotate;

    bool			bOnModified;
    IC bool			IsModified				(){return bOnModified;}
    IC void 		Modified				(){bOnModified=true;}

    AnsiString		m_LoadName;
    int				m_RefCount;
protected:
    int				m_ObjectVersion;

    void 			ClearGeometry			();

	void 			PrepareBones			();
    void			DefferedLoadRP			();
    void			DefferedUnloadRP		();

	void __stdcall  OnChangeTransform		(PropValue* prop);
    void __stdcall 	OnChangeShader			(PropValue* prop);
public:
	enum{
	    LS_RBUFFERS	= (1<<0),
//	    LS_GEOMETRY	= (1<<1),
    };
    Flags32			m_LoadState;

	AnsiString		m_LibName;
public:
    // constructor/destructor methods
					CEditableObject			(LPCTSTR name);
	virtual 		~CEditableObject		();

    LPCTSTR			GetName					(){ return m_LibName.c_str();}

	void			SetVersionToCurrent		(BOOL bCreate, BOOL bModif);

    void			Optimize				();

    IC EditMeshIt	FirstMesh				()	{return m_Meshes.begin();}
    IC EditMeshIt	LastMesh				()	{return m_Meshes.end();}
    IC EditMeshVec& Meshes					()	{return m_Meshes; }
    IC int			MeshCount				()	{return m_Meshes.size();}
	IC void			AppendMesh				(CEditableMesh* M){m_Meshes.push_back(M);}
    IC SurfaceVec&	Surfaces				()	{return m_Surfaces;}
    IC SurfaceIt	FirstSurface			()	{return m_Surfaces.begin();}
    IC SurfaceIt	LastSurface				()	{return m_Surfaces.end();}
    IC int			SurfaceCount			()	{return m_Surfaces.size();}
    IC int 			Version 				() 	{return m_ObjectVersion;}

    // LOD
	xr_string		GetLODTextureName		();
    LPCTSTR			GetLODShaderName		(){return LOD_SHADER_NAME;}
    void			GetLODFrame				(int frame, Fvector p[4], Fvector2 t[4], const Fmatrix* parent=0);

    // skeleton
    IC BPIt			FirstBonePart			()	{return m_BoneParts.begin();}
    IC BPIt			LastBonePart			()	{return m_BoneParts.end();}
	IC BPVec&		BoneParts				()	{return m_BoneParts;}
    IC int			BonePartCount			()	{return m_BoneParts.size();}
    IC BPIt			BonePart				(CBone* B);

    IC BoneIt		FirstBone				()	{return m_Bones.begin();}
    IC BoneIt		LastBone				()	{return m_Bones.end();}
	IC BoneVec&		Bones					()	{return m_Bones;}
    IC int			BoneCount				()	{return m_Bones.size();}
    shared_str		BoneNameByID			(int id);
    int				GetRootBoneID			();
    int				PartIDByName			(LPCTSTR name);
    IC CBone*		GetBone					(u32 idx){VERIFY(idx<m_Bones.size()); return m_Bones[idx];}
    void			GetBoneWorldTransform	(u32 bone_idx, float t, CSMotion* motion, Fmatrix& matrix);
    IC SMotionIt	FirstSMotion			()	{return m_SMotions.begin();}
    IC SMotionIt	LastSMotion				()	{return m_SMotions.end();}
	SMotionVec&		SMotions				()	{return m_SMotions;}
    IC int			SMotionCount 			()	{return m_SMotions.size();}
    IC bool			IsAnimated	 			()	{return SMotionCount() || m_SMotionRefs.size();}
//.    IC LPCTSTR		SMotionRefs				()	{return *m_SMotionRefs; }
    IC void			SkeletonPlay 			()	{m_SMParam.Play();}
    IC void			SkeletonStop 			()	{m_SMParam.Stop();}
    IC void			SkeletonPause 			(bool val)	{m_SMParam.Pause(val);}

///    IC bool			CheckVersion			()  {if(m_LibRef) return (m_ObjVer==m_LibRef->m_ObjVer); return true;}
    // get object properties methods

	IC xr_string&	GetClassScript			()	{return m_ClassScript;}
    IC const Fbox&	GetBox					() 	{return m_Box;}
    IC LPCTSTR		GetLODs					()	{return m_LODs.c_str();}

    // animation
    IC bool			IsSkeleton				()	{return !!m_Bones.size();}
    IC bool			IsSMotionActive			()	{return IsSkeleton()&&m_ActiveSMotion; }
    CSMotion*		GetActiveSMotion		()	{return m_ActiveSMotion; }
	void			SetActiveSMotion		(CSMotion* mot);
	bool 			CheckBoneCompliance		(CSMotion* M);
    bool			VerifyBoneParts			();
    void			OptimizeSMotions		();

	bool 			LoadBoneData			(IReader& F);
	void 			SaveBoneData			(IWriter& F);
    void			ResetBones				();
	CSMotion*		ResetSAnimation			(bool bGotoBindPose=true);
    void			CalculateAnimation		(CSMotion* motion);
    void			CalculateBindPose		();
	void			GotoBindPose			();
    void			OnBindTransformChange	();

    // statistics methods
	void 			GetFaceWorld			(const Fmatrix& parent, CEditableMesh* M, int idx, Fvector* verts);
    int 			GetFaceCount			();
	int 			GetVertexCount			();
    int 			GetSurfFaceCount		(LPCTSTR surf_name);

    // render methods
	void 			Render					(const Fmatrix& parent, int priority, bool strictB2F);
	void 			RenderSelection			(const Fmatrix& parent, CEditableMesh* m=0, CSurface* s=0, u32 c=0x40E64646);
 	void 			RenderEdge				(const Fmatrix& parent, CEditableMesh* m=0, CSurface* s=0, u32 c=0xFFC0C0C0);
	void 			RenderBones				(const Fmatrix& parent);
	void 			RenderAnimation			(const Fmatrix& parent);
	void 			RenderSingle			(const Fmatrix& parent);
	void 			RenderSkeletonSingle	(const Fmatrix& parent);
	void 			RenderLOD				(const Fmatrix& parent);

    // update methods
	void 			OnFrame					();
	void 			UpdateBox				();
	void		    EvictObject				();

    // pick methods
	bool 			RayPick					(float& dist, const Fvector& S, const Fvector& D, const Fmatrix& inv_parent, SRayPickInfo* pinf=0);
    // change position/orientation methods
	void 			TranslateToWorld		(const Fmatrix& parent);

    // clone/copy methods
    void			RemoveMesh				(CEditableMesh* mesh);

    bool			RemoveSMotion			(LPCTSTR name);
    bool			RenameSMotion			(LPCTSTR old_name, LPCTSTR new_name);
    bool			AppendSMotion			(LPCTSTR fname, SMotionVec* inserted=0);
    void			ClearSMotions			();
    bool			SaveSMotions			(LPCTSTR fname);

    // load/save methods
	//void 			LoadMeshDef				(FSChunkDef *chunk);
	bool 			Reload					();
	bool 			Load					(LPCTSTR fname);
	bool 			LoadObject				(LPCTSTR fname);
	bool 			SaveObject				(LPCTSTR fname);
  	bool 			Load					(IReader&);
	void 			Save					(IWriter&);
	bool			Import_LWO				(LPCTSTR fname, bool bNeedOptimize);

    // contains methods
    CEditableMesh* 	FindMeshByName			(LPCTSTR name, CEditableMesh* Ignore=0);
    void			VerifyMeshNames			();
    bool 			ContainsMesh			(const CEditableMesh* m);
	CSurface*		FindSurfaceByName		(LPCTSTR surf_name, int* s_id=0);
    int				FindBoneByNameIdx		(LPCTSTR name);
    BoneIt			FindBoneByNameIt		(LPCTSTR name);
    CBone*			FindBoneByName			(LPCTSTR name);
    int				GetSelectedBones		(BoneVec& sel_bones);
    u16				GetBoneIndexByWMap		(LPCTSTR wm_name);
    CSMotion* 		FindSMotionByName		(LPCTSTR name, const CSMotion* Ignore=0);
    void			GenerateSMotionName		(char* buffer, LPCTSTR start_name, const CSMotion* M);
    bool			GenerateBoneShape		(bool bSelOnly);

    // device dependent routine
	void 			OnDeviceCreate 			();
	void 			OnDeviceDestroy			();

    // utils
    void			PrepareOGFDesc			(ogf_desc& desc);
    // skeleton
    bool			PrepareSVGeometry		(IWriter& F, u8 infl);
    bool			PrepareSVKeys			(IWriter& F);
    bool			PrepareSVDefs			(IWriter& F);
    bool			PrepareSkeletonOGF		(IWriter& F, u8 infl);
    // rigid
    bool			PrepareRigidOGF			(IWriter& F, bool gen_tb, CEditableMesh* mesh);
	// ogf
    bool			PrepareOGF				(IWriter& F, u8 infl, bool gen_tb, CEditableMesh* mesh);
	bool			ExportOGF				(LPCTSTR fname, u8 skl_infl);
    // omf
    bool			PrepareOMF				(IWriter& F);
	bool			ExportOMF				(LPCTSTR fname);
    // obj
    bool			ExportOBJ				(LPCTSTR name);

	LPCTSTR			GenerateSurfaceName		(LPCTSTR base_name);
#ifdef _MAX_EXPORT
	BOOL			ExtractTexName			(Texmap *src, LPSTR dest);
	BOOL			ParseStdMaterial		(StdMat* src, CSurface* dest);
	BOOL			ParseMultiMaterial		(MultiMtl* src, u32 mid, CSurface* dest);
	BOOL			ParseXRayMaterial		(XRayMtl* src, u32 mid, CSurface* dest);
	CSurface*		CreateSurface			(Mtl* M, u32 mat_id);
	bool			ImportMAXSkeleton		(CExporter* exporter);
#endif
	bool			ExportLWO				(LPCTSTR fname);

    bool			Validate				();
};
//----------------------------------------------------
//----------------------------------------------------
#define EOBJ_CURRENT_VERSION		0x0010
//----------------------------------------------------
#define EOBJ_CHUNK_OBJECT_BODY		0x7777
#define EOBJ_CHUNK_VERSION		  	0x0900
#define EOBJ_CHUNK_REFERENCE     	0x0902
#define EOBJ_CHUNK_FLAGS           	0x0903
#define EOBJ_CHUNK_SURFACES			0x0905
#define EOBJ_CHUNK_SURFACES2		0x0906                                 
#define EOBJ_CHUNK_SURFACES3		0x0907
#define EOBJ_CHUNK_EDITMESHES      	0x0910
#define _EOBJ_CHUNK_LIB_VERSION_   	0x0911 // obsolette
#define EOBJ_CHUNK_CLASSSCRIPT     	0x0912
#define EOBJ_CHUNK_BONES			0x0913
//#define EOBJ_CHUNK_OMOTIONS			0x0914
#define EOBJ_CHUNK_SMOTIONS			0x0916
#define EOBJ_CHUNK_SURFACES_XRLC	0x0918
#define EOBJ_CHUNK_BONEPARTS		0x0919
#define EOBJ_CHUNK_ACTORTRANSFORM	0x0920
#define EOBJ_CHUNK_BONES2			0x0921
#define EOBJ_CHUNK_DESC				0x0922
#define EOBJ_CHUNK_BONEPARTS2		0x0923
#define EOBJ_CHUNK_SMOTIONS2		0x0924
#define EOBJ_CHUNK_LODS				0x0925
#define EOBJ_CHUNK_SMOTIONS3		0x0926
//----------------------------------------------------


#endif /*_INCDEF_EditObject_H_*/


