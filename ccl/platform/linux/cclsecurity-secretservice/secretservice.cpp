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
// Filename    : ccl/platform/linux/cclsecurity-secretservice/secretservice.cpp
// Description : CCL Security Secret Service Integration using D-Bus
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/shared/interfaces/platformsecurity.h"
#include "ccl/platform/shared/interfaces/platformintegration.h"
#include "ccl/platform/linux/platformintegration/dbusintegration.h"

#include "ccl/base/asyncoperation.h"

#include "version.h"

#include "org-freedesktop-secret-service-client.h"
#include "org-freedesktop-secret-session-client.h"
#include "org-freedesktop-secret-prompt-client.h"
#include "org-freedesktop-secret-collection-client.h"
#include "org-freedesktop-secret-item-client.h"

namespace CCL {
namespace PlatformIntegration {

//************************************************************************************************
// SecretServicePrompt
//************************************************************************************************

class SecretServicePrompt: public Unknown,
						   public DBusProxy<org::freedesktop::Secret::Prompt_proxy>
{
public:
	SecretServicePrompt (IDBusSupport& dbusSupport, const sdbus::ObjectPath& objectPath);

	static constexpr CStringPtr kPromptSpecialValue = "/";

	bool show (CStringPtr windowId);
	bool run (CStringPtr windowId);
	void hide ();

	// Prompt_proxy
	void onCompleted (const bool& dismissed, const sdbus::Variant& result) override;

	UNKNOWN_REFCOUNT

private:
	AutoPtr<AsyncOperation> operation;

	IAsyncOperation::State getState () const;
};

//************************************************************************************************
// SecretServiceStore
//************************************************************************************************

class SecretServiceStore: public IPlatformCredentialStore,
						  public DBusProxy<org::freedesktop::Secret::Service_proxy>
{
public:
	SecretServiceStore (IDBusSupport& dbusSupport);

	static constexpr CStringPtr kDestination = "org.freedesktop.secrets";
	static constexpr CStringPtr kObjectPath = "/org/freedesktop/secrets";

	// IPlatformCredentialStore
	tbool unlock (tbool silent = false) override;
	tbool setCredentials (CStringPtr targetName, CStringPtr userName, void* data, uint32 dataLength) override;
	tbool getCredentials (Core::IO::IByteStream& username, Core::IO::IByteStream& data, CStringPtr targetName) override;
	tbool removeCredentials (CStringPtr targetName) override;
	void setProperty (const Core::Property& value) override;
	void getProperty (Core::Property& value) override;
	void release () override;

	// Service_proxy
	void onCollectionCreated (const sdbus::ObjectPath& collection) override;
	void onCollectionDeleted (const sdbus::ObjectPath& collection) override;
	void onCollectionChanged (const sdbus::ObjectPath& collection) override;

protected:
	static constexpr CStringPtr kAlgorithm = "plain"; // could also be "dh-ietf1024-sha256-aes128-cbc-pkcs7";
	static constexpr CStringPtr kDefaultCollection = "/org/freedesktop/secrets/aliases/default";

	sdbus::ObjectPath session;
	sdbus::ObjectPath collection;

	bool openSession ();
	bool unlockDefaultCollection (bool silent = true);
};

//************************************************************************************************
// SecretServiceCollection
//************************************************************************************************

class SecretServiceCollection: public DBusProxy<org::freedesktop::Secret::Collection_proxy>
{
public:
	SecretServiceCollection (IDBusSupport& dbusSupport, const sdbus::ObjectPath& objectPath);
	~SecretServiceCollection ();

	static constexpr CStringPtr kTargetNameAttribute = "target";
	static constexpr CStringPtr kUserNameAttribute = "username";

	bool createItem (const sdbus::ObjectPath& session, CStringPtr targetName, CStringPtr userName, void* data, uint32 dataLength);
	sdbus::ObjectPath searchItem (const sdbus::ObjectPath& session, CStringPtr targetName);

	// Collection_proxy
	void onItemCreated (const sdbus::ObjectPath& item) override;
	void onItemDeleted (const sdbus::ObjectPath& item) override;
	void onItemChanged (const sdbus::ObjectPath& item) override;

private:
	static constexpr CStringPtr kLabelProperty = "org.freedesktop.Secret.Item.Label";
	static constexpr CStringPtr kAttributesProperty = "org.freedesktop.Secret.Item.Attributes";
	static constexpr CStringPtr kDefaultMimeType = "application/octet-stream";
};

//************************************************************************************************
// SecretServiceItem
//************************************************************************************************

class SecretServiceItem: public DBusProxy<org::freedesktop::Secret::Item_proxy>
{
public:
	SecretServiceItem (IDBusSupport& dbusSupport, const sdbus::ObjectPath& objectPath);

	bool getCredentials (Core::IO::IByteStream& username, Core::IO::IByteStream& data, const sdbus::ObjectPath& session);
	void remove ();
};

} // namespace PlatformIntegration
} // namespace CCL

using namespace CCL;
using namespace PlatformIntegration;
using namespace Core;

//************************************************************************************************
// SecretServiceStore
//************************************************************************************************

SecretServiceStore::SecretServiceStore (IDBusSupport& dbusSupport)
: DBusProxy (dbusSupport, kDestination, kObjectPath)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SecretServiceStore::openSession ()
{
	if(!session.empty ())
		return true;

	try
	{
		auto result = OpenSession (kAlgorithm, "");
		session = std::get<1> (result);
	}
	CATCH_DBUS_ERROR
	
	return !session.empty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool SecretServiceStore::unlock (tbool silent)
{
	if(!openSession ())
		return false;
	
	return unlockDefaultCollection (silent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SecretServiceStore::unlockDefaultCollection (bool silent)
{
	std::tuple<std::vector<sdbus::ObjectPath>, sdbus::ObjectPath> result;
	try
	{
		result = Unlock ({ kDefaultCollection });
	}
	CATCH_DBUS_ERROR

	auto& unlockedObjects = std::get<0> (result);
	if(std::find (unlockedObjects.begin (), unlockedObjects.end (), kDefaultCollection) != std::end (unlockedObjects))
		return true;

	if(silent)
		return false;

	auto& promptObject = std::get<1> (result);
	SecretServicePrompt prompt (dbusSupport, promptObject);
	return prompt.run (""); // Note: we could set a parent window ID here, but this doesn't seem to be handled in any secret service implementation
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool SecretServiceStore::setCredentials (CStringPtr targetName, CStringPtr userName, void* data, uint32 dataLength)
{
	if(!openSession ())
		return false;

	if(!unlockDefaultCollection ())
		return false;
	
	SecretServiceCollection collection (dbusSupport, kDefaultCollection);
	return collection.createItem (session, targetName, userName, data, dataLength);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool SecretServiceStore::getCredentials (IO::IByteStream& username, IO::IByteStream& password, CStringPtr targetName)
{
	if(!openSession ())
		return false;

	if(!unlockDefaultCollection ())
		return false;
	
	SecretServiceCollection collection (dbusSupport, kDefaultCollection);
	sdbus::ObjectPath itemObject = collection.searchItem (session, targetName);
	if(itemObject.empty ())
		return false;

	SecretServiceItem item (dbusSupport, itemObject);
	return item.getCredentials (username, password, session);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool SecretServiceStore::removeCredentials (CStringPtr targetName)
{
	if(!openSession ())
		return false;

	if(!unlockDefaultCollection ())
		return false;
	
	SecretServiceCollection collection (dbusSupport, kDefaultCollection);
	sdbus::ObjectPath itemObject = collection.searchItem (session, targetName);
	if(itemObject.empty ())
		return false;

	SecretServiceItem item (dbusSupport, itemObject);
	item.remove ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SecretServiceStore::onCollectionCreated (const sdbus::ObjectPath& collection)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SecretServiceStore::onCollectionDeleted (const sdbus::ObjectPath& collection)
{}


//////////////////////////////////////////////////////////////////////////////////////////////////

void SecretServiceStore::onCollectionChanged (const sdbus::ObjectPath& collection)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SecretServiceStore::setProperty (const Property& value)
{}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void SecretServiceStore::getProperty (Property& value)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SecretServiceStore::release ()
{
	delete this;
}

//************************************************************************************************
// SecretServiceCollection
//************************************************************************************************

SecretServiceCollection::SecretServiceCollection (IDBusSupport& dbusSupport, const sdbus::ObjectPath& objectPath)
: DBusProxy (dbusSupport, SecretServiceStore::kDestination, std::move (objectPath))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SecretServiceCollection::~SecretServiceCollection ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SecretServiceCollection::createItem (const sdbus::ObjectPath& session, CStringPtr targetName, CStringPtr userName, void* data, uint32 dataLength)
{
	std::map<std::string, sdbus::Variant> properties;
	properties[kLabelProperty] = targetName;

	std::map<std::string, std::string> attributes;
	attributes[kTargetNameAttribute] = targetName;
	attributes[kUserNameAttribute] = userName;
	properties[kAttributesProperty] = attributes;

	std::vector<uint8_t> dataVector;
	dataVector.assign (static_cast<uint8_t*> (data), static_cast<uint8_t*> (data) + dataLength);

	sdbus::Struct<sdbus::ObjectPath, std::vector<uint8_t>, std::vector<uint8_t>, std::string> secret =
	{
		session, 			// the session that was used to encode the secret.
		{},					// algorithm dependent parameters for secret value encoding.
		dataVector,			// possibly encoded secret value.
		kDefaultMimeType 	// the content type of the secret.
	};

	std::tuple<sdbus::ObjectPath, sdbus::ObjectPath> result;
	try
	{
		result = CreateItem (properties, secret, true);
	}
	CATCH_DBUS_ERROR

	if(std::get<0> (result) != SecretServicePrompt::kPromptSpecialValue) // special value "/" means a prompt is required
		return true;

	auto& promptObject = std::get<1> (result);
	SecretServicePrompt prompt (dbusSupport, promptObject);
	return prompt.run ("");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sdbus::ObjectPath SecretServiceCollection::searchItem (const sdbus::ObjectPath& session, CStringPtr targetName)
{
	std::map<std::string, std::string> attributes;
	attributes[kTargetNameAttribute] = targetName;

	std::vector<sdbus::ObjectPath> result;
	try
	{
		result = SearchItems (attributes);
	}
	CATCH_DBUS_ERROR

	if(result.empty ())
		return "";
	return result[0];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SecretServiceCollection::onItemCreated (const sdbus::ObjectPath& item)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SecretServiceCollection::onItemDeleted (const sdbus::ObjectPath& item)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SecretServiceCollection::onItemChanged (const sdbus::ObjectPath& item)
{}

//************************************************************************************************
// SecretServicePrompt
//************************************************************************************************

SecretServicePrompt::SecretServicePrompt (IDBusSupport& dbusSupport, const sdbus::ObjectPath& objectPath)
: DBusProxy (dbusSupport, SecretServiceStore::kDestination, std::move (objectPath))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SecretServicePrompt::show (CStringPtr windowId)
{
	try
	{
		Prompt (windowId);
		operation = NEW AsyncOperation;
		operation->setState (IAsyncOperation::kStarted);
		return true;
	}
	CATCH_DBUS_ERROR
	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SecretServicePrompt::hide ()
{
	try
	{
		Dismiss ();	
	}
	CATCH_DBUS_ERROR
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SecretServicePrompt::run (CStringPtr windowId)
{
	if(!show (windowId))
		return false;
	while(getState () == IAsyncOperation::kStarted)
	{
		if(!dbusSupport.flushUpdates ())
			break;
	}
	return getState () == IAsyncOperation::kCompleted;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation::State SecretServicePrompt::getState () const
{
	if(operation.isValid () == false)
		return IAsyncOperation::kNone;
	return operation->getState ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SecretServicePrompt::onCompleted (const bool& dismissed, const sdbus::Variant& result)
{
	if(operation)
		operation->setState (dismissed ? IAsyncOperation::kFailed : IAsyncOperation::kCompleted);
}

//************************************************************************************************
// SecretServiceItem
//************************************************************************************************

SecretServiceItem::SecretServiceItem (IDBusSupport& dbusSupport, const sdbus::ObjectPath& objectPath)
: DBusProxy (dbusSupport, SecretServiceStore::kDestination, std::move (objectPath))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SecretServiceItem::getCredentials (Core::IO::IByteStream& username, Core::IO::IByteStream& data, const sdbus::ObjectPath& session)
{
	std::map<std::string, std::string> attributes;
	try
	{
		attributes = Attributes ();
	}
	CATCH_DBUS_ERROR

	auto name = attributes.find (SecretServiceCollection::kUserNameAttribute);
	if(name != attributes.end ())
		username.writeBytes (name->second.c_str (), int(name->second.size ()) + 1);

	auto secret = GetSecret (session);
	auto value = std::get<2> (secret);
	data.writeBytes (&value[0], int(value.size ()));

	return !value.empty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SecretServiceItem::remove ()
{
	sdbus::ObjectPath promptObject;
	try 
	{
		promptObject = Delete ();
	}
	CATCH_DBUS_ERROR

	if(promptObject == SecretServicePrompt::kPromptSpecialValue) // special value "/" means no prompt is required
		return;

	SecretServicePrompt prompt (dbusSupport, promptObject);
	prompt.run ("");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef DBusClassFactory<SecretServiceStore, IPlatformCredentialStore> SecretStoreFactory;

DEFINE_PLATFORMINTEGRATION_CLASS2 (SecretStoreClass, "SecretService", "{2e9db42a-a539-4213-b3af-f8b437b82e91}", DEFINE_PLATFORMINTEGRATION_ATTRIBUTES (DBUS_ENVIRONMENT, ""), SecretStoreFactory::createInstance)

BEGIN_CORE_CLASSINFO_BUNDLE (DEFINE_CORE_VERSIONINFO (PLUG_NAME, PLUG_COMPANY, PLUG_VERSION, PLUG_COPYRIGHT, PLUG_WEBSITE))
	ADD_CORE_CLASSINFO (SecretStoreClass)
END_CORE_CLASSINFO_BUNDLE
