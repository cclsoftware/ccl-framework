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
// Filename    : core/platform/shared/coreplatformatomicstack.h
// Description : Atomic Stack platform implementation base
//
//************************************************************************************************

#ifndef _coreplatformatomicstack_h
#define _coreplatformatomicstack_h

#include "core/public/coreplatform.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// AtomicStackElement
/** Atomic stack element.
	\ingroup core_thread */
//************************************************************************************************

CORE_ALIGN(struct, 16) AtomicStackElement
{
	AtomicStackElement* next;
	AtomicStackElement (): next (nullptr) {}
};

//************************************************************************************************
// IAtomicStack
//************************************************************************************************

struct IAtomicStack
{
	virtual ~IAtomicStack () {}
	
	virtual AtomicStackElement* pop () = 0;
	virtual void push (AtomicStackElement* e) = 0;
	virtual void flush () = 0;
	virtual int depth () = 0;
};

} // namespace Platform
} // namespace Core

#endif // _coreplatformatomicstack_h
