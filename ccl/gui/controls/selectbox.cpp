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
// Filename    : ccl/gui/controls/selectbox.cpp
// Description : Select Box
//
//************************************************************************************************

#include "ccl/gui/controls/selectbox.h"
#include "ccl/gui/controls/button.h"
#include "ccl/gui/controls/controlaccessibility.h"

#include "ccl/gui/touch/touchhandler.h"
#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/theme/renderer/selectboxrenderer.h"
#include "ccl/gui/theme/renderer/comboboxrenderer.h"

#include "ccl/base/message.h"

#include "ccl/public/gui/framework/imenu.h"
#include "ccl/public/gui/framework/itextmodel.h"

namespace CCL {
	
//************************************************************************************************
// SelectBoxAccessibilityProvider
//************************************************************************************************

class SelectBoxAccessibilityProvider: public ValueControlAccessibilityProvider,
									  public IAccessibilityExpandCollapseProvider
{
public:
	DECLARE_CLASS_ABSTRACT (SelectBoxAccessibilityProvider, ValueControlAccessibilityProvider)

	SelectBoxAccessibilityProvider (SelectBox& owner);

	// ValueControlAccessibilityProvider
	AccessibilityElementRole CCL_API getElementRole () const override;

	// IAccessibilityExpandCollapseProvider
	tresult CCL_API expand (tbool state) override;
	tbool CCL_API isExpanded () override;

	CLASS_INTERFACE (IAccessibilityExpandCollapseProvider, ValueControlAccessibilityProvider)

protected:
	SelectBox& getSelectBox () const;
};

//************************************************************************************************
// SelectBoxDragOpenMouseHandler
//************************************************************************************************

class SelectBoxDragOpenMouseHandler: public MouseHandler
{
public:
	SelectBoxDragOpenMouseHandler (SelectBox* selectBox)
	: MouseHandler (selectBox)
	{
	}

	void onBegin () override
	{
		view->setMouseState (View::kMouseDown);
	}
	
	bool onMove (int moveFlags) override
	{
		constexpr Coord kMinDistance = 3;

		if(!ccl_equals (current.where.x, first.where.x, kMinDistance) || !ccl_equals (current.where.y, first.where.y, kMinDistance))
		{
			(NEW Message ("showMenu"))->post (view);
			return false;
		}
		return true;
	}
};

//************************************************************************************************
// SelectBox::PopupTouchHandler
//************************************************************************************************

class SelectBox::PopupTouchHandler: public RemotePopupTouchHandler
{
public:
	PopupTouchHandler (SelectBox* selectBox = nullptr)
	: RemotePopupTouchHandler (selectBox),
	  selectBox (selectBox)
	{
		openPopupImmediately (!selectBox->getStyle ().isCustomStyle (Styles::kSelectBoxBehaviorDragOpen));
		
		setMinMoveDistance (3);
	}

	// RemotePopupTouchHandler
	void openPopup () override
	{
		selectBox->showMenu ();
	}

	PopupSelector* getPopupSelector () const override
	{
		return selectBox->popupSelector;
	}

	tbool CCL_API onGesture (const GestureEvent& event) override
	{
		if(event.getType () == GestureEvent::kSingleTap)
		{
			if(!popupOpened () && !selectBox->popupSelector)
				selectBox->showMenu ();
			return true;
		}
		return SuperClass::onGesture (event);
	}

private:
	SelectBox* selectBox;
	using SuperClass = RemotePopupTouchHandler;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// SelectBox
//************************************************************************************************

BEGIN_STYLEDEF (SelectBox::customStyles)
	{"hidetext",	Styles::kSelectBoxAppearanceHideText},
	{"hidefocus",	Styles::kSelectBoxAppearanceHideFocus},
	{"hidebutton",	Styles::kSelectBoxAppearanceHideButton},
	{"ignorekeys",	Styles::kSelectBoxBehaviorIgnoreKeys},
	{"inversewheel",	Styles::kSelectBoxBehaviorInverseWheel},
	{"stayopenonclick", Styles::kSelectBoxBehaviorStayOpenOnClick},
	{"leadingbutton", 	Styles::kSelectBoxAppearanceLeadingButton},
	{"trailingbutton",	Styles::kSelectBoxAppearanceTrailingButton},
	{"offstate",	Styles::kSelectBoxAppearanceOffState},
	{"nowheel",     Styles::kSelectBoxBehaviorNoWheel},
	{"nomodifier",	Styles::kSelectBoxBehaviorNoModifier},
	{"hideimage",	Styles::kSelectBoxAppearanceHideImage},
	{"showtitle",	Styles::kSelectBoxAppearanceTitleAsText},
	{"dragopen", 	Styles::kSelectBoxBehaviorDragOpen},
	{"closemenu", 	Styles::kSelectBoxBehaviorCloseMenu},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (SelectBox, TextBox)
DEFINE_CLASS_UID (SelectBox, 0xD44924D4, 0xE4B4, 0x436A, 0xB4, 0xE4, 0x8A, 0x44, 0xF1, 0xB2, 0x1F, 0x15)

//////////////////////////////////////////////////////////////////////////////////////////////////

SelectBox::SelectBox (const Rect& size, IParameter* param, StyleRef style, StringRef title)
: TextBox (size, param, style, title),
  popupSelector (nullptr),
  popupOptions (style.isVertical () ? PopupSizeInfo::kRight|PopupSizeInfo::kTop : PopupSizeInfo::kLeft|PopupSizeInfo::kBottom)
{
	wantsFocus (true);
	setWheelEnabled (style.isCustomStyle (Styles::kSelectBoxBehaviorNoWheel) == false);
}
//////////////////////////////////////////////////////////////////////////////////////////////////

SelectBox::~SelectBox ()
{
	cancelSignals ();
	if(popupSelector)
		popupSelector->removeObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelector* SelectBox::getPopupSelector ()
{
	if(!popupSelector)
	{
		popupSelector = NEW PopupSelector;
		popupSelector->addObserver (this);
	}
	return popupSelector;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SelectBox::setPopupVisualStyle (VisualStyle* visualStyle)
{
	getPopupSelector ()->setVisualStyle (visualStyle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectBox::isOpen ()
{
	return getPopupSelector ()->isOpen ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* SelectBox::getRenderer ()
{
	if(renderer == nullptr)
	{
		renderer = getTheme ().createRenderer (ThemePainter::kSelectBoxRenderer, visualStyle);
		// keep this order - initPopupSelector () might call getRenderer () 
		initPopupSelector ();
	}
	return renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SelectBox::initPopupSelector ()
{
	getPopupSelector ()->setTheme (getTheme ());

	// set decorform name, when no explicit popupStyle/visualStyle is set.
	if(popupSelector->getVisualStyle () == nullptr)
		popupSelector->setDecorNameFromStyle (getVisualStyle ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SelectBox::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IParameter::kUpdateMenu)
	{
		if(popupSelector && popupSelector->isOpen ())
		{
			// forward message to popup selector client
			IPopupSelectorWindow* window = popupSelector->getCurrentWindow ();
			UnknownPtr<IObserver> client (window ?  window->getClient () : nullptr);
			if(client)
				client->notify (subject, msg);
		}
	}
	else if(msg == IParameter::kRequestFocus)
	{
		if(isAttached ())
			showMenu ();
	}
	else if(msg == IParameter::kReleaseFocus)
	{
		if(popupSelector && popupSelector->getCurrentWindow ())
			popupSelector->getCurrentWindow ()->closePopup ();
	}
	else if(msg == PopupSelector::kPopupClosed)
	{
		setMouseState (kMouseNone);
	}
	else if(msg == "showMenu")
	{
		showMenu ();
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SelectBox::showMenu ()
{
	if(param && !getPopupSelector ()->isOpen ())
	{
		#if CCL_PLATFORM_WINDOWS
		// defer opening our popup when another popup with the same parent window is about to close (e.g. closed via CCLMouseHook, but WM_CLOSE/WM_DESTROY not delivered yet)
		PopupSelectorWindow* popup = ccl_cast<PopupSelectorWindow> (Desktop.getTopWindow (kPopupLayer));
		if(popup && popup->isCloseRequested () && popup->getParentWindow () == getWindow ())
		{
			(NEW Message (IParameter::kRequestFocus))->post (this, -1);
			return;
		}
		#endif

		CString menuType = MenuPresentation::kTree;
		if(visualStyle)
		{
			if(visualStyle->getMetric<bool> ("native", false))
				menuType = MenuPresentation::kNative;
			else if(visualStyle->getMetric<bool> ("extended", false))
				menuType = MenuPresentation::kExtended;
			else if(visualStyle->getMetric<bool> ("compact", false))
				menuType = MenuPresentation::kCompact;
			else if(visualStyle->getMetric<bool> ("singlecolumn", false))
				menuType = MenuPresentation::kSingleColumn;
		}
		
		Point offset;
		VisualStyle* popupStyle = popupSelector->getVisualStyle ();
		if(popupStyle)
			offset (popupStyle->getMetric<Coord> ("popup.offset.x", 0), popupStyle->getMetric<Coord> ("popup.offset.y", 0));

		PopupSizeInfo sizeInfo (this, popupOptions, offset);
		sizeInfo.canFlipParentEdge (true);
		sizeInfo.sizeLimits.minWidth = getWidth ();

		// check for size limits in popup style
		if(popupStyle)
		{
			Coord minHeight = popupStyle->getMetric<Coord> ("minHeight", -1);
			if(minHeight > 0)
				sizeInfo.sizeLimits.minHeight = minHeight;

			sizeInfo.sizeLimits.minWidth = ccl_max (sizeInfo.sizeLimits.minWidth, popupStyle->getMetric<Coord> ("minWidth", -1));

			Coord maxHeight = popupStyle->getMetric<Coord> ("maxHeight", -1);
			if(maxHeight > 0)
			{
				sizeInfo.sizeLimits.maxHeight = maxHeight;
				sizeInfo.sizeLimits.minHeight = ccl_min (sizeInfo.sizeLimits.minHeight, maxHeight);
			}
			
			if(popupStyle->getMetric<bool> ("popup.palette.left", false))
			{
				sizeInfo.flags &= ~PopupSizeInfo::kRight;
				sizeInfo.flags |= PopupSizeInfo::kLeft;
			}
			else if(popupStyle->getMetric<bool> ("popup.palette.right", false))
			{
				sizeInfo.flags &= ~PopupSizeInfo::kLeft;
				sizeInfo.flags |= PopupSizeInfo::kRight;
			}
		}

		int32 behavior = IPopupSelector::kAcceptsAfterSwipe|IPopupSelector::kHideHScroll;

		if(style.isCustomStyle (Styles::kSelectBoxBehaviorStayOpenOnClick))
			behavior = IPopupSelector::kStayOpenOnClick;

		popupSelector->setBehavior (behavior);
	
		if(style.isCustomStyle (Styles::kSelectBoxBehaviorCloseMenu))
			popupSelector->closeAfterDrag (true);
		
		popupSelector->popup (param, sizeInfo, menuType);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SelectBox::onSize (const Point& delta)
{
	SuperClass::onSize (delta);
	invalidate (); // TextBox only considers the text, but background or button might require complete redraw
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectBox::onMouseWheel (const MouseWheelEvent& event)
{
	if(View::onMouseWheel (event))
		return true;

	if(isWheelEnabled ())
	{
		// we usually invert direction for scrolling through menu param; can be changed via style flag
		bool inverse = !getStyle ().isCustomStyle (Styles::kSelectBoxBehaviorInverseWheel);
		return tryWheelParam (event, inverse);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectBox::onMouseDown (const MouseEvent& event)
{
	if(style.isCustomStyle (Styles::kSelectBoxBehaviorNoModifier))
		if(event.keys & (KeyState::kCommand|KeyState::kOption|KeyState::kShift))
			return false;

	if(style.isCustomStyle (Styles::kSelectBoxBehaviorDragOpen))
		return tryMouseHandler (event);

	if(event.keys.isSet (KeyState::kLButton))
	{
		setMouseState (IView::kMouseDown);
		showMenu ();
		return true;
	}
	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* SelectBox::createMouseHandler (const MouseEvent& event)
{
	if(style.isCustomStyle (Styles::kSelectBoxBehaviorDragOpen))
		return NEW SelectBoxDragOpenMouseHandler (this);
	
	return SuperClass::createMouseHandler (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectBox::onKeyDown (const KeyEvent& event)
{
	SharedPtr<View> keeper (this);
	switch(event.state.getModifiers ())
	{
		case 0:
			if(style.isCustomStyle (Styles::kSelectBoxBehaviorIgnoreKeys))
				break;

			switch(event.vKey)
			{
				case VKey::kLeft:
				case VKey::kUp:
					if(param)
					{
						param->beginEdit ();
						param->decrement ();
						param->endEdit ();
					}
					return true;

				case VKey::kRight:
				case VKey::kDown:
					if(param)
					{
						param->beginEdit ();
						param->increment ();
						param->endEdit ();
					}
					return true;
			}
			break;

		case KeyState::kOption:
			switch(event.vKey)
			{
				case VKey::kUp:
				case VKey::kDown:
					showMenu ();
					return true;
			}
			break;
	}
	return Control::onKeyDown (event);
}


//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectBox::onMouseEnter (const MouseEvent& event)
{
	setMouseState (kMouseOver);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectBox::onMouseLeave (const MouseEvent& event)
{
	if(!popupSelector || !popupSelector->isOpen ())
		setMouseState (kMouseNone);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectBox::onGesture (const GestureEvent& event)
{
	if(style.isCustomStyle (Styles::kSelectBoxBehaviorDragOpen))
	{
		if(event.getType () == GestureEvent::kSwipe)
		{
			showMenu ();
			return true;
		}
	}
	else if(event.getType () == GestureEvent::kSingleTap)
	{
		showMenu ();
		return true;
	}
	return SuperClass::onGesture (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef SelectBox::getText ()
{
	if(style.isCustomStyle (Styles::kSelectBoxAppearanceTitleAsText))
		return getTitle ();

	return SuperClass::getText ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* SelectBox::createTouchHandler (const TouchEvent& event)
{
	#if 1 // always allow swiping to open the menu (as for mouse input)
	return NEW PopupTouchHandler (this);
	#else
	if(style.isCustomStyle (Styles::kSelectBoxDragOpen))
		return NEW PopupTouchHandler (this);

	return NEW GestureHandler (this, GestureEvent::kSingleTap);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord SelectBox::getDisplayWidth () const
{
	if(renderer)
	{
		Rect contentRect;
		renderer->getPartRect (this, kPartDisplayArea, contentRect);
		return contentRect.getWidth ();
	}
	else
		return TextBox::getWidth ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord SelectBox::getDisplayHeight () const
{	
	if(renderer)
	{
		Rect contentRect;
		renderer->getPartRect (this, kPartDisplayArea, contentRect);
		return contentRect.getHeight ();
	}
	else
		return TextBox::getHeight ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SelectBox::calcAutoSize (Rect& r)
{
	SuperClass::calcAutoSize (r);

	if(!isHFitAndFitText () && !style.isCustomStyle (Styles::kSelectBoxAppearanceHideButton))
		r.right += getDropDownButtonWidth ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord SelectBox::getHFitWidth () const
{	
	if(style.isCustomStyle (Styles::kSelectBoxAppearanceHideButton))
		return SuperClass::getHFitWidth ();
	else
		return SuperClass::getHFitWidth () + getDropDownButtonWidth ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord SelectBox::getDropDownButtonWidth () const
{
	// compensate for contract (1) in getDisplayWidth
	Coord width = 2;
	
	Rect buttonRect;
	ccl_const_cast (this)->getRenderer ()->getPartRect (this, SelectBox::kPartDropDownButton, buttonRect);
	width += buttonRect.getWidth ();
	
	return width;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* SelectBox::getAccessibilityProvider ()
{
	if(!accessibilityProvider)
		accessibilityProvider = NEW SelectBoxAccessibilityProvider (*this);
	return accessibilityProvider;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (SelectBox)
	DEFINE_METHOD_NAME ("showMenu")
END_METHOD_NAMES (SelectBox)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SelectBox::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "showMenu")
	{
		bool deferred = msg.getArgCount () > 0 && msg[0].asBool ();
		if(deferred)
			(NEW Message (IParameter::kRequestFocus))->post (this);
		else
			showMenu ();
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// ComboBox
//************************************************************************************************

BEGIN_STYLEDEF (ComboBox::customStyles)
	{"password",	Styles::kTextBoxBehaviorPasswordEdit},
	{"doubleclick",	Styles::kTextBoxBehaviorDoubleClickEdit},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (ComboBox, SelectBox)
DEFINE_CLASS_UID (ComboBox, 0x8382E45D, 0xF6EA, 0x499C, 0xB8, 0x84, 0xB7, 0x03, 0x95, 0xA9, 0x90, 0xB2);

//////////////////////////////////////////////////////////////////////////////////////////////////

ComboBox::ComboBox (const Rect& size, 
					IParameter* selectParam, 
					IParameter* _editParam,
					StyleRef style, 
					StringRef title)
: SelectBox (size, selectParam, style, title),
  editParam (nullptr),
  textControl (nullptr),
  returnKeyType (Styles::kReturnKeyDefault),
  keyboardType (Styles::kKeyboardTypeAutomatic)
{
	if(_editParam)
		setEditParam (_editParam);
	if(style.isCustomStyle (Styles::kSelectBoxAppearanceHideFocus))
		wantsFocus (false);

	// our derived getTextParameter () could not be called while in constructor of base class TextBox (see TextBox::createTextModel / TextBox::setTextModel)
	// workaround: initialize the edit string again, now with the correct parameter (editParam)
	if(textModel)
	{
		String paramString;
		getTextParameter ()->toString (paramString);
		textModel->fromParamString (paramString);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ComboBox::~ComboBox ()
{
	setEditParam (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ComboBox::getEditParam () const
{
	return editParam;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ComboBox::setEditParam (IParameter* _editParam)
{
	if(editParam != _editParam)
	{
		if(editParam)
		{
			ISubject::removeObserver (editParam, this);
			editParam->release ();
		}

		editParam = _editParam;

		if(editParam)
		{
			if(param && !param->isOutOfRange ()) // don't copy a list value to edit param if the list param was marked as "out of range"
				paramChanged ();

			ISubject::addObserver (editParam, this);
			editParam->retain ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ComboBox::getTextParameter () const
{
	// base class TextBox builds the text based on our (list) param; we want to prefer the editParam, which can have a value that is not in the list
	if(editParam)
		return editParam;

	return SuperClass::getTextParameter ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* ComboBox::getRenderer ()
{
	if(renderer == nullptr)
	{
		renderer = getTheme ().createRenderer (ThemePainter::kComboBoxRenderer, visualStyle);
		// keep this order - initPopupSelector () might call getRenderer ()
		initPopupSelector ();
	}
	return renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ComboBox::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IParameter::kRequestFocus && isEqualUnknown (subject, editParam))
		return; // prevent focusing the selectBox parameter if editParam should be focused

	SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ComboBox::onSize (const Point& delta)
{
	killFocus ();
	SuperClass::onSize (delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ComboBox::onMove (const Point& delta)
{
	killFocus ();
	View::onMove (delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ComboBox::canEdit () const
{
	return editParam && editParam->isEnabled ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ComboBox::attached (View* parent)
{
	SuperClass::attached (parent);
	if(editParam && textControl == nullptr)
	{
		Rect r;
		getRenderer ()->getPartRect (this, kPartDisplayArea, r);

		StyleFlags editBoxStyle (getStyle ());
		editBoxStyle.setCommonStyle (Styles::kTransparent);
		editBoxStyle.setCustomStyle (Styles::kEditBoxAppearanceHideText);
		editBoxStyle.setCustomStyle (Styles::kTextBoxAppearanceMultiLine, false);

		textControl = NEW EditBox (r, editParam, editBoxStyle);
		textControl->setSizeMode (IView::kAttachAll);
		textControl->setKeyboardType (keyboardType);
		addView (textControl);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ComboBox::removed (View* parent)
{
	if(textControl)
	{
		removeView (textControl);
		safe_release (textControl);
	}
	SuperClass::removed (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ComboBox::syncEditTextWithList ()
{
	bool different = false;
	if(editParam && param)
	{
		String listString;
		param->toString (listString);

		String editString;
		editParam->toString (editString);

		if(listString != editString)
		{
			different = true;
			editParam->fromString (listString);
		}
	}
	return different;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ComboBox::onFocus (const FocusEvent& event)
{
	Control::onFocus (event); // update focus + invalidate

	if(event.eventType == FocusEvent::kKillFocus)
	{
		if(textControl)
			textControl->onFocus (event);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ComboBox::onMouseDown (const MouseEvent& event)
{
	ThemeRenderer* renderer = getRenderer ();
	if(renderer->hitTest (this, event.where, nullptr) == kPartContentArea && canEdit ())
	{
		if(textControl && (!style.isCustomStyle (Styles::kTextBoxBehaviorDoubleClickEdit) || detectDoubleClick (event)))
		{
			textControl->takeFocus ();
			return textControl->onMouseDown (event);
		}
	}

	return SelectBox::onMouseDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ComboBox::onKeyDown (const KeyEvent& event)
{
	if(event.state.getModifiers () == 0 && !style.isCustomStyle (Styles::kSelectBoxBehaviorIgnoreKeys))
	{
		switch(event.vKey)
		{
			case VKey::kUp:
			case VKey::kDown:
			{
				if(param && editParam)
				{
					String editString;
					editParam->toString (editString);

					String listString;
					param->toString (listString);
					if(listString.isEmpty ())
						return false;

					if(editString == listString)
					{
						// editString matches the selected list item: move up / down in list
						SelectBox::onKeyDown (event);
					}
					else
					{
						// select the first list item that starts with editString
						int min = param->getMin ();
						int max = param->getMax ();
						int current = param->getValue ();
						for(int v = min; v <= max; v++)
						{
							param->getString (listString, v);
							if(listString.startsWith (editString, false))
							{
								param->setValue (v);
								if(v == current)
									paramChanged (); // force update of textControl

								return true;
							}
						}

						// no match found: select first list item
						param->setValue (min);
						if(min == current)
							paramChanged (); // force update of textControl
					}
				}

				return true;
			}
		}
	}

	return SelectBox::onKeyDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ComboBox::paramChanged ()
{
	// list value selected: copy string to edit param
	if(editParam && param && !param->getName ().isEmpty ()) // control initializes param with dummy if none is provided
	{
		UnknownPtr<IListParameter> listParam (param);
		if(!listParam.isValid () || !listParam->isEmpty ()) // consider an empty list as out of bounds
		{
			String listString;
			param->toString (listString);
			editParam->fromString (listString);
		}
	}

	SelectBox::paramChanged ();
}

//************************************************************************************************
// SelectBoxAccessibilityProvider
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SelectBoxAccessibilityProvider, ValueControlAccessibilityProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

SelectBoxAccessibilityProvider::SelectBoxAccessibilityProvider (SelectBox& owner)
: ValueControlAccessibilityProvider (owner)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityElementRole CCL_API SelectBoxAccessibilityProvider::getElementRole () const
{
	return AccessibilityElementRole::kComboBox;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SelectBox& SelectBoxAccessibilityProvider::getSelectBox () const
{
	return static_cast<SelectBox&> (view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SelectBoxAccessibilityProvider::expand (tbool state)
{
	if(state && !isExpanded ())
	{
		getSelectBox ().showMenu ();
		return kResultOk;
	}
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SelectBoxAccessibilityProvider::isExpanded ()
{
	SelectBox& selectBox = getSelectBox ();
	return selectBox.isOpen ();
}
