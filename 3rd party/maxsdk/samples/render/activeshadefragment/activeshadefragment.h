//
// Copyright 2014 Autodesk, Inc.  All rights reserved. 
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc. and
// are protected under applicable copyright and trade secret law.  They may
// not be disclosed to, copied or used by any third party without the prior
// written consent of Autodesk, Inc.
//
//

#pragma once

#include <Graphics/FragmentGraph/ViewFragment.h>
#include <Graphics/ViewSystem/EvaluationContext.h>
#include <Graphics/BaseRasterHandle.h>
#include <Graphics/TargetHandle.h>
#include <interactiverender.h>
#include <Rendering/Renderer.h>
#include <vector>
#include "IActiveshadeFragment.h"

namespace MaxGraphics 
{
	// warning C4100: 'active' : unreferenced formal parameter
#pragma warning(disable:4100)
	class ActiveShadeFragment : 
		public MaxSDK::Graphics::ViewFragment, 
		public IIRenderMgr, 
		public IIRenderMgrSelector,
        public IActiveShadeFragment
	{
	public:
		ActiveShadeFragment();
		virtual ~ActiveShadeFragment();

		virtual bool DoEvaluate(MaxSDK::Graphics::EvaluationContext* evaluationContext);
		virtual Class_ID GetClassID() const;

        //From IActiveShadeFragment
        //Set the active shade renderer instance to use for running active shade, this has to be an active shade compatible renderer
        //If none is set or the one set is not compatible with active shade, we use the active shade renderer from the render settings
        virtual void SetActiveShadeRenderer(Renderer* pRenderer) override;
        virtual Renderer* GetActiveShadeRenderer(void)const override;

	protected:
		// from IIRenderMgr
		virtual bool CanExecute()
		{
			return mIRenderInterface != NULL;
		}
		virtual void SetActive(bool active)
		{
			return;
		}
		virtual MCHAR* GetName()
		{
			return _M("");
		}
		virtual bool IsActive()
		{
			return true;
		}
		virtual HWND GetHWnd() const
		{
			return mHwnd;
		}
		virtual ViewExp *GetViewExp()
		{
			return mpViewExp;
		}
		virtual void SetPos(int X, int Y, int W, int H)
		{
			// do nothing.
		}
		virtual void Show()
		{
			// do nothing.
		}
		virtual void Hide()
		{
			// do nothing.
		}
        virtual void UpdateDisplay();

		virtual void Render()
		{
			// do nothing.
		}
		virtual void SetDelayTime(int msecDelay)
		{
			// do nothing.
		}
		virtual int GetDelayTime()
		{
			return 100;
		}
		virtual void Close()
		{
			// do nothing.
		}
		virtual void Delete()
		{
			// do nothing.
		}
		virtual void SetCommandMode(IIRenderMgr::CommandMode commandMode)
		{
			// do nothing.
		}
		virtual IIRenderMgr::CommandMode GetCommandMode() const
		{
			return CMD_MODE_NULL;
		}
		virtual void SetActOnlyOnMouseUp(bool actOnlyOnMouseUp)
		{
			// do nothing.
		}
		virtual bool GetActOnlyOnMouseUp() const
		{
			return true;
		}
		virtual void ToggleToolbar() const
		{
			// do nothing.
		}
		virtual IImageViewer::DisplayStyle GetDisplayStyle() const
		{
			return IImageViewer::IV_DOCKED;
		}
		virtual BOOL IsRendering()
		{
			if(mIRenderInterface == NULL)
			{
				return FALSE;
			}
			return mIRenderInterface->IsRendering(); 
		}
		virtual BOOL AreAnyNodesSelected() const
		{
			return FALSE;
		}
		virtual IIRenderMgrSelector* GetNodeSelector()
		{
			return this;
		}
		virtual BOOL AnyUpdatesPending()
		{
			return TRUE;
		}

	protected:
		// from IIRenderMgrSelector
		virtual BOOL IsSelected(INode* pINode)
		{
			UNUSED_PARAM(pINode);
			return FALSE;
		}

	private:
		void Startup(MaxSDK::Graphics::EvaluationContext* evaluationContext, size_t width, size_t height);
		void Cleanup();
       
	private:
		Bitmap* mpBitmap                        = nullptr;
		Renderer* mpRenderer                    = nullptr;
		IInteractiveRender* mIRenderInterface   = nullptr;

		HWND mHwnd                              = NULL;
		ViewExp* mpViewExp                      = nullptr;;

		DefaultLight mDefaultLights[2];
		int mNumDefaultLights                   = 0;

		std::vector<BMM_Color_64> mLineColors;

		bool SynchronizeToTarget(MaxSDK::Graphics::BaseRasterHandle& targetHandle, MaxSDK::Graphics::EvaluationContext* evaluationContext);

		size_t			mBitmapWidth            = 0;
		size_t			mBitmapHeight           = 0;
		Box2			mRenderRegion;
	};
#pragma warning(default:4100)
}