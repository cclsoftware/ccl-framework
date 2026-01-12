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
// Filename    : core/platform/cocoa/coreatomicstack.cocoa.cpp
// Description : Atomic Stack Cocoa implementation
//
//************************************************************************************************

#include "coreatomicstack.cocoa.h"

using namespace Core;
using namespace Platform;

//************************************************************************************************
// CocoaAtomicStack
//************************************************************************************************

CocoaAtomicStack::CocoaAtomicStack ()
: head (OS_ATOMIC_QUEUE_INIT),
  stackDepth (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaAtomicStack::~CocoaAtomicStack ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaAtomicStack::Element* CocoaAtomicStack::pop ()
{
	Element* e = reinterpret_cast<Element*> (OSAtomicDequeue (&head, offsetof (Element, next)));
	if(e)
		AtomicAdd (stackDepth, -1);
	return e;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaAtomicStack::push (Element* e)
{
	OSAtomicEnqueue (&head, e, offsetof (Element, next));
	AtomicAdd (stackDepth, 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaAtomicStack::flush ()
{
	while(Element* e = pop ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CocoaAtomicStack::depth ()
{
	return stackDepth;
}
