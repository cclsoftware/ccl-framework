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
// Filename    : ccl/gui/controls/button.h
// Description : Button Controls
//
//************************************************************************************************

#ifndef _ccl_button_h
#define _ccl_button_h

#include "ccl/gui/controls/control.h"

#include "ccl/gui/theme/visualstyleclass.h"
#include "ccl/public/gui/framework/themeelements.h"

namespace CCL {

class Image;
class PopupSelector;

//************************************************************************************************
// Button
/** Button control base class and simple push button control.
A Button is a control that changes the value of its parameter on a click/touch. 
By default, this happens when the mouse button or touch is released, which can be changed with the option "immediate".

The Button base class behaves like a push button: it momentarily sets the parameter value to its maximum 
value and then resets it afterwards to its minimum. Derived classes like Toggle and MultiToggle behave differently. 
\see Toggle \see MultiToggle */
//************************************************************************************************

class Button: public Control,
			  public Control::PhaseProperty<Button>,
			  public IButton
{
public:
	DECLARE_CLASS (Button, Control)
	DECLARE_METHOD_NAMES (Button)

	Button (const Rect& size = Rect (), IParameter* param = nullptr, StyleRef style = 0, StringRef title = nullptr);
	~Button ();
	
	void CCL_API push () override; ///< IButton
	virtual void preview ();
	virtual int getCurrentFrame () const;
	virtual int getNumFrames () const;
	virtual bool isOn () const;

	void setTitleParam (IParameter* p);

	IImage* getIcon () const;
	void setIcon (IImage* icon);
	
	IParameter* getColorParam () const;
	void setColorParam (IParameter* colorParam);
	
	DECLARE_STYLEDEF (customStyles)

	PROPERTY_FLAG (privateFlags, kHasIconFromVisualStyle, hasIconFromVisualStyle)
	PROPERTY_FLAG (privateFlags, kEditingParameter, isEditingParameter)

	// Control
	ThemeRenderer* getRenderer () override;
	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	ITouchHandler* createTouchHandler (const TouchEvent& event) override;
	void removed (View* parent) override;
	bool onMouseEnter (const MouseEvent& event) override;
	bool onMouseLeave (const MouseEvent& event) override;
	bool onMouseDown (const MouseEvent& event) override;
	bool onKeyDown (const KeyEvent& event) override;
	void calcAutoSize (Rect& r) override;
	void calcSizeLimits () override;
	void onSize (const Point& delta) override;
	void onVisualStyleChanged () override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	AccessibilityProvider* getAccessibilityProvider () override;

	CLASS_INTERFACE (IButton, Control)

protected:
	SharedPtr<IImage> icon;
	IParameter* titleParam;
	IParameter* colorParam;

	enum PrivateFlags
	{
		kHasIconFromVisualStyle = 1<<(kLastPrivateFlag + 1),
		kEditingParameter		= 1<<(kLastPrivateFlag + 2)
	};
};

//************************************************************************************************
// Toggle
/** A Button with two states. 
A Toggle is a Button with two stable states. 
On each click it toggles the parameter value between it's minimum and maximum.*/
//************************************************************************************************

class Toggle: public Button
{
public:
	DECLARE_CLASS (Toggle, Button)

	Toggle (const Rect& size = Rect (), IParameter* param = nullptr, StyleRef style = 0, StringRef title = nullptr);

	DECLARE_STYLEDEF (customStyles)

	// Button
	void CCL_API push () override;
	void preview () override;
	int getNumFrames () const override;
	int getCurrentFrame () const override;
	bool isOn () const override;
	ThemeRenderer* getRenderer () override;
	AccessibilityProvider* getAccessibilityProvider () override;
};

//************************************************************************************************
// MultiToggle
/** A Button with multiple states. 
A MultiToggle is a Button that has as many stable states as the parameter (usually of type integer) has. 
Every click increments the parameter until it wraps around from maximum to minimum. */
//************************************************************************************************
	
class MultiToggle: public Toggle
{
public:
	DECLARE_CLASS (MultiToggle, Toggle)

	MultiToggle (const Rect& size = Rect (), IParameter* param = nullptr, StyleRef style = 0, StringRef title = nullptr);

	DECLARE_STYLEDEF (customStyles)
	
	// Toggle
	void CCL_API push () override;
	void preview () override;
	int getNumFrames () const override;
	int getCurrentFrame () const override;
	ThemeRenderer* getRenderer () override;
};

//************************************************************************************************
// CheckBox
/** A standard dialog control with a check/unchecked state.
A Checkbox is a Toggle that can draw an additional title besides the image. */
//************************************************************************************************

class CheckBox: public Toggle
{
public:
	DECLARE_CLASS (CheckBox, Toggle)

	CheckBox (const Rect& size = Rect (), IParameter* param = nullptr, StyleRef style = 0, StringRef title = nullptr);

	IImage* getMixedIcon () const;
	void setMixedIcon (IImage* icon);
	bool isMixed () const;

	DECLARE_STYLEDEF (customStyles)

	// Toggle
	ThemeRenderer* getRenderer () override;
	void calcAutoSize (Rect& r) override;
	bool isOn () const override;
	void calcSizeLimits () override;

protected:
	SharedPtr<IImage> mixedIcon;
};

//************************************************************************************************
// RadioButton
/** A RadioButton sets a parameter to a specific value. 
A RadioButton sets a parameter to a specific value. It appears in the "on" state when the parameter has that value.

Usually multiple RadioButtons for the same parameter are used together to give direct access to different values. */
//************************************************************************************************

class RadioButton: public CheckBox
{
public:
	DECLARE_CLASS (RadioButton, CheckBox)

	RadioButton (const Rect& size = Rect (), IParameter* param = nullptr, StyleRef style = 0, StringRef title = nullptr, float value = 0);
	
	DECLARE_STYLEDEF (customStyles)
	
	PROPERTY_VARIABLE (float, value, Value)

	// Checkbox
	void CCL_API push () override;
	void preview () override;
	bool isEnabled () const override;
	bool isOn () const override;
	int getCurrentFrame () const override;
	ThemeRenderer* getRenderer () override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
};

//************************************************************************************************
// ToolButton
/** A ToolButton is a RadioButton with an additional icon.
The ToolButton behaves like a RadioButton, but can have an icon like the normal Button.
A ToolButton can also have an an additional "mode parameter" that is used for tools with different modes. 
But this feature is not available on skin level. */
//************************************************************************************************

class ToolButton: public RadioButton
{
public:
	DECLARE_CLASS (ToolButton, RadioButton)

	ToolButton (const Rect& size = Rect (), IParameter* param = nullptr, StyleRef style = 0, float value = 0);
	~ToolButton ();

	PROPERTY_VARIABLE (int, popupOptions, PopupOptions)
	void setModeParam (IParameter* p);
	void showModeMenu ();
	void setStyle (StyleRef _style) override;
	
	DECLARE_STYLEDEF (customStyles)
	static const int kModeMenuDelay = 300;
	
	// Button
	ThemeRenderer* getRenderer () override;
	void draw (const UpdateRgn& updateRgn) override;
	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	ITouchHandler* createTouchHandler (const TouchEvent& event) override;
	bool onGesture (const GestureEvent& event) override;
	bool onMouseWheel (const MouseWheelEvent& event) override;
	bool onContextMenu (const ContextMenuEvent& event) override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	int getNumFrames () const override;
	
protected:
	// Control
	void paramChanged () override;
	
private:
	IParameter* modeParam;
	PopupSelector* popupSelector;
	bool activateAfterModeSelection;
	
	bool isPopupStyleButton () const;

	class PopupTouchHandler;
};

DECLARE_VISUALSTYLE_CLASS (ToolButton)

} // namespace CCL

#endif // _ccl_button_h
