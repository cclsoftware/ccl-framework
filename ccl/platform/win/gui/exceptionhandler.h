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
// Filename    : ccl/platform/win/gui/exceptionhandler.h
// Description : Win32 Exception Handler
//
//************************************************************************************************

#define EXCEPTION_HANDLER_ENABLED RELEASE

#ifndef _ccl_exceptionhandler_h
#define _ccl_exceptionhandler_h

#include "ccl/base/singleton.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/signalsource.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/system/idiagnosticdataprovider.h"

#include "ccl/platform/win/cclwindows.h"

namespace CCL {
class LogBuffer;

namespace Win32 {

//************************************************************************************************
// ExceptionHandler macros
//************************************************************************************************

#if EXCEPTION_HANDLER_ENABLED
	#define TRY_MESSAGE		__try
	#define EXCEPT_MESSAGE	__except (CCL::Win32::ExceptionHandler::handleGUIException (GetExceptionInformation ()))
#else
	#define TRY_MESSAGE
	#define EXCEPT_MESSAGE
#endif

//************************************************************************************************
// ExceptionHandler
//************************************************************************************************

class ExceptionHandler: public Unknown,
						public IDiagnosticDataProvider,
						public UnmanagedSingleton<ExceptionHandler>
{
public:
	ExceptionHandler ();
	~ExceptionHandler ();

	void install ();
	void uninstall ();

	static void test ();	///< simulate crash
	static LONG WINAPI handleGUIException (LPEXCEPTION_POINTERS exceptionInfo);
	static LONG WINAPI handleException (LPEXCEPTION_POINTERS exceptionInfo);
	
	// IDiagnosticDataProvider
	int CCL_API countDiagnosticData () const override;
	tbool CCL_API getDiagnosticDescription (DiagnosticDescription& description, int index) const override;
	IStream* CCL_API createDiagnosticData (int index) override;

	CLASS_INTERFACE (IDiagnosticDataProvider, Unknown)

protected:
	Url miniDumpPath;
	NativePath nativeDumpPath;
	String dumpFilePrefix;
	uchar* timestampPtr;
	DWORD mainThreadId;
	String messageText;
	MemoryStream xmlStream;
	MemoryStream appStream;
	static ExceptionHandler* theInstance;
	LPTOP_LEVEL_EXCEPTION_FILTER previousFilter;
	SignalSource signalSource;
	LogBuffer& logBuffer;

	Vector<Url> savedDumps;
	bool savedDumpsValid;

	bool createMiniDump (LPEXCEPTION_POINTERS exceptionInfo);
	static void catchExceptionsInTimers (bool catchExceptions);
	void scanDumps ();
};

} // namespace Win32
} // namespace CCL

#endif // _ccl_exceptionhandler_h
