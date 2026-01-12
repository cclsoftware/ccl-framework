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
// Filename    : ccl/public/system/ipersistentexpression.h
// Description : Expression tree interface
//
//************************************************************************************************

#ifndef _ccl_ipersistentexpression_h
#define _ccl_ipersistentexpression_h

#include "ccl/public/base/iunknown.h"

namespace CCL {
namespace Persistence {

//************************************************************************************************
// IExpression
/** Expression tree interface. */
//************************************************************************************************

interface IExpression: public IUnknown
{
	DEFINE_ENUM (Type)
	{
		kAnd,
		kOr,
		kNot,

		kEquals,
		kNonEquals,
		kGreaterThan,
		kGreaterOrEqual,
		kLessThan,
		kLessOrEqual,
		kLike,
		kContains,
		kIn
	};

	virtual Type CCL_API getExpressionType () = 0;

	virtual IExpression* CCL_API getOperand1 () = 0;		///< for kAnd, kOr, kNot

	virtual IExpression* CCL_API getOperand2 () = 0;		///< for kAnd, kOr

	virtual StringID CCL_API getVariableName () = 0;		///< for kEquals, ... kIn

	virtual VariantRef CCL_API getValue () = 0;				///< for kEquals, ... kContains

	virtual const Variant* CCL_API getValueAt (int i) = 0;	///< for kIn

	DECLARE_IID (IExpression)
};

DEFINE_IID (IExpression, 0xB9CEF7B3, 0x4EA1, 0x4C9E, 0xBD, 0x72, 0x50, 0x88, 0xE0, 0x7C, 0x5E, 0xF4)

} // namespace Persistence
} // namespace CCL

#endif // _ccl_ipersistentexpression_h
