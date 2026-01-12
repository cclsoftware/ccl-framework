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
// Filename    : ccl/platform/win/system/system.win.cpp
// Description : Windows system class
//
//************************************************************************************************

#include "ccl/system/system.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/asyncoperation.h"

#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/system/ipackagemetainfo.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

#include "ccl/platform/win/system/system.win.h"
#include "ccl/platform/win/system/cclcom.h"
#include "ccl/platform/win/system/registry.h"

#include "ccl/main/cclargs.h"

#include "ccl/platform/win/system/management.h"

#include <Tlhelp32.h>
#include <Lmcons.h>
#include <time.h>

#pragma comment (lib, "version.lib")

namespace CCL {

//************************************************************************************************
// WindowsSystemInformation
//************************************************************************************************

class WindowsSystemInformation: public SystemInformation
{
public:
	using SuperClass = SystemInformation;

	// ISystemInformation
	void CCL_API getLocalTime (DateTime& dateTime) const override;
	void CCL_API convertLocalTimeToUTC (DateTime& utc, const DateTime& localTime) const override;
	void CCL_API convertUTCToLocalTime (DateTime& localTime, const DateTime& utc) const override;
	void CCL_API convertUnixTimeToUTC (DateTime& utc, int64 unixTime) const override;
	int64 CCL_API convertUTCToUnixTime (const DateTime& utc) const override;
	int64 CCL_API getSecureComputerTime () const override;
	void CCL_API getComputerName (String& name, int flags = 0) const override;
	void CCL_API getUserName (String& name, int flags = 0) const override;
	int CCL_API getNumberOfCPUs () const override;
	int CCL_API getNumberOfCores () const override;
	void CCL_API getMemoryInfo (System::MemoryInfo& memoryInfo) const override;
	void CCL_API getComputerInfo (IAttributeList& attributes, int flags = 0) const override;
	IAsyncOperation* CCL_API searchApplications (StringRef filter) const override;
	
protected:
	bool getKnownLocation (uchar* path, KNOWNFOLDERID id, int length) const;

	// SystemInformation
	bool getNativeLocation (IUrl& url, System::FolderType type) const override;
};

//************************************************************************************************
// WindowsExecutableLoader
//************************************************************************************************

class WindowsExecutableLoader: public ExecutableLoader
{
public:
	// ExecutableLoader
	tresult CCL_API loadImage (IExecutableImage*& image, UrlRef path) override;
	IExecutableImage* CCL_API createImage (ModuleRef module) override;
	IExecutableIterator* CCL_API createIterator () override;
	tresult CCL_API execute (Threading::ProcessID& processId, UrlRef path, ArgsRef args,
							 int flags = 0, IUnknown* context = nullptr) override;
	tresult CCL_API relaunch (ArgsRef args) override;
	tresult CCL_API terminate (Threading::ProcessID processId) override;
	tresult CCL_API getExecutablePath (IUrl& path, Threading::ProcessID processId) override;
	tbool CCL_API isProcessRunning (UrlRef executableFile) override;
	tresult CCL_API getModuleInfo (IAttributeList& attributes, UrlRef path) override;
};

//************************************************************************************************
// WindowsLibraryImage
//************************************************************************************************

class WindowsLibraryImage: public ExecutableImage
{
public:
	WindowsLibraryImage (ModuleRef nativeRef, bool isLoaded);
	~WindowsLibraryImage ();

	void setReference (ModuleRef ref);

	// ExecutableImage
	tbool CCL_API getPath (IUrl& path) const override;
	void* CCL_API getFunctionPointer (CStringPtr name) const override;
	const IAttributeList* CCL_API getMetaInfo () const override;

private:
	void unload ();
};

//************************************************************************************************
// ModuleIterator
//************************************************************************************************

class ModuleIterator: public Unknown,
					  public IExecutableIterator
{
public:
	ModuleIterator (HANDLE hSnapshot);
	~ModuleIterator ();

	// IExecutableIterator
	const IExecutableImage* CCL_API getNextImage () override;

	CLASS_INTERFACE (IExecutableIterator, Unknown)

protected:
	WindowsLibraryImage image;
	HANDLE hSnapshot;
	bool first;
};

//************************************************************************************************
// VersionInfo
//************************************************************************************************

class VersionInfo
{
public:
	VersionInfo ();
	~VersionInfo ();

	PROPERTY_VARIABLE (WORD, codePage, CodePage)
	PROPERTY_VARIABLE (WORD, langID, LangID)

	bool readFromModule (HMODULE handle);
	bool readFromFile (LPCWSTR path);
	void empty ();

	VS_FIXEDFILEINFO* getFixedFileInfo () const;
	bool getStringFileInfo (String& result, const char* which) const;
	String getStringFileInfo (const char* which) const;
	bool toAttributes (IAttributeList& attributes) const;

protected:
	char* data;
};

//************************************************************************************************
// ApplicationSearcher
//************************************************************************************************

class ApplicationSearcher: public AsyncOperation
{
public:
	void find (StringRef filter);

protected:
	UnknownList resultList;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Services API
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ModuleRef CCL_API System::CCL_ISOLATED (GetMainModuleRef) ()
{
	return ::GetModuleHandle (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT tresult CCL_API System::CCL_ISOLATED (CreateUID) (UIDBytes& uid)
{
	uid.prepare ();

	GUID guid = {0};
	if(SUCCEEDED (::CoCreateGuid (&guid)))
	{
		::memcpy (&uid, &guid, 16);
		return kResultOk;
	}
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void CCL_API System::CCL_ISOLATED (LockMemory) (tbool state, void* address, int size)
{
	if(state)
		::VirtualLock (address, size);
	else
		::VirtualUnlock (address, size);
}

//************************************************************************************************
// WindowsSystemInformation
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (SystemInformation, WindowsSystemInformation)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowsSystemInformation::getNativeLocation (IUrl& path, System::FolderType folderType) const
{
	bool result = false, normalize = false;
	uchar p[Url::kMaxLength] = {0};
	int pathType = Url::kFolder;

	switch(folderType)
	{
	case System::kSystemFolder :
		result = ::GetWindowsDirectory (p, Url::kMaxLength) != 0;
		break;

	case System::kProgramsFolder :
		result = ::SHGetSpecialFolderPath (nullptr, p, CSIDL_PROGRAM_FILES, FALSE) != 0;
		break;

	case System::kTempFolder :
		result = ::GetTempPath (Url::kMaxLength, p) != 0;
		break;

	case System::kUserSettingsFolder :
	case System::kUserPreferencesFolder :
		result = ::SHGetSpecialFolderPath (nullptr, p, CSIDL_APPDATA, FALSE) != 0;
		break;

	case System::kSharedDataFolder :
		result = ::SHGetSpecialFolderPath (nullptr, p, CSIDL_COMMON_DOCUMENTS, FALSE) != 0;
		break;

	case System::kSharedSettingsFolder :
		result = ::SHGetSpecialFolderPath (nullptr, p, CSIDL_COMMON_APPDATA, FALSE) != 0;
		break;

	case System::kAppSupportFolder :
		{
			normalize = true; // we might get ".." segments when running within a debugger
			result = ::GetModuleFileName (NULL, p, Url::kMaxLength) != 0;
			uchar* ptr = ::wcsrchr (p, '\\');
			if(ptr)
				*ptr = 0;
		}
		break;

	case System::kSharedSupportFolder :
		result = ::SHGetSpecialFolderPath (nullptr, p, CSIDL_PROGRAM_FILES_COMMON, FALSE) != 0;
		break;
	
	case System::kUserAppSupportFolder :
		result = getKnownLocation (p, FOLDERID_UserProgramFilesCommon, IUrl::kMaxLength);
		break;

	case System::kUserDownloadsFolder :
		result = getKnownLocation (p, FOLDERID_Downloads, IUrl::kMaxLength);
		if(result) // fall through otherwise
			break;

	case System::kUserDocumentFolder :
		result = ::SHGetSpecialFolderPath (nullptr, p, CSIDL_PERSONAL, FALSE) != 0;
		break;

	case System::kUserMusicFolder :
		result = ::SHGetSpecialFolderPath (nullptr, p, CSIDL_MYMUSIC, FALSE) != 0;
		break;

	case System::kDesktopFolder :
		result = ::SHGetSpecialFolderPath (nullptr, p, CSIDL_DESKTOPDIRECTORY, FALSE) != 0;
		break;

    case System::kAppPluginsFolder :
		result = getNativeLocation (path, System::kAppSupportFolder);
		if(result)
			path.descend (CCLSTR ("Plugins"), Url::kFolder);
		return result;
	}

	if(result)
	{
		path.fromNativePath (p, pathType);

		if(normalize)
			path.normalize (IUrl::kRemoveDotSegments);
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowsSystemInformation::getKnownLocation (uchar* path, KNOWNFOLDERID id, int length) const
{
	bool succeeded = false;
	Win32::ComPtr<IKnownFolderManager> manager;
	if(SUCCEEDED (::CoCreateInstance (CLSID_KnownFolderManager, nullptr, CLSCTX_INPROC_SERVER, IID_IKnownFolderManager, manager)))
	{
		Win32::ComPtr<IKnownFolder> folder;
		if(SUCCEEDED (manager->GetFolder (id, folder)))
		{
			LPWSTR buffer = nullptr;
			if(SUCCEEDED (folder->GetPath (0, &buffer)))
			{
				succeeded = true;
				::wcsncpy (path, buffer, length);
				::CoTaskMemFree (buffer);
			}
		}
	}
	return succeeded;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowsSystemInformation::getLocalTime (DateTime& dateTime) const
{
	SYSTEMTIME st = {0};
	::GetLocalTime (&st);

	Win32::fromSystemTime (dateTime, st);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowsSystemInformation::convertLocalTimeToUTC (DateTime& utc, const DateTime& localTime) const
{
	SYSTEMTIME stLocal = {0};
	Win32::toSystemTime (stLocal, localTime);

	SYSTEMTIME stUTC = {0};
	BOOL result = ::TzSpecificLocalTimeToSystemTime (nullptr, &stLocal, &stUTC);
	ASSERT (result != 0)

	Win32::fromSystemTime (utc, stUTC);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowsSystemInformation::convertUTCToLocalTime (DateTime& localTime, const DateTime& utc) const
{
	SYSTEMTIME stUTC = {0};
	Win32::toSystemTime (stUTC, utc);

	SYSTEMTIME stLocal = {0};
	BOOL result = ::SystemTimeToTzSpecificLocalTime (nullptr, &stUTC, &stLocal);
	ASSERT (result != 0)

	Win32::fromSystemTime (localTime, stLocal);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowsSystemInformation::convertUnixTimeToUTC (DateTime& utc, int64 unixTime) const
{
	time_t value = unixTime;
	tm t = {0};
	::gmtime_s (&t, &value);
	CRTTypeConverter::tmToDateTime (utc, t);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API WindowsSystemInformation::convertUTCToUnixTime (const DateTime& utc) const
{
	// ATTENTION: This works only if CRT and OS have the same understanding of local time!
	DateTime localTime;
	convertUTCToLocalTime (localTime, utc);

	tm t = {0};
	CRTTypeConverter::tmFromDateTime (t, localTime);
	t.tm_isdst = -1; // determine if Daylight Saving Time was in effect
	return ::mktime (&t);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API WindowsSystemInformation::getSecureComputerTime () const
{
	Win32::ManagementServices services;
	ASSERT (services.isValid ())

	AutoPtr<Win32::ManagementObject> object;
	AutoPtr<Win32::ManagementEnumerator> enumerator;
	Variant value;

	Date date;
	Time time;
	if(enumerator = services.execQuery ("SELECT * FROM Win32_UTCTime"))
		while(object = enumerator->next ())
		{
			if(object->getProperty (value, "Day"))
				date.setDay (value.asInt ());
			if(object->getProperty (value, "Month"))
				date.setMonth (value.asInt ());
			if(object->getProperty (value, "Year"))
				date.setYear (value.asInt ());
			if(object->getProperty (value, "Hour"))
				time.setHour (value.asInt ());
			if(object->getProperty (value, "Minute"))
				time.setMinute (value.asInt ());
			if(object->getProperty (value, "Second"))
				time.setSecond (value.asInt ());
			if(object->getProperty (value, "Milliseconds"))
				time.setMilliseconds (value.asInt ());
		}
	DateTime dateTime (date, time);

	if(dateTime == DateTime ())
		return getUnixTime ();

	return convertUTCToUnixTime (dateTime);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowsSystemInformation::getComputerName (String& name, int flags) const
{
	WCHAR buffer[MAX_COMPUTERNAME_LENGTH + 1] = {0};
	DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
	::GetComputerName (buffer, &size);
	name.empty ();
	name.append (buffer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowsSystemInformation::getUserName (String& name, int flags) const
{
	WCHAR buffer[UNLEN + 1] = {0};
	DWORD size = UNLEN + 1;
	::GetUserName (buffer, &size);
	name.empty ();
	name.append (buffer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API WindowsSystemInformation::getNumberOfCPUs () const
{
	SYSTEM_INFO systemInfo = {0};
	::GetSystemInfo (&systemInfo);
	return systemInfo.dwNumberOfProcessors;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API WindowsSystemInformation::getNumberOfCores () const
{
 	return getNumberOfCPUs ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowsSystemInformation::getMemoryInfo (System::MemoryInfo& memoryInfo) const
{
	/*
		Memory Performance Information
		http://msdn.microsoft.com/en-us/library/windows/desktop/aa965225%28v=vs.85%29.aspx
	*/

	MEMORYSTATUSEX status = {0};
	status.dwLength = sizeof(MEMORYSTATUSEX);
	::GlobalMemoryStatusEx (&status);

	memoryInfo.physicalRAMSize = status.ullTotalPhys;
	memoryInfo.processMemoryTotal = status.ullTotalVirtual;

	#if 0
	PROCESS_MEMORY_COUNTERS_EX memCounters = {0};
	memCounters.cb = sizeof(PROCESS_MEMORY_COUNTERS_EX);
	::GetProcessMemoryInfo (::GetCurrentProcess (), (PPROCESS_MEMORY_COUNTERS)&memCounters, sizeof(memCounters));
	memoryInfo.processMemoryAvailable = status.ullTotalVirtual - memCounters.WorkingSetSize;
	#else
	memoryInfo.processMemoryAvailable = status.ullAvailVirtual;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowsSystemInformation::getComputerInfo (IAttributeList& attributes, int flags) const
{
	attributes.setAttribute (System::kDeviceModel, CCLSTR ("PC"));

	String computerName;
	getComputerName (computerName);
	attributes.setAttribute (System::kDeviceIdentifier, computerName);

	if((flags & System::kQueryExtendedComputerInfo) == 0)
		return;

	// Caller wants extended information, this could take a while...
	Win32::ManagementServices services;
	ASSERT (services.isValid ())

	#if (0 && DEBUG)
	services.dumpAll ();
	#endif

	AutoPtr<Win32::ManagementObject> object;
	AutoPtr<Win32::ManagementEnumerator> enumerator;
	Variant value;

	// *** Operating System ****
	int64 systemDriveIndex = 0;
	if(enumerator = services.execQuery ("SELECT * FROM Win32_OperatingSystem"))
		if(object = enumerator->next ())
		{
			if(object->getProperty (value, "Name"))
			{
				// MSDN says: Operating system instance within a computer system.
				// Example: "Microsoft Windows 10 Pro|C:\WINDOWS|\Device\Harddisk0\Partition4"
				String osInstanceKey = VariantString (value);
				String harddiskPrefix ("\\Harddisk");
				int index = osInstanceKey.index (harddiskPrefix);
				if(index > 0)
				{
					String remainder = osInstanceKey.subString (index + harddiskPrefix.length ());
					remainder.getIntValue (systemDriveIndex);
				}
			}

			if(object->getProperty (value, "Caption"))
				attributes.setAttribute (System::kOSName, value);
			if(object->getProperty (value, "Version"))
				attributes.setAttribute (System::kOSVersion, value);
		}

	// *** CPU Information ****
	if(enumerator = services.execQuery ("SELECT * FROM Win32_Processor"))
		if(object = enumerator->next ())
		{
			if(object->getProperty (value, "Caption"))
			{
				attributes.setAttribute (System::kCPUIdentifier, value);
				attributes.setAttribute (System::kCPUModelHumanReadable, value);
			}
		}

	// use cached CPU speed because it seems to be unstable
	int cpuClockSpeed = Win32::ManagementRegistry (services).getCpuClockSpeed ();
	attributes.setAttribute (System::kCPUSpeed, cpuClockSpeed);
	attributes.setAttribute (System::kCPUSpeedMHz, double(cpuClockSpeed));

	// *** Memory Information ***
	int64 physicalMemoryAmount = 0;
	if(enumerator = services.execQuery ("SELECT * FROM Win32_PhysicalMemory"))
		while(object = enumerator->next ())
		{
			if(object->getProperty (value, "Capacity"))
			{
				int64 capacity = 0;
				if(value.isString ())
					value.asString ().getIntValue (capacity);
				else
					capacity = value.asLargeInt ();
				ASSERT (capacity != 0)
				physicalMemoryAmount += capacity;
			}
		}
	attributes.setAttribute (System::kPhysicalRAMSize, physicalMemoryAmount);

	// *** Disk Information ***
	bool diskDriveFound = false;
	for(int retryCount = 1; retryCount <= 2 && !diskDriveFound; retryCount++)
	{
		if(enumerator = services.execQuery ("SELECT * FROM Win32_DiskDrive"))
			while(object = enumerator->next ())
			{
				object->getProperty (value, "MediaType");
				if(!VariantString (value).contains ("fixed", false))
					continue;

				// prefer system drive on first attempt
				if(retryCount == 1)
				{
					int64 driveIndex = -1;
					if(object->getProperty (value, "Index"))
						driveIndex = value.asLargeInt ();
					if(driveIndex != systemDriveIndex)
						continue;
				}

				object->getProperty (value, "SerialNumber");
				attributes.setAttribute (System::kDiskSerialNumber, value);

				if(object->getProperty (value, "Model"))
				{
					attributes.setAttribute (System::kDiskModelHumanReadable, value);
					diskDriveFound = true;
					break;
				}
			}
	}

	// Determine unique file system identifier of Windows folder
	// See https://msdn.microsoft.com/en-us/library/windows/desktop/hh802691(v=vs.85).aspx
	// Volume serial number is generated when drive is formatted based on date/time. A cloned drive has same serial number.
	// See https://en.wikipedia.org/wiki/Volume_serial_number

	TCHAR winDir[MAX_PATH] = {0};
	::GetWindowsDirectory (winDir, MAX_PATH);
	HANDLE winDirHandle = ::CreateFile (winDir, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if(winDirHandle)
	{
		FILE_ID_INFO fileIdInfo = {0};
		if(::GetFileInformationByHandleEx (winDirHandle, FileIdInfo, &fileIdInfo, sizeof(FILE_ID_INFO)))
		{
			if(fileIdInfo.VolumeSerialNumber != 0)
			{
				MutableCString volumeSerialString;
				volumeSerialString.appendFormat ("%" FORMAT_INT64 "X", fileIdInfo.VolumeSerialNumber);
				attributes.setAttribute (System::kVolumeSerialNumber, String (volumeSerialString));
			}

			MutableCString fileIdString;
			for(int i = 0; i < ARRAY_COUNT (fileIdInfo.FileId.Identifier); i++)
			{
				int byteValue = fileIdInfo.FileId.Identifier[i];
				fileIdString.appendFormat ("%02X", byteValue);
			}
			attributes.setAttribute (System::kSystemFolderFSID, String (fileIdString));
		}
		::CloseHandle (winDirHandle);
	}

	// *** Ethernet Adapter ***
	Win32::NetworkAdapterList adapterList;
	adapterList.collect (services);

	Win32::NetworkAdapterList::AdapterInfo primaryAdapterInfo;
	if(adapterList.getPrimaryAdapterInfo (primaryAdapterInfo))
	{
		attributes.setAttribute (System::kMACAddress, primaryAdapterInfo.macAddress);
		attributes.setAttribute (System::kEthernetAdapter, primaryAdapterInfo.name);
	}

	// optionally pass all network adapters to caller
	if(flags & System::kQueryEthernetAdapterList)
	{
		for(int i = 0; i < adapterList.getAdapters ().count (); i++)
		{
			auto& adapterInfo = adapterList.getAdapters ()[i];

			AutoPtr<IAttributeList> adapterAttr = AttributeAccessor (attributes).newAttributes ();
			adapterAttr->setAttribute (System::kMACAddress, adapterInfo.macAddress);
			adapterAttr->setAttribute (System::kEthernetAdapter, adapterInfo.name);

			attributes.queueAttribute (System::kEthernetAdapterList,
									   static_cast<IAttributeList*> (adapterAttr),
									   IAttributeList::kShare);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API WindowsSystemInformation::searchApplications (StringRef filter) const
{
	ApplicationSearcher* searcher = NEW ApplicationSearcher ();
	searcher->find (filter);
	return searcher;
}

//************************************************************************************************
// WindowsExecutableLoader
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (ExecutableLoader, WindowsExecutableLoader)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WindowsExecutableLoader::loadImage (IExecutableImage*& image, UrlRef path)
{
	NativePath np (path);
	HMODULE hModule = ::LoadLibrary (np);
	if(hModule)
	{
		image = NEW WindowsLibraryImage (hModule, true);
		return kResultOk;
	}

	#if DEBUG
	CHAR msg [256] = {0};
	FormatMessageA (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, ::GetLastError (), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), msg, 254, nullptr);
	Debugger::printf ("ExecutableLoader::loadImage FAILED: %s (error = %s)\n", MutableCString (UrlFullString (path)).str (), msg);
	#endif
	image = nullptr;
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IExecutableImage* CCL_API WindowsExecutableLoader::createImage (ModuleRef module)
{
	return NEW WindowsLibraryImage (module, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IExecutableIterator* CCL_API WindowsExecutableLoader::createIterator ()
{
	HANDLE hSnapshot = ::CreateToolhelp32Snapshot (TH32CS_SNAPMODULE, 0);
	if(hSnapshot == INVALID_HANDLE_VALUE)
		return nullptr;

	return NEW ModuleIterator (hSnapshot);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static HANDLE DuplicateHandleHelper (HANDLE hSource)
{
	HANDLE hTarget = NULL;
	::DuplicateHandle (::GetCurrentProcess (), hSource, ::GetCurrentProcess (), &hTarget, 0, TRUE, DUPLICATE_SAME_ACCESS);
	return hTarget;
}

tresult CCL_API WindowsExecutableLoader::execute (Threading::ProcessID& processId, UrlRef path, ArgsRef args,
												  int flags, IUnknown* context)
{
	ASSERT (!path.isEmpty ())
	String pathString;
	path.toDisplayString (pathString, Url::kStringNativePath);

	StringChars pathStringChars (pathString);
	LPCWSTR applicationName = pathStringChars;

	String commandString;
	commandString << "\"" << pathString << "\"";  // repeat executable path as first argument

	// append arguments to command line
	String argumentString;
	args.toString (argumentString);
	commandString << " " << argumentString;

	WCHAR* commandLine = nullptr; // command line can be modified by CreateProcess()!
	if(!commandString.isEmpty ())
	{
		int bufferLength = commandString.length () + 1;
		commandLine = NEW WCHAR[bufferLength];
		commandString.copyTo (commandLine, bufferLength);
	}
	VectorDeleter<WCHAR> commandLineDeleter (commandLine);

	STARTUPINFO startupInfo = {0};
	startupInfo.cb = sizeof(STARTUPINFO);

	if(flags & System::kSuppressProcessGUI)
	{
		startupInfo.dwFlags |= STARTF_USESHOWWINDOW;
		startupInfo.wShowWindow = SW_HIDE;
	}

	BOOL inheritHandles = FALSE;
	if(flags & System::kRedirectProcessOutput)
	{
		UnknownPtr<INativeFileStream> fileStream (context);
		ASSERT (fileStream.isValid ())
		if(fileStream)
		{
			startupInfo.dwFlags |= STARTF_USESTDHANDLES;
			startupInfo.hStdInput = DuplicateHandleHelper (::GetStdHandle (STD_INPUT_HANDLE));
			startupInfo.hStdOutput = DuplicateHandleHelper ((HANDLE)fileStream->getNativeFileStream ());
			startupInfo.hStdError = DuplicateHandleHelper ((HANDLE)fileStream->getNativeFileStream ());
			inheritHandles = TRUE;
		}
	}

	DWORD creationFlags = 0;
	PROCESS_INFORMATION processInfo = {nullptr};
	BOOL result = ::CreateProcess (applicationName, commandLine, nullptr, nullptr,
								  inheritHandles, creationFlags, nullptr, nullptr,
								  &startupInfo, &processInfo);
	tresult tr = kResultOk;
	if(!result)
	{
		tr = kResultFailed;

		DWORD lastError = ::GetLastError ();
		CCL_WARN ("CreateProcess() %ws failed with error %d!\n", applicationName, lastError);

		if(flags & System::kWaitForProcessExit)
			tr = static_cast<tresult> (lastError);
	}
	else
	{
		processId = processInfo.dwProcessId;
		::CloseHandle (processInfo.hThread);

		if(flags & System::kWaitForProcessExit)
		{
			::WaitForSingleObject(processInfo.hProcess, INFINITE);

			DWORD exitCode = -1000;
			result = ::GetExitCodeProcess (processInfo.hProcess, &exitCode);
			ASSERT (result)

			tr = static_cast<tresult> (exitCode);
		}

		::CloseHandle (processInfo.hProcess);
	}

	if(inheritHandles)
	{
		::CloseHandle (startupInfo.hStdInput);
		::CloseHandle (startupInfo.hStdError);
		::CloseHandle (startupInfo.hStdOutput);
	}

	return tr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WindowsExecutableLoader::relaunch (ArgsRef args)
{
	Url exePath;
	getMainImage ().getPath (exePath);
	Threading::ProcessID processId = 0;
	return execute (processId, exePath, args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WindowsExecutableLoader::terminate (Threading::ProcessID processId)
{
	HANDLE hProcess = ::OpenProcess (PROCESS_TERMINATE, FALSE, (DWORD)processId);
	if(hProcess == NULL)
		return kResultFailed;

	BOOL result = ::TerminateProcess (hProcess, 0);
	::CloseHandle (hProcess);
	return result ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WindowsExecutableLoader::getExecutablePath (IUrl& path, Threading::ProcessID processId)
{
	HANDLE hProcess = ::OpenProcess (PROCESS_QUERY_LIMITED_INFORMATION, FALSE, (DWORD)processId);
	if(hProcess == NULL)
		return kResultFailed;

	uchar nativePath[Url::kMaxLength] = {0};
	DWORD length = Url::kMaxLength;
	bool result = ::QueryFullProcessImageName (hProcess, 0, nativePath, &length);
	path.setPath (nativePath, IUrl::kFile);

	::CloseHandle (hProcess);

	return result ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
tbool CCL_API WindowsExecutableLoader::isProcessRunning (UrlRef executableFile)
{
	String appFileName;
	executableFile.getName (appFileName, true);

	bool exists = false;
	PROCESSENTRY32 entry = {0};
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = ::CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, NULL);

	if(::Process32First (snapshot, &entry)) // System Process
		while(::Process32Next (snapshot, &entry))
		{
			String compare (entry.szExeFile);
			if(compare == appFileName)
			{
				exists = true;
				break;
			}
		}

	::CloseHandle (snapshot);
	return exists;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WindowsExecutableLoader::getModuleInfo (IAttributeList& attributes, UrlRef path)
{
	NativePath dllPathNative (path);
	VersionInfo versionInfo;
	if(versionInfo.readFromFile (dllPathNative))
		if(versionInfo.toAttributes (attributes))
			return kResultOk;

	return kResultFailed;
}

//************************************************************************************************
// WindowsLibraryImage
//************************************************************************************************

WindowsLibraryImage::WindowsLibraryImage (ModuleRef nativeRef, bool isLoaded)
: ExecutableImage (nativeRef, isLoaded)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsLibraryImage::~WindowsLibraryImage ()
{
	if(isLoaded && nativeRef)
		unload ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsLibraryImage::setReference (ModuleRef ref)
{
	ASSERT (isLoaded == false)

	if(metaInfo)
		metaInfo->release (),
		metaInfo = nullptr;

	nativeRef = ref;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsLibraryImage::unload ()
{
	::FreeLibrary ((HMODULE)nativeRef);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsLibraryImage::getPath (IUrl& path) const
{
	ASSERT (nativeRef != nullptr)
	uchar nativePath[Url::kMaxLength] = {0};
	if(::GetModuleFileName ((HINSTANCE)nativeRef, nativePath, Url::kMaxLength))
	{
		path.fromNativePath (nativePath);
		path.normalize (IUrl::kRemoveDotSegments); // we might get ".." segments when running within a debugger
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* CCL_API WindowsLibraryImage::getFunctionPointer (CStringPtr name) const
{
	ASSERT (nativeRef != nullptr)
	return ::GetProcAddress ((HMODULE)nativeRef, name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IAttributeList* CCL_API WindowsLibraryImage::getMetaInfo () const
{
	ASSERT (nativeRef != nullptr)
	if(!metaInfo)
	{
		VersionInfo versionInfo;
		if(versionInfo.readFromModule ((HMODULE)nativeRef))
		{
			metaInfo = NEW Attributes;
			versionInfo.toAttributes (*metaInfo);
		}
	}
	return metaInfo;
}

//************************************************************************************************
// ModuleIterator
//************************************************************************************************

ModuleIterator::ModuleIterator (HANDLE hSnapshot)
: hSnapshot (hSnapshot),
  image (nullptr, false),
  first (true)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ModuleIterator::~ModuleIterator ()
{
	::CloseHandle (hSnapshot);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IExecutableImage* CCL_API ModuleIterator::getNextImage ()
{
	BOOL result = FALSE;
	MODULEENTRY32 entry = {0};
	if(first)
	{
		result = ::Module32First (hSnapshot, &entry);
		first = false;
	}
	else
		result = ::Module32Next (hSnapshot, &entry);

	if(result)
	{
		image.setReference (entry.hModule);
		return &image;
	}
	return nullptr;
}

//************************************************************************************************
// VersionInfo
//************************************************************************************************

VersionInfo::VersionInfo ()
: data (nullptr),
  codePage (0),
  langID (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

VersionInfo::~VersionInfo ()
{
	empty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VersionInfo::empty ()
{
	if(data)
		delete [] data;

	data = nullptr;
	codePage = 0;
	langID = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VersionInfo::readFromModule (HMODULE handle)
{
	WCHAR nativePath[Url::kMaxLength] = {0};
	::GetModuleFileName ((HMODULE)handle, nativePath, Url::kMaxLength);
	return readFromFile (nativePath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VersionInfo::readFromFile (LPCWSTR path)
{
	empty ();

	DWORD unused = 0;
	DWORD dataSize = ::GetFileVersionInfoSize (path, &unused);
	if(dataSize > 0)
	{
		data = NEW char[dataSize];
		::memset (data, 0, dataSize);
		::GetFileVersionInfo (path, unused, dataSize, data);

		void* ptr = nullptr;
		UINT length = 0;
		::VerQueryValue (data, L"\\VarFileInfo\\Translation", &ptr, &length);
		if(ptr)
		{
			codePage = LOWORD(*(DWORD*)ptr);
			langID = HIWORD(*(DWORD*)ptr);
		}
	}
	return data != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VS_FIXEDFILEINFO* VersionInfo::getFixedFileInfo () const
{
	void* ptr = nullptr;
	UINT length = 0;
	if(data && ::VerQueryValue (data, L"\\", &ptr, &length))
		return (VS_FIXEDFILEINFO*)ptr;
	else
		return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VersionInfo::getStringFileInfo (String& result, const char* which) const
{
	result.empty ();
	if(data != nullptr)
	{
		String name;
		name << "\\StringFileInfo\\";
		name.appendHexValue (codePage, 4);
		name.appendHexValue (langID, 4);
		name << "\\" << which;

		void* ptr = nullptr;
		UINT length = 0;
		if(::VerQueryValue (data, (LPWSTR)(const uchar*)StringChars (name), &ptr, &length) && ptr != nullptr)
		{
			result.append ((uchar*)ptr);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String VersionInfo::getStringFileInfo (const char* which) const
{
	String result;
	getStringFileInfo (result, which);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VersionInfo::toAttributes (IAttributeList& attributes) const
{
	if(data == nullptr)
		return false;

	attributes.setAttribute (Meta::kPackageID, getStringFileInfo ("InternalName"));
	attributes.setAttribute (Meta::kPackageName, getStringFileInfo ("ProductName"));
	attributes.setAttribute (Meta::kPackageCopyright, getStringFileInfo ("LegalCopyright"));
	attributes.setAttribute (Meta::kPackageVendor, getStringFileInfo ("CompanyName"));
	attributes.setAttribute (Meta::kPackageDescription, getStringFileInfo ("FileDescription"));
	attributes.setAttribute (Meta::kPackageVersion, getStringFileInfo ("FileVersion"));

	return true;
}

//************************************************************************************************
// ApplicationSearcher
//************************************************************************************************

void ApplicationSearcher::find (StringRef filter)
{
	setResult (static_cast<IUnknownList*> (&resultList));
	resultList.removeAll ();
	Vector<Url> urls;

	for(int i = 0; i < 2; i++)
	{
		Registry::RegAccess access = i == 0 ? Registry::kAccess32Bit : Registry::kAccess64Bit;

		Registry::Accessor accessor (Registry::kKeyLocalMachine, CCLSTR ("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall"), access);

		ForEachRegistryKey (accessor.newKeyIterator (), keyName)
			String displayName;
			String displayIcon;

			if(accessor.readString (displayName, keyName, CCLSTR ("DisplayName")) && accessor.readString (displayIcon, keyName, CCLSTR ("DisplayIcon")) )
			{
				bool collectItem = false;
				if(filter.isEmpty () == false)
				{
					ForEachStringToken (filter, CCLSTR (",;"), token)
						if(displayName.contains (token, false))
						{
							collectItem = true;
							break;
						}
					EndFor
				}
				else
					collectItem = true;

				if(collectItem)
				{
					String executablePath = displayIcon;
					int iconExt = executablePath.index (",");
					if(iconExt > 0)
						executablePath.remove (iconExt, -1);

					Url path;
					path.fromDisplayString (executablePath, Url::kFile);

					if(urls.contains (path) == false && System::GetFileSystem ().fileExists (path))
					{
						urls.add (path);

						AutoPtr<Attributes> attr = NEW Attributes;
						attr->set (Meta::kPackageExecutable, executablePath);
						attr->set (Meta::kPackageName, displayName);
						attr->set (Meta::kPackageIcon, displayIcon);

						String string;
						if(accessor.readString (string, keyName, CCLSTR ("Publisher")))
							attr->set (Meta::kPackageVendor, string);

						if(accessor.readString (string, keyName, CCLSTR ("DisplayVersion")))
							attr->set (Meta::kPackageVersion, string);
						else
						{
						   NativePath executablePathNative (path);
						   DWORD handle;
 						   DWORD fileInfoSize = GetFileVersionInfoSize (executablePathNative, &handle);
						   if(fileInfoSize > 0)
						   {
								Vector<BYTE> data (fileInfoSize);
								if(GetFileVersionInfo (executablePathNative, handle, fileInfoSize, data))
								{
									LPVOID info;
								    if(VerQueryValue (data, L"\\", &info, nullptr))
									{
										VS_FIXEDFILEINFO* fileInfo = static_cast<VS_FIXEDFILEINFO*> (info);
										int major = HIWORD (fileInfo->dwProductVersionMS);
										int minor = LOWORD (fileInfo->dwProductVersionMS);
										string.empty ();
										string.appendFormat (CCLSTR ("%(1).%(2)"), major, minor);
										attr->set (Meta::kPackageVersion, string);
									}
								}
							}
						}
						resultList.add (ccl_as_unknown (attr), true);
					}
				}
			}
		EndFor
	}
	setState (kCompleted);
}

