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
// Filename    : ccl/public/system/iexecutable.h
// Description : DLL Interfaces
//
//************************************************************************************************

#ifndef _ccl_iexecutable_h
#define _ccl_iexecutable_h

#include "ccl/public/system/ithreading.h"

namespace CCL {

interface IAttributeList;

//************************************************************************************************
// IExecutableImage
/** Interface for dynamically loaded executables (Windows: DLL/EXE, OSX: Bundle with executable or .dylib).
	\ingroup ccl_system */
//************************************************************************************************

interface IExecutableImage: IUnknown
{
	/** Get path on disk. */
	virtual tbool CCL_API getPath (IUrl& path) const = 0;

	/** Get image identifier. */
	virtual tbool CCL_API getIdentifier (String& id) const = 0;

	/** Get native executable reference (Windows: HMODULE, Mac/iOS: CFBundleRef). */
	virtual ModuleRef CCL_API getNativeReference () const = 0;

	/** Retrieve address of exported function by name. */
	virtual void* CCL_API getFunctionPointer (CStringPtr name) const = 0;

	/** Get meta information (optional). */
	virtual const IAttributeList* CCL_API getMetaInfo () const = 0;
	
	/** Get path of binary file of executable (can be different from getPath() for Mac/iOS bundles). */
	virtual tbool CCL_API getBinaryPath (IUrl& path) const = 0;

	DECLARE_IID (IExecutableImage)
};

DEFINE_IID (IExecutableImage, 0x46fb66f3, 0x71b7, 0x43aa, 0xa9, 0x6b, 0xfa, 0x18, 0x9c, 0x7d, 0xc3, 0xb3)

//************************************************************************************************
// IExecutableIterator
/** Interface for iterating loaded executables in address space.
	\ingroup ccl_system */
//************************************************************************************************

interface IExecutableIterator: IUnknown
{
	/**	Get next image, returns null when iteration finished. 
		IMPORTANT: You *must not* keep a reference on the object returned!
		It is only valid until iteration is advanced to next image. */
	virtual const IExecutableImage* CCL_API getNextImage () = 0;
	
	DECLARE_IID (IExecutableIterator)
};

DEFINE_IID (IExecutableIterator, 0x329ef88a, 0x6d73, 0x4f6f, 0xb4, 0xb5, 0x81, 0x2d, 0xcc, 0xcd, 0x57, 0xea)

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Constants
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace System
{
	/** Flags used with IExecutableLoader::execute(). 
		\ingroup ccl_system */
	enum ExecutionFlags
	{
		kSuppressProcessGUI = 1<<0,		///< suppress GUI of executed process 
		kWaitForProcessExit = 1<<1,		///< wait until process exits, and return its exit code
		kRedirectProcessOutput = 1<<2	///< redirect process output to file passed via context argument
	};
}

//************************************************************************************************
// IExecutableLoader
/** Management functions for dynmanically loaded executables. 
	\ingroup ccl_system */
//************************************************************************************************

interface IExecutableLoader: IUnknown
{	
	/** Get image object of main executable. */
	virtual const IExecutableImage& CCL_API getMainImage () = 0;

	/** Map executable into address space. It will be unmapped when calling release() on the image object. */
	virtual tresult CCL_API loadImage (IExecutableImage*& image, UrlRef path) = 0;

	/** Create image object for given module reference (must be released by caller!) */
	virtual IExecutableImage* CCL_API createImage (ModuleRef module) = 0;

	/** Create iterator of loaded executables. */
	virtual IExecutableIterator* CCL_API createIterator () = 0;

	/** Register module reference (already loaded). */
	virtual void CCL_API addNativeImage (ModuleRef module) = 0;

	/** Unregister module reference (no unload). */
	virtual void CCL_API removeNativeImage (ModuleRef module) = 0;

	/** Executes another program with given arguments and options (see System::ExecutionFlags). */
	virtual tresult CCL_API execute (Threading::ProcessID& processId, UrlRef path, ArgsRef args,
									 int flags = 0, IUnknown* context = nullptr) = 0;

	/** Start new instance of main executable. */
	virtual tresult CCL_API relaunch (ArgsRef args) = 0;

	/** Terminate process with given identifier. */
	virtual tresult CCL_API terminate (Threading::ProcessID processId) = 0;

	/** Get the path to the main module of a process. */
	virtual tresult CCL_API getExecutablePath (IUrl& path, Threading::ProcessID processId) = 0;
	
	/** Determine if a process is currently running. */
	virtual tbool CCL_API isProcessRunning (UrlRef executableFile) = 0;
	
	/** Get platform version information for module. */
	virtual tresult CCL_API getModuleInfo (IAttributeList& attributes, UrlRef path) = 0;
	
	DECLARE_IID (IExecutableLoader)
};

DEFINE_IID (IExecutableLoader, 0xa5c4e43c, 0x26eb, 0x4b50, 0xa2, 0x84, 0xf4, 0x83, 0x30, 0x25, 0x3e, 0x48)

} // namespace CCL

#endif // _ccl_iexecutable_h
