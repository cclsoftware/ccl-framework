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
// Filename    : core/portable/corecomponentflags.h
// Description : Component Flags
//
//************************************************************************************************

#ifndef _corecomponentflags_h
#define _corecomponentflags_h

#include "core/public/coreplatform.h"

namespace Core {
namespace Portable {

//************************************************************************************************
// ComponentFlags
//************************************************************************************************

namespace ComponentFlags
{
	enum
	{
		kMutable = 1<<0,
		kSaveDisabled = 1<<1,
		kOwnedByArray = 1<<2
	};
}

} // namespace Portable
} // namespace Core

#endif // _corecomponentflags_h
