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
// Filename    : core/portable/corepool.h
// Description : Memory Pool
//
//************************************************************************************************

#ifndef _corepool_h
#define _corepool_h

#include "core/public/coremempool.h"

#include "core/system/coreatomicstack.h"

namespace Core {
namespace Portable {

//************************************************************************************************
// AtomicPolicy
/** Helper functions for memory pool.
	\ingroup core_portable */
//************************************************************************************************

class AtomicPolicy
{
public:
	#if CORE_HAS_ATOMIC_STACK
	typedef AtomicStack Stack;
	#else
	typedef AtomicStackLocked Stack;
	#endif

	typedef Stack::Element Element;

	static Stack* createStack () { return NEW Stack; }
	static void releaseStack (Stack* stack) { delete stack; }
	static INLINE int32 add (int32 volatile& variable, int32 value)
	{
		#if CORE_HAS_ATOMICS
			return AtomicAdd (variable, value);
		#else
			// This is not thread-safe!
			int temp = variable;
			variable += value;
			return temp;
		#endif
	}
};

//************************************************************************************************
// CoreMemoryPool
/** Memory pool class.
	\ingroup core_portable */
//************************************************************************************************

typedef MemoryPool<AtomicPolicy> CoreMemoryPool;

} // namespace Portable
} // namespace Core

#endif // _corepool_h
