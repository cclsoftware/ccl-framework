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
// Filename    : ccl/platform/wasm/skia/skiaengine.wasm.h
// Description : WebAssembly Skia Engine
//
//************************************************************************************************

#ifndef _ccl_skia_engine_wasm_h
#define _ccl_skia_engine_wasm_h

#include "ccl/platform/shared/skia/skiaengine.h"

namespace CCL {
class Native3DGraphicsFactory;

//************************************************************************************************
// WasmSkiaEngine
//************************************************************************************************

class WasmSkiaEngine: public SkiaEngine
{
public:
	DECLARE_CLASS_ABSTRACT (WasmSkiaEngine, SkiaEngine)

	enum GraphicsBackendType
	{
		kSoftware,
		kWebGL
	};

	static WasmSkiaEngine* getInstance ();

	GraphicsBackendType getGraphicsBackend () const;
	Native3DGraphicsFactory* create3DGraphicsFactory ();

	// SkiaEngine
	bool hasGraphicsLayers () override;
	IGraphicsLayer* createGraphicsLayer (UIDRef classID) override;
	NativeBitmap* createScreenshotFromWindow (Window* window) override;
	Object* createPrintJob () override;
	INative3DSupport* get3DSupport () override;
	GrRecordingContext* getGPUContext () override;
};

} // namespace CCL

#endif // _ccl_skia_engine_wasm_h
