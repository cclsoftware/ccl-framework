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
// Filename    : ccl/platform/cocoa/gui/accessibility.mac.mm
// Description : macOS Accessibility
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/views/view.h"
#include "ccl/gui/windows/window.h"

#include "ccl/platform/cocoa/gui/accessibility.mac.h"
#include "ccl/platform/cocoa/gui/nativeview.mac.h"
#include "ccl/platform/cocoa/quartz/nshelper.h"

#include "ccl/public/base/ccldefpush.h"
#include <AppKit/AppKit.h>

using namespace CCL;
using namespace MacOS;

//************************************************************************************************
// AccessibilityScrollbar
//************************************************************************************************

static const AccessibilityScrollDirection kHorizontalOrientation = AccessibilityScrollDirection::kLeft;
static const AccessibilityScrollDirection kVerticalOrientation = AccessibilityScrollDirection::kUp;

@interface CCL_ISOLATED (AccessibilityScrollbar) : NSAccessibilityElement
{
	NSMutableArray* children;
	NSAccessibilityRole assignedRole;
	AccessibilityScrollDirection orientation;
	CCL_ISOLATED (AccessibilityElement)* parent;
}

- (id)initWithParent:(CCL_ISOLATED (AccessibilityElement)*)parent role:(NSAccessibilityRole)role orientation:(AccessibilityScrollDirection)orientation;
- (void)dealloc;

// NSAccessibility protocol
- (NSArray*)accessibilityChildren;
- (BOOL)isAccessibilityEnabled;
- (BOOL)isAccessibilityElement;
- (id)accessibilityParent;
- (id)accessibilityTopLevelUIElement;
- (id)accessibilityWindow;
- (BOOL)accessibilityPerformIncrement;
- (BOOL)accessibilityPerformDecrement;
- (NSAccessibilityRole)accessibilityRole;
- (id)accessibilityValue;
- (id)accessibilityMinValue;
- (id)accessibilityMaxValue;
- (NSAccessibilityOrientation)accessibilityOrientation;
@end


//************************************************************************************************
// AccessibilityElement
//************************************************************************************************

@interface CCL_ISOLATED (AccessibilityElement) : NSAccessibilityElement
{
	NSAccessibilityElementProvider* provider;
	BOOL rootView;
	NSMutableArray* children;
	CCL_ISOLATED (AccessibilityScrollbar)* horizontalScrollbar;
	CCL_ISOLATED (AccessibilityScrollbar)* verticalScrollbar;
	NSMutableArray<NSAccessibilityCustomAction*>* customActions;
}
- (void)dealloc;
- (id)initWithProvider:(NSAccessibilityElementProvider*)provider;

- (void)disconnect;
- (void)addChildToRoot:(NSAccessibilityElement*)childElement;
- (void)removeChild:(NSAccessibilityElement*)childElement;
- (void)removeChildFromRoot:(NSAccessibilityElement*)childElement;
#if DEBUG_LOG
- (NSString*) getRoleString;
#endif
- (BOOL)isRootView;
- (NSWindow*)nsWindow;
- (id)accessibilityScrollBarForOrientation:(AccessibilityScrollDirection)orientation;
- (id)accessibilityScrollValueForOrientation:(AccessibilityScrollDirection)orientation;
- (BOOL)accessibilityShowContextMenu; // registered as custom action
- (BOOL)accessibilityShowContextMenuWithOptions:(BOOL)checkonly;

// NSAccessibility protocol
- (NSArray*)accessibilityChildren;
- (NSAccessibilityRole)accessibilityRole;
- (NSString*)accessibilityLabel;
- (id)accessibilityValue;
- (id)accessibilityParent;
- (id)accessibilityTopLevelUIElement;
- (id)accessibilityWindow;
- (BOOL)isAccessibilityElement;
- (BOOL)isAccessibilityEnabled;
- (BOOL)isAccessibilityFocused;
- (id)isAccessibilityExpanded;
- (NSRect)accessibilityFrame;
- (NSPoint)accessibilityActivationPoint;
- (BOOL)accessibilityPerformPress;
- (BOOL)accessibilityPerformIncrement;
- (BOOL)accessibilityPerformDecrement;
- (BOOL)accessibilityPerformShowMenu;
- (BOOL)accessibilityPerformConfirm;
- (id)accessibilityHorizontalScrollBar;
- (id)accessibilityVerticalScrollBar;
- (NSArray<NSAccessibilityCustomAction*>*)accessibilityCustomActions;
- (BOOL)isAccessibilitySelectorAllowed:(SEL)selector;

// NSAccessibilityElement
- (NSRect)accessibilityFrameInParentSpace;
- (id)accessibilityHitTest:(NSPoint)point;
@end

//************************************************************************************************
// AccessibilityElement
//************************************************************************************************

@implementation CCL_ISOLATED (AccessibilityElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG_LOG
- (NSString*) getRoleString
{
	if(!provider)
		return @"";

	AccessibilityElementRole role = provider->getEffectiveProvider ().getElementRole ();
	
	switch(role)
	{
	case AccessibilityElementRole::kGroup :
		return @"Group";
	case AccessibilityElementRole::kRoot :
		return @"Window";
	case AccessibilityElementRole::kButton :
		return @"Button";
	case AccessibilityElementRole::kList :
		return @"List";
	case AccessibilityElementRole::kLabel :
		return @"StaticText";
	case AccessibilityElementRole::kSlider :
		return @"Slider";
	case AccessibilityElementRole::kComboBox :
		return @"ComboBox";
	case AccessibilityElementRole::kCustom :
		return @"Custom";
	case AccessibilityElementRole::kTextField :
		return @"TextField";
	case AccessibilityElementRole::kTree :
		return @"Tree";
	case AccessibilityElementRole::kDataItem :
		return @"DataItem";
	default :
		CCL_PRINTF ("getRoleString: Unknown role %d\n", role)
		break;
	}
	
	return @"";
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)dealloc
{
	if(children)
	{
		ASSERT (children.count == 0)
		[children release];
	}
	if(customActions)
	{
		[customActions removeAllObjects];
		[customActions release];
	}

	if(horizontalScrollbar)
		[horizontalScrollbar release];
	if(verticalScrollbar)
		[verticalScrollbar release];

	[super dealloc];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)disconnect
{
	provider = nil;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)initWithProvider:(NSAccessibilityElementProvider*)_provider
{
	if(!_provider)
		return nil;
	
	if(self = [super init])
	{
		provider = _provider;

		if(provider->getEffectiveProvider ().getElementRole () == AccessibilityElementRole::kRoot)
			rootView = YES;

		else if(UnknownPtr<IAccessibilityScrollProvider> scroll = provider->getEffectiveProvider ().asUnknown ())
		{
			if(scroll->canScroll(AccessibilityScrollDirection::kLeft))
				horizontalScrollbar = [[CCL_ISOLATED (AccessibilityScrollbar) alloc] initWithParent:self role:NSAccessibilityScrollBarRole orientation:kHorizontalOrientation];
			if(scroll->canScroll(AccessibilityScrollDirection::kUp))
				verticalScrollbar = [[CCL_ISOLATED (AccessibilityScrollbar) alloc] initWithParent:self role:NSAccessibilityScrollBarRole orientation:kVerticalOrientation];
		}
		#if DEBUG_LOG
		if([self isAccessibilityElement])
			NSLog (@"A11y: InitElement: '%@' (%@) at %@, id %@", self.accessibilityLabel, self.getRoleString, NSStringFromPoint (self.accessibilityFrame.origin), self);
		#endif
	}

	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)isRootView
{
	return rootView;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSArray*)accessibilityChildren
{
	return children;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)addChildToRoot:(NSAccessibilityElement*)childElement
{
	if(!provider || !rootView)
		return;

	NSWindow* window = [self nsWindow];
	if(!window)
		return;

	// We cannot call the NSView's accessibilityAddChildElement: method, so we have to manipulate accessibilityChildren array
	[window setAccessibilityChildren:[[window accessibilityChildren] arrayByAddingObject:childElement]];

	NSAccessibilityPostNotification(window, NSAccessibilityLayoutChangedNotification);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)removeChildFromRoot:(NSAccessibilityElement*)childElement
{
	if(!provider || !rootView)
		return;

	NSWindow* window = [self nsWindow];
	if(!window)
		return;

	[window setAccessibilityChildren:[[window accessibilityChildren] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"SELF != %@", childElement]]];

	NSAccessibilityPostNotification(window, NSAccessibilityLayoutChangedNotification);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)addChild:(NSAccessibilityElement*)childElement
{
	if(!children)
		children = [[NSMutableArray alloc] initWithCapacity:5];
	[children addObject:childElement];

	NSAccessibilityPostNotification (self, NSAccessibilityLayoutChangedNotification);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)removeChild:(NSAccessibilityElement*)childElement
{
	ASSERT (children)
	if(children)
		[children removeObject:childElement];
	NSAccessibilityPostNotification (self, NSAccessibilityLayoutChangedNotification);
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

- (NSString*)accessibilityTitle
{
	if(!provider)
		return nullptr;

	String label;
	provider->getEffectiveProvider ().getElementName (label);
	return [label.createNativeString<NSString*> () autorelease];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)accessibilityValue
{
	if(!provider)
		return nullptr;

	if(UnknownPtr<CCL::IAccessibilityValueProvider> valueProvider = provider->getValueProvider ().asUnknown ())
	{
		String value;
		tresult result = valueProvider->getValue (value);
		if(result == kResultOk)
			return [value.createNativeString<NSString*> () autorelease];
	}

	if(UnknownPtr<CCL::IAccessibilityToggleProvider> toggleProvider = provider->getValueProvider ().asUnknown ())
	{
		String label;
		AccessibilityProvider::getToggleText (label, toggleProvider->isToggleOn ());
		return [label.createNativeString<NSString*> () autorelease];
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

// Get the element’s parent in the accessibility hierarchy.
- (id)accessibilityParent
{
	if(!provider)
		return nullptr;

	if(rootView) // Reached the top
		return [self nsWindow];

	id element = nullptr;

	// We need to skip elements that are not taking part in the accessibility hierarchy 'manually' here.
	if(AccessibilityProvider* parent = provider->getOwner ().getParentProvider ())
	{
		AccessibilityElementRole role = NSAccessibilityElementProvider::toPlatformProvider (parent)->getEffectiveProvider ().getElementRole ();
		NSAccessibilityElement* element = NSAccessibilityElementProvider::toPlatformProvider (parent)->getElement ();
		if(element && [element isAccessibilityElement])
			return element;
		else
			return [element accessibilityParent];
	}
	else
	{
		ASSERT (provider->getOwner ().getParentProvider () != nullptr)
		return nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSWindow*) nsWindow
{
	if(!provider)
		return nil;

	if(View* view = provider->getEffectiveProvider ().getView ())
	{
		if(NSView* rootNSView = (NSView*)((view->getWindow ())->getSystemWindow ()))
		{
			return [rootNSView window];
		}
	}

	return nil;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)accessibilityTopLevelUIElement
{
	return [self nsWindow];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)accessibilityWindow
{
	return [self nsWindow];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSAccessibilityRole)accessibilityRole
{
	if(!provider)
		return NSAccessibilityUnknownRole;

	AccessibilityElementRole role = provider->getEffectiveProvider ().getElementRole ();
			
	switch(role)
	{
	case AccessibilityElementRole::kGroup :
		if([self accessibilityScrollBarForOrientation:kHorizontalOrientation] || [self accessibilityScrollBarForOrientation:kVerticalOrientation])
			return NSAccessibilityScrollAreaRole;
		else
			return NSAccessibilityGroupRole;
	case AccessibilityElementRole::kRoot :
		return NSAccessibilityWindowRole;
	case AccessibilityElementRole::kButton :
		return NSAccessibilityButtonRole;
	case AccessibilityElementRole::kList :
		return NSAccessibilityListRole;
	case AccessibilityElementRole::kLabel :
		return NSAccessibilityStaticTextRole;
	case AccessibilityElementRole::kSlider :
		return NSAccessibilitySliderRole;
	case AccessibilityElementRole::kComboBox :
		return NSAccessibilityComboBoxRole;
	case AccessibilityElementRole::kCustom :
		return NSAccessibilityUnknownRole;
	case AccessibilityElementRole::kTextField :
		return NSAccessibilityTextFieldRole;
	case AccessibilityElementRole::kTree:
		return NSAccessibilityListRole;
	case AccessibilityElementRole::kDataItem :
		return NSAccessibilityUnknownRole;
	default :
		CCL_PRINTF ("Accessibility: Unknown AccessibilityElementRole %d !\n", role)
		break;
	}
	
	return NSAccessibilityUnknownRole; // Regarded as warning in accessibility audit
}


//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)isAccessibilityEnabled
{
	if(!provider)
		return NO;

	return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)isAccessibilityElement
{
	if(!provider)
		return NO;

	AccessibilityElementRole role = provider->getEffectiveProvider ().getElementRole ();
	if(role == AccessibilityElementRole::kGroup)
	{
		String label;
		provider->getLabelProvider ().getElementName (label);
		if(!label.isEmpty ())
			return YES;

		return NO;
	}

	if(role == AccessibilityElementRole::kList || role == AccessibilityElementRole::kRoot)
		return NO;

	return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)isAccessibilityFocused
{
	if(!provider)
		return NO;

	AccessibilityElementRole role = provider->getEffectiveProvider ().getElementRole ();
	if(role == AccessibilityElementRole::kGroup || role == AccessibilityElementRole::kList || role == AccessibilityElementRole::kRoot)
		return NO;

	return NO;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)isAccessibilityExpanded
{
	// For some reason this has to be NSNumber although declared differently in documentation

	if(!provider)
		return [NSNumber numberWithFloat:0.0];

	if(UnknownPtr<CCL::IAccessibilityExpandCollapseProvider> expandProvider = provider->getEffectiveProvider ().asUnknown ())
	{
		if(expandProvider->isExpanded ())
			return [NSNumber numberWithFloat:1.0];
	}
	return [NSNumber numberWithFloat:0.0];
}

////////////////////////////////////////////////////////////////////////////////////////////////

- (NSRect)accessibilityFrame
{
	if(!provider || rootView)
		return NSZeroRect;

	return provider->getFrameInScreenCoordinates ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSRect)accessibilityFrameInParentSpace
{
	if(!provider)
		return NSZeroRect;

	return provider->getFrameInParentSpace (rootView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSPoint)accessibilityActivationPoint
{
	NSRect frame = [self accessibilityFrame];
	NSPoint point = frame.origin;
	point.x += frame.size.width * 0.5f;
	point.y += frame.size.height * 0.5f;

	return point;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)accessibilityPerformPick
{
	// TODO: Pick equals hitting Enter in a ComboBox
	CCL_PRINTLN ("Perform Pick!")
	return NO;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)accessibilityPerformShowMenu
{
	return NO;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)accessibilityPerformConfirm
{
	if(!provider)
		return NO;

	AccessibilityElementRole role = provider->getEffectiveProvider ().getElementRole ();
	if(role == AccessibilityElementRole::kComboBox) // VoiceOver triggers Confirm action when hitting VO+Space to open the dialog
	{
		if(UnknownPtr<CCL::IAccessibilityExpandCollapseProvider> expandProvider = provider->getEffectiveProvider ().asUnknown ())
			return expandProvider->expand (true) == kResultOk ? YES : NO;
		#if DEBUG_LOG
		NSLog (@"accessibilityPerformConfirm: Element %@ has no expand provider. Confirm ignored.", [self getRoleString]);
		#endif
	}
	return NO;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)accessibilityPerformPress
{
	if(!provider)
		return NO;

	AccessibilityElementRole role = provider->getEffectiveProvider ().getElementRole ();
	if(role == AccessibilityElementRole::kButton || role == AccessibilityElementRole::kComboBox)
	{
		if(UnknownPtr<CCL::IAccessibilityActionProvider> actionProvider = provider->getEffectiveProvider ().asUnknown ())
			return actionProvider->performAction () == kResultOk ? YES : NO;
		#if DEBUG_LOG
		NSLog (@"accessibilityPerformPress: Element %@ has no ActionProvider. Press ignored.", [self getRoleString]);
		#endif
	}

	return NO;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)accessibilityPerformIncrement
{
	if(!provider)
		return NO;

	tresult result = kResultFailed;
	if(UnknownPtr<CCL::IAccessibilityValueProvider> valueProvider = provider->getValueProvider ().asUnknown ())
	{
		result = valueProvider->increment ();
		NSAccessibilityPostNotification(self, NSAccessibilityValueChangedNotification);
	}
	else if(UnknownPtr<IAccessibilityScrollProvider> scroll = provider->getEffectiveProvider ().asUnknown ())
	{
		if(scroll->canScroll(AccessibilityScrollDirection::kUp))
		   return kResultOk == scroll->scroll (AccessibilityScrollDirection::kUp, AccessibilityScrollAmount::kStep);
		if(scroll->canScroll(AccessibilityScrollDirection::kRight))
		   return kResultOk == scroll->scroll (AccessibilityScrollDirection::kRight, AccessibilityScrollAmount::kStep);
	}

	return result == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)accessibilityPerformDecrement
{
	if(!provider)
		return NO;

	tresult result = kResultFailed;
	if(UnknownPtr<CCL::IAccessibilityValueProvider> valueProvider = provider->getValueProvider ().asUnknown ())
	{
		result = valueProvider->decrement ();
		NSAccessibilityPostNotification(self, NSAccessibilityValueChangedNotification);
	}
	else if(UnknownPtr<IAccessibilityScrollProvider> scroll = provider->getEffectiveProvider ().asUnknown ())
	{
		if(scroll->canScroll(AccessibilityScrollDirection::kDown))
		   return kResultOk == scroll->scroll (AccessibilityScrollDirection::kDown, AccessibilityScrollAmount::kStep);
		if(scroll->canScroll(AccessibilityScrollDirection::kLeft))
		   return kResultOk == scroll->scroll (AccessibilityScrollDirection::kLeft, AccessibilityScrollAmount::kStep);
	}

	return result == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)accessibilityScrollBarForOrientation:(AccessibilityScrollDirection)orientation
{
	if(!provider)
		return nil;

	if(UnknownPtr<IAccessibilityScrollProvider> scroll = provider->getEffectiveProvider ().asUnknown ())
	{
		if(scroll->canScroll (orientation))
		{
			if(orientation == kHorizontalOrientation)
				return horizontalScrollbar;
			if(orientation == kVerticalOrientation)
				return verticalScrollbar;
		}
	}

	if(children)
	{
		for(id element in children)
		{
			if([element accessibilityScrollBarForOrientation:orientation] != nil)
				return [element accessibilityScrollBarForOrientation:orientation];
		}
	}

	return nil;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)accessibilityScrollValueForOrientation:(AccessibilityScrollDirection)orientation
{
	if(!provider)
		return nil;

	if(UnknownPtr<IAccessibilityScrollProvider> scroll = provider->getEffectiveProvider ().asUnknown ())
	{
		if(orientation == kHorizontalOrientation)
			return [NSNumber numberWithFloat:scroll->getNormalizedScrollPositionX ()];

		if(orientation == kVerticalOrientation)
			return [NSNumber numberWithFloat:scroll->getNormalizedScrollPositionY ()];
	}

	return nil;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)accessibilityHorizontalScrollBar
{
	if(!provider)
		return nil;

	if(horizontalScrollbar != nil)
		return horizontalScrollbar;

	return [self accessibilityScrollBarForOrientation:kHorizontalOrientation];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)accessibilityVerticalScrollBar
{
	if(!provider)
		return nil;

	if(verticalScrollbar != nil)
		return verticalScrollbar;

	return [self accessibilityScrollBarForOrientation:kVerticalOrientation];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)accessibilityShowContextMenu
{
	return [self accessibilityShowContextMenuWithOptions:false];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)accessibilityShowContextMenuWithOptions:(BOOL)checkonly
{
	if(!provider)
		return NO;

	if(View* view = provider->getEffectiveProvider ().getView ())
	{
		if(Window* window = view->getWindow ())
		{
			NSPoint nsWhere = [self accessibilityActivationPoint];
			CCL::Point where (nsWhere.x, MacOS::flipCoord (nsWhere.y));
			window->screenToClient (where);
			if(checkonly)
				return window->canPopupContextMenu (where);
			window->popupContextMenu (where);
			return YES;
		}
	}
	return NO;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSArray<NSAccessibilityCustomAction*>*)accessibilityCustomActions;
{
	if(!customActions)
	{
		if([self accessibilityShowContextMenuWithOptions:true])
		{
			customActions = [[NSMutableArray alloc] initWithCapacity:1];
			NSAccessibilityCustomAction *action = [[NSAccessibilityCustomAction alloc] initWithName:@"show menu" target:self selector:@selector(accessibilityShowContextMenu)];
			[customActions addObject:action];
		}
	}
	return customActions;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)isAccessibilitySelectorAllowed:(SEL)selector
{
	if(selector == @selector(accessibilityShowContextMenu))
		return YES;

	return [super isAccessibilitySelectorAllowed:selector];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)accessibilityHitTest:(NSPoint)point
{
	if(!provider)
		return nil;

	NSArray* children = [self accessibilityChildren];

	if(children)
	{
		for(id element in children)
		{
			if(CGRectContainsPoint ([element accessibilityFrame], point))
			{
				id hit = [element accessibilityHitTest:point];
				if(hit)
					return hit;
			}
		}
	}

	if([self isAccessibilityElement])
		return self;
	else
		return nil;
}

@end

//************************************************************************************************
// AccessibilityScrollbar
//************************************************************************************************

@implementation CCL_ISOLATED (AccessibilityScrollbar)

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)initWithParent:(CCL_ISOLATED (AccessibilityElement)*)_parent role:(NSAccessibilityRole)role orientation:(AccessibilityScrollDirection)_orientation
{
	if(self = [super init])
	{
		parent = _parent;
		assignedRole = role;
		orientation = _orientation;

		// If we are the main scrollbar element, assemble a macOS style scrollbar with subroles
		if(assignedRole == NSAccessibilityScrollBarRole)
		{
			children = [[NSMutableArray alloc] initWithCapacity:5];
			[children addObject:[[CCL_ISOLATED (AccessibilityScrollbar) alloc] initWithParent:parent role:NSAccessibilityValueIndicatorRole orientation:orientation]];
			[children addObject:[[CCL_ISOLATED (AccessibilityScrollbar) alloc] initWithParent:parent role:NSAccessibilityIncrementArrowSubrole orientation:orientation]];
			[children addObject:[[CCL_ISOLATED (AccessibilityScrollbar) alloc] initWithParent:parent role:NSAccessibilityDecrementArrowSubrole orientation:orientation]];
			[children addObject:[[CCL_ISOLATED (AccessibilityScrollbar) alloc] initWithParent:parent role:NSAccessibilityIncrementPageSubrole orientation:orientation]];
			[children addObject:[[CCL_ISOLATED (AccessibilityScrollbar) alloc] initWithParent:parent role:NSAccessibilityDecrementPageSubrole orientation:orientation]];
		}
	}

	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)dealloc
{
	if(children)
	{
		[children removeAllObjects];
		[children release];
	}

	[super dealloc];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSArray*)accessibilityChildren
{
	return children;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)isAccessibilityEnabled;
{
	return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)isAccessibilityElement
{
	return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)accessibilityParent
{
	return parent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)accessibilityTopLevelUIElement
{
	return [parent accessibilityTopLevelUIElement];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)accessibilityWindow
{
	return [parent accessibilityWindow];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSAccessibilityRole)accessibilityRole
{
	if(assignedRole == NSAccessibilityScrollBarRole)
		return NSAccessibilityScrollBarRole;

	if(assignedRole == NSAccessibilityIncrementArrowSubrole ||
	   assignedRole == NSAccessibilityDecrementArrowSubrole ||
	   assignedRole == NSAccessibilityIncrementPageSubrole ||
	   assignedRole == NSAccessibilityDecrementPageSubrole)
		return NSAccessibilityButtonRole;

	if(assignedRole ==  NSAccessibilityValueIndicatorRole)
		return NSAccessibilityValueIndicatorRole;

	return NSAccessibilityUnknownRole;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSAccessibilityRole)accessibilitySubrole
{
	return assignedRole;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)accessibilityPerformPress
{
	if(assignedRole == NSAccessibilityIncrementArrowSubrole)
		return [self accessibilityPerformIncrement];
	if(assignedRole == NSAccessibilityDecrementArrowSubrole)
		return [self accessibilityPerformDecrement];

	return NO;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)accessibilityPerformIncrement
{
	ASSERT (parent)
	return [parent accessibilityPerformIncrement];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)accessibilityPerformDecrement
{
	ASSERT (parent)
	return [parent accessibilityPerformDecrement];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)accessibilityValue
{
	if(orientation == kHorizontalOrientation)
		return [parent accessibilityScrollValueForOrientation:kHorizontalOrientation];
	else
		return [parent accessibilityScrollValueForOrientation:kVerticalOrientation];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)accessibilityMinValue
{
	return [NSNumber numberWithFloat:0.0];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)accessibilityMaxValue
{
	return [NSNumber numberWithFloat:1.0];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSAccessibilityOrientation)accessibilityOrientation
{
	if(orientation == kHorizontalOrientation)
		return NSAccessibilityOrientationHorizontal;
	else
		return NSAccessibilityOrientationVertical;
}

@end

//************************************************************************************************
// MacOS::NSAccessibilityElementProvider
//************************************************************************************************

NSAccessibilityElementProvider* NSAccessibilityElementProvider::toPlatformProvider (AccessibilityProvider* provider)
{
	return provider ? ccl_cast<NSAccessibilityElementProvider> (provider->getPlatformProvider ()) : nil;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NSAccessibilityElement* NSAccessibilityElementProvider::getElement () const
{
	return element;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (NSAccessibilityElementProvider, PlatformAccessibilityProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

NSAccessibilityElementProvider::NSAccessibilityElementProvider (AccessibilityProvider& owner)
: PlatformAccessibilityProvider (owner),
  owner (owner)
{
	ASSERT (&owner)
	element = [[CCL_ISOLATED (AccessibilityElement) alloc] initWithProvider:this];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider& NSAccessibilityElementProvider::getOwner () const
{
	return owner;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NSAccessibilityElementProvider::disconnect ()
{

	if(AccessibilityProvider* parentProvider = owner.getParentProvider ())
	{
		if(NSAccessibilityElementProvider* platformParent = toPlatformProvider (parentProvider))
			platformParent->disconnectFromParent (&owner);
	}

	if(element)
		[element disconnect];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NSAccessibilityElementProvider::disconnectFromParent (AccessibilityProvider* childProvider)
{
	if(NSAccessibilityElementProvider* platformChild = toPlatformProvider (childProvider))
	{
		if(element)
		{
			if([element isRootView]) // We reached the top
			{
				if((platformChild->getElement ().isAccessibilityElement))
					[element removeChildFromRoot:platformChild->getElement ()];
				return;
			}

			if([element isAccessibilityElement])
				[element removeChild:platformChild->getElement ()];
			else // We are not an accessibility element -> pass up
			{
				if(AccessibilityProvider* parentProvider = owner.getParentProvider ())
				{
					if(NSAccessibilityElementProvider* platformParent = toPlatformProvider (parentProvider))
						platformParent->disconnectFromParent (childProvider);
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NSAccessibilityElementProvider::sendPlatformEvent (AccessibilityEvent e)
{
	if(e == AccessibilityEvent::kValueChanged)
		NSAccessibilityPostNotification (element, NSAccessibilityValueChangedNotification);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NSRect NSAccessibilityElementProvider::getFrameInScreenCoordinates () const
{
	Rect rect;
	owner.getElementBounds (rect, AccessibilityCoordSpace::kScreen);
	NSRect frame;
	MacOS::toNSRect (frame, rect);
	
	frame.origin.y = MacOS::flipCoord(frame.origin.y);
	frame.origin.y -= frame.size.height;
	return frame;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NSRect NSAccessibilityElementProvider::getFrameInParentSpace (bool isRootView) const
{
	Rect rect;
	Rect parentRect;
	if(AccessibilityProvider* parentProvider = owner.getParentProvider ())
		parentProvider->getElementBounds (parentRect, AccessibilityCoordSpace::kScreen);

	if(isRootView)
	{
		Window* window = getEffectiveProvider ().getView ()->getWindow ();
		window->getFrameSize (parentRect);
	}
	
	owner.getElementBounds (rect, AccessibilityCoordSpace::kScreen);
	
	rect.offset (-parentRect.left, -parentRect.top);
	
	NSRect frame;
	MacOS::toNSRect (frame, rect);
	
	if(isRootView)
		frame.origin.y = MacOS::flipCoord (frame.origin.y);

	return frame;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NSAccessibilityElementProvider::onChildProviderAdded (AccessibilityProvider* childProvider)
{
	SuperClass::onChildProviderAdded (childProvider);
	if(NSAccessibilityElementProvider* platformChild = toPlatformProvider (childProvider))
	{
		if(element)
		{
			if([element isRootView]) // We reached the top
			{
				if((platformChild->getElement ().isAccessibilityElement))
					[element addChildToRoot:platformChild->getElement ()];
				return;
			}

			if([element isAccessibilityElement])
				[element addChild:platformChild->getElement ()];
			else // We are not an accessibility element -> pass up
			{
				if(AccessibilityProvider* parentProvider = owner.getParentProvider ())
				{
					if(NSAccessibilityElementProvider* platformParent = toPlatformProvider (parentProvider))
						platformParent->onChildProviderAdded (childProvider);
				}
			}
		}
	}
}


//************************************************************************************************
// MacOS::NSAccessibilityManager
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (AccessibilityManager, NSAccessibilityManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

PlatformAccessibilityProvider* NSAccessibilityManager::createPlatformProvider (AccessibilityProvider& provider)
{
	return NEW NSAccessibilityElementProvider (provider);
}
