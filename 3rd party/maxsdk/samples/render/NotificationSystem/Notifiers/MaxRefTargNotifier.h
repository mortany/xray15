//
// Copyright 2013 Autodesk, Inc.  All rights reserved. 
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc. and
// are protected under applicable copyright and trade secret law.  They may
// not be disclosed to, copied or used by any third party without the prior
// written consent of Autodesk, Inc.
//
//

#pragma once

#include "Notifiers/MaxNotifier.h"

namespace Max{

namespace NotificationAPI{

	class MaxRefTargNotifier : public MaxNotifier
	{
	public:
		explicit MaxRefTargNotifier(ReferenceTarget& RefTarg);

		ReferenceTarget* GetMaxRefTarg() const;

        //From MaxNotifier
        virtual void DebugPrintToFile(FILE* f, size_t indent)const;

        //-- From ReferenceMaker
		virtual RefResult NotifyRefChanged(
			const Interval& changeInt, 
			RefTargetHandle hTarget, 
			PartID& partID, 
			RefMessage message,
			BOOL propagate );
		virtual int NumRefs();
		virtual RefTargetHandle GetReference(int i);

		//-- From Animatable
		virtual void GetClassName(MSTR& s) { s = _M("Max::NotificationAPI::MaxRefTargNotifier"); }


    protected:

        // Protected destructor forces going through delete_this()
        virtual ~MaxRefTargNotifier();

	private:
		ReferenceTarget* m_refTarg;
        //from ReferenceMaker
        virtual void SetReference(int , RefTargetHandle rtarg);
	};

} } // namespaces
