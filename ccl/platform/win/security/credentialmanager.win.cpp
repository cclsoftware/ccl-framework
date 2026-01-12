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
// Filename    : ccl/platform/win/security/credentialmanager.win.cpp
// Description : Win32 Credential Manager
//
//************************************************************************************************

#include "ccl/security/credentialmanager.h"

#include "ccl/platform/win/cclwindows.h"

#include <WinCred.h>

namespace CCL {
namespace Security {

//************************************************************************************************
// Win32CredentialStore
//************************************************************************************************

class Win32CredentialStore: public CredentialStore
{
public:
	// CredentialStore
	tresult add (Credential& credential) override;
	Credential* get (StringRef targetNam, StringRef groupName) override;
	tresult remove (StringRef targetName) override;
};

//************************************************************************************************
// Win32CredentialAttributeName
//************************************************************************************************

struct Win32CredentialAttributeName
{
	WCHAR keyBuffer[16] = {};

	void setPartName (int index)
	{
		::wsprintf (keyBuffer, L"CCL_CredPart%02d", index+1); // start at 1
	}
};

static LPWSTR kCredentialAttributePartFormat = L"CCL Credential Attribute Part Format";

//************************************************************************************************
// Win32CredentialAttributeBuffer
//************************************************************************************************

struct Win32CredentialAttributeBuffer
{
	static constexpr int kTotalSize = CRED_MAX_ATTRIBUTES * CRED_MAX_VALUE_SIZE; // 16KB

	int attributeCount = 0;
	CREDENTIAL_ATTRIBUTE attributes[CRED_MAX_ATTRIBUTES] = {};
	Win32CredentialAttributeName attributeNames[CRED_MAX_ATTRIBUTES];
	BYTE valueBuffer[kTotalSize] = {};

	int assignParts (const BYTE* data, int length)
	{
		int bytesCopied = 0;
		const BYTE* src = data;
		for(attributeCount = 0; length > 0 && attributeCount < CRED_MAX_ATTRIBUTES; attributeCount++)
		{
			Win32CredentialAttributeName& name = attributeNames[attributeCount];
			name.setPartName (attributeCount);

			int toCopy = get_min (length, CRED_MAX_VALUE_SIZE);

			CREDENTIAL_ATTRIBUTE& a = attributes[attributeCount];
			a.Keyword = name.keyBuffer;
			a.Flags = 0;
			a.Value = valueBuffer + attributeCount * CRED_MAX_VALUE_SIZE;
			a.ValueSize = toCopy;
			
			::memcpy (a.Value, src, toCopy);

			length -= toCopy;
			bytesCopied += toCopy;
			src += toCopy;
		}
		return bytesCopied;
	}
};

} // namespace Security
} // namespace CCL

using namespace CCL;
using namespace Security;

//************************************************************************************************
// CredentialManager
//************************************************************************************************

CredentialStore* CredentialManager::createNativeStore ()
{
	return NEW Win32CredentialStore;
}

//************************************************************************************************
// Win32CredentialStore
//************************************************************************************************

tresult Win32CredentialStore::add (Credential& src)
{
	Crypto::Block dataBlock;
	if(src.getData ())
		dataBlock = src.getData ()->asBlock ();

	CREDENTIAL credential = {};
	credential.Type = CRED_TYPE_GENERIC;
	credential.Persist = CRED_PERSIST_LOCAL_MACHINE;

	StringChars targetName (src.getTargetName ());
	credential.TargetName = const_cast<uchar*> (static_cast<const uchar*> (targetName));
	ASSERT (src.getTargetName ().length () <= CRED_MAX_GENERIC_TARGET_NAME_LENGTH)

	StringChars userName (src.getUserName ());
	credential.UserName = const_cast<uchar*> (static_cast<const uchar*> (userName));
	ASSERT (src.getUserName ().length () <= CRED_MAX_USERNAME_LENGTH)

	// ATTENTION: Credential blob size is limited to 2.5KB (see CRED_MAX_CREDENTIAL_BLOB_SIZE).
	// We use credential attributes to store up to 16KB additional data.
	
	Deleter<Win32CredentialAttributeBuffer> attributesDeleter (nullptr);
	static constexpr uint32 kMaxBlobSize = CRED_MAX_CREDENTIAL_BLOB_SIZE;	
	if(dataBlock.length > kMaxBlobSize) 
	{
		credential.CredentialBlob = dataBlock.data;
		credential.CredentialBlobSize = kMaxBlobSize;

		const uint8* additionalData = dataBlock.data + kMaxBlobSize;
		int additionalSize = dataBlock.length - kMaxBlobSize;		
		ASSERT (additionalSize <= Win32CredentialAttributeBuffer::kTotalSize)
		
		auto* attributeBuffer = NEW Win32CredentialAttributeBuffer;
		attributesDeleter._ptr = attributeBuffer;
		if(attributeBuffer->assignParts (additionalData, additionalSize) != additionalSize)
			return kResultOutOfMemory;

		credential.Attributes = attributeBuffer->attributes;
		credential.AttributeCount = attributeBuffer->attributeCount;
		credential.Comment = kCredentialAttributePartFormat;
	}
	else
	{
		credential.CredentialBlob = dataBlock.data;
		credential.CredentialBlobSize = dataBlock.length;
	}

	BOOL result = ::CredWrite (&credential, 0);
	if(!result)
	{
		int error = ::GetLastError ();
		CCL_WARN ("CredWrite() failed with error %d!\n", error)
	}
	return result ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Credential* Win32CredentialStore::get (StringRef targetName, StringRef /*groupName*/)
{
	AutoPtr<Credential> object;
	
	CREDENTIAL* credential = nullptr;
	BOOL result = ::CredRead (StringChars (targetName), CRED_TYPE_GENERIC, 0, &credential);
	if(result)
	{
		AutoPtr<Crypto::Material> data = NEW Crypto::Material (Crypto::Block (credential->CredentialBlob, credential->CredentialBlobSize));

		// check for additional data stored in attributes
		if(credential->Comment && ::wcscmp (credential->Comment, kCredentialAttributePartFormat) == 0)
		{
			for(int i = 0; i < credential->AttributeCount; i++)
			{
				Win32CredentialAttributeName expectedName;
				expectedName.setPartName (i);

				const CREDENTIAL_ATTRIBUTE& a = credential->Attributes[i];
				if(a.Keyword && ::wcscmp (a.Keyword, expectedName.keyBuffer) == 0) // it's unclear if order is always preserved
					data->appendBytes (a.Value, a.ValueSize);
				else
				{
					CCL_WARN ("CredRead() unexpected credential attribute!\n", 0)
					data.release ();
					break;
				}
			}
		}

		if(data)
		{
			object = NEW Credential;
			object->setTargetName (targetName);
			object->setUserName (String () << credential->UserName);
			object->setData (data);
		}
	}
	else
	{
		int error = ::GetLastError ();
		if(error != ERROR_NOT_FOUND)
			CCL_WARN ("CredRead() failed with error %d!\n", error)
	}

	if(credential)
		::CredFree (credential);
	return object.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult Win32CredentialStore::remove (StringRef targetName)
{
	BOOL result = ::CredDelete (StringChars (targetName), CRED_TYPE_GENERIC, 0);
	return result ? kResultOk : kResultFailed;
}
