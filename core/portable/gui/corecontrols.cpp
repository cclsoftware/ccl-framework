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
// Filename    : core/portable/gui/corecontrols.cpp
// Description : Control classes
//
//************************************************************************************************

#include "corecontrols.h"

#include "corelistview.h"
#include "corekeyboard.h"
#include "coreviewbuilder.h"
#include "corefont.h"

#include "core/portable/coreprofiling.h"

namespace Core {
namespace Portable {

//************************************************************************************************
// MenuView
//************************************************************************************************

class MenuView: public ListView,
				public ListViewModel
{
public:
	MenuView (RectRef size, ListParam* listParam);
	~MenuView ();

	// ListView
	bool onTouchInput (const TouchEvent& e) override;
	
	// ListViewModel
	void selectListItem (int index, bool state, ListView* view, int trigger) override;
	bool isSelectionHandler () const override;
protected:
	ListParam* listParam;
};

//************************************************************************************************
// TextEditView
//************************************************************************************************

class TextEditView: public ContainerView,
					public ViewController,
					public IKeyboardInputReceiver,
					public IParamObserver
{
public:
	BEGIN_CORE_CLASS ('TxEV', TextEditView)
		ADD_CORE_CLASS_ (ViewController)
		ADD_CORE_CLASS_ (IParamObserver)
	END_CORE_CLASS (ContainerView)

	TextEditView (RectRef size, Parameter* textParam, CStringPtr prompt = nullptr, TextInput::ICompletionCallback* callback = nullptr);
	~TextEditView ();

	void setKeyboardMode (Skin::KeyboardLayout::Mode mode);
	void setKeyboardCapitalizationMode (Skin::KeyboardCapitalization::Mode mode);
	
	// ViewController
	View* createView (CStringPtr type) override;
	void* getObjectForView (CStringPtr name, CStringPtr type) override;
	
	// ContainerView
	void addView (View* view) override;
	bool onWheelInput (const WheelEvent& e) override;
	
	// IKeyboardInputReceiver
	Text& getText () override;
	void textChanged () override;
	void textInputDone (bool canceled) override;
	int getCursorIndex () const override;
	void setCursorIndex (int index) override;
	
	// IParamObserver
	void paramChanged (Parameter* p, int msg) override;

protected:
	Parameter* textParam;
	StringParam previewParam;
	StringParam promptParam;
	NumericParam clearParam;
	NumericParam cursorParam;
	bool inputBoxInitialized;

	TextInput::ICompletionCallback* completionCallback;

	static const ParamInfo previewParamInfo;
	static const ParamInfo promptParamInfo;
	static const ParamInfo clearParamInfo;
	static const ParamInfo cursorParamInfo;
	
	TouchKeyboard* getTouchKeyboard () const;
	TextInputBox* getTextInputBox () const;
};
	
//************************************************************************************************
// ViewTypeFilter
/** Filter to find view by type. */
//************************************************************************************************
template <class T>
struct ViewTypeFilter: ViewFilter
{
	ViewTypeFilter () {}
	bool matches (const View* view) const override { return core_cast<T> (view); }
};

} // namespace Portable
} // namespace Core

using namespace Core;
using namespace Portable;

//************************************************************************************************
// Label
//************************************************************************************************

Label::Label (RectRef size, CStringPtr title)
: View (size),
  title (title)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Label::setAttributes (const Attributes& a)
{
	View::setAttributes (a);
	
	options |= ViewAttributes::getOptions (a, Skin::Enumerations::labelOptions);

	setTitle (a.getString (ViewAttributes::kTitle));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Label::draw (const DrawEvent& e)
{
	const Style& style = getStyle ();

	if(isColorize ())
		e.graphics.fillRect (e.updateRect, style.getBackColor ());

	if(!title.isEmpty ())
	{
		Rect r;
		getClientRect (r);	
		e.graphics.drawString (r, title, style.getTextColor (), style.getFontName (), style.getTextAlign ());
	}
}

//************************************************************************************************
// MultiLineLabel
//************************************************************************************************

MultiLineLabel::MultiLineLabel (RectRef size, CStringPtr title)
: View (size),
  title (title)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiLineLabel::setAttributes (const Attributes& a)
{
	View::setAttributes (a);
	
	setTitle (a.getString (ViewAttributes::kTitle));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiLineLabel::draw (const DrawEvent& e)
{
	Rect r;
	getClientRect (r);
	
	const Style& style = getStyle ();
	e.graphics.drawMultiLineString (r, title, style.getTextColor (), style.getFontName (), style.getTextAlign ());
}

//************************************************************************************************
// ImageView
//************************************************************************************************

ImageView::ImageView (RectRef size)
: ContainerView (size),
  parameter (nullptr),
  imageAlpha (1.f)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ImageView::~ImageView ()
{
	if(parameter)
		parameter->removeObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageView::setAttributes (const Attributes& a)
{
	ContainerView::setAttributes (a);

	setImage (ViewAttributes::getBitmap (a));
	options |= ViewAttributes::getOptions (a, Skin::Enumerations::imageViewOptions);
	
	if(size.isEmpty ())
	{
		Rect newSize = size;
		if(ViewAttributes::autoSizeToBitmap (newSize, image.getBitmap ()))
			setSize (newSize);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageView::draw (const DrawEvent& e)
{
	if(Bitmap* bitmap = image.getBitmap ())
	{
		BitmapMode mode;
		if(imageAlpha < 1.f)
		{
			mode.paintMode = BitmapMode::kBlend;
			mode.alphaF = imageAlpha;
		}

		if(parameter)
		{
			int frame = parameter->getIntValue ();
			BitmapPainter::draw (e.graphics, Point (), *bitmap, frame, &mode);
		}
		else	
			e.graphics.drawBitmap (Point (e.updateRect.left, e.updateRect.top), *bitmap, e.updateRect, &mode);
	}
	else
	{
		if(isColorize ())
			e.graphics.fillRect (e.updateRect, getStyle ().getBackColor ());
	}

	ContainerView::draw (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr ImageView::getConnectionType () const
{
	return kParamType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageView::connect (void* object)
{
	Parameter* p = reinterpret_cast<Parameter*> (object);
	ASSERT (p != nullptr)
	parameter = p;
	if(parameter)
		parameter->addObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageView::paramChanged (Parameter* p, int msg)
{
	if(msg == Parameter::kChanged)
	{
		invalidate ();
	}
	else if(msg == Parameter::kDestroyed)
	{
		p->removeObserver (this);
		parameter = nullptr;
	}
}

//************************************************************************************************
// VariantView
//************************************************************************************************

VariantView::VariantView (RectRef size)
: ContainerView (size),
  parameter (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

VariantView::~VariantView ()
{
	if(parameter)
		parameter->removeObserver (this);

	VectorForEachFast (variants, View*, v)
		if(!isChildView (v, false))
			delete v;
	EndFor
	variants.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VariantView::addView (View* view)
{
	variants.add (view);

	// init current variant
	int index = parameter ? (int)parameter->getValue () : -1;
	if(children.isEmpty () && variants.count ()-1 == index)
		selectVariant (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VariantView::resizeToChildren ()
{
	Rect childSize;
	VectorForEachFast (variants, View*, v)
		childSize.join (v->getSize ());
	EndFor
	size.setWidth (childSize.right);
	size.setHeight (childSize.bottom);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr VariantView::getConnectionType () const
{
	return kParamType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VariantView::connect (void* object)
{
	Parameter* p = reinterpret_cast<Parameter*> (object);
	ASSERT (p != nullptr)
	parameter = p;
	parameter->addObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VariantView::paramChanged (Parameter* p, int msg)
{
	if(msg == Parameter::kChanged)
	{
		int index = (int)p->getValue ();
		selectVariant (index);
	}
	else if(msg == Parameter::kDestroyed)
	{
		p->removeObserver (this);
		parameter = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VariantView::selectVariant (int index)
{
	View* oldView = children.first ();
	bool hadFocus = false;
	RootView* root = getRootView ();
	bool anyFocus = root ? (root->getFocusView () != nullptr) : false;
	if(oldView)
	{
		View* focus = root ? root->getFocusView () : nullptr;
		if(focus && oldView->asContainer () && oldView->asContainer ()->isChildView (focus))
			hadFocus = true;
		
		if(IVariantChildView* variantChild = core_cast<IVariantChildView> (oldView))
			variantChild->onVariantAttached (false);

		removeView (oldView);
	}

	View* newView = variants.at (index);
	if(newView)
	{
		if(IVariantChildView* variantChild = core_cast<IVariantChildView> (newView))
			variantChild->onVariantAttached (true);

		ContainerView::addView (newView); // call superclass!
		
		// the view we removed had focus, so we need to replace that foucs with something else now.
		if((hadFocus || !anyFocus) && root)
			root->findFirstFocusView ();
	}
}

//************************************************************************************************
// AlignView
//************************************************************************************************

AlignView::AlignView (RectRef size)
: ContainerView (size),
  alignment (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlignView::setAttributes (const Attributes& a)
{
	ContainerView::setAttributes (a);

	setAlignment (ViewAttributes::getAlign (a, ViewAttributes::kTextAlign, Alignment::kCenter));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlignView::layoutAll ()
{
	VectorForEachFast (children, View*, view)
		layoutChild (view);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlignView::layoutChild (View* view)
{
	Rect clientRect;
	getClientRect (clientRect);

	Rect rect (view->getSize ());
	rect.align (clientRect, alignment);
	view->setSize (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlignView::addView (View* view)
{
	if(!size.isEmpty ())
		layoutChild (view);

	ContainerView::addView (view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlignView::setSize (RectRef newSize)
{
	ContainerView::setSize (newSize);
	layoutAll ();
}

//************************************************************************************************
// Control
//************************************************************************************************

const double Control::kEditResetDelay = 1.0; // one second

//////////////////////////////////////////////////////////////////////////////////////////////////

Control::Control (RectRef size, Parameter* p)
: View (size),
  parameter (nullptr),
  wheelAccumulation (0),
  endEditTime (0.0)
{
	if(p != nullptr)
		setParameter (p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Control::~Control ()
{
	setParameter (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Control::setParameter (Parameter* p)
{
	if(parameter != p)
	{
		if(parameter)
			parameter->removeObserver (this);
		parameter = p;
		if(parameter)
			parameter->addObserver (this);

		if(isAlwaysDisabled () == false) // if they haven't explicitly disabled, enable it based on the parameter:
			enable (parameter && parameter->isEnabled ());
		invalidate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parameter* Control::getParameter () const
{
	return parameter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Control::paramChanged (Parameter* p, int msg)
{
	ASSERT (msg != Parameter::kDestroyed)	
	if(msg == Parameter::kChanged)
	{
		if(p == parameter)
		{
			if(isAlwaysDisabled () == false) // if they haven't explicitly disabled the control, enable it based on the parameter:
				enable (parameter->isEnabled ());
			invalidate ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr Control::getConnectionType () const
{
	return kParamType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Control::connect (void* object)
{
	setParameter (reinterpret_cast<Parameter*> (object));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Control::onWheelInput (const WheelEvent& e)
{
    if(!isEnabled ())
		return true;
    
	if(parameter && parameter->isEnabled ())
	{
		int delta = e.delta;
		if(parameter->getPrecision() < 100)
		{
			static const int kWheelAccumulationSteps = 40;
			wheelAccumulation += delta;
			if(abs (wheelAccumulation) < kWheelAccumulationSteps)
				return true;
			
			delta = wheelAccumulation / kWheelAccumulationSteps;
			wheelAccumulation = 0;
		}
		
		// Don't just repeatedly call beginEdit() and endEdit(). Give it a short (1s?) delay...
		// This is needed for things like touch automation where it depends on a begin and end edit.
		if(!parameter->isEditing ())
			parameter->beginEdit ();
		endEditTime = SystemClock::getSeconds () + kEditResetDelay;
		
		if(e.delta > 0)
			parameter->increment (delta);
		else if(e.delta < 0)
			parameter->decrement (-delta);
		
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Control::onIdle ()
{
	View::onIdle ();
	
	if(endEditTime > 0.0 && parameter && parameter->isEditing ())
	{
		double now = SystemClock::getSeconds ();
		if(now >= endEditTime)
		{
			parameter->endEdit ();
			endEditTime = 0.0;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Control::drawFocusFrame (const DrawEvent& e)
{
	Rect r;
	getClientRect (r);
	ThemePainter::instance ().drawFocusFrame (e.graphics, r);
}

//************************************************************************************************
// Button
//************************************************************************************************

Button::Button (RectRef size, Parameter* p)
: Control (size, p),
  down (false)
{
	wantsTouch (true);
	wantsFocus (false);
	isTransparent (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Button::setAttributes (const Attributes& a)
{
	Control::setAttributes (a);

	int options = ViewAttributes::getOptions (a, Skin::Enumerations::buttonOptions);
	if(options & Skin::kButtonBehaviorWantsFocus)
		wantsFocus (true);
	if(options & Skin::kButtonBehaviorDeferred)
		isDeferred (true);
	if(options & Skin::kButtonBehaviorSilentTracking)
		isSilentTracking (true);
	if(options & Skin::kButtonAppearanceTransparent)
		isTransparent (true);
		
	setImage (ViewAttributes::getBitmap (a));	
	setIcon (ViewAttributes::getBitmap (a, ViewAttributes::kIcon));
	setTitle (a.getString (ViewAttributes::kTitle));

	if(size.isEmpty ())
	{
		Rect newSize = size;
		if(ViewAttributes::autoSizeToBitmap (newSize, image.getBitmap ()))
			setSize (newSize);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Button::draw (const DrawEvent& e)
{
	Rect r;
	getClientRect (r);
	const Style& style = getStyle ();

	bool disabled = !isEnabled ();
	bool on = isOn ();
	if(Bitmap* bitmap = image.getBitmap ())
	{
		int frame = on ? 1 : 0;
		int frameCount = bitmap->getFrameCount ();
		if(disabled && frameCount > 3)
			frame = 3;
		else if(down && frameCount > 2)
			frame = 2;
		BitmapPainter::draw (e.graphics, Point (), *bitmap, frame);
	}
	else if(!isTransparent ())
	{
		Color color = disabled ? style.getBackColorDisabled () : style.getBackColor ();
		if(on)
			color = disabled ? style.getForeColorDisabled () : style.getForeColor ();
		e.graphics.fillRect (e.updateRect, color);
	}

	if(icon.isValid ())
		BitmapPainter::drawCentered (e.graphics, r, *icon.getBitmap ());

	if(title.isEmpty () == false)
	{
		Color color = disabled ? style.getTextColorDisabled () : (on ? style.getTextColorOn () : style.getTextColor ());
		e.graphics.drawString (r, title, color, style.getFontName (), style.getTextAlign ());
	}

	if(isFocused ())
		drawFocusFrame (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Button::getHandledGestures (GestureVector& gestures, PointRef where)
{
	gestures.add (kGestureSingleTap|kGesturePriorityNormal);
	gestures.add (kGestureLongPress|kGesturePriorityNormal);
	gestures.add (kGestureSwipe|kGesturePriorityNormal);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Button::onGestureInput (const GestureEvent& e)
{
	if(parameter == nullptr)
		return false;

	switch(e.getState ())
	{
	case kGesturePossible:
		{
			if(!isDeferred ())
				push ();

			// button can be removed upon push ()
			bool isAttached = getRootView () != nullptr;
			down = isAttached;
			invalidate ();
		}
		break;

	case kGestureBegin:
		if(e.getType () == kGestureSingleTap)
		{
			parameter->beginEdit ();

			// kGestureSingleTap has no kGestureEnd, so ignore the deferred option here
			if(!down || isDeferred ())
				push ();

			down = false;
			parameter->endEdit ();
			invalidate ();
		}
		else
		{
			parameter->beginEdit ();

			if(!isDeferred () && !down)
				push ();

			// button can be removed upon push ()
			bool isAttached = getRootView () != nullptr;
			down = isAttached;
		
			invalidate ();
		}
		break;

	case kGestureEnd:
		if(isDeferred ())
		{
			if(down)
				push ();
		}

		down = false;
		parameter->endEdit ();
		invalidate ();
		break;
	
	case kGestureChanged:
		if(isDeferred ())
		{
			Rect r;
			bool inside = getClientRect (r).pointInside (e.where);
			if(inside != down)
			{
				down = inside;
				invalidate ();
			}
		}
		break;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Button::onTouchInput (const TouchEvent& e)
{
	if(parameter == nullptr)
		return false;
	
	if(!isEnabled ())
		return false;

	switch(e.type)
	{
	case TouchEvent::kDown:
		{
			parameter->beginEdit();

			if(!isDeferred ())
				push ();

			// button can be removed upon push ()
			bool isAttached = getRootView () != nullptr;
			down = isAttached;
		
			// if it's not deferred (ie., we want an action immediately) don't
			// bother invalidating -- this is an optimization that saves a draw
			// operation and seemingly touch latency.
			if(isDeferred ())
				invalidate ();
		}
		break;
	
	case TouchEvent::kUp:
		if(isDeferred () && down)
			push ();

		down = false;
		parameter->endEdit();
		invalidate ();
		break;

	case TouchEvent::kMove:
		if(isDeferred ())
		{
			Rect r;
			bool inside = getClientRect (r).pointInside (e.where);
			if(inside != down)
			{
				down = inside;
				invalidate ();
			}
		}
		break;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Button::isOn () const
{
	if(parameter && parameter->getValue () > 0)
		return true;
	return down;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Button::push ()
{
	if(!isEnabled () || isSilentTracking ())
		return;

	#if DEBUG_LOG
	DebugPrintf ("   Button::push\n");
	#endif

	down = false;
	if(parameter)
	{
		parameter->setValue (1, true);
		parameter->setValue (0, false); // reset
	}
}

//************************************************************************************************
// Toggle
//************************************************************************************************

Toggle::Toggle (RectRef size, Parameter* p)
: Button (size, p)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Toggle::isOn () const
{
	return parameter && parameter->getValue () != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Toggle::push ()
{
	if(!isEnabled ())
		return;

	#if DEBUG_LOG
	DebugPrintf ("   Toggle::push: %d\n", parameter->getValue () ? 0 : 1);
	#endif

	if(parameter)
	{
		parameter->beginEdit ();
		if(parameter->getValue () != 0)
			parameter->setValue (0, true);
		else
			parameter->setValue (1, true);
		parameter->endEdit ();
	}
}

//************************************************************************************************
// RadioButton
//************************************************************************************************

RadioButton::RadioButton (RectRef size, Parameter* p, int v)
: Button (size, p),
  value (v)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RadioButton::setAttributes (const Attributes& a)
{
	Button::setAttributes (a);
	
	setValue ((int)a.getInt (ViewAttributes::kRadioValue));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RadioButton::isOn () const
{
	return (parameter && ((int) parameter->getValue ()) == value) || down;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RadioButton::push ()
{
	if(!isEnabled ())
		return;

	#if DEBUG_LOG
	DebugPrintf ("   RadioButton::push: %d\n", value);
	#endif

	down = false;
	if(parameter)
	{
		parameter->beginEdit ();
		parameter->setValue ((ParamValue)value, true);
		parameter->endEdit ();
	}
}

//************************************************************************************************
// ValueBar
//************************************************************************************************

ValueBar::ValueBar (RectRef size, Parameter* p)
: Control (size, p)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ValueBar::setAttributes (const Attributes& a)
{
	Control::setAttributes (a);

	options |= ViewAttributes::getOptions (a, Skin::Enumerations::valueBarOptions);

	setBackground (ViewAttributes::getBitmap (a, ViewAttributes::kBackground));	
	setImage (ViewAttributes::getBitmap (a));	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ValueBar::drawBackground (const DrawEvent& e)
{
	ThemePainter::instance ().drawBackground (e.graphics, e.updateRect, getStyle (), background.getBitmap ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ValueBar::draw (const DrawEvent& e)
{
	drawBackground (e);
	
	if(parameter == nullptr)
		return;

	Rect r;
	getClientRect (r);

	int drawOptions = options;
	if(parameter->isBipolar ())
		drawOptions |= Skin::kValueBarAppearanceCentered;
	
	float value = parameter->getNormalized ();	
	ThemePainter::instance ().drawValueBar (e.graphics, r, drawOptions, value, getStyle (), image.getBitmap ());

	if(isFocused ())
		drawFocusFrame (e);
}

//************************************************************************************************
// Slider
//************************************************************************************************

Slider::Slider (RectRef size, Parameter* p)
: ValueBar (size, p)
{
	wantsFocus (true);
	wantsTouch (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Slider::onTouchInput (const TouchEvent& e)
{
	switch(e.type)
	{
	case TouchEvent::kDown :
		if(parameter)
			parameter->beginEdit ();
		break;

	case TouchEvent::kUp :
		if(parameter)
			parameter->endEdit ();
		break;

	case TouchEvent::kMove :
		if(parameter)
		{
			float newValue = 0.f;
			if(isVertical ())
				newValue = 1.f - (float)e.where.y / (float)size.getHeight ();
			else
				newValue = (float)e.where.x / (float)size.getWidth ();
			
			parameter->setNormalized (newValue, true);
		}
		break;
	}
	return true;
}

//************************************************************************************************
// TextBox
//************************************************************************************************

TextBox::TextBox (RectRef size, Parameter* p)
: Control (size, p),
  trimMode (Skin::kTextTrimNone)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::draw (const DrawEvent& e)
{
	Rect r;
	getClientRect (r);
	drawText (e.graphics, r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::setAttributes (const Attributes& a)
{
	Control::setAttributes (a);
	
	if(const Attributes* styleAttr = ViewAttributes::getStyleAttributes (a))
	{
		trimMode = (Skin::TextTrimMode)ViewAttributes::getExlusiveOption (*styleAttr, 
																		  Skin::Enumerations::textTrimModes, 
																		  ViewAttributes::kTextTrimMode, 
																		  Skin::kTextTrimNone);
		hideText (styleAttr->getInt ("hidetext") > 0);
	}	

	options |= ViewAttributes::getOptions (a, Skin::Enumerations::textBoxOptions);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::getText (TextValue& text) const
{
	if(parameter)
		parameter->toString (text.getBuffer (), text.getSize ());
	else
		text.empty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::collapseString (TextValue& string, const Graphics& graphics, CStringPtr fontName, int maxWidth, Skin::TextTrimMode trimMode)
{
	// Todo: Do we want this elsewhere? If so move to Graphics...
	if(trimMode == Skin::kTextTrimNone)
		return;
	
	if(graphics.getStringWidth (string, fontName) <= maxWidth)
		return;
	
	// first remove all spaces...
	TextValue drawString;
	for(int i = 0; i < string.length (); ++i)
		if(!CStringClassifier::isWhitespace (string[i]))
			drawString.append (string[i]);
	
	if(graphics.getStringWidth (drawString, fontName) <= maxWidth)
		return;
	
	static CStringPtr dots = "..";
	int originalLength = drawString.length ();
	int length = originalLength;
	TextValue temp;
	switch(trimMode)
	{
	case Skin::kTextTrimMiddle:
		{
			length -= 2;
			while(length > 2)
			{
				int halfLength = length / 2;
				drawString.subString (temp, 0, halfLength);
				temp.append (dots);
				TextValue half;
				drawString.subString (half, originalLength - halfLength, halfLength);
				temp.append (half);
				if(graphics.getStringWidth (temp, fontName) <= maxWidth)
					break;
				length--;
			}
			drawString = temp;
		} 
		break;
			
	case Skin::kTextTrimRight:
		{
			length -= 2;
			while(length > 2)
			{
				drawString.subString (temp, 0, length);
				temp.append (dots);
				if(graphics.getStringWidth (temp, fontName) <= maxWidth)
					break;
				length--;
			}
			drawString = temp;
		} 
		break;
		
	default:
		break;
	}
	
	string = drawString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::drawText (Graphics& graphics, RectRef textRect)
{
	if(hideText ())
		return;
	
	TextValue string;
	getText (string);

	if(!string.isEmpty ())
	{
		const Style& style = getStyle ();
		Color color = isEnabled () ? style.getTextColor () : style.getTextColorDisabled ();
		if(isMultiLine ())
			graphics.drawMultiLineString (textRect, string, color, style.getFontName (), style.getTextAlign ());
		else
		{
			collapseString (string, graphics, style.getFontName (), textRect.getWidth (), trimMode);
			graphics.drawString (textRect, string, color, style.getFontName (), style.getTextAlign ());
		}
	}
}

//************************************************************************************************
// EditBox
//************************************************************************************************

EditBox::EditBox (RectRef size, Parameter* p)
: TextBox (size, p),
  keyLayout (Skin::KeyboardLayout::kLetters),
  capitalizationMode (Skin::KeyboardCapitalization::kFirst)
{
	wantsFocus (false);
	wantsTouch (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::setAttributes (const Attributes& a)
{
	TextBox::setAttributes (a);
	
	keyLayout = (Skin::KeyboardLayout::Mode)ViewAttributes::getExlusiveOption (a, Skin::Enumerations::keyboardLayouts, 
																			   ViewAttributes::kKeyboardLayout, 
																			   Skin::KeyboardLayout::kLetters);
	
	capitalizationMode = (Skin::KeyboardCapitalization::Mode)ViewAttributes::getExlusiveOption (a, Skin::Enumerations::keyboardCapitalizationModes,
																			   ViewAttributes::kKeyboardCapitalization,
																			   Skin::KeyboardCapitalization::kFirst);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBox::draw (const DrawEvent& e)
{
	Rect r;
	getClientRect (r);
	e.graphics.drawRect (r, getStyle ().getForeColor ());

	r.left += 2;
	drawText (e.graphics, r);

	if(isFocused ())
		drawFocusFrame (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBox::onTouchInput (const TouchEvent& e)
{
	if(e.type == TouchEvent::kDown)
		if(RootView* rootView = getRootView ())
		{
			Rect r;
			rootView->getClientRect (r);
			TextEditView* editView = NEW TextEditView (r, parameter);
			editView->setKeyboardCapitalizationMode (capitalizationMode);
			editView->setKeyboardMode (keyLayout);
			rootView->setModalView (editView);
		}
	return true;
}

//************************************************************************************************
// SelectBox
//************************************************************************************************

SelectBox::SelectBox (RectRef size, Parameter* p)
: TextBox (size, p),
  menuStyle (nullptr)
{
	wantsFocus (false);
	wantsTouch (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SelectBox::setAttributes (const Attributes& a)
{
	TextBox::setAttributes (a);
	
	setImage (ViewAttributes::getBitmap (a));
	
	if(size.isEmpty ())
	{
		Rect newSize = size;
		if(ViewAttributes::autoSizeToBitmap (newSize, image.getBitmap ()))
			setSize (newSize);
	}
	
	if(const Attributes* styleAttr = ViewAttributes::getStyleAttributes (a))
		menuStyle = styleAttr->getAttributes ("menustyle");

	menuRect = ViewAttributes::getSize (a, "menurect");
	textRect = ViewAttributes::getSize (a, "textrect");
	
	DpiSetting::instance ().scaleRect (menuRect);
	DpiSetting::instance ().scaleRect (textRect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SelectBox::draw (const DrawEvent& e)
{
	Rect r;
	getClientRect (r);
	bool focused = isFocused ();
	bool drewFocus = false;
	if(Bitmap* bitmap = image.getBitmap ())
	{
		int frame = 0;
		if(isFocused () && bitmap->getFrameCount () > 1)
		{
			frame = 1;
			drewFocus = true;
		}
		BitmapPainter::draw (e.graphics, Point (), *bitmap, frame);
	}
	else
	{
		e.graphics.drawRect (r, getStyle ().getForeColor ());
		// TODO: drop-down arrow!
	}

	Rect _textRect (textRect);
	if(_textRect.isEmpty ())
	{
		_textRect = r;
		_textRect.left += 2;
	}
	drawText (e.graphics, _textRect);

	if(focused && !drewFocus)
		drawFocusFrame (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectBox::onTouchInput (const TouchEvent& e)
{
	if(e.type == TouchEvent::kDown)
	{
		ListParam* listParam = ListParam::cast (parameter);
		if(listParam == nullptr)
			if(AliasParam* alias = AliasParam::cast (parameter))
				listParam = ListParam::cast (alias->getOriginal ());

		if(listParam)
		{
			if(RootView* rootView = getRootView ())
			{
				Rect menuRect (this->menuRect);				
				if(menuRect.isEmpty ())
				{
					menuRect (0, 0, size.getWidth (), 100);
					DpiSetting::instance ().scaleCoord (menuRect.bottom);
					Point p (0, size.getHeight ());
					clientToRoot (p);
					menuRect.offset (p);
				
					if(menuRect.bottom > rootView->getSize ().bottom)
						menuRect.offset (0, -menuRect.getHeight () - size.getHeight ());
				}
				else
				{
					// menuRect is in global coords, which doesn't work well in a flexible layout
					// if we find an AlignView as ancestor, let menuRect refer to the next ancestor inside it
					View* reference = this;
					while((reference = reference->getParent ()))
					{
						if(core_cast<AlignView> (reference->getParent ()))
						{
							Point offset;
							reference->clientToRoot (offset);
							menuRect.offset (offset);
							break;
						}
					}
				}
				
				MenuView* menuView = NEW MenuView (menuRect, listParam);
				if(menuStyle)
				{
					menuView->setAttributes (*menuStyle);
					menuView->setSize (menuRect); // the menuStyle attributes will have overwritten this...
				}
				rootView->setModalView (menuView);
				menuView->makeSelectedItemVisible ();
			}
		}
	}
	return true;
}

//************************************************************************************************
// MenuView
//************************************************************************************************

MenuView::MenuView (RectRef size, ListParam* listParam)
: ListView (size),
  listParam (listParam)
{
	setModel (this);

	for(int i = 0, max = (int)listParam->getMax (); i <= max; i++)
	{
		ListViewItem* item = NEW ListViewItem;
		CString128& title = const_cast<CString128&> (item->getTitle ());
		listParam->getStringForValue (title.getBuffer (), title.getSize (), i);
		IParamMenuCustomizer* customizer = core_cast<IParamMenuCustomizer>(listParam->getController ());
		bool itemEnabled = customizer ? customizer->isParamMenuItemEnabled (listParam, i) : true;
		item->setEnabled (itemEnabled);
		addItem (item);
	}

	selectItem ((int)listParam->getValue (), kTriggerInternal);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuView::~MenuView ()
{
	setModel (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuView::onTouchInput (const TouchEvent& e)
{
	if(e.type == TouchEvent::kDown)
	{
		if(RootView* rootView = getRootView ())
		{
			ASSERT (rootView->getModalView () == this)
			rootView->setModalView (nullptr); // <--- 'this' is deleted here!
		}
	}
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuView::selectListItem (int index, bool state, ListView* view, int trigger)
{
	if(!canSelectItem (index))
		return;
	
	ListViewModel::selectListItem (index, state, view, trigger);
	
	if(state)
	{
		if(listParam && (index >= 0 && index < getItemCount ()))
			listParam->setValue ((ParamValue)index, true);
		
		if(RootView* rootView = getRootView ())
		{
			ASSERT (rootView->getModalView () == this)
			rootView->setModalView (nullptr); // <--- 'this' is deleted here!
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuView::isSelectionHandler () const 
{
	return true;
}

//************************************************************************************************
// TextEditView
//************************************************************************************************

const ParamInfo TextEditView::previewParamInfo = PARAM_STRING (1, "preview", 0);
const ParamInfo TextEditView::promptParamInfo = PARAM_STRING (1, "pompt", PARAM_READONLY);
const ParamInfo TextEditView::clearParamInfo = PARAM_TOGGLE (1, "clear", 0, nullptr, 0);
const ParamInfo TextEditView::cursorParamInfo = PARAM_INT (1, "cursor", 0, 255, 255, nullptr, nullptr, 0, 0);

//////////////////////////////////////////////////////////////////////////////////////////////////

TextEditView::TextEditView (RectRef size, Parameter* textParam, CStringPtr prompt, TextInput::ICompletionCallback* callback)
: ContainerView (size),
  textParam (textParam),
  previewParam (previewParamInfo),
  promptParam (promptParamInfo),
  clearParam (clearParamInfo),
  cursorParam (cursorParamInfo),
  inputBoxInitialized (false),
  completionCallback (callback)
{
	// init text
	textParam->toString (getText ().getBuffer (), getText ().getSize ());

	// init prompt
	if(prompt)
		promptParam.fromString (prompt);
	
	clearParam.addObserver (this);
	cursorParam.addObserver (this);

	// add touch keyboard, etc.
	ViewBuilder::instance ().buildView (*this, "Standard.TextEditor", this);
	
	// moving cursor to the end of the string
	cursorParam.setIntValue (previewParam.getText ().length ());
	
	wantsFocus (true);
	wantsTouch (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextEditView::~TextEditView ()
{
	clearParam.removeObserver (this);
	cursorParam.removeObserver (this);
	
	if(completionCallback)
		completionCallback->textInputFinished (true);
	
	if(TextInputBox* box = getTextInputBox ())
		box->setReceiver (nullptr);
	
	removeAll (); // remove before StringParam dtor!
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TouchKeyboard* TextEditView::getTouchKeyboard () const
{
	return core_cast<TouchKeyboard> (findView (ViewTypeFilter<TouchKeyboard> ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextInputBox* TextEditView::getTextInputBox () const
{
	return core_cast<TextInputBox> (findView (ViewTypeFilter<TextInputBox> ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextEditView::setKeyboardMode (Skin::KeyboardLayout::Mode mode)
{
	if(TouchKeyboard* keyboard = getTouchKeyboard ())
		keyboard->selectMode (mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextEditView::setKeyboardCapitalizationMode (Skin::KeyboardCapitalization::Mode mode)
{
	if(TouchKeyboard* keyboard = getTouchKeyboard ())
		keyboard->setCapitalizationMode (mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* TextEditView::createView (CStringPtr type)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* TextEditView::getObjectForView (CStringPtr _name, CStringPtr type)
{
	ConstString name (_name);
	if(ConstString (type) == kParamType)
	{
		if(name == "preview")
			return &previewParam;

		if(name == "prompt")
			return &promptParam;
		
		if(name =="clear")
			return &clearParam;
	}
	else if(ConstString (type) == kKeyboardInputType)
		return static_cast<IKeyboardInputReceiver*> (this);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextEditView::addView (View* view)
{
	ContainerView::addView (view);

	if(!inputBoxInitialized)
	{
		if(TextInputBox* box = getTextInputBox ())
		{
			box->setReceiver (this);
			inputBoxInitialized = true;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextEditView::onWheelInput (const WheelEvent &e)
{
	ContainerView::onWheelInput (e);
	
	// we never want to scroll by much, so only inc/dec by 1
	if(e.delta > 0)
		cursorParam.increment ();
	else
		cursorParam.decrement ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IKeyboardInputReceiver::Text& TextEditView::getText ()
{
	return const_cast<Text&> (previewParam.getText ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextEditView::setCursorIndex (int index)
{
	index = bound (index, 0, getText ().length ());
	cursorParam.setIntValue (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TextEditView::getCursorIndex () const
{	
	return cursorParam.getIntValue ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextEditView::paramChanged (Parameter* p, int msg)
{
	if(msg == Parameter::kChanged)
	{
		if(p == &clearParam && p->getValue () > 0.f)
		{
			previewParam.fromString (""); // clear it...
			
			// notifying the keyboard that the text has been cleared
			if(TouchKeyboard* keyboard = getTouchKeyboard ())
				keyboard->clear ();
		}
		else if(p == &cursorParam)
		{
			if(TextInputBox* box = getTextInputBox ())
				box->updateCursor (true); // resets the blink state of the cursor
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextEditView::textChanged ()
{
	previewParam.changed ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextEditView::textInputDone (bool canceled)
{
	if(canceled == false)
		textParam->fromString (previewParam.getText (), true);

	if(completionCallback)
	{
		completionCallback->textInputFinished (canceled);
		completionCallback = nullptr;
	}

	if(RootView* rootView = getRootView ())
	{
		ASSERT (rootView->getModalView () == this)
		rootView->setModalView (nullptr); // <--- 'this' is deleted here!
	}
}

//************************************************************************************************
// TextInput
//************************************************************************************************

void TextInput::start (RootView* rootView, Parameter* textParam, CStringPtr prompt, TextInput::ICompletionCallback* callback, 
					   Skin::KeyboardLayout::Mode mode, Skin::KeyboardCapitalization::Mode capitalizationMode)
{
	ASSERT(rootView);
	if(!rootView)
		return;
	
	Rect r;
	rootView->getClientRect (r);
	TextEditView* editView = NEW TextEditView (r, textParam, prompt, callback);
	editView->setKeyboardCapitalizationMode (capitalizationMode);
	editView->setKeyboardMode (mode);
	rootView->setModalView (editView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextInput::isEditing (RootView* rootView)
{
	ASSERT(rootView);
	if(!rootView)
		return false;
	
	if(core_cast<TextEditView> (rootView->getModalView ()) != nullptr)
		return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextInput::stop (RootView* rootView, bool canceled)
{
	ASSERT(rootView);
	if(!rootView)
		return;

	if(TextEditView* editView = core_cast<TextEditView> (rootView->getModalView ()))
		editView->textInputDone (canceled);
}
