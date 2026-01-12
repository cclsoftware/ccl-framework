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
// Filename    : ccl/platform/cocoa/metal/metalclient.mm
// Description : Metal Client
//
//************************************************************************************************

#include "ccl/platform/cocoa/metal/metalclient.h"

#include "ccl/public/base/ccldefpush.h"
#include <Metal/Metal.h>

#define DISABLE_METAL 0

using namespace CCL;

//************************************************************************************************
// MetalClient
//************************************************************************************************

DEFINE_SINGLETON (MetalClient)

//////////////////////////////////////////////////////////////////////////////////////////////////

MetalClient::MetalClient ()
: initialized (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetalClient::isSupported ()
{
	if(initialized == false)
		initialize ();
		
	return queue != nil;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalClient::initialize ()
{
	#if !DISABLE_METAL
	device = MTLCreateSystemDefaultDevice ();
	if(device != nil)
		queue = [device newCommandQueue];
	#endif
	
	initialized = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

id<MTLDevice> MetalClient::getDevice ()
{
	if(initialized == false)
		initialize ();
		
	return device;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

id<MTLCommandQueue> MetalClient::getQueue ()
{
	if(initialized == false)
		initialize ();
		
	return queue;
}

