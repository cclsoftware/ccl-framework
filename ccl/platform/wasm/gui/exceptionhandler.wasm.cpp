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
// Filename    : ccl/platform/wasm/gui/exceptionhandler.wasm.cpp
// Description : WebAssembly Exception Handler
//
//************************************************************************************************

#include "ccl/platform/wasm/gui/exceptionhandler.wasm.h"

#include "ccl/gui/gui.h"

#include "ccl/public/gui/iapplication.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/isafetymanager.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/cclversion.h"

#include "core/text/coreutfcodec.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace CCL {
namespace Wasm {

//////////////////////////////////////////////////////////////////////////////////////////////////
// JavaScript Exception Hooks
//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS_DEPS (exceptionHandlerDependencies, "$dynCall,$stringToUTF8OnStack");

EM_JS (void, installJavaScriptExceptionHandlers, (void* handlerCallback),
{
	// Store the original abort handler so we can restore it later
	if(!Module['__originalAbort'] && Module['onAbort'])
		Module['__originalAbort'] = Module['onAbort'];

	// Override Emscripten's abort handler
	Module['onAbort'] = function (reason)
	{
		console.error ("CCL: Emscripten abort:", reason);

		// Module.onAbort runs synchronously, still nested in the call chain that aborted, so the
		// current stack still contains the crash frames here (unlike the two listeners below).
		const stack = new Error ().stack || "";
		console.error ("Abort stack:", stack);

		// Call CCL exception handler
		const reasonStr = stringToUTF8OnStack (reason || "Unknown abort");
		const stackStr = stringToUTF8OnStack (stack);
		dynCall ('viii', handlerCallback, [0, reasonStr, stackStr]);

		// Chain to original handler if it existed
		if(Module['__originalAbort'])
			Module['__originalAbort'] (reason);
	};

	// Install global error handler for WASM traps
	window.addEventListener ('error', function(event)
	{
		if(event.error && event.error instanceof WebAssembly.RuntimeError)
		{
			// Must use the stack captured by the browser at the throw site (event.error.stack):
			// by the time this listener runs, the original call stack has already unwound, so a
			// fresh `new Error()` here would not contain the crashing frames.
			const stack = event.error.stack || "";
			console.error ("CCL: WebAssembly trap:", event.error.message);
			console.error ("Trap stack:", stack);

			// Report the trap
			const reasonStr = stringToUTF8OnStack ("WASM trap: " + event.error.message);
			const stackStr = stringToUTF8OnStack (stack);
			dynCall ('viii', handlerCallback, [0, reasonStr, stackStr]);
		}
	});

	// Install unhandled promise rejection handler
	window.addEventListener ('unhandledrejection', function(event)
	{
		if(event.reason instanceof WebAssembly.RuntimeError)
		{
			// Same reasoning as the 'error' listener above: use the stack captured at the throw
			// site (event.reason.stack), not a freshly captured one.
			const stack = event.reason.stack || "";
			console.error ("CCL: Unhandled WASM rejection:", event.reason);

			const reasonStr = stringToUTF8OnStack ("WASM rejection: " + event.reason.message);
			const stackStr = stringToUTF8OnStack (stack);
			dynCall ('viii', handlerCallback, [0, reasonStr, stackStr]);
		}
	});
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, removeJavaScriptExceptionHandlers, (),
{
	// Restore original abort handler
	if(Module['__originalAbort'])
	{
		Module['onAbort'] = Module['__originalAbort'];
		delete Module['__originalAbort'];
	}
});

} // namespace Wasm
} // namespace CCL

using namespace CCL;
using namespace Wasm;

//************************************************************************************************
// WasmExceptionHandler
//************************************************************************************************

DEFINE_UNMANAGED_SINGLETON (WasmExceptionHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

static const CString kCrashLogPrefix = "CrashLog";
static const int kTimestampLength = 18; // "YYYYMMDD_HHMMSSmmm"

//////////////////////////////////////////////////////////////////////////////////////////////////

// Fixed-width, allocation-free timestamp formatting - shared by install() (to size/locate the
// placeholder) and writeCrashLog() (to patch it in place at crash time).
static void makeTimestamp (char* buffer, int bufferSize)
{
	DateTime now;
	System::GetSystem ().getLocalTime (now);
	const Date& date = now.getDate ();
	const Time& time = now.getTime ();

	char formatted[kTimestampLength + 1] = "";
	::snprintf (formatted, sizeof(formatted), "%04d%02d%02d_%02d%02d%02d%03d",
		date.getYear (), date.getMonth (), date.getDay (),
		time.getHour (), time.getMinute (), time.getSecond (), time.getMilliseconds ());

	::memcpy (buffer, formatted, ccl_min (bufferSize, kTimestampLength));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

// Raw write() - no CCL stream/string allocation - since this runs at crash time.
static void writeCrashLogText (int fd, CStringPtr text)
{
	::write (fd, text, strlen (text));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmExceptionHandler::WasmExceptionHandler ()
: crashLogTimestampPtr (nullptr),
  crashLogsValid (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmExceptionHandler::~WasmExceptionHandler ()
{
	uninstall ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmExceptionHandler::install ()
{
	System::GetSystem ().getLocation (crashLogFolder, System::kAppSettingsPlatformFolder);

	updateDumpFileName ();

	// Install JavaScript exception handlers
	// This installs:
	// - Module.onAbort - Captures Emscripten aborts (assertions, explicit aborts)
	// - window.error - Captures WebAssembly traps (out of bounds, divide by zero, unreachable)
	// - window.unhandledrejection - Captures async WASM errors
	installJavaScriptExceptionHandlers ((void*)&WasmExceptionHandler::handleException);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmExceptionHandler::uninstall ()
{
	removeJavaScriptExceptionHandlers ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmExceptionHandler::updateDumpFileName ()
{
	// Precompute the full crash log path (with a placeholder timestamp) once, here, outside of
	// any crash, so writeCrashLog() only has to patch the timestamp digits in place and
	// open/write/close the file - no String/Url allocation while the heap might already be
	// corrupted by the crash itself (mirrors Win32::ExceptionHandler::install()'s nativeDumpPath/
	// timestampPtr and its preallocated buffers).
	char timestamp[kTimestampLength + 1] = "";
	makeTimestamp (timestamp, sizeof(timestamp));

	crashLogFileName = kCrashLogPrefix;
	crashLogFileName.append ("_");
	crashLogFileName.append (timestamp);

	if(IApplication* app = GUI.getApplication ())
	{
		crashLogFileName.append ("_");
		crashLogFileName.append (app->getApplicationID ());
	}

	crashLogFileName.append ("_");
	crashLogFileName.append (STRINGIFY (CCL_VERSION_BUILD));

	crashLogFileName.append (".log");

	Url crashLogFile (crashLogFolder);
	crashLogFile.descend (String (Text::kUTF8, crashLogFileName), IUrl::kFile);
	crashLogFile.toPOSIXPath (crashLogPath, crashLogPath.size ());

	crashLogTimestampPtr = crashLogPath.path + CString (crashLogPath).index (timestamp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmExceptionHandler::handleException (void* crashSite, CStringPtr reason, CStringPtr stack)
{
	WasmExceptionHandler& instance = WasmExceptionHandler::instance ();

	// Write our own crash log before reporting, so its path can be passed on as the crash's system dump file
	bool wroteLog = instance.writeCrashLog (reason, stack);

	// Report to SafetyManager for module identification and logging
	System::GetSafetyManager ().reportException (crashSite, wroteLog ? instance.crashLogNativePath.path : nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmExceptionHandler::writeCrashLog (CStringPtr reason,CStringPtr stack)
{
	// Patch the timestamp digits in place - no String/Url allocation here, since we may already
	// be in a crash with a corrupted heap.
	makeTimestamp (crashLogTimestampPtr, kTimestampLength + 1);

	int fd = ::open (crashLogPath.path, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
	if(fd < 0)
		return false;

	writeCrashLogText (fd, "File: ");
	writeCrashLogText (fd, crashLogFileName);
	writeCrashLogText (fd, "\n\nReason: ");
	writeCrashLogText (fd, (reason && reason[0]) ? reason : "(unknown)");
	writeCrashLogText (fd, "\n\nStack trace:\n");
	writeCrashLogText (fd, (stack && stack[0]) ? stack : "(unavailable)");
	writeCrashLogText (fd, "\n");

	::close (fd);

	// ISafetyManager::reportException() expects a wide (UStringPtr) path; decodeUTF8String() only
	// writes into the buffer we already own, so this stays allocation-free too.
	Core::Text::UTFFunctions::decodeUTF8String (crashLogNativePath.path, sizeof(crashLogNativePath.path), crashLogPath.path, (int)::strlen (crashLogPath.path));

	crashLogsValid = false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmExceptionHandler::scanCrashLogs () const
{
	if(crashLogsValid)
		return;

	savedCrashLogs.removeAll ();

	FileInfo info;
	int64 now = UnixTime::getTime ();
	ForEachFile (System::GetFileSystem ().newIterator (crashLogFolder, IFileIterator::kFiles), file)
		if(!isCrashLogFile (*file))
			continue;
		if(System::GetFileSystem ().getFileInfo (info, *file) && UnixTime::fromLocal (info.modifiedTime) > now - 7 * DateTime::kSecondsInDay)
			savedCrashLogs.add (*file);
		if(savedCrashLogs.count () >= 10)
			break;
	EndFor

	crashLogsValid = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmExceptionHandler::isCrashLogFile (UrlRef file) const
{
	String fileName;
	file.getName (fileName);
	return fileName.startsWith (String (Text::kUTF8, kCrashLogPrefix));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API WasmExceptionHandler::countDiagnosticData () const
{
	scanCrashLogs ();
	return savedCrashLogs.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WasmExceptionHandler::getDiagnosticDescription (DiagnosticDescription& description, int index) const
{
	scanCrashLogs ();

	if(index >= 0 && index < savedCrashLogs.count ())
	{
		description.categoryFlags = DiagnosticDescription::kErrorInformation;
		savedCrashLogs[index].getName (description.fileName);
		description.subFolder = "Dumps";
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* CCL_API WasmExceptionHandler::createDiagnosticData (int index)
{
	scanCrashLogs ();

	if(index >= 0 && index < savedCrashLogs.count ())
	{
		return System::GetFileSystem ().openStream (savedCrashLogs[index], IStream::kOpenMode | IStream::kShareRead);
	}
	return nullptr;
}
