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
// Filename    : ccl/platform/win/backgroundservice.win.cpp
// Description : Win32 Background Service
//
//************************************************************************************************

#include "ccl/main/backgroundservice.h"
#include "ccl/main/cclargs.h"

#include "ccl/base/message.h"

#include "ccl/platform/win/cclwindows.h"
#include "ccl/public/systemservices.h"

#include <signal.h>
#include <Dbt.h>

namespace CCL {
namespace Win32 {

//************************************************************************************************
// Win32::PlatformService
//************************************************************************************************

class PlatformService
{
public:
	PlatformService ();
	~PlatformService ();

	static PlatformService* getInstance ();

	bool registerControlHandler (LPCWSTR serviceName);
	void registerDeviceNotifications (const ConstVector<UID>& filter);
	void unregisterDeviceNotifications ();
	bool reportStatus (DWORD state);
	bool waitForStop (DWORD timeout);
	void stop ();

protected:
	static PlatformService* theInstance;

	SERVICE_STATUS status;
	HANDLE hStopEvent;
	SERVICE_STATUS_HANDLE statusHandle;
	Vector<HDEVNOTIFY> notificationHandles;

	static DWORD WINAPI handler (DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);
};

static void WINAPI ServiceMain (DWORD dwArgc, LPTSTR* lpszArgv);

} // namespace Win32
} // namespace CCL

using namespace CCL;

//************************************************************************************************
// BackgroundService
//************************************************************************************************

bool BackgroundService::startPlatformService ()
{
	const uchar* serviceName = StringChars (name);

	SERVICE_TABLE_ENTRY serviceTable[] =
    {
        {const_cast<uchar*> (serviceName), Win32::ServiceMain},
        {nullptr, nullptr}
    };

	bool success = ::StartServiceCtrlDispatcher (serviceTable) != 0;

	// switch main thread back to the real main thread
	System::SwitchMainThread ();

	return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BackgroundService::startDevelopmentService ()
{
	static bool quitRequested = false;

	struct Handler
	{
		static void onSignal (int sig)
		{
			quitRequested = true;
		}
	};

	::signal (SIGINT, Handler::onSignal);

	if(!startup (*g_ArgumentList))
		return false;

	while(!quitRequested)
	{
		MSG msg;
		if(::PeekMessage (&msg, 0, 0, 0, PM_REMOVE|PM_NOYIELD))
		{
			::TranslateMessage (&msg);
			::DispatchMessage (&msg);
		}

		onIdle ();
		System::ThreadSleep (kIdlePeriod);
	}

	shutdown ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BackgroundService::flushPlatformUpdates ()
{
	if(runningAsService == false)
	{
		static const int kMaxMessageCount = 10;

		MSG msg;
		int messageCount = 0;
		while(++messageCount < kMaxMessageCount && ::PeekMessage (&msg, 0, 0, 0, PM_REMOVE|PM_NOYIELD))
		{
			::TranslateMessage (&msg);
			::DispatchMessage (&msg);
		}
	}

	static DWORD lastIdleTime = 0;
	DWORD now = ::GetTickCount ();
	if(now - lastIdleTime >= kIdlePeriod)
	{
		onIdle ();
		lastIdleTime = now;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BackgroundService::enableDeviceNotifications (const ConstVector<UID>& filter)
{
	auto platformService = Win32::PlatformService::getInstance ();
	ASSERT (platformService)
	if(platformService)
		platformService->registerDeviceNotifications (filter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BackgroundService::install (bool state)
{
	WCHAR binaryPath[MAX_PATH] = {};
	if(!::GetModuleFileName (NULL, binaryPath, MAX_PATH))
		return false;

	SC_HANDLE managerHandle = ::OpenSCManager (nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
	if(managerHandle == NULL)
		return false;

	bool success = false;

	if(state)
	{
		SC_HANDLE serviceHandle = ::CreateService (managerHandle,
												   StringChars (name), // service name
												   StringChars (name), // display name
												   SERVICE_ALL_ACCESS,
												   SERVICE_WIN32_OWN_PROCESS,
												   SERVICE_AUTO_START, // start automatically during system startup
												   SERVICE_ERROR_NORMAL,
												   binaryPath,
												   nullptr, nullptr, nullptr,
												   nullptr, // TODO: "NT AUTHORITY\LocalService" ???
												   nullptr); // no password

		if(serviceHandle)
		{
			if(description.isEmpty () == false)
			{
				SERVICE_DESCRIPTION desc = {0};
				desc.lpDescription = (LPWSTR)(LPCWSTR)StringChars (description);
				::ChangeServiceConfig2 (serviceHandle, SERVICE_CONFIG_DESCRIPTION, &desc);
			}

			// configure restart on crash
			SC_ACTION actions[1];
			actions[0].Type = SC_ACTION_RESTART;
			actions[0].Delay = 5000; // wait 5 seconds before restarting
			SERVICE_FAILURE_ACTIONS sfa = {0};
			sfa.dwResetPeriod = INFINITE;
			sfa.cActions = 1;
			sfa.lpsaActions = actions;
			::ChangeServiceConfig2 (serviceHandle, SERVICE_CONFIG_FAILURE_ACTIONS, &sfa);

			// start the service
			::StartService (serviceHandle, 0, nullptr);

			success = true;
			::CloseServiceHandle (serviceHandle);
		}
		#if DEBUG
		else
		{
			DWORD error = ::GetLastError ();
			ASSERT (error == ERROR_SERVICE_EXISTS)
		}
		#endif
	}
	else
	{
		SC_HANDLE serviceHandle = ::OpenService (managerHandle,
												 StringChars (name), // service name
												 SERVICE_ALL_ACCESS);

		if(serviceHandle)
		{
			// send stop request to service
			SERVICE_STATUS status = {0};
			::ControlService (serviceHandle, SERVICE_CONTROL_STOP, &status);

			// wait for service to stop
			static const int kStopTimeout = 10000; // 10 sec
			DWORD waitStarted = ::GetTickCount ();
			while(status.dwCurrentState != SERVICE_STOPPED)
			{
				::Sleep (100);

				if(!::QueryServiceStatus (serviceHandle, &status))
					break;

				DWORD now = ::GetTickCount ();
				if(now - waitStarted >= kStopTimeout)
					break;
			}

			// delete from service database
			success = ::DeleteService (serviceHandle) != 0;
			::CloseServiceHandle (serviceHandle);
		}
	}

	::CloseServiceHandle (managerHandle);
	return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// ServiceMain
//////////////////////////////////////////////////////////////////////////////////////////////////

void WINAPI Win32::ServiceMain (DWORD dwArgc, LPTSTR* lpszArgv)
{
	// switch main thread to the current thread
	System::SwitchMainThread ();

	BackgroundService* appService = BackgroundService::getInstance ();
	ASSERT (appService != 0)

	// Register handler for service
	Win32::PlatformService platformService;
	if(!platformService.registerControlHandler (StringChars (appService->getName ())))
		return;

	// Startup
	platformService.reportStatus (SERVICE_START_PENDING);
	if(!appService->startup (MutableArgumentList (dwArgc, lpszArgv)))
	{
		platformService.reportStatus (SERVICE_STOPPED);
		return;
	}

	// Main loop
	platformService.reportStatus (SERVICE_RUNNING);
	while(!platformService.waitForStop (BackgroundService::kIdlePeriod))
		appService->onIdle ();

	// cleanup device notifications
	platformService.unregisterDeviceNotifications ();

	// Stop
	platformService.reportStatus (SERVICE_STOP_PENDING);
	appService->shutdown ();
	platformService.reportStatus (SERVICE_STOPPED);
}

//************************************************************************************************
// Win32::PlatformService
//************************************************************************************************

Win32::PlatformService* Win32::PlatformService::theInstance = nullptr;
Win32::PlatformService* Win32::PlatformService::getInstance () { return theInstance; }

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32::PlatformService::PlatformService ()
: hStopEvent (::CreateEvent (nullptr, TRUE, FALSE, nullptr)),
  statusHandle (NULL)
{
	::memset (&status, 0, sizeof(SERVICE_STATUS));
	status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;

	ASSERT (theInstance == nullptr)
	theInstance = this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32::PlatformService::~PlatformService ()
{
	::CloseHandle (hStopEvent);

	ASSERT (theInstance == this)
	theInstance = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32::PlatformService::registerControlHandler (LPCWSTR serviceName)
{
	statusHandle = ::RegisterServiceCtrlHandlerEx (serviceName, handler, this);
	return statusHandle != NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32::PlatformService::registerDeviceNotifications (const ConstVector<UID>& filter)
{
	for(int i = 0; i < filter.count (); i++)
	{
		UIDBytes guid (filter[i]);

		DEV_BROADCAST_DEVICEINTERFACE platformFilter = {0};
		platformFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
		platformFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
		platformFilter.dbcc_classguid = reinterpret_cast<const GUID&> (guid);

		HDEVNOTIFY hDevNotify = ::RegisterDeviceNotification (statusHandle, &platformFilter, DEVICE_NOTIFY_SERVICE_HANDLE);
		ASSERT (hDevNotify != 0)
		if(hDevNotify)
			notificationHandles.add (hDevNotify);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32::PlatformService::unregisterDeviceNotifications ()
{
	VectorForEachFast (notificationHandles, HDEVNOTIFY, hDevNotify)
		::UnregisterDeviceNotification (hDevNotify);
	EndFor
	notificationHandles.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32::PlatformService::reportStatus (DWORD state)
{
	status.dwCurrentState = state;

	if(state == SERVICE_START_PENDING)
		status.dwControlsAccepted = 0;
	else
		status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;

	if(state == SERVICE_RUNNING || state == SERVICE_STOPPED)
		status.dwCheckPoint = 0;
	else
		status.dwCheckPoint++;

	return ::SetServiceStatus (statusHandle, &status) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32::PlatformService::waitForStop (DWORD timeout)
{
	return ::WaitForSingleObject (hStopEvent, timeout) == WAIT_OBJECT_0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32::PlatformService::stop ()
{
	::SetEvent (hStopEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/*static*/ DWORD WINAPI Win32::PlatformService::handler (DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
	auto platformService = reinterpret_cast<Win32::PlatformService*> (lpContext);
	ASSERT (platformService != 0)
	DWORD result = NO_ERROR;

	switch(dwControl)
	{
	case SERVICE_CONTROL_STOP :
		platformService->stop ();
		break;

	case SERVICE_CONTROL_SHUTDOWN :
		platformService->stop ();
		break;

	case SERVICE_CONTROL_INTERROGATE :
		// MSDN says to return NO_ERROR here, even if unimplemented.
		break;

	case SERVICE_CONTROL_DEVICEEVENT :
		switch(dwEventType)
		{
		case DBT_DEVICEARRIVAL :
		case DBT_DEVICEREMOVECOMPLETE :
			{
				DEV_BROADCAST_DEVICEINTERFACE* data = reinterpret_cast<DEV_BROADCAST_DEVICEINTERFACE*> (lpEventData);
				if(data && data->dbcc_size >= sizeof(DEV_BROADCAST_DEVICEINTERFACE) && data->dbcc_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
				{
					BackgroundService* appService = BackgroundService::getInstance ();
					ASSERT (appService != 0)

					String guidString;
					UID (reinterpret_cast<UIDRef> (data->dbcc_classguid)).toString (guidString);
					(NEW Message (BackgroundService::kDeviceNotification, guidString))->post (appService);
				}
			}
			break;
		}
		break;

	default :
		result = ERROR_CALL_NOT_IMPLEMENTED;
	}

	return result;
}
