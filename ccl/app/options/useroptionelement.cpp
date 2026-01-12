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
// Filename    : ccl/app/options/useroptionelement.cpp
// Description : User Option Element
//
//************************************************************************************************

#include "ccl/app/options/useroptionelement.h"

#include "ccl/app/params.h"

#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//************************************************************************************************
// UserOptionElement
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (UserOptionElement, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

UserOptionElement::UserOptionElement (IParameter* editParam)
: editParam (editParam)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserOptionElement::setLabel (StringRef label)
{
	if(labelParam == nullptr)
	{
		MutableCString name;
		if(editParam)
			name = editParam->getName ();
		ASSERT (!name.isEmpty ())
		name += ".label";
		labelParam = NEW StringParam (name);
	}

	labelParam->fromString (label);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserOptionElement::getEditValue (Variant& value) const
{
	ASSERT (editParam != nullptr)
	if(editParam)
		value = editParam->getValue ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserOptionElement::setEditValue (VariantRef value)
{
	ASSERT (editParam != nullptr)
	if(editParam)
		editParam->setValue (value);
}

//************************************************************************************************
// ConfigurationElement
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ConfigurationElement, UserOptionElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

ConfigurationElement::ConfigurationElement (StringID section, StringID key, IParameter* editParam)
: UserOptionElement (editParam),
  section (section),
  key (key),
  needsRedraw (false),
  applyCallback (nullptr)
{
	if(editParam && editParam->getName ().isEmpty ())
	{
		MutableCString editName (section);
		editName += ".";
		editName += key;
		editParam->setName (editName);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Configuration::IRegistry& ConfigurationElement::getRegistry () const
{
	return Configuration::Registry::instance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ConfigurationElement::getCurrentValue (Variant& value) const
{
	if(!getRegistry ().getValue (value, section, key))
	{
		if(editParam)
			value = editParam->getDefaultValue ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ConfigurationElement::setCurrentValue (VariantRef value)
{
	getRegistry ().setValue (section, key, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ConfigurationElement::init ()
{
	Variant value;
	getCurrentValue (value);
	setEditValue (value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ConfigurationElement::needsApply () const
{
	Variant currentValue;
	getCurrentValue (currentValue);

	Variant editValue;
	getEditValue (editValue);

	return editValue != currentValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ConfigurationElement::apply ()
{
	Variant editValue;
	getEditValue (editValue);
	setCurrentValue (editValue);

	if(applyCallback)
		(*applyCallback) ();

	if(needsRedraw)
		System::GetDesktop ().redrawAll ();
}

//************************************************************************************************
// FrameworkOptionElement
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (FrameworkOptionElement, ConfigurationElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameworkOptionElement::FrameworkOptionElement (StringID section, StringID key, IParameter* editParam)
: ConfigurationElement (section, key, editParam)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Configuration::IRegistry& FrameworkOptionElement::getRegistry () const
{
	return System::GetFrameworkConfiguration ();
}
