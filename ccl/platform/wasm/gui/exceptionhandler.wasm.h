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
// Filename    : ccl/platform/wasm/gui/exceptionhandler.wasm.h
// Description : WebAssembly Exception Handler
//
//************************************************************************************************

#ifndef _ccl_wasm_exceptionhandler_h
#define _ccl_wasm_exceptionhandler_h

#include "ccl/base/singleton.h"
#include "ccl/base/storage/url.h"
#include "ccl/public/text/cstring.h"

#include "ccl/public/system/idiagnosticdataprovider.h"

#define EXCEPTION_HANDLER_ENABLED RELEASE

namespace CCL {
namespace Wasm {

//************************************************************************************************
// WasmExceptionHandler
//************************************************************************************************

class WasmExceptionHandler: public Object,
							public IDiagnosticDataProvider,
							public UnmanagedSingleton<WasmExceptionHandler>
{
public:
	WasmExceptionHandler ();
	~WasmExceptionHandler ();

	void install ();
	void uninstall ();

	void updateDumpFileName ();

	// Exception handler callback (called from JavaScript)
	static void handleException (void* crashSite, CStringPtr reason, CStringPtr stack);

	// IDiagnosticDataProvider
	int CCL_API countDiagnosticData () const override;
	tbool CCL_API getDiagnosticDescription (DiagnosticDescription& description, int index) const override;
	IStream* CCL_API createDiagnosticData (int index) override;

	CLASS_INTERFACE (IDiagnosticDataProvider, Object)

protected:
	Url crashLogFolder;
	MutableCString crashLogFileName;
	POSIXPath crashLogPath;
	char* crashLogTimestampPtr;
	NativePath crashLogNativePath;

	mutable Vector<Url> savedCrashLogs;
	mutable bool crashLogsValid;

	bool writeCrashLog (CStringPtr reason, CStringPtr stack);
	void scanCrashLogs () const;
	bool isCrashLogFile (UrlRef file) const;
};

} // namespace Wasm
} // namespace CCL

#endif // _ccl_wasm_exceptionhandler_h
