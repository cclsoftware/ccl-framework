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
// Filename    : ccl/gui/skin/skinexpression.cpp
// Description : Skin Expression Parser
//
//************************************************************************************************

#include "ccl/gui/skin/skinexpression.h"
#include "ccl/gui/skin/skinwizard.h"

using namespace CCL;

//************************************************************************************************
// SkinVariableResolver
//************************************************************************************************

class SkinVariableResolver: public Unknown,
							public ExpressionParser::IVariableResolver
{
public:
	SkinVariableResolver (const SkinWizard& wizard)
	: wizard (wizard)
	{}
	
	// ExpressionParser::IVariableResolver
	tbool CCL_API getValue (Variant& value, StringID identifier) const override
	{
		// (SkinWizard could directly implement IVariableResolver)
		if(const SkinVariable* variable = wizard.getVariable (identifier))
		{
			value = variable->getValue ();
			return true;
		}
		return false;
	}

	CLASS_INTERFACE (ExpressionParser::IVariableResolver, Unknown)

private:
	const SkinWizard& wizard;
};

//************************************************************************************************
// SkinExpressionParser
//************************************************************************************************

bool SkinExpressionParser::evaluate (Variant& value, StringRef expression, const ExpressionParser::IVariableResolver& resolver)
{
	return ExpressionParser::evaluate (value, expression, resolver);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinExpressionParser::evaluate (Variant& value, StringRef expression, const SkinWizard& wizard)
{
	SkinVariableResolver resolver (wizard);
	return evaluate (value, expression, resolver);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinExpressionParser::evaluate (Variant& value, StringRef expression, const IAttributeList& variables)
{
	return ExpressionParser::evaluate (value, expression, variables);
}
