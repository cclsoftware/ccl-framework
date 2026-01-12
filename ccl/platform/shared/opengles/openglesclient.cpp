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
// Filename    : ccl/platform/shared/opengles/openglesclient.cpp
// Description : OpenGL ES Client Context
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/shared/opengles/openglesclient.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

using namespace CCL;

//************************************************************************************************
// OpenGLESClient
//************************************************************************************************

const CStringPtr OpenGLESClient::kRequiredGLExtensions[] = 
{
	"GL_OES_required_internalformat",
	"GL_EXT_texture_format_BGRA8888"
};

//////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLESClient::OpenGLESClient ()
: initialized (false),
  display (EGL_NO_DISPLAY),
  config (nullptr),
  context (EGL_NO_CONTEXT)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLESClient::~OpenGLESClient ()
{
	terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLESClient::initialize (EGLNativeDisplayType nativeDisplay)
{
	ASSERT (display == EGL_NO_DISPLAY)
	
	display = eglGetDisplay (nativeDisplay);
	if(display == EGL_NO_DISPLAY)
	{
		CCL_WARN ("Failed to initialize an EGL display: %s\n", getErrorString ());
		return;
	}
	
	EGLint majorVersion = 0;
	EGLint minorVersion = 0;
	if(!eglInitialize (display, &majorVersion, &minorVersion))
	{
		CCL_WARN ("Failed to initialize EGL: %s\n", getErrorString ());
		return;
	}
	
	if(!eglBindAPI (EGL_OPENGL_ES_API))
	{
		CCL_WARN ("Failed to bind OpenGL ES API: %s\n", getErrorString ());
		return;
	}

	int configCount = 0;
	if(eglGetConfigs (display, nullptr, 0, &configCount) != EGL_TRUE || configCount == 0)
	{
		CCL_WARN ("%s\n", "No EGL configurations available");
		return;
	}

	EGLint eglAttributes[] =
	{
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 0,
		EGL_STENCIL_SIZE, 0,
		EGL_SAMPLES, 0,
		EGL_NONE
	};

	int count = configCount;
	if(eglChooseConfig (display, eglAttributes, &config, 1, &count) != EGL_TRUE || count != 1)
	{
		CCL_WARN ("Failed to set EGL configuration: %s\n", getErrorString ());
		return;
	}
	
	EGLint contextAttributes[] = 
	{ 
		EGL_CONTEXT_CLIENT_VERSION, 2, 
		EGL_NONE, EGL_NONE 
	};

	context = eglCreateContext (display, config, EGL_NO_CONTEXT, contextAttributes);
	if(context == EGL_NO_CONTEXT)
	{
		CCL_WARN ("%s\n", "Failed to initialize an EGL context");
		return;
	}
	eglMakeCurrent (display, nullptr, nullptr, context);
	
	clientExtensions = eglQueryString (nullptr, EGL_EXTENSIONS);
	displayExtensions = eglQueryString (display, EGL_EXTENSIONS);
	glExtensions = reinterpret_cast<CStringPtr> (glGetString (GL_EXTENSIONS));

	CCL_PRINTF ("EGL client extensions: %s\n", clientExtensions.str ())
	CCL_PRINTF ("EGL display extensions: %s\n", displayExtensions.str ())
	CCL_PRINTF ("GL extensions: %s\n", glExtensions.str ())

	for(CStringPtr requiredExtension : kRequiredGLExtensions)
	{
		if(!isGLExtensionSupported (requiredExtension))
		{
			CCL_WARN ("Missing required GL extension: %s\n", requiredExtension)
			terminate ();
			return;
		}
	}

	initialized = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLESClient::terminate ()
{
	gpuContext = nullptr;

	if(display != EGL_NO_DISPLAY)
	{
		if(context != EGL_NO_CONTEXT)
			eglDestroyContext (display, context);
		eglTerminate (display);
	}
	terminatePlatform ();
	initialized = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OpenGLESClient::isSupported ()
{
	static bool first = true;
	static bool result = false;
	
	if(first)
	{
		if(!initialized)
			initializePlatform ();
		first = false;
	
		result = isInitialized ();

		if(result == false)
			CCL_WARN ("%s\n", "OpenGL ES is not supported!")
	}
		
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OpenGLESClient::isEGLExtensionSupported (CStringPtr extensionName) const
{
	ForEachCStringToken (displayExtensions, " ", extension)
		if(CString (extension) == extensionName)
			return true;
	EndFor
	ForEachCStringToken (clientExtensions, " ", extension)
		if(CString (extension) == extensionName)
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OpenGLESClient::isGLExtensionSupported (CStringPtr extensionName) const
{
	ForEachCStringToken (glExtensions, " ", extension)
		if(CString (extension) == extensionName)
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr OpenGLESClient::getErrorString ()
{
	switch(eglGetError ())
	{
	case EGL_SUCCESS : return "No error";
	case EGL_NOT_INITIALIZED : return "EGL not initialized or failed to initialize";
	case EGL_BAD_ACCESS : return "Resource inaccessible";
	case EGL_BAD_ALLOC : return "Cannot allocate resources";
	case EGL_BAD_ATTRIBUTE : return "Unrecognized attribute or attribute value";
	case EGL_BAD_CONTEXT : return "Invalid EGL context";
	case EGL_BAD_CONFIG : return "Invalid EGL frame buffer configuration";
	case EGL_BAD_CURRENT_SURFACE : return "Current surface is no longer valid";
	case EGL_BAD_DISPLAY : return "Invalid EGL display";
	case EGL_BAD_SURFACE : return "Invalid surface";
	case EGL_BAD_MATCH : return "Inconsistent arguments";
	case EGL_BAD_PARAMETER : return "Invalid argument";
	case EGL_BAD_NATIVE_PIXMAP : return "Invalid native pixmap";
	case EGL_BAD_NATIVE_WINDOW : return "Invalid native window";
	case EGL_CONTEXT_LOST : return "Context lost";
	}
	return "Unknown error";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GrDirectContext* OpenGLESClient::getGPUContext ()
{
	if(gpuContext == nullptr)
	{
		if(!initialized || display == EGL_NO_DISPLAY || context == EGL_NO_CONTEXT)
			return nullptr;

		sk_sp<const GrGLInterface> nativeInterface = GrGLInterfaces::MakeEGL ();
		GrContextOptions contextOptions;
		#if DEBUG
		contextOptions.fSkipGLErrorChecks = GrContextOptions::Enable::kNo;
		#endif
		gpuContext = GrDirectContexts::MakeGL (nativeInterface, contextOptions);
		if(gpuContext == nullptr)
			CCL_WARN ("%s\n", "Failed to initialize Skia GL bindings");
	}

	return gpuContext.get ();
}
