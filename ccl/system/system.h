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
// Filename    : ccl/system/system.h
// Description : System class
//
//************************************************************************************************

#ifndef _ccl_system_h
#define _ccl_system_h

#include "ccl/base/singleton.h"

#include "ccl/system/threading/threadlocks.h"

#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/iexecutable.h"
#include "ccl/public/collections/linkedlist.h"

struct tm;

namespace CCL {

class Url;
class Attributes;

//************************************************************************************************
// CRTTypeConverter
//************************************************************************************************

namespace CRTTypeConverter
{
	void tmToDateTime (DateTime& dateTime, const tm& t);
	void tmFromDateTime (tm& t, const DateTime& dateTime);
}

//************************************************************************************************
// SystemInformation
//************************************************************************************************

class SystemInformation: public Object,
						 public ISystemInformation,
						 public ExternalSingleton<SystemInformation>
{
public:
	DECLARE_CLASS (SystemInformation, Object)
	DECLARE_METHOD_NAMES (SystemInformation)

	SystemInformation ();
	~SystemInformation ();

	// ISystemInformation
	void CCL_API setApplicationName (StringRef companyName, StringRef productName, int productVersion) override;
	tbool CCL_API setLocation (System::FolderType type, UrlRef url) override;
	tbool CCL_API getLocation (IUrl& url, System::FolderType type) const override;
	tbool CCL_API resolveLocation (IUrl& resolved, UrlRef url) const override;
	void CCL_API getLocalTime (DateTime& dateTime) const override;
	void CCL_API convertLocalTimeToUTC (DateTime& utc, const DateTime& localTime) const override;
	void CCL_API convertUTCToLocalTime (DateTime& localTime, const DateTime& utc) const override;
	void CCL_API convertUnixTimeToUTC (DateTime& utc, int64 unixTime) const override;
	int64 CCL_API convertUTCToUnixTime (const DateTime& utc) const override;	
	int64 CCL_API getUnixTime () const override;	
	int64 CCL_API getSecureComputerTime () const override;
	void CCL_API getComputerName (String& name, int flags = 0) const override;
	void CCL_API getUserName (String& name, int flags = 0) const override;    
	int CCL_API getNumberOfCPUs () const override;
	int CCL_API getNumberOfCores () const override;
	void CCL_API getMemoryInfo (System::MemoryInfo& memoryInfo) const override;
	void CCL_API getComputerInfo (IAttributeList& attributes, int flags = 0) const override;
	tbool CCL_API isProcessSandboxed () const override;
	IAsyncOperation* CCL_API searchApplications (StringRef filter) const override;	
	void CCL_API terminate () override;

	CLASS_INTERFACE (ISystemInformation, Object)

protected:
	String appCompanyName;
	String appProductName;
	int appProductVersion;
	Url& contentLocation;
	Url& deploymentLocation;

	enum Flags
	{
		kIsVersionSpecific = 1<<0,
		kIsPlatformSpecific = 1<<1
	};

	bool getCompanySpecificFolder (IUrl& url, System::FolderType baseType) const;
	bool getAppSpecificFolder (IUrl& url, System::FolderType baseType, int flags = 0) const;

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;

	// to be implemented by platform subclass:
	virtual bool getNativeLocation (IUrl& url, System::FolderType type) const;
};

//************************************************************************************************
// ExecutableImage
//************************************************************************************************

class ExecutableImage: public Unknown,
					   public IExecutableImage
{
public:
	ExecutableImage (ModuleRef nativeRef, bool isLoaded);
	~ExecutableImage ();

	// IExecutableImage
	tbool CCL_API getPath (IUrl& path) const override;
	tbool CCL_API getIdentifier (String& id) const override;
	ModuleRef CCL_API getNativeReference () const override;
	void* CCL_API getFunctionPointer (CStringPtr name) const override;
	const IAttributeList* CCL_API getMetaInfo () const override;
	tbool CCL_API getBinaryPath (IUrl& path) const override;

	CLASS_INTERFACE (IExecutableImage, Unknown)

protected:
	ModuleRef nativeRef;
	bool isLoaded;
	mutable Attributes* metaInfo;
};

//************************************************************************************************
// ExecutableImageList
//************************************************************************************************

class ExecutableImageList: public LinkedList<IExecutableImage*>
{
public:
	ExecutableImageList (bool shared = false);
	ExecutableImageList (const ExecutableImageList& other, bool shared = false);
	~ExecutableImageList ();

	typedef ListIterator<IExecutableImage*> Iterator;

protected:
	bool shared;
};

//************************************************************************************************
// ExecutableLoader
//************************************************************************************************

class ExecutableLoader: public Object,
						public IExecutableLoader,
						public ExternalSingleton<ExecutableLoader>
{
public:
	ExecutableLoader ();
	~ExecutableLoader ();

	void addImage (IExecutableImage* image);
	void removeImage (IExecutableImage* image);

	// IExecutableLoader
	const IExecutableImage& CCL_API getMainImage () override;
	tresult CCL_API loadImage (IExecutableImage*& image, UrlRef path) override;
	IExecutableImage* CCL_API createImage (ModuleRef module) override;
	IExecutableIterator* CCL_API createIterator () override;
	void CCL_API addNativeImage (ModuleRef module) override;
	void CCL_API removeNativeImage (ModuleRef module) override;
	tresult CCL_API execute (Threading::ProcessID& processId, UrlRef path, ArgsRef args, 
							 int flags = 0, IUnknown* context = nullptr) override;
	tresult CCL_API relaunch (ArgsRef args) override;
	tresult CCL_API terminate (Threading::ProcessID processId) override;
	tresult CCL_API getExecutablePath (IUrl& path, Threading::ProcessID processId) override;
	tbool CCL_API isProcessRunning (UrlRef executableFile) override;
	tresult CCL_API getModuleInfo (IAttributeList& attributes, UrlRef path) override;

	CLASS_INTERFACE (IExecutableLoader, Unknown)

protected:
	IExecutableImage* mainImage;
	Threading::NativeCriticalSection lock;
	ExecutableImageList imageList;
	
	#if DEBUG
	void dump ();
	#endif
};

} // namespace CCL

#endif // _ccl_system_h
