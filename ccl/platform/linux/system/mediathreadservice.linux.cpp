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
// Filename    : ccl/platform/linux/system/mediathreadservice.linux.cpp
// Description : Linux Multimedia Threading Services
//
//************************************************************************************************

#define DEBUG_LOG 1

#include "ccl/system/threading/mediathreadservice.h"
#include "ccl/system/threading/thread.h"

namespace CCL {

//************************************************************************************************
// LinuxMediaThreadService
//************************************************************************************************

class LinuxMediaThreadService: public MediaThreadService
{
public:
	// IMediaThreadService
	tresult CCL_API startup () override;
	tresult CCL_API shutdown () override;
	double CCL_API getMediaTime () override;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Threading APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IMediaThreadService& CCL_API System::CCL_ISOLATED (GetMediaThreadService) ()
{
	static LinuxMediaThreadService theMediaThreadService;
	return theMediaThreadService;
}

//************************************************************************************************
// LinuxMediaThreadService
//************************************************************************************************

tresult CCL_API LinuxMediaThreadService::startup ()
{
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinuxMediaThreadService::shutdown ()
{
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API LinuxMediaThreadService::getMediaTime ()
{
	return System::GetProfileTime ();
}
