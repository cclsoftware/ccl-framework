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
// Filename    : ccl/platform/cocoa/security/credentialmanager.cocoa.mm
// Description : MacOS Credential Manager
//
//************************************************************************************************

#include "ccl/security/credentialmanager.h"
#include "ccl/public/base/streamer.h"
#include "ccl/public/base/memorystream.h"

#include "ccl/platform/cocoa/cclcocoa.h"

namespace CCL {
namespace Security {

//************************************************************************************************
// CocoaCredentialStore
//************************************************************************************************

class CocoaCredentialStore: public CredentialStore
{
public:
	// CredentialStore
	bool setSilentMode (bool state) override;
	tresult add (Credential& credential) override;
	tresult remove (StringRef targetName) override;
	Credential* get (StringRef targetName, StringRef groupName) override;

protected:
	static NSMutableDictionary* createKeychainQuery (StringRef target);
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
	return NEW CocoaCredentialStore;
}

//************************************************************************************************
// CocoaCredentialStore
//************************************************************************************************

bool CocoaCredentialStore::setSilentMode (bool state)
{
	#if CCL_PLATFORM_IOS
	return false;
	#else
	// SecKeychain is deprecated, there appears to be no replacement
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	SecKeychainSetUserInteractionAllowed (!state);
	#pragma pop
	return CredentialStore::setSilentMode (state);
	#endif

}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CocoaCredentialStore::add (Credential& src)
{
	// Read credential data
	NSString* userString = [((NSString*)src.getUserName ().createNativeString<CFStringRef> ()) autorelease];
	Crypto::Block data = src.getData ()->asBlock ();
	NSData* password = [NSData dataWithBytes:data.data length:data.length];
	NSString* group = [((NSString*)src.getGroupName ().createNativeString<CFStringRef> ()) autorelease];
	if(group.length == 0)
		group = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"KeychainAccessGroup"];
	
	NSMutableDictionary* keychainQuery = [createKeychainQuery (src.getTargetName ()) autorelease];
	if(group.length != 0)
	{
		[keychainQuery setObject:group forKey:(id)kSecAttrAccessGroup];
		[keychainQuery setObject:(id)kCFBooleanTrue forKey:(id)kSecAttrSynchronizable];
	}
	
	NSDictionary* updates = [NSDictionary dictionaryWithObjectsAndKeys:userString, (id)kSecAttrAccount, password, (id)kSecValueData, nil];
	
	// try first to update existing item before creating a new one (see article : https://developer.apple.com/documentation/security/keychain_services/keychain_items/updating_and_deleting_keychain_items)
	OSStatus status = SecItemUpdate ((CFDictionaryRef)keychainQuery, (CFDictionaryRef)updates);
	if(status != noErr)
	{	
		[keychainQuery setObject:userString forKey:(id)kSecAttrAccount];
		[keychainQuery setObject:password forKey:(id)kSecValueData];
		status = SecItemAdd ((CFDictionaryRef)keychainQuery, NULL);
	}
	if(status == errSecInvalidOwnerEdit)
		return kResultAccessDenied;
	else if(status != noErr)
		return kResultFailed;
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CocoaCredentialStore::remove (StringRef targetName)
{
	NSMutableDictionary* keychainQuery = [createKeychainQuery (targetName) autorelease];
	if(!keychainQuery)
		return kResultFailed;
	
	[keychainQuery setObject:(id)kSecAttrSynchronizableAny forKey:(id)kSecAttrSynchronizable];
	
	OSStatus status = SecItemDelete ((CFDictionaryRef)keychainQuery);

	if(status == errSecInvalidOwnerEdit)
		return kResultAccessDenied;
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Credential* CocoaCredentialStore::get (StringRef targetName, StringRef groupName)
{
	AutoPtr<Credential> object;

	NSMutableDictionary* keychainQuery = [createKeychainQuery (targetName) autorelease];
	[keychainQuery setObject:(id)kCFBooleanTrue forKey:(id)kSecReturnAttributes];
	[keychainQuery setObject:(id)kCFBooleanTrue forKey:(id)kSecReturnData];
		
	NSDictionary* keychainEntry = nil;
	
	// Default: Try to get iOS style keychain entry from any available access group
	[keychainQuery setObject:(id)kCFBooleanTrue forKey:(id)kSecAttrSynchronizable];
	OSStatus status = SecItemCopyMatching ((CFDictionaryRef)keychainQuery, (CFTypeRef*)&keychainEntry);
	
	// Fallback: Try to get (legacy) macOS file-based keychain entry
	if(status != noErr)
	{
		[keychainQuery setObject:(id)kCFBooleanFalse forKey:(id)kSecAttrSynchronizable];
		status = SecItemCopyMatching ((CFDictionaryRef)keychainQuery, (CFTypeRef*)&keychainEntry);
	}
	
	if(status == noErr)
	{
		object = NEW Credential;
		object->setTargetName (targetName);
		
		NSString* userName = [keychainEntry objectForKey:(id)kSecAttrAccount];
		ASSERT(userName)
		if(userName)
		{
			String user;
			user.appendNativeString (userName);
			object->setUserName (user);
		}
		
		NSString* entryGroupName = [keychainEntry objectForKey:(id)kSecAttrAccessGroup];
		if(entryGroupName)
		{
			String entryGroup;
			entryGroup.appendNativeString (entryGroupName);
			object->setGroupName (entryGroup);
		}
		
		NSData* password = [keychainEntry objectForKey:(id)kSecValueData];
		ASSERT(password)
		if(password)
		{
			AutoPtr<Crypto::Material> data = NEW Crypto::Material (Crypto::Block (password.bytes, (uint32)password.length));
			object->setData (data);
		}
	}
	else if(status != errSecItemNotFound)
	{
		CCL_WARN ("SecItemCopyMatching() failed with error %d!\n", status)
	}	
	
	if(keychainEntry)
		[keychainEntry release];
		
	return object.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NSMutableDictionary* CocoaCredentialStore::createKeychainQuery (StringRef target)
{
	NSString* targetString = [((NSString*)target.createNativeString<CFStringRef> ()) autorelease];
	return [[NSMutableDictionary alloc] initWithObjectsAndKeys:(id)kSecClassGenericPassword, (id)kSecClass, targetString, (id)kSecAttrService, nil];
}
	
