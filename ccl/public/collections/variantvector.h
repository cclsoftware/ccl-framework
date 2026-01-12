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
// Filename    : ccl/public/collections/variantvector.h
// Description : Variant Vector
//
//************************************************************************************************

#ifndef _ccl_variantvector_h
#define _ccl_variantvector_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/base/iarrayobject.h"
#include "ccl/public/collections/vector.h"

namespace CCL {

//************************************************************************************************
// VariantVectorTemplate
/** \ingroup base_collect */
//************************************************************************************************

template <typename T>
class VariantVectorTemplate: public Unknown,
							 public Vector<T>,
							 public IMutableArray
{
public:
	// IArrayObject
	int CCL_API getArrayLength () const override
	{ return Vector<T>::count (); }

	tbool CCL_API getArrayElement (Variant& var, int index) const override
	{ var = Vector<T>::at (index); return true; }
	
	// IMutableArray
	tbool CCL_API addArrayElement (VariantRef var) override
	{ Vector<T>::add (var); return true; }

	tbool CCL_API removeArrayElement (int index) override
	{ Vector<T>::removeAt (index); return true; }

	tbool CCL_API setArrayElement (int index, VariantRef var) override
	{ Vector<T>::at (index) = var; return true; }
	
	CLASS_INTERFACE2 (IArrayObject, IMutableArray, Unknown)
};

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef VariantVectorTemplate<int> VariantIntVector;
typedef VariantVectorTemplate<double> VariantDoubleVector;
typedef VariantVectorTemplate<String> VariantStringVector;
typedef VariantVectorTemplate<Variant> VariantVector;

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_variantvector_h
