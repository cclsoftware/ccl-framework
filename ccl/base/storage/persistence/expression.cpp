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
// Filename    : ccl/base/storage/persistence/expression.cpp
// Description : Expression tree
//
//************************************************************************************************

#include "ccl/base/storage/persistence/expression.h"

namespace CCL {
namespace Persistence {

//************************************************************************************************
// ExpressionImpl
/** Base class for IExpression implementations. */
//************************************************************************************************

class ExpressionImpl: public Unknown,
					  public IExpression
{
public:
	// IExpression
	virtual IExpression* CCL_API getOperand1 () override;		///< for kAnd, kOr, kNot
	virtual IExpression* CCL_API getOperand2 () override;		///< for kAnd, kOr
	virtual StringID CCL_API getVariableName () override;		///< for kEquals, ... kIn
	virtual VariantRef CCL_API getValue () override;			///< for kEquals, ... kContains
	virtual const Variant* CCL_API getValueAt (int i) override;	///< for kIn

	CLASS_INTERFACE (IExpression, Unknown)
};

//************************************************************************************************
// CompoundExpression
/** Base class for compound expressions that combine 2 child expressions via AND / OR. */
//************************************************************************************************

class CompoundExpression: public ExpressionImpl
{
public:
	CompoundExpression (IExpression* e1, IExpression* e2)
	: e1 (e1), e2 (e2) {}

	// IExpression
	IExpression* CCL_API getOperand1 () override { return e1; }
	IExpression* CCL_API getOperand2 () override { return e2; }

protected:
	SharedPtr<IExpression> e1;
	SharedPtr<IExpression> e2;
};

//************************************************************************************************
// AndExpression
/** Combines 2 child expressions via AND. */
//************************************************************************************************

class AndExpression: public CompoundExpression
{
public:
	AndExpression (IExpression* e1, IExpression* e2)
	: CompoundExpression (e1, e2) {}

	// IExpression
	Type CCL_API getExpressionType () override { return kAnd; }
};

//************************************************************************************************
// OrExpression
/** Combines 2 child expressions via OR. */
//************************************************************************************************

class OrExpression: public CompoundExpression
{
public:
	OrExpression (IExpression* e1, IExpression* e2)
	: CompoundExpression (e1, e2) {}

	// IExpression
	Type CCL_API getExpressionType () override { return kOr; }
};

//************************************************************************************************
// NotExpression
/** Negates it's child expression. */
//************************************************************************************************

class NotExpression: public CompoundExpression
{
public:
	NotExpression (IExpression* e)
	: CompoundExpression (e, nullptr) {}

	// IExpression
	Type CCL_API getExpressionType () override { return kNot; }
};

//************************************************************************************************
// CompareExpression
/** Compares a member with a constant value using an operator defined by the template argument. */
//************************************************************************************************

template<IExpression::Type exprType>
class CompareExpression: public ExpressionImpl
{
public:
	CompareExpression (StringID varName, VariantRef val)
	: varName (varName), value (val)
	{ value.share (); }

	// IExpression
	Type CCL_API getExpressionType () override		{ return exprType; }
	StringID CCL_API getVariableName () override	{ return varName; }
	VariantRef CCL_API getValue () override			{ return value; }

private:
	MutableCString varName;
	Variant value;
};

//************************************************************************************************
// Instantiations of CompareExpression<>
//************************************************************************************************

typedef CompareExpression<IExpression::kEquals>			EqualsExpression;
typedef CompareExpression<IExpression::kNonEquals>		NonEqualsExpression;
typedef CompareExpression<IExpression::kGreaterThan>	GreaterExpression;
typedef CompareExpression<IExpression::kGreaterOrEqual>	GreaterOrEqualExpression;
typedef CompareExpression<IExpression::kLessThan>		LessThanExpression;
typedef CompareExpression<IExpression::kLessOrEqual>	LessOrEqualExpression;
typedef CompareExpression<IExpression::kLike>			LikeExpression;
typedef CompareExpression<IExpression::kContains>		ContainsExpression;

//************************************************************************************************
// InExpression
/** Checks if a member equals any element of a set of constant values. */
//************************************************************************************************

class InExpression: public ExpressionImpl
{
public:
	InExpression (StringID varName, VariantRef v1)
	: varName (varName), values (1)
	{ values.add (v1); }

	InExpression (StringID varName, VariantRef v1, VariantRef v2)
	: varName (varName), values (2)
	{ values.add (v1); values.add (v2); }

	InExpression (StringID varName, VariantRef v1, VariantRef v2, VariantRef v3)
	: varName (varName), values (3)
	{ values.add (v1); values.add (v2); values.add (v3); }

	InExpression (StringID varName, VariantRef v1, VariantRef v2, VariantRef v3, VariantRef v4)
	: varName (varName), values (4)
	{ values.add (v1); values.add (v2); values.add (v3); values.add (v4); }

	InExpression (StringID varName, VariantRef v1, VariantRef v2, VariantRef v3, VariantRef v4, VariantRef v5)
	: varName (varName), values (5)
	{ values.add (v1); values.add (v2); values.add (v3); values.add (v4); values.add (v5); }

	// IExpression
	Type CCL_API getExpressionType () override			{ return kIn; }
	StringID CCL_API getVariableName () override		{ return varName; }
	const Variant* CCL_API getValueAt (int i) override	{ return i < values.count () ? &values[i] : nullptr; }

private:
	MutableCString varName;
	Vector<Variant> values;
};

} // namespace Persistence
} // namespace CCL

using namespace CCL;
using namespace Persistence;

//************************************************************************************************
// Member
//************************************************************************************************

Expression Member::operator== (VariantRef value)	{ return Expression (NEW EqualsExpression (name, value)); }
Expression Member::operator!= (VariantRef value)	{ return Expression (NEW NonEqualsExpression (name, value)); }
Expression Member::operator> (VariantRef value)		{ return Expression (NEW GreaterExpression (name, value)); }
Expression Member::operator>= (VariantRef value)	{ return Expression (NEW GreaterOrEqualExpression (name, value)); }
Expression Member::operator< (VariantRef value)		{ return Expression (NEW LessThanExpression (name, value)); }
Expression Member::operator<= (VariantRef value)	{ return Expression (NEW LessOrEqualExpression (name, value)); }
Expression Member::like (StringRef pattern)			{ return Expression (NEW LikeExpression (name, pattern)); }
Expression Member::contains (StringRef value)		{ return Expression (NEW ContainsExpression (name, value)); }
Expression Member::in (VariantRef v1)																{ return Expression (NEW InExpression (name, v1)); }
Expression Member::in (VariantRef v1, VariantRef v2)												{ return Expression (NEW InExpression (name, v1, v2)); }
Expression Member::in (VariantRef v1, VariantRef v2, VariantRef v3)									{ return Expression (NEW InExpression (name, v1, v2, v3)); }
Expression Member::in (VariantRef v1, VariantRef v2, VariantRef v3, VariantRef v4)					{ return Expression (NEW InExpression (name, v1, v2, v3, v4)); }
Expression Member::in (VariantRef v1, VariantRef v2, VariantRef v3, VariantRef v4, VariantRef v5)	{ return Expression (NEW InExpression (name, v1, v2, v3, v4, v5)); }

//************************************************************************************************
// Expression
//************************************************************************************************

Expression Expression::operator && (const Expression& expression)
{
	return Expression (NEW AndExpression (*this, expression));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Expression Expression::operator || (const Expression& expression)
{
	return Expression (NEW OrExpression (*this, expression));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Expression Expression::operator ! ()
{
	return Expression (NEW NotExpression (*this));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IExpression* CCL_API ExpressionImpl::getOperand1 ()
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IExpression* CCL_API ExpressionImpl::getOperand2 ()
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API ExpressionImpl::getVariableName ()
{
	return CString::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VariantRef CCL_API ExpressionImpl::getValue ()
{
	static Variant dummy;
	return dummy;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Variant* CCL_API ExpressionImpl::getValueAt (int i)
{
	return nullptr;
}
