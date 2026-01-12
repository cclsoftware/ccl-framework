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
// Filename    : ccl/platform/cocoa/gui/accessibility.ios.mm
// Description : iOS Accessibility
//
//************************************************************************************************

#include "ccl/platform/cocoa/gui/accessibility.ios.h"

#include "ccl/gui/views/view.h"

#include "ccl/platform/cocoa/gui/window.ios.h"
#include "ccl/platform/cocoa/gui/nativeview.ios.h"
#include "ccl/platform/cocoa/quartz/cghelper.h"

#include "ccl/public/text/translation.h"

#include "ccl/public/base/ccldefpush.h"

using namespace CCL;
using namespace iOS;

//************************************************************************************************
// AccessibilityElement
//************************************************************************************************

@interface CCL_ISOLATED (AccessibilityElement) : UIAccessibilityElement
{
	UIAccessibilityElementProvider* provider;
	NSMutableArray* children;
}

- (id)initWithProvider:(UIAccessibilityElementProvider*)provider;
- (void)dealloc;

- (void)disconnect;
- (void)addChild:(UIAccessibilityElement*)child;
- (void)removeChild:(UIAccessibilityElement*)child;
- (void)postNotification:(UIAccessibilityNotifications)event argument:(id)a;

// UIAccessibilityElement
- (BOOL)isAccessibilityElement;
- (NSString*)accessibilityLabel;
- (NSString*)accessibilityHint;
- (NSString*)accessibilityValue;
- (CGRect)accessibilityFrame;
- (UIAccessibilityTraits)accessibilityTraits;

// UIAccessibilityAction
- (BOOL)accessibilityActivate;
- (void)accessibilityIncrement;
- (void)accessibilityDecrement;
- (BOOL)accessibilityScroll:(UIAccessibilityScrollDirection)direction;

// UIAccessibilityContainer
- (NSArray*)accessibilityElements;
- (NSInteger)accessibilityElementCount;
- (NSInteger)indexOfAccessibilityElement:(id)element;
- (UIAccessibilityContainerType)accessibilityContainerType;

@end

//************************************************************************************************
// AccessibilityElement
//************************************************************************************************

@implementation CCL_ISOLATED (AccessibilityElement)

- (id)initWithProvider:(UIAccessibilityElementProvider*)_provider
{
	if(!_provider)
		return nil;

	id element = nil;
	if(AccessibilityProvider* parent = _provider->getOwner ().getParentProvider ())
		element = UIAccessibilityElementProvider::toPlatformProvider (parent)->getElement ();
	else
	{
		ASSERT (_provider->getOwner ().getElementRole () == AccessibilityElementRole::kRoot)
		element = _provider->getRootView ();
	}

	if(self = [super initWithAccessibilityContainer:element])
		provider = _provider;

	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)dealloc
{
	if(children)
		[children release];

	[super dealloc];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)disconnect
{
	provider = nil;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)addChild:(UIAccessibilityElement*)child
{
	if(!children)
		children = [[NSMutableArray alloc] initWithCapacity:5];
	[children addObject:child];
	[self postNotification:UIAccessibilityLayoutChangedNotification argument:nil];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)removeChild:(UIAccessibilityElement*)child
{
	ASSERT(children)
	if(children)
		[children removeObject:child];
	[self postNotification:UIAccessibilityLayoutChangedNotification argument:nil];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)isAccessibilityElement
{
	if(!provider)
		return NO;

	AccessibilityElementRole role = provider->getEffectiveProvider ().getElementRole ();
	if(role == AccessibilityElementRole::kRoot || role == AccessibilityElementRole::kGroup || role == AccessibilityElementRole::kList)
		return NO;

	return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSString*)accessibilityLabel
{
	if(!provider)
		return nil;

	String label;
	provider->getLabelProvider ().getElementName (label);
	return [label.createNativeString<NSString*> () autorelease];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSString*)accessibilityHint
{
	if(!provider)
		return nil;

	// this is not yet defined in IAccessibilityProvider

	return nil;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSString*)accessibilityValue
{
	if(!provider)
		return nil;

	if(UnknownPtr<CCL::IAccessibilityValueProvider> valueProvider = provider->getValueProvider ().asUnknown ())
	{
		String value;
		tresult result = valueProvider->getValue (value);
		if(result == kResultOk)
			return [value.createNativeString<NSString*> () autorelease];
	}

	if(UnknownPtr<CCL::IAccessibilityToggleProvider> toggleProvider = provider->getEffectiveProvider ().asUnknown ())
	{
		String label;
		AccessibilityProvider::getToggleText (label, toggleProvider->isToggleOn ());
		return [label.createNativeString<NSString*> () autorelease];
	}

	return nil;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (UIAccessibilityTraits)accessibilityTraits
{
	UIAccessibilityTraits traits = 0;
	if(!provider)
		return traits;

	AccessibilityElementRole role = provider->getEffectiveProvider ().getElementRole ();
	if(role == AccessibilityElementRole::kButton)
		traits |= UIAccessibilityTraitButton;
	else if(role == AccessibilityElementRole::kLabel)
		traits |= UIAccessibilityTraitStaticText;

	if(provider->getEffectiveProvider ().hasInterface<IAccessibilityToggleProvider> ())
		if(@available (iOS 17, *))
			traits |= UIAccessibilityTraitToggleButton;

	int state = provider->getEffectiveProvider ().getElementState ();
	if(state & AccessibilityElementState::kHasFocus) // is this the correct mapping?
		traits |= UIAccessibilityTraitSelected;

	if(UnknownPtr<CCL::IAccessibilityValueProvider> valueProvider = provider->getValueProvider ().asUnknown ())
		if(valueProvider->canIncrement ())
			traits |= UIAccessibilityTraitAdjustable;

	return traits;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (CGRect)accessibilityFrame
{
	if(!provider)
		return CGRectZero;

	return provider->getFrame ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSArray*)accessibilityElements
{
	return children;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSInteger)accessibilityElementCount
{
	if(children)
		return children.count;

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSInteger)indexOfAccessibilityElement:(id)element
{
	if(children)
		return [children indexOfObject:element];
	else
		return NSNotFound;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (UIAccessibilityContainerType)accessibilityContainerType
{
	if(!provider)
		return UIAccessibilityContainerTypeNone;

	AccessibilityElementRole role = provider->getEffectiveProvider ().getElementRole ();
	if(role == AccessibilityElementRole::kList)
		return UIAccessibilityContainerTypeList;

	return UIAccessibilityContainerTypeNone;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)accessibilityActivate
{
	if(!provider)
		return NO;

	if(UnknownPtr<CCL::IAccessibilityToggleProvider> toggleProvider = provider->getEffectiveProvider ().asUnknown ())
		return toggleProvider->toggle () == kResultOk ? YES : NO;

	return NO;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)accessibilityIncrement
{
	if(!provider)
		return;

	if(UnknownPtr<CCL::IAccessibilityValueProvider> valueProvider = provider->getValueProvider ().asUnknown ())
	{
		valueProvider->increment ();
		[self postNotification:UIAccessibilityAnnouncementNotification argument:[self accessibilityValue]];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)accessibilityDecrement
{
	if(!provider)
		return;

	if(UnknownPtr<CCL::IAccessibilityValueProvider> valueProvider = provider->getValueProvider ().asUnknown ())
	{
		valueProvider->decrement ();
		[self postNotification:UIAccessibilityAnnouncementNotification argument:[self accessibilityValue]];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)accessibilityScroll:(UIAccessibilityScrollDirection)direction
{
	if(!provider)
		return NO;

	AccessibilityScrollDirection scrollDirection = AccessibilityScrollDirection::kUndefined;
	switch(direction)
	{
	case UIAccessibilityScrollDirectionRight: scrollDirection = AccessibilityScrollDirection::kRight; break;
	case UIAccessibilityScrollDirectionLeft	: scrollDirection = AccessibilityScrollDirection::kLeft; break;
	case UIAccessibilityScrollDirectionUp	: scrollDirection = AccessibilityScrollDirection::kUp; break;
	case UIAccessibilityScrollDirectionDown	: scrollDirection = AccessibilityScrollDirection::kDown; break;
	}
	if(scrollDirection == AccessibilityScrollDirection::kUndefined)
		return NO;

	if(UnknownPtr<IAccessibilityScrollProvider> scrollProvider = provider->getEffectiveProvider ().asUnknown ())
	{
		if(scrollProvider->scroll (scrollDirection, AccessibilityScrollAmount::kPage) != kResultOk)
			return NO;

		int currentPage = 0;
		int totalPages = 0;
		if(scrollDirection == AccessibilityScrollDirection::kLeft || scrollDirection == AccessibilityScrollDirection::kRight)
		{
			currentPage = scrollProvider->getPagePositionX ();
			totalPages = scrollProvider->countPagesX ();
		}
		else if(scrollDirection == AccessibilityScrollDirection::kUp || scrollDirection == AccessibilityScrollDirection::kDown)
		{
			currentPage = scrollProvider->getPagePositionY ();
			totalPages = scrollProvider->countPagesY ();
		}
		if(totalPages > 0)
		{
			String message;
			AccessibilityProvider::getPaginationText (message, currentPage, totalPages);
			[self postNotification:UIAccessibilityPageScrolledNotification argument:[message.createNativeString<NSString*> () autorelease]];
		}
		return YES;
	}

	return NO;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)postNotification:(UIAccessibilityNotifications)event argument:(id)a
{
	if(UIAccessibilityIsVoiceOverRunning ())
		UIAccessibilityPostNotification (event, a);
}

@end

//************************************************************************************************
// iOS::UIAccessibilityElementProvider
//************************************************************************************************

UIAccessibilityElementProvider* UIAccessibilityElementProvider::toPlatformProvider (AccessibilityProvider* provider)
{
	return provider ? ccl_cast<UIAccessibilityElementProvider> (provider->getPlatformProvider ()) : nil;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (UIAccessibilityElementProvider, PlatformAccessibilityProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

UIAccessibilityElementProvider::UIAccessibilityElementProvider (AccessibilityProvider& owner)
: PlatformAccessibilityProvider (owner)
{
	element = [[CCL_ISOLATED (AccessibilityElement) alloc] initWithProvider:this];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UIAccessibilityElementProvider::disconnect ()
{
	if(element)
		[element disconnect];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UIAccessibilityElementProvider::sendPlatformEvent (AccessibilityEvent e)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UIAccessibilityElementProvider::onChildProviderAdded (AccessibilityProvider* childProvider)
{
	SuperClass::onChildProviderAdded (childProvider);
	if(UIAccessibilityElementProvider* platformChild = toPlatformProvider (childProvider))
		if(element)
			[element addChild:platformChild->getElement ()];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UIAccessibilityElementProvider::onChildProviderRemoved (AccessibilityProvider* childProvider)
{
	SuperClass::onChildProviderRemoved (childProvider);
	if(UIAccessibilityElementProvider* platformChild = toPlatformProvider (childProvider))
		if(element)
			[element removeChild:platformChild->getElement ()];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider& UIAccessibilityElementProvider::getOwner () const
{
	return owner;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIView* UIAccessibilityElementProvider::getRootView () const
{
	View* parentView = owner.getView ();
	if(!parentView)
		return nil;

	IOSWindow* iosWindow = IOSWindow::cast (parentView->getWindow ());
	if(!iosWindow)
		return nil;

	NativeView* nativeView = iosWindow->getNativeView ();
	if(!nativeView)
		return nil;

	return nativeView->getView ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIAccessibilityElement* UIAccessibilityElementProvider::getElement () const
{
	return element;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CGRect UIAccessibilityElementProvider::getFrame () const
{
	Rect rect;
	owner.getElementBounds (rect, AccessibilityCoordSpace::kScreen);
	CGRect frame;
	MacOS::toCGRect (frame, rect);

	return frame;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NSArray* UIAccessibilityElementProvider::getChildren () const
{
	if(!element)
		return nil;

	return [element accessibilityElements];
}

//************************************************************************************************
// iOS::UIAccessibilityManager
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (AccessibilityManager, UIAccessibilityManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

PlatformAccessibilityProvider* UIAccessibilityManager::createPlatformProvider (AccessibilityProvider& provider)
{
	return NEW UIAccessibilityElementProvider (provider);
}
