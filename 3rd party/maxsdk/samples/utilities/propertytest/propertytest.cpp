/**********************************************************************
 *<
	FILE:			PropertyTest.cpp

	DESCRIPTION:	PropertySet Test Utility

	CREATED BY:		Christer Janson

	HISTORY:		Created Tuesday, September 22, 1998

 *>	Copyright (c) 1998, All Rights Reserved.
 **********************************************************************/

/**********************************************************************

This Utility demonstrates how to access the property sets stored in 
the MAX Scene. There are three property sets, the SummaryInformation,
the DocumentSummaryInformation and the UserDefinedProperties.
SummaryInformation and DocumentSummaryInformation are standard property
sets with a fixed set of properties, and the UserDefinedProperty set
contains (surprise) user defined properties.
One benefit with these property sets is that anyone outside of MAX
can parse them as they are stored in the MAX file in a standard way.

This Utility, however, demonstrates the property API inside of MAX and
will only work on the currently loaded scene.

Summary Information
===================
These are the relevant entries in the SummaryInformation
property set:

Property Name	PropertyID Str.	Prop. ID	VT Type
==============	===============	==========	======================
Title			PID_TITLE		0x00000002	VT_LPSTR 
Subject			PID_SUBJECT		0x00000003	VT_LPSTR
Author			PID_AUTHOR		0x00000004	VT_LPSTR
Keywords		PID_KEYWORDS	0x00000005	VT_LPSTR
Comments		PID_COMMENTS	0x00000006	VT_LPSTR
Thumbnail		PID_THUMBNAIL	0x00000011	VT_CF		*)

*) The thumbnail is generated on save and then thrown away.
It is part of the property set in the MAX file, but it is not
in the scene while MAX is running.

Document Summary Information
============================
These are the relevant entries in the DocumentSummaryInformation
property set:

Property Name	PropertyID Str.	Prop. ID	VT Type
==============	===============	==========	======================
Category		PID_CATEGORY	0x00000002	VT_LPSTR
HeadingPairs	PID_HEADINGPAIR	0x0000000C	VT_VARIANT | VT_VECTOR
TitlesofParts	PID_DOCPARTS	0x0000000D	VT_LPSTR | VT_VECTOR
Manager			PID_MANAGER		0x0000000E	VT_LPSTR
Company			PID_COMPANY		0x0000000F	VT_LPSTR

User Defined Properties
=======================
These are all custom properties.
The internal structure supports any property type, but the
user interface in the MAX property editor supports only
Text, Number, BOOL and Date.
You can use this API to manage other types in the user defined
property set.

***********************************************************************/

#include "PropertyTest.h"
#include <strbasic.h>
#include <maxvariant.h>

HINSTANCE hInstance;

#define PROPERTYTEST_CLASS_ID	Class_ID(0xb3f3a6cc,0x9a708091)

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
		case 0: return GetPropertyTestDesc();
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

static PropertyTest thePropertyTest;

class PropertyTestClassDesc:public ClassDesc {
	public:
	int 			IsPublic() {return 1;}
	void *			Create(BOOL loading = FALSE) {return &thePropertyTest;}
	const TCHAR *	ClassName() {return GetString(IDS_CLASSNAME);}
	SClass_ID		SuperClassID() {return UTILITY_CLASS_ID;}
	Class_ID		ClassID() {return PROPERTYTEST_CLASS_ID;}
	const TCHAR* 	Category() {return _T("");}
};

static PropertyTestClassDesc PropertyTestDesc;
ClassDesc* GetPropertyTestDesc() {return &PropertyTestDesc;}


static INT_PTR CALLBACK PropertyTestDlgProc(
		HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG:
			thePropertyTest.Init(hWnd);
			break;

		case WM_DESTROY:
			thePropertyTest.Destroy(hWnd);
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_CLOSEBUTTON:
					thePropertyTest.iu->CloseUtility();
					break;
				case IDC_SHOWPROP:
					thePropertyTest.ShowProperties();
					break;
			}
			break;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
			thePropertyTest.ip->RollupMouseMessage(hWnd,msg,wParam,lParam); 
			break;

		default:
			return FALSE;
	}
	return TRUE;
}	

PropertyTest::PropertyTest()
{
	iu = NULL;
	ip = NULL;	
	hPanel = NULL;
}

PropertyTest::~PropertyTest()
{
}

void PropertyTest::BeginEditParams(Interface *ip,IUtil *iu) 
{
	this->iu = iu;
	this->ip = ip;
	hPanel = ip->AddRollupPage(
		hInstance,
		MAKEINTRESOURCE(IDD_PANEL),
		PropertyTestDlgProc,
		GetString(IDS_PARAMETERS),
		0);
}
	
void PropertyTest::EndEditParams(Interface *ip,IUtil *iu) 
{
	this->iu = NULL;
	this->ip = NULL;
	ip->DeleteRollupPage(hPanel);
	hPanel = NULL;
}

void PropertyTest::Init(HWND hWnd)
{
}

void PropertyTest::Destroy(HWND hWnd)
{
}

INT_PTR CALLBACK staticPropDlgProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
void VariantToString(PROPVARIANT* pProp, TCHAR* szString, int bufSize);
void TypeNameFromVariant(PROPVARIANT* pProp, TCHAR* szString, int bufSize);

void PropertyTest::ShowProperties()
{
	DialogBox(hInstance,
			  MAKEINTRESOURCE(IDD_PROPERTYDIALOG),
			  ip->GetMAXHWnd(),
			  (DLGPROC)staticPropDlgProc);
	}

INT_PTR CALLBACK staticPropDlgProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
	{
	return thePropertyTest.PropDlgProc(hWnd,message,wParam,lParam);
	}


INT_PTR PropertyTest::PropDlgProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
	{

	switch (message) {
		case WM_INITDIALOG:
			{
			SetCursor(LoadCursor(NULL,IDC_ARROW));
			CenterWindow(hWnd, GetParent(hWnd));

			// Arrange the columns in the list view
			LV_COLUMN column;
			column.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
			column.fmt = LVCFMT_LEFT;
			column.pszText = GetString(IDS_NAME);
			column.cx = 80;
			ListView_InsertColumn(GetDlgItem(hWnd, IDC_CUSTOM), 0, &column);
			column.pszText = GetString(IDS_VALUE);
			ListView_InsertColumn(GetDlgItem(hWnd, IDC_CUSTOM), 1, &column);
			column.pszText = GetString(IDS_TYPE);
			ListView_InsertColumn(GetDlgItem(hWnd, IDC_CUSTOM), 2, &column);

			GetProperties(hWnd);
			}
			return 1;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
				case IDCANCEL:
					EndDialog(hWnd, 0);
					break;
				}
			return 1;
		}
	return 0;
	}
	
void PropertyTest::GetProperties(HWND hDlg)
	{
	HWND hContents = GetDlgItem(hDlg, IDC_CONTENTS);
	HWND hCustom = GetDlgItem(hDlg, IDC_CUSTOM);

	PROPSPEC	PropSpec;
	int			idx;

	PropSpec.ulKind = PRSPEC_PROPID;

	/////////////////////////////////////////////////////////////////////////
	//
	// The General Page
	//
	//
	PropSpec.propid = PIDSI_TITLE;
	idx = ip->FindProperty(PROPSET_SUMMARYINFO, &PropSpec);
	if (idx != -1) {
		const PROPVARIANT* prop = ip->GetPropertyVariant(PROPSET_SUMMARYINFO, idx);
		if (prop) {
			SetDlgItemText(hDlg, IDC_TITLE, MaxSDK::Util::VariantToString(prop).ToMCHAR());
			}
		}

	PropSpec.propid = PIDSI_SUBJECT;
	idx = ip->FindProperty(PROPSET_SUMMARYINFO, &PropSpec);
	if (idx != -1) {
		const PROPVARIANT* prop = ip->GetPropertyVariant(PROPSET_SUMMARYINFO, idx);
		if (prop) {
			SetDlgItemText(hDlg, IDC_SUBJECT, MaxSDK::Util::VariantToString(prop).ToMCHAR());
			}
		}

	PropSpec.propid = PIDSI_AUTHOR;
	idx = ip->FindProperty(PROPSET_SUMMARYINFO, &PropSpec);
	if (idx != -1) {
		const PROPVARIANT* prop = ip->GetPropertyVariant(PROPSET_SUMMARYINFO, idx);
		if (prop) {
			SetDlgItemText(hDlg, IDC_AUTHOR, MaxSDK::Util::VariantToString(prop).ToMCHAR());
			}
		}

	PropSpec.propid = PID_MANAGER;
	idx = ip->FindProperty(PROPSET_DOCSUMMARYINFO, &PropSpec);
	if (idx != -1) {
		const PROPVARIANT* prop = ip->GetPropertyVariant(PROPSET_DOCSUMMARYINFO, idx);
		if (prop) {
			SetDlgItemText(hDlg, IDC_MANAGER, MaxSDK::Util::VariantToString(prop).ToMCHAR());
			}
		}

	PropSpec.propid = PID_COMPANY;
	idx = ip->FindProperty(PROPSET_DOCSUMMARYINFO, &PropSpec);
	if (idx != -1) {
		const PROPVARIANT* prop = ip->GetPropertyVariant(PROPSET_DOCSUMMARYINFO, idx);
		if (prop) {
			SetDlgItemText(hDlg, IDC_COMPANY, MaxSDK::Util::VariantToString(prop).ToMCHAR());
			}
		}

	PropSpec.propid = PID_CATEGORY;
	idx = ip->FindProperty(PROPSET_DOCSUMMARYINFO, &PropSpec);
	if (idx != -1) {
		const PROPVARIANT* prop = ip->GetPropertyVariant(PROPSET_DOCSUMMARYINFO, idx);
		if (prop) {
			SetDlgItemText(hDlg, IDC_CATEGORY, MaxSDK::Util::VariantToString(prop).ToMCHAR());
			}
		}

	PropSpec.propid = PIDSI_KEYWORDS;
	idx = ip->FindProperty(PROPSET_SUMMARYINFO, &PropSpec);
	if (idx != -1) {
		const PROPVARIANT* prop = ip->GetPropertyVariant(PROPSET_SUMMARYINFO, idx);
		if (prop) {
			SetDlgItemText(hDlg, IDC_KEYWORDS, MaxSDK::Util::VariantToString(prop).ToMCHAR());
			}
		}


	PropSpec.propid = PIDSI_COMMENTS;
	idx = ip->FindProperty(PROPSET_SUMMARYINFO, &PropSpec);
	if (idx != -1) {
		const PROPVARIANT* prop = ip->GetPropertyVariant(PROPSET_SUMMARYINFO, idx);
		if (prop) {
			SetDlgItemText(hDlg, IDC_COMMENTS, MaxSDK::Util::VariantToString(prop).ToMCHAR());
			}
		}

	/////////////////////////////////////////////////////////////////////////
	//
	// The Contents Page
	//
	//

	const PROPVARIANT*	pPropHeading;
	const PROPVARIANT*	pPropDocPart;

	PropSpec.propid = PID_HEADINGPAIR;

	// First, get the PID_HEADINGPAIR property.
	// This property contains an array of the headings (titles) of the contents list.
	// For each heading, there is also a number, representing the number of docparts
	// for that heading. The docparts is an array of strings.
	if ((idx = ip->FindProperty(PROPSET_DOCSUMMARYINFO, &PropSpec)) != -1) {
		if ((pPropHeading = ip->GetPropertyVariant(PROPSET_DOCSUMMARYINFO, idx)) != NULL) {

			// Get the docparts.
			PropSpec.propid = PID_DOCPARTS;
			if ((idx = ip->FindProperty(PROPSET_DOCSUMMARYINFO, &PropSpec)) != -1) {
				if ((pPropDocPart = ip->GetPropertyVariant(PROPSET_DOCSUMMARYINFO, idx)) != NULL) {

					const CAPROPVARIANT*	pHeading = &pPropHeading->capropvar;
					std::vector<MaxSDK::Util::MaxString> pDocPart;

					MaxSDK::Util::VariantToStringVector(pPropDocPart, pDocPart);

					int nDocPart = 0;
					for (UINT i=0; i<pHeading->cElems; i+=2) {

						// For each heading:
						// Add the heading...

						SendMessage(hContents, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)MaxSDK::Util::VariantToString(pHeading->pElems[i]).ToMCHAR());
						for (int j=0; j<pHeading->pElems[i+1].lVal; j++) {

							// ...and then all the docparts for this heading
							// The docparts are tab indented to make it look better.
							TSTR tBuf;
							tBuf  = _T("\t");
							tBuf += pDocPart[nDocPart];
							SendMessage(hContents, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)tBuf.data());
							nDocPart++;
							}
						}
					}
				}
			}
		}

	/////////////////////////////////////////////////////////////////////////
	//
	// The Custom Page
	//
	//

	TCHAR	szBuf[80];
	int		numProps;

	ListView_DeleteAllItems(hCustom);

	numProps = ip->GetNumProperties(PROPSET_USERDEFINED);

	for (int i=0; i<numProps; i++) {
		const PROPSPEC* pPropSpec = ip->GetPropertySpec(PROPSET_USERDEFINED, i);
		const PROPVARIANT* pPropVar = ip->GetPropertyVariant(PROPSET_USERDEFINED, i);

		if (pPropSpec->ulKind == PRSPEC_PROPID) {
			_sntprintf_s(szBuf, _countof(szBuf), _TRUNCATE, _T("%ld"), pPropSpec->propid);
			}
		else {
			_tcsncpy_s(szBuf, _countof(szBuf), TSTR::FromOLESTR(pPropSpec->lpwstr), _TRUNCATE);
			}

		LV_ITEM item;
		item.mask = LVIF_TEXT;
		item.iItem = i;
		item.iSubItem = 0;
		item.pszText = szBuf;
		item.cchTextMax = static_cast<int>(_tcslen(szBuf));	// SR DCAST64: Downcast to 2G limit.
		ListView_InsertItem(hCustom, &item);

		VariantToString(pPropVar, szBuf, _countof(szBuf));
		item.iSubItem = 1;
		item.pszText = szBuf;
		item.cchTextMax = static_cast<int>(_tcslen(szBuf));	// SR DCAST64: Downcast to 2G limit.
		ListView_SetItem(hCustom, &item);

		TypeNameFromVariant(pPropVar, szBuf, _countof(szBuf));
		item.iSubItem = 2;
		item.pszText = szBuf;
		item.cchTextMax = static_cast<int>(_tcslen(szBuf));	// SR DCAST64: Downcast to 2G limit.
		ListView_SetItem(hCustom, &item);
		}
	}

// Get the type of a PROPVARIANT into readable format
//

void PropertyTest::TypeNameFromVariant(const PROPVARIANT* pProp, TCHAR* szString, int bufSize)
	{
	switch (pProp->vt) {
		case VT_LPWSTR:
		case VT_LPSTR:
		case VT_BSTR:
			_tcsncpy_s(szString, bufSize, GetString(IDS_PROPTEXT), _TRUNCATE);
			break;
		case VT_I4:
		case VT_R4:
      case VT_R8:
      // SR NOTE64: New types -- there could be several more
      case VT_I8:
      case VT_INT:
      case VT_INT_PTR:
		  _tcsncpy_s(szString, bufSize, GetString(IDS_PROPNUMBER), _TRUNCATE);
			break;
		case VT_BOOL:
			_tcsncpy_s(szString, bufSize, GetString(IDS_PROPYESNO), _TRUNCATE);
			break;
		case VT_FILETIME:
			_tcsncpy_s(szString, bufSize, GetString(IDS_PROPDATE), _TRUNCATE);
			break;
		default:
			_tcsncpy_s(szString, bufSize, _T(""), _TRUNCATE);
			break;
		}
	}

// Convert (well, copy) a PROPVARIANT into a string
//
void PropertyTest::VariantToString(const PROPVARIANT* pProp, TCHAR* szString, int bufSize)
	{
	switch (pProp->vt) {
		case VT_BSTR:
		case VT_LPWSTR:
			_tcsncpy_s(szString, bufSize, TSTR::FromUTF16(pProp->pwszVal), _TRUNCATE);
			break;
		case VT_LPSTR:
			_tcsncpy_s(szString, bufSize, TSTR::FromACP(pProp->pszVal), _TRUNCATE);
			break;
#if !defined( _WIN64 )
      case VT_INT_PTR:        // PROPVARIANT doesn't have the INT_PTR support built-in
#endif
		case VT_I4:
			_sntprintf_s(szString, bufSize, _TRUNCATE, _T("%ld"), pProp->lVal);
			break;
#if defined( _WIN64 )
      case VT_INT_PTR:        // PROPVARIANT doesn't have the INT_PTR support built-in
#endif
      case VT_I8:            
         _sntprintf_s(szString, bufSize, _TRUNCATE, _T("%I64d"), pProp->hVal.QuadPart);
         break;
      case VT_INT:
         _sntprintf_s(szString, bufSize, _TRUNCATE, _T("%ld"), V_INT(pProp));
         break;
		case VT_R4:
			_sntprintf_s(szString, bufSize, _TRUNCATE, _T("%f"), pProp->fltVal);
			break;
		case VT_R8:
			_sntprintf_s(szString, bufSize, _TRUNCATE, _T("%lf"), pProp->dblVal);
			break;
		case VT_BOOL:
			_sntprintf_s(szString, bufSize, _TRUNCATE, _T("%s"), pProp->boolVal ? GetString(IDS_PROPYES) : GetString(IDS_PROPNO));
			break;
		case VT_FILETIME:
			SYSTEMTIME sysTime;
			FileTimeToSystemTime(&pProp->filetime, &sysTime);
			GetDateFormat(LOCALE_SYSTEM_DEFAULT,
						  DATE_SHORTDATE,
						  &sysTime,
						  NULL,
						  szString,
						  bufSize);
			break;
		default:
			_tcsncpy_s(szString, bufSize, _T(""), _TRUNCATE);
			break;
		}
	}
