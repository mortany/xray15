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

//	This provides definitions for all versions of CAT,
//	including sub-versioning for rig and motion presets,
//	and probably anything else that comes along...

// All CAT versions grow from here.  A version is its
// number multiplied by 1000.  The result is to be treated
// as a DWORD.

#define CAT_VERSION_2521			((DWORD) 2521)
#define CAT_VERSION_2520			((DWORD) 2520)
#define CAT_VERSION_2519			((DWORD) 2519)
#define CAT_VERSION_2515			((DWORD) 2515)
#define CAT_VERSION_2510			((DWORD) 2510)
#define CAT_VERSION_2502			((DWORD) 2502)
#define CAT_VERSION_2500			((DWORD) 2500)
#define CAT_VERSION_2450			((DWORD) 2450)
#define CAT_VERSION_2447			((DWORD) 2447)
#define CAT_VERSION_2445			((DWORD) 2445)
#define CAT_VERSION_2441			((DWORD) 2441)
#define CAT_VERSION_2438			((DWORD) 2438)
#define CAT_VERSION_2436			((DWORD) 2436)
#define CAT_VERSION_2435			((DWORD) 2435)
#define CAT_VERSION_2433			((DWORD) 2433)
#define CAT_VERSION_2432			((DWORD) 2432)
#define CAT_VERSION_2431			((DWORD) 2431)
#define CAT_VERSION_2430			((DWORD) 2430)
#define CAT_VERSION_2420			((DWORD) 2420)
#define CAT_VERSION_2400			((DWORD) 2400)
#define CAT_VERSION_2300			((DWORD) 2300)
#define CAT_VERSION_2200			((DWORD) 2200)
#define CAT_VERSION_2110			((DWORD) 2110)
#define CAT_VERSION_2100			((DWORD) 2100)
#define CAT_VERSION_2000			((DWORD) 2000)
#define CAT_VERSION_1800			((DWORD) 1800)
#define CAT_VERSION_1790			((DWORD) 1790)
#define CAT_VERSION_1740			((DWORD) 1730)
#define CAT_VERSION_1730			((DWORD) 1730)
#define CAT_VERSION_1720			((DWORD) 1720)
#define CAT_VERSION_1710			((DWORD) 1710)
//1st CAT2 beta
#define CAT_VERSION_1700			((DWORD) 1700)

// Last Point release before CAT2
// majior new mocap importers etc..
#define CAT_VERSION_1400			((DWORD) 1400)

#define CAT_VERSION_1330			((DWORD) 1330)		// fixer number 3
#define CAT_VERSION_1320			((DWORD) 1320)		// fixer number two
#define CAT_VERSION_1301			((DWORD) 1301)		// Post Siggraph Point Release
#define CAT_VERSION_1300			((DWORD) 1300)		// Post Siggraph Point Release
#define CAT_VERSION_1210			((DWORD) 1210)		// Post Siggraph Point Release
#define CAT_VERSION_1205			((DWORD) 1205)		// Post Siggraph Point Release

#define CAT_VERSION_1201			((DWORD) 1201)		// 1.201 Second point release
#define CAT_VERSION_1200			((DWORD) 1200)		// 1.200 Second point release in development
#define CAT_VERSION_1151			((DWORD) 1151)		// 1.150 point release patch
#define CAT_VERSION_1150			((DWORD) 1150)		// 1.150 Second point release
#define CAT_VERSION_1100			((DWORD) 1100)		// 1.100 First point release (Tue 11 Dec 2003 3:00am)
#define CAT_VERSION_1000			((DWORD) 1000)		// 1.000 First public release (Tue 11 Nov 2003 3:00am)
#define CAT_VERSION_0550			((DWORD) 550)		// 0.550 (Beta) -- nobody versioned 0.54 =(
#define CAT_VERSION_0530			((DWORD) 530)		// 0.530 (Beta) -- we missed versioning for 0.51 and 0.52 =/
#define CAT_VERSION_0500			((DWORD) 500)		// 0.500 (1st Public Beta)
#define CAT_VERSION_0420			((DWORD) 420)		// 0.420 (Beta)
#define CAT_VERSION_0370			((DWORD) 370)		// 0.370 (Beta) -- the first release with versioning
#define CAT_VERSION_0360			((DWORD) 360)		// 0.360 (Beta) -- the last release without versioning

///////////////////////////////////////////////////////////////////

#define CAT3_VERSION_2700			((DWORD) 2700)
#define CAT3_VERSION_2704			((DWORD) 2704)
#define CAT3_VERSION_2706			((DWORD) 2706)
#define CAT3_VERSION_2707			((DWORD) 2707)
#define CAT3_VERSION_2708			((DWORD) 2708)
#define CAT3_VERSION_2709			((DWORD) 2709)
#define CAT3_VERSION_2800			((DWORD) 2800)
#define CAT3_VERSION_2801			((DWORD) 2801)
#define CAT3_VERSION_2802			((DWORD) 2802)
#define CAT3_VERSION_3000			((DWORD) 3000)
#define CAT3_VERSION_3001			((DWORD) 3001)
#define CAT3_VERSION_3003			((DWORD) 3003)
#define CAT3_VERSION_3004			((DWORD) 3004)
#define CAT3_VERSION_3005			((DWORD) 3005)
#define CAT3_VERSION_3010			((DWORD) 3010)
#define CAT3_VERSION_3011			((DWORD) 3011)
#define CAT3_VERSION_3013			((DWORD) 3012)
#define CAT3_VERSION_3014			((DWORD) 3014)
#define CAT3_VERSION_3015			((DWORD) 3015)
#define CAT3_VERSION_3016			((DWORD) 3016)
#define CAT3_VERSION_3100			((DWORD) 3100)
#define CAT3_VERSION_3110			((DWORD) 3110)
#define CAT3_VERSION_3150			((DWORD) 3150)
#define CAT3_VERSION_3200			((DWORD) 3200)
#define CAT3_VERSION_3210			((DWORD) 3210)
#define CAT3_VERSION_3220			((DWORD) 3220)
#define CAT3_VERSION_3230			((DWORD) 3230)
#define CAT3_VERSION_3300			((DWORD) 3300)
#define CAT3_VERSION_3310			((DWORD) 3310)	// Licensing Removed
#define CAT3_VERSION_3316			((DWORD) 3316)	// Renoir BEta
#define CAT3_VERSION_3317			((DWORD) 3317)	//
#define CAT3_VERSION_3500			((DWORD) 3500)	//

///////////////////////////////////////////////////////////////////
#define CAT_VERSION_CURRENT			CAT3_VERSION_3500

