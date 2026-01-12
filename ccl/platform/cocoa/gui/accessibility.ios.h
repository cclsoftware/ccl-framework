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
// Filename    : ccl/platform/cocoa/gui/accessibility.ios.h
// Description : iOS Accessibility
//
//************************************************************************************************

#ifndef _ccl_accessibility_ios_h
#define _ccl_accessibility_ios_h

#include "ccl/gui/system/accessibility.h"

#include "ccl/platform/cocoa/macutils.h"
#include "ccl/platform/cocoa/cclcocoa.h"

@class CCL_ISOLATED (AccessibilityElement);

namespace CCL {
namespace iOS {

//************************************************************************************************
// iOS::UIAccessibilityElementProvider
//************************************************************************************************

class UIAccessibilityElementProvider: public PlatformAccessibilityProvider
{
public:
	DECLARE_CLASS_ABSTRACT (UIAccessibilityElementProvider, PlatformAccessibilityProvider)

	UIAccessibilityElementProvider (AccessibilityProvider& owner);

	static UIAccessibilityElementProvider* toPlatformProvider (AccessibilityProvider* provider);

	AccessibilityProvider& getOwner () const;
	UIView* getRootView () const;
	CGRect getFrame () const;
	NSArray* getChildren () const;
	UIAccessibilityElement* getElement () const;

	// PlatformAccessibilityProvider
	void disconnect () override;
	void sendPlatformEvent (AccessibilityEvent e) override;
	void onChildProviderAdded (AccessibilityProvider* childProvider) override;
	void onChildProviderRemoved (AccessibilityProvider* childProvider) override;

private:
	NSObj<CCL_ISOLATED (AccessibilityElement)> element;
	NSObj<NSMutableArray> children;
};

//************************************************************************************************
// iOS::UIAccessibilityManager
//************************************************************************************************

class UIAccessibilityManager: public AccessibilityManager
{
public:
	// AccessibilityManager
	PlatformAccessibilityProvider* createPlatformProvider (AccessibilityProvider& provider) override;
};

} // namespace iOS
} // namespace CCL

#endif // _ccl_accessibility_ios_h
