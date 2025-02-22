/*===========================================================================*\
 | 
 |  FILE:   wM3_impexp.cpp
 |       Weighted Morpher for MAX R3
 |       Import and Export code
 | 
 |  AUTH:   Harry Denholm
 |       Copyright(c) Kinetix 1999
 |       All Rights Reserved.
 |
 |  HIST:   Started 25-11-98
 | 
\*===========================================================================*/

#include "wM3.h"

#include "3dsmaxport.h"

// WORK IN PROGRESS
// I ran out of time :)
//
// Something for the future... unless you want to impliment your own impexp
//
// Did a load of work on the xport interface, letting the user choose specific 
// channels. UIs are built, just no code to export as yet.
//


/*===========================================================================*\
 |
 | Import and Export mappings dialog handlers
 |
\*===========================================================================*/



INT_PTR CALLBACK IMPORT_DlgProc(
      HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   MorphR3 *mp = DLGetWindowLongPtr<MorphR3*>(hWnd);
   if (!mp && msg!=WM_INITDIALOG) return FALSE;

   switch (msg) {
      case WM_INITDIALOG:{
         mp = (MorphR3*)lParam;
         DLSetWindowLongPtr(hWnd, mp);

         HWND maxWnd = mp->ip->GetMAXHWnd();
         CenterWindow(hWnd,maxWnd);
         EnableWindow(maxWnd,FALSE);
         break;
         }

      case WM_CLOSE:
         break;

      case WM_DESTROY:
         break;

      case WM_COMMAND:
         switch (LOWORD(wParam)) {
            case IDC_CANCEL:
               EnableWindow(mp->ip->GetMAXHWnd(),TRUE);
               EndDialog(hWnd,1);
               break;
         }

         break;

      default:
         return FALSE;
   }
   return TRUE;
}






void ValidateExportDlg( HWND hWnd, HWND clist )
{
   int sCount = (int)SendMessage(clist,LB_GETSELCOUNT,0,0);
   TCHAR s[255];
   _stprintf(s,_T("%i %s"),sCount,GetString(IDS_CHANSEL));
   SetWindowText(GetDlgItem(hWnd,IDC_SEL),s);

   if(sCount<1) EnableWindow(GetDlgItem(hWnd,IDC_DOSAVE),FALSE);
   else EnableWindow(GetDlgItem(hWnd,IDC_DOSAVE),TRUE);
}


INT_PTR CALLBACK EXPORT_DlgProc(
      HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   MorphR3 *mp = DLGetWindowLongPtr<MorphR3*>(hWnd);
   if (!mp && msg!=WM_INITDIALOG) return FALSE;

   switch (msg) {
      case WM_INITDIALOG:{
         mp = (MorphR3*)lParam;
         DLSetWindowLongPtr(hWnd, mp);

         HWND maxWnd = mp->ip->GetMAXHWnd();
         CenterWindow(hWnd,maxWnd);
         EnableWindow(maxWnd,FALSE);

         HWND clist = GetDlgItem(hWnd,IDC_CLIST);
         SendMessage(clist,LB_RESETCONTENT,0,0);

         for(int i=0;i<mp->chanBank.size();i++)
         {
            TCHAR tmp[255];
            int Cind = i+1;

            _stprintf(tmp, _T("%i : %s"), Cind, mp->chanBank[i].mName.data());
            SendMessage(clist,LB_ADDSTRING,0,(LPARAM)tmp);
         }
         break;
         }

      case WM_CLOSE:
      case WM_DESTROY:
         break;

      case WM_COMMAND:
         if ( HIWORD(wParam) == LBN_SELCHANGE ) {
            HWND clist = GetDlgItem(hWnd,IDC_CLIST);
            ValidateExportDlg(hWnd,clist);
         }

         switch (LOWORD(wParam)) {
            case IDC_CANCEL:
               EnableWindow(mp->ip->GetMAXHWnd(),TRUE);
               EndDialog(hWnd,1);
               break;

            case IDC_SELUSED:{
               HWND clist = GetDlgItem(hWnd,IDC_CLIST);
               SendMessage(clist,LB_SELITEMRANGE,FALSE,MAKELPARAM(0,mp->chanBank.size()));

               int caret = (int)SendMessage(clist,LB_GETCARETINDEX,0,0);

               for(int i=0;i<mp->chanBank.size();i++)
               {
                  if(mp->chanBank[i].mModded) SendMessage(clist,LB_SETSEL,TRUE,(LPARAM)i);
               }
               SendMessage(clist,LB_SETCARETINDEX,caret,MAKELPARAM(TRUE,0));

               ValidateExportDlg(hWnd,clist);

               break;}

            case IDC_SELINVERT:{
               HWND clist = GetDlgItem(hWnd,IDC_CLIST);
               int caret = (int)SendMessage(clist,LB_GETCARETINDEX,0,0);

               for(int i=0;i<mp->chanBank.size();i++)
               {
                  SendMessage(clist,LB_SETSEL,!(SendMessage(clist,LB_GETSEL,i,0)),(LPARAM)i);
               }
               SendMessage(clist,LB_SETCARETINDEX,caret,MAKELPARAM(TRUE,0));

               ValidateExportDlg(hWnd,clist);

               break;}

            case IDC_SELNONE:{
               HWND clist = GetDlgItem(hWnd,IDC_CLIST);

               SendMessage(clist,LB_SELITEMRANGE,FALSE,MAKELPARAM(0,mp->chanBank.size()));
               ValidateExportDlg(hWnd,clist);
               break;}

            case IDC_SELALL:{
               HWND clist = GetDlgItem(hWnd,IDC_CLIST);

               SendMessage(clist,LB_SELITEMRANGE,TRUE,MAKELPARAM(0,mp->chanBank.size()));
               ValidateExportDlg(hWnd,clist);
               break;}
         }

         break;

      default:
         return FALSE;
   }
   return TRUE;
}
