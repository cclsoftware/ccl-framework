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
// Filename    : ccl/gui/skin/skincontrols.h
// Description : Skin Control Elements
//
//************************************************************************************************

#ifndef _ccl_skincontrols_h
#define _ccl_skincontrols_h

#include "ccl/gui/skin/skinmodel.h"

namespace CCL {

class ItemControlBase;
interface IItemModel;
interface IParameter;

namespace SkinElements {

//************************************************************************************************
// ControlElement
/** Base class for all controls.
A control is a user interface element that operates on a parameter in a defined way.
The name attribute of a control refers to the name of the parameter that the controller must supply
when the user interface is constructed from the skin description. Instead of a parameter, a control can also trigger a command. */
//************************************************************************************************

class ControlElement: public ViewElement
{
public:
	DECLARE_CLASS (ControlElement, ViewElement)
	DECLARE_TYPELIB_CLASS (ControlElement)

	PROPERTY_MUTABLE_CSTRING (commandCategory, CommandCategory)
	PROPERTY_MUTABLE_CSTRING (commandName, CommandName)

	static IParameter* getParameter (const CreateArgs& args, CStringRef paramName, Element* caller, bool mustResolveName = true);
	static IUnknown* getObject (const CreateArgs& args, CStringRef objectName, UIDRef classID);

	IParameter* getParameter (const CreateArgs& args);
	Image* getImage (const CreateArgs& args, CStringRef name);

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// ButtonElement
//************************************************************************************************

class ButtonElement: public ControlElement
{
public:
	DECLARE_CLASS (ButtonElement, ControlElement)
	DECLARE_TYPELIB_CLASS (ButtonElement)

	PROPERTY_MUTABLE_CSTRING (imageName, ImageName)
	PROPERTY_MUTABLE_CSTRING (iconName, IconName)
	PROPERTY_MUTABLE_CSTRING (colorName, colorName)
	PROPERTY_MUTABLE_CSTRING (titleName, TitleName)
	
	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	bool appendOptions (String& string) const override;
	View* createView (const CreateArgs& args, View* view) override;

protected:
	SharedPtr<Image> image;
	SharedPtr<Image> icon;
};

//************************************************************************************************
// DialogButtonElement
//************************************************************************************************

class DialogButtonElement: public ButtonElement
{
public:
	DECLARE_CLASS (DialogButtonElement, ButtonElement)
	DECLARE_TYPELIB_CLASS (DialogButtonElement)

	DialogButtonElement ();

	PROPERTY_VARIABLE (int, dialogResult, DialogResult)
	PROPERTY_STRING (dialogResultString, DialogResultString)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	View* createView (const CreateArgs& args, View* view) override;

protected:
	enum Flags
	{
		kResolveDialogResult = kLastViewElementFlag << 1
	};

	PROPERTY_FLAG (parseFlags, kResolveDialogResult, mustResolveDialogResult)
};

//************************************************************************************************
// ToggleElement
//************************************************************************************************

class ToggleElement: public ButtonElement
{
public:
	DECLARE_CLASS (ToggleElement, ButtonElement)
	DECLARE_TYPELIB_CLASS (ToggleElement)

	ToggleElement ();

	PROPERTY_BOOL (accessibilityExposeAsButton, AccessibilityExposeAsButton)

	// ViewElement
	View* createView (const CreateArgs& args, View* view) override;
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	bool appendOptions (String& string) const override;
};

//************************************************************************************************
// MultiToggleElement
//************************************************************************************************

class MultiToggleElement: public ToggleElement
{
public:
	DECLARE_CLASS (MultiToggleElement, ToggleElement)
	DECLARE_TYPELIB_CLASS (MultiToggleElement)
	
	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// CheckBoxElement
//************************************************************************************************

class CheckBoxElement: public ToggleElement
{
public:
	DECLARE_CLASS (CheckBoxElement, ToggleElement)
	DECLARE_TYPELIB_CLASS (CheckBoxElement)

	PROPERTY_MUTABLE_CSTRING (mixedIconName, MixedIconName)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	bool appendOptions (String& string) const override;
	View* createView (const CreateArgs& args, View* view) override;

protected:
	SharedPtr<Image> mixedIcon;
};

//************************************************************************************************
// RadioButtonElement
//************************************************************************************************

class RadioButtonElement: public ButtonElement
{
public:
	DECLARE_CLASS (RadioButtonElement, ButtonElement)
	DECLARE_TYPELIB_CLASS (RadioButtonElement)

	PROPERTY_STRING (value, Value)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	View* createView (const CreateArgs& args, View* view) override;

protected:
	float getRadioValue (const CreateArgs& args) const;
};

//************************************************************************************************
// ToolButtonElement
//************************************************************************************************

class ToolButtonElement: public RadioButtonElement
{
public:
	DECLARE_CLASS (ToolButtonElement, RadioButtonElement)
	DECLARE_TYPELIB_CLASS (ToolButtonElement)
	
	ToolButtonElement ();
	
	PROPERTY_MUTABLE_CSTRING (modeName, ModeName)
	PROPERTY_VARIABLE (int, popupOptions, PopupOptions)
	
	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	bool appendOptions (String& string) const override;
	View* createView (const CreateArgs& args, View* view) override;
	
};

//************************************************************************************************
// ValueControlElement
//************************************************************************************************

class ValueControlElement: public ControlElement
{
public:
	DECLARE_CLASS_ABSTRACT (ValueControlElement, ControlElement)
	DECLARE_TYPELIB_CLASS (ValueControlElement)
	PROPERTY_MUTABLE_CSTRING (colorName, ColorName)
	
	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// ValueBarElement
//************************************************************************************************

class ValueBarElement: public ValueControlElement
{
public:
	DECLARE_CLASS (ValueBarElement, ValueControlElement)
	DECLARE_TYPELIB_CLASS (ValueBarElement)

	PROPERTY_MUTABLE_CSTRING (imageName, ImageName)
	
	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	View* createView (const CreateArgs& args, View* view) override;

protected:
	SharedPtr<Image> image;
};

//************************************************************************************************
// ProgressBarElement
//************************************************************************************************

class ProgressBarElement: public ValueBarElement
{
public:
	DECLARE_CLASS (ProgressBarElement, ValueBarElement)
	DECLARE_TYPELIB_CLASS (ProgressBarElement)

	// ViewElement
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// ActivityIndicatorViewElement
//************************************************************************************************

class ActivityIndicatorViewElement: public ProgressBarElement
{
public:
	DECLARE_CLASS (ActivityIndicatorViewElement, ProgressBarElement)
	DECLARE_TYPELIB_CLASS (ActivityIndicatorViewElement)

	// ViewElement
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// SliderElement
//************************************************************************************************

class SliderElement: public ValueControlElement
{
public:
	DECLARE_CLASS (SliderElement, ValueControlElement)
	DECLARE_TYPELIB_CLASS (SliderElement)

	SliderElement ();

	PROPERTY_VARIABLE (int, mode, Mode)
	PROPERTY_VARIABLE (int, xyDistance, XYDistance)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	bool appendOptions (String& string) const override;
	View* createView (const CreateArgs& args, View* view) override;
	
protected:
	bool shouldSetAutoOrientation;
};

//************************************************************************************************
// RangeSliderElement
//************************************************************************************************

class RangeSliderElement: public SliderElement
{
public:
	DECLARE_CLASS (RangeSliderElement, SliderElement)
	DECLARE_TYPELIB_CLASS (RangeSliderElement)
	
	PROPERTY_MUTABLE_CSTRING (paramName2, ParamName2)
	
	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	bool appendOptions (String& string) const override;
	View* createView (const CreateArgs& args, View* view) override;
};
	
	
//************************************************************************************************
// KnobElement
//************************************************************************************************

class KnobElement: public SliderElement
{
public:
	DECLARE_CLASS (KnobElement, SliderElement)
	DECLARE_TYPELIB_CLASS (KnobElement)

	PROPERTY_MUTABLE_CSTRING (referenceName, ReferenceName)
	
	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	bool appendOptions (String& string) const override;
	View* createView (const CreateArgs& args, View* view) override;

};

//************************************************************************************************
// VectorPadElement
//************************************************************************************************

class VectorPadElement: public ValueControlElement
{
public:
	DECLARE_CLASS (VectorPadElement, ValueControlElement)
	DECLARE_TYPELIB_CLASS (VectorPadElement)

	PROPERTY_MUTABLE_CSTRING (yName, YName)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// TriVectorPadElement
//************************************************************************************************

class TriVectorPadElement: public VectorPadElement
{
public:
	DECLARE_CLASS (TriVectorPadElement, VectorPadElement)
	DECLARE_TYPELIB_CLASS (TriVectorPadElement)

	PROPERTY_MUTABLE_CSTRING (zName, ZName)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	bool appendOptions (String& string) const override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// TextBoxElement
//************************************************************************************************

class TextBoxElement: public ControlElement
{
public:
	DECLARE_CLASS (TextBoxElement, ControlElement)
	DECLARE_TYPELIB_CLASS (TextBoxElement)

	TextBoxElement ();

	PROPERTY_MUTABLE_CSTRING (labelName, LabelName)
	PROPERTY_MUTABLE_CSTRING (colorName, colorName)
	PROPERTY_VARIABLE (int, textTrimMode, TextTrimMode)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	bool appendOptions (String& string) const override;
	View* createView (const CreateArgs& args, View* view) override;

protected:
	Rect getTextBoxSize () const;
};

//************************************************************************************************
// EditBoxElement
//************************************************************************************************

class EditBoxElement: public TextBoxElement
{
public:
	DECLARE_CLASS (EditBoxElement, TextBoxElement)
	DECLARE_TYPELIB_CLASS (EditBoxElement)

	EditBoxElement ();

	PROPERTY_VARIABLE (int, returnKeyType, ReturnKeyType)
	PROPERTY_VARIABLE (int, keyboardType, KeyboardType)
	PROPERTY_VARIABLE (int, autofillType, AutofillType)
	PROPERTY_STRING (autofillTypeString, autofillTypeString)
	PROPERTY_STRING (placeholder, PlaceholderString)

protected:
	enum Flags
	{
		kResolveAutofillType = kLastViewElementFlag << 1
	};

	PROPERTY_FLAG (parseFlags, kResolveAutofillType, mustResolveAutofillType)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	bool appendOptions (String& string) const override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// TextEditorElement
//************************************************************************************************

class TextEditorElement: public EditBoxElement
{
public:
	DECLARE_CLASS (TextEditorElement, EditBoxElement)
	DECLARE_TYPELIB_CLASS (TextEditorElement)

	TextEditorElement ();

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	bool appendOptions (String& string) const override;
	View* createView (const CreateArgs& args, View* view) override;

private:
	MutableCString horizontalScrollBarStyle;
	MutableCString verticalScrollBarStyle;
};

//************************************************************************************************
// ValueBoxElement
//************************************************************************************************

class ValueBoxElement: public EditBoxElement
{
public:
	DECLARE_CLASS (ValueBoxElement, EditBoxElement)
	DECLARE_TYPELIB_CLASS (ValueBoxElement)

	ValueBoxElement ();
	
	PROPERTY_VARIABLE (int, xyDistance, XYDistance)
	
	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// SelectBoxElement
//************************************************************************************************

class SelectBoxElement: public TextBoxElement
{
public:
	DECLARE_CLASS (SelectBoxElement, TextBoxElement)
	DECLARE_TYPELIB_CLASS (SelectBoxElement)

	SelectBoxElement ();

	PROPERTY_VARIABLE (int, popupOptions, PopupOptions)
	PROPERTY_MUTABLE_CSTRING (popupStyleName, PopupStyleName)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	bool appendOptions (String& string) const override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// ComboBoxElement
//************************************************************************************************

class ComboBoxElement: public SelectBoxElement
{
public:
	DECLARE_CLASS (ComboBoxElement, SelectBoxElement)
	DECLARE_TYPELIB_CLASS (ComboBoxElement)
	
	ComboBoxElement ();

	PROPERTY_MUTABLE_CSTRING (editName, EditName)
	PROPERTY_VARIABLE (int, returnKeyType, ReturnKeyType)
	PROPERTY_VARIABLE (int, keyboardType, KeyboardType)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	bool appendOptions (String& string) const override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// SegmentBoxElement
//************************************************************************************************

class SegmentBoxElement: public ControlElement
{
public:
	DECLARE_CLASS (SegmentBoxElement, ControlElement)
	DECLARE_TYPELIB_CLASS (SegmentBoxElement)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	bool appendOptions (String& string) const override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// ColorBoxElement
//************************************************************************************************

class ColorBoxElement: public ControlElement
{
public:
	DECLARE_CLASS (ColorBoxElement, ControlElement)
	DECLARE_TYPELIB_CLASS (ColorBoxElement)

	ColorBoxElement ();

	PROPERTY_VARIABLE (Coord, radius, Radius)
	PROPERTY_MUTABLE_CSTRING (selectName, SelectName)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// UpDownButtonElement
//************************************************************************************************

class UpDownButtonElement: public ButtonElement
{
public:
	DECLARE_CLASS (UpDownButtonElement, ButtonElement)
	DECLARE_TYPELIB_CLASS (UpDownButtonElement)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// UpDownBoxElement
//************************************************************************************************

class UpDownBoxElement: public ControlElement
{
public:
	DECLARE_CLASS (UpDownBoxElement, ControlElement)
	DECLARE_TYPELIB_CLASS (UpDownBoxElement)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// SwipeBoxElement
//************************************************************************************************

class SwipeBoxElement: public ControlElement
{
public:
	DECLARE_CLASS (SwipeBoxElement, ControlElement)
	DECLARE_TYPELIB_CLASS (SwipeBoxElement)

	PROPERTY_MUTABLE_CSTRING (targetClass, TargetClass)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// DividerElement
//************************************************************************************************

class DividerElement: public ControlElement
{
public:
	DECLARE_CLASS (DividerElement, ControlElement)
	DECLARE_TYPELIB_CLASS (DividerElement)

	DividerElement ();

	PROPERTY_MUTABLE_CSTRING (imageName, ImageName)
	PROPERTY_VARIABLE (Coord, outreach, Outreach)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// AlignViewElement
//************************************************************************************************

class AlignViewElement: public ControlElement
{
public:
	DECLARE_CLASS (AlignViewElement, ControlElement)
	DECLARE_TYPELIB_CLASS (AlignViewElement)

	PROPERTY_MUTABLE_CSTRING (persistenceID, PersistenceID)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	bool appendOptions (String& string) const override;
	View* createView (const CreateArgs& args, View* view) override;
	void viewCreated (View* view) override;
};

//************************************************************************************************
// ScrollBarElement
//************************************************************************************************

class ScrollBarElement: public ControlElement
{
public:
	DECLARE_CLASS (ScrollBarElement, ControlElement)
	DECLARE_TYPELIB_CLASS (ScrollBarElement)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// ScrollButtonElement
//************************************************************************************************

class ScrollButtonElement: public ScrollBarElement
{
public:
	DECLARE_CLASS (ScrollButtonElement, ScrollBarElement)
	DECLARE_TYPELIB_CLASS (ScrollButtonElement)

	ScrollButtonElement ();

	PROPERTY_VARIABLE (int, partCode, PartCode)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// PageControlElement
//************************************************************************************************

class PageControlElement: public ScrollBarElement
{
public:
	DECLARE_CLASS (PageControlElement, ScrollBarElement)
	DECLARE_TYPELIB_CLASS (PageControlElement)

	// ViewElement
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// ScrollPickerElement
//************************************************************************************************

class ScrollPickerElement: public ControlElement
{
public:
	DECLARE_CLASS (ScrollPickerElement, ControlElement)
	DECLARE_TYPELIB_CLASS (ScrollPickerElement)
	
	PROPERTY_MUTABLE_CSTRING (applyName, ApplyName)
	
	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// TabViewElement
//************************************************************************************************

class TabViewElement: public ControlElement
{
public:
	DECLARE_CLASS (TabViewElement, ControlElement)
	DECLARE_TYPELIB_CLASS (TabViewElement)

	PROPERTY_MUTABLE_CSTRING (persistenceID, PersistenceID)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool appendOptions (String& string) const override;
	bool getAttributes (SkinAttributes& a) const override;
	View* createView (const CreateArgs& args, View* view) override;
};

} // namespace SkinElements
} // CCL

#endif // _ccl_skincontrols_h
