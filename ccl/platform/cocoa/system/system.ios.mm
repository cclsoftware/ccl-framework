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
// Filename    : ccl/platform/cocoa/system/system.ios.mm
// Description : iOS System Information
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/cocoa/system/system.shared.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/platform/cocoa/macutils.h"

#include "ccl/public/systemservices.h"

#include <mach/mach.h>
#include <os/proc.h>

namespace CCL {

//************************************************************************************************
// IOSSystemInformation
//************************************************************************************************

class IOSSystemInformation: public CocoaSystemInformation
{
public:
	// CocoaSystemInformation
	void CCL_API getComputerName (String& name, int flags = 0) const;
	void CCL_API getUserName (String& name, int flags = 0) const;
	void CCL_API getMemoryInfo (System::MemoryInfo& memoryInfo) const;
	void CCL_API getComputerInfo (IAttributeList& attributes, int flags = 0) const;	
	tbool CCL_API isProcessSandboxed () const;

protected:
	// CocoaSystemInformation
	bool getNativeLocation (IUrl& url, System::FolderType type) const;
};

//************************************************************************************************
// IOSExecutableLoader
//************************************************************************************************

class IOSExecutableLoader: public ExecutableLoader
{
public:
	// ExecutableLoader
	tresult CCL_API loadImage (IExecutableImage*& image, UrlRef path) override;
	IExecutableImage* CCL_API createImage (ModuleRef module) override;
	tresult CCL_API getExecutablePath (IUrl& path, Threading::ProcessID processId) override;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// IOSSystemInformation
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (SystemInformation, IOSSystemInformation)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IOSSystemInformation::getNativeLocation (IUrl& path, System::FolderType type) const
{
	bool result = false;
	NSString* nsFileName = nil;
	NSURL* nsUrl = nil;
	NSSearchPathDirectory nsDirId = (NSSearchPathDirectory)0;

	switch(type)
	{
	case System::kSystemFolder :
		nsDirId = NSLibraryDirectory;
		break;

	case System::kProgramsFolder :
		nsDirId = NSApplicationDirectory;
		break;

	case System::kTempFolder :
		nsFileName = NSTemporaryDirectory ();
		break;

	case System::kAppSupportFolder :
		nsUrl = [[NSBundle mainBundle] bundleURL];
		break;

	case System::kUserDocumentFolder :
		{
			NSArray *paths = NSSearchPathForDirectoriesInDomains (NSDocumentDirectory, NSUserDomainMask, YES);
			nsFileName = ([paths count] > 0) ? [paths objectAtIndex:0] : nil;
		}
		break;

	case System::kUserDownloadsFolder :
		nsDirId = NSDownloadsDirectory;
		break;

	case System::kUserMusicFolder :
		nsDirId = NSMusicDirectory;
		break;

	case System::kUserSettingsFolder :
	case System::kSharedSettingsFolder :
	case System::kSharedDataFolder :
		nsDirId = NSApplicationSupportDirectory;
		break;

	case System::kDesktopFolder :
		nsDirId = NSDesktopDirectory;
		break;

	case System::kUserPreferencesFolder :
		nsDirId = NSDocumentDirectory;
		break;

	case System::kAppPluginsFolder :
		nsUrl = [[[NSBundle mainBundle] bundleURL] URLByAppendingPathComponent:@"PlugIns" isDirectory:YES];
		break;

	case System::kAppFactoryContentFolder :
		{
			NSArray* dirs = NSSearchPathForDirectoriesInDomains (NSApplicationSupportDirectory, NSUserDomainMask, YES);
			nsFileName = [dirs objectAtIndex: 0];
			if(nsFileName != nil)
				nsUrl = [NSURL fileURLWithPath: nsFileName];
			if(nsUrl != nil)
			{
				nsUrl = [nsUrl URLByAppendingPathComponent:@"Content" isDirectory:YES];
				[nsUrl setResourceValue:[NSNumber numberWithBool:YES] forKey:NSURLIsExcludedFromBackupKey error:nil];
				result = MacUtils::urlFromNSUrl (path, nsUrl, IUrl::kFolder);
			}
		}
		break;
	}

	if(!result)
	{
		if(nsDirId != 0)
		{
			NSArray* dirs = NSSearchPathForDirectoriesInDomains (nsDirId, NSUserDomainMask, YES);
			nsFileName = [dirs objectAtIndex: 0];
		}
		
		if(nsFileName != nil)
			nsUrl = [NSURL fileURLWithPath: nsFileName];
		
		if(nsUrl != nil)
			result = MacUtils::urlFromNSUrl (path, nsUrl, IUrl::kFolder);
	}

	ASSERT (result)
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API IOSSystemInformation::getComputerName (String& name, int flags) const
{
	NSString *devName = [[UIDevice currentDevice] name];

	name.empty ();
	name.appendNativeString (devName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API IOSSystemInformation::getUserName (String& name, int flags) const
{
	// iOS doesn't really have the concept of a user name... you have a device name, that's about it.
	getComputerName (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API IOSSystemInformation::getComputerInfo (IAttributeList& attributes, int flags) const
{
	String model;
	NSString* devModel = [[UIDevice currentDevice] model];
	model.appendNativeString (devModel);
	attributes.setAttribute (System::kDeviceModel, model);

	String identifier;
	NSString* uuid = [[[UIDevice currentDevice] identifierForVendor] UUIDString];
	identifier.appendNativeString (uuid);
	CCL_PRINTF ("SystemInformation::getComputerInfo() uniqueID: %s\n", MutableCString (identifier).str ());
	attributes.setAttribute (System::kDeviceIdentifier, identifier);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API IOSSystemInformation::getMemoryInfo (System::MemoryInfo& memoryInfo) const
{
	memoryInfo.physicalRAMSize = [[NSProcessInfo processInfo] physicalMemory];

	task_vm_info t_info;
	mach_msg_type_number_t t_info_count = TASK_VM_INFO_REV1_COUNT;
	task_info (mach_task_self (), TASK_VM_INFO, (task_info_t)&t_info, &t_info_count);
	memoryInfo.processMemoryTotal = t_info.virtual_size;

	#if TARGET_OS_SIMULATOR
	memoryInfo.processMemoryAvailable = memoryInfo.processMemoryTotal - t_info.resident_size;
	#else
	memoryInfo.processMemoryAvailable = os_proc_available_memory ();
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API IOSSystemInformation::isProcessSandboxed () const
{
	return true;
}

//************************************************************************************************
// IOSExecutableLoader
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (ExecutableLoader, IOSExecutableLoader)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API IOSExecutableLoader::loadImage (IExecutableImage*& image, UrlRef path)
{
	CFObj<CFURLRef> bundleURL;
	
	String pathString;
	path.toDisplayString (pathString);

	if(pathString.endsWith (".plugin"))
	{
		// get the module identifier
		// e.g. PlugIns/plugname.plugin => plugname
		String moduleId;
		path.getName (moduleId, false);

		// for plug-ins, get the corresponding framework bundle
		// e.g. Plugins/plugname.plugin => Frameworks/plugname.framework
		pathString = moduleId.append (".framework");
		NSObj<NSString> nativePathString = pathString.createNativeString<NSString*> ();
		NSURL* url = [[[[[NSBundle mainBundle] bundleURL] URLByAppendingPathComponent:@"Frameworks" isDirectory:YES] URLByAppendingPathComponent:nativePathString isDirectory:NO] retain];
		bundleURL = (CFURLRef)url;;
	}
	else
	{
		CFObj<CFStringRef> nativePathString = pathString.createNativeString<CFStringRef> ();
		bundleURL = CFURLCreateWithFileSystemPath (0, nativePathString, kCFURLPOSIXPathStyle, false);
	}
	
	if(bundleURL != nil)
	{
		CFBundleRef bundle = CFBundleCreate (0, bundleURL);
		if(bundle != nil)
		{
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
	}
	image = nil;
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IExecutableImage* CCL_API IOSExecutableLoader::createImage (ModuleRef module)
{
	return NEW BundleImage (module, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API IOSExecutableLoader::getExecutablePath (IUrl& path, Threading::ProcessID processId)
{
	// iOS sandboxing will not allow us to collect information of other executables

	if(processId != System::GetProcessSelfID ())
		return kResultNotImplemented;

	if(!getMainImage ().getPath (path))
		return kResultFailed;

	return kResultOk;
}
