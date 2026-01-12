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
// Filename    : ccl/public/collections/stack.h
// Description : Stack class
//
//************************************************************************************************

#ifndef _ccl_stack_h
#define _ccl_stack_h

#include "ccl/public/collections/deque.h"

namespace CCL {

//************************************************************************************************
// Stack
/** \ingroup base_collect */
//************************************************************************************************

template<class T>
class Stack: protected Deque<T>
{
public:
	using Deque<T>::isEmpty;
	using Deque<T>::remove;
	using Deque<T>::removeAll;
	using Deque<T>::at;
	using Deque<T>::begin;
	using Deque<T>::end;
	using Deque<T>::count;

	void push (const T& data) { Deque<T>::addFront (data); }
	T pop () { return Deque<T>::popFront (); }
	const T& peek () const { return Deque<T>::peekFront (); }
};

} // namespace CCL

#endif // _ccl_stack_h
