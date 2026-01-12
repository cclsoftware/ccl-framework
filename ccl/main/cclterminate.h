//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2025 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/main/cclterminate.h
// Description : Termination Sequence
//
//************************************************************************************************

#ifndef _ccl_terminate_h
#define _ccl_terminate_h

#include "ccl/public/system/ifilemanager.h"
#include "ccl/public/system/ithreadpool.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/systemservices.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// ccl_terminate
//////////////////////////////////////////////////////////////////////////////////////////////////

inline void ccl_terminate ()
{
	System::GetSystem ().terminate ();
	System::GetPlugInManager ().terminate ();
	System::GetFileManager ().terminate ();
	System::GetThreadPool ().terminate ();
}

} // namesopace CCL

#endif // _ccl_terminate_h
