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
// Filename    : ccl/gui/controls/selectbox.h
// Description : Select Box
//
//************************************************************************************************

#ifndef _ccl_selectbox_h
#define _ccl_selectbox_h

#include "ccl/gui/controls/editbox.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/gui/popup/popupselector.h"

namespace CCL {

//************************************************************************************************
// SelectBox
/** A SelectBox is a TextBox with an additional popup menu. 
The SelectBox shows a button besides the text. 
Clicking this button opens a popup menu that shows a list of possible parameter values for the user to choose from. */
//************************************************************************************************

class SelectBox: public TextBox
{
public:
	DECLARE_CLASS (SelectBox, TextBox)
	DECLARE_METHOD_NAMES (SelectBox)

	SelectBox (const Rect& size = Rect (), IParameter* param = nullptr,
			   StyleRef style = 0, StringRef title = nullptr);
	
	~SelectBox ();

	DECLARE_STYLEDEF (customStyles)

	PROPERTY_VARIABLE (int, popupOptions, PopupOptions)
	void setPopupVisualStyle (VisualStyle* visualStyle);
	bool isOpen ();
	void showMenu ();

	enum SelectBoxParts
	{
		kPartNone = 0,
		kPartContentArea = 1,
		kPartDropDownButton = 2,
		kPartDisplayArea = 3  // equals the content area but with padding taken into account
	};

	// TextBox
	StringRef getText () override;
	ThemeRenderer* getRenderer () override;
	void onSize (const Point& delta) override;
	bool onMouseWheel (const MouseWheelEvent& event) override;
	bool onMouseEnter  (const MouseEvent& event) override;
	bool onMouseLeave  (const MouseEvent& event) override;
	bool onMouseDown  (const MouseEvent& event) override;
	bool onKeyDown (const KeyEvent& event) override;
	bool onGesture (const GestureEvent& event) override;
	ITouchHandler* createTouchHandler (const TouchEvent& event) override;
 	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
	AccessibilityProvider* getAccessibilityProvider () override;
	
	Coord getDisplayWidth () const override;
	Coord getDisplayHeight () const override;
	void calcAutoSize (Rect& r) override;

private:
	AutoPtr<PopupSelector> popupSelector;

protected:
	PopupSelector* getPopupSelector ();
	void initPopupSelector ();
	class PopupTouchHandler;

	Coord getDropDownButtonWidth () const;
	Coord getHFitWidth () const override;
};

//************************************************************************************************
// ComboBox
/** A combobox is a combination of a selectbox and a text edit field. 
A ComboBox allows to enter free text like an EditBox, and additionally 
offers a popupmenu with values to choose from, like a SelectBox. */
//************************************************************************************************

class ComboBox: public SelectBox
{
public:
	DECLARE_CLASS (ComboBox, SelectBox)

	ComboBox (const Rect& size = Rect (), 
			  IParameter* selectParam = nullptr, IParameter* editParam = nullptr,
			  StyleRef style = 0, StringRef title = nullptr);
	~ComboBox ();

	DECLARE_STYLEDEF (customStyles)

	PROPERTY_VARIABLE (int, returnKeyType, ReturnKeyType)
	PROPERTY_VARIABLE (int, keyboardType, KeyboardType)
	
	IParameter* getEditParam () const;
	void setEditParam (IParameter* editParam);
	bool isEditing () const override;

	// SelectBox
	IParameter* getTextParameter () const override;
	void attached (View* parent) override;
	void removed (View* parent) override;
	void onSize (const Point& delta) override;
	void onMove (const Point& delta) override;
	bool onFocus (const FocusEvent& event) override;
	bool onMouseDown (const MouseEvent& event) override;
	bool onKeyDown (const KeyEvent& event) override;
	void paramChanged () override;
	ThemeRenderer* getRenderer () override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	bool canEdit () const;

	IParameter* editParam;
	EditBox* textControl;

	bool syncEditTextWithList ();
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline 
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool ComboBox::isEditing () const
{ return textControl && textControl->isEditing (); }

} // namespace CCL

#endif // _ccl_selectbox_h
