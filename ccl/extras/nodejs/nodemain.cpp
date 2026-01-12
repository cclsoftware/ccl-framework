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
// Filename    : ccl/extras/nodejs/nodemain.cpp
// Description : Node Addon main entry point
//
//************************************************************************************************

#include "ccl/main/cclinit.h"
#include "ccl/public/systemservices.h"

#include "ccl/extras/nodejs/napihelpers.h"
#include "ccl/extras/nodejs/nodeaddon.h"

using namespace CCL;
using namespace NodeJS;

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace CCL {
namespace NodeJS {

static ModuleRef g_ModuleReference = 0;

static void nodeShutdown (void* arg);
static napi_value nodeInitAll (napi_env environment, napi_value exports);

} // namesapce CCL
} // namespace NodeJS

///////////////////////////////////////////////////////////////////////////////////////////////////

ModuleRef CCL::System::GetCurrentModuleRef ()
{
	return g_ModuleReference;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void CCL::NodeJS::nodeShutdown (void* arg)
{
	NodeAddon::getInstance ().shutdown ();

	FrameworkInitializer initializer;
	initializer.exit ();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

napi_value CCL::NodeJS::nodeInitAll (napi_env environment, napi_value exports)
{
	napi_status status = napi_add_env_cleanup_hook (environment, nodeShutdown, environment);
	assert (status == napi_ok);

	NodeAddon& addon = NodeAddon::getInstance ();
	g_ModuleReference = addon.initPlatformModule ();

	FrameworkInitializer initializer;
	initializer.init ();

	addon.startup (environment);
	exports = addon.createExportsObject ();

	return exports;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPI_MODULE (ucaddon, nodeInitAll)
