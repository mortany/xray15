/**********************************************************************
 *<
	FILE: rendvue.h

	DESCRIPTION: .VUE file renderer class definition

	CREATED BY: Dan Silva

	HISTORY: created 10/8/96

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef RENDVUE__H
#define RENDVUE__H
#include "maxtextfile.h"

#define VREND_CLASS_ID Class_ID(5,0);

class VueRenderer : public Renderer {
	public:
		TSTR vueFileName;
		MaxSDK::Util::TextFile::Writer *vuefile;
		INode *sceneNode;
		INode *viewNode;
		ViewParams viewParams; // view params for rendering ortho or user viewport
		RendParams RP;  	// common renderer parameters
		BOOL anyLights;
		TCHAR buffer[256];
		int nlts,nobs;
		VueRenderer() { vuefile = NULL; sceneNode = NULL; viewNode = NULL; anyLights = FALSE; nlts = nobs = 0; }
		int Open(
			INode *scene,     	// root node of scene to render
			INode *vnode,     	// view node (camera or light), or NULL
			ViewParams *viewPar,// view params for rendering ortho or user viewport
			RendParams &rp,  	// common renderer parameters
			HWND hwnd, 				// owner window, for messages
			DefaultLight* defaultLights=NULL, // Array of default lights if none in scene
			int numDefLights=0,	// number of lights in defaultLights array
			RendProgressCallback* prog = NULL
			);
		int Render(
			TimeValue t,   			// frame to render.
   			Bitmap *tobm, 			// optional target bitmap
			FrameRendParams &frp,	// Time dependent parameters
			HWND hwnd, 				// owner window
			RendProgressCallback *prog=NULL,
			ViewParams *vp=NULL
			);
		void Close(	HWND hwnd, RendProgressCallback* prog = NULL );		
		RefTargetHandle Clone(RemapDir &remap);

		// Adds rollup page(s) to renderer configure dialog
		// If prog==TRUE then the rollup page should just display the parameters
		// so the user has them for reference while rendering, they should not be editable.
		RendParamDlg *CreateParamDialog(IRendParams *ir,BOOL prog=FALSE);
		void ResetParams();
		void DeleteThis() { delete this;  }
		Class_ID ClassID() { return VREND_CLASS_ID;}
		void GetClassName(TSTR& s) {s = GetString(IDS_VRENDTITLE);}
		// IO
		IOResult Save(ISave *isave);
		IOResult Load(ILoad *iload);
		void WriteViewNode(INode* vnode, TimeValue t);
		void WriteViewParams(ViewParams *v);

		virtual RefResult NotifyRefChanged (
			const Interval		&changeInt, 
			RefTargetHandle		 hTarget, 
			PartID				&partID,  
			RefMessage			 message, 
			BOOL				 propagate
			);

        virtual void StopRendering() override;
        virtual bool IsStopSupported() const override;
        virtual PauseSupport IsPauseSupported() const override;
        virtual void PauseRendering() override;
        virtual void ResumeRendering() override;
        virtual bool HasRequirement(Requirement requirement) override;
        virtual IInteractiveRender* GetIInteractiveRender() override;
        virtual void GetVendorInformation(MSTR& info) const override;
        virtual void GetPlatformInformation(MSTR& info) const override;
        virtual bool CompatibleWithAnyRenderElement() const override;
        virtual bool CompatibleWithRenderElement(IRenderElement& pIRenderElement) const override;

	};



#endif