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
// Filename    : core/platform/shared/posix/coreatomicstack.posix.h
// Description : Atomic Stack POSIX implementation
//               based on https://github.com/microsoft/msphpsql/blob/master/source/shared/interlockedslist.h
//
//************************************************************************************************

#ifndef _coreatomicstack_posix_h
#define _coreatomicstack_posix_h

#include "core/platform/shared/coreplatformatomicstack.h"
#include "core/system/coreatomic.h"

namespace Core {
namespace Platform {

struct ListHeader;
    
//************************************************************************************************
// PosixAtomicStack
//************************************************************************************************

class PosixAtomicStack: public IAtomicStack
{
public:
	PosixAtomicStack ();
	~PosixAtomicStack ();

	typedef Platform::AtomicStackElement Element;

	// IAtomicStack
	Element* pop () override;
	void push (Element* e) override;
	void flush () override;
	int depth () override;

protected:
	struct PriorityScope;
	ListHeader* head;
	volatile int32 maxThreadPriority;
};

typedef PosixAtomicStack AtomicStack;

} // namespace Platform
} // namespace Core

#endif // _coreatomicstack_posix_h
