/**********************************************************************
 *
 * FILE:        CIN.H
 * AUTHOR:      greg finch
 *
 * DESCRIPTION: SMPTE Digital Picture Exchange Format,
 *              SMPTE CIN, Kodak Cineon
 *
 * CREATED:     20 june, 1997
 *
 * $Log: cin.h,v $
 * Revision 1.2  1999/05/18 23:31:34  lshuler
 * Milestone One check in. Includes updated files and status files.
 *
 * 
 * Copyright (c) 1997, All Rights Reserved.
 **********************************************************************/

#include <max.h>
#include <bmmlib.h>
#include "gamma.h"
#include "resource.h"

#ifndef _CIN_H_
#define _CIN_H_

#include "cineon.h"

#define CIN_BITMAPIO_VERSION     0x01
#define CIN_ClassID              Class_ID(0x5dd872f0, 0x664308eb)
//#define CIN_IMAGE_INFO_VERSION   0x0001

#define CIN_USERDATA_VERSION     1
#define CIN_INI_FILENAME    _T("Cineon.ini")
#define CIN_INI_SECTION     _T("Default Settings")
#define CIN_INI_REF_WHITE   _T("Reference White")
#define CIN_INI_REF_BLACK   _T("Reference Black")
#define CIN_INI_VERSION     _T("Current Version")

// defined in dllmain.cpp
extern HINSTANCE hInst;

class File
{
public:
    FILE*   mStream;

    File        (const TCHAR *filename, const TCHAR *mode)    
	{
		// for a360 support - allows binary diff syncing
		if (mode != nullptr && (mode[0] == _T('w') || mode[0] == _T('a') || (mode[0] == _T('r') && mode[1] == _T('+') )))
		{
			MaxSDK::Util::Path storageNamePath(filename);
			storageNamePath.SaveBaseFile();
		}
		mStream = _tfopen(filename,mode);
	}
   ~File        ()                                  {Close();}
    void Close  ()                                  {if (mStream) fclose(mStream); mStream = NULL;}
};

TCHAR*  GetResIDCaption(int id);

//--------------------< struct to hold user data >---------------------

struct CIN_UserData {
	DWORD	version;        // the user data version
    int     mRefWhite;
    int     mRefBlack;
    CIN_UserData()
        { version   = CIN_USERDATA_VERSION;
          mRefWhite = REFERENCE_WHITE_CODE;
          mRefBlack = REFERENCE_BLACK_CODE;
        }
   ~CIN_UserData() {}
};

//-----------------------------------------------------------------------------
//-- Class Definition ---------------------------------------------------------
//

class BitmapIO_CIN : public BitmapIO, public BitmapIOMetaData {
private:
    void            GetINIFilename(TCHAR*);

public:
    CineonFile      mCineonImage;    //FIXME move to private
    CIN_UserData    mUserData;          // move to private
    //int             mRefWhite;
    //int             mRefBlack;
    BitmapIO_CIN();
   ~BitmapIO_CIN();

 // from BitmapIO
    const TCHAR*    AuthorName(void);
    int             Capability();
    DWORD           ChannelsRequired();
    int             Close(int);
    const TCHAR*    CopyrightMessage(void);
    unsigned long   EvaluateConfigure(void);
    const TCHAR*    Ext(int n);
    int             ExtCount();
    unsigned short  GetImageInfo(BitmapInfo*);
    BMMRES          GetImageInfoDlg(HWND, BitmapInfo*, const TCHAR*);
    BitmapStorage*  Load(BitmapInfo*, Bitmap*, unsigned short*);
    const TCHAR*    LongDesc(void);
    int             LoadConfigure(void*, DWORD);
    BMMRES          OpenOutput(BitmapInfo*, Bitmap*);
    int             SaveConfigure(void*);
    const TCHAR*    ShortDesc(void);
    void            ShowAbout(HWND);
    BOOL            ShowControl(HWND, DWORD);
    BOOL            ShowImage(HWND, BitmapInfo*);
    unsigned int    Version(void);
    BMMRES          Write(int);

    INT_PTR         ImageInfoDialog(HWND, UINT, WPARAM, LPARAM);
    void            ReadINI();
    void            WriteINI();

    // BitmapIOMetaData
    virtual bool   UseLinearStorage(bool save);

    // extension
    BaseInterface  *GetInterface(Interface_ID id);
};

//--------------------< CIN's Class Description >---------------------
class CIN_ClassDesc : public ClassDesc {
public:
    int             IsPublic()              {return TRUE;}
    void*           Create(BOOL loading)    {return new BitmapIO_CIN;}
    const TCHAR*    ClassName()             {return GetResIDCaption(IDS_CIN);}
    SClass_ID       SuperClassID()          {return BMM_IO_CLASS_ID;}
    Class_ID        ClassID()               {return CIN_ClassID;}
    const TCHAR*    Category()              {return GetResIDCaption(IDS_BITMAP_IO);}
};

static CIN_ClassDesc CIN_CDesc;

ClassDesc*
Get_CIN_Desc() {
    return &CIN_CDesc;
}

struct CIN_ControlDialog {
public:
    int   mRefPts[2];
    float mWhite[2];
    float mRed[2];
    float mGreen[2];
    float mBlue[2];
    U16*                mDensityLUT;
    ISpinnerControl*    mWhitePtSpinner;
    ISpinnerControl*    mBlackPtSpinner;
    //ISpinnerControl*    mWhiteXPtSpinner;
    //ISpinnerControl*    mWhiteYPtSpinner;
    //ISpinnerControl*    mRedXPtSpinner;
    //ISpinnerControl*    mRedYPtSpinner;
    //ISpinnerControl*    mGreenXPtSpinner;
    //ISpinnerControl*    mGreenYPtSpinner;
    //ISpinnerControl*    mBlueXPtSpinner;
    //ISpinnerControl*    mBlueYPtSpinner;
    CIN_ControlDialog();
   ~CIN_ControlDialog();
    BOOL    SetDensities();
    void    DrawBox(HWND);
    BOOL    InitSpinners(HWND);
    void    SpinnersChanged(WORD);
    void    RemoveSpinners();
};

#endif
