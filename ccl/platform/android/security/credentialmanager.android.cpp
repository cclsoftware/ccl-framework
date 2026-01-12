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
// Filename    : ccl/platform/win/security/credentialmanager.android.cpp
// Description : Android Credential Manager
//
//************************************************************************************************

#include "ccl/security/credentialmanager.h"

#include "ccl/platform/android/interfaces/iandroidsystem.h"
#include "ccl/platform/android/interfaces/iframeworkactivity.h"
#include "ccl/platform/android/interfaces/jni/androidcontent.h"

#include "ccl/base/security/cipher.h"

#include "ccl/public/securityservices.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/system/isysteminfo.h"

namespace CCL {
namespace Security {

//************************************************************************************************
// AndroidCredentialStore
//************************************************************************************************

class AndroidCredentialStore: public CredentialStore
{
public:
	// CredentialStore
	tresult add (Credential& credential) override;
	Credential* get (StringRef targetName, StringRef groupName) override;
	tresult remove (StringRef targetName) override;

private:
	static CStringPtr kUser;
	static CStringPtr kData;

	jobject getSharedPreferences (StringRef targetName) const;
	Crypto::Cipher* createCredentialsCipher () const;
};

} // namespace Security
} // namespace CCL

using namespace CCL;
using namespace Android;
using namespace Security;

//************************************************************************************************
// CredentialManager
//************************************************************************************************

CredentialStore* CredentialManager::createNativeStore ()
{
	return NEW AndroidCredentialStore;
}

//************************************************************************************************
// AndroidCredentialStore
//************************************************************************************************

CStringPtr AndroidCredentialStore::kUser = "user";
CStringPtr AndroidCredentialStore::kData = "data";

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult AndroidCredentialStore::add (Credential& src)
{
	JniAccessor jni;
	LocalRef sharedPreferences (jni, getSharedPreferences (src.getTargetName ()));
	if(!sharedPreferences)
		return kResultUnexpected;

	LocalRef editor (jni, SharedPreferences.edit (sharedPreferences));

	StringRef userName = src.getUserName ();
	JniString jUserName (jni, StringChars (userName), userName.length ());

	MutableCString dataBase64;
	if(Crypto::Material* data = src.getData ())
	{
		AutoPtr<Crypto::Cipher> cipher = createCredentialsCipher ();
		if(!cipher)
			return kResultUnexpected;

		Crypto::Material encryptedData;
		cipher->encrypt (encryptedData, *data);

		dataBase64 = encryptedData.toCBase64 ();
	}

	SharedPreferencesEditor.putString (editor, JniString (jni, kUser), jUserName);
	SharedPreferencesEditor.putString (editor, JniString (jni, kData), JniString (jni, dataBase64));

	if(!SharedPreferencesEditor.commit (editor))
		return kResultFailed;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Credential* AndroidCredentialStore::get (StringRef targetName, StringRef /*groupName*/)
{
	JniAccessor jni;
	LocalRef sharedPreferences (jni, getSharedPreferences (targetName));
	if(!sharedPreferences)
		return nullptr;

	if(!SharedPreferences.contains (sharedPreferences, JniString (jni, kUser)) ||
	   !SharedPreferences.contains (sharedPreferences, JniString (jni, kData)))
		return nullptr;

	LocalStringRef jUserName (jni, SharedPreferences.getString (sharedPreferences, JniString (jni, kUser), nullptr));
	LocalStringRef jDataBase64 (jni, SharedPreferences.getString (sharedPreferences, JniString (jni, kData), nullptr));

	Crypto::Material encryptedData;
	encryptedData.fromBase64 (fromJavaString (jDataBase64));

	AutoPtr<Crypto::Cipher> cipher = createCredentialsCipher ();
	if(!cipher)
		return nullptr;

	Crypto::Material data;
	cipher->decrypt (data, encryptedData);

	return NEW Credential (targetName, fromJavaString (jUserName), data.asBlock ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult AndroidCredentialStore::remove (StringRef targetName)
{
	JniAccessor jni;
	LocalRef sharedPreferences (jni, getSharedPreferences (targetName));
	if(!sharedPreferences)
		return kResultUnexpected;

	LocalRef editor (jni, SharedPreferences.edit (sharedPreferences));

	SharedPreferencesEditor.clear (editor);

	if(!SharedPreferencesEditor.commit (editor))
		return kResultFailed;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

jobject AndroidCredentialStore::getSharedPreferences (StringRef targetName) const
{
	JniAccessor jni;
	JniString jTargetName (jni, StringChars (targetName), targetName.length ());

	if(UnknownPtr<IAndroidSystem> androidSystem = &System::GetSystem ())
		if(IFrameworkActivity* activity = androidSystem->getNativeActivity ())
			return Context.getSharedPreferences (activity->getJObject (), jTargetName, Context.MODE_PRIVATE);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Crypto::Cipher* AndroidCredentialStore::createCredentialsCipher () const
{
	AutoPtr<Crypto::Cipher> cipher = NEW Crypto::Cipher;
	if(cipher->setFromKeyStore (KeyID::kCredentials))
		return cipher.detach ();

	ASSERT (false)
	return nullptr;
}
