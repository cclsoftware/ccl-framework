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
// Filename    : ccl/platform/shared/posix/gui/exceptionhandler.posix.h
// Description : POSIX Exception Handler
//
//************************************************************************************************

#define EXCEPTION_HANDLER_ENABLED RELEASE

#ifndef _ccl_exceptionhandler_posix_h
#define _ccl_exceptionhandler_posix_h

#include "ccl/base/storage/url.h"

#include "ccl/public/system/idiagnosticdataprovider.h"

#include <signal.h>

namespace CCL {

//************************************************************************************************
// PosixExceptionHandler
//************************************************************************************************

class PosixExceptionHandler: public Unknown,
							 public IDiagnosticDataProvider
{
public:
	PosixExceptionHandler ();

	virtual void install ();
	virtual void uninstall ();

	// IDiagnosticDataProvider
	int CCL_API countDiagnosticData () const override;
	tbool CCL_API getDiagnosticDescription (DiagnosticDescription& description, int index) const override;
	IStream* CCL_API createDiagnosticData (int index) override;

	CLASS_INTERFACE (IDiagnosticDataProvider, Unknown)
	
protected:
	struct sigaction previousSigSegVHandler;
	struct sigaction previousSigAbrtHandler;
	struct sigaction previousSigBusHandler;
	struct sigaction previousSigIllHandler;
	struct sigaction previousSigTrapHandler;

	void (*signalHandler) (int, siginfo_t*, void*);

	Vector<Url> savedDumps;
	bool savedDumpsValid;
	
	virtual bool isValidDumpFile (UrlRef file) const { return false; }
	virtual void scanDumps () {}
	virtual void scanDumpFolder (UrlRef path);
};

} // namespace CCL

#endif // _ccl_exceptionhandler_posix_h
