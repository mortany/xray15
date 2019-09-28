//
// Copyright [2014] Autodesk, Inc.  All rights reserved. 
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc. and
// are protected under applicable copyright and trade secret law.  They may
// not be disclosed to, copied or used by any third party without the prior
// written consent of Autodesk, Inc.
//

#ifndef __SIMPWAVE__H
#define __SIMPWAVE__H

#define FLOATWAVE_CONTROL_CLASS_ID  0x930Abc78

class BaseWaveControlInternal :public LockableStdControl {
public:
	virtual int		GetWaveformsCount() {return 0;}
	virtual void	SetWaveformsCount(int iCount){}
};

#endif
