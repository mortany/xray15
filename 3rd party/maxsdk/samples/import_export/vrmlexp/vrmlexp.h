/**********************************************************************
*<
FILE: vrmlexp.h

DESCRIPTION:  VRML.WRL file export class defs

CREATED BY: Scott Morrison

HISTORY: created 15 February, 1996

*>	Copyright (c) 1996, All Rights Reserved.
**********************************************************************/

#include "maxtextfile.h"
#include <set>
#include <unordered_map>

// these are used for the type of output polygon
#define OUTPUT_TRIANGLES        0
#define OUTPUT_QUADS            1
#define OUTPUT_NGONS            2
#define OUTPUT_VISIBLE_EDGES    3

// Animation key types
#define KEY_FLOAT	 (1<<0)
#define KEY_POS		 (1<<1)
#define KEY_ROT		 (1<<2)
#define KEY_SCL		 (1<<3)
#define KEY_COLOR	 (1<<4)
#define KEY_COORD	 (1<<5)
#define KEY_TIMER	 (1<<6)
#define KEY_TIMER_SCRIPT (1<<7)
#define KEY_SCL_ORI  (1<<8)

#define VRBL_EXPORT_CLASS_ID	Class_ID(0xACAD8213, 0)

class BitmapTex;
class CallbackTable;
class VRMLInsObject;
class LODObject;
class INodeList;

#include "vrml_api.h"

// Round numbers near zero to zero.  This help reduce VRML file size.
inline float roundToZero(float f)
{
	// This is used often, so we avoid calling fabs
	if (f < 0.0f) {
		if (f > -1.0e-5) {
			return 0.0f;
		}
		return f;
	}
	if (f < 1.0e-5) {
		return 0.0f;
	}
	return f;
}

// Enum indicating the type of export we are performing
//------------------------------------------------------

enum ExportType {
	Export_VRBL,
	Export_VRML_1_0,
	Export_VRML_2_0
};

class TextureDesc {
public:
	TextureDesc(BitmapTex* t, TSTR& s, TSTR& u)
	{
		tex = t; name = s; url = u;
	}
	BitmapTex* tex;
	TSTR       name;
	TSTR       url;
};

class NormalTable;

class NodeTable {
public:
	NodeTable() { }
	~NodeTable();

	const TCHAR* GetNodeName(INode* node);

private:
	std::unordered_map<INode*, TSTR> mNodeToName;  // associate unique node name with node
	std::set<TSTR> mUniqueNodeNames; // store unique names used so far for fast lookup
};

class VRBLExport : public SceneExport {
public:
	VRBLExport();
	~VRBLExport();

	int		      ExtCount();
	const TCHAR * Ext(int n);
	const TCHAR * LongDesc();
	const TCHAR * ShortDesc();
	const TCHAR * AuthorName();
	const TCHAR * CopyrightMessage();
	const TCHAR * OtherMessage1();
	const TCHAR * OtherMessage2();
	unsigned int  Version();
	void	      ShowAbout(HWND hWnd);
	int           DoExport(const TCHAR *name, ExpInterface *ei, Interface *i, BOOL suppressPrompts = FALSE, DWORD options = 0);

	inline BOOL GetGenNormals() { return mGenNormals; }
	inline void SetGenNormals(BOOL gen) { mGenNormals = gen; }
	inline BOOL GetIndent() { return mIndent; }
	inline void SetIndent(BOOL in) { mIndent = in; }
	inline ExportType GetExportType() { return mType; }
	inline void SetExportType(ExportType t) { mType = t; }
	inline Interface* GetIP() { return mIp; }
	inline INode* GetCamera() { return mCamera; }
	inline INode* GetNavInfo() { return mNavInfo; }
	inline INode* GetBackground() { return mBackground; }
	inline INode* GetFog() { return mFog; }
	inline void SetCamera(INode* cam) { mCamera = cam; }
	inline void SetNavInfo(INode* ni) { mNavInfo = ni; }
	inline void SetBackground(INode* ni) { mBackground = ni; }
	inline void SetFog(INode* ni) { mFog = ni; }
	inline void SetUsePrefix(BOOL u) { mUsePrefix = u; }
	inline BOOL GetUsePrefix() { return mUsePrefix; }
	inline void SetUrlPrefix(TSTR& s) { mUrlPrefix = s; }
	inline TSTR& GetUrlPrefix() { return mUrlPrefix; }
	inline void SetFields(BOOL f) { mGenFields = f; }
	inline BOOL GetFields() { return mGenFields; }
	void GetCameras(INode *inode, Tab<INode*> *camList,
		Tab<INode*> *navInfoList, Tab<INode*> *backgrounds,
		Tab<INode*> *fogs);
	inline BOOL IsVRML2() { return mType == Export_VRML_2_0; }
	inline BOOL GetZUp() { return mZUp; }
	inline void SetZUp(BOOL zup) { mZUp = zup; }
	inline int GetDigits() { return mDigits; }
	inline void SetDigits(int n) { mDigits = n; }
	inline BOOL GetCoordInterp() { return mCoordInterp; }
	inline void SetCoordInterp(BOOL ci) { mCoordInterp = ci; }
#ifdef _LEC_
	inline BOOL GetFlipBook() { return mFlipBook; }
	inline void SetFlipBook(BOOL ci) { mFlipBook = ci; }
#endif

	inline BOOL GetTformSample() { return mTformSample; }
	inline void SetTformSample(BOOL b) { mTformSample = b; }
	inline int GetTformSampleRate() { return mTformSampleRate; }
	inline void SetTformSampleRate(int rate) { mTformSampleRate = rate; }

	inline BOOL GetCoordSample() { return mCoordSample; }
	inline void SetCoordSample(BOOL b) { mCoordSample = b; }
	inline int GetCoordSampleRate() { return mCoordSampleRate; }
	inline void SetCoordSampleRate(int rate) { mCoordSampleRate = rate; }

	inline BOOL GetFlipbookSample() { return mFlipbookSample; }
	inline void SetFlipbookSample(BOOL b) { mFlipbookSample = b; }
	inline int GetFlipbookSampleRate() { return mFlipbookSampleRate; }
	inline void SetFlipbookSampleRate(int rate) { mFlipbookSampleRate = rate; }

	inline TSTR& GetInfo() { return mInfo; }
	inline void SetInfo(TCHAR* info) { mInfo = info; }
	inline TSTR& GetTitle() { return mTitle; }
	inline void SetTitle(TCHAR* title) { mTitle = title; }
	inline BOOL GetExportHidden() { return mExportHidden; }
	inline void SetExportHidden(BOOL eh) { mExportHidden = eh; }

	inline BOOL GetEnableProgressBar() { return mEnableProgressBar; }
	inline void SetEnableProgressBar(BOOL epb) { mEnableProgressBar = epb; }

	inline BOOL GetPrimitives() { return mPrimitives; }
	inline void SetPrimitives(BOOL eh) { mPrimitives = eh; }

	inline int  GetPolygonType() { return mPolygonType; }
	inline void SetPolygonType(int type) { mPolygonType = type; }

	inline int  GetPreLight() { return mPreLight; }
	inline void SetPreLight(int i) { mPreLight = i; }

	inline int  GetCPVSource() { return mCPVSource; }
	inline void SetCPVSource(int i) { mCPVSource = i; }

	CallbackTable*  GetCallbacks() { return &mCallbacks; }

	Interface* mIp;         // MAX interface pointer

	// UI controls
	static ISpinnerControl* tformSpin;
	static ISpinnerControl* coordSpin;
	static ISpinnerControl* flipbookSpin;

private:
	friend __declspec(dllexport) void LibInitialize();
	static void UpdateAppDataLoadProc(Animatable *anim, const Class_ID& cid, SClass_ID sid, ILoad* iload, Tab<DWORD> &subIDs);

private:
	TCHAR* point(Point3& p);
	TCHAR* scalePoint(Point3& p);
	TCHAR* normPoint(Point3& p);
	TCHAR* axisPoint(Point3& p, float ang);
	TCHAR* texture(UVVert& uv);
	TCHAR* color(Color& c);
	TCHAR* color(Point3& c);
	TCHAR* floatVal(float f);

	void initializeDefaults();
	// VRBL Output routines
	void Indent(int level);
	void StartNode(INode* node, Object* obj, int level, BOOL outputName);
	void EndNode(INode* node, int level, BOOL lastChild);
	//BOOL IsBBoxTrigger(INode* node);
	void OutputNodeTransform(INode* node, int level);
	void OutputMultiMtl(Mtl* mtl, int level);
	void OutputNoTexture(int level);
	BOOL OutputMaterial(INode* node, BOOL& twoSided, int level);
	BOOL HasTexture(INode *node);
	TextureDesc*GetMatTex(INode* node);
	void OutputNormalIndices(Mesh& mesh, NormalTable* normTab, int level);
	NormalTable* OutputNormals(Mesh& mesh, int level);
	void OutputTriObject(INode* node, TriObject* obj, BOOL multiMat,
		BOOL twoSided, int level);
	BOOL isVrblObject(INode * node, Object *obj, INode* parent);
	void VrblOutObject(INode* node, INode* parent, Object* obj, int level);
	BOOL VrblOutSphere(INode * node, Object *obj, int level);
	BOOL VrblOutCylinder(INode* node, Object *obj, int level);
	BOOL VrblOutCone(INode* node, Object *obj, int level);
	BOOL VrblOutCube(INode* node, Object *obj, int level);
	BOOL VrblOutCamera(INode* node, Object* obj, int level);
	//void VrblAnchorHeader(MrBlueObject* obj, VRBL_TriggerType type,
	//                      BOOL fromParent, int level);
	//BOOL VrblOutMrBlue(INode* node, INode* parent, MrBlueObject* obj,
	//                   int* level, BOOL fromParent);
	BOOL VrblOutInline(VRMLInsObject* obj, int level);
	BOOL VrblOutLOD(INode* node, LODObject* obj, int level);
	//BOOL IsAimTarget(INode* node);
	BOOL VrblOutTarget(INode* node, int level);
	BOOL VrblOutSpecial(INode* node, INode* parent, Object* obj, int level);
	//int StartMrBlueHelpers(INode* node, int level);
	//void EndMrBlueNode(INode* childNode, int& level);
	//void EndMrBlueHelpers(INode* node, int level);
	BOOL VrblOutPointLight(INode* node, LightObject* light, int level);
	BOOL VrblOutDirectLight(INode* node, LightObject* light, int level);
	BOOL VrblOutSpotLight(INode* node, LightObject* light, int level);
	BOOL VrblOutTopPointLight(INode* node, LightObject* light);
	BOOL VrblOutTopDirectLight(INode* node, LightObject* light);
	BOOL VrblOutTopSpotLight(INode* node, LightObject* light);
	void OutputTopLevelLight(INode* node, LightObject *light);
	BOOL WriteTCBKeys(INode* node, Control *cont, int type, int level);
	void WriteLinearKeys(INode* node,
		Tab<TimeValue>& posTimes,
		Tab<Point3>& posKeys,
		Tab<TimeValue>& rotTimes,
		Tab<AngAxis>& rotKeys,
		Tab<TimeValue>& sclTimes,
		Tab<Point3>& sclKeys,
		int type, int level);
	int WriteAllControllerData(INode* node, int flags, int level, Control *lc);
	void WritePositionKey0(INode* node, TimeValue t, int level, BOOL force);
	void WriteRotationKey0(INode* node, TimeValue t, int level, BOOL force);
	void WriteScaleKey0(INode* node, TimeValue t, int level, BOOL force);

	void WriteVisibilityData(INode* node, int level);
	BOOL IsLight(INode* node);
	Control* GetLightColorControl(INode* node);
	void VrblOutControllers(INode* node, int level);
	void VrblOutAnimationFrames();
	void ScanSceneGraph();
	void ComputeWorldBoundBox(INode* node, ViewExp& vpt);
	void TraverseNode(INode* node);
	BOOL ObjectIsLODRef(INode* node);
	void VrmlOutTopLevelCamera(int level, INode* node, BOOL topLevel);
	void VrblOutFileInfo();
	void VrblOutNode(INode* node, INode* parent, int level, BOOL isLOD,
		BOOL lastChild);
	void GenerateUniqueNodeNames(INode* node);
	BOOL VRBLExport::IsMirror(INode* node);
	BOOL GetCallbackMethods();

	MaxSDK::Util::TextFile::Writer       mStream;     // The file mStream to write
	BOOL       mGenNormals; // Generate normals in the VRML file
	BOOL       mIndent;     // Should we indent?
	INodeList* mLodList;    // List of LOD objects in the scene
	ExportType mType;       // Language to export (VRML, VRBL, ...)
	INode*     mCamera;     // Initial camera;
	INode*     mNavInfo;    // Initial NavigationInfo
	INode*     mBackground; // Initial Background node
	INode*     mFog;        // Initial Fog node
	BOOL       mUsePrefix;  // Use URL Prefix
	TSTR       mUrlPrefix;  // The URL prefix
	BOOL       mGenFields;  // Generate "fields" statements
	BOOL       mHadAnim;    // File has animation data
	TimeValue  mStart;      // First frame of the animation
	BOOL       mZUp;        // Z axis if true, Y axis otherwise
	int        mDigits;     // Digits of precision on output
	BOOL       mCoordInterp;// Generate coordinate interpolators
#ifdef _LEC_
	BOOL       mFlipBook;   // Generate multiple file one file per frame (LEC request)
#endif
	BOOL       mTformSample;// TRUE for once per frame, FALSE for cusom rate
	int        mTformSampleRate; // Custom sample rate
	BOOL       mCoordSample;// TRUE for once per frame, FALSE for cusom rate
	int        mCoordSampleRate; // Custom sample rate
	BOOL       mFlipbookSample;     // TRUE for once per frame, FALSE for cusom rate
	int        mFlipbookSampleRate; // Custom sample rate
	Box3       mBoundBox;   // Bounding box for the whole scene
	TSTR       mTitle;      // Title of world
	TSTR       mInfo;       // Info for world
	BOOL       mExportHidden; // Export hidden objects
	BOOL       mEnableProgressBar;  // Enable export progress bar
	BOOL       mPrimitives;   // Create VRML primitves
	int         mPolygonType;   // 0 triangle, 1 QUADS, 2 NGONS
	BOOL       mPreLight;       // should we calculate the color per vertex
	BOOL       mCPVSource;  // 1 if MAX; 0 if we should calculate the color per vertex
	NodeTable  mNodes;      // Array of Unique node names in the scene
	CallbackTable   mCallbacks; // callback methods
};


