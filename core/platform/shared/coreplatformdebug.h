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
// Filename    : core/platform/shared/coreplatformdebug.h
// Description : Debug Functions platform implementation base
//
//************************************************************************************************

#ifndef _coreplatformdebug_h
#define _coreplatformdebug_h

#include "core/public/coretypes.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// Debug Functions
//************************************************************************************************

namespace Debug
{
	void print (CStringPtr string);
}

} // namespace Platform
} // namespace Core

#endif // _coreplatformdebug_h
