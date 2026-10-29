//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/wasm/security/credentialmanager.wasm.cpp
// Description : WebAssembly Credential Manager
//
//************************************************************************************************

#include "ccl/security/credentialmanager.h"

#include "ccl/base/asyncoperation.h"
#include "ccl/base/storage/settings.h"
#include "ccl/base/security/cryptomaterial.h"

namespace CCL {
namespace Security {

//////////////////////////////////////////////////////////////////////////////////////////////////
// JS Helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS_DEPS (credentialManagerDependencies, "$dynCall");

/**
  Encrypt data with a non-extractable AES-GCM key kept in IndexedDB (see CCL.getCredentialsKey in
  cclsystem.js) - the raw key material never enters WASM memory, only plaintext/ciphertext do.
  Result delivered via onComplete is (12-byte IV || ciphertext). targetName/groupName/userName are
  round-tripped through JS, since there is nowhere on the C++ side to hold them for a plain
  function-pointer callback. All pointers are read synchronously before the first await, so the
  caller's buffers only need to stay alive for the duration of this call, not the whole operation.
*/
EM_JS (void, credentialManagerEncrypt, (void* operation, const uint8* data, int dataLength,
			CStringPtr _targetName, CStringPtr _groupName, CStringPtr _userName, void* onComplete),
{
	const plainBytes = HEAPU8.slice (data, data + dataLength);
	const targetName = UTF8ToString (_targetName);
	const groupName = UTF8ToString (_groupName);
	const userName = UTF8ToString (_userName);

	(async () =>
	{
		try
		{
			const key = await CCL.getCredentialsKey ();
			const iv = crypto.getRandomValues (new Uint8Array (12));
			const cipherBytes = new Uint8Array (await crypto.subtle.encrypt ({ name: 'AES-GCM', iv }, key, plainBytes));

			const combined = new Uint8Array (iv.length + cipherBytes.length);
			combined.set (iv, 0);
			combined.set (cipherBytes, iv.length);

			const resultPtr = _malloc (combined.length);
			HEAPU8.set (combined, resultPtr);

			dynCall ('viiiiiii', onComplete, [operation, resultPtr, combined.length, stringToNewUTF8 (targetName), stringToNewUTF8 (groupName), stringToNewUTF8 (userName), 1]);
		}
		catch(e)
		{
			dynCall ('viiiiiii', onComplete, [operation, 0, 0, 0, 0, 0, 0]);
		}
	}) ();
});

//////////////////////////////////////////////////////////////////////////////////////////////////

/**
  Decrypt data produced by credentialManagerEncrypt (see above). Same round-trip rationale as
  credentialManagerEncrypt.
*/
EM_JS (void, credentialManagerDecrypt, (void* operation, const uint8* data, int dataLength,
			CStringPtr _targetName, CStringPtr _groupName, CStringPtr _userName, void* onComplete),
{
	const combined = HEAPU8.slice (data, data + dataLength);
	const targetName = UTF8ToString (_targetName);
	const groupName = UTF8ToString (_groupName);
	const userName = UTF8ToString (_userName);

	(async () =>
	{
		try
		{
			if(combined.length <= 12)
				throw new Error ('ciphertext too short');

			const iv = combined.slice (0, 12);
			const cipherBytes = combined.slice (12);

			const key = await CCL.getCredentialsKey ();
			const plainBytes = new Uint8Array (await crypto.subtle.decrypt ({ name: 'AES-GCM', iv }, key, cipherBytes));

			const resultPtr = _malloc (plainBytes.length);
			HEAPU8.set (plainBytes, resultPtr);

			dynCall ('viiiiiii', onComplete, [operation, resultPtr, plainBytes.length, stringToNewUTF8 (targetName), stringToNewUTF8 (groupName), stringToNewUTF8 (userName), 1]);
		}
		catch(e)
		{
			dynCall ('viiiiiii', onComplete, [operation, 0, 0, 0, 0, 0, 0]);
		}
	}) ();
});

//************************************************************************************************
// WasmCredentialStore
//************************************************************************************************

class WasmCredentialStore: public CredentialStore
{
public:
	WasmCredentialStore ();
	~WasmCredentialStore ();

	// CredentialStore
	tresult add (Credential& credential) override { return kResultFailed; }
	Credential* get (StringRef targetName, StringRef groupName) override { return nullptr; }
	tresult remove (StringRef targetName) override;
	IAsyncOperation* addAsync (Credential& credential) override;
	IAsyncOperation* getAsync (StringRef targetName, StringRef groupName) override;

private:
	class CredentialOperation: public AsyncOperation
	{
	public:
		CredentialOperation (WasmCredentialStore* owner)
		: owner (owner)
		{}

		PROPERTY_POINTER (WasmCredentialStore, owner, Owner)
	};

	XmlSettings credentials;
	Vector<SharedPtr<CredentialOperation>> pendingOperations;

	Settings::Section* getSection (StringRef targetName, StringRef groupName = nullptr, bool create = false);
	void removePending (AsyncOperation* operation);

	static void onEncryptComplete (void* operation, uint8* resultData, int resultLength,
		char* targetName, char* groupName, char* userName, bool success);
	static void onDecryptComplete (void* operation, uint8* resultData, int resultLength,
		char* targetName, char* groupName, char* userName, bool success);
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
	return NEW WasmCredentialStore;
}

//************************************************************************************************
// WasmCredentialStore
/** Browsers only expose credential encryption (Web Crypto's non-extractable CryptoKey) as
	Promises, which cannot back a synchronous call, so credentials are only ever handled through
	the Async entry points here; the plain add()/get() are unsupported. Encrypted blobs are kept
	in the same IndexedDB-backed settings location used for other user data. */
//************************************************************************************************

WasmCredentialStore::WasmCredentialStore ()
: credentials ("Credentials")
{
	credentials.restore ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmCredentialStore::~WasmCredentialStore ()
{
	VectorForEach (pendingOperations, SharedPtr<CredentialOperation>, operation)
		operation->setOwner (nullptr);
		operation->cancel ();
	EndFor
	pendingOperations.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Settings::Section* WasmCredentialStore::getSection (StringRef targetName, StringRef groupName, bool create)
{
	String path;
	if(!groupName.isEmpty ())
		path << groupName << "/";
	path << targetName;

	return credentials.getSection (path, create);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmCredentialStore::removePending (AsyncOperation* operation)
{
	pendingOperations.removeIf ([operation] (SharedPtr<CredentialOperation>& item) { return item == operation; });
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult WasmCredentialStore::remove (StringRef targetName)
{
	Settings::Section* section = getSection (targetName);
	if(!section)
		return kResultFailed;

	if(!credentials.remove (section->getPath ()))
		return kResultFailed;

	return credentials.flush () ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* WasmCredentialStore::addAsync (Credential& src)
{
	AutoPtr<CredentialOperation> operation = NEW CredentialOperation (this);
	operation->setState (IAsyncInfo::kStarted);
	pendingOperations.add (operation);

	// Kept alive for the pending JS round-trip regardless of what pendingOperations/the caller do
	// in the meantime; released in onEncryptComplete once the round-trip is done with it.
	operation->retain ();

	Crypto::Material data;
	if(Crypto::Material* srcData = src.getData ())
		data = *srcData;
	Crypto::Block block = data.asBlock ();

	MutableCString target (src.getTargetName (), Text::kUTF8);
	MutableCString group (src.getGroupName (), Text::kUTF8);
	MutableCString user (src.getUserName (), Text::kUTF8);

	credentialManagerEncrypt (operation, block.data, int(block.length), target, group, user, (void*)onEncryptComplete);

	return operation.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* WasmCredentialStore::getAsync (StringRef targetName, StringRef groupName)
{
	Settings::Section* section = getSection (targetName, groupName);
	if(!section)
		return AsyncOperation::createFailed ();

	Attributes& a = section->getAttributes ();

	Crypto::Material encryptedData;
	a.get (encryptedData, "data");

	String userName;
	a.get (userName, "userName");

	AutoPtr<CredentialOperation> operation = NEW CredentialOperation (this);
	operation->setState (IAsyncInfo::kStarted);
	pendingOperations.add (operation);

	// Kept alive for the pending JS round-trip regardless of what pendingOperations/the caller do
	// in the meantime; released in onDecryptComplete once the round-trip is done with it.
	operation->retain ();

	Crypto::Block block = encryptedData.asBlock ();

	MutableCString target (targetName, Text::kUTF8);
	MutableCString group (groupName, Text::kUTF8);
	MutableCString user (userName, Text::kUTF8);

	credentialManagerDecrypt (operation, block.data, int(block.length), target, group, user, (void*)onDecryptComplete);

	return operation.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmCredentialStore::onEncryptComplete (void* _operation, uint8* resultData, int resultLength,
	char* _targetName, char* _groupName, char* _userName, bool success)
{
	CredentialOperation* operation = static_cast<CredentialOperation*> (_operation);

	if(WasmCredentialStore* This = operation->getOwner ())
	{
		// Only write/complete if nobody canceled us in the meantime; but even if they did,
		// the store itself is still alive here, so remove ourselves from its pending list either way.
		if(operation->getState () == IAsyncInfo::kStarted)
		{
			if(success && resultData)
			{
				Crypto::Material encryptedData (Crypto::Block (resultData, resultLength));

				String targetName (Text::kUTF8, _targetName);
				String userName (Text::kUTF8, _userName);
				String groupName (Text::kUTF8, _groupName);
				Settings::Section* section = This->getSection (targetName, groupName, true);
				Attributes& a = section->getAttributes ();
				a.set ("userName", userName);
				a.set ("data", encryptedData);

				bool flushed = This->credentials.flush ();
				operation->setStateDeferred (flushed ? IAsyncInfo::kCompleted : IAsyncInfo::kFailed);
			}
			else
				operation->setStateDeferred (IAsyncInfo::kFailed);
		}

		This->removePending (operation);
	}

	if(resultData)
		::free (resultData);
	if(_targetName)
		::free (_targetName);
	if(_groupName)
		::free (_groupName);
	if(_userName)
		::free (_userName);

	operation->release (); // balances the retain in addAsync
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmCredentialStore::onDecryptComplete (void* _operation, uint8* resultData, int resultLength,
	char* _targetName, char* _groupName, char* _userName, bool success)
{
	CredentialOperation* operation = static_cast<CredentialOperation*> (_operation);

	if(WasmCredentialStore* This = operation->getOwner ())
	{
		if(operation->getState () == IAsyncInfo::kStarted)
		{
			if(success && resultData)
			{
				Crypto::Block data (resultData, resultLength);
				String targetName (Text::kUTF8, _targetName);
				String userName (Text::kUTF8, _userName);
				String groupName (Text::kUTF8, _groupName);
				AutoPtr<Credential> credential = NEW Credential (targetName, userName, data, groupName);
				operation->setResult (Variant (credential->asUnknown (), true));
				operation->setStateDeferred (IAsyncInfo::kCompleted);
			}
			else
				operation->setStateDeferred (IAsyncInfo::kFailed);
		}

		This->removePending (operation);
	}

	if(resultData)
		::free (resultData);
	if(_targetName)
		::free (_targetName);
	if(_groupName)
		::free (_groupName);
	if(_userName)
		::free (_userName);

	operation->release (); // balances the retain in getAsync
}