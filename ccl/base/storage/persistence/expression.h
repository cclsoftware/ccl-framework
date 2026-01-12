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
// Filename    : ccl/base/storage/persistence/expression.h
// Description : Expression tree
//
//************************************************************************************************

#ifndef _ccl_expression_h
#define _ccl_expression_h

#include "ccl/public/system/ipersistentexpression.h"

#include "ccl/public/base/unknown.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/collections/vector.h"

namespace CCL {
namespace Persistence {

//************************************************************************************************
// Expression
/** SmartPointer owning an IExpression. */
//************************************************************************************************

class Expression: public AutoPtr<IExpression>
{
public:
	Expression (IExpression* e = nullptr)  : AutoPtr<IExpression> (e) {}
	Expression (const Expression& e) : AutoPtr<IExpression> (e) {}

	// building compound expressions in the familiar C++ syntax
	Expression operator && (const Expression& expression);	///< build AND expression
	Expression operator || (const Expression& expression);	///< build OR expression
	Expression operator ! ();								///< negate expression (NOT)
};

//************************************************************************************************
// Member
/** Describes a member variable in an expression. */
//************************************************************************************************

class Member
{
public:
	Member (StringID name);

	PROPERTY_MUTABLE_CSTRING (name, Name)

	// building expressions in the familiar C++ syntax
	Expression operator== (VariantRef value);
	Expression operator!= (VariantRef value);
	Expression operator> (VariantRef value);
	Expression operator>= (VariantRef value);
	Expression operator< (VariantRef value);
	Expression operator<= (VariantRef value);
	Expression like (StringRef pattern);
	Expression contains (StringRef value);
	Expression in (VariantRef v1);
	Expression in (VariantRef v1, VariantRef v2);
	Expression in (VariantRef v1, VariantRef v2, VariantRef v3);
	Expression in (VariantRef v1, VariantRef v2, VariantRef v3, VariantRef v4);
	Expression in (VariantRef v1, VariantRef v2, VariantRef v3, VariantRef v4, VariantRef v5);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline Member::Member (StringID name) : name (name) {}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Persistence
} // namespace CCL

#endif // _ccl_expression_h
