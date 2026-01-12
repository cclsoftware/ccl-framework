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
// Filename    : ccl/system/threading/atomic.cpp
// Description : Atomic Primitives
//
//************************************************************************************************

#include "core/system/coreatomicstack.h"
#include "core/system/corespinlock.h"

#include "ccl/public/base/unknown.h"
#include "ccl/public/systemservices.h"

namespace CCL {
namespace Threading {

//************************************************************************************************
// AtomicStack
//************************************************************************************************

#if CORE_HAS_ATOMIC_STACK
typedef Core::AtomicStack AtomicStackBase;
#else
typedef Core::AtomicStackLocked AtomicStackBase;
#endif

class AtomicStack: public Unknown,
				   public AtomicStackBase,
				   public IAtomicStack
{
public:
	// IAtomicStack
	IAtomicStack::Element* CCL_API pop () override
	{
		return reinterpret_cast<IAtomicStack::Element*> (AtomicStackBase::pop ());
	}

	void CCL_API push (IAtomicStack::Element* e) override
	{
		AtomicStackBase::push (reinterpret_cast<AtomicStackBase::Element*> (e));
	}

	void CCL_API flush () override
	{
		AtomicStackBase::flush ();
	}

	int CCL_API depth () override
	{
		return AtomicStackBase::depth ();
	}

	CLASS_INTERFACE (IAtomicStack, Unknown)
};

} // namespace CCL
} // namespace Threading

using namespace CCL;
using namespace Threading;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Threading APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IAtomicStack* CCL_API System::CCL_ISOLATED (CreateAtomicStack) ()
{
	return NEW AtomicStack;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Atomic Primitives APIs
////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT int32 CCL_API System::CCL_ISOLATED (AtomicAdd) (int32 volatile& variable, int32 value)
{
	return Core::AtomicAdd (variable, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT int32 CCL_API System::CCL_ISOLATED (AtomicSet) (int32 volatile& variable, int32 value)
{
	return Core::AtomicSet (variable, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT int32 CCL_API System::CCL_ISOLATED (AtomicGet) (const int32 volatile& variable)
{
	return Core::AtomicGet (variable);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT tbool CCL_API System::CCL_ISOLATED (AtomicTestAndSet) (int32 volatile& variable, int32 value, int32 comperand)
{
	return Core::AtomicTestAndSet (variable, value, comperand);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void* CCL_API System::CCL_ISOLATED (AtomicSetPtr) (void* volatile& variable, void* value)
{
	return Core::AtomicSetPtr (variable, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void* CCL_API System::CCL_ISOLATED (AtomicGetPtr) (void* const volatile& variable)
{
	return Core::AtomicGetPtr (variable);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT tbool CCL_API System::CCL_ISOLATED (AtomicTestAndSetPtr) (void* volatile& variable, void* value, void* comperand)
{
	return Core::AtomicTestAndSetPtr (variable, value, comperand);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Spin Lock APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT tbool CCL_API System::CCL_ISOLATED (SpinLockTryLock) (int32 volatile& lock)
{
	return Core::CoreSpinLock::tryLock (lock);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void CCL_API System::CCL_ISOLATED (SpinLockLock) (int32 volatile& lock)
{
	return Core::CoreSpinLock::lock (lock);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void CCL_API System::CCL_ISOLATED (SpinLockUnlock) (int32 volatile& lock)
{
	return Core::CoreSpinLock::unlock (lock);
}
