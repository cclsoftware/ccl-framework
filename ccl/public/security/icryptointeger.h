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
// Filename    : ccl/public/security/icryptointeger.h
// Description : Big Integer Interface
//
//************************************************************************************************

#ifndef _ccl_icryptointeger_h
#define _ccl_icryptointeger_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

class MutableCString;

namespace Security {
namespace Crypto {

//************************************************************************************************
// Crypto::IInteger
/** \ingroup base_io */
//************************************************************************************************

interface IInteger: IUnknown
{
	/** Assign from string representation. */
	virtual tresult CCL_API fromCString (CStringPtr string, int base = 16) = 0;

	/** Convert to string representation. */
	virtual tresult CCL_API toCString (MutableCString& string, int base = 16) const = 0;

	/** Add operation. */
	virtual tresult CCL_API add (IInteger& result, const IInteger& value) = 0;
	
	/** Substract operation. */
	virtual tresult CCL_API substract (IInteger& result, const IInteger& value) = 0;
	
	/** Multiply operation. */
	virtual tresult CCL_API multiply (IInteger& result, const IInteger& factor) = 0;
	
	/** Division operation. */
	virtual tresult CCL_API divide (IInteger& result, const IInteger& divisor) = 0;
	
	/** Exponential mod operation. */
	virtual tresult CCL_API expMod (IInteger& result, const IInteger& exp, const IInteger& mod) = 0;

	/** Modulo operation. */
	virtual tresult CCL_API modulo (IInteger& result, const IInteger& value) = 0;
	
	DECLARE_IID (IInteger)
};

DEFINE_IID (IInteger, 0x9b7a9b18, 0xb292, 0x490c, 0xb4, 0x3c, 0xb4, 0x7e, 0x74, 0xae, 0xc3, 0x5f)

} // namespace Crypto
} // namespace Security
} // namespace CCL

#endif // _ccl_icryptointeger_h
