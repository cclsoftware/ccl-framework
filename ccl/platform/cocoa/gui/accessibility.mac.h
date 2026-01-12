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
// Filename    : ccl/platform/cocoa/gui/accessibility.mac.h
// Description : macOS Accessibility
//
//************************************************************************************************

#ifndef _ccl_accessibility_mac_h
#define _ccl_accessibility_mac_h

#include "ccl/gui/system/accessibility.h"

#include "ccl/platform/cocoa/macutils.h"
#include "ccl/platform/cocoa/cclcocoa.h"

@class CCL_ISOLATED (AccessibilityElement);

namespace CCL {
namespace MacOS {

//************************************************************************************************
// MacOS::NSAccessibilityElementProvider
//************************************************************************************************

class NSAccessibilityElementProvider: public PlatformAccessibilityProvider
{
public:
	DECLARE_CLASS_ABSTRACT (NSAccessibilityElementProvider, PlatformAccessibilityProvider)

	NSAccessibilityElementProvider (AccessibilityProvider& owner);
	
	AccessibilityProvider& getOwner () const;

	static NSAccessibilityElementProvider* toPlatformProvider (AccessibilityProvider* provider);
	
	NSAccessibilityElement* getElement () const;
	NSRect getFrameInScreenCoordinates () const;
	NSRect getFrameInParentSpace (bool isRootView) const;
	void disconnectFromParent (AccessibilityProvider* childProvider);

	// PlatformAccessibilityProvider
	void disconnect () override;
	void sendPlatformEvent (AccessibilityEvent e) override;
	void onChildProviderAdded (AccessibilityProvider* childProvider) override;
	
private:
	AccessibilityProvider& owner;
	NSObj<CCL_ISOLATED (AccessibilityElement)> element;
};

//************************************************************************************************
// MacOS::NSAccessibilityManager
//************************************************************************************************

class NSAccessibilityManager: public AccessibilityManager
{
public:
	// AccessibilityManager
	PlatformAccessibilityProvider* createPlatformProvider (AccessibilityProvider& provider) override;
};

} // namespace MacOS
} // namespace CCL

#endif // _ccl_accessibility_mac_h
