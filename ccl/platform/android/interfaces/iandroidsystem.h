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
// Filename    : ccl/platform/android/interfaces/iandroidsystem.h
// Description : interface to cclsystem framework
//
//************************************************************************************************

#ifndef _ccl_android_iandroidsystem_h
#define _ccl_android_iandroidsystem_h

#include "ccl/public/base/iunknown.h"

namespace CCL {
namespace Android {

interface IFrameworkActivity;

//************************************************************************************************
// IAndroidSystem
/** Interface to AndroidSystemInformation (used by cclgui). */
//************************************************************************************************

interface IAndroidSystem: IUnknown
{
	virtual void CCL_API setNativeActivity (IFrameworkActivity* activity) = 0;
	
	virtual IFrameworkActivity* CCL_API getNativeActivity () const = 0;

	virtual int CCL_API callAndroidMain (tbool startup) = 0;

	virtual void CCL_API onConfigurationChanged () = 0;

	DECLARE_IID (IAndroidSystem)
};

DEFINE_IID (IAndroidSystem, 0x40ca295b, 0x8b42, 0x4d1d, 0x9d, 0xc1, 0x87, 0x47, 0xa7, 0xec, 0xd5, 0xd8)

} // namespace Android
} // namespace CCL

#endif // _ccl_android_iandroidsystem_h
