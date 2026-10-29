//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/cocoa/gui/inputmethod.mac.mm
// Description : macOS Input Method
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/system/inputmethod.h"
#include "ccl/gui/views/view.h"

#include "ccl/gui/keyevent.h"

#include "ccl/platform/cocoa/macutils.h"
#include "ccl/platform/cocoa/quartz/nshelper.h"
#include "ccl/platform/cocoa/gui/window.mac.h"
#include "ccl/platform/cocoa/gui/nativeview.mac.h"

#include "ccl/public/base/ccldefpush.h"
#include <AppKit/AppKit.h>

namespace CCL {

class MacInputMethod;

} // namespace CCL

//************************************************************************************************
// TextInputClient
//************************************************************************************************

@interface CCL_ISOLATED (TextInputClient): NSView<NSTextInputClient>
{
	CCL::MacInputMethod* inputMethod;
	CCL::Window* window;
	CCL::ITextInputClient* client;

	int caret;
	NSRange markedRange;
}

- (id)initWithInputMethod:(CCL::MacInputMethod*)inputMethod;
- (void)selectionChanged;
- (void)visibleRectChanged;

@end

namespace CCL {

//************************************************************************************************
// MacInputMethod
//************************************************************************************************

class MacInputMethod: public Object,
					  public IInputMethod
{
public:
	MacInputMethod (ITextInputClient* client);
	~MacInputMethod ();

	ITextInputClient* getClient () const { return client; }
	Window* getWindow () const { return window; }

	// IInputMethod
	tresult CCL_API enable (tbool state) override;
	void CCL_API selectionChanged () override;
	void CCL_API textChanged () override;
	void CCL_API visibleRectChanged () override;

	CLASS_INTERFACE (IInputMethod, Object)

protected:
	SharedPtr<ITextInputClient> client;
	NSObj<CCL_ISOLATED (TextInputClient)> nativeClient;
	OSXWindow* window;
	NSWindow* nativeWindow;
	bool enabled;
};

//************************************************************************************************
// MacInputMethodManager
//************************************************************************************************

class MacInputMethodManager: public InputMethodManager
{
public:
	// InputMethodManager
	IInputMethod* createInputMethod (ITextInputClient& client) override;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// TextInputClient
//************************************************************************************************

@implementation CCL_ISOLATED (TextInputClient)

- (id)initWithInputMethod:(MacInputMethod*)_inputMethod
{
	if(!_inputMethod)
		return nil;

	if(self = [super init])
	{
		inputMethod = _inputMethod;
		client = _inputMethod->getClient ();
		window = _inputMethod->getWindow ();
		caret = 0;
		markedRange = NSMakeRange (NSNotFound, 0);
	}

	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)selectionChanged
{
	int oldcaret = caret;
	int unused = 0;
	client->getSelection (caret, unused, ITextInputClient::kSelectionOriginal);
	CCL_PRINTF ("selectionChanged: caret (%d)\n", caret)

	if(caret != oldcaret)
		[self resetPreedit];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)visibleRectChanged
{
	[self.inputContext invalidateCharacterCoordinates];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)acceptsFirstResponder
{
	return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)keyDown:(NSEvent*)event
{
	// Hand the event to NSTextInputContext which drives the active IME.
	// The IME will call back into NSTextInputClient methods below.
	BOOL handled = [self.inputContext handleEvent:event];
	if(!handled)
		[super keyDown:event];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (nullable NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range actualRange:(nullable NSRangePointer)actualRange
{
	CCL_PRINTF ("attributedSubstringForProposedRange location=%d length=%d\n", (int)range.location, (int)range.length)

	NSRange safeRange = NSIntersectionRange (range, NSMakeRange (0, client->getTextLength ()));
	if(actualRange)
		*actualRange = safeRange;
	if(safeRange.length == 0)
		return nil;

	String string;
	client->getText (string, (int)range.location, (int)range.length);
	NSString* nativeString = [string.createNativeString<NSString*> () autorelease];
	NSAttributedString* attributedString = [[[NSAttributedString alloc] initWithString:nativeString] autorelease];
	actualRange = &range;

	return attributedString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSUInteger)characterIndexForPoint:(NSPoint)point
{
	// this appears to be never called
	ASSERT (false)
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)doCommandBySelector:(nonnull SEL)selector
{
	#if DEBUG_LOG
	NSLog (@"doCommandBySelector: %@", NSStringFromSelector (selector));
	#endif
	NSEvent* event = [NSApp currentEvent];
	NSEventType eventType = [event type];
	if(eventType == NSEventTypeKeyDown || eventType == NSEventTypeKeyUp)
	{
		KeyEvent key;
		VKey::fromSystemEvent (key, SystemEvent (event));
		switch(key.eventType)
		{
		case KeyEvent::kKeyDown :
			window->onKeyDown (key);
			break;
		case KeyEvent::kKeyUp :
			window->onKeyUp (key);
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(nullable NSRangePointer)actualRange
{
	// ignore requested range, always use caret rect
	CCL::Rect rect;
	client->getCaretRect (rect);
	if(actualRange)
		*actualRange = range;

	CCL::Point origin = rect.getLeftTop ();
	if(const IView* view = client->getInputView ())
	{
		view->clientToScreen (origin);
		rect.moveTo (origin);
	}

	NSRect frame;
	MacOS::toNSRect (frame, rect);
	frame.origin.y = MacOS::flipCoord (frame.origin.y);
	frame.origin.y -= frame.size.height;

	return frame;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)hasMarkedText
{
	return markedRange.location != NSNotFound;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)insertText:(nonnull id)text replacementRange:(NSRange)replacementRange
{
	CCL_PRINTF ("insertText replacementRange (%d, %d)\n", replacementRange.location, replacementRange.length)
	[self resetPreedit];
	NSString* insertString = [text isKindOfClass:[NSAttributedString class]] ? [(NSAttributedString*)text string] : (NSString*)text;
	String string;
	string.appendNativeString (insertString);
	if(replacementRange.location != NSNotFound) // replace instead of insert
	{
		int offset = (int)replacementRange.location - caret;
		client->deleteText (offset < 0 ? -offset : 0, offset > 0 ? offset : 0);
		if(offset < 0)
		{
			caret += offset;
			client->setSelection (caret, caret);
		}
	}
	client->applyText (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSRange)markedRange
{
	return markedRange;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSRange)selectedRange
{
	CCL_PRINTF ("selectedRange　(%d, %d)\n", caret, 0)
	return NSMakeRange (caret, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)setMarkedText:(nonnull id)text selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange
{
	NSString* insertString = [text isKindOfClass:[NSAttributedString class]] ? [(NSAttributedString*)text string] : (NSString*)text;
	if([insertString length] == 0 || [insertString isEqualToString:@" "])
	{
		[self resetPreedit];
		return;
	}

	String preedit;
	preedit.appendNativeString (insertString);
	#if DEBUG_LOG
	MutableCString prededitstr (preedit);
	CCL_PRINTF ("setMarkedText selectedRange \"%s\" (%d, %d) replacementRange (%d, %d)\n", prededitstr.str (), selectedRange.location, selectedRange.length, replacementRange.location, replacementRange.length)
	#endif

	client->setPreeditText (preedit);
	int caretPosition = (int)(caret + selectedRange.location);
	client->setSelection (caretPosition, caretPosition);
	markedRange = NSMakeRange (caret, [insertString length]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)unmarkText
{
	[self resetPreedit];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSArray<NSAttributedStringKey>*)validAttributesForMarkedText
{
	return @[];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)resetPreedit
{
	client->setPreeditText (String::kEmpty);
	markedRange = NSMakeRange (NSNotFound, 0);
	[self.inputContext discardMarkedText];
}

@end

//************************************************************************************************
// MacInputMethodManager
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (InputMethodManager, MacInputMethodManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

IInputMethod* MacInputMethodManager::createInputMethod (ITextInputClient& client)
{
	return NEW MacInputMethod (&client);
}

//************************************************************************************************
// MacInputMethod
//************************************************************************************************

MacInputMethod::MacInputMethod (ITextInputClient* client)
: client (client),
  window (nil),
  enabled (false)
{
	const IView* iView = client->getInputView ();
	if(View* view = unknown_cast<View> (iView))
		if(window = OSXWindow::cast (view->getWindow ()))
			if(NativeView* nativeView = window->getNativeView ())
				if(NSView* macView = nativeView->getView ())
					nativeWindow = [macView window];
	ASSERT (nativeWindow)
	if(nativeWindow == nil)
		return;

	nativeClient = [[CCL_ISOLATED (TextInputClient) alloc] initWithInputMethod:this];
	[[nativeWindow contentView] addSubview:nativeClient];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MacInputMethod::~MacInputMethod ()
{
	if(nativeClient)
		[nativeClient removeFromSuperview];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MacInputMethod::enable (tbool state)
{
	enabled = state;
	if(state)
	{
		window->setSuppressKeyEquivalents (true);
		[nativeWindow makeFirstResponder:nativeClient];
	}
	else
	{
		window->setSuppressKeyEquivalents (false);

		if([nativeWindow firstResponder] == nativeClient.ref)
		{
			if(NativeView* nativeView = window->getNativeView ())
				[nativeWindow makeFirstResponder:nativeView->getView ()];
		}
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MacInputMethod::textChanged ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MacInputMethod::selectionChanged ()
{
	// a click into the input view makes the window's view under the mouse first responder
	if(enabled && [nativeWindow firstResponder] != nativeClient.ref)
		[nativeWindow makeFirstResponder:nativeClient];

	[nativeClient selectionChanged];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MacInputMethod::visibleRectChanged ()
{
	[nativeClient visibleRectChanged];
}
