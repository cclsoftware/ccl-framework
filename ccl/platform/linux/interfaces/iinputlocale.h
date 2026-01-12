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
// Filename    : ccl/platform/linux/system/iinputlocale.h
// Description : Platform-specific interface
//
//************************************************************************************************

#ifndef _ccl_linux_iinputlocale_h
#define _ccl_linux_iinputlocale_h

#include "ccl/public/base/iunknown.h"

struct xkb_keymap;

namespace CCL {
namespace Linux {

//************************************************************************************************
// IInputLocale
/** Interface to LinuxLocaleManager (used by cclgui). */
//************************************************************************************************

interface IInputLocale: IUnknown
{
	virtual void CCL_API setKeyMap (xkb_keymap* keyMap) = 0;
	
	virtual CStringPtr CCL_API getInputLocale () const = 0;

	DECLARE_IID (IInputLocale)
};

DEFINE_IID (IInputLocale, 0x99180c26, 0x4af3, 0x4b6f, 0xaa, 0x26, 0x15, 0x06, 0xce, 0x1c, 0xfd, 0x0f)

} // namespace Linux
} // namespace CCL

#endif // _ccl_linux_iinputlocale_h
