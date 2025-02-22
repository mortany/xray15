/**********************************************************************
 *<
	FILE: nspline.cpp

	DESCRIPTION:  Normalize Spline

	CREATED BY: Peter Watje

	HISTORY: created Jan 20, 1997

 *>	Copyright (c) 1997, All Rights Reserved.
 **********************************************************************/

#include "mods.h"

#include "iparamm2.h"
#include "shape.h"
#include "spline3d.h"
#include "splshape.h"
#include "linshape.h"

//--- NormalizeSpline -----------------------------------------------------------

const ChannelMask EDITSPL_CHANNELS = (GEOM_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL|DISP_ATTRIB_CHANNEL|TOPO_CHANNEL);

#define MIN_AMOUNT		float(-1.0E30)
#define MAX_AMOUNT		float(1.0E30)

#define SPLINETYPE 0
#define CIRCULAR 0


#define PW_PATCH_TO_SPLINE1 0x1c450e5c
#define PW_PATCH_TO_SPLINE2 0x2e0e0902

//#define DEBUG 1

// IF YOU CHANGE THIS!!!!
// KZ 2015/11/19; If you change this modifier, update spline IK. bones.cpp depends on this class.
// To find it, do a search using the classid defined just above.

class NormalizeSpline: public Modifier {
	
	protected:
		IParamBlock2 *pblock;
		static IObjParam *ip;
		
	public:
		static IParamMap *pmapParam;
		static float nlength;

		// From Animatable
		void DeleteThis() { delete this; }
		void GetClassName(TSTR& s) { s= GetString(IDS_PW_NSPLINE); }  
		virtual Class_ID ClassID() { return Class_ID(PW_PATCH_TO_SPLINE1,PW_PATCH_TO_SPLINE2);}		
		RefTargetHandle Clone(RemapDir& remap);
		const TCHAR *GetObjectName() { return GetString(IDS_PW_NSPLINE); }
		IOResult Load(ILoad *iload);
		IOResult Save(ISave *isave);

		NormalizeSpline();
		virtual ~NormalizeSpline();

		ChannelMask ChannelsUsed()  { return EDITSPL_CHANNELS; }
		ChannelMask ChannelsChanged() { return EDITSPL_CHANNELS; }

		void ModifyObject(TimeValue t, ModContext &mc, ObjectState *os, INode *node);

		Class_ID InputType() { return Class_ID(SPLINESHAPE_CLASS_ID,0); }
		
		Interval LocalValidity(TimeValue t);

		// From BaseObject
		BOOL ChangeTopology() {return TRUE;}

		int NumRefs() {return 1;}
		RefTargetHandle GetReference(int i) {return pblock;}
private:
		virtual void SetReference(int i, RefTargetHandle rtarg) {pblock=(IParamBlock2*)rtarg;}
public:

 		int NumSubs() { return 1; }  
		Animatable* SubAnim(int i) { return pblock; }
		TSTR SubAnimName(int i) { return TSTR(GetString(IDS_RB_PARAMETERS));}		

		RefResult NotifyRefChanged( const Interval& changeInt,RefTargetHandle hTarget, 
		   PartID& partID, RefMessage message, BOOL propagate);

		CreateMouseCallBack* GetCreateMouseCallBack() {return NULL;} 

		void BeginEditParams(IObjParam *ip, ULONG flags,Animatable *prev);
		void EndEditParams(IObjParam *ip, ULONG flags,Animatable *next);

		void BuildSkin(TimeValue t,ModContext &mc, ObjectState * os);
		void BuildSkin_Ver1(TimeValue t,ModContext &mc, ObjectState * os);

		void UpdateUI(TimeValue t) {}
		Interval GetValidity(TimeValue t);
		ParamDimension *GetParameterDim(int pbIndex);
		TSTR GetParameterName(int pbIndex);

// JBW: direct ParamBlock access is added
		int	NumParamBlocks() { return 1; }					// return number of ParamBlocks in this instance
		IParamBlock2* GetParamBlock(int i) { if (i == 0) return pblock; 
											 else return NULL;
										   } // return i'th ParamBlock
		IParamBlock2* GetParamBlockByID(BlockID id) {if (pblock->ID() == id) return pblock ;
													else return  NULL; } // return id'd ParamBlock

		bool mUseNewMethod;
		int mVersion;
	};

class NormalizeSplineClassDesc:public ClassDesc2 {
	public:
	int 			IsPublic() { return 0; }
	void *			Create(BOOL loading = FALSE) { return new NormalizeSpline; }
	const TCHAR *	ClassName() { return GetString(IDS_PW_NSPLINE); }
	SClass_ID		SuperClassID() { return OSM_CLASS_ID; }
	Class_ID		ClassID() { return  Class_ID(PW_PATCH_TO_SPLINE1,PW_PATCH_TO_SPLINE2); }
	const TCHAR* 	Category() { return GetString(IDS_RB_DEFDEFORMATIONS);}
// JBW: new descriptor data accessors added.  Note that the 
//      internal name is hardwired since it must not be localized.
	const TCHAR*	InternalName() { return _T("Normalize_Spline"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle
	};

static NormalizeSplineClassDesc normalizeSplineDesc;
extern ClassDesc* GetNormalizeSplineDesc() { return &normalizeSplineDesc; }

IObjParam*		NormalizeSpline::ip        = NULL;
IParamMap *		NormalizeSpline::pmapParam = NULL;
float			NormalizeSpline::nlength = 20.0f;
 
#define PBLOCK_REF		0

// per instance geosphere block
static ParamBlockDesc2 nspline_param_blk ( nspline_params, _T("Parameters"),  0, &normalizeSplineDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_REF, 
	//rollout
	IDD_NSPLINE, IDS_RB_PARAMETERS, 0, 0, NULL,
	// params

	nspline_length,  _T("Length"),	TYPE_FLOAT, 	0, 	IDS_RB_LENGTH, 
	p_default, 		20.0f,	
	p_range, 		1.0f,999999999.0f, 
	p_ui, 			TYPE_SPINNER, EDITTYPE_POS_UNIVERSE, IDC_LENGTH_EDIT,IDC_LENGTH_SPIN,  1.0f,
	p_end, 

	nspline_accuracy,  _T("Accuracy"),	TYPE_INT, 	0, 	0, 
	p_default, 		10,	
	p_range, 		1,20, 
	p_ui, 			TYPE_SPINNER, EDITTYPE_INT, IDC_ACC_EDIT,IDC_ACC_SPIN,  1,
	p_end, 
		
	p_end
	);

static ParamBlockDescID descVer3[] = {
	{ TYPE_INT, NULL, FALSE, static_cast<DWORD>(-1) },
	{ TYPE_FLOAT, NULL, TRUE, nspline_length }
	 };

#define PBLOCK_LENGTH	2

// Array of old versions
static ParamVersionDesc versions[] = {
	ParamVersionDesc(descVer3,2,5),	
	};
#define NUM_OLDVERSIONS	1

// Current version
#define CURRENT_VERSION	5
//   static ParamVersionDesc curVersion(descVer3,PBLOCK_LENGTH,CURRENT_VERSION);

NormalizeSpline::NormalizeSpline()
	{
	pblock = NULL;
	GetNormalizeSplineDesc()->MakeAutoParamBlocks(this);
	mUseNewMethod = true;
    mVersion  = 1;
	}

NormalizeSpline::~NormalizeSpline()
	{	
	}

Interval NormalizeSpline::LocalValidity(TimeValue t)
	{
	// if being edited, return NEVER forces a cache to be built 
	// after previous modifier.
	if (TestAFlag(A_MOD_BEING_EDITED))
		return NEVER;  
	Interval valid = GetValidity(t);	
	return valid;
	}

RefTargetHandle NormalizeSpline::Clone(RemapDir& remap)
	{
	NormalizeSpline* newmod = new NormalizeSpline();	
	newmod->ReplaceReference(0,remap.CloneRef(pblock));
	BaseClone(this, newmod, remap);
	return(newmod);
	}

void NormalizeSpline::ModifyObject(TimeValue t, ModContext &mc, ObjectState * os, INode *node) 
	{	
//DebugPrint(_T("Extrude modifying object\n"));

	// Get our personal validity interval...
	Interval valid = GetValidity(t);
	// and intersect it with the channels we use as input (see ChannelsUsed)
	valid &= os->obj->ChannelValidity(t,TOPO_CHAN_NUM);
	valid &= os->obj->ChannelValidity(t,GEOM_CHAN_NUM);
	Matrix3 modmat,minv;
	
	pblock->GetValue(nspline_length,t,nlength,valid);

	if (nlength < 0.0001f) nlength = 0.0001f;

	if ((mVersion == 0) && (mUseNewMethod == false))
	BuildSkin(t, mc, os);
	else 
		BuildSkin_Ver1(t, mc, os);

	os->obj->SetChannelValidity(TOPO_CHAN_NUM, valid);
	os->obj->SetChannelValidity(GEOM_CHAN_NUM, valid);
	os->obj->SetChannelValidity(TEXMAP_CHAN_NUM, valid);
	os->obj->SetChannelValidity(MTL_CHAN_NUM, valid);
	os->obj->SetChannelValidity(SELECT_CHAN_NUM, valid);
	os->obj->SetChannelValidity(SUBSEL_TYPE_CHAN_NUM, valid);
	os->obj->SetChannelValidity(DISP_ATTRIB_CHAN_NUM, valid);

	os->obj->UnlockObject();
//	os->obj->InvalidateGeomCache();

	}

void NormalizeSpline::BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev )
	{
	this->ip = ip;

	TimeValue t = ip->GetTime();


	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_BEGIN_EDIT);
	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_MOD_DISPLAY_ON);

	SetAFlag(A_MOD_BEING_EDITED);

	normalizeSplineDesc.BeginEditParams(ip, this, flags, prev);
	}

void NormalizeSpline::EndEditParams( IObjParam *ip, ULONG flags,Animatable *next)
	{
	this->ip = NULL;
	
	TimeValue t = ip->GetTime();

	// aszabo|feb.05.02 This flag must be cleared before sending the REFMSG_END_EDIT
	ClearAFlag(A_MOD_BEING_EDITED);
	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_END_EDIT);
	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_MOD_DISPLAY_OFF);

	normalizeSplineDesc.EndEditParams(ip, this, flags, next);
	}

Interval NormalizeSpline::GetValidity(TimeValue t)
	{
	float f;
	Interval valid = FOREVER;

	// Start our interval at forever...
	// Intersect each parameters interval to narrow it down.
	pblock->GetValue(nspline_length,t,f,valid);

	return valid;
	}

RefResult NormalizeSpline::NotifyRefChanged(
		const Interval& changeInt, 
		RefTargetHandle hTarget, 
   		PartID& partID, 
		RefMessage message, 
		BOOL propagate ) 
   	{
	return(REF_SUCCEED);
	}

ParamDimension *NormalizeSpline::GetParameterDim(int pbIndex)
	{
	return defaultDim;
	}

TSTR NormalizeSpline::GetParameterName(int pbIndex)
	{
	return TSTR(_T(""));
	}

#define NEWMETHOD_CHUNK		0x0100
#define NSPLINE_VERSION_CHUNK		0x0110
IOResult NormalizeSpline::Load(ILoad *iload)
	{
	Modifier::Load(iload);

	IOResult res = IO_OK;
	ULONG nb = 0;

	mUseNewMethod = false;
	mVersion = 0;


	USHORT next = iload->PeekNextChunkID();
	if (next == NEWMETHOD_CHUNK) {
		while (IO_OK==(res=iload->OpenChunk())) {
			switch (iload->CurChunkID()) {
				case NEWMETHOD_CHUNK:
					mUseNewMethod = true;
					break;				
				case NSPLINE_VERSION_CHUNK:
					iload->Read(&mVersion,sizeof(mVersion), &nb);
					break;
				}
			iload->CloseChunk();
			if (res!=IO_OK)  return res;
			}
	}


	ParamBlock2PLCB* plcb = new ParamBlock2PLCB(versions, NUM_OLDVERSIONS, &nspline_param_blk, this, PBLOCK_REF);
	iload->RegisterPostLoadCallback(plcb);

	return IO_OK;
	}


IOResult NormalizeSpline::Save(ISave *isave)
	{

	Modifier::Save(isave);

	if (mUseNewMethod)
	{
		isave->BeginChunk(NEWMETHOD_CHUNK);
		isave->EndChunk();
	}
	
	ULONG nb = 0;
	isave->BeginChunk(NSPLINE_VERSION_CHUNK);
	isave->Write(&mVersion,sizeof(mVersion),&nb);
	isave->EndChunk();

	
	return IO_OK;
	}


void NormalizeSpline::BuildSkin(TimeValue t,ModContext &mc, ObjectState * os) {

SplineShape *shape = (SplineShape *)os->obj->ConvertToType(t,splineShapeClassID);

int polys = shape->shape.splineCount;
int poly;

float cinc = 0.0f;
float TotalLength = 0.0f;

for(poly = 0; poly < polys; ++poly) 
	{
	shape->shape.splines[poly]->ComputeBezPoints();

	TotalLength += shape->shape.splines[poly]->SplineLength();
	}

	cinc = 	nlength/TotalLength;

	cinc = cinc/10.0f;
	if (cinc>0.001f) cinc = 0.001f;

	for(poly = 0; poly < polys; ++poly) 
	{
		//get spline
		//get number segs

		//get points
		float SegLength;

		SegLength = nlength*nlength;

		float inc = 0.001f;
		float CurrentPercent = 0.0f;

		inc = cinc;
		Point3 CurrentPoint,NextPoint;
		Tab<Point3> PointList;
		PointList.ZeroCount();

		while (CurrentPercent < 1.0)
		{
			CurrentPoint = shape->shape.splines[poly]->InterpCurve3D(CurrentPercent);
			PointList.Append(1,&CurrentPoint,1);
			NextPoint = CurrentPoint;
			while ((LengthSquared(CurrentPoint-NextPoint)<SegLength) && (CurrentPercent <1.0f))
			{
				CurrentPercent += inc;
				NextPoint = shape->shape.splines[poly]->InterpCurve3D(CurrentPercent);
			}
		}
		int i,closed;
		closed = shape->shape.splines[poly]->Closed();
		if (!shape->shape.splines[poly]->Closed())
		{
			NextPoint = shape->shape.splines[poly]->GetKnotPoint(shape->shape.splines[poly]->KnotCount()-1);
			PointList.Append(1,&NextPoint,1);
		}
		shape->shape.splines[poly]->NewSpline();
		//add new points
		if (closed)
			shape->shape.splines[poly]->SetClosed();
		else shape->shape.splines[poly]->SetOpen();

		for (i=0;i<PointList.Count();i++)
		{
			shape->shape.splines[poly]->AddKnot(SplineKnot(KTYPE_AUTO,LTYPE_CURVE,
				PointList[i],PointList[i],PointList[i]));
		}

		if (shape->shape.splines[poly]->KnotCount() == 1)
			shape->shape.splines[poly]->AddKnot(SplineKnot(KTYPE_AUTO,LTYPE_CURVE,
			PointList[PointList.Count()-1],PointList[PointList.Count()-1],PointList[PointList.Count()-1]));

		shape->shape.splines[poly]->ComputeBezPoints();
		for (i=0;i<shape->shape.splines[poly]->KnotCount();i++)
			shape->shape.splines[poly]->SetKnotType(i,KTYPE_AUTO);


		shape->shape.splines[poly]->ComputeBezPoints();


	}

	shape->shape.UpdateSels();	// Make sure it readies the selection set info
	shape->shape.InvalidateGeomCache();
}


void NormalizeSpline::BuildSkin_Ver1(TimeValue t,ModContext &mc, ObjectState * os) {

SplineShape *shape = (SplineShape *)os->obj->ConvertToType(t,splineShapeClassID);

int polys = shape->shape.splineCount;
int poly;

float cinc = 0.0f;
float TotalLength = 0.0f;

for(poly = 0; poly < polys; ++poly) 
	{
	shape->shape.splines[poly]->ComputeBezPoints();

	TotalLength += shape->shape.splines[poly]->SplineLength();
	}

if (TotalLength == 0.0f) return;

cinc = 	nlength/TotalLength;

if (mUseNewMethod)
	cinc = cinc/2.0f;  //we can bump the inc up since we are now a little more adaptive
else
cinc = cinc/10.0f;
if (cinc>0.001f) cinc = 0.001f;

int maxDepth = pblock->GetInt(nspline_accuracy);; //this is the maximum number of times we reduce the increment before giving up


for(poly = 0; poly < polys; ++poly) 
	{
//get spline
//get number segs

//get points
	float SegLength;
	if (mUseNewMethod )
		SegLength = nlength;
	else
		SegLength = nlength*nlength;

	float inc = 0.001f;
	float splineInterp = 0.0f;

	inc = cinc;
	Point3 StartPoint,NextPoint;
	Tab<Point3> PointList; //the points where we hold our insert points
	PointList.ZeroCount();
	float holdInc = inc;

	//the algorithm walks along the spline starting at 0 and incrementing by a small amount
	//measuring the distance as it goes.  When the distance is greater than the segment length
	//it steps back to the previous point, reduces the increment and continues again until  it
	//comes really close to the goal where it stops, create a point and then resets the increment
	//and continues on.
	while (splineInterp < 1.0)
	{
		//get our starting point
		StartPoint = shape->shape.splines[poly]->InterpCurve3D(splineInterp);
		PointList.Append(1,&StartPoint,1);
		NextPoint = StartPoint;
		
		inc = holdInc;
		
		int currentDepth = 0;
		float runningDistance = 0;
		float lSquared = LengthSquared(StartPoint-NextPoint);
		
		while ((runningDistance<nlength) && (splineInterp <1.0f) && (inc > 1.0E-6)) //when out interp is greater than 1 we are done for this spline
		{
			//advance our interp and check to see how far we have gone
			splineInterp += inc;
			Point3 prevPoint = NextPoint;
			NextPoint = shape->shape.splines[poly]->InterpCurve3D(splineInterp);
			float prevlSquared = lSquared;
			lSquared = LengthSquared(StartPoint-NextPoint);
			float prevRunningDistance = runningDistance;
			
			if (mUseNewMethod)
				runningDistance += Length(NextPoint-prevPoint);
			else
				runningDistance += LengthSquared(NextPoint-prevPoint);

			if (mUseNewMethod && currentDepth < maxDepth)
			{
				//if we have gone past our goal back step and reduce our increment
				if (runningDistance>nlength)
				{
					//backstep
					splineInterp -= inc;
					NextPoint = prevPoint;
					lSquared = prevlSquared;
					runningDistance = prevRunningDistance;
					//reduce our increment
					inc = inc*0.5;
					currentDepth++;										
				}
			}			
		}
	}
	
	if (!shape->shape.splines[poly]->Closed() && PointList.Count() <2 ) return;
	if (shape->shape.splines[poly]->Closed() && PointList.Count() < 3 ) return;

   int i,closed;
   closed = shape->shape.splines[poly]->Closed();
   if (!shape->shape.splines[poly]->Closed())
		{
		NextPoint = shape->shape.splines[poly]->GetKnotPoint(shape->shape.splines[poly]->KnotCount()-1);
		PointList.Append(1,&NextPoint,1);
		}
	shape->shape.splines[poly]->NewSpline();
//add new points
	if (closed)
		shape->shape.splines[poly]->SetClosed();
		else shape->shape.splines[poly]->SetOpen();

	for (i=0;i<PointList.Count();i++)
		{
		shape->shape.splines[poly]->AddKnot(SplineKnot(KTYPE_AUTO,LTYPE_CURVE,
					PointList[i],PointList[i],PointList[i]));
		}

	if (shape->shape.splines[poly]->KnotCount() == 1)
		shape->shape.splines[poly]->AddKnot(SplineKnot(KTYPE_AUTO,LTYPE_CURVE,
					PointList[PointList.Count()-1],PointList[PointList.Count()-1],PointList[PointList.Count()-1]));

	shape->shape.splines[poly]->ComputeBezPoints();
	for (i=0;i<shape->shape.splines[poly]->KnotCount();i++)
	{
//just some code to smooth out the last segment if it is really short compared to the other
//segments.  Since the knots are set to auto if you have a real short last segment on a closed
//spline you may get overshot which will cause a loop so it is set to linear in that case
		if ( (i == shape->shape.splines[poly]->KnotCount()-1) && mUseNewMethod)
		{			
			SplineKnot knot1 = shape->shape.splines[poly]->GetKnot(0);
			if (!shape->shape.splines[poly]->Closed())
				knot1 = shape->shape.splines[poly]->GetKnot(i-1);
			SplineKnot knot2 = shape->shape.splines[poly]->GetKnot(i);
			Point3 a = knot1.Knot();
			Point3 b = knot2.Knot();
			float len = Length(a-b);
			if (len < (nlength * 0.2f))
			{
				if (shape->shape.splines[poly]->Closed())
					shape->shape.splines[poly]->SetLineType(i,LTYPE_LINE);
				else
					shape->shape.splines[poly]->SetLineType(i-1,LTYPE_LINE);
			}
		}
		else
			shape->shape.splines[poly]->SetKnotType(i,KTYPE_AUTO);
	}


	shape->shape.splines[poly]->ComputeBezPoints();


	}

shape->shape.UpdateSels();	// Make sure it readies the selection set info
shape->shape.InvalidateGeomCache();
}

