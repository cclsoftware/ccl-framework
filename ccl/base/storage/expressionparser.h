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
// Filename    : ccl/base/storage/expressionparser.h
// Description : Expression parser
//
//************************************************************************************************

#ifndef _ccl_expressionparser_h
#define _ccl_expressionparser_h

#include "ccl/base/storage/textparser.h"

namespace CCL {

interface IAttributeList;

//************************************************************************************************
// ExpressionParser macros
//************************************************************************************************

/** Prefix for variable names in expression operands.
 Example: $var */
#define CCL_VARIABLE_PREFIX "$"

/** Prefix for (absolute) property paths in expression operands.
 Example: ^://hostapp/DocumentManager/ActiveDocument.title */
#define CCL_PROPERTY_PREFIX "^"

//************************************************************************************************
// ExpressionParser
/** Evaluates arithmetic expressions with variables ($i), properties and constant literals.
	Operands can be
		- numeric integer literals: -23
		- string literals in single quotes: 'Hamburg'
		- variables: $var
		- (absolute) property paths: ^://hostapp/DocumentManager/ActiveDocument.title

	Supported operators: (highest precedence first, parenthesis "()" can be used)
		!					(logical NOT)
		*  /  %
		+  -
		>  >=  <  <=  =		(result is 1 if true, 0 otherwise)
		& |					(logical AND, OR)
*/
//************************************************************************************************

class ExpressionParser: public TextParser
{
public:
	interface IVariableResolver;

	static bool evaluate (Variant& value, StringRef expression, const IVariableResolver& resolver);
	static bool evaluate (Variant& value, StringRef expression, const IAttributeList& variables);

protected:
	enum LogicalOperator { kAnd, kOr };
	enum ProductOperator { kMultiply, kDivide, kModulo };
	enum RelationalOperator { kLess, kLessOrEqual, kGreater, kGreaterOrEqual, kEqual };

	class AttributesVariableResolver;
	struct DivideHelper;

	static const CString kVariablePrefix;
	static const CString kPropertyPrefix;

	ExpressionParser (IStream& stream, const IVariableResolver& resolver);

	bool readExpression (Variant& value);
	bool readBoolExpression (Variant& value);
	bool readRelation (Variant& value);
	bool readSum (Variant& value);
	bool readProduct (Variant& value);
	bool readFactor (Variant& value);
	bool readVariable (Variant& value);
	bool readConstant (Variant& value);
	bool readBoolOperator (int& op);
	bool readRelationalOperator (int& op);
	bool readProductOperator (int& op);

protected:
	const IVariableResolver& variableResolver;
};

//************************************************************************************************
// ExpressionParser::IVariableResolver
//************************************************************************************************

interface ExpressionParser::IVariableResolver: IUnknown
{
	/** Get value of variable. */
	virtual tbool CCL_API getValue (Variant& value, StringID identifier) const = 0;
};

} // namespace CCL

#endif // _ccl_expressionparser_h
