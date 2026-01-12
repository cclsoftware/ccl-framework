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
// Filename    : ccl/app/components/notificationcomponent.cpp
// Description : Notification Component
//
//************************************************************************************************

#include "ccl/app/components/notificationcomponent.h"

#include "ccl/app/params.h"

#include "ccl/base/message.h"
#include "ccl/base/boxedtypes.h"
#include "ccl/base/storage/attributes.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/gui/framework/idropbox.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/graphics/iimage.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/system/formatter.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum NotificationComponentTags
	{
		kNotificationTitle = 100,
		kNotificationBody,
		kNotificationSeen,
		kNotificationIcon,
		kNotificationURL,
		kNotificationIssuedAt,
		kNotificationRemove,
		kNotificationAction = 1000 // indexed
	};

	enum NotificationListComponentTags
	{
		kUnseenNotificationCount = 100,
		kTotalNotificationCount,
		kDeleteNotifications
	};
}

//************************************************************************************************
// NotificationComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (NotificationComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

NotificationComponent::NotificationComponent (StringRef name)
: Component (name),
  seenIterations (0)
{
	paramList.addString ("title", Tag::kNotificationTitle);
	paramList.addString ("body", Tag::kNotificationBody);
	paramList.addParam ("seen", Tag::kNotificationSeen);
	paramList.addImage ("icon", Tag::kNotificationIcon);
	paramList.addString ("url", Tag::kNotificationURL);
	paramList.addString ("issuedAt", Tag::kNotificationIssuedAt);
	paramList.addParam ("remove", Tag::kNotificationRemove);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NotificationComponent::setNotification (INotification* n)
{
	if(notification != n)
	{
		notification = n;
		update ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NotificationComponent::update ()
{
	UnknownPtr<IImageProvider> iconProvider (paramList.byTag (Tag::kNotificationIcon));
	if(notification)
	{
		paramList.byTag (Tag::kNotificationTitle)->fromString (notification->getTitle ());
		paramList.byTag (Tag::kNotificationBody)->fromString (notification->getBody ());
		paramList.byTag (Tag::kNotificationSeen)->setValue (get_flag (notification->getState (), INotification::kSeen));
		paramList.byTag (Tag::kNotificationIssuedAt)->fromString (printIssuedAt (notification->getIssuedAt ()));
		
		AttributeReadAccessor reader (notification->getAttributes ());
		iconProvider->setImage (UnknownPtr<IImage> (reader.getUnknown (INotification::kIcon)));
		paramList.byTag (Tag::kNotificationURL)->fromString (reader.getString (INotification::kAlternativeURL));

		if(paramList.getParamArrayCount ("action") != notification->getNumActions ())
		{
			paramList.removeArray ("action");
			paramList.removeArray ("actionTitle");
			for(int i = 0; i < notification->getNumActions (); i++)
			{
				NotificationActionProperties properties;
				notification->getActionProperties (properties, i);
				paramList.addIndexedParam ("action", NEW Parameter, Tag::kNotificationAction + i);
				paramList.addIndexedParam ("actionTitle", NEW StringParam)->fromString (properties.title);
			}
		}
	}
	else
	{
		paramList.byTag (Tag::kNotificationTitle)->fromString (String::kEmpty);
		paramList.byTag (Tag::kNotificationBody)->fromString (String::kEmpty);
		paramList.byTag (Tag::kNotificationSeen)->setValue (false);
		paramList.byTag (Tag::kNotificationIssuedAt)->fromString (String::kEmpty);
		iconProvider->setImage (nullptr);
		paramList.byTag (Tag::kNotificationURL)->fromString (String::kEmpty);
		paramList.removeArray ("action");
		paramList.removeArray ("actionTitle");
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NotificationComponent::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "numActions")
	{
		var = notification ? notification->getNumActions () : 0;
		return true;
	}
	MutableCString indexString;
	if(propertyId.getBetween (indexString, "actionTitle[", "]"))
	{
		int index = 0;
		if(indexString.getIntValue (index))
		{
			IParameter* titleParam = paramList.getIndexedParam ("actionTitle", index);
			if(titleParam)
			{
				var.fromString (titleParam->getValue ().asString ());
				return true;
			}
		}
	}
	if(propertyId == "hasUrl")
	{
		if(notification)
		{
			AttributeReadAccessor reader (notification->getAttributes ());
			var = !reader.getString (INotification::kAlternativeURL).isEmpty ();
		}
		else
			var = false;
		return true;
	}
	if(propertyId == "category")
	{
		String category;
		if(notification)
			category = String (notification->getCategory ());
		var.fromString (category);
		return true;
	}
	
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NotificationComponent::paramChanged (IParameter* param)
{
	if(notification)
	{
		if(param->getTag () == Tag::kNotificationRemove)
			System::GetNotificationCenter ().removeNotification (notification);
		else
		{
			int index = param->getTag () - Tag::kNotificationAction;
			NotificationActionProperties properties;
			if(notification->getActionProperties (properties, index) == kResultOk)
			{
				System::GetNotificationCenter ().triggerAction (notification, properties.id);
				return true;
			}
		}
	}

	return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INotification* NotificationComponent::getNotification () const
{
	return notification;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String NotificationComponent::printIssuedAt (const DateTime& issuedAt)
{
	String result;
	String issuedTimeString = String ().appendFormat ("%int(1:2):%int(2:2)", issuedAt.getTime ().getHour (), issuedAt.getTime ().getMinute ());
	int64 now = UnixTime::getTime ();
	int64 issued = UnixTime::fromLocal (issuedAt);
	if(now >= issued && ((now - issued) < DateTime::kSecondsInDay * 7))
		result = Format::TimeAgo::print (issuedAt);
	else
		result = Format::DateTime::print (issuedAt, Format::DateTime::kDate);
	result << " " << issuedTimeString;
	return result;
}

//************************************************************************************************
// NotificationListComponent
//************************************************************************************************

static DEFINE_ARRAY_COMPARE (SortByIssued, NotificationComponent, lhs, rhs)
	INotification* ln = lhs->getNotification ();
	INotification* rn = rhs->getNotification ();
	if(ln && rn)
	{
		Boxed::DateTime lt (ln->getIssuedAt ());
		Boxed::DateTime rt (rn->getIssuedAt ());
		return lt.compare (rt);
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (NotificationListComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

NotificationListComponent::NotificationListComponent (StringRef name, IObjectFilter* filter)
: Component (name),
  unseenCount (0),
  totalCount (0),
  filter (filter)
{
	notificationComponents.objectCleanup (true);
	
	paramList.addString ("unseenCount", Tag::kUnseenNotificationCount)->setReadOnly (true);
	paramList.addString ("totalCount", Tag::kTotalNotificationCount)->setReadOnly (true);
	paramList.addParam ("deleteNotifications", Tag::kDeleteNotifications);

	addObject ("Notifications", this);
	
	setItemFormName (MutableCString (getName ()).append (IDropBox::kItemSuffix));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NotificationListComponent::initialize (IUnknown* context)
{
	UnknownPtr<ISubject> notificationCenter (&System::GetNotificationCenter ());
	signalSlots.advise (notificationCenter, INotificationCenter::kNotificationAdded,
						this, &NotificationListComponent::onNotificatonAdded);
	signalSlots.advise (notificationCenter, INotificationCenter::kNotificationRemoved,
						this, &NotificationListComponent::onNotificationRemoved);
	signalSlots.advise (notificationCenter, INotificationCenter::kNotificationUpdated,
						this, &NotificationListComponent::onNotificationUpdatedSeen);
	signalSlots.advise (notificationCenter, INotificationCenter::kNotificationSeen,
						this, &NotificationListComponent::onNotificationUpdatedSeen);
	signalSlots.advise (notificationCenter, INotificationCenter::kNotificationsChanged, 
						this, &NotificationListComponent::onNotificationsChanged);

	bool added = false;
	ForEachUnknown (System::GetNotificationCenter (), unk)
		UnknownPtr<INotification> notification = unk;
		if(notification)
		{
			addNotification (notification);
			added = true;
		}
	EndFor
	if(added)
		updateCount ();

	return SuperClass::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NotificationListComponent::terminate ()
{
	UnknownPtr<ISubject> notificationCenter (&System::GetNotificationCenter ());
	signalSlots.unadvise (notificationCenter);

	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NotificationComponent* NotificationListComponent::findComponentForNotification (const INotification* n) const
{
	return notificationComponents.findIf<NotificationComponent> ([n] (const NotificationComponent& nc)
		{
			return nc.getNotification () == n;
		});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NotificationListComponent::setSeenState () const
{
	for(auto notificationComponent : iterate_as<NotificationComponent> (notificationComponents))
		if(notificationComponent->getSeenIterations () > kSeenIterationThreshold)
		{
			auto notification = notificationComponent->getNotification ();
			if(notification && !get_flag (notification->getState (), INotification::kSeen))
				System::GetNotificationCenter ().setState (notification, notification->getState () | INotification::kSeen);
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NotificationListComponent::addNotification (INotification* notification)
{
	ASSERT (notification)

	if(filter.isValid () && !filter->matches (notification))
		return;

	auto nc = NEW NotificationComponent;
	nc->setNotification (notification);
	notificationComponents.addSorted (nc, &SortByIssued, true);
	signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NotificationListComponent::updateCount ()
{
	auto printCount = [] (int count) -> String
	{
		String result;
		if(count <= kMaxNotificationDisplayCount)
			result.appendIntValue (count);
		else
			result.appendIntValue (kMaxNotificationDisplayCount).append ("+");
		return result;
	};
	
	INotificationCenter::Stats stats;
	System::GetNotificationCenter ().getStats (stats, notificationCategory);
	unseenCount = stats.unseenCount;
	totalCount = stats.totalCount;
	paramList.byTag (Tag::kUnseenNotificationCount)->setValue (printCount (unseenCount));
	paramList.byTag (Tag::kTotalNotificationCount)->setValue (printCount (totalCount));
	signal (Message (kPropertyChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NotificationListComponent::onNotificatonAdded (MessageRef m)
{
	UnknownPtr<INotification> n (m[0].asUnknown ());
	addNotification (n);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NotificationListComponent::onNotificationRemoved (MessageRef m)
{
	UnknownPtr<INotification> n (m[0].asUnknown ());
	ASSERT (n)
	if(auto nc = findComponentForNotification (n))
	{
		notificationComponents.remove (nc);
		nc->release ();
		signal (Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NotificationListComponent::onNotificationUpdatedSeen (MessageRef m)
{
	UnknownPtr<INotification> n (m[0].asUnknown ());
	ASSERT (n)
	if(auto nc = findComponentForNotification (n))
		nc->update ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NotificationListComponent::onNotificationsChanged (MessageRef m)
{
	updateCount ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NotificationListComponent::getSubItems (IUnknownList& items, ItemIndexRef index)
{
	for(auto nc : notificationComponents)
		items.add (nc->asUnknown (), true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NotificationListComponent::viewAttached (IItemView* itemView)
{
	ItemViewObserver<AbstractItemModel>::viewAttached (itemView);
	startTimer (kCheckSeenInterval);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NotificationListComponent::viewDetached (IItemView* itemView)
{
	stopTimer ();
	setSeenState ();
	ItemViewObserver<AbstractItemModel>::viewDetached (itemView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API NotificationListComponent::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == MutableCString ("Notifications").append (IDropBox::kItemSuffix))
	{
		return getTheme ()->createView (getItemFormName (), data.asUnknown ());
	}
	return SuperClass::createView (name, data, bounds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NotificationListComponent::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "hasNotifications")
	{
		var = totalCount != 0;
		return true;
	}
	else if(propertyId == "hasUnseenNotifications")
	{
		var = unseenCount != 0;
		return true;
	}
	
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NotificationListComponent::paramChanged (IParameter* param)
{
	if(param->getTag () == Tag::kDeleteNotifications)
	{
		Vector<INotification*> deletedNotifications;
		for(auto nc : iterate_as<NotificationComponent> (notificationComponents))
			deletedNotifications.add (nc->getNotification ());
		for(auto notification : deletedNotifications)
			System::GetNotificationCenter ().removeNotification (notification);
	}
	
	return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NotificationListComponent::onIdleTimer ()
{
	for(auto itemView : getItemViews ())
	{
		UnknownPtr<IView> view (itemView);
		if(view)
		{
			ViewBox viewBox (view);
			Rect viewRect;
			viewBox.getVisibleClient (viewRect);
			int index = 0;
			for(auto notificationComponent : iterate_as<NotificationComponent> (notificationComponents))
			{
				Rect itemRect;
				itemView->getItemRect (itemRect, ItemIndex (index));
				if(viewRect.intersect (itemRect))
					notificationComponent->setSeenIterations (notificationComponent->getSeenIterations () + 1);
				index++;
			}
		}
	}
}
