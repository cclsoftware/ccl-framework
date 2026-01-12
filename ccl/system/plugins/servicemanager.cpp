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
// Filename    : ccl/system/plugins/servicemanager.cpp
// Description : Service Manager
//
//************************************************************************************************

#include "ccl/system/plugins/servicemanager.h"
#include "ccl/system/plugins/plugmanager.h"

#include "ccl/base/storage/settings.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/system/ilogger.h"
#include "ccl/public/plugins/icomponent.h"
#include "ccl/public/plugins/iclassfactory.h"
#include "ccl/public/system/cclsafety.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

namespace CCL {

//************************************************************************************************
// ServiceSettingsSaver
//************************************************************************************************

class ServiceSettingsSaver: public SettingsSaver
{
public:
	// SettingsSaver
	void restore (Settings&) override
	{
		// nothing here
	}

	void flush (Settings&) override
	{
		ServiceManager::instance ().commitSettings ();
	}
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Plug-in Service APIs
////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IServiceManager& CCL_API System::CCL_ISOLATED (GetServiceManager) ()
{
	return ServiceManager::instance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
////////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("ServiceManager")
	XSTRING (StartingService, "Starting %(1)...")
	XSTRING (ServiceFailed, "%(1) failed!")
END_XSTRINGS

//************************************************************************************************
// ServiceDescription
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ServiceDescription, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ServiceDescription::ServiceDescription (Type type)
: type (type),
  priority (1000),
  flags (0),
  instance (nullptr),
  classFactory (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ServiceDescription::~ServiceDescription ()
{
	ASSERT (instance == nullptr)
	if(instance)
		instance->release ();

	ASSERT (classFactory == nullptr)
	if(classFactory)
		classFactory->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ServiceDescription::compare (const Object& obj) const
{
	const ServiceDescription& other = (const ServiceDescription&)obj;
	int prioDiff = priority - other.getPriority ();
	if(prioDiff)
		return prioDiff;

	return name.compare (other.getName ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ServiceDescription::isRunning () const
{
	return instance != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ServiceDescription::getSettingsID (String& settingsID) const
{
	classID.toString (settingsID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API ServiceDescription::getServiceName () const
{
	return getName ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API ServiceDescription::getServiceTitle () const
{
	return getTitle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API ServiceDescription::getServiceDescription () const
{
	return getDescription ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ServiceDescription::isUserService () const
{
	return isUser ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ServiceDescription::isUserEnabled () const
{
	return isEnabled ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIDRef CCL_API ServiceDescription::getServiceID () const
{
	return getClassID ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ServiceDescription::getServiceInstance () const
{
	return instance;
}

//************************************************************************************************
// ServiceManager
//************************************************************************************************

DEFINE_CLASS (ServiceManager, Object)
DEFINE_CLASS_NAMESPACE (ServiceManager, NAMESPACE_CCL)
DEFINE_SINGLETON (ServiceManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

ServiceManager::ServiceManager ()
: name (CCLSTR ("Services")),
  settings (nullptr)
{
	services.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ServiceManager::~ServiceManager ()
{
	safe_release (settings);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* ServiceManager::newIterator () const
{
	return services.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ServiceManager::countServices () const
{
	return services.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IServiceDescription* CCL_API ServiceManager::getService (int index) const
{
	return (ServiceDescription*)services.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ServiceManager::getInstance (UIDRef cid, UIDRef iid, void** object) const
{
	ForEach (services, ServiceDescription, s)
		if(cid.isValid ()) // filter class
		{
			if(s->classID != cid)
				continue;
		}

		if(s->instance)
			if(s->instance->queryInterface (iid, object) == kResultOk)
			{
				reinterpret_cast<IUnknown*> (*object)->release (); // hmm... get rule???
				return kResultOk;
			}
	EndFor

	*object = nullptr;
	return kResultClassNotFound;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ServiceManager::enableService (const IServiceDescription& _description, tbool state)
{
	ServiceDescription* desc = unknown_cast<ServiceDescription> (&_description);
	if(desc == nullptr)
		return kResultInvalidArgument;

	ASSERT (desc->isUser ())
	if(!desc->isUser ())
		return kResultFailed;

	desc->isEnabled (state != 0);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ServiceManager::registerNotification (IServiceNotification* notification)
{
	notifications.append (notification);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ServiceManager::unregisterNotification (IServiceNotification* notification)
{
	notifications.remove (notification);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Settings& ServiceManager::getSettings ()
{
	if(settings == nullptr)
	{
		settings = NEW XmlSettings (name);
		settings->isPlatformSpecific (true);
		settings->isAutoSaveEnabled (true);
		settings->isBackupEnabled (true);
		settings->enableSignals (true);
		settings->restore ();
		settings->addSaver (NEW ServiceSettingsSaver);
	}
	return *settings;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes& ServiceManager::getAttributes (ServiceDescription& desc)
{
	String pathID;
	desc.getSettingsID (pathID);
	return getSettings ().getAttributes (pathID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ServiceManager::collectServices (StringRef category, ServiceDescription::Type type)
{
	ForEachPlugInClass (category, desc)
		ServiceDescription* s = NEW ServiceDescription (type);
		s->setName (desc.getName ());
		s->setClassID (desc.getClassID ());

		Variant priority;
		if(desc.getClassAttribute (priority, Meta::kServicePriority))
			s->setPriority (priority.asInt ());

		String title;
		desc.getLocalizedName (title);
		s->setTitle (title);

		String description;
		desc.getLocalizedDescription (description);
		s->setDescription (description);

		bool enabled = true;
		if(type == ServiceDescription::kUser)
			getAttributes (*s).getBool (enabled, "enabled");
		s->isEnabled (enabled);

		services.addSorted (s);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ServiceManager::startup (IProgressNotify* progress)
{
	// *** Collect Services ***
	collectServices (PLUG_CATEGORY_USERSERVICE, ServiceDescription::kUser);
	collectServices (PLUG_CATEGORY_PROGRAMSERVICE, ServiceDescription::kProgram);
	collectServices (PLUG_CATEGORY_FRAMEWORKSERVICE, ServiceDescription::kFramework);
	
	if(progress)
		progress->updateProgress (0.);
	int count = services.count ();
	int current = 0;

	// ** Start Services ***
	ForEach (services, ServiceDescription, s)
		current++;
		if(s->isEnabled () && ccl_safety_check (s->getClassID ()))
			startService (*s, progress);

		if(progress)
            progress->updateProgress (double(current) / (double)count);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ServiceManager::shutdown ()
{
	// ** Stop Services (in reverse order!) ***
	ForEachReverse (services, ServiceDescription, s)
		if(s->isRunning ())
			stopService (*s);
	EndFor

	if(settings)
		settings->flush (); // commitSettings() should be called by saver
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ServiceManager::commitSettings ()
{
	getSettings ().removeAll ();
	ForEach (services, ServiceDescription, s)
		if(!s->isUser ())
			continue;
		Attributes& a = getAttributes (*s);
		a.set ("friendlyName", s->getName ()); // hint for editing the settings xml file manually
		a.set ("enabled", s->isEnabled ());
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ServiceManager::canShutdown () const
{
	ForEach (services, ServiceDescription, s)
		if(s->instance)
		{
			bool canExit = s->instance->canTerminate () != 0;
			if(!canExit)
				return false;
		}
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ServiceManager::startService (ServiceDescription& desc, IProgressNotify* progress)
{
	if(desc.isRunning ())
		return kResultOk;

	// *** Activatation Notification ***
	if(!notifications.isEmpty ())
	{
		ListForEach (notifications, IServiceNotification*, n)
			if(n->onServiceNotification (desc, IServiceNotification::kServiceActivate) == kResultFailed)
				return kResultFailed;
		EndFor
	}

	if(!desc.isHidden ())
	{
		String message;
		message.appendFormat (XSTR (StartingService), desc.getTitle ());
		System::GetLogger ().reportEvent (message);
		if(progress)
			progress->setProgressText (message);		
	}

	// *** Instanciate IComponent ***
	IComponent* instance = ccl_new<IComponent> (desc.getClassID ());
	if(!instance)
		return kResultFailed;

	// *** Init IComponent ***
	tresult result = instance->initialize (nullptr/*no context!*/);
	if(result != kResultOk)
	{
		instance->terminate ();
		ccl_release (instance);

		if(!desc.isHidden ())
		{
			String message;
			message.appendFormat (XSTR (ServiceFailed), desc.getTitle ());
			System::GetLogger ().reportEvent (Alert::Event (message, Alert::kError));
		}
		return result;
	}

	// *** Register Classes ***
	UnknownPtr<IClassFactory> classFactory (instance);
	if(classFactory)
	{
		if(PlugInManager::instance ().registerFactory (classFactory) == kResultOk)
			desc.classFactory = classFactory;
	}

	desc.instance = instance;

	// *** Startup Notification ***
	if(!notifications.isEmpty ())
	{
		ListForEach (notifications, IServiceNotification*, n)
			n->onServiceNotification (desc, IServiceNotification::kServiceStarted);
		EndFor
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ServiceManager::stopService (ServiceDescription& desc)
{
	if(!desc.isRunning ())
		return true;

	bool canExit = desc.instance->canTerminate () ? true : false;
	ASSERT (canExit == true)

	// must be checked earlier!!
	//if(!canExit)
	//	return false;

	// *** Shutdown Notification ***
	if(!notifications.isEmpty ())
	{
		ListForEach (notifications, IServiceNotification*, n)
			n->onServiceNotification (desc, IServiceNotification::kServiceStopped);
		EndFor
	}

	// *** Unregister Classes ***
	if(desc.classFactory)
	{
		PlugInManager::instance ().unregisterFactory (desc.classFactory);
		desc.classFactory = nullptr;
	}

	// *** Terminate IComponent ***
	desc.instance->terminate ();

	ccl_release (desc.instance);
	desc.instance = nullptr;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ServiceManager)
	DEFINE_METHOD_ARGS ("getInstance", "cid")
END_METHOD_NAMES (ServiceManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ServiceManager::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "getInstance")
	{
		UID cid;
		if(UnknownPtr<IBoxedUID> boxedID = msg[0].asUnknown ())
			boxedID->copyTo (cid);
		else
			cid.fromString (msg[0].asString ());

		IObject* obj = getInstance<IObject> (cid);
		returnValue.takeShared (obj);
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}
