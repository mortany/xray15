//**************************************************************************/
// Copyright 2018 Autodesk, Inc.  All rights reserved. 
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc. and
// are protected under applicable copyright and trade secret law.  They may
// not be disclosed to, copied or used by any third party without the prior
// written consent of Autodesk, Inc.
//**************************************************************************/
#pragma once

#include <stdio.h>  // DllMain.cpp --> maxscript/maxscript.h --> stdio.h

// Encapsulates further nested banned.h errors in standard headers
#include <ios> // maxscript/maxscript.h --> kernel/exceptions.h -> strclass.h --> maxstring.h --> <ios>

#include <3dsmax_banned.h>
