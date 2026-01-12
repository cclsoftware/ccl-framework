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
// Filename    : ccl/platform/linux/skia/skiaengine.linux.h
// Description : Linux Skia Engine
//
//************************************************************************************************

#ifndef _ccl_skia_engine_linux_h
#define _ccl_skia_engine_linux_h

#include "ccl/platform/shared/skia/skiaengine.h"

namespace CCL {
class Native3DGraphicsFactory;

//************************************************************************************************
// LinuxSkiaEngine
//************************************************************************************************

class LinuxSkiaEngine: public SkiaEngine
{
public:
	DECLARE_CLASS_ABSTRACT (LinuxSkiaEngine, SkiaEngine)

	enum GraphicsBackendType
	{
		kSoftware,
		kOpenGLES2,
		kVulkan
	};

	static LinuxSkiaEngine* getInstance ();

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

#endif // _ccl_skia_engine_cocoa_h
