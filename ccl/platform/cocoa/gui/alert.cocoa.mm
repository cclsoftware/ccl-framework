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
// Filename    : ccl/platform/cocoa/gui/alert.cocoa.mm
// Description : Platform alert
//
//************************************************************************************************

#include "ccl/platform/cocoa/cclcocoa.h"

#include "ccl/platform/cocoa/macutils.h"
#include "ccl/gui/dialogs/alert.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/gui.h"
#include "ccl/base/asyncoperation.h"

using namespace CCL;

extern void modalStateEnter ();
extern void modalStateLeave ();

namespace CCL {

//************************************************************************************************
// CocoaAlertBox
//************************************************************************************************

class CocoaAlertBox: public AlertBox
{
public:
	DECLARE_CLASS (CocoaAlertBox, AlertBox)

	// AlertBox
	void closePlatform () override;
	IAsyncOperation* runAsyncPlatform () override;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// Alert::ButtonMapping
//************************************************************************************************

int Alert::ButtonMapping::getResultAtButtonIndex (int buttonIndex) const
{
	if(otherResult != kUndefined) switch(buttonIndex)
	{
	case 0 : return alternateResult;	// left
	case 1 : return otherResult;		// middle
	case 2 : return defaultResult;		// right
	}
	else switch(buttonIndex)
	{
	case 0 : return alternateResult; // left
	case 1 : return defaultResult;   // right
	}	
	return kUndefined;
}

//************************************************************************************************
// CocoaAlertBox
//************************************************************************************************

DEFINE_CLASS (CocoaAlertBox, AlertBox)
DEFINE_CLASS_UID (CocoaAlertBox, 0x9bf3ecb5, 0x5bb2, 0x4eb4, 0xaa, 0xac, 0x29, 0xaf, 0xf4, 0x66, 0x45, 0xa5) // ClassID::AlertBox

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaAlertBox::closePlatform ()
{
	[NSApp abortModal];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CocoaAlertBox::runAsyncPlatform ()
{
	Window* progress = Desktop.getTopWindow (kDialogLayer); 
	if(progress && progress->getStyle ().isCustomStyle (Styles::kWindowBehaviorProgressDialog))
		progress->hide ();

	NSAlert* alert = [[NSAlert alloc] init];	
	[alert setMessageText:[getTitle ().createNativeString<NSString*> () autorelease]];
	[alert setInformativeText: [getText ().createNativeString<NSString*> () autorelease]];
	if(alertType == Alert::kError)
		[alert setAlertStyle:NSAlertStyleCritical];
	
	auto getKeyEquivalent = [] (StringRef buttonText)
	{
		if(buttonText == AlertService::instance ().getButtonTitle (Alert::kCancel))
			return @"\x1b"; // escape
		else
			return @"";
	};
	
	// add buttons from right to left
	if(NSString *buttonTitle = getThirdButton ().createNativeString<NSString*> ())
	{
		if(buttonTitle.length > 0)
		{
			NSButton* button = [alert addButtonWithTitle:buttonTitle];
			[button setTag:getButtonResult (2)];
			NSString* key = getKeyEquivalent (getThirdButton ());
			if([key length] > 0)
				[button setKeyEquivalent:key];
		}
		[buttonTitle release];
	}
	if(NSString *buttonTitle = getSecondButton ().createNativeString<NSString*> ())
	{
		if(buttonTitle.length > 0)
		{
			NSButton* button = [alert addButtonWithTitle:buttonTitle];
			[button setTag:getButtonResult (1)];
			NSString* key = getKeyEquivalent (getSecondButton ());
			if([key length] > 0)
				[button setKeyEquivalent:key];
		}
		[buttonTitle release];
	}
	if(NSString *buttonTitle = getFirstButton ().createNativeString<NSString*> ())
	{
		if(buttonTitle.length > 0)
		{
			NSButton* button = [alert addButtonWithTitle:buttonTitle];
			[button setTag:getButtonResult (0)];
			NSString* key = getKeyEquivalent (getFirstButton ());
			if([key length] > 0)
				[button setKeyEquivalent:key];
		}
		[buttonTitle release];
	}

	GUI.resetCursor ();
	GUI.hideTooltip ();

	modalStateEnter ();
	
	int itemHit = 0;
	@try
	{
		itemHit = (int)[alert runModal];
	}
	@catch (NSException* exception)
	{
		ASSERT (0);
	}

	modalStateLeave ();
	[alert release];
	
	if(progress && progress->getStyle ().isCustomStyle (Styles::kWindowBehaviorProgressDialog))
		progress->show ();

	return AsyncOperation::createCompleted (itemHit);
}
