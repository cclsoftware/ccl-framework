//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/wasm/system/mediathreadservice.wasm.cpp
// Description : WebAssembly Multimedia Threading Services
//
//************************************************************************************************

#include "ccl/system/threading/mediathreadservice.h"
#include "ccl/system/threading/thread.h"

namespace CCL {

//************************************************************************************************
// WasmMediaThreadService
//************************************************************************************************

class WasmMediaThreadService: public MediaThreadService
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
	static WasmMediaThreadService theMediaThreadService;
	return theMediaThreadService;
}

//************************************************************************************************
// WasmMediaThreadService
//************************************************************************************************

tresult CCL_API WasmMediaThreadService::startup ()
{
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WasmMediaThreadService::shutdown ()
{
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API WasmMediaThreadService::getMediaTime ()
{
	return System::GetProfileTime ();
}
