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
// Filename    : ccl/public/collections/vector.h
// Description : Vector class
//
//************************************************************************************************

#ifndef _ccl_vector_h
#define _ccl_vector_h

#include "ccl/public/base/platform.h"

#include "core/public/corevector.h"

namespace CCL {

#if !DOXYGEN

//////////////////////////////////////////////////////////////////////////////////////////////////
// LAMBDA_VECTOR_COMPARE : Define lambda compare function for pointers to objects.
//////////////////////////////////////////////////////////////////////////////////////////////////

#define LAMBDA_VECTOR_COMPARE(Type, lhs, rhs) \
[] (const void* __lhs, const void* __rhs) -> int \
{ Type* lhs = *(Type**)__lhs; \
  Type* rhs = *(Type**)__rhs;

//////////////////////////////////////////////////////////////////////////////////////////////////
// LAMBDA_VECTOR_COMPARE_OBJECT : Define lambda compare function for objects or build-in types.
//////////////////////////////////////////////////////////////////////////////////////////////////

#define LAMBDA_VECTOR_COMPARE_OBJECT(Type, lhs, rhs) \
[] (const void* __lhs, const void* __rhs) -> int \
{ Type* lhs = (Type*)__lhs; \
  Type* rhs = (Type*)__rhs;

//////////////////////////////////////////////////////////////////////////////////////////////////

#endif

using Core::Vector;
using Core::VectorIterator;
using Core::ConstVector;
using Core::FixedSizeVector;
using Core::VectorCompareFunction;
using Core::RangeIterator;
using Core::InitializerList;

} // namespace CCL

#endif // _ccl_vector_h
