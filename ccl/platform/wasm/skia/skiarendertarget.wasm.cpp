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
// Filename    : ccl/platform/wasm/skia/skiarendertarget.wasm.cpp
// Description : Skia Render Target for WebAssembly
//
//************************************************************************************************

#include "ccl/platform/wasm/skia/skiaengine.wasm.h"
#include "ccl/platform/wasm/opengles/openglesrendertarget.wasm.h"

#include "ccl/platform/shared/skia/skiarendertarget.h"

using namespace CCL;
using namespace Wasm;

//************************************************************************************************
// SkiaWindowRenderTarget
//************************************************************************************************

SkiaWindowRenderTarget* SkiaWindowRenderTarget::create (Window& window)
{
	WasmSkiaEngine* engine = WasmSkiaEngine::getInstance ();
	if(engine == nullptr)
		return nullptr;

	switch(engine->getGraphicsBackend ())
	{
	case WasmSkiaEngine::kWebGL : return NEW WebGLWindowRenderTarget (window);
	default : return nullptr; // TODO
	}
}
