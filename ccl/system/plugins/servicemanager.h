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
// Filename    : ccl/system/plugins/servicemanager.h
// Description : Service Manager
//
//************************************************************************************************

#ifndef _ccl_servicemanager_h
#define _ccl_servicemanager_h

#include "ccl/base/singleton.h"
#include "ccl/base/boxedtypes.h"
#include "ccl/base/collections/objectarray.h"
#include "ccl/public/collections/linkedlist.h"

#include "ccl/public/plugins/iservicemanager.h"

namespace CCL {

class Settings;
class Attributes;
interface IComponent;
interface IClassFactory;
interface IProgressNotify;

//************************************************************************************************
// ServiceDescription
//************************************************************************************************

class ServiceDescription: public Object,
						  public IServiceDescription
{
public:
	DECLARE_CLASS (ServiceDescription, Object)

	enum Type { kProgram, kUser, kFramework };

	ServiceDescription (Type type = kProgram);
	~ServiceDescription ();

	PROPERTY_VARIABLE (Type, type, Type)
	PROPERTY_VARIABLE (int, priority, Priority) ///< lower values first

	bool isUser () const	{ return type == kUser; }
	bool isHidden () const	{ return type == kFramework; }

	PROPERTY_STRING (name, Name)
	PROPERTY_STRING (title, Title)
	PROPERTY_STRING (description, Description)
	PROPERTY_OBJECT (Boxed::UID, classID, ClassID)
	PROPERTY_FLAG (flags, 1<<0, isEnabled)

	bool isRunning () const;
	void getSettingsID (String& settingsID) const;

	// Object
	int compare (const Object& obj) const override;

	CLASS_INTERFACE (IServiceDescription, Object)

protected:
	friend class ServiceManager;

	int flags;
	IComponent* instance;
	IClassFactory* classFactory;

	// IServiceDescription
	tbool CCL_API isUserService () const override;
	tbool CCL_API isUserEnabled () const override;
	StringRef CCL_API getServiceName () const override;
	StringRef CCL_API getServiceTitle () const override;
	StringRef CCL_API getServiceDescription () const override;
	UIDRef CCL_API getServiceID () const override;
	IUnknown* CCL_API getServiceInstance () const override;
};

//************************************************************************************************
// ServiceManager
//************************************************************************************************

class ServiceManager: public Object,
					  public IServiceManager,
					  public Singleton<ServiceManager>
{
public:
	DECLARE_CLASS (ServiceManager, Object)
	DECLARE_METHOD_NAMES (ServiceManager)

	ServiceManager ();
	~ServiceManager ();

	Iterator* newIterator () const;

	// IServiceManager
    void CCL_API startup (IProgressNotify* progress = nullptr) override;
	void CCL_API shutdown () override;
	tbool CCL_API canShutdown () const override;
	int CCL_API countServices () const override;
	const IServiceDescription* CCL_API getService (int index) const override;
	tresult CCL_API getInstance (UIDRef cid, UIDRef iid, void** object) const override;
	tresult CCL_API enableService (const IServiceDescription& description, tbool state) override;
	void CCL_API registerNotification (IServiceNotification* notification) override;
	void CCL_API unregisterNotification (IServiceNotification* notification) override;

	using IServiceManager::getInstance;

	CLASS_INTERFACE (IServiceManager, Object)

protected:
	String name;
	ObjectArray services;
	Settings* settings;
	LinkedList<IServiceNotification*> notifications;

	Settings& getSettings ();
	Attributes& getAttributes (ServiceDescription& desc);
	friend class ServiceSettingsSaver;
	void commitSettings ();

	void collectServices (StringRef category, ServiceDescription::Type type);
	tresult startService (ServiceDescription& desc, IProgressNotify* progress = nullptr);
	bool stopService (ServiceDescription& desc);

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

} // namespace CCL

#endif // _ccl_servicemanager_h
