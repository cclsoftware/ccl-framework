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
// Filename    : ccl/gui/system/accessibility.h
// Description : Accessibility Classes
//
//************************************************************************************************

#ifndef _ccl_accessibility_h
#define _ccl_accessibility_h

#include "ccl/public/gui/framework/iaccessibility.h"
#include "ccl/public/text/cstring.h"

#include "ccl/base/singleton.h"
#include "ccl/base/collections/objectarray.h"

namespace CCL {
	
class View;
class AccessibilityProvider;

//////////////////////////////////////////////////////////////////////////////////////////////////
// AccessibilityEvent
//////////////////////////////////////////////////////////////////////////////////////////////////

enum class AccessibilityEvent: int32
{
	kValueChanged ///< Value has changed, @see IAccessibilityValueProvider
};

//************************************************************************************************
// PlatformAccessibilityProvider
//************************************************************************************************

class PlatformAccessibilityProvider: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (PlatformAccessibilityProvider, Object)
	
	PlatformAccessibilityProvider (AccessibilityProvider& owner);

	virtual void disconnect () = 0;

	virtual void sendPlatformEvent (AccessibilityEvent e) = 0;

	AccessibilityProvider& getEffectiveProvider () const;
	AccessibilityProvider& getLabelProvider () const;
	AccessibilityProvider& getValueProvider () const;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	AccessibilityProvider& owner;

	virtual void onChildProviderAdded (AccessibilityProvider* childProvider);
	virtual void onChildProviderRemoved (AccessibilityProvider* childProvider);
};

//************************************************************************************************
// AccessibilityProvider
//************************************************************************************************

class AccessibilityProvider: public Object,
							 public AbstractAccessibilityProvider
{
public:
	DECLARE_CLASS_ABSTRACT (AccessibilityProvider, Object)

	AccessibilityProvider ();

	static void getPaginationText (String& text, int currentPage, int totalPages);
	static void getToggleText (String& text, bool isOn);

	template<typename Interface>
	bool hasInterface () { return UnknownPtr<Interface> (asUnknown ()).isValid (); }
	
	PlatformAccessibilityProvider* getPlatformProvider ();
	void sendEvent (AccessibilityEvent e);

	PROPERTY_POINTER (AccessibilityProvider, parentProvider, ParentProvider)
	const ObjectArray& getChildren () const { return children; }
	void addChildProvider (AccessibilityProvider* childProvider);
	void removeChildProvider (AccessibilityProvider* childProvider);
	void disconnect ();

	virtual AccessibilityProvider* findElementProvider (AccessibilityDirection direction) const { return nullptr; }
	virtual AccessibilityProvider* findElementProviderAt (PointRef pos, AccessibilityCoordSpace space) const { return nullptr; }
	virtual AccessibilityProvider* getFocusElementProvider () const { return nullptr; }

	virtual View* getView () const = 0;

	// Object
	void CCL_API signal (MessageRef msg) override;

	CLASS_INTERFACE (IAccessibilityProvider, Object)

protected:
	AutoPtr<PlatformAccessibilityProvider> platformProvider;
	ObjectArray children;

	// IAccessibilityProvider
	IAccessibilityProvider* CCL_API getParentIProvider () const override
	{ return getParentProvider (); }
	const IContainer* CCL_API getChildrenIProvider () const override
	{ return &getChildren (); }
	IAccessibilityProvider* CCL_API findElementIProvider (AccessibilityDirection direction) const override
	{ return findElementProvider (direction); }
	IAccessibilityProvider* CCL_API findElementIProviderAt (PointRef pos, AccessibilityCoordSpace space) const override
	{ return findElementProviderAt (pos, space); }
	IAccessibilityProvider* CCL_API geFocusElementIProvider () const override
	{ return getFocusElementProvider (); }
	IView* CCL_API getIView () const override;
};

//************************************************************************************************
// AccessibilityManager
//************************************************************************************************

class AccessibilityManager: public Object,
							public IAccessibilityManager,
							public ExternalSingleton<AccessibilityManager>
{
public:
	DECLARE_CLASS_ABSTRACT (AccessibilityManager, Object)

	static bool isEnabled ();

	virtual PlatformAccessibilityProvider* createPlatformProvider (AccessibilityProvider& provider);
	
	virtual void shutdown ();
	void registerAccessibleView (View* view, StringID accessibilityId = CString::kEmpty);
	void setViewRelation (View* view, AccessibilityRelation relation, StringID childName);
	AccessibilityProvider* findRelatedProvider (const IAccessibilityProvider* provider, AccessibilityRelation relation) const;
	
	AccessibilityProvider& getEffectiveProvider (AccessibilityProvider& provider) const;
	AccessibilityProvider& getValueProvider (AccessibilityProvider& provider) const;
	AccessibilityProvider& getLabelProvider (AccessibilityProvider& provider) const;

	// IAccessibilityManager
	tbool CCL_API anyAccessibilityClientsListening () const override;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	
	CLASS_INTERFACE (IAccessibilityManager, Object)

protected:
	struct RelationEntry
	{
		AccessibilityRelation relation = AccessibilityRelation::kUndefined;
		MutableCString childId;
	};
	struct ViewEntry
	{
		View* view;
		ISubject* subject;
		MutableCString accessibilityId;
		Vector<RelationEntry> relations;

		ViewEntry (View* view = nullptr, StringID accessibilityId = CString::kEmpty);

		bool operator == (const ViewEntry& other) const
		{
			return other.view == view;
		}
	};
	Vector<ViewEntry> views;
	
	CString getAccessibilityId (View* view) const;
};

} // namespace CCL

#endif // _ccl_accessibility_h
