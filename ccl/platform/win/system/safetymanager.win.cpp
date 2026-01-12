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
// Filename    : ccl/platform/win/system/safetymanager.win.cpp
// Description : Windows Safety Manager
//
//************************************************************************************************

#include "ccl/system/safetymanager.h"

#include "ccl/platform/win/cclwindows.h"
#include "ccl/public/storage/iurl.h"

#include <eh.h>
#include <dbghelp.h>

#include <exception>

#pragma comment (lib, "Dbghelp.lib")

#define STD_CPP_EXCEPTION_CODE 0xE06D7363

namespace CCL {

//************************************************************************************************
// SEHException
//************************************************************************************************

class SEHException: public std::exception
{
public:
	SEHException (LPEXCEPTION_POINTERS info)
	: exceptionInfo (info)
	{}

	LPEXCEPTION_POINTERS exceptionInfo;
};

//************************************************************************************************
// ScopedSETranslator
//************************************************************************************************

class ScopedSETranslator
{
public:
    ScopedSETranslator (_se_translator_function translator)
	: previousTranslator (_set_se_translator (translator))
	{}

    ~ScopedSETranslator ()
	{ 
		_set_se_translator (previousTranslator); 
	}
	
private:
    const _se_translator_function previousTranslator;
};

//************************************************************************************************
// WindowsSafetyManager
//************************************************************************************************

class WindowsSafetyManager: public SafetyManager
{
public:
	WindowsSafetyManager ();

	static DWORD WINAPI applicationRecoveryCallback (PVOID pvParameter);

	void onApplicationRecovery ();

protected:
	static const int kPingInterval = RECOVERY_DEFAULT_PING_INTERVAL;
	FixedSizeVector<NativePath, 3> modulePaths;
	FixedSizeVector<NativePath, 32> unstableModuleBuffer;
	Threading::CriticalSection exceptionLock;

	static bool findModuleCausingException (NativePath& modulePath, LPEXCEPTION_POINTERS exceptionInfo);
	static void translateException (unsigned int exceptionCode, LPEXCEPTION_POINTERS exceptionInfo);
	int findModulesInCallStack (NativePath modules[], int maxCount, LPEXCEPTION_POINTERS exceptionInfo);

	void reportUnexpectedBehavior (const uchar* modulePath);

	void flush ();

	// SafetyManager
	void enableCrashRecovery (bool state) override;
	void CCL_API reportException (void* exceptionInformation, const uchar* systemDumpFile) override;
	tbool CCL_API handleException () override;
	tresult CCL_API checkStability () override;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// WindowsSafetyManager
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (SafetyManager, WindowsSafetyManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsSafetyManager::WindowsSafetyManager ()
{	
	unstableModuleBuffer.setCount (unstableModuleBuffer.getCapacity ());
	for(auto& buffer : unstableModuleBuffer)
		buffer[0] = '\0';
}

/////////////////////////////////////////////////////////////////////////////////////////////////

DWORD WINAPI WindowsSafetyManager::applicationRecoveryCallback (PVOID pvParameter)
{
	static_cast<WindowsSafetyManager&> (SafetyManager::instance ()).onApplicationRecovery ();
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsSafetyManager::onApplicationRecovery ()
{
	// TODO
	// periodically (at least every kPingInterval milliseconds) call ApplicationRecoveryInProgress 
	// when finished call ApplicationRecoveryFinished
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsSafetyManager::enableCrashRecovery (bool state)
{
	if(state)
	{
		RegisterApplicationRestart (nullptr, RESTART_NO_PATCH | RESTART_NO_REBOOT);
		// RegisterApplicationRecoveryCallback (applicationRecoveryCallback, nullptr, kPingInterval, 0);
	}
	else
	{
		UnregisterApplicationRestart ();
		// UnregisterApplicationRecoveryCallback ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowsSafetyManager::reportException (void* exceptionInformation, const uchar* systemDumpFile)
{
	if(findModuleCausingException (modulePaths[0], static_cast<LPEXCEPTION_POINTERS> (exceptionInformation)) || systemDumpFile != nullptr)
	{
		reportCrash (modulePaths[0], systemDumpFile);
		int count = findModulesInCallStack (modulePaths.getItems (), modulePaths.getCapacity (), static_cast<LPEXCEPTION_POINTERS> (exceptionInformation));
		for(int i = 0; i < count; i++)
			if(modulePaths[i][0] != '\0')
				reportCallingModule (modulePaths[i]);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsSafetyManager::handleException ()
{
	try
	{
		// Temporarily set the SE translator function for this thread
		ScopedSETranslator translator (translateException);

		// Rethrow the exception to trigger the translation
		throw;
	}
	catch(const SEHException& exception)
	{
		if(exception.exceptionInfo && exception.exceptionInfo->ExceptionRecord)
			if(exception.exceptionInfo->ExceptionRecord->ExceptionCode != STD_CPP_EXCEPTION_CODE)
			{
				if(findModuleCausingException (modulePaths[0], exception.exceptionInfo))
				{
					reportUnexpectedBehavior (modulePaths[0]);
					int count = findModulesInCallStack (modulePaths.getItems (), modulePaths.getCapacity (), exception.exceptionInfo);
					for(int i = 0; i < count; i++)
						if(modulePaths[i][0] != '\0')
							reportUnexpectedBehavior (modulePaths[i]);
				}
				return true;
			}
	}
	catch(...)
	{}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WindowsSafetyManager::checkStability ()
{
	flush ();
	return SafetyManager::checkStability ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsSafetyManager::reportUnexpectedBehavior (const uchar* modulePath)
{
	{
		Threading::ScopedLock guard (exceptionLock);

		for(auto& buffer : unstableModuleBuffer)
		{
			if(::wcsncmp (buffer, modulePath, buffer.size ()) == 0)
				return;
			else if(buffer[0] == '\0' && ::wcslen (modulePath) < buffer.size ())
			{
				::wcscpy (buffer, modulePath);
				break;
			}
		}
	}

	if(System::IsInMainThread ())
		flush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowsSafetyManager::findModuleCausingException (NativePath& module, LPEXCEPTION_POINTERS exceptionInfo)
{
	module[0] = '\0';
	
	if(exceptionInfo == nullptr)
		return false;

	if(exceptionInfo->ContextRecord == nullptr)
		return false;
	
#if defined (CCL_PLATFORM_ARM)
	auto exceptionAddress = exceptionInfo->ContextRecord->Pc;
#elif defined (CCL_PLATFORM_INTEL) && defined (CCL_PLATFORM_64BIT)
	auto exceptionAddress = exceptionInfo->ContextRecord->Rip;
#else
	auto exceptionAddress = exceptionInfo->ContextRecord->Eip;
#endif

	HMODULE handle;
	if(GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)exceptionAddress, &handle) == 0)
		return false;

	if(GetModuleFileName (handle, module, module.size ()) > 0)
		return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL::WindowsSafetyManager::findModulesInCallStack (NativePath modules[], int maxCount, LPEXCEPTION_POINTERS exceptionInfo)
{
	for(int i = 0; i < maxCount; i++)
		modules[i][0] = '\0';
	
	if(exceptionInfo == nullptr)
		return 0;

	if(exceptionInfo->ContextRecord == nullptr)
		return 0;

#if defined (CCL_PLATFORM_ARM)
	DWORD imageType = IMAGE_FILE_MACHINE_ARM64;
	auto exceptionAddress = exceptionInfo->ContextRecord->Pc;
	auto exceptionFrame = exceptionInfo->ContextRecord->Fp;
	auto exceptionStack = exceptionInfo->ContextRecord->Sp;
#elif defined (CCL_PLATFORM_INTEL) && defined (CCL_PLATFORM_64BIT)
	DWORD imageType = IMAGE_FILE_MACHINE_AMD64;
	auto exceptionAddress = exceptionInfo->ContextRecord->Rip;
	auto exceptionFrame = exceptionInfo->ContextRecord->Rsp;
	auto exceptionStack = exceptionInfo->ContextRecord->Rsp;
#else
	DWORD imageType = IMAGE_FILE_MACHINE_I386;
	auto exceptionAddress = exceptionInfo->ContextRecord->Eip;
	auto exceptionFrame = exceptionInfo->ContextRecord->Ebp;
	auto exceptionStack = exceptionInfo->ContextRecord->Esp;
#endif

	HMODULE previousModuleHandle;
	if(GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)exceptionAddress, &previousModuleHandle) == 0)
		return 0;
	int moduleCount = 0;

	STACKFRAME64 stackFrame;
	memset (&stackFrame, 0, sizeof(stackFrame));
	stackFrame.AddrPC.Offset = exceptionAddress;
	stackFrame.AddrPC.Mode = AddrModeFlat;
	stackFrame.AddrFrame.Offset = exceptionFrame;
	stackFrame.AddrFrame.Mode = AddrModeFlat;
	stackFrame.AddrStack.Offset = exceptionStack;
	stackFrame.AddrStack.Mode = AddrModeFlat;

	HANDLE process = GetCurrentProcess ();
	HANDLE thread = GetCurrentThread ();
	
	SymInitialize (process, nullptr, TRUE);
	
	SYMBOL_INFO symbol;
	symbol.SizeOfStruct = sizeof(SYMBOL_INFO);
	symbol.MaxNameLen = 0;

	for(int frame = 0; moduleCount < maxCount && stackFrame.AddrPC.Offset != 0; frame++)
	{
		if(!StackWalk64 (imageType, process, thread, &stackFrame, exceptionInfo->ContextRecord, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
			break;
		
		SymFromAddr (process, stackFrame.AddrPC.Offset, nullptr, &symbol);

		HMODULE moduleHandle;
		if(GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)symbol.Address, &moduleHandle) == 0)
			break;

		if(moduleHandle && moduleHandle != previousModuleHandle)
		{
			if(GetModuleFileName (moduleHandle, modules[moduleCount], modules[moduleCount].size ()) > 0)
			{
				previousModuleHandle = moduleHandle;
				moduleCount++;
			}
		}

		if(stackFrame.AddrPC.Offset == stackFrame.AddrReturn.Offset)
			break;

		if(stackFrame.AddrReturn.Offset == 0)
			break;
	}

	return moduleCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsSafetyManager::translateException (unsigned int exceptionCode, LPEXCEPTION_POINTERS exceptionInfo)
{
	throw SEHException (exceptionInfo);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsSafetyManager::flush ()
{
	Threading::ScopedLock guard (exceptionLock);

	for(auto& buffer : unstableModuleBuffer)
	{
		if(buffer[0] != '\0')
		{
			SafetyManager::reportUnexpectedBehavior (buffer);
			buffer[0] = '\0';
		}
	}
}
