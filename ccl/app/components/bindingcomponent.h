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
// Filename    : ccl/app/components/bindingcomponent.h
// Description : Parameter Binding Component
//
//************************************************************************************************

#ifndef _ccl_bindingcomponent_h
#define _ccl_bindingcomponent_h

#include "ccl/app/component.h"

#include "ccl/public/gui/iparamprovider.h"

namespace CCL {

//************************************************************************************************
// ParameterBindingComponent
//************************************************************************************************

class ParameterBindingComponent: public Component
{
public:
	DECLARE_CLASS_ABSTRACT (ParameterBindingComponent, Component)

	ParameterBindingComponent (IParameterProvider& provider, StringRef name = nullptr);

	// Component
	tbool CCL_API paramChanged (IParameter* param) override;

protected:
	SharedPtr<IParameterProvider> provider;

	void syncParamValues ();
};

} // namespace CCL

#endif // _ccl_bindingcomponent_h
