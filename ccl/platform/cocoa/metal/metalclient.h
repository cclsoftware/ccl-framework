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
// Filename    : ccl/platform/cocoa/metal/metalclient.h
// Description : Metal Client Context
//
//************************************************************************************************

#ifndef _metalclient_h
#define _metalclient_h

#include "ccl/base/singleton.h"

#include "ccl/platform/cocoa/macutils.h"

@protocol MTLDevice;
@protocol MTLCommandQueue;

namespace CCL {

//************************************************************************************************
// MetalClient
//************************************************************************************************

class MetalClient: public Object,
				   public Singleton<MetalClient>
{
public:
	MetalClient();
	
	bool isSupported ();
	id<MTLDevice> getDevice ();
	id<MTLCommandQueue> getQueue ();

protected:
	bool initialized;
	
	void initialize ();
	NSObj<NSObject<MTLDevice>> device;
	NSObj<NSObject<MTLCommandQueue>> queue;
};

} // namespace CCL

#endif // _metalclient_h

