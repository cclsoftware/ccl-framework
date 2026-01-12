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
// Filename    : ccl/app/components/notificationcomponent.h
// Description : Notification Component
//
//************************************************************************************************

#ifndef _ccl_notificationcomponent_h
#define _ccl_notificationcomponent_h

#include "ccl/app/component.h"

#include "ccl/base/collections/objectarray.h"

#include "ccl/public/gui/framework/iitemmodel.h"
#include "ccl/public/gui/framework/inotificationcenter.h"
#include "ccl/public/gui/framework/idleclient.h"

namespace CCL {

//************************************************************************************************
// NotificationComponent
//************************************************************************************************

class NotificationComponent: public Component
{
public:
	DECLARE_CLASS (NotificationComponent, Component)

	NotificationComponent (StringRef name = nullptr);

	PROPERTY_VARIABLE (int, seenIterations, SeenIterations)

	void setNotification (INotification* n);
	INotification* getNotification () const;

	void update ();

	// Component
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API paramChanged (IParameter* param) override;

protected:
	SharedPtr<INotification> notification;

	static String printIssuedAt (const DateTime& issuedAt);
};

//************************************************************************************************
// NotificationListComponent
//************************************************************************************************

class NotificationListComponent: public Component,
								 public ItemViewObserver<AbstractItemModel>,
								 public IdleClient
{
public:
	DECLARE_CLASS (NotificationListComponent, Component)

	NotificationListComponent (StringRef name = nullptr, IObjectFilter* filter = nullptr);

	PROPERTY_MUTABLE_CSTRING (notificationCategory, NotificationCategory)
	PROPERTY_MUTABLE_CSTRING (itemFormName, ItemFormName)
	
	// Component
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API paramChanged (IParameter* param) override;

	// IItemModel
	tbool CCL_API getSubItems (IUnknownList& items, ItemIndexRef index) override;
	void CCL_API viewAttached (IItemView* itemView) override;
	void CCL_API viewDetached (IItemView* itemView) override;

	CLASS_INTERFACE (IItemModel, Component)

protected:
	static constexpr int kCheckSeenInterval = 500;
	static constexpr int kSeenIterationThreshold = 3;
	static constexpr int kMaxNotificationDisplayCount = 99;

	int unseenCount;
	int totalCount;
	ObjectArray notificationComponents;
	AutoPtr<IObjectFilter> filter;

	NotificationComponent* findComponentForNotification (const INotification* n) const;
	void setSeenState () const;

	void addNotification (INotification* notification);
	void updateCount ();

	void onNotificatonAdded (MessageRef m);
	void onNotificationRemoved (MessageRef m);
	void onNotificationUpdatedSeen (MessageRef m);
	void onNotificationsChanged (MessageRef m);

	// IdleClient
	void onIdleTimer () override;
};

} // namespace CCL

#endif // _ccl_notificationcomponent_h
