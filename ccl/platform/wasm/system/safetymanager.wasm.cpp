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
// Filename    : ccl/platform/wasm/system/safetymanager.wasm.cpp
// Description : WebAssembly Safety Manager
//
//************************************************************************************************

#include "ccl/system/safetymanager.h"

#include "ccl/public/storage/iurl.h"

#include "core/text/coreutfcodec.h"

#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>

namespace CCL {
namespace Wasm {

//////////////////////////////////////////////////////////////////////////////////////////////////
// JS Helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS_DEPS (safetyManagerDependencies, "$dynCall,$stringToUTF8OnStack");

EM_JS (int, walkStackAndReportModules, (CStringPtr stack, void* reportModuleCallback),
{
	try
	{
		const lines = UTF8ToString (stack).split ('\n');

		// Track unique modules we've reported
		const reportedModules = new Set ();

		// Parse stack lines to extract function locations
		for(const line of lines)
		{
			// Skip the first line (usually "Error")
			if(line.trim () === "" || line.startsWith ('Error'))
				continue;

			// Stack line format varies by browser:
			// Chrome/Edge: "    at functionName (file.wasm:0x1234)"
			// Firefox:     "    functionName@file.wasm:line:col"
			// Safari:      "    functionName@file.wasm:line:col"

			let modulePath = null;

			// Try Chrome/V8 format
			const chromeMatch = line.match (/\\(([^)]+\\.wasm)[:\\s]/);
			if(chromeMatch)
				modulePath = chromeMatch[1];

			// Try Firefox/Safari format
			if(!modulePath)
			{
				const firefoxMatch = line.match (/@([^@]+\\.wasm)/);
				if(firefoxMatch)
					modulePath = firefoxMatch[1];
			}

			// If we found a module path and haven't reported it yet
			if(modulePath && !reportedModules.has (modulePath))
			{
				reportedModules.add (modulePath);

				// Call the C++ callback with this module path
				const pathStr = stringToUTF8OnStack (modulePath);
				dynCall ('vi', reportModuleCallback, [pathStr]);
			}
		}

		return reportedModules.size;
	}
	catch(e)
	{
		console.error ("Failed to walk stack:", e);
		return 0;
	}
});

} // namespace Wasm

//************************************************************************************************
// WasmSafetyManager
//************************************************************************************************

class WasmSafetyManager: public SafetyManager
{
public:
	WasmSafetyManager ();

	static void handleModuleFromStack (CStringPtr modulePath);

protected:
	POSIXPath modulePath;
	FixedSizeVector<POSIXPath, 10> modulePaths;

	char crashLogText[CORE_STRING_STACK_MAX];
	
	CStringPtr readCrashLogStackTrace (UStringPtr systemDumpFile);

	// SafetyManager
	void enableCrashRecovery (bool state) override;
	void CCL_API reportException (void* exceptionInformation, UStringPtr systemDumpFile) override;
};

} // namespace CCL

using namespace CCL;
using namespace Wasm;

//************************************************************************************************
// WasmSafetyManager
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (SafetyManager, WasmSafetyManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmSafetyManager::WasmSafetyManager ()
: crashLogText ("")
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmSafetyManager::handleModuleFromStack (CStringPtr modulePath)
{
	WasmSafetyManager& instance = static_cast<WasmSafetyManager&> (SafetyManager::instance ());

	// Store module path in buffer if there's space
	int count = instance.modulePaths.count ();
	if(count < instance.modulePaths.getCapacity ())
	{
		POSIXPath& path = instance.modulePaths[count];
		CString (modulePath).copyTo (path.path, path.size ());

		if(path.path[0] != '\0')
			instance.modulePaths.setCount (count + 1);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmSafetyManager::enableCrashRecovery (bool state)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr WasmSafetyManager::readCrashLogStackTrace (UStringPtr systemDumpFile)
{
	crashLogText[0] = '\0';
	if(!systemDumpFile)
		return crashLogText;

	int wideLength = 0;
	while(systemDumpFile[wideLength] != 0)
		wideLength++;

	char narrowPath[IUrl::kMaxLength];
	Core::Text::UTFFunctions::encodeUTF8String (narrowPath, sizeof(narrowPath), systemDumpFile, wideLength);

	int fd = ::open (narrowPath, O_RDONLY);
	if(fd < 0)
		return crashLogText;

	int bytesRead = (int)::read (fd, crashLogText, sizeof(crashLogText) - 1);
	::close (fd);
	crashLogText[bytesRead > 0 ? bytesRead : 0] = '\0';
	CString crashLog (crashLogText);

	static const char kStackTraceMarker[] = "Stack trace:\n";
	int position = crashLog.index (kStackTraceMarker);
	return position >= 0 ? crashLogText + position + (ARRAY_COUNT (kStackTraceMarker) - 1) : crashLogText;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmSafetyManager::reportException (void* exceptionInformation, UStringPtr systemDumpFile)
{
	modulePath.path[0] = '\0';
	modulePaths.setCount (0);

	void* crashSite = exceptionInformation;

	// Best-effort: resolve the crashing module via dladdr if a real native address was supplied.
	// No current caller has one (the browser exposes none to C++ for a WASM trap or abort), so
	// this only matters for a hypothetical future caller that does.
	if(crashSite)
	{
		Dl_info info;
		if(dladdr (crashSite, &info) != 0 && info.dli_fname)
			CString (info.dli_fname).copyTo (modulePath.path, modulePath.size ());
	}

	// Read the stack trace back out of the crash log WasmExceptionHandler already wrote (its path
	// is handed to us as systemDumpFile) and walk it for ".wasm" frames. For SIDE_MODULE=2 with
	// dynamic linking, it may reference multiple modules; the first one found is the crashing
	// module if dladdr didn't already resolve one, the rest are its callers.
	CStringPtr stackText = readCrashLogStackTrace (systemDumpFile);
	walkStackAndReportModules (stackText, (void*)&WasmSafetyManager::handleModuleFromStack);

	if(modulePath.path[0] == '\0' && !modulePaths.isEmpty ())
		modulePath = modulePaths[0];

	if(modulePath.path[0] != '\0')
	{
		// reportCrash()/reportCallingModule() are the cross-platform SafetyManager interface and
		// need a wide (UStringPtr) path; convert only here, at the boundary. These are local,
		// stack-allocated fixed buffers, not heap allocations.
		NativePath crashingModule;
		Core::Text::UTFFunctions::decodeUTF8String (crashingModule.path, sizeof(crashingModule.path), modulePath.path, (int)strlen (modulePath.path));
		reportCrash (crashingModule.path, systemDumpFile);

		// Report the other modules found in the stack as calling modules
		for(int i = 0; i < modulePaths.count (); i++)
		{
			if(modulePaths[i].path[0] != '\0')
			{
				CString path (modulePaths[i].path);
				// Skip if this is the same as the crashing module
				if(path != modulePath.path)
				{
					NativePath callingModule;
					Core::Text::UTFFunctions::decodeUTF8String (callingModule.path, sizeof(callingModule.path), path, path.length ());
					reportCallingModule (callingModule.path);
				}
			}
		}
	}
	else if(systemDumpFile)
	{
		// Fallback: report crash even without module info
		reportCrash (nullptr, systemDumpFile);
	}
}
