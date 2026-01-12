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
// Filename    : ccl/gui/system/notificationcenter.cpp
// Description : Notification Center
//
//************************************************************************************************

#include "ccl/gui/system/notificationcenter.h"

#include "ccl/base/message.h"

#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/guiservices.h"

namespace CCL {

//************************************************************************************************
// NotificationAction
//************************************************************************************************

class NotificationAction: public Object
{
public:
	DECLARE_CLASS (NotificationAction, Object)
	
	NotificationAction (StringID id = nullptr, StringRef title = nullptr)
	: id (id),
	  title (title)
	{}
	
	PROPERTY_MUTABLE_CSTRING (id, Id)
	PROPERTY_STRING (title, Title)
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// GUI Services API
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT INotificationCenter& CCL_API System::CCL_ISOLATED (GetNotificationCenter) ()
{
	return NotificationCenter::instance ();
}

//************************************************************************************************
// Notification
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Notification, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Notification::Notification (const NotificationProperties& properties, const IAttributeList* _attributes)
: id (properties.id),
  category (properties.category),
  title (properties.title),
  body (properties.body),
  issuedAt (properties.issuedAt),
  expiration (properties.expiration),
  scope (properties.scope),
  state (properties.state)
{
	actions.objectCleanup (true);

	if(_attributes)
		attributes.addFrom (*_attributes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Notification::~Notification ()
{
	signal (Message (kDestroyed));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Notification::takeFrom (const Notification& other)
{
	id = other.id;
	category = other.category;
	title = other.title;
	body = other.body;
	issuedAt = other.issuedAt;
	expiration = other.expiration;
	scope = other.scope;
	state = other.state;
	attributes.removeAll ();
	attributes.addFrom (other.attributes);
	actions.removeAll ();
	actions.add (other.actions, Container::kClone);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Notification::getActionProperties (NotificationActionProperties& properties, int index) const
{
	if(auto notificationAction = ccl_cast<NotificationAction> (actions.at (index)))
	{
		properties.id = notificationAction->getId ();
		properties.title = notificationAction->getTitle ();
		return kResultOk;
	}
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Notification::addAction (const NotificationActionProperties& properties)
{
	actions.add (NEW NotificationAction (properties.id, properties.title));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Notification::clearActions ()
{
	actions.removeAll ();
}

//************************************************************************************************
// NotificationCenter
//************************************************************************************************

DEFINE_CLASS_HIDDEN (NotificationCenter, Object)
DEFINE_SINGLETON (NotificationCenter)
const String NotificationCenter::kInAppNotificationPrefix = "inappnotification_";

//////////////////////////////////////////////////////////////////////////////////////////////////

NotificationCenter::NotificationCenter ()
: inAppNotificationCount (0)
{
	notifications.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NotificationCenter::~NotificationCenter ()
{
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NotificationCenter::reportEvent (const Alert::Event& e)
{
	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return;

	NotificationProperties p;
	p.title = e.message;
	p.issuedAt = e.time;
	addNotification (NEW Notification (p));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NotificationCenter::setReportOptions (Severity minSeverity, int eventFormat)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

INotification* CCL_API NotificationCenter::createNotification (const NotificationProperties& properties,
															   const IAttributeList* attributes,
															   const NotificationActionProperties actionProperties[],
															   int actionCount) const
{
	ASSERT (actionCount == 0 || actionProperties != nullptr)
	if(actionCount > 0 && actionProperties == nullptr)
		return nullptr;
	
	auto notification = NEW Notification (properties, attributes);
	for(int i = 0; i < actionCount; i++)
		notification->addAction (actionProperties [i]);
	return notification;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NotificationCenter::addNotification (INotification* _n)
{
	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return kResultWrongThread;

	auto n = unknown_cast<Notification> (_n);
	ASSERT (n)
	if(!n)
		return kResultInvalidPointer;
	
	// check for update
	if(!n->getID ().isEmpty ())
		if(auto existing = find (n->getID ()))
		{
			existing->takeFrom (*n);
			signal (Message (kNotificationUpdated, existing->asUnknown ()));
			n->release ();
			return kResultOk;
		}

	notifications.add (n);
	signal (Message (kNotificationAdded, n->asUnknown ()));

	statsChanged ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NotificationCenter::removeNotification (INotification* _n)
{
	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return kResultWrongThread;
	
	auto n = unknown_cast<Notification> (_n);
	ASSERT (n)
	if(!n)
		return kResultInvalidPointer;

	if(!notifications.remove (n))
		return kResultInvalidArgument;

	signal (Message (kNotificationRemoved, n->asUnknown ()));
	statsChanged ();
	n->release ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NotificationCenter::setInAppNotificationIcon (IImage* icon)
{
	inAppNotificationIcon = icon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INotification* CCL_API NotificationCenter::sendInAppNotification (StringRef title, StringRef body,
														   const IAttributeList* attributes, 
														   const NotificationActionProperties actionProperties[], 
														   int actionCount)
{
	int currentcount = inAppNotificationCount++;
	
	NotificationProperties properties;
	properties.id = kInAppNotificationPrefix;
	properties.id.appendFormat ("%d", currentcount);
	
	DateTime now;
	System::GetSystem ().getLocalTime (now);
	properties.issuedAt = now;
	
	properties.category = kInAppNotificationCategory;

	properties.title = title;
	properties.body = body;
	
	Attributes notificationAttributes;
	if(inAppNotificationIcon)
		notificationAttributes.setAttribute (INotification::kIcon, Variant (inAppNotificationIcon), Attributes::kShare);
	if(attributes)
		notificationAttributes.addFrom (*attributes);

	AutoPtr<INotification> notification = createNotification (properties, &notificationAttributes, actionProperties, actionCount);
	if(notification == nullptr)
		return nullptr;
	tresult result = addNotification (notification);
	return result == kResultOk ? notification.detach () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API NotificationCenter::createIterator () const
{
	return notifications.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Notification* NotificationCenter::find (StringID id) const
{
	for(auto n : iterate_as<Notification> (notifications))
		if(n->getID () == id)
			return n;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INotification* CCL_API NotificationCenter::findNotification (StringID id) const
{
	return find (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NotificationCenter::setState (INotification* _n, int state)
{
	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return kResultWrongThread;

	auto n = unknown_cast<Notification> (_n);
	ASSERT (n)
	if(!n)
		return kResultInvalidPointer;

	if(state != n->getState ())
	{
		n->setState (state);
		signal (Message (kNotificationSeen, n->asUnknown ()));
		statsChanged ();
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NotificationCenter::triggerAction (INotification* n, StringID actionId) const
{
	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return kResultWrongThread;

	ASSERT (n && !actionId.isEmpty ())
	if(n == nullptr || actionId.isEmpty ())
		return kResultInvalidArgument;

	INotificationActionHandler* actionHandler = nullptr;
	for(auto handler : handlers)
		if(handler->canExecute (actionId, *n))
		{
			actionHandler = handler;
			break;
		}

	if(actionHandler == nullptr)
		return kResultFailed;

	return actionHandler->execute (actionId, *n);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NotificationCenter::getStats (Stats& stats, StringID category) const
{
	stats = Stats ();
	for(auto n : iterate_as<Notification> (notifications))
		if(category.isEmpty () || n->getCategory () == category)
		{
			if(!n->wasSeen ())
				stats.unseenCount++;
			stats.totalCount++;
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NotificationCenter::registerHandler (INotificationActionHandler* handler)
{
	ASSERT (handler && !handlers.contains (handler))
	if(handlers.contains (handler))
		return kResultInvalidArgument;

	handlers.add (handler);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NotificationCenter::unregisterHandler (INotificationActionHandler* handler)
{
	ASSERT (handler && handlers.contains (handler))
	if(!handlers.contains (handler))
		return kResultInvalidArgument;
	
	handlers.remove (handler);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NotificationCenter::statsChanged ()
{
	deferSignal (NEW Message (kNotificationsChanged));
}

//************************************************************************************************
// NotificationAction
//************************************************************************************************

DEFINE_CLASS (NotificationAction, Object)
