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
#include "ccl/gui/theme/renderer/editboxrenderer.h"
#include "ccl/gui/commands.h"

#include "ccl/base/message.h"
#include "ccl/base/boxedtypes.h"
#include "ccl/base/signalsource.h"

#include "ccl/public/collections/stack.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/math/mathprimitives.h"

#include "ccl/public/guiservices.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/iparamobserver.h"
#include "ccl/public/gui/icontextmenu.h"
#include "ccl/public/gui/commanddispatch.h"
#include "ccl/public/gui/framework/abstractdraghandler.h"
#include "ccl/public/gui/framework/itextmodel.h"

#if CCL_PLATFORM_DESKTOP || CCL_PLATFORM_WASM
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
bool EditBox::isAnyEditing ()
{
	return editCount > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::isEditNavigationKey (const KeyEvent& event)
{
	switch(event.vKey)
	{
	case VKey::kEscape :
	case VKey::kEnter :
	case VKey::kReturn :
	case VKey::kUp :
	case VKey::kDown :
	case VKey::kPageUp :
	case VKey::kPageDown :
	case VKey::kTab :
	case VKey::kBackspace :
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (EditBox, TextBox)
DEFINE_CLASS_UID (EditBox, 0x49292662, 0xD30F, 0x4C11, 0xBA, 0x7B, 0xAF, 0x41, 0x77, 0xCC, 0x17, 0xAB)
const Configuration::BoolValue EditBox::useNativeTextControl ("GUI.EditBox", "UseNativeControl", EDITBOX_USE_NATIVE_CONTROL);

//////////////////////////////////////////////////////////////////////////////////////////////////

EditBox::EditBox (const Rect& size, IParameter* param, StyleRef style, StringRef title)
: TextBox (size, param, style, title),
  undoHandler (NEW UndoHandler (*this)),
  currentDragHandler (nullptr),
  dragSelectionRange (0, 0),
  preeditCaretOffset (0),
  preeditSelectionOffset (0),
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
  selectionLocked (false),
  displayOffset (0),
  selectionUpdateNeeded (true)
{
	wantsFocus (true);
	noFocusOnContextMenu (true);
	setWheelEnabled (style.isCustomStyle (Styles::kTextBoxBehaviorNoWheel) == false);
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
	bool editing = isEditing ();
	showCaret (editing || style.isCustomStyle (Styles::kEditBoxBehaviorPermanentCaret));
	if(editing && !shouldUseNativeControl ())
		startTimer (500, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::attached (View* parent)
{
	SuperClass::attached (parent);

	if(autofillType != Styles::kAutofillTypeNone)
		AutofillManager::instance ().addClient (this);
		
	if(!shouldUseNativeControl ())
		inputMethod = InputMethodManager::instance ().createInputMethod (*this);

	if(textModel)
		textModel->toDisplayString (plainTextCache);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::removed (View* parent)
{
	if(autofillType != Styles::kAutofillTypeNone)
		AutofillManager::instance ().removeClient (this);

	if(inputMethod.isValid () && isFocused ())
		inputMethod->enable (false); // removed while focused, the focus is only killed afterwards

	inputMethod.release ();

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
		renderer = getTheme ().createRenderer (ThemePainter::kEditBoxRenderer, visualStyle, this);

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
	getRenderer ()->getPartRect (size, kPartNativeControlArea);

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
	UnknownPtr<IEditControlHost> host;
	if(getParameter ())
		host = getParameter ()->getController ();
	if(!host)
		host = GetViewInterfaceUpwards<IEditControlHost> (this);

	return host;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::onFocus (const FocusEvent& event)
{
	takeEditFocusInternal (event.eventType == FocusEvent::kSetFocus, event.directed);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::startEditing ()
{
	if(shouldUseNativeControl ())
	{
		ASSERT (nativeControl == nullptr)
		if(!nativeControl && !inCreateNativeControl ())
			setNativeControl (createNativeControl ());
	}
	else
		updatePadding ();

	paramFocused (true);
	showCaret (true);

	Window* window = getWindow ();
	if(window && !window->shouldCollectUpdates ())
		window->redraw ();

	if(!shouldUseNativeControl () && currentDragHandler == nullptr)
	{
		if(!style.isCustomStyle (Styles::kTextBoxAppearanceMultiLine))
		{
			Variant start = plainTextCache.length ();
			Variant length = -plainTextCache.length ();
			(NEW Message (IParameter::kSetSelection, start, length))->post (this, -1);
			initialSelectionPending (true);
		}

		if(inputMethod.isValid ())
			inputMethod->enable (true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::takeEditFocusInternal (bool state, bool directed)
{
	if(!shouldUseNativeControl () && state == isFocused ())
		return;

	if(state)
	{
		if(!style.isCustomStyle (Styles::kTextBoxBehaviorDoubleClickEdit) || inKeyDown || wantReopen || forceOpen)
			startEditing ();

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

		if(inputMethod.isValid ())
			inputMethod->enable (false);

		setPreeditText (String::kEmpty);
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
		selectionUpdateNeeded = true;
		updateCaretRect ();
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

	if(inputMethod.isValid ())
		inputMethod->visibleRectChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::setCaretPosition (int textPosition, bool implicit)
{
	ASSERT (textPosition >= 0)

	if(selectionLocked)
		return;

	selectionUpdateNeeded = true;
	if(preeditText.isEmpty ())
		caretTextPosition = textPosition;
	else
		preeditCaretOffset = textPosition - caretTextPosition;

	if(!implicit || inKeyDown)
		updateDisplayOffset ();

	if(!implicit)
		lastExplicitCaretPosition = caretRect.getLeftTop ();

	if(inKeyDown)
	{
		Rect visibleCaretRect (caretRect);
		visibleCaretRect.offset (getTextRect ().getLeftTop () - Point (displayOffset, 0));
		makeVisible (visibleCaretRect);
	}

	invalidate ();
	resetTimer ();

	if(inputMethod.isValid ())
		inputMethod->selectionChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::setSelectionPosition (int textPosition)
{
	if(selectionLocked)
		return;

	selectionUpdateNeeded = true;
	if(preeditText.isEmpty ())
		selectionTextPosition = textPosition;
	else
		preeditSelectionOffset = textPosition - selectionTextPosition;

	invalidate ();

	if(inputMethod.isValid ())
		inputMethod->selectionChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::updateDisplayOffset ()
{
	if(shouldUseNativeControl ())
		return;

	if(const ITextLayout* textLayout = getTextLayout ())
	{
		RectF bounds;
		if(caretTextPosition >= 0 && textLayout->getCharacterBounds (bounds, caretTextPosition + preeditCaretOffset) == kResultOk)
		{
			caretRect = rectFToInt (bounds);
			caretRect.setWidth (1);
		}
		else
			caretRect.setEmpty ();

		Rect r = getTextRect ();
		if(style.isCustomStyle (Styles::kTextBoxAppearanceMultiLine))
			displayOffset = 0;
		else
		{
			Coord textWidth = r.getWidth ();
			const Coord kOffsetShift = 20;
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
				displayOffset = newDisplayOffset;
		}
	}
	else
	{
		const IVisualStyle& vs = getVisualStyle ();

		displayOffset = 0;
		caretRect.setHeight (ceil (Font::measureLineHeight (vs.getTextFont ())));
		caretRect.setWidth (1);

		Rect r = getTextRect ();
		Coord caretLeft = 0;
		switch(vs.getTextAlignment ().getAlignH ())
		{
		case Alignment::kHCenter :
			caretLeft = r.getWidth () / 2;
			break;

		case Alignment::kRight :
			caretLeft = r.getWidth ();
			break;
		}

		Coord caretTop = 0;
		if(!style.isCustomStyle (Styles::kTextBoxAppearanceMultiLine))
			caretTop = (r.getHeight () - caretRect.getHeight ()) / 2;

		caretRect.moveTo (Point (caretLeft, caretTop));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::updateCaretRect ()
{
	if(caret)
	{
		PointRef textPos = getTextRect ().getLeftTop ();
		caret->move (Rect (caretRect).offset (textPos.x - displayOffset, textPos.y));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::setCaretColor (ColorRef color)
{
	if(caret == nullptr)
		return;

	SolidDrawable* caretShape = static_cast<SolidDrawable*> (caret->getDrawable ());
	if(caretShape->getBrush ().getColor () != color)
	{
		caretShape->setBrush (SolidBrush (color));
		caret->refresh ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::updateSelection ()
{
	if(dragSelectionRange.length != 0)
		return;

	selection.setEmpty ();

	const ITextLayout* textLayout = getTextLayout ();
	if(textLayout && !shouldUseNativeControl () && isTextSelected ())
	{
		int left = ccl_min (caretTextPosition, selectionTextPosition);
		int right = ccl_max (caretTextPosition, selectionTextPosition);
		textLayout->getTextBounds (selection, {left, right - left});
	}

	selectionUpdateNeeded = false;
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
	setCaretPosition (plainTextCache.length ());
	setSelectionPosition (0);
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
	setCaretPosition (startPosition);
	setSelectionPosition (startPosition);

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

	if(style.isCustomStyle (Styles::kEditBoxBehaviorMustHitText) && !isEditing ())
	{
		Rect r;
		getRenderer ()->getPartRect (r, kPartTextExtent); 
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
	{
		if(initialSelectionPending ())
			return NEW NullMouseHandler (this);

		return NEW EditBoxMouseHandler (this);
	}

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
	
	return handler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::onGesture (const GestureEvent& event)
{
	if(event.getType () == GestureEvent::kDoubleTap && style.isCustomStyle (Styles::kTextBoxBehaviorDoubleClickEdit))
	{
		startEditing ();
		return true;
	}
	return SuperClass::onGesture (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::handleClick (const MouseEvent& event)
{
	if(!acceptClick (event))
		return false;

	if(initialSelectionPending ())
	{
		clickCount = 0;
		return false;
	}

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
			}

			startEditing ();
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
			for(RectRef rect : getSelection ())
			{
				if(rect.pointInside (event.where) && detectDrag (event))
				{
					dragSelection ();
					return true;
				}
			}
		}
	}

	if(style.isCustomStyle (Styles::kTextBoxBehaviorDoubleClickEdit) && !isEditing () && event.keys.isSet (KeyState::kLButton) && !detectDoubleClick (event))
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

	bool result = View::onMouseDown (event);

	if(event.keys.isSet (KeyState::kLButton) && (isEditing () || !style.isCustomStyle (Styles::kEditBoxBehaviorMustHitText)))
		result = isEnabled (); // An enabled editbox will usually swallow the left mouseclick. Any action happens on focus event.

	return result;
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
		for(RectRef rect : getSelection ())
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
		const ITextLayout* textLayout = getTextLayout ();
		if(textLayout && textLayout->hitTest (textPosition, position) == kResultOk)
		{
			// Snap to grapheme boundary based on mouse position
			int graphemePosition = snapToGraphemeBoundary (textPosition, position);
			if(graphemePosition != textPosition)
			{
				ITextLayout::Range lineRange (0, 0);
				if(textLayout->getLineRange (lineRange, textPosition, false) == kResultOk && lineRange.start + lineRange.length >= graphemePosition)
					textPosition = graphemePosition;
			}
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
	const ITextLayout* textLayout = getTextLayout ();
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

	bool result = Control::onContextMenu (event);

	if(isFocused () && style.isCustomStyle (Styles::kTextBoxBehaviorNoContextMenu) == false)
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

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::onKeyDown (const KeyEvent& event)
{
	if(!shouldUseNativeControl ())
		GUI.hideTooltip ();

	ScopedVar<bool> guard (inKeyDown, true);
	selectionLocked = false;

	// check for IEditControlHost interface: parent views or controller
	if(isEditNavigationKey (event))
	{
		IEditControlHost* host = getEditControlHost ();
		if(host && host->onEditNavigation (event, this))
			return true;
	}

	if(!shouldUseNativeControl ())
	{
		if(isEditing ())
		{
			if(handleKeyDown (event))
				return true;

			if(tryEdit (event))
				return true;
		}

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

int EditBox::getNextGraphemePosition (int textPosition) const
{
	AutoPtr<IGraphemeIterator> iter = System::CreateGraphemeIterator (plainTextCache, textPosition);
	return iter->next ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int EditBox::getPreviousGraphemePosition (int textPosition) const
{
	AutoPtr<IGraphemeIterator> iter = System::CreateGraphemeIterator (plainTextCache, textPosition);
	return iter->previous ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int EditBox::getNextWordPosition (int textPosition) const
{
	const ITextLayout* textLayout = getTextLayout ();
	if(textLayout == nullptr)
		return getNextGraphemePosition (textPosition);

	// skip the word, stop at the start of the next word
	int position = textPosition;
	for(int step = 0; step < 2 && position < plainTextCache.length (); step++)
	{
		ITextLayout::Range range (0, 0);
		if(textLayout->getWordRange (range, position) != kResultOk || range.start + range.length <= position)
		{
			position = getNextGraphemePosition (position); // e.g. newline
			break;
		}

		bool whitespace = Unicode::isWhitespace (plainTextCache[position]);
		position = range.start + range.length;
		if(whitespace)
			break;
	}
	return position;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int EditBox::getPreviousWordPosition (int textPosition) const
{
	const ITextLayout* textLayout = getTextLayout ();
	if(textLayout == nullptr)
		return getPreviousGraphemePosition (textPosition);

	// skip whitespace, stop at the start of the word
	int position = textPosition;
	for(int step = 0; step < 2 && position > 0; step++)
	{
		ITextLayout::Range range (0, 0);
		if(textLayout->getWordRange (range, position - 1) != kResultOk || range.length <= 0 || range.start >= position)
		{
			position = getPreviousGraphemePosition (position); // e.g. newline
			break;
		}

		position = range.start;
		if(!Unicode::isWhitespace (plainTextCache[position]))
			break;
	}
	return position;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::isWordModifier (const KeyEvent& event)
{
	#if CCL_PLATFORM_MAC || CCL_PLATFORM_IOS
	return (event.state.getModifiers () & KeyState::kOption) != 0;
	#else
	return (event.state.getModifiers () & KeyState::kCommand) != 0;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int EditBox::snapToGraphemeBoundary (int textPosition) const
{
	// Snap position to nearest grapheme cluster boundary
	if(textPosition <= 0 || textPosition >= plainTextCache.length ())
		return textPosition;

	// Find the start of the cluster containing this position
	int clusterStart = getPreviousGraphemePosition (textPosition);
	int clusterEnd = getNextGraphemePosition (clusterStart);

	// If position is already at a boundary, return as-is
	if(textPosition == clusterStart || textPosition == clusterEnd)
		return textPosition;

	// Position is inside a cluster - snap to the closest boundary
	// If distances are equal (exact middle), prefer the end (forward direction)
	int distanceToStart = textPosition - clusterStart;
	int distanceToEnd = clusterEnd - textPosition;

	return (distanceToStart < distanceToEnd) ? clusterStart : clusterEnd;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int EditBox::snapToGraphemeBoundary (int textPosition, PointF position) const
{
	const ITextLayout* textLayout = getTextLayout ();
	if(textLayout == nullptr)
		return textPosition;

	if(textPosition < 0 || textPosition >= plainTextCache.length ())
		return textPosition;

	// Get the bounds of the character at this position
	RectF charBounds;
	if(textLayout->getCharacterBounds (charBounds, textPosition) != kResultOk)
		return textPosition;  // Fallback to no snapping

	// Find the grapheme cluster starting at this position
	int clusterEnd = getNextGraphemePosition (textPosition);

	// If this is a single-character cluster, no adjustment needed
	if(clusterEnd == textPosition + 1)
		return textPosition;

	// Check if click is in the right half of the character
	float charMidpoint = charBounds.left + (charBounds.getWidth () / 2.0f);
	bool clickedRightHalf = (position.x >= charMidpoint);

	// If clicked on right half and there's a multi-character cluster, snap to end
	if(clickedRightHalf && clusterEnd > textPosition)
		return clusterEnd;

	return textPosition;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::moveCaretToCoord (CoordF layoutY, bool shiftDown)
{
	const ITextLayout* textLayout = getTextLayout ();
	if(textLayout == nullptr)
		return false;

	PointF hitPosition (lastExplicitCaretPosition.x, layoutY);

	int textPosition = 0;
	if(textLayout->hitTest (textPosition, hitPosition) != kResultOk)
		return false;

	setCaretPosition (snapToGraphemeBoundary (textPosition), true);
	if(!shiftDown)
		setSelectionPosition (caretTextPosition);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::handleKeyDown (const KeyEvent& e)
{
	bool shiftDown = (e.state.getModifiers () & KeyState::kShift);
	bool commandDown = (e.state.getModifiers () & KeyState::kCommand);

	switch(e.vKey)
	{
	case VKey::kLeft :
		if(isTextSelected () && !shiftDown && !isWordModifier (e))
		{
			setCaretPosition (ccl_min (selectionTextPosition, caretTextPosition));
			setSelectionPosition (caretTextPosition);
		}
		else if(caretTextPosition > 0)
		{
			setCaretPosition (isWordModifier (e) ? getPreviousWordPosition (caretTextPosition) : getPreviousGraphemePosition (caretTextPosition));
			if(!shiftDown)
				setSelectionPosition (caretTextPosition);
		}
		undoHandler->resetAction ();
		return true;

	case VKey::kRight :
		if(isTextSelected () && !shiftDown && !isWordModifier (e))
		{
			setCaretPosition (ccl_max (selectionTextPosition, caretTextPosition));
			setSelectionPosition (caretTextPosition);
		}
		else if(caretTextPosition < plainTextCache.length ())
		{
			setCaretPosition (isWordModifier (e) ? getNextWordPosition (caretTextPosition) : getNextGraphemePosition (caretTextPosition));
			if(!shiftDown)
				setSelectionPosition (caretTextPosition);
		}
		undoHandler->resetAction ();
		return true;

	case VKey::kUp :
		{
			const ITextLayout* textLayout = getTextLayout ();
			if(textLayout && style.isCustomStyle (Styles::kTextBoxAppearanceMultiLine))
			{
				ITextLayout::Range lineRange (0, 0);
				if(textLayout->getLineRange (lineRange, caretTextPosition) != kResultOk)
					break;

				int endOfPreviousLine = lineRange.start - 1;
				if(endOfPreviousLine < 0)
				{
					// already on the first line: move to the very beginning, but keep the remembered column
					setCaretPosition (0, true);
					if(!shiftDown)
						setSelectionPosition (caretTextPosition);
				}
				else
				{
					if(textLayout->getLineRange (lineRange, endOfPreviousLine) != kResultOk)
						break;

					RectF previousLineRect;
					if(textLayout->getCharacterBounds (previousLineRect, lineRange.start) != kResultOk)
						break;

					moveCaretToCoord (previousLineRect.getCenter ().y, shiftDown);
				}

				undoHandler->resetAction ();
				return true;
			}
		}
		break;

	case VKey::kDown :
		{
			const ITextLayout* textLayout = getTextLayout ();
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

				if(startOfNextLine >= plainTextCache.length ())
				{
					// already on the last line: move to the very end, but keep the remembered column
					setCaretPosition (plainTextCache.length (), true);
					if(!shiftDown)
						setSelectionPosition (caretTextPosition);
				}
				else
					moveCaretToCoord (nextLineRect.getCenter ().y, shiftDown);

				undoHandler->resetAction ();
				return true;
			}
		}
		break;

	case VKey::kHome :
		if(const ITextLayout* textLayout = getTextLayout ())
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
		if(const ITextLayout* textLayout = getTextLayout ())
		{
			if(commandDown)
				setCaretPosition (plainTextCache.length ());
			else
			{
				ITextLayout::Range lineRange (0, 0);
				if(textLayout->getLineRange (lineRange, caretTextPosition, false) != kResultOk)
					break;

				setCaretPosition (lineRange.start + lineRange.length);
			}
			if(!shiftDown)
				setSelectionPosition (caretTextPosition);
		}
		undoHandler->resetAction ();
		return true;

	case VKey::kPageUp :
	case VKey::kPageDown :
		{
			const ITextLayout* textLayout = getTextLayout ();
			if(textLayout && style.isCustomStyle (Styles::kTextBoxAppearanceMultiLine))
			{
				bool pageDown = (e.vKey == VKey::kPageDown);

				Rect visible;
				RectF caretBounds;
				if(!getVisibleClient (visible) || textLayout->getCharacterBounds (caretBounds, caretTextPosition) != kResultOk)
					break;

				// the line at the far edge of the viewport becomes the line at the opposite edge
				Rect textRect = getTextRect ();
				Coord edge = pageDown ? visible.bottom - 1 : visible.top;
				PointF edgePosition (0.f, CoordF(edge - textRect.top));

				int anchorPosition = 0;
				RectF anchorBounds;
				if(textLayout->hitTest (anchorPosition, edgePosition) != kResultOk)
					break;
				if(textLayout->getCharacterBounds (anchorBounds, anchorPosition) != kResultOk)
					break;

				Coord anchorTop = textRect.top + coordFToInt (anchorBounds.top);
				Coord anchorBottom = textRect.top + coordFToInt (anchorBounds.bottom);

				if((pageDown && anchorBottom > visible.bottom) || (!pageDown && anchorTop < visible.top))
				{
					ITextLayout::Range lineRange (0, 0);
					if(textLayout->getLineRange (lineRange, anchorPosition) != kResultOk)
						break;

					int fullyVisiblePosition = pageDown ? lineRange.start - 1 : lineRange.start + lineRange.length;
					RectF fullyVisibleBounds;
					if(fullyVisiblePosition >= 0 && fullyVisiblePosition <= plainTextCache.length () 
						&& textLayout->getCharacterBounds (fullyVisibleBounds, fullyVisiblePosition) == kResultOk)
					{
						anchorTop = textRect.top + coordFToInt (fullyVisibleBounds.top);
						anchorBottom = textRect.top + coordFToInt (fullyVisibleBounds.bottom);
					}
				}

				RectF textBounds;
				textLayout->getBounds (textBounds);

				Coord pageTop = 0;
				Coord pageBottom = 0;
				if(pageDown)
				{
					pageTop = anchorTop;
					pageBottom = ccl_max (pageTop + visible.getHeight (), textRect.top + coordFToInt (textBounds.bottom));
				}
				else
				{
					pageBottom = anchorBottom;
					pageTop = pageBottom - visible.getHeight ();
				}

				Rect pageRect (visible.left, pageTop, visible.right, pageBottom);
				Coord scrollDelta = pageRect.top - visible.top;
				makeVisible (pageRect);

				// the caret travels with the view, so it keeps its position on screen
				CoordF targetY = caretBounds.getCenter ().y + scrollDelta;

				if(targetY < textBounds.top || targetY > textBounds.bottom)
				{
					// past either end of the text: go there, but keep the remembered column
					setCaretPosition (targetY < textBounds.top ? 0 : plainTextCache.length (), true);
					if(!shiftDown)
						setSelectionPosition (caretTextPosition);
				}
				else
					moveCaretToCoord (targetY, shiftDown);

				undoHandler->resetAction ();
				return true;
			}
		}
		break;

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

			if(isMultiline == false || canceled)
			{
				if(!canceled)
					submit (false, e.state);

				killFocus ();
				return canceled || !style.isCustomStyle (Styles::kEditBoxBehaviorDialogEdit); // dialog edit: don't swallow return/enter, let the dialog react
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

			// Delete entire grapheme cluster or word before caret
			int prevPos = isWordModifier (event) ? getPreviousWordPosition (caretTextPosition) : getPreviousGraphemePosition (caretTextPosition);
			int numChars = caretTextPosition - prevPos;
			if(numChars > 0)
			{
				int textPosition = caretTextPosition;
				int caretOffset = removeTextInternal (textPosition, -numChars, actionID);
				setCaretPosition (textPosition - caretOffset);
				setSelectionPosition (caretTextPosition);
			}

			return true;
		}

	case VKey::kDelete :
		if(isTextSelected ())
		{
			deleteSelection ();
			return true;
		}

		// Delete entire grapheme cluster or word after caret
		undoHandler->setDeferSaveCaretPosition ();
		int nextPos = isWordModifier (event) ? getNextWordPosition (caretTextPosition) : getNextGraphemePosition (caretTextPosition);
		int numChars = nextPos - caretTextPosition;
		removeTextInternal (caretTextPosition, numChars, int(UndoHandler::ActionID::kDeleteLeading));
		return true;
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
	textModified (true);
	int textPosition = caretTextPosition;
	int insertedCharacters = textModel->insertText (textPosition, text, undoHandler->isUndoActive () ? 0 : IMutableTextModel::kMergeUndo);
	setCaretPosition (textPosition + insertedCharacters);
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

	IMutableTextModel::EditOptions options = 0;
	if(!undoHandler->isUndoActive ())
		options |= IMutableTextModel::kMergeUndo;

	textModified (true);
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
			}
			else
			{
				editCount--;
				stopTimer ();
				initialSelectionPending (false);
			}

			// rebuild text layout to show a collapsed string when not editing
			UnknownPtr<ISubject> subject (textModel);
			if(subject)
				subject->signal (Message (kChanged));

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

void EditBox::submit (bool check, const KeyState& keys)
{
	bool modified = textModified ();
	textModified (false);

	IParameter* param = getTextParameter ();
	if(param == nullptr)
		return;

	String modelString;
	textModel->toParamString (modelString);
	String paramString;
	param->toString (paramString);

	if(modelString == paramString && keys.isSet (KeyState::kShift|KeyState::kOption))
	{
		// when shift or option is pressed, notify controller even if text didn't change, to allow special functionality
		param->beginEdit ();
		param->performUpdate ();
		param->endEdit ();
		return;
	}

	if(!modified || (check && modelString == paramString))
		return;

	// set new parameter value
	param->beginEdit ();
	param->fromString (modelString, true);
	param->endEdit ();

	// update text from parameter value (may have changed due to parameter validation)
	param->toString (paramString);
	textModel->fromParamString (paramString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::paramChanged ()
{
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
	{
		int textLength = plainTextCache.length ();
		caretTextPosition = ccl_min (caretTextPosition, textLength);
		selectionTextPosition = ccl_min (selectionTextPosition, textLength);

		SuperClass::paramChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API EditBox::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IParameter::kSetSelection)
	{
		initialSelectionPending (false);

		int start = msg[0];
		int length = msg[1];
		if(nativeControl)
			nativeControl->setSelection (start, length);

		if(!shouldUseNativeControl ())
		{
			setCaretPosition (start);
			setSelectionPosition (start + length);
		}
	}

	if(msg == kChanged && isEqualUnknown (subject, getTextParameter ()))
		textModified (false);

	bool isImmediate = style.isCustomStyle (Styles::kEditBoxBehaviorImmediate);
	bool textModelChanged = msg == kChanged && textModel && isEqualUnknown (subject, textModel);
	if(textModelChanged)
	{
		String oldText = plainTextCache;
		textModel->toDisplayString (plainTextCache);
		if(oldText != plainTextCache)
		{
			selectionUpdateNeeded = true;
			if(inputMethod.isValid ())
				inputMethod->textChanged ();
		}
	}

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

const ConstVector<Rect>& EditBox::getSelection ()
{
	if(selectionUpdateNeeded)
		updateSelection ();

	return selection.getRects ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API EditBox::setSelection (int caret, int selection)
{
	setCaretPosition (caret);
	setSelectionPosition (selection);
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

	if(msg.category == "Edit" && !msg.checkOnly ())
	{
		if(msg.name == "Undo")
		{
			undoHandler->undo ();
			return true;
		}
		else if(msg.name == "Redo")
		{
			undoHandler->redo ();
			return true;
		}
		else if(msg.name == "Paste")
		{
			paste ();
			return true;
		}
		else if(msg.name == "Select All")
		{
			selectAll ();
			return true;
		}
		if(msg.name == "Delete")
		{
			if(isTextSelected ())
				deleteSelection ();
			return true;
		}
		else if(msg.name == "Cut")
		{
			if(isTextSelected ())
				cutSelection ();
			return true;
		}
		else if(msg.name == "Copy")
		{
			if(isTextSelected ())
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

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::modifyDisplayString (String& displayString) const
{
	if(caretTextPosition >= 0 && !preeditText.isEmpty ())
		displayString.insert (caretTextPosition, preeditText);

	SuperClass::modifyDisplayString (displayString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::modifyLayout (ITextLayout& textLayout) const
{
	if(caretTextPosition >= 0 && !preeditText.isEmpty ())
		textLayout.setFontStyle ({caretTextPosition, preeditText.length ()}, Font::Styles::kUnderline, true);

	SuperClass::modifyLayout (textLayout);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IView* CCL_API EditBox::getInputView () const
{
	return this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API EditBox::getTextLength () const 
{
	return plainTextCache.length ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API EditBox::getText (String& result, int start, int length) const
{
	if(start < 0 || start > plainTextCache.length ())
		return kResultInvalidArgument;
	if(length > plainTextCache.length () - start)
		return kResultInvalidArgument;

	if(length < 0)
		result = plainTextCache;
	else 
		result = plainTextCache.subString (start, length);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API EditBox::getSelection (int& caret, int& selection, SelectionDomain selectionDomain) const
{
	switch(selectionDomain)
	{
	default :
	case kSelectionOriginal :
		caret = ccl_min (caretTextPosition, plainTextCache.length ());
		selection = ccl_min (selectionTextPosition, plainTextCache.length ());
		break;

	case kSelectionPreedit :
		caret = preeditCaretOffset;
		selection = preeditSelectionOffset;
		break;

	case kSelectionMerged :
		caret = ccl_min (caretTextPosition, plainTextCache.length ()) + preeditCaretOffset;
		selection = ccl_min (selectionTextPosition, plainTextCache.length ()) + preeditSelectionOffset;
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API EditBox::getCaretRect (Rect& caretRect) const
{
	if(!caret.isValid ())
		return kResultFailed;
	caretRect = caret->getSize ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API EditBox::getInputType () const
{
	return getEffectiveKeyboardType ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API EditBox::getInputFlags () const
{
	int flags = 0;
	if(style.isCustomStyle (Styles::kTextBoxBehaviorPasswordEdit))
		set_flag<int> (flags, ITextInputClient::kSensitiveData);
	if(style.isCustomStyle (Styles::kTextBoxAppearanceMultiLine))
		set_flag<int> (flags, ITextInputClient::kMultiline);
	return flags;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API EditBox::getPreeditText (String& text) const
{
	text = preeditText;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API EditBox::setPreeditText (StringRef text)
{
	preeditText = text;
	preeditCaretOffset = preeditText.length ();
	preeditSelectionOffset = preeditText.length ();

	UnknownPtr<ISubject> subject (textModel);
	if(subject)
		subject->signal (Message (kChanged));
	invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API EditBox::applyText (StringRef text)
{
	insertTextInternal (text, int(UndoHandler::ActionID::kType));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API EditBox::deleteText (int beforeLength, int afterLength)
{
	removeTextInternal (caretTextPosition - beforeLength, beforeLength + afterLength, int(UndoHandler::ActionID::kType));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API EditBox::replaceText (StringRef text, int start, int length)
{
	removeTextInternal (start, length, int(UndoHandler::ActionID::kCut));
	setCaret (start);
	insertTextInternal (text, int(UndoHandler::ActionID::kPaste));
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
	if(undoStack.isEmpty ())
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
	editBox.textModified (true);
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

	editBox.textModified (true);
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

	if(EditBox::isEditNavigationKey (e))
	{
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

		default :
			if(owner.onKeyDown (e))
				return true;

			if(owner.getStyle ().isCustomStyle (Styles::kTextBoxAppearanceMultiLine)) // no focus navigation here...
				break;

			if(FocusNavigator::instance ().onKeyDown (e))
				return true;
			break;
		}
	}
	else
	{
		if(FocusNavigator::instance ().onKeyDown (e))
			return true;
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

void NativeTextControl::getControlText (String& string) const
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

int NativeTextControl::getControlTextLength () const
{
	String text;
	getControlText (text);
	return text.length ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeTextControl::getSelectionPositions (int& caret, int& anchor, int start, int length) const
{
	int textLength = getControlTextLength ();
	caret = ccl_bound (start, 0, textLength);
	anchor = ccl_bound (start + length, 0, textLength);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeTextControl::selectAll ()
{
	int textLength = getControlTextLength ();
	setSelection (textLength, -textLength);
}

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
	const ITextLayout* textLayout = editBox.getTextLayout ();
	if(textLayout != nullptr && rect.pointInside (event.where))
	{
		int textPosition = 0;
		PointF position (event.where.x, event.where.y);
		position.offset (editBox.displayOffset - rect.left, -rect.top);
		if(textLayout->hitTest (textPosition, position) == kResultOk)
		{
			// Snap to grapheme boundary based on drag position
			textPosition = editBox.snapToGraphemeBoundary (textPosition, position);
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

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API EditBox::getTextBounds (IMutableRegion& bounds, int start, int length)
{
	const ITextLayout* textLayout = getTextLayout ();
	if(textLayout)
		textLayout->getTextBounds (bounds, {start, length});
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
