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
// Filename    : ccl/extras/nodejs/nodetimer.h
// Description : Node.js timer support
//
//************************************************************************************************

#ifndef _ccl_nodetimer_h
#define _ccl_nodetimer_h

#include "ccl/public/base/platform.h"

#include "submodules/nodejs/include/node_api.h"

struct uv_timer_s;

namespace CCL {
namespace NodeJS {

//************************************************************************************************
// NodeTimer
//************************************************************************************************

class NodeTimer
{
public:
	NodeTimer ();
	~NodeTimer ();

	void initialize (napi_env environment);
	void start (uint64 timeout, uint64 interval);
	void stop ();

protected:
	virtual void timerExpired () = 0;

private:
	uv_timer_s* handle;

	static void timerCallback (uv_timer_s* handle);
};

} // namespace NodeJS
} // namespace CCL

#endif // _ccl_nodetimer_h
