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
// Filename    : ccl/system/system.cpp
// Description : System class
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/system/system.h"

#include "ccl/base/message.h"
#include "ccl/base/boxedtypes.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/attributes.h"

#include "ccl/public/text/istringdict.h"
#include "ccl/public/system/ipackagemetainfo.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/cclversion.h"

#include <time.h>

namespace CCL {

//************************************************************************************************
// CRTTypeConverter
//************************************************************************************************

namespace CRTTypeConverter
{
	time_t to_time_t (int64 unixTime)
	{
		return static_cast<time_t> (unixTime);
	}

	void tmToDateTime (DateTime& dateTime, const tm& t)
	{
		dateTime.setDate (Date (1900 + t.tm_year, t.tm_mon + 1, t.tm_mday));
		dateTime.setTime (Time (t.tm_hour, t.tm_min, t.tm_sec));
	}

	void tmFromDateTime (tm& t, const DateTime& dateTime)
	{
		t.tm_sec = dateTime.getTime ().getSecond ();
		t.tm_min = dateTime.getTime ().getMinute ();
		t.tm_hour = dateTime.getTime ().getHour ();
		t.tm_isdst = -1;

		t.tm_mday = dateTime.getDate ().getDay ();
		t.tm_mon = dateTime.getDate ().getMonth () - 1;
		t.tm_year = dateTime.getDate ().getYear () - 1900;
		t.tm_wday = 0;
		t.tm_yday = 0;
	}
}

//************************************************************************************************
// ExecutableIterator
//************************************************************************************************

class ExecutableIterator: public Unknown,
						  public IExecutableIterator
{
public:
	ExecutableIterator (const ExecutableImageList& imageList)
	: snapshot (imageList, true),
	  snapshotIterator (snapshot)
	{}

	// IExecutableIterator
	const IExecutableImage* CCL_API getNextImage () override 
	{
		return snapshotIterator.next ();
	}

	CLASS_INTERFACE (IExecutableIterator, Unknown)

protected:
	ExecutableImageList snapshot;
	ExecutableImageList::Iterator snapshotIterator;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (Thread, kFrameworkLevelFirst)
{
	Threading::NativeThread::initMainThread ();
	SystemInformation::instance (); // enforce singleton creation
	ExecutableLoader::instance ();

	#if DEBUG // check if Unix time / UTC conversion works correctly
	int64 time1 = SystemInformation::instance ().getUnixTime ();
	int64 time2 = UnixTime::fromUTC (UnixTime::toUTC (time1));
	ASSERT (time1 == time2)
	#endif

	return true;
}

CCL_KERNEL_TERM_LEVEL (Thread, kFrameworkLevelFirst)
{
	Threading::NativeThread::exitMainThread ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Services API
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ISystemInformation& CCL_API System::CCL_ISOLATED (GetSystem) ()
{
	return SystemInformation::instance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IExecutableLoader& CCL_API System::CCL_ISOLATED (GetExecutableLoader) ()
{
	return ExecutableLoader::instance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT StringRef CCL_API System::CCL_ISOLATED (GetModuleIdentifier) (String& result, ModuleRef module)
{
	result.empty ();
	int64 value = (int64)module;
	result.appendHexValue (value, 2 * sizeof(void*));
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ModuleRef CCL_API System::CCL_ISOLATED (GetModuleWithIdentifier) (StringRef string)
{
	if(string.isEmpty ())
		return System::GetMainModuleRef ();

	int64 value = 0;
	string.getHexValue (value);
	return (ModuleRef)value;
}

//************************************************************************************************
// SystemInformation
//************************************************************************************************

DEFINE_CLASS (SystemInformation, Object)
DEFINE_CLASS_NAMESPACE (SystemInformation, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

SystemInformation::SystemInformation ()
: contentLocation (*NEW Url),
  deploymentLocation (*NEW Url),
  appProductVersion (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SystemInformation::~SystemInformation ()
{
	contentLocation.release ();
	deploymentLocation.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SystemInformation::setApplicationName (StringRef companyName, StringRef productName, int productVersion)
{
	appCompanyName = LegalFileName (companyName);
	appProductName = LegalFileName (productName);
	appProductVersion = productVersion;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SystemInformation::setLocation (System::FolderType type, UrlRef url)
{
	if(type == System::kUserContentFolder)
	{
		if(contentLocation != url)
		{
			Url oldLocation (contentLocation);
			contentLocation = url;
			SignalSource (Signals::kSystemInformation).signal (Message (Signals::kContentLocationChanged, contentLocation.asUnknown (), oldLocation.asUnknown ()));
		}
		return true;
	}

	if(type == System::kAppDeploymentFolder)
	{
		if(deploymentLocation != url)
		{
			deploymentLocation = url;
			SignalSource (Signals::kSystemInformation).signal (Message (Signals::kDeploymentLocationChanged, deploymentLocation.asUnknown ()));
		}
		return true;
	}

	CCL_DEBUGGER ("Location can not be set!")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SystemInformation::getCompanySpecificFolder (IUrl& url, System::FolderType baseType) const
{
	if(!appCompanyName.isEmpty ()) // company name is optional
	{
		if(!getNativeLocation (url, baseType))
			return false;

		url.descend (appCompanyName, IUrl::kFolder);
		return true;
	}
	else // fall back to application-specific folder
		return getAppSpecificFolder (url, baseType);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SystemInformation::getAppSpecificFolder (IUrl& url, System::FolderType baseType, int flags) const
{
	if(!getNativeLocation (url, baseType))
		return false;

	if(!appCompanyName.isEmpty ()) // company name is optional
		url.descend (appCompanyName, IUrl::kFolder);

	ASSERT (appProductName.isEmpty () == false)
	if(!appProductName.isEmpty ())
	{
		String productFolder (appProductName);
		if((flags & kIsVersionSpecific) && appProductVersion > 0)
			productFolder << " " << appProductVersion;
		
		url.descend (productFolder, IUrl::kFolder);

		if(flags & kIsPlatformSpecific)
		{
			#if CCL_PLATFORM_ARM
			static const String platformFolder64 ("Arm64");
			static const String platformFolder32 ("Arm");
			#elif CCL_PLATFORM_ARM64EC
			static const String platformFolder64 ("Arm64EC");
			static const String platformFolder32 ("Arm");
			#else
			static const String platformFolder64 ("x64");
			static const String platformFolder32 ("x86");
			#endif

			String folderName;
			#if CCL_PLATFORM_64BIT
			folderName = platformFolder64;
			#else
			folderName = platformFolder32;
			#endif

			url.descend (folderName, IUrl::kFolder);
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SystemInformation::getLocation (IUrl& url, System::FolderType type) const
{
	if(type == System::kUserContentFolder)
	{
		if(contentLocation.isEmpty ())
		{
			getNativeLocation (contentLocation, System::kUserDocumentFolder);
			
			#if CCL_PLATFORM_DESKTOP
			ASSERT (appProductName.isEmpty () == false)
			if(!appProductName.isEmpty ())
				contentLocation.descend (appProductName, IUrl::kFolder);
			#endif
		}

		url.assign (contentLocation);
		return true;
	}

	if(type == System::kAppDeploymentFolder)
	{
		if(deploymentLocation.isEmpty ())
			return getLocation (url, System::kAppSupportFolder);
		else
		{
			url.assign (deploymentLocation);
			return true;
		}
	}

	if(type == System::kAppFactoryContentFolder)
	{
		if(getNativeLocation (url, type))
			return true;
		return getLocation (url, System::kUserContentFolder);
	}

	switch(type)
	{
	case System::kCompanySettingsFolder :
		return getCompanySpecificFolder (url, System::kSharedSettingsFolder);
	case System::kCompanySupportFolder :
		return getCompanySpecificFolder (url, System::kSharedSupportFolder);
	case System::kCompanyContentFolder :
		return getCompanySpecificFolder (url, System::kSharedDataFolder);

	case System::kSharedContentFolder : 
		return getAppSpecificFolder (url, System::kSharedDataFolder);

	case System::kAppSettingsFolder :
		return getAppSpecificFolder (url, System::kUserSettingsFolder, kIsVersionSpecific);

	case System::kAppSettingsPlatformFolder :
		return getAppSpecificFolder (url, System::kUserSettingsFolder, kIsVersionSpecific|kIsPlatformSpecific);
		
	case System::kSharedAppSettingsFolder :
		return getAppSpecificFolder (url, System::kSharedSettingsFolder, kIsVersionSpecific);
	}

	return getNativeLocation (url, type);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SystemInformation::resolveLocation (IUrl& resolved, UrlRef url) const
{
	StringRef symbol (url.getHostName ());
	static const String dollarSign ("$");
	if(symbol.startsWith (dollarSign))
	{
		static struct { const char* name; System::FolderType type; } folderSymbols[] =
		{
			{"SYSTEM",			System::kSystemFolder},
			{"PROGRAMS",		System::kProgramsFolder},
			{"SHAREDDATA",		System::kSharedDataFolder},
			{"SHAREDSETTINGS",	System::kSharedSettingsFolder},

			{"TEMP",			System::kTempFolder},
			{"DESKTOP",			System::kDesktopFolder},
			{"USERSETTINGS",	System::kUserSettingsFolder},
			{"USERPREFERENCES",	System::kUserPreferencesFolder},
			{"USERDOCS",		System::kUserDocumentFolder},
			{"USERMUSIC",		System::kUserMusicFolder},
			{"DOWNLOADS",		System::kUserDownloadsFolder},

			{"USERCONTENT",		System::kUserContentFolder},
			{"SHAREDCONTENT",	System::kSharedContentFolder},
			{"APPSETTINGS",		System::kAppSettingsFolder},
			{"APPSETTINGSPLATFORM",	System::kAppSettingsPlatformFolder},
			{"APPSETTINGSALL",	System::kSharedAppSettingsFolder},
			{"APPSUPPORT",		System::kAppSupportFolder},
			{"DEPLOYMENT",		System::kAppDeploymentFolder}
		};


		System::FolderType type = -1;
		MutableCString name (symbol.subString (1));
		for(int i = 0; i < ARRAY_COUNT (folderSymbols); i++)
		{
			if(name.compare (folderSymbols[i].name, false) == 0)
			{
				type = folderSymbols[i].type;
				break;
			}
		}

		if(type != -1)
		{
			if(getLocation (resolved, type))
			{
				resolved.descend (url.getPath (), url.getType ());
				return true;
			}
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SystemInformation::getNativeLocation (IUrl& path, System::FolderType folderType) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SystemInformation::getLocalTime (DateTime& dateTime) const
{
	CCL_NOT_IMPL ("Implement in derived class!\n")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SystemInformation::convertLocalTimeToUTC (DateTime& utc, const DateTime& localTime) const
{
	CCL_NOT_IMPL ("Implement in derived class!\n")
	utc = localTime;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SystemInformation::convertUTCToLocalTime (DateTime& localTime, const DateTime& utc) const
{
	CCL_NOT_IMPL ("Implement in derived class!\n")
	localTime = utc;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SystemInformation::convertUnixTimeToUTC (DateTime& utc, int64 unixTime) const
{
	CCL_NOT_IMPL ("Implement in derived class!\n")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API SystemInformation::convertUTCToUnixTime (const DateTime& utc) const
{
	CCL_NOT_IMPL ("Implement in derived class!\n")
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API SystemInformation::getUnixTime () const
{
	return ::time (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API SystemInformation::getSecureComputerTime () const
{
	return getUnixTime ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SystemInformation::getComputerName (String& name, int flags) const
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SystemInformation::getUserName (String& name, int flags) const
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API SystemInformation::getNumberOfCPUs () const
{
	return 1; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API SystemInformation::getNumberOfCores () const
{
	return getNumberOfCPUs ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SystemInformation::getMemoryInfo (System::MemoryInfo& memoryInfo) const
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SystemInformation::getComputerInfo (IAttributeList& attributes, int flags) const
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API SystemInformation::searchApplications (StringRef filter) const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SystemInformation::terminate ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SystemInformation::isProcessSandboxed () const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (SystemInformation)
	DEFINE_METHOD_ARGR ("getLocalTime", nullptr, "DateTime")
END_METHOD_NAMES (SystemInformation)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SystemInformation::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "getLocalTime")
	{
		AutoPtr<Boxed::DateTime> dt = NEW Boxed::DateTime;
		getLocalTime (*dt);
		returnValue.takeShared (dt->asUnknown ());
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// ExecutableLoader
//************************************************************************************************

ExecutableLoader::ExecutableLoader ()
: mainImage (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ExecutableLoader::~ExecutableLoader ()
{
	if(mainImage != nullptr)
	{
		removeImage (mainImage);
		mainImage->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExecutableLoader::addImage (IExecutableImage* image)
{
	lock.lock ();

	#if DEBUG_LOG
	Url path;
	image->getPath (path);
	CCL_PRINTF ("ExecutableLoader::addImage image=%p path=%s\n", image, MutableCString (path.getPath()).str ());
	#endif
	
	imageList.append (image);
    
	#if DEBUG_LOG
	dump ();
    #endif
	
	lock.unlock ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExecutableLoader::removeImage (IExecutableImage* image)
{
	lock.lock ();

	CCL_PRINTF ("ExecutableLoader::removeImage image=%p\n", image);
	
	imageList.remove (image);
    
	#if DEBUG_LOG
	dump ();
    #endif

	lock.unlock ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IExecutableImage& CCL_API ExecutableLoader::getMainImage ()
{
	if(mainImage == nullptr)
		addImage (mainImage = createImage (System::GetMainModuleRef ()));
	return *mainImage;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ExecutableLoader::addNativeImage (ModuleRef module)
{
	#if DEBUG
	lock.lock ();
	ListForEach (imageList, IExecutableImage*, image)
		if(image->getNativeReference () == module)
			CCL_DEBUGGER ("Executable image already registered!\n")
	EndFor
	lock.unlock ();
	#endif

	CCL_PRINTF ("ExecutableLoader::addNativeImage module=%p\n", module);
	addImage (createImage (module));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ExecutableLoader::removeNativeImage (ModuleRef module)
{
	CCL_PRINTF ("ExecutableLoader::removeNativeImage module=%p\n", module) ;

	lock.lock ();
	ListForEach (imageList, IExecutableImage*, image)
		if(image->getNativeReference () == module)
		{
			imageList.remove (image);
			unsigned int refCount = image->release ();
			ASSERT (refCount == 0)
			break;
		}
	EndFor
	lock.unlock ();
	
	#if DEBUG_LOG
	dump ();
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ExecutableLoader::loadImage (IExecutableImage*& image, UrlRef path)
{
	image = nullptr;
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IExecutableImage* CCL_API ExecutableLoader::createImage (ModuleRef module)
{
	return NEW ExecutableImage (module, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IExecutableIterator* CCL_API ExecutableLoader::createIterator ()
{
	ExecutableIterator* iterator = nullptr;
	lock.lock ();
	iterator = NEW ExecutableIterator (imageList);
	lock.unlock ();
	return iterator;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ExecutableLoader::execute (Threading::ProcessID& processId, UrlRef path, ArgsRef args,
										   int flags, IUnknown* context)
{
	processId = 0;
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ExecutableLoader::relaunch (ArgsRef args) 
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ExecutableLoader::terminate (Threading::ProcessID processId)
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ExecutableLoader::getExecutablePath (IUrl& path, Threading::ProcessID processId)
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ExecutableLoader::isProcessRunning (UrlRef executableFile)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ExecutableLoader::getModuleInfo (IAttributeList& attributes, UrlRef path)
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG
void ExecutableLoader::dump ()
{
	#if DEBUG_LOG
	CCL_PRINTF ("ExecutableLoader number of images loaded: %d\n", imageList.count ());
	ListForEach (imageList, IExecutableImage*, anImage)
	   CCL_PRINTF (">> image=%p\n", anImage);
	EndFor
	#endif
}
#endif

//************************************************************************************************
// ExecutableImageList
//************************************************************************************************

ExecutableImageList::ExecutableImageList (bool shared)
: shared (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ExecutableImageList::ExecutableImageList (const ExecutableImageList& other, bool shared)
: shared (shared)
{
	ListForEach (other, IExecutableImage*, image)
		append (image);
		if(shared)
			image->retain ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ExecutableImageList::~ExecutableImageList ()
{
	if(shared)
	{
		ListForEach (*this, IExecutableImage*, image)
			image->release ();
		EndFor
	}
}

//************************************************************************************************
// ExecutableImage
//************************************************************************************************

ExecutableImage::ExecutableImage (ModuleRef nativeRef, bool isLoaded)
: nativeRef (nativeRef),
  isLoaded (isLoaded),
  metaInfo (nullptr)
{
	if(isLoaded)
		ExecutableLoader::instance ().addImage (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ExecutableImage::~ExecutableImage ()
{
	if(isLoaded)
	{
		nativeRef = nullptr;
		ExecutableLoader::instance ().removeImage (this);
	}

	if(metaInfo)
		metaInfo->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ExecutableImage::getPath (IUrl& path) const
{
	ASSERT (0)
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ExecutableImage::getIdentifier (String& id) const
{
	id.empty ();

	if(getMetaInfo ())
		id = metaInfo->getString (Meta::kPackageID);
	
	if(id.isEmpty ())
	{
		int64 value = (int64)nativeRef;
		id.appendHexValue (value, 2 * sizeof(void*));
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ModuleRef CCL_API ExecutableImage::getNativeReference () const 
{ 
	return nativeRef; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* CCL_API ExecutableImage::getFunctionPointer (CStringPtr name) const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IAttributeList* CCL_API ExecutableImage::getMetaInfo () const
{
	return metaInfo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ExecutableImage::getBinaryPath (IUrl& path) const
{
	return getPath (path);
}
