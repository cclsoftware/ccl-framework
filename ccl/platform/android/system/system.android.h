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
// Filename    : ccl/platform/android/system/system.android.h
// Description : Android system class
//
//************************************************************************************************

#ifndef _ccl_system_android_h
#define _ccl_system_android_h

#include "ccl/system/system.h"

#include "ccl/platform/android/cclandroidjni.h"

#include "ccl/platform/android/interfaces/iandroidsystem.h"

struct AAssetManager;
struct AConfiguration;

namespace CCL {

//************************************************************************************************
// AndroidSystemInformation
//************************************************************************************************

class AndroidSystemInformation: public SystemInformation,
								public Android::IAndroidSystem
{
public:
	static int64 getPhysicalMemoryAvailable ();

	static AndroidSystemInformation& getInstance ();

	AndroidSystemInformation ();
	~AndroidSystemInformation ();

	AAssetManager* getAssetManager () const;
	jobject getJavaAssetManager () const;

	AConfiguration* getConfiguration () const;
	StringRef getAppProductFolderName () const;

	// IAndroidSystem
	void CCL_API setNativeActivity (Android::IFrameworkActivity* activity) override;
	Android::IFrameworkActivity* CCL_API getNativeActivity () const override;
	int CCL_API callAndroidMain (tbool startup) override;
	void CCL_API onConfigurationChanged () override;

	// SystemInformation
	bool getNativeLocation (IUrl& path, System::FolderType type) const override;
	void CCL_API getLocalTime (DateTime& dateTime) const override;
	void CCL_API convertLocalTimeToUTC (DateTime& utc, const DateTime& localTime) const override;
	void CCL_API convertUTCToLocalTime (DateTime& localTime, const DateTime& utc) const override;
	void CCL_API convertUnixTimeToUTC (DateTime& utc, int64 unixTime) const override;
	int64 CCL_API convertUTCToUnixTime (const DateTime& utc) const override;
	void CCL_API getComputerName (String& name, int flags = 0) const override;
	void CCL_API getUserName (String& name, int flags = 0) const override;
	void CCL_API getMemoryInfo (System::MemoryInfo& memoryInfo) const override;
	void CCL_API getComputerInfo (IAttributeList& attributes, int flags = 0) const override;
	tbool CCL_API isProcessSandboxed () const override;
	
	CLASS_INTERFACE (IAndroidSystem, SystemInformation)

private:
	Android::IFrameworkActivity* activity;
	Android::JniObject javaAssetManager;
	AAssetManager* assetManager;
	AConfiguration* configuration;

	jobject getSystemService (StringID serviceID) const;
};

//************************************************************************************************
// AndroidExecutableLoader
//************************************************************************************************

class AndroidExecutableLoader: public ExecutableLoader
{
public:
	// ExecutableLoader
	tresult CCL_API loadImage (IExecutableImage*& image, UrlRef path) override;
	IExecutableImage* CCL_API createImage (ModuleRef module) override;
	tresult CCL_API relaunch (ArgsRef args) override;
    tresult CCL_API getExecutablePath (IUrl& path, Threading::ProcessID processId) override;

	static ModuleRef getMainModuleRef ();
};

//************************************************************************************************
// DynamicLibraryImage
//************************************************************************************************

class DynamicLibraryImage: public ExecutableImage
{
public:
	DynamicLibraryImage (StringRef moduleId, ModuleRef nativeRef, bool isLoaded);
	~DynamicLibraryImage ();

	// ExecutableImage
	tbool CCL_API getPath (IUrl& path) const override;
	void* CCL_API getFunctionPointer (CStringPtr name) const override;
};

//************************************************************************************************
// MainModuleImage
//************************************************************************************************

class MainModuleImage: public DynamicLibraryImage
{
public:
	MainModuleImage (ModuleRef nativeRef, bool isLoaded);
	
	// ExecutableImage
	tbool CCL_API getPath (IUrl& path) const override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline 
//////////////////////////////////////////////////////////////////////////////////////////////////

inline AAssetManager* AndroidSystemInformation::getAssetManager () const
{ return assetManager; }

inline jobject AndroidSystemInformation::getJavaAssetManager () const
{ return javaAssetManager; }

inline AConfiguration* AndroidSystemInformation::getConfiguration () const
{ return configuration; }

} // namespace CCL

#endif // _ccl_system_android_h
