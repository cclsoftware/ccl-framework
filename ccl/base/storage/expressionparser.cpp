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
// Filename    : ccl/base/storage/expressionparser.cpp
// Description : Expression parser
//
//************************************************************************************************

#include "ccl/base/trigger.h"
#include "ccl/base/storage/expressionparser.h"

#include "ccl/public/base/memorystream.h"
#include "ccl/public/storage/iattributelist.h"

using namespace CCL;

//************************************************************************************************
// ExpressionParser::AttributesVariableResolver
//************************************************************************************************

class ExpressionParser::AttributesVariableResolver: public Unknown,
													public IVariableResolver
{
public:
	AttributesVariableResolver (const IAttributeList& attributes)
	: attributes (attributes)
	{}

	// IVariableResolver
	tbool CCL_API getValue (Variant& value, StringID identifier) const override
	{
		return attributes.getAttribute (value, identifier);
	}

	CLASS_INTERFACE (IVariableResolver, Unknown)

private:
	const IAttributeList& attributes;
};

//************************************************************************************************
// ExpressionParser::DivideHelper
//************************************************************************************************

struct ExpressionParser::DivideHelper
{
	template<class T> static void divide (Variant& result, T v1, T v2)
	{
		if(v2 == 0)
		{
			CCL_WARN ("ExpressionParser: division by zero.", 0);
			result = 0;
		}
		else
			result = v1 / v2;
	}

	template<class T> static void modulo (Variant& result, T v1, T v2)
	{
		if(v2 == 0)
		{
			CCL_WARN ("ExpressionParser: division by zero.", 0);
			result = 0;
		}
		else
			result = v1 % v2;
	}
};

//************************************************************************************************
// ExpressionParser
//************************************************************************************************

const CString ExpressionParser::kVariablePrefix = CCL_VARIABLE_PREFIX;
const CString ExpressionParser::kPropertyPrefix = CCL_PROPERTY_PREFIX;

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExpressionParser::evaluate (Variant& value, StringRef expression, const IVariableResolver& resolver)
{
	StringChars chars (expression);
	MemoryStream memstream ((void*)(const uchar*)chars, (expression.length () + 1) * sizeof(uchar));

	ExpressionParser parser (memstream, resolver);
	return parser.readExpression (value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExpressionParser::evaluate (Variant& value, StringRef expression, const IAttributeList& variables)
{
	AttributesVariableResolver resolver (variables);
	return evaluate (value, expression, resolver);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ExpressionParser::ExpressionParser (IStream& stream, const IVariableResolver& resolver)
: TextParser (stream),
  variableResolver (resolver)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExpressionParser::readExpression (Variant& value)
{
	return readBoolExpression (value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExpressionParser::readBoolOperator (int& op)
{
	if(read ('&'))
		op = kAnd;
	else if(read ('|'))
		op = kOr;
	else
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExpressionParser::readBoolExpression (Variant& value)
{
	Variant v1;
	if(!readRelation (v1))
		return false;

	skipWhite ();
	int op = kAnd;
	while(readBoolOperator (op))
	{
		Variant v2;
		if(!readRelation (v2))
			return false;

		if(v1.isString ())
			v1.asString ().scanFormat ("%(1)", &v1, 1);
		if(v2.isString ())
			v2.asString ().scanFormat ("%(1)", &v2, 1);

		switch(op)
		{
		case kAnd:
			v1 = v1.asBool () && v2.asBool ();
			break;

		case kOr:
			v1 = v1.asBool () || v2.asBool ();
			break;
		}
		skipWhite ();
	}
	value = v1;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExpressionParser::readRelationalOperator (int& op)
{
	if(read ('<'))
		op = read ('=') ? kLessOrEqual : kLess;
	else if(read ('>'))
		op = read ('=') ? kGreaterOrEqual : kGreater;
	else if(read ('='))
		op = kEqual;
	else
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExpressionParser::readRelation (Variant& value)
{
	Variant v1;
	if(!readSum (v1))
		return false;

	skipWhite ();
	int op = kEqual;
	while(readRelationalOperator (op))
	{
		Variant v2;
		if(!readSum (v2))
			return false;

		bool result = false;
		switch(op)
		{
		case kLess:
			result = v1 < v2;
			break;
		case kLessOrEqual:
			result = v1 < v2 || v1 == v2;
			break;
		case kGreater:
			result = v1 > v2;
			break;
		case kGreaterOrEqual:
			result = v1 > v2 || v1 == v2;
			break;
		case kEqual:
			result = v1 == v2;
			break;
		}
		v1 = result ? 1 : 0;

		skipWhite ();
	}
	value = v1;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExpressionParser::readSum (Variant& value)
{
	Variant v1;
	if(!readProduct (v1))
		return false;

	skipWhite ();
	bool plus = false;
	while((plus = read ('+')) || read ('-'))
	{
		Variant v2;
		if(!readProduct (v2))
			return false;

		int sign = plus ? 1 : -1;

		if(v1.isString ())
			v1.asString ().scanFormat ("%(1)", &v1, 1);
		if(v2.isString ())
			v2.asString ().scanFormat ("%(1)", &v2, 1);

		if(v1.isInt () && v2.isInt ())
			v1 = v1.asLargeInt () + sign * v2.asLargeInt ();
		else
			v1 = v1.asDouble () + sign * v2.asDouble ();

		skipWhite ();
	}
	value = v1;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExpressionParser::readProductOperator (int& op)
{
	if(read ('*'))
		op = kMultiply;
	else if(read ('/'))
		op = kDivide;
	else if(read ('%'))
		op = kModulo;
	else
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExpressionParser::readProduct (Variant& value)
{
	Variant v1;
	if(!readFactor (v1))
		return false;

	skipWhite ();
	int op = kMultiply;
	while(readProductOperator (op))
	{
		Variant v2;
		if(!readFactor (v2))
			return false;

		if(v1.isString ())
			v1.asString ().scanFormat ("%(1)", &v1, 1);
		if(v2.isString ())
			v2.asString ().scanFormat ("%(1)", &v2, 1);

		switch(op)
		{
		case kMultiply:
			if(v1.isInt () && v2.isInt ())
				v1 = v1.asLargeInt () * v2.asLargeInt ();
			else
				v1 = v1.asDouble () * v2.asDouble ();
			break;

		case kDivide:
			if(v1.isInt () && v2.isInt ())
				DivideHelper::divide (v1, v1.asLargeInt (), v2.asLargeInt ());
			else
				DivideHelper::divide (v1, v1.asDouble (), v2.asDouble ());
			break;

		case kModulo:
			DivideHelper::modulo (v1, v1.asLargeInt (), v2.asLargeInt ());
			break;
		}
		skipWhite ();
	}
	value = v1;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExpressionParser::readFactor (Variant& value)
{
	skipWhite ();

	if(read ('!'))
	{
		Variant v;
		if(readFactor (v))
		{
			value = !v.asBool ();
			return true;
		}
		return false;
	}

	if(readVariable (value))
		return true;

	if(readConstant (value))
		return true;

	if(read ('('))
	{
		if(!readExpression (value))
			return false;

		skipWhite ();
		if(!read (')'))
			return false;

		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExpressionParser::readVariable (Variant& value)
{
	if(read (kVariablePrefix[0]))
	{
		char varName[256] = { kVariablePrefix[0] };
		char* identifier = varName + 1;
		readIdentifier (identifier, ARRAY_COUNT (varName) - 1);

		if(!variableResolver.getValue (value, varName))
			value.clear ();

		return true;
	}
	else if(read (kPropertyPrefix[0]))
	{
		MutableCString propertyPath;
		readPropertyPath (propertyPath);
		Property (propertyPath).get (value);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExpressionParser::readConstant (Variant& value)
{
	int64 intValue;
	if(readInt (intValue))
	{
		value = intValue;
		return true;
	}
	else if(peek () == '\'')
	{
		String string;
		readStringLiteral (string, '\'');
		value = Variant (string, true);
		return true;
	}
	return false;
}
