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
// Filename    : ccl/platform/cocoa/gui/textbox.cocoa.mm
// Description : platform-specific text control implementation
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/controls/editbox.h"
#include "ccl/gui/keyevent.h"

#include "ccl/base/message.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/text/cclstring.h"

#include "ccl/platform/cocoa/gui/window.mac.h"
#include "ccl/platform/cocoa/gui/nativeview.mac.h"
#include "ccl/platform/cocoa/quartz/nshelper.h"

#include "core/platform/cocoa/macosversion.h"

#include "ccl/public/base/ccldefpush.h"

namespace CCL {

//************************************************************************************************
// CocoaTextControl
//************************************************************************************************

class CocoaTextControl: public NativeTextControl
{
public:
	CocoaTextControl (Control& owner, const Rect& clientRect, int returnKeyType, int keyboardType);
	~CocoaTextControl ();

	bool changed;

	// NativeTextControl
	void updateText ();
	void getControlText (String& string);
	void setSelection (int start, int length);
	void setScrollPosition (PointRef where);
	Point getScrollPosition () const;
	void setSize (RectRef clientRect);
	void updateVisualStyle ();

protected:
	id textField;
	id delegate;
};

} // namespace CCL

using namespace CCL;
using namespace MacOS;

//************************************************************************************************
// CustomTextFieldDelegate
//************************************************************************************************

@interface CCL_ISOLATED (CustomTextFieldDelegate) : NSObject<NSTextFieldDelegate>
{
	CocoaTextControl* _nativeControl;
	NSUndoManager* _undoManager;
}
- (id)init:(CocoaTextControl*)c;
- (NSUndoManager*)undoManagerForTextView:(NSTextView*)aTextView;

@end

//////////////////////////////////////////////////////////////////////////////////////////////////

@implementation CCL_ISOLATED (CustomTextFieldDelegate)

- (id)init:(CocoaTextControl*)c
{
	if(self = [super init])
	{
		_nativeControl = c;
		_undoManager = [[NSUndoManager alloc] init];
	}
	
	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)dealloc
{
	if(_undoManager)
	{
		[_undoManager release];
		_undoManager = nil;
	}
	[super dealloc];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)control:(NSControl*)control textView:(NSTextView*)fieldEditor doCommandBySelector:(SEL)commandSelector
{
	return [self textView:fieldEditor doCommandBySelector:commandSelector];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)textView:(NSTextView*)textView doCommandBySelector:(SEL)commandSelector
{
	NSEvent* currentEvent = NSApp.currentEvent;
	int keyState = 0;
	if(currentEvent.modifierFlags & NSEventModifierFlagShift)
		keyState |= KeyState::kShift;
	if(currentEvent.modifierFlags & NSEventModifierFlagControl)
		keyState |= KeyState::kControl;
	if(currentEvent.modifierFlags & NSEventModifierFlagOption)
		keyState |= KeyState::kOption;
	if(currentEvent.modifierFlags & NSEventModifierFlagCommand)
		keyState |= KeyState::kCommand;

	if(commandSelector == @selector(insertNewline:)
	|| commandSelector == @selector(insertNewlineIgnoringFieldEditor:)) // this is the case when the Return key is pressed with Option (and possibly other modifiers)
	{
		if(! _nativeControl->handleKeyDown (KeyEvent (KeyEvent::kKeyDown, VKey::kReturn, 0, 0, keyState)))
			[textView insertText:@"\n" replacementRange:NSMakeRange ([[textView string] length], 1)];
		return YES;
	}
	else if(commandSelector == @selector(insertTab:))
	{
		if(! _nativeControl->handleKeyDown (KeyEvent (KeyEvent::kKeyDown, VKey::kTab, 0, 0, keyState)))
			[textView insertText:@"\t" replacementRange:NSMakeRange ([[textView string] length], 1)];
		return YES;
	}
	else if(commandSelector == @selector(insertBacktab:))
	{
		keyState |= KeyState::kShift;
		return _nativeControl->handleKeyDown (KeyEvent (KeyEvent::kKeyDown, VKey::kTab, 0, 0, keyState));
	}
	else if(commandSelector == @selector(deleteBackward:))
		return _nativeControl->handleKeyDown (KeyEvent (KeyEvent::kKeyDown, VKey::kBackspace, 0, 0, keyState));
	else if(commandSelector == @selector(moveUp:))
		return _nativeControl->handleKeyDown (KeyEvent (KeyEvent::kKeyDown, VKey::kUp, 0, 0, keyState));
	else if(commandSelector == @selector(moveDown:))
		return _nativeControl->handleKeyDown (KeyEvent (KeyEvent::kKeyDown, VKey::kDown, 0, 0, keyState));
	else if(commandSelector == @selector(pageUp:) || commandSelector == @selector(scrollPageUp:))
		return _nativeControl->handleKeyDown (KeyEvent (KeyEvent::kKeyDown, VKey::kPageUp, 0, 0, keyState));
	else if(commandSelector == @selector(pageDown:) || commandSelector == @selector(scrollPageDown:))
		return _nativeControl->handleKeyDown (KeyEvent (KeyEvent::kKeyDown, VKey::kPageDown, 0, 0, keyState));
	else if(commandSelector == @selector(cancelOperation:))
		return _nativeControl->handleKeyDown (KeyEvent (KeyEvent::kKeyDown, VKey::kEscape, 0, 0, keyState));
	else
		return NO;

	return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)textDidChange:(NSNotification*)aNotification
{
	// calling controlTextDidChange for the multiline case (CustomTextView delegate)
	[self controlTextDidChange:aNotification];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)controlTextDidChange:(NSNotification*)aNotification
{
	_nativeControl->changed = true;

	if(_nativeControl->isImmediateUpdate ())
		(NEW Message ("checkSubmit"))->post (_nativeControl);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSUndoManager *)undoManagerForTextView:(NSTextView*)aTextView;
{
	return _undoManager;
}

@end

//************************************************************************************************
// CustomTextView
//************************************************************************************************

@interface CCL_ISOLATED (CustomTextView) : NSTextView
{
	@public
	NSScrollView* parent;
}
+ (NSMenu*)defaultMenu;
- (void)setStringValue:(NSString *)stringValue;
- (NSString*)stringValue;
- (void)setSelection:(int)start length:(int)length;

@end

//////////////////////////////////////////////////////////////////////////////////////////////////

@implementation CCL_ISOLATED (CustomTextView)

//////////////////////////////////////////////////////////////////////////////////////////////////

+ (NSMenu*)defaultMenu
{
	NSMenu* contextMenu = [NSTextView defaultMenu];
	if(contextMenu)
	{
		NSMutableArray* suppressedItems = [NSMutableArray arrayWithCapacity:2];
		for(NSMenuItem* item in [contextMenu itemArray])
		 	if([item hasSubmenu])
				for(NSMenuItem* subItem in [[item submenu] itemArray])
					if([subItem action] == @selector(changeLayoutOrientation:) || [subItem action] == @selector(addFontTrait:))
					{
						[suppressedItems addObject:item];
						break;
					}

		for(NSMenuItem* submenu in suppressedItems)
			[contextMenu removeItem:submenu];
	}

    return contextMenu;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)initWithFrame:(NSRect)frameRect
{
	if(self = [super initWithFrame:frameRect])
		parent = nil;
	
	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)setStringValue:(NSString*)stringValue
{
	[self setString:stringValue];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSString*)stringValue
{
	return [self string];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)performKeyEquivalent:(NSEvent*)event
{
	NSString* key = [event charactersIgnoringModifiers];
	NSEventModifierFlags modifiers = [event modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask;
	if(modifiers == NSEventModifierFlagCommand)
	{
		// The command key is the ONLY modifier key being pressed.
		if([key isEqualToString:@"x"])
			return [NSApp sendAction:@selector(cut:) to:[[self window] firstResponder] from:self];
		else if([key isEqualToString:@"c"])
			return [NSApp sendAction:@selector(copy:) to:[[self window] firstResponder] from:self];
		else if([key isEqualToString:@"v"])
			return [NSApp sendAction:@selector(paste:) to:[[self window] firstResponder] from:self];
		else if([key isEqualToString:@"a"])
			return [NSApp sendAction:@selector(selectAll:) to:[[self window] firstResponder] from:self];
		else if([key isEqualToString:@"z"])
		{
			[self.undoManager undo];
			return YES;
		}
		else if([key isEqualToString:@"y"])
		{
			[self.undoManager redo];
			return YES;
		}
	}
	
	if(modifiers == (NSEventModifierFlagCommand | NSEventModifierFlagShift))
	{
		if([key isEqualToString:@"Z"])
		{
			[self.undoManager redo];
			return YES;
		}
	}
	
	return [super performKeyEquivalent:event];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)setSelection:(int)start length:(int)length
{
	[self setSelectedRange:NSMakeRange (0,0)];
	if(length >= 0)
		[self setSelectedRange:NSMakeRange (start, length)];
}

@end

//************************************************************************************************
// CustomTextField
//************************************************************************************************

@interface CCL_ISOLATED (CustomTextField) : NSTextField
{}

- (NSDictionary*)selectedTextAttributes;
- (void)setSelectedTextAttributes:(NSDictionary*)attributes;
- (void)setSelection:(int)start length:(int)length;
- (NSTextView*)fieldEditor;
- (void)setTextColor:(NSColor*)textColor;

@end

//////////////////////////////////////////////////////////////////////////////////////////////////

@implementation CCL_ISOLATED (CustomTextField)

- (BOOL)performKeyEquivalent:(NSEvent*)event
{
	NSString* key = [event charactersIgnoringModifiers];
	NSEventModifierFlags modifiers = [event modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask;
	if(modifiers == NSEventModifierFlagCommand)
	{
		// The command key is the ONLY modifier key being pressed.
		if([key isEqualToString:@"x"])
			return [NSApp sendAction:@selector(cut:) to:[[self window] firstResponder] from:self];
		else if([key isEqualToString:@"c"])
			return [NSApp sendAction:@selector(copy:) to:[[self window] firstResponder] from:self];
		else if([key isEqualToString:@"v"])
			return [NSApp sendAction:@selector(paste:) to:[[self window] firstResponder] from:self];
		else if([key isEqualToString:@"a"])
			return [NSApp sendAction:@selector(selectAll:) to:[[self window] firstResponder] from:self];
		else if([key isEqualToString:@"z"])
		{
			[[[[self window] firstResponder] undoManager] undo];
			return YES;
		}
		else if([key isEqualToString:@"y"])
		{
			[[[[self window] firstResponder] undoManager] redo];
			return YES;
		}
	}

	if(modifiers == (NSEventModifierFlagCommand | NSEventModifierFlagShift))
	{
		if([key isEqualToString:@"Z"])
		{
			[[[[self window] firstResponder] undoManager] redo];
			return YES;
		}
	}
	
    return [super performKeyEquivalent:event];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)isOpaque
{
	return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSTextView*)fieldEditor
{
	return (NSTextView *)[[self window] fieldEditor:YES forObject:self];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSDictionary*)selectedTextAttributes
{
	NSTextView* editor = [self fieldEditor];
	return [editor selectedTextAttributes];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)setSelectedTextAttributes:(NSDictionary*)attributes
{
	NSTextView* editor = [self fieldEditor];
	[editor setSelectedTextAttributes:attributes];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)setSelection:(int)start length:(int)length
{
	[self selectText:nil];
	if(length >= 0)
	{
		NSTextView* editor = [self fieldEditor];
		[editor setSelectedRange:NSMakeRange (start, length)];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)setTextColor:(NSColor*)textColor
{
	[super setTextColor:textColor];
	[[self fieldEditor] setInsertionPointColor:textColor];
}

@end

//************************************************************************************************
// CustomSecureTextField
//************************************************************************************************

@interface CCL_ISOLATED (CustomSecureTextField) : NSSecureTextField
{}

- (NSDictionary*)selectedTextAttributes;
- (void)setSelectedTextAttributes:(NSDictionary*)attributes;
- (void)setSelection:(int)start length:(int)length;
- (NSTextView*)fieldEditor;
- (void)setTextColor:(NSColor*)textColor;

@end

//////////////////////////////////////////////////////////////////////////////////////////////////

@implementation CCL_ISOLATED (CustomSecureTextField)

- (BOOL)performKeyEquivalent:(NSEvent*)event
{
	NSString* key = [event charactersIgnoringModifiers];
	NSEventModifierFlags modifiers = [event modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask;
	if(modifiers == NSEventModifierFlagCommand)
	{
		// The command key is the ONLY modifier key being pressed.
		if([key isEqualToString:@"v"])
			return [NSApp sendAction:@selector(paste:) to:[[self window] firstResponder] from:self];
		else if([key isEqualToString:@"a"])
			return [NSApp sendAction:@selector(selectAll:) to:[[self window] firstResponder] from:self];
	}

    return [super performKeyEquivalent:event];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)isOpaque
{
	return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSTextView*)fieldEditor
{
	return (NSTextView*)[[self window] fieldEditor:YES forObject:self];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSDictionary*)selectedTextAttributes
{
	NSTextView* editor = [self fieldEditor];
	return [editor selectedTextAttributes];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)setSelectedTextAttributes:(NSDictionary*)attributes
{
	NSTextView* editor = [self fieldEditor];
	[editor setSelectedTextAttributes:attributes];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)setSelection:(int)start length:(int)length
{
	[self selectText:nil];
	if(length >= 0)
	{
		NSTextView* editor = [self fieldEditor];
		[editor setSelectedRange:NSMakeRange (start, length)];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)setTextColor:(NSColor*)textColor
{
	[super setTextColor:textColor];
	[[self fieldEditor] setInsertionPointColor:textColor];
}

@end

//************************************************************************************************
// NativeTextControl
//************************************************************************************************

NativeTextControl* NativeTextControl::create (Control& owner, RectRef clientRect, int returnKeyType, int keyboardType)
{
	return NEW CocoaTextControl (owner, clientRect, returnKeyType, keyboardType);
}

//************************************************************************************************
// CocoaTextControl
//************************************************************************************************

CocoaTextControl::CocoaTextControl (Control& owner, const Rect& clientRect, int returnKeyType, int keyboardType)
: NativeTextControl (owner, returnKeyType, keyboardType),
  textField (nil),
  delegate (nil),
  changed (false)
{
	// owning control must be attached!
	OSXWindow* osxWindow = OSXWindow::cast (owner.getWindow ());
	
	ASSERT (osxWindow != nullptr)
	if(!osxWindow)
		return;

	// Text color
	Color tc = getVisualStyle ().getTextColor ();
	NSColor* textColor = [NSColor colorWithDeviceRed:tc.getRedF () green:tc.getGreenF () blue:tc.getBlueF () alpha:tc.getAlphaF ()];
	
	Rect rect (clientRect);
	
	Point offset;
	owner.clientToWindow (offset);
	rect.offset (offset);
	
	NSRect frame;
	MacOS::toNSRect (frame, rect);

	if(owner.getStyle ().isCustomStyle (Styles::kTextBoxAppearanceMultiLine))
	{
		NSScrollView* scrollView = [[[NSScrollView alloc] initWithFrame:frame] autorelease];
		[scrollView setBorderType:NSNoBorder];
		[scrollView setVerticalScrollElasticity:NSScrollElasticityNone];
		[scrollView setHorizontalScrollElasticity:NSScrollElasticityNone];
		[scrollView setHasVerticalScroller:owner.getStyle ().isVertical ()];
		[scrollView setHasHorizontalScroller:owner.getStyle ().isHorizontal ()];
		[scrollView setDrawsBackground:NO];
		CCL_ISOLATED (CustomTextView)* textView = [[CCL_ISOLATED (CustomTextView) alloc] initWithFrame:NSMakeRect(0, 0, [scrollView contentSize].width, [scrollView contentSize].height)];
		
		NSMutableParagraphStyle* lineBreakMode = [[[NSParagraphStyle defaultParagraphStyle] mutableCopy] autorelease];
		if(getVisualStyle ().getTextFormat ().isWordBreak ())
			[lineBreakMode setLineBreakMode:NSLineBreakByWordWrapping];
		else
			[lineBreakMode setLineBreakMode:NSLineBreakByTruncatingTail];
		[textView setDefaultParagraphStyle:lineBreakMode];
		[textView setAllowsUndo:YES];
		[textView setAutomaticQuoteSubstitutionEnabled:NO];
		[textView setAutomaticDashSubstitutionEnabled:NO];
		[textView setInsertionPointColor:textColor];
		textView->parent = scrollView;
		[scrollView setDocumentView:textView];
		NativeView subView (scrollView);
		osxWindow->embed (&subView);
		textField = textView;
	}
	else
	{
		if(owner.getStyle ().isCustomStyle (Styles::kTextBoxBehaviorPasswordEdit))
			textField = [[CCL_ISOLATED (CustomSecureTextField) alloc] initWithFrame:frame];
		else
			textField = [[CCL_ISOLATED (CustomTextField) alloc] initWithFrame:frame];
		[textField setBezeled: NO];
		[textField setBordered: YES];

		#if 0
	    [textField setUsesSingleLineMode:YES];
	    [textField setMaximumNumberOfLines:1];
	    [textField setLineBreakMode:NSLineBreakByCharWrapping];
	    #else
		// [textField setUsesSingleLineMode:YES] is not working for unknown reasons, use (deprecated) cell method instead
		NSTextFieldCell* cell = [textField cell];
		[cell setLineBreakMode:NSLineBreakByClipping];
		[cell setScrollable:YES];
		#endif
		
		NativeView subView (textField);
		osxWindow->embed (&subView);
	}

	[textField setSelectable: YES];
	[textField setEditable: YES];

	[textField setFocusRingType:NSFocusRingTypeExterior];
	delegate = [[CCL_ISOLATED (CustomTextFieldDelegate) alloc] init:this];
	[textField setDelegate: delegate];
	
	// find parent view with a graphics layer
	View* parentLayerView = &owner;
	while(parentLayerView && !parentLayerView->getGraphicsLayer ())
		parentLayerView = parentLayerView->getParent ();

	// add layer for textField if it would be obscured by parent layer
	if(parentLayerView && parentLayerView->getGraphicsLayer () && parentLayerView != parentLayerView->getWindow ())
		[textField setWantsLayer: YES];

	updateVisualStyle ();
	updateText ();
	setSize (clientRect);

	if([textField acceptsFirstResponder])
		[toNSWindow(osxWindow) makeFirstResponder:textField];

	owner.takeFocus ();

	// the background color for selected text might be dynamic (since Mojave), make it an RGB color and set a foreground color with contrast
	NSMutableDictionary* selectedAttributes = [NSMutableDictionary dictionaryWithDictionary:[textField selectedTextAttributes]];
	NSColor* selectedBackground = [selectedAttributes objectForKey:NSBackgroundColorAttributeName];
	NSColor* selectedForeground = [NSColor blackColor];
	NSColor* rgbColor = [selectedBackground colorUsingColorSpace:[NSColorSpace genericRGBColorSpace]];
	if([rgbColor brightnessComponent] < 0.5)
		selectedForeground = [NSColor whiteColor];
	[selectedAttributes setObject:rgbColor forKey:NSBackgroundColorAttributeName];
	[selectedAttributes setObject:selectedForeground forKey:NSForegroundColorAttributeName];
	[textField setSelectedTextAttributes:selectedAttributes];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaTextControl::~CocoaTextControl ()
{
	if(textField)
	{
		if(!canceled || changed)
			submitText ();
		
		// give first responder state back to window
		if([[textField window] firstResponder] == textField)
			[[textField window] makeFirstResponder:[textField window]];
		
		if(owner.getStyle ().isCustomStyle (Styles::kTextBoxAppearanceMultiLine))
		{
			CCL_ISOLATED (CustomTextView)* textView = (CCL_ISOLATED (CustomTextView)*)textField;
			NSScrollView* scrollview = textView->parent;
			[scrollview removeFromSuperview];
		}

		[textField removeFromSuperview];
		[textField release];
		[delegate release];
		delegate = nil;
		textField = nil;
	}
	
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaTextControl::updateText ()
{
	String text;
	IParameter* p = getTextParameter ();
	if(p)	
		p->toString (text);
	
	NSString* nsText = text.createNativeString<NSString*> ();
	[textField setStringValue:nsText];
	[nsText release];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaTextControl::getControlText (String& text)
{
	if(textField != nil)
		text.appendNativeString ([textField stringValue]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaTextControl::setSelection (int start, int length)
{
	[textField setSelection:start length:length];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaTextControl::setScrollPosition (PointRef where)
{
	if(textField != nil)
	{
		NSView* clipView = [textField superview];
		if([clipView respondsToSelector:@selector(scrollToPoint:)])
			[(NSClipView*)clipView scrollToPoint:NSMakePoint (where.x, where.y)];
		NSView* parent = [clipView superview];
		if([parent respondsToSelector:@selector(reflectScrolledClipView:)])
			[parent reflectScrolledClipView:textField];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::Point CocoaTextControl::getScrollPosition () const
{
	Point where;
	if(textField != nil)
	{
		NSView* clipView = [textField superview];
		NSPoint origin = [clipView bounds].origin;
		where.x = (Coord) origin.x;
		where.y = (Coord) origin.y;
	}
	return where;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaTextControl::setSize (RectRef clientRect)
{
	if(textField != nil)
	{
		Rect rect (clientRect);
		Point offset;
		owner.clientToWindow (offset);
		rect.offset (offset);

		if(owner.getStyle ().isCustomStyle (Styles::kTextBoxAppearanceMultiLine))
		{
			CCL_ISOLATED (CustomTextView)* textView = (CCL_ISOLATED (CustomTextView)*)textField;
			NSRect frame;
			MacOS::toNSRect (frame, rect);
			NSScrollView* scrollView = textView->parent;
			[scrollView setFrame:frame];
			[textView setFrame:NSMakeRect(0, 0, [scrollView contentSize].width, [scrollView contentSize].height)];
		}
		else
		{
			[textField sizeToFit];
			NSRect frame = [textField frame];
			Coord offset = (Coord)(frame.size.height - rect.getHeight ()) / 2;
			rect.top -= offset;
			rect.setHeight ((Coord)frame.size.height);

			MacOS::toNSRect (frame, rect);
			[textField setFrame: frame];
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaTextControl::updateVisualStyle ()
{
	if(textField != nil)
	{
		// Background color
		Color bc = getVisualStyle ().getBackColor ();
		NSColor* backColor = [NSColor colorWithDeviceRed:bc.getRedF () green:bc.getGreenF () blue:bc.getBlueF () alpha:1.0 ];
		
		// Text color
		Color tc = getVisualStyle ().getTextColor ();
		NSColor* textColor = [NSColor colorWithDeviceRed:tc.getRedF () green:tc.getGreenF () blue:tc.getBlueF () alpha:tc.getAlphaF ()];

		// Background color
		[textField setBackgroundColor:backColor];
		
		// Text color
		[textField setTextColor:textColor];

		// Set font
		Font font = getVisualStyle ().getTextFont ();
		CCL::String fontName (font.getFace ());
		if(!font.getStyleName ().isEmpty ())
			fontName << " " << font.getStyleName ();

		NSString* nsFontName = [fontName.createNativeString<NSString*> () autorelease];
		NSFont* nsFont = [NSFont fontWithName:nsFontName size:font.getSize ()];
		if(nsFont == nil)
		{
			nsFontName = [font.getFace ().createNativeString<NSString*> () autorelease];
			nsFont = [[NSFontManager sharedFontManager] fontWithFamily:nsFontName traits:0 weight:0 size:font.getSize ()];
		}

		if(nsFont == nil)
			nsFont = [NSFont systemFontOfSize:font.getSize ()];

		if(font.getStyle () != 0)
		{
			// Text style
			NSFontTraitMask mask = 0;
			if(font.getStyle () & Font::kBold)
				mask |= NSBoldFontMask;
			if(font.getStyle () & Font::kItalic)
				mask |= NSItalicFontMask;

			NSFont* nsStyledFont = [[NSFontManager sharedFontManager] convertFont:nsFont toHaveTrait:mask];
			if(nsStyledFont != nil)
				nsFont = nsStyledFont;
		}

		[textField setFont:nsFont];

		// alignment
		int alignment = getVisualStyle ().getTextAlignment ().getAlignH ();
		if(alignment & Alignment::kLeft)
			[textField setAlignment: NSTextAlignmentLeft];
		else if(alignment & Alignment::kRight)
			[textField setAlignment: NSTextAlignmentRight];
		else
			[textField setAlignment: NSTextAlignmentCenter];
	}
}
