/**********************************************************************
 *<
	FILE:			OSDInterfaceTest.cpp

	DESCRIPTION:	OpenSubdiv Interface Test Utility

	CREATED BY:		Tom Hudson

	HISTORY:		Created Wednesday, October 15, 2014

 *>	Copyright (c) 2014 Autodesk, Inc., All Rights Reserved.
 **********************************************************************/

/**********************************************************************

This Utility demonstrates how to determine if an object in a scene is
using the Pixar OpenSubdiv subdivision capability, whether provided by
the OpenSubdivMod modifier or some other method, and how to retrieve
the parameters required by the OpenSubdiv library.

In this way, renderers or other features can do their own subdivision
as required.

***********************************************************************/

#include "OSDInterfaceTest.h"
#include <strbasic.h>
#include <maxvariant.h>

extern PolyLibExport Class_ID polyObjectClassID;

HINSTANCE hInstance;

#define PROPERTYTEST_CLASS_ID	Class_ID(0x9cbb35a1,0xcc3ab295)

// Handy dummy view for testing the mesh generation
class NullView : public View
{
public:
	Point2 ViewToScreen(Point3 p)
		{ return Point2(p.x,p.y); }
	NullView() {
		worldToView.IdentityMatrix();
		screenW=640.0f; screenH = 480.0f;
	}
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved)
{
	switch (fdwReason) {
		case DLL_PROCESS_ATTACH:
			MaxSDK::Util::UseLanguagePackLocale();
			hInstance = hinstDLL;
			DisableThreadLibraryCalls(hInstance);
			break;
		case DLL_PROCESS_DETACH:
			break;
	}

	return (TRUE);
}

TCHAR *GetString(int id)
{
	static TCHAR buf[256];

	if (hInstance)
		return LoadString(hInstance, id, buf, _countof(buf)) ? buf : NULL;
	return NULL;
}

__declspec( dllexport ) const TCHAR* LibDescription()
{
	return GetString(IDS_LIBDESC);
}

/// MUST CHANGE THIS NUMBER WHEN ADD NEW CLASS
__declspec( dllexport ) int LibNumberClasses()
{
	return 1;
}

__declspec( dllexport ) ClassDesc* LibClassDesc(int i)
{
	switch(i) {
		case 0: return GetOSDInterfaceTestDesc();
		default: return 0;
	}
}

__declspec( dllexport ) ULONG LibVersion()
{
	return VERSION_3DSMAX;
}

// Let the plug-in register itself for deferred loading
__declspec( dllexport ) ULONG CanAutoDefer()
{
	return 1;
}

static OSDInterfaceTest theOSDInterfaceTest;

class OSDInterfaceTestClassDesc:public ClassDesc {
	public:
	int 			IsPublic() {return 1;}
	void *			Create(BOOL loading = FALSE) {return &theOSDInterfaceTest;}
	const TCHAR *	ClassName() {return GetString(IDS_CLASSNAME);}
	SClass_ID		SuperClassID() {return UTILITY_CLASS_ID;}
	Class_ID		ClassID() {return PROPERTYTEST_CLASS_ID;}
	const TCHAR* 	Category() {return _T("");}
};

static OSDInterfaceTestClassDesc OSDInterfaceTestDesc;
ClassDesc* GetOSDInterfaceTestDesc() {return &OSDInterfaceTestDesc;}

PickTarget	OSDInterfaceTest::pickCB;

static INT_PTR CALLBACK OSDInterfaceTestDlgProc(
		HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ICustButton *but;
	switch (msg) {
		case WM_INITDIALOG:
			// Define the button
			but = GetICustButton(GetDlgItem(hWnd, IDC_PICK_OBJECT));
			but->SetType(CBT_CHECK);
			ReleaseICustButton(but);

			theOSDInterfaceTest.Init(hWnd);
			break;

		case WM_DESTROY:
			theOSDInterfaceTest.Destroy(hWnd);
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_PICK_OBJECT:
					if (theOSDInterfaceTest.ip->GetCommandMode()->ID() == CID_STDPICK) {
						theOSDInterfaceTest.ip->SetStdCommandMode(CID_OBJMOVE);
						return FALSE;
						}
					theOSDInterfaceTest.pickCB.it = &theOSDInterfaceTest;
					theOSDInterfaceTest.ip->SetPickMode(&theOSDInterfaceTest.pickCB);
					return TRUE;                  
			}
			break;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
			theOSDInterfaceTest.ip->RollupMouseMessage(hWnd,msg,wParam,lParam); 
			break;

		default:
			return FALSE;
	}
	return TRUE;
}	

OSDInterfaceTest::OSDInterfaceTest()
{
	iu = NULL;
	ip = NULL;	
	hPanel = NULL;
}

OSDInterfaceTest::~OSDInterfaceTest()
{
}

void OSDInterfaceTest::BeginEditParams(Interface *ip,IUtil *iu) 
{
	this->iu = iu;
	this->ip = ip;
	hPanel = ip->AddRollupPage(
		hInstance,
		MAKEINTRESOURCE(IDD_PANEL),
		OSDInterfaceTestDlgProc,
		GetString(IDS_PARAMETERS),
		0);
	ClearInfoStrings();
}
	
void OSDInterfaceTest::EndEditParams(Interface *ip,IUtil *iu) 
{
	this->iu = NULL;
	this->ip = NULL;
	ip->DeleteRollupPage(hPanel);
	hPanel = NULL;
}

void OSDInterfaceTest::Init(HWND hWnd)
{
}

void OSDInterfaceTest::Destroy(HWND hWnd)
{
}

void OSDInterfaceTest::ClearInfoStrings()
{
	// Clear information strings
	SetDlgItemText(hPanel, IDC_LINE1, _T(""));
	SetDlgItemText(hPanel, IDC_LINE2, _T(""));
	SetDlgItemText(hPanel, IDC_LINE3, _T(""));
	SetDlgItemText(hPanel, IDC_LINE4, _T(""));
	SetDlgItemText(hPanel, IDC_LINE5, _T(""));
	SetDlgItemText(hPanel, IDC_LINE6, _T(""));
	SetDlgItemText(hPanel, IDC_LINE7, _T(""));
	SetDlgItemText(hPanel, IDC_LINE8, _T(""));
	SetDlgItemText(hPanel, IDC_LINE9, _T(""));
}

void OSDInterfaceTest::CheckForOSDInterface(INode *node)
{
	// Evaluate the node at this point in time
	TimeValue t = ip->GetTime();
	ObjectState objState = node->EvalWorldState(t);

	// Test for OSD interface availability
	OpenSubdivParameters *i = (OpenSubdivParameters *)node->GetInterface(OSD_PARAMETER_INTERFACE);
	if(i) {
		// Got the interface, display its contents
		TSTR work;
		TSTR trueString = GetString(IDS_TRUE);
		TSTR falseString = GetString(IDS_FALSE);
		work.printf(GetString(IDS_LINE1_PATTERN), i->TessellationLevel(t, FOREVER));
		SetDlgItemText(hPanel, IDC_LINE1, work);
		work.printf(GetString(IDS_LINE2_PATTERN), i->InterpolateBoundaryVert());
		SetDlgItemText(hPanel, IDC_LINE2, work);
		work.printf(GetString(IDS_LINE3_PATTERN), i->InterpolateBoundaryFVar());
		SetDlgItemText(hPanel, IDC_LINE3, work);
		work.printf(GetString(IDS_LINE4_PATTERN), i->CreaseMethod());
		SetDlgItemText(hPanel, IDC_LINE4, work);
		work.printf(GetString(IDS_LINE5_PATTERN), (i->SmoothTriangle() == true) ? trueString : falseString);
		SetDlgItemText(hPanel, IDC_LINE5, work);
		work.printf(GetString(IDS_LINE6_PATTERN), (i->PropagateCorners() == true) ? trueString : falseString);
		SetDlgItemText(hPanel, IDC_LINE6, work);
		work.printf(GetString(IDS_LINE7_PATTERN), (i->Adaptive() == true) ? trueString : falseString);
		SetDlgItemText(hPanel, IDC_LINE7, work);
		work.printf(GetString(IDS_LINE8_PATTERN), i->AdaptiveTessellationLevel(t, FOREVER));
		SetDlgItemText(hPanel, IDC_LINE8, work);

		// Now get information on the object.
		Object *obj = objState.obj;

		// If it's an adaptive OSD, we can get the cage by a simple evaluation...
		if(i->Adaptive()) {
			// OSD objects use PolyObjects to store their cages (crease values stored in vertex and edge data)
			if(obj->IsSubClassOf(polyObjectClassID)) {
				PolyObject *cageObj = (PolyObject *)obj;
				// Display cage information
				work.printf(GetString(IDS_CAGE_PATTERN), cageObj->GetMesh().numv, cageObj->GetMesh().numf);
				SetDlgItemText(hPanel, IDC_LINE9, work);
			}
			else
				DbgAssert(0);	// This should never happen
		}
		else {
			// Not adaptive; it's going to contain the final uniform mesh -- Let's get it
			if(obj->SuperClassID() == GEOMOBJECT_CLASS_ID ) {
				BOOL needDelete = FALSE;
				NullView view;
				Mesh *theMesh = ((GeomObject *)obj)->GetRenderMesh(t, node, view, needDelete);
				if(theMesh) {
					// Display mesh information
					work.printf(GetString(IDS_MESH_PATTERN), theMesh->numVerts, theMesh->numFaces);
					SetDlgItemText(hPanel, IDC_LINE9, work);
					// Delete mesh if necessary
					if (needDelete) delete theMesh;
				}
				else
					DbgAssert(0);	// This should never happen
			}
			else
				DbgAssert(0);	// This should never happen
		}
	}
	else {
		ClearInfoStrings();
		SetDlgItemText(hPanel, IDC_LINE1, GetString(IDS_NO_INTERFACE));
	}
}

BOOL PickTarget::Filter(INode *node)
   {
	// We don't care what this is, just that it's a valid node
	return node ? TRUE : FALSE;
   }

BOOL PickTarget::HitTest(
      IObjParam *ip,HWND hWnd,ViewExp *vpt,IPoint2 m,int flags)
   {
	INode *node = ip->PickNode(hWnd,m,this);
	TimeValue t = ip->GetTime();
	
	return node ? TRUE : FALSE;
   }

BOOL PickTarget::Pick(IObjParam *ip,ViewExp *vpt)
   {
	INode *node = vpt->GetClosestHit();
	TimeValue t = ip->GetTime();
	if(!node) {
		DbgAssert(0);
		return FALSE;
	}

	it->CheckForOSDInterface(node);

	return TRUE;
	}

void PickTarget::EnterMode(IObjParam *ip)
   {
   ICustButton *iBut = GetICustButton(GetDlgItem(it->hPanel,IDC_PICK_OBJECT));
   iBut->SetCheck(TRUE);
   ReleaseICustButton(iBut);
   }

void PickTarget::ExitMode(IObjParam *ip)
   {
   ICustButton *iBut = GetICustButton(GetDlgItem(it->hPanel,IDC_PICK_OBJECT));
   iBut->SetCheck(FALSE);
   ReleaseICustButton(iBut);
   }

