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
// Filename    : ccl/platform/android/system/nativefilesystem.android.h
// Description : Android native file system
//
//************************************************************************************************

#ifndef _ccl_nativefilesystem_android_h
#define _ccl_nativefilesystem_android_h

#include "ccl/platform/shared/posix/system/nativefilesystem.posix.h"

namespace CCL {

//************************************************************************************************
// AndroidNativeFileSystem
//************************************************************************************************

class AndroidNativeFileSystem: public PosixNativeFileSystem
{
public:
	static AndroidNativeFileSystem& getInstance ();

	static String translateMode (int mode);

	IStream* createStreamFromHandle (int handle);

	// NativeFileSystem
	IStream* openPlatformStream (UrlRef url, int mode) override;

private:
	class AndroidFileStream;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline AndroidNativeFileSystem& AndroidNativeFileSystem::getInstance ()
{ return static_cast<AndroidNativeFileSystem&> (instance ()); }

} // namespace CCL

#endif // _ccl_nativefilesystem_android_h
