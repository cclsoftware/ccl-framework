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
// Filename    : ccl/gui/system/notificationcenter.h
// Description : Notification Center
//
//************************************************************************************************

#ifndef _ccl_notificationcenter_h
#define _ccl_notificationcenter_h

#include "ccl/public/gui/framework/inotificationcenter.h"
#include "ccl/public/gui/graphics/iimage.h"

#include "ccl/base/singleton.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/collections/objectarray.h"

namespace CCL {

//************************************************************************************************
// Notification
//************************************************************************************************

class Notification: public Object,
					public INotification
{
public:
	DECLARE_CLASS (Notification, Object)

	Notification (const NotificationProperties& properties = NotificationProperties (),
				  const IAttributeList* attributes = nullptr);
	~Notification ();

	// INotification
	StringID CCL_API getID () const override { return id; }
	StringID CCL_API getCategory () const override { return category; }
	StringRef CCL_API getTitle () const override { return title; }
	StringRef CCL_API getBody () const override { return body; }
	const DateTime& CCL_API getIssuedAt () const override { return issuedAt; }
	const DateTime& CCL_API getExpiration () const override { return expiration; }
	NotificationScope CCL_API getScope () const override { return scope; }
	const IAttributeList& CCL_API getAttributes () const override { return attributes; }
	int CCL_API getState () const override { return state; }
	int CCL_API getNumActions () const override { return actions.count (); };
	tresult CCL_API getActionProperties (NotificationActionProperties& properties, int index) const override;
	
	// Internal methods
	void addAction (const NotificationActionProperties& properties);
	void clearActions ();
	void setState (int _state) { state = _state; }
	PROPERTY_READONLY_FLAG (state, kSeen, wasSeen)

	void takeFrom (const Notification& other);

	CLASS_INTERFACE (INotification, Object)

protected:
	MutableCString id;
	MutableCString category;
	String title;
	String body;
	DateTime issuedAt;
	DateTime expiration;
	NotificationScope scope;
	Attributes attributes;
	int state;
	ObjectArray actions;
};

//************************************************************************************************
// NotificationCenter
//************************************************************************************************

class NotificationCenter: public Object,
						  public INotificationCenter,
						  public Singleton<NotificationCenter>
{
public:
	DECLARE_CLASS (NotificationCenter, Object)
	
	NotificationCenter ();
	~NotificationCenter ();
	
	// Alert::IReporter
	void CCL_API reportEvent (const Alert::Event& e) override;
	void CCL_API setReportOptions (Severity minSeverity, int eventFormat) override;

	// INotificationCenter
	INotification* CCL_API createNotification (const NotificationProperties& properties,
											   const IAttributeList* attributes = nullptr,
											   const NotificationActionProperties actionProperties[] = nullptr,
											   int actionCount = 0) const override;
	tresult CCL_API addNotification (INotification* n) override;
	tresult CCL_API removeNotification (INotification* n) override;
	void CCL_API setInAppNotificationIcon (IImage* icon) override;
	INotification* CCL_API sendInAppNotification (StringRef title, StringRef body,
												   const IAttributeList* attributes = nullptr,
												   const NotificationActionProperties actionProperties[] = nullptr,
												   int actionCount = 0) override;
	IUnknownIterator* CCL_API createIterator () const override;
	INotification* CCL_API findNotification (StringID id) const override;
	tresult CCL_API setState (INotification* n, int state) override;
	tresult CCL_API triggerAction (INotification* n, StringID actionId) const override;
	void CCL_API getStats (Stats& stats, StringID category = nullptr) const override;
	tresult CCL_API registerHandler (INotificationActionHandler* handler) override;
	tresult CCL_API unregisterHandler (INotificationActionHandler* handler) override;

	CLASS_INTERFACE2 (IReporter, INotificationCenter, Object)

protected:
	static const String kInAppNotificationPrefix;

	ObjectArray notifications;
	Vector<INotificationActionHandler*> handlers;
	SharedPtr<IImage> inAppNotificationIcon;
	int inAppNotificationCount;

	Notification* find (StringID id) const;
	void statsChanged ();
};

} // namespace CCL

#endif // _ccl_notificationcenter_h
