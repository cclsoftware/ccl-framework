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
// Filename    : ccl/platform/win/storeversion.h
// Description : Utility functions for Microsoft Store apps
//
//************************************************************************************************

#ifndef _ccl_storeversion_h
#define _ccl_storeversion_h

namespace CCL {
namespace Win32 {

inline String MakeMicrosoftStoreVersion (int major, int minor, int build)
{
	return String ("Microsoft Store Build: ") << major << "." << minor << "." << build << ".0";
}

} // namespace Win32
} // namespace CCL

#endif // _ccl_storeversion_h
