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
// Filename    : core/platform/win/coreatomicstack.win.h
// Description : Atomic Stack Windows implementation
//
//************************************************************************************************

#ifndef _coreatomicstack_win_h
#define _coreatomicstack_win_h

#include "core/platform/shared/coreplatformatomicstack.h"
#include "core/system/coreatomic.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// Win32AtomicStack
//************************************************************************************************

class Win32AtomicStack: public IAtomicStack
{
public:
	Win32AtomicStack ();
	~Win32AtomicStack ();

	typedef Platform::AtomicStackElement Element;

	// IAtomicStack
	Element* pop () override;
	void push (Element* e) override;
	void flush () override;
	int depth () override;

protected:
	SLIST_HEADER* head;
};

typedef Win32AtomicStack AtomicStack;

} // namespace Platform
} // namespace Core

#endif // _coreatomicstack_win_h
