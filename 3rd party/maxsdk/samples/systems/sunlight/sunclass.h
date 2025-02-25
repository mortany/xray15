//**************************************************************************/
// Copyright (c) 1998-2007 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/
#pragma once

#include <ref.h>
#include <maxapi.h>
#include <control.h>
#include <CommCtrl.h>
#include <IDaylightSystem.h>
#include "TimeDimension.h"

// forward declarations
class IParamBlock;
class NatLightAssembly;
class SlaveControl; // defined below
class ISpinnerControl;
class ICustStatus;
struct JulianCalendar;

/*===========================================================================*\
| Definitions:
\*===========================================================================*/

#define LIGHT_TM	0
#define LIGHT_MULT	1
#define LIGHT_INTENSE 2
#define LIGHT_SKY	3

// Param block reference
#define REF_PARAM	0
// Compass (parent) obj reference
#define REF_POINT	1
// LookAt Ctrl reference
#define REF_LOOKAT	2
// Another controller ref
#define REF_MULTIPLIER 3
// Light\Assembly objects monitor ref
#define REF_OBJECT_MONITOR 4
#define NUM_REF 5

int getTimeZone(float longi);

/*===========================================================================*\
 | Sun Master Class:
\*===========================================================================*/

class SunMaster: public ReferenceTarget, public TimeChangeCallback { 
	friend class SunMasterPostLoad;

	// Object parameters
	IParamBlock *pblock;


	//Local epoch scheme when animating time
	float timeref;
	long dateref;
	BOOL refset;
    BOOL mbOldAnimation;    // Flag to keep compatibility with sunlight
                            // systems that animate date and time together.
                            // This flag is reset, when either the date or
                            // time is changed, and Animating() is on.

	//Don't want to process msgs which we induce 
	BOOL ignore_point_msgs;

	//The dependent variables
	double alt, az;
	TCHAR city[64];
	// Used only to load legacy files. This node pointer gets transformed into 
	// an indirect node reference via a postload callback
	INode* theLegacyObject;

	public:

		// thePoint is the helper object about which the light rotates
		INode* thePoint;
		ReferenceTarget* theObjectMon;
		Control* theLookat;
		Control* theMultiplier;

		SYSTEMTIME theTime;
		Interval tvalid;// here we maintain the validity of theTime; 
		bool daylightSystem;
		bool controlledByWeatherFile; //new function to disable automatic city calc's etc... from lat/lon
		NatLightAssembly* natLight;

		void SetCityInfo(const TCHAR* name, int& zone, float& lati, float& longi);

		// Constructor.
		SunMaster(bool daylight);
		~SunMaster();
		
		static void SetInCreate(bool value);
		static bool IsInCreate();

		void SetPosition(const Point3& position);
		Point3 GetPosition();
		void SetCompassDiameter(float compassDiameter);
		float GetCompassDiameter();
			

		// method to rotate the helper object around its z axis. This is done by spinning
		// a spinner on the parameters rollout which seems a bit convoluted.
		void align_north(TimeValue t, float r);
		
		// calculates the azimuth and altitude from the independent vars
		void calculate(TimeValue t, Interval& valid);

		float calcIntensity(TimeValue t, Interval& valid);

		//access to the private dependent vars
		double Getalt(){return alt;}
		double Getaz(){return az;}
		// Note: I would _like_ to be able to return a const char*, but
		// the ICustStatus::SetText() method requires a char*, even though
		// it doesn't change the text!
		TCHAR* GetCity() { return city; }
		void SetCity(const TCHAR* nam);
		//"Calculates" the city from the curent latitude and longitude
		void calculateCity(TimeValue t);

		// The following methods set and get values from the parameter block
		void SetRad(TimeValue t, float r);
		void SetLat(TimeValue t, float r);
		void SetLong(TimeValue t, float r);
		void SetNorth(TimeValue t, float r);
		void SetDate(TimeValue t, const JulianCalendar& jd);
		void SetTime(TimeValue t, const JulianCalendar& jd);
		void SetZone(TimeValue t, int h);
		void SetDst(TimeValue t, BOOL h);
		void SetManual(TimeValue t, BOOL h);
		void SetSkyCondition(TimeValue t, float sky);

		float GetRad(TimeValue t, Interval& valid = Interval(0,0) );
		float GetLat(TimeValue t, Interval& valid = Interval(0,0) );
		float GetLong(TimeValue t, Interval& valid = Interval(0,0) );
		float GetNorth(TimeValue t, Interval& valid = Interval(0,0) );
		void GetTime(TimeValue t, Interval& valid = Interval(0,0) );
		int GetZone(TimeValue t, Interval& valid = Interval(0,0) );
		int GetDst(TimeValue t, Interval& valid = Interval(0,0) );
		int GetManual(TimeValue t, Interval& valid = Interval(0,0) );
		float GetSkyCondition(TimeValue t, Interval& valid = Interval(0,0) );
		Point2 GetAzAlt(TimeValue t, Interval& valid );

		//The controller needs to interpolate time as if it were a single float. However
		//the user will be changing the hours, mins, secs independently. I've created set
		//and get methods for each of the date/time fields as if there were parameter block
		//entries for each of them. These methods just call settime and gettime respectively.
		//The set methods actually place their values into a local structure theTime, and then
		// call SetTime which converts the contents of the struct into something which can be
		//stuck in the parameter block. GetTime pulls the interpolated time from the pblock 
		// and explodes it into the local stucture. Therefore GetHour for example calls GetTime
		// and then returns the appropriate field of the structure.
		
		// the next methods are pseudo PBLOCK methods which call SetTime
		void SetHour(TimeValue t, int h);
		void SetMin(TimeValue t, int m);
		void SetSec(TimeValue t, int s);
		void SetMon(TimeValue t, int m);
		void SetDay(TimeValue t, int d);
		void SetYr(TimeValue t, int y);


		// the next methods are pseudo PBLOCK methods which call GetTime
		int GetHour(TimeValue t, Interval& valid = Interval(0,0));
		int GetMin(TimeValue t, Interval& valid = Interval(0,0));
		int GetSec(TimeValue t, Interval& valid = Interval(0,0));
		int GetMon(TimeValue t, Interval& valid = Interval(0,0));
		int GetDay(TimeValue t, Interval& valid = Interval(0,0));
		int GetYr(TimeValue t, Interval& valid = Interval(0,0));

#if defined(NO_MOTION_PANEL)
		static bool IsCreateModeActive();
#endif	// defined(NO_MOTION_PANEL)
		
		// The slave controllers call this method to retrieve their
		// value.  This allows the master object to control the entire
		// system.
		void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method, int id);
		// This method is used to set the value of the controller
		// at the specified time.
		void SetValue(TimeValue t, void *val, int commit, GetSetMethod method, int id);
		// This makes sure that the spinner controls track when animating 
		// and in Motion Panel.
		void UpdateUI(TimeValue t);

		HWND hMasterParams;

		// The following static variables are 'class variables'.   

		// The window handle of the rollup page
		//static HWND hMasterParams;
		//This won't work if you have multiple animated systems trying to update the same
		//UI controls

		// The interface pointer for calling functions provided by MAX.
		static IObjParam *iObjParams;


		// The spinner controls...
		static ISpinnerControl *radSpin;
		static ISpinnerControl *latSpin;
		static ISpinnerControl *longSpin;
		static ISpinnerControl *hourSpin;
		static ISpinnerControl *minSpin;
		static ISpinnerControl *secSpin;
		static ISpinnerControl *yearSpin;
		static ISpinnerControl *monthSpin;
		static ISpinnerControl *daySpin;
		static ISpinnerControl *northSpin;
		static ISpinnerControl *zoneSpin;
		static ICustStatus *altEdit;	
		static ICustStatus	*azEdit;	
		static ICustStatus	*cityDisplay;
		// icon
		static HIMAGELIST hIconImages;	
		
		// --- Inherited virtual methods from Animatable ---

		// This method returns the number of sub-anims this plug-in
		// has.  Is has one, the parameter block.
		int NumSubs()  { 
			return daylightSystem ? 3 : 1;
		}
		// This methods returns the ith sub-anim (parameter)
		Animatable* SubAnim(int i);
		// This method returns the name of the ith sub-anim.
		TSTR SubAnimName(int i);

		// russom - August 16, 2006 - 775527
		BOOL CanAssignController(int subAnim);
		BOOL AssignController(Animatable *control,int subAnim);
		

		// This method returns the unique class ID of the plug-in
		Class_ID ClassID() { return (daylightSystem ? DAYLIGHT_CLASS_ID : SUNLIGHT_CLASS_ID); }  
		// This method returns the super class ID of the plug-in.  This
		// describes the type of plug-in this is.  The list of possible
		// values is defined by the system.
		SClass_ID SuperClassID() { return SYSTEM_CLASS_ID; }  

		// This method returns the name of the plug-in class.  This is 
		// just used in debugging.
		void GetClassName(TSTR& s);
		void DeleteThis() { delete this; }
		int IsKeyable(){ return 1;}
		BOOL GetNextKeyTime(TimeValue t,DWORD flags,TimeValue &nt);

		void BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev );
		void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next );

		// The master controller of a system plug-in should implement this 
		// method to give MAX a list of nodes that are part of the system.   
		void GetSystemNodes(INodeTab &nodes, SysNodeContext);

		// --- Inherited virtual methods from from ReferenceMaker/Target ---

		// reference the parameter block and the helper
		int NumRefs();
		// This method returns the ith reference.
		RefTargetHandle GetReference(int i);
		// This method sets the ith reference to the target passed.
private:
		virtual void SetReference(int i, RefTargetHandle rtarg);
public:
		// This method is called to copy the plug-in class.
		RefTargetHandle Clone(RemapDir& remap);
		void RefThePointClone();	// This is used to patch up thePoint ref on clone

		// This method recieves the change notification methods
		RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL);
		// These methods handle the loading and saving of the plug-ins
		// data.  See the Advanced Topics section on Loading and Saving
		// for more details.
		IOResult Save(ISave *isave);
		IOResult Load(ILoad *iload);

		// For TimeChangeCallback
		virtual void TimeChanged(TimeValue t);

		void ValuesUpdated();//called by daylight system
		void SetWeatherMDYHMS(TimeValue t,int month,int day, int year, int hour, int minute, int second, BOOL dst);

		//called when SetManual is called and we set the control type, also called when we undo/redo setting the manual.
		//this is a big function that's then notifies the daylight system which resets stuff up if the weather file is loaded.
		//and then also communicates with mrSun and mr Sky
		void NotifyNatLightManualChanged(IDaylightSystem2::DaylightControlType type);
	private:

		// Loads files that are considered legacy
		IOResult DoLoadLegacy(ILoad *iload);
		// Loads non-legacy files
		IOResult DoLoad(ILoad *iload);

		// Version number, saved into max file
		static const unsigned short kVERSION_01	= 1;
		static const unsigned short kVERSION_CURRENT = kVERSION_01;
		unsigned short mVerLoaded;
		
		TimeDimension mTimeDimension;
		static bool sInCreate;
};


 /*===========================================================================*\
 | Slave Controller Class:
\*===========================================================================*/

	
// The unique 32-bit Class IDs of the slave controller
#define SLAVE_CONTROL_CID1 0x77e3272e
#define SLAVE_CONTROL_CID2 0x13747060

#define DAYLIGHT_SLAVE_CONTROL_CID1 0x35924ed0
#define DAYLIGHT_SLAVE_CONTROL_CID2 0x59cf079c
 
class SlaveControl : public Control {
	public:		
		// This controller maintains a reference to the master.
		SunMaster *master;
		// This is the id of the slave controller. Passed to the master.
		ULONG id;
		// Does this belong to a daylight system?
		bool	daylightSystem;

		// Constructors, destructor, assignment operator.
		SlaveControl(bool daylight, BOOL loading=FALSE) : daylightSystem(daylight) { master = NULL; id = 0; }
		SlaveControl(const SlaveControl& ctrl);
		SlaveControl(const SunMaster* m, int i);
		void SetID( ULONG i) { id = i;}
		virtual ~SlaveControl() {}	
		SlaveControl& operator=(const SlaveControl& ctrl);

		// --- Inherited virtual methods from Control ---
		void Copy(Control *from) {}
		void CommitValue(TimeValue t) {}
		void RestoreValue(TimeValue t) {}
		// If a controller has sub-controllers or references it is not
		// a leaf controller. 
		virtual BOOL IsLeaf() {return FALSE;}
		// This method is used to retrieve the value of the controller
		// at the specified time.
		void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method=CTRL_ABSOLUTE);
		// This method is used to set the value of the controller
		// at the specified time.
		void SetValue(TimeValue t, void *val, int commit=1, GetSetMethod method=CTRL_ABSOLUTE);

		// russom - August 16, 2006 - 775527
		BOOL CanAssignController(int subAnim){ return master->CanAssignController(subAnim); }
		BOOL AssignController(Animatable *control,int subAnim) { return master->AssignController(control, subAnim); }

		BOOL IsReplaceable() {return FALSE;}

		// --- Inherited virtual methods from Animatable ---
		void* GetInterface(ULONG id);
		int NumSubs()  { return master ? master->NumSubs() : 0; }
		Animatable* SubAnim(int i) { return master->SubAnim(i); }
		TSTR SubAnimName(int i) { return master->SubAnimName(i); }
		BOOL BypassTreeView(){return TRUE;}

		// If an anim doesn't want to be copied in track view it can implement
		// this method to return FALSE.
		BOOL CanCopyAnim() {return FALSE;}

		Class_ID ClassID()
			{ return daylightSystem
				? Class_ID(DAYLIGHT_SLAVE_CONTROL_CID1,DAYLIGHT_SLAVE_CONTROL_CID2)
				: Class_ID(SLAVE_CONTROL_CID1,SLAVE_CONTROL_CID2); }  
		SClass_ID SuperClassID();
		void GetClassName(TSTR& s);
		void DeleteThis() { delete this; }		
		int IsKeyable(){ return 1;}
		BOOL GetNextKeyTime(TimeValue t,DWORD flags,TimeValue &nt){return master->GetNextKeyTime(t,flags,nt);};

		//  This implementation simply calls the same method of the SunMaster
		// object.  This is done because the UI controls managed by
		// the ring master control the entire system.
		void BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev) { assert(master); master->BeginEditParams(ip,flags,prev); } 
		// This is called to end the parameter editing.  Again, the 
		// sun master method is called.
		void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next) { if (master != NULL) master->EndEditParams(ip,flags,next); } 

		// --- Inherited virtual methods from ReferenceMaker and Target ---
		RefTargetHandle Clone(RemapDir& remap);
		// The slave controller has a single reference to the sun master.
		int NumRefs() { return 1; };	
		RefTargetHandle GetReference(int i)  
			{ assert(i==0); return master; }
private:
		virtual void SetReference(int i, RefTargetHandle rtarg) 
			{ assert(i==0); master = (SunMaster *)rtarg; }		
public:
		RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL) {return REF_SUCCEED;}
		// These methods load and save the controller data from/to the MAX file
		IOResult Save(ISave *isave);
		IOResult Load(ILoad *iload);

		////////////////////////////////////////////////////////////////////////////
		// API METHODS
		////////////////////////////////////////////////////////////////////////////
		/*
		| Other applications might like to get some info from a (the) sun. 
		| The following methods can be called.
		|
		| void GetSunTime( TimeValue t, SYSTEMTIME&	sunt )
		|
		| This method provides access to the solar time (GMT) at the time requested.
		|
		| void GetSunLoc(TimeValue t, Point2& origin, Point2& orient)
		|
		| This method provides access to the latitude and longitude as well as the 
		| azimuth and altitude at the requested time.
		| After the call:
		| 
		| origin.x = latitude
		| origin.y = longitude
		| orient.x = azimuth
		| orient.y = altitude
		|
		////////////////////////////////////////////////////////////////////////////
		*/
		__declspec(dllexport) virtual void GetSunTime( TimeValue t, SYSTEMTIME&	sunt );
		__declspec(dllexport) virtual void GetSunLoc(TimeValue t, Point2& origin, Point2& orient);
	};


