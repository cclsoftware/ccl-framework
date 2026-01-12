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
// Filename    : ccl/platform/cocoa/gui/exceptionhandler.h
// Description : Cocoa Exception Handler
//
//************************************************************************************************

#define EXCEPTION_HANDLER_ENABLED RELEASE

#ifndef _ccl_exceptionhandler_h
#define _ccl_exceptionhandler_h

#include "ccl/platform/shared/posix/gui/exceptionhandler.posix.h"

#include "ccl/base/singleton.h"

namespace CCL {
namespace MacOS {

//************************************************************************************************
// CocoaExceptionHandler
//************************************************************************************************

class CocoaExceptionHandler: public PosixExceptionHandler,
							 public UnmanagedSingleton<CocoaExceptionHandler>
{
public:
	CocoaExceptionHandler ();
	~CocoaExceptionHandler ();

	static void handleSignal (int signal, siginfo_t* info, void* context);

	// PosixExceptionHandler
	void install () override;
	
protected:
	static CocoaExceptionHandler* theInstance;
	NativePath nativeDumpPath;
	String dumpFilePrefix;

	// PosixExceptionHandler
	void scanDumps () override;
	bool isValidDumpFile (UrlRef file) const override;
};

} // namespace MacOS
} // namespace CCL

#endif // _ccl_exceptionhandler_h
