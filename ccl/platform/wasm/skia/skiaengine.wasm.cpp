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
// Filename    : ccl/platform/wasm/skia/skiaengine.wasm.cpp
// Description : WebAssembly Skia Engine
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/wasm/skia/skiaengine.wasm.h"
#include "ccl/platform/wasm/skia/skialayer.wasm.h"

#include "ccl/platform/shared/skia/skiabitmap.h"

#include "ccl/platform/shared/opengles/opengles3dsupport.h"

#include "ccl/base/storage/configuration.h"

#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Wasm;

//************************************************************************************************
// WasmSkiaEngine
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (WasmSkiaEngine, SkiaEngine)

const Configuration::BoolValue useWebGL ("CCL.Graphics.WebGL", "Enabled", true);

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmSkiaEngine* WasmSkiaEngine::getInstance ()
{
	return ccl_cast<WasmSkiaEngine> (&NativeGraphicsEngine::instance ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmSkiaEngine::GraphicsBackendType WasmSkiaEngine::getGraphicsBackend () const
{
	if(useWebGL)
		return kWebGL;
	return kSoftware;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DGraphicsFactory* WasmSkiaEngine::create3DGraphicsFactory ()
{
	switch(getGraphicsBackend ())
	{
	case kWebGL : return NEW OpenGLES3DGraphicsFactory;
	default : return nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmSkiaEngine::hasGraphicsLayers ()
{
	switch(getGraphicsBackend ())
	{
	case kWebGL : return true;
	default : return false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsLayer* WasmSkiaEngine::createGraphicsLayer (UIDRef classID)
{
	switch(getGraphicsBackend ())
	{
	case kWebGL : return SkiaLayerFactory::createLayer (classID);
	default : return nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeBitmap* WasmSkiaEngine::createScreenshotFromWindow (Window* window)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* WasmSkiaEngine::createPrintJob ()
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INative3DSupport* WasmSkiaEngine::get3DSupport ()
{
	switch(getGraphicsBackend ())
	{
	case kWebGL : return &OpenGLES3DSupport::instance ();
	default : return nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GrRecordingContext* WasmSkiaEngine::getGPUContext ()
{
	return nullptr; // we can't provide a global context with WebGL
}
