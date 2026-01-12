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
// Filename    : core/portable/gui/corecontrols.h
// Description : Control classes
//
//************************************************************************************************

#ifndef _corecontrols_h
#define _corecontrols_h

#include "core/portable/gui/coreview.h"

#include "core/portable/coreparams.h"

namespace Core {
namespace Portable {

//************************************************************************************************
// Label
/** A label renders static text.
	\ingroup core_gui */
//************************************************************************************************

class Label: public View
{
public:
	DECLARE_CORE_CLASS ('Labl', Label, View)
	DECLARE_CORE_VIEWCLASS (ViewClasses::kLabel)

	Label (RectRef size = Rect (), CStringPtr title = "");

	PROPERTY_CSTRING_BUFFER (128, title, Title)	
	PROPERTY_FLAG (options, Skin::kLabelAppearanceColorize, isColorize)

	// View
	void setAttributes (const Attributes& a) override;
	void draw (const DrawEvent& e) override;
};
	
//************************************************************************************************
// MultiLineLabel
/** A label that renders multiple lines of static text. Less efficient than Label, use only when necessary.
	\ingroup core_gui */
//************************************************************************************************

class MultiLineLabel: public View
{
public:
	DECLARE_CORE_CLASS ('MLbl', MultiLineLabel, View)
	DECLARE_CORE_VIEWCLASS (ViewClasses::kMultiLineLabel)
	
	MultiLineLabel (RectRef size = Rect (), CStringPtr title = "");
	
	static const int kMaxLength = GraphicsRenderer::kMaxMultilineStringLength;
	PROPERTY_CSTRING_BUFFER (kMaxLength, title, Title)
	
	// View
	void setAttributes (const Attributes& a) override;
	void draw (const DrawEvent& e) override;
};

//************************************************************************************************
// ImageView
/** An image view renders a static bitmap.
	\ingroup core_gui */
//************************************************************************************************

class ImageView: public ContainerView,
 			     public IParamObserver

{
public:
	BEGIN_CORE_CLASS ('ImgV', ImageView)
		ADD_CORE_CLASS_ (IParamObserver)
	END_CORE_CLASS (ContainerView)
	DECLARE_CORE_VIEWCLASS (ViewClasses::kImageView)

	ImageView (RectRef size = Rect ());
	~ImageView ();

	PROPERTY_VARIABLE (BitmapReference, image, Image)
	PROPERTY_FLAG (options, Skin::kImageViewAppearanceColorize, isColorize)
	PROPERTY_VARIABLE (float, imageAlpha, ImageAlpha)

	// ContainerView
	void setAttributes (const Attributes& a) override;
	void draw (const DrawEvent& e) override;
	void connect (void* object) override;
	CStringPtr getConnectionType () const override;

protected:
	Parameter* parameter;

	// IParamObserver
	void paramChanged (Parameter* p, int msg) override;
};

//************************************************************************************************
// VariantView
/** Select child view via parameter.
	\ingroup core_gui */
//************************************************************************************************

class VariantView: public ContainerView,
				   public IParamObserver
{
public:
	BEGIN_CORE_CLASS ('VarV', VariantView)
		ADD_CORE_CLASS_ (IParamObserver)
	END_CORE_CLASS (ContainerView)
	DECLARE_CORE_VIEWCLASS (ViewClasses::kVariantView)

	VariantView (RectRef size = Rect ());
	~VariantView ();

	// ContainerView
	void addView (View* view) override;
	CStringPtr getConnectionType () const override;
	void connect (void* object) override;
	void resizeToChildren () override;

protected:
	Parameter* parameter;
	ViewChildren variants;

	void selectVariant (int index);

	// IParamObserver
	void paramChanged (Parameter* p, int msg) override;
};

//************************************************************************************************
// IVariantChildView
/** \ingroup core_gui */
//************************************************************************************************

struct IVariantChildView: ITypedObject
{
	DECLARE_CORE_CLASS_ ('VChV', IVariantChildView)

	virtual void onVariantAttached (bool state) = 0;
};

//************************************************************************************************
// AlignView
/** A container view that arranges child views according to alignment flags.
	\ingroup core_gui */
//************************************************************************************************

class AlignView: public ContainerView
{
public:
	DECLARE_CORE_CLASS ('AlnV', AlignView, ContainerView)
	DECLARE_CORE_VIEWCLASS (ViewClasses::kAlignView)

	AlignView (RectRef size = Rect ());

	PROPERTY_VARIABLE (int, alignment, Alignment)

	// ContainerView
	void setAttributes (const Attributes& a) override;
	void addView (View* view) override;
	void setSize (RectRef newSize) override;

protected:
	void layoutChild (View* view);
	void layoutAll ();
};

//************************************************************************************************
// Control
/** A control is dependent on a parameter.
	\ingroup core_gui */
//************************************************************************************************

class Control: public View,
			   public IParamObserver
{
public:
	BEGIN_CORE_CLASS ('Cntl', Control)
		ADD_CORE_CLASS_ (IParamObserver)
	END_CORE_CLASS (View)
	DECLARE_CORE_VIEWCLASS (ViewClasses::kControl)

	Control (RectRef size = Rect (), Parameter* p = nullptr);
	~Control ();

	void setParameter (Parameter* p);
	Parameter* getParameter () const;
	
	// IParamObserver
	void paramChanged (Parameter* p, int msg) override;

	// View
	CStringPtr getConnectionType () const override;
	void connect (void* object) override;
	bool onWheelInput (const WheelEvent& e) override;
	void onIdle () override;
	
protected:
	Parameter* parameter;
	int wheelAccumulation;

	void drawFocusFrame (const DrawEvent& e);
	
	double endEditTime;
	static const double kEditResetDelay;
};

//************************************************************************************************
// Button
/** A simple push button.
	\ingroup core_gui */
//************************************************************************************************

class Button: public Control
{
public:
	DECLARE_CORE_CLASS ('Bttn', Button, Control)
	DECLARE_CORE_VIEWCLASS (ViewClasses::kButton)

	Button (RectRef size = Rect (), Parameter* p = nullptr);

	PROPERTY_VARIABLE (BitmapReference, image, Image)
	PROPERTY_VARIABLE (BitmapReference, icon, Icon)
	PROPERTY_CSTRING_BUFFER (128, title, Title)
	PROPERTY_FLAG (options, Skin::kButtonBehaviorDeferred, isDeferred)
	PROPERTY_FLAG (options, Skin::kButtonAppearanceTransparent, isTransparent)
	PROPERTY_FLAG (options, Skin::kButtonBehaviorSilentTracking, isSilentTracking)
	
	// Control
	void setAttributes (const Attributes& a) override;
	void draw (const DrawEvent& e) override;
	bool onTouchInput (const TouchEvent& e) override;
	bool onGestureInput (const GestureEvent& e) override;
	void getHandledGestures (GestureVector& gestures, PointRef where) override;
	
protected:
	bool down;

	virtual bool isOn () const;
	virtual void push ();	
};

//************************************************************************************************
// Toggle
/** Value is toggled between on/off.
	\ingroup core_gui */
//************************************************************************************************

class Toggle: public Button
{
public:
	DECLARE_CORE_CLASS ('Tggl', Toggle, Button)
	DECLARE_CORE_VIEWCLASS (ViewClasses::kToggle)

	Toggle (RectRef size = Rect (), Parameter* p = nullptr);

protected:
	// Button
	bool isOn () const override;
	void push () override;
};
	
//************************************************************************************************
// RadioButton
/** A RadioButton sets a parameter to a specific value.
 A RadioButton sets a parameter to a specific value. It appears in the "on" state when the parameter has that value.
 Usually multiple RadioButtons for the same parameter are used together to give direct access to different values.
	\ingroup core_gui */
//************************************************************************************************

class RadioButton: public Button
{
public:
	DECLARE_CORE_CLASS ('RBtn', RadioButton, Button)
	DECLARE_CORE_VIEWCLASS (ViewClasses::kRadioButton)

	RadioButton (RectRef size = Rect (), Parameter* p = nullptr, int value = 0);
	
	PROPERTY_VARIABLE (int, value, Value)

	// Button
	void setAttributes (const Attributes& a) override;
	
protected:
	// Button
	bool isOn () const override;
	void push () override;
};

//************************************************************************************************
// ValueBar
/** Value is drawn as filled rectangle.
	\ingroup core_gui */
//************************************************************************************************

class ValueBar: public Control
{
public:
	DECLARE_CORE_CLASS ('ValB', ValueBar, Control)
	DECLARE_CORE_VIEWCLASS (ViewClasses::kValueBar)

	ValueBar (RectRef size = Rect (), Parameter* p = nullptr);

	PROPERTY_FLAG (options, Skin::kValueBarAppearanceVertical, isVertical)
	PROPERTY_FLAG (options, Skin::kValueBarAppearanceCentered, isCentered)
	PROPERTY_FLAG (options, Skin::kValueBarAppearanceFilmstrip, isFilmstrip)
	PROPERTY_VARIABLE (BitmapReference, background, Background)
	PROPERTY_VARIABLE (BitmapReference, image, Image)

	// Control
	void setAttributes (const Attributes& a) override;
	void draw (const DrawEvent& e) override;

protected:
	virtual void drawBackground (const DrawEvent& e);
};

//************************************************************************************************
// Slider
/** Value is edited continuously.
	\ingroup core_gui */
//************************************************************************************************

class Slider: public ValueBar
{
public:
	DECLARE_CORE_CLASS ('Sldr', Slider, ValueBar)
	DECLARE_CORE_VIEWCLASS (ViewClasses::kSlider)

	Slider (RectRef size = Rect (), Parameter* p = nullptr);

	// ValueBar
	bool onTouchInput (const TouchEvent& e) override;
};

//************************************************************************************************
// TextBox
/** Displays parameter value as string.
	\ingroup core_gui */
//************************************************************************************************

class TextBox: public Control
{
public:
	DECLARE_CORE_CLASS ('TxtB', TextBox, Control)
	DECLARE_CORE_VIEWCLASS (ViewClasses::kTextBox)
	
	TextBox (RectRef size = Rect (), Parameter* p = nullptr);

	PROPERTY_VARIABLE (Skin::TextTrimMode, trimMode, TrimMode)
	PROPERTY_FLAG (options, Skin::kTextBoxAppearanceMultiLine, isMultiLine)
	PROPERTY_FLAG (options, Skin::kTextBoxAppearanceHideText, hideText)

	typedef StringParam::TextValue TextValue;
	static void collapseString (TextValue& string, const Graphics& graphics, CStringPtr fontName, int maxWidth, Skin::TextTrimMode trimMode);
	
	// Control
	void draw (const DrawEvent& e) override;
	void setAttributes (const Attributes& a) override;

protected:
	void getText (TextValue& text) const;
	void drawText (Graphics& graphics, RectRef textRect);
};

//************************************************************************************************
// EditBox
/** Edit parameter string.
	\ingroup core_gui */
//************************************************************************************************

class EditBox: public TextBox
{
public:
	DECLARE_CORE_CLASS ('EdBx', EditBox, TextBox)
	DECLARE_CORE_VIEWCLASS (ViewClasses::kEditBox)

	EditBox (RectRef size = Rect (), Parameter* p = nullptr);

	PROPERTY_VARIABLE (Skin::KeyboardLayout::Mode, keyLayout, KeyboardLayout)
	PROPERTY_VARIABLE (Skin::KeyboardCapitalization::Mode, capitalizationMode, CapitalizationMode)
		
	// TextBox
	void draw (const DrawEvent& e) override;
	bool onTouchInput (const TouchEvent& e) override;
	void setAttributes (const Attributes& a) override;
};

//************************************************************************************************
// SelectBox
/** Presents menu for list parameter.
	\ingroup core_gui */
//************************************************************************************************

class SelectBox: public TextBox
{
public:
	DECLARE_CORE_CLASS ('SlBx', SelectBox, TextBox)
	DECLARE_CORE_VIEWCLASS (ViewClasses::kSelectBox)

	SelectBox (RectRef size = Rect (), Parameter* p = nullptr);

	PROPERTY_VARIABLE (BitmapReference, image, Image)

	// TextBox
	void draw (const DrawEvent& e) override;
	bool onTouchInput (const TouchEvent& e) override;
	void setAttributes (const Attributes& a) override;
	
protected:
	const Attributes* menuStyle;
	Rect menuRect;
	Rect textRect;
};

//************************************************************************************************
// TextInput
/** \ingroup core_gui */
//************************************************************************************************

struct TextInput 
{		
	struct ICompletionCallback
	{
		virtual void textInputFinished (bool canceled) = 0;
	};

	static void start (RootView* parent, 
					   Parameter* textParam, 
					   CStringPtr prompt = nullptr, 
					   ICompletionCallback* cb = nullptr, 
					   Skin::KeyboardLayout::Mode mode = Skin::KeyboardLayout::kLetters,
					   Skin::KeyboardCapitalization::Mode capitalizationMode = Skin::KeyboardCapitalization::kFirst);

	static bool isEditing (RootView* parent);
	static void stop (RootView* parent, bool canceled);
};

} // namespace Portable
} // namespace Core

#endif // _corecontrols_h
