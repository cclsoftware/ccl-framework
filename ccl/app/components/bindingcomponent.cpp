//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/app/components/bindingcomponent.cpp
// Description : Parameter Binding Component
//
//************************************************************************************************

#include "ccl/app/components/bindingcomponent.h"

#include "ccl/public/gui/iparameter.h"

using namespace CCL;

//************************************************************************************************
// ParameterBindingComponent
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ParameterBindingComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

ParameterBindingComponent::ParameterBindingComponent (IParameterProvider& _provider, StringRef name)
: Component (name),
  provider (&_provider)
{
	paramList.addParameters (_provider);

	// mark parameters from provider public so a derived component can add additional ones
	for(int i = 0; i < paramList.count (); i++)
		paramList.at (i)->setPublic (true);

	syncParamValues ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParameterBindingComponent::syncParamValues ()
{
	for(int i = 0; i < paramList.count (); i++)
	{
		IParameter* p = paramList.at (i);
		if(p->isPublic ())
		{
			Variant value;
			if(provider->getParameterValue (value, p->getTag ()) == kResultOk)
				p->setValue (value);
		}		
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ParameterBindingComponent::paramChanged (IParameter* param)
{
	if(param->isPublic ())
		provider->setParameterValue (param->getTag (), param->getValue ());

	return true;
}
