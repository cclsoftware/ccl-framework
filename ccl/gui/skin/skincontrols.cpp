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
// Filename    : ccl/gui/skin/skincontrols.cpp
// Description : Skin Control Elements
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/skin/skincontrols.h"

#include "ccl/gui/controls/button.h"
#include "ccl/gui/controls/slider.h"
#include "ccl/gui/controls/knob.h"
#include "ccl/gui/controls/tabview.h"
#include "ccl/gui/controls/selectbox.h"
#include "ccl/gui/controls/valuebox.h"
#include "ccl/gui/controls/colorbox.h"
#include "ccl/gui/controls/segmentbox.h"
#include "ccl/gui/controls/texteditor.h"
#include "ccl/gui/controls/popupbox.h"
#include "ccl/gui/controls/updownbox.h"
#include "ccl/gui/controls/scrollbar.h"
#include "ccl/gui/controls/vectorpad.h"
#include "ccl/gui/controls/trivectorpad.h"
#include "ccl/gui/controls/swipehandler.h"
#include "ccl/gui/controls/scrollpicker.h"

#include "ccl/gui/dialogs/dialogbuilder.h"

#include "ccl/gui/layout/divider.h"
#include "ccl/gui/layout/alignview.h"

#include "ccl/gui/skin/form.h"
#include "ccl/gui/skin/skinattributes.h"
#include "ccl/gui/skin/skinlayouts.h"
#include "ccl/gui/skin/skinwizard.h"

#include "ccl/public/gui/icontroller.h"
#include "ccl/public/gui/framework/skinxmldefs.h"

#include "ccl/gui/commands.h"

namespace CCL {
namespace SkinElements {

//////////////////////////////////////////////////////////////////////////////////////////////////

void linkSkinControls () {} // force linkage of this file

//************************************************************************************************
// ControlElement
//************************************************************************************************

IParameter* ControlElement::getParameter (const CreateArgs& args, CStringRef inParamName, Element* caller, bool mustResolveName)
{
	if(!inParamName.isEmpty ())
	{
		IParameter* parameter = nullptr;
		UnknownPtr<IController> controller;

		SkinWizard::ResolvedName resolvedParamName (args.wizard, inParamName, mustResolveName);
		CStringRef paramName = resolvedParamName.string ();
		
		// try to interpret the name as "controllerPath/paramName"
		int pos = paramName.lastIndex ('/');
		if(pos >= 0)
		{
			MutableCString controllerPath (paramName.subString (0, pos));
			MutableCString pName (paramName.subString (pos + 1));
			controller = args.wizard.lookupController (args.controller, controllerPath);
			if(controller)
				parameter = controller->findParameter (pName);
		}
		else
		{
			controller = args.controller;
			if(controller)
				parameter = controller->findParameter (paramName);
		}

		if(parameter) // success :-)
			return parameter;

		if(controller == nullptr)
		{
			SKIN_WARNING (caller, "Controller not found for Parameter: '%s'", inParamName.str ())
			CCL_PRINT (inParamName);
			CCL_DEBUGGER (" -> Controller not found for Parameter\n")
		}
		else
		{
			SKIN_WARNING (caller, "Parameter not found: '%s'", paramName.str ())
			CCL_PRINT (paramName);
			CCL_DEBUGGER (" -> Parameter not found\n")
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* ControlElement::getObject (const CreateArgs& args, CStringRef objectName, UIDRef classID)
{
	if(!objectName.isEmpty ())
	{
		UnknownPtr<IController> controller (args.controller);
		if(controller)
		{
			SkinWizard::ResolvedName resolvedName (args.wizard, objectName);
			CStringRef name = resolvedName.string ();
			IUnknown* object = controller->getObject (name, classID);
			if(object)
				return object;

			// try to interpret the name as "controllerPath/objectName"
			int pos = name.lastIndex ('/');
			if(pos >= 0)
			{
				MutableCString controllerPath (name.subString (0, pos));
				MutableCString oName (name.subString (pos + 1));
				controller = args.wizard.lookupController (controller, controllerPath);
				if(controller)
				{
					object = controller->getObject (oName, classID);
					if(object)
						return object;
				}
			}

			CCL_PRINT (name);
			CCL_DEBUGGER (" -> Object not found\n")
		}
		else
		{
			CCL_PRINT (objectName);
			CCL_DEBUGGER (" -> Controller not found for Object\n")
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (ControlElement, ViewElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (ControlElement, ViewElement, TAG_CONTROL, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (ControlElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (ControlElement, Control)
BEGIN_TYPELIB_MEMBERS (ControlElement)
	ADD_TYPELIB_MEMBER_STRING (ATTR_CMDCATEGORY) ///< If a control triggers a command, this is the command category.
	ADD_TYPELIB_MEMBER_STRING (ATTR_CMDNAME) ///< If a control triggers a command, this is the command name.
END_TYPELIB_MEMBERS (ControlElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ControlElement::getParameter (const CreateArgs& args)
{
	if(!getName ().isEmpty ())
		return getParameter (args, getName (), this, mustResolveName ());

	if(!commandCategory.isEmpty () && !commandName.isEmpty ())
	{
		SkinWizard::ResolvedName resolvedCategory (args.wizard, commandCategory);
		SkinWizard::ResolvedName resolvedName (args.wizard, commandName);
		return CommandTable::instance ().getCommandParam (resolvedCategory.string (), resolvedName.string ());
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* ControlElement::getImage (const CreateArgs& args, CStringRef name)
{
	if(name.isEmpty ())
		return nullptr;

	if(name.startsWith (SkinVariable::kPrefix))
	{
		// try object from skin variables
		if(const SkinVariable* var = args.wizard.getVariable (name))
		{
			if(Image* result = unknown_cast<Image> (var->getValue ().asUnknown ()))
				return result;
		}
	}

	SkinWizard::ResolvedName resolvedName (args.wizard, name);
	return args.wizard.getModel ().getImage (resolvedName.string (), this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ControlElement::setAttributes (const SkinAttributes& a)
{
	commandCategory = a.getString (ATTR_CMDCATEGORY);
	commandName = a.getString (ATTR_CMDNAME);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ControlElement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_CMDCATEGORY, commandCategory);
	a.setString (ATTR_CMDNAME, commandName);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ControlElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		IParameter* p = getParameter (args);
		view = NEW Control (size, p, 0, getTitle ());
	}
	return SuperClass::createView (args, view);
}

//************************************************************************************************
// ButtonElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ButtonElement, ControlElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (ButtonElement, ControlElement, TAG_BUTTON, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (ButtonElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (ButtonElement, Button)
BEGIN_TYPELIB_MEMBERS (ButtonElement)
	ADD_TYPELIB_MEMBER_STRING (ATTR_IMAGE)	///< Background image. The button uses different frames of the image to show its different states.
	ADD_TYPELIB_MEMBER_STRING (ATTR_ICON)	///< The icon is plotted onto the button. This allows the same background bitmap to be used and varied with the icon image.
	ADD_TYPELIB_MEMBER_STRING (ATTR_TITLENAME)
	ADD_TYPELIB_MEMBER_COLOR (ATTR_COLORNAME)
END_TYPELIB_MEMBERS (ButtonElement)
DEFINE_TYPELIB_ENUMERATION (TAG_BUTTON, ATTR_OPTIONS, Button::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ButtonElement::setAttributes (const SkinAttributes& a)
{
	imageName = a.getString (ATTR_IMAGE);
	iconName = a.getString (ATTR_ICON);
	titleName = a.getString (ATTR_TITLENAME);
	colorName = a.getString (ATTR_COLORNAME);
	a.getOptions (options, ATTR_OPTIONS, Button::customStyles);

	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ButtonElement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_IMAGE, imageName);
	a.setString (ATTR_ICON, iconName);
	a.setString (ATTR_TITLENAME, titleName);
	a.setString (ATTR_COLORNAME, colorName);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ButtonElement::appendOptions (String& string) const
{
	SkinAttributes::makeOptionsString (string, options.custom, Button::customStyles);
	return SuperClass::appendOptions (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ButtonElement::createView (const CreateArgs& args, View* view)
{
	Rect r (size);

	if(!view)
		view = NEW Button (r, getParameter (args), options, title);

	if(view)
	{
		if(!imageName.isEmpty ())
			image = getImage (args, imageName);

		// assign button image
		if(image)
		{
			AutoPtr<VisualStyle> vs = NEW VisualStyle;
			vs->setImage (StyleID::kBackground, image);
			view->setVisualStyle (vs);

			// resize to image
			if(size.isEmpty ())
			{
				r (0, 0, image->getWidth (), image->getHeight ());
				view->setSize (r);
			}
		}
		
		if(!iconName.isEmpty ())
			icon = getImage (args, iconName);

		if(icon)
			((Button*)view)->setIcon (icon);
		
		if(!titleName.isEmpty ())
		{
			IParameter* param = ControlElement::getParameter (args, titleName, this);
			ASSERT (param)
			if(param)
				((Button*)view)->setTitleParam (param);
		}

		if(!colorName.isEmpty ())
			((Button*)view)->setColorParam (getParameter (args, colorName, this));
	}

	return SuperClass::createView (args, view);
}

//************************************************************************************************
// DialogButtonElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DialogButtonElement, ButtonElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (DialogButtonElement, ButtonElement, TAG_DIALOGBUTTON, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (DialogButtonElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (DialogButtonElement, StandardButton)
BEGIN_TYPELIB_MEMBERS (DialogButtonElement)
	ADD_TYPELIB_MEMBER (ATTR_RESULT, TypeNames::kEnum)
END_TYPELIB_MEMBERS (DialogButtonElement)
DEFINE_TYPELIB_ENUMERATION (TAG_DIALOGBUTTON, ATTR_RESULT, Dialog::dialogButtons, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

DialogButtonElement::DialogButtonElement ()
: dialogResult (DialogResult::kOkay)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DialogButtonElement::setAttributes (const SkinAttributes& a)
{
	dialogResultString = a.getString (ATTR_RESULT);
	if(dialogResultString.startsWith (SkinVariable::prefix))
		mustResolveDialogResult (true);
	else
		dialogResult = Styles::toDialogResult (a.getOptions (ATTR_RESULT, Dialog::dialogButtons, true, Styles::kOkayButton));

	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DialogButtonElement::getAttributes (SkinAttributes& a) const
{
	a.setOptions (ATTR_RESULT, Styles::toDialogButton (dialogResult), Dialog::dialogButtons, true);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* DialogButtonElement::createView (const CreateArgs& args, View* view)
{
	if(view == nullptr)
	{
		if(mustResolveDialogResult ())
		{
			String resolvedResultString = args.wizard.resolveTitle (dialogResultString);
			if(!resolvedResultString.getIntValue (dialogResult))
				dialogResult = Styles::toDialogResult (SkinAttributes::parseOptions (resolvedResultString, Dialog::dialogButtons, true, Styles::kOkayButton));
		}

		view = DialogBuilder::createStandardButton (size, dialogResult, mustResolveTitle () ? args.wizard.resolveTitle (title) : title);
		if(view)
			view->setStyle (options);
		
		if(mustResolveTitle ())
		{
			ScopedVar scope (title, String::kEmpty); // prevent overriding title from StandardButton ctor when variable is empty
			return SuperClass::createView (args, view);
		}
}
	return SuperClass::createView (args, view);
}

//************************************************************************************************
// ToggleElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ToggleElement, ButtonElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (ToggleElement, ButtonElement, TAG_TOGGLE, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (ToggleElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (ToggleElement, Toggle)
BEGIN_TYPELIB_MEMBERS (ToggleElement)
	ADD_TYPELIB_MEMBER (ATTR_ACCESSIBILITYBUTTON, TypeNames::kBool)
END_TYPELIB_MEMBERS (ToggleElement)
DEFINE_TYPELIB_ENUMERATION (TAG_TOGGLE, ATTR_OPTIONS, Toggle::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

ToggleElement::ToggleElement ()
: accessibilityExposeAsButton (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ToggleElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
		view = NEW Toggle (size, getParameter (args), options, title);

	((Toggle*)view)->setAccessibilityExposeAsButton (accessibilityExposeAsButton);

	return SuperClass::createView (args, view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToggleElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);

	StyleFlags toggleOptions;
	a.getOptions (toggleOptions, ATTR_OPTIONS, Toggle::customStyles);
	options.custom |= toggleOptions.custom;

	accessibilityExposeAsButton = a.getBool (ATTR_ACCESSIBILITYBUTTON, false);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToggleElement::getAttributes (SkinAttributes& a) const
{
	a.setBool (ATTR_ACCESSIBILITYBUTTON, accessibilityExposeAsButton);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToggleElement::appendOptions (String& string) const
{
	SkinAttributes::makeOptionsString (string, options.custom, Toggle::customStyles);
	return SuperClass::appendOptions (string);
}

//************************************************************************************************
// MultiToggleElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MultiToggleElement, ToggleElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (MultiToggleElement, ToggleElement, TAG_MULTITOGGLE, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (MultiToggleElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (MultiToggleElement, MultiToggle)
DEFINE_TYPELIB_ENUMERATION (TAG_MULTITOGGLE, ATTR_OPTIONS, MultiToggle::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

View* MultiToggleElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
		view = NEW MultiToggle (size, getParameter (args), options, title);
	
	return SuperClass::createView (args, view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MultiToggleElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);
	
	StyleFlags multiToggleOptions;
	a.getOptions (multiToggleOptions, ATTR_OPTIONS, MultiToggle::customStyles);
	options.custom |= multiToggleOptions.custom;

	return true;
}
	
//************************************************************************************************
// CheckBoxElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CheckBoxElement, ToggleElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (CheckBoxElement, ToggleElement, TAG_CHECKBOX, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (CheckBoxElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (CheckBoxElement, CheckBox)
BEGIN_TYPELIB_MEMBERS (CheckBoxElement)
	ADD_TYPELIB_MEMBER_STRING (ATTR_ICON_MIXED) ///< Icon for "mixed" state in a tri-state checkbox
END_TYPELIB_MEMBERS (CheckBoxElement)
DEFINE_TYPELIB_ENUMERATION (TAG_CHECKBOX, ATTR_OPTIONS, CheckBox::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CheckBoxElement::setAttributes (const SkinAttributes& a)
{
	bool succeeded = SuperClass::setAttributes (a);

	mixedIconName = a.getString (ATTR_ICON_MIXED);

	StyleFlags checkBoxOptions;
	a.getOptions (checkBoxOptions, ATTR_OPTIONS, CheckBox::customStyles);
	options.custom |= checkBoxOptions.custom;

	return succeeded;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CheckBoxElement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_ICON_MIXED, mixedIconName);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CheckBoxElement::appendOptions (String& string) const
{
	SkinAttributes::makeOptionsString (string, options.custom, CheckBox::customStyles);
	return SuperClass::appendOptions (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* CheckBoxElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
		view = NEW CheckBox (size, getParameter (args), options, title);

	if(!mixedIconName.isEmpty ())
		mixedIcon = getImage (args, mixedIconName);

	if(mixedIcon)
		static_cast<CheckBox*> (view)->setMixedIcon (mixedIcon);

	return SuperClass::createView (args, view);
}

//************************************************************************************************
// RadioButtonElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (RadioButtonElement, ButtonElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (RadioButtonElement, ButtonElement, TAG_RADIOBUTTON, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (RadioButtonElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (RadioButtonElement, RadioButton)
BEGIN_TYPELIB_MEMBERS (RadioButtonElement)
	ADD_TYPELIB_MEMBER_STRING (ATTR_VALUE) ///< The parameter value that this button represents
END_TYPELIB_MEMBERS (RadioButtonElement)
DEFINE_TYPELIB_ENUMERATION (TAG_RADIOBUTTON, ATTR_OPTIONS, RadioButton::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RadioButtonElement::setAttributes (const SkinAttributes& a)
{
	value = a.getString (ATTR_VALUE);
	
	SuperClass::setAttributes (a);
	
	StyleFlags radioButtonOptions;
	a.getOptions (radioButtonOptions, ATTR_OPTIONS, RadioButton::customStyles);
	options.custom |= radioButtonOptions.custom;
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RadioButtonElement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_VALUE, value);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float RadioButtonElement::getRadioValue (const CreateArgs& args) const
{
	double v = 0.;

	MutableCString valueCString (this->value);
	SkinWizard::ResolvedName resolvedValue (args.wizard, valueCString);
	resolvedValue.string ().getFloatValue (v);

	return (float)v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* RadioButtonElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		IParameter* p = getParameter (args);
		float v = getRadioValue (args);

		view = NEW RadioButton (size, p, options, title, v);
	}
	return SuperClass::createView (args, view);
}

//************************************************************************************************
// ToolButtonElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ToolButtonElement, RadioButtonElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (ToolButtonElement, RadioButtonElement, TAG_TOOLBUTTON, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (ToolButtonElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (ToolButtonElement, ToolButton)
BEGIN_TYPELIB_MEMBERS (ToolButtonElement)
	ADD_TYPELIB_MEMBER (ATTR_POPUP, TypeNames::kEnum) ///< Specifies the alignment of the popup relative to the ToolButton.
	ADD_TYPELIB_MEMBER_STRING (ATTR_MODENAME) ///< modename: parameter name for the modeParam used for this toolButton
END_TYPELIB_MEMBERS (ToolButtonElement)
DEFINE_TYPELIB_ENUMERATION_WITH_PARENT (TAG_TOOLBUTTON, ATTR_POPUP, nullptr, TAG_POPUPBOX, ATTR_POPUP, SKIN_TYPELIB)
DEFINE_TYPELIB_ENUMERATION (TAG_TOOLBUTTON, ATTR_OPTIONS, ToolButton::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

ToolButtonElement::ToolButtonElement ()
: popupOptions (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToolButtonElement::setAttributes (const SkinAttributes& a)
{
	bool result = SuperClass::setAttributes (a);
	options.custom |= a.getOptions (ATTR_OPTIONS, ToolButton::customStyles);
	popupOptions = a.getOptions (ATTR_POPUP, PopupSelector::popupStyles);
	modeName = a.getString (ATTR_MODENAME);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToolButtonElement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_MODENAME, modeName);
	a.setOptions (ATTR_POPUP, popupOptions, PopupSelector::popupStyles);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToolButtonElement::appendOptions (String& string) const
{
	SkinAttributes::makeOptionsString (string, options.custom, ToolButton::customStyles);
	return SuperClass::appendOptions (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ToolButtonElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		IParameter* p = getParameter (args);
		float v = getRadioValue (args);
	
		ToolButton* tb = NEW ToolButton (size, p, options, v);
		
		if(!modeName.isEmpty ())
		{
			SkinWizard::ResolvedName resolvedModeName (args.wizard, modeName, mustResolveName ());
			if(!resolvedModeName.string ().isEmpty ())
				tb->setModeParam (getParameter (args, resolvedModeName.string (), this));
		}

		if(popupOptions)
			tb->setPopupOptions (popupOptions);
		
		view = tb;
	}
	return SuperClass::createView (args, view);
}

//************************************************************************************************
// ValueControlElement
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ValueControlElement, ControlElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (ValueControlElement, ControlElement, TAG_VALUECONTROL, SKIN_TYPELIB)
DEFINE_TYPELIB_ABSTRACT (ValueControlElement)
DEFINE_TYPELIB_DOCGROUP (ValueControlElement, DOC_GROUP_CONTROLS)
BEGIN_TYPELIB_MEMBERS (ValueControlElement)
	ADD_TYPELIB_MEMBER_STRING (ATTR_COLORNAME) ///< name of a color parameter used to colorize the hilite parts of value controls
END_TYPELIB_MEMBERS (ValueControlElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ValueControlElement::setAttributes (const SkinAttributes& a)
{
	a.getOptions (options, ATTR_OPTIONS);
	if(options.common == 0)
		options.common = Styles::kVertical;

	colorName = a.getString (ATTR_COLORNAME);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ValueControlElement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_COLORNAME, colorName);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ValueControlElement::createView (const CreateArgs& args, View* view)
{
	if(view && !colorName.isEmpty ())
	{
		SkinWizard::ResolvedName resolvedColorName (args.wizard, colorName, mustResolveName ());
		if(!resolvedColorName.string ().isEmpty ())
			if(UnknownPtr<IController> controller = args.controller)
				((ValueControl*)view)->setColorParam (controller->findParameter (resolvedColorName.string ()));
	}
	return SuperClass::createView (args, view);
}

//************************************************************************************************
// ValueBarElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ValueBarElement, ValueControlElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (ValueBarElement, ValueControlElement, TAG_VALUEBAR, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (ValueBarElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (ValueBarElement, ValueBar)
BEGIN_TYPELIB_MEMBERS (ValueBarElement)
	ADD_TYPELIB_MEMBER_STRING (ATTR_IMAGE) ///< Image with 2 frames: "normal" for the background, "normalOn" for the value bar
END_TYPELIB_MEMBERS (ValueBarElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ValueBarElement::setAttributes (const SkinAttributes& a)
{
	imageName = a.getString (ATTR_IMAGE);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ValueBarElement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_IMAGE, imageName);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ValueBarElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		IParameter* p = getParameter (args);
		view = NEW ValueBar (size, p, options);
	}
		
	if(!imageName.isEmpty ())
	{
		image = getImage (args, imageName);
		if(image)
		{

			
			AutoPtr<VisualStyle> vs = NEW VisualStyle;
			vs->setImage ("image", image);
			view->setVisualStyle (vs);
		}
	}

	return SuperClass::createView (args, view);
}

//************************************************************************************************
// ProgressBarElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ProgressBarElement, ValueBarElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (ProgressBarElement, ValueBarElement, TAG_PROGRESSBAR, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (ProgressBarElement, DOC_GROUP_CONTROLS) 
DEFINE_TYPELIB_RELATED_CLASS (ProgressBarElement, ProgressBar)

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ProgressBarElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		IParameter* p = getParameter (args);
		view = NEW ProgressBar (size, p, options);
	}

	return ValueBarElement::createView (args, view);
}

//************************************************************************************************
// ActivityIndicatorViewElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ActivityIndicatorViewElement, ProgressBarElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (ActivityIndicatorViewElement, ProgressBarElement, TAG_ACTIVITYVIEW, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (ActivityIndicatorViewElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (ActivityIndicatorViewElement, ActivityIndicatorView)

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ActivityIndicatorViewElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		view = NEW ActivityIndicatorView (size, options);
		
		if(IParameter* p = getParameter (args)) // parameter is optional
			((ActivityIndicatorView*)view)->setParameter (p);
	}

	return ProgressBarElement::createView (args, view);
}

//************************************************************************************************
// SliderElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SliderElement, ValueControlElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (SliderElement, ValueControlElement, TAG_SLIDER, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (SliderElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (SliderElement, Slider)
BEGIN_TYPELIB_MEMBERS (SliderElement)
	ADD_TYPELIB_MEMBER (ATTR_XYEDITING, TypeNames::kInt) ///< X/Y edit mode distance
	ADD_TYPELIB_MEMBER (ATTR_MODE, TypeNames::kEnum)
END_TYPELIB_MEMBERS (SliderElement)
DEFINE_TYPELIB_ENUMERATION (TAG_SLIDER, ATTR_MODE, Slider::modes, SKIN_TYPELIB)
DEFINE_TYPELIB_ENUMERATION (TAG_SLIDER, ATTR_OPTIONS, Slider::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

SliderElement::SliderElement ()
: mode (Styles::kSliderModeDefault),
  xyDistance (0),
  shouldSetAutoOrientation (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SliderElement::setAttributes (const SkinAttributes& a)
{
	StyleFlags checkFlags;
	a.getOptions (checkFlags, ATTR_OPTIONS);
	if(checkFlags.common == 0)
		shouldSetAutoOrientation = true;

	SuperClass::setAttributes (a);

	StyleFlags sliderStyle;
	a.getOptions (sliderStyle, ATTR_OPTIONS, Slider::customStyles);
	options.custom |= sliderStyle.custom;
	xyDistance = a.getInt (ATTR_XYEDITING);

	mode = a.getOptions (ATTR_MODE, Slider::modes, true, Styles::kSliderModeDefault);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SliderElement::getAttributes (SkinAttributes& a) const
{
	a.setOptions (ATTR_MODE, mode, Slider::modes, true);
	a.setInt (ATTR_XYEDITING, xyDistance);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SliderElement::appendOptions (String& string) const
{
	SkinAttributes::makeOptionsString (string, options.custom, Slider::customStyles);
	return SuperClass::appendOptions (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* SliderElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		IParameter* p = getParameter (args);
		
		if(shouldSetAutoOrientation)
		{
			if(size.getWidth () > size.getHeight ())
			{
				options.setCommonStyle (Styles::kVertical, false);
				options.setCommonStyle (Styles::kHorizontal);
			}
			else
			{
				options.setCommonStyle (Styles::kHorizontal, false);
				options.setCommonStyle (Styles::kVertical);
			}
		}
		
		if(size.getWidth () == 0)
			size.setWidth (getTheme ()->getThemeMetric (ThemeElements::kSliderHandleSize));
		if(size.getHeight () == 0)
			size.setHeight (getTheme ()->getThemeMetric (ThemeElements::kSliderHandleSize));
		
		view = NEW Slider (size, p, options);
		((Slider*)view)->setMode (mode);
		((Slider*)view)->setXYEditDistance (xyDistance);
	}
	
	return SuperClass::createView (args, view);
}
	
//************************************************************************************************
// RangeSliderElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (RangeSliderElement, SliderElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (RangeSliderElement, SliderElement, TAG_RANGESLIDER, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (RangeSliderElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (RangeSliderElement, RangeSlider)
BEGIN_TYPELIB_MEMBERS (RangeSliderElement)
	ADD_TYPELIB_MEMBER_STRING (ATTR_NAME2) ///< The name of the second range parameter
END_TYPELIB_MEMBERS (RangeSliderElement)
DEFINE_TYPELIB_ENUMERATION (TAG_RANGESLIDER, ATTR_OPTIONS, RangeSlider::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RangeSliderElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);

	StyleFlags rangeSliderStyle;
	a.getOptions (rangeSliderStyle, ATTR_OPTIONS, RangeSlider::customStyles);
	options.custom |= rangeSliderStyle.custom;
	
	paramName2 = a.getString (ATTR_NAME2);	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RangeSliderElement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_NAME2, paramName2);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RangeSliderElement::appendOptions (String& string) const
{
	SkinAttributes::makeOptionsString (string, options.custom, RangeSlider::customStyles);
	return SuperClass::appendOptions (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* RangeSliderElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		IParameter* p = getParameter (args);
		if(p && !paramName2.isEmpty ())
		{
			IParameter* p2 = getParameter (args, paramName2, this, mustResolveName ());
			
			if(shouldSetAutoOrientation)
			{
				if(size.getWidth () > size.getHeight ())
				{
					options.setCommonStyle (Styles::kVertical, false);
					options.setCommonStyle (Styles::kHorizontal);
				}
				else
				{
					options.setCommonStyle (Styles::kHorizontal, false);
					options.setCommonStyle (Styles::kVertical);
				}
			}
			
			view = NEW RangeSlider (size, p, p2, options);
			((Slider*)view)->setMode (mode);
			((Slider*)view)->setXYEditDistance (xyDistance);
		}
	}
	
	return SuperClass::createView (args, view);
}

//************************************************************************************************
// KnobElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (KnobElement, SliderElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (KnobElement, SliderElement, TAG_KNOB, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (KnobElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (KnobElement, Knob)
BEGIN_TYPELIB_MEMBERS (KnobElement)
	ADD_TYPELIB_MEMBER_STRING (ATTR_REFERENCENAME) ///< The name of the offset parameter
END_TYPELIB_MEMBERS (KnobElement)
DEFINE_TYPELIB_ENUMERATION (TAG_KNOB, ATTR_OPTIONS, Knob::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool KnobElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);

	StyleFlags knobStyle;
	a.getOptions (knobStyle, ATTR_OPTIONS, Knob::customStyles);
	referenceName = a.getString (ATTR_REFERENCENAME);
	
	options.custom |= knobStyle.custom;
	return true;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

bool KnobElement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_REFERENCENAME, referenceName);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool KnobElement::appendOptions (String& string) const
{
	SkinAttributes::makeOptionsString (string, options.custom, Knob::customStyles);
	return SuperClass::appendOptions (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* KnobElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		IParameter* p = getParameter (args);
		view = NEW Knob (size, p, options);
		
		if(p && !referenceName.isEmpty ())
		{
			if(IParameter* referenceParam = getParameter (args, referenceName, this, mustResolveName ()))
				if(referenceParam != p)
					((Knob*)view)->setOffsetReferenceParameter (referenceParam);
		}
	}
	return SliderElement::createView (args, view);
}

//************************************************************************************************
// VectorPadElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (VectorPadElement, ValueControlElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (VectorPadElement, ValueControlElement, TAG_VECTORPAD, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (VectorPadElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (VectorPadElement, VectorPad)
BEGIN_TYPELIB_MEMBERS (VectorPadElement)
	ADD_TYPELIB_MEMBER_STRING (ATTR_YNAME) ///< The name of the second (y) parameter
END_TYPELIB_MEMBERS (VectorPadElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VectorPadElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);
	yName = a.getString (ATTR_YNAME);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VectorPadElement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_YNAME, yName);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* VectorPadElement::createView (const CreateArgs& args, View* view)
{
	if(view == nullptr)
	{
		IParameter* param = getParameter (args);
		IParameter* yParam = getParameter (args, yName, this);
		view = NEW VectorPad (size, param, yParam, options);
	}
	return SuperClass::createView (args, view);
}

//************************************************************************************************
// TriVectorPadElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TriVectorPadElement, VectorPadElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (TriVectorPadElement, VectorPadElement, TAG_TRIVECTORPAD, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (TriVectorPadElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (TriVectorPadElement, TriVectorPad)
BEGIN_TYPELIB_MEMBERS (TriVectorPadElement)
	ADD_TYPELIB_MEMBER_STRING (ATTR_ZNAME) ///< The name of the third (z) parameter (triangular vector pad)
END_TYPELIB_MEMBERS (TriVectorPadElement)
DEFINE_TYPELIB_ENUMERATION (TAG_TRIVECTORPAD, ATTR_OPTIONS, TriVectorPad::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriVectorPadElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);
	options.custom |= a.getOptions (ATTR_OPTIONS, TriVectorPad::customStyles);
	zName = a.getString (ATTR_ZNAME);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriVectorPadElement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_ZNAME, zName);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriVectorPadElement::appendOptions (String& string) const
{
	SkinAttributes::makeOptionsString (string, options.custom, TriVectorPad::customStyles);
	return SuperClass::appendOptions (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* TriVectorPadElement::createView (const CreateArgs& args, View* view)
{
	if(view == nullptr)
	{
		IParameter* param = getParameter (args);
		IParameter* yParam = getParameter (args, yName, this);
		IParameter* zParam = getParameter (args, zName, this);
		view = NEW TriVectorPad (size, param, yParam, zParam, options);
	}
	return SuperClass::createView (args, view);
}

//************************************************************************************************
// TextBoxElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TextBoxElement, ControlElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (TextBoxElement, ControlElement, TAG_TEXTBOX, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (TextBoxElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (TextBoxElement, TextBox)
BEGIN_TYPELIB_MEMBERS (TextBoxElement)
	ADD_TYPELIB_MEMBER_STRING (ATTR_LABELNAME) ///< (optional) specifies a parameter that gives a label text to be appended to the parameter value (like a unit).
	ADD_TYPELIB_MEMBER_STRING (ATTR_COLORNAME) ///< name of a color parameter that sets the textcolor or helps switching the text color depending on the background luminance
	ADD_TYPELIB_MEMBER (ATTR_TEXTTRIMMODE, TypeNames::kEnum) ///< how the text should be abbreviated when it doesn't fit in the view
END_TYPELIB_MEMBERS (TextBoxElement)
DEFINE_TYPELIB_ENUMERATION (TAG_TEXTBOX, ATTR_OPTIONS, TextBox::customStyles, SKIN_TYPELIB)
DEFINE_TYPELIB_ENUMERATION (TAG_TEXTBOX, ATTR_TEXTTRIMMODE, FontElement::textTrimModes, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

TextBoxElement::TextBoxElement ()
: textTrimMode (Font::kTrimModeDefault)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextBoxElement::setAttributes (const SkinAttributes& a)
{
	a.getOptions (options, ATTR_OPTIONS, TextBox::customStyles);
	labelName = a.getString (ATTR_LABELNAME);
	colorName = a.getString (ATTR_COLORNAME);
	textTrimMode = a.getOptions (ATTR_TEXTTRIMMODE, FontElement::textTrimModes, true, Font::kTrimModeDefault);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextBoxElement::getAttributes (SkinAttributes& a) const
{
	SuperClass::getAttributes (a);
	a.setString (ATTR_LABELNAME, labelName);
	a.setString (ATTR_COLORNAME, colorName);
	a.setOptions (ATTR_TEXTTRIMMODE, textTrimMode, FontElement::textTrimModes, true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextBoxElement::appendOptions (String& string) const
{
	SkinAttributes::makeOptionsString (string, options.custom, TextBox::customStyles);
	return SuperClass::appendOptions (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect TextBoxElement::getTextBoxSize () const
{
	Rect r (size);
	if(r.getHeight () == 0 && !options.isCustomStyle (Styles::kTextBoxAppearanceMultiLine))
		r.setHeight (getTheme ()->getThemeMetric (ThemeElements::kTextBoxHeight));
	return r;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* TextBoxElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		IParameter* p = getParameter (args);
		view = NEW TextBox (getTextBoxSize (), p, options, title);
	}

	if(TextBox* textBox = ccl_cast<TextBox> (view))
	{
		textBox->setTextTrimMode (textTrimMode);

		if(!labelName.isEmpty ())
			textBox->setLabelParam (getParameter (args, labelName, this));
		if(!colorName.isEmpty ())
	 		textBox->setColorParam (getParameter (args, colorName, this));
	}

	return SuperClass::createView (args, view);
}

//************************************************************************************************
// EditBoxElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (EditBoxElement, TextBoxElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (EditBoxElement, TextBoxElement, TAG_EDITBOX, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (EditBoxElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (EditBoxElement, EditBox)
BEGIN_TYPELIB_MEMBERS (EditBoxElement)
	ADD_TYPELIB_MEMBER (ATTR_RETURNKEY_TYPE, TypeNames::kEnum)
	ADD_TYPELIB_MEMBER (ATTR_KEYBOARD_TYPE, TypeNames::kEnum)
	ADD_TYPELIB_MEMBER_STRING (ATTR_PLACEHOLDER) ///< optional placeholder string that is displayed with textcolor.placeholder when editString is empty
END_TYPELIB_MEMBERS (EditBoxElement)
DEFINE_TYPELIB_ENUMERATION (TAG_EDITBOX, ATTR_RETURNKEY_TYPE, NativeTextControl::returnKeyTypes, SKIN_TYPELIB)
DEFINE_TYPELIB_ENUMERATION (TAG_EDITBOX, ATTR_KEYBOARD_TYPE, NativeTextControl::keyboardTypes, SKIN_TYPELIB)
DEFINE_TYPELIB_ENUMERATION (TAG_EDITBOX, ATTR_OPTIONS, EditBox::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditBoxElement::EditBoxElement ()
: returnKeyType (Styles::kReturnKeyDefault),
  keyboardType (Styles::kKeyboardTypeAutomatic),
  autofillType (Styles::kAutofillTypeNone)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBoxElement::setAttributes (const SkinAttributes& a)
{
	bool result = SuperClass::setAttributes (a);
	options.custom |= a.getOptions (ATTR_OPTIONS, EditBox::customStyles);
	returnKeyType = a.getOptions (ATTR_RETURNKEY_TYPE, NativeTextControl::returnKeyTypes, true);
	keyboardType = a.getOptions (ATTR_KEYBOARD_TYPE, NativeTextControl::keyboardTypes, true);

	autofillTypeString = a.getString (ATTR_AUTOFILL);
	if(autofillTypeString.startsWith (SkinVariable::prefix))
		mustResolveAutofillType (true);
	else
		autofillType = a.getOptions (ATTR_AUTOFILL, IAutofillClient::types, true);

	placeholder = a.getString (ATTR_PLACEHOLDER);
	if(a.getBool (ATTR_LOCALIZE, true))
		placeholder = translate (placeholder);

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBoxElement::getAttributes (SkinAttributes& a) const
{
	SuperClass::getAttributes (a);
	a.setOptions (ATTR_RETURNKEY_TYPE, returnKeyType, NativeTextControl::returnKeyTypes, true);
	a.setOptions (ATTR_KEYBOARD_TYPE, keyboardType, NativeTextControl::keyboardTypes, true);
	a.setOptions (ATTR_AUTOFILL, autofillType, IAutofillClient::types, true);
	a.setString (ATTR_PLACEHOLDER, placeholder);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBoxElement::appendOptions (String& string) const
{
	SkinAttributes::makeOptionsString (string, options.custom, EditBox::customStyles);
	return SuperClass::appendOptions (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* EditBoxElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		IParameter* p = getParameter (args);
		view = NEW EditBox (getTextBoxSize (), p, options, title);
	}
	
	if(EditBox* editBox = ccl_cast<EditBox> (view))
	{
		editBox->setKeyboardType (keyboardType);
		editBox->setReturnKeyType (returnKeyType);

		if(mustResolveAutofillType ())
		{
			String resolvedAutofillTypeString = args.wizard.resolveTitle (autofillTypeString);
			if(!resolvedAutofillTypeString.getIntValue (autofillType))
				autofillType = SkinAttributes::parseOptions (resolvedAutofillTypeString, IAutofillClient::types, true);
		}
		editBox->setAutofillType (autofillType);

		if(!placeholder.isEmpty ())
			editBox->setPlaceholderString (placeholder);
	}

	return TextBoxElement::createView (args, view);
}

//************************************************************************************************
// TextEditorElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TextEditorElement, EditBoxElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (TextEditorElement, EditBoxElement, TAG_TEXTEDITOR, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (TextEditorElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (TextEditorElement, TextEditor)

//////////////////////////////////////////////////////////////////////////////////////////////////

TextEditorElement::TextEditorElement ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextEditorElement::setAttributes (const SkinAttributes& a)
{
	bool result = SuperClass::setAttributes (a);
	horizontalScrollBarStyle = a.getString (ATTR_HSCROLLSTYLE);
	verticalScrollBarStyle = a.getString (ATTR_VSCROLLSTYLE);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextEditorElement::getAttributes (SkinAttributes& a) const
{
	if(!horizontalScrollBarStyle.isEmpty ())
		a.setString (ATTR_HSCROLLSTYLE, horizontalScrollBarStyle);
	if(!verticalScrollBarStyle.isEmpty ())
		a.setString (ATTR_VSCROLLSTYLE, verticalScrollBarStyle);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextEditorElement::appendOptions (String& string) const
{
	return SuperClass::appendOptions (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* TextEditorElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		IParameter* p = getParameter (args);
		TextEditor* editor = NEW TextEditor (size, p, options, title);
		
		// individual scrollbar styles
		if(!horizontalScrollBarStyle.isEmpty ())
			if(VisualStyle* visualStyle = args.wizard.getModel ().getStyle (horizontalScrollBarStyle, this))
				editor->setHScrollBarStyle (visualStyle);
		
		if(!verticalScrollBarStyle.isEmpty ())
			if(VisualStyle* visualStyle = args.wizard.getModel ().getStyle (verticalScrollBarStyle, this))
				editor->setVScrollBarStyle (visualStyle);
		
		view = editor;
	}
	return ControlElement::createView (args, view);
}

//************************************************************************************************
// ValueBoxElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ValueBoxElement, EditBoxElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (ValueBoxElement, EditBoxElement, TAG_VALUEBOX, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (ValueBoxElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (ValueBoxElement, ValueBox)
BEGIN_TYPELIB_MEMBERS (ValueBoxElement)
	ADD_TYPELIB_MEMBER (ATTR_XYEDITING, TypeNames::kInt) ///< X/Y edit mode distance
END_TYPELIB_MEMBERS (ValueBoxElement)
DEFINE_TYPELIB_ENUMERATION (TAG_VALUEBOX, ATTR_OPTIONS, ValueBox::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

ValueBoxElement::ValueBoxElement ()
: xyDistance (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
bool ValueBoxElement::setAttributes (const SkinAttributes& a)
{
	bool result = SuperClass::setAttributes (a);
	options.custom |= a.getOptions (ATTR_OPTIONS, ValueBox::customStyles);
	xyDistance = a.getInt (ATTR_XYEDITING);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ValueBoxElement::getAttributes (SkinAttributes& a) const
{
	SuperClass::getAttributes (a);
	a.setInt (ATTR_XYEDITING, xyDistance);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ValueBoxElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		IParameter* p = getParameter (args);
		view = NEW ValueBox (getTextBoxSize (), p, options);
		((ValueBox*)view)->setXYEditDistance (xyDistance);
	}
	return SuperClass::createView (args, view);
}

//************************************************************************************************
// SelectBoxElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SelectBoxElement, TextBoxElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (SelectBoxElement, TextBoxElement, TAG_SELECTBOX, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (SelectBoxElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (SelectBoxElement, SelectBox)
BEGIN_TYPELIB_MEMBERS (SelectBoxElement)
	ADD_TYPELIB_MEMBER (ATTR_POPUP, TypeNames::kEnum) ///< Specifies the alignment of the popup menu relative to the SelectBox.
	ADD_TYPELIB_MEMBER_STRING (ATTR_POPUPSTYLE) ///< Name of a Style that is applied to the popup menu
END_TYPELIB_MEMBERS (SelectBoxElement)
DEFINE_TYPELIB_ENUMERATION_WITH_PARENT (TAG_SELECTBOX, ATTR_POPUP, nullptr, TAG_POPUPBOX, ATTR_POPUP, SKIN_TYPELIB)
DEFINE_TYPELIB_ENUMERATION (TAG_SELECTBOX, ATTR_OPTIONS, SelectBox::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

SelectBoxElement::SelectBoxElement ()
: popupOptions (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectBoxElement::setAttributes (const SkinAttributes& a)
{
	bool result = SuperClass::setAttributes (a);
	options.custom |= a.getOptions (ATTR_OPTIONS, SelectBox::customStyles);
	popupOptions = a.getOptions (ATTR_POPUP, PopupSelector::popupStyles);
	popupStyleName = a.getString (ATTR_POPUPSTYLE);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectBoxElement::getAttributes (SkinAttributes& a) const
{
	TextBoxElement::getAttributes (a);
	a.setOptions (ATTR_POPUP, popupOptions, PopupSelector::popupStyles);
	a.setString (ATTR_POPUPSTYLE, popupStyleName);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectBoxElement::appendOptions (String& string) const
{
	SkinAttributes::makeOptionsString (string, options.custom, SelectBox::customStyles);
	return SuperClass::appendOptions (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* SelectBoxElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		IParameter* p = getParameter (args);
		view = NEW SelectBox (getTextBoxSize (), p, options, title);
	}

	if(popupOptions)
		((SelectBox*)view)->setPopupOptions (popupOptions);

	if(!popupStyleName.isEmpty ())
	{
		SkinWizard::ResolvedName resolvedName (args.wizard, popupStyleName);
		CString resolvedPopupStyle = resolvedName.string ();
		if(VisualStyle* popupStyle = args.wizard.getModel ().getStyle (resolvedPopupStyle, this))
			((SelectBox*)view)->setPopupVisualStyle (popupStyle);
	}
	
	return TextBoxElement::createView (args, view);
}

//************************************************************************************************
// ComboBoxElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ComboBoxElement, SelectBoxElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (ComboBoxElement, SelectBoxElement, TAG_COMBOBOX, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (ComboBoxElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (ComboBoxElement, ComboBox)
BEGIN_TYPELIB_MEMBERS (ComboBoxElement)
	ADD_TYPELIB_MEMBER_STRING (ATTR_EDITNAME) ///< Specifies the parameter, that is used for the edit field of the combo box. The controller is responsible for updating the menu part of the combo box.
	ADD_TYPELIB_MEMBER (ATTR_RETURNKEY_TYPE, TypeNames::kEnum)
	ADD_TYPELIB_MEMBER (ATTR_KEYBOARD_TYPE, TypeNames::kEnum)
END_TYPELIB_MEMBERS (ComboBoxElement)
DEFINE_TYPELIB_ENUMERATION_WITH_PARENT (TAG_COMBOBOX, ATTR_RETURNKEY_TYPE, nullptr, TAG_EDITBOX, ATTR_RETURNKEY_TYPE, SKIN_TYPELIB)
DEFINE_TYPELIB_ENUMERATION (TAG_COMBOBOX, ATTR_KEYBOARD_TYPE, NativeTextControl::keyboardTypes, SKIN_TYPELIB)
DEFINE_TYPELIB_ENUMERATION (TAG_COMBOBOX, ATTR_OPTIONS, ComboBox::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

ComboBoxElement::ComboBoxElement ()
: returnKeyType (Styles::kReturnKeyDefault),
  keyboardType (Styles::kKeyboardTypeAutomatic)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ComboBoxElement::setAttributes (const SkinAttributes& a)
{
	SelectBoxElement::setAttributes (a);

	editName = a.getString (ATTR_EDITNAME);
	
	options.custom |= a.getOptions (ATTR_OPTIONS, ComboBox::customStyles);
	returnKeyType = a.getOptions (ATTR_RETURNKEY_TYPE, NativeTextControl::returnKeyTypes, true);
	keyboardType = a.getOptions (ATTR_KEYBOARD_TYPE, NativeTextControl::keyboardTypes, true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ComboBoxElement::getAttributes (SkinAttributes& a) const
{
	SelectBoxElement::getAttributes (a);

	a.setString (ATTR_EDITNAME, editName);
	a.setOptions (ATTR_RETURNKEY_TYPE, returnKeyType, NativeTextControl::returnKeyTypes, true);
	a.setOptions (ATTR_KEYBOARD_TYPE, keyboardType, NativeTextControl::keyboardTypes, true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ComboBoxElement::appendOptions (String& string) const
{
	SkinAttributes::makeOptionsString (string, options.custom, ComboBox::customStyles);
	return SuperClass::appendOptions (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ComboBoxElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		IParameter* selectParam = getParameter (args);
		IParameter* editParam = !editName.isEmpty () ? getParameter (args, editName, this) : nullptr;
		view = NEW ComboBox (getTextBoxSize (), selectParam, editParam, options, title);
		((ComboBox*)view)->setReturnKeyType (returnKeyType);
		((ComboBox*)view)->setKeyboardType (keyboardType);
	}
	return SelectBoxElement::createView (args, view);
}

//************************************************************************************************
// SelectBoxElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SegmentBoxElement, ControlElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (SegmentBoxElement, ControlElement, TAG_SEGMENTBOX, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (SegmentBoxElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (SegmentBoxElement, SegmentBox)
DEFINE_TYPELIB_ENUMERATION (TAG_SEGMENTBOX, ATTR_OPTIONS, SegmentBox::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SegmentBoxElement::setAttributes (const SkinAttributes& a)
{
	a.getOptions (options, ATTR_OPTIONS);
	
	StyleFlags boxStyle;
	a.getOptions (boxStyle, ATTR_OPTIONS, SegmentBox::customStyles);
	options.custom |= boxStyle.custom;

	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SegmentBoxElement::appendOptions (String& string) const
{
	SkinAttributes::makeOptionsString (string, options.custom, SegmentBox::customStyles);
	return SuperClass::appendOptions (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SegmentBoxElement::getAttributes (SkinAttributes& a) const
{
	a.setOptions (ATTR_OPTIONS, options);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* SegmentBoxElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		IParameter* p = getParameter (args);
		view = NEW SegmentBox (size, p, options);
	}
	return SuperClass::createView (args, view);
}

//************************************************************************************************
// ColorBoxElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ColorBoxElement, ControlElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (ColorBoxElement, ControlElement, TAG_COLORBOX, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (ColorBoxElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (ColorBoxElement, ColorBox)
BEGIN_TYPELIB_MEMBERS (ColorBoxElement)
	ADD_TYPELIB_MEMBER (ATTR_RADIUS, TypeNames::kInt) ///< For a color box with rounded courners, set the radius attribute to the desired value.
	ADD_TYPELIB_MEMBER_STRING (ATTR_SELECTNAME) ///< This attribute is the name of an optional parameter, that can be triggered, when the user clicks on the color box.
END_TYPELIB_MEMBERS (ColorBoxElement)
DEFINE_TYPELIB_ENUMERATION (TAG_COLORBOX, ATTR_OPTIONS, ColorBox::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorBoxElement::ColorBoxElement ()
: radius (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorBoxElement::setAttributes (const SkinAttributes& a)
{
	a.getOptions (options, ATTR_OPTIONS, ColorBox::customStyles);
	radius = a.getInt (ATTR_RADIUS);
	selectName = a.getString (ATTR_SELECTNAME);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorBoxElement::getAttributes (SkinAttributes& a) const
{
	a.setOptions (ATTR_OPTIONS, options, ColorBox::customStyles);
	a.setInt (ATTR_RADIUS, radius);
	a.setString (ATTR_SELECTNAME, selectName);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ColorBoxElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		IParameter* p = getParameter (args);
		view = NEW ColorBox (size, p, options);
		((ColorBox*)view)->setRadius (radius);

		if(!selectName.isEmpty ())
		{
			IParameter* selectParam = getParameter (args, selectName, this);
			((ColorBox*)view)->setSelectParam (selectParam);
		}
	}
	return SuperClass::createView (args, view);
}

//************************************************************************************************
// UpDownButtonElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (UpDownButtonElement, ButtonElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (UpDownButtonElement, ButtonElement, TAG_UPDOWNBUTTON, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (UpDownButtonElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (UpDownButtonElement, UpDownButton)
DEFINE_TYPELIB_ENUMERATION (TAG_UPDOWNBUTTON, ATTR_OPTIONS, UpDownButton::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UpDownButtonElement::setAttributes (const SkinAttributes& a)
{
	bool result = SuperClass::setAttributes (a);
	StyleFlags upDownOptions;
	a.getOptions (upDownOptions, ATTR_OPTIONS, UpDownButton::customStyles);
	options.custom |= upDownOptions.custom;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* UpDownButtonElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
		view = NEW UpDownButton (size, getParameter (args), options);

	return SuperClass::createView (args, view);
}

//************************************************************************************************
// UpDownBoxElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (UpDownBoxElement, ControlElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (UpDownBoxElement, ControlElement, TAG_UPDOWNBOX, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (UpDownBoxElement, DOC_GROUP_CONTROLS) 
DEFINE_TYPELIB_RELATED_CLASS (UpDownBoxElement, UpDownBox)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UpDownBoxElement::setAttributes (const SkinAttributes& a)
{
	bool result = SuperClass::setAttributes (a);
	a.getOptions (options, ATTR_OPTIONS);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* UpDownBoxElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		IParameter* p = getParameter (args);
		view = NEW UpDownBox (size, p, options);
	}
	return SuperClass::createView (args, view);
}

//************************************************************************************************
// SwipeBoxElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SwipeBoxElement, ControlElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (SwipeBoxElement, ControlElement, TAG_SWIPEBOX, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (SwipeBoxElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (SwipeBoxElement, SwipeBox)
BEGIN_TYPELIB_MEMBERS (SwipeBoxElement)
	ADD_TYPELIB_MEMBER_STRING (ATTR_TARGET) ///< (optional) Name of a target Contol class. Only controls of the given class will be used when swiping
END_TYPELIB_MEMBERS (SwipeBoxElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SwipeBoxElement::setAttributes (const SkinAttributes& a)
{
	targetClass = a.getString (ATTR_TARGET);
	a.getOptions (options, ATTR_OPTIONS, SwipeBox::customStyles);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SwipeBoxElement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_TARGET, targetClass);
	a.setOptions (ATTR_OPTIONS, options, SwipeBox::customStyles);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* SwipeBoxElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		IParameter* p = getParameter (args);
		view = NEW SwipeBox (size, targetClass, p, options);
	}
	return SuperClass::createView (args, view);
}

//************************************************************************************************
// DividerElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DividerElement, ControlElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (DividerElement, ControlElement, TAG_DIVIDER, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (DividerElement, DOC_GROUP_LAYOUT)
DEFINE_TYPELIB_RELATED_CLASS (DividerElement, Divider)
BEGIN_TYPELIB_MEMBERS (DividerElement)
	ADD_TYPELIB_MEMBER_STRING (ATTR_IMAGE) ///< name of a background image
	ADD_TYPELIB_MEMBER (ATTR_OUTREACH, TypeNames::kInt) ///< the divider area is extended by this number of pixels outside the divider. Only works in a layout container.
END_TYPELIB_MEMBERS (DividerElement)
BEGIN_TYPELIB_ATTRIBUTES (DividerElement)
	ADD_TYPELIB_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_VIEWSSTATEMENTS)
	ADD_TYPELIB_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_FRAMECHILDREN)
END_TYPELIB_ATTRIBUTES (DividerElement)
DEFINE_TYPELIB_ENUMERATION (TAG_DIVIDER, ATTR_OPTIONS, Divider::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

DividerElement::DividerElement ()
: outreach (-1)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DividerElement::setAttributes (const SkinAttributes& a)
{
	a.getOptions (options, ATTR_OPTIONS, Divider::customStyles);
	imageName = a.getString (ATTR_IMAGE);
	outreach = a.getInt (ATTR_OUTREACH, -1);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DividerElement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_IMAGE, imageName);
	a.setOptions (ATTR_OPTIONS, options, Divider::customStyles);
	a.setInt (ATTR_OUTREACH, outreach);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* DividerElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		IParameter* p = getParameter (args);

		Rect r (size);
		if(r.isEmpty ())
		{
			Coord ds = getTheme ()->getThemeMetric (ThemeElements::kDividerSize);
			if(r.getWidth () <= 0)
				r.setWidth (r.getWidth () == 0 ? ds : 0);
			if(r.getHeight () <= 0)
				r.setHeight (r.getHeight () == 0 ? ds : 0);
		}

		if(!options.isHorizontal () && !options.isVertical ())
			if(AnchorLayoutElement* parentLayout = ccl_cast<AnchorLayoutElement> (getParent ()))
			{
				if(parentLayout->getOptions ().isHorizontal ())
					options.common |= Styles::kHorizontal;
				else
					options.common |= Styles::kVertical;
			}

		view = NEW Divider (r, p, options);

		if(!imageName.isEmpty ())
		{
			// assign background image
			Image* image = getImage (args, imageName);
			if(image)
			{
				AutoPtr<VisualStyle> vs = NEW VisualStyle;
				vs->setImage (StyleID::kBackground, image);
				view->setVisualStyle (vs);
			}
		}
	}
	if(outreach >= 0)
		((Divider*)view)->setOutreach (outreach);

	return SuperClass::createView (args, view);
}

//************************************************************************************************
// AlignViewElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AlignViewElement, ControlElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (AlignViewElement, ControlElement, TAG_ALIGNVIEW, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (AlignViewElement, DOC_GROUP_LAYOUT)
DEFINE_TYPELIB_RELATED_CLASS (AlignViewElement, AlignView)
BEGIN_TYPELIB_MEMBERS (AlignViewElement)
	ADD_TYPELIB_MEMBER_STRING (ATTR_PERSISTENCE_ID) ///< storage id used to store and restore the selected alignment
END_TYPELIB_MEMBERS (AlignViewElement)
DEFINE_TYPELIB_ENUMERATION (TAG_ALIGNVIEW, ATTR_OPTIONS, AlignView::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AlignViewElement::setAttributes (const SkinAttributes& a)
{
	persistenceID = a.getString (ATTR_PERSISTENCE_ID);
	a.getOptions (options, ATTR_OPTIONS, AlignView::customStyles);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AlignViewElement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_PERSISTENCE_ID, persistenceID);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AlignViewElement::appendOptions (String& string) const
{
	SkinAttributes::makeOptionsString (string, options.custom, AlignView::customStyles);
	return SuperClass::appendOptions (string);
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

View* AlignViewElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		IParameter* p = getParameter (args);

		view = NEW AlignView (size, p, options);
		((AlignView*)view)->setPersistenceID (persistenceID);
	}
	return SuperClass::createView (args, view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlignViewElement::viewCreated (View* view)
{
	((AlignView*)view)->restoreState ();

	SuperClass::viewCreated (view);
}

//************************************************************************************************
// ScrollBarElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ScrollBarElement, ControlElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (ScrollBarElement, ControlElement, TAG_SCROLLBAR, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (ScrollBarElement, DOC_GROUP_CONTROLS) 
DEFINE_TYPELIB_RELATED_CLASS (ScrollBarElement, ScrollBar)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollBarElement::setAttributes (const SkinAttributes& a)
{
	a.getOptions (options, ATTR_OPTIONS, ScrollBar::customStyles);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollBarElement::getAttributes (SkinAttributes& a) const
{
	a.setOptions (ATTR_OPTIONS, options, ScrollBar::customStyles);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ScrollBarElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		IParameter* p = getParameter (args);
		view = NEW ScrollBar (size, p, options);
	}

	return SuperClass::createView (args, view);
}

//************************************************************************************************
// ScrollButtonElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ScrollButtonElement, ScrollBarElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (ScrollButtonElement, ScrollBarElement, TAG_SCROLLBUTTON, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (ScrollButtonElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (ScrollButtonElement, ScrollButton)
BEGIN_TYPELIB_MEMBERS (ScrollButtonElement)
	ADD_TYPELIB_MEMBER (ATTR_PART, TypeNames::kEnum)
END_TYPELIB_MEMBERS (ScrollButtonElement)
DEFINE_TYPELIB_ENUMERATION (TAG_SCROLLBUTTON, ATTR_PART, ScrollBar::partNames, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

ScrollButtonElement::ScrollButtonElement ()
: partCode (ScrollBar::kPartNone)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollButtonElement::setAttributes (const SkinAttributes& a)
{
	partCode = a.getOptions (ATTR_PART, ScrollBar::partNames, true);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ScrollButtonElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		IParameter* p = getParameter (args);
		view = NEW ScrollButton (size, p, partCode);
	}

	return SuperClass::createView (args, view);
}

//************************************************************************************************
// PageControlElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PageControlElement, ScrollBarElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (PageControlElement, ScrollBarElement, TAG_PAGECONTROL, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (PageControlElement, DOC_GROUP_CONTROLS) 
DEFINE_TYPELIB_RELATED_CLASS (PageControlElement, PageControl)

//////////////////////////////////////////////////////////////////////////////////////////////////

View* PageControlElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		IParameter* p = getParameter (args);
		view = NEW PageControl (size, p, options);
	}
	return SuperClass::createView (args, view);
}

//************************************************************************************************
// ScrollPickerElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ScrollPickerElement, ControlElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (ScrollPickerElement, ControlElement, TAG_SCROLLPICKER, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (ScrollPickerElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (ScrollPickerElement, ScrollPicker)
BEGIN_TYPELIB_MEMBERS (ScrollPickerElement)
	ADD_TYPELIB_MEMBER_STRING (ATTR_APPLYNAME) ///< parameter name for an optional, transparent apply button in the center
END_TYPELIB_MEMBERS (ScrollPickerElement)
DEFINE_TYPELIB_ENUMERATION (TAG_SCROLLPICKER, ATTR_OPTIONS, ScrollPicker::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollPickerElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);
	applyName = a.getString (ATTR_APPLYNAME);
	
	a.getOptions (options, ATTR_OPTIONS, ScrollPicker::customStyles);
	if(options.common == 0)
		options.common = Styles::kVertical;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollPickerElement::getAttributes (SkinAttributes& a) const
{
	a.setOptions (ATTR_OPTIONS, options, ScrollPicker::customStyles);
	a.setString (ATTR_APPLYNAME, applyName);

	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ScrollPickerElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		if(IParameter* p = getParameter (args))
		{
			view = NEW ScrollPicker (size, p, options);
			
			if(p && !applyName.isEmpty ())
			{
				if(IParameter* applyParameter = getParameter (args, applyName, this))
					if(applyParameter != p)
						((ScrollPicker*)view)->setApplyParameter (applyParameter);
			}
		}
	}
	
	return SuperClass::createView (args, view);
}

//************************************************************************************************
// TabViewElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TabViewElement, ControlElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (TabViewElement, ControlElement, TAG_TABVIEW, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (TabViewElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (TabViewElement, TabView)
BEGIN_TYPELIB_MEMBERS (TabViewElement)
	ADD_TYPELIB_MEMBER_STRING (ATTR_PERSISTENCE_ID) ///< A storage id used to store and restore the index of the current tab
END_TYPELIB_MEMBERS (TabViewElement)
DEFINE_TYPELIB_ENUMERATION (TAG_TABVIEW, ATTR_OPTIONS, TabView::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TabViewElement::setAttributes (const SkinAttributes& a)
{
	a.getOptions (options, ATTR_OPTIONS, TabView::customStyles);
	persistenceID = a.getString (ATTR_PERSISTENCE_ID);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TabViewElement::appendOptions (String& string) const
{
	SkinAttributes::makeOptionsString (string, options.custom, TabView::customStyles);
	return SuperClass::appendOptions (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TabViewElement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_PERSISTENCE_ID, persistenceID);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* TabViewElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		view = NEW TabView (size, getParameter (args), options);
		((TabView*)view)->setPersistenceID (persistenceID);
	}
	return SuperClass::createView (args, view);
}

} // namespace SkinElements
} // CCL
