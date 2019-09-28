/*

Copyright 2010 Autodesk, Inc.  All rights reserved. 

Use of this software is subject to the terms of the Autodesk license agreement provided at 
the time of installation or download, or which otherwise accompanies this software in either 
electronic or hard copy form. 

*/

//**************************************************************************/
// DESCRIPTION: Unwrap UI classes
// AUTHOR: Peter Watje
// DATE: 2010/08/31 
//***************************************************************************/

/*




*/

#include "UnwrapCustomUI.h"
#include "unwrap.h"
#include "modsres.h"

#define SAFE_RELEASE_ISPINNER(P)     if(P){ReleaseISpinner(P); P = NULL;}
#define SAFE_RELEASE_ICUSTEDIT(P)    if(P){ReleaseICustEdit(P); P = NULL;}
#define SAFE_RELEASE_ICUSTBUTTON(P)  if(P){ReleaseICustButton(P); P = NULL;}

UnwrapButton::UnwrapButton(int id, HWND hwnd)
{
	mID = id;
	mButton = GetICustButton(hwnd);
}
UnwrapButton::~UnwrapButton()
{
	if (mButton)
		ReleaseICustButton(mButton);
}
UnwrapButton::UnwrapButton() : mID(0)
{
	mButton = NULL;
}
ICustButton* UnwrapButton::ButtonPtr()
{
	return mButton;
}
int UnwrapButton::ID()
{
	return mID;
}
	 

UnwrapCustomUI::UnwrapCustomUI()
{
	mMod = NULL;
	mhInstance = NULL;
}
UnwrapCustomUI::~UnwrapCustomUI()
{
	Free( NULL );	
	FreeButtons();
}

void UnwrapCustomUI::FreeButtons()
{
	for (int i = 0; i < mButtons.Count(); i++)
	{
		if (mButtons[i])
			delete mButtons[i];
		mButtons[i] = NULL;
	}
	mButtons.SetCount(0);
}

void UnwrapCustomUI::Free( HWND parentWindow)
{
	// Release cached UI control pointers
	while( mSpinnerControls.begin() != mSpinnerControls.end() )
	{
		SAFE_RELEASE_ISPINNER( mSpinnerControls.begin()->second );
		mSpinnerControls.erase( mSpinnerControls.begin() );
	}
	while( mEditControls.begin() != mEditControls.end() )
	{
		SAFE_RELEASE_ICUSTEDIT( mEditControls.begin()->second );
		mEditControls.erase( mEditControls.begin() );
	}
	while( mButtonControls.begin() != mButtonControls.end() )
	{
		SAFE_RELEASE_ICUSTBUTTON( mButtonControls.begin()->second );
		mButtonControls.erase( mButtonControls.begin() );
	}

	for (int i = 0; i < mToolbars.Count(); i++)
	{
		if (mToolbars[i] == NULL)
		{
			mToolbars.Delete(i,1);
			mToolbars[i] = NULL;
			i--;
		}
		else
		{
			bool del = false;
			if (parentWindow == NULL)
				del = true;
			else if ( parentWindow == mToolbars[i]->GetParWindow() )
				del = true;
			if (del)
			{
				delete mToolbars[i];
				mToolbars.Delete(i,1);
				i--;
			}
		}
	}
}

void UnwrapCustomUI::SetMod(UnwrapMod *mod, HINSTANCE hinstance)
{
	mMod = mod;
	mhInstance = hinstance;
	PeelModeDialog::GetSingleton().SetMod(mod);
}


int UnwrapCustomUI::NumberToolBars()
{
	return mToolbars.Count();
}

const MCHAR* UnwrapCustomUI::Name(int index)
{
	if ( (index >= 0) && (index < mToolbars.Count()))
	{
		return mToolbars[index]->Name();
	}
	return NULL;
}

ToolBarFrame* UnwrapCustomUI::GetToolBarFrame(const MCHAR* name)
{
	for (int i = 0; i < mToolbars.Count(); i++)
	{
		if (_tcscmp(name,mToolbars[i]->Name()) == 0)
			return mToolbars[i];
	}
	return NULL;
}

void UnwrapCustomUI::AddButton(int id,HWND hWnd, BOOL checkButton)
{
	UnwrapButton *but = new UnwrapButton(id,hWnd);
	if (but)
	{
		if (checkButton)
			but->ButtonPtr()->SetType(CBT_CHECK);
		mButtons.Append(1,&but,100);
	}
}

void UnwrapCustomUI::AppendToolBar(int owner, HWND parent, const MCHAR* name, int pos,  int x, int y, int width, int height, bool popup)
{
	ToolBarFrame *toolBarFrame = new ToolBarFrame(mhInstance,owner,parent,x,y,width,height,(ToolBarFrame::DockPos)pos,popup,name);	
	toolBarFrame->Show(true);

	mToolbars.Append(1,&toolBarFrame,10);

}

void UnwrapCustomUI::AppendToolBar(HWND hwnd, int owner, const MCHAR* name)
{
	ToolBarFrame *toolBarFrame = new ToolBarFrame(hwnd, owner ,name);	
	toolBarFrame->Show(true);

	mToolbars.Append(1,&toolBarFrame,10);

}

void UnwrapCustomUI::UpdatePositions()
{
	int ct = 0;
	int processed = 0;
	mCornerHeight[0] = 0;
	mCornerHeight[1] = 0;
	mCornerHeight[2] = 0;
	mCornerHeight[3] = 0;

	Tab<int> upLeftW;
	Tab<int> upRightW;
	Tab<int> lowerLeftW;
	Tab<int> lowerRightW;

	for (int j = 0; j < mToolbars.Count(); j++)
	{
		int w = mToolbars[j]->Width();
		if (mToolbars[j]->Location() == ToolBarFrame::kDockUpperLeft)
			upLeftW.Append(1,&w,10);
		else if (mToolbars[j]->Location() == ToolBarFrame::kDockUpperRight)
			upRightW.Append(1,&w,10);
		else if (mToolbars[j]->Location() == ToolBarFrame::kDockLowerLeft)
			lowerLeftW.Append(1,&w,10);
		else if (mToolbars[j]->Location() == ToolBarFrame::kDockLowerRight)
			lowerRightW.Append(1,&w,10);
	}

	
	int topIndex = 0;
	int bottomIndex = 0;
	while (processed != mToolbars.Count())
	{
		HWND parentHWND = mToolbars[ct]->GetParWindow();
		int yPos[4];
		Rect r;
		GetClientRect(parentHWND,&r);
		yPos[0] = 0;
		yPos[1] = 0;
		yPos[2] = r.h() - mToolbars[ct]->Height()-2;
		yPos[3] = r.h() - mToolbars[ct]->Height()-2;
		int clientWidth  = r.w();

		bool ctSet = false;
		for (int j = (ct); j < mToolbars.Count(); j++)
		{
			if (mToolbars[j]->GetParWindow() == parentHWND)
			{
				int w = mToolbars[j]->Width();
				int h = mToolbars[j]->Height();


				if (mToolbars[j]->Location() == ToolBarFrame::kFloat)
					mToolbars[j]->UpdateWindow();
				else if (mToolbars[j]->Location() == ToolBarFrame::kDockUpperLeft)
				{
					if (topIndex < upRightW.Count())
						w = clientWidth - upRightW[topIndex];
					mToolbars[j]->ResizeWindow(0,yPos[0],w,h,FALSE);
					yPos[0] += h;
					mCornerHeight[0] += h;
					topIndex++;
				}
				else if (mToolbars[j]->Location() == ToolBarFrame::kDockUpperRight)
				{
					int x = r.w()-w;
					if (x < 0)
						x = 0;
					int tw = w;
					if (x + tw > clientWidth)
						tw = clientWidth - x;

					mToolbars[j]->ResizeWindow(x,yPos[1],tw,h,FALSE);
					yPos[1] += h;
					mCornerHeight[1] += h;
					topIndex++;
				}
				else if (mToolbars[j]->Location() == ToolBarFrame::kDockLowerLeft)
				{
					if (bottomIndex < lowerRightW.Count())
						w = clientWidth - lowerRightW[bottomIndex];
					mToolbars[j]->ResizeWindow(0,yPos[2],w,h,FALSE);
					yPos[2] -= h;
					mCornerHeight[2] += h;
					bottomIndex++;
				}
				else if (mToolbars[j]->Location() == ToolBarFrame::kDockLowerRight)
				{
					int x = r.w()-w;
					if (x < 0)
						x = 0;
					int tw = w;
					if (x + tw > clientWidth)
						tw = clientWidth - x;


					mToolbars[j]->ResizeWindow(x,yPos[3],tw,h,FALSE);
					yPos[3] -= h;
					mCornerHeight[3] += h;
					bottomIndex++;
				}
				processed++;

				
			}
			else
			{
				if ((ctSet == false) && (j != ct))
				{
					ct = j;
					ctSet = true;
				}				
			}
		}
	}
}

int UnwrapCustomUI::GetFlyOut(int buttonID)
{
	ICustButton *button = GetButtonControl(buttonID);
	if (button)
	{
		int v = button->GetCurFlyOff(); 
		return v;
	}
	return 0;
}

void UnwrapCustomUI::SetFlyOut(int buttonID, int flyOut, BOOL notify)
{
	ICustButton *button = GetButtonControl(buttonID);
	if (button)
	{
		button->SetCurFlyOff(flyOut,notify);
	}
}


void UnwrapCustomUI::Enable(int buttonID, BOOL enable)
{
	ICustButton *button = GetButtonControl(buttonID);
	if (button)
	{
		if (enable)
			button->Enable();
		else
			button->Disable();
	}

	ISpinnerControl *spinner = GetSpinnerControl(buttonID);
	if (spinner)
	{
		ShowWindow(spinner->GetHwnd(), enable ? SW_SHOW : SW_HIDE);
		spinner->SetIndeterminate(!enable);
		spinner->Enable(enable);
	}					

	for (int i = 0; i < mButtons.Count(); i++)
	{
		if (mButtons[i] && mButtons[i]->ID() == buttonID)
		{
			if (enable)
				mButtons[i]->ButtonPtr()->Enable();
			else
				mButtons[i]->ButtonPtr()->Disable();
		}
	}
}


void UnwrapCustomUI::SetIndeterminate(int buttonID,BOOL indet)
{
	ISpinnerControl *spin = GetSpinnerControl(buttonID);
	if (spin)
	{
		spin->SetIndeterminate(indet);
	}
}

void UnwrapCustomUI::SetSpinFValue(int buttonID, float v)
{
	ISpinnerControl *spin = GetSpinnerControl(buttonID);
	if (spin)
	{
		int axis;
		if( (mMod!=NULL) && (mMod->IsSpinnerPixelUnits(buttonID,&axis)) )
			spin->SetValue((int)round(v * mMod->GetScalePixelUnits(axis)),FALSE);
		else
			spin->SetValue(v,FALSE);
	}
}

float UnwrapCustomUI::GetSpinFValue(int buttonID)
{
	ISpinnerControl *spin = GetSpinnerControl(buttonID);
	if (spin)
	{
		float v= spin->GetFVal();
		int axis;
		if( (mMod!=NULL) && (mMod->IsSpinnerPixelUnits(buttonID,&axis)) )
			v = (v / mMod->GetScalePixelUnits(axis));
		return v;
	}
	return 0.0f;
}

bool UnwrapCustomUI:: IsSpinEnabled(int buttonID)
{
	ISpinnerControl *spin = GetSpinnerControl(buttonID);
	if (spin)
	{
		bool bEnabled = spin->IsEnabled() ? true : false;
		return bEnabled;
	}
	return true;
}

void UnwrapCustomUI::SetSpinIValue(int buttonID, int v)
{
	ISpinnerControl *spin = GetSpinnerControl(buttonID);
	if (spin)
	{
		int axis;
		if( (mMod!=NULL) && (mMod->IsSpinnerPixelUnits(buttonID,&axis)) )
			spin->SetValue((int)(v * mMod->GetScalePixelUnits(axis)),FALSE);
		else
			spin->SetValue(v,FALSE);
	}
}

int UnwrapCustomUI::GetSpinIValue(int buttonID)
{
	ISpinnerControl *spin = GetSpinnerControl(buttonID);
	if (spin)
	{
		int v= spin->GetIVal();
		int axis;
		if( (mMod!=NULL) && (mMod->IsSpinnerPixelUnits(buttonID,&axis)) )
			v = (v / mMod->GetScalePixelUnits(axis));
		return v;
	}
	return 0;
}

BOOL UnwrapCustomUI::IsChecked(int buttonID)
{
	for (int i = 0; i < mToolbars.Count(); i++)
	{
		//get the tool bar
		HWND toolBarHWND = mToolbars[i]->GetToolbarWindow();
		ICustToolbar *toolBar = GetICustToolbar(toolBarHWND); 
		if (toolBar)
		{
			//get the elements
			int numItems = toolBar->GetNumItems();
			// see if it is a check box if so update it
			for (int j = 0; j < numItems; j++)
			{
				int testId = toolBar->GetItemID(j);
				if ( buttonID == testId )
				{
					if (SendMessage(toolBar->GetItemHwnd(buttonID),BM_GETCHECK,0,0) == BST_CHECKED)
					{
						ReleaseICustToolbar(toolBar);
						return TRUE;
					}
					else
					{
						ReleaseICustToolbar(toolBar);
						return FALSE;
					}

				}
			}
			//fix mem leak
			ReleaseICustToolbar(toolBar);
		}
	}

	for (int i = 0; i < mButtons.Count(); i++)
	{
		if (mButtons[i] && mButtons[i]->ID() == buttonID)
		{
			return mButtons[i]->ButtonPtr()->IsChecked();
		}
	}

	return FALSE;
}

HWND UnwrapCustomUI::FindControl(int buttonID, BOOL* isButton)
{
	for (int i = 0; i < mToolbars.Count(); i++)
	{
		//get the tool bar
		HWND toolBarHWND = mToolbars[i]->GetToolbarWindow();
		ICustToolbar *toolBar = GetICustToolbar(toolBarHWND); 
		if (toolBar)
		{
			//get the elements
			int numItems = toolBar->GetNumItems();
			for (int j = 0; j < numItems; j++)
			{
				int testId = toolBar->GetItemID(j);
				if ( buttonID == testId )
				{
					ICustButton* button = toolBar->GetICustButton(buttonID);
					if( isButton!=NULL )
						(*isButton) = (button!=NULL);
					// Can't use toolBar->GetItemHwnd() if the item is a button,
					// the method returns NULL in that case
					HWND controlHWnd =
						(button!=NULL? button->GetHwnd() : toolBar->GetItemHwnd(buttonID));
					if( button!=NULL ) ReleaseICustButton(button);
					ReleaseICustToolbar(toolBar);
					return controlHWnd;
				}
			}
			//fix mem leak
			ReleaseICustToolbar(toolBar);
		}
	}
	return NULL;
}

ISpinnerControl* UnwrapCustomUI::GetSpinnerControl(int controlID)
{
	ISpinnerControl* item = NULL;
	
	// Get UI control from mapping, or, find and add it to mapping
	IDToSpinnerMap::iterator iter = mSpinnerControls.find(controlID);
	if( iter!=mSpinnerControls.end() )
		item = iter->second;
	else
	{
		// Error checking, ensure caller didn't pass a button ID by mistake
		BOOL isButton=FALSE;
		HWND hSpinner = FindControl( controlID, &isButton );
		if( hSpinner!=NULL )
		{
			if( isButton==FALSE )
				item = GetISpinner( hSpinner );
			mSpinnerControls[controlID] = item;
		}
		// else if null, might not be initialized yet,
		// so don't update the mapping, but search again later
	}
	return item;
}

ICustEdit* UnwrapCustomUI::GetEditControl(int controlID)
{
	ICustEdit* item = NULL;
	
	// Get UI control from mapping, or, find and add it to mapping
	IDToEditMap::iterator iter = mEditControls.find(controlID);
	if( iter!=mEditControls.end() )
		item = iter->second;
	else
	{
		// Error checking, ensure caller didn't pass a button ID by mistake
		BOOL isButton=FALSE;
		HWND hEdit = FindControl( controlID, &isButton );
		if( hEdit!=NULL )
		{
			if( isButton==FALSE )
				item = GetICustEdit( hEdit );
			mEditControls[controlID] = item;
		}
		// else if null, might not be initialized yet,
		// so don't update the mapping, but search again later
	}
	return item;
}

ICustButton* UnwrapCustomUI::GetButtonControl(int controlID)
{
	ICustButton* item = NULL;

	// Get UI control from mapping, or, find and add it to mapping
	IDToButtonMap::iterator iter = mButtonControls.find(controlID);
	if( iter!=mButtonControls.end() )
		item = iter->second;
	else
	{
		// Error checking, ensure caller passed a valid button ID,
		// not some other control type by mistake
		BOOL isButton=FALSE;
		HWND hButton = FindControl( controlID, &isButton );
		if( hButton!=NULL )
		{
			if( isButton==TRUE )
				item = GetICustButton( hButton );
			mButtonControls[controlID] = item;
		}
		// else if null, might not be initialized yet,
		// so don't update the mapping, but search again later
	}
	return item;
}

template <typename ToolBarFrameType>
void UnwrapCustomUI::UpdateCheckButtonsOnToolBars(const Tab<ToolBarFrameType*> &toolbars)
{
	//loop through all the tool bars 
	for (int i = 0; i < toolbars.Count(); i++)
	{
		//get the tool bar
		HWND toolBarHWND = toolbars[i]->GetToolbarWindow();
		ICustToolbar *toolBar = GetICustToolbar(toolBarHWND); 
		if (toolBar)
		{
			//get the elements
			int numItems = toolBar->GetNumItems();
			// see if it is a check box if so update it
			for (int j = 0; j < numItems; j++)
			{
				int id = toolBar->GetItemID(j);

				HWND toolHWND = toolBar->GetItemHwnd(id);

				//this ownly works with items tagged as other
				if (toolHWND)
				{
					BOOL currentlyEnabled = IsWindowEnabled(toolHWND);
					BOOL needsToBe = mMod->WtIsEnabled(id);
					if (currentlyEnabled != needsToBe)
						EnableWindow(toolHWND,needsToBe);
				}


				ICustButton *button = toolBar->GetICustButton(id);
				if (button)
				{
					BOOL currentState = mMod->WtIsChecked(id);
					BOOL checked = button->IsChecked();
					if (currentState != checked)
						button->SetCheck(currentState);

					BOOL currentlyEnabled = button->IsEnabled();
					BOOL needsToBe = mMod->WtIsEnabled(id);
					if (currentlyEnabled != needsToBe)
						button->Enable(needsToBe);

					ReleaseICustButton(button);
				}
				else //it may be check box
				{
					HWND hwnd = toolBar->GetItemHwnd(id);
					BOOL currentState = mMod->WtIsChecked(id);
					BOOL checked = FALSE;
					if (SendMessage(hwnd,BM_GETCHECK,0,0) == BST_CHECKED)
						checked = TRUE;
					if (currentState != checked)
					{
						if (currentState)
							SendMessage(hwnd,BM_SETCHECK,BST_CHECKED,0);
						else
							SendMessage(hwnd,BM_SETCHECK,BST_UNCHECKED,0);

					}

				}
			}
			ReleaseICustToolbar(toolBar);
		}
	}
}

void UnwrapCustomUI::UpdateCheckButtonsOnButtons(const Tab<UnwrapButton*> &buttons)
{
	for (int i = 0; i < buttons.Count(); i++)
	{
		ICustButton *button = buttons[i]->ButtonPtr();
		if (button)
		{
			int id = buttons[i]->ID();
			BOOL currentState = mMod->WtIsChecked(id);
			BOOL checked = button->IsChecked();
			if (currentState != checked)
				button->SetCheck(currentState);

			BOOL currentlyEnabled = button->IsEnabled();
			BOOL needsToBe = mMod->WtIsEnabled(id);
			if (currentlyEnabled != needsToBe)
				button->Enable(needsToBe);
		}
	}
}

//updates all our check boxes UIs
void UnwrapCustomUI::UpdateCheckButtons()
{
	UpdateCheckButtonsOnToolBars(mToolbars);
	UpdateCheckButtonsOnButtons(mButtons);
	//Part of the fix for MAXX-32379 - we need to make sure the spinners are updated
	if (mMod) mMod->SetupTypeins();
}


void UnwrapCustomUI::InvalidateToolbar(HWND parent)
{
	for (int i = 0; i < mToolbars.Count(); i++)
	{
		bool paint = false;
		if (parent == NULL)
			paint = true;
		else if (mToolbars[i]->GetParWindow() == parent)
			paint = true;
		if (paint)
			InvalidateRect(mToolbars[i]->GetToolbarWindow(),NULL,TRUE);

	}
}


int UnwrapCustomUI::MessageProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL iret = FALSE;
	switch (msg) 
	{
		case WM_INITDIALOG:
			{			
				break;
			}
		case CC_SPINNER_BUTTONDOWN:
			if ( (LOWORD(wParam) == ID_SPINNERU)  ||
				 (LOWORD(wParam) == ID_SPINNERV) ||
				 (LOWORD(wParam) == ID_SPINNERW) )
			{
				iret = TRUE;
			}
			break;

		case CC_SPINNER_CHANGE:
			if (LOWORD(wParam) == ID_SOFTSELECTIONLIMIT_SPINNER) 
			{
				mMod->fnSetLimitSoftSelRange(mMod->GetUIManager()->GetSpinIValue(ID_SOFTSELECTIONLIMIT_SPINNER));
				mMod->RebuildDistCache();
				UpdateWindow(hWnd);
				mMod->InvalidateView();
				iret = TRUE;
			}			
			else if (LOWORD(wParam) == ID_SOFTSELECTIONSTR_SPINNER) 
			{
				mMod->falloffStr = mMod->GetUIManager()->GetSpinFValue(ID_SOFTSELECTIONSTR_SPINNER);
				mMod->RebuildDistCache();
				UpdateWindow(hWnd);
				mMod->InvalidateView();
				iret = TRUE;
			}
			else  if ( (LOWORD(wParam) == ID_SPINNERU)  ||
				       (LOWORD(wParam) == ID_SPINNERV) ||
					   (LOWORD(wParam) == ID_SPINNERW) )
			{
				if (!theHold.Holding()) {
					theHold.SuperBegin();
					theHold.Begin();
				}


				switch (LOWORD(wParam)) {
				case ID_SPINNERU: // IDC_UNWRAP_USPIN:
					mMod->tempWhich = 0;
					mMod->TypeInChanged(0);
					break;
				case ID_SPINNERV: // case IDC_UNWRAP_VSPIN:
					mMod->tempWhich = 1;
					mMod->TypeInChanged(1);
					break;
				case ID_SPINNERW: // case IDC_UNWRAP_WSPIN:
					mMod->tempWhich = 2;
					mMod->TypeInChanged(2);
					break;
				}

				UpdateWindow(hWnd);
				iret = TRUE;
			}
			break;

		case WM_CUSTEDIT_ENTER:
		case CC_SPINNER_BUTTONUP:
			if ( (LOWORD(wParam) == ID_BRUSH_FALLOFFEDIT) ||
				(LOWORD(wParam) == ID_BRUSH_FALLOFFSPINNER) )
			{
				float val = mMod->GetUIManager()->GetSpinFValue(ID_BRUSH_FALLOFFSPINNER);
				mMod->fnSetPaintFallOffSize(val);
			}
			else if ( (LOWORD(wParam) == ID_BRUSH_STRENGTHEDIT) ||
				(LOWORD(wParam) == ID_BRUSH_STRENGTHSPINNER) )
			{
				float val = mMod->GetUIManager()->GetSpinFValue(ID_BRUSH_STRENGTHSPINNER);
				mMod->fnSetPaintFullStrengthSize(val);
			}
			else if ( (LOWORD(wParam) == ID_GROUPSETDENSITY_SPINNER) ||
				(LOWORD(wParam) == ID_GROUPSETDENSITY_EDIT) )
			{
				float val = mMod->GetUIManager()->GetSpinFValue(ID_GROUPSETDENSITY_SPINNER);
				mMod->fnGroupSetTexelDensity(val);
			}
			else if ( (LOWORD(wParam) == ID_PLANARSPIN) ||
				(LOWORD(wParam) == ID_PLANAREDIT) )
			{
				float angle = mMod->GetUIManager()->GetSpinFValue(ID_PLANARSPIN);
				mMod->fnSetGeomPlanarModeThreshold(angle);
				//send macro message
				TSTR mstr = mMod->GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap2.setGeomPlanarThreshold"));
				macroRecorder->FunctionCall(mstr, 1, 0,
					mr_float,mMod->fnGetGeomPlanarModeThreshold());
				macroRecorder->EmitScript();
			}
			else if ( (LOWORD(wParam) == ID_SOFTSELECTIONSTR_SPINNER)  ||
				      (LOWORD(wParam) == ID_SOFTSELECTIONSTR_EDIT)  )
			{
				mMod->falloffStr = mMod->GetUIManager()->GetSpinFValue(ID_SOFTSELECTIONSTR_SPINNER);
				mMod->RebuildDistCache();
				UpdateWindow(hWnd);
				mMod->InvalidateView();
				iret = TRUE;
			}
			else if ( (LOWORD(wParam) == ID_SOFTSELECTIONLIMIT_SPINNER) ||
				      (LOWORD(wParam) == ID_SOFTSELECTIONLIMIT_EDIT) )
			{
				float str = mMod->GetUIManager()->GetSpinFValue(ID_SOFTSELECTIONLIMIT_SPINNER);//iStr->GetFVal();
				TSTR mstr = mMod->GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap.setFalloffDist"));
				macroRecorder->FunctionCall(mstr, 1, 0,
					mr_float,str);
				macroRecorder->EmitScript();
				mMod->RebuildDistCache();
				mMod->InvalidateView();
				UpdateWindow(hWnd);
				iret = TRUE;
			}
			else  if ( (LOWORD(wParam) == ID_SPINNERU)  ||
				(LOWORD(wParam) == ID_SPINNERV) ||
				(LOWORD(wParam) == ID_SPINNERW) ||
			 	(LOWORD(wParam) == ID_EDITU) ||
				(LOWORD(wParam) == ID_EDITV) ||
				(LOWORD(wParam) == ID_EDITW) 
				)
			{
				if (HIWORD(wParam) || msg==WM_CUSTEDIT_ENTER) {

					if (theHold.Holding())
					{
						theHold.Accept(GetString(IDS_PW_MOVE_UVW));
						theHold.SuperAccept(GetString(IDS_PW_MOVE_UVW));
					}


					if (mMod->tempWhich ==0)
					{
						TSTR mstr = mMod->GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap.moveX"));
						macroRecorder->FunctionCall(mstr, 1, 0,
							mr_float,mMod->tempAmount);
					}
					else if (mMod->tempWhich ==1)
					{
						TSTR mstr = mMod->GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap.moveY"));
						macroRecorder->FunctionCall(mstr, 1, 0,
							mr_float,mMod->tempAmount);
					}
					else if (mMod->tempWhich ==2)
					{
						TSTR mstr = mMod->GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap.moveZ"));
						macroRecorder->FunctionCall(mstr, 1, 0,
							mr_float,mMod->tempAmount);
					}

					
					mMod->SetupTypeins();


				} else {
					theHold.Cancel();
					theHold.SuperCancel();

					mMod->NotifyDependents(FOREVER, PART_TEXMAP, REFMSG_CHANGE);
					mMod->InvalidateView();
					UpdateWindow(hWnd);
					GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());

					mMod->SetupTypeins();

				}
				iret = TRUE;
			}
			else if ( (LOWORD(wParam) == ID_WELDTHRESH_SPINNER) ||
				(LOWORD(wParam) == ID_WELDTHRESH_EDIT) )
			{
				float val = mMod->GetUIManager()->GetSpinFValue(ID_WELDTHRESH_SPINNER);
				mMod->fnSetWeldThreshold(val);
			}
			break;
		case WM_NOTIFY:
			{
				LPNMHDR hdr = (LPNMHDR)lParam;
				if(hdr->code == TTN_NEEDTEXT) 
				{
					LPTOOLTIPTEXT lpttt;
					lpttt = (LPTOOLTIPTEXT)hdr;				
					switch (lpttt->hdr.idFrom) {
					case ID_TOOL_WELDDROPDOWN:
						switch(mMod->GetUIManager()->GetFlyOut(ID_TOOL_WELDDROPDOWN)){
						case 0:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_WELDSELECTED);
							break;
						case 1:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_WELDSELECTEDSHARED);
							break;
						case 2:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_WELDALLSHARED);
							break;
						}				
						break;
					case ID_FREEFORMSNAP:
						switch(mMod->GetUIManager()->GetFlyOut(ID_FREEFORMSNAP)){
						case 0:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_FREEFORMSNAPUPPERLEFT);
							break;
						case 1:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_FREEFORMSNAPUPPERRIGHT);
							break;
						case 2:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_FREEFORMSNAPLOWERLEFT);
							break;
						case 3:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_FREEFORMSNAPLOWERRIGHT);
							break;
						case 4:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_FREEFORMSNAPCENTER);
							break;
						}
						break;
					case ID_ALIGNH_BUTTONS:
						switch(mMod->GetUIManager()->GetFlyOut(ID_ALIGNH_BUTTONS)){
						case 0:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_ALIGNHPIVOT);
							break;
						case 1:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_ALIGNHPLACE);
							break;
						}
						break;
					case ID_ALIGNV_BUTTONS:
						switch(mMod->GetUIManager()->GetFlyOut(ID_ALIGNV_BUTTONS)){
						case 0:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_ALIGNVPIVOT);
							break;
						case 1:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_ALIGNVPLACE);
							break;
						}
						break;
					case ID_RELAXBUTTONS:
						switch(mMod->GetUIManager()->GetFlyOut(ID_RELAXBUTTONS)){
						case 0:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_RELAXBUTTONS);
							break;
						case 1:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_RELAXSETTINGS);
							break;
						}
						break;
					case ID_STITCHBUTTONS:
						switch(mMod->GetUIManager()->GetFlyOut(ID_STITCHBUTTONS)){
						case 0:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_STITCHBUTTONS);
							break;
						case 1:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_STITCHSETTINGS);
							break;
						}
						break;
					case ID_FLATTENBUTTONS:
						switch(mMod->GetUIManager()->GetFlyOut(ID_FLATTENBUTTONS)){
						case 0:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_FLATTENMAPDIALOG);
							break;
						case 1:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_FLATTENMAPSETTINGS);
							break;
						}
						break;
					case ID_PACKBUTTONS:
						switch (mMod->GetUIManager()->GetFlyOut(ID_PACKBUTTONS)){
						case 0:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_PACK_BUTTONS);
							break;
						case 1:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_PACKSETTINGS);
							break;
						}
						break;
					case ID_FALLOFF:
					case ID_BRUSH_FALLOFF_TYPE:
						switch(mMod->GetUIManager()->GetFlyOut(lpttt->hdr.idFrom)){
						case 0:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_FALLOFFLINEAR);
							break;
						case 1:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_FALLOFFSMOOTH);
							break;
						case 2:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_FALLOFFSLOWOUT);
							break;
						case 3:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_FALLOFFFASTOUT);
							break;
						}
						break;
					case ID_RELAX_MOVE_BRUSH:
						switch (mMod->GetUIManager()->GetFlyOut(lpttt->hdr.idFrom)) {
						case 0:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_RELAXBYPOLYGON);
							break;
						case 1:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_RELAXBYEDGE);
							break;
						case 2:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_RELAXBYCENTER);
							break;
						}
						break;
					case ID_FALLOFF_SPACE:
						switch(mMod->GetUIManager()->GetFlyOut(ID_FALLOFF_SPACE)){
						case 0:
							lpttt->lpszText = GetString(IDS_TOOLTIP_FALLOFF_OBJECTSPACE);
							break;
						case 1:
							lpttt->lpszText = GetString(IDS_TOOLTIP_FALLOFF_TEXTURESPACE);
							break;
						}
						break;
					case ID_HIDE:
						switch(mMod->GetUIManager()->GetFlyOut(ID_HIDE)){
						case 0:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_HIDE);
							break;
						case 1:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_UNHIDE);
							break;
						}
						break;
					case ID_FREEZE:
						switch(mMod->GetUIManager()->GetFlyOut(ID_FREEZE)){
						case 0:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_FREEZE);
							break;
						case 1:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_UNFREEZE);
							break;
						}
						break;
					case ID_ZOOMEXTENT:
						switch(mMod->GetUIManager()->GetFlyOut(ID_ZOOMEXTENT)){
						case 0:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_ZOOMTOALLTEXTURE);
							break;
						case 1:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_ZOOMTOCURRENTSEL);
							break;
						case 2:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_ZOOMTOELEMENTS);
							break;
						}
						break;
					case ID_SNAP:
						switch(mMod->GetUIManager()->GetFlyOut(ID_SNAP)){
						case 0:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_SNAPSTOGGLE);
							break;
						case 1:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_SNAPSETTING);
							break;
						}
						break;
					case ID_MOVE:
						switch(mMod->GetUIManager()->GetFlyOut(ID_MOVE)){
						case 0:
							lpttt->lpszText = GetString(IDS_TOOLTIP_MODE_MOVE);
							break;
						case 1:
							lpttt->lpszText = GetString(IDS_TOOLTIP_MODE_MOVEHORIZONTAL);
							break;
						case 2:
							lpttt->lpszText = GetString(IDS_TOOLTIP_MODE_MOVEVERTICAL);
							break;
						}
						break;
					case ID_SCALE:
						switch(mMod->GetUIManager()->GetFlyOut(ID_SCALE)){
						case 0:
							lpttt->lpszText = GetString(IDS_TOOLTIP_MODE_SCALE);
							break;
						case 1:
							lpttt->lpszText = GetString(IDS_TOOLTIP_MODE_SCALEHORIZONTAL);
							break;
						case 2:
							lpttt->lpszText = GetString(IDS_TOOLTIP_MODE_SCALEVERTICAL);
							break;
						}
						break;
					case ID_MIRROR:
						switch(mMod->GetUIManager()->GetFlyOut(ID_MIRROR)){
						case 0:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_MIRRORVERTICAL);
							break;
						case 1:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_MIRRORHORIZONTAL);
							break;
						case 2:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_FLIPHORIZONTAL);
							break;
						case 3:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_FLIPVERTICAL);
							break;
						}
						break;
					case ID_UVW:
						switch(mMod->GetUIManager()->GetFlyOut(ID_UVW))
						{
						case 0:
						case 1:
						case 2:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_UVWSPACE);
							break;
						}
						break;
					case ID_QUICKMAP_ALIGN:
						switch(mMod->GetUIManager()->GetFlyOut(ID_QUICKMAP_ALIGN)){
						case 0:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_QMAP_ALIGNX);
							break;
						case 1:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_QMAP_ALIGNY);
							break;
						case 2:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_QMAP_ALIGNZ);
							break;
						case 3:
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_QMAP_ALIGNNORMALS);
							break;
						}
						break;
					case ID_ABSOLUTETYPEIN:
						if(mMod->fnGetRelativeTypeInMode() == FALSE)
						{
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_ABSOLUTETYPEIN);
						}
						else
						{
							lpttt->lpszText = GetString(IDS_TOOLTIP_TOOL_OFFSETTYPEIN);
						}
						break;
					default:
						break;
					
					}
				}
			}
			break;
		case WM_COMMAND:
			{				
				iret = mMod->WtExecute(LOWORD(wParam),HIWORD(wParam),FALSE,TRUE);
			}

	}


	return iret;
}


int UnwrapCustomUI::LoadSingleFromIniFile(const MCHAR *iniFileName, int owner, HWND parentWindow, HWND toolbarHWND)
{
	//see if key exists if so load it
	bool done = false;
	int currentSection = 0;
	int ct = 0;
	bool found = false;
	TSTR section;
	if(mIniFileIO.GetIniFileName() != TSTR(iniFileName))
	{
		return ct;
	}
	while (!done)
	{
		
		section.printf(_T("Dialog_ToolBar_%d"),currentSection);
		int iValue = mIniFileIO.GetIntFromSectionKey(section, TSTR(_T("Owner")));
		if(iValue != 0)
		{
			if(iValue == owner)
			{
				done = true;
				found = true;
			}
			else
			{
				currentSection++;
			}
		}
		else
		{
			done = true;
		}
	}
	if (found)
	{
		ToolBarFrame *bar = new ToolBarFrame(mhInstance,section,mIniFileIO,parentWindow,toolbarHWND);
		if (bar->Loaded())
		{
			if ( bar->Owner() == owner ) 
			{
				mToolbars.Append(1,&bar,10);
				ct++;
			}
			else
				delete bar;
		}
		else
		{
			delete bar;
		}
	}
	return ct;
}

int UnwrapCustomUI::Load(const MCHAR *iniFileName, int owner, HWND parentWindow)
{
	//see if key exists if so load it
	bool done = false;
	int currentSection = 0;
	int a = 0;
	int b = 99;
	if (owner == 100)
	{
		a = 100;
		b = 199;
	}
	else if (owner == 200)
	{
		a = 200;
		b = 299;
	}
	
	int ct = 0;
	if(mIniFileIO.GetIniFileName() != TSTR(iniFileName))
	{
		return ct;
	}
	while (!done)
	{
		TSTR section;
		section.printf(_T("Dialog_ToolBar_%d"),currentSection);
		ToolBarFrame *bar = new ToolBarFrame(mhInstance,section,mIniFileIO,parentWindow,NULL);
		currentSection++;
		if (bar->Loaded())
		{

			if ( (bar->Owner() >= a) && (bar->Owner() <= b) ) 
			{
			mToolbars.Append(1,&bar,10);
				ct++;
			}
			else
				delete bar;
		}
		else
		{
			delete bar;
			done = true;
		}
	}
	return ct;
}
void UnwrapCustomUI::Save(const MCHAR *iniFileName)
{
	for (int i = 0; i < mToolbars.Count(); i++)
	{
		TSTR section;
		section.printf(_T("Dialog_ToolBar_%d"),i);
		mToolbars[i]->Save(section, mIniFileIO);
	}
	mIniFileIO.SaveAs(iniFileName);

}

int  UnwrapCustomUI::GetCornerHeight(int corner)
{
	if (corner >= 0 && corner < 4)
		return mCornerHeight[corner];
	return 0;
}

void  UnwrapCustomUI::DisplayFloaters(BOOL show)
{
	for (int i = 0; i < mToolbars.Count(); i++)
	{
		if (mToolbars[i]->GetParWindow() == mMod->hDialogWnd)
		{
			
			if (show)
			{								
				mToolbars[i]->Show(true);				
			}
			else
			{
				mToolbars[i]->Show(false);				
			}

		}
		
	}
}

IniFileIO& UnwrapCustomUI::GetIniFileCache()
{
	return mIniFileIO;
}
