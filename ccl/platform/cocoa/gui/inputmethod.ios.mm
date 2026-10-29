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
// Filename    : ccl/platform/cocoa/gui/inputmethod.ios.mm
// Description : iOS Input Method
//
//************************************************************************************************

#include "ccl/gui/system/inputmethod.h"
#include "ccl/gui/views/view.h"

#include "ccl/gui/graphics/mutableregion.h"

#include "ccl/platform/cocoa/macutils.h"
#include "ccl/platform/cocoa/gui/window.ios.h"
#include "ccl/platform/cocoa/gui/nativeview.ios.h"
#include "ccl/platform/cocoa/quartz/cghelper.h"

#include "ccl/public/base/ccldefpush.h"
#include <UIKit/UIKit.h>

namespace CCL {

class IOSInputMethod;

} // namespace CCL

//************************************************************************************************
// IndexPosition
//************************************************************************************************

@interface IndexPosition: UITextPosition
@property (nonatomic) NSInteger index;
+ (instancetype)positionWithIndex:(NSInteger)index;
@end

//************************************************************************************************
// IndexRange
//************************************************************************************************

@interface IndexRange: UITextRange
@property (nonatomic) NSInteger startIndex;
@property (nonatomic) NSInteger endIndex;
+ (instancetype)rangeFrom:(NSInteger)start to:(NSInteger)end;
- (NSRange)nsRange;
@end

//************************************************************************************************
// TextInputClient
//************************************************************************************************

@interface CCL_ISOLATED (TextInputClient): UIView<UITextInput>
{
	CCL::IOSInputMethod* inputMethod;
	CCL::ITextInputClient* client;

	int caret;
	int selection;
}

@property (copy) UITextRange* selectedTextRange;
@property (nonatomic, strong) UITextRange* markedTextRange;
@property (nonatomic, copy) NSDictionary* markedTextStyle;
@property (nonatomic, assign) id<UITextInputDelegate> inputDelegate;
@property (nonatomic, strong) id<UITextInputTokenizer> tokenizer;

- (id)initWithInputMethod:(CCL::IOSInputMethod*)inputMethod;
- (void)handleUndo:(UIKeyCommand*)cmd;
- (void)handleRedo:(UIKeyCommand*)cmd;
- (void)deleteForward;
- (void)textChanged;
- (void)selectionChanged;

@end

namespace CCL {

//************************************************************************************************
// IOSInputMethod
//************************************************************************************************

class IOSInputMethod: public Object,
					  public IInputMethod
{
public:
	IOSInputMethod (ITextInputClient* client);
	~IOSInputMethod ();

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
	IOSWindow* window;
	UIWindow* nativeWindow;
};

//************************************************************************************************
// IOSInputMethodManager
//************************************************************************************************

class IOSInputMethodManager: public InputMethodManager
{
public:
	// InputMethodManager
	IInputMethod* createInputMethod (ITextInputClient& client) override;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// IndexPosition
//************************************************************************************************

@implementation IndexPosition

+ (instancetype)positionWithIndex:(NSInteger)index
{
	IndexPosition *p = [IndexPosition new];
	p.index = index;
	return p;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)copyWithZone:(NSZone*)zone
{
	return [IndexPosition positionWithIndex:self.index];
}

@end

//************************************************************************************************
// IndexRange
//************************************************************************************************

@implementation IndexRange

+ (instancetype)rangeFrom:(NSInteger)start to:(NSInteger)end
{
	IndexRange* r = [IndexRange new];
	r.startIndex = start;
	r.endIndex = end;
	return r;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)copyWithZone:(NSZone*)zone
{
	return [IndexRange rangeFrom:self.startIndex to:self.endIndex];
}


//////////////////////////////////////////////////////////////////////////////////////////////////

- (UITextPosition*)start
{
	return [IndexPosition positionWithIndex:self.startIndex];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (UITextPosition*)end
{
	return [IndexPosition positionWithIndex:self.endIndex];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)isEmpty
{
	return self.startIndex == self.endIndex;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSRange)nsRange
{
	return NSMakeRange (self.startIndex, self.endIndex - self.startIndex);
}
@end

//************************************************************************************************
// TextInputClient
//************************************************************************************************

@implementation CCL_ISOLATED (TextInputClient)

- (id)initWithInputMethod:(IOSInputMethod*)_inputMethod
{
	if(!_inputMethod)
		return nil;

	if(self = [super init])
	{
		inputMethod = _inputMethod;
		client = _inputMethod->getClient ();
		caret = 0;
		selection = 0;
		_markedTextRange = nil;
		_tokenizer = [[UITextInputStringTokenizer alloc] init];
	}

	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)canBecomeFirstResponder
{
	return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (UITextRange*)selectedTextRange
{
	return [IndexRange rangeFrom:ccl_min (caret, selection) to:ccl_max (caret, selection)];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)setSelectedTextRange:(UITextRange*)selectedTextRange
{
	[_inputDelegate selectionWillChange:self];

	IndexRange* range = (IndexRange*)selectedTextRange;
	int newCaret = range ? (int)range.startIndex : 0;
	// only support for left and right cursor keys for now
	if(ccl_abs (newCaret - caret) == 1)
	{
		caret = newCaret;
		selection = range ? (int)range.endIndex : 0;
		client->setSelection (caret, selection);
	}

	[_inputDelegate selectionDidChange:self];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)copy:(id)sender
{
	KeyEvent keydown (KeyEvent::kKeyDown, VKey::kUnknown, 'c', 0, KeyState::kCommand);
	inputMethod->getWindow ()->onKeyDown (keydown);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)paste:(id)sender
{
	KeyEvent keydown (KeyEvent::kKeyDown, VKey::kUnknown, 'v', 0, KeyState::kCommand);
	inputMethod->getWindow ()->onKeyDown (keydown);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

- (void)cut:(id)sender
{
	KeyEvent keydown (KeyEvent::kKeyDown, VKey::kUnknown, 'x', 0, KeyState::kCommand);
	inputMethod->getWindow ()->onKeyDown (keydown);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)selectAll:(id)sender
{
	KeyEvent keydown (KeyEvent::kKeyDown, VKey::kUnknown, 'a', 0, KeyState::kCommand);
	inputMethod->getWindow ()->onKeyDown (keydown);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSArray<UIKeyCommand*>*)keyCommands
{
	return @[
		[UIKeyCommand keyCommandWithInput:@"z" modifierFlags:UIKeyModifierCommand action:@selector(handleUndo:)],
		[UIKeyCommand keyCommandWithInput:@"z" modifierFlags:UIKeyModifierCommand | UIKeyModifierShift action:@selector(handleRedo:)],
		[UIKeyCommand keyCommandWithInput:@"y" modifierFlags:UIKeyModifierCommand action:@selector(handleRedo:)]
	];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)pressesBegan:(NSSet<UIPress*>*)presses withEvent:(UIPressesEvent*)event
{
	BOOL handled = NO;
	for(UIPress* press in presses)
		if(press.key.keyCode == UIKeyboardHIDUsageKeyboardDeleteForward)
		{
			[self deleteForward];
			handled = YES;
		}
	if(!handled)
		[super pressesBegan:presses withEvent:event];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)handleUndo:(UIKeyCommand*)cmd
{
	KeyEvent keydown (KeyEvent::kKeyDown, VKey::kUnknown, 'z', 0, KeyState::kCommand);
	inputMethod->getWindow ()->onKeyDown (keydown);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)handleRedo:(UIKeyCommand*)cmd
{
	KeyEvent keydown (KeyEvent::kKeyDown, VKey::kUnknown, 'Z', 0, KeyState::kCommand);
	inputMethod->getWindow ()->onKeyDown (keydown);
}


//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)deleteForward
{
	KeyEvent keydown (KeyEvent::kKeyDown, VKey::kDelete, 0);
	inputMethod->getWindow ()->onKeyDown (keydown);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)textChanged
{
	[self.inputDelegate textDidChange:self];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)selectionChanged
{
	[self.inputDelegate selectionWillChange:self];
	client->getSelection (caret, selection);
	[self.inputDelegate selectionDidChange:self];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)insertText:(NSString*)text
{
	String string;
	string.appendNativeString (text);
	client->applyText (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)deleteBackward
{
	KeyEvent keydown (KeyEvent::kKeyDown, VKey::kBackspace, 0);
	inputMethod->getWindow ()->onKeyDown (keydown);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)hasText
{
	return client->getTextLength () > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSString*)textInRange:(UITextRange*)textRange
{
	String string;
	client->getText (string, 0, -1);
	NSString* nativeString = [string.createNativeString<NSString*> () autorelease];
	NSRange range = [(IndexRange*)textRange nsRange];
	range = NSIntersectionRange (range, NSMakeRange (0, [nativeString length]));
	return [nativeString substringWithRange:range];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)replaceRange:(UITextRange*)textRange withText:(NSString*)text
{
	NSRange range = [(IndexRange*)textRange nsRange];
	[self.inputDelegate textWillChange:self];
	String newText;
	newText.appendNativeString (text);
	client->replaceText (newText, (int)range.location, (int)range.length);
	[self.inputDelegate textDidChange:self];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)setMarkedText:(NSString*)markedText selectedRange:(NSRange)selectedRange
{
	String preedit;
	preedit.appendNativeString (markedText);
	client->setPreeditText (preedit);
	_markedTextRange = [IndexRange rangeFrom:caret to:caret + [markedText length]];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)unmarkText
{
	String markedText;
	client->getPreeditText (markedText);
	client->setPreeditText (String::kEmpty);
	_markedTextRange = nil;
	client->applyText (markedText);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (UITextRange*)textRangeFromPosition:(UITextPosition*)fromPosition toPosition:(UITextPosition*)toPosition
{
	NSInteger a = [(IndexPosition*)fromPosition index];
	NSInteger b = [(IndexPosition*)toPosition index];
	return [IndexRange rangeFrom:ccl_min (a,b) to:ccl_max (a,b)];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (UITextPosition*)positionFromPosition:(UITextPosition*)position offset:(NSInteger)offset
{
	NSInteger idx = [(IndexPosition*)position index] + offset;
	idx = ccl_max (0, ccl_min ((int)idx, client->getTextLength ()));
	return [IndexPosition positionWithIndex:idx];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (UITextPosition*)positionFromPosition:(UITextPosition*)position inDirection:(UITextLayoutDirection)direction offset:(NSInteger)offset
{
	NSInteger idx = [(IndexPosition*)position index];
	// no multi-line support for iOS yet
	if(direction == UITextLayoutDirectionRight)
		idx += offset;
	else if(direction == UITextLayoutDirectionLeft)
		idx -= offset;

	idx = ccl_max (0, ccl_min ((int)idx, client->getTextLength ()));
	return [IndexPosition positionWithIndex:idx];;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (UITextPosition*)beginningOfDocument
{
	return [IndexPosition positionWithIndex:0];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (UITextPosition*)endOfDocument
{
	return [IndexPosition positionWithIndex:client->getTextLength () - 1];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSComparisonResult)comparePosition:(UITextPosition*)position toPosition:(UITextPosition*)other
{
	NSInteger a = [(IndexPosition*)position index];
	NSInteger b = [(IndexPosition*)other index];
	if(a < b) return NSOrderedAscending;
	if(a > b) return NSOrderedDescending;
	return NSOrderedSame;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSInteger)offsetFromPosition:(UITextPosition*)from toPosition:(UITextPosition*)to
{
	return [(IndexPosition*)to index] - [(IndexPosition*)from index];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (UITextPosition*)positionWithinRange:(UITextRange*)range farthestInDirection:(UITextLayoutDirection)direction
{
	IndexRange* r = (IndexRange*)range;
	return (direction == UITextLayoutDirectionRight || direction == UITextLayoutDirectionDown) ? [IndexPosition positionWithIndex:r.endIndex] : [IndexPosition positionWithIndex:r.startIndex];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (UITextRange*)characterRangeByExtendingPosition:(UITextPosition*)position inDirection:(UITextLayoutDirection)direction
{
	NSInteger idx = [(IndexPosition*)position index];
	return (direction == UITextLayoutDirectionRight || direction == UITextLayoutDirectionDown) ? [IndexRange rangeFrom:idx to:ccl_min (idx + 1, (NSInteger)client->getTextLength ())] : [IndexRange rangeFrom:ccl_max ((int)idx - 1, 0) to:idx];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSWritingDirection)baseWritingDirectionForPosition:(UITextPosition*)position inDirection:(UITextStorageDirection)direction
{
	return NSWritingDirectionNatural;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)setBaseWritingDirection:(NSWritingDirection)writingDirection forRange:(UITextRange*)range
{
	// Implement for bidirectional text
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (CGRect)firstRectForRange:(UITextRange*)r
{
	IndexRange* range = static_cast<IndexRange*>(r);
	SelectionRegion region;
	client->getTextBounds (region, (int)[range nsRange].location, (int)[range nsRange].length);
	if(region.getRects ().isEmpty ())
		return CGRectZero;

	CCL::Rect firstRect (region.getRects ().at (0));
	CCL::Point origin = firstRect.getLeftTop ();
	if(const IView* view = client->getInputView ())
	{
		view->clientToWindow (origin);
		firstRect.moveTo (origin);
	}

	CGRect rect;
	MacOS::toCGRect (rect, firstRect);
	return rect;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (UITextPosition*)closestPositionToPoint:(CGPoint)point
{
	// this appears to be never called
	ASSERT (0)
	return [IndexPosition positionWithIndex:0];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSArray<UITextSelectionRect*>*)selectionRectsForRange:(UITextRange*)range
{
	return @[];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (UITextPosition*)closestPositionToPoint:(CGPoint)point withinRange:(UITextRange*)range
{
	// this appears to be never called
	ASSERT (0)
	return [IndexPosition positionWithIndex:0];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (UITextRange*)characterRangeAtPoint:(CGPoint)point
{
	UITextPosition* pos = [self closestPositionToPoint:point];
	return [self textRangeFromPosition:pos toPosition:pos];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (CGRect)caretRectForPosition:(UITextPosition*)position
{
	// ignore position, the text system is only interested in the size of the caret, which is constant in our case
	CCL::Rect rect;
	client->getCaretRect (rect);
	CCL::Point origin = rect.getLeftTop ();
	if(const IView* view = client->getInputView ())
	{
		view->clientToWindow (origin);
		rect.moveTo (origin);
	}
	CGRect frame;
	MacOS::toCGRect (frame, rect);
	return frame;
}

@end

//************************************************************************************************
// IOSInputMethodManager
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (InputMethodManager, IOSInputMethodManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

IInputMethod* IOSInputMethodManager::createInputMethod (ITextInputClient& client)
{
	return NEW IOSInputMethod (&client);
}

//************************************************************************************************
// IOSInputMethod
//************************************************************************************************

IOSInputMethod::IOSInputMethod (ITextInputClient* client)
: client (client),
  window (nil)
{
	const IView* iView = client->getInputView ();
	if(View* view = unknown_cast<View> (iView))
		if(window = IOSWindow::cast (view->getWindow ()))
			if(NativeView* nativeView = window->getNativeView ())
				if(UIView* iosView = nativeView->getView ())
					nativeWindow = [iosView window];
	ASSERT (nativeWindow)
	if(nativeWindow == nil)
		return;

	nativeClient = [[CCL_ISOLATED (TextInputClient) alloc] initWithInputMethod:this];
	[[[nativeWindow rootViewController] view] addSubview:nativeClient];
	RectRef size = iView->getSize ();
	[nativeClient setFrame:CGRectMake (size.left, size.top, size.getWidth (), size.getHeight ())];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IOSInputMethod::~IOSInputMethod ()
{
	if(nativeClient)
		[nativeClient removeFromSuperview];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API IOSInputMethod::enable (tbool state)
{
	tresult result = kResultOk;
	if(state)
	{
		window->setSuppressKeyEvents (true);
		result = [nativeClient becomeFirstResponder] == YES ? kResultOk : kResultFailed;
	}
	else
	{
		window->setSuppressKeyEvents (false);
		result = [nativeClient resignFirstResponder] == YES ? kResultOk : kResultFailed;
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API IOSInputMethod::textChanged ()
{
	[nativeClient textChanged];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API IOSInputMethod::selectionChanged ()
{
	[nativeClient selectionChanged];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API IOSInputMethod::visibleRectChanged ()
{
	// this triggers a requery of the coordinates
	[nativeClient selectionChanged];
}
