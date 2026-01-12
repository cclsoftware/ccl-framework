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
// Filename    : ccl/security/securityservices.cpp
// Description : Security Service APIs
//
//************************************************************************************************

#include "ccl/base/kernel.h"
#include "ccl/base/security/cryptobox.h"

#include "ccl/security/securityhost.h"

#include "ccl/public/base/memorystream.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/system/iexecutable.h"
#include "ccl/public/plugins/itypelibregistry.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/securityservices.h"
#include "ccl/public/cclversion.h"

using namespace CCL;
using namespace Security;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (SecurityServices, kFrameworkLevelFirst)
{
	System::GetExecutableLoader ().addNativeImage (System::GetCurrentModuleRef ());

	// register type libraries
	MetaClassRegistry& typeLib = Kernel::instance ().getClassRegistry ();
	typeLib.setLibName (CCLSECURITY_FILE_DESCRIPTION);
	System::GetTypeLibRegistry ().registerTypeLib (typeLib);

	System::GetScriptingManager ().startup (CCLSECURITY_PACKAGE_ID, System::GetCurrentModuleRef (), nullptr, false);
	System::GetScriptingManager ().getHost ().registerObject ("Security", SecurityHost::instance ());

	System::GetPlugInManager ().addHook (SecurityHost::instance ().asUnknown ());

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_TERM_LEVEL (SecurityServices, kFrameworkLevelFirst)
{
	System::GetPlugInManager ().removeHook (SecurityHost::instance ().asUnknown ());

	System::GetScriptingManager ().getHost ().unregisterObject (SecurityHost::instance ());
	System::GetScriptingManager ().shutdown (System::GetCurrentModuleRef (), false);

	// unregister type libraries
	System::GetTypeLibRegistry ().unregisterTypeLib (Kernel::instance ().getClassRegistry ());

	System::GetExecutableLoader ().removeNativeImage (System::GetCurrentModuleRef ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Security Service APIs
////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void CCL_API System::CCL_ISOLATED (CreateNameBasedUID) (UIDBytes& uid, StringRef name, UIDRef _nameSpace)
{
	// UUID Type 5: General concept: Append name to a namespace, hash the whole string,
	// truncate the result and replace a few bytes to mark it as "name based UUID".
	// Respects endianness.
	// https://www.rfc-editor.org/rfc/rfc4122#section-4.3
		
	static const UID kNetNamespaceDNS (0x6ba7b810, 0x9dad, 0x11d1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8);

	UID nameSpace = _nameSpace;
	if(nameSpace == kNullUID)
		nameSpace = kNetNamespaceDNS;
	
	UIDBuffer temp = {};
	static const int kUIDByteLength = 16; // 128 bit
	nameSpace.toBuffer (temp);
	
	MemoryStream hashInputStream;
	hashInputStream.write (temp, kUIDByteLength);
	MutableCString inputBuffer (name, Text::kUTF8);
	hashInputStream.write (inputBuffer, name.length () + 1);
	hashInputStream.rewind ();
	
	MemoryStream sha1;
	sha1.allocateMemory (Security::Crypto::kSHA1_DigestSize, true);
	Security::Crypto::Block sha1Block (sha1.getBuffer (), Security::Crypto::kSHA1_DigestSize);
	Security::Crypto::SHA1::calculate (sha1Block, hashInputStream);
	sha1.setBytesWritten (Security::Crypto::kSHA1_DigestSize);
	sha1.rewind ();
	sha1.read (temp, kUIDByteLength);
	
	// set high nibble to 5 to indicate version 5 (name based)
	temp[6] &= 0x0F;
	temp[6] |= 0x50;
	
	// Set the variant bits to "10"
	temp[8] &= 0x3F;
	temp[8] |= 0x80;
	
	UID out;
	out.fromBuffer (temp);
	uid = out;
}
