//
// Copyright 2017 Autodesk, Inc.  All rights reserved. 
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc. and
// are protected under applicable copyright and trade secret law.  They may
// not be disclosed to, copied or used by any third party without the prior
// written consent of Autodesk, Inc.
//
//

#include "activeshadefragment.h"
#include <iparamb2.h>
#include <Graphics/ViewSystem/EvaluationContext.h>
#include "Resource.h"
#include "gamma.h"
#include <Rendering/IActiveShadeFragmentManager.h>

extern HINSTANCE hInstance;

using namespace MaxSDK::Graphics;

#define ACTIVE_SHADE_FRAGMENT_CLASS_ID Class_ID(0x12c31f85, 0xe40a704e)

class ActiveShadeFragmentDesc:public ClassDesc2
{
public:
	int 			IsPublic() {return TRUE;}
	void *			Create(BOOL loading)
	{
		loading;
		return new ::MaxGraphics::ActiveShadeFragment;
	}
	const MCHAR *	ClassName() { return _M("ActiveShadeFragment"); }
	SClass_ID		SuperClassID() { return Fragment_CLASS_ID; }
	Class_ID		ClassID() { return ACTIVE_SHADE_FRAGMENT_CLASS_ID;}
	const MCHAR* 	Category() { return _M("Fragment"); }

	const TCHAR*	InternalName() { return _T("ActiveShadeFragment"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }				// returns owning module handle
	bool			UseOnlyInternalNameForMAXScriptExposure() { return true; }
};
static ActiveShadeFragmentDesc oneActiveShadeFragmentDesc;
ClassDesc2* GetActiveShadeFragmentDesc() { return &oneActiveShadeFragmentDesc; }

namespace MaxGraphics 
{
	enum ActiveShadeFragmentInput
	{
		ActiveShadeFragmentInput_ColorTarget, 
		ActiveShadeFragmentInput_Count, 
	};

	enum ActiveShadeFragmentOutput
	{
		ActiveShadeFragmentOutput_ColorTarget, 
		ActiveShadeFragmentOutput_Count, 
	};

	ActiveShadeFragment::ActiveShadeFragment()
	{
		mpBitmap = NULL;
		mpRenderer = NULL;
		mIRenderInterface = NULL;

		mHwnd = NULL;
		mpViewExp = NULL;
		memset(mDefaultLights, 0, sizeof(mDefaultLights));
		mNumDefaultLights = 0;

		mBitmapWidth				= 1;
		mBitmapHeight				= 1;

		Class_ID inputIDs[ActiveShadeFragmentInput_Count];
		inputIDs[ActiveShadeFragmentInput_ColorTarget] = TargetHandle::ClassID();
		InitializeInputs(ActiveShadeFragmentInput_Count, inputIDs);

		Class_ID outputIDs[ActiveShadeFragmentOutput_Count];
		outputIDs[ActiveShadeFragmentOutput_ColorTarget] = TargetHandle::ClassID();
		InitializeOutputs(ActiveShadeFragmentOutput_Count, outputIDs);
	}

	ActiveShadeFragment::~ActiveShadeFragment()
	{
		Cleanup();
	}

	inline int clamp(int x, int a, int b)
	{
		return (x < a) ? a : ((x > b) ? b : x);
	}


	bool ActiveShadeFragment::DoEvaluate(EvaluationContext* evaluationContext)
	{
		if(NULL == evaluationContext)
		{
			return false;
		}
		ViewParameter* pViewportParameter = evaluationContext->pViewParameter.GetPointer();

		if(pViewportParameter->IsRenderRegionVisible())
		{
			mBitmapWidth	= size_t(pViewportParameter->GetWidth() *pViewportParameter->GetRenderRegion().mScreenSpaceScale.x);
			mBitmapHeight	= size_t(pViewportParameter->GetHeight()*pViewportParameter->GetRenderRegion().mScreenSpaceScale.y);

			const FBox2& ClippedSourceRegion = pViewportParameter->GetClippedSourceRegion();

			mRenderRegion.left = clamp(int(ClippedSourceRegion.pmin.x), 0, int(mBitmapWidth - 1));
			mRenderRegion.top = clamp(int(ClippedSourceRegion.pmin.y), 0, int(mBitmapHeight - 1));
			mRenderRegion.right = clamp(int(ClippedSourceRegion.pmax.x), 0, int(mBitmapWidth - 1));
			mRenderRegion.bottom = clamp(int(ClippedSourceRegion.pmax.y), 0, int(mBitmapHeight - 1));

			TargetHandle targetHandle;
			if (!GetSmartHandleInput(targetHandle,ActiveShadeFragmentInput_ColorTarget) ||
				!targetHandle.IsValid())
			{
				return false;
			}

			if(SynchronizeToTarget(targetHandle,evaluationContext))
			{
				return SetSmartHandleOutput(targetHandle,ActiveShadeFragmentOutput_ColorTarget);
			}
		}

		return false;	
	}

	bool ActiveShadeFragment::SynchronizeToTarget(BaseRasterHandle& targetHandle,
		EvaluationContext* evaluationContext)
	{
		if (NULL == mIRenderInterface) //Not yet taken the mIRenderInterface so call startup
		{
			Startup(evaluationContext, mBitmapWidth, mBitmapHeight);
		}

		if (NULL == mIRenderInterface)
		{
            DbgAssert(0);
			return false;
		}

		if (mpBitmap == NULL || 
			mpBitmap->Width() != mBitmapWidth || 
			mpBitmap->Height() != mBitmapHeight)
		{
			Cleanup();
			Startup(evaluationContext, mBitmapWidth, mBitmapHeight);
		}

		if (targetHandle.GetWidth() != mBitmapWidth ||
			targetHandle.GetHeight() != mBitmapHeight)
		{
			targetHandle.SetSize(mBitmapWidth,mBitmapHeight);
		}
		if(targetHandle.GetFormat() != TargetFormatA8R8G8B8)
		{
			targetHandle.SetFormat(TargetFormatA8R8G8B8);
		}

        //This flag is updated when Bitmap::PutPixels is called, so if it was the case since last call we need to update the raster, if not, then skip this
        if (0 == (mpBitmap->Flags() & MAP_WAS_UPDATED)) {
            return true;//The Bitmap has not received any updates so ignore the copy into the raster
        }

		mLineColors.resize(mBitmapWidth);
		BMM_Color_64* mLineColorsPtr = &mLineColors[0];
		int nLineWidth = mRenderRegion.w();

		// update color target!
		size_t destRowPitch = 0, destSlicePitch = 0;
		byte* pDestData = (byte*)targetHandle.Map(WriteAcess,destRowPitch,destSlicePitch);
		pDestData += destRowPitch * mRenderRegion.top;

		for (int y = mRenderRegion.top; y <= mRenderRegion.bottom; ++y)
		{
			mpBitmap->GetPixels(mRenderRegion.left, y, nLineWidth, mLineColorsPtr + mRenderRegion.left);
			for (int x = mRenderRegion.left; x <= mRenderRegion.right; ++x)
			{
				BMM_Color_64 LineColor = mLineColorsPtr[x];

				DWORD color = 
					(((DWORD)(LineColor.a >> 8)) << 24) |
					(((DWORD)(GAMMA16to8(LineColor.r)))) |
					(((DWORD)(GAMMA16to8(LineColor.g))) << 8) |
					(((DWORD)(GAMMA16to8(LineColor.b)))<< 16);
				*((DWORD*)pDestData + x) = GetDeviceCompatibleARGBColor(color);
			}
			pDestData += destRowPitch;
		}
        
        //Set the bitmap is up to date
        mpBitmap->ClearFlag(MAP_WAS_UPDATED);

		targetHandle.UnMap();

		return true;
	}

	void ActiveShadeFragment::Startup(EvaluationContext* evaluationContext, size_t width, size_t height)
	{
        Interface* ip = GetCOREInterface();
        if (nullptr == ip){ 
            return; 
        }

        Renderer* pRenderer = mpRenderer;
        if (nullptr == pRenderer) {
            //No renderer has been set so use the one from the render settings
            pRenderer = ip->GetRenderer(RS_IReshade);
        }
            
        mpRenderer = (pRenderer) ? (Renderer*)CloneRefHierarchy(pRenderer) : nullptr;
        if (nullptr == mpRenderer){
            DbgAssert(0 && _T("nullptr == mpRenderer"));
            return;
        }

		mIRenderInterface = mpRenderer->GetIInteractiveRender();
        if (nullptr == mIRenderInterface) {
            DbgAssert(0 && _T("Renderer set in fragment is not compatible with active shade"));
            return; //not an active shade renderer
        }

		// create the bitmap
		BitmapInfo bi;
		bi.SetWidth((int)width);
		bi.SetHeight((int)height);
		bi.SetType(BMM_TRUE_64); // 64-bit color: 16 bits each for Red, Green, Blue, and Alpha.
		bi.SetFlags(MAP_HAS_ALPHA);
		bi.SetAspect(1.0f);
		mpBitmap = TheManager->Create(&bi);

		ViewParameter* pViewportParameter = evaluationContext->pViewParameter.GetPointer();
		
		mpViewExp = pViewportParameter->GetViewExp();
		mHwnd = mpViewExp->GetHWnd();		
        
        //Add default lights to Active shade
		mNumDefaultLights = GetCOREInterface7()->InitDefaultLights(mDefaultLights, 2, FALSE, mpViewExp, TRUE);
        mIRenderInterface->SetDefaultLights(mDefaultLights, mNumDefaultLights);

		mIRenderInterface->SetOwnerWnd( mpViewExp->GetHWnd() );
		mIRenderInterface->SetIIRenderMgr(this);
		mIRenderInterface->SetBitmap( mpBitmap );
		mIRenderInterface->SetSceneINode( ip->GetRootNode() );
        mIRenderInterface->SetViewExp(mpViewExp);

        //Set the active shade manager as the Progress Callback for this active shade fragment
        MaxSDK::IActiveShadeFragmentManager* pActiveShadeFragmentManager = static_cast<MaxSDK::IActiveShadeFragmentManager*>(GetCOREInterface(IACTIVE_SHADE_VIEWPORT_MANAGER_INTERFACE));
        if (nullptr != pActiveShadeFragmentManager) {
            IRenderProgressCallback * pProgressCallback = pActiveShadeFragmentManager->GetActiveShadeFragmentProgressCallback(this);
            DbgAssert(pProgressCallback);
            mIRenderInterface->SetProgressCallback(pProgressCallback);
        }

		mIRenderInterface->SetRegion(mRenderRegion);
		mIRenderInterface->BeginSession();
	}

	void ActiveShadeFragment::Cleanup()
	{
        if( mIRenderInterface )
		{
			mIRenderInterface->EndSession();
		}

        MaxSDK::IActiveShadeFragmentManager* pActiveShadeFragmentManager = static_cast<MaxSDK::IActiveShadeFragmentManager*>(GetCOREInterface(IACTIVE_SHADE_VIEWPORT_MANAGER_INTERFACE));
        if (nullptr != pActiveShadeFragmentManager) {
            pActiveShadeFragmentManager->RemoveActiveShadeFragmentProgressCallback(this);
        }


		if (NULL != mpRenderer)
		{
			if (NULL != mIRenderInterface)
			{
				// we're done with the reshading interface
				mIRenderInterface = NULL;
			}
			mpRenderer->MaybeAutoDelete();
			mpRenderer = NULL;
		}

		if (NULL != mpBitmap)
		{
			mpBitmap->DeleteThis();
			mpBitmap = NULL;
		}
	}

	Class_ID ActiveShadeFragment::GetClassID() const
	{
		return ACTIVE_SHADE_FRAGMENT_CLASS_ID;
	}
    void ActiveShadeFragment::UpdateDisplay()
    {
        if (IsRendering())
            SetFlag(FragmentFlagsDirty, true);
    }

    void ActiveShadeFragment::SetActiveShadeRenderer(Renderer* pRenderer)
    {
        DbgAssert(pRenderer);
        mpRenderer = pRenderer;
    }

    Renderer* ActiveShadeFragment::GetActiveShadeRenderer(void)const
    {
        return mpRenderer;
    }
}


