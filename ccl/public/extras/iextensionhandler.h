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
// Filename    : ccl/public/extras/iextensionhandler.h
// Description : Extension Handler Interface
//
//************************************************************************************************

#ifndef _ccl_iextensionhandler_h
#define _ccl_iextensionhandler_h

#include "ccl/public/base/iunknown.h"

namespace CCL {
interface IAttributeList;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Extension Definitions
//////////////////////////////////////////////////////////////////////////////////////////////////

#define PLUG_CATEGORY_EXTENSIONHANDLER	CCLSTR ("ExtensionHandler")

#if CCL_PLATFORM_MAC
	#define EXTENSION_PLATFORM_FOLDER "mac"
#elif CCL_PLATFORM_IOS
	#define EXTENSION_PLATFORM_FOLDER "ios"
#elif CCL_PLATFORM_WINDOWS
	#if CCL_PLATFORM_64BIT
		#if CCL_PLATFORM_ARM
			#define EXTENSION_PLATFORM_FOLDER "win_arm64"
		#elif CCL_PLATFORM_ARM64EC
			#define EXTENSION_PLATFORM_FOLDER "win_arm64ec"
		#else
			#define EXTENSION_PLATFORM_FOLDER "win_x64"
		#endif
	#else
		#if CCL_PLATFORM_ARM
			#define EXTENSION_PLATFORM_FOLDER "win_arm"
		#else
			#define EXTENSION_PLATFORM_FOLDER "win_x86"
		#endif
	#endif
#elif CCL_PLATFORM_LINUX
	#if CCL_PLATFORM_64BIT
		#if CCL_PLATFORM_ARM
			#define EXTENSION_PLATFORM_FOLDER "linux_arm64"
		#else
			#define EXTENSION_PLATFORM_FOLDER "linux_x64"
		#endif
	#else
		#if CCL_PLATFORM_ARM
			#define EXTENSION_PLATFORM_FOLDER "linux_arm"
		#else
			#define EXTENSION_PLATFORM_FOLDER "linux_x86"
		#endif
	#endif
#elif CCL_PLATFORM_ANDROID
	#define EXTENSION_PLATFORM_FOLDER "android"
#else
	#error unknown platform
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// Extension Manager Signals
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Signals
{
	/** Signals related to Extensions. */
	DEFINE_STRINGID (kExtensionManager, "CCL.ExtensionManager")

		/**	Extension has been installed. 
			arg[0]: IExtensionDescription */
		DEFINE_STRINGID (kExtensionInstalled, "ExtensionInstalled")
}

namespace Install {

//************************************************************************************************
// IExtensionDescription
//************************************************************************************************

interface IExtensionDescription: IUnknown
{
	virtual UrlRef CCL_API getPath () const = 0;

	virtual StringRef CCL_API getShortIdentifier () const = 0;

	virtual String CCL_API getPlatformIndependentIdentifier () const = 0;

	virtual void CCL_API getMetaInfo (IAttributeList& metaInfo) const = 0;

	DECLARE_IID (IExtensionDescription)
};

DEFINE_IID (IExtensionDescription, 0xe793a7ac, 0x66ad, 0x4039, 0x98, 0xc, 0x40, 0x65, 0xe8, 0x5c, 0x93, 0x60)

//************************************************************************************************
// IExtensionHandler
//************************************************************************************************

interface IExtensionHandler: IUnknown
{
	virtual int CCL_API startupExtension (IExtensionDescription& description) = 0;

	//TODO:
	//virtual void CCL_API shutdownExtension (IExtensionDescription& description) = 0;

	DECLARE_IID (IExtensionHandler)
};

DEFINE_IID (IExtensionHandler, 0x7f6be900, 0x9507, 0x4baa, 0xaf, 0x84, 0x7d, 0x12, 0xc0, 0xf4, 0xfa, 0x9e)

//************************************************************************************************
// IExtensionCompatibilityHandler
/** Optional interface for extension handlers to participate in early compatibility check. */
//************************************************************************************************

interface IExtensionCompatibilityHandler: IUnknown
{
	virtual tresult CCL_API checkCompatibility (IExtensionDescription& description) = 0;

	DECLARE_IID (IExtensionCompatibilityHandler)
};

DEFINE_IID (IExtensionCompatibilityHandler, 0x6a0c9169, 0x724b, 0x48a8, 0xbc, 0x97, 0xda, 0xb5, 0x18, 0xb4, 0xac, 0x9e)

} // namespace Install
} // namespace CCL

#endif // _ccl_iextensionhandler_h
