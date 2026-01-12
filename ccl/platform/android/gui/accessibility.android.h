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
// Filename    : ccl/platform/android/gui/accessibility.android.h
// Description : Android Accessibility
//
//************************************************************************************************

#ifndef _ccl_accessibility_android_h
#define _ccl_accessibility_android_h

#include "ccl/gui/system/accessibility.h"

#include "ccl/platform/android/cclandroidjni.h"

#include "ccl/public/collections/map.h"

namespace CCL {
namespace Android {

class FrameworkView;

//************************************************************************************************
// AccessibilityElementProvider
//************************************************************************************************

class AccessibilityElementProvider: public PlatformAccessibilityProvider
{
public:
	DECLARE_CLASS_ABSTRACT (AccessibilityElementProvider, PlatformAccessibilityProvider)

	static constexpr int kProviderNone	= 0;
	static constexpr int kProviderFirst = 1;

	AccessibilityElementProvider (AccessibilityProvider& owner);
	~AccessibilityElementProvider ();

	virtual void fillAccessibilityNodeInfo (jobject view, int virtualViewId, jobject info) const;
	virtual int getVirtualViewAt (PointRef pos) const { return kProviderNone; }

	int getProviderId () const { return providerId; };

	static AccessibilityElementProvider* toPlatformProvider (AccessibilityProvider* provider);
	
	// PlatformAccessibilityProvider
	void disconnect () override;
	void sendPlatformEvent (AccessibilityEvent e) override;
	void onChildProviderAdded (AccessibilityProvider* childProvider) override;
	void onChildProviderRemoved (AccessibilityProvider* childProvider) override;

protected:
	typedef KeyValue<int, AccessibilityElementProvider*> AccessibilityElementProviderPair;

	static Vector<AccessibilityElementProviderPair> providers;
	static int nextProviderId;

	int providerId;

	Vector<SharedPtr<AccessibilityProvider>> children;
	AccessibilityElementProvider* parent;

	float getContentScaleFactor () const;
	FrameworkView* getFrameworkView () const;
};

//************************************************************************************************
// AccessibilityRootElementProvider
//************************************************************************************************

class AccessibilityRootElementProvider: public AccessibilityElementProvider
{
public:
	DECLARE_CLASS_ABSTRACT (AccessibilityRootElementProvider, AccessibilityElementProvider)

	static constexpr int kProviderRoot = -1;

	AccessibilityRootElementProvider (AccessibilityProvider& owner);

	void fillAccessibilityNodeInfo (jobject view, int virtualViewId, jobject info) const override;
	int getVirtualViewAt (PointRef pos) const override;
};

//************************************************************************************************
// AndroidAccessibilityManager
//************************************************************************************************

class AndroidAccessibilityManager: public AccessibilityManager
{
public:
	// AccessibilityManager
	PlatformAccessibilityProvider* createPlatformProvider (AccessibilityProvider& provider) override;
};

} // namespace Android
} // namespace CCL

#endif // _ccl_accessibility_android_h
