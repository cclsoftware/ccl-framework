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
// Filename    : ccl/platform/cocoa/gui/textbox.ios.mm
// Description : platform-specific text control implementation
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/controls/editbox.h"
#include "ccl/gui/windows/dialog.h"
#include "ccl/gui/touch/touchinput.h"
#include "ccl/gui/keyevent.h"

#include "ccl/base/message.h"
#include "ccl/public/base/primitives.h"

#include "ccl/platform/cocoa/quartz/cghelper.h"
#include "ccl/platform/cocoa/gui/nativeview.ios.h"
#include "ccl/platform/cocoa/gui/autofill.ios.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/text/cclstring.h"

#include "ccl/platform/cocoa/cclcocoa.h"

#include "ccl/public/base/ccldefpush.h"

#define TEXTFIELDDELEGATE ((CCL_ISOLATED (CustomTextFieldDelegate)*)delegate)
bool textEditChanged = false;

namespace CCL {

//************************************************************************************************
// IOSNativeTextControl
//************************************************************************************************

class IOSNativeTextControl: public NativeTextControl
{
public:
	IOSNativeTextControl (Control& owner, const Rect& clientRect, int returnKeyType, int keyboardType);
	~IOSNativeTextControl ();

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
	Rect size;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// CustomTextFieldDelegate
//************************************************************************************************

@interface CCL_ISOLATED (CustomTextFieldDelegate) : NSObject<UITextFieldDelegate, UITextViewDelegate>
{
	NativeTextControl* nativeControl;
}
- (id) init:(NativeTextControl*)c;
- (void) keyboardWillShow:(NSNotification*)note;
- (void) keyboardWillHide:(NSNotification*)note;
- (void) keyboardDidHide:(NSNotification*)note;
- (void) observeValueForKeyPath:(NSString*)keyPath ofObject:(id)object change:(NSDictionary*)change context:(void*)context;
@property (nonatomic, assign) UITextField* textfield;
@property (nonatomic, assign) IParameter::Type paramType;
@end

//////////////////////////////////////////////////////////////////////////////////////////////////

@implementation CCL_ISOLATED (CustomTextFieldDelegate)

@synthesize textfield, paramType;

- (id)init:(NativeTextControl*)c
{
	self = [super init];
	nativeControl = c;
	paramType = IParameter::kString;

	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)textFieldDidEndEditing:(UITextField*)textField
{
	if(nativeControl)
		nativeControl->getOwner ().killFocus ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)textViewDidEndEditing:(UITextView*)textView
{
	if(nativeControl)
		nativeControl->getOwner ().killFocus ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)textFieldDidChange:(NSNotification*)aNotification
{
	textEditChanged = true;

	if(nativeControl->isImmediateUpdate ())
		(NEW Message ("checkSubmit"))->post (nativeControl);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)textViewDidChange:(UITextView*)textView
{
	textEditChanged = true;
	
	if(nativeControl->isImmediateUpdate ())
		(NEW Message ("checkSubmit"))->post (nativeControl);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)textFieldShouldClear:(UITextField*)textField
{
    return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)textFieldShouldReturn:(UITextField*)textField
{
    nativeControl->handleKeyDown (KeyEvent (KeyEvent::kKeyDown, VKey::kReturn));
    return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)keyboardWillShow:(NSNotification*)note
{
	if(!textfield)
		return;
	
	NSDictionary* userInfo = [note userInfo];
	
	NSValue* keyboardEndFrameValue = [userInfo objectForKey:UIKeyboardFrameEndUserInfoKey];
	UIWindow* window = [UIApplication sharedApplication].keyWindow;
	
	CGRect keyboardRect = [keyboardEndFrameValue CGRectValue];
	
	CGRect textRect = [[[window rootViewController] view] convertRect:textfield.frame fromView:[textfield superview]];
	// we must do some coordinate mangling, because apparently the keyboard and window dimensions might
	// not be in the same rotation...
	CGRect newWindowFrame = window.bounds;
	
	const float pad = 80.f;//textRect.size.height * 4.f;
	
	UIInterfaceOrientation orientation = [[UIApplication sharedApplication] statusBarOrientation];
	const CGFloat vDiff = (textRect.origin.y + textRect.size.height + pad) - (newWindowFrame.size.height - keyboardRect.size.height);

	if(vDiff > 0.0f)
		newWindowFrame.origin.y -= vDiff;
	
	if(!CGRectEqualToRect (newWindowFrame, window.bounds))
	{
        if(nativeControl)
            nativeControl->getOwner ().getWindow ()->getTouchInputState ().discardTouchesForView (nativeControl->getOwner (), true);
        
		NSValue* animationDurationValue = [userInfo objectForKey:UIKeyboardAnimationDurationUserInfoKey];
		NSTimeInterval animationDuration = 0;
		[animationDurationValue getValue:&animationDuration];
		
		[UIView animateWithDuration:animationDuration animations:^
		{
			window.frame = newWindowFrame;
		}];
		
		[self invalidatePopover];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)keyboardWillHide:(NSNotification*)note
{
	NSDictionary* userInfo = [note userInfo];
	
	UIWindow* window = [UIApplication sharedApplication].keyWindow;
	
	if(window.frame.origin.x != 0 || window.frame.origin.y != 0)
	{
		CGRect newWindowFrame = window.frame;
		newWindowFrame.origin.x = 0;
		newWindowFrame.origin.y = 0;
		
		NSValue* animationDurationValue = [userInfo objectForKey:UIKeyboardAnimationDurationUserInfoKey];
		NSTimeInterval animationDuration = 0;
		[animationDurationValue getValue:&animationDuration];
		[UIView animateWithDuration:animationDuration animations:^
		{
			window.frame = newWindowFrame;
		}];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)keyboardDidHide:(NSNotification*)note
{
	[self invalidatePopover];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)invalidatePopover
{
	if(nativeControl)
		if(Dialog* dialog = ccl_cast<Dialog> (nativeControl->getOwner ().getWindow ()))
			if(UIView* nativeView = dialog->getNativeView ()->getView () )
				[nativeView setNeedsDisplay];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary*)change context:(void*)context
{
	if([keyPath isEqualToString:@"frame"])
	{
		UITextRange* selection = [textfield selectedTextRange];
		[textfield setSelectedTextRange:nil];
		[textfield setSelectedTextRange:selection];
	}
}

@end

//************************************************************************************************
// NativeTextControl
//************************************************************************************************

NativeTextControl* NativeTextControl::create (Control& owner, const Rect& clientRect, int returnKeyType, int keyboardType)
{
	return NEW IOSNativeTextControl (owner, clientRect, returnKeyType, keyboardType);
}

//************************************************************************************************
// IOSNativeTextControl
//************************************************************************************************

IOSNativeTextControl::IOSNativeTextControl (Control& owner, const Rect& clientRect, int returnKeyType, int keyboardType)
: NativeTextControl (owner, returnKeyType, keyboardType),
  textField (nil),
  delegate (nil)
{
	// owning control must be attached!
	IOSWindow* w = IOSWindow::cast (owner.getWindow ());
	ASSERT (w != nil)
	if(!w)
		return;

	textEditChanged = false;
	
	Rect rect (clientRect);
	
	Point offset;
	owner.clientToWindow (offset);
	rect.offset (offset);
	
	Font font = getVisualStyle ().getTextFont ();
	if(!owner.getStyle ().isCustomStyle (Styles::kTextBoxAppearanceMultiLine))
	{
		static const int minTextFieldHeight = 22; // make sure that the text box is taller than the clear button, big enough for a finger, etc...
		Coord minHeight = MAX ((Coord)(font.getSize () + font.getSize () / 3 + 2), minTextFieldHeight);
		if(rect.getHeight () < minHeight)
		{
			Rect r (rect);
			rect.setHeight (minHeight);
			rect.center (r);
		}
	}
	
	CGRect frame;
	MacOS::toCGRect (frame, rect);
	
	if(owner.getStyle ().isCustomStyle (Styles::kTextBoxAppearanceMultiLine))
	{
		textField = [[UITextView alloc] initWithFrame:frame];
	}
	else
	{
		textField = [[UITextField alloc] initWithFrame:frame];
		if(owner.getStyle ().isCustomStyle (Styles::kEditBoxBehaviorNoClearButton) || rect.getWidth () < 45)
			[textField setClearButtonMode:UITextFieldViewModeNever];
		else
			[textField setClearButtonMode:UITextFieldViewModeAlways];

		[textField setContentVerticalAlignment:UIControlContentVerticalAlignmentCenter];
		[textField setSecureTextEntry:owner.getStyle ().isCustomStyle (Styles::kTextBoxBehaviorPasswordEdit)];
	}
	[textField setEnabled:YES];

	[textField setOpaque:YES];
	//textField.clearsOnInsertion = YES;

	IParameter* param = getTextParameter ();
	
	switch(keyboardType)
	{
	case Styles::kKeyboardTypeEmail :
		[textField setKeyboardType:UIKeyboardTypeEmailAddress];
		break;
	case Styles::kKeyboardTypeUrl :
		[textField setKeyboardType:UIKeyboardTypeURL];
		break;
	case Styles::kKeyboardTypePhoneNumber :
		[textField setKeyboardType:UIKeyboardTypePhonePad];
		break;
	case Styles::kKeyboardTypeNumeric :
		[textField setKeyboardType:UIKeyboardTypeNumberPad];
		break;
	case Styles::kKeyboardTypeNumericSigned :
		[textField setKeyboardType:UIKeyboardTypeNumbersAndPunctuation];
		break;
	case Styles::kKeyboardTypeDecimal :
		[textField setKeyboardType:UIKeyboardTypeDecimalPad];
		break;
	case Styles::kKeyboardTypeDecimalSigned :
		[textField setKeyboardType:UIKeyboardTypeNumbersAndPunctuation];
		break;
	default :
		[textField setKeyboardType:UIKeyboardTypeDefault];
	}
	
	switch(returnKeyType)
	{
	case Styles::kReturnKeyDefault :
		[textField setReturnKeyType:UIReturnKeyDefault];
		break;
	case Styles::kReturnKeyGo :
		[textField setReturnKeyType:UIReturnKeyGo];
		break;
	case Styles::kReturnKeyNext :
		[textField setReturnKeyType:UIReturnKeyNext];
		break;
	case Styles::kReturnKeySearch :
		[textField setReturnKeyType:UIReturnKeySearch];
		break;
	case Styles::kReturnKeySend :
		[textField setReturnKeyType:UIReturnKeySend];
		break;
	case Styles::kReturnKeyDone :
		[textField setReturnKeyType:UIReturnKeyDone];
		break;
	}

	if(IAutofillClient* client = UnknownPtr<IAutofillClient> (owner.asUnknown ()))
		iOS::UIAutofill::setContentType (textField, *client);

	delegate = [[CCL_ISOLATED (CustomTextFieldDelegate) alloc] init:this];
	[TEXTFIELDDELEGATE setTextfield:textField];
	if(param)
		TEXTFIELDDELEGATE.paramType = (IParameter::Type) param->getType ();
	[textField setDelegate:TEXTFIELDDELEGATE];
	if([textField isKindOfClass:[UITextField class]])
		[textField addTarget:TEXTFIELDDELEGATE action:@selector(textFieldDidChange:) forControlEvents:UIControlEventEditingChanged];

	[[NSNotificationCenter defaultCenter] addObserver:TEXTFIELDDELEGATE selector:@selector(keyboardWillShow:) name:UIKeyboardWillShowNotification object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:TEXTFIELDDELEGATE selector:@selector(keyboardWillHide:) name:UIKeyboardWillHideNotification object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:TEXTFIELDDELEGATE selector:@selector(keyboardDidHide:) name:UIKeyboardDidHideNotification object:nil];

	updateVisualStyle ();
	updateText ();
	setSize (clientRect);
	
	if(NativeView* view = w->getNativeView ())
		if(UIView* uiView = view->getView ())
		{
			[uiView addSubview:textField];
			[uiView addObserver:TEXTFIELDDELEGATE forKeyPath:@"frame" options:0 context:nil];
		}
	owner.takeFocus ();
	
	[textField becomeFirstResponder];

	if(!owner.getStyle ().isCustomStyle (Styles::kTextBoxAppearanceMultiLine))
		setSelection (0, -1); // select all
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IOSNativeTextControl::~IOSNativeTextControl ()
{
	if(!canceled || textEditChanged)
		submitText ();

	if(textField)
	{
		[textField resignFirstResponder];
		[textField removeFromSuperview];
		[textField setDelegate:nil];
		if([textField isKindOfClass:[UITextField class]])
			[textField removeTarget:TEXTFIELDDELEGATE action:@selector(textFieldDidChange:) forControlEvents:UIControlEventEditingChanged];
		[textField setSelectedTextRange:nil];
		[textField release];
	}
	
	if(IOSWindow* w = IOSWindow::cast (owner.getWindow ()))
		if(NativeView* view = w->getNativeView ())
			if(UIView* uiView = view->getView ())
				[uiView removeObserver:TEXTFIELDDELEGATE forKeyPath:@"frame"];
	
	if(TEXTFIELDDELEGATE)
	{
		[[NSNotificationCenter defaultCenter] removeObserver:TEXTFIELDDELEGATE name:UIKeyboardWillShowNotification object:nil];
		[[NSNotificationCenter defaultCenter] removeObserver:TEXTFIELDDELEGATE name:UIKeyboardWillHideNotification object:nil];
		[[NSNotificationCenter defaultCenter] removeObserver:TEXTFIELDDELEGATE name:UIKeyboardDidHideNotification object:nil];
		[TEXTFIELDDELEGATE release];
	}
	delegate = nil;

	cancelSignals ();
	
	// cancel all pending touches (touchesEnd can be delayed)
	if(Window* window = owner.getWindow ())
		window->getTouchInputState ().discardTouches (false, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSNativeTextControl::updateText ()
{
	String text;
	IParameter* p = getTextParameter ();
	if(p)	
		p->toString (text);
	
	NSString* nsText = text.createNativeString<NSString*> ();
	[textField setText:nsText];
	[nsText release];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSNativeTextControl::getControlText (String& text)
{
	if(textField != nil)
		text.appendNativeString ([textField text]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSNativeTextControl::setSelection (int start, int length)
{	
	UITextRange* selection = nil;
	if(start != -1)
	{
		UITextPosition* startPosition = [textField positionFromPosition:[textField beginningOfDocument] offset:start];
		UITextPosition* endPosition = nil;
		if(length == -1)
			endPosition = [textField endOfDocument];
		else
			endPosition = [textField positionFromPosition: startPosition offset:length];
		selection = [textField textRangeFromPosition:startPosition toPosition:endPosition];
	}
	[textField setSelectedTextRange:selection];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSNativeTextControl::setScrollPosition (PointRef where)
{
	if(textField)
	{
		UIScrollView* scrollView = textField;
		if([scrollView respondsToSelector:@selector (setContentOffset:)])
			[scrollView setContentOffset:CGPointMake (where.x, where.y)];
	 }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::Point IOSNativeTextControl::getScrollPosition () const
{
	Point where;
	if(textField)
	{
		UIScrollView* scrollView = textField;
		if([scrollView respondsToSelector:@selector (contentOffset)])
		{
			CGPoint origin = [scrollView contentOffset];
			where.x = (Coord)origin.x;
			where.y = (Coord)origin.y;
		}
	}
	return where;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSNativeTextControl::setSize (RectRef clientRect)
{
	if(textField != nil)
	{
		Rect rect (clientRect);
		Point offset;
		owner.clientToWindow (offset);
		rect.offset (offset);

		UIScrollView* textView = textField;
		CGRect frame;
		MacOS::toCGRect (frame, rect);
		[textView setFrame:frame];

		if(!owner.getStyle ().isCustomStyle (Styles::kTextBoxAppearanceMultiLine))
		{
			[textField sizeToFit];
			CGRect frame = [textField frame];
			Coord offset = (Coord)(frame.size.height - rect.getHeight ()) / 2;
			rect.top -= offset;
			rect.setHeight ((Coord)frame.size.height);

			MacOS::toCGRect (frame, rect);
			[textField setFrame: frame];
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSNativeTextControl::updateVisualStyle ()
{
	if(textField != nil)
	{
		// Background color
		Color bc = getVisualStyle ().getBackColor ();
		UIColor* backColor = [UIColor colorWithRed:bc.getRedF () green:bc.getGreenF () blue:bc.getBlueF () alpha:1.0 ];

		// Text color
		Color tc = getVisualStyle ().getTextColor ();
		UIColor* textColor = [UIColor colorWithRed:tc.getRedF () green:tc.getGreenF () blue:tc.getBlueF () alpha:tc.getAlphaF ()];

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
		UIFont* uiFont = [UIFont fontWithName:nsFontName size:font.getSize ()];
		if(uiFont == nil)
		{
			nsFontName = [font.getFace ().createNativeString<NSString*> () autorelease];
			uiFont = [UIFont fontWithName:nsFontName size:font.getSize ()];
		}

		if(uiFont == nil)
			uiFont = [UIFont systemFontOfSize:font.getSize ()];

		if(font.getStyle () != 0)
		{
			UIFont* styledUIFont = nil;
			// Text style
			bool bold = font.getStyle () & Font::kBold;
			bool italic = font.getStyle () & Font::kItalic;
			if(bold && italic)
				styledUIFont = [UIFont fontWithDescriptor:[[uiFont fontDescriptor] fontDescriptorWithSymbolicTraits:UIFontDescriptorTraitBold | UIFontDescriptorTraitItalic] size:uiFont.pointSize];
			else if(bold)
				styledUIFont = [UIFont fontWithDescriptor:[[uiFont fontDescriptor] fontDescriptorWithSymbolicTraits:UIFontDescriptorTraitBold] size:uiFont.pointSize];
			else if(italic)
				styledUIFont = [UIFont fontWithDescriptor:[[uiFont fontDescriptor] fontDescriptorWithSymbolicTraits:UIFontDescriptorTraitItalic] size:uiFont.pointSize];

			if(styledUIFont != nil)
				uiFont = styledUIFont;
		}

		[textField setFont:uiFont];
		[textField setAutocorrectionType:UITextAutocorrectionTypeNo];
		[textField setAutocapitalizationType:UITextAutocapitalizationTypeNone];

		// alignment
		int alignment = getVisualStyle ().getTextAlignment ().getAlignH ();
		if(alignment & Alignment::kLeft)
			[textField setTextAlignment:NSTextAlignmentLeft];
		else if(alignment & Alignment::kRight)
			[textField setTextAlignment:NSTextAlignmentRight];
		else
			[textField setTextAlignment:NSTextAlignmentCenter];
	}
}
