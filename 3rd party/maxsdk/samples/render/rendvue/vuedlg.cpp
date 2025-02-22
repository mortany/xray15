/**********************************************************************
 *<
   FILE: vuedlg.cpp

   DESCRIPTION: .VUE file selector

   CREATED BY: Dan Silva

   HISTORY:

 *>   Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#include "rvuepch.h"
#include "resource.h"
#include "rendvue.h"

#include "3dsmaxport.h"

class RendVueParamDlg : public RendParamDlg {
public:
   VueRenderer *rend;
   IRendParams *ir;
   HWND hPanel;
   BOOL prog;
   TSTR workFileName;
   RendVueParamDlg(VueRenderer *r,IRendParams *i,BOOL prog);
   ~RendVueParamDlg();
   void AcceptParams();
   void DeleteThis() {delete this;}
   void InitParamDialog(HWND hWnd);
   void InitProgDialog(HWND hWnd);
   void ReleaseControls() {}
   BOOL FileBrowse();
   INT_PTR WndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);
   };

RendVueParamDlg::~RendVueParamDlg()
   {
   ir->DeleteRollupPage(hPanel);
   }

INT_PTR RendVueParamDlg::WndProc(
      HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
   {
   switch (msg) {
      case WM_INITDIALOG:
         if (prog) InitProgDialog(hWnd);
         else InitParamDialog(hWnd);
         break;
      
      case WM_DESTROY:
         if (!prog) ReleaseControls();
         break;
      default:
         return FALSE;
      }
   return TRUE;
   }

static INT_PTR CALLBACK RendVueParamDlgProc(
      HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
   {
   RendVueParamDlg *dlg = DLGetWindowLongPtr<RendVueParamDlg*>(hWnd);
   switch (msg) {
      case WM_INITDIALOG:
         dlg = (RendVueParamDlg*)lParam;
         DLSetWindowLongPtr(hWnd, lParam);
         break;
      case WM_LBUTTONDOWN:
      case WM_MOUSEMOVE:
      case WM_LBUTTONUP:
         dlg->ir->RollupMouseMessage(hWnd,msg,wParam,lParam);
         break;
      case WM_COMMAND:
         case IDC_RENDVUE_FILE:
            if (dlg->FileBrowse()) {
               SetDlgItemText(hWnd,IDC_RENDVUE_FILENAME, dlg->workFileName.data());
               }
            break;
      case CC_SPINNER_CHANGE:   
         {
         }
         break;
      }  
   if (dlg) return dlg->WndProc(hWnd,msg,wParam,lParam);
   else return FALSE;
   }

RendVueParamDlg::RendVueParamDlg(
      VueRenderer *r,IRendParams *i,BOOL prog)
   {
   rend       = r;
   ir         = i;
   this->prog = prog;
   if (prog) {    
      hPanel = ir->AddRollupPage(
         hInstance, 
         MAKEINTRESOURCE(IDD_RENDVUE_PROG),
         RendVueParamDlgProc,
         GetString(IDS_VRENDTITLE),
         (LPARAM)this);
   } else {
      hPanel = ir->AddRollupPage(
         hInstance, 
         MAKEINTRESOURCE(IDD_RENDVUE_PARAMS),
         RendVueParamDlgProc,
         GetString(IDS_VRENDTITLE),
         (LPARAM)this);
      }
   }

void RendVueParamDlg::InitParamDialog(HWND hWnd) {
   workFileName = rend->vueFileName;
   SetDlgItemText(hWnd,IDC_RENDVUE_FILENAME, workFileName);
   }

void RendVueParamDlg::InitProgDialog(HWND hWnd) {
   SetDlgItemText(hWnd,IDC_RENDVUE_FILENAME, rend->vueFileName.data());
   }

void RendVueParamDlg::AcceptParams() {
   rend->vueFileName = workFileName;
   }

RendParamDlg * VueRenderer::CreateParamDialog(IRendParams *ir,BOOL prog) {
   return new RendVueParamDlg(this,ir,prog);
   }



// File Browse ------------------------------------------------------------

#define VUEEXT _T(".vue")
#define VUEFILTER _T("*.vue")

void FixFileExt(OPENFILENAME &ofn, TCHAR* ext = VUEEXT) {
   int l = static_cast<int>(_tcslen(ofn.lpstrFile));  // SR DCAST64: Downcast to 2G limit.
   int e = static_cast<int>(_tcslen(ext));   // SR DCAST64: Downcast to 2G limit.
   if (_tcsicmp(ofn.lpstrFile+l-e, ext)) {
      _tcscat(ofn.lpstrFile,ext);   
      }
   }

#if 0
UINT_PTR WINAPI FileHook( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
   {
   switch (message) {
      case WM_INITDIALOG:
         SetDlgItemText(hDlg, IDOK, _T("OK"));
         break;
      case WM_COMMAND:{
         }
         
         break;
      }
   return FALSE;
   }  

UINT_PTR PMFileHook(HWND hWnd,UINT message,WPARAM wParam,LPARAM   lParam) 
   {
   switch (message) {
      case WM_INITDIALOG:
         SetDlgItemText(hWnd, IDOK, _T("OK"));
         break;
      case WM_COMMAND:{
         }
         
         break;
      }
   return 0;
   }
#endif

BOOL RendVueParamDlg::FileBrowse() {

   TSTR filename;
   TCHAR saveDir[1024];
   {
      TSTR dir;
      SplitFilename(workFileName, &dir, &filename,NULL);
      _tcscpy(saveDir,dir.data());
   }
   TCHAR fname[512];
   _tcscpy(fname,filename.data());
   _tcscat(fname, VUEEXT);

   FilterList filterList;
   filterList.Append(GetString(IDS_VUE_FILE));
   filterList.Append(VUEFILTER);

   OPENFILENAME  ofn;
   memset(&ofn, 0, sizeof(ofn));

   ofn.lStructSize      = sizeof(OPENFILENAME);
   ofn.hwndOwner        = hPanel;
   ofn.hInstance        = hInstance;

   static const int filterIndex = 1;
   ofn.nFilterIndex = filterIndex;
   ofn.lpstrFilter  = filterList;

   ofn.lpstrTitle   = GetString(IDS_WRITE_VUEFILE);
   ofn.lpstrFile    = fname;
   ofn.nMaxFile     = _countof(fname);

   Interface *iface = GetCOREInterface();
   
   ofn.lpstrInitialDir = saveDir[0] ? saveDir : iface->GetDir(APP_SCENE_DIR);
   ofn.Flags = OFN_HIDEREADONLY | OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLESIZING;
   ofn.lpfnHook = (LPOFNHOOKPROC)MaxSDK::MinimalMaxFileOpenHookProc;
   ofn.lCustData = 0;      // 0 for save, 1 for open

   FixFileExt(ofn,VUEEXT); // add ".vue" if absent

   while (GetSaveFileName(&ofn)) {
      FixFileExt(ofn,VUEEXT); // add ".vue" if absent

      workFileName = ofn.lpstrFile;
      return TRUE;
   }
   return FALSE;
}
