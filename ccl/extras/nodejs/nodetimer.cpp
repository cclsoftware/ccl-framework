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
// Filename    : ccl/extras/nodejs/nodetimer.cpp
// Description : Node.js timer support
//
//************************************************************************************************

#include "nodejs/include/uv.h"
#include "nodejs/include/node_api.h"

#include "ccl/extras/nodejs/nodetimer.h"

using namespace CCL;
using namespace NodeJS;

//************************************************************************************************
// NodeTimer
//************************************************************************************************

NodeTimer::NodeTimer () 
: handle (new uv_timer_s)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NodeTimer::~NodeTimer ()
{
	delete handle;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void NodeTimer::initialize (napi_env environment)
{
	uv_loop_s *loop;
	napi_status status;

	status = napi_get_uv_event_loop (environment, &loop);
	assert (status == napi_ok);

	uv_timer_init (loop, handle);
	handle->data = this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void NodeTimer::start (uint64 timeout, uint64 interval)
{
	uv_timer_start (handle, &timerCallback, timeout, interval);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void NodeTimer::stop ()
{
	uv_timer_stop (handle);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void NodeTimer::timerCallback (uv_timer_s* handle)
{
	NodeTimer* timer = reinterpret_cast<NodeTimer*> (handle->data);
	timer->timerExpired ();
}
