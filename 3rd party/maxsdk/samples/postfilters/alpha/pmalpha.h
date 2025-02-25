//-----------------------------------------------------------------------------
// ------------------
// File ....: PMAlpha.h
// ------------------
// Author...: Gus J Grubba
// Date ....: September 1995
// Descr....: PMAlpha Compositor
//
// History .: Sep, 27 1995 - Started
//            Apr, 09 1997 - Added G Channel Support (GG)
//            
//-----------------------------------------------------------------------------
        
#ifndef _PMALPHACLASS_
#define _PMALPHACLASS_

#define DLLEXPORT __declspec(dllexport)

//-----------------------------------------------------------------------------
//-- Class Definition ---------------------------------------------------------
//

class ImageFilter_PMAlpha : public ImageFilter {
    
	public:
     
		//-- Constructors/Destructors
        
                       ImageFilter_PMAlpha( ) {}
                      ~ImageFilter_PMAlpha( ) {}
               
       //-- Filter Info  ---------------------------------

       const TCHAR   *Description         ( ) ;
       const TCHAR   *AuthorName          ( ) { return _T("Gus J Grubba");}
       const TCHAR   *CopyrightMessage    ( ) { return _T("Copyright 1995, Yost Group");}
       UINT           Version             ( ) { return (200);}

       //-- Filter Capabilities --------------------------
        
       DWORD          Capability          ( ) { return(IMGFLT_COMPOSITOR | IMGFLT_MASK); }

       //-- Show DLL's About box -------------------------
        
       void           ShowAbout           ( HWND hWnd );  
       BOOL           ShowControl         ( HWND hWnd ) { return (FALSE); }  

       //-- Show Time ------------------------------------
        
       BOOL           Render              ( HWND hWnd );


};

#endif

//-- EOF: PMAlpha.h ----------------------------------------------------------
