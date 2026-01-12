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
// Filename    : ccl/platform/android/system/system.android.cpp
// Description : Android system class
//
//************************************************************************************************

#include "ccl/platform/android/system/system.android.h"
#include "ccl/platform/android/system/assetfilesystem.h"

#include "ccl/platform/android/interfaces/iframeworkactivity.h"
#include "ccl/platform/android/interfaces/jni/androidcontent.h"

#include "ccl/platform/android/androidmain.h"

#include "ccl/platform/shared/posix/system/system.posix.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/attributes.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/system/formatter.h"
#include "ccl/public/system/ipackagemetainfo.h"
#include "ccl/public/systemservices.h"

#include "core/public/coreversion.h"

#include <android/asset_manager_jni.h>
#include <android/configuration.h>
#include <android/log.h>

#include <sys/mman.h>
#include <dlfcn.h>

namespace CCL {
namespace Android {

//************************************************************************************************
// android.os.Debug
//************************************************************************************************

DECLARE_JNI_CLASS (AndroidDebug, "android/os/Debug")
	DECLARE_JNI_STATIC_METHOD (int64, getNativeHeapAllocatedSize)
	DECLARE_JNI_STATIC_METHOD (int64, getNativeHeapFreeSize)
	DECLARE_JNI_STATIC_METHOD (int64, getNativeHeapSize)
END_DECLARE_JNI_CLASS (AndroidDebug)

DEFINE_JNI_CLASS (AndroidDebug)
	DEFINE_JNI_STATIC_METHOD (getNativeHeapAllocatedSize, "()J")
	DEFINE_JNI_STATIC_METHOD (getNativeHeapFreeSize, "()J")
	DEFINE_JNI_STATIC_METHOD (getNativeHeapSize, "()J")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// android.app.ActivityManager
//************************************************************************************************

DECLARE_JNI_CLASS (ActivityManager, "android/app/ActivityManager")
	DECLARE_JNI_METHOD (void, getMemoryInfo, jobject)
	
	DECLARE_STRINGID_MEMBER (kServiceID)
END_DECLARE_JNI_CLASS (ActivityManager)

DEFINE_JNI_CLASS (ActivityManager)
	DEFINE_JNI_METHOD (getMemoryInfo, "(Landroid/app/ActivityManager$MemoryInfo;)V")
END_DEFINE_JNI_CLASS

DEFINE_STRINGID_MEMBER_ (ActivityManagerClass, kServiceID, "ACTIVITY_SERVICE")

//************************************************************************************************
// android.app.ActivityManager.MemoryInfo
//************************************************************************************************

DECLARE_JNI_CLASS (MemoryInfo, "android/app/ActivityManager$MemoryInfo")
	DECLARE_JNI_FIELD (int64, availMem)
	DECLARE_JNI_FIELD (int64, threshold)
	DECLARE_JNI_FIELD (int64, totalMem)
	DECLARE_JNI_FIELD (bool, lowMemory)
END_DECLARE_JNI_CLASS (MemoryInfo)

DEFINE_JNI_CLASS (MemoryInfo)
	DEFINE_JNI_DEFAULT_CONSTRUCTOR
	DEFINE_JNI_FIELD (availMem, "J")
	DEFINE_JNI_FIELD (threshold, "J")
	DEFINE_JNI_FIELD (totalMem, "J")
	DEFINE_JNI_FIELD (lowMemory, "Z")
END_DEFINE_JNI_CLASS

} // namespace Android
} // namespace CCL

namespace CCL {

ModuleRef gMainModuleRef = nullptr;

} // namespace CCL

using namespace Core;
using namespace CCL;
using namespace CCL::Android;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Services API
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ModuleRef CCL_API System::CCL_ISOLATED (GetMainModuleRef) ()
{
	return gMainModuleRef;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT tresult CCL_API System::CCL_ISOLATED (CreateUID) (UIDBytes& uid)
{
	JniAccessor jni;
	LocalRef javaUID (jni, Java::UUID.randomUUID ());
	if(javaUID)
	{
		LocalStringRef localString (jni, Java::UUID.toString (javaUID));
		JniCStringChars string (jni, localString);
		
		int v[11] = {0};
		int result = ::sscanf (string, "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", 
							   &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &v[8], &v[9], &v[10]);
		if(result == 11)
		{
			uid.data1 = (unsigned int)v[0];
			uid.data2 = (unsigned short)v[1];
			uid.data3 = (unsigned short)v[2];
			for(int i = 0; i < 8; i++)
				uid.data4[i] = (unsigned char)v[3 + i];
		}
		return kResultOk;
	}
	ASSERT (0)
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void CCL_API System::CCL_ISOLATED (LockMemory) (tbool state, void* address, int size)
{
	if(state)
		mlock (address, size);
	else
		munlock (address, size);
}

//************************************************************************************************
// AndroidSystemInformation
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (SystemInformation, AndroidSystemInformation)

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidSystemInformation& AndroidSystemInformation::getInstance ()
{
	return static_cast<AndroidSystemInformation&> (SystemInformation::instance ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidSystemInformation::AndroidSystemInformation ()
: activity (nullptr),
  assetManager (nullptr),
  configuration (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidSystemInformation::~AndroidSystemInformation ()
{
	if(configuration)
		AConfiguration_delete (configuration);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidSystemInformation::setNativeActivity (IFrameworkActivity* activity)
{
	CCL_PRINTF ("AndroidSystemInformation::setNativeActivity %x", activity)

	this->activity = activity;

	if(activity)
	{
		// get asset manager
		JniAccessor jni;
		javaAssetManager.assign (jni, activity->getAssetManager ());
		assetManager = AAssetManager_fromJava (jni, javaAssetManager); // use our global reference here!

		// update configuration
		if(!configuration)
			configuration = AConfiguration_new ();

		AConfiguration_fromAssetManager (configuration, assetManager);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFrameworkActivity* CCL_API AndroidSystemInformation::getNativeActivity () const
{
	return this->activity;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API AndroidSystemInformation::callAndroidMain (tbool startup)
{
	// find CCLAndroidMain function, should be exported by main module (androidmain.cpp)
	CCLAndroidMain mainFunc = (CCLAndroidMain)::dlsym (RTLD_DEFAULT, "CCLAndroidMain");
	CCL_PRINTF ("callAndroidMain %x", mainFunc)
	ASSERT (mainFunc)
	if(!mainFunc)
		return kExitError;

	if(!gMainModuleRef)
	{
		gMainModuleRef = AndroidExecutableLoader::getMainModuleRef ();
		if(!gMainModuleRef)
		{
			__android_log_write (ANDROID_LOG_FATAL, "CCL Native", "Could not determine main module handle");
			exit (1);
		}
	}

	return mainFunc (gMainModuleRef, startup);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidSystemInformation::onConfigurationChanged ()
{
	// update saved configuration
	AConfiguration_fromAssetManager (configuration, assetManager);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

jobject AndroidSystemInformation::getSystemService (StringID serviceID) const
{
	if(!activity)
		return nullptr;

	JniAccessor jni;

	jstring serviceIDString = jobject_cast<jstring> (jni.getStaticField (Context, serviceID, "Ljava/lang/String;"));
	return Context.getSystemService (activity->getJObject (), serviceIDString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef AndroidSystemInformation::getAppProductFolderName () const
{
	return appProductName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AndroidSystemInformation::getNativeLocation (IUrl& path, System::FolderType type) const
{
	JniAccessor jni;
	LocalRef file;

	auto getPrivateDir = [&] (CStringPtr subFolder) -> jobject
	{
		if(!activity)
			return nullptr;

		JniString name (jni, subFolder);
		return Context.getDir (activity->getJObject (), name, Context.MODE_PRIVATE);
	};

	auto getExternalFilesDir = [&] (CStringPtr type) -> jobject
	{
		if(!activity)
			return nullptr;

		JniString t (jni, type);
		return Context.getExternalFilesDir (activity->getJObject (), t);
	};

	switch(type)
	{
	case System::kSystemFolder :
	case System::kProgramsFolder :
		break;
		
	case System::kTempFolder :
		if(activity)
			file.assign (jni, Context.getCacheDir (activity->getJObject ())); // also: getExternalCacheDir
		break;
		
	case System::kAppSupportFolder :
		#if 1
		path.assign (AssetUrl (String::kEmpty, Url::kFolder)); // "assets" folder in app package
		return true;
		#else
		return APKImage (gMainModuleRef, false).getPath (path) != 0; // PackageResourcePath
		#endif
		break;
		
	case System::kUserDocumentFolder :
		file.assign (jni, getExternalFilesDir (::Android::Context.DIRECTORY_DOCUMENTS));
		break;
		
	case System::kUserDownloadsFolder :
		file.assign (jni, getExternalFilesDir (::Android::Context.DIRECTORY_DOWNLOADS));
		//Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS), needs android.permission.WRITE_EXTERNAL_STORAGE
		break;
		
	case System::kUserMusicFolder :
		file.assign (jni, getExternalFilesDir (::Android::Context.DIRECTORY_MUSIC));
		break;
		
	case System::kDesktopFolder :
		break;
	
	case System::kUserSettingsFolder :
	case System::kUserPreferencesFolder :
	case System::kSharedSettingsFolder :
		file.assign (jni, getPrivateDir ("settings"));
		break;

	case System::kAppPluginsFolder :
		path.assign (AssetUrl (CCLSTR ("Plugins"), Url::kFolder));
		return true;

	case System::kAppFactoryContentFolder :
		if(activity)
			file.assign (jni, Context.getNoBackupFilesDir (activity->getJObject ()));
		break;
	}

    if(file != 0)
	{ 
		LocalStringRef localString (jni, Java::File.getAbsolutePath (file));
		JniCStringChars pathString (jni, localString);
		if(pathString)
		{
			path.fromPOSIXPath (pathString, IUrl::kFolder);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidSystemInformation::getLocalTime (DateTime& dateTime) const
{
	PosixTimeConversion::getLocalTime (dateTime);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidSystemInformation::convertLocalTimeToUTC (DateTime& utc, const DateTime& localTime) const
{
	PosixTimeConversion::convertLocalTimeToUTC (utc, localTime);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidSystemInformation::convertUTCToLocalTime (DateTime& localTime, const DateTime& utc) const
{
	PosixTimeConversion::convertUTCToLocalTime (localTime, utc);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidSystemInformation::convertUnixTimeToUTC (DateTime& utc, int64 unixTime) const
{
	PosixTimeConversion::convertUnixTimeToUTC (utc, unixTime);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API AndroidSystemInformation::convertUTCToUnixTime (const DateTime& utc) const
{
	return PosixTimeConversion::convertUTCToUnixTime (utc);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidSystemInformation::getComputerName (String& name, int flags) const
{
	if(activity)
		activity->getComputerName (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidSystemInformation::getUserName (String& name, int flags) const
{
	if(activity)
		activity->getUserName (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 AndroidSystemInformation::getPhysicalMemoryAvailable ()
{
	JniAccessor jni;

	JniObject info;
	info.newObject (jni, MemoryInfo);

	JniObject activityManager (jni, AndroidSystemInformation::getInstance ().getSystemService (ActivityManager.kServiceID));
	if(activityManager)
	{
		ActivityManager.getMemoryInfo (activityManager, info);
		return jni.getField (info, MemoryInfo.availMem);
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidSystemInformation::getMemoryInfo (System::MemoryInfo& memoryInfo) const
{
	JniAccessor jni;

	JniObject info;
	info.newObject (jni, MemoryInfo);

	JniObject activityManager (jni, getSystemService (ActivityManager.kServiceID));
	if(activityManager)
		ActivityManager.getMemoryInfo (activityManager, info);

	memoryInfo.physicalRAMSize = jni.getField (info, MemoryInfo.totalMem);
	memoryInfo.processMemoryTotal = jni.getField (info, MemoryInfo.totalMem);
	memoryInfo.processMemoryAvailable = jni.getField (info, MemoryInfo.availMem);

	#if DEBUG_LOG
	if(LocalString::hasTable ()) // might be too early (LocaleInfoBase::printByteSize uses translated strings)
	{
		// physical memory:
		CCL_PRINTF ("ActivityManager.MemoryInfo: totalMem %s, availMem %s, threshold %s, lowMemory: %d\n",
			MutableCString (Format::ByteSize::print (jni.getField (info, MemoryInfo.totalMem))).str (),
			MutableCString (Format::ByteSize::print (jni.getField (info, MemoryInfo.availMem))).str (),
			MutableCString (Format::ByteSize::print (jni.getField (info, MemoryInfo.threshold))).str (),
			jni.getField (info, MemoryInfo.lowMemory))
	
		// native heap
		CCL_PRINTF ("android.os.Debug: nativeHeapAllocatedSize %s, nativeHeapSize %s, nativeHeapFreeSize %s\n",
			MutableCString (Format::ByteSize::print (AndroidDebug.getNativeHeapAllocatedSize ())).str (),
			MutableCString (Format::ByteSize::print (AndroidDebug.getNativeHeapSize ())).str (),
			MutableCString (Format::ByteSize::print (AndroidDebug.getNativeHeapFreeSize ())).str ())

		// java virtual machine memory:
		JniObject runtime (jni, Java::Runtime.getRuntime ());

		CCL_PRINTF ("VM: maxMemory %s, totalMemory %s, freeMemory %s\n",
			MutableCString (Format::ByteSize::print (Java::Runtime.maxMemory (runtime))).str (),	// current total + unalllocated memory
			MutableCString (Format::ByteSize::print (Java::Runtime.totalMemory (runtime))).str (),	// currently used + free, can grow up to maxMemory
			MutableCString (Format::ByteSize::print (Java::Runtime.freeMemory (runtime))).str ())
	}
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidSystemInformation::getComputerInfo (IAttributeList& attributes, int flags) const
{
	attributes.setAttribute (System::kDeviceModel, CCLSTR ("Android"));
	
	if(activity)
	{
		String deviceID;
		activity->getDeviceID (deviceID);
		attributes.setAttribute (System::kDeviceIdentifier, deviceID);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AndroidSystemInformation::isProcessSandboxed () const
{
	return true;
}
	
//************************************************************************************************
// AndroidExecutableLoader
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (ExecutableLoader, AndroidExecutableLoader)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidExecutableLoader::loadImage (IExecutableImage*& image, UrlRef path)
{
	String moduleId;
	String module = path.getPath ();

	if(module.endsWith (".plugin"))
	{
		// get the module identifier
		// e.g. Plugins/plugname.plugin => plugname
		path.getName (moduleId, false);

		// for plug-ins, get the corresponding shared library
		// e.g. Plugins/plugname.plugin => libplugname.so
		module = String ("lib").append (moduleId).append (".so");
	}

	ASSERT (module.endsWith (".so"))

	// load plug-ins through Java to trigger JNI_OnLoad call
	if(!moduleId.isEmpty ())
		Java::System.loadLibrary (JniCCLString (moduleId));

	// now get the native module handle
	void* handle = ::dlopen (MutableCString (module, Text::kUTF8).str (), 0);
	if(handle)
	{
		image = NEW DynamicLibraryImage (moduleId, handle, true);
		return kResultOk;
	}
	else
		CCL_WARN ("Module could not be loaded: %s\n", ::dlerror ());

	#if DEBUG
	Debugger::printf ("ExecutableLoader::loadImage FAILED: %s\n", MutableCString (UrlFullString (path)).str ());
	#endif
	image = 0;
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IExecutableImage* CCL_API AndroidExecutableLoader::createImage (ModuleRef module)
{
	if(module == gMainModuleRef)
		return NEW MainModuleImage (module, false);

	// get module ID of existing image
	String moduleId;
	AutoPtr<IExecutableIterator> imageIterator = createIterator ();
	while(const IExecutableImage* image = imageIterator->getNextImage ())
	{
		if(image->getNativeReference () == module)
		{
			image->getIdentifier (moduleId);
			break;
		}
	}

	return NEW DynamicLibraryImage (moduleId, module, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidExecutableLoader::relaunch (ArgsRef args)
{
	if(IFrameworkActivity* activity = AndroidSystemInformation::getInstance ().getNativeActivity ())
		activity->relaunchActivity ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidExecutableLoader::getExecutablePath (IUrl& path, Threading::ProcessID processId)
{
	if(processId != System::GetProcessSelfID ())
		return kResultNotImplemented;

	// return the path to the app's main module here
	if(IFrameworkActivity* activity = AndroidSystemInformation::getInstance ().getNativeActivity ())
	{
		String libraryDir;
		String moduleID;
		activity->getNativeLibraryDir (libraryDir);
		activity->getMainModuleID (moduleID);

		String fileName = libraryDir.append ("/lib").append (moduleID).append (".so");
		path.fromPOSIXPath (MutableCString (fileName), IUrl::kFile);
		return kResultOk;
	}
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ModuleRef AndroidExecutableLoader::getMainModuleRef ()
{
	ModuleRef module = 0;
		
	// query main module ID from Java activity to build the corresponding .so name
	if(IFrameworkActivity* activity = AndroidSystemInformation::getInstance ().getNativeActivity ())
	{
		String moduleID;
		activity->getMainModuleID (moduleID);
		String fileName = String ("lib").append (moduleID).append (".so");
		module = ::dlopen (MutableCString (fileName).str (), RTLD_NOLOAD); // get handle of already loaded module
		if(module)
			::dlclose (module);
	}
	ASSERT (module)
	return module;
}

//************************************************************************************************
// DynamicLibraryImage
//************************************************************************************************

DynamicLibraryImage::DynamicLibraryImage (StringRef moduleId, ModuleRef nativeRef, bool isLoaded)
: ExecutableImage (nativeRef, isLoaded)
{
	String packageId = moduleId;
	if(packageId.isEmpty ())
	{
		if(void* addr = ::dlsym (nativeRef, nativeRef == gMainModuleRef ? "CCLAndroidMain" : "CCLModuleMain"))
		{
			Dl_info info;
			if(::dladdr (addr, &info))
			{
				Url moduleUrl (info.dli_fname);
				moduleUrl.getName (packageId, false);
				if(packageId.startsWith ("lib"))
					packageId = packageId.subString (3);
			}
		}
	}

	if(!packageId.isEmpty ())
	{
		metaInfo = NEW Attributes ();
		metaInfo->setAttribute (Meta::kPackageID, packageId);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DynamicLibraryImage::~DynamicLibraryImage ()
{
	if(isLoaded && nativeRef)
		::dlclose (nativeRef);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DynamicLibraryImage::getPath (IUrl& path) const
{
	ASSERT (nativeRef != 0)

	// todo: ?
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* CCL_API DynamicLibraryImage::getFunctionPointer (CStringPtr name) const
{
	ASSERT (nativeRef != 0)
	return ::dlsym (nativeRef, name);
}

//************************************************************************************************
// MainModuleImage
//************************************************************************************************

MainModuleImage::MainModuleImage (ModuleRef nativeRef, bool isLoaded)
: DynamicLibraryImage (String::kEmpty, nativeRef, isLoaded)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MainModuleImage::getPath (IUrl& path) const
{
	IFrameworkActivity* activity = AndroidSystemInformation::getInstance ().getNativeActivity ();
	if(nativeRef == 0 || activity == 0)
		return false;
	
	ASSERT (nativeRef == gMainModuleRef)

	JniAccessor jni;
	LocalStringRef localString (jni, Context.getPackageResourcePath (activity->getJObject ()));
	JniCStringChars pathString (jni, localString);
	if(pathString)
	{
		path.fromPOSIXPath (pathString, IUrl::kFolder);
		return true;
	}
	return false;
}
