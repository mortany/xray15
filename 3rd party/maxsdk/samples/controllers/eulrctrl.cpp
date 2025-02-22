/**********************************************************************
 *<
   FILE: eulrctrl.cpp

   DESCRIPTION: An Euler angle rotation controller

   CREATED BY: Rolf Berteig

   HISTORY: created 13 June 1995

 *>   Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#include "ctrl.h"
#include "eulrctrl.h"
#include "interpik.h"
#include "euler.h"
#include "notify.h"

#include <float.h>

#include "3dsmaxport.h"

#define EULER_CONTROL_CNAME      GetString(IDS_RB_EULERXYZ)

#define EULER_X_REF     0
#define EULER_Y_REF     1
#define EULER_Z_REF     2

#define THRESHHOLD      1.0f

static DWORD subColor[] = {PAINTCURVE_XCOLOR, PAINTCURVE_YCOLOR, PAINTCURVE_ZCOLOR};

EulerDlg *EulerRotation::dlg = NULL;
IObjParam *EulerRotation::ip = NULL;
ULONG EulerRotation::beginFlags = 0;
EulerRotation *EulerRotation::editControl = NULL;

class JointParamsEuler : public JointParams {
   public:           
      // RB 10/27/2000: Added to support HI IK
      float preferredAngle[3];
      BOOL pfInit;

      JointParamsEuler() : JointParams((DWORD)JNT_ROT,3) 
         {flags |= JNT_LIMITEXACT; preferredAngle[0]=preferredAngle[1]=preferredAngle[2]=0.0f; pfInit=FALSE; flags |= JNT_PARAMS_EULER;}
      JointParamsEuler(const JointParams&);
      void SpinnerChange(InterpCtrlUI *ui,WORD id,ISpinnerControl *spin,BOOL interactive);
      IOResult Save(ISave *isave);
      IOResult Load(ILoad *iload);
   };

static INT_PTR CALLBACK EulerParamDialogProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);

static const int editButs[] = {IDC_EULER_X,IDC_EULER_Y,IDC_EULER_Z};

static int eulerIDs[] = {
   IDS_RB_EULERTYPE0,IDS_RB_EULERTYPE1,IDS_RB_EULERTYPE2,
   IDS_RB_EULERTYPE3,IDS_RB_EULERTYPE4,IDS_RB_EULERTYPE5,
   IDS_RB_EULERTYPE6,IDS_RB_EULERTYPE7,IDS_RB_EULERTYPE8};

typedef int EAOrdering[3];
static EAOrdering orderings[] = {
   // With the Euler order type o,
   // orerdings[o][i] gives the rotation axis of the i-th angle.
   {0,1,2},
   {0,2,1},
   {1,2,0},
   {1,0,2},
   {2,0,1},
   {2,1,0},
   {0,1,0},
   {1,2,1},
   {2,0,2},
   };
const int kRepEulStart = 6;
const EAOrdering inverseOrderings[] = {
   // With the Euler order type o,
   // inverseOrderings[o][0] gives the position of the X-axis;
   // inverseOrderings[o][1] gives the position of the Y-axis;
   // inverseOrderings[o][2] gives the position of the Z-axis.
   // When o = 2(YZX), for example, the X-axis is the 2nd angle,
   // The Y-axis is the 0th angle, and the Z-axis is the 1st angle.
   // Since, for the repetitive types, the same axis appears twice in the
   // Euler angles, this mapping is undefined for them.
   {0,1,2},
   {0,2,1},
   {2,0,1},
   {1,0,2},
   {1,2,0},
   {2,1,0},
   };

inline int subToRef(int o, int a)
{
  return o < kRepEulStart ? orderings[o][a] : a;
}

inline Control* EulerRotation::SubControl(int i)
{
  switch (subToRef(order, i)) {
  case EULER_X_REF: return rotX;
  case EULER_Y_REF: return rotY;
  case EULER_Z_REF: return rotZ;
  default: return NULL;
  }
}

inline Point3 refToSub(int o, const Point3& r)
{
  return o < kRepEulStart
   ? Point3(r[orderings[o][0]], r[orderings[o][1]], r[orderings[o][2]])
   : r;
}

static int xyzIDs[] = {IDS_RB_X,IDS_RB_Y,IDS_RB_Z};
static int xyzRotIDs[] = {IDS_RB_XROTATION,IDS_RB_YROTATION,IDS_RB_ZROTATION};
static int xyzAxisIDs[] = {IDS_RB_XAXIS,IDS_RB_YAXIS,IDS_RB_ZAXIS};

int EulerDlg::cur = EDIT_X;

EulerDlg::EulerDlg(EulerRotation *cont,IObjParam *ip)
   {
   this->ip   = ip;
   this->cont = cont;
   for (int i=0; i<3; i++) {
      iEdit[i] = NULL;
      }
   
   hWnd = ip->AddRollupPage( 
      hInstance,
      MAKEINTRESOURCE(IDD_EULER_PARAMS),
      EulerParamDialogProc,
      GetString(IDS_RB_EULERPARAMS), 
      (LPARAM)this);
   ip->RegisterDlgWnd(hWnd);  
   
   SetCur(cur,EULER_BEGIN);   
   UpdateWindow(hWnd);
   }

EulerDlg::~EulerDlg()
   {
   SetCur(cur,EULER_END);
   for (int i=0; i<3; i++) {
      ReleaseICustButton(iEdit[i]);    
      }
   ip->UnRegisterDlgWnd(hWnd);
   ip->DeleteRollupPage(hWnd);
   hWnd = NULL;
   }

void EulerDlg::EndingEdit(EulerRotation *next)
   {
   Control *nextSub = NULL;
	if(next)
		nextSub = next->SubControl(cur);
   cont->SubControl(cur)
     ->EndEditParams(ip, 0, nextSub);
   cont = NULL;
   ip   = NULL;
   }

void EulerDlg::BeginingEdit(EulerRotation *cont,IObjParam *ip,EulerRotation *prev)
   {
   this->ip   = ip;
   this->cont = cont;
   Control *prevSub = NULL;
   if(prev)
	   prevSub = prev->SubControl(cur);
   cont->SubControl(cur)
     ->BeginEditParams(ip, BEGIN_EDIT_MOTION, prevSub);
   UpdateWindow(hWnd);
   }

void EulerDlg::SetButtonText()
   {
   for (int i=0; i<3; i++) {
      iEdit[i]->SetText(GetString(
         xyzIDs[orderings[cont->order][i]]));
      }
   }

void EulerDlg::Init()
   {  
   for (int i=0; i<3; i++) {
      iEdit[i] = GetICustButton(GetDlgItem(hWnd,editButs[i]));    
      iEdit[i]->SetType(CBT_CHECK);    
      }
   iEdit[cur]->SetCheck(TRUE);   
   SetButtonText();

   SendDlgItemMessage(hWnd,IDC_EULER_ORDER,CB_RESETCONTENT,0,0);
   for (int i=0; i<9; i++) {
      SendDlgItemMessage(hWnd,IDC_EULER_ORDER,CB_ADDSTRING,0,
         (LPARAM)GetString(eulerIDs[i]));
      }
   SendDlgItemMessage(hWnd,IDC_EULER_ORDER,CB_SETCURSEL,cont->order,0);
   }

void EulerDlg::AdjustCur(int prev_ref)
{
  if (cont->order < kRepEulStart) {
   // Keep the sub-controller.
   //
   int sub = inverseOrderings[cont->order][prev_ref];
   if (sub != cur) {
     iEdit[cur]->SetCheck(FALSE);
     cur = sub;
     iEdit[cur]->SetCheck(TRUE);
   }
  } else {
   // Keep the Euler angle position, i.e., cur.
   //
   Control *prev = (Control*)cont->GetReference(prev_ref);
   Control *next = cont->SubControl(cur);
   if (prev != next) {
     prev->EndEditParams(ip, END_EDIT_REMOVEUI, next);
     next->BeginEditParams(ip, BEGIN_EDIT_MOTION, prev);
   }
  }
}

void EulerDlg::SetCur(int c,int code)
   {
   if (c==cur && code==EULER_MIDDLE) return;
   // Pre-condition:
   DbgAssert(0 <= c && c < 3);
   DbgAssert(0 <= cur && cur < 3);

   Control *prev = NULL, *next = NULL;
   if (code!=EULER_END) {
      next = cont->SubControl(c);
      }

   if (code!=EULER_BEGIN) {
      prev = cont->SubControl(cur);
      prev->EndEditParams(ip, END_EDIT_REMOVEUI, next);
      }

   cur = c;

   if (code!=EULER_END) {
      next->BeginEditParams(ip, BEGIN_EDIT_MOTION, prev);
      }
   }

void EulerDlg::WMCommand(int id, int notify, HWND hCtrl)
   {
   switch (id) {
      case IDC_EULER_X:
         SetCur(0);
         iEdit[0]->SetCheck(TRUE);
         iEdit[1]->SetCheck(FALSE);
         iEdit[2]->SetCheck(FALSE);
         break;
      case IDC_EULER_Y:
         SetCur(1);
         iEdit[0]->SetCheck(FALSE);
         iEdit[1]->SetCheck(TRUE);
         iEdit[2]->SetCheck(FALSE);
         break;
      case IDC_EULER_Z:
         SetCur(2);
         iEdit[0]->SetCheck(FALSE);
         iEdit[1]->SetCheck(FALSE);
         iEdit[2]->SetCheck(TRUE);
         break;

      case IDC_EULER_ORDER:
         if (notify==CBN_SELCHANGE) {
            int res = SendDlgItemMessage(hWnd,IDC_EULER_ORDER,CB_GETCURSEL,0,0);
            if (res!=CB_ERR) {
               int prev_o = cont->order;
               if (prev_o != res) {
                 int prev_ref = subToRef(prev_o, cur);
                 cont->ChangeOrdering(res);
                 AdjustCur(prev_ref);
                  ip->RedrawViews(ip->GetTime());
                 }
               SetButtonText();
               }
            }
         break;         
      }
   }

static INT_PTR CALLBACK EulerParamDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
   {
   EulerDlg *dlg = DLGetWindowLongPtr<EulerDlg*>(hDlg);

   switch (message) {
      case WM_INITDIALOG:
         dlg = (EulerDlg*)lParam;         
         DLSetWindowLongPtr(hDlg, lParam);
         dlg->hWnd = hDlg;
         dlg->Init();
         break;
      
      case WM_COMMAND:
         dlg->WMCommand(LOWORD(wParam),HIWORD(wParam),(HWND)lParam);
         break;

      case WM_LBUTTONDOWN:case WM_LBUTTONUP: case WM_MOUSEMOVE:
         dlg->ip->RollupMouseMessage(hDlg,message,wParam,lParam);
         break;
            
      default:
         return FALSE;
      }
   return TRUE;
   }


//********************************************************
// EULER CONTROL
//********************************************************
static Class_ID eulerControlClassID(EULER_CONTROL_CLASS_ID,0); 
class EulerClassDesc:public ClassDesc {
   public:
   int         IsPublic() { return 1; }
   void *         Create(BOOL loading) { return new EulerRotation(loading); }
   const TCHAR *  ClassName() { return EULER_CONTROL_CNAME; }
   SClass_ID      SuperClassID() { return CTRL_ROTATION_CLASS_ID; }
   Class_ID    ClassID() { return eulerControlClassID; }
   const TCHAR*   Category() { return _T("");  }
   };
static EulerClassDesc eulerCD;
ClassDesc* GetEulerCtrlDesc() {return &eulerCD;}

EulerRotation::EulerRotation(const EulerRotation &ctrl)
   {
   HoldSuspend	hs;
   blockUpdate = FALSE;
   order = EULERTYPE_XYZ;
   rotX = NULL;
   rotY = NULL;
   rotZ = NULL;
   mLocked = ctrl.mLocked;
   if (ctrl.rotX) {
      ReplaceReference(EULER_X_REF,ctrl.rotX);
   } else {
      ReplaceReference(EULER_X_REF,NewDefaultFloatController());
      }
   if (ctrl.rotY) {
      ReplaceReference(EULER_Y_REF,ctrl.rotY);
   } else {
      ReplaceReference(EULER_Y_REF,NewDefaultFloatController());
      }
   if (ctrl.rotZ) {
      ReplaceReference(EULER_Z_REF,ctrl.rotZ);
   } else {
      ReplaceReference(EULER_Z_REF,NewDefaultFloatController());
      }
   curval = ctrl.curval;
   ivalid = ctrl.ivalid;
   }

EulerRotation::EulerRotation(BOOL loading) 
  : reorderOnLoad(0)
   {
   HoldSuspend	hs;
   blockUpdate = FALSE;
   order = EULERTYPE_XYZ;
   rotX = NULL;
   rotY = NULL;
   rotZ = NULL;
   if (!loading) {
      ReplaceReference(EULER_X_REF,NewDefaultFloatController());
      ReplaceReference(EULER_Y_REF,NewDefaultFloatController());
      ReplaceReference(EULER_Z_REF,NewDefaultFloatController());
      ivalid = FOREVER;
      curval = Point3::Origin;
   } else {
      ivalid.SetEmpty();
      }  
   }

BOOL EulerRotation::CreateLockKey(TimeValue t, int which)
{
	if(GetLocked()==false)
	{
		if (rotX) rotX->CreateLockKey(t,which);
		if (rotY) rotY->CreateLockKey(t,which);
		if (rotZ) rotZ->CreateLockKey(t,which);
		return TRUE;
	}
	return FALSE;
}


RefTargetHandle EulerRotation::Clone(RemapDir& remap) 
   {
   EulerRotation *euler = new EulerRotation(TRUE); 
   euler->ReplaceReference(EULER_X_REF, remap.CloneRef(rotX));
   euler->ReplaceReference(EULER_Y_REF, remap.CloneRef(rotY));
   euler->ReplaceReference(EULER_Z_REF, remap.CloneRef(rotZ));
   euler->order = order;
   euler->mLocked = mLocked;
   JointParams *jp = (JointParams*)GetProperty(PROPID_JOINTPARAMS);
    if (jp) {
      JointParamsEuler *jp2 = new JointParamsEuler(*jp);
      euler->SetProperty(PROPID_JOINTPARAMS,jp2);
      }
   BaseClone(this, euler, remap);

   return euler;
   }



EulerRotation::~EulerRotation()
   {
   DeleteAllRefsFromMe();
   }

void EulerRotation::GetClassName(TSTR& s)
   {     
   TSTR format(GetString(IDS_RB_EULERNAME));
   s.printf(format,GetString(eulerIDs[order]));
   }

// This copy method will sample the from controller and smooth out all flips
// Nikolai 1-15-99
void EulerRotation::Copy(Control *from)
{
   if(GetLocked()==false)
   {
   if (from->ClassID()==ClassID()) {
      EulerRotation *ctrl = (EulerRotation*)from;
      ReplaceReference(EULER_X_REF,ctrl->rotX);
      ReplaceReference(EULER_Y_REF,ctrl->rotY);
      ReplaceReference(EULER_Z_REF,ctrl->rotZ);
      curval = ctrl->curval;
      ivalid = ctrl->ivalid;
      order  = ctrl->order;
	  mLocked = ctrl->mLocked;
   } else {    
	  if(from&&GetLockedTrackInterface(from))
		  mLocked = GetLockedTrackInterface(from)->GetLocked();
      Quat qPrev;
      Quat qCurr;
      Interval iv;
      int num;    
      if ((num=from->NumKeys())!=NOT_KEYFRAMEABLE && num>0) {
         SuspendAnimate();
         AnimateOn();
         Interval anim;

         anim.SetStart(from->GetKeyTime(0));

         float eaCurr[3];
         float eaPrev[3];
         float EulerAng[3] = {0,0,0};

         from->GetValue(anim.Start(),&qPrev,iv);

         Matrix3 tm;
         qPrev.MakeMatrix(tm);

         MatrixToEuler(tm,EulerAng, order);
            
         rotX->SetValue(anim.Start(),&EulerAng[0],TRUE, CTRL_ABSOLUTE);
         rotY->SetValue(anim.Start(),&EulerAng[1],TRUE, CTRL_ABSOLUTE);
         rotZ->SetValue(anim.Start(),&EulerAng[2],TRUE, CTRL_ABSOLUTE);
         
         if(num>1)
         {
            float dEuler[3],f;   
            Matrix3 tmPrev, tmCurr;

            anim.SetEnd(from->GetKeyTime(num-1));
            
            // Here we sample over the time range, to detect flips
			// #888168: normally we loop over every single tick (1/160th of a second) of 
			// the 'from' controller range.  To prevent hanging in the case of a super-wide      
			// range, we will skip some ticks if the range is over 50000 frames long.  Note       
			// that this may cause some keys to be missed (leonarni)
			int incr = (anim.End() - anim.Start())/4000000;
			if (incr < 1)
				incr = 1;
            for (TimeValue time = anim.Start()+1; time <= anim.End() ; time += incr)
            {
               from->GetValue(time,&qCurr,iv);
               
               qPrev.MakeMatrix(tmPrev);
               qCurr.MakeMatrix(tmCurr);

               // The Euler/Quat ratio is the relation of the angle difference in Euler space to 
               // the angle difference in Quat space. If this ration is bigger than PI the rotation 
               // between the two time steps contains a flip

               f = GetEulerMatAngleRatio(tmPrev,tmCurr,eaPrev,eaCurr,order);  
                              
               if(  f > PI)
               {
                  // We found a flip here
                  for(int j=0 ; j < 3 ; j++)
                  {           
                     // find the sign flip :
                     if(fabs((eaCurr[j]-eaPrev[j])) < 2*PI-THRESHHOLD )
                        dEuler[j] = eaCurr[j]-eaPrev[j];
                     else
                        // unflip the flip
                        dEuler[j] = (2*PI - (float) (fabs(eaCurr[j]) + fabs(eaPrev[j]))) * (eaPrev[j] > 0 ? 1 : -1);
                     
                     EulerAng[j] += dEuler[j];
                  }
               }
               else
               {
                  // Add up the angle difference
                  for(int j=0 ; j < 3 ; j++)
                  {
                     dEuler[j] = eaCurr[j]-eaPrev[j];
                     EulerAng[j] += dEuler[j];
                  }
               }
               if(from->IsKeyAtTime(time,KEYAT_ROTATION))
               {
                  // Create the keys
                  rotX->SetValue(time,&EulerAng[0],TRUE, CTRL_ABSOLUTE);
                  rotY->SetValue(time,&EulerAng[1],TRUE, CTRL_ABSOLUTE);
                  rotZ->SetValue(time,&EulerAng[2],TRUE, CTRL_ABSOLUTE);
               }
               qPrev = qCurr;
            }
         }
         // RB 2/10/99: A key at frame 0 may have been created
         if (num>0 && from->GetKeyTime(0)!=0) {
            rotX->DeleteKeyAtTime(0);
            rotY->DeleteKeyAtTime(0);
            rotZ->DeleteKeyAtTime(0);
         }
         ResumeAnimate();
      } else {
         from->GetValue(0,&qCurr,ivalid);
         SetValue(0,&qCurr,TRUE,CTRL_ABSOLUTE);
         }
      }

   NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
   }
}

  /*
void EulerRotation::Copy(Control *from)
   {
   if (from->ClassID()==ClassID()) {
      EulerRotation *ctrl = (EulerRotation*)from;
      ReplaceReference(EULER_X_REF,ctrl->rotX);
      ReplaceReference(EULER_Y_REF,ctrl->rotY);
      ReplaceReference(EULER_Z_REF,ctrl->rotZ);
      curval = ctrl->curval;
      ivalid = ctrl->ivalid;
      order  = ctrl->order;
   } else {    
      Quat v;
      Interval iv;
      int num;    
      if (num=from->NumKeys()) {
         SuspendAnimate();
         AnimateOn();
         for (int i=0; i<num; i++) {
            TimeValue t = from->GetKeyTime(i);
            from->GetValue(t,&v,iv);
            SetValue(t,&v,TRUE,CTRL_ABSOLUTE);  
            }
         ResumeAnimate();
      } else {
         from->GetValue(0,&v,ivalid);
         SetValue(0,&v,TRUE,CTRL_ABSOLUTE);
         }
      }
   NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
   }
*/

// added 020819  --prs.
void EulerRotation::SelectKeyByIndex(int i, BOOL sel)
   {
   int nk = NumKeys();           // sets up keyTimes table
   if (i < nk) {
      TimeValue tv = GetKeyTime(i);
      if (rotX) {
         int xkey = rotX->GetKeyIndex(tv);
         if (xkey >= 0)
            rotX->SelectKeyByIndex(xkey, sel);
         }
      if (rotY) {
         int ykey = rotY->GetKeyIndex(tv);
         if (ykey >= 0)
            rotY->SelectKeyByIndex(ykey, sel);
         }
      if (rotZ) {
         int zkey = rotZ->GetKeyIndex(tv);
         if (zkey >= 0)
            rotZ->SelectKeyByIndex(zkey, sel);
         }
      }
   }

// added 020819  --prs.
BOOL EulerRotation::IsKeySelected(int i)
   {
   if (i < keyTimes.Count()) {
      TimeValue tv = GetKeyTime(i);
      if (rotX) {
         int xkey = rotX->GetKeyIndex(tv);
         if (xkey >= 0 && rotX->IsKeySelected(xkey))
            return TRUE;
         }
      if (rotY) {
         int ykey = rotY->GetKeyIndex(tv);
         if (ykey >= 0 && rotY->IsKeySelected(ykey))
            return TRUE;
         }
      if (rotZ) {
         int zkey = rotZ->GetKeyIndex(tv);
         if (zkey >= 0 && rotZ->IsKeySelected(zkey))
            return TRUE;
         }
      }
   return FALSE;
   }

void EulerRotation::Update(TimeValue t)
   {
   if (!ivalid.InInterval(t)) {
      ivalid = FOREVER;
      Point3 ang(0,0,0);
      if (rotX) rotX->GetValue(t,&ang.x,ivalid);
      if (rotY) rotY->GetValue(t,&ang.y,ivalid);
      if (rotZ) rotZ->GetValue(t,&ang.z,ivalid);

//DebugPrint(_T(" rot ang = (%f, %f, %f)\n"), ang.x, ang.y, ang.z);
      curval = refToSub(order, ang);
      }
   }

void EulerRotation::ChangeOrdering(int newOrder)
   {
   order = newOrder;
   ivalid.SetEmpty();
   NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
   NotifyDependents(FOREVER,PART_ALL,REFMSG_SUBANIM_STRUCTURE_CHANGED);
   }

// RB 11/14/2000: There seems to be a bug in EulerToQuat(). We're going to
// use this work-around for now.
static void MyEulerToQuat(float *ang, Quat &q, int order)
   {
   Matrix3 mat(1);
   for (int i=0; i<3; i++) {
      switch (orderings[order][i]) {
         case 0: mat.RotateX(ang[i]); break;
         case 1: mat.RotateY(ang[i]); break;
         case 2: mat.RotateZ(ang[i]); break;
         }
      }
   q = Quat(mat);
   }

static void GetEulerAxis(const float ang[], int order, Point3 eulerAxis[])
{
   Matrix3 mat(1);
   // Find each Euler axis by applying the angles to a matrix in
   // reverse order. Note that the resulting matrix is the same
   // as if we built it up in the regular order because we're
   // pre-multiplying the rotations.

   int i = orderings[order][2];
   eulerAxis[2] = mat.GetRow(i);
   switch (i) {
   case 0: mat.PreRotateX(ang[2]); break;
   case 1: mat.PreRotateY(ang[2]); break;
   case 2: mat.PreRotateZ(ang[2]); break;
   }
   i = orderings[order][1];
   eulerAxis[1] = mat.GetRow(i);
   switch (i) {
   case 0: mat.PreRotateX(ang[1]); break;
   case 1: mat.PreRotateY(ang[1]); break;
   case 2: mat.PreRotateZ(ang[1]); break;
   }
   eulerAxis[0] = mat.GetRow(orderings[order][0]);
}

static int IsGimbalAxis(const Point3& a)
{
   int i = a.MaxComponent();
   if (a[i] != 1.0f) return -1;
   int j = (i + 1) % 3, k = (i + 2) % 3;
   if (a[j] != 0.0f ||
      a[k] != FLT_MIN)
      return -1;
   return i;
}

// RB 11/17/2000
// I reworked CTRL_RELATIVE case to handle multiple revolutions
// and to preserve angle windings.
//
void EulerRotation::SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
{
	if(GetLocked()==false)
	{
	   Quat v;
	   float ang[3];
	   
	   Update(t);
	   // By default, we're going to "SetValue()" each sub controller with the
	   // current rotation for that controller.
	   ang[0] = curval.x;
	   ang[1] = curval.y;
	   ang[2] = curval.z;

	   if (method == CTRL_RELATIVE) {
	   do {
		  AngAxis aa(*((AngAxis*)val));

		  int i = -1;
		  if (GetCOREInterface()->GetRefCoordSys() == COORDS_GIMBAL
			 && (i = IsGimbalAxis(aa.axis)) >= 0) {
			 // Gimbal axis in Gimbal mode:
			 //
			 if (order <= 5)
				// Non repetitive Euler angles:
				ang[inverseOrderings[order][i]] -= aa.angle;
			 else {
				// Repetitive Euler angles:
				if (i == orderings[order][2])
				   ang[2] -= aa.angle;
				else if (i == orderings[order][1])
				   ang[1] -= aa.angle;
				else
				   ang[0] -= aa.angle;
				}
			 break;
			 }

		  Point3 eulerAxis[3];

		  GetEulerAxis(ang, order, eulerAxis);

		  // See if the rotation axis matches one of the euler axis
		  BOOL match = FALSE;
		  for (i=0; i<3; i++) {
			 float res = DotProd(eulerAxis[i], aa.axis);
			 if (fabs(fabs(res)-1.0) < 0.0001f) {
				// Just modify the euler angle directly
				ang[i] += res<0.0f? aa.angle : -aa.angle;
				match = TRUE;
				break;
				}
			 }

		  // If match'ed, an actual Gimbal rotation has been performed
		  // no matter what the current UI reference system is.
		  //
		  if (match) break; // break out CTRL_RELATIVE

		  if (0 <= order && order <= 5) {
			 // Non-repetitive Euler angles, namely, XYZ, YZX, etc.,
			 // as supposed to XYX, YZY, etc.
			 //
			 // ang == curval
			 Quat curquat;
			 MyEulerToQuat(ang, curquat, order);
			 float total_angle = aa.angle;
			 aa.angle = 0.0f;
			 // In order to keep the winding information,
			 // we subdivide the whole range of the angle if
			 // it exceeds increm_cap. increm_cap is now
			 // set to (3/4)pi. It can be any number little less
			 // than pi.
			 float increm_cap = (TWOPI + PI)/4.0f;
			 bool forward = total_angle > 0.0f;
			 if (!forward) increm_cap = -increm_cap;
			 bool proceed = true;
			 do {
				if (forward
				   ? total_angle > increm_cap
				   : total_angle < increm_cap) {
				   aa.angle = increm_cap;
				   total_angle -= increm_cap;
				   }
				else {
				   aa.angle = total_angle;
				   proceed = false;
				   }
				curquat *= Quat(aa);
				ContinuousQuatToEuler(curquat, ang, order);
				} while (proceed);
			 }
		  else {
			 // Repetitive Euler angles, such as XYX, YZY, ZXZ
			 // The rotation axis does not match one of the Euler axis.
			 // We'll apply the rotation in quaternion space.

			 // First, figure out how much each angle is wound up.
			 int windX = int(curval.x/TWOPI);
			 int windY = int(curval.y/TWOPI);
			 int windZ = int(curval.z/TWOPI);

			 // Convert to quats and combine rotations.
			 // Then go back to Euler.
			 Quat curquat;
			 MyEulerToQuat(curval, curquat, order);
			 v = curquat * Quat(aa);
			 QuatToEuler(v, ang, order);

			 // The conversion to Quat space and back will loose the
			 // windings. Re-wind the rotations.
			 ang[0] += TWOPI * windX;
			 ang[1] += TWOPI * windY;
			 ang[2] += TWOPI * windZ;
			 }
		  } while (false);
	   // End of if (method == CTRL_RELATIVE)
	   } else {
		  // Assign the incoming value to our current value
		  v = *((Quat*)val);

		   if (0 <= order && order <= 5) {
			  if (GetCOREInterface()->InProgressiveMode()) {
			   Tab<TimeValue> xkeys;
			   Tab<TimeValue> ykeys;
			   Tab<TimeValue> zkeys;
			   Interval range(TIME_NegInfinity, t-1);
			   if (rotX) rotX->GetKeyTimes(xkeys, range, KEYAT_ROTATION);
			   if (rotY) rotY->GetKeyTimes(ykeys, range, KEYAT_ROTATION);
			   if (rotZ) rotZ->GetKeyTimes(zkeys, range, KEYAT_ROTATION);
			   TimeValue pkt = TIME_NegInfinity;
			   int c = xkeys.Count();
			   if (c > 0) {
				TimeValue t0 = xkeys[c-1];
				if (t0 > pkt) pkt = t0;
				 }
			   c = ykeys.Count();
			   if (c > 0) {
				TimeValue t0 = ykeys[c-1];
				if (t0 > pkt) pkt = t0;
				 }
			   c = zkeys.Count();
			   if (c > 0) {
				TimeValue t0 = zkeys[c-1];
				if (t0 > pkt) pkt = t0;
				 }
			   if (pkt > TIME_NegInfinity) {
				Interval dummy = FOREVER;
				if (rotX) rotX->GetValue(pkt, &ang[0], dummy);
				if (rotY) rotY->GetValue(pkt, &ang[1], dummy);
				if (rotZ) rotZ->GetValue(pkt, &ang[2], dummy);
				 }
			   }
			 ContinuousQuatToEuler(v, ang, order);
			 }
		  else {
			 QuatToEuler(v, ang, order);
			  }
		  }

	   Point3 tmp_curval;
	   if (order < kRepEulStart) {
		 tmp_curval.x = ang[inverseOrderings[order][0]];
		 tmp_curval.y = ang[inverseOrderings[order][1]];
		 tmp_curval.z = ang[inverseOrderings[order][2]];
	   }
	   else {
		 tmp_curval.x = ang[0];
		 tmp_curval.y = ang[0];
		 tmp_curval.z = ang[0];
	   }
	   curval = refToSub(order, tmp_curval);
	   ivalid.SetInstant(t);

	   DbgAssert(!blockUpdate);
	   blockUpdate = TRUE;
	   BOOL doHold = theHold.Holding() && !TestAFlag(A_HELD);
	   if (doHold)
		 theHold.Put(new SetValueRestore(this, true));

	   if (order < kRepEulStart) {
		 if (rotX) rotX->SetValue(t,&ang[inverseOrderings[order][0]]);
		 if (rotY) rotY->SetValue(t,&ang[inverseOrderings[order][1]]);
		 if (rotZ) rotZ->SetValue(t,&ang[inverseOrderings[order][2]]);
		 }
	   else {
		 if (rotX) rotX->SetValue(t,&ang[0]);
		 if (rotY) rotY->SetValue(t,&ang[1]);
		 if (rotZ) rotZ->SetValue(t,&ang[2]);
		 }

	   if (doHold)
		 theHold.Put(new SetValueRestore(this, false));
	   blockUpdate = FALSE;

	   ivalid.SetEmpty();
	   NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
   }
}
void EulerRotation::GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method)
   {
   DbgAssert(!blockUpdate);
   if (!blockUpdate) Update(t);

   valid &= ivalid;
   Matrix3 tm(1);
   Quat q;

   for (int i=0; i<3; i++) {
      switch (orderings[order][i]) {
         case 0: tm.RotateX(curval[i]); break;
         case 1: tm.RotateY(curval[i]); break;
         case 2: tm.RotateZ(curval[i]); break;
         }
      }
   if (method == CTRL_RELATIVE) {
      Matrix3 *mat = (Matrix3*)val;    
      *mat = tm * *mat;

   } else {
      q = Quat(tm);
      *((Quat*)val) = q;
   }
   }

bool EulerRotation::GetLocalTMComponents(
   TimeValue   t,
   TMComponentsArg& cmpts,
   Matrix3Indirect&)
//
// This rotation controller does not need parent matrix
//
{
  // Short circuit
  if (cmpts.rotation == NULL) return true;
  assert(cmpts.rotValidity);

  if (rotX) rotX->GetValue(t, (void*)cmpts.rotation, *cmpts.rotValidity);
  if (rotY) rotY->GetValue(t, (void*)(cmpts.rotation+1), *cmpts.rotValidity);
  if (rotZ) rotZ->GetValue(t, (void*)(cmpts.rotation+2), *cmpts.rotValidity);
  cmpts.rotRep = (TMComponentsArg::RotationRep)order;
  return true;
}

void EulerRotation::CommitValue(TimeValue t)
   {
   if (rotX) rotX->CommitValue(t);
   if (rotY) rotY->CommitValue(t);
   if (rotZ) rotZ->CommitValue(t);
   }

void EulerRotation::RestoreValue(TimeValue t)
   {
   if (rotX) rotX->RestoreValue(t);
   if (rotY) rotY->RestoreValue(t);
   if (rotZ) rotZ->RestoreValue(t);
   }

RefTargetHandle EulerRotation::GetReference(int i)
   {
   switch (i) {
      case EULER_X_REF: return rotX;
      case EULER_Y_REF: return rotY;
      case EULER_Z_REF: return rotZ;
      default: return NULL;
      }
   }

void EulerRotation::SetReference(int i, RefTargetHandle rtarg)
   {
   switch (i) {
      case EULER_X_REF: rotX = (Control*)rtarg; break;
      case EULER_Y_REF: rotY = (Control*)rtarg; break;
      case EULER_Z_REF: rotZ = (Control*)rtarg; break;
      }
   }

int EulerRotation::RemapRefOnLoad(int iref)
{
  return reorderOnLoad ? orderings[order][iref] : iref;
}

Animatable* EulerRotation::SubAnim(int i)
{
   return SubControl(i);
}

int EulerRotation::SubNumToRefNum(int subNum)
{
   return subToRef(order, subNum);
}

DWORD EulerRotation::GetSubAnimCurveColor(int subNum)
{
   return subColor[subToRef(order, subNum)];
}

TSTR EulerRotation::SubAnimName(int i)
   {  
   switch (i) {
      case EULER_X_REF: return GetString(xyzRotIDs[orderings[order][0]]);
      case EULER_Y_REF: return GetString(xyzRotIDs[orderings[order][1]]);
      case EULER_Z_REF: return GetString(xyzRotIDs[orderings[order][2]]);
      default: return _T("");
      }
   }

RefResult EulerRotation::NotifyRefChanged(
      const Interval& iv, 
      RefTargetHandle hTarg, 
      PartID& partID, 
	  RefMessage msg, 
	  BOOL propagate) 
   {
   switch (msg) {
      case REFMSG_CHANGE:
         if (blockUpdate)
            return REF_STOP;
         ivalid.SetEmpty();
         break;
      case REFMSG_TARGET_DELETED:
         if (rotX == hTarg) rotX = NULL;
         if (rotY == hTarg) rotY = NULL;
         if (rotZ == hTarg) rotZ = NULL; 
         break;
      case REFMSG_GET_CONTROL_DIM: {
         ParamDimension **dim = (ParamDimension **)partID;
         assert(dim);
         *dim = stdAngleDim;
         return REF_HALT;
         }
      }
   return REF_SUCCEED;
   }

BOOL EulerRotation::AssignController(Animatable *control,int subAnim)
{
   if(GetLocked()==false)
   {
	   if (0 <= subAnim && subAnim < 3)
	   {
		   Control *cont = SubControl(subAnim);
		   if(cont&&GetLockedTrackInterface(cont)&&GetLockedTrackInterface(cont)->GetLocked()==true)//the control is locked, don't assign over it.
			   return FALSE;
		   ReplaceReference(SubNumToRefNum(subAnim), (RefTargetHandle)control);
	   }
	   NotifyDependents(FOREVER,0,REFMSG_CONTROLREF_CHANGE);  //fix for 894778 MZ
	   NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);  
	   return TRUE;
   }
   return FALSE;
}

void EulerRotation::AddNewKey(TimeValue t,DWORD flags)
{
	if(GetLocked()==false)
	{
	   if (rotX) rotX->AddNewKey(t,flags);
	   if (rotY) rotY->AddNewKey(t,flags);
	   if (rotZ) rotZ->AddNewKey(t,flags);
	}
}

int EulerRotation::NumKeys()
   {
#if 1    // 020819  --prs.
   int xnum = rotX->NumKeys();
   int ynum = rotY->NumKeys();
   int znum = rotZ->NumKeys();
   keyTimes.ZeroCount();
   for (int i = 0, j = 0, k = 0;
       i < xnum || j < ynum || k < znum; ) {
      TimeValue k1 = i < xnum ? rotX->GetKeyTime(i) : TIME_PosInfinity;
      TimeValue k2 = j < ynum ? rotY->GetKeyTime(j) : TIME_PosInfinity;
      TimeValue k3 = k < znum ? rotZ->GetKeyTime(k) : TIME_PosInfinity;
      TimeValue kmin = k1 < k2 ? (k3 < k1 ? k3 : k1) : (k3 < k2 ? k3 : k2);
      keyTimes.Append(1, &kmin, 10);
      if (k1 == kmin) ++i;
      if (k2 == kmin) ++j;
      if (k3 == kmin) ++k;
   }
   return keyTimes.Count();
#else
   int num = 0;
   if (rotX) num += rotX->NumKeys(); 
   if (rotY) num += rotY->NumKeys();
   if (rotZ) num += rotZ->NumKeys();
   return num;
#endif
   }

TimeValue EulerRotation::GetKeyTime(int index)
   {
#if 1 // 020819  --prs.
   return keyTimes[index];
#else
   int onum,num = 0;
   if (rotX) num += rotX->NumKeys(); 
   if (index < num) return rotX->GetKeyTime(index);
   onum = num;
   if (rotY) num += rotY->NumKeys(); 
   if (index < num) return rotY->GetKeyTime(index-onum);
   onum = num;
   if (rotZ) num += rotZ->NumKeys(); 
   if (index < num) return rotZ->GetKeyTime(index-onum);
   return 0;
#endif
   }

void EulerRotation::CopyKeysFromTime(TimeValue src,TimeValue dst,DWORD flags)
{
	if(GetLocked()==false)
	{
	   if (rotX) rotX->CopyKeysFromTime(src,dst,flags);
	   if (rotY) rotY->CopyKeysFromTime(src,dst,flags);
	   if (rotZ) rotZ->CopyKeysFromTime(src,dst,flags);
	}
}

BOOL EulerRotation::IsKeyAtTime(TimeValue t,DWORD flags)
   {
   if (rotX && rotX->IsKeyAtTime(t,flags)) return TRUE;
   if (rotY && rotY->IsKeyAtTime(t,flags)) return TRUE;
   if (rotZ && rotZ->IsKeyAtTime(t,flags)) return TRUE;
   return FALSE;
   }

void EulerRotation::DeleteKeyAtTime(TimeValue t)
{
	if(GetLocked()==false)
	{
	   if (rotX) rotX->DeleteKeyAtTime(t);
	   if (rotY) rotY->DeleteKeyAtTime(t);
	   if (rotZ) rotZ->DeleteKeyAtTime(t);
	}
}

// added 020823  --prs.
void EulerRotation::DeleteKeys(DWORD flags)
{
	if(GetLocked()==false)
	{
	   if (rotX) rotX->DeleteKeys(flags);
	   if (rotY) rotY->DeleteKeys(flags);
	   if (rotZ) rotZ->DeleteKeys(flags);
	}
}

// added 020813  --prs.
void EulerRotation::DeleteKeyByIndex(int index)
{
	if(GetLocked()==false)
	{
	   int nk = NumKeys();  // sets up keyTimes table

	   if (index < nk)
		  DeleteKeyAtTime(keyTimes[index]);
   }
}

BOOL EulerRotation::GetNextKeyTime(TimeValue t,DWORD flags,TimeValue &nt)
   {
   TimeValue at,tnear = 0;
   BOOL tnearInit = FALSE;
   
   if (rotX && rotX->GetNextKeyTime(t,flags,at)) {
      if (!tnearInit) {
         tnear = at;
         tnearInit = TRUE;
      } else 
      if (ABS(at-t) < ABS(tnear-t)) tnear = at;
      }

   if (rotY && rotY->GetNextKeyTime(t,flags,at)) {
      if (!tnearInit) {
         tnear = at;
         tnearInit = TRUE;
      } else 
      if (ABS(at-t) < ABS(tnear-t)) tnear = at;
      }

   if (rotZ && rotZ->GetNextKeyTime(t,flags,at)) {
      if (!tnearInit) {
         tnear = at;
         tnearInit = TRUE;
      } else 
      if (ABS(at-t) < ABS(tnear-t)) tnear = at;
      }
   
   if (tnearInit) {
      nt = tnear;
      return TRUE;
   } else {
      return FALSE;
      }
   }
      

void EulerRotation::BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev )
{
   if(GetLocked()==false)
   {
	   if (flags&BEGIN_EDIT_HIERARCHY) {
		  JointParamsEuler *jp = (JointParamsEuler*)GetProperty(PROPID_JOINTPARAMS);
		  InterpCtrlUI *ui = NULL; 

		  if (!jp) {
			 jp = new JointParamsEuler();
			 SetProperty(PROPID_JOINTPARAMS,jp);
			 }

		  if (prev &&
			 prev->ClassID()==ClassID() && 
			  (ui = (InterpCtrlUI*)prev->GetProperty(PROPID_INTERPUI)) != NULL) {
			 JointParams *prevjp = (JointParams*)prev->GetProperty(PROPID_JOINTPARAMS);
			 prevjp->EndDialog(ui);
			 ui->cont = this;
			 ui->ip   = ip;
			 prev->SetProperty(PROPID_INTERPUI,NULL);
			 JointDlgData *jd = DLGetWindowLongPtr<JointDlgData*>(ui->hParams);
			 jd->jp = jp;
			 jp->InitDialog(ui);
		  } else {
			 ui = new InterpCtrlUI(NULL,ip,this);
			 DWORD f=0;
			 if (jp && !jp->RollupOpen()) f = APPENDROLL_CLOSED;   

			 ui->hParams = ip->AddRollupPage( 
				hInstance, 
				MAKEINTRESOURCE(IDD_STDJOINTPARAMS),
				JointParamDlgProc,
				GetString(IDS_RB_ROTJOINTPARAMS), 
				(LPARAM)new JointDlgData(ui,jp),f); 
			 }
	   
		  SetDlgItemText(ui->hParams,IDC_XAXIS_LABEL,
			 GetString(xyzAxisIDs[orderings[order][0]]));
		  SetDlgItemText(ui->hParams,IDC_YAXIS_LABEL,
			 GetString(xyzAxisIDs[orderings[order][1]]));
		  SetDlgItemText(ui->hParams,IDC_ZAXIS_LABEL,
			 GetString(xyzAxisIDs[orderings[order][2]]));

		  SetProperty(PROPID_INTERPUI,ui);
		  editControl = this;
		  beginFlags = flags;
	   } else 
	   if (flags&BEGIN_EDIT_MOTION) {
		  this->ip = ip;

		  if (dlg) {
			 dlg->BeginingEdit(this,ip,(EulerRotation*)prev);
			 dlg->Init();
		  } else {
			 dlg = new EulerDlg(this,ip);  
			 }
		  }
   }
}
void EulerRotation::EndEditParams( IObjParam *ip, ULONG flags,Animatable *next )
{  
   EulerRotation *cont=NULL;
   if (next && next->ClassID()==ClassID()) {
      cont = (EulerRotation*)next;
	  if(cont->GetLocked()==true)
		  cont = NULL; // it's locked so just delete it and stop
    }

   if (dlg) {
      if (cont) {
         dlg->EndingEdit(cont);
      } else {
         delete dlg;
         dlg = NULL;
         }
   } else {
      if (cont) return;
      
      editControl = NULL;
      beginFlags = 0;

      int index = aprops.FindProperty(PROPID_INTERPUI);
      if (index>=0) {
         InterpCtrlUI *ui = (InterpCtrlUI*)aprops[index];
         if (ui->hParams) {
            ip->UnRegisterDlgWnd(ui->hParams);
            ip->DeleteRollupPage(ui->hParams);        
            }
         index = aprops.FindProperty(PROPID_INTERPUI);
         if (index>=0) {
            delete aprops[index];
            aprops.Delete(index,1);
            }
         }
      }
   }

int EulerRotation::SetProperty(ULONG id, void *data)
   {
   if (id==PROPID_JOINTPARAMS) {    
      if (!data) {
         int index = aprops.FindProperty(id);
         if (index>=0) {
            aprops.Delete(index,1);
            }
      } else {
         JointParamsEuler *jp = (JointParamsEuler*)GetProperty(id);
         if (jp) {
            *jp = *((JointParamsEuler*)data);
            delete (JointParamsEuler*)data;
         } else {
            DbgAssert(((JointParams*)data)->flags | JNT_PARAMS_EULER);
            aprops.Append(1,(AnimProperty**)&data);
            }              
         }
      return 1;
   } else
   if (id==PROPID_INTERPUI) {    
      if (!data) {
         int index = aprops.FindProperty(id);
         if (index>=0) {            
            aprops.Delete(index,1);
            }
      } else {
         InterpCtrlUI *ui = (InterpCtrlUI*)GetProperty(id);
         if (ui) {
            *ui = *((InterpCtrlUI*)data);
         } else {
            aprops.Append(1,(AnimProperty**)&data);
            }              
         }
      return 1;
   } else {
      return Animatable::SetProperty(id,data);
      }
   }

void* EulerRotation::GetProperty(ULONG id)
   {
   if (id==PROPID_INTERPUI || id==PROPID_JOINTPARAMS) {
      int index = aprops.FindProperty(id);
      if (index>=0) {
         return aprops[index];
      } else {
         return NULL;
         }
   } else {
      return Animatable::GetProperty(id);
      }
   }


#define JOINTPARAMEULER_CHUNK 0x1002
#define ORDER_CHUNK           0x1003
#define LOCK_CHUNK			  0x2535  //the lock value
IOResult EulerRotation::Save(ISave *isave)
{  
   ULONG nb;
   JointParamsEuler *jp = (JointParamsEuler*)GetProperty(PROPID_JOINTPARAMS);
   if (jp) {
      isave->BeginChunk(JOINTPARAMEULER_CHUNK);
      jp->Save(isave);
      isave->EndChunk();
      }

   isave->BeginChunk(ORDER_CHUNK);
   // We changed the way that X, Y, Z, sub controllers are interpreted.
   // Before the change, they are the first, second, and third, Euler
   // angles, respectively. rotX is the X-angle for te XYZ order, and
   // Z-angle for the ZXY or ZYX order.
   // After the change, they are the X-, Y-, and Z-angles, respectively,
   // regardless of the Euler order. We add to order by 100 to indicate
   // this change. It will be modulo'ed out at load time.
   //
   int order1 = order > 0 ? order + 100 : order;
   isave->Write(&order1,sizeof(order),&nb);
   isave->EndChunk();

	int on = (mLocked==true) ? 1 :0;
	isave->BeginChunk(LOCK_CHUNK);
	isave->Write(&on,sizeof(on),&nb);	
	isave->EndChunk();	

   return IO_OK;
   }

IOResult EulerRotation::Load(ILoad *iload)
   {
   ULONG nb;
   IOResult res = IO_OK;
   reorderOnLoad = 0;
   while (IO_OK==(res=iload->OpenChunk())) {
      switch (iload->CurChunkID()) {
         case ORDER_CHUNK:
            res=iload->Read(&order,sizeof(order),&nb);
            if (order / 100 > 0) {
              order %= 100;
            } else {
              if (0 < order && order < kRepEulStart) {
               // We only need to reorder for non-repetitive euler
               // angles. And, we don't have to do it for XYZ angle.
               reorderOnLoad = 1;
              }
            }
            break;
			case LOCK_CHUNK:
			{
				int on;
				res=iload->Read(&on,sizeof(on),&nb);
				if(on)
					mLocked = true;
				else
					mLocked = false;
			}
			break;
         case JOINTPARAMEULER_CHUNK: {
            JointParamsEuler *jp = new JointParamsEuler;
            jp->Load(iload);
            jp->flags |= JNT_LIMITEXACT;
            SetProperty(PROPID_JOINTPARAMS,jp);
            break;
            }
         }     
      iload->CloseChunk();
      if (res!=IO_OK)  return res;
      }

   return IO_OK;
   }


#define JP_PREFANGLE_CHUNK    0x0090

JointParamsEuler::JointParamsEuler(const JointParams& jp)
   : JointParams(jp)
{
   flags |= JNT_PARAMS_EULER;
   if (jp.flags & JNT_PARAMS_EULER) {
      const JointParamsEuler& jpe = (const JointParamsEuler&)jp;
      preferredAngle[0] = jpe.preferredAngle[0];
      preferredAngle[1] = jpe.preferredAngle[1];
      preferredAngle[2] = jpe.preferredAngle[2];
      pfInit = jpe.pfInit;
   } else {
      preferredAngle[0] = 0.0f;
      preferredAngle[1] = 0.0f;
      preferredAngle[2] = 0.0f;
      pfInit = FALSE;
   }
}

IOResult JointParamsEuler::Save(ISave *isave)
   {
   ULONG nb;

   if (pfInit) {
      // Only save preferred angle if it has been initialized
      isave->BeginChunk(JP_PREFANGLE_CHUNK);
      isave->Write(preferredAngle,sizeof(float)*3,&nb);
      isave->EndChunk();
      }

   // Default saving
   return JointParams::Save(isave);
   }

IOResult JointParamsEuler::Load(ILoad *iload)
   {
   ULONG nb;
   IOResult res;

   // Read the perferred angle chunk if it's present.
   if (iload->PeekNextChunkID()==JP_PREFANGLE_CHUNK) {
      res = iload->OpenChunk();
      res = iload->Read(preferredAngle, sizeof(float)*3, &nb);
      res = iload->CloseChunk();
      if (res!=IO_OK)  return res;
   } else {
      // Note that the preferred angle fields have not been initialized.
      pfInit = FALSE;
      }

   // Default loading
   res = JointParams::Load(iload);

   return res;
   }

void EulerRotation::SetOrder(int newOrder)
   {
   if (order != newOrder && newOrder >= 0 && newOrder < 9) {
      int prev_ref = (dlg && dlg->cont == this) ? subToRef(order, dlg->cur) : 0;
      ChangeOrdering(newOrder);
      if (dlg && dlg->cont == this) {
         SendDlgItemMessage(dlg->hWnd,IDC_EULER_ORDER,CB_SETCURSEL,(WPARAM)newOrder,0);
         dlg->AdjustCur(prev_ref);
         dlg->SetButtonText();
         }
      }
   }

void EulerRotation::EnumIKParams(IKEnumCallback &callback)
   {
   JointParamsEuler *jp = (JointParamsEuler*)GetProperty(PROPID_JOINTPARAMS);
   for (int i=2; i>=0; i--) {
      if (!jp || jp->Active(i)) {
         callback.proc(this,i);
         }
      }
   }

BOOL EulerRotation::CompDeriv(TimeValue t,Matrix3& ptm,IKDeriv& derivs,DWORD flags)
   {
   JointParamsEuler *jp = (JointParamsEuler*)GetProperty(PROPID_JOINTPARAMS);
   Quat q;
   Interval valid;
   Point3 a(0,0,0);

   if (rotX) rotX->GetValue(t,&a[0],valid);
   if (rotY) rotY->GetValue(t,&a[1],valid);
   if (rotZ) rotZ->GetValue(t,&a[2],valid);

   for (int i=2; i>=0; i--) {
      if (!jp || jp->Active(i)) {
         for (int j=0; j<derivs.NumEndEffectors(); j++) {
            Point3 r = derivs.EndEffectorPos(j) - ptm.GetTrans(); 
      
            Point3 axis = ptm.GetRow(orderings[order][i]);
            if (!(ptm.GetIdentFlags()&SCL_IDENT)) {
               axis = Normalize(axis);
               if (ptm.Parity()) axis = -axis;
               }

            if (flags&POSITION_DERIV) {
               derivs.DP(CrossProd(axis,r),j);
               }
            if (flags&ROTATION_DERIV) {
               derivs.DR(axis,j);
               }
            }
         derivs.NextDOF();       
         }
      switch (orderings[order][i]) {
         case 0: ptm.PreRotateX(a[i]); break;
         case 1: ptm.PreRotateY(a[i]); break;
         case 2: ptm.PreRotateZ(a[i]); break;
         }
      }  

   return TRUE;
   }

#define MAX_IKROT DegToRad(4.0f)
#define SGN(a) (a<0?-1:1)

float EulerRotation::IncIKParam(TimeValue t,int index,float delta)
   {
   JointParamsEuler *jp = (JointParamsEuler*)GetProperty(PROPID_JOINTPARAMS);

   if ((float)fabs(delta)>MAX_IKROT) delta = MAX_IKROT * SGN(delta);
   
   if (jp) {
      float v=0.0f;     
      if (jp->Limited(index) || jp->Spring(index)) {
         Interval valid;
         switch (index) {
            case 0: if (rotX) rotX->GetValue(t,&v,valid); break;
            case 1: if (rotY) rotY->GetValue(t,&v,valid); break;
            case 2: if (rotZ) rotZ->GetValue(t,&v,valid); break;
            }
         }
      delta = jp->ConstrainInc(index,v,delta);
      }
   switch (index) {
      case 0: if (rotX) rotX->SetValue(t,&delta,FALSE,CTRL_RELATIVE); break;
      case 1: if (rotY) rotY->SetValue(t,&delta,FALSE,CTRL_RELATIVE); break;
      case 2: if (rotZ) rotZ->SetValue(t,&delta,FALSE,CTRL_RELATIVE); break;
      }  

   return delta;  
   }

void EulerRotation::ClearIKParam(Interval iv,int index) 
   {
   switch (index) {
      case 0: if (rotX) rotX->DeleteTime(iv,TIME_INCRIGHT|TIME_NOSLIDE); break;
      case 1: if (rotY) rotY->DeleteTime(iv,TIME_INCRIGHT|TIME_NOSLIDE); break;
      case 2: if (rotZ) rotZ->DeleteTime(iv,TIME_INCRIGHT|TIME_NOSLIDE); break;
      }
   }

void EulerRotation::MirrorIKConstraints(int axis,int which)
   {
   JointParamsEuler *jp = (JointParamsEuler*)GetProperty(PROPID_JOINTPARAMS);
   if (jp) jp->MirrorConstraints(axis);
   }

BOOL EulerRotation::CanCopyIKParams(int which)
   {
   return ::CanCopyIKParams(this,which);
   }

IKClipObject *EulerRotation::CopyIKParams(int which)
   {
   return ::CopyIKParams(this,which);
   }

BOOL EulerRotation::CanPasteIKParams(IKClipObject *co,int which)
{
   if(GetLocked()==false)
	   return ::CanPasteIKParams(this,co,which);
   else
	   return false;
}

void EulerRotation::PasteIKParams(IKClipObject *co,int which)
   {
   ::PasteIKParams(this,co,which);
   }

void JointParamsEuler::SpinnerChange(
      InterpCtrlUI *ui,WORD id,ISpinnerControl *spin,BOOL interactive)
   {
   EulerRotation *c = (EulerRotation*)ui->cont;

   Point3 a(0,0,0);
   BOOL set = FALSE;
   Interval valid;

   if (c->rotX) c->rotX->GetValue(ui->ip->GetTime(),&a[0],valid);
   if (c->rotY) c->rotY->GetValue(ui->ip->GetTime(),&a[1],valid);
   if (c->rotZ) c->rotZ->GetValue(ui->ip->GetTime(),&a[2],valid);

   switch (id) {
      case IDC_XFROMSPIN:
         a[0] = min[0] = DegToRad(spin->GetFVal()); 
         set = TRUE;
         break;
      case IDC_XTOSPIN:
         a[0] = max[0] = DegToRad(spin->GetFVal());
         set = TRUE;
         break;
      case IDC_XSPRINGSPIN:
         a[0] = spring[0] = DegToRad(spin->GetFVal());
         set = TRUE;
         break;
      
      case IDC_YFROMSPIN:
         a[1] = min[1] = DegToRad(spin->GetFVal()); 
         set = TRUE;
         break;
      case IDC_YTOSPIN:
         a[1] = max[1] = DegToRad(spin->GetFVal());
         set = TRUE;
         break;
      case IDC_YSPRINGSPIN:
         a[0] = spring[1] = DegToRad(spin->GetFVal());
         set = TRUE;
         break;
      
      case IDC_ZFROMSPIN:
         a[2] = min[2] = DegToRad(spin->GetFVal()); 
         set = TRUE;
         break;
      case IDC_ZTOSPIN:
         a[2] = max[2] = DegToRad(spin->GetFVal());
         set = TRUE;
         break;
      case IDC_ZSPRINGSPIN:
         a[2] = spring[0] = DegToRad(spin->GetFVal());
         set = TRUE;
         break;
      
      case IDC_XDAMPINGSPIN:
         damping[0] = spin->GetFVal(); break;      
      case IDC_YDAMPINGSPIN:
         damping[1] = spin->GetFVal(); break;      
      case IDC_ZDAMPINGSPIN:
         damping[2] = spin->GetFVal(); break;

      case IDC_XSPRINGTENSSPIN:
         stens[0] = spin->GetFVal()/SPRINGTENS_UI; break;
      case IDC_YSPRINGTENSSPIN:
         stens[1] = spin->GetFVal()/SPRINGTENS_UI; break;
      case IDC_ZSPRINGTENSSPIN:
         stens[2] = spin->GetFVal()/SPRINGTENS_UI; break;
      }
   
   if (set && interactive) {           
      if (c->rotX) c->rotX->SetValue(ui->ip->GetTime(),&a[0],TRUE,CTRL_ABSOLUTE);
      if (c->rotY) c->rotY->SetValue(ui->ip->GetTime(),&a[1],TRUE,CTRL_ABSOLUTE);
      if (c->rotZ) c->rotZ->SetValue(ui->ip->GetTime(),&a[2],TRUE,CTRL_ABSOLUTE);
      ui->ip->RedrawViews(ui->ip->GetTime(),REDRAW_INTERACTIVE);
      }
   }

BOOL EulerRotation::GetIKJoints(InitJointData *posData,InitJointData *rotData)
{
   // Copied from core\interpik.cpp, GetIKJointsRot().
   //
   BOOL del = FALSE;
   JointParams *jp = (JointParams*)GetProperty(PROPID_JOINTPARAMS);
   if (!jp) {
      jp = new JointParams((DWORD)JNT_ROT,3);
      del = TRUE;
      }

   for (int i=0; i<3; i++) {
      rotData->min[i]     = jp->min[i];
      rotData->max[i]     = jp->max[i];
      rotData->damping[i] = jp->damping[i];     
      rotData->active[i]  =  jp->flags & (JNT_XACTIVE<<i);
      rotData->limit[i]   =  jp->flags & (JNT_XLIMITED<<i);
      rotData->ease[i]    =  jp->flags & (JNT_XEASE<<i);
      }

   if (del) delete jp;
   return TRUE;
}

void EulerRotation::InitIKJoints2(InitJointData2 *posData,InitJointData2 *rotData)
   {
   JointParamsEuler *jp   = (JointParamsEuler*)GetProperty(PROPID_JOINTPARAMS);
   InitJointData3* data3 = DowncastToJointData3(rotData);
   /*
   // Must be 0
   if (rotData->flags) return;
   */
   if (rotData->flags && data3 == NULL) return;

   if (!jp) {
      jp = new JointParamsEuler();
      SetProperty(PROPID_JOINTPARAMS,jp);
      }

   jp->flags &= ~(
      JNT_XACTIVE|JNT_YACTIVE|JNT_ZACTIVE|
      JNT_XLIMITED|JNT_YLIMITED|JNT_ZLIMITED|
      JNT_XEASE|JNT_YEASE|JNT_ZEASE);

   for (int i=0; i<3; i++) {
      jp->min[i]            = rotData->min[i];
      jp->max[i]            = rotData->max[i];
      jp->damping[i]        = rotData->damping[i];
      jp->preferredAngle[i] = rotData->preferredAngle[i];

      if (rotData->active[i]) jp->flags |= JNT_XACTIVE<<i;
      if (rotData->limit[i])  jp->flags |= JNT_XLIMITED<<i;
      if (rotData->ease[i])   jp->flags |= JNT_XEASE<<i;

      jp->pfInit = TRUE;
      }

   if (data3 != NULL) {
     jp->SetSpring(0, data3->springOn[0] ? TRUE : FALSE);
     jp->SetSpring(1, data3->springOn[1] ? TRUE : FALSE);
     jp->SetSpring(2, data3->springOn[2] ? TRUE : FALSE);
     jp->spring[0] = data3->spring[0];
     jp->spring[1] = data3->spring[1];
     jp->spring[2] = data3->spring[2];
     jp->stens[0] = data3->springTension[0];
     jp->stens[1] = data3->springTension[1];
     jp->stens[2] = data3->springTension[2];
     }
   }

BOOL EulerRotation::GetIKJoints2(InitJointData2 *posData,InitJointData2 *rotData)
   {
   BOOL del = FALSE;
   JointParamsEuler *jp   = (JointParamsEuler*)GetProperty(PROPID_JOINTPARAMS);

   if (!jp) {
      jp = new JointParamsEuler();
      del = TRUE;
      }

   // Init preferred angle to current controller value
   Quat qt(0.0f,0.0f,0.0f,1.0f);
   Point3 rot;
   if (!jp->pfInit) {
      GetValue(GetCOREInterface()->GetTime(), &qt, FOREVER, CTRL_ABSOLUTE);
      QuatToEuler(qt, rot, EULERTYPE_XYZ);      
      }

   for (int i=0; i<3; i++) {
      rotData->min[i]     = jp->min[i];
      rotData->max[i]     = jp->max[i];
      rotData->damping[i] = jp->damping[i];     
      rotData->active[i]  =  jp->flags & (JNT_XACTIVE<<i);
      rotData->limit[i]   =  jp->flags & (JNT_XLIMITED<<i);
      rotData->ease[i]    =  jp->flags & (JNT_XEASE<<i);

      if (jp->pfInit) {
         rotData->preferredAngle[i] = jp->preferredAngle[i];
      } else {
         rotData->preferredAngle[i] = rot[i];
         }
      }

   InitJointData3* data3 = DowncastToJointData3(rotData);
   if (data3 != NULL) {
     data3->springOn[0] = jp->Spring(0) ? true : false;
     data3->springOn[1] = jp->Spring(1) ? true : false;
     data3->springOn[2] = jp->Spring(2) ? true : false;
     data3->spring.Set(jp->spring[0], jp->spring[1], jp->spring[2]);
     data3->springTension.Set(jp->stens[0], jp->stens[1], jp->stens[2]);
   }

   if (del) delete jp;

   return TRUE;
   }

int EulerRotation::GetORT(int type)
   {
   if (rotX) return rotX->GetORT(type);  // AF -- you got to pick something...
   return 0;
   }

void EulerRotation::SetORT(int ort,int type)
   {
   if (rotX) rotX->SetORT(ort, type);
   if (rotY) rotY->SetORT(ort, type);
   if (rotZ) rotZ->SetORT(ort, type);
   }

void EulerRotation::EnableORTs(BOOL enable)
   {
   if (rotX) rotX->EnableORTs(enable);
   if (rotY) rotY->EnableORTs(enable);
   if (rotZ) rotZ->EnableORTs(enable);
   }

///A TrackBar filter for hiding the keys reported by the Euler controller.
///The subTree nature of the TrackBar already includes the keys reported by the 
///Euler controller since it gets them directly from the subAnims.
bool NoEulerFilter(Animatable* anim, Animatable* parent, int subIndex, Animatable* grandParent, INode* node)
{
   if(anim != NULL && anim->ClassID() == Class_ID(EULER_CONTROL_CLASS_ID,0))
      return false;
   return true;
}

///Register a TrackBar filter at startup.
void EulerNotifyStartup(void *param, NotifyInfo *info)
{
   ITrackBar* tb = GetCOREInterface()->GetTrackBar();
   if (tb != NULL)
   {
      ITrackBarFilterManager* filterManager = (ITrackBarFilterManager*)tb->GetInterface(TRACKBAR_FILTER_MANAGER_INTERFACE);
      if (filterManager != NULL)
      {
         filterManager->RegisterFilter(NoEulerFilter, NULL,  _T(""), Class_ID(0x333e373f, 0x2f4c4168), true, false);
      }
   }
}

///Registers a startup callback to add a TrackBar filter
class EulerTrackBarFilterRegister
{
public:
   EulerTrackBarFilterRegister()
   {
      RegisterNotification(EulerNotifyStartup, NULL, NOTIFY_SYSTEM_STARTUP);
   }
};
EulerTrackBarFilterRegister theEulerTrackBarFilterRegister;

