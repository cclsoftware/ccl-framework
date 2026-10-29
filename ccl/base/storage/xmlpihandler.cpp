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

#include "ccl/meta/generated/cpp/xmlinstructions-generated.h"

using namespace CCL;

//************************************************************************************************
// XmlProcessingInstructionHandler
//************************************************************************************************

CStringPtr XmlProcessingInstructionHandler::getPlatform ()
{
	return CCL_PLATFORM_ID_CURRENT;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr XmlProcessingInstructionHandler::getPlatformArchitecture ()
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
	#elif CCL_PLATFORM_WASM
		return PI_PLATFORMARCH_WASM;
	#else
		#error unknown platform
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr XmlProcessingInstructionHandler::getConfiguration (int processingOptions)
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

	// A nested instruction (with data) keeps skipping if the enclosing block is skipped and saves the state of
	// the enclosing block, a nested instruction without data ends it and restores that state.
	// A plain instruction replaces the current block and ends all nesting.
	bool nested = false;
	const CString kNestingPrefix (PI_NESTING_PREFIX);
	if(target.startsWith (kNestingPrefix))
	{
		nested = true;
		target = target.subString (kNestingPrefix.length ());
	}

	if(nested)
	{
		if(data.isEmpty ())
		{
			skipping = skippingStack.isEmpty () ? false : skippingStack.pop ();
			return;
		}

		skippingStack.push (oldState);
	}
	else
		skippingStack.removeAll ();

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
			skipping = isDefined (_data) ? false : true;
	}

	if(invert)
		skipping = !skipping;

	if(nested && oldState) // keep skipping if nested and was skipping before
		skipping = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlProcessingInstructionHandler::isDefined (StringRef query) const
{
	ASSERT (query.index (" ") == -1) // only single tokens allowed

	String definitions;
	Configuration::Registry::instance ().getValue (definitions, Configuration::kXmlParsers, Configuration::kDefinitions);
	if(definitions.contains (query))
		return true;

	return false;
}
