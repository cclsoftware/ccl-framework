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
// Filename    : ccl/public/system/threadsync.cpp
// Description : Synchronization classes
//
//************************************************************************************************

#include "ccl/public/system/threadsync.h"

using namespace CCL;
using namespace Threading;

//************************************************************************************************
// SyncObject
//************************************************************************************************

SyncObject::SyncObject (ISyncPrimitive* primitive)
: primitive (primitive)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SyncObject::~SyncObject ()
{ 
	if(primitive)
		primitive->release (); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SyncObject::SyncObject (const SyncObject&)
: primitive (nullptr)
{
	ASSERT (0) 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SyncObject& SyncObject::operator = (const SyncObject&)
{ 
	ASSERT (0) 
	return *this; 
}

//************************************************************************************************
// CriticalSection
//************************************************************************************************

CriticalSection::CriticalSection ()
: SyncObject (System::CreateSyncPrimitive (ClassID::CriticalSection))
{}

//************************************************************************************************
// Signal
//************************************************************************************************

Signal::Signal (bool manualReset)
: SyncObject (System::CreateSyncPrimitive (manualReset ? ClassID::ManualSignal : ClassID::Signal))
{}
