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
// Filename    : ccl/platform/linux/gui/exceptionhandler.h
// Description : Linux Exception Handler
//
//************************************************************************************************

#ifndef _ccl_exceptionhandler_h
#define _ccl_exceptionhandler_h

#include "ccl/platform/shared/posix/gui/exceptionhandler.posix.h"

#include "ccl/base/singleton.h"

namespace CCL {
namespace Linux {

//************************************************************************************************
// LinuxExceptionHandler
//************************************************************************************************

class LinuxExceptionHandler: public PosixExceptionHandler,
							 public UnmanagedSingleton<LinuxExceptionHandler>
{
public:
	LinuxExceptionHandler ();
	~LinuxExceptionHandler ();

	static void handleSignal (int signal, siginfo_t* info, void* context);

	// IDiagnosticDataProvider
	void install () override;
	
protected:
	static LinuxExceptionHandler* theInstance;
	String applicationFileName;

	// PosixExceptionHandler
	void scanDumps () override;
	bool isValidDumpFile (UrlRef file) const override;
};

} // namespace Linux
} // namespace CCL

#endif // _ccl_exceptionhandler_h
