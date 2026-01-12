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
// Filename    : ccl/platform/shared/opengles/openglesclient.h
// Description : OpenGL ES Client Context
//
//************************************************************************************************

#ifndef _openglesclient_h
#define _openglesclient_h

#include "ccl/platform/shared/skia/skiaglue.h"

#include "ccl/base/singleton.h"

#include "ccl/public/text/cstring.h"

#if CCL_PLATFORM_LINUX
#include "ccl/public/base/ccldefpush.h"
#include <wayland-egl.h> // must be included before EGL headers
#include "ccl/public/base/ccldefpop.h"
#endif

#include <EGL/egl.h>

namespace CCL {

//************************************************************************************************
// OpenGLESClient
//************************************************************************************************

class OpenGLESClient: public Object,
					  public ExternalSingleton<OpenGLESClient>
{
public:
	OpenGLESClient ();
	~OpenGLESClient ();
	
	void initialize (EGLNativeDisplayType nativeDisplay);
	void terminate ();
	bool isInitialized () const { return initialized; }
	
	bool isSupported ();

	EGLDisplay getDisplay () const { return display; }
	EGLConfig getConfig () const { return config; }
	EGLContext getContext () const { return context; }
	bool isGLExtensionSupported (CStringPtr extensionName) const;
	bool isEGLExtensionSupported (CStringPtr extensionName) const;

	static CStringPtr getErrorString ();

	GrDirectContext* getGPUContext ();

protected:
	static const CStringPtr kRequiredGLExtensions[];

	bool initialized;
	
	sk_sp<GrDirectContext> gpuContext;

	EGLDisplay display;
	EGLConfig config;
	EGLContext context;

	MutableCString clientExtensions;
	MutableCString displayExtensions;
	MutableCString glExtensions;

	virtual void initializePlatform () {};
	virtual void terminatePlatform () {};
};

} // namespace CCL

#endif // _openglesclient_h

