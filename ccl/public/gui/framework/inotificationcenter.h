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
// Filename    : ccl/public/gui/framework/inotificationcenter.h
// Description : Notification Center Interface
//
//************************************************************************************************

#ifndef _ccl_inotificationcenter_h
#define _ccl_inotificationcenter_h

#include "ccl/public/system/alerttypes.h"

#include "ccl/public/text/cstring.h"

namespace CCL {

interface IAttributeList;
interface IUnknownIterator;
interface IImage;

//************************************************************************************************
// NotificationScope
//************************************************************************************************

DEFINE_ENUM (NotificationScope)
{
	kAppNotification,
	kSystemNotification
};

//************************************************************************************************
// NotificationProperties
//************************************************************************************************

struct NotificationProperties
{
	MutableCString id;
	MutableCString category;
	String title;
	String body;
	DateTime issuedAt; // local time
	DateTime expiration; // local time
	NotificationScope scope = kAppNotification;
	int state = 0;
};

//************************************************************************************************
// NotificationActionProperties
//************************************************************************************************

struct NotificationActionProperties
{
	MutableCString id;
	String title;
};

//************************************************************************************************
// INotification
/** Notification interface.
	\ingroup ccl_system */
//************************************************************************************************

interface INotification: IUnknown
{
	virtual StringID CCL_API getID () const = 0;

	virtual StringID CCL_API getCategory () const = 0;

	virtual StringRef CCL_API getTitle () const = 0;

	virtual StringRef CCL_API getBody () const = 0;

	/** Date at which the notification was issued at in local time. */
	virtual const DateTime& CCL_API getIssuedAt () const = 0;

	/** Date at which the notification will expire in local time. */
	virtual const DateTime& CCL_API getExpiration () const = 0;

	virtual NotificationScope CCL_API getScope () const = 0;

	virtual const IAttributeList& CCL_API getAttributes () const = 0;

	static constexpr int kSeen = 1<<0;

	virtual int CCL_API getState () const = 0;

	virtual int CCL_API getNumActions () const = 0;

	virtual tresult CCL_API getActionProperties (NotificationActionProperties& action, int index) const = 0;

	// Notificaton attributes
	DECLARE_STRINGID_MEMBER (kIcon)
	DECLARE_STRINGID_MEMBER (kSubCategory)
	DECLARE_STRINGID_MEMBER (kAlternativeURL)

	DECLARE_IID (INotification)
};

DEFINE_IID (INotification, 0xaa477569, 0x2db1, 0x4923, 0x86, 0x3e, 0x21, 0x1c, 0xb8, 0x76, 0x14, 0x3c)
DEFINE_STRINGID_MEMBER (INotification, kIcon, "icon")
DEFINE_STRINGID_MEMBER (INotification, kSubCategory, "subcategory")
DEFINE_STRINGID_MEMBER (INotification, kAlternativeURL, "alternativeURL")

//************************************************************************************************
// INotificationActionHandler
/** Notification action handler interface.
	\ingroup ccl_system */
//************************************************************************************************

interface INotificationActionHandler: IUnknown
{
	virtual tbool CCL_API canExecute (StringID actionId, const INotification& n) const = 0;

	virtual tresult CCL_API execute (StringID actionId, INotification& n) = 0;

	DECLARE_IID (INotificationActionHandler)
};

DEFINE_IID (INotificationActionHandler, 0xa3cb9e, 0xc097, 0x4b75, 0x90, 0xd5, 0x8f, 0x69, 0x9c, 0x1a, 0xec, 0x2c)

//************************************************************************************************
// INotificationCenter
/** Notification center interface.

	Threading Policy:
	Called from main thread only.
	
	\ingroup ccl_system */
//************************************************************************************************

interface INotificationCenter: Alert::IReporter
{	
	/** Create notification (owned by caller). 
		Optionally a list of actions can be added to the notification.
		The first action in the array will be the default action. */
	virtual INotification* CCL_API createNotification (const NotificationProperties& properties,
													   const IAttributeList* attributes = nullptr,
													   const NotificationActionProperties actionProperties[] = nullptr,
													   int actionCount = 0) const = 0;

	/** Add and display notification (takes ownership). 
		Existing notification with same id will be updated. */
	virtual tresult CCL_API addNotification (INotification* n) = 0;

	/** Remove notification (owned by caller). */
	virtual tresult CCL_API removeNotification (INotification* n) = 0;

	/** Set icon used for in-app notifications. */
	virtual void CCL_API setInAppNotificationIcon (IImage* icon) = 0;

	/** Create and add an in-app notification. */
	virtual INotification* CCL_API sendInAppNotification (StringRef title, StringRef body,
												   const IAttributeList* attributes = nullptr,
												   const NotificationActionProperties actionProperties[] = nullptr,
												   int actionCount = 0) = 0;

	/** Create iterator of existing notifications. */
	virtual IUnknownIterator* CCL_API createIterator () const = 0;

	/** Find existing notification. */
	virtual INotification* CCL_API findNotification (StringID id) const = 0;

	/** Set notification state (seen by user). */
	virtual tresult CCL_API setState (INotification* n, int state) = 0;

	/** Trigger notification action. */
	virtual tresult CCL_API triggerAction (INotification* n, StringID actionId) const = 0;

	struct Stats
	{
		int unseenCount = 0;	///< number of unseen notifications
		int totalCount = 0;		///< total number of notifications

		bool operator == (const Stats& other) const
		{
			return	unseenCount == other.unseenCount &&
					totalCount == other.totalCount;
		}

		bool operator != (const Stats& other) const
		{ return !(*this == other); }
	};

	/** Get notification statistics, can be filtered by category. */
	virtual void CCL_API getStats (Stats& stats, StringID category = nullptr) const = 0;

	/** Register action handler in the notification center. */
	virtual tresult CCL_API registerHandler (INotificationActionHandler* handler) = 0;

	/** Unregister action handler in the notification center. */
	virtual tresult CCL_API unregisterHandler (INotificationActionHandler* handler) = 0;
	
	//////////////////////////////////////////////////////////////////////////////////////////////

	DECLARE_STRINGID_MEMBER (kNotificationAdded)	///< Notification has been added (args[0]: INotification)
	DECLARE_STRINGID_MEMBER (kNotificationRemoved)	///< Notification has been removed (args[0]: INotification)
	DECLARE_STRINGID_MEMBER (kNotificationUpdated)	///< Notification has been updated (args[0]: INotification)
	DECLARE_STRINGID_MEMBER (kNotificationSeen)		///< Notification was seen by user (args[0]: INotification)
	DECLARE_STRINGID_MEMBER (kNotificationsChanged)	///< Notification statistics changed
		
	DECLARE_STRINGID_MEMBER (kInAppNotificationCategory)

	DECLARE_IID (INotificationCenter)
};

DEFINE_IID (INotificationCenter, 0xb1e17031, 0xb529, 0x4a43, 0x90, 0x3a, 0xc1, 0x64, 0x6b, 0x9a, 0xf0, 0xa2)
DEFINE_STRINGID_MEMBER (INotificationCenter, kNotificationAdded, "notificationAdded")
DEFINE_STRINGID_MEMBER (INotificationCenter, kNotificationRemoved, "notificationRemoved")
DEFINE_STRINGID_MEMBER (INotificationCenter, kNotificationUpdated, "notificationUpdated")
DEFINE_STRINGID_MEMBER (INotificationCenter, kNotificationSeen, "notificationSeen")
DEFINE_STRINGID_MEMBER (INotificationCenter, kNotificationsChanged, "notificationsChanged")
DEFINE_STRINGID_MEMBER (INotificationCenter, kInAppNotificationCategory, "Application")

} // namespace CCL

#endif // _ccl_inotificationcenter_h
