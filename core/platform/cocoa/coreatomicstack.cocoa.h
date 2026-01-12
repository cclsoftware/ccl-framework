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
// Filename    : core/platform/cocoa/coreatomicstack.cocoa.h
// Description : Atomic Stack Cocoa implementation
//
//************************************************************************************************

#ifndef _coreatomicstack_cocoa_h
#define _coreatomicstack_cocoa_h

#include "core/platform/shared/coreplatformatomicstack.h"
#include "core/platform/cocoa/coreatomic.cocoa.h"

#include <libkern/OSAtomic.h>

namespace Core {
namespace Platform {

//************************************************************************************************
// CocoaAtomicStack
//************************************************************************************************

class CocoaAtomicStack: public IAtomicStack
{
public:
	CocoaAtomicStack ();
	~CocoaAtomicStack ();

	typedef Platform::AtomicStackElement Element;

	// IAtomicStack
	Element* pop () override;
	void push (Element* e) override;
	void flush () override;
	int depth () override;

protected:
	OSQueueHead head;
	int stackDepth;
};

typedef CocoaAtomicStack AtomicStack;

} // namespace Platform
} // namespace Core

#endif // _coreatomicstack_win_h
