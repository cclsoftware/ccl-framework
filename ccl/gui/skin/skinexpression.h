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
// Filename    : ccl/gui/skin/skinexpression.h
// Description : Skin Expression Parser
//
//************************************************************************************************

#ifndef _ccl_skinexpression_h
#define _ccl_skinexpression_h

#include "ccl/base/storage/expressionparser.h"

namespace CCL {

class SkinWizard;

//************************************************************************************************
// SkinExpressionParser
//************************************************************************************************

class SkinExpressionParser
{
public:
	static bool evaluate (Variant& value, StringRef expression, const ExpressionParser::IVariableResolver& resolver);
	static bool evaluate (Variant& value, StringRef expression, const SkinWizard& wizard);
	static bool evaluate (Variant& value, StringRef expression, const IAttributeList& variables);
};

} // namespace CCL

#endif // _ccl_skinexpression_h
