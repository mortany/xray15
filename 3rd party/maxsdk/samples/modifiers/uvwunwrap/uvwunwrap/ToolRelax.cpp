/********************************************************************** *<
FILE: Toolrelax.cpp

DESCRIPTION: This contains methods related to unwrap relax functions at the UI level


HISTORY: 12/31/96
CREATED BY: Peter Watje
Copyright (c) 2007, All Rights Reserved.

********************************************************************** */

#include "ToolRelax.h"
#include <helpsys.h>

unsigned __stdcall  CallRelaxThread(void *data)
{
	MaxSDK::Util::UseLanguagePackLocale();

	RelaxThreadData *threadData = (RelaxThreadData *) data;
	//if (threadData->mod->IsLocked()) return;
	if (threadData->mod->LockMeshTopoData())
	{
		_endthreadex(0);
		return 0;
	}
	threadData->mLocalData.SetCount( threadData->mod->GetMeshTopoDataCount());
	for (int i = 0; i < threadData->mLocalData.Count(); i++)
		threadData->mLocalData[i] =  threadData->mod->GetMeshTopoData(i);

	threadData->mStarted = TRUE;

	DisableAccelerators();  

	BOOL rigSelected = FALSE;
	MeshTopoData *peltLD = threadData->mod->peltData.mBaseMeshTopoDataCurrent;
	if (threadData->mod->peltData.peltDialog.hWnd)
	{		
		if (peltLD)
		{
			for (int i = 0; i < threadData->mod->peltData.rigPoints.Count(); i++)
			{
				int index = threadData->mod->peltData.rigPoints[i].lookupIndex;
				if (peltLD->GetTVVertSelected(index))
					rigSelected = TRUE;
			}
		}
	}

	GetCOREInterface()->DisableSceneRedraw();
	//if we are not instanced we can just start up unwrap relax and go
	if ((threadData->mod->GetMeshTopoDataCount() == 1) && !rigSelected)
	{
		if (threadData->mType == 0)
			threadData->mod->fnRelaxByFaceAngle(threadData->mIterations,threadData->mStretch,threadData->mAmount,threadData->mBoundary,NULL);
		else if (threadData->mType == 1)
			threadData->mod->fnRelaxByEdgeAngle(threadData->mIterations,threadData->mStretch,threadData->mAmount,threadData->mBoundary,NULL);
		else if (threadData->mType == 2)
			threadData->mod->RelaxVerts2(threadData->mAmount,threadData->mIterations,threadData->mBoundary,threadData->mCorner,FALSE);
	}
	//if we have multiple instance it gets more complicated
	//since only the first relax instance will ever run
	else
	{
		BOOL done = FALSE;
		threadData->mIterations = 5;
		// 1/13/2016 XY W. MultiThread Problem: if user setUserCancel(TRUE) before the following statements,
		// The thread will never get out of the while loop. Actually, no need to initialize UserCancel to be FALSE.
		//for (int ldID = 0; ldID < threadData->mLocalData.Count(); ldID++)
		//{
		//	if (threadData->mLocalData[ldID])
		//		threadData->mLocalData[ldID]->SetUserCancel(FALSE);
		//}
		while(!done)
		{
			if (threadData->mLocalData.Count() == 0) done = TRUE;
			for (int ldID = 0; ldID < threadData->mLocalData.Count(); ldID++)
			{
				if (threadData->mLocalData[ldID] && threadData->mLocalData[ldID]->GetUserCancel())
					done = TRUE;
			}
			if (threadData->mType == 0)
				threadData->mod->fnRelaxByFaceAngle(threadData->mIterations,threadData->mStretch,threadData->mAmount,threadData->mBoundary,NULL);
			else if (threadData->mType == 1)
				threadData->mod->fnRelaxByEdgeAngle(threadData->mIterations,threadData->mStretch,threadData->mAmount,threadData->mBoundary,NULL);
			else if (threadData->mType == 2)
				threadData->mod->RelaxVerts2(threadData->mAmount,threadData->mIterations,threadData->mBoundary,threadData->mCorner,FALSE);

			if (rigSelected && threadData->mod)
			{
				threadData->mod->peltData.RelaxRig(threadData->mIterations, threadData->mAmount, threadData->mBoundary);
				threadData->mod->InvalidateView();
			}	
		}
	}

	for (int ldID = 0; ldID < threadData->mod->GetMeshTopoDataCount(); ldID++)
	{
		threadData->mod->GetMeshTopoData(ldID)->SetUserCancel(FALSE);
	}

	GetCOREInterface()->EnableSceneRedraw();
	EnableAccelerators();   
	threadData->mStarted = FALSE;

	ICustButton *iButton = GetICustButton(GetDlgItem(threadData->mod->relaxHWND, IDC_RUN_BUTTON));
	if (iButton)
	{
		if (iButton->IsChecked())
		{
			iButton->SetText(GetString(IDS_SPLINEMAP_STARTRELAX));
			iButton->SetCheck(FALSE);
		}
		ReleaseICustButton(iButton);
	}

	threadData->mod->UnlockMeshTopoData();
	_endthreadex(0);

	return 0;
}

void	UnwrapMod::fnRelaxThreaded(int threadOp)
{
	RelaxThreadOp(threadOp, nullptr);
}



void UnwrapMod::RelaxThreadOp(int operation, HWND hWnd)
{
	ISpinnerControl *iAmount = NULL;
	ISpinnerControl *iStretch = NULL;

	mRelaxThreadData.mod = this;
	mRelaxThreadData.mType = 0;
	mRelaxThreadData.mAmount = 0.01f;
	mRelaxThreadData.mBoundary = FALSE;
	mRelaxThreadData.mStretch = 0.0f;
	mRelaxThreadData.mCorner = FALSE;
	mRelaxThreadData.mIterations = 1000000;

	if (hWnd)
	{
		//this is a structure that holds all our info we are passing to the thread
		iAmount = GetISpinner(GetDlgItem(hWnd, IDC_RELAX_AMOUNTSPIN));
		iStretch = GetISpinner(GetDlgItem(hWnd, IDC_RELAX_STRETCHSPIN));

		mRelaxThreadData.mod = this;
		mRelaxThreadData.mType = relaxType;

		//KZ, 04/2017, Added a divide by 1000 to shift down the parameter space
		//for relax by center. Default value of 0.1 was too fast.
		auto parameterSpaceShift = 1.0f;
		if (relaxType == 2) //relax by center
			parameterSpaceShift = 0.001f;

		if (iAmount)
			mRelaxThreadData.mAmount = iAmount->GetFVal();
		else
			mRelaxThreadData.mAmount = 0.1f;
		mRelaxThreadData.mAmount *= parameterSpaceShift;

		mRelaxThreadData.mBoundary = IsDlgButtonChecked(hWnd, IDC_BOUNDARY_CHECK);
		if (iStretch)
			mRelaxThreadData.mStretch = iStretch->GetFVal();
		else
			mRelaxThreadData.mStretch = 0.0f;
		mRelaxThreadData.mCorner = IsDlgButtonChecked(hWnd, IDC_CORNERS_CHECK);

		mRelaxThreadData.mIterations = 1000000;

		ReleaseISpinner(iAmount);
		ReleaseISpinner(iStretch);
	}

	if (operation == KThreadStart)
	{
		mInRelax = true;
		unsigned int threadID;
		if (hWnd)
		{
			ICustButton *iButton = GetICustButton(GetDlgItem(relaxHWND, IDC_RUN_BUTTON));
			if (iButton)
			{
				iButton->SetCheck(TRUE);
				iButton->SetText(GetString(IDS_SPLINEMAP_ENDRELAX));
			}
			if (iButton)
				ReleaseICustButton(iButton);

			HWND iApplyButton = GetDlgItem(relaxHWND, IDC_APPLY);
			if (iApplyButton)
			{
				EnableWindow(iApplyButton, FALSE);
			}
		}
		mRelaxThreadHandle = (HANDLE)_beginthreadex(NULL, 0, &CallRelaxThread, (void *)&mRelaxThreadData, 0, &threadID);
	}
	else if (operation == KThreadReStart)
	{
		mInRestart = true;
		//only restart if we have a current thread running
		if (mRelaxThreadHandle && mRelaxThreadData.mStarted)
		{
			ICustButton *iButton = nullptr;
			if (hWnd)
			{
				iButton = GetICustButton(GetDlgItem(relaxHWND, IDC_RUN_BUTTON));
				if (iButton)
				{
					iButton->SetCheck(FALSE);
				}
			}

			for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
			{
				mMeshTopoData[ldID]->SetUserCancel(TRUE);
			}

			WaitForSingleObject(mRelaxThreadHandle, INFINITE);

			CloseHandle(mRelaxThreadHandle);

			mRelaxThreadHandle = NULL;

			for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
			{
				mMeshTopoData[ldID]->SetUserCancel(FALSE);
			}

			if (iButton)
			{
				iButton->SetCheck(TRUE);
			}

			if (iButton)
				ReleaseICustButton(iButton);

			unsigned int threadID;
			mRelaxThreadHandle = (HANDLE)_beginthreadex(NULL, 0, &CallRelaxThread, (void *)&mRelaxThreadData, 0, &threadID);
		}
		mInRestart = false;
	}
	else if (operation == KThreadEnd)
	{
		while (mInRestart) {};  //we are in a race with the restart wait for the restart to finish
		if (mRelaxThreadHandle)
		{
			if (hWnd)
			{
				ICustButton *iButton = GetICustButton(GetDlgItem(relaxHWND, IDC_RUN_BUTTON));
				if (iButton)
				{
					iButton->SetText(GetString(IDS_SPLINEMAP_STARTRELAX));
					iButton->SetCheck(FALSE);
					ReleaseICustButton(iButton);
				}
			}

			for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
			{
				mMeshTopoData[ldID]->SetUserCancel(TRUE);
			}

			WaitForSingleObject(mRelaxThreadHandle, INFINITE);

			CloseHandle(mRelaxThreadHandle);

			mRelaxThreadHandle = NULL;

			for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
			{
				mMeshTopoData[ldID]->SetUserCancel(FALSE);
			}

			if (hWnd)
			{
				HWND iApplyButton = GetDlgItem(relaxHWND, IDC_APPLY);
				if (iApplyButton)
				{
					EnableWindow(iApplyButton, TRUE);
				}
			}
		}
		mInRelax = false;
	}
}

void UnwrapMod::fnRelax(int iteration, float str, BOOL lockEdges, BOOL matchArea)
{
	theHold.Begin();
	HoldPoints();

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		mMeshTopoData[ldID]->Relax(mTVSubObjectMode, iteration, str, lockEdges, matchArea);
	}

	theHold.Accept(GetString(IDS_PW_MOVE_UVW));
	NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
	InvalidateView();
	if (ip) ip->RedrawViews(ip->GetTime());
}

void UnwrapMod::fnFit(int iteration, float str)
{
	theHold.Begin();
	HoldPoints();
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		mMeshTopoData[ldID]->RelaxFit(mTVSubObjectMode,iteration,str);
	}
	theHold.Accept(GetString(IDS_PW_RELAX));
	NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
	InvalidateView();
	if (ip) ip->RedrawViews(ip->GetTime());
}

float UnwrapMod::fnGetRelaxAmount()
{
	return relaxAmount;
}
void  UnwrapMod::fnSetRelaxAmount(float amount)
{
	relaxAmount = amount;
}

int      UnwrapMod::fnGetRelaxIter()
{
	return relaxIteration;
}

void  UnwrapMod::fnSetRelaxIter(int iter)
{
	if (iter < 1) iter = 1;
	relaxIteration = iter;
}


BOOL  UnwrapMod::fnGetRelaxBoundary()
{
	return relaxBoundary;
}

void  UnwrapMod::fnSetRelaxBoundary(BOOL boundary)
{
	relaxBoundary = boundary;
}

BOOL  UnwrapMod::fnGetRelaxSaddle()
{
	return relaxSaddle;
}

void  UnwrapMod::fnSetRelaxSaddle(BOOL saddle)
{
	relaxSaddle = saddle;
}

void  UnwrapMod::fnRelax2()
{
	theHold.Begin();
	HoldPoints();
	theHold.Accept(GetString(IDS_PW_RELAX));

	if (relaxType == 0)
		fnRelaxByFaceAngle(relaxIteration,relaxStretch,relaxAmount,relaxBoundary);
	else if (relaxType == 1)
		fnRelaxByEdgeAngle(relaxIteration,relaxStretch,relaxAmount,relaxBoundary);
	else if (relaxType == 2)
		RelaxVerts2(relaxAmount, relaxIteration, relaxBoundary, relaxSaddle);
}

void  UnwrapMod::fnRelax2Dialog()
{
	//bring up the dialog
	if (hRelaxDialog == NULL)
	{
		hRelaxDialog = CreateDialogParam(   hInstance,
			MAKEINTRESOURCE(IDD_RELAXDIALOG),
			//                   GetCOREInterface()->GetMAXHWnd(),
			hDialogWnd,
			UnwrapRelaxFloaterDlgProc,
			(LPARAM)this );
		ShowWindow(hRelaxDialog,SW_SHOW);
	}
}

void  UnwrapMod::RelaxVerts2(float relax, int iter, BOOL boundary, BOOL saddle, BOOL updateUI)
{
	bool anythingSelected = AnyThingSelected();
	BOOL applyToAll = TRUE;
	if (anythingSelected)
		applyToAll = FALSE;

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		mMeshTopoData[ldID]->RelaxVerts2(mTVSubObjectMode, relax, iter, boundary, saddle, peltData, applyToAll,updateUI);
		if (mMeshTopoData[ldID]->GetUserCancel())
			ldID = mMeshTopoData.Count();
	}
}

void  UnwrapMod::SetRelaxDialogPos()
{
	if (relaxWindowPos.length != 0) 
		SetWindowPlacement(relaxHWND,&relaxWindowPos);
}

void  UnwrapMod::SaveRelaxDialogPos()
{
	relaxWindowPos.length = sizeof(WINDOWPLACEMENT); 
	GetWindowPlacement(relaxHWND,&relaxWindowPos);
}

INT_PTR CALLBACK UnwrapRelaxFloaterDlgProc(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UnwrapMod *mod = DLGetWindowLongPtr<UnwrapMod*>(hWnd);
	//POINTS p = MAKEPOINTS(lParam); commented out by sca 10/7/98 -- causing warning since unused.

	static ISpinnerControl *iAmount = NULL;
	static ISpinnerControl *iIterations = NULL;
	static ISpinnerControl *iStretch = NULL;

	static BOOL bBoundary = TRUE;
	static BOOL bCorner = FALSE;
	static float amount = 1.0f;
	static int iterations = 1;

	switch (msg) {
	  case WM_INITDIALOG:
		  {
			  mod = (UnwrapMod*)lParam;
			  UnwrapMod::relaxHWND = hWnd;

			  DLSetWindowLongPtr(hWnd, lParam);
			  ::SetWindowContextHelpId(hWnd, idh_unwrap_relax);

			  ICustButton *iButton = GetICustButton(GetDlgItem(hWnd, IDC_RUN_BUTTON));
			  if (iButton)
			  {
				  iButton->SetType(CBT_CHECK);
				  iButton->SetCheck(FALSE);
				  ReleaseICustButton(iButton);
			  }

			  //create relax amount spinner and set value
			  iAmount = SetupFloatSpinner(
				  hWnd,IDC_RELAX_AMOUNTSPIN,IDC_RELAX_AMOUNT,
				  0.0f,1.0f,mod->relaxAmount);  
			  iAmount->SetScale(0.01f);
			  amount = mod->relaxAmount;

			  iIterations = SetupIntSpinner(
				  hWnd,IDC_RELAX_ITERATIONSSPIN,IDC_RELAX_ITERATIONS,
				  0,100000,mod->relaxIteration);   
			  iIterations->SetScale(1.f);
			  iterations = mod->relaxIteration;

			  iStretch = SetupFloatSpinner(
				  hWnd,IDC_RELAX_STRETCHSPIN,IDC_RELAX_STRETCH,
				  0.0f,1.0f,mod->relaxStretch); 
			  iStretch->SetScale(0.001f);

			  HWND hType = GetDlgItem(hWnd,IDC_RELAXTYPE_COMBO);
			  SendMessage(hType, CB_RESETCONTENT, 0, 0);
			  SendMessage(hType, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) GetString(IDS_PW_RELAX_FACE));
			  SendMessage(hType, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) GetString(IDS_PW_RELAX_EDGE));
			  SendMessage(hType, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) GetString(IDS_PW_RELAX_CENTERS));
			  SendMessage(hType, CB_SETCURSEL, (mod->relaxType), 0L);

			  //set align cluster
			  bBoundary = mod->relaxBoundary;
			  bCorner = mod->relaxSaddle;
			  CheckDlgButton(hWnd,IDC_BOUNDARY_CHECK,bBoundary);
			  CheckDlgButton(hWnd,IDC_CORNERS_CHECK,bCorner);

			  //restore window pos
			  mod->SetRelaxDialogPos();
			  //start the hold begin

			  if (mod->relaxType == 2)
			  {           
				  iStretch->Enable(FALSE);
				  HWND hCheck = GetDlgItem(hWnd,IDC_BOUNDARY_CHECK);
				  EnableWindow(hCheck,TRUE);
				  hCheck = GetDlgItem(hWnd,IDC_CORNERS_CHECK);
				  EnableWindow(hCheck,TRUE);
			  }
			  else
			  {           
				  iStretch->Enable(TRUE);
				  HWND hCheck = GetDlgItem(hWnd,IDC_BOUNDARY_CHECK);
				  EnableWindow(hCheck,TRUE);
				  hCheck = GetDlgItem(hWnd,IDC_CORNERS_CHECK);
				  EnableWindow(hCheck,FALSE);
			  }

			  break;
		  }
	  case WM_SYSCOMMAND:
		  if ((wParam & 0xfff0) == SC_CONTEXTHELP) 
		  {
			  MaxSDK::IHelpSystem::GetInstance()->ShowProductHelpForTopic(idh_unwrap_relax); 
		  }
		  return FALSE;
		  break;
	  case CC_SPINNER_BUTTONDOWN:
		  if (LOWORD(wParam) == IDC_UNWRAP_BIASSPIN) 
		  {
		  }
		  break;

	  case WM_CUSTEDIT_ENTER:
	  case CC_SPINNER_BUTTONUP:
		  mod->RelaxThreadOp(UnwrapMod::KThreadReStart,hWnd);
		  break;

	  case WM_CLOSE:
		  mod->SaveRelaxDialogPos();
		  mod->RelaxThreadOp(UnwrapMod::KThreadEnd,hWnd);

		  if (iAmount) ReleaseISpinner(iAmount);
		  if (iIterations) ReleaseISpinner(iIterations);
		  if (iStretch) ReleaseISpinner(iStretch);
		  iAmount = NULL;
		  iIterations = NULL;
		  iStretch = NULL;

		  mod->hRelaxDialog = NULL;
		  EndDialog(hWnd,0);
		  break;
	  case WM_ACTIVATE:
		  mod->RelaxThreadOp(UnwrapMod::KThreadEnd,hWnd);
		  break;

	  case WM_COMMAND:
		  switch (LOWORD(wParam)) 
		  {

		  case IDC_BOUNDARY_CHECK:
		  case IDC_CORNERS_CHECK:
			  mod->RelaxThreadOp(UnwrapMod::KThreadReStart,hWnd);
			  break;
		  case IDC_RUN_BUTTON:
			  {
				  ICustButton *iButton = GetICustButton(GetDlgItem(hWnd, IDC_RUN_BUTTON));
				  if (iButton && iButton->IsChecked())
				  {
					  theHold.Begin();
					  mod->HoldPoints();
					  theHold.Accept(GetString(IDS_PW_RELAX));

					  mod->RelaxThreadOp(UnwrapMod::KThreadStart,hWnd);

				  }
				  else
					  mod->RelaxThreadOp(UnwrapMod::KThreadEnd,hWnd);

				  mod->InvalidateView();
				  TimeValue t = GetCOREInterface()->GetTime();
				  mod->NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
				  GetCOREInterface()->RedrawViews(t);

				  break;
			  }
			  break;
		  case IDC_APPLY:
			  {
				  //make sure when end any existing threads
				  mod->RelaxThreadOp(UnwrapMod::KThreadEnd,hWnd);

				  theHold.Begin();
				  mod->HoldPoints();
				  theHold.Accept(GetString(IDS_PW_RELAX));

				  bBoundary = IsDlgButtonChecked(hWnd,IDC_BOUNDARY_CHECK);
				  bCorner = IsDlgButtonChecked(hWnd,IDC_CORNERS_CHECK);

				  amount = iAmount->GetFVal();
				  float stretch = iStretch->GetFVal();
				  iterations = iIterations->GetIVal();

				  HWND hType = GetDlgItem(hWnd,IDC_RELAXTYPE_COMBO);
				  int type = SendMessage(hType, CB_GETCURSEL, 0L, 0);

				  HWND hStatus = GetDlgItem(hWnd,IDC_STATUSTEXT);

				  if (type == 0)
					  mod->fnRelaxByFaceAngle(iterations,stretch,amount,bBoundary,hStatus);
				  else if (type == 1)
					  mod->fnRelaxByEdgeAngle(iterations,stretch,amount,bBoundary,hStatus);
				  else if (type == 2)
				  {
					  //KZ, 04/2017, Added a divide by 100 to shift down the parameter space
					  //for relax by center. Default value of 0.1 was too fast.
					  mod->RelaxVerts2(amount * 0.01, iterations, bBoundary, bCorner);
				  }

				  mod->InvalidateView();
				  TimeValue t = GetCOREInterface()->GetTime();
				  mod->NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
				  GetCOREInterface()->RedrawViews(t);

				  mod->relaxAmount = amount;
				  mod->relaxIteration = iterations;
				  mod->relaxStretch = stretch;
				  mod->relaxType = type;
				  mod->relaxBoundary = bBoundary;
				  mod->relaxSaddle = bCorner;
				  break;
			  }

		  case IDC_REVERT:
			  {
				  mod->RelaxThreadOp(UnwrapMod::KThreadEnd,hWnd);

				  mod->hRelaxDialog = NULL;
				  EndDialog(hWnd,0);
				  break;
			  }
		  case IDC_RELAXTYPE_COMBO:
			  if (HIWORD(wParam)== CBN_SELCHANGE)
			  {
				  mod->RelaxThreadOp(UnwrapMod::KThreadReStart,hWnd);

				  HWND hType = GetDlgItem(hWnd,IDC_RELAXTYPE_COMBO);
				  int type = SendMessage(hType, CB_GETCURSEL, 0L, 0);
				  mod->relaxType = type;
				  // update icon on UV panel
				  mod->GetUIManager()->SetFlyOut(ID_RELAX_MOVE_BRUSH, type, FALSE);
				  if (type == 2)
				  {           
					  iStretch->Enable(FALSE);
					  HWND hCheck = GetDlgItem(hWnd,IDC_BOUNDARY_CHECK);
					  EnableWindow(hCheck,TRUE);
					  hCheck = GetDlgItem(hWnd,IDC_CORNERS_CHECK);
					  EnableWindow(hCheck,TRUE);
				  }
				  else
				  {           
					  iStretch->Enable(TRUE);
					  HWND hCheck = GetDlgItem(hWnd,IDC_BOUNDARY_CHECK);
					  EnableWindow(hCheck,TRUE);
					  hCheck = GetDlgItem(hWnd,IDC_CORNERS_CHECK);
					  EnableWindow(hCheck,FALSE);
				  }
			  }
			  break;
		  case IDC_DEFAULT:
			  {
				  mod->RelaxThreadOp(UnwrapMod::KThreadEnd,hWnd);
				  //get bias
				  amount = iAmount->GetFVal();
				  iterations = iIterations->GetIVal();
				  float stretch = iStretch->GetFVal();
				  HWND hType = GetDlgItem(hWnd,IDC_RELAXTYPE_COMBO);
				  int type = SendMessage(hType, CB_GETCURSEL, 0L, 0);

				  mod->relaxAmount = amount;
				  mod->relaxIteration = iterations;
				  mod->relaxStretch = stretch;
				  mod->relaxType = type;

				  //get align
				  bCorner = IsDlgButtonChecked(hWnd,IDC_CORNERS_CHECK);
				  bBoundary = IsDlgButtonChecked(hWnd,IDC_BOUNDARY_CHECK);
				  mod->relaxBoundary = bBoundary;
				  mod->relaxSaddle = bCorner;
				  //set as defaults
				  mod->fnSetAsDefaults(RELAXSECTION);
				  break;
			  }
		  }
		  break;
	  default:
		  return FALSE;
	}
	return TRUE;
}

float UnwrapMod::fnGetRelaxStretch()
{
	return relaxStretch;
}

void UnwrapMod::fnSetRelaxStretch(float stretch)
{
	if (stretch < 0.0f) stretch = 0.0f;
	if (stretch > 1.0f) stretch = 1.0f;
	relaxStretch = stretch;
}

int UnwrapMod::fnGetRelaxType()
{
	return relaxType;
}

void UnwrapMod::fnSetRelaxType(int type)
{
	if (type < 0) type = 0;
	if (type > 2) type = 2;
	if (type != relaxType)
	{
		relaxType = type;
		// update UI in UV Panel
		mUIManager.SetFlyOut(ID_RELAX_MOVE_BRUSH, relaxType, FALSE);
		// update UI in Relax Dialog
		HWND hType = GetDlgItem(relaxHWND, IDC_RELAXTYPE_COMBO);
		SendMessage(hType, CB_SETCURSEL, relaxType, 0L);
	}
}
