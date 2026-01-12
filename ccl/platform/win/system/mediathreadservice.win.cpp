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
// Filename    : ccl/platform/win/system/mediathreadservice.win.cpp
// Description : Windows Multimedia Threading Services
//
//************************************************************************************************

#define DEBUG_LOG 1

#include "ccl/system/threading/mediathreadservice.h"
#include "ccl/system/threading/thread.h"

#include "ccl/platform/win/cclwindows.h"

namespace CCL {

//************************************************************************************************
// WindowsMediaThreadService
//************************************************************************************************

class WindowsMediaThreadService: public MediaThreadService
{
public:
	// IMediaThreadService
	tresult CCL_API startup () override;
	tresult CCL_API shutdown () override;
	double CCL_API getMediaTime () override;

private:
	bool schedulingChanged = false;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Threading APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IMediaThreadService& CCL_API System::CCL_ISOLATED (GetMediaThreadService) ()
{
	static WindowsMediaThreadService theMediaThreadService;
	return theMediaThreadService;
}

//************************************************************************************************
// WindowsMediaThreadService
//************************************************************************************************

tresult CCL_API WindowsMediaThreadService::startup ()
{
	schedulingChanged = Core::Platform::Win32Thread::enableHighResolutionScheduling (true);
	ASSERT (schedulingChanged == true)

	SIZE_T minimumWorkingSetSize = 0;
	SIZE_T maximumWorkingSetSize = 0;
	BOOL result = ::GetProcessWorkingSetSize (::GetCurrentProcess(), &minimumWorkingSetSize, &maximumWorkingSetSize);

	minimumWorkingSetSize = 512 * 1024 * 1024;
	maximumWorkingSetSize = 1024 * 1024 * 1024;
	result = ::SetProcessWorkingSetSize (::GetCurrentProcess(), minimumWorkingSetSize, maximumWorkingSetSize);

	::SetProcessPriorityBoost (::GetCurrentProcess(), TRUE);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WindowsMediaThreadService::shutdown ()
{
	if(schedulingChanged)
		Core::Platform::Win32Thread::enableHighResolutionScheduling (false);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API WindowsMediaThreadService::getMediaTime ()
{
	return (double)::timeGetTime () / 1000.;
}
