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
// Filename    : ccl/platform/cocoa/system/system.cocoa.mm
// Description : OSX System Information
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/cocoa/system/system.shared.h"

#include "ccl/platform/cocoa/cclcocoa.h"
#include "ccl/platform/cocoa/macutils.h"
#include "core/platform/cocoa/macosversion.h"

#include "ccl/main/cclargs.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/asyncoperation.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ipackagemetainfo.h"
#include "ccl/public/system/threadsync.h"
#include "ccl/public/collections/unknownlist.h"

#include <sys/sysctl.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <mach/kern_return.h>

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/network/IOEthernetInterface.h>
#include <IOKit/network/IONetworkInterface.h>
#include <IOKit/network/IOEthernetController.h>
#include <IOKit/storage/IOBlockStorageDevice.h>

using namespace CCL;

static kern_return_t findService (io_iterator_t* matchingServices, const char* bsdName);
static kern_return_t getNetworkAddressAndName (io_iterator_t iterator, UInt8* macAddress, String& name);
static kern_return_t getDiskName (io_iterator_t iterator, String& name);

namespace CCL {

//************************************************************************************************
// OSXSystemInformation
//************************************************************************************************

class OSXSystemInformation: public CocoaSystemInformation
{
public:
	OSXSystemInformation ();

	// CocoaSystemInformation
	void CCL_API getComputerName (String& name, int flags = 0) const override;
	void CCL_API getUserName (String& name, int flags = 0) const override;
	void CCL_API getMemoryInfo (System::MemoryInfo& memoryInfo) const override;
	void CCL_API getComputerInfo (IAttributeList& attributes, int flags = 0) const override;
	IAsyncOperation* CCL_API searchApplications (StringRef filter) const override;
	tbool CCL_API isProcessSandboxed () const override;
	
protected:
	bool sandboxed;
	mutable Threading::CriticalSection lock;
	
	void checkSandbox ();
	
	// CocoaSystemInformation
	bool getNativeLocation (IUrl& url, System::FolderType type) const override;
};

//************************************************************************************************
// OSXExecutableLoader
//************************************************************************************************

class OSXExecutableLoader: public ExecutableLoader
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
	tbool CCL_API isProcessRunning (UrlRef executableFile) override;
	tresult CCL_API getModuleInfo (IAttributeList& attributes, UrlRef path) override;
};

//************************************************************************************************
// ApplicationSearcher
//************************************************************************************************

class ApplicationSearcher: public AsyncOperation
{
public:
	ApplicationSearcher ();
	~ApplicationSearcher ();
	
	void find (StringRef filter);
	void searchCompleted ();
	
protected:
	UnknownList resultList;
	NSMetadataQuery* query;
	id<NSObject> spotlightObserver;
	
	void cleanUp ();
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// OSXSystemInformation
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (SystemInformation, OSXSystemInformation)

//////////////////////////////////////////////////////////////////////////////////////////////////

OSXSystemInformation::OSXSystemInformation ()
: sandboxed (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OSXSystemInformation::checkSandbox ()
{
	CFObj<SecCodeRef> me;
	OSStatus err = SecCodeCopySelf (kSecCSDefaultFlags, &(me.ref));
	ASSERT (err == errSecSuccess);
	CFObj<CFDictionaryRef> dynamicInfo;
	err = SecCodeCopySigningInformation (me, static_cast<SecCSFlags> (kSecCSDynamicInformation), &(dynamicInfo.ref));
	ASSERT (err == errSecSuccess);
	if(err == errSecSuccess)
	{
		CFDictionaryRef entitlements = static_cast<CFDictionaryRef> (CFDictionaryGetValue (dynamicInfo, kSecCodeInfoEntitlementsDict));
		if(entitlements)
		{
			CFBooleanRef sandboxEnabled = static_cast<CFBooleanRef> (CFDictionaryGetValue (entitlements, @"com.apple.security.app-sandbox"));
			if(sandboxEnabled)
				sandboxed = CFBooleanGetValue (sandboxEnabled) ? true : false;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OSXSystemInformation::getNativeLocation (IUrl& path, System::FolderType type) const
{
	bool result = false;

	if(System::GetSystem ().isProcessSandboxed ())
	{
		switch(type)
		{
		case System::kSystemFolder :
			return false;
			break;

		case System::kProgramsFolder :
			return false;
			break;

		case System::kSharedDataFolder :
			type = System::kUserSettingsFolder;
			break;

		case System::kSharedSettingsFolder :
			type = System::kUserSettingsFolder;
			break;

		case System::kSharedSupportFolder :
			type = System::kUserSettingsFolder;
			break;
		}
	}
	
	switch(type)
	{
	// /System
	case System::kSystemFolder :
		result = MacUtils::urlFromNSUrl (path, [NSURL fileURLWithPath:@"/System" isDirectory:YES], IUrl::kFolder);
		break;

	// /Applications
	case System::kProgramsFolder :
		result = MacUtils::urlFromNSUrl (path, [[NSFileManager defaultManager] URLForDirectory:NSApplicationDirectory inDomain:NSLocalDomainMask appropriateForURL:nil create:NO error:nil], IUrl::kFolder);
		break;

	// /var/folders/1f/zd10rxwj4457m_32x8x4_kd40000gn/T/TemporaryItems/
	case System::kTempFolder :
		result = MacUtils::urlFromNSUrl (path, [NSURL fileURLWithPath:NSTemporaryDirectory () isDirectory:YES], IUrl::kFolder);
		break;

	// /Applications/My\ Application.app/Contents
	case System::kAppSupportFolder :
		result = MacUtils::urlFromNSUrl (path, [[[NSBundle mainBundle] resourceURL] URLByDeletingLastPathComponent], IUrl::kFolder);
		break;
	
	// /Users/Shared
	case System::kSharedDataFolder :
		result = MacUtils::urlFromNSUrl (path, [[[NSFileManager defaultManager] URLForDirectory:NSUserDirectory inDomain:NSLocalDomainMask appropriateForURL:nil create:NO error:nil] URLByAppendingPathComponent:@"Shared" isDirectory:YES], IUrl::kFolder);
		break;

	// /Users/$USER/Music
	case System::kUserMusicFolder :
		result = MacUtils::urlFromNSUrl (path, [[NSFileManager defaultManager] URLForDirectory:NSMusicDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:NO error:nil], IUrl::kFolder);
		break;

	// /Users/$USER/Downloads
	case System::kUserDownloadsFolder :
		result = MacUtils::urlFromNSUrl (path, [[NSFileManager defaultManager] URLForDirectory:NSDownloadsDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:NO error:nil], IUrl::kFolder);
		break;

	// /Users/$USER/Documents
	case System::kUserDocumentFolder :
		result = MacUtils::urlFromNSUrl (path, [[NSFileManager defaultManager] URLForDirectory:NSDocumentDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:NO error:nil], IUrl::kFolder);
		if(result)
			// if in sandbox container, do not use
			if(path.getPath().contains (String ("Library/Containers")))
				result = false;
		break;
	
	// /Users/$USER/Library/Preferences
	case System::kUserPreferencesFolder :
		result = MacUtils::urlFromNSUrl (path, [[[NSFileManager defaultManager] URLForDirectory:NSLibraryDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:NO error:nil]  URLByAppendingPathComponent:@"Preferences" isDirectory:YES], IUrl::kFolder);
		break;

	// /Users/$USER/Library
	case System::kUserAppSupportFolder :
		result = MacUtils::urlFromNSUrl (path, [[NSFileManager defaultManager] URLForDirectory:NSLibraryDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:NO error:nil], IUrl::kFolder);
		break;

	// /Users/$USER/Library/Application\ Support
	case System::kUserSettingsFolder :
		result = MacUtils::urlFromNSUrl (path, [[NSFileManager defaultManager] URLForDirectory:NSApplicationSupportDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:NO error:nil], IUrl::kFolder);
		break;
			
	// /Users/Shared/Library/Application Support
	case System::kSharedSettingsFolder :
		result = MacUtils::urlFromNSUrl (path, [[[NSFileManager defaultManager] URLForDirectory:NSUserDirectory inDomain:NSLocalDomainMask appropriateForURL:nil create:NO error:nil] URLByAppendingPathComponent:@"Shared/Library/Application Support" isDirectory:YES], IUrl::kFolder);
		break;

	// /Users/$USER/Desktop
	case System::kDesktopFolder :
		result = MacUtils::urlFromNSUrl (path, [[NSFileManager defaultManager] URLForDirectory:NSDesktopDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:NO error:nil], IUrl::kFolder);
		break;

	// /Library/Application Support
	case System::kSharedSupportFolder :
		result = MacUtils::urlFromNSUrl (path, [[NSFileManager defaultManager] URLForDirectory:NSApplicationSupportDirectory inDomain:NSLocalDomainMask appropriateForURL:nil create:NO error:nil], IUrl::kFolder);
		break;
	
	// /Applications/My\ Application.app/Contents/Plugins
	case System::kAppPluginsFolder :
		result = MacUtils::urlFromNSUrl (path, [[NSBundle mainBundle] builtInPlugInsURL], IUrl::kFolder);
		break;
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API OSXSystemInformation::getComputerName (String& name, int flags) const
{
	name.empty ();
	name.appendNativeString ([[NSHost currentHost] localizedName]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API OSXSystemInformation::getUserName (String& name, int flags) const
{
	name.empty ();
	name.appendNativeString (NSFullUserName ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API OSXSystemInformation::getMemoryInfo (System::MemoryInfo& memoryInfo) const
{
	static int64 memSize = 0;
	if(memSize == 0)
	{
		size_t size = sizeof(memSize);
		if(sysctlbyname ("hw.memsize", &memSize, &size, nullptr, 0) != 0)
			memSize = 1 * 1024 * 1024 * 1024;  // 1 GB Minimum
	}
	memoryInfo.physicalRAMSize = memSize;
	
	task_basic_info t_info;
	mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;

	task_info (mach_task_self(), TASK_BASIC_INFO, (task_info_t)&t_info, &t_info_count);
	
	#if CCL_PLATFORM_64BIT
	memoryInfo.processMemoryTotal =  1L << 48; // 256TB (AMD64 Architecture Programmer's Manual)
	#else
	memoryInfo.processMemoryTotal = NumericLimits::kMaxUnsignedInt; // 4GB
	#endif
	memoryInfo.processMemoryAvailable = memoryInfo.processMemoryTotal - t_info.resident_size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API OSXSystemInformation::getComputerInfo (IAttributeList& attributes, int flags) const
{
	attributes.setAttribute (System::kDeviceModel, CCLSTR ("Mac"));

	String computerName;
	getComputerName (computerName);
	attributes.setAttribute (System::kDeviceIdentifier, computerName);
	
	if((flags & System::kQueryExtendedComputerInfo) == 0)
		return;
	
	// Operating system
	attributes.setAttribute (System::kOSName, CCLSTR ("macOS"));
	int major = 0;
	int minor = 0;
	int patch = 0;
	if(GetMacOSVersion (major, minor, patch))
	{
		String version;
		version << major << "." << minor << "." << patch;
		attributes.setAttribute (System::kOSVersion, version);
	}
	
	// CPU information
	struct clockinfo cinfo = {0};
	size_t size = sizeof(cinfo);
	if(sysctlbyname ("hw.cpufrequency_max", &cinfo, &size, nullptr, 0) == 0 && cinfo.hz != 0)
	{
		uint32 speed = (uint32)cinfo.hz;
		speed /= 1000000;
		attributes.setAttribute (System::kCPUSpeedMHz, (int64)speed);
		speed *= 1000000; // Cut digits
		attributes.setAttribute (System::kCPUSpeed, (int)speed);
	}
	else
	{
		attributes.setAttribute (System::kCPUModelHumanReadable, 1000);
		attributes.setAttribute (System::kCPUIdentifier, 10000000);
	}
	
	int64 memSize = 0;
	size = sizeof(memSize);
	if(sysctlbyname ("hw.memsize", &memSize, &size, nullptr, 0) == 0)
		attributes.setAttribute (System::kPhysicalRAMSize, memSize);

	int type = 0;
	size = sizeof(type);
	if(sysctlbyname ("hw.cputype", &type, &size, nullptr, 0) == 0)
		attributes.setAttribute (System::kCPUIdentifier, type);
	
	char label[256];
	size_t labelSize = sizeof(label);
	if(sysctlbyname ("machdep.cpu.brand_string", &label, &labelSize, nullptr, 0) == 0)
		attributes.setAttribute (System::kCPUModelHumanReadable, label);

	// hard disk name
	kern_return_t kernResult = KERN_SUCCESS;
	io_iterator_t serviceIterator = IO_OBJECT_NULL;
	String diskName;
	kernResult = findService (&serviceIterator, "disk0");
	if(kernResult == KERN_SUCCESS)
		kernResult = getDiskName (serviceIterator, diskName);
	attributes.setAttribute (System::kDiskModelHumanReadable, diskName);

	// MAC address
	kernResult = KERN_SUCCESS;
	serviceIterator = IO_OBJECT_NULL;
	UInt8 macAddress[kIOEthernetAddressSize] = { 0 };
	String adapterName;
	kernResult = findService (&serviceIterator, "en0");
	if(kernResult == KERN_SUCCESS)
		kernResult = getNetworkAddressAndName (serviceIterator, macAddress, adapterName);
	if(serviceIterator != IO_OBJECT_NULL)
		IOObjectRelease (serviceIterator);

	MutableCString macString;
	macString.appendFormat ("%02x-%02x-%02x-%02x-%02x-%02x", macAddress[0], macAddress[1], macAddress[2], macAddress[3], macAddress[4], macAddress[5]);
	attributes.setAttribute (System::kMACAddress, String (macString));
	attributes.setAttribute (System::kEthernetAdapter, adapterName);
	
	labelSize = sizeof(label);
	if(sysctlbyname ("hw.product", &label, &labelSize, nullptr, 0) == 0)
		attributes.setAttribute (System::kDeviceModelSubtype, label);
	
	int flag = 0;
	size_t flagSize = sizeof(flag);
	if(sysctlbyname ("sysctl.proc_translated", &flag, &flagSize, nullptr, 0) == 0)
		attributes.setAttribute (System::kProcessIsTranslated, flag);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API OSXSystemInformation::searchApplications (StringRef filter) const
{
	ApplicationSearcher* searcher = NEW ApplicationSearcher ();
	searcher->find (filter);
	return searcher;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API OSXSystemInformation::isProcessSandboxed () const
{
	static bool initialized = false;
	if(!initialized)
	{
		Threading::ScopedTryLock tryLock (lock);
		if(tryLock.success)
		{
			// determine if the app is sandboxed (this has to be called from a background thread, as used by a global dispatch queue)
			dispatch_async (dispatch_get_global_queue (QOS_CLASS_USER_INITIATED, 0), ^
			{
				ccl_const_cast (this)->checkSandbox ();
				initialized = true;
			});
		}

		while(!initialized)
			System::ThreadSleep (10);
	}

	return sandboxed;
}

//************************************************************************************************
// OSXExecutableLoader
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (ExecutableLoader, OSXExecutableLoader)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API OSXExecutableLoader::loadImage (IExecutableImage*& image, UrlRef path)
{
	String pathString;
	path.toDisplayString (pathString);
	
	CFObj<CFStringRef> nativePathString = pathString.createNativeString<CFStringRef> ();
	CFObj<CFURLRef> bundleURL = CFURLCreateWithFileSystemPath (NULL, nativePathString, kCFURLPOSIXPathStyle, false);
	if(bundleURL != NULL)
	{
		CFBundleRef bundle = CFBundleCreate (0, bundleURL);
		if(bundle != NULL)
		{
			Url url;
			CFURLRef bundleURL = CFBundleCopyExecutableURL (bundle);
			if(!bundleURL)
			{
				CCL_WARN ("Module at %s could not be loaded: executable not found.\n ", MutableCString (path.getPath ()).str ());
				return kResultFailed;
			}
			MacUtils::urlFromCFURL (url, bundleURL, Url::kFile);
			CFRelease (bundleURL);
			url.ascend ();
			System::GetFileSystem().setWorkingDirectory (url);
			
			CFErrorRef error = NULL;
			if(CFBundleLoadExecutableAndReturnError (bundle, &error) == false)
			{
				CFStringRef desc = CFErrorCopyDescription (error);
				CFStringRef reason = CFErrorCopyFailureReason (error);
				
				String descString;
				descString.appendNativeString (desc);

				String reasonString;
				reasonString.appendNativeString (reason);
				
				String name;
				path.getName (name, false);
				
				CCL_WARN ("Module %s could not be loaded: %s -> %s\n",
					MutableCString (name).str (),
					MutableCString (descString).str (),
					MutableCString (reasonString).str ())
				
				if(desc) CFRelease (desc);
				if(reason) CFRelease (reason);
				if(error) CFRelease (error);
				return kResultFailed;
			}
			 
			image = NEW BundleImage (bundle, true);
			
			return kResultOk;
		}
		else
		{
			// try .dylib.....
			POSIXPath posixPath;
			path.toPOSIXPath (posixPath, posixPath.size ());
			void* handle = ::dlopen (posixPath.path, RTLD_LOCAL);
			if(handle)
			{
				DylibImage* dylib = NEW DylibImage (handle, true);
				dylib->setPath (path);
				image = dylib;
				return kResultOk;
			}
			else
				CCL_WARN ("Module could not be loaded: %s\n", ::dlerror ());
		}
	}
	// TODO error reporting
	image = nullptr;
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IExecutableImage* CCL_API OSXExecutableLoader::createImage (ModuleRef module)
{
	return NEW BundleImage (module, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API OSXExecutableLoader::execute (Threading::ProcessID& processId, UrlRef path, ArgsRef args,
											  int flags, IUnknown* context)
{
	@try
	{
		String pathString;
		path.toDisplayString (pathString);
		NSString* nsPathString = [pathString.createNativeString<NSString*>() autorelease];
		NSBundle* bundle = [NSBundle bundleWithPath:nsPathString];
		if(bundle)
			if(NSString* execPath = [bundle executablePath])
				nsPathString = execPath;
			
		NSArray* nsArgs = nil;
		if(args.count ())
		{
			nsArgs = [[NSArray alloc] init];
			
			for(int i = 0; i < args.count (); i++)
			{
				CCL_PRINTLN (args.at(i))
				
				NSString* nsString = args.at(i).createNativeString<NSString*> ();
				nsArgs = [nsArgs arrayByAddingObject: nsString];
			}
		}
		
		NSTask* executable = [[NSTask alloc] init];
		[executable setLaunchPath: nsPathString];
		if(nsArgs)
			[executable setArguments: nsArgs];
		
		NSPipe* output = nil;
		NSPipe* error = nil;
		if(flags & System::kRedirectProcessOutput)
		{
			[executable setStandardOutput: output = [NSPipe pipe]];
			[executable setStandardError: error = [NSPipe pipe]];
		}
		
		[executable launch];
		processId = [executable processIdentifier];
		
		tresult result = kResultOk;
		if(flags & System::kWaitForProcessExit)
		{
			[executable waitUntilExit];
			result = static_cast<tresult> ([executable terminationStatus]);
		}

		if(flags & System::kRedirectProcessOutput)
		{
			NSData* outputData = [[output fileHandleForReading] readDataToEndOfFile];
			NSData* errorData = [[error fileHandleForReading] readDataToEndOfFile];

			UnknownPtr<IStream> fileStream (context);
			if([outputData length] > 0)
				fileStream->write ([outputData bytes], (int)[outputData length]);
			if([errorData length] > 0)
				fileStream->write ([errorData bytes], (int)[errorData length]);
		}
		
		if(executable != nil)
			return result;
	}
	@catch (NSException* e)
	{
		CCL_WARN ("Unexpected exception in ExecutableLoader::execute", 0)
	}
	
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API OSXExecutableLoader::relaunch (ArgsRef args)
{
	NSURL *url = [NSURL fileURLWithPath:[[NSBundle mainBundle] bundlePath]];

	LSLaunchURLSpec launchSpec;
	launchSpec.appURL = (CFURLRef)url;
	launchSpec.itemURLs = nullptr;
	launchSpec.passThruParams = nullptr;
	launchSpec.launchFlags = kLSLaunchDefaults|kLSLaunchNewInstance;
	launchSpec.asyncRefCon = nullptr;

	LSOpenFromURLSpec (&launchSpec, nullptr);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API OSXExecutableLoader::terminate (Threading::ProcessID processId)
{
	int result = kill ((int)processId, SIGKILL);
	return (result == 0) ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API OSXExecutableLoader::getExecutablePath (IUrl& path, Threading::ProcessID processId)
{
	tresult result = kResultFailed;
	int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
	size_t buffer_size;
	sysctl (mib, 4, nullptr, &buffer_size, nullptr, 0);
	
	struct kinfo_proc* procs = (kinfo_proc*)core_malloc ((unsigned int)buffer_size);
	sysctl (mib, 4, procs, &buffer_size, nullptr, 0);
	
	int num_procs = (int)(buffer_size / sizeof(struct kinfo_proc));
	for(int i = 0; i < num_procs; i++)
	{
		struct kinfo_proc* pproc = procs + i;
		if(pproc->kp_proc.p_pid == processId)
		{
			int mib[3] = { CTL_KERN, KERN_PROCARGS, pproc->kp_proc.p_pid };
			size_t argv_len;
			sysctl (mib, 3, nullptr, &argv_len, nullptr, 0);
			char* proc_argv = (char*)core_malloc ((unsigned int)(sizeof(char) * argv_len));
			sysctl (mib, 3, proc_argv, &argv_len, nullptr, 0);
			path.setPath (proc_argv, IUrl::kFile);
			core_free (proc_argv);
			result = kResultOk;
			break;
		}
	}
	
	core_free (procs);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API OSXExecutableLoader::isProcessRunning (UrlRef executableFile)
{
	String appFileName;
	executableFile.getName (appFileName, false);
	
	NSArray* applications = [[NSWorkspace sharedWorkspace] runningApplications];
	for(int i = 0; i < [applications count]; i++)
	{
		NSRunningApplication* app = [applications objectAtIndex:i];
		String compare;
		compare.appendNativeString (app.localizedName);
		if(compare == appFileName)
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API OSXExecutableLoader::getModuleInfo (IAttributeList& attributes, UrlRef path)
{
	tresult result = kResultFailed;

	String pathString;
	path.toDisplayString (pathString);
	CFObj<CFStringRef> nativePathString = pathString.createNativeString<CFStringRef> ();
	CFObj<CFURLRef> bundleURL = CFURLCreateWithFileSystemPath (NULL, nativePathString, kCFURLPOSIXPathStyle, false);
	if(bundleURL != NULL)
	{
		CFBundleRef bundle = CFBundleCreate (0, bundleURL);
		AutoPtr<IExecutableImage> image = createImage (bundle);
		if(image && image->getMetaInfo () != 0)
		{
			attributes.copyFrom (*image->getMetaInfo ());
			result = kResultOk;
		}
	}
	return result;
}

//************************************************************************************************
// ApplicationSearcher
//************************************************************************************************

CCL::ApplicationSearcher::ApplicationSearcher ()
: query (nil),
  spotlightObserver (nil)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::ApplicationSearcher::~ApplicationSearcher ()
{
	cleanUp ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL::ApplicationSearcher::cleanUp ()
{
	if(spotlightObserver)
	{
		[[NSNotificationCenter defaultCenter] removeObserver:spotlightObserver];
		spotlightObserver = nil;
	}
	
	if(query)
	{
		[query autorelease];
		query = nil;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL::ApplicationSearcher::find (StringRef filter)
{
	ASSERT(spotlightObserver == nil)
	if(spotlightObserver)
		return;
	
	setResult (static_cast<IUnknownList*> (&resultList));
	resultList.removeAll ();
	
	NSMutableString* search = [NSMutableString stringWithString:@"kMDItemContentType=\"com.apple.application-bundle\""];
	if(filter.isEmpty () == false)
	{
		[search appendString:@"&&("];
		bool first = true;
		ForEachStringToken (filter, CCLSTR (",;"), token)
			if(!token.endsWith (CCLSTR (".app")))
				token.append (CCLSTR (".app"));
		
			if(first)
				first = false;
			else
				[search appendString:@"||"];
			[search appendString:@"kMDItemFSName LIKE[c] \""];
			[search appendString:[token.createNativeString<NSString*> () autorelease]];
			[search appendString:@"\""];
		EndFor
		[search appendString:@")"];
	}
	
	NSPredicate* searchPredicate = nil;
	@try
	{
		searchPredicate = [NSPredicate predicateWithFormat:search];
	}
	@catch (NSException *exception)
	{
		setState (kFailed);
		return;
	}
	
	query = [[NSMetadataQuery alloc] init];
	[query setPredicate:searchPredicate];
	NSArray *searchScopes = @[ NSMetadataQueryLocalComputerScope ];
	[query setSearchScopes:searchScopes];
	spotlightObserver = [[NSNotificationCenter defaultCenter] addObserverForName:NSMetadataQueryDidFinishGatheringNotification object:query queue:[NSOperationQueue mainQueue] usingBlock:^(NSNotification* notification)
	{
		if(query)
		{
			[query stopQuery];
			for(int i = 0; i < [query resultCount]; i++)
			{
				NSMetadataItem* item = [query resultAtIndex:i];
				String fileName;
				fileName.appendNativeString ((NSString*)[item valueForAttribute:(NSString*)kMDItemFSName]);
				NSString* nsPath = (NSString*)[item valueForAttribute:(NSString*)kMDItemPath];
				String executablePath;
				executablePath.appendNativeString (nsPath);
				String version;
				CFURLRef cfUrl = (CFURLRef)[NSURL fileURLWithPath:nsPath];
				NSDictionary* nsDictionary = [(NSDictionary*)CFBundleCopyInfoDictionaryInDirectory (cfUrl) autorelease];
				version.appendNativeString ([nsDictionary objectForKey:@"CFBundleShortVersionString"]);
				
				AutoPtr<Attributes> attr = NEW Attributes;
				attr->set (Meta::kPackageExecutable, executablePath);
				attr->set (Meta::kPackageName, fileName);
				if(!version.isEmpty ())
					attr->set (Meta::kPackageVersion, version);
				resultList.add (attr->asUnknown (), true);
			}
			setState (kCompleted);
		}
		this->release ();
	}];
	[query startQuery];
	setState (kStarted);
	this->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static kern_return_t findService (io_iterator_t *matchingServices, const char* bsdName)
{
	mach_port_t masterPort;
	kern_return_t kernResult = IOMasterPort (MACH_PORT_NULL, &masterPort);
	if(kernResult != KERN_SUCCESS)
		return kernResult;

	CFMutableDictionaryRef matchingDict = IOBSDNameMatching (masterPort, 0, bsdName);
	return IOServiceGetMatchingServices (masterPort, matchingDict, matchingServices);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static kern_return_t getNetworkAddressAndName (io_iterator_t iterator, UInt8* macAddress, String& name)
{
	kern_return_t kernResult = KERN_FAILURE;
	bzero (macAddress, kIOEthernetAddressSize);
	
	while(io_object_t intfService = IOIteratorNext (iterator))
	{
		CFTypeRef  macAddressAsCFData;
		CFStringRef vendorString = NULL;
		CFStringRef modelString = NULL;
	
		// IONetworkControllers can't be found directly by the IOServiceGetMatchingServices call,
		// since they are hardware nubs and do not participate in driver matching. In other words,
		// registerService () is never called on them. So we've found the IONetworkInterface and will
		// get its parent controller by asking for it specifically.
		
		io_object_t controllerService;
		kernResult = IORegistryEntryGetParentEntry (intfService, kIOServicePlane, &controllerService);
		if(kernResult == KERN_SUCCESS)
		{
			macAddressAsCFData = IORegistryEntryCreateCFProperty (controllerService,CFSTR (kIOMACAddress), kCFAllocatorDefault, 0);
			if(macAddressAsCFData)
			{
				UInt8 thisMacAddress[kIOEthernetAddressSize];
				bzero (thisMacAddress, kIOEthernetAddressSize);
				CFDataGetBytes ((CFDataRef)macAddressAsCFData, CFRangeMake (0, kIOEthernetAddressSize), thisMacAddress);
				CFRelease (macAddressAsCFData);
				
				if(memcmp (thisMacAddress, macAddress, kIOEthernetAddressSize) != 0)
					memcpy (macAddress, thisMacAddress, kIOEthernetAddressSize);
			}
			vendorString = (CFStringRef)IORegistryEntryCreateCFProperty (controllerService, CFSTR (kIOVendor), kCFAllocatorDefault, 0);
			if(vendorString)
			{
				name.appendNativeString (vendorString);
				CFRelease (vendorString);
			}
			
			modelString = (CFStringRef)IORegistryEntryCreateCFProperty (controllerService, CFSTR (kIOModel), kCFAllocatorDefault, 0);
			if(modelString)
			{
				name.append (" ");
				name.appendNativeString (modelString);
				CFRelease (modelString);
			}
			IOObjectRelease (controllerService);
		}
		IOObjectRelease (intfService);
	}
	
	return kernResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static kern_return_t getDiskName (io_iterator_t iterator, String& name)
{
	while(io_object_t mediaService = IOIteratorNext (iterator))
	{
		io_object_t driverService;
		if(IORegistryEntryGetParentEntry (mediaService, kIOServicePlane, &driverService) != KERN_SUCCESS)
			continue;
		io_object_t deviceService;
		if(IORegistryEntryGetParentEntry (driverService, kIOServicePlane, &deviceService) != KERN_SUCCESS)
			continue;

		CFMutableDictionaryRef deviceProperties = NULL;
		IORegistryEntryCreateCFProperties (deviceService, &deviceProperties, kCFAllocatorDefault, 0);
		if(deviceProperties)
		{
			CFDictionaryRef deviceCharacteristics = NULL;
			if(CFDictionaryGetValueIfPresent (deviceProperties, (void*)CFSTR (kIOPropertyDeviceCharacteristicsKey), (const void**)&deviceCharacteristics))
			{
				CFStringRef productString = NULL;
				if(CFDictionaryGetValueIfPresent (deviceCharacteristics, (void*)CFSTR (kIOPropertyProductNameKey), (const void**)&productString))
					if(productString)
						name.appendNativeString (productString);
			}
			CFRelease (deviceProperties);
			if(!name.isEmpty ())
				return KERN_SUCCESS;
		}
	}
	
	return KERN_FAILURE;
}

