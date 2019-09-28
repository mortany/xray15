//
// Copyright 2018 Autodesk, Inc.  All rights reserved. 
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc. and
// are protected under applicable copyright and trade secret law.  They may
// not be disclosed to, copied or used by any third party without the prior
// written consent of Autodesk, Inc.
//
//

#pragma once

#include <Rendering/Renderer.h>

namespace MaxGraphics 
{
	class IActiveShadeFragment		
	{
	public:
		virtual ~IActiveShadeFragment() {};

		//Set the active shade renderer instance to use for running active shade, this has to be an active shade compatible renderer
        //If none is set or the one set is not compatible with active shade, we use the active shade renderer from the render settings
        virtual void SetActiveShadeRenderer(Renderer* pRenderer) = 0;
        virtual Renderer* GetActiveShadeRenderer(void)const = 0;
	};
}