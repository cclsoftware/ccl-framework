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
// Filename    : ccl/gui/controls/editbox.cpp
// Description : Edit Box
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/controls/editbox.h"
#include "ccl/gui/controls/controlaccessibility.h"
#include "ccl/gui/system/clipboard.h"
#include "ccl/gui/system/dragndrop.h"
#include "ccl/gui/windows/dialog.h"
#include "ccl/gui/views/focusnavigator.h"
#include "ccl/gui/touch/touchhandler.h"
#include "ccl/gui/commands.h"

#include "ccl/base/message.h"
#include "ccl/base/boxedtypes.h"
#include "ccl/base/signalsource.h"

#include "ccl/public/collections/stack.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/guiservices.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/iparamobserver.h"
#include "ccl/public/gui/icontextmenu.h"
#include "ccl/public/gui/commanddispatch.h"
#include "ccl/public/gui/framework/abstractdraghandler.h"
#include "ccl/public/gui/framework/itextmodel.h"

#if CCL_PLATFORM_LINUX
	#define EDITBOX_USE_NATIVE_CONTROL false
#else
	#define EDITBOX_USE_NATIVE_CONTROL true
#endif

namespace CCL {

//************************************************************************************************
// EditBoxMouseHandler
//************************************************************************************************

class EditBox::EditBoxMouseHandler: public MouseHandler
{
public:
	EditBoxMouseHandler (EditBox* editBox = nullptr)
	: MouseHandler (editBox, MouseHandler::kAutoScroll)
	{}

	void onBegin () override
	{
		bool clearSelection = (current.keys.getModifiers () & KeyState::kShift) == 0;
		static_cast<EditBox*> (view)->moveCaretTo (current.where, clearSelection);
	}

	bool onMove (int moveFlags) override
	{
		return static_cast<EditBox*> (view)->moveCaretTo (current.where);
	}
};

//************************************************************************************************
// EditBox::TextDragHandler
//************************************************************************************************

class EditBox::TextDragHandler: public Unknown,
								public AbstractDragHandler
{
public:
	TextDragHandler (EditBox& editBox);
	~TextDragHandler ();

	// AbstractDragHandler
	tbool CCL_API dragEnter (const DragEvent& event) override;
	tbool CCL_API dragOver (const DragEvent& event) override;
	tbool CCL_API afterDrop (const DragEvent& event) override;
	tbool CCL_API dragLeave (const DragEvent& event) override;
	tbool CCL_API drop (const DragEvent& event) override;

	CLASS_INTERFACE (IDragHandler, Unknown)

private:
	EditBox& editBox;
	ITextLayout::Range selectionRange;
};

//************************************************************************************************
// EditBox::UndoHandler
//************************************************************************************************

class EditBox::UndoHandler
{
public:
	UndoHandler (EditBox& editBox);

	enum class ActionID
	{
		kNone,
		kInitialize,
		kMoveCaret,
		kType,
		kDeleteTrailing,
		kDeleteLeading,
		kCut,
		kPaste,
		kDrop
	};

	bool canUndo () const;
	bool canRedo () const;
	bool undo ();
	bool redo ();
	bool isUndoActive () const;

	void setDeferSaveCaretPosition ();
	void beginTransaction (ActionID actionId);
	void endTransaction ();
	void resetAction ();
	void saveCaretPosition ();
	void setUndoActive (bool value) { undoActive = value; }

private:
	EditBox& editBox;

	struct Item
	{
		int firstCaretPosition;
		int lastCaretPosition;

		Item (int firstCaretPosition = 0, int lastCaretPosition = 0)
		: firstCaretPosition (firstCaretPosition),
		  lastCaretPosition (lastCaretPosition)
		{}
	};
	
	Stack<Item> undoStack;
	Stack<Item> redoStack;
	ActionID lastAction;
	int caretPosition;
	bool undoActive;
	bool deferredSaveCaretPosition;

	void applyCaretPosition (int caretPosition);
};

//************************************************************************************************
// EditBox::TransactionScope
//************************************************************************************************

class EditBox::TransactionScope
{
public:
	TransactionScope (EditBox::UndoHandler* undoHandler, EditBox::UndoHandler::ActionID actionId)
	: undoHandler (undoHandler)
	{
		if(undoHandler)
			undoHandler->beginTransaction (actionId);
	}

	~TransactionScope ()
	{
		if(undoHandler)
			undoHandler->endTransaction ();
	}

private:
	EditBox::UndoHandler* undoHandler;
};

//************************************************************************************************
// EditBoxAccessibilityProvider
//************************************************************************************************

class EditBoxAccessibilityProvider: public ValueControlAccessibilityProvider
{
public:
	DECLARE_CLASS_ABSTRACT (EditBoxAccessibilityProvider, ValueControlAccessibilityProvider)

	EditBoxAccessibilityProvider (EditBox& owner);
	
	// ValueControlAccessibilityProvider
	AccessibilityElementRole CCL_API getElementRole () const override;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_IID_ (ITextParamProvider, 0x91a8d451, 0xddae, 0x4b52, 0xb0, 0x57, 0xc7, 0xe3, 0x18, 0x30, 0x7, 0xa7)

//************************************************************************************************
// EditBox
//************************************************************************************************

BEGIN_STYLEDEF (EditBox::customStyles)
	{"password",		Styles::kTextBoxBehaviorPasswordEdit},
	{"doubleclick",		Styles::kTextBoxBehaviorDoubleClickEdit},
	{"dialogedit",		Styles::kEditBoxBehaviorDialogEdit},
	{"musthittext",		Styles::kEditBoxBehaviorMustHitText},
	{"immediate",		Styles::kEditBoxBehaviorImmediate},
	{"hidetext",		Styles::kEditBoxAppearanceHideText},
	{"nowheel",  		Styles::kEditBoxBehaviorNoWheel},
	{"nosuggestions",	Styles::kEditBoxBehaviorNoSuggestions},
	{"noclearbutton",	Styles::kEditBoxBehaviorNoClearButton},
	{"extended",		Styles::kEditBoxBehaviorExtended},
	{"dragtext",		Styles::kEditBoxBehaviorDragText},
	{"permanentcaret",	Styles::kEditBoxBehaviorPermanentCaret},
	{"opaqueedit",		Styles::kEditBoxAppearanceOpaqueEdit},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::inKeyDown = false;
int EditBox::editCount = 0;
bool EditBox::isAnyEdtiting ()
{
	return editCount > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (EditBox, TextBox)
DEFINE_CLASS_UID (EditBox, 0x49292662, 0xD30F, 0x4C11, 0xBA, 0x7B, 0xAF, 0x41, 0x77, 0xCC, 0x17, 0xAB)
const Configuration::BoolValue EditBox::useNativeTextControl ("GUI.TextBox", "NativeControl", EDITBOX_USE_NATIVE_CONTROL);

//////////////////////////////////////////////////////////////////////////////////////////////////

EditBox::EditBox (const Rect& size, IParameter* param, StyleRef style, StringRef title)
: TextBox (size, param, style, title),
  undoHandler (NEW UndoHandler (*this)),
  currentDragHandler (nullptr),
  dragSelectionRange (0, 0),
  nativeControl (nullptr),
  returnKeyType (Styles::kReturnKeyDefault),
  keyboardType (Styles::kKeyboardTypeAutomatic),
  autofillType (Styles::kAutofillTypeNone),
  wantReopen (false),
  forceOpen (false),
  caretTextPosition (0),
  selectionTextPosition (0),
  canceled (false),
  textLayoutInitialized (false),
  clickCount (0),
  latestClickTime (0.0),
  selectionLocked (false)
{
	wantsFocus (true);
	noFocusOnContextMenu (true);
	setWheelEnabled (style.isCustomStyle (Styles::kEditBoxBehaviorNoWheel) == false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditBox::~EditBox ()
{
	ASSERT (nativeControl == nullptr)
	setNativeControl (nullptr);

	delete undoHandler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int EditBox::getEffectiveKeyboardType () const
{
	int effectiveKeyboardType = getKeyboardType ();
	if(param && param->getFormatter () == nullptr && effectiveKeyboardType == Styles::kKeyboardTypeAutomatic)
	{
		switch(param->getType ())
		{
		case IParameter::kToggle :
		case IParameter::kInteger :
			effectiveKeyboardType = (param->getMin ().asInt () < 0) ? Styles::kKeyboardTypeNumericSigned : Styles::kKeyboardTypeNumeric;
			break;
			
		case IParameter::kFloat :
			effectiveKeyboardType = (param->getMin ().asFloat () < 0.f) ? Styles::kKeyboardTypeDecimalSigned : Styles::kKeyboardTypeDecimal;
			break;
			
		default:
			effectiveKeyboardType = Styles::kKeyboardTypeGeneric;
		}
	}
	
	return effectiveKeyboardType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::onIdleTimer ()
{
	bool showEdit = isEditing () && caret && !caret->isVisible ();
	bool showPermanent = !isEditing () && style.isCustomStyle (Styles::kEditBoxBehaviorPermanentCaret);
	showCaret (showEdit || showPermanent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::showCaret (bool state)
{
	if(shouldUseNativeControl ())
		return;

	if(caret == nullptr && state)
	{
		const IVisualStyle& vs = getVisualStyle ();
		Color textColor = vs.getTextColor ();
		AutoPtr<IDrawable> shape = NEW SolidDrawable (textColor);
		caret = Sprite::createSprite (this, shape, Rect ());
		updateCaretRect ();
	}

	if(caret)
	{
		if(!caret->isVisible () && state)
			caret->show ();
		else if(caret->isVisible () && !state)
			caret->hide ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::resetTimer ()
{
	showCaret (isEditing () || style.isCustomStyle (Styles::kEditBoxBehaviorPermanentCaret));
	startTimer (500, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::attached (View* parent)
{
	SuperClass::attached (parent);

	if(autofillType != Styles::kAutofillTypeNone)
		AutofillManager::instance ().addClient (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::removed (View* parent)
{
	if(autofillType != Styles::kAutofillTypeNone)
		AutofillManager::instance ().removeClient (this);

	stopTimer ();
	showCaret (false);

	SuperClass::removed (parent);

	if(nativeControl)
	{
		if(!nativeControl->isCanceled ())
			nativeControl->submitText ();
		setNativeControl (nullptr);
	}
	paramFocused (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::setNativeControl (NativeTextControl* toSet)
{
	if(nativeControl != toSet)
	{
		if(nativeControl)
		{
			NativeTextControl* oldControl = nativeControl;
			nativeControl = nullptr;
			oldControl->release ();
			editCount--;
		}
		
		nativeControl = toSet;
		
		if(nativeControl)
			editCount++;

		invalidate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* EditBox::getRenderer ()
{
	if(renderer == nullptr)
		renderer = getTheme ().createRenderer (ThemePainter::kEditBoxRenderer, visualStyle);

	return renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::setStyle (StyleRef style)
{
	// discard renderer if extended flag changes (see shouldUseNativeControl)
	if(shouldUseNativeControl () && style.isCustomStyle (Styles::kEditBoxBehaviorExtended) != getStyle ().isCustomStyle (Styles::kEditBoxBehaviorExtended))
		safe_release (renderer);

	SuperClass::setStyle (style);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeTextControl* EditBox::createNativeControl ()
{
	if(shouldUseNativeControl ())
	{
		ScopedFlag<kInCreateNativeControl> scope (privateFlags);

		Rect size;
		calculateNativeControlSize (size);

		NativeTextControl* nc = NativeTextControl::create (*this, size, getReturnKeyType (), getEffectiveKeyboardType ());
		nc->setImmediateUpdate (style.isCustomStyle (Styles::kEditBoxBehaviorImmediate));
		return nc;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::calculateNativeControlSize (Rect& size)
{
	getRenderer ()->getPartRect (this, kPartNativeControlArea, size);
	
	// use sizeLimits maxWidth if applicable and sizeMode hfit is set
	if((getSizeMode () & View::SizeModes::kHFitSize) != 0)
	{
		if(getSizeLimits ().isValid ())
			size.right = getSizeLimits ().maxWidth;
	}

	// restrict native control size to visible area
	Rect visibleClient;
	getVisibleClient (visibleClient);
	size.bound (visibleClient);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IEditControlHost* EditBox::getEditControlHost ()
{
	UnknownPtr<IEditControlHost> host (GetViewInterfaceUpwards<IEditControlHost> (this));
	if(!host && getParameter ())
		host = getParameter ()->getController ();
	return host;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::onFocus (const FocusEvent& event)
{
	takeEditFocusInternal (event.eventType == FocusEvent::kSetFocus, event.directed);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::takeEditFocusInternal (bool state, bool directed)
{
	if(!shouldUseNativeControl () && state == isFocused ())
		return;

	if(state)
	{
		if(!style.isCustomStyle (Styles::kTextBoxBehaviorDoubleClickEdit) || inKeyDown || wantReopen || forceOpen)
		{
			if(!shouldUseNativeControl ())
				updatePadding ();

			Rect r;
			getClientRect (r);

			Window* window = getWindow ();
			if(window && !window->shouldCollectUpdates ())
				window->redraw ();

			ASSERT (nativeControl == nullptr)
			if(!nativeControl && !inCreateNativeControl ())
				setNativeControl (createNativeControl ());

			paramFocused (true);
			showCaret (true);

			if(!shouldUseNativeControl () && currentDragHandler == nullptr)
			{
				setChanged ();
				if(!style.isCustomStyle (Styles::kTextBoxAppearanceMultiLine))
				{
					Variant start = 0;
					Variant length = plainTextCache.length ();
					(NEW Message (IParameter::kSetSelection, start, length))->post (this, -1);
				}
			}
		}
		wantReopen = false;
	}
	else
	{
		wantReopen = false;

		if(nativeControl)
		{
			if(!nativeControl->isCanceled ())
				nativeControl->submitText ();
		}
		else
			setChanged ();

		setNativeControl (nullptr);

		Rect size (getSize ());
		size.expand (2);
		if(getParent ())
			getParent ()->invalidate (size);

		// if kKillFocus was caused by window deactivation, reopen in next kSetFocus
		wantReopen = !directed;
		if(directed && !style.isCustomStyle (Styles::kTextBoxAppearanceMultiLine) && isTextSelected ())
			setSelectionPosition (caretTextPosition);

		paramFocused (false);
		showCaret (style.isCustomStyle (Styles::kEditBoxBehaviorPermanentCaret));

		if(IEditControlHost* host = getEditControlHost ())
			host->onEditControlLostFocus (this);
	}

	forceOpen = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::onSize (const Point& delta)
{
	SuperClass::onSize (delta);

	if(shouldUseNativeControl ())
	{
		if(nativeControl)
		{
			Rect clientArea;
			calculateNativeControlSize (clientArea);
			if(!clientArea.isEmpty ())
				nativeControl->setSize (clientArea);
			else
				killFocus ();
		}
	}
	else
	{
		updateCaretRect ();
		updateSelection ();
	}

	if(autofillType != Styles::kAutofillTypeNone)
		AutofillManager::instance ().updateClient (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::onMove (const Point& delta)
{
	SuperClass::onMove (delta);

	if(shouldUseNativeControl ())
	{
		if(nativeControl)
		{
			Rect clientArea;
			calculateNativeControlSize (clientArea);
			if(!clientArea.isEmpty ())
				nativeControl->setSize (clientArea);
			else
				killFocus ();
		}
	}

	if(autofillType != Styles::kAutofillTypeNone && isAttached ())
		AutofillManager::instance ().updateClient (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::buildTextLayout ()
{
	SuperClass::buildTextLayout ();
	
	if(!shouldUseNativeControl () && !textLayoutInitialized)
	{
		TransactionScope scope (undoHandler, UndoHandler::ActionID::kInitialize);
		textLayoutInitialized = true;
	}

	bool multiline = style.isCustomStyle (Styles::kTextBoxAppearanceMultiLine);
	if(!multiline && textLayout)
	{
		RectF bounds;
		textLayout->getBounds (bounds);
		Rect rect = getTextRect ();
		int newAlignH = getVisualStyle ().getTextAlignment ().getAlignH ();
		if(bounds.getWidth () > rect.getWidth ())
			newAlignH = Alignment::kLeft;

		if(savedAlignment.getAlignH () != newAlignH)
		{
			savedAlignment.setAlignH (newAlignH);
			SuperClass::buildTextLayout ();
		}
	}

	updateCaretRect ();
	updateSelection ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::setCaretPosition (int textPosition, bool implicit)
{
	ASSERT (textPosition >= 0)

	if(selectionLocked)
		return;

	caretTextPosition = textPosition;
	updateCaretRect ();
	updateSelection ();
	if(!implicit)
		lastExplicitCaretPosition = caretRect.getLeftTop ();

	if(inKeyDown)
		makeVisible (caretRect);

	invalidate ();
	resetTimer ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::setSelectionPosition (int textPosition)
{
	if(selectionLocked)
		return;

	selectionTextPosition = textPosition;
	updateSelection ();
	invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::updateCaretRect ()
{
	if(textLayout && !shouldUseNativeControl ())
	{
		bool multiline = style.isCustomStyle (Styles::kTextBoxAppearanceMultiLine);
		Rect r = getTextRect ();
		RectF bounds;
		if(caretTextPosition >= 0 && textLayout->getCharacterBounds (bounds, caretTextPosition) == kResultOk)
		{
			caretRect = rectFToInt (bounds);
			caretRect.setWidth (1);
			if(!multiline)
			{
				Coord textWidth = r.getWidth ();
				const Coord kOffsetShift = 20;
				RectF bounds;
				textLayout->getBounds (bounds);
				Coord newDisplayOffset = displayOffset;
				if(bounds.getWidth () <= textWidth)
					newDisplayOffset = 0;
				else
				{
					Coord maxOffset = Coord (bounds.getWidth () - textWidth);
					if(caretRect.left - displayOffset <= 0)
						newDisplayOffset = ccl_max (0, caretRect.left - kOffsetShift);
					else if(caretRect.left - displayOffset >= textWidth)
						newDisplayOffset = ccl_min (maxOffset, caretRect.left - (textWidth - kOffsetShift));
					else if(displayOffset > maxOffset)
						newDisplayOffset = maxOffset;
				}

				if(newDisplayOffset != displayOffset)
				{
					displayOffset = newDisplayOffset;
					setChanged ();
				}

				caretRect.offset (-displayOffset, 0);
			}
		}
		else
			caretRect.setEmpty ();

		if(caret)
			caret->move (Rect (caretRect).offset (r.getLeftTop ()));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::updateSelection ()
{
	if(dragSelectionRange.length != 0)
		return;

	selection.setEmpty ();
	if(textLayout && !shouldUseNativeControl () && isTextSelected ())
	{
		int left = ccl_min (caretTextPosition, selectionTextPosition);
		int right = ccl_max (caretTextPosition, selectionTextPosition);
		textLayout->getTextBounds (selection, {left, right - left});
		if(displayOffset != 0)
		{
			Vector<Rect> offsetSelection;
			for(Rect rect : selection.getRects ())
			{
				rect.offset (-displayOffset, 0);
				offsetSelection.add (rect);
			}
			selection.setEmpty ();
			for(RectRef rect : offsetSelection)
				selection.addRect (rect);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String EditBox::getSelectedText (bool plainText) const
{
	int textIndex = ccl_min (selectionTextPosition, caretTextPosition);
	int length = ccl_abs (selectionTextPosition - caretTextPosition);

	String text;
	if(textModel && !plainText)
		textModel->copyText (text, textIndex, length);
	else
		text = plainTextCache.subString (textIndex, length);

	return text;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::dragSelection ()
{
	dragSelectionRange.start = ccl_min (selectionTextPosition, caretTextPosition);
	dragSelectionRange.length = ccl_abs (selectionTextPosition - caretTextPosition);

	AutoPtr<DragSession> session (DragSession::create (this->asUnknown ()));
	session->setSource (this->asUnknown ());
	session->getItems ().add (ccl_as_unknown (NEW Boxed::String (getSelectedText (true))), false);
	setCursor (static_cast<MouseCursor*> (nullptr));

	session->drag ();

	dragSelectionRange.start = 0;
	dragSelectionRange.length = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::selectAll ()
{
	undoHandler->resetAction ();
	setCaretPosition (0);
	setSelectionPosition (plainTextCache.length ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::deleteSelection ()
{
	deleteSelectionInternal (int(UndoHandler::ActionID::kDeleteLeading));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int EditBox::deleteSelectionInternal (int actionID)
{
	int startPosition = ccl_min (caretTextPosition, selectionTextPosition);
	int endPosition = ccl_max (caretTextPosition, selectionTextPosition);
	int caretOffset = removeTextInternal (startPosition, endPosition - startPosition, actionID);
	setCaretPosition (caretTextPosition - caretOffset);
	setSelectionPosition (caretTextPosition);

	return caretOffset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::cutSelection ()
{
	copySelection ();
	deleteSelectionInternal (int(UndoHandler::ActionID::kCut));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::copySelection ()
{
	Clipboard::instance ().setText (getSelectedText (false));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::paste ()
{
	String text;
	if(Clipboard::instance ().getText (text))
		insertTextInternal (text, int(UndoHandler::ActionID::kPaste));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::onActivate (bool state)
{
	if(!state)
	{
		killFocus ();
		SuperClass::onActivate (state);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::canHandleDoubleTap () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::onVisualStyleChanged ()
{
	SuperClass::onVisualStyleChanged ();

	if(nativeControl)
		nativeControl->updateVisualStyle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::acceptClick (const MouseEvent& event)
{
	if(!event.keys.isSet (KeyState::kLButton))
		return false;

	if(style.isCustomStyle (Styles::kEditBoxBehaviorMustHitText))
	{
		Rect r;
		getRenderer ()->getPartRect (this, kPartTextExtent, r); 
		makeVisible (r);

		// if text is too small (e.g. empty), accept click on left half
		if(r.getWidth () < 10)
			r.setWidth (ccl_max (10, getWidth () / 2));
		if(r.getHeight () < 10)
			r.setHeight (getHeight ());

		return r.pointInside (event.where);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* EditBox::createMouseHandler (const MouseEvent& event)
{
	if(isResetClick (event) && isEnabled () && !style.isCustomStyle (Styles::kTextBoxBehaviorDoubleClickEdit))
	{
		performReset ();
		return NEW NullMouseHandler (this);	// swallow mouse click
	}

	if(!shouldUseNativeControl () && isEditing () && event.keys.isSet (KeyState::kLButton))
		return NEW EditBoxMouseHandler (this);

	return SuperClass::createMouseHandler (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDragHandler* EditBox::createDragHandler (const DragEvent& event)
{
	if(shouldUseNativeControl ())
		return SuperClass::createDragHandler (event);

	if(isEditing ())
		return NEW TextDragHandler (*this);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* EditBox::createTouchHandler (const TouchEvent& event)
{
	GestureHandler* handler = NEW GestureHandler (this);
	
	if(style.isCustomStyle (Styles::kTextBoxBehaviorDoubleClickEdit))
		handler->addRequiredGesture (GestureEvent::kDoubleTap);
	else
		handler->addRequiredGesture (GestureEvent::kSingleTap);
	
	return handler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::onGesture (const GestureEvent& event)
{
	if(event.getType () == GestureEvent::kDoubleTap && style.isCustomStyle (Styles::kTextBoxBehaviorDoubleClickEdit))
	{
		setNativeControl (createNativeControl ());
		paramFocused (true);
		return true;
	}
	return SuperClass::onGesture (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::handleClick (const MouseEvent& event)
{
	if(!acceptClick (event))
		return false;

	int timesClicked = event.doubleClicked == 1 ? 2 : 1;
	double currentTime = System::GetProfileTime ();
	double doubleClickDelay = System::GetGUI ().getDoubleClickDelay ();
	if(currentTime - latestClickTime < doubleClickDelay && clickCount < 3)
		clickCount += timesClicked;
	else
		clickCount = timesClicked;

	latestClickTime = currentTime;

	if(clickCount == 1 && (isFocused () || !style.isCustomStyle (Styles::kTextBoxBehaviorDoubleClickEdit)))
	{
		if(!shouldUseNativeControl ())
			return false; // let mouse handler perform
	}
	else if(clickCount > 1)
	{
		if(!shouldUseNativeControl () && isFocused ())
		{
			if(clickCount == 2) // select word
				selectWordOrLine (false);
			else if(clickCount == 3) // select line
				selectWordOrLine (true);
		}
		else
		{
			clickCount = 0;

			if(shouldUseNativeControl ())
			{
				Rect r;
				getClientRect (r);
				makeVisible (r);

				if(!nativeControl)
					setNativeControl (createNativeControl ());
			}

			paramFocused (true);
		}

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::onMouseDown (const MouseEvent& event)
{
	bool oldState = selectionLocked;
	selectionLocked = false;
	ScopedVar<bool> guard (selectionLocked, oldState);

	if(tryModelEditText (event))
		return true;

	if(!shouldUseNativeControl () && isFocused () && event.keys.isSet (KeyState::kLButton))
	{
		if(style.isCustomStyle (Styles::kEditBoxBehaviorDragText))
		{
			for(RectRef rect : selection.getRects ())
			{
				if(rect.pointInside (event.where) && detectDrag (event))
				{
					dragSelection ();
					return true;
				}
			}
		}
	}

	if(style.isCustomStyle (Styles::kTextBoxBehaviorDoubleClickEdit) && !isEditing () && !detectDoubleClick (event))
	{
		// pass through single click (derived class might behave different!)
		if(myClass () == ccl_typeid<EditBox> ())
		{
			View::onMouseDown (event);
			return false;
		}
	}

	if(handleClick (event))
		return true;

	View::onMouseDown (event);
	return isEnabled (); // An editbox will always swallow the mouseclick, if it is enabled. Any action happens on focus event.
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::onMouseEnter (const MouseEvent& event)
{
	if(shouldUseNativeControl ())
		return SuperClass::onMouseEnter (event);

	if(isEditing ())
		setCursor (getTextCursor ());

	SuperClass::onMouseEnter (event);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::onMouseMove (const MouseEvent& event)
{
	bool selectionHovered = false;
	if(!shouldUseNativeControl () && isEditing ())
	{
		for(RectRef rect : selection.getRects ())
		{
			if(rect.pointInside (event.where))
			{
				selectionHovered = true;
				break;
			}
		}

		if(selectionHovered)
			setCursor (getTheme ().getThemeCursor (ThemeElements::kTextCursor));
		else
			setCursor (getTextCursor ());
	}

	return SuperClass::onMouseMove (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::moveCaretTo (const Point& point, bool clearSelection)
{
	if(isFocused ())
	{
		Rect rect = getTextRect ();
		int textPosition = 0;
		PointF position (point.x, point.y);
		position.offset (displayOffset - rect.left, -rect.top);
		if(textLayout && textLayout->hitTest (textPosition, position) == kResultOk)
		{
			setCaretPosition (textPosition);

			if(clearSelection)
				setSelectionPosition (caretTextPosition);

			undoHandler->resetAction ();
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::selectWordOrLine (bool selectLine)
{
	if(textLayout == nullptr)
		return;

	ITextLayout::Range range (0, 0);
	tresult result = kResultFailed;
	if(selectLine)
		result = textLayout->getExplicitLineRange (range, caretTextPosition);
	else
		result = textLayout->getWordRange (range, caretTextPosition);

	if(result == kResultOk)
	{
		caretTextPosition = range.start + range.length;
		selectionTextPosition = range.start;

		setCaretPosition (caretTextPosition);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::onContextMenu (const ContextMenuEvent& event)
{
	if(shouldUseNativeControl ())
		return Control::onContextMenu (event); // use OS context menu

	if(isFocused ())
	{
		bool result = Control::onContextMenu (event);

		if(style.isCustomStyle (Styles::kTextBoxBehaviorNoContextMenu) == false)
		{
			Rect r;
			getRenderer ()->getPartRect (this, kPartTextExtent, r);
			if(!r.isEmpty () && (event.wasKeyPressed || r.pointInside (event.where)))
			{
				auto addEditCommand = [this,&event] (CStringRef name)
				{
					CommandWithTitle command = CommandRegistry::find ("Edit", name);
					event.contextMenu.addCommandItem (command, this);
				};

				event.contextMenu.addSeparatorItem ();
				addEditCommand ("Cut");
				addEditCommand ("Copy");
				addEditCommand ("Paste");
				addEditCommand ("Delete");
				addEditCommand ("Select All");
				event.contextMenu.addSeparatorItem ();
				addEditCommand ("Undo");
				addEditCommand ("Redo");
				result = true; // avoid other items to appear
			}
		}

		return result;
	}
	else
		return SuperClass::onContextMenu (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::onKeyDown (const KeyEvent& event)
{
	if(!shouldUseNativeControl ())
		GUI.hideTooltip ();

	ScopedVar<bool> guard (inKeyDown, true);
	selectionLocked = false;

	// check for IEditControlHost interface: parent views or controller
	IEditControlHost* host = getEditControlHost ();
	if(host && host->onEditNavigation (event, this))
		return true;

	if(!shouldUseNativeControl ())
	{
		if(handleKeyDown (event))
			return true;

		if(tryEdit (event))
			return true;

		if(FocusNavigator::instance ().onKeyDown (event))
		{
			if(style.isCustomStyle (Styles::kEditBoxBehaviorImmediate))
				(NEW Message ("checkSubmit"))->post (this);
			return true;
		}
	}

	return SuperClass::onKeyDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::handleKeyDown (const KeyEvent& e)
{
	bool shiftDown = (e.state.getModifiers () & KeyState::kShift);
	bool commandDown = (e.state.getModifiers () & KeyState::kCommand);
	
	switch(e.vKey)
	{
	case VKey::kLeft :
		if(isTextSelected () && !shiftDown)
		{
			setCaretPosition (ccl_min (selectionTextPosition, caretTextPosition));
			setSelectionPosition (caretTextPosition);
		}
		else if(caretTextPosition > 0)
		{
			if(caretTextPosition -1 > 0 && plainTextCache[caretTextPosition - 1] == '\n' && plainTextCache[caretTextPosition - 2] == '\r')
				setCaretPosition (caretTextPosition - 2);
			else
				setCaretPosition (caretTextPosition - 1);
			if(!shiftDown)
				setSelectionPosition (caretTextPosition);
		}
		undoHandler->resetAction ();
		return true;

	case VKey::kRight :
		if(isTextSelected () && !shiftDown)
		{
			setCaretPosition (ccl_max (selectionTextPosition, caretTextPosition));
			setSelectionPosition (caretTextPosition);
		}
		else if(caretTextPosition < plainTextCache.length ())
		{
			if(caretTextPosition + 1 < plainTextCache.length () && plainTextCache[caretTextPosition] == '\r' && plainTextCache[caretTextPosition + 1] == '\n')
				setCaretPosition (caretTextPosition + 2);
			else
				setCaretPosition (caretTextPosition + 1);
			if(!shiftDown)
				setSelectionPosition (caretTextPosition);
		}
		undoHandler->resetAction ();
		return true;

	case VKey::kUp :
		if(textLayout && style.isCustomStyle (Styles::kTextBoxAppearanceMultiLine))
		{
			PointF newCaretPosition;
			newCaretPosition.x = lastExplicitCaretPosition.x;

			ITextLayout::Range lineRange (0, 0);
			if(textLayout->getLineRange (lineRange, caretTextPosition) != kResultOk)
				break;

			int endOfPreviousLine = lineRange.start - 1;
			if(endOfPreviousLine < 0)
			{
				endOfPreviousLine = 0;
				newCaretPosition.x = 0;
			}
			if(textLayout->getLineRange (lineRange, endOfPreviousLine) != kResultOk)
				break;

			RectF previousLineRect;
			if(textLayout->getCharacterBounds (previousLineRect, lineRange.start) != kResultOk)
				break;

			newCaretPosition.y = previousLineRect.top + (previousLineRect.getHeight () / 2);

			Rect rect = getTextRect ();
			int textPosition = 0;
			newCaretPosition.offset (displayOffset - rect.left, 0);
			if(textLayout->hitTest (textPosition, newCaretPosition) == kResultOk)
			{
				setCaretPosition (textPosition, true);
				if(!shiftDown)
					setSelectionPosition (caretTextPosition);
			}
			undoHandler->resetAction ();
			return true;
		}
		break;

	case VKey::kDown :
		if(textLayout && style.isCustomStyle (Styles::kTextBoxAppearanceMultiLine))
		{
			ITextLayout::Range lineRange (0, 0);
			if(textLayout->getLineRange (lineRange, caretTextPosition) != kResultOk)
				break;

			int startOfNextLine = lineRange.start + lineRange.length;
			if(textLayout->getLineRange (lineRange, startOfNextLine) != kResultOk)
				break;

			RectF nextLineRect;
			if(textLayout->getCharacterBounds (nextLineRect, lineRange.start) != kResultOk)
				break;

			PointF newCaretPosition;
			newCaretPosition.x = lastExplicitCaretPosition.x;
			newCaretPosition.y = nextLineRect.top + (nextLineRect.getHeight () / 2);

			Rect rect = getTextRect ();
			int textPosition = 0;
			newCaretPosition.offset (displayOffset - rect.left, 0);
			if(textLayout->hitTest (textPosition, newCaretPosition) == kResultOk)
			{
				if(startOfNextLine >= plainTextCache.length ())
					textPosition = plainTextCache.length ();

				setCaretPosition (textPosition, true);
				if(!shiftDown)
					setSelectionPosition (caretTextPosition);
			}

			undoHandler->resetAction ();
			return true;
		}
		break;

	case VKey::kHome :
		if(textLayout)
		{
			if(commandDown)
				setCaretPosition (0);
			else
			{
				ITextLayout::Range lineRange (0, 0);
				if(textLayout->getLineRange (lineRange, caretTextPosition) != kResultOk)
					break;

				setCaretPosition (lineRange.start);
			}
			if(!shiftDown)
				setSelectionPosition (caretTextPosition);
		}
		undoHandler->resetAction ();
		return true;

	case VKey::kEnd :
		if(textLayout)
		{
			if(commandDown)
				setCaretPosition (plainTextCache.length ());
			else
			{
				ITextLayout::Range lineRange (0, 0);
				if(textLayout->getLineRange (lineRange, caretTextPosition) != kResultOk)
					break;

				setCaretPosition (lineRange.start + lineRange.length - 1);
			}
			if(!shiftDown)
				setSelectionPosition (caretTextPosition);
		}
		undoHandler->resetAction ();
		return true;

	case VKey::kEscape :
	case VKey::kEnter :
	case VKey::kReturn :
		{
			canceled = (e.vKey == VKey::kEscape) ? true : false;

			bool isMultiline = false;

			 // on iOS, always close keyboard on "return"/"go"/"done"/... (no linefeed functionality in the multiline case)
			#if !CCL_PLATFORM_IOS
			if(style.isCustomStyle (Styles::kTextBoxAppearanceMultiLine))
				isMultiline = true;
			#endif

			bool nonDialogEdit = true;
			if(style.isCustomStyle (Styles::kEditBoxBehaviorDialogEdit))
				nonDialogEdit = false;

			if(isMultiline == false || canceled)
			{
				killFocus ();
				return true;
			}
		}
		break;
	}

	if(commandDown && e.isCharValid ())
	{
		switch(e.character)
		{
		case 'a' :
			selectAll ();
			return true;
		case 'c' :
			if(isTextSelected ())
				copySelection ();
			return true;
		case 'v' :
			paste ();
			return true;
		case 'x' :
			if(isTextSelected ())
				cutSelection ();
			return true;
		case 'z' :
			if(undoHandler->undo ())
				return true;
			break;
		case 'Z' :
		case 'y' :
			if(undoHandler->redo ())
				return true;
			break;
		}
	}

	canceled = false;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::tryEdit (const KeyEvent& event)
{
	switch(event.vKey)
	{
	case VKey::kEnter :
	case VKey::kReturn :
		{
			bool isMultiline = false;

			// on iOS, always close keyboard on "return"/"go"/"done"/... (no linefeed functionality in the multiline case)
			#if !CCL_PLATFORM_IOS
			if(style.isCustomStyle (Styles::kTextBoxAppearanceMultiLine))
				isMultiline = true;
			#endif

			if(isMultiline)
			{
				insertText (String::getLineEnd ());
				return true;
			}
		}
		break;

	case VKey::kBackspace :
		{
			int actionID = int(UndoHandler::ActionID::kDeleteTrailing);
			if(isTextSelected ())
			{
				deleteSelectionInternal (actionID);
				return true;
			}

			int caretOffset = removeTextInternal (caretTextPosition, -1, actionID);
			setCaretPosition (caretTextPosition - caretOffset);
			setSelectionPosition (caretTextPosition);

			return true;
		}

	case VKey::kDelete :
		if(isTextSelected ())
		{
			deleteSelection ();
			return true;
		}

		undoHandler->setDeferSaveCaretPosition ();
		if(removeTextInternal (caretTextPosition, 1, int(UndoHandler::ActionID::kDeleteLeading)) != 0)
			return true;

		break;
	}

	if(event.isComposedCharValid () && (Unicode::isPrintable (event.composedCharacter) || (event.composedCharacter == '\t' && style.isCustomStyle (Styles::kTextBoxAppearanceMultiLine))))
	{
		insertText (String ().append (&event.composedCharacter, 1));
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API EditBox::insertText (StringRef text)
{
	bool isSingleCharacter = text.length () == 1 || text == String::getLineEnd (Text::kCRLFLineFormat);
	insertTextInternal (text, int(isSingleCharacter ? UndoHandler::ActionID::kType : UndoHandler::ActionID::kPaste));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::insertTextInternal (StringRef text, int actionID)
{
	if(textModel == nullptr)
		return;

	deleteSelectionInternal (actionID);
	TransactionScope scope (undoHandler, UndoHandler::ActionID(actionID));
	int insertedCharacters = textModel->insertText (caretTextPosition, text, undoHandler->isUndoActive () ? 0 : ITextModel::kMergeUndo);
	setCaretPosition (caretTextPosition + insertedCharacters);
	setSelectionPosition (caretTextPosition);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API EditBox::removeText (int length)
{
	removeTextInternal (caretTextPosition, length, int(length > 0 ? UndoHandler::ActionID::kDeleteLeading : UndoHandler::ActionID::kDeleteTrailing));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API EditBox::setCaret (int textPosition)
{
	setCaretPosition (textPosition);
	setSelectionPosition (textPosition);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int EditBox::removeTextInternal (int textPosition, int length, int actionID)
{
	if(textModel == nullptr || length == 0
		|| textPosition < 0 || (textPosition == 0 && length < 0)
		|| textPosition > plainTextCache.length () || (textPosition == plainTextCache.length () && length > 0))
	{
		return 0;
	}

	TransactionScope scope (undoHandler, UndoHandler::ActionID(actionID));

	ITextModel::EditOptions options = 0;
	if(!undoHandler->isUndoActive ())
		options |= ITextModel::kMergeUndo;

	int removedCharacters = textModel->removeText (textPosition, length, options);
	if(selectionTextPosition > caretTextPosition)
		return 0; // we return a caret offset. If selected text after the caret is removed, the caret stays where it is.

	return removedCharacters;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::paramFocused (bool state)
{
	if(!shouldUseNativeControl ())
	{
		if(isFocused () != state)
		{
			isFocused (state);
			if(state)
			{
				editCount++;
				selectionLocked = isTextSelected ();
				setChanged (); // make sure to rebuild the text layout to not show a collapsed string when editing
			}
			else
				editCount--;

			invalidate ();

			if(!state && !canceled)
				submit ();
		}

		if(state)
		{
			if(caretTextPosition > plainTextCache.length ())
				setCaretPosition (plainTextCache.length (), true);
			else
				resetTimer ();
		}
	}

	IParamPreviewHandler* previewHandler = getPreviewHandler ();
	if(!previewHandler || !param)
		return;

	ParamPreviewEvent e;
	e.type = state ? ParamPreviewEvent::kFocus : ParamPreviewEvent::kUnfocus;
	previewHandler->paramPreview (param, e);
	ASSERT (e.handlerData.isNil ()) // unsupported
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::submit (bool check)
{
	if(IParameter* param = getTextParameter ())
	{
		// set new parameter value
		String modelString;
		textModel->toParamString (modelString);
		String paramString;
		param->toString (paramString);
		if(check && modelString == paramString)
			return;

		param->beginEdit ();
		param->fromString (modelString, true);
		param->endEdit ();

		// update text from parameter value (may have changed due to parameter validation)
		param->toString (paramString);
		textModel->fromParamString (paramString);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::paramChanged ()
{
	setChanged ();

	if(nativeControl)
	{
		if(nativeControl->isImmediateUpdate ())
		{
			// suppress update if text did not change
			String text;
			nativeControl->getControlText (text);
			if(IParameter* param = getTextParameter ())
			{
				String paramString;
				param->toString (paramString);
				if(paramString == text)
					return;
			}
		}
		nativeControl->updateText ();
	}
	else
		SuperClass::paramChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API EditBox::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IParameter::kSetSelection)
	{
		int start = msg[0];
		int length = msg[1];
		if(nativeControl)
			nativeControl->setSelection (start, length);

		if(!shouldUseNativeControl ())
		{
			setCaretPosition (start);
			if(length >= 0)
				setSelectionPosition (start + length);
			else
				setSelectionPosition (caretTextPosition);
		}
	}

	bool isImmediate = style.isCustomStyle (Styles::kEditBoxBehaviorImmediate);
	bool textModelChanged = msg == kChanged && textModel && isEqualUnknown (subject, textModel);

	if(msg == IParameter::kRequestFocus)
	{
		if(isAttached ())
		{
			forceOpen = true;
			takeFocus ();
		}
	}
	else if(msg == IParameter::kReleaseFocus)
	{
		if(isAttached () && isFocused ())
			killFocus ();
	}
	else if(!shouldUseNativeControl () && (msg == "checkSubmit" || (textModelChanged && isImmediate)))
	{
		submit (true);

		if(textModelChanged)
			SuperClass::notify (subject, msg);
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::getCaretRect (Rect& rect) const
{
	rect = caretRect;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ConstVector<Rect>& EditBox::getSelection () const
{
	return selection.getRects ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::isEditing () const
{
	return !shouldUseNativeControl () && (isFocused () || currentDragHandler != nullptr) || nativeControl != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* EditBox::getAccessibilityProvider ()
{
	if(!accessibilityProvider)
		accessibilityProvider = NEW EditBoxAccessibilityProvider (*this);
	return accessibilityProvider;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditBox::interpretCommand (const CommandMsg& msg)
{
	if(shouldUseNativeControl () || !isEditing ())
		return SuperClass::interpretCommand (msg);

	if(msg.category == "Edit")
	{
		if(msg.name == "Undo")
		{
			if(!msg.checkOnly ())
				undoHandler->undo ();
			return undoHandler->canUndo ();
		}
		if(msg.name == "Redo")
		{
			if(!msg.checkOnly ())
				undoHandler->redo ();
			return undoHandler->canRedo ();
		}

		if(msg.name == "Paste")
		{
			if(!msg.checkOnly ())
				paste ();
			return true;
		}
		if(msg.name == "Select All")
		{
			if(!msg.checkOnly ())
				selectAll ();
			return true;
		}

		if(!isTextSelected ())
			return false;

		if(msg.name == "Delete")
		{
			if(!msg.checkOnly ())
				deleteSelection ();
			return true;
		}
		if(msg.name == "Cut")
		{
			if(!msg.checkOnly ())
				cutSelection ();
			return true;
		}
		if(msg.name == "Copy")
		{
			if(!msg.checkOnly ())
				copySelection ();
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int EditBox::getAutofillClientType () const
{
	return getAutofillType ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* EditBox::getAutofillClientView ()
{
	return this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::receiveAutofillText (StringRef text)
{
	setCaret (0);
	removeText (plainTextCache.length ());
	insertText (text);
	submit (true);
}

//************************************************************************************************
// EditBox::UndoHandler
//************************************************************************************************

EditBox::UndoHandler::UndoHandler (EditBox& editBox)
: editBox (editBox),
  lastAction (ActionID::kNone),
  caretPosition (-1),
  undoActive (false),
  deferredSaveCaretPosition (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::UndoHandler::resetAction ()
{
	lastAction = ActionID::kNone;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::UndoHandler::saveCaretPosition ()
{
	caretPosition = editBox.caretTextPosition;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::UndoHandler::setDeferSaveCaretPosition ()
{
	deferredSaveCaretPosition = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::UndoHandler::beginTransaction (ActionID actionId)
{
	undoActive = lastAction != actionId;
	lastAction = actionId;

	if(!deferredSaveCaretPosition)
		saveCaretPosition ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::UndoHandler::endTransaction ()
{
	if(deferredSaveCaretPosition)
		saveCaretPosition ();

	deferredSaveCaretPosition = false;

	if(!undoActive)
	{
		caretPosition = undoStack.peek ().firstCaretPosition;
		undoStack.pop ();
	}

	undoStack.push ({caretPosition, editBox.caretTextPosition});
	redoStack.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::UndoHandler::applyCaretPosition (int caretPosition)
{
	editBox.setCaretPosition (caretPosition);
	editBox.setSelectionPosition (caretPosition);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::UndoHandler::canUndo () const
{
	if(undoStack.count () < 2)
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::UndoHandler::canRedo () const
{
	if(redoStack.isEmpty ())
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::UndoHandler::undo ()
{
	if(canUndo () == false)
		return false;

	redoStack.push (undoStack.pop ());
	if(editBox.textModel && editBox.textModel->undo ())
	{
		applyCaretPosition (redoStack.peek ().firstCaretPosition);
		lastAction = ActionID::kNone;
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::UndoHandler::redo ()
{
	if(canRedo () == false)
		return false;

	if(editBox.textModel && editBox.textModel->redo ())
	{
		applyCaretPosition (redoStack.peek ().lastCaretPosition);
		undoStack.push (redoStack.pop ());
		lastAction = ActionID::kNone;
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::UndoHandler::isUndoActive () const
{
	return undoActive;
}

//************************************************************************************************
// NativeTextControl
//************************************************************************************************

DEFINE_STRINGID_ (Signals::kNativeTextControl, "CCL.NativeTextControl")
DEFINE_STRINGID_ (Signals::kNativeTextControlCreated, "NativeTextControlCreated")
DEFINE_STRINGID_ (Signals::kNativeTextControlDestroyed, "NativeTextControlDestroyed")

//////////////////////////////////////////////////////////////////////////////////////////////////

int NativeTextControl::textControlCount = 0;

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_STYLEDEF (NativeTextControl::returnKeyTypes)
	{"default",	Styles::kReturnKeyDefault},
	{"go",		Styles::kReturnKeyGo},
	{"next",	Styles::kReturnKeyNext},
	{"search",	Styles::kReturnKeySearch},
	{"send",	Styles::kReturnKeySend},
	{"done",	Styles::kReturnKeyDone},
END_STYLEDEF

BEGIN_STYLEDEF (NativeTextControl::keyboardTypes)
	{"automatic",		Styles::kKeyboardTypeAutomatic},
	{"default",			Styles::kKeyboardTypeGeneric},
	{"email",			Styles::kKeyboardTypeEmail},
	{"url",				Styles::kKeyboardTypeUrl},
	{"phone",			Styles::kKeyboardTypePhoneNumber},
	{"numeric",			Styles::kKeyboardTypeNumeric},
	{"numericsigned",	Styles::kKeyboardTypeNumericSigned},
	{"decimal",			Styles::kKeyboardTypeDecimal},
	{"decimalsigned",	Styles::kKeyboardTypeDecimalSigned},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeTextControl::NativeTextControl (Control& owner, int returnKeyType, int keyboardType)
: owner (owner),
  canceled (true),
  inSubmitText (false),
  isSubmitted (false),
  immediateUpdate (false),
  returnKeyType (returnKeyType),
  keyboardType (keyboardType)
{
	textControlCount++;
	SignalSource (Signals::kNativeTextControl).signal (Message (Signals::kNativeTextControlCreated));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeTextControl::~NativeTextControl ()
{
	textControlCount--;
	SignalSource (Signals::kNativeTextControl).signal (Message (Signals::kNativeTextControlDestroyed));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Control& NativeTextControl::getOwner () const
{
	return owner;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NativeTextControl::handleKeyDown (const KeyEvent& e)
{
	GUI.hideTooltip ();

	switch(e.vKey)
	{
	case VKey::kEscape :
	case VKey::kEnter :
	case VKey::kReturn :
		{
			canceled = (e.vKey == VKey::kEscape) ? true : false;
				
			bool isMultiline = false;

			 // on iOS, always close keyboard on "return"/"go"/"done"/... (no linefeed functionality in the multiline case)
			#if !CCL_PLATFORM_IOS
			if(TextBox* textBox = ccl_cast<TextBox> (&owner))
				if(textBox->getStyle ().isCustomStyle (Styles::kTextBoxAppearanceMultiLine))
					isMultiline = true;
			#endif

			bool nonDialogEdit = true;
			if(EditBox* editBox = ccl_cast<EditBox> (&owner))
				if(editBox->getStyle ().isCustomStyle (Styles::kEditBoxBehaviorDialogEdit))
					nonDialogEdit = false;

			if(isMultiline == false || canceled)
			{
				if(nonDialogEdit == false)
					if(Dialog* dialog = ccl_cast<Dialog> (owner.getWindow ()))
					{
						// pass key to the dialog
						dialog->onKeyDown (e);
						return true;
					}

				if(owner.onKeyDown (e))
					return true;

				owner.killFocus (); // kills this!!!
				return false;
			}
			else if(isMultiline)
			{
				if(owner.onKeyDown (e))
					return true;
			}
		}
		break;

	case VKey::kUp :
	case VKey::kDown :
	case VKey::kPageUp :
	case VKey::kPageDown :
	case VKey::kTab :
	case VKey::kBackspace :
		if(owner.onKeyDown (e))
			return true;

		if(owner.getStyle ().isCustomStyle (Styles::kTextBoxAppearanceMultiLine)) // no focus navigation here...
			break;

		CCL_FALLTHROUGH
	default :
		if(FocusNavigator::instance ().onKeyDown (e))
			return true;
		break;
	}

	// allow commands with function keys
	if(e.vKey >= VKey::kF1 && e.vKey <=  VKey::kF24)
		if(CommandTable::instance ().translateKey (e))
			return true;

	if(isImmediateUpdate ())
		(NEW Message ("checkSubmit"))->post (this);

	canceled = false;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NativeTextControl::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "checkSubmit")
	{
		if(IParameter* param = getTextParameter ())
		{
			String text;
			getControlText (text);
			if(param->getValue ().asString () != text)
			{
				param->beginEdit ();
				param->fromString (text, true);
				param->endEdit ();
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* NativeTextControl::getTextParameter () const
{
	UnknownPtr<ITextParamProvider> provider (ccl_as_unknown (owner));
	ASSERT (provider.isValid () == true)
	return provider ? provider->getTextParameter () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeTextControl::updateText ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeTextControl::submitText ()
{
	if(inSubmitText || isSubmitted)
		return;

	ScopedVar<bool> guard (inSubmitText, true);

	if(IParameter* p = getTextParameter ())
	{
		String text;
		getControlText (text);

		KeyState keys;
		GUI.getKeyState (keys);
		if(keys.isSet (KeyState::kShift|KeyState::kOption) && p->getValue ().asString () == text)
		{
			// when shift or option is pressed, notify controller even if text didn't change, to allow special functionality
			p->beginEdit ();
			p->performUpdate ();
			p->endEdit ();
		}
		else
		{
			p->beginEdit ();
			p->fromString (text, true);
			p->endEdit ();
		}
	}
	updateText ();
	isSubmitted = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeTextControl::getControlText (String& string)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NativeTextControl::isCanceled ()
{
	return canceled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeTextControl::setSelection (int start, int length)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeTextControl::setScrollPosition (PointRef _where)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point NativeTextControl::getScrollPosition () const
{
	return Point ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IVisualStyle& NativeTextControl::getVisualStyle () const
{
	ThemeRenderer* renderer = owner.getRenderer ();
	if(renderer)
		if(IVisualStyle* visualStyle = renderer->getVisualStyle ())
			return *visualStyle;

	return owner.getVisualStyle ();
}

//************************************************************************************************
// EditBox::TextDragHandler
//************************************************************************************************

EditBox::TextDragHandler::TextDragHandler (EditBox& editBox)
: editBox (editBox),
  selectionRange (editBox.dragSelectionRange)
{
	editBox.currentDragHandler = this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditBox::TextDragHandler::~TextDragHandler ()
{
	ASSERT (editBox.currentDragHandler != this)
	editBox.invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditBox::TextDragHandler::dragEnter (const DragEvent& event)
{
	String text;
	if(event.session.getText (text))
	{
		if(event.session.getSource () == editBox.asUnknown ())
			event.session.setResult (IDragSession::kDropMove);
		else
			event.session.setResult (IDragSession::kDropCopyReal);
	}
	return AbstractDragHandler::dragEnter (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditBox::TextDragHandler::dragOver (const DragEvent& event)
{
	Rect rect = editBox.getTextRect ();
	if(editBox.textLayout != nullptr && rect.pointInside (event.where))
	{
		int textPosition = 0;
		PointF position (event.where.x, event.where.y);
		position.offset (editBox.displayOffset - rect.left, -rect.top);
		if(editBox.textLayout->hitTest (textPosition, position) == kResultOk)
		{
			editBox.setCaretPosition (textPosition);
			editBox.setSelectionPosition (textPosition);
		}
	}
	return AbstractDragHandler::dragOver (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditBox::TextDragHandler::afterDrop (const DragEvent& event)
{
	String text;
	if(event.session.getText (text) == false)
		return false;

	if(event.session.getSource () == editBox.asUnknown ())
	{
		if(selectionRange.start < editBox.caretTextPosition && selectionRange.start + selectionRange.length > editBox.caretTextPosition)
			return true;
	}

	if(selectionRange.start < editBox.caretTextPosition)
	{
		editBox.setCaretPosition (editBox.caretTextPosition - selectionRange.length);
		editBox.setSelectionPosition (editBox.selectionTextPosition - selectionRange.length);
	}

	int actionID = int(UndoHandler::ActionID::kDrop);
	if(event.session.getSource () == editBox.asUnknown ())
	{
		editBox.removeTextInternal (selectionRange.start, selectionRange.length, actionID);
		selectionRange.length = 0;
	}

	editBox.insertTextInternal (text, actionID);

	return AbstractDragHandler::afterDrop (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditBox::TextDragHandler::dragLeave (const DragEvent& event)
{
	editBox.currentDragHandler = nullptr;
	return AbstractDragHandler::dragLeave (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditBox::TextDragHandler::drop (const DragEvent& event)
{
	editBox.currentDragHandler = nullptr;
	return AbstractDragHandler::drop (event);
}

//************************************************************************************************
// EditBoxAccessibilityProvider
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (EditBoxAccessibilityProvider, ValueControlAccessibilityProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditBoxAccessibilityProvider::EditBoxAccessibilityProvider (EditBox& owner)
: ValueControlAccessibilityProvider (owner)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityElementRole CCL_API EditBoxAccessibilityProvider::getElementRole () const
{
	return AccessibilityElementRole::kTextField;
}
