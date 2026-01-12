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
// Filename    : ccl/public/base/iarrayobject.h
// Description : Array Object Interface
//
//************************************************************************************************

#ifndef _ccl_iarrayobject_h
#define _ccl_iarrayobject_h

#include "ccl/public/base/variant.h"

namespace CCL {

//************************************************************************************************
// IArrayObject
/** Array object interface. 
	\ingroup ccl_base */
//************************************************************************************************

interface IArrayObject: IUnknown
{
	/** Get length of array. */
	virtual int CCL_API getArrayLength () const = 0;
	
	/** Get array element at given index. */
	virtual tbool CCL_API getArrayElement (Variant& var, int index) const = 0;
	
	//////////////////////////////////////////////////////////////////////////////////////////////
	
	Variant operator [] (int index) const { Variant v; getArrayElement (v, index); return v; }
	
	DECLARE_IID (IArrayObject)
};

DEFINE_IID (IArrayObject, 0x929f632d, 0xb8f, 0x4594, 0xa5, 0xf, 0xd3, 0x3d, 0xb1, 0x9f, 0xa4, 0xc5)

//************************************************************************************************
// IMutableArray
/** Mutable array object interface. 
	\ingroup ccl_base */
//************************************************************************************************

interface IMutableArray: IArrayObject
{
	/** Add elememt to array. */
	virtual tbool CCL_API addArrayElement (VariantRef var) = 0;
	
	/** Set element in array. */
	virtual tbool CCL_API setArrayElement (int index, VariantRef var) = 0;

	/** Remove element from array. */
	virtual tbool CCL_API removeArrayElement (int index) = 0;

	DECLARE_IID (IMutableArray)
};

DEFINE_IID (IMutableArray, 0xef8f85a, 0x8254, 0x4466, 0xa5, 0x2f, 0x5f, 0x64, 0x4e, 0x73, 0x28, 0x2e)

} // namespace CCL

#endif // _ccl_iarrayobject_h
