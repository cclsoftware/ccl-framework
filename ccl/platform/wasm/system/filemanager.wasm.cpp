//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/wasm/filemanager.wasm.cpp
// Description : WebAssembly file manager
//
//************************************************************************************************

#include "filemanager.wasm.h"

using namespace CCL;

//************************************************************************************************
// WasmFileManager
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (FileManager, WasmFileManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmFileManager::WasmFileManager ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmFileManager::~WasmFileManager ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmFileManager::terminate ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult WasmFileManager::startWatching (UrlRef url, int flags)
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult WasmFileManager::stopWatching (UrlRef url)
{
	return kResultNotImplemented;
}
