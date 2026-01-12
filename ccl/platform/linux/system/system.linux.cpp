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
// Filename    : ccl/platform/linux/system/system.linux.cpp
// Description : Linux CCL System implementation
//
//************************************************************************************************

#define DEBUG_LOG 1

#include "ccl/system/system.h"

#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/propertyfile.h"
#include "ccl/base/asyncoperation.h"

#include "ccl/public/base/istream.h"
#include "ccl/public/base/uid.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/text/itextstreamer.h"
#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/plugins/ipluginmanager.h"
#include "ccl/public/system/ifilesystem.h"
#include "ccl/public/system/ipackagemetainfo.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

#include "ccl/main/cclargs.h"

#include "ccl/platform/linux/interfaces/ilinuxsystem.h"
#include "ccl/platform/shared/host/platformintegrationloader.h"
#include "ccl/platform/shared/interfaces/platformintegration.h"
#include "ccl/platform/shared/posix/system/system.posix.h"

#include "mountinfo.h"

#include <uuid/uuid.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/utsname.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <net/if.h> 
#include <netinet/in.h>
#include <errno.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <pwd.h>
#include <link.h>

#ifndef CCL_INSTALL_PREFIX
#define CCL_INSTALL_PREFIX "/"
#endif

namespace CCL {
	
//************************************************************************************************
// LinuxPlatformIntegrationLoader
//************************************************************************************************

class LinuxPlatformIntegrationLoader: public PlatformIntegration::PlatformIntegrationLoader
{
protected:
	int getPriority(const IClassDescription& description) const;
};

//************************************************************************************************
// LinuxSystemInformation
//************************************************************************************************

class LinuxSystemInformation: public SystemInformation,
							  public ILinuxSystem
{
public:
	LinuxSystemInformation ();
	
	DECLARE_STRINGID_MEMBER (kPlatformIntegrationFolder)
	
	DECLARE_STRINGID_MEMBER (kXdgConfigHome)
	DECLARE_STRINGID_MEMBER (kXdgDataHome)
	DECLARE_STRINGID_MEMBER (kXdgConfigDirs)
	DECLARE_STRINGID_MEMBER (kXdgDesktopDir)
	DECLARE_STRINGID_MEMBER (kXdgDownloadDir)
	DECLARE_STRINGID_MEMBER (kXdgDocumentsDir)
	DECLARE_STRINGID_MEMBER (kXdgMusicDir)
	
	// ILinuxSystem
	void CCL_API setDBusSupport (IDBusSupport* dbusSupport) override;
	IDBusSupport* CCL_API getDBusSupport () const override;

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
	void CCL_API terminate () override;
	
	CLASS_INTERFACES (SystemInformation)
	
protected:
	LinuxPlatformIntegrationLoader platformIntegrationLoader;
	IDBusSupport* dbusSupport;
	
	bool getXdgUserDir (IUrl& url, StringID id) const;
	bool getHardDiskInfo (String& serial, String& model, String& volumeSerial, StringRef devicePath) const;
	
	// SystemInformation
	bool getNativeLocation (IUrl& url, System::FolderType type) const override;
};

//************************************************************************************************
// LinuxImage
//************************************************************************************************

class LinuxImage: public ExecutableImage
{
public:
	LinuxImage (ModuleRef nativeRef, bool isLoaded);
	~LinuxImage ();
	
	// ExecutableImage
	void* CCL_API getFunctionPointer (CStringPtr name) const override;
	tbool CCL_API getPath (IUrl& url) const override;
	
private:
	void unload ();
};
	
//************************************************************************************************
// LinuxExecutableLoader
//************************************************************************************************

class LinuxExecutableLoader: public ExecutableLoader
{
public:
	// IExecutableLoader
	tresult CCL_API loadImage (IExecutableImage*& image, UrlRef path) override;
	IExecutableImage* CCL_API createImage (ModuleRef module) override;
	tresult CCL_API execute (Threading::ProcessID& processId, UrlRef path, ArgsRef args,
							 int flags = 0, IUnknown* context = nullptr) override;
	tresult CCL_API relaunch (ArgsRef args) override;
	tresult CCL_API terminate (Threading::ProcessID processId) override;
	tresult CCL_API getExecutablePath (IUrl& path, Threading::ProcessID processId) override;
};

//************************************************************************************************
// ApplicationSearcher
//************************************************************************************************

class ApplicationSearcher: public AsyncOperation
{
public:
	void find (StringRef filter, const IUnknownList& directories);

protected:
	UnknownList resultList;
	
	void scan (StringRef filter, UrlRef directory);
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Services API
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ModuleRef CCL_API System::CCL_ISOLATED (GetMainModuleRef) ()
{
	ModuleRef mainModule = ::dlopen (nullptr, RTLD_LAZY | RTLD_NOLOAD);
	return mainModule;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void CCL_API System::CCL_ISOLATED (LockMemory) (tbool state, void* address, int size)
{
	if(state)
		::mlock (address, size);
	else
		::munlock (address, size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT tresult CCL_API System::CCL_ISOLATED (CreateUID) (UIDBytes& uid)
{
	uuid_t buffer;
	uuid_generate (buffer);
	
	UID tmp;
	tmp.fromBuffer (buffer);
	
	uid = tmp;
	
	return kResultOk;
}

//************************************************************************************************
// LinuxPlatformIntegrationLoader
//************************************************************************************************

int LinuxPlatformIntegrationLoader::getPriority (const IClassDescription& description) const
{
	int priority = 0;

	Variant var;
	description.getClassAttribute (var, PLATFORMINTEGRATION_ENVIRONMENT);
	ForEachStringToken (var.asString (), CCLSTR (";"), statement)
		String variableName;
		String expectedValue;
		int separatorPosition = statement.index ("=");
		if(separatorPosition < 0)
		{
			variableName = statement;
		}
		else
		{
			variableName = statement.subString (0, separatorPosition);
			expectedValue = statement.subString (separatorPosition + 1);
		}

		CStringPtr value = ::getenv (MutableCString (variableName, Text::kSystemEncoding));
		if(value != nullptr)
		{
			// if no expected value is specified, the environment variable just needs to have any non-empty value
			if(expectedValue.isEmpty ())
			{
				priority++;
			}
			// otherwise, the environment variable must have a specific value or a colon-separated list containing that specific value
			else
			{
				ForEachStringToken (String (Text::kSystemEncoding, value), CCLSTR (":"), valueListItem)
					if(valueListItem == expectedValue)
					{
						priority++;
						break;
					}
				EndFor
			}
		}
	EndFor

	return priority + PlatformIntegrationLoader::getPriority (description);
}

//************************************************************************************************
// LinuxSystemInformation
//************************************************************************************************

static int64 getUtcOffset (const DateTime& dateTime)
{
	// based on https://stackoverflow.com/questions/9076494/how-to-convert-from-utc-to-local-time-in-c
	
	tm currentUtc = {0};
	CRTTypeConverter::tmFromDateTime (currentUtc, dateTime);
	
	time_t now = ::mktime (&currentUtc);
	tm local = *::gmtime (&now);
	
	return local.tm_sec - currentUtc.tm_sec
			+60LL*(local.tm_min - currentUtc.tm_min)
			+3600LL*(local.tm_hour - currentUtc.tm_hour)
			+86400LL*(local.tm_yday - currentUtc.tm_yday)
			+(local.tm_year-70)*31536000LL
			-(local.tm_year-69)/4*86400LL
			+(local.tm_year-1)/100*86400LL
			-(local.tm_year+299)/400*86400LL
			-(currentUtc.tm_year-70)*31536000LL
			+(currentUtc.tm_year-69)/4*86400LL
			-(currentUtc.tm_year-1)/100*86400LL
			+(currentUtc.tm_year+299)/400*86400LL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void getHomeDirectory (IUrl& path)
{
	uid_t uid = ::geteuid ();
	passwd* pw = ::getpwuid (uid);
	if(pw)
		path.fromPOSIXPath (pw->pw_dir, IUrl::kFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_EXTERNAL_SINGLETON (SystemInformation, LinuxSystemInformation)
DEFINE_STRINGID_MEMBER_ (LinuxSystemInformation, kPlatformIntegrationFolder, "PlatformIntegration")

DEFINE_STRINGID_MEMBER_ (LinuxSystemInformation, kXdgConfigHome, "XDG_CONFIG_HOME")
DEFINE_STRINGID_MEMBER_ (LinuxSystemInformation, kXdgDataHome, "XDG_DATA_HOME")
DEFINE_STRINGID_MEMBER_ (LinuxSystemInformation, kXdgConfigDirs, "XDG_CONFIG_DIRS")
DEFINE_STRINGID_MEMBER_ (LinuxSystemInformation, kXdgDesktopDir, "XDG_DESKTOP_DIR")
DEFINE_STRINGID_MEMBER_ (LinuxSystemInformation, kXdgDownloadDir, "XDG_DOWNLOAD_DIR")
DEFINE_STRINGID_MEMBER_ (LinuxSystemInformation, kXdgDocumentsDir, "XDG_DOCUMENTS_DIR")
DEFINE_STRINGID_MEMBER_ (LinuxSystemInformation, kXdgMusicDir, "XDG_MUSIC_DIR")

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxSystemInformation::LinuxSystemInformation ()
{
	Url path;
	getLocation (path, System::kAppSupportFolder);
	path.descend (String (kPlatformIntegrationFolder), IUrl::kFolder);
	platformIntegrationLoader.setPlatformIntegrationFolder (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinuxSystemInformation::queryInterface (UIDRef iid, void** ptr)
{
	if(iid == ccl_iid<PlatformIntegration::IPlatformIntegrationLoader> ())
		return platformIntegrationLoader.queryInterface (iid, ptr);
	QUERY_INTERFACE (ILinuxSystem)
	return SystemInformation::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxSystemInformation::terminate ()
{
	platformIntegrationLoader.terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxSystemInformation::setDBusSupport (IDBusSupport* support)
{
	dbusSupport = support;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDBusSupport* CCL_API LinuxSystemInformation::getDBusSupport () const
{
	return dbusSupport;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxSystemInformation::getNativeLocation (IUrl& path, System::FolderType folderType) const
{
	bool succeeded = false;
	
	switch(folderType)
	{
	case System::kSystemFolder :
		path.fromPOSIXPath ("/", Url::kFolder);
		succeeded = true;
		break;

	case System::kProgramsFolder :
		path.fromPOSIXPath (CCL_INSTALL_PREFIX "opt", Url::kFolder);
		succeeded = true;
		break;

	case System::kTempFolder :
		path.fromPOSIXPath ("/var/tmp", Url::kFolder);
		succeeded = true;
		break;

	case System::kUserSettingsFolder : CCL_FALLTHROUGH
	case System::kUserPreferencesFolder :
		path.fromDisplayString (String (Text::kSystemEncoding, ::getenv (kXdgConfigHome)), IUrl::kFolder);
		if(path.isEmpty ())
		{
			getHomeDirectory (path);
			path.descend (".config", IUrl::kFolder);
		}
		succeeded = true;
		break;

	case System::kUserAppSupportFolder :
		getHomeDirectory (path);
		succeeded = true;
		break;

	case System::kSharedDataFolder : CCL_FALLTHROUGH
	case System::kSharedSettingsFolder : CCL_FALLTHROUGH
	case System::kSharedSupportFolder :
		#ifdef CCL_PREFER_USERDATA_DIRECTORY
			path.fromDisplayString (String (Text::kSystemEncoding, ::getenv (kXdgDataHome)), IUrl::kFolder);
			if(path.isEmpty ())
			{
				getHomeDirectory (path);
				path.descend (".local/share", IUrl::kFolder);
			}
			succeeded = true;
		#else
			path.fromPOSIXPath (CCL_INSTALL_PREFIX "var/opt", Url::kFolder);
			succeeded = true;
		#endif
		break;

	case System::kAppSupportFolder :
		{
			char applicationPath[STRING_STACK_SPACE_MAX];
			ssize_t length = ::readlink ("/proc/self/exe", applicationPath, ARRAY_COUNT (applicationPath));
			if(length > 0 && length + 1 < STRING_STACK_SPACE_MAX)
			{
				applicationPath[length] = 0;
				path.fromPOSIXPath (applicationPath, IUrl::kFile);
				path.ascend ();
				succeeded = true;
			}
		}
		break;

	case System::kUserDownloadsFolder :
		if(!getXdgUserDir (path, kXdgDownloadDir))
		{
			getHomeDirectory (path);
			path.descend ("Downloads", IUrl::kFolder);
		}
		succeeded = true;
		break;

	case System::kUserDocumentFolder :
		if(!getXdgUserDir (path, kXdgDocumentsDir))
		{
			getHomeDirectory (path);
			path.descend ("Documents", IUrl::kFolder);
		}
		succeeded = true;
		break;

	case System::kUserMusicFolder :
		if(!getXdgUserDir (path, kXdgMusicDir))
		{
			getHomeDirectory (path);
			path.descend ("Music", IUrl::kFolder);
		}
		succeeded = true;
		break;

	case System::kDesktopFolder :
		if(!getXdgUserDir (path, kXdgDesktopDir))
		{
			getHomeDirectory (path);
			path.descend ("Desktop", IUrl::kFolder);
		}
		succeeded = true;
		break;

    case System::kAppPluginsFolder :
		succeeded = getNativeLocation (path, System::kAppSupportFolder);
		if(succeeded)
			path.descend (CCLSTR ("Plugins"), Url::kFolder);
		break;
	}

	return succeeded;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxSystemInformation::getHardDiskInfo (String& serial, String& model, String& volumeSerial, StringRef devicePath) const
{
	struct stat fileInfo = {};
	if(::stat (MutableCString (devicePath), &fileInfo) < 0)
		return false;

	CStringPtr type = nullptr;
	if(S_ISCHR (fileInfo.st_mode))
		type = "char";
	else if(S_ISBLK (fileInfo.st_mode))
		type = "block";
	else
		return false; // not a device

	MutableCString udevPath;
	udevPath.appendFormat ("/run/udev/data/%c%u:%u", type[0], major (fileInfo.st_rdev), minor (fileInfo.st_rdev));

	Java::PropertyFile file;
	Url fileUrl;
	fileUrl.fromPOSIXPath (udevPath);
	if(!file.loadFromFile (fileUrl))
		return false;

	const StringDictionary& properties = file.getProperties ();
	model = properties.lookupValue ("E:ID_MODEL");
	serial = properties.lookupValue ("E:ID_SERIAL_SHORT");
	volumeSerial = properties.lookupValue ("E:ID_FS_UUID");

	return !model.isEmpty () && !serial.isEmpty () && !volumeSerial.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxSystemInformation::getXdgUserDir (IUrl& url, StringID id) const
{
	Url homeDirectory;
	getHomeDirectory (homeDirectory);
	
	Vector<Url> directories;
	
	Url xdgConfigPath;
	xdgConfigPath.fromDisplayString (String (Text::kSystemEncoding, ::getenv (kXdgConfigHome)), IUrl::kFolder);
	directories.add (xdgConfigPath);
	
	Url defaultPath = homeDirectory;
	defaultPath.descend (".config", IUrl::kFolder);
	directories.add (defaultPath);
	
	String searchDirs (Text::kSystemEncoding, ::getenv (kXdgConfigDirs));
	while(!searchDirs.isEmpty ())
	{
		int pos = searchDirs.index (":");
		
		String directory = searchDirs.subString (0, pos);
		if(!directory.isEmpty ())
		{
			Url path;
			path.fromDisplayString (directory, IUrl::kFolder);
			directories.add (directory);
		}
		searchDirs = pos >= 0 ? searchDirs.subString (pos + 1) : "";
	}
	
	bool succeeded = false;
		
	for(UrlRef directory : directories)
	{
		xdgConfigPath = directory;
		xdgConfigPath.descend ("user-dirs.dirs");
		
		FILE* fp = ::fopen (MutableCString (UrlDisplayString (xdgConfigPath), Text::kSystemEncoding), "r");
		if(fp == nullptr)
			continue;

		char* lineBuffer = nullptr;
		size_t length = 0;
		ssize_t bytesRead = 0;
		while((bytesRead = ::getline (&lineBuffer, &length, fp)) != -1)
		{
			String line (Text::kSystemEncoding, lineBuffer);
			(::free) (lineBuffer);
			lineBuffer = nullptr;
			if(line.startsWith (String (Text::kSystemEncoding, id)))
			{
				int startPos = line.index ("=") + 2;
				int endPos = line.lastIndex ("\"");
				String value = line.subString (startPos, endPos - startPos);
				value.replace ("~", UrlDisplayString (homeDirectory));
				value.replace ("$HOME", UrlDisplayString (homeDirectory));
			
				url.fromDisplayString (value, IUrl::kFolder);
				succeeded = true;
				break;
			}
		}
		if(lineBuffer)
			(::free) (lineBuffer);
		::fclose (fp);
		
		if(succeeded)
			break;
	}
	
	return succeeded;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxSystemInformation::getLocalTime (DateTime& dateTime) const
{
	PosixTimeConversion::getLocalTime (dateTime);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxSystemInformation::convertLocalTimeToUTC (DateTime& utc, const DateTime& localTime) const
{
	PosixTimeConversion::convertLocalTimeToUTC (utc, localTime);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxSystemInformation::convertUTCToLocalTime (DateTime& localTime, const DateTime& utc) const
{
	PosixTimeConversion::convertUTCToLocalTime (localTime, utc);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxSystemInformation::convertUnixTimeToUTC (DateTime& utc, int64 unixTime) const
{
	PosixTimeConversion::convertUnixTimeToUTC (utc, unixTime);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API LinuxSystemInformation::convertUTCToUnixTime (const DateTime& utc) const
{
	return PosixTimeConversion::convertUTCToUnixTime (utc);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API LinuxSystemInformation::getSecureComputerTime () const
{
	return ::time (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxSystemInformation::getComputerName (String& name, int flags) const
{
	char hostname[STRING_STACK_SPACE_MAX];
	::gethostname (hostname, ARRAY_COUNT (hostname));
	name.appendCString (Text::kSystemEncoding, hostname);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxSystemInformation::getUserName (String& name, int flags) const
{
	uid_t uid = ::geteuid ();
	passwd* pw = ::getpwuid (uid);
	if(pw)
		name.appendCString (Text::kSystemEncoding, pw->pw_name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API LinuxSystemInformation::getNumberOfCPUs () const
{
	return getNumberOfCores ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API LinuxSystemInformation::getNumberOfCores () const
{
	cpu_set_t cpuSet;
	CPU_ZERO (&cpuSet);
	::sched_getaffinity (0, sizeof(cpuSet), &cpuSet);

	int count = 0;
	for(int i = 0; i < sizeof(cpuSet); i++)
	{
		if(!CPU_ISSET (i, &cpuSet))
			break;
		count++;
	}	
	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxSystemInformation::getMemoryInfo (System::MemoryInfo& memoryInfo) const
{
	long pages = ::sysconf (_SC_PHYS_PAGES);
	long pageSize = ::sysconf (_SC_PAGE_SIZE);
	
	memoryInfo.physicalRAMSize = pages * pageSize;
	
	long allocatedMemory = 0;
	FILE* fp = ::fopen ("/proc/self/statm", "r");
	if(fp)
	{
		::fscanf (fp, "%*s%ld", &allocatedMemory);
		::fclose (fp);
	}
	allocatedMemory *= pageSize;
	
	rlimit limit = {0};
	if(::getrlimit (RLIMIT_AS, &limit) == 0)
	{
		memoryInfo.processMemoryTotal = limit.rlim_cur;
		memoryInfo.processMemoryAvailable = memoryInfo.processMemoryTotal - allocatedMemory;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxSystemInformation::getComputerInfo (IAttributeList& attributes, int flags) const
{
	attributes.setAttribute (System::kDeviceModel, CCLSTR ("Linux"));
	
	String computerName;
	getComputerName (computerName);
	attributes.setAttribute (System::kDeviceIdentifier, computerName);
	
	if((flags & System::kQueryExtendedComputerInfo) == 0)
		return;

	// Operating System
	utsname utsName {};
	if(::uname (&utsName) == 0)
	{
		attributes.setAttribute (System::kOSName, String (Text::kUTF8, utsName.sysname));
		attributes.setAttribute (System::kOSVersion, String (Text::kUTF8, utsName.release));
	}
	
	// CPU
	FILE* fp = ::fopen ("/proc/cpuinfo", "r");
	if(fp)
	{
		char* lineBuffer = nullptr;
		size_t length = 0;
		ssize_t bytesRead = 0;
		while((bytesRead = ::getline (&lineBuffer, &length, fp)) != -1)
		{
			String line (Text::kUTF8, lineBuffer);
			(::free) (lineBuffer);
			lineBuffer = nullptr;
			String value = line.subString (line.index (":") + 1).trimWhitespace ();
			if(value.startsWith ("\""))
				value = value.subString (1, value.length () - 2);
			if(line.startsWith ("model name"))
				attributes.setAttribute (System::kCPUModelHumanReadable, value);
			else if(line.startsWith ("model"))
				attributes.setAttribute (System::kCPUIdentifier, value);
	    }
	    if(lineBuffer)
			(::free) (lineBuffer);
	    ::fclose (fp);
	}

	fp = ::fopen ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", "r");
	if(fp)
	{
		char* lineBuffer = nullptr;
		size_t length = 0;
		ssize_t bytesRead = 0;
		while((bytesRead = ::getline (&lineBuffer, &length, fp)) != -1)
		{
			String line (Text::kUTF8, lineBuffer);
			(::free) (lineBuffer);
			lineBuffer = nullptr;
			double floatValue = 0;
			line.getFloatValue (floatValue);
			int64 intValue = 0;
			line.getIntValue (intValue);
			attributes.setAttribute (System::kCPUSpeedMHz, floatValue / 1024.);
			attributes.setAttribute (System::kCPUSpeed, intValue);
	    }
	    if(lineBuffer)
			(::free) (lineBuffer);
	    ::fclose (fp);
	}
	
	// Memory
	System::MemoryInfo memoryInfo;
	getMemoryInfo (memoryInfo);
	attributes.setAttribute (System::kPhysicalRAMSize, memoryInfo.physicalRAMSize);
	
	// Ethernet
	bool foundPrimaryAdapter = false;

	int lowestInterfaceIndex = NumericLimits::kMaxInt32;
	String primaryMacAddress;
	String primaryAdapterName;

	Url netDevicePath;
	netDevicePath.fromPOSIXPath ("/sys/class/net");
	ForEachFile (System::GetFileSystem ().newIterator (netDevicePath, IFileIterator::kFolders), folderUrl)
		String adapterName;
		folderUrl->getName (adapterName);

		Url ueventUrl (*folderUrl);
		ueventUrl.descend ("uevent", IUrl::kFile);

		AutoPtr<IStream> fileStream = System::GetFileSystem ().openStream (ueventUrl, IStream::kReadMode | IStream::kShareRead);
		if(fileStream == nullptr)
			continue;
		AutoPtr<ITextStreamer> textStreamer = System::CreateTextStreamer (*fileStream, {Text::kUTF8, Text::kSystemLineFormat});
		
		String line;
		int currentInterfaceIndex = NumericLimits::kMaxInt32;
		while(!textStreamer->isEndOfStream ())
		{
			textStreamer->readLine (line);
			if(line.startsWith ("IFINDEX="))
			{
				String value = line.subString (8);
				value.getIntValue (currentInterfaceIndex);
				break;
			}
		}
		
		Url addressUrl (*folderUrl);
		addressUrl.descend ("address", IUrl::kFile);

		fileStream = System::GetFileSystem ().openStream (addressUrl, IStream::kReadMode | IStream::kShareRead);
		if(fileStream == nullptr)
			continue;
		textStreamer = System::CreateTextStreamer (*fileStream, {Text::kUTF8, Text::kSystemLineFormat});
		textStreamer->readLine (line);
		if(!line.isEmpty () && line != "00:00:00:00:00:00")
		{
			line.toUppercase ();

			if(currentInterfaceIndex < lowestInterfaceIndex)
			{
				lowestInterfaceIndex = currentInterfaceIndex;
				primaryAdapterName = adapterName;
				primaryMacAddress = line;
			}

			if(flags & System::kQueryEthernetAdapterList)
			{
				AutoPtr<IAttributeList> adapterAttr = AttributeAccessor (attributes).newAttributes ();
				adapterAttr->setAttribute (System::kMACAddress, line);
				adapterAttr->setAttribute (System::kEthernetAdapter, adapterName);
				attributes.queueAttribute (System::kEthernetAdapterList, static_cast<IAttributeList*> (adapterAttr), IAttributeList::kShare);
			}
		}
	EndFor

	if(!foundPrimaryAdapter && !primaryMacAddress.isEmpty ())
	{
		attributes.setAttribute (System::kMACAddress, primaryMacAddress);
		attributes.setAttribute (System::kEthernetAdapter, primaryAdapterName);
		foundPrimaryAdapter = true;
	}

	// Fallback: use a socket to find available network interfaces.
	// Does not work for inactive interfaces.
	if(!foundPrimaryAdapter)
	{
		int socket = ::socket (AF_INET, SOCK_DGRAM, IPPROTO_IP);
		if(socket >= 0)
		{
			ifreq record {};
			ifconf config {};
			
			char buffer[STRING_STACK_SPACE_MAX];
			config.ifc_len = sizeof(buffer);
			config.ifc_buf = buffer;
			if(::ioctl (socket, SIOCGIFCONF, &config) >= 0)
			{
				ifreq* it = config.ifc_req;
				ifreq* end = it + (config.ifc_len / sizeof(record));
				for(; it != end; it++)
				{
					::strcpy (record.ifr_name, it->ifr_name);
					if(::ioctl (socket, SIOCGIFFLAGS, &record) != 0)
						continue;
					if((record.ifr_flags & IFF_LOOPBACK) != 0)
						continue;
					if(::ioctl (socket, SIOCGIFHWADDR, &record) != 0)
						continue;

					String macAddress;
					for(int i = 0; i < 6; i++)
					{
						macAddress.appendHexValue ((uint8)record.ifr_hwaddr.sa_data[i], 2);
						if(i + 1 < 6)
							macAddress.append (":");
					}
					String adapterName (Text::kASCII, record.ifr_name);
					
					if(!foundPrimaryAdapter)
					{
						attributes.setAttribute (System::kMACAddress, macAddress);
						attributes.setAttribute (System::kEthernetAdapter, adapterName);
						foundPrimaryAdapter = true;
					}
					
					if(flags & System::kQueryEthernetAdapterList)
					{
						AutoPtr<IAttributeList> adapterAttr = AttributeAccessor (attributes).newAttributes ();
						adapterAttr->setAttribute (System::kMACAddress, macAddress);
						adapterAttr->setAttribute (System::kEthernetAdapter, adapterName);
						attributes.queueAttribute (System::kEthernetAdapterList, static_cast<IAttributeList*> (adapterAttr), IAttributeList::kShare);
					}
				}
			}
		}
		::close (socket);
	}
	
	// Disk
	String serial;
	String model;
	String volumeSerial;

	MountInfo info;
	if(info.load ())
	{
		Url appPath;
		if(getNativeLocation (appPath, System::kAppSupportFolder))
		{
			ASSERT (appPath.isAbsolute ())

			const MountInfo::Entry* mountPoint = info.find (appPath);
			if(mountPoint != nullptr)
			{
				getHardDiskInfo (serial, model, volumeSerial, mountPoint->mountSource);
				if(!serial.isEmpty ())
					attributes.setAttribute (System::kDiskSerialNumber, serial);
				if(!model.isEmpty ())
					attributes.setAttribute (System::kDiskModelHumanReadable, model);
				if(!volumeSerial.isEmpty ())
					attributes.setAttribute (System::kVolumeSerialNumber, volumeSerial);
			}
		}
			
		Url rootPath;
		rootPath.fromPOSIXPath ("/");
		const MountInfo::Entry* mountPoint = info.find (rootPath);
		if(mountPoint != nullptr)
		{
			getHardDiskInfo (serial, model, volumeSerial, mountPoint->mountSource);
			Variant value;
			if(!serial.isEmpty ())
				if(attributes.getAttribute (value, System::kDiskSerialNumber) == false || value.asString ().isEmpty ())
					attributes.setAttribute (System::kDiskSerialNumber, serial);
			if(!model.isEmpty ())
				if(attributes.getAttribute (value, System::kDiskModelHumanReadable) == false || value.asString ().isEmpty ())
					attributes.setAttribute (System::kDiskModelHumanReadable, model);
			if(!volumeSerial.isEmpty ())
			{
				if(attributes.getAttribute (value, System::kVolumeSerialNumber) == false || value.asString ().isEmpty ())
					attributes.setAttribute (System::kVolumeSerialNumber, volumeSerial);
				attributes.setAttribute (System::kSystemFolderFSID, volumeSerial);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API LinuxSystemInformation::searchApplications (StringRef filter) const
{
	AutoPtr<Url> userApps = NEW Url;

	userApps->fromDisplayString (String (Text::kSystemEncoding, ::getenv (kXdgDataHome)), IUrl::kFolder);
	if(userApps->isEmpty ())
	{
		getHomeDirectory (*userApps);
		userApps->descend (".local", IUrl::kFolder);
		userApps->descend ("share", IUrl::kFolder);
	}
	userApps->descend ("applications", IUrl::kFolder);
	
	AutoPtr<Url> localSystemApps = NEW Url;
	localSystemApps->fromPOSIXPath ("/usr/local/share/applications");
	
	AutoPtr<Url> systemApps = NEW Url;
	systemApps->fromPOSIXPath ("/usr/share/applications");
	
	UnknownList directories;
	directories.add (userApps->asUnknown (), true);
	directories.add (localSystemApps->asUnknown (), true);
	directories.add (systemApps->asUnknown (), true);
	
	ApplicationSearcher* searcher = NEW ApplicationSearcher ();
	searcher->find (filter, directories);
	return searcher;
}

//************************************************************************************************
// LinuxExecutableLoader
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (ExecutableLoader, LinuxExecutableLoader)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinuxExecutableLoader::loadImage (IExecutableImage*& image, UrlRef path)
{
	MutableCString systemPath (UrlDisplayString (path), Text::kSystemEncoding);
	void* handle = ::dlopen (systemPath.str (), RTLD_NOW);
	if(handle)
	{
		LinuxImage* lib = NEW LinuxImage (handle, true);
		image = lib;
		return kResultOk;
	}
	else
		CCL_WARN ("Module could not be loaded: %s\n", ::dlerror ());
	
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IExecutableImage* CCL_API LinuxExecutableLoader::createImage (ModuleRef module)
{
	return NEW LinuxImage (module, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinuxExecutableLoader::execute (Threading::ProcessID& processId, UrlRef path, ArgsRef args,
												  int flags, IUnknown* context)
{
	ASSERT (!path.isEmpty ())
	if(!System::GetFileSystem ().fileExists (path))
		return kResultFailed;
	
	String commandString;
	path.toDisplayString (commandString);
	commandString.replace (" ", "\\ ");
		
	// append arguments to command line
	String argumentString;
	args.toString (argumentString);
	commandString << " " << argumentString;

	// redirect stderr to stdout
	commandString << " 2>&1";
	
	MutableCString command (commandString, Text::kSystemEncoding);
	
	int fd[2] = {0};
	if(::pipe (fd) != 0)
		return kResultFailed;
	
	if((flags & System::kRedirectProcessOutput) == 0)
		if(::fcntl(fd[1], F_SETFD, ::fcntl (fd[1], F_GETFD) | FD_CLOEXEC) != 0)
			return kResultFailed;
	
	pid_t pid = 0;
	pid = ::fork ();
	if(pid == -1)
		return kResultFailed;
	else if(pid == 0)
	{
		// this is executed in the child process
		
		::close (fd[0]); // close read file descriptor
		
		if(flags & System::kRedirectProcessOutput)
			::dup2 (fd[1], 1); // redirect output to write file descriptor
		
		::setpgid (pid, pid);
		::execl ("/bin/sh", "/bin/sh", "-c", command.str (), nullptr);
		
		if((flags & System::kRedirectProcessOutput) == 0)
			::write (fd[1], &errno, sizeof(int));
		
		::_exit (0);
	}
	else
	{
		// this is executed in the parent process
		
		processId = pid;
		::close (fd[1]); // close write file descriptor
	}
	
	tresult result = kResultOk;
	
	UnknownPtr<IStream> stream;
	if(flags & System::kRedirectProcessOutput)
	{
		stream = context;
		
		FILE* fp = ::fdopen (fd[0], "r");
		if(fp == nullptr)
			return kResultFailed;
		
		char buffer[STRING_STACK_SPACE_MAX];
		while(::fgets (buffer, ARRAY_COUNT (buffer), fp) != nullptr)
		{
			if(stream)
				stream->write (buffer, int (::strlen (buffer)));
		}
		::fclose (fp);
	}
	else
	{
		ssize_t count = -1;
		int error = 0;
		while((count = ::read (fd[0], &error, sizeof(errno))) == -1)
		{
			if(errno != EAGAIN && errno != EINTR)
				break;
		}
		
		if(count)
		{
			CCL_WARN ("Child process exited with error: %s\n", ::strerror (error));
			return kResultFailed;
		}
	}

	if(flags & System::kWaitForProcessExit)
	{
		int stat;
		while(::waitpid (pid, &stat, 0) == -1)
		{
			if(errno != EINTR)
			{
				result = kResultFailed;
				break;
			}
		}
	}
	
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinuxExecutableLoader::relaunch (ArgsRef args)
{
	Url exePath;
	getMainImage ().getPath (exePath);
	Threading::ProcessID processId = 0;
	return execute (processId, exePath, args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinuxExecutableLoader::terminate (Threading::ProcessID processId)
{
	if(::kill (__pid_t (processId), SIGTERM) != 0)
		return kResultFailed;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinuxExecutableLoader::getExecutablePath (IUrl& path, Threading::ProcessID processId)
{
	MutableCString linkName;
	linkName.appendFormat ("/proc/%d/exe", processId);
	
	char executablePath[STRING_STACK_SPACE_MAX];
	ssize_t length = ::readlink (linkName.str (), executablePath, ARRAY_COUNT (executablePath));
	if(length <= 0 || length + 1 >= STRING_STACK_SPACE_MAX)
		return kResultFailed;
	executablePath[length] = 0;
	
	path.fromPOSIXPath (executablePath, IUrl::kFile);
	return kResultOk;
}

//************************************************************************************************
// LinuxImage
//************************************************************************************************

LinuxImage::LinuxImage (ModuleRef nativeRef, bool isLoaded)
: ExecutableImage (nativeRef, isLoaded)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxImage::~LinuxImage ()
{
	if(isLoaded && nativeRef)
		unload ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxImage::unload ()
{
	if(nativeRef)
		::dlclose (nativeRef);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* CCL_API LinuxImage::getFunctionPointer (CStringPtr name) const
{
	void* result = nullptr;
	if(nativeRef)
	{
		result = ::dlsym (nativeRef, name);
		if(!result)
			CCL_PRINTF ("Function pointer not found in library: %s\n", ::dlerror ());
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinuxImage::getPath (IUrl& url) const
{
	link_map* linkMap = nullptr;
	if(::dlinfo (nativeRef, RTLD_DI_LINKMAP, &linkMap) != 0 || linkMap == nullptr || linkMap->l_name == nullptr)
		return false;
	
	if(linkMap->l_name != nullptr && linkMap->l_name[0] != '\0')
	{
		url.fromPOSIXPath (linkMap->l_name);
		return true;
	}
	else
	{
		if(nativeRef == System::GetMainModuleRef ())
		{
			char applicationPath[STRING_STACK_SPACE_MAX];
			ssize_t length = ::readlink ("/proc/self/exe", applicationPath, ARRAY_COUNT (applicationPath));
			if(length > 0 && length + 1 < STRING_STACK_SPACE_MAX)
			{
				applicationPath[length] = '\0';
				url.fromPOSIXPath (applicationPath, IUrl::kFile);
				return true;
			}
		}
	}
	
	return false;
}

//************************************************************************************************
// ApplicationSearcher
//************************************************************************************************

void ApplicationSearcher::find (StringRef filter, const IUnknownList& directories)
{
	setResult (static_cast<IUnknownList*> (&resultList));
	resultList.removeAll ();
	Vector<Url> urls;

	ForEachUnknown (directories, unk)
		UnknownPtr<IUrl> directory (unk);
		if(directory)
			scan (filter, *directory);
	EndFor
	
	setState (kCompleted);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ApplicationSearcher::scan (StringRef filter, UrlRef directory)
{
	ForEachFile (System::GetFileSystem ().newIterator (directory, IFileIterator::kFiles), file)
		AutoPtr<IStream> fileStream = System::GetFileSystem ().openStream (*file, IStream::kReadMode | IStream::kShareRead);
		if(fileStream == nullptr)
			continue;
		AutoPtr<ITextStreamer> textStreamer = System::CreateTextStreamer (*fileStream, {Text::kUTF8, Text::kSystemLineFormat});
		
		AutoPtr<Attributes> attr = NEW Attributes;
		String line;
		String value;;
		while(!textStreamer->isEndOfStream ())
		{
			textStreamer->readLine (line);
			if(line.startsWith ("[") && !line.startsWith ("[Desktop Entry]"))
				break;
			else if(line.startsWith ("Name="))
			{
				value = line.subString (5);
				if(value.isEmpty ())
					continue;
				
				bool collectItem = filter.isEmpty ();
				ForEachStringToken (filter, CCLSTR (",;"), token)
					if(value.contains (token, false))
					{
						collectItem = true;
						attr->set (Meta::kPackageName, value);
						break;
					}
				EndFor
				
				if(!collectItem)
					break;
			}
			else if(line.startsWith ("Exec="))
			{
				value = line.subString (5);
				if(!value.isEmpty ())
					attr->set (Meta::kPackageExecutable, value);
			}
			else if(line.startsWith ("Icon="))
			{
				value = line.subString (5);
				if(!value.isEmpty ())
					attr->set (Meta::kPackageIcon, value);
			}
		}
		
		if(attr->contains (Meta::kPackageExecutable) && attr->contains (Meta::kPackageName))
			resultList.add (attr->asUnknown (), true);
	EndFor
}

