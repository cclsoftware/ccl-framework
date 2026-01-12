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
// Filename    : ccl/base/storage/xmlpihandler.cpp
// Description : XML Processing Instruction Handler
//
//************************************************************************************************

#include "ccl/base/storage/xmlpihandler.h"
#include "ccl/base/storage/configuration.h"

#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/cclversion.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// XML Processing Instructions
//////////////////////////////////////////////////////////////////////////////////////////////////

#define PI_NESTING_PREFIX			"nested:"
#define PI_INVERT_PREFIX			"not:"

#define PI_TARGET_PLATFORM			"platform"

#define PI_TARGET_PLATFORMARCH		"platform_arch"
#define PI_PLATFORMARCH_WIN_X86		"win_x86"
#define PI_PLATFORMARCH_WIN_X64		"win_x64"
#define PI_PLATFORMARCH_WIN_ARM64	"win_arm64"
#define PI_PLATFORMARCH_MAC_X64		"mac_x64"
#define PI_PLATFORMARCH_MAC_ARM64	"mac_arm64"
#define PI_PLATFORMARCH_IOS_ARM64	"ios_arm64"
#define PI_PLATFORMARCH_ANDR_X86	"android_x86"
#define PI_PLATFORMARCH_ANDR_X64	"android_x64"
#define PI_PLATFORMARCH_ANDR_ARM	"android_arm"
#define PI_PLATFORMARCH_ANDR_ARM64	"android_arm64"
#define PI_PLATFORMARCH_LINUX_X86	"linux_x86"
#define PI_PLATFORMARCH_LINUX_X64	"linux_x64"
#define PI_PLATFORMARCH_LINUX_ARM64	"linux_arm64"

#define PI_TARGET_PLATFORM64		"platform64"
#define PI_TARGET_DESKTOPPLATFORM	"desktop_platform"

#define PI_TARGET_CONFIG			"config"
#define PI_CONFIG_DEBUG				"debug"
#define PI_CONFIG_RELEASE			"release"

#define PI_TARGET_LANGUAGE			"language"

#define PI_TARGET_DEFINED			"defined"

//************************************************************************************************
// XmlProcessingInstructionHandler
//************************************************************************************************

const char* XmlProcessingInstructionHandler::getPlatform ()
{
	return CCL_PLATFORM_ID_CURRENT;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const char* XmlProcessingInstructionHandler::getPlatformArchitecture ()
{
	#if CCL_PLATFORM_WINDOWS
		#if defined(CCL_PLATFORM_INTEL)
			#if defined(CCL_PLATFORM_64BIT)
				return PI_PLATFORMARCH_WIN_X64;
			#else
				return PI_PLATFORMARCH_WIN_X86;
			#endif
		#elif defined(CCL_PLATFORM_ARM)
			return PI_PLATFORMARCH_WIN_ARM64;
		#endif
	#elif CCL_PLATFORM_IOS
		return PI_PLATFORMARCH_IOS_ARM64;
	#elif CCL_PLATFORM_ANDROID
		#if defined(CCL_PLATFORM_INTEL)
			#if defined(CCL_PLATFORM_64BIT)
				return PI_PLATFORMARCH_ANDR_X64;
			#else
				return PI_PLATFORMARCH_ANDR_X86;
			#endif
		#elif defined(CCL_PLATFORM_ARM)
			#if defined(CCL_PLATFORM_64BIT)
				return PI_PLATFORMARCH_ANDR_ARM64;
			#else
				return PI_PLATFORMARCH_ANDR_ARM;
			#endif
		#endif
	#elif CCL_PLATFORM_LINUX
		#if defined(CCL_PLATFORM_INTEL)
			#if defined(CCL_PLATFORM_64BIT)
				return PI_PLATFORMARCH_LINUX_X64;
			#else
				return PI_PLATFORMARCH_LINUX_X86;
			#endif
		#elif defined(CCL_PLATFORM_ARM)
			return PI_PLATFORMARCH_LINUX_ARM64;
		#endif
	#elif CCL_PLATFORM_MAC
		#if defined(CCL_PLATFORM_INTEL)
			return PI_PLATFORMARCH_MAC_X64;
		#elif defined(CCL_PLATFORM_ARM)
			return PI_PLATFORMARCH_MAC_ARM64;
		#endif
	#else
		#error unknown platform
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const char* XmlProcessingInstructionHandler::getConfiguration (int processingOptions)
{
#if DEBUG
	if(processingOptions & kForceReleaseConfiguration)
		return PI_CONFIG_RELEASE;
	else
		return PI_CONFIG_DEBUG;
#else
	return PI_CONFIG_RELEASE;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlProcessingInstructionHandler::XmlProcessingInstructionHandler (int processingOptions)
: processingOptions (processingOptions),
  skipping (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XmlProcessingInstructionHandler::handleInstruction (StringRef _target, StringRef _data)
{
	MutableCString target (_target);
	MutableCString data (_data);
	bool oldState = skipping;

	// simple handling of nested instructions
	// TODO: implement real stack!
	bool nested = false;
	const CString kNestingPrefix (PI_NESTING_PREFIX);
	if(target.startsWith (kNestingPrefix))
	{
		nested = true;
		target = target.subString (kNestingPrefix.length ());
	}

	bool invert = false;
	const CString kInvertPrefix (PI_INVERT_PREFIX);
	if(target.startsWith (kInvertPrefix))
	{
		invert = true;
		target = target.subString (kInvertPrefix.length ());
	}

	if(target == PI_TARGET_PLATFORM)
	{
		skipping = !data.isEmpty () && !data.contains (getPlatform ());
	}
	else if(target == PI_TARGET_PLATFORMARCH)
	{
		skipping = !data.isEmpty () && !data.contains (getPlatformArchitecture ());
	}
	else if(target == PI_TARGET_PLATFORM64)
	{
		bool dataBool = data != "0";
		#if defined(CCL_PLATFORM_64BIT)
		skipping = !data.isEmpty () && dataBool == false;
		#else
		skipping = !data.isEmpty () && dataBool == true;
		#endif
	}
	else if(target == PI_TARGET_DESKTOPPLATFORM)
	{
		bool dataBool = data != "0";
		#if CCL_PLATFORM_DESKTOP
		skipping = !data.isEmpty () && dataBool == false;
		#else
		skipping = !data.isEmpty () && dataBool == true;
		#endif
	}
	else if(target == PI_TARGET_CONFIG)
	{
		skipping = (!data.isEmpty () && data.compare (getConfiguration (processingOptions), false) != 0) || data == "0";
	}
	else if(target == PI_TARGET_LANGUAGE)
	{
		static MutableCString language (System::GetLocaleManager ().getLanguage ());
		skipping = !data.isEmpty () && !data.contains (language);
	}
	else if(target == PI_TARGET_DEFINED)
	{
		if(data.isEmpty ())
			skipping = false;
		else
		{
			skipping = true;

			String definitions;
			Configuration::Registry::instance ().getValue (definitions, "XML.Parsers", "definitions");
			if(definitions.contains (_data))
				skipping = false;
		}
	}

	if(invert)
		skipping = !skipping;

	if(nested && oldState) // keep skipping if nested and was skipping before
		skipping = true;
}
