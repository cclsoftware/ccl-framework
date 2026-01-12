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
// Filename    : ccl/gui/controls/texteditor.cpp
// Description : Text Editor Control
//
//************************************************************************************************

#include "ccl/gui/controls/texteditor.h"
#include "ccl/gui/views/scrollview.h"

#include "ccl/app/params.h"

namespace CCL {

//************************************************************************************************
// TextEditorEditBox
//************************************************************************************************

class TextEditorEditBox: public EditBox
{
public:
	DECLARE_CLASS_ABSTRACT (TextEditorEditBox, EditBox)

	TextEditorEditBox (TextEditor& editor, const Rect& size = Rect (), IParameter* param = nullptr,
		StyleRef style = 0, StringRef title = nullptr);

	// EditBox
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	void onMove (const Point& delta) override;
	bool onFocus (const FocusEvent& event) override;
	bool onGesture (const GestureEvent& event) override;
	bool onMouseDown (const MouseEvent& event) override;

private:
	TextEditor& editor;
	Point scrollPos;
	bool inUpdateTargetSizeMessage;

	DECLARE_STRINGID_MEMBER (kUpdateScrollPosition)
	DECLARE_STRINGID_MEMBER (kUpdateScrollTargetSize)

	bool makeNativeControl ();

	// EditBox
	void updateCaretRect () override;
	void setChanged () override;
	void calculateNativeControlSize (Rect& size) override;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// TextEditor
//************************************************************************************************

DEFINE_CLASS (TextEditor, Control)
DEFINE_CLASS_UID (TextEditor, 0xCDF7C711, 0xA7B1, 0x46C6, 0xA1, 0x38, 0x7A, 0x42, 0x2B, 0xFF, 0xC7, 0x6C)

//////////////////////////////////////////////////////////////////////////////////////////////////

TextEditor::TextEditor (const Rect& size, IParameter* param, StyleRef style, StringRef title)
: Control (size, param, style, title),
  editBox (nullptr),
  scrollView (nullptr)
{
	this->style.custom |= Styles::kTextBoxAppearanceMultiLine;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextEditor::~TextEditor ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextEditor::makeFrameworkView ()
{
	if(!isEmpty ())
		return;

	ASSERT (editBox == nullptr)

	removeAll ();

	StyleFlags scrollStyle (getStyle ().common & (Styles::kHorizontal|Styles::kVertical|Styles::kSmall|Styles::kTransparent), 0);
	if(!scrollStyle.isVertical ())
		scrollStyle.setCustomStyle (Styles::kScrollViewBehaviorAutoHideVBar);

	if(scrollStyle.isTransparent ())
		scrollStyle.setCustomStyle (Styles::kScrollViewBehaviorNoScreenScroll);

	StyleFlags textBoxStyle (getStyle ());
	textBoxStyle.setCommonStyle (Styles::kTextBoxAppearanceMultiLine);
	textBoxStyle.setCommonStyle (Styles::kBorder, false);
	textBoxStyle.setCommonStyle (Styles::kHorizontal|Styles::kVertical, false); // (e.g. on windows, this forces scrollbars and prevents wordbreak)

	if(EditBox::useNativeTextControl == false || getStyle ().isCustomStyle (Styles::kEditBoxBehaviorExtended))
	{
		textBoxStyle.setCustomStyle (Styles::kEditBoxBehaviorImmediate);
		scrollStyle.setCustomStyle (Styles::kScrollViewBehaviorAutoHideHBar | Styles::kScrollViewBehaviorExtendTarget);
	}

	Rect clientRect;
	getClientRect (clientRect);

	// multiline edit box
	editBox = NEW TextEditorEditBox (*this, clientRect, getParameter (), textBoxStyle);
	editBox->setVisualStyle (visualStyle);
	editBox->setSizeMode (kAttachAll);

	// scrollview
	scrollView = NEW ScrollView (clientRect, editBox, scrollStyle, visualStyle);
	scrollView->setSizeMode (kAttachAll);

	if(vBarStyle)
		scrollView->setVScrollBarStyle (vBarStyle);
	if(hBarStyle)
		scrollView->setHScrollBarStyle (hBarStyle);
	
	addView (scrollView);
	updateScrollTargetSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextEditor::setHScrollBarStyle (VisualStyle* visualStyle)
{
	hBarStyle = visualStyle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextEditor::setVScrollBarStyle (VisualStyle* visualStyle)
{
	vBarStyle = visualStyle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextEditor::attached (View* parent)
{
	SuperClass::attached (parent);

	if(isEmpty ())
		makeFrameworkView ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextEditor::onSize (const Point& delta)
{
	SuperClass::onSize (delta);
	updateScrollTargetSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextEditor::onVisualStyleChanged ()
{
	SuperClass::onVisualStyleChanged ();
	if(editBox)
		editBox->setVisualStyle (visualStyle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextEditor::paramChanged ()
{
	SuperClass::paramChanged ();
	updateScrollTargetSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextEditor::updateScrollTargetSize ()
{
	if(scrollView != nullptr && editBox != nullptr)
	{
		Rect clipViewRect;
		scrollView->getClipViewRect (clipViewRect);
		Coord maxWidth = -1;
		if(visualStyle->getTextOptions () & TextFormat::kWordBreak)
			maxWidth = clipViewRect.getWidth ();

		editBox->setSizeLimits (SizeLimit (clipViewRect.getWidth (), clipViewRect.getHeight (), maxWidth, -1));
		editBox->autoSize ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextEditor::updateScrollPosition (RectRef caretRect)
{
	if(scrollView != nullptr && editBox != nullptr)
		scrollView->makeVisible (Rect (caretRect).offset (editBox->getSize ().getLeftTop ()), true);
}

//************************************************************************************************
// TextEditorEditBox
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (TextEditorEditBox, EditBox)

DEFINE_STRINGID_MEMBER_ (TextEditorEditBox, kUpdateScrollPosition, "updateScrollPosition")
DEFINE_STRINGID_MEMBER_ (TextEditorEditBox, kUpdateScrollTargetSize, "updateScrollTargetSize")

//////////////////////////////////////////////////////////////////////////////////////////////////

TextEditorEditBox::TextEditorEditBox (TextEditor& editor, const Rect& size, IParameter* param,	StyleRef style, StringRef title)
: EditBox (size, param, style, title),
  editor (editor),
  inUpdateTargetSizeMessage (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TextEditorEditBox::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kUpdateScrollPosition)
	{
		if(shouldUseNativeControl ())
			if(ScrollView* scrollView = ccl_cast<ScrollView> (editor.getFirst ()))
				scrollView->scrollTo (scrollPos);
		else
			editor.updateScrollPosition (caretRect);
	}
	else if(msg == kUpdateScrollTargetSize)
	{
		ScopedVar<bool> scope (inUpdateTargetSizeMessage, true);
		editor.updateScrollTargetSize ();
	}
	SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextEditorEditBox::updateCaretRect ()
{
	EditBox::updateCaretRect ();
	(NEW Message (kUpdateScrollPosition))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextEditorEditBox::onMove (const Point& delta)
{
	// don't kill focus
	View::onMove (delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextEditorEditBox::onFocus (const FocusEvent& event)
{
	if(!shouldUseNativeControl ())
		return SuperClass::onFocus (event);

	if(event.eventType == FocusEvent::kSetFocus)
	{
		if(!inKeyDown && !wantReopen)
			return true; // swallow, open nativeControl in onGesture / onMouseDown
	}
	else
	{
		if(nativeControl)
			scrollPos = nativeControl->getScrollPosition () * -1;
	}

	SuperClass::onFocus (event);

	if(event.eventType == FocusEvent::kKillFocus)
	{
		setChanged ();
		(NEW Message (kUpdateScrollPosition))->post (this);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextEditorEditBox::onGesture (const GestureEvent& event)
{
	if(!shouldUseNativeControl ())
		return SuperClass::onGesture (event);

	if(nativeControl == nullptr
		&& event.getType () == GestureEvent::kSingleTap
		&& !style.isCustomStyle (Styles::kTextBoxBehaviorDoubleClickEdit))
	{
		return makeNativeControl ();
	}
	return SuperClass::onGesture (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextEditorEditBox::onMouseDown (const MouseEvent& event)
{
	if(!shouldUseNativeControl ())
		return SuperClass::onMouseDown (event);

	if(nativeControl == nullptr
		&& event.keys.isSet (KeyState::kLButton)
		&& !style.isCustomStyle (Styles::kTextBoxBehaviorDoubleClickEdit))
	{
		return makeNativeControl ();
	}
	return SuperClass::onMouseDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextEditorEditBox::setChanged ()
{
	EditBox::setChanged ();

	if(!inUpdateTargetSizeMessage)
		(NEW Message (kUpdateScrollTargetSize))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextEditorEditBox::makeNativeControl ()
{
	if(getParameter ()->isReadOnly ()) // disallow editing if flagged as read-only
		return false;

	int tempScrollFlags = 0;
	if(ScrollView* scrollView = ccl_cast<ScrollView> (editor.getFirst ()))
	{
		scrollView->getPosition (scrollPos);

		if(scrollView->canScrollV ())
			tempScrollFlags = Styles::kVertical;
	}

	if(ScrollView* scrollView = ccl_cast<ScrollView> (editor.getFirst ()))
		scrollView->scrollTo (Point ());

	int commonStyle = style.common | tempScrollFlags;  // temp style to force native scrollbar in the kScrollViewBehaviorAutoHideVBar case
	commonStyle &= ~Styles::kBorder; // remove border style from native control
	ScopedVar<int> scope (style.common, commonStyle);

	setNativeControl (createNativeControl ());
	if(nativeControl)
		nativeControl->setScrollPosition (scrollPos * -1);

	paramFocused (true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextEditorEditBox::calculateNativeControlSize (Rect& size)
{
	SuperClass::calculateNativeControlSize (size);

	if(ScrollView* scrollView = ccl_cast<ScrollView> (editor.getFirst ()))
	{
		Rect clientRect;
		scrollView->getClientRect (clientRect);

		size.right = ccl_min (size.right, clientRect.right);
		size.bottom = ccl_min (size.bottom, clientRect.bottom);
	}
}
