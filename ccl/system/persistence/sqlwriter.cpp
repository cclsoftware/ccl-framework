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
// Filename    : ccl/system/persistence/sqlwriter.cpp
// Description : SQL Writer
//
//************************************************************************************************

#include "ccl/system/persistence/sqlwriter.h"
#include "ccl/system/persistence/classinfo.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/system/ipersistentexpression.h"

using namespace CCL;
using namespace Persistence;

namespace {

//////////////////////////////////////////////////////////////////////////////////////////////////

template<IExpression::Type type> inline const char* getOperatorLiteral ()			{ return nullptr; }
template<>inline const char* getOperatorLiteral<IExpression::kAnd> ()				{ return " AND "; }
template<>inline const char* getOperatorLiteral<IExpression::kOr> ()				{ return " OR "; }
template<>inline const char* getOperatorLiteral<IExpression::kNot> ()				{ return " NOT "; }
template<>inline const char* getOperatorLiteral<IExpression::kEquals> ()			{ return "="; }
template<>inline const char* getOperatorLiteral<IExpression::kNonEquals> ()			{ return "!="; }
template<>inline const char* getOperatorLiteral<IExpression::kGreaterThan> ()		{ return ">"; }
template<>inline const char* getOperatorLiteral<IExpression::kGreaterOrEqual> ()	{ return ">="; }
template<>inline const char* getOperatorLiteral<IExpression::kLessThan> ()			{ return "<"; }
template<>inline const char* getOperatorLiteral<IExpression::kLessOrEqual> ()		{ return "<="; }
template<>inline const char* getOperatorLiteral<IExpression::kLike> ()				{ return " LIKE "; }
template<>inline const char* getOperatorLiteral<IExpression::kContains> ()			{ return " LIKE "; }
template<>inline const char* getOperatorLiteral<IExpression::kIn> ()				{ return " IN("; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template<int expressionType>
inline void writeExpressionValue (SqlWriter& writer, IExpression* expression)
{
	VariantRef value = expression->getValue ();
	if(value.isValid ())
		writer.writeLiteral (value);
	else
		writer.write ("?"); // an empty Variant denotes a variable that can be bound later
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<>
inline void writeExpressionValue<IExpression::kContains> (SqlWriter& writer, IExpression* expression)
{
	// wrap the string in wildcards
	VariantRef value = expression->getValue ();
	String string (value.string);
	string.prepend ("%");
	string.append ("%");
	writer.writeStringLiteral (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<>
inline void writeExpressionValue<IExpression::kIn> (SqlWriter& writer, IExpression* expression)
{
	// hmm, could also support variables ("?") for each value
	int i = 0;
	while(const Variant* value = expression->getValueAt (i))
	{
		if(i != 0)
			writer.write (",");
		writer.writeLiteral (*value);
		i++;
	}
	writer.write (")");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline int getPriority (int expressionType)
{
	// operator priority in SQL
	switch(expressionType)
	{
		case IExpression::kOr:
			return 0;
		case IExpression::kAnd:
			return 1;
		default: // all other operators (==, >, ...)
			return 2;
	}
}

} // anonymous namespace

//************************************************************************************************
// SqlWriter
//************************************************************************************************

SqlWriter::SqlWriter ()
: outerPriority (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SqlWriter& SqlWriter::writeLiteral (VariantRef value)
{
	switch(value.getType ())
	{
		case Variant::kInt:
			sql.appendFormat ("%d", value.lValue);
			break;

		case Variant::kFloat:
			sql.appendFormat ("%f", value.fValue);
			break;

		case Variant::kString:
		{
			String string (value.string);
			writeStringLiteral (string);
		}
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SqlWriter& SqlWriter::writeStringLiteral (StringRef string)
{
	sql.append ("'");

	if(string.contains ("'")) // must quote ' characters
	{
		// first convert source string to utf8
		MutableCString utf8 (string, Text::kUTF8);

		// replace ' by ''
		if(const char* ptr = utf8.str ())
		{
			CStringWriter<128> writer (sql, false);
			while(*ptr)
			{
				if(*ptr ==  '\'')
					writer.append ('\'');

				writer.append (*ptr);
				ptr++;
			}
			writer.flush ();
		}
	}
	else
		sql.append (string, Text::kUTF8);

	sql.append ("'");
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SqlWriter& SqlWriter::writeExpression (IExpression* expression, ClassInfo& classInfo)
{
	IExpression::Type type = expression->getExpressionType ();
	switch(type)
	{
		case IExpression::kAnd:
			writeCompoundExpression<IExpression::kAnd> (expression, classInfo);
			break;
		case IExpression::kOr:
			writeCompoundExpression<IExpression::kOr> (expression, classInfo);
			break;
		case IExpression::kNot:
			writeNotExpression (expression, classInfo);
			break;
		case IExpression::kEquals:
			writeCompareExpression<IExpression::kEquals> (expression, classInfo);
			break;
		case IExpression::kNonEquals:
			writeCompareExpression<IExpression::kNonEquals> (expression, classInfo);
			break;
		case IExpression::kGreaterThan:
			writeCompareExpression<IExpression::kGreaterThan> (expression, classInfo);
			break;
		case IExpression::kGreaterOrEqual:
			writeCompareExpression<IExpression::kGreaterOrEqual> (expression, classInfo);
			break;
		case IExpression::kLessThan:
			writeCompareExpression<IExpression::kLessThan> (expression, classInfo);
			break;
		case IExpression::kLessOrEqual:
			writeCompareExpression<IExpression::kLessOrEqual> (expression, classInfo);
			break;
		case IExpression::kLike:
			writeCompareExpression<IExpression::kLike> (expression, classInfo);
			break;
		case IExpression::kContains:
			writeCompareExpression<IExpression::kContains> (expression, classInfo);
			break;
		case IExpression::kIn:
			writeCompareExpression<IExpression::kIn> (expression, classInfo);
			break;
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<int expressionType>
void SqlWriter::writeCompoundExpression (IExpression* expression, ClassInfo& classInfo)
{
	IExpression* e1 = expression->getOperand1 ();
	IExpression* e2 = expression->getOperand2 ();
	int priority = getPriority (expressionType);

	bool needBrackets = priority < outerPriority;
	if(needBrackets)
		sql.append ("(");

	// write first operand
	outerPriority = priority;
	writeExpression (e1, classInfo);

	// write AND / OR
	sql.append (getOperatorLiteral<expressionType> ());

	// write second operand
	outerPriority = priority;
	writeExpression (e2, classInfo);

	if(needBrackets)
		sql.append (")");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SqlWriter::writeNotExpression (IExpression* expression, ClassInfo& classInfo)
{
	IExpression* e = expression->getOperand1 ();
	sql.append ("NOT(");

	outerPriority = 0;
	writeExpression (e, classInfo);

	sql.append (")");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<int expressionType>
void SqlWriter::writeCompareExpression (IExpression* expression, ClassInfo& classInfo)
{
	if(MemberInfo* member = classInfo.getMappedMember (expression->getVariableName ()))
	{
		sql.append (member->getColumnName ());
		sql.append (getOperatorLiteral<expressionType> ());
		writeExpressionValue<expressionType> (*this, expression);
	}
	else
		sql.append ("TRUE");
}
