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
// Filename    : ccl/platform/cocoa/interfaces/icocoalocalemanager.h
// Description : Locale Manager (macOS) internal interface
//
//************************************************************************************************

#ifndef _ccl_ilocalemanager_cocoa_h
#define _ccl_ilocalemanager_cocoa_h

#include "ccl/public/base/iunknown.h"

namespace CCL {
namespace MacOS {

//************************************************************************************************
// ICocoaLocaleManager
/** Additional interface implemented by the cocoa locale manager.
	\ingroup system */
//************************************************************************************************

interface ICocoaLocaleManager: IUnknown
{
	/** Get system key code (kVK_ANSI_ ...) for an alphanumeric character in the current input language. */
	virtual tbool CCL_API getSystemKeyForCharacter (uint16& sysKey, uchar characterLocal) const = 0;

	DECLARE_IID (ICocoaLocaleManager)
};

DEFINE_IID_ (ICocoaLocaleManager, 0x8D2C6B50, 0x8F3D, 0x4171, 0x9B, 0x1B, 0x4C, 0x9F, 0xAB, 0xE9, 0x59, 0xBC)

} // namespace MacOS
} // namespace CCL

#endif // _ccl_ilocalemanager_cocoa_h
